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
typedef struct diskrequest_t
{
    struct diskrequest_t *prev;  // Ponteiro para a requisição anterior
    struct diskrequest_t *next;  // Ponteiro para a próxima requisição
    int cmd;            // Comando da requisição (leitura ou escrita)
    int block;          // Número do bloco no disco
    void *buffer;       // Buffer para leitura/escrita
    task_t *task;       // Ponteiro para a tarefa
} diskrequest_t;


// estrutura que representa um disco no sistema operacional
typedef struct
{
    int numBlocks;       // Número total de blocos no disco
    int blockSize;       // Tamanho de cada bloco em bytes
    int busy;            // Indica se o disco está ocupado (1) ou livre (0)
    diskrequest_t *filaDisco;  // Fila de tarefas em espera para acesso ao disco
    task_t *tarefaDisco;       //Terafe do disco
    int disparado;       // Indica se o gerente de disco foi acordado
    int cabeca;          // Posição da cabeça de leitura no disco
    int distperc;        // Distância percorrida pela cabeça de leitura
    semaphore_t acesso;  // Semáforo para controle de acesso ao disco
} disk_t;


int disk_mgr_init(int *numBlocks, int *blockSize);

// leitura de um bloco, do disco para o buffer
int disk_block_read(int block, void *buffer);

// escrita de um bloco, do buffer para o disco
int disk_block_write(int block, void *buffer);

#endif
