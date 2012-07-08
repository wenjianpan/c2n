#ifndef  __PCC_MAIN__
#define  __PCC_MAIN__

#include "stack.h"


#if 0
#define DEBUG                    // debug print out of definition in human langurage
#else
#define TEST_OUTPUT
#endif

#ifdef  TEST_OUTPUT
SqStack output_stk;
#endif


#define ALIGNMENT   4

#define MAX_NR_OF_PARAMS  64

typedef struct _CType_list{
	struct list_head link;
	int  cnt;
	CType * ct;
} CType_list;


#define  IN_LOOP      1
#define  IN_SWITCH    2
#define  IN_FUNCTION  3

#define SWI_MAX_CASE  1024
typedef struct {
	int constant[SWI_MAX_CASE];
	int label[SWI_MAX_CASE];
	int cnt;
	int default_label;
} jmp_table_t;

typedef struct {
	int sta_t;
	int continue_label;  /* only used IN_LOOP */
	int out_label;
	char *start_switch_code;  /* only used IN_SWITCH */
	char *end_switch_code;    /* only used IN_SWITCH */
	jmp_table_t *jt;          /* only used IN_SWITCH */
	symtab_t *symt;    /* This is for function scope identifier, only used when sta_t is IN_FUNCTION */
	struct list_head label_l;   /* This is only used when sta_t is IN_FUNCTION */
} statement_ctx;


#define MAX_STACK_REGS  256
#define NA  1
typedef struct {
	int cur_statck_size;              /* current stack size, for local variables */

	struct {
		short offset;
		short used;
	} stack_reg[MAX_STACK_REGS];    /* 256 32bits stack memory reg, used when out of registers,
	                                               each is offset of allocated 'memory reg',
	                                               for IA32 arch, all offset will be initialized to NA(not available 1) (because stack offset is <=0 [EBP]) */

	char * stk_instr_addr;
	int stk_instr_len;
	char *buf;
	int buf_index;

	int hidden_local_value_sz;
	int hidden_local_value_offset;   /* offset in the satck */
	
	bool is_ESI_used;
	bool is_EDI_used;


} func_ctx;


typedef struct {
	struct list_head link;
	char * name;
	
} label_t;

#define  STAGE_GLOBAL       1
#define  STAGE_PARAM        2
#define  STAGE_FUNCTION   4


extern void dump_regs();

#endif

