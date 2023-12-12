#include "ppos_disk.h"
#include "ppos-core-globals.h"
#include "disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
// Se der algum problema de que a leitura dos blocos parece estar indo para o lado, troque o separador de linhas do disk.dat para LF
struct sigaction usrsig;
disk_t *disk;

void add_waiting_task(diskrequest_t *task)
{
    mutex_lock(&disk->queueMtx);

    if (disk->waitingTasks == NULL)
    {
        disk->waitingTasks = task;
        mutex_unlock(&disk->queueMtx);
        return;
    }

    diskrequest_t *lt = disk->waitingTasks;
    while (lt->next != NULL)
    {
        lt = lt->next;
    }

    lt->next = task;

    mutex_unlock(&disk->queueMtx);
}

diskrequest_t *pop_waiting_task()
{
    if (disk->waitingTasks == NULL)
    {
        return NULL;
    }
    diskrequest_t *wt = disk->waitingTasks;
    disk->waitingTasks = wt->next;
    wt->next = NULL;
    return wt;
}

diskrequest_t *create_waiting_task(task_t *task, int command, int block, void *buffer)
{
    diskrequest_t *wt = malloc(sizeof(diskrequest_t));
    wt->task = task;
    wt->command = command;
    wt->block = block;
    wt->buffer = buffer;
    wt->next = NULL;

    return wt;
}

int disk_execute_command(diskrequest_t *wt)
{
    mutex_lock(&disk->diskAccessMtx);

    disk->processingTask = wt;
    if (disk_cmd(wt->command, wt->block, wt->buffer) < 0)
    {
        printf("T%d - Erro escrevendo disco\n", task_id());
        exit(1);
    }

    wt->task->state = PPOS_TASK_STATE_SUSPENDED;
    task_suspend(wt->task, &sleepQueue);
    task_yield();

    while (disk_cmd(DISK_CMD_STATUS, 0, 0) != DISK_STATUS_IDLE)
    {
    }

    mutex_unlock(&disk->diskAccessMtx);

    return 0;
}

void tratador(int signum)
{
    task_resume(disk->processingTask->task);
}

void disk_scheduler_FCFS(void *arg)
{
    int blocks_traveled = 0;
    int currentblock = 0;

    clock_t start_time = clock();

    while (sleepQueue != NULL || readyQueue != NULL)
    {
        diskrequest_t *task = pop_waiting_task();
        if (task != NULL)
        {
            int distance = task->block - currentblock;
            if (distance < 0)
                distance = currentblock - task->block;

            blocks_traveled += distance;
            currentblock = task->block;
            task_resume(task->task);
        }
        else
        {
            taskExec->state = PPOS_TASK_STATE_SUSPENDED;
            task_suspend(taskExec, &sleepQueue);
            task_yield();
        }
    }

    clock_t end_time = clock();

    printf("Total de blocos percorridos: %d\nTempo decorrido: %.f \n", blocks_traveled, (double)((end_time - start_time) / 10000));
}

void disk_scheduler_CSCAN(void *arg)
{
    int currentblock = 0;
    int diskSize = disk_cmd(DISK_CMD_DISKSIZE, 0, NULL);
    int blocks_traveled = 0;
    int distance = 0;

    clock_t start_time = clock();

    while (sleepQueue != NULL || readyQueue != NULL)
    {
        if (currentblock == (diskSize - 1))
        {
            currentblock = 0;
        }
        diskrequest_t *task = pop_waiting_task();
        if (task != NULL)
        {
            if (task->block >= currentblock)
            {
                distance = task->block - currentblock;
                task_resume(task->task);
            }
            else
            {
                distance = currentblock - task->block;
            }
            blocks_traveled += distance;
            currentblock = task->block;
            task = task->next;
        }
        else
        {
            taskExec->state = PPOS_TASK_STATE_SUSPENDED;
            task_suspend(taskExec, &sleepQueue);
            task_yield();
        }
    }
    clock_t end_time = clock();
    printf("Total de blocos percorridos: %d \nTempo decorrido: %.f \n", blocks_traveled, (double)((end_time - start_time) / 10000));
}

void disk_scheduler_SSTF(void *arg)
{
    int currentblock = 0;
    int blocks_traveled = 0;
    int distance = 0;

    clock_t start_time = clock();

    while (sleepQueue != NULL || readyQueue != NULL)
    {
        int shortest_distance = 99999999;
        diskrequest_t *selected_task = NULL;

        diskrequest_t *task = disk->waitingTasks;

        while (task != NULL)
        {

            if (task->block >= currentblock)
                distance = task->block - currentblock;
            else
                distance = currentblock - task->block;

            if (distance < shortest_distance)
            {
                shortest_distance = distance;
                selected_task = task;
            }

            task = task->next;
        }

        if (selected_task != NULL)
        {
            if (selected_task->block >= currentblock)
                blocks_traveled += selected_task->block - currentblock;
            else
                blocks_traveled += currentblock - selected_task->block;
            currentblock = selected_task->block;
            task_resume(selected_task->task);
            pop_waiting_task(selected_task);
        }
        else
        {
            taskExec->state = PPOS_TASK_STATE_SUSPENDED;
            task_suspend(taskExec, &sleepQueue);
            task_yield();
        }
    }
    clock_t end_time = clock();

    printf("Total de blocos percorridos: %d\nTempo decorrido: %.f \n", blocks_traveled, (double)((end_time - start_time) / 10000));
}

int disk_mgr_init(int *numBlocks, int *blockSize)
{
    disk = malloc(sizeof(disk_t));
    disk->waitingTasks = NULL;

    usrsig.sa_handler = tratador;
    sigemptyset(&usrsig.sa_mask);
    usrsig.sa_flags = 0;
    if (sigaction(SIGUSR1, &usrsig, 0) < 0)
    {
        perror("Erro em sigaction");
        exit(1);
    }

    if (mutex_create(&disk->diskAccessMtx))
    {
        perror("Erro criando mutex de acesso ao disco");
        exit(1);
    }

    if (mutex_create(&disk->queueMtx))
    {
        perror("Erro criando mutex de fila");
        exit(1);
    }

    if (disk_cmd(DISK_CMD_INIT, 0, 0) < 0)
    {
        perror("Erro iniciando disco");
        exit(1);
    }

    //altere o scheduler para disk_scheduler_FCFS, disk_scheduler_SSTF ou disk_scheduler_CSCAN
    task_create(&disk->scheduler, disk_scheduler_CSCAN, "");
    return 0;
}

int disk_block_read(int block, void *buffer)
{
    diskrequest_t *wt = create_waiting_task(taskExec, DISK_CMD_READ, block, buffer);

    add_waiting_task(wt);

    wt->task->state = PPOS_TASK_STATE_SUSPENDED;
    task_suspend(wt->task, &sleepQueue);
    task_yield();

    disk_execute_command(wt);

    return 0;
}

int disk_block_write(int block, void *buffer)
{
    diskrequest_t *wt = create_waiting_task(taskExec, DISK_CMD_WRITE, block, buffer);

    add_waiting_task(wt);

    wt->task->state = PPOS_TASK_STATE_SUSPENDED;
    task_suspend(wt->task, &sleepQueue);
    task_yield();

    disk_execute_command(wt);

    return 0;
}