// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Julho de 2017

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

#include "ppos.h" // Certifique-se de incluir o arquivo de cabeçalho do PingPongOS

// Estrutura que representa um disco no sistema operacional
typedef struct {
  int numBlocks, blockSize, currentPosition;
} disk_t;

// Estrutura que representa uma solicitação de disco
typedef struct request_t {
  int cmd;
  int block;
  void *buffer;
  task_t *task;
  struct request_t *next, *prev;
} request_t;

// Estrutura que representa o gerente de disco no sistema operacional
typedef struct {
  semaphore_t acesso;
  int num_blocks;
  int block_size;
  int ocupado;
  int cabeca;
  long int distperc;
  task_t *filaDisco;
  request_t *filaSolicitacoes;
  short disparado;
} disk_mgr_t;

// Inicialização do gerente de disco
// Retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init(int *numBlocks, int *blockSize);

// Leitura de um bloco, do disco para o buffer
int disk_block_read(int block, void *buffer);

// Escrita de um bloco, do buffer para o disco
int disk_block_write(int block, void *buffer);

// Função para adicionar um pedido à fila de solicitações do disco
void adiciona_pedido(int cmd, int block, void *buffer);

#endif
