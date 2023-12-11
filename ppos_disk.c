#include "ppos_disk.h"
#include "disk.h"

static disk_t ppos_disk;  // Alteração: Não precisa mais ser um ponteiro
static request_t *request_queue = NULL;
static task_t disk_manager_task;  // Alteração: Não precisa mais ser um ponteiro
static semaphore_t disk_semaphore;  // Semáforo para acesso exclusivo ao disco

void diskDriverBody(void *args) {
    while (1) {
        // Obtenha o semáforo de acesso ao disco
        sem_down(&disk_semaphore);

        // Se o disco gerou um sinal
        if (disk_cmd(DISK_CMD_STATUS, 0, 0) == DISK_STATUS_IDLE && request_queue != NULL) {
            // Atenda a próxima solicitação na fila (FCFS)
            request_t *current_request = request_queue;
            request_queue = request_queue->next;

            // Libere o semáforo de acesso ao disco antes de realizar a E/S
            sem_up(&disk_semaphore);

            // Solicite a operação de E/S ao disco
            if (current_request->operation == 0) {
                disk_cmd(DISK_CMD_READ, current_request->block, current_request->buffer);
            } else {
                disk_cmd(DISK_CMD_WRITE, current_request->block, current_request->buffer);
            }

            // Obtenha novamente o semáforo de acesso ao disco após a E/S
            sem_down(&disk_semaphore);

            // Acorda a tarefa cujo pedido foi atendido
            task_resume(current_request->task);

            // Libere a memória alocada para a solicitação
            free(current_request);
        }

        // Libere o semáforo de acesso ao disco

        // Suspenda a tarefa corrente (retorna ao dispatcher)
        task_suspend(taskExec, &request_queue);
    }
}

int disk_mgr_init(int *numBlocks, int *blockSize) {
    // Inicialize o gerenciador de disco (pode incluir inicializações de semáforos, etc.)
    sem_create(&disk_semaphore, 1);  // Semáforo para acesso exclusivo ao disco

    // Inicialize as variáveis numBlocks e blockSize
    *numBlocks =  256;
    *blockSize =  64;

    // Crie a tarefa do gerente de disco
    task_create(&disk_manager_task, diskDriverBody, NULL);  // Alteração: task_create agora usa &disk_manager_task

    return 0; // Sucesso
}

int disk_block_read(int block, void *buffer) {
    // Obtenha o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Inclua o pedido na fila_disco
    request_t *new_request = malloc(sizeof(request_t));
    new_request->block = block;
    new_request->buffer = buffer;
    new_request->operation = 0;  // Operação de leitura
    new_request->task = &taskExec;  // Alteração: taskExec agora é referenciado como &taskExec
    new_request->next = NULL;

    // Adicione o pedido à fila de espera
    if (request_queue == NULL) {
        request_queue = new_request;
    } else {
        request_t *last = request_queue;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = new_request;
    }

    // Se o gerente de disco está dormindo, acorde-o
    task_resume(&disk_manager_task);  // Alteração: task_manager_task agora é referenciado como &disk_manager_task

    // Libere o semáforo de acesso ao disco
    sem_up(&disk_semaphore);

    // Suspenda a tarefa corrente (retorna ao dispatcher)
    task_suspend(&taskExec, &request_queue);  // Alteração: taskExec agora é referenciado como &taskExec
}

int disk_block_write(int block, void *buffer) {
    // Obtenha o semáforo de acesso ao disco
    sem_down(&disk_semaphore);

    // Inclua o pedido na fila_disco
    request_t *new_request = malloc(sizeof(request_t));
    new_request->block = block;
    new_request->buffer = buffer;
    new_request->operation = 1;  // Operação de escrita
    new_request->task = &taskExec;  // Alteração: taskExec agora é referenciado como &taskExec
    new_request->next = NULL;

    // Adicione o pedido à fila de espera
    if (request_queue == NULL) {
        request_queue = new_request;
    } else {
        request_t *last = request_queue;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = new_request;
    }

    // Se o gerente de disco está dormindo, acorde-o
    task_resume(&disk_manager_task);  // Alteração: task_manager_task agora é referenciado como &disk_manager_task
    // Libere o semáforo de acesso ao disco
    sem_up(&disk_semaphore);

    // Suspenda a tarefa corrente (retorna ao dispatcher)
    task_suspend(&taskExec, &request_queue);  // Alteração: taskExec agora é referenciado como &taskExec
}