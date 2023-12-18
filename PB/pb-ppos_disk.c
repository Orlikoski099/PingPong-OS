// AGRADECIMENTOS À SAULO CENTENARO PELA AJUDA NA MELHORA DE ALGUMAS LÓGICAS E SOLUÇÃO DE ALGUNS PROBLEMAS

#include "ppos_disk.h"
#include "ppos-core-globals.h"
#include "disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <time.h>

#define MAX_WAIT_SECONDS 10

struct sigaction usrsig;
disk_t *disk;

void push_waiting_task(diskrequest_t *task)
{
    mutex_lock(&disk->mutex_queue);

    // Se a lista de espera estiver vazia, insere a tarefa como a primeira da lista
    if (disk->awaiting_tasks != NULL)
    {
        // Encontra o final da lista de espera
        diskrequest_t *last_task = disk->awaiting_tasks;

        while (last_task->next != NULL)
        {
            last_task = last_task->next;
        }
        // Adiciona a tarefa ao final da lista
        last_task->next = task;
    }
    else
        disk->awaiting_tasks = task;

    mutex_unlock(&disk->mutex_queue);
}

diskrequest_t *pop_waiting_task()
{
    mutex_lock(&disk->mutex_queue); // Bloqueia o mutex para operações seguras na lista

    diskrequest_t *waiting_task = disk->awaiting_tasks; // Obtém a head da lista de espera

    if (waiting_task == NULL){
        mutex_unlock(&disk->mutex_queue); // Desbloqueia o mutex após as operações na lista
        return NULL;
    }

    disk->awaiting_tasks = waiting_task->next; // Atualiza a lista removendo o primeiro elemento
    waiting_task->next = NULL;               // Desconecta o elemento removido da lista

    mutex_unlock(&disk->mutex_queue); // Desbloqueia o mutex após as operações na lista

    return waiting_task;
}

diskrequest_t *create_waiting_task(task_t *task, int command, int block, void *buffer)
{
    // Verifica se task é diferente de NULL
    if (task == NULL)
    {
        fprintf(stderr, "Error: NULL task pointer in create_waiting_task\n");
        exit(EXIT_FAILURE); // Ou lida com o erro de uma maneira apropriada ao seu sistema
    }

    // Aloca dinamicamente a estrutura diskrequest_t
    diskrequest_t *waiting_task = malloc(sizeof(diskrequest_t));
    waiting_task->next = NULL;
    waiting_task->command = command;
    waiting_task->task = task;
    waiting_task->buffer = buffer;
    waiting_task->block = block;

    return waiting_task;
}

int disk_exec_cmd(diskrequest_t *pedido)
{
    mutex_lock(&disk->mutex_disk_access);

    // Define a tarefa em processamento e executa o comando
    disk->processing_task = pedido;

    int result = disk_cmd(pedido->command, pedido->block, pedido->buffer);

    if (result < 0)
    {
        printf("T%d - Erro na escrita do disco\n", task_id());
        exit(1);
    }

    // Suspende a tarefa
    pedido->task->state = PPOS_TASK_STATE_SUSPENDED;

    task_suspend(pedido->task, &sleepQueue);

    task_yield();

    // Obtém o tempo atual antes de entrar no loop
    time_t start_time = time(NULL);

    // Aguarda a conclusão do comando do disco com um mecanismo de timeout
    while (disk_cmd(DISK_CMD_STATUS, 0, 0) != DISK_STATUS_IDLE)
    {
        // Verifica se o tempo de espera máximo foi atingido (difftime pertence a time.h)
        if (difftime(time(NULL), start_time) > MAX_WAIT_SECONDS)
        {
            printf("T%d - Timeout - esperando o disco ficar idle\n", task_id());

            break; // ou retorne um código de erro, lance uma exceção, etc.
        }
    }

    // Libera o bloqueio do mutex após a conclusão do comando do disco
    mutex_unlock(&disk->mutex_disk_access);

    return 0;
}

void tratador(int signum)
{
    task_resume(disk->processing_task->task);
}

void scheduler_FCFS(void *arg)
{
    int current_block = 0;
    int total_blocks_traveled = 0;

    clock_t end_time;
    clock_t start_time = clock(); // Anota o tempo de início da função

    // Loop enquanto tiver tarefas na fila de espera/tarefas prontas
    while (readyQueue != NULL || sleepQueue != NULL)
    {
        // Obtém a próxima tarefa da fila de espera
        diskrequest_t *next_task = pop_waiting_task();

        if (next_task != NULL)
        {
            // Calcula a distância entre o bloco atual e o bloco da próxima tarefa
            int distance = next_task->block - current_block;
            if (distance < 0)
                distance = current_block - next_task->block;

            // Atualiza o bloco atual
            current_block = next_task->block;

            // Atualiza o total de blocos percorridos
            total_blocks_traveled += distance;

            // Retoma a execução da tarefa associada à próxima tarefa
            task_resume(next_task->task);
        }
        else
        {
            // Se não há tarefas na fila de espera, suspende a tarefa corrente
            taskExec->state = PPOS_TASK_STATE_SUSPENDED;
            task_suspend(taskExec, &sleepQueue);
            task_yield();
        }
    }

    end_time = clock();

    // Imprime as estatísticas no final da execução
    printf("[FCFS]Total de blocos percorridos: %d\nTempo decorrido: %.2f segundos\n", total_blocks_traveled, (double)(end_time - start_time) / CLOCKS_PER_SEC);
}

