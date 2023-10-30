#include "ppos.h"
#include "ppos-core-globals.h"

// ****************************************************************************
// Coloque aqui as suas modificações, p.ex. includes, defines variáveis, 
// estruturas e funções
#include "sys/time.h"
#include <signal.h>
#include "math.h"

#define DEFAULT_INIT_TASK_TIME 99999

int quantum = 20;
unsigned int systemTime = 0; // Contador de tempo do sistema
unsigned int preemption; // Variável para controlar a preempção
task_t *dispatcher; // Tarefa do despachante
task_t *taskMain;// Tarefa principal (main)
ready_queue_t *readyQueue;
int systemActive;

//////////
void main_task(void *arg) {
    // Coloque o código da tarefa principal aqui
    task_t *self = (task_t *)arg; // Obtém um ponteiro para a tarefa principal

    // Após a conclusão, você pode encerrar a tarefa principal
    self->state = 'e'; // Defina o estado da tarefa principal como 'terminada'
    
    // Retorne se a tarefa principal terminar
    task_exit(0);
}
/////////////



void init_timer()
{
    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 1000;  // 1 ms em microssegundos
    timer.it_interval = timer.it_value;  // Configura interrupções periódicas

    // Configura o temporizador
    setitimer(ITIMER_REAL, &timer, 0);
}

void timer_handler(int signum) {
    systemTime++;  // Incrementa a contagem de ticks a cada interrupção

    // Verifica se a preempção está habilitada (preemption == 1)
    if (preemption == 1) {
        if (taskExec != NULL && taskExec->isSystemTask == 0) { // Verifique se é uma tarefa de usuário
            taskExec->quantum--; // Decrementa o quantum da tarefa atual
            if (taskExec->quantum == 0) {
                taskExec->state = 'r'; // Coloca a tarefa de volta na fila de prontas
                task_switch(dispatcher); // Volta ao dispatcher
            }
        }

    
}
}

void dispatcher_body() {
    while (1) {
        task_t *nextTask = scheduler(); // Use a função scheduler para escolher a próxima tarefa

        if (nextTask != NULL) {
            if (nextTask->state != 'r') {
                // Se a tarefa não está em estado 'r', coloque-a na fila de prontas
                task_yield();
            }
            task_switch(nextTask); // Troque para a próxima tarefa
        }
    }
}

// Função para capturar e tratar o sinal SIGALRM
void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = timer_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("Erro ao configurar o tratador de sinais");
        exit(1);
    }
    preemption = 1; //habilita preempção
}



// Função para atualizar o tempo restante da tarefa
void update_remaining_time(task_t *task) {
    unsigned int elapsedTime = systemTime - task->lastActivation;
    task->remainingExecutionTime -= elapsedTime;
    task->lastActivation = systemTime;
}

void task_set_eet(task_t *task, int et) {
    if (task == NULL) {
        task = taskExec;
    }
    task->estimatedExecutionTime = et;
    task->remainingExecutionTime = et;
}

int task_get_eet(task_t *task) {
    if (task == NULL) {
        task = taskExec;
    }
    return task->estimatedExecutionTime;
}

int task_get_ret(task_t *task) {
    if (task == NULL) {
        task = taskExec;
    }
    return task->remainingExecutionTime;
}



task_t *scheduler() {
    systemTime++; // Atualize o tempo do sistema
    printf("Está na schedule\n");
  

    task_t *tempTask = readyQueue->head;
    task_t *highestPriorityTask = dispatcher; //cria um ponteiro de tarefa de maior priotridade

    if (readyQueue == NULL) {
        if (highestPriorityTask == NULL) {
            return NULL;
        }
        return highestPriorityTask;
    } //se a fila estiver vazia, retorna null
    


    task_t *nextTask = taskExec;
    int shortestRemainingTime = nextTask->remainingExecutionTime;


    // Verifique se o tempo de execução da tarefa em execução atingiu o quantum
    if (taskExec != NULL) {
        taskExec->running_time++; // Incrementa o tempo de execução
        if (taskExec->running_time >= quantum) {
            taskExec->state = 'r'; // Coloca a tarefa de volta na fila de prontas
        }
    }

    // Se a tarefa (em execução) não estiver suspensa, atualiza o tempo restante
    if (taskExec != NULL && taskExec->state != 's') {
        update_remaining_time(taskExec);
        if (taskExec->running_time >= quantum) {
            taskExec->state = 'r'; // retorna p/ a fila de prontas
        }
    }

    while (tempTask != NULL) {

        if (tempTask->priority < shortestRemainingTime) {
            highestPriorityTask = tempTask;
            shortestRemainingTime = tempTask->priority;
        }

        int remainingTime = task_get_ret(tempTask);

        if (remainingTime < shortestRemainingTime) {
            shortestRemainingTime = remainingTime;
            nextTask = tempTask;
            
        }

        // Verifica se o tempo de execução da tarefa atual atingiu o quantum
        if (tempTask->running_time >= quantum) {
            tempTask->state = 'r'; // Coloca a tarefa de volta na fila de prontas
        }

        tempTask = tempTask->next;
    }
    return nextTask;
}


