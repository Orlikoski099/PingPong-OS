// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Julho de 2017

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

#include "ppos.h"    // Inclua o arquivo ppos.h
#include "queue.h"   
#include "ppos-core-globals.h"


// Definição da estrutura para as requisições de disco
typedef struct diskrequest_t {
    task_t* task;
    int command;
    int block;
    void *buffer;
    struct diskrequest_t* next;
} diskrequest_t;

// estrutura que representa um disco no sistema operacional
typedef struct {
    mutex_t mutex_disk_access;
    mutex_t mutex_queue;
    diskrequest_t* awaiting_tasks;
    diskrequest_t* processing_task;
    task_t scheduler;
} disk_t;


int disk_mgr_init(int *numBlocks, int *blockSize);

// leitura de um bloco, do disco para o buffer
int disk_block_read(int block, void *buffer);

// escrita de um bloco, do buffer para o disco
int disk_block_write(int block, void *buffer);

#endif
