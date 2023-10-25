#include "ppos.h"
#include "ppos-core-globals.h"

// ****************************************************************************
// Coloque aqui as suas modificações, p.ex. includes, defines variáveis,
// estruturas e funções

#include "sys/time.h"
#include "signal.h"

int TASK_AGING = -1;

#define quantum 20

struct sigaction action;

struct itimerval timer;

unsigned int lastQuantum = 0;

void setTimer()
{
    timer.it_value.tv_usec = 1000;    // primeiro disparo, em micro-segundos
    timer.it_interval.tv_usec = 1000; // disparos subsequentes, em micro-segundos
    timer.it_value.tv_sec = 0;
    timer.it_interval.tv_sec = 0;

    if (setitimer(ITIMER_REAL, &timer, 0) < 0) {
        perror("Erro em setitimer: ");
        exit(1);
    }
}

void sig_Handler(int sinal) {
    systemTime++;

    if (systemTime - lastQuantum < quantum) { return; } // early return para quando a tarefa estiver dentro do quantum

    if (taskExec->state == 'X') {
        taskExec->state = 'S';
    }
    task_t *nextTask = scheduler();
    if (nextTask != NULL) {
        if (nextTask->state == 'S') {
            nextTask->state = 'X';
        }
        task_yield();
    }
    lastQuantum = systime();
}

void set_handler() {
    action.sa_handler = sig_Handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, 0) < 0) {
        perror("Erro em sigaction: ");
        exit(1);
    }
}

int task_getprio(task_t *task) {
    if (task == NULL) {
        task = taskExec;
    }
    return task->staticPriority;
}

void task_setprio(task_t *task, int prio) {
    if (prio < -20) {
        prio = -20;
    } else if (prio > 20) {
        prio = 20;
    }

    if (task == NULL) {
        task = taskExec;
    }
    task->staticPriority = prio;
    task->dynamicPriority = prio;
}

int task_getdprio(task_t *task)  {
    if (task == NULL) {
        task = taskExec;
    }
    return task->dynamicPriority;
}

void task_agedprio(task_t *task, int aging) {
    if (task == NULL) {
        task = taskExec;
    }
    if (aging == 0) {
        task->dynamicPriority = task_getprio(task);
    }

    if (task->dynamicPriority + aging < -20) {
        task->dynamicPriority = -20;
        return;
    } else if (task->dynamicPriority + aging > 20) {
        task->dynamicPriority = 20;
        return;
    }

    task->dynamicPriority += aging;
}

task_t *scheduler() {
    if (readyQueue == NULL) {
        return NULL;
    }

    task_t *prioTask = readyQueue;
    task_t *it = readyQueue;

    do {
        if (task_getdprio(it) <= task_getdprio(prioTask) && it != prioTask) {
            task_agedprio(prioTask, TASK_AGING);
            prioTask = it;
        } else if (it != prioTask) {
            task_agedprio(it, TASK_AGING);
        }
        it = it->next;
    } while (it != NULL && it->id != 0 && it != readyQueue);

    task_agedprio(prioTask, 0);

    return prioTask;
}

void printContab(task_t *task) {
    printf("Task %d exit: execution time %d ms, processorTime %d ms, %d activations\n", task->id, systemTime - task->startTime, task->processorTime, task->activations);
}

// ****************************************************************************

void before_ppos_init()
{
    setTimer();
    set_handler();
}

void before_task_create(task_t *task) {
    task->processorTime = 0;
    task->activations = 0;
    task->startTime = systime();
}

void before_task_exit() {
    printContab(taskExec);
}

void after_ppos_init() {
}

void before_task_switch(task_t *task)
{
    task->state = 'X';
    task->lastActivation = systime();
    task->activations++;
    if(task == taskMain && systime() > 0 && countTasks == 1) { // processos já terminaram e pode imprimir a contabilização da main
        printContab(taskExec);
    }
}

void before_task_yield() {
    taskExec->processorTime += (systime() - taskExec->lastActivation);
}

void after_task_create(task_t *task) {
}

void after_task_exit() {
}

void after_task_switch(task_t *task) {
}

void after_task_yield() {
}

void before_task_suspend(task_t *task) {
}

void after_task_suspend(task_t *task) {
}

void before_task_resume(task_t *task) {
}

void after_task_resume(task_t *task) {
}

void before_task_sleep() {
}

void after_task_sleep() {
}

int before_task_join(task_t *task) {
    return 0;
}

int after_task_join(task_t *task) {
    return 0;
}

int before_sem_create(semaphore_t *s, int value) {
    return 0;
}

int after_sem_create(semaphore_t *s, int value) {
    return 0;
}

int before_sem_down(semaphore_t *s) {
    return 0;
}

int after_sem_down(semaphore_t *s) {
    return 0;
}

int before_sem_up(semaphore_t *s) {
    return 0;
}

int after_sem_up(semaphore_t *s) {
    return 0;
}

int before_sem_destroy(semaphore_t *s) {
    return 0;
}

int after_sem_destroy(semaphore_t *s) {
    return 0;
}

int before_mutex_create(mutex_t *m) {
    return 0;
}

int after_mutex_create(mutex_t *m) {
    return 0;
}

int before_mutex_lock(mutex_t *m) {
    return 0;
}

int after_mutex_lock(mutex_t *m) {
    return 0;
}

int before_mutex_unlock(mutex_t *m) {
    return 0;
}

int after_mutex_unlock(mutex_t *m) {
    return 0;
}

int before_mutex_destroy(mutex_t *m) {
    return 0;
}

int after_mutex_destroy(mutex_t *m) {
    return 0;
}

int before_barrier_create(barrier_t *b, int N) {
    return 0;
}

int after_barrier_create(barrier_t *b, int N) {
    return 0;
}

int before_barrier_join(barrier_t *b) {
    return 0;
}

int after_barrier_join(barrier_t *b) {
    return 0;
}

int before_barrier_destroy(barrier_t *b) {
    return 0;
}

int after_barrier_destroy(barrier_t *b) {
    return 0;
}

int before_mqueue_create(mqueue_t *queue, int max, int size) {
    return 0;
}

int after_mqueue_create(mqueue_t *queue, int max, int size) {
    return 0;
}

int before_mqueue_send(mqueue_t *queue, void *msg) {
    return 0;
}

int after_mqueue_send(mqueue_t *queue, void *msg) {
    return 0;
}

int before_mqueue_recv(mqueue_t *queue, void *msg) {
    return 0;
}

int after_mqueue_recv(mqueue_t *queue, void *msg) {
    return 0;
}

int before_mqueue_destroy(mqueue_t *queue) {
    return 0;
}

int after_mqueue_destroy(mqueue_t *queue) {
    return 0;
}

int before_mqueue_msgs(mqueue_t *queue) {
    return 0;
}

int after_mqueue_msgs(mqueue_t *queue) {
    return 0;
}