#ifndef __STACK_H
#define __STACK_H

#include "symtab.h"

#define STACK_INIT_SIZE   100
#define STACKINCREMENT    10

typedef int Status;
typedef void * SElemType;         // common stack

typedef struct {
   SElemType  *base;
   SElemType  *top;
   int  stacksize;
}SqStack;

Status InitStack(SqStack *S);
Status DestroyStack(SqStack *S);
Status StackEmpty(SqStack *S);
int    StackLength(SqStack *S);
Status GetTop(SqStack *S, SElemType *e);
Status GetBottom(SqStack *S, SElemType *e);
Status Push(SqStack *S, SElemType e);
Status Pop(SqStack *S, SElemType *e);
Status ModifyTop(SqStack *S, SElemType e);

#endif


