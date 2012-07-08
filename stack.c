#include "stack.h"

Status InitStack(SqStack *S)
{
     S->base = (SElemType *)malloc(STACK_INIT_SIZE * sizeof(SElemType));
     if(!S->base) exit(R_NOMEMORY);
     S->top = S->base;
     S->stacksize = STACK_INIT_SIZE;
     return R_SUCCESS;
} 

void FreeStack(SqStack *S)
{
	free(S->base);
	S->top = 0;
	S->base = 0;
	S->stacksize = 0;
	/* The stack should be always not malloced value,
	so we do not need to free stack variable itself */
}

bool StackEmpty(SqStack *S)
{
     if(S->top == S->base)
	 	return T;
	 return F;
}

int StackLength(SqStack *S)
{
     return (S->top - S->base);
}

Status GetTop(SqStack *S, SElemType *e)
{
     if(S->top == S->base)  return R_UNSUCCESS;
     *e = *(S->top - 1);
     return R_SUCCESS;
}

Status GetBottom(SqStack *S, SElemType *e)
{
     if(S->top == S->base)  return R_UNSUCCESS;
     *e = *(S->base);
     return R_SUCCESS;
}


Status ModifyTop(SqStack *S, SElemType e)
{
     if(S->top == S->base)  return R_UNSUCCESS;
	 *(S->top - 1) = e;
	 return R_SUCCESS;
}

Status Push(SqStack *S, SElemType e)
{
     if(S->top - S->base >= S->stacksize) {
          S->base = (SElemType *)realloc(S->base, (S->stacksize + STACKINCREMENT) * sizeof(SElemType));
          if(!S->base) exit(R_OVERFLOW);
	  S->top = S->base + S->stacksize;
	  S->stacksize += STACKINCREMENT;
     }
     
     *S->top++ = e;
     return R_SUCCESS;
}

Status Pop(SqStack *S, SElemType *e)
{
     if(S->top == S->base) return R_UNSUCCESS;
     *e = * --S->top;
     return R_SUCCESS;
}


#ifdef TEST
int main() {
  SqStack sym_stack;
  SElemType e;
  int i = 1;

  if(InitStack(&sym_stack) != R_SUCCESS) {
     printf("init stack failed\n");
	 return 0;	 
  }

  printf("empty ? %d\n", StackEmpty(&sym_stack));
  e = (SElemType)(i++);
  Push(&sym_stack, e);
  e = (SElemType)(i++);
  Push(&sym_stack, e);

  printf("empty ? %d\n", StackEmpty(&sym_stack));
  printf("stack size %d\n", StackLength(&sym_stack));
  GetTop(&sym_stack, &e);
  printf("get top 0x%x\n", e);

  Pop(&sym_stack, &e);
  printf("pop 0x%x\n", e);
  printf("stack size %d\n", StackLength(&sym_stack));
  Pop(&sym_stack, &e);
  printf("pop 0x%x\n", e);
  printf("stack size %d\n", StackLength(&sym_stack));

  printf("empty ? %d\n", StackEmpty(&sym_stack));
}

#endif