void dispatcher_handler(int signum) {
    // Verifica se a tarefa atual é de sistema (dispatcher) antes de permitir preempção
    if (taskExec->isSystemTask == 1) {
        return;
    }
}


void ppos_init() {
    // Inicialize as estruturas de dados necessárias, como a fila de tarefas prontas

    readyQueue->head = NULL;
    readyQueue->tail = NULL;

    systemTime = 0;//Inicializa o tempo do sistema em 0

    setup_signal_handler();
    preemption = 1; //habilita preempcao

    // Crie a tarefa de sistema para o dispatcher com alta prioridade
    task_create(dispatcher, dispatcher_body, NULL);
    dispatcher->priority = 0; // Prioridade alta (tarefa de sistema)
    dispatcher->isSystemTask = 1; // Flag indicando que é tarefa de sistema

    task_create(taskMain, main_task, NULL);

    // Configure o tratador de sinais para SIGUSR1
    struct sigaction sa_usr1;
    sa_usr1.sa_handler = dispatcher_handler;
    sa_usr1.sa_flags = 0;
    sigemptyset(&sa_usr1.sa_mask);

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        perror("Erro ao configurar o tratador de sinais para SIGUSR1");
        exit(1);
    }

    dispatcher->priority = 0; // Prioridade alta para tarefa de sistema

    // Configura o temporizador
    init_timer();

    // Define o sistema para o estado ->em execução<-
    systemActive = 1;

     // define taskExec para a tarefa principal (main)
      taskExec = taskMain;

}

/////////
// ****************************************************************************


void before_ppos_init () {
    // put your customization here
    setup_signal_handler();
    init_timer();
#ifdef DEBUG
    printf("\ninit - BEFORE");
#endif
}

void after_task_create(task_t *task) {
    task_set_eet(task, DEFAULT_INIT_TASK_TIME);
    task->creationTime = systemTime;
    task->lastActivation = systime();
    task->running_time = 0;
    task->state = PPOS_TASK_STATE_READY; // Defina o estado como READY
    taskExec = task;
#ifdef DEBUG
    printf("\ntask_create - AFTER - [%d]", task->id);
#endif
}

void after_task_exit() {
    // put your customization here
    unsigned int currentTime = systime();
    unsigned int executionTime = currentTime - taskExec->creationTime;
    unsigned int processorTime = taskExec->running_time;
    int numActivations = taskExec->activations;
    taskExec->state = PPOS_TASK_STATE_TERMINATED; // Defina o estado como TERMINATED
    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", taskExec->id, executionTime, processorTime, numActivations);

#ifdef DEBUG
    printf("\ntask_exit - AFTER- [%d]", taskExec->id);
#endif
}

void before_task_switch(task_t *task) {
    // put your customization here
    if (taskExec != NULL && taskExec->state != 's') {
        taskExec->running_time += systime() - taskExec->lastActivation;
        taskExec->state = PPOS_TASK_STATE_READY; // Atualize o estado para READY
    }
#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
#endif
}

void before_task_yield() {
    // put your customization here
    if (taskExec != NULL && taskExec->state != 's') {
        taskExec->running_time += systime() - taskExec->lastActivation;
        taskExec->state = PPOS_TASK_STATE_READY; // Atualize o estado para READY
    }
#ifdef DEBUG
    printf("\ntask_yield - BEFORE - [%d]", taskExec->id);
#endif
}

void before_task_suspend( task_t *task ) {
    // put your customization here
    if (taskExec->state != 's') {
        taskExec->running_time += systime() - taskExec->lastActivation;
    }
#ifdef DEBUG
    printf("\ntask_suspend - BEFORE - [%d]", task->id);
#endif
}

void after_ppos_init () {
    // put your customization here
#ifdef DEBUG
    printf("\ninit - AFTER");
#endif
}

void before_task_create (task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_create - BEFORE - [%d]", task->id);
#endif
}

void before_task_exit () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_exit - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_switch ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]", taskExec->id, task->id);
#endif
}


void after_task_yield () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - AFTER - [%d]", taskExec->id);
#endif
}

void after_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - AFTER - [%d]", task->id);
#endif
}

void before_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - BEFORE - [%d]", task->id);
#endif
}

void after_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - AFTER - [%d]", task->id);
#endif
}

void before_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - AFTER - [%d]", taskExec->id);
#endif
}

int before_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}


int before_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