void scheduler_CSCAN(void *arg)
{
    int disk_size = disk_cmd(DISK_CMD_DISKSIZE, 0, NULL);
    int current_block = 0;
    int total_blocks_traveled = 0;
    int distance = 0;

    clock_t end_time;
    clock_t start_time = clock(); // Guarda o tempo atual

    // Loop enquanto há tarefas na fila de espera ou tarefas prontas
    while (readyQueue != NULL || sleepQueue != NULL)
    {
        if (current_block+1 == disk_size)
        {
            current_block = 0;
        }
        diskrequest_t *next_task = pop_waiting_task();
        if (next_task != NULL)
        {

            distance = next_task->block >= current_block ? next_task->block - current_block : current_block - next_task->block;

            if (next_task->block >= current_block)
            {
                task_resume(next_task->task);
            }
            total_blocks_traveled += distance;
            current_block = next_task->block;
            next_task = next_task->next;
        }
        else
        {
            taskExec->state = PPOS_TASK_STATE_SUSPENDED;
            task_suspend(taskExec, &sleepQueue);
            task_yield();
        }
    }

    end_time = clock();

    // Imprime as estatísticas no final da execução
    printf("[CSCAN]Total de blocos percorridos: %d \nTempo decorrido: %.2f segundos\n", total_blocks_traveled, (double)(end_time - start_time) / CLOCKS_PER_SEC);
}

void scheduler_SSTF(void *arg)
{
    int current_block = 0;          // Bloco atual do cabeçote de leitura/gravação
    int total_blocks_traveled = 0;  // Total de blocos percorridos
    int shortest_distance = 0;      // Menor distância entre blocos

    clock_t end_time;
    clock_t start_time = clock();   // Tempo de início do escalonamento

    // Obtém o tamanho total do disco
    int disk_size = disk_cmd(DISK_CMD_DISKSIZE, 0, NULL);

    while (readyQueue != NULL || sleepQueue != NULL)
    {
        shortest_distance = INT_MAX; // Reinicia a menor distância para cada iteração
        diskrequest_t *selected_task = NULL;

        diskrequest_t *current_task = disk->awaiting_tasks;

        while (current_task != NULL)
        {
            // Calcula a distância entre o bloco atual e o bloco da tarefa
            int task_distance = (current_task->block >= current_block) ?
                                (current_task->block - current_block) :
                                (current_block - current_task->block);
            // Verifica se a tarefa está dentro dos limites do disco
            if (current_task->block >= 0 && current_task->block < disk_size)
            {
                // Atualiza a tarefa selecionada se a distância for menor
                if (task_distance < shortest_distance)
                {
                    shortest_distance = task_distance;
                    selected_task = current_task;
                }
            }

            current_task = current_task->next;
        }

        if (selected_task != NULL)
        {
            // Calcula a distância percorrida pela cabeça de leitura/gravação
            int total_blocks_traveled = (selected_task->block >= current_block) ?
                                  (selected_task->block - current_block) :
                                  (current_block - selected_task->block);

            total_blocks_traveled += total_blocks_traveled; // Atualiza o total de blocos percorridos
            current_block = selected_task->block;     // Atualiza o bloco atual

            task_resume(selected_task->task);         // Retoma a tarefa selecionada
            pop_waiting_task(selected_task);          // Remove a tarefa da lista de espera
        }
        else
        {
            taskExec->state = PPOS_TASK_STATE_SUSPENDED;
            task_suspend(taskExec, &sleepQueue);
            task_yield();
        }
    }

    end_time = clock(); // Tempo de término do escalonamento

    printf("[SSTF]Total de blocos percorridos: %d\nTempo decorrido: %.f \n", total_blocks_traveled, (double)((end_time - start_time) / 10000));
}

int disk_mgr_init(int *numBlocks, int *blockSize)
{
    // Aloca espaço para a estrutura de disco
    disk = malloc(sizeof(disk_t));
    disk->awaiting_tasks = NULL;

    // Configura o tratador de sinal
    usrsig.sa_handler = tratador;
    sigemptyset(&usrsig.sa_mask);
    usrsig.sa_flags = 0;

    // Registra o tratador de sinal e verifica por erros
    if (sigaction(SIGUSR1, &usrsig, 0) < 0)
    {
        perror("Erro ao registrar tratador de sinal em sigaction");
        exit(1);
    }
    // Cria o mutex de acesso ao disco e verifica por erros
    if (mutex_create(&disk->mutex_disk_access))
    {
        perror("Erro ao criar mutex de acesso ao disco em mutex_create");
        exit(1);
    }

    // Cria o mutex da fila de espera e verifica por erros
    if (mutex_create(&disk->mutex_queue))
    {
        perror("Erro ao criar mutex de fila em mutex_create");
        exit(1);
    }

    // Inicializa o disco e verifica por erros
    if (disk_cmd(DISK_CMD_INIT, 0, 0) < 0)
    {
        perror("Erro ao inicializar disco em disk_cmd");
        exit(1);
    }
    
    //Altere entre: scheduler_CSCAN | scheduler_FCFS | scheduler_SSTF
    task_create(&disk->scheduler, scheduler_CSCAN, "");

    return 0;
}

int initiate_disk_block_operation(int command, int block, void *buffer)
{
    diskrequest_t *request = create_waiting_task(taskExec, command, block, buffer);

    push_waiting_task(request);

    request->task->state = PPOS_TASK_STATE_SUSPENDED;
    task_suspend(request->task, &sleepQueue);
    task_yield();

    disk_exec_cmd(request);

    return 0;
}

int disk_block_read(int block, void *buffer)
{
    return initiate_disk_block_operation(DISK_CMD_READ, block, buffer);
}

int disk_block_write(int block, void *buffer)
{
    return initiate_disk_block_operation(DISK_CMD_WRITE, block, buffer);
}