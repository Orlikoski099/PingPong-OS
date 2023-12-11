#include <stdio.h>
#include <stdlib.h>
#include "disk.h"
#include "ppos_disk.h"
#include "ppos-core-globals.h"

disk_t ppos_disco;

void wakeUpDiskManager()
{
    task_resume(ppos_disco.filaDisco); 
}

int disk_mgr_init(int *numBlocks, int *blockSize)
{
    sem_create(&(ppos_disco.acesso), 1);
    ppos_disco.numBlocks = *numBlocks;
    ppos_disco.blockSize = *blockSize;
    ppos_disco.busy = 0;
    ppos_disco.filaDisco = NULL;
    ppos_disco.disparado = 0;
    ppos_disco.cabeca = 0;
    ppos_disco.distperc = 0;

    return 0;
}

int disk_block_read(int block, void *buffer)
{
    sem_down(&(ppos_disco.acesso));

    diskrequest_t *request = malloc(sizeof(diskrequest_t));
    request->cmd = DISK_CMD_READ;
    request->block = block;
    request->buffer = buffer;

    queue_append((queue_t **)&ppos_disco.filaDisco, (queue_t *)request);

    wakeUpDiskManager();

    sem_up(&(ppos_disco.acesso));

    task_yield();

    return 0; 
}

int disk_block_write(int block, void *buffer)
{
    sem_down(&(ppos_disco.acesso));

    diskrequest_t *request = malloc(sizeof(diskrequest_t));
    request->cmd = DISK_CMD_WRITE;
    request->block = block;
    request->buffer = buffer;

    queue_append((queue_t **)&ppos_disco.filaDisco, (queue_t *)request);

    wakeUpDiskManager();

    sem_up(&(ppos_disco.acesso));

    task_yield(); 

    return 0;
}

void diskDriverBody(void *args)
{
    while (1)
    {
        sem_down(&(ppos_disco.acesso));

        if (ppos_disco.disparado == 1)
        {
            task_resume(ppos_disco.filaDisco);
            ppos_disco.disparado = 0;
        }

        if (ppos_disco.busy == 0 && ppos_disco.filaDisco != NULL)
        {
            diskrequest_t *pedido = (diskrequest_t *)ppos_disco.filaDisco;
            ppos_disco.filaDisco = ppos_disco.filaDisco->next;
            ppos_disco.cabeca = pedido->block;

            printf("cabeca: %d\n", ppos_disco.cabeca);
        }

        sem_up(&(ppos_disco.acesso));
        task_yield();
    }
}
