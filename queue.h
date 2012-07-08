#ifndef __QUEUE_H
#define __QUEUE_H

#include "common.h"
#include "symtab.h"

typedef void * QElemType;         // common stack

typedef struct QNode{
   QElemType  data;
   struct QNode *next;
}QNode, *QueuePtr;

typedef struct {
   QueuePtr front;
   QueuePtr rear;
   int qlen;
} LinkQueue;

void InitQueue(LinkQueue *Q);
void DestroyQueue(LinkQueue *Q);
Status QueueEmpty(LinkQueue *Q);
int    QueueLength(LinkQueue *Q);
Status GetHead(LinkQueue *Q, QElemType *e);
void EnQueue(LinkQueue *Q, QElemType e);
Status DeQueue(LinkQueue *Q, QElemType *e);


#endif


