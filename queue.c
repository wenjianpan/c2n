#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

void InitQueue(LinkQueue *Q)
{
	Q->front = Q->rear = (QueuePtr)malloc(sizeof(QNode));
	if(!Q->front) {
		printf("Out of memory\n");
		exit(0); 
	}
	Q->front->next = NULL;
	Q->qlen = 0;
}

void DestroyQueue(LinkQueue *Q)
{
	while(Q->front) {
		Q->rear = Q->front->next;
		free(Q->front);
		Q->front = Q->rear;
	}
}

Status QueueEmpty(LinkQueue *Q)
{
	return (Q->qlen == 0);
}

int QueueLength(LinkQueue *Q)
{
	return Q->qlen;
}


void EnQueue(LinkQueue *Q, QElemType e)
{
	QNode * p = (QueuePtr)malloc(sizeof(QNode));
	if(!p) {
		printf("Out of memory\n");
		exit(0); 
	}
	p->data = e;
	p->next = NULL;
	Q->rear->next = p;
	Q->rear = p;
	Q->qlen++;
}

Status DeQueue(LinkQueue *Q, QElemType *e)
{
	QNode *p;
	if(Q->front == Q->rear) return R_UNSUCCESS;
	p = Q->front->next;
	*e = p->data;
	Q->front->next = p->next;

	if(Q->rear == p)
		Q->rear = Q->front;
	free(p);
	Q->qlen--;
	return R_SUCCESS;
}

