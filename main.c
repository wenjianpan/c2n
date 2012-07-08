/*
 *  C programming  language parser.
 *
 *  flex -o scan.c scan.l
 *  gcc main.c symtab.c stack.c -mno-cygwin -o cparser.exe
 *
 *  Pan Wenjian
 *  2009.6.3   Started this program
 *  ????          The syntax parser is released 
 *  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>

#include "symtab.h"
#include "stack.h"
#include "scan.c"

#include "i386.h"
#include "pcc.h"


SqStack g_sym_stack;

/* global stack for statement context */
SqStack statement_ctx_stk;

func_ctx g_func_ctx;    /* function context */
bool is_in_function = F;
bool g_ajust_offset_needed = F;
char * g_ajust_offset_ptr;

#define  OUTBUF     g_func_ctx.buf
#define  OUTBUFIDX  g_func_ctx.buf_index

int  g_cur_stage;
int  g_stk_h2l = 1;   // for Intel CPU
int  g_label = 0;

char out2[4096];

typedef struct {
	CType **ap;
	int cnts;
} call_rop;

struct pre_inc_code {
	char * start;
	char *end;
} g_pre_inc_code;

/* it's tricky, just handle like &(*a) */
SqStack addr_op_stk;

/******************** function prototype **********************/
int gettoken(void);
void restoretoken();
void restoretoken2(char *s);
int type_qualifier();
int type_qualifier_list(qualifier *qu);
void declarator(CType *pCType, int *dt);
void declarator2(CType *pCType, int *dt);
CType * pointer();
int parameter_declaration(CType **param_ctype);
int parameter_list(CType *pCType);
void parameter_type_list(CType *pCType);
void identifier_list(CType *pCType);
int type_specifier(CType *pCType, CType **ct_out, bool tag_must_def_before);
int storage_class_specifier();
int declaration_specifiers(CType *pCType, bool tag_must_def_before);
CType * init_declarator(CType *pctype, int idx, bool no_init);
CType_list * init_declarator_list(CType *pCType, bool no_init);
CType_list * declaration(bool has_decl_spec, bool no_init);
void type_copy(CType *to, CType *from);
char *store_name(const char *from, int strlen);
CType * create_ctype();
void free_ctype(void*);
void build_ctype(CType *pCType);
int symtabs_lookup( const char *key, int type, symvalue_t **value);
void error(const char *p);
void print_ctype(CType *pCType, char *out);
int struct_or_union_specifier(CType * pCType, CType **ct_out, bool tag_must_def_before);
int struct_or_union();
void struct_declaration_list(CType * pctype) ;
void struct_declarator(CType * pctype);
CType_list * struct_declarator_list(CType * pctype);
CType_list * struct_declaration();
int  enum_specifier(CType **ct_out, bool tag_must_def_before);
CType_list * create_ctype_list();
void free_ctype_list(CType_list *ct);
void CType_cpy(CType * to, CType * from, bool copy_name_and_link);
CType * add_decl_to_symtab(CType* pctype);
bool is_ctype_compatible(CType * pctype1, CType * pctype2);
value_t* create_c_val();
void free_c_val(value_t *p);
int primary_expr(CType **p);
int postfix_expr(CType **p);
int unary_expr(CType **p);
int cast_expr(CType **p);
int multiplicative_expr(CType **p);
int additive_expr(CType **p);
int shift_expr(CType ** p);
int relational_expr(CType **p);
int equality_expr(CType **p);
int and_expr(CType **p);
int exclusive_or_expr(CType **p);
int inclusive_or_expr(CType **p);
int logical_and_expr(CType **p);
int logical_or_expr(CType **p);
int  conditional_expr(CType **p);
int assignment_expr(CType **p);
int expr(CType **p, bool later_gen);
int  constant_expr(CType ** p);
label_t *create_label(char *name);
void free_label(label_t *p);
statement_ctx *create_sta_ctx();
void free_sta_ctx(statement_ctx *p);
int find_sta_ctx(int sta_t, statement_ctx **p);
void do_label(statement_ctx *ctx);
int generate_tmp_ctype(CType ** to, CType * from);
bool is_ctype_array(CType * pctype);
void gen_assign(CType *pp, CType *tmp);
void add_ctype_to_stack(CType *pctype);
CType * genop(CType * ct);
void adjust_offset(int size);

/**************************************************************/

char out_buffer[4096];

void oprintf(char *format, ...) 
{
	static char        buffer[1024] = {0};
	va_list     ap;   
	va_start(ap, format);
	vsprintf(buffer, format, ap);
	va_end(ap);

	OUTBUFIDX += sprintf(&(OUTBUF[OUTBUFIDX]), buffer);

}

void error(const char *p)
{
	printf(p);
	exit(-1);
}

void dump(CType * tmp)
{
	struct list_head *head;
	struct list_head *curr;
	int end = 0;

	printf("-------Dump CType\n");	
	printf("CType is %x\n", tmp);
	printf("getfrom 0x%x\n", tmp->get_from);
	head = &(tmp->link);
	printf("next is %x\n", head->next);
	printf("prev is %x\n\n", head->prev);
	
 	for (curr = head->next; curr != NULL; ) {
		printf("curr is %x\n", curr );

		if(tmp->get_from != NULL && curr == head->prev) {
			printf("end\n");
			end = 1;
		}
		curr = curr->next;	
		if(end) break;
 	}
	printf("------------------\n");
}


value_t *create_c_val()
{
    value_t * p;
	p = malloc(sizeof(value_t));
    if (p == NULL) {
       printf("error: Out of memory\n");
	   exit(-1);
    }
	memset(p, 0, sizeof(value_t));
	INIT_LIST_HEAD(&(p->link));
	return p;
}

void free_c_val(value_t *p)
{
	
	if(p->string_t == STR_CHAR)
		free(p->content.s);
	else if(p->string_t == STR_LONG)
		free(p->content.ls);
	else {
		/* do nothing, not string literal */
	}
	free(p);
}


void free_ctype(void *p)
{
	CType * tmp = (CType *)p;
	struct list_head *head;
	struct list_head *curr;
	int i;
	int end = 0;

	if(tmp->datatype == DATATYPE_ELLIPSIS || tmp->symtype == SYM_TYPE_LABEL
		|| tmp->datatype == DATATYPE_OP){
		free(tmp);
		return;
	}
	if(tmp->datatype == DATATYPE_POINTER){	/* This is to free ctype pointer created from '&' */
		if(tmp->types.pointer.is_from_addrop == T) {
			free(tmp);
		}
		return;
	}
	
	if(tmp->datatype == DATATYPE_STRUCT|| tmp->datatype == DATATYPE_UNION)
	{
		if(tmp->name) {
			//printf("free struct '%s'\n", tmp->name);
			free(tmp->name);
		}
		if(tmp->types.struct_union_enum.table != NULL)
			symtab_destroy(&(tmp->types.struct_union_enum.table), free_ctype);
		if(tmp->types.struct_union_enum.item_list != NULL)
			free(tmp->types.struct_union_enum.item_list);
		free(p);
		return;
	}

	if(tmp->datatype == DATATYPE_ENUM) {
		if(tmp->name) {
			//printf("free ENUM '%s'\n", tmp->name);
			free(tmp->name);
		}
		// tmp->types.struct_union_enum.table is not need to be freed;
		// it's NULL or !NULL
		free(p);
		return;
	}

	assert(tmp->datatype == DATATYPE_NORMAL);	

	if(tmp->symtype == SYM_TYPE_NO_NAME_VAL) {
		free_c_val(tmp->v);
	}
	
	if(tmp->name) {
		//printf("free '%s'\n", tmp->name);
		free(tmp->name);
	}

	if(tmp->types.normal.d_attr.type_spec == TS_STRUCT 
		|| tmp->types.normal.d_attr.type_spec == TS_UNION) {
		CType *ct = tmp->types.normal.d_attr.st_un;
		//printf("is %x\n", ct);
		if(tmp->types.normal.d_attr.tag) {
			//printf("free tag '%s'\n", tmp->types.normal.d_attr.tag);
			free(tmp->types.normal.d_attr.tag);
		}
		if(ct->types.struct_union_enum.attach_to == tmp) {
			//printf("free attached %x\n", ct);
			free_ctype(ct);
		}
	}

	/* dump(tmp); */

	head = &(tmp->link);
	if(tmp->get_from != NULL && head == head->prev){
		free(p);
		return;
	}

	/*
		if ctype list is not get from other, free all the list (i.e. free all ctypes 
		in the list until next is NULL), else only free until the head->prev point to.
		e.g
		typedef int *P;
		P ptr;

               P (DATATYPE_NORMAL)   (DATATYPE_POINT)
		+--------+                 +-------+
		|       int      |   --- ->   | point to  | ----> NULL
		| (typedef)  |     next     |               |   next
		+--------+                 +-------+
		         |            prev                 ^
		         +------------------|       
		                                               |                       
               ptr (DATATYPE_NORMAL)         |
		+--------+                           |
		|       int      |   --- -> NULL    |
		|                 |                           |
		+--------+                           |
		         |           get_from            |
		         +------------------+
		
	*/
 	for (curr = head->next; curr != NULL; ) {
		if(tmp->get_from != NULL && curr == head->prev)
			end = 1;
		CType * elt = list_entry(curr, CType, link);
		curr = curr->next;
		
		assert(elt->name == NULL); 
		
		if(elt->datatype == DATATYPE_FUNCTION) {
			if(elt->types.function.param_list != NULL) {
				for(i = elt->types.function.nr_param; i>0; i--) {
					//printf("free param\n");
					CType * ct = elt->types.function.param_list[i-1];
					if(ct->datatype == DATATYPE_ELLIPSIS)
						free_ctype((void*)ct);
				}
				free(elt->types.function.param_list);
			}
			/* if  elt->types.function.nr_param == 0,  then elt->types.function.table should == NULL */
			if(elt->types.function.table != NULL)
				symtab_destroy(&(elt->types.function.table), free_ctype);
		}
		free(elt);
		if(end) break;
	}

	free(p);
}


bool is_ctype_compatible(CType * pctype1, CType * pctype2)
{
	struct list_head* list1;
	struct list_head* list2;
	CType *tmp1, *tmp2;

	/* TODO: a well defined type compatible function */
	list1 = &(pctype1->link);
	list2 = &(pctype2->link);

	while(list1 != NULL && list2 != NULL) {
		
		tmp1 = list_entry(list1, CType, link);
		tmp2 = list_entry(list2, CType, link);
		if((tmp1->datatype == tmp2->datatype && tmp1->symtype == tmp2->symtype) ||
		   (tmp1->datatype == tmp2->datatype && tmp1->symtype != tmp2->symtype &&
		     (tmp1->symtype != SYM_TYPE_TAG && tmp1->symtype != SYM_TYPE_TYPEDEF) &&
		     (tmp2->symtype != SYM_TYPE_TAG && tmp2->symtype != SYM_TYPE_TYPEDEF))	
			){
			if(tmp1->datatype == DATATYPE_NORMAL) {
				if(tmp1->types.normal.d_attr.has_stor_spec ==
					tmp2->types.normal.d_attr.has_stor_spec &&
					tmp1->types.normal.d_attr.stor_spec ==
					tmp2->types.normal.d_attr.stor_spec &&	
					tmp1->types.normal.d_attr.has_type_spec ==
					tmp2->types.normal.d_attr.has_type_spec &&
					tmp1->types.normal.d_attr.type_spec ==
					tmp2->types.normal.d_attr.type_spec &&	
					tmp1->types.normal.d_attr.st_un ==
					tmp2->types.normal.d_attr.st_un) {
						if(tmp1->types.normal.d_attr.qua.q_const 
								== tmp2->types.normal.d_attr.qua.q_const
							&& tmp1->types.normal.d_attr.qua.q_volatile 
								== tmp2->types.normal.d_attr.qua.q_volatile) {
						} else { 
							printf("type not compatible0 %d %d\n", tmp1->symtype, tmp2->symtype);
							return F;
						}
					} else {
						if(tmp1->types.normal.d_attr.type_spec !=
							tmp2->types.normal.d_attr.type_spec) {
							if(tmp1->types.normal.d_attr.type_spec < TS_FLOAT &&
								tmp2->types.normal.d_attr.type_spec < TS_FLOAT)
								return T;
						}
						printf("type not compatible1 %d %d\n", tmp1->symtype, tmp2->symtype);
						printf("%d %d\n", tmp1->types.normal.d_attr.has_stor_spec,
							tmp2->types.normal.d_attr.has_stor_spec);
						printf("%d %d\n", tmp1->types.normal.d_attr.stor_spec,
							tmp2->types.normal.d_attr.stor_spec);
						printf("%d %d\n", tmp1->types.normal.d_attr.has_type_spec,
							tmp2->types.normal.d_attr.has_type_spec);
						printf("%d %d\n", tmp1->types.normal.d_attr.type_spec,
							tmp2->types.normal.d_attr.type_spec);	
						printf("%d %d\n", tmp1->types.normal.d_attr.st_un,
							tmp2->types.normal.d_attr.st_un);

						return F; 
					}
					
			} else if(tmp1->datatype == DATATYPE_POINTER) {
				if(tmp1->types.normal.d_attr.qua.q_const 
						== tmp2->types.normal.d_attr.qua.q_const
						&& tmp1->types.normal.d_attr.qua.q_volatile 
							== tmp2->types.normal.d_attr.qua.q_volatile) {
				} else return F;
			} else if(tmp1->datatype == DATATYPE_FUNCTION) {
				int i=0;
				if(tmp1->types.function.nr_param == 0 || tmp2->types.function.nr_param == 0)
					return T;
				if(tmp1->types.function.is_old_style == T ||
					tmp2->types.function.is_old_style == T) {
					return T;
				} 
				for(; i< tmp1->types.function.nr_param; i++) {
					if(is_ctype_compatible(tmp1->types.function.param_list[i], 
						tmp2->types.function.param_list[i]) == F)
						return F;
				}
					
			} else if(tmp1->datatype == DATATYPE_ARRAY) {
				/* do nothing */
			} else if(tmp1->datatype == DATATYPE_ELLIPSIS) {
				/* do nothing */
			} else {
				printf("Type compare, This should not happen\n");
				exit(0);
			}
		}
		else {
			printf("type not compatible2 %d %d\n", tmp1->symtype, tmp2->symtype);
			return F; 
		}
go_on:		
		list1 = list1->next;
		list2 = list2->next;
	}
	if(list1 == NULL && list2 == NULL) return T;
	printf("type not compatible3 %d %d\n", tmp1->symtype, tmp2->symtype);
	return F;
}

void free_ctype_list(CType_list *ct)
{
	struct list_head *curr;
	struct list_head *head;
	CType_list * tmp;

	head = &(ct->link);
	
 	for (curr = head->next; curr != head; ) {
		tmp = list_entry(curr, CType_list, link);
		curr = curr->next;
		free(tmp);
	}
	free(ct);
}

void print_help()
{
    printf( 
    "A tool for analyse C language declaration.\n"
    "Pan Wenjian\n"
    "2009.6.5\n"
    "If you find bugs, please mail to panwenjian@yeah.net. Thank you! ;-) \n"
    "\n"
    "examples:\n"
    "int f(char * const *(*next)());\n"
    "f:  function ( parameters: next: pointer to function returning pointer to const\n"
    "pointer to char ) returning int\n"
    "\n"
    "char *(* c[])(int **p);\n"
    "c:  array[] of pointer to function ( parameters: p: pointer to pointer to int )\n"
    "returning pointer to char\n"
    "\n"
    "void (*signal(int sig, void (*func)(int a)))(int b);\n"
    "signal:  function ( parameters: sig: int,func: pointer to function (parameters:\n"
    "a: int ) returning void ) returning pointer to function ( parameters: b: int )\n"
    "returning void\n\r"
    );
}

/*
  store name: make sure 'strlen' is string length of 'from'
*/
char *store_name(const char *from, int strlen)
{
	char *name;
	
	name = malloc(strlen+1);
	if(name == NULL) {
		error("Out of memory\n");
	}
	strcpy(name, from);
	return name;
}

CType_list * create_ctype_list()
{
	CType_list * p;
	p = malloc(sizeof(CType_list));
    if (p == NULL) {
       printf("error: Out of memory\n");
	   exit(-1);
    }
	INIT_LIST_HEAD(&(p->link));
	p->cnt = 0;
	p->ct = NULL;
	return p;
}

CType * create_ctype()
{
    CType * p;
	p = malloc(sizeof(CType));
    if (p == NULL) {
       printf("error: Out of memory\n");
	   exit(-1);
    }
	memset(p, 0, sizeof(CType));
	INIT_LIST_HEAD(&(p->link));
	p->datatype = DATATYPE_NORMAL;  /* The first CType is always normal */
	p->storage_type = STORAGE_UNKNOW;
	p->lvalue = 1;        /* default we assume this Ctype has lvalue */
	return p;
}

CType * create_op_ctype()
{
    CType * p;
	p = malloc(sizeof(CType));
    if (p == NULL) {
       printf("error: Out of memory\n");
	   exit(-1);
    }
	memset(p, 0, sizeof(CType));
	INIT_LIST_HEAD(&(p->link));
	p->datatype = DATATYPE_OP;  
	return p;
}

static unsigned long sizeof_ctype(CType *pCType, bool check_size)
{
	CType *temp = NULL;

	if(pCType == NULL) {
		// error
		printf("sizeof ctype error!\n");
		exit(0);
	}
	
	if(is_ctype_normal(pCType)){
		int d_type = pCType->types.normal.d_attr.type_spec;
		if((d_type & TS_LONG_LONG) == TS_LONG_LONG) {
			return 8;
		} else if((d_type & TS_LONG) == TS_LONG) {
			return 4;
		} else if((d_type & TS_SHORT) == TS_SHORT) {
			return 2;
		} else if((d_type & TS_SHORT_INT) == TS_SHORT_INT) {
			return 2;
		} else if((d_type & TS_INT) == TS_INT) {
			return 4;
		} else if((d_type & TS_CHAR) == TS_CHAR) {
			return 1;
		} else if((d_type & TS_FLOAT) == TS_FLOAT) {
			return 4;
		} else if((d_type & TS_DOUBLE) == TS_DOUBLE) {
			return 8;
		}
		/* TODO: other basic types ....*/
	} else if(is_ctype_pointer(pCType)){
		struct list_head *curr;
		
		curr = pCType->link.next;
		
		if(curr != NULL) {
			CType * elt = list_entry(curr, CType, link);
			if(elt->datatype == DATATYPE_ARRAY &&
				pCType->storage_type != STORAGE_PARAM)
				goto is_array;
		}

		return 4;
	} else if(is_ctype_array(pCType)) {
	
		struct list_head *curr;
		CType * elt;

		is_array:
		curr = pCType->link.next;
		elt = list_entry(curr, CType, link);			

		generate_tmp_ctype(&temp, pCType);
		temp->link.next = temp->link.next->next;
		if(check_size && elt->types.array.is_pointer_alike) {
			printf("%d: error: size unknown\n", line);
			exit(0);
		}
		pCType->size_of = elt->types.array.array_size * sizeof_ctype(temp, check_size);
		free(temp);
	} else if(is_ctype_struct(pCType) || is_ctype_union(pCType)) {
		int i = 0;
		int size = 0;
		int oldsize;
		CType * tmp1 = pCType->types.normal.d_attr.st_un;

		pCType->size_of = tmp1->size_of;
	} 

	return pCType->size_of ;
}

void build_code_gen_fields(CType *pCType)
{
	/* 1. storage type */
	
	if(pCType->types.normal.d_attr.has_stor_spec == T  &&  
		pCType->types.normal.d_attr.stor_spec == STATIC) {
		pCType->storage_type = STORAGE_STATIC;
	} else {
		if(g_cur_stage == STAGE_FUNCTION) {
			if(pCType->types.normal.d_attr.stor_spec != TYPEDEF)
				pCType->storage_type = STORAGE_LOCAL;
		} else if (g_cur_stage == STAGE_PARAM) {
			pCType->storage_type = STORAGE_PARAM;
		} else if(g_cur_stage == STAGE_GLOBAL) {
			pCType->storage_type = STORAGE_GLOBAL;
		} else {
		    // maybe reg or constant 
		} 
	}

	/* 2. size of */
	pCType->size_of = sizeof_ctype(pCType, F);

	/* 3. */	
}

void build_ctype(CType *pCType)
{
	if(pCType->datatype == DATATYPE_ELLIPSIS){
		pCType->link.prev->next = NULL;
		return;
	}
	
	assert(pCType->datatype == DATATYPE_NORMAL);

	if(pCType->name != NULL && (pCType->types.normal.d_attr.type_spec == TS_STRUCT || 
		pCType->types.normal.d_attr.type_spec == TS_UNION) ){
		CType *tmp;
		assert(pCType->types.normal.d_attr.st_un != NULL);
		tmp = pCType->types.normal.d_attr.st_un;

		// struct or union table may be empty, ex. struct s {};
		if(tmp->types.struct_union_enum.table == NULL && 
			(tmp->types.struct_union_enum.is_empty != T && 
			 pCType->types.normal.d_attr.stor_spec != TYPEDEF)) {
			printf("%d: size of struct '%s' unknow\n", line, pCType->name);
			exit(0);
		}
	}
	
	if(pCType->name != NULL && pCType->link.next == NULL 
		&& pCType->types.normal.d_attr.type_spec == TS_VOID) {
		printf("%d: storage of '%s' unknow\n", line, pCType->name);
		exit(0);
	}

	if(pCType->types.normal.d_attr.has_stor_spec == T  &&  
		pCType->types.normal.d_attr.stor_spec == TYPEDEF) {
		pCType->symtype = SYM_TYPE_TYPEDEF;
	}

	if(pCType->name != NULL) {
		if(pCType->symtype == 0) 
		  	 pCType->symtype = SYM_TYPE_OBJECT;
	} else if(pCType->symtype == 0){
		pCType->symtype = SYM_TYPE_NONAME; 
	}
	
	if(pCType->get_from == NULL) {
		pCType->link.prev->next = NULL;	
	} else {
		assert(pCType->get_from->types.normal.d_attr.has_stor_spec == T
			&& pCType->get_from->types.normal.d_attr.stor_spec == TYPEDEF );
		
		pCType->datatype = pCType->get_from->datatype;

		/* check if ()() ()[] []() , like 
			typedef int tf();
			tf  tf2();
		*/
		CType * tmp;
		struct list_head * l;
		assert(pCType->link.prev != NULL);
		tmp = list_entry((pCType->link.prev), CType, link);
		if(tmp->datatype == DATATYPE_ARRAY) {
			l = pCType->get_from->link.next;
			tmp = list_entry(l, CType, link);
			if(tmp && tmp->datatype == DATATYPE_FUNCTION) {
				printf("%d: error: '%s' declared as array of function\n", line, pCType->name);
				exit(0);
			}
		} else if(tmp->datatype == DATATYPE_FUNCTION) {
			l = pCType->get_from->link.next;
			tmp = list_entry(l, CType, link);
			/*
				if tmp is NULL, this is happen when
				typedef struct {} tf;
				tf  tf2();
			*/
			if(tmp) {
				if(tmp->datatype == DATATYPE_FUNCTION) {
					printf("%d: error: '%s' declared as function return a function\n", line, pCType->name);
					exit(0);
				}else if (tmp->datatype == DATATYPE_ARRAY){
					printf("%d: error: '%s' declared as function return a array\n", line, pCType->name);
					exit(0);
				} 
			}
		}

		/* pCType->types.normal id get from pCType->get_from->types.normal
			except the storage type
		*/
		pCType->types.normal.d_attr.has_type_spec
			= pCType->get_from->types.normal.d_attr.has_type_spec;
		pCType->types.normal.d_attr.type_spec
			= pCType->get_from->types.normal.d_attr.type_spec;	
		if(pCType->types.normal.d_attr.qua.q_const == 0)
			pCType->types.normal.d_attr.qua.q_const 
				= pCType->get_from->types.normal.d_attr.qua.q_const;
		if(pCType->types.normal.d_attr.qua.q_volatile == 0)
			pCType->types.normal.d_attr.qua.q_volatile 
				= pCType->get_from->types.normal.d_attr.qua.q_volatile;		
		if(pCType->get_from->types.normal.d_attr.tag) {
			pCType->types.normal.d_attr.tag
				= store_name(pCType->get_from->types.normal.d_attr.tag, 
					strlen(pCType->get_from->types.normal.d_attr.tag));
		}
		pCType->types.normal.d_attr.st_un
			= pCType->get_from->types.normal.d_attr.st_un;
		
		pCType->link.prev->next = pCType->get_from->link.next;
	}

	if(pCType->types.normal.d_attr.type_spec == TS_SIGNED || 
		pCType->types.normal.d_attr.type_spec == TS_USIGNED) {
		pCType->types.normal.d_attr.type_spec |= INT_MASK;
	} else {
		int d_type = pCType->types.normal.d_attr.type_spec & (~(SIGNED_MASK|UNSIGNED_MASK));
		if( d_type == TS_LONG || d_type == TS_SHORT || d_type == TS_LONG_LONG)
			pCType->types.normal.d_attr.type_spec |= INT_MASK;
	}
	
	if(pCType->types.normal.d_attr.type_spec == 0 && (pCType->types.normal.d_attr.has_stor_spec ||
		(pCType->types.normal.d_attr.qua.q_const != 0 || pCType->types.normal.d_attr.qua.q_volatile != 0))) {
		pCType->types.normal.d_attr.type_spec = INT_MASK;
		pCType->types.normal.d_attr.has_type_spec = T;
	}

	/* next, build code generation fileds */
	build_code_gen_fields(pCType);

#if 1
	/*
		If function return value (eg. structure ) which size is big than 4,
		means a 32bits reg can not hold it, we need push a pointer of
		a return value before the first pushed parameter
	*/
	if(is_ctype_function(pCType)){
		CType * rct = NULL;
		generate_tmp_ctype(&rct, pCType);

		struct list_head *lh;
		lh = rct->link.next->next;
		rct->link.next = lh;			 // function return value ctype
		rct->size_of = sizeof_ctype(rct, F);

		CType *c_tmp;
		struct list_head *cur = pCType->link.next;
		c_tmp = list_entry(cur, CType, link);

		if(rct->size_of > 4) {
			int i = 0;
			for(i = 0; i < c_tmp->types.function.nr_param; i++) {
				CType *ct = c_tmp->types.function.param_list[i];
				if(ct->datatype == DATATYPE_ELLIPSIS) {
					assert(i+1 == c_tmp->types.function.nr_param);
					break;
				}
				else
					ct->offset += 4;	/* pointer size is 4 */
			}
		}
		free(rct);
	}
#endif

	return;
}


/*
 This function only copy a head ctype, for create a new ctype list
*/
void CType_cpy(CType * to, CType * from,  bool copy_name_and_link)
{
	assert(from->datatype == DATATYPE_NORMAL);
	memcpy(to, from, sizeof(CType));
	
	if(copy_name_and_link == T) {
		if(from->name != NULL)
			to->name = store_name(from->name, strlen(from->name));
	}
	else  {
		to->name = NULL;
		INIT_LIST_HEAD(&(to->link));
	}
	if(from->types.normal.d_attr.tag) {
		to->types.normal.d_attr.tag
		= store_name(from->types.normal.d_attr.tag, strlen(from->types.normal.d_attr.tag));
	}	
}


static char *
itoa(int value, char *string, int radix)
{
	char tmp[33];
	char *tp = tmp;
	int i;
	unsigned v;
	int sign;
	char *sp;
	
	if (radix > 36 || radix <= 1)
	{
		return 0;
	}
	
	sign = (radix == 10 && value < 0);
	if (sign)
		v = -value;
	else
		v = (unsigned)value;
	while (v || tp == tmp)
	{
		i = v % radix;
		v = v / radix;
		if (i < 10)
		  *tp++ = i+'0';
		else
		  *tp++ = i + 'a' - 10;
	}
	
	if (string == 0)
		string = (char *)malloc((tp-tmp)+sign+1);
	sp = string;
	
	if (sign)
		*sp++ = '-';
	while (tp > tmp)
		*sp++ = *--tp;
	*sp = 0;
	return string;
}

void print_ctype(CType *pCType, char *out)
{
	struct list_head *head;
	struct list_head *curr;
	int i;
	int tmp;
	char buff[256];

	if(pCType->line > 0) {
		char buf[100];
		sprintf(buf, "%d: ", pCType->line); 
		strcat(out, buf);
	}
		
	if(pCType->datatype == DATATYPE_ELLIPSIS){
		assert(pCType->link.next == NULL);
		strcat(out, " ...");
		return;	
	}

	if(pCType->types.normal.d_attr.has_stor_spec) {
		tmp = pCType->types.normal.d_attr.stor_spec;
		switch(tmp)
		{
			case AUTO:
				strcat(out, "auto <- ");
				break;
			case REGISTER:
				strcat(out, "register <- ");
				break;
			case STATIC:
				strcat(out, "static <- ");
				break;
			case EXTERN:
				strcat(out, "extern <- ");
				break;
			case TYPEDEF:
				strcat(out, "typedef <- ");				
				break;
			default:
				break;
		}
	}

	if(pCType->name != NULL) {
		  strcat(out, pCType->name);
		  strcat(out, ": ");
	} 

	if(pCType->types.normal.d_attr.has_enum_val == T) {	/* is enum ID itself */
		char buf[256];
		assert(pCType->link.next == NULL);
		strcat(out, " enum ID, val = ");
		sprintf(buf, "%x", pCType->types.normal.d_attr.enum_val);
		strcat(out, buf);
		return;
	}

	head = &(pCType->link);
    for (curr = head->next; curr != NULL; ) {
        CType * elt = list_entry(curr, CType, link);
		curr = curr->next;

		assert(elt->datatype != DATATYPE_NORMAL);
		if(elt->datatype == DATATYPE_ARRAY) {
			strcat(out, " array[");
			if(elt->types.array.array_size > 0) {
				char *str = itoa(elt->types.array.array_size, 0, 10);
				assert(str != NULL);
				strcat(out, str);
				free(str);
			}
			strcat(out, "] of");
		}else if(elt->datatype == DATATYPE_FUNCTION) {
			strcat(out, " function");
			if(elt->types.function.param_list != NULL) {
				strcat(out, " (parameters: ");
				for(i = 0; i < elt->types.function.nr_param; ) {
					print_ctype(elt->types.function.param_list[i], out);
					if(++i < elt->types.function.nr_param)
						strcat(out, ", ");
				}
				strcat(out, " )");
			}
			strcat(out, " return");
		}else if(elt->datatype == DATATYPE_POINTER) {
			if(tmp = elt->types.pointer.qua.q_const == CONST) {
				strcat(out, " const");
			}
			if(tmp = elt->types.pointer.qua.q_volatile == VOLATILE) {
				strcat(out, " volatile");
			}
			strcat(out, " pointer to");
			
		} else if(elt->datatype == DATATYPE_OP){
			return;
		} else {
			error("This should not happen!!\n");
		}
		
    }
	

	if(tmp = pCType->types.normal.d_attr.qua.q_const == CONST) {
		strcat(out, " const");
	}
	if(tmp = pCType->types.normal.d_attr.qua.q_volatile == VOLATILE) {
		strcat(out, " volatile");
	}
	
	switch(pCType->types.normal.d_attr.type_spec)
	{
		case TS_SIGNED:
			strcat(out, " signed");
			break;
		case TS_USIGNED:
			strcat(out, " unsigned");
			break;
		case TS_CHAR:
			strcat(out, " char");
			break;			
		case TS_SCHAR:
			strcat(out, " signed char");
			break;			
		case TS_UCHAR:
			strcat(out, " unsigned char");
			break;			
		case TS_SHORT:
			strcat(out, " short");
			break;			
		case TS_SSHORT:
			strcat(out, " signed short");
			break;			
		case TS_USHORT:
			strcat(out, " unsigned short");
			break;			
		case TS_INT:
			strcat(out, " int");
			break;			
		case TS_SINT:
			strcat(out, " signed int");
			break;			
		case TS_UINT:
			strcat(out, " unsigned int");
			break;
		case TS_SHORT_INT:
			strcat(out, " short int");
			break;
		case TS_SSHORT_INT:
			strcat(out, " signed short int");
			break;			
		case TS_USHORT_INT:
			strcat(out, " unsigned short int");
			break;			
		case TS_LONG:
			strcat(out, " long");
			break;
		case TS_SLONG:
			strcat(out, " signed long");
			break;
		case TS_ULONG:
			strcat(out, " unsigned long");
			break;
		case TS_LONG_INT:
			strcat(out, " long int");
			break;
		case TS_SLONG_INT:
			strcat(out, " signed long int");
			break;
		case TS_ULONG_INT:
			strcat(out, " unsigned long int");
			break;
		case TS_LONG_LONG:
			strcat(out, " long long");
			break;
		case TS_SLONG_LONG:
			strcat(out, " signed long long");
			break;
		case TS_ULONG_LONG:
			strcat(out, " unsigned long long");
			break;
		case TS_LONG_LONG_INT:
			strcat(out, " long long int");
			break;
		case TS_SLONG_LONG_INT:
			strcat(out, " signed long long int");
			break;
		case TS_ULONG_LONG_INT:
			strcat(out, " unsigned long long int");
			break;
		case TS_FLOAT:
			strcat(out, " float");
			break;
		case TS_DOUBLE:
			strcat(out, " double");
			break;
		case TS_LONG_DOUBLE:
			strcat(out, " long double");
			break;
		case TS_VOID:
			strcat(out, " void");
			break;			
		case TS_STRUCT:
			strcat(out, " struct");
			char buf[256];
 			if(pCType->types.normal.d_attr.tag){
				sprintf(buf, " %s", pCType->types.normal.d_attr.tag);
 			}
			else {
				sprintf(buf, " %x", pCType->types.normal.d_attr.st_un);	
			}
			strcat(out, buf);
			break;
		case TS_UNION:
			strcat(out, " union");
			if(pCType->types.normal.d_attr.tag){
				sprintf(buf, " %s", pCType->types.normal.d_attr.tag);
			}
			else {
				sprintf(buf, " %x", pCType->types.normal.d_attr.st_un); 
			}
			strcat(out, buf);
			break;
		case TS_ENUM:
			strcat(out, " enum");
			break;
		case 0:
			break;
		default:
			printf("This should not happen! type_spec %d\n",
				pCType->types.normal.d_attr.type_spec);
			exit(0);
			break;
	}

	if(pCType->types.normal.bitfield_size > 0) {
		char buf[256];
		strcat(out, " : bit size");
		sprintf(buf, " %x", pCType->types.normal.bitfield_size);
		strcat(out, buf);
	}

	if(pCType->v) {
		char buf[256];
		strcat(out, " #value:");
		if(pCType->v->string_t == STR_CHAR) {
			sprintf(buf, " %s", pCType->v->content.s);
		} else if(pCType->v->string_t == STR_LONG) {
			sprintf(buf, " %ls", pCType->v->content.ls);
		}
		else {
			int type_spec = pCType->types.normal.d_attr.type_spec;
			if(type_spec & CHAR_MASK)
				sprintf(buf, " %c", pCType->v->content.c);
			else if((type_spec & LONG_MASK) && (type_spec & INT_MASK))
				sprintf(buf, " 0x%x", pCType->v->content.ull);
			else if((type_spec & DOUBLE_MASK) || (type_spec & FLOAT_MASK)) {
				float f = pCType->v->content.ld;
				sprintf(buf, " %f", f);
			}
			else {
				printf("Constant value error, this shold not happen!\n");
				exit(0);
			}
		}
		strcat(out, buf);
	}

	sprintf(buff, " <size %d", pCType->size_of);
	strcat(out, buff);
	
	//if(pCType->storage_type == STORAGE_LOCAL) 
		sprintf(buff, ", offset %d, ", pCType->offset);
	//else 
		//sprintf(buff, ",");

	strcat(out, buff);

	sprintf(buff, "storage %d>\n", pCType->storage_type);
	
	strcat(out, buff);
}


int gettoken()
{
	char c;
	int ret = yylex();
	if(ret == '#') {
		while( (c= input()) != '\n') ;
		line++;
	} else {
		return ret;
	}
	return  yylex();
}

void restoretoken()
{
    int i;
    /* Copy yytext because unput() trashes yytext */
    char *yycopy = strdup( yytext );
    for ( i = yyleng - 1; i >= 0; --i )
         unput( yycopy[i] );

    free( yycopy );
}

void restoretoken2(char *s)
{
	int i = 0;
	while(s[i++]) ;
	
    while (i) {
      unput( s[i-1] );
      i--;
    }
}

int symtabs_lookup( const char *key, int type, symvalue_t **value)
{
    symtab_t *symtab;
    SElemType *index;

    if(StackEmpty(&g_sym_stack)) {
	   printf("Stack empty\n");
       return R_UNSUCCESS;
    }
	
	index = g_sym_stack.top - 1;
    while(1) {
       symtab = *index;
	   //printf("-------- search table 0x%x\n", symtab);
	   if(symtab != NULL && symtab_lookup(symtab, key, type, value) == R_SUCCESS) {
          return R_SUCCESS;
	   }
	   if(index == g_sym_stack.base) break;
	   index--;
    }

	return R_UNSUCCESS;
}

int type_qualifier()
{
  	int type;
  	if((type =gettoken()) == CONST) {
  		return CONST;
  	}else if (type == VOLATILE) {
  	  return VOLATILE;
  	}
  	restoretoken();  
	  return 0;
}

/*
type_qualifier_list
	: type_qualifier
	| type_qualifier_list type_qualifier
	;
*/

int type_qualifier_list(qualifier *qu)
{
	int n = 0;
	int type;
	
	while(type = type_qualifier()) {
		n++;
		if(type == CONST) {
			qu->q_const = type;
		}
		else {
			qu->q_volatile = type;
		}
	}
	return n;
}

/*
pointer
	: '*'
	| '*' type_qualifier_list
	| '*' pointer
	| '*' type_qualifier_list pointer
	;
*/

CType * pointer()
{
	int type;
	CType *pctype;
	CType *tmp;
	
	if((type =gettoken()) == '*') {
		pctype = create_ctype();
		pctype->datatype = DATATYPE_POINTER;
		if((type =gettoken()) == '*') {
			restoretoken();
			tmp = pointer();
			list_add(&(tmp->link), &(pctype->link));
		} else {
			restoretoken();
			if(type_qualifier_list(&(pctype->types.pointer.qua)) == 0) {
				return pctype;  /* no qualifier */
			}
			if((type =gettoken()) == '*') {
				restoretoken();
				tmp = pointer();
				list_add(&(tmp->link), &(pctype->link));
			} else {
				restoretoken();  
				return pctype;
			}
		}
	}else {    /* not pointer */
		restoretoken();  
		return NULL;
	}
}

/*
parameter-declaration: 
declaration-specifiers declarator
declaration-specifiers abstract-declarator
declaration-specifiers

abstract-declarator:
pointer
direct-abstract-declarator
pointer direct-abstract-declarator


direct-abstract-declarator:
'(' abstract-declarator ')'
direct-abstract-declarator '[' constant-expression ']'
direct-abstract-declarator '[' ']'
'[' constant-expression ']'
direct-abstract-declarator '(' parameter-type-list ')'
direct-abstract-declarator '(' ')'
'(' parameter-type-list ')'


declarator:
direct-declarator
pointer direct-declarator


direct-declarator:
identifier
'(' declarator ')'
direct-declarator '[' constant-expressionopt ']'
direct-declarator '(' parameter-type-list ')'
direct-declarator '(' identifier-list ')'
direct-declarator '(' ')'

*/

int parameter_declaration(CType **param_ctype)
{
	int type;
	CType *tmp_ctype;
	int dt;

	tmp_ctype = create_ctype();
	*param_ctype = tmp_ctype;

	dt = DECLARATOR_BOTH;
	
	if(declaration_specifiers(tmp_ctype, T)) {
 		declarator(tmp_ctype, &dt);

		build_ctype(tmp_ctype);

		if(tmp_ctype->types.normal.d_attr.has_stor_spec && 
			tmp_ctype->types.normal.d_attr.stor_spec != REGISTER) {
			printf("%d: error: storage class specified for parameter\n", line);
			exit(0);
		}
		
		if(tmp_ctype->is_abstract && (tmp_ctype->link.next == NULL)
				&& tmp_ctype->types.normal.d_attr.type_spec == TS_VOID) {
			if(tmp_ctype->types.normal.d_attr.has_stor_spec ||
			    tmp_ctype->types.normal.d_attr.qua.q_const != 0 || 
			    tmp_ctype->types.normal.d_attr.qua.q_volatile != 0) {
				printf("%d: error: \"void\" as only parameter may not be qualified\n", line);
				exit(0);
			}
			return 1;    /* has a void parameter */
		}
		return 0;
	}

	build_ctype(tmp_ctype);
	free_ctype(tmp_ctype);
	return -1;
}

void array2pointer(CType* ct)
{
	struct list_head *curr;
	curr = ct->link.next;
	
	assert(curr != NULL);
	CType * elt = list_entry(curr, CType, link);
	assert(elt->datatype == DATATYPE_ARRAY);
	elt->datatype = DATATYPE_POINTER;

	elt->types.pointer.qua.q_const = 0;	
	elt->types.pointer.qua.q_volatile = 0;	
	elt->types.pointer.is_from_addrop = 0;	
	ct->size_of = 4;
}


/*
parameter-list
  : parameter-declaration
  | parameter-list ',' parameter-declaration
*/

int parameter_list(CType *pCType) 
{
	int type;
	int res;
	bool has_void_param = F;
	CType *param_type;
	symtab_t *symtab;

	int offset = FUNC_STACK_FRAME;

	assert(pCType->datatype == DATATYPE_FUNCTION);

	pCType->types.function.param_list = malloc(MAX_NR_OF_PARAMS * sizeof(CType *));
	if(pCType->types.function.param_list == NULL) {
		error("Out of memory\n");
	}
	
	if(GetTop(&g_sym_stack, (SElemType*)&symtab) != R_SUCCESS) {
		error("Get top of stack failed\n");
	}
		
	while((res = parameter_declaration(&param_type)) >= 0) 
	{
		symvalue_t * value;
		if(param_type->name != NULL) {	
			if(symtab == NULL) { /* symbol table not initialized yet */
				if(symtab_create(5, NULL, NULL, 1, &symtab) != R_SUCCESS) {
					printf("symtab_create failed\n");
					exit(-1);
				}
				if(ModifyTop(&g_sym_stack, symtab) != R_SUCCESS) {
					printf("modify top of stack failed\n");
					exit(-1);
				}
			}
			if(symtab_lookup(symtab, param_type->name, param_type->symtype, &value) == R_SUCCESS) {
				printf("%d: error: parameter name conflict\n", line);
				exit(-1);
			}
			symtab_define(symtab, param_type, symexists_reject, NULL, F);
		} 

		if(is_ctype_array(param_type)) {
			/* Special case for array in parameter */
			/* array in function parameter is treated as pointer */
			array2pointer(param_type);
		}
		pCType->types.function.param_list[pCType->types.function.nr_param++]
			= param_type;
		if(pCType->types.function.nr_param > MAX_NR_OF_PARAMS) {
			printf("error: function has too many paramters\n");
			exit(0);
		}
		
		if(res > 0) has_void_param = T;
		else {
			/* calculate offset */
			if(offset & (ALIGNMENT-1)) {
				if(param_type->size_of <= (ALIGNMENT - ( offset & (ALIGNMENT-1)))) {
					param_type->offset = offset;
					offset += param_type->size_of;
				} else {
					offset += ALIGNMENT - ( offset & (ALIGNMENT-1));
					param_type->offset = offset;
					offset += param_type->size_of;
				}
			} else {
				/* size currently is aligned */
				param_type->offset = offset;
				offset += param_type->size_of;
			} 
			
		}
		
		if(has_void_param && pCType->types.function.nr_param > 1)  {
			/* parameters >1, and has void*/
			printf("error: \"void\" must be the only parameter\n");
			exit(0);
		}
		
		if((type=gettoken()) != ',') {
			restoretoken(); 
			return 0;	
		}

  	}
	
	if(pCType->types.function.nr_param == 0) {
		/* not parameter_declaration() */
		free(pCType->types.function.param_list);
		pCType->types.function.param_list = NULL;
		return 0;
	} 
  
	restoretoken2(",");  /* restore ',' */
	return pCType->types.function.nr_param;
}

/*
parameter-type-list
  : parameter-list
  | parameter-list ',' '...'
*/

void parameter_type_list(CType *pCType)
{
	int type;
	symtab_t *symtab;
	CType * tmp;
	int last_stage = g_cur_stage;

	g_cur_stage = STAGE_PARAM;

	assert(pCType->datatype == DATATYPE_FUNCTION);
	
	if(Push(&g_sym_stack, NULL) != R_SUCCESS) {
		printf("Push to stack failed\n");
        	exit(-1);    
	}
	 	
	if(parameter_list(pCType) == 0) {
		 if(Pop(&g_sym_stack, (SElemType*)&symtab) != R_SUCCESS) {
			printf("Pop from stack failed\n");
			exit(-1);	 
		 } 
		 if(symtab)
		 	pCType->types.function.table = symtab;
		 g_cur_stage = last_stage;
	 	 return;
	 }
	 
	 if(Pop(&g_sym_stack, (SElemType*)&symtab) != R_SUCCESS) {
		printf("Pop from stack failed\n");
		exit(-1);	 
	 }
	 if(symtab)
	 	pCType->types.function.table = symtab;
	 	
	 if((type=gettoken()) == ',') {
	    if((type=gettoken()) == ELLIPSIS) {
		tmp = create_ctype();
		tmp->datatype = DATATYPE_ELLIPSIS;
		build_ctype(tmp);
		assert(pCType->types.function.param_list != NULL);
		pCType->types.function.param_list[pCType->types.function.nr_param++]
			= tmp;
		if(pCType->types.function.nr_param > MAX_NR_OF_PARAMS) {
			printf("error: function has too many paramters\n");
			exit(0);
		}
	    } else {
	    	printf("parse error before %s\n", yytext);
                exit(0);
            }
            g_cur_stage = last_stage;
            return;
	 }
	 
         printf("parse error before %s\n", yytext);
         exit(0);
}

/*
identifier-list
  : identifier
  | identifier-list ',' identifier
*/

void identifier_list(CType *pCType)
{
	int type;
	int cnt = 0;
	CType * tmp;
	symtab_t *symtab = NULL;
	
	assert(pCType->datatype == DATATYPE_FUNCTION);
	
	pCType->types.function.param_list = malloc(MAX_NR_OF_PARAMS * sizeof(CType *));
	if(pCType->types.function.param_list == NULL) {
		error("Out of memory\n");
	}

	while((type=gettoken()) == IDENTIFIER) 
	{
		tmp = create_ctype();
		tmp->name = store_name(yytext, yyleng);
		build_ctype(tmp);
		pCType->types.function.param_list[pCType->types.function.nr_param++]
			= tmp;
		if(pCType->types.function.nr_param > MAX_NR_OF_PARAMS) {
			printf("error: function has too many paramters\n");
			exit(0);
		}
		pCType->types.function.is_old_style = T;
		if(tmp->name != NULL) {	
			if(symtab == NULL) { /* symbol table not initialized yet */
				if(symtab_create(5, NULL, NULL, 1, &symtab) != R_SUCCESS) {
					printf("symtab_create failed\n");
					exit(-1);
				}
			}
			symvalue_t * value;
			if(symtab_lookup(symtab, tmp->name, 
				SYM_TYPE_OBJECT, &value) == R_SUCCESS) {
				//printf("find same name in id_list\n");
				pCType->types.function.has_same_id = T;
			}
			symtab_define(symtab, tmp, symexists_add, NULL, F);
		}		
		if((type=gettoken()) != ',') {
			restoretoken(); 
			if(symtab)
	 			pCType->types.function.table = symtab;
			return;	
		}
	}
  
	if(pCType->types.function.nr_param == 0) {   /* no identifiers */
		restoretoken();
		free(pCType->types.function.param_list);
		pCType->types.function.param_list = NULL;
		return;
	}
  
	restoretoken();
	restoretoken2(",");
	if(symtab)
	 	pCType->types.function.table = symtab;	
	return;
}

#if 1
/*
struct_or_union_specifier
	:
	|struct_or_union identifier '{' struct_declaration_list '}'
	|struct_or_union '{' struct_declaration_list '}'
	|struct_or_union identifier
*/

#define NOT_STRUCT_OR_UNION   0
#define NEW_STRUCT_OR_UNION_NO_TAG   1
#define NEW_STRUCT_OR_UNION_WITH_TAG    2
#define EXIST_STRUCT_OR_UNION_ONLY_TAG   3
#define NEW_STRUCT_OR_UNION_ONLY_TAG  4

int struct_or_union_specifier(CType * pCType, CType **ct_out, bool tag_must_def_before)
{
	int type, st_or_un;
	CType * tmp;
	char *tmp_name;
	int datatype;
	symvalue_t * value = NULL;
	
	if(st_or_un = struct_or_union()) {
		datatype = (st_or_un == STRUCT ? DATATYPE_STRUCT: DATATYPE_UNION);
		type = gettoken();
		if(type == '{') {
			tmp = create_ctype();
			*ct_out = tmp;
			tmp->datatype = datatype;
			tmp->symtype = SYM_TYPE_TAG;
			struct_declaration_list(tmp);
			if((type = gettoken()) != '}') {
				printf("error: parse error before %s\n", yytext);
				exit(0);
			}
			tmp->types.struct_union_enum.attach_to = pCType;
			return NEW_STRUCT_OR_UNION_NO_TAG;
		} else if(type == IDENTIFIER) {			
			tmp_name = store_name(yytext, yyleng);
			if((type = gettoken()) == '{') {
				symtab_t *symtab;
				if(GetTop(&g_sym_stack, (SElemType*)&symtab) != R_SUCCESS) {
					error("Get top of stack failed\n");
				}
				if(symtab != NULL) {
					if(symtab_lookup(symtab, tmp_name, SYM_TYPE_TAG, &value) == R_SUCCESS) {
						if(value->datatype != datatype) {
							printf("error: a tag '%s' found before\n", yytext);
							exit(0);
						}
					}
				}
				if(value ) {
					if(value->types.struct_union_enum.table != NULL) {
						printf("error: a tag '%s' found before\n", tmp_name);
						exit(0);
					}
				}

				if(symtab == NULL){
					if(symtab_create(5, NULL, NULL, 1, &symtab) != R_SUCCESS) {
						printf("symtab_create failed\n");
						exit(-1);
					}
					if(ModifyTop(&g_sym_stack, symtab) != R_SUCCESS) {
						printf("modify top of stack failed\n");
						exit(-1);
					}
				}
				if(value) {
					*ct_out = value;
					tmp = value;
				}
				else {
					tmp = create_ctype();
					*ct_out = tmp;
					tmp->datatype = datatype;
					tmp->symtype = SYM_TYPE_TAG;
					tmp->name = tmp_name;
					symtab_define(symtab,  tmp, symexists_add, NULL, F);
				}

				struct_declaration_list(tmp);
				if((type = gettoken()) != '}') {
					printf("error: parse error before %s\n", tmp_name);
					exit(0);
				}

				return NEW_STRUCT_OR_UNION_WITH_TAG;
			}
			else {
				restoretoken();
				if(symtabs_lookup(tmp_name, SYM_TYPE_TAG, &value) == R_SUCCESS) {
					if(value->datatype != datatype) {
						printf("error: a tag '%s' type mismatch\n", tmp_name);
						exit(0);
					}
				} else {
					if(tag_must_def_before == T) {
						printf("error: tag '%s' has not defined before\n", tmp_name);
						exit(0);
					}
				}
				if(value ) {
					*ct_out = value;
					return EXIST_STRUCT_OR_UNION_ONLY_TAG;
				}
				
				tmp = create_ctype();
				*ct_out = tmp;
				tmp->datatype = datatype;
				tmp->symtype = SYM_TYPE_TAG;
				tmp->name = tmp_name;

				symtab_t *symtab;
					
				if(GetTop(&g_sym_stack, (SElemType*)&symtab) != R_SUCCESS) {
						error("Get top of stack failed\n");
				}	
				if(symtab == NULL){
					if(symtab_create(5, NULL, NULL, 1, &symtab) != R_SUCCESS) {
						printf("symtab_create failed\n");
						exit(-1);
					}
					if(ModifyTop(&g_sym_stack, symtab) != R_SUCCESS) {
						printf("modify top of stack failed\n");
						exit(-1);
					}
				}
				symtab_define(symtab,  tmp, symexists_replace, free_ctype, F);
				
				return NEW_STRUCT_OR_UNION_ONLY_TAG;
			}
		} else {
			printf("error: parse error before %s\n", yytext);
			exit(0);
		}
	}
	
	return  NOT_STRUCT_OR_UNION; /* not struct_or_union_specifier*/
}

/*
struct_or_union
	:
	|'struct' 
	|'union'
*/

int struct_or_union()
{
  	int type;
  	if((type =gettoken()) == STRUCT) {
  		return STRUCT;
  	}else if (type == UNION) {
  	  return UNION;
  	}
  	restoretoken();  
	return 0;
}


/*
struct_declaration_list
	:
	|struct_declaration
	|struct_declaration_list struct_declaration
*/

void struct_declaration_list(CType * pctype) 
{
	symtab_t *symtab;
	CType_list * ct_list, *tmp;
	CType * ct_tmp;
	int i = 0;
	CType_list * head = create_ctype_list();

	assert(pctype->symtype == SYM_TYPE_TAG );

	if(symtab_create(5, NULL, NULL, 1, &(pctype->types.struct_union_enum.table)) 
		!= R_SUCCESS) {
		printf("symtab_create failed\n");
		exit(-1);
	}
	symtab = (symtab_t *)(pctype->types.struct_union_enum.table);
	
	while(1) {
		if((ct_list = struct_declaration()) !=  NULL) {
			symvalue_t * value;
			struct list_head *curr;
			if(ct_list == (CType_list *)(!NULL)) continue;

			list_for_each(curr, &(ct_list->link)){ 
				tmp = list_entry(curr, CType_list, link);
				ct_tmp = tmp->ct; 

				symvalue_t * value;
				i++;

				assert(ct_tmp->datatype == DATATYPE_NORMAL);
				if(ct_tmp->datatype == DATATYPE_NORMAL && 
					ct_tmp->types.normal.d_attr.has_stor_spec == T) {
					printf("error: declaration has storage type in struct\n");
					exit(0);
				}

				if(ct_tmp->link.next != NULL) {
					CType *tmp_ct;
					tmp_ct = list_entry(ct_tmp->link.next, CType, link);
					if(tmp_ct->datatype == DATATYPE_FUNCTION)
					{
						printf("error: bit-field could not be function type\n");
						exit(0);
					}
				}
						
				if(ct_tmp->types.normal.bitfield_size > 0) {
					if(ct_tmp->types.normal.d_attr.type_spec  & FLOAT_MASK 
						|| ct_tmp->types.normal.d_attr.type_spec  & DOUBLE_MASK 
						|| ct_tmp->types.normal.d_attr.type_spec  & STRUCT_MASK
						|| ct_tmp->types.normal.d_attr.type_spec  & UNION_MASK
						|| ct_tmp->types.normal.d_attr.type_spec  & VOID_MASK)
						{
							printf("error: bit-field has invalid type\n");
							exit(0);
						}
						if(ct_tmp->link.next != NULL)
						{
							printf("error: bit-field has invalid type2\n");
							exit(0);
						}
				}
					
				if(ct_tmp->name != NULL) {	   	
					if(symtab_lookup(symtab, ct_tmp->name, 
						SYM_TYPE_OBJECT, &value) == R_SUCCESS) {
						printf("error: declaration name conflict in struct or union\n");
						exit(-1);
					}
					symtab_define(symtab, ct_tmp, symexists_reject, NULL, F);
				} else {
					if(ct_tmp->types.normal.bitfield_size <= 0) {
						printf("error: declaration has no name\n");
						exit(0);
					}
					else free_ctype(ct_tmp);   /* FIXME: how to use bit field, like int:1;*/
				}
			}
			/* concatenate this  ct_list to head */
			list_concatenate(&(head->link), &(ct_list->link));
			head->cnt += ct_list->cnt;
			list_del(&(ct_list->link));
			free(ct_list);
			
		}
		else break;
	}

	if(i == 0){
		pctype->types.struct_union_enum.is_empty = T;
		symtab_destroy(&(pctype->types.struct_union_enum.table), free_ctype);
		pctype->types.struct_union_enum.table = NULL;
	}

	/* create the struct or union 'item_list' from 'head' */
	assert(pctype->datatype == DATATYPE_STRUCT || 
		pctype->datatype == DATATYPE_UNION);
	pctype->types.struct_union_enum.item_list 
		= malloc(head->cnt * sizeof(CType *));
	pctype->types.struct_union_enum.items = head->cnt;
	
	struct list_head *curr;
	i = 0;
	list_for_each(curr, &(head->link)){ 	
		tmp = list_entry(curr, CType_list, link);
		pctype->types.struct_union_enum.item_list[i] = tmp->ct;
		tmp->ct->parent = pctype;
		tmp->ct->idx = i++;
	}
	assert(i == pctype->types.struct_union_enum.items);
	free_ctype_list(head);
	
#ifdef  DEBUG 	
        /* print structrue info */
	if(pctype->name) printf("%s: ", pctype->name);
	printf(" %s {\n", pctype->datatype == DATATYPE_STRUCT ? "struct": "union");
#endif

	int size = 0;
	int max = 0;

	for(i = 0; i<pctype->types.struct_union_enum.items; i++) {
		CType * ct = pctype->types.struct_union_enum.item_list[i];
		if(pctype->datatype == DATATYPE_STRUCT) {
			if(size & (ALIGNMENT-1)) {
				if(ct->size_of <= (ALIGNMENT - ( size & (ALIGNMENT-1)))) {
					ct->offset = size;
					size += ct->size_of;
				} else {
					size += ALIGNMENT - ( size & (ALIGNMENT-1));
					ct->offset = size;
					size += ct->size_of;
				}
			} else {
				/* size currently is aligned */
				ct->offset = size;
				size += ct->size_of;
			}
		} else {
			if(ct->size_of > max)
				max = ct->size_of;
			ct->offset = 0;
		}
#ifdef  DEBUG			
		out2[0] = '\0';
		printf("\t");
		print_ctype(pctype->types.struct_union_enum.item_list[i], out2);
		printf(out2);
		printf("\n\r");
#endif
		
	}

	if(pctype->datatype == DATATYPE_STRUCT) {
		if(size <= 4 && pctype->types.struct_union_enum.items == 1) { 
			// the struct has only one basic item 
			CType * ct = pctype->types.struct_union_enum.item_list[0];
			size = ct->size_of;
		} else {
			if(size & (ALIGNMENT-1))
				size += ALIGNMENT - ( size & (ALIGNMENT-1)); 
		}
		pctype->size_of = size;  
	} else 
		pctype->size_of = max;
		
#ifdef  DEBUG	
	printf("}\n\r");
	printf("@tag@ size %d\n\r", pctype->size_of);
#endif

}

/*
struct_declaration
	:
	|specifier_qualifier_list struct_declarator_list;

specifier_qualifier_list
	:
	|type_specifier specifier_qualifier_list
	|type_specifier 
	|type_qualifier specifier_qualifier_list
	|type_qualifier

struct_declarator_list
	:
	|struct_declarator
	|struct_declarator_list ',' struct_declarator

struct_declarator
	:
	|declarator
	|declarator  ':' constant_expression
	| ':' constant_expression
*/

void struct_declarator(CType * pctype)
{
	int dt = DECLARATOR_DIRECT;
	int type;
	CType *tmp;

	type = gettoken();
	if(type == ':') {
		// do nothing		
	} else {
		restoretoken();
		declarator(pctype, &dt);
		if((type = gettoken()) != ':') {
			restoretoken();
			return;
		}
	}

	if(constant_expr(&tmp) == 0) {
		printf("error: parse error before '%s'\n", yytext);
		exit(0);
	}
	if(tmp->v == NULL) {
		printf("error: bit field size is unknown\n");
		exit(0);
	}
	if(tmp->types.normal.d_attr.type_spec & INT_MASK == 0) {
		printf("error: bit field size not integer constant\n");
		exit(0);
	}

	assert(pctype->datatype == DATATYPE_NORMAL);
	pctype->types.normal.bitfield_size = (int)tmp->v->content.ull;
	if(pctype->types.normal.bitfield_size <= 0) {
		printf("error: bitfield size error\n");
		exit(0);
	}
	return;
}

CType_list * struct_declarator_list(CType * pctype)
{
	CType_list * tmp, *new;
	int type;
	CType ct_save;

	assert(pctype->link.next == pctype->link.prev);
	memcpy(&ct_save, pctype, sizeof(CType));
	
	tmp = create_ctype_list();	
	
	struct_declarator(pctype);
	new = create_ctype_list();
	new->ct = pctype;
	list_add_tail(&(new->link), &(tmp->link));
	tmp->cnt++;

	while((type = gettoken()) == ',') {
		CType *ct = create_ctype();
		CType_cpy(ct, &ct_save, F);
		
		struct_declarator(ct);
		new = create_ctype_list();
		new->ct = ct;
		list_add_tail(&(new->link), &(tmp->link));	
		tmp->cnt++;
	}

	restoretoken();
	return tmp;
}

CType_list * struct_declaration()
{
	int type;
	int tag_status;
	int ret;
	CType * pCType;
	CType_list * ct_list, *tmp;
	pCType = create_ctype();
			
	if(ret = declaration_specifiers(pCType, F)) {
		if(pCType->datatype == DATATYPE_NORMAL && 
			pCType->types.normal.d_attr.has_stor_spec == T) {
			printf("error: declaration has storage type in struct\n");
			exit(0);
		}
		if((type = gettoken()) == ';') {
			if(((ret == STRUCT || ret == UNION))
				|| ret == ENUM) {
				/* do nothing */
			}
			else printf("warning: empty declaration\n");
			build_ctype(pCType);
			free_ctype(pCType);
			return  (CType_list*)(!NULL);
		}  
		restoretoken();
		ct_list = struct_declarator_list(pCType);
		if((type = gettoken()) != ';') {
		   printf("error: parse error before %s\n", yytext);
		   exit(0);
		}
	} else {
		//printf("not declaration\n");
		build_ctype(pCType);
		free_ctype(pCType);
		return NULL;	
	}

	struct list_head *curr;
	list_for_each(curr, &(ct_list->link)){ 	
		tmp = list_entry(curr, CType_list, link);
		pCType = tmp->ct;
		build_ctype(pCType);
	}

	return ct_list;
}

#endif
/*
declarator
	: declarator2
	| pointer declarator2
	;

declarator2
	: identifier
	| '(' declarator ')'
	| declarator2 '[' ']'
	| declarator2 '[' constant_expr ']'
	| declarator2 '(' ')'
	| declarator2 '(' parameter_type_list ')'
	| declarator2 '(' identifier_list ')'
	;
*/

void declarator2(CType *pCType, int *dt)
{
	int type;
	int res;
	int constant;
	int last_is_func = F;
	int last_is_array = F;
	CType *tmp;
	
	if((type =gettoken()) == '(') {
		declarator(pCType, dt);
		if((type =gettoken()) != ')') {
			printf("error: parse error before %s\n", yytext);
		   	exit(0);
		}
	} else if(type == IDENTIFIER) {  /* variable name */
		if(*dt == DECLARATOR_ABSTRACT) {
			printf("error: abstract declarator should has no identifier\n");
			exit(0);
		}
		if(*dt == DECLARATOR_DIRECT || *dt == DECLARATOR_BOTH) {
			pCType->name = store_name(yytext,  yyleng);
			pCType->line = line;
		} else {
			printf("This should not happen\n");
			exit(0);	    
		}  
	}else {
		if(*dt == DECLARATOR_BOTH || *dt == DECLARATOR_ABSTRACT ) {
			*dt = DECLARATOR_ABSTRACT;
			pCType->is_abstract = T;
			restoretoken();
		} else {
			printf("%d: error: expected name or(dcl), type %d\n", __LINE__, type);
			exit(0);
		}
	}
	
	while((type = gettoken())== '(' || type == '[') {
	   	 if(type == '('){
	   	 	if(last_is_func)
	   	 	{
	   	 		printf("error: function can't return function\n");
	   	 		exit(0);
	   	 	}
	   	 	if(last_is_array)
	   	 	{
	   	 		printf("error: array of function is not possible\n");
	   	 		exit(0);   	 	  	
	   	 	}
	   	 	last_is_func = T;

			tmp = create_ctype();
			tmp->datatype = DATATYPE_FUNCTION;
	        
			if((type=gettoken()) == ')')
				restoretoken();
			else if(type == IDENTIFIER) {
				symvalue_t * value;
				if(symtabs_lookup(yytext, SYM_TYPE_TYPEDEF, &value) == R_SUCCESS) {
					restoretoken();
					parameter_type_list(tmp);
	 			} else {

					if(*dt == DECLARATOR_ABSTRACT) {
						printf("parse error before %s\n", yytext);
						exit(0);        	    	
	        			}
					restoretoken();
					identifier_list(tmp); 
				}
	        	} else {
	           		restoretoken();
	           		parameter_type_list(tmp);
	        	}
	        
	       		list_add_tail(&(tmp->link), &(pCType->link));
	        
	  		if((type=gettoken()) != ')') {
	  	  		printf("error: parse error before %s\n", yytext);
	    			exit(0);
	    		}
	     	} else if(type == '[') {
	   	 	if(last_is_func)
	   	 	{
	   	 	  	printf("error: function can't return array\n");
	   	 	  	exit(0);
	   	 	}
	   	 	last_is_array = T; 	
	     	
			tmp = create_ctype();
			tmp->datatype = DATATYPE_ARRAY;

	        	/* constant or not */
			CType *tmp2;
	        	if(constant_expr(&tmp2) == 0) {
					tmp->types.array.is_pointer_alike = T;
					tmp->types.array.is_p_alike_init = T;
	        	} else {
	        		assert(tmp2->datatype == DATATYPE_NORMAL);
				if(tmp2->v == NULL) {
					printf("error: array size is unknown\n");
					exit(0);
				}
				if(tmp2->types.normal.d_attr.type_spec & INT_MASK == 0) {
					printf("error: array size not integer constant\n");
					exit(0);
				}
				tmp->types.array.array_size = (int)tmp2->v->content.ull;
				//array  size  (int)tmp2->v->content.ull;
	        	}
	        
		    	list_add_tail(&(tmp->link), &(pCType->link));
		 	if((type=gettoken()) != ']') {
	 			printf("parse error befor %s\n", yytext);
		         	exit(0);
			}
		} 
	}
	restoretoken();  // next token not '(' or '[', declarator2 end, restore
	return;
}

void declarator(CType *pCType, int *dt)
{
   CType *tmp;
   
   tmp = pointer();
   declarator2(pCType, dt);

   if(tmp != NULL) {
        list_concatenate(&(pCType->link), &(tmp->link));
   }
}


/*
enum_specifier
	:
	|'enum' identifier  '{' enumerator_list '}'
	|'enum' '{' enumerator_list  '}'
	|'enum' identifier
	
enumerator_list
	:
	|enumerator
	|enumerator_list  ','  enumerator

enumerator
	:
	|identifier
	|identifier  '='  constant_expression

*/

void enumerator_list(CType * in_ct)
{
	int type;
	int n = 0;
	int enum_value = 0;
	CType *pctype;
	char name[256];
		
	while((type = gettoken()) == IDENTIFIER) {
		strcpy(name, yytext);
		n++;
		if((type = gettoken()) == '=') {
			CType *tmp;
			if(constant_expr(&tmp) == 0) {
				printf("error: parse error before '%s'\n", yytext);
				exit(0);
			}
			if(tmp->v == NULL) {
				printf("error: enumerator value is unknown\n");
				exit(0);
			}
			if(tmp->types.normal.d_attr.type_spec & INT_MASK == 0) {
				printf("error: enumerator value for '%s' not integer constant\n", name);
				exit(0);
			}
			enum_value = (int)tmp->v->content.ull;
		}
		else restoretoken();

		/* create a enum ID */
		symvalue_t * value;
		if(symtabs_lookup(name, SYM_TYPE_TYPEDEF|SYM_TYPE_OBJECT, &value) == R_SUCCESS) {
			printf("error: enum ID '%s' is decleared before \n", name);
			exit(0);
		}
		pctype = create_ctype();
		pctype->name = store_name(name, strlen(name));
		pctype->symtype = SYM_TYPE_OBJECT;
		pctype->types.normal.d_attr.has_type_spec = T;
		pctype->types.normal.d_attr.type_spec = TS_ENUM;
		pctype->types.normal.d_attr.has_enum_val = T;     /* ENUM ID itself */
		pctype->types.normal.d_attr.enum_val = enum_value;
		pctype->link.next = NULL;

#ifdef  DEBUG 
		out2[0] = '\0';
		print_ctype(pctype, out2);
		printf(out2);
		printf("\n\r");
#endif
		symtab_t *symtab;
		if(GetTop(&g_sym_stack, (SElemType*)&symtab) != R_SUCCESS) {
			error("Get top of stack failed\n");
		}		
				
		assert(symtab != NULL);
				
		symtab_define(symtab,  pctype, symexists_reject, NULL, F);
				
		if((type = gettoken()) != ',') {
			restoretoken();
			break;
		}
		enum_value++;
	}

	if(n == 0) {
		printf("error: empty enum list\n");
		exit(0);
	}

	/* enum symtab is only used for check if enum TAG re-declear */
	in_ct->types.struct_union_enum.table = (struct symtab*)!(NULL);    /* FIXME */
}

#define NOT_ENUM   0
#define NEW_ENUM_NO_TAG   1
#define NEW_ENUM_WITH_TAG    2
#define EXIST_ENUM_ONLY_TAG   3
#define NEW_ENUM_ONLY_TAG  4

int  enum_specifier(CType **ct_out, bool tag_must_def_before)
{
	int type, st_or_un;
	CType * tmp;
	char *tmp_name;
	symvalue_t * value = NULL;

	type = gettoken();
	if(type == '{') {
		tmp = create_ctype();
		*ct_out = tmp;
		tmp->datatype = DATATYPE_ENUM;
		tmp->symtype = SYM_TYPE_TAG;
		enumerator_list(tmp);
		if((type = gettoken()) != '}') {
			printf("error: parse error before %s\n", yytext);
			exit(0);
		}
		return NEW_ENUM_NO_TAG;
	} else if(type == IDENTIFIER) {		
		tmp_name = store_name(yytext, yyleng);
		if((type = gettoken()) == '{') {
			symtab_t *symtab;
			if(GetTop(&g_sym_stack, (SElemType*)&symtab) != R_SUCCESS) {
				error("Get top of stack failed\n");
			}
			if(symtab != NULL) {
				if(symtab_lookup(symtab, tmp_name, SYM_TYPE_TAG, &value) == R_SUCCESS) {
					if(value->datatype != DATATYPE_ENUM) {
						printf("error: a tag '%s' found before\n", tmp_name);
						exit(0);
					}
				}
			}			
			if(value ) {
				if(value->types.struct_union_enum.table != NULL) {
					printf("error: a tag '%s' found before\n", tmp_name);
					exit(0);
				}
			}
			tmp = create_ctype();
			*ct_out = tmp;
			tmp->datatype = DATATYPE_ENUM;
			tmp->symtype = SYM_TYPE_TAG;
			tmp->name = tmp_name;

			if(symtab == NULL){
				if(symtab_create(5, NULL, NULL, 1, &symtab) != R_SUCCESS) {
					printf("symtab_create failed\n");
					exit(-1);
				}
			}
			symtab_define(symtab,  tmp, symexists_replace, free_ctype, F);

			enumerator_list(tmp);
			if((type = gettoken()) != '}') {
				printf("error: parse error before %s\n", tmp_name);
				exit(0);
			}
			
			return NEW_ENUM_WITH_TAG;
		}else {
			restoretoken();
			symtab_t *symtab;
			if(symtabs_lookup(tmp_name, SYM_TYPE_TAG, &value) == R_SUCCESS) {
				if(value->datatype != DATATYPE_ENUM) {
					printf("error: a tag '%s' type mismatch\n", tmp_name);
					exit(0);
				}
			} else {
				if(tag_must_def_before == T) {
					printf("error: tag '%s' has not defined before\n", tmp_name);
					exit(0);
				}
			}

			if(value ) {
				*ct_out = value;
				return EXIST_ENUM_ONLY_TAG;
			}
				
			tmp = create_ctype();
			*ct_out = tmp;
			tmp->datatype = DATATYPE_ENUM;
			tmp->symtype = SYM_TYPE_TAG;
			tmp->name = tmp_name;

			if(GetTop(&g_sym_stack, (SElemType*)&symtab) != R_SUCCESS) {
					error("Get top of stack failed\n");
			}	
			if(symtab == NULL){
				if(symtab_create(5, NULL, NULL, 1, &symtab) != R_SUCCESS) {
					printf("symtab_create failed\n");
					exit(-1);
				}
				if(ModifyTop(&g_sym_stack, symtab) != R_SUCCESS) {
					printf("modify top of stack failed\n");
					exit(-1);
				}
			}
			symtab_define(symtab,  tmp, symexists_replace, free_ctype, F);
			
			return NEW_ENUM_ONLY_TAG; 
		}
	} else {
		printf("error: parse error before %s\n", yytext);
		exit(0);
	}
	
	return  0; /* not enum_specifier*/
}


int type_specifier(CType *pCType, CType **ct_out, bool tag_must_def_before)
{
	int type;
	int ret;
	*ct_out = NULL;
   
	type=gettoken();
	switch(type)
	{
		case VOID:
			return VOID;
		case CHAR:
   	  		return CHAR;
		case SHORT:
			return SHORT;
		case INT:
			return INT;
		case LONG:
			return LONG;
		case FLOAT:
			return FLOAT;
		case DOUBLE:
			return DOUBLE;
		case SIGNED:
			return SIGNED;
		case UNSIGNED:
			return UNSIGNED;
		case IDENTIFIER:	/* should be typedef-name */
			return IDENTIFIER;
		default:
			break;
	}

	restoretoken();
	ret = struct_or_union_specifier(pCType, ct_out, tag_must_def_before);
	if(ret != NOT_STRUCT_OR_UNION)
		return type;	/* type is 'struct' or 'union' */

	if((type = gettoken()) == ENUM) {
		ret = enum_specifier(ct_out, tag_must_def_before);
		return type;	/* type is ENUM */
	}
	
	restoretoken();
	return 0;
}

int storage_class_specifier()
{
   int type;
   
   type=gettoken();
   switch(type)
   {
   	  case AUTO:
   	  	return AUTO;
   	  case REGISTER:
   	  	return REGISTER;
   	  case STATIC:
   	  	return STATIC;
   	  case EXTERN:
   	 	return EXTERN;
   	  case TYPEDEF:
   	 	return TYPEDEF;
   	  default:
   	 	break;
   }
   restoretoken();
   return 0;   
}

/*
declaration_specifiers
	: storage_class_specifier
	| storage_class_specifier declaration_specifiers
	| type_specifier
	| type_specifier declaration_specifiers
	| type_qualifier
	| type_qualifier declaration_specifiers
	;
*/

int declaration_specifiers(CType *pCType, bool tag_must_def_before)
{
	int type;
	int tag_st;
	CType *tmp;

	assert(pCType->datatype == DATATYPE_NORMAL);

	if(type = storage_class_specifier()) {
		if(pCType->types.normal.d_attr.has_stor_spec == T ) {
			printf("error: two or more storage class specifiers\n");
			exit(0);
		}
		pCType->types.normal.d_attr.has_stor_spec = T;
		pCType->types.normal.d_attr.stor_spec = type;
		declaration_specifiers(pCType, tag_must_def_before);
	}
	else if(type = type_specifier(pCType, &tmp, tag_must_def_before)) {
		if(type == IDENTIFIER) {
			if(pCType->types.normal.d_attr.has_type_spec == T) {  /* this should be identifiler name */
				restoretoken();
				return 1;
			}
			symvalue_t * value;
		 
			if(symtabs_lookup(yytext, SYM_TYPE_TYPEDEF, &value) == R_SUCCESS) {
				if(symtabs_lookup(yytext, SYM_TYPE_OBJECT, &value) == R_SUCCESS) {
					/* it's not a declaration_specifiers */
					restoretoken();
					return 0;
				}		
				pCType->get_from = value;
				pCType->types.normal.d_attr.has_type_spec = T;
				pCType->types.normal.d_attr.is_typedef_name = T;
			}else {
				restoretoken();
				return 0; 
			}
		 
		} else if(type == SIGNED || type == UNSIGNED) {
			if(pCType->types.normal.d_attr.is_typedef_name == T) {
				printf("error: data definition should has no 'signed' or 'unsigned' specifier\n");
				exit(0);
			}
			if(pCType->types.normal.d_attr.type_spec & (SIGNED_MASK | UNSIGNED_MASK)) {
				printf("error: 'signed' or 'unsigned' used error\n");
				exit(0);
			} else {
				if((pCType->types.normal.d_attr.type_spec & FLOAT_MASK) 
					|| pCType->types.normal.d_attr.type_spec & DOUBLE_MASK
					|| pCType->types.normal.d_attr.type_spec & (SUE_MASK | VOID_MASK)){
					printf("error: 'signed' or 'unsigned' sepcify error\n");
					exit(0);
				}
				else {
					if(type == SIGNED)
						pCType->types.normal.d_attr.type_spec |= SIGNED_MASK;
					else 
						pCType->types.normal.d_attr.type_spec |= UNSIGNED_MASK;
					pCType->types.normal.d_attr.has_type_spec = T;
				}
			}
		} else if(type == LONG) { 
			if(pCType->types.normal.d_attr.is_typedef_name == T) {
				printf("error: data definition should has no 'long' specifier\n");
				exit(0);
			}
			int tmp = 
				pCType->types.normal.d_attr.type_spec & (~(SIGNED_MASK | UNSIGNED_MASK));

			if(tmp == 0 || tmp == TS_INT || tmp == TS_DOUBLE) {
				pCType->types.normal.d_attr.type_spec |= LONG_MASK;
				pCType->types.normal.d_attr.has_type_spec = T;
			}else if(tmp == TS_LONG || tmp == TS_LONG_INT){
				pCType->types.normal.d_attr.type_spec |= (LONG_MASK<<1);
				pCType->types.normal.d_attr.has_type_spec = T;
			}else if(tmp == TS_LONG_LONG || tmp == TS_LONG_LONG_INT ) {
				printf("error: too long data type\n");
				exit(0);
			}else {
				printf("error: 'long' sepcify error\n");
				exit(0);
			}

		} else if (type == SHORT) {
			if(pCType->types.normal.d_attr.is_typedef_name == T) {
				printf("error: data definition should has no 'short' specifier\n");
				exit(0);
			}
			if(pCType->types.normal.d_attr.type_spec & SHORT_MASK) {
				printf("error: duplicate 'short' specifier\n");
				exit(0);
			}
			int tmp = 
				pCType->types.normal.d_attr.type_spec & (~(SIGNED_MASK | UNSIGNED_MASK));
			if(tmp == 0 || tmp == INT) {
				pCType->types.normal.d_attr.type_spec |= SHORT_MASK;
				pCType->types.normal.d_attr.has_type_spec = T;
			}else {
				printf("error: 'short' sepcify error\n");
				exit(0);
			}
		} else if(type == STRUCT || type == UNION) {

			if(pCType->types.normal.d_attr.has_type_spec == T ) {
				printf("error: two or more type specifiers\n");
				exit(0);
			}
	
			pCType->types.normal.d_attr.has_type_spec = T;
			if(type == STRUCT)
				pCType->types.normal.d_attr.type_spec = TS_STRUCT;
			else 
				pCType->types.normal.d_attr.type_spec = TS_UNION;
			if(tmp->name)
				pCType->types.normal.d_attr.tag = store_name(tmp->name, strlen(tmp->name));
			pCType->types.normal.d_attr.st_un = tmp;
			
		} 
		else if(type == ENUM) {

			if(pCType->types.normal.d_attr.has_type_spec == T ) {
				printf("error: two or more type specifiers\n");
				exit(0);
			}
			pCType->types.normal.d_attr.type_spec = TS_ENUM;
			pCType->types.normal.d_attr.has_type_spec = T;		
		}
		else {  /* int, char, float, double */
			if(pCType->types.normal.d_attr.is_typedef_name == T
				|| pCType->types.normal.d_attr.type_spec & (SUE_MASK | VOID_MASK)) {
				printf("error: two or more type specifiers\n");
				exit(0);
			} 

			int tmp = 
				pCType->types.normal.d_attr.type_spec & (~(SIGNED_MASK | UNSIGNED_MASK));

			if(type == INT) { 
				if(tmp == 0 || tmp == TS_SHORT || tmp == TS_LONG || tmp == TS_LONG_LONG) {
					pCType->types.normal.d_attr.type_spec |= INT_MASK;
				}else {
					printf("error: two or more type specifiers\n");
					exit(0);
				}
			} else if(type == CHAR) {
				if(tmp == 0) {
					pCType->types.normal.d_attr.type_spec |= CHAR_MASK;
				}else {
					printf("error: two or more type specifiers\n");
					exit(0);
				}
			} else if(type == FLOAT) {
				if(pCType->types.normal.d_attr.has_type_spec == T) {
					printf("error: two or more type specifiers\n");
					exit(0);
				}
				pCType->types.normal.d_attr.type_spec |= TS_FLOAT;
			} else if(type == DOUBLE) {
				if(pCType->types.normal.d_attr.type_spec == 0 ||
					pCType->types.normal.d_attr.type_spec == TS_LONG) {
					pCType->types.normal.d_attr.type_spec |= DOUBLE_MASK;
				} else {
					printf("error: two or more type specifiers\n");
					exit(0);
				}
			} else if(type == VOID) {
				if(pCType->types.normal.d_attr.has_type_spec == T) {
					printf("error: two or more type specifiers\n");
					exit(0);
				}
				pCType->types.normal.d_attr.type_spec = TS_VOID;
			}
		
			pCType->types.normal.d_attr.has_type_spec = T;
		}
		declaration_specifiers(pCType, tag_must_def_before);
	}
	else if(type = type_qualifier()) {
		if(type == CONST) 
			pCType->types.normal.d_attr.qua.q_const = CONST;
		else 
			pCType->types.normal.d_attr.qua.q_volatile = VOLATILE;
		declaration_specifiers(pCType, tag_must_def_before);
   	  
	}
	else {
		return 0;
	}
	
	return type;
}


#if 1
/*
designator
	:
	| '[' constant-expression ']'
	| '.' identifier
	;
	
*/
int designator_list(CType *pctype, CType **out);
void get_first_field(CType *pctype, CType **out);	
void get_next_field(CType *pctype, CType **out); 
int initializer(CType *pctype, CType **next_field, bool first);
int initializer_list(CType *pctype, CType *first, CType ** out_next_field);
void  pre_init(CType * pCType);
void post_init(CType * pCType);
void print_initializer(CType * curr_field, CType * p);

//#ifdef DEBUG
   char  init_dbg_buf[1024];

void print_decl(CType * curr_field)
{
	if(curr_field->parent != NULL) {
		if(curr_field->parent->datatype == DATATYPE_STRUCT ||
			curr_field->parent->datatype == DATATYPE_UNION)
			print_decl(curr_field->parent->types.struct_union_enum.associate_with);
		else
			print_decl(curr_field->parent);
	}
	
	if(curr_field->name){
		char buf[100];
		sprintf(buf, "%s%s", curr_field->parent ? "." : "", curr_field->name); 
		strcat(init_dbg_buf, buf);
	}
	else {
		char buf[100];
		sprintf(buf, "[%d]", curr_field->idx); 
		strcat(init_dbg_buf, buf);
	}

}

void print_initializer(CType * curr_field, CType * p)
{
	init_dbg_buf[0] = '\0';
	print_decl(curr_field);	

	/* strcat '=' and value */

	printf("%s\n", init_dbg_buf);
}

//#endif
inline int designator(CType *pctype, CType **out)
{
	int type;
	CType * tmp;
	int idx;

	type = gettoken();
	if(type == '['){
		if(is_ctype_array(pctype)) {
			CType * const_ct;
			if(constant_expr(&const_ct)) {
				idx = (int)const_ct->v->content.ull;
			} else {
				printf("__LINE__ %d: %d: error: expect constant expr\n", __LINE__, line);
				exit(0);
			}
			
			struct list_head *cur = pctype->link.next;
			CType *c_tmp;
			c_tmp = list_entry(cur, CType, link);
			if(idx >= c_tmp->types.array.array_size) {
				// error
				printf("%d: error: array index in initializer exceeds array bounds\n", line);
				exit(0);
			}
			tmp = pctype->child;
			tmp->idx = idx;
			tmp->offset = pctype->offset + idx*sizeof_ctype(tmp, F);
			// tmp->lvalue ??
			*out = tmp;
			
			if(gettoken() != ']') {
				printf("%d: error: parse error before '%s'\n", line, yytext);
				exit(0);
			}
		}
		else {
			//error
			printf("%d: error: initialized object '%s' is not a array\n", 
				line, pctype->name);
			exit(0);
		}
	}else if(type == '.') {
		if(is_ctype_struct(pctype)) {
			if(gettoken() == IDENTIFIER) {
				symvalue_t * value;
				CType *tmp = pctype->types.normal.d_attr.st_un;
				if(symtab_lookup(tmp->types.struct_union_enum.table, yytext, 
					SYM_TYPE_OBJECT, &value) != R_SUCCESS) {
					printf("%d: error: %s is not a member of struct '%s'\n", 
						line, yytext, pctype->name);
					exit(0);
				}
				*out = value;
				
			} else {
				printf("%d: error: parse error before '%s'\n", line, yytext);
				exit(0);
			}
		}
		else if(is_ctype_union(pctype)){
			if(gettoken() == IDENTIFIER) {
				symvalue_t * value;
				CType *tmp = pctype->types.normal.d_attr.st_un;
				if(symtab_lookup(tmp->types.struct_union_enum.table, yytext, 
					SYM_TYPE_OBJECT, &value) != R_SUCCESS) {
					printf("%d: error: %s is not a member of union '%s'\n", 
						line, yytext, pctype->name);
					exit(0);
				}
				*out = value;
				
			} else {
				printf("%d: error: parse error before '%s'\n", line, yytext);
				exit(0);
			}			
		} else {
			// error
			printf("%d: error: initialized object '%s' is not a struct or union\n", 
				line, pctype->name);
			exit(0);
		}
	} else {
		restoretoken();
		return 0;   // not designator
	}

	return 1;
}

/*
designator-list
	:
	| designator
	| designator-list designator
	;
*/

int designator_list(CType *pctype, CType **out)
{
	int i = 0;
	CType *in, *out2;

	in = pctype;
	
	while(designator(in, &out2)){
		i++;
		in = out2;
	}

	if(i>0) {
		*out = out2;
		return  1;
	}
	return 0;
}

/*

initializer
	:
	| assignment-expression
	| '{' initializer-list '}'
	| '{' initializer-list ',' '}'
	;
	
initializer-list
	:
	| designation initializer
	| initializer
	| initializer-list ',' designation initializer
	| initializer-list ',' initializer
	;
	
designation
	:
	| designator-list '='
	;
*/
void get_first_field(CType *pctype, CType **out)
{
	CType * curr_field;
	
	if(is_ctype_array(pctype)) {
		assert(pctype->child != NULL);
		curr_field = pctype->child;
		curr_field->idx = 0;
		curr_field->offset = pctype->offset;
		while(curr_field->child != NULL) {
			curr_field = curr_field->child;
			curr_field->idx = 0;
			curr_field->offset = pctype->offset;
		}
		*out = curr_field;
		get_first_field(curr_field, out);			
	} else if(is_ctype_struct(pctype) || is_ctype_union(pctype)) {
		CType *tmp = pctype->types.normal.d_attr.st_un;
		curr_field = tmp->types.struct_union_enum.item_list[0];
		curr_field->size_of = pctype->size_of;
		*out = curr_field;
		get_first_field(curr_field, out);
	}  
	else *out = pctype;

}

void get_next_field(CType *pctype, CType **out) 
{
	CType * curr_field;
	CType * tmp;

	curr_field = pctype;
	
	if(curr_field->parent != NULL) {
		if(is_ctype_array(curr_field->parent)) {
			curr_field->idx ++;

			struct list_head *cur = curr_field->parent->link.next;
			CType *c_tmp;
			c_tmp = list_entry(cur, CType, link);
			if(c_tmp->types.array.is_p_alike_init) {
				/*
					for eg. int a[][2]={{2,3},4};
				*/
				curr_field->offset = curr_field->parent->offset + 
										curr_field->size_of * curr_field->idx;
				c_tmp->types.array.array_size = curr_field->idx+1;
				c_tmp->types.array.is_pointer_alike = F;
				*out = curr_field;

			} else if(curr_field->idx >= c_tmp->types.array.array_size) {
				curr_field->idx = 0;
				curr_field->offset = curr_field->parent->offset + 
										curr_field->size_of * c_tmp->types.array.array_size;				
				get_next_field(curr_field->parent, out);
			} else {
				curr_field->offset = curr_field->parent->offset + 
										curr_field->size_of * curr_field->idx;
				*out = curr_field; 
			}
		} else if(curr_field->parent->datatype == DATATYPE_STRUCT){
			if(curr_field->idx + 1 >= curr_field->parent->types.struct_union_enum.items) {
				CType * tmp = 
					curr_field->parent->types.struct_union_enum.associate_with;
				tmp->offset += sizeof_ctype(tmp, F);

				int i;
				int offset = tmp->offset;
				tmp = curr_field->parent;
				for(i = 0; i < tmp->types.struct_union_enum.items; i++) {
					CType * ct = tmp->types.struct_union_enum.item_list[i];
					ct->offset = offset;
					offset += sizeof_ctype(ct, F);
				}			
				get_next_field(curr_field->parent->types.struct_union_enum.associate_with, out);
			} else {
				*out = curr_field->parent->types.struct_union_enum.item_list[curr_field->idx + 1];
			}
		} 
		else if (curr_field->parent->datatype == DATATYPE_UNION) {
			get_next_field(curr_field->parent->types.struct_union_enum.associate_with, out);
		}
		else {
			// not possible
			printf("not possible\n");
			exit(0);
		}
	} else {
		*out = NULL;
	}
		
}

int initializer(CType *pctype, CType **next_field, bool first)
{
	CType *p;
	CType *curr_field;
	int type;

	if(pctype == NULL) {
		printf("%d: error: excess elements in initializer\n", line);
		exit(0);
	}
	
	if(is_ctype_array(pctype)) {
		assert(pctype->child != NULL);
		CType * tmp = pctype->child;
		while(tmp != NULL){
			tmp->idx = 0;
			tmp = tmp->child;
		}
	} else if(is_ctype_struct(pctype) || is_ctype_union(pctype) ) {
		/* current will be the first field of the struct or union */
		CType *tmp = pctype->types.normal.d_attr.st_un;
		tmp->types.struct_union_enum.associate_with = pctype;
	} 
			
	type = gettoken();
	if(type == '{') {

		if(is_ctype_array(pctype)) {
			curr_field = pctype->child;
			curr_field->idx = 0;
			// curr_field->lvalue;
			curr_field->offset = pctype->offset;
			
		} else if(is_ctype_struct(pctype) || is_ctype_union(pctype) ) {
			/* current will be the first field of the struct or union */
			CType *tmp = pctype->types.normal.d_attr.st_un;
			curr_field = tmp->types.struct_union_enum.item_list[0];

			// curr_field->lvalue;
		} else {
			curr_field = pctype;
			// curr_field->lvalue;
		}
		if(initializer_list(pctype, curr_field, next_field)) {
			if(gettoken() != ',')
				restoretoken();
		}
		if(gettoken() != '}') {
			// error
			printf("_LINE_ %d: %d: parse error before '%s\n'", __LINE__, line, yytext);
			exit(0);
		}
		if(*next_field != NULL && (*next_field)->parent != pctype->parent)
			get_next_field(pctype, next_field);
	} else {
		restoretoken();
		if(assignment_expr(&p)) {
			if(first) {
				/* assign value of 'p' to 'pctype' */
				/* TODO: if 'p' is string and 'pctype' is array */
				//.......................................
				bool bk_ = g_ajust_offset_needed;
				p = genop(p);
				freereg(p->reg);
				g_ajust_offset_needed = bk_;
				/* TODO: type compatible check */ 
				gen_assign(pctype, p);
				if(is_in_function == F && p->storage_type != STORAGE_CONST) {
					printf("%d: error: initializer element is not constant\n", line);
					exit(0);
				}
				
			} else {
				/* get the first */
				get_first_field(pctype, &curr_field);
				/* assign value of 'p' to 'curr_field' */
				/* TODO: if 'p' is string and 'pctype' is array */
				//.......................................
				bool bk_ = g_ajust_offset_needed;
				p = genop(p);
				freereg(p->reg);
				g_ajust_offset_needed = bk_;
				/* TODO: type compatible check */
				gen_assign(curr_field, p);
				if(is_in_function == F && p->storage_type != STORAGE_CONST) {
					printf("%d: error: initializer element is not constant\n", line);
					exit(0);					
				}				
#ifdef DEBUG
				print_initializer(curr_field, p);
#endif
				/* get next field */
				get_next_field(curr_field, next_field);
			}
		}
		else {
			printf("%d: error: expect assignment expr or initializer\n", line);
			exit(0);
		}
	}

	return 1;
}


int initializer_list(CType *pctype, CType *first, CType ** out_next_field)
{
	int i;
	int flag = 0;
	CType * out;
	CType * curr_field;	
	CType * next_field; 

	curr_field = first;

	while(1) {
		if(designator_list(pctype, &out)) {
			int type;
			i++;
			if((type = gettoken()) == '=') {
				// current field is 'out'
				if(initializer(out, &next_field, F)) {
					curr_field = next_field;
					*out_next_field = next_field;
					if(gettoken() != ',') {
						restoretoken();
						break;
					}
				} else {
					// error
					printf("%d: error: except initializer\n", line);
					exit(0);
				}
			} else {
				// error
				printf("%d: error: except '='\n", line);
				exit(0);
			}
		} else { 
		
			if(initializer(curr_field, &next_field, F)) {
				i++;
				curr_field = next_field;
				*out_next_field = next_field;

			} else {
				flag = 1;
				break;
			}

			if(gettoken() != ',') {
				restoretoken();
				break;
			}
		}
	}

	if(i>0) { 
		if(flag == 1) restoretoken2(",");
		return 1;
	}

	*out_next_field = NULL;
	return 0;
}

void  pre_init(CType * pCType)
{
	if(is_ctype_array(pCType)) {
		CType * tmp = pCType;
		while(is_ctype_array(tmp)) {
			struct list_head *curr;	
			curr = tmp->link.next;
			
			assert(curr != NULL); 
			CType * elt = list_entry(curr, CType, link);
			assert(elt->datatype == DATATYPE_ARRAY);
		
			if(elt->types.array.is_p_alike_init) {
				g_ajust_offset_needed = T; 
				g_ajust_offset_ptr = &(OUTBUF[OUTBUFIDX]);
			}
			
			CType * tmp2 = create_ctype();
			CType_cpy(tmp2, tmp, F);

			tmp2->link.next = (tmp->link.next)->next;
			tmp2->get_from = pCType;
			tmp2->parent = tmp;
			tmp->child = tmp2;
			tmp2->idx = 0;
			tmp2->size_of = sizeof_ctype(tmp2, F);
			tmp = tmp2;
		}
		pre_init(tmp);
	} else if( is_ctype_struct(pCType)) {
		int i = 0;
		CType * tmp1 = pCType->types.normal.d_attr.st_un;
		int offset = pCType->offset;
		for(i = 0; i < tmp1->types.struct_union_enum.items; i++) {
			CType * ct = tmp1->types.struct_union_enum.item_list[i];
			ct->offset = offset;
			offset += sizeof_ctype(ct, F);
			pre_init(ct);
		}
	} else if( is_ctype_union(pCType)) {
		int i = 0;
		CType * tmp1 = pCType->types.normal.d_attr.st_un;
		for(i = 0; i < tmp1->types.struct_union_enum.items; i++) {
			CType * ct = tmp1->types.struct_union_enum.item_list[i];
			ct->offset = pCType->offset;
			pre_init(ct);
		}
	}
}

void post_init(CType * pCType)
{
	if(is_ctype_array(pCType)) {
		CType * tmp = pCType->child;
		while(tmp!= NULL){
			CType *tmp2 = tmp->child;
			if(is_ctype_struct(tmp) || is_ctype_union(tmp))
				post_init(tmp);
			free_ctype(tmp);
			tmp = tmp2;
		}
		pCType->child = NULL;
	} else if(is_ctype_struct(pCType) || is_ctype_union(pCType)) {
		int i = 0;
		CType * tmp1 = pCType->types.normal.d_attr.st_un;
		tmp1->types.struct_union_enum.associate_with = NULL;
		for(i = 0; i < tmp1->types.struct_union_enum.items; i++) {
			CType * ct = tmp1->types.struct_union_enum.item_list[i];
			post_init(ct);
		}
	}
}

#endif

static char instr_out_buf[4*1024*1024];   // should be big enough

void reset_func_ctx()
{
	g_func_ctx.cur_statck_size = 0;
	g_func_ctx.is_EDI_used = F;
	g_func_ctx.is_ESI_used = F;
	
	g_func_ctx.stk_instr_addr = NULL;
	g_func_ctx.stk_instr_len = 0;
	instr_out_buf[0] = '\0';
	g_func_ctx.buf = instr_out_buf;
	g_func_ctx.buf_index = 0;

	g_func_ctx.hidden_local_value_sz = 0;	
	
#ifdef TEST_OUTPUT
	int i;
	for(i=0; i<MAX_STACK_REGS; i++) {
		g_func_ctx.stack_reg[i].offset = NA;
		g_func_ctx.stack_reg[i].used = 0;
	}

#endif	
}

/*
init_declarator
	: declarator
	| declarator '=' initializer
	;
*/

CType * init_declarator(CType *pctype, int idx, bool no_init)
{
	symvalue_t * value;
	symtab_t * symtab;
	struct list_head *curr;
	CType * tmp_ct;
	int type;
	int dt = DECLARATOR_DIRECT;
	  
	declarator(pctype, &dt);

	build_ctype(pctype);

	if(no_init == T)     /* if no_init is true, ctype is not add to symbol table in init_declarator() */
		return pctype;

	type = gettoken();
	if(type == ',' || type == ';' || type == '=') {
		restoretoken();
	} else if(is_ctype_function(pctype) && idx == 1) {  
		/* may be function definition */	
		restoretoken();

		CType *c_tmp;
		struct list_head *cur = pctype->link.next;
		c_tmp = list_entry(cur, CType, link);
		if(c_tmp->types.function.is_old_style == T)
		{
			CType_list * func_ct_list;
			CType *ct_tmp1;
			const int nr_params = c_tmp->types.function.nr_param;
			int n = 0;
			int last_stage = g_cur_stage;
			g_cur_stage = STAGE_PARAM;
				
			if(c_tmp->types.function.has_same_id == T) {
				printf("error: multiple parameters has same name\n");
				exit(0);
			}

			while((func_ct_list = declaration(T, T)) !=  NULL) {
				list_for_each(curr, &(func_ct_list->link)){ 
					CType_list* cl_tmp = list_entry(curr, CType_list, link);
					ct_tmp1 = cl_tmp->ct; 

					/* Here need to check if declaration has srorage type */
					if(ct_tmp1->types.normal.d_attr.has_stor_spec &&
						ct_tmp1->types.normal.d_attr.stor_spec != REGISTER) {
						printf("error: function param declaration has storage type\n");
						exit(0);
					}
					
					if(ct_tmp1->name != NULL) {	
						if(symtab_lookup(c_tmp->types.function.table, ct_tmp1->name, 
							SYM_TYPE_OBJECT, &value) != R_SUCCESS) {
							printf("function param name %s not in param list\n", ct_tmp1->name);
							exit(0);
						}
						n++;
						assert(value->link.next == NULL);

						/* the exist ctype get from the parameter ctype, 
						     we also get the name and link from the parameter
						*/
						CType_cpy(value, ct_tmp1, T);

						/* and free the parameter ctype head */
						ct_tmp1->link.prev = &(ct_tmp1->link);
						ct_tmp1->link.next = NULL;
						free_ctype(ct_tmp1);
						
					} else {
						printf("function param declaration has no name\n");
						exit(0);
					}
					
				}
				free_ctype_list(func_ct_list); 
			}
			if (n > nr_params){
				printf("old style function definition param decleartion mismatch!\n");
				printf("This could not possible!\n");
				exit(0);
			}

			/* there maybe no decl identifier in paramters, like 'c' in the fallowing example:
			     int f(a, b, c)
			     int b,a;
			     {}
			     
			     We need to set it to int type
			*/
			int offset = FUNC_STACK_FRAME;
			
			if(nr_params > 0) {
				int i =0;
				for(; i< nr_params; i++) {
					CType *ct = c_tmp->types.function.param_list[i];
					if(ct->types.normal.d_attr.has_type_spec == 0) {
						ct->types.normal.d_attr.type_spec = INT_MASK; 
						ct->size_of = sizeof_ctype(ct, F);
					}
					if(is_ctype_array(ct)) {
						/* Special case for array in parameter */
						array2pointer(ct);
					}
					/*
					 calculate offset base on BASE REG for function parameters
					*/
					if(offset & (ALIGNMENT-1)) {
						if(ct->size_of <= (ALIGNMENT - ( offset & (ALIGNMENT-1)))) {
							ct->offset = offset;
							offset += ct->size_of;
						} else {
							offset += ALIGNMENT - ( offset & (ALIGNMENT-1));
							ct->offset = offset;
							offset += ct->size_of;
						}
					} else {
						// size currently is aligned 
						ct->offset = offset;
						offset += ct->size_of;
					}
				}


			}

			g_cur_stage = last_stage;
		} 

		c_tmp->types.function.is_definition = T;
		c_tmp->types.function.is_old_style = F;

		tmp_ct = add_decl_to_symtab(pctype);   /* pctype may be freed */

		cur = tmp_ct->link.next;
		c_tmp = list_entry(cur, CType, link);
		assert(c_tmp->datatype == DATATYPE_FUNCTION);
			
		/* prepare for function definition */
		symtab_t *new_symt = NULL;
		if(symtab_create(5, NULL, NULL, 1, &new_symt) 
			!= R_SUCCESS) {
			printf("symtab_create failed\n");
			exit(-1);
		}

		/* if function has parameters */
		if(c_tmp->types.function.nr_param > 0) {
			int i =0;
			for(; i< c_tmp->types.function.nr_param; i++) {
				CType *ct = c_tmp->types.function.param_list[i];
				if(ct->datatype != DATATYPE_ELLIPSIS) {
					if(symtab_define(new_symt, ct, symexists_add, NULL, T) != R_SUCCESS) {
						printf("line %d: error: parameter name omitted\n", line);  
						exit(0);
					}
				}
			}
		}
		
		/* We are going into funtion */
		g_cur_stage = STAGE_FUNCTION;
		InitStack(&statement_ctx_stk);
		statement_ctx *s_ctx;
		s_ctx = create_sta_ctx();
		s_ctx->sta_t = IN_FUNCTION;
		s_ctx->symt = new_symt;
		
		if(gettoken() == '{' && StackEmpty(&statement_ctx_stk)  != T ) {
			printf("error: local function definition not supported \n");
			exit(0);
		}else restoretoken();
		
		if(Push(&statement_ctx_stk, (SElemType)s_ctx) != R_SUCCESS) {	
			printf("Push to stack failed\n");
			exit(-1);	 
		}
		/* initialize function context */
		reset_func_ctx();

		// debug
		// printf("--- function \"%s\" ----\n", pctype->name);

		is_in_function = T;
		g_label = 1;
#ifdef TEST_OUTPUT

		/* generate function in code */
		oprintf("%s:\n", tmp_ct->name);
		oprintf("\tPUSH\tEBP\n"   
			   	"\tMOV\tEBP,ESP\n");   

		g_func_ctx.stk_instr_addr = &(OUTBUF[OUTBUFIDX]);
		g_func_ctx.stk_instr_len = 32;
		memset(&(OUTBUF[OUTBUFIDX]),' ',32);
		OUTBUF[OUTBUFIDX+31] = '\n';
		OUTBUFIDX += 32;
	
#endif
		
		/* parse function body */
		if(compound_statement(new_symt) == 0) {
			printf("error: parse error before %s", yytext);
			exit(0);
		}

		if(g_func_ctx.cur_statck_size & (ALIGNMENT-1))
			g_func_ctx.cur_statck_size += ALIGNMENT - ( g_func_ctx.cur_statck_size & (ALIGNMENT-1)); 		

#ifdef TEST_OUTPUT
		if(g_func_ctx.cur_statck_size > 0) {
			int ret;
			ret = sprintf(g_func_ctx.stk_instr_addr, 
				"\tSUB\tESP,0x%x\n",
				g_func_ctx.cur_statck_size); 
			g_func_ctx.stk_instr_addr[ret] = ' ';
		}

#if 0
		if(g_func_ctx.is_EDI_used)
			oprintf("\tPOP\tEDI\n");
		if(g_func_ctx.is_ESI_used)
			oprintf("\tPOP\tESI\n");
#endif

		if(g_func_ctx.cur_statck_size > 0) {
			/* generate function out code */
			oprintf("\tLEAVE\n"
				   "\tRET\n"); 
		} else {
			oprintf("\tPOP\tEBP\n"
				   "\tRET\n"); 
		}
		/**********************/
		oprintf("\r\n");

		OUTBUF[OUTBUFIDX] = '\0';
		printf(OUTBUF);  // print instr buf now
		//dump_regs();
		
#endif

		is_in_function = F;
		/* comming out of function */	
		g_func_ctx.cur_statck_size = 0;
                g_cur_stage = STAGE_GLOBAL;
		if(Pop(&statement_ctx_stk, (SElemType *)&s_ctx) != R_SUCCESS) { 
			printf("Push to stack failed\n");
			exit(-1);	 
		}
		assert(s_ctx->sta_t == IN_FUNCTION);
		do_label(s_ctx);
		free_sta_ctx(s_ctx);
		assert(StackEmpty(&statement_ctx_stk) == T);
		FreeStack(&statement_ctx_stk);
		if(new_symt != NULL)
			symtab_destroy(&new_symt, free_ctype);
		
		return tmp_ct;
	}

	tmp_ct = add_decl_to_symtab(pctype);
	
   	if(gettoken() == '=') {
		if(tmp_ct->types.normal.d_attr.stor_spec == TYPEDEF) {
			printf("%d: error: typedef object initialized\n", line);
			exit(0);
		}
		if(is_ctype_function(tmp_ct)) {
			printf("%d: error: function initialized\n", line);
			exit(0);
		}
		pre_init(tmp_ct);
		CType * dummy = NULL;
		initializer(tmp_ct, &dummy, T);
		post_init(tmp_ct);
		/* generate init value */
   	}
   	else 
		restoretoken();

	return tmp_ct;
}


void adjust_offset(int size)
{
	char *buf = g_ajust_offset_ptr;
	char *p;
	char s;
	char num[64];

#ifndef isnum
#define isnum(c) ((c) >= '0' && (c) <= '9')
#endif
	
	while(p=strstr(buf, "(X)")) {
		s = p[-1];
		assert(s!='-');
		char *ptr = p+3;
		int i=0;

		if(s == '+')
			p--;			

		while(*ptr && isnum(*ptr)) {
			num[i++] = *ptr; 
			ptr++;
		}
		num[i] = '\0';
		int replace_len = ptr-p;

		int newsz = size + atoi(num);

		char *new_str = NULL;
		if(newsz > 0) {
			itoa(newsz, &num[1], 10);
			num[0] = '+';
			new_str = num;
		} else if(newsz < 0) {
			itoa(newsz, num, 10);  /* itoa() will generate '-' */
			new_str = num;
		} else
			new_str = NULL;

		/* replace (p, +replace_len) with new_str */
		int new_rp_len = strlen(new_str);
		int new_len = strlen(buf) + 1 + ( new_rp_len - replace_len);
		char *tmp = malloc(new_len);
		memcpy(tmp, buf, p-buf);
		if(new_str)
			memcpy((p-buf)+tmp, new_str, new_rp_len);
		strcpy((p-buf)+tmp+new_rp_len, ptr);

		/* now 'tmp' is new string, copy back */
		memcpy(buf, tmp, new_len);	/* we assume 'buf' is big enough */
		free(tmp);

		buf = p;
		buf += new_rp_len;
	}

	OUTBUFIDX = (g_ajust_offset_ptr - OUTBUF) + strlen(g_ajust_offset_ptr);
}


/*
init_declarator_list
	: init_declarator
	| init_declarator_list ',' init_declarator
	;
*/

CType_list * init_declarator_list(CType *pCType, bool no_init)
{
	CType_list * tmp, *new;
	int type;
	CType ct_save;
	int idx = 1;

	assert(pCType->link.next == pCType->link.prev);
	memcpy(&ct_save, pCType, sizeof(CType));
	
	tmp = create_ctype_list();	// create head

	new = create_ctype_list();
	new->ct = init_declarator(pCType, idx++, no_init);
			
	if(is_ctype_array(new->ct)) {
		struct list_head *curr;
		
		curr = new->ct->link.next;
		
		if(curr != NULL) {
			CType * elt = list_entry(curr, CType, link);
			if(elt->types.array.is_pointer_alike == T && g_cur_stage != STAGE_PARAM) {
				printf("%d: error: array size missing in '%s'\n", line, new->ct->name);
				exit(0); 
			}
		
			/* recheck if array size is known */
			int oldsz = new->ct->size_of;
			new->ct->size_of = sizeof_ctype(new->ct, T);
			if(oldsz == 0 && new->ct->size_of != 0){
				add_ctype_to_stack(new->ct);
			}
		}
		if(g_ajust_offset_needed) {

#ifdef DEBUG			
			printf("g_ajust_offset_needed 1\n");
			printf("--------------\n");
			printf("%s\n", g_ajust_offset_ptr);
			printf("--------------\n");
#endif

			/* adjust offset */
			adjust_offset(new->ct->offset);
		}
	}
	g_ajust_offset_needed = F;

	list_add_tail(&(new->link), &(tmp->link));

	if(is_ctype_func_definition(new->ct))
		return tmp;

	while((type = gettoken()) == ',') {
		CType *ct = create_ctype();
		CType_cpy(ct, &ct_save, F);

		new = create_ctype_list();
		new->ct = init_declarator(ct, idx++, no_init);

		if(is_ctype_array(new->ct)) {
			struct list_head *curr;
			
			curr = new->ct->link.next;
			
			if(curr != NULL) {
				CType * elt = list_entry(curr, CType, link);
				if(elt->types.array.is_pointer_alike == T && g_cur_stage != STAGE_PARAM) {
					printf("%d: error: array size missing in '%s'\n", line, new->ct->name);
					exit(0); 
				}
				/* recheck if array size is known */
				int oldsz = new->ct->size_of;
				new->ct->size_of = sizeof_ctype(new->ct, T);
				if(oldsz == 0 && new->ct->size_of != 0){
					add_ctype_to_stack(new->ct);
				}				
			}
			if(g_ajust_offset_needed) {
#ifdef DEBUG
				printf("g_ajust_offset_needed 2\n");
				printf("--------------\n");
				printf("%s\n", g_ajust_offset_ptr);
				printf("--------------\n");	
#endif
				/* adjust offset */
				adjust_offset(new->ct->offset);
			}
		}
		g_ajust_offset_needed = F;
		
		list_add_tail(&(new->link), &(tmp->link));	
	}

	restoretoken();
	return tmp;
}

/*
type-name
	:
	specifier-qualifier-list  abstract-declarator
	specifier-qualifier-list 
*/

CType * type_name()
{
	int type;
	int tag_status;
	int ret;
	CType * pCType;
	int dt = DECLARATOR_ABSTRACT;

	pCType = create_ctype();
			
	if(ret = declaration_specifiers(pCType, T)) {
		if(pCType->datatype == DATATYPE_NORMAL && 
			pCType->types.normal.d_attr.has_stor_spec == T) {
			printf("error: type name must has no storage type\n");
			exit(0);
		}

		declarator(pCType, &dt);
	} else {
		//printf("not declaration\n");
		build_ctype(pCType);
		free_ctype(pCType);
		return NULL;	
	}
	build_ctype(pCType);
	return pCType;
}

/*
declaration
	: declaration_specifiers ';'
	| declaration_specifiers init_declarator_list ';'
	;
*/
CType_list * declaration(bool  has_decl_spec, bool no_init)
{
	int type;
	int ret;
	CType * pCType;
	CType_list * ct_list, *tmp;
	pCType = create_ctype();

	if(has_decl_spec == F) {
		pCType->types.normal.d_attr.type_spec = TS_INT;
		pCType->types.normal.d_attr.has_type_spec = T;
		goto aftrer_decl_spec;
	}
			
	if(ret = declaration_specifiers(pCType, F)) {
		if((type = gettoken()) == ';') {
			if(((ret == STRUCT || ret == UNION )) 
				|| ret == ENUM) {
				/* do nothing */
			}
			else printf("warning: empty declaration\n");
			build_ctype(pCType);
			free_ctype(pCType);
			return  (CType_list*)(!NULL);
		}  
		restoretoken();

aftrer_decl_spec:		
		ct_list = init_declarator_list(pCType, no_init);
		if((type = gettoken()) != ';') {

			/* if only one decl in list and it's function decl,
			  * it may be function definition 
			*/
			CType_list * ct_l;
			struct list_head *curr = ct_list->link.next;
			ct_l = list_entry(curr, CType_list, link);
		   
			if(( curr->next == &(ct_list->link)) &&    // only one decl in list ?
				(ct_l->ct->link.next != NULL)){   // and maybe function ? 
				struct list_head *cur = ct_l->ct->link.next;
				CType *c_tmp;
				c_tmp = list_entry(cur, CType, link);
				if(c_tmp->datatype == DATATYPE_FUNCTION &&
					c_tmp->types.function.is_definition == T) {
					restoretoken();
					return ct_list;
				}
			}

			printf("error: parse error before %s\n", yytext);
			exit(0);

		}
	} else {
		//printf("not declaration\n");
		build_ctype(pCType);
		free_ctype(pCType);
		return NULL;	
	}

	return ct_list;
}

void add_ctype_to_stack(CType *pctype)
{
	if(!StackEmpty(&statement_ctx_stk)) {  /* we are in function */
		if(pctype->storage_type == STORAGE_LOCAL &&
			pctype->size_of > 0) {

			/* if stack growth from higher address to lower address */
			if(g_stk_h2l) {
				int size = g_func_ctx.cur_statck_size;
				if(size & (ALIGNMENT-1)) {
					if(pctype->size_of <= (ALIGNMENT - ( size & (ALIGNMENT-1)))) {
						pctype->offset = size;
						size += pctype->size_of;
					} else {
						size += ALIGNMENT - ( size & (ALIGNMENT-1));
						pctype->offset = size;
						size += pctype->size_of;
					}
				} else {
					/* size currently is aligned */
					pctype->offset = size;
					size += pctype->size_of;
				} 
				g_func_ctx.cur_statck_size = size;
				pctype->offset = -g_func_ctx.cur_statck_size; 
			}
			else {
				/* TODO: not handle yet */
				pctype->offset = g_func_ctx.cur_statck_size; 
				g_func_ctx.cur_statck_size += pctype->size_of;
			}

		}
	}
}

CType * add_decl_to_symtab(CType* pctype)
{
	symtab_t *symtab;
	symvalue_t * value;
	
	/* add pCType to symbol table */	
	
	if(GetTop(&g_sym_stack, (SElemType*)&symtab) != R_SUCCESS) {
		error("Get top of stack failed\n");
	}
	if(symtab == NULL) {
		if(symtab_create(10, NULL, NULL, 1, &symtab) != R_SUCCESS) {
			printf("symtab_create failed\n");
			exit(-1);
		}
		if(ModifyTop(&g_sym_stack, symtab) != R_SUCCESS) {
			printf("modify top of stack failed\n");
			exit(-1);
		}
	}
		
	if(pctype->name != NULL) {		
		if(symtab_lookup(symtab, pctype->name, 
			SYM_TYPE_OBJECT|SYM_TYPE_TYPEDEF, &value) == R_SUCCESS) {
				
			/* if is function decleartion */
			if(value->symtype == SYM_TYPE_OBJECT && pctype->symtype == SYM_TYPE_OBJECT) {
				CType *c_tmp1, *c_tmp2;
				struct list_head *cur1 = value->link.next;
				struct list_head *cur2 = pctype->link.next;
	
				if(cur1 != NULL && cur2 != NULL) {
					c_tmp1 = list_entry(cur1, CType, link);
					c_tmp2 = list_entry(cur2, CType, link);
					if(c_tmp1->datatype == DATATYPE_FUNCTION &&
						c_tmp2->datatype == DATATYPE_FUNCTION) {
						if(is_ctype_compatible(value, pctype)) {
							if(c_tmp1->types.function.is_definition == T &&
								c_tmp2->types.function.is_definition == T) {
								printf("%d: error: function definition conflict\n", line);
								exit(0);
							}
							if(c_tmp1->types.function.is_old_style == T) {
								/* preverse function decl is old style */
								int i = 0;
								for(; i< c_tmp2->types.function.nr_param; i++)
									c_tmp1->types.function.param_list[i] =
										c_tmp2->types.function.param_list[i];

								symtab_destroy(&(c_tmp1->types.function.table),free_ctype);
								c_tmp1->types.function.table = 
									c_tmp2->types.function.table;
								c_tmp1->types.function.nr_param =
									c_tmp2->types.function.nr_param;
								c_tmp2->types.function.table = NULL;
								c_tmp2->types.function.nr_param = 0;
								c_tmp1->types.function.is_old_style =
									c_tmp2->types.function.is_old_style;
							}
							if(c_tmp2->types.function.is_definition == T){
								c_tmp1->types.function.is_definition = T;
								c_tmp1->types.function.is_old_style = F;
							}

							/*TODO: composite type ??????? */
							
							free_ctype((void *)pctype);
							
							return value;
						} else {
							printf("%d: error: function declaration name conflict\n", line);
							exit(0);
						}
					}
				}
			}
	
			printf("%d: error: declaration name conflict\n", line);
			printf("previous declaration was here %d\n", value->line);
			exit(0);
		}
		symtab_define(symtab, pctype, symexists_reject, NULL, F);

		add_ctype_to_stack(pctype);

#ifdef   DEBUG
		out_buffer[0] = '\0';
		print_ctype(pctype, out_buffer);
		printf(out_buffer);
		printf("\n\r");
#endif

		return pctype;
	} else {
		printf("declaration has no name\n");
		exit(0);
	}	

}

#if 1

/*
primary_expr	
	: identifier	
	| CONSTANT	
	| STRING_LITERAL	
	| '(' expr ')'	
	;
*/

extern int str2int(char *str, unsigned long long *out);
extern int str2float(char *str, long double *out);
extern long str2char(char * s, bool *is_L);
extern long * str_literal(char * s, bool *is_L);

int primary_expr(CType **p)
{
	int type;

	type = gettoken();
	if(type == IDENTIFIER) {
		symvalue_t * value;
		if(symtabs_lookup(yytext, SYM_TYPE_OBJECT, &value) != R_SUCCESS) {
			if(symtabs_lookup(yytext, SYM_TYPE_TAG|SYM_TYPE_TYPEDEF, &value) == R_SUCCESS) {
				printf("error: %s is a tag or a typedef name\n", yytext);
				exit(0);
			}
			printf("error: undecleared identifier %s\n", yytext);
			exit(0);
		}
		*p = value;
		return type;
	}else if(type == ICONSTANT) {
		unsigned long long out;
		CType *tmp= create_ctype();
		tmp->symtype = SYM_TYPE_NO_NAME_VAL;
		tmp->datatype = DATATYPE_NORMAL;
		// tmp->address = 
		// store this val to .data

		tmp->v = create_c_val();
		
		tmp->types.normal.d_attr.has_type_spec = T;
		tmp->types.normal.d_attr.type_spec = str2int(yytext, &out);
		tmp->v->content.ull = out;
		build_ctype(tmp);
		tmp->storage_type = STORAGE_CONST;
		tmp->lvalue = 0;
		*p = tmp;
		
#ifdef   DEBUG
		out_buffer[0] = '\0';
		print_ctype(tmp, out_buffer);
		printf(out_buffer);
		printf("\n\r");
#endif						
		return type;
	}else if(type == CCONSTANT) {
		bool is_L;
		CType *tmp= create_ctype();
		tmp->symtype = SYM_TYPE_NO_NAME_VAL;
		tmp->datatype = DATATYPE_NORMAL;
		// tmp->address = 
		// store this val to .data

		tmp->v = create_c_val();

		tmp->types.normal.d_attr.has_type_spec = T;
		tmp->v->content.ull = str2char(yytext, &is_L);
		if(is_L == T)
			tmp->types.normal.d_attr.type_spec = TS_LONG;
		else
			tmp->types.normal.d_attr.type_spec = TS_CHAR;
		build_ctype(tmp);
		tmp->storage_type = STORAGE_CONST;
		tmp->lvalue = 0;
		*p = tmp;
#ifdef   DEBUG
		out_buffer[0] = '\0';
		print_ctype(tmp, out_buffer);
		printf(out_buffer);
		printf("\n\r");
#endif
		return type;
	}else if(type == FCONSTANT) {
		long double out;
		CType *tmp= create_ctype();
		tmp->symtype = SYM_TYPE_NO_NAME_VAL;
		tmp->datatype = DATATYPE_NORMAL;
		// tmp->address = 
		// store this val to .data
		
		tmp->v = create_c_val();
		
		tmp->types.normal.d_attr.has_type_spec = T;
		tmp->types.normal.d_attr.type_spec = str2float(yytext, &out);
		tmp->v->content.ld = out;
		build_ctype(tmp);
		tmp->storage_type = STORAGE_CONST;
		tmp->lvalue = 0;
		*p = tmp;

#ifdef   DEBUG
		out_buffer[0] = '\0';
		print_ctype(tmp, out_buffer);
		printf(out_buffer);
		printf("\n\r");
#endif
		return type;
	}else if(type == STRING_LITERAL) {
		bool is_L;
		CType *tmp= create_ctype();
		tmp->symtype = SYM_TYPE_NO_NAME_VAL;
		tmp->datatype = DATATYPE_NORMAL;
		// tmp->address = 
		// store this val to .data

		tmp->v = create_c_val();		

		tmp->types.normal.d_attr.has_type_spec = T;
		tmp->v->content.ls = str_literal(yytext, &is_L);
		if(is_L == T) {
			tmp->v->string_t = STR_LONG;
			tmp->types.normal.d_attr.type_spec = TS_LONG;
		}
		else {
			tmp->v->string_t = STR_CHAR;
			tmp->types.normal.d_attr.type_spec = TS_CHAR;
			tmp->v->content.s = (char *)tmp->v->content.ls;
		}
		CType *tmp2 = create_ctype();
		tmp2->datatype = DATATYPE_POINTER;
		list_add_tail(&(tmp2->link), &(tmp->link));
		build_ctype(tmp);
		tmp->storage_type = STORAGE_CONST;
		tmp->lvalue = 0;
		*p = tmp;
#ifdef   DEBUG
		out_buffer[0] = '\0';
		print_ctype(tmp, out_buffer);
		printf(out_buffer);
		printf("\n\r");
#endif
		return type;
	}else if(type == '('){
		CType *pp;
		
		int res = expr(&pp, F);
		if(res == 0) {
			//error
			printf("error: except a expresion\n");
			exit(0);
		}
		if((type = gettoken()) != ')') {
			printf("parse error before %s\n", yytext);
			exit(0);
		}
		*p = pp;
		return res;
	}

	restoretoken();
	return 0;
}


bool  is_ctype_normal(CType * pctype)
{

	assert(pctype->datatype == DATATYPE_NORMAL);
	
	if(pctype->link.next == NULL) {
		if(pctype->types.normal.d_attr.type_spec != 0 && 
			pctype->types.normal.d_attr.st_un == 0) {
			return T;
		}
	}

	return F;
}


bool  is_ctype_struct(CType * pctype)
{

	assert(pctype->datatype == DATATYPE_NORMAL);
	
	if(pctype->link.next == NULL) {
		if(pctype->types.normal.d_attr.type_spec == TS_STRUCT && 
			pctype->types.normal.d_attr.st_un != 0) {
			return T;
		}
	}

	return F;
}


bool  is_ctype_union(CType * pctype)
{

	assert(pctype->datatype == DATATYPE_NORMAL);
	
	if(pctype->link.next == NULL) {
		if(pctype->types.normal.d_attr.type_spec == TS_UNION && 
			pctype->types.normal.d_attr.st_un != 0) {
			return T;
		}
	}

	return F;
}


bool is_ctype_pointer(CType * pctype)
{
	struct list_head *curr;
	
	curr = pctype->link.next;
	
	if(curr != NULL) {
		CType * elt = list_entry(curr, CType, link);
		if(elt->datatype == DATATYPE_POINTER)
			return T;
		else if(elt->datatype == DATATYPE_ARRAY &&
			elt->types.array.is_pointer_alike == T)
			return T;
	}

	return F;	
}

bool is_ctype_array(CType * pctype)
{
	struct list_head *curr;
	
	curr = pctype->link.next;
	
	if(curr != NULL) {
		CType * elt = list_entry(curr, CType, link);
		if(elt->datatype == DATATYPE_ARRAY)
			return T;
	}

	return F;
}

bool is_ctype_function(CType * pctype)
{
	struct list_head *curr;
	
	curr = pctype->link.next;
	
	if(curr != NULL) {
		CType * elt = list_entry(curr, CType, link);
		if(elt->datatype == DATATYPE_FUNCTION)
			return T;
	}

	return F;
}

bool is_ctype_func_definition(CType * pctype)
{
	struct list_head *curr;
	
	curr = pctype->link.next;
	
	if(curr != NULL) {
		CType * elt = list_entry(curr, CType, link);
		if(elt->datatype == DATATYPE_FUNCTION 
			&& elt->types.function.is_definition == T)
			return T;
	}

	return F;
}

/*
argument_expr_list	
	: assignment_expr	
	| argument_expr_list ',' assignment_expr	
	;
*/


int generate_tmp_ctype(CType ** to, CType * from)
{
	struct list_head *curr;

	/*FIXME  do we need to free 'from' if it's ? */

	// debug
	//if(from->datatype != DATATYPE_NORMAL)
		//printf("from->datatype 0x%x\n", from->datatype);
	
	if(from->datatype != DATATYPE_NORMAL &&
		from->datatype != DATATYPE_FUNCTION) {	
		printf("Wrong Datatype 0x%x\n", from->datatype);
		printf("This is should not happen!\n");
		return -1;
	}
	
	if(*to == NULL) {
		*to = create_ctype();
	}else {
		if(((*to)->link.prev) !=  &((*to)->link)) {
			curr = (*to)->link.next;
			while(curr != (*to)->link.prev) {
				/* the poinetr ctype is created from '&', so try to free it */	
				CType * elt = list_entry(curr, CType, link);
				assert(is_ctype_pointer(elt));
				free_ctype(elt);	
				curr = curr->next;
			}
		}	
		memset((*to), 0, sizeof(CType));
		INIT_LIST_HEAD(&((*to)->link));
		(*to)->datatype = DATATYPE_NORMAL;
	}
	(*to)->line = line;

	(*to)->types.normal.bitfield_size = from->types.normal.bitfield_size;
	if(from->types.normal.d_attr.stor_spec == REGISTER) {
		(*to)->types.normal.d_attr.has_stor_spec = T;
		(*to)->types.normal.d_attr.stor_spec = from->types.normal.d_attr.stor_spec;
	}
	(*to)->types.normal.d_attr.has_type_spec = from->types.normal.d_attr.has_type_spec;
	(*to)->types.normal.d_attr.type_spec = from->types.normal.d_attr.type_spec;
	(*to)->types.normal.d_attr.st_un = from->types.normal.d_attr.st_un;
	(*to)->types.normal.d_attr.qua.q_const = from->types.normal.d_attr.qua.q_const;
	(*to)->types.normal.d_attr.qua.q_volatile = from->types.normal.d_attr.qua.q_volatile;

	(*to)->get_from = from;

	/* generate_tmp_ctype will always has only the first ctype, then it linked to 
	the type which it's get from, after that,  the poinetr ctype is created from '&'(address operation)
	may add after the first ctype.
	*/
	(*to)->link.prev = &((*to)->link);
	(*to)->link.prev->next = (*to)->get_from->link.next;
	(*to)->v = (*to)->get_from->v;

	(*to)->size_of = from->size_of;
	(*to)->offset = from->offset;
	(*to)->storage_type = from->storage_type;
	(*to)->lvalue = from->lvalue;
	(*to)->reg = from->reg;
	
	return 0;
}

/*
 * generate code of bracket []
 */
void  gen_opb(CType * pp, CType * ep)
{
	int o_offset = pp->offset; 
	int is_array = 0;
	int is_pointer = 0;
	
	// assert( pp->storage_type != STORAGE_CONST)
	// assert pp is pointer? 
	if(is_ctype_array(pp) ) {
		is_array = 1;
	} else if(is_ctype_pointer(pp)) {
		is_pointer = 1;
	}
	
	{
		/*
		  * generate a new ctype from 'pp', eg. 'pp' will change from "array of char" to "char"
		  */
		
		CType * pctype = pp;
		struct list_head *l;
		struct list_head *curr;
	
		l = pctype->link.next->next;
		curr =  pctype->link.next;

		/* all the generated ctype lists create from exists ctype lists only has a 
		     ctype head (specify the basic type), so their 'link.prev' point to itself.
		     excpet the ctype list from '&'
		*/
		if(pctype->link.prev != &(pctype->link)) {
			/* the pointer ctype is created from '&', so try to free it */
			CType * elt = list_entry(curr, CType, link);
			free_ctype(elt);
		}
		if(pctype->link.prev == curr) { 	
			pctype->link.prev = &(pctype->link);	
		}
					
		pctype->link.next = l;
		
		pctype->lvalue = 1;
		
		/* (2) here 'pctype' maybe eg "char"
		     recalculate size_of of 'pctype'
		     what about offset? it may be changed in gen_opb() 
		*/
		pctype->size_of = sizeof_ctype(pctype, F);
	}
				
	if(ep->storage_type == STORAGE_CONST) {
		if(ep->types.normal.d_attr.type_spec == TS_LONG_DOUBLE
			|| ep->types.normal.d_attr.type_spec == TS_FLOAT) {
			printf("%d: error: array subscript is not an integer\n", line);
			exit(0);
		}
		if(pp->storage_type != STORAGE_REG) {
			if(is_array) {
				pp->offset = ep->v->content.ull * pp->size_of + o_offset;   		
				return; 
			} else if(is_pointer) {
				pp->offset = 0;   /* offset unknown */
				int offset = ep->v->content.ull * pp->size_of ;

				int reg_idx = getreg(REG_FLAG_NORMAL, 4);
				if(reg_idx >= 0) {
					pp->storage_type = STORAGE_REG;
					pp->reg = reg_idx;
					
#ifdef TEST_OUTPUT
					if(o_offset > 0)
						oprintf("\tMOV\t%s,[EBP+%d]\n", g_reg_str[reg_idx >> REG_SHF], o_offset); 
					else if(o_offset < 0)
						oprintf("\tMOV\t%s,[EBP-%d]\n", g_reg_str[reg_idx >> REG_SHF], -o_offset);
					else
						oprintf("\tMOV\t%s,[EBP]\n", g_reg_str[reg_idx >> REG_SHF]);

					if(offset)
						oprintf("\tADD\t%s,%d\n", g_reg_str[reg_idx >> REG_SHF], offset);
#endif				
				} else {
					printf("*** out of reg1 ***\n");
				}
				return;
			}
		} else {
			pp->offset = 0;   /* offset unknown */
			int offset = ep->v->content.ull * pp->size_of ;

			int reg_idx = getreg(REG_FLAG_NORMAL, 4);
			if( reg_idx >=0 ) {
				pp->storage_type = STORAGE_REG;
				pp->reg = reg_idx;
			
#ifdef TEST_OUTPUT
				if(offset)
					oprintf("\tADD\t%s,%d\n", g_reg_str[reg_idx >> REG_SHF], offset);
#endif 
			} else {
				printf("*** out of reg2 ***\n");
			}
		}
	} else if(ep->storage_type == STORAGE_REG) {
		pp->offset = 0;   /* offset unknown */
		
	} else if(ep->storage_type == STORAGE_LOCAL || 
			ep->storage_type == STORAGE_PARAM){
		pp->offset = 0;   /* offset unknown */	

		int reg_idx = getreg(REG_FLAG_NORMAL, 4);
		if(reg_idx >= 0) {
			pp->storage_type = STORAGE_REG;
			pp->reg = reg_idx;
			
#ifdef TEST_OUTPUT
			if(o_offset > 0)
				oprintf("\tLEA\t%s,[EBP+%d]\n", g_reg_str[reg_idx >> REG_SHF], o_offset);
			else if(o_offset < 0)
				oprintf("\tLEA\t%s,[EBP-%d]\n", g_reg_str[reg_idx >> REG_SHF], -o_offset);
			else 
				oprintf("\tLEA\t%s,[EBP]\n", g_reg_str[reg_idx >> REG_SHF]);
			
			if(ep->offset > 0)
				oprintf("\tADD\t%s,[EBP+%d]\n", g_reg_str[reg_idx >> REG_SHF], ep->offset);
			else if(ep->offset < 0)
				oprintf("\tADD\t%s,[EBP-%d]\n", g_reg_str[reg_idx >> REG_SHF], -ep->offset);
			else
				oprintf("\tADD\t%s,[EBP]\n", g_reg_str[reg_idx >> REG_SHF]);
#endif	
		} else {
			printf("*** out of reg3 ***\n");
		}

	} else {
		// 
	}
	
}


/*
postfix_expr	
	: primary_expr	
	| postfix_expr '[' expr ']'	
	| postfix_expr '(' ')'	
	| postfix_expr '(' argument_expr_list ')'	
	| postfix_expr '.' identifier	
	| postfix_expr PTR_OP identifier
	| postfix_expr INC_OP	
	| postfix_expr DEC_OP	
	;
*/

int postfix_expr(CType **p)
{
	int type;
	CType *pp, *pctype = NULL;
	struct list_head *l;
	struct list_head *curr;
	int ret;
	CType * rootp = NULL;
	
	if((ret = primary_expr(&pp)) == 0) {
		/* not a postfix_expr */
		return 0;
	}
	
	// (1) here 'pctype' may be eg. "array of char" 
	pctype = pp;
	rootp = pctype;

	while(1) {
		type = gettoken();
		if(type == '[') {
			CType *ep;
			ret = expr(&ep, F);
			if(ret == 0) {
				// error
				printf("error: except expression\n");
				exit(0);
			}

			/* generate syntax tree operation [] (bracket) 
			     eg.   array[3]
			           []   
			          /   \
			       array  3
			*/
			CType* op = create_op_ctype();
			op->op_t = OPBRACKET;
			if(!rootp)
				op->lchild = pctype;
			else
				op->lchild = rootp;
			op->rchild = ep;
			rootp = op;

			if((type = gettoken()) != ']') {
				printf("parse error before %s\n", yytext);
				exit(0);
			} 
		} else if(type == '(') {
			if(is_ctype_function(pctype) == F) {
				printf("error: subscripted value '%s' is not a function\n", pctype->name);
				exit(0);
			}
			
			CType **ap = malloc(sizeof(CType *) * MAX_NR_OF_PARAMS); 
			call_rop * crop = malloc(sizeof(call_rop));
			crop->ap = ap;
			crop->cnts = 0;
			int n =0;
			int i = 0;
			bool flag;

			while(1) {
				if(n >= MAX_NR_OF_PARAMS) {
					printf("error: Too many parameters\n");
					exit(0);
				}
				if(assignment_expr(&ap[n]) == 0) {					
					flag = T;
					break;
				}
				n++;
				
				if(gettoken() != ',') {
					restoretoken();
					flag = F;
					break;
				}
			}
			if(n > 0 && flag == T) restoretoken2(",");
			
			/* generate syntax tree for function call 
				 eg.   func(pa, pb)
					   CALL_OP	
					    /   \
				        func  crop
			*/
			CType* op = create_op_ctype();
			op->op_t = CALL_OP;
			if(!rootp)
				op->lchild = pctype;
			else
				op->lchild = rootp;
			op->rchild = (CType *)crop;
			rootp = op;			
			crop->cnts = n;

			if((type = gettoken()) != ')') {
				printf("error: parse error before %s\n", yytext);
				exit(0);
			}

		}
		else if(type == '.' || type == PTR_OP)  {  
			int op_t = type;

			type = gettoken();
			if(type != IDENTIFIER) {
				printf("parse error before %s\n", yytext);
				exit(0);
			}

			/* 
			     eg.   struct.id
			            .   
			            | \
			           struct id  
			*/
			CType* op = create_op_ctype();
			op->op_t = op_t;
			if(!rootp)
				op->lchild = pctype;
			else
				op->lchild = rootp;
			op->rchild =  (CType *)strdup(yytext);
			rootp = op;
			
		} else if(type == INC_OP || type == DEC_OP) {
			/* 
			     eg.   i++                i--
			           ++                  --
			           /  \                 /  \
			          i    null             i   null
			*/
			CType* op = create_op_ctype();
			op->op_t = type;
			if(!rootp)
				op->lchild = pctype;
			else
				op->lchild = rootp;
			op->rchild =  NULL;
			rootp = op;

		} else 
			break;
	}

	*p = rootp;
	restoretoken();
	return 1;
}


/*
unary_operator
	: '&'	
	| '*'	
	| '+'	
	| '-'	
	| '~'	
	| '!'	
	;
*/

inline bool unary_operator(int type)
{

	switch(type) {
		case '&':
		case '*':
		case '+':
		case '-':
		case '!':
		case '~':
			return T;
		break;
	}

	return F;
}

/*
unary_expr	
	: postfix_expr	
	| INC_OP unary_expr	
	| DEC_OP unary_expr	
	| unary_operator cast_expr	
	| SIZEOF unary_expr	
	| SIZEOF '(' type_name ')'	
	;
*/

int unary_expr(CType **p)
{
	int type;
	CType *pp;
	struct list_head *l, *curr;

	if(postfix_expr(&pp)) {
		*p = pp;
		return 1;
	}

	type = gettoken();
	if(unary_operator(type) == T) {
		if(type == '&')
			if(Push(&addr_op_stk, (SElemType)1) != R_SUCCESS) {
				printf("Push to stack failed\n");
				exit(-1);	 
			}
			
		if(cast_expr(&pp)) {
			
		} else {
			// error
			printf("error: parse error before\n", yytext);
			exit(0);
		}
		
		CType* op = create_op_ctype();
		op->op_t = type;
		op->lchild = pp;
		op->rchild = NULL;
		*p = op;

		if(type == '&') {
			int symtab;
			if(Pop(&addr_op_stk, (SElemType*)&symtab) != R_SUCCESS) {
				printf("Pop from stack failed\n");
				exit(-1);    
			}
		}
	}
	else if(type == INC_OP || type == DEC_OP) {
		if(unary_expr(&pp)== 0) {
			// error
			printf("error: parse error before\n", yytext);
			exit(0);
		}
		CType* op = create_op_ctype();
		if(type == INC_OP)
			op->op_t = PRE_INC_OP;
		else
			op->op_t = PRE_DEC_OP;
		op->lchild = pp;
		*p = op;
		
	}  else if(type == SIZEOF) {
		CType *ct = NULL;
		CType *tmp= create_ctype();
		tmp->symtype = SYM_TYPE_NO_NAME_VAL;
		tmp->datatype = DATATYPE_NORMAL;
		// tmp->address = 
		// store this val to .data

		tmp->v = create_c_val();
		
		tmp->types.normal.d_attr.has_type_spec = T;
		tmp->types.normal.d_attr.type_spec |= (LONG_MASK|INT_MASK);	
		build_ctype(tmp);
		generate_tmp_ctype(&ct, tmp);
		
		type = gettoken();
		if(type == '(') {
			CType  *new;
			if((new = type_name()) != NULL) {
				tmp->v->content.ull = sizeof_ctype(new, F);
				free_ctype(new);
				*p = ct;
				if(gettoken() != ')') {
					printf("error: parse error before\n", yytext);
					exit(0);
				}
			} else {		/* still unary_expr(), but only must be expr() */
				
				restoretoken2("(");
				goto _unary;		
			}
		} else {
			restoretoken();

_unary:			
			if(unary_expr(&pp)) {
				CType* op = create_op_ctype();
				op->op_t = SIZEOF_OP;
				op->lchild = pp;
				op->rchild = NULL;
				*p = op;				

			} else {
				// error	
				printf("error: parse error before\n", yytext);
				exit(0);
			}
		}
		
	} else {
		restoretoken();
		return 0; 
	}

	return 1;
}

/*
cast_expr	
	: unary_expr	
	| '(' type_name ')' cast_expr	
	;
*/

bool is_cast_available(CType * from, CType *to)
{
	return T;
}

int cast_expr(CType **p)
{
	int type;
	CType *pp;

	type = gettoken();
	if(type == '(') {
		CType  *new;
		if((new = type_name()) != NULL) {
			//tmp->xxx = size_of(ctype);
			if(gettoken() != ')') {
				printf("error: parse error before '%s'\n", yytext);
				exit(0);
			}
			if(cast_expr(&pp)) {
				// make sure *pp can be cast to *new
				if(is_cast_available(pp, new)) {
					free_ctype(pp);
				
				} else {
					// error
					printf("error: can't cast from.. to ..\n");
					exit(0);
				}
			} else {
				// error
				printf("error: parse error from '%s'\n", yytext);
				exit(0);
			}
			
			CType* op = create_op_ctype();
			op->op_t = CAST_OP;
			op->lchild = new;
			op->rchild = pp;
			
			*p = op;

#ifdef   DEBUG				
			out_buffer[0] = '\0';
			print_ctype(new, out_buffer);
			printf(out_buffer);
			printf("\n\r");
#endif

			return 1;
		} else {		/* still unary_expr() */
			restoretoken2("(");
			goto _unary;
		}
	} else {
		restoretoken();
_unary:
		if(unary_expr(&pp)) {

			*p = pp;
#ifdef   DEBUG				
			out_buffer[0] = '\0';
			print_ctype(pp, out_buffer);
			printf(out_buffer);
			printf("\n\r");
#endif			
			return 1;
		} 
	}

	return 0;
}


/*
multiplicative_expr	
	: cast_expr	
	| multiplicative_expr '*' cast_expr	
	| multiplicative_expr '/' cast_expr	
	| multiplicative_expr '%' cast_expr	
	;
*/

bool first_cast_expr_is_parsed = F;
CType * cast_expr_ret;

int multiplicative_expr(CType **p)
{
	int flag = 0;
	int type;
	CType *pp;
	CType *rootp = NULL;
	int casts = 0;

	if(first_cast_expr_is_parsed == T) {
		first_cast_expr_is_parsed = F;
		pp = cast_expr_ret;
		goto after_cast_expr;
	}

	/*
	   eg. A*B%C*D
	   syntax tree like this:
	                      *
	                     / \
	                   %   D
	                 /  \
	               *     C
	             /  \
	           A     B
	*/
	
	while(cast_expr(&pp)) {

after_cast_expr:

		casts++;

		if(rootp)
			rootp->rchild = pp;
			
		flag = 1;
		type = gettoken();
		if(type == '*' || type == '/' || type == '%') {

			CType* op = create_op_ctype();
			op->op_t = type;

			if(rootp)
				op->lchild = rootp;
			else 
				op->lchild = pp;
				
			rootp = op;
			
			flag = 2;
		} else {
			restoretoken();
			goto _OK;
		}

	}

	if(flag == 0) {
		/* not multiplicative_expr */
		return 0;
	}
	if(flag == 2){
		printf("parse error before %s\n", yytext);
		exit(0);
	}

_OK:	
	if(casts == 1)
		*p = pp;
	else 
		*p = rootp;	
	return 1;
}


/*
additive_expr	
	: multiplicative_expr	
	| additive_expr '+' multiplicative_expr	
	| additive_expr '-' multiplicative_expr	
	;
*/

int additive_expr(CType **p)
{
	int flag = 0;
	int type;
	CType *pp;
	CType *rootp = NULL;
	int mexprs = 0;
	
	while(multiplicative_expr(&pp)) {
		mexprs++;

		if(rootp)
			rootp->rchild = pp;
		
		flag = 1;
		type = gettoken();
		if(type == '+' || type == '-' ) {
			CType* op = create_op_ctype();
			op->op_t = type;

			if(rootp)
				op->lchild = rootp;
			else 
				op->lchild = pp;
				
			rootp = op;

			flag = 2;
		} else {
			restoretoken();
			goto _OK;			

		}
	}

	if(flag == 0) {
		/* not additive_expr */
		return 0;
	}
	if(flag == 2){
		printf("parse error before %s\n", yytext);
		exit(0);
	}

_OK:	
	if(mexprs == 1)
		*p = pp;
	else 
		*p = rootp;			
	return 1;
}


/*
shift_expr	
	: additive_expr	
	| shift_expr LEFT_OP additive_expr	
	| shift_expr RIGHT_OP additive_expr	
	;
*/
		
int shift_expr(CType ** p)
{
	int flag = 0;
	int type;
	CType *pp;
	CType * rootp = NULL;
	int aexprs = 0;
	
	while(additive_expr(&pp)) {
		aexprs++;

		if(rootp)
			rootp->rchild = pp;
		
		flag = 1;
		type = gettoken();
		if(type == LEFT_OP || type == RIGHT_OP) {
			CType* op = create_op_ctype();
			op->op_t = type;

			if(rootp)
				op->lchild = rootp;
			else 
				op->lchild = pp;
				
			rootp = op;
			flag = 2;
		} else {
			restoretoken();
			goto _OK;
		}
	}

	if(flag == 0) {
		/* not shift_expr */
		return 0;
	}
	if(flag == 2){
		printf("parse error before %s\n", yytext);
		exit(0);
	}
_OK:	
	if(aexprs == 1)
		*p = pp;
	else 
		*p = rootp;
	return 1;
}


/*
relational_expr	
	: shift_expr	
	| relational_expr '<' shift_expr	
	| relational_expr '>' shift_expr	
	| relational_expr LE_OP shift_expr	
	| relational_expr GE_OP shift_expr	
	;
*/

int relational_expr(CType **p)
{
	int flag = 0;
	int type;
	CType *pp;
	CType *rootp = NULL;
	int sexprs = 0;
		
	while(shift_expr(&pp)) {
		sexprs++;

		if(rootp)
			rootp->rchild = pp;
		
		flag = 1;
		type = gettoken();
		if(type == '<' || type == '>' ||
			type == LE_OP || type == GE_OP ) {
			CType* op = create_op_ctype();
			op->op_t = type;

			if(rootp)
				op->lchild = rootp;
			else 
				op->lchild = pp;
				
			rootp = op;
			flag = 2;
			
		} else {
			restoretoken();
			goto _OK;
		}
	}

	if(flag == 0) {
		/* not relational_expr */
		return 0;
	}
	if(flag == 2){
		printf("parse error before %s\n", yytext);
		exit(0);
	}

_OK:	
	if(sexprs == 1)
		*p = pp;
	else 
		*p = rootp;	
	return 1;
}


/*
equality_expr	
	: relational_expr	
	| equality_expr EQ_OP relational_expr	
	| equality_expr NE_OP relational_expr	
	;

*/

int equality_expr(CType **p)
{
	int flag = 0;
	int type;
	CType *pp;
	CType *rootp = NULL;
	int rexprs = 0;
		
	while(relational_expr(&pp)) {
		rexprs++;

		if(rootp)
			rootp->rchild = pp;
		
		flag = 1;
		type = gettoken();
		if(type == EQ_OP || type == NE_OP) {
			CType* op = create_op_ctype();
			op->op_t = type;

			if(rootp)
				op->lchild = rootp;
			else 
				op->lchild = pp;
				
			rootp = op;
			flag = 2;
		} else {
			restoretoken();
			goto _OK;
		}
	}

	if(flag == 0) {
		/* not equality_expr */
		return 0;
	}
	if(flag == 2){
		printf("parse error before %s\n", yytext);
		exit(0);
	}

_OK:	
	if(rexprs == 1)
		*p = pp;
	else 
		*p = rootp;			
	return 1;
}


/*
and_expr	
	: equality_expr	
	| and_expr '&' equality_expr	
	;
*/

int and_expr(CType **p)
{
	int flag = 0;
	int type;
	CType *pp;
	CType *rootp = NULL;
	int eexpr = 0;
		
	while(equality_expr(&pp)) {
		eexpr++;

		if(rootp)
			rootp->rchild = pp;
		
		flag = 1;
		type = gettoken();
		if(type == '&') {
			CType* op = create_op_ctype();
			op->op_t = type;

			if(rootp)
				op->lchild = rootp;
			else 
				op->lchild = pp;
				
			rootp = op;
			flag = 2;
		} else {
			restoretoken();
			goto _OK;
		}
	}

	if(flag == 0) {
		/* not and_expr */
		return 0;
	}
	if(flag == 2){
		printf("parse error before %s\n", yytext);
		exit(0);
	}

_OK:	
	if(eexpr == 1)
		*p = pp;
	else 
		*p = rootp;				
	return 1;
}



/*
exclusive_or_expr	
	: and_expr	
	| exclusive_or_expr '^' and_expr	
	;
*/

int exclusive_or_expr(CType **p)
{
	int flag = 0;
	int type;
	CType *pp;
	CType *rootp = NULL;
	int aexpr = 0;
		
	while(and_expr(&pp)) {
		aexpr++;

		if(rootp)
			rootp->rchild = pp;
		
		flag = 1;
		type = gettoken();
		if(type == '^') {
			CType* op = create_op_ctype();
			op->op_t = type;

			if(rootp)
				op->lchild = rootp;
			else 
				op->lchild = pp;
				
			rootp = op;
			flag = 2;
		} else {
			restoretoken();
			goto _OK;
		}
	}

	if(flag == 0) {
		/* not exclusive_or_expr */
		return 0;
	}
	if(flag == 2){
		printf("parse error before %s\n", yytext);
		exit(0);
	}
_OK:	
	if(aexpr == 1)
		*p = pp;
	else 
		*p = rootp;			
	return 1;
}



/*
inclusive_or_expr	
	: exclusive_or_expr	
	| inclusive_or_expr '|' exclusive_or_expr	
	;
*/

int inclusive_or_expr(CType **p)
{
	int flag = 0;
	int type;
	CType *pp;
	CType *rootp = NULL;
	int eexpr = 0;
		
	while(exclusive_or_expr(&pp)) {
		eexpr++;

		if(rootp)
			rootp->rchild = pp;
		
		flag = 1;
		type = gettoken();
		if(type == '|') {
			CType* op = create_op_ctype();
			op->op_t = type;

			if(rootp)
				op->lchild = rootp;
			else 
				op->lchild = pp;
				
			rootp = op;
			flag = 2;
		} else {
			restoretoken();
			goto _OK;
		}
	}

	if(flag == 0) {
		/* not inclusive_or_expr */
		return 0;
	}
	if(flag == 2){
		printf("parse error before %s\n", yytext);
		exit(0);
	}

_OK:	
	if(eexpr == 1)
		*p = pp;
	else 
		*p = rootp;
	return 1;
}



/*
logical_and_expr	
	: inclusive_or_expr	
	| logical_and_expr AND_OP inclusive_or_expr	
	;
*/

int logical_and_expr(CType **p)
{
	int flag = 0;
	int type;
	CType *pp;
	CType *rootp = NULL;
	int iexpr = 0;
		
	while(inclusive_or_expr(&pp)) {
		iexpr++;

		if(rootp)
			rootp->rchild = pp;
		
		flag = 1;
		type = gettoken();
		if(type == AND_OP) {
			CType* op = create_op_ctype();
			op->op_t = type;

			if(rootp)
				op->lchild = rootp;
			else 
				op->lchild = pp;
				
			rootp = op;
			flag = 2;
		} else {
			restoretoken();
			goto _OK;
		}
	}

	if(flag == 0) {
		/* not logical_and_expr */
		return 0;
	}
	if(flag == 2){
		printf("parse error before %s\n", yytext);
		exit(0);
	}
	
_OK:	
	if(iexpr == 1)
		*p = pp;
	else 
		*p = rootp;
	return 1;
}



/*
logical_or_expr	
	: logical_and_expr	
	| logical_or_expr OR_OP logical_and_expr	
	;
*/

int logical_or_expr(CType **p)
{
	int flag = 0;
	int type;
	CType *pp;
	CType *rootp = NULL;
	int lexprs = 0;
		
	while(logical_and_expr(&pp)) {
		lexprs++;

		if(rootp)
			rootp->rchild = pp;
		
		flag = 1;
		type = gettoken();
		if(type == OR_OP) {
			CType* op = create_op_ctype();
			op->op_t = type;

			if(rootp)
				op->lchild = rootp;
			else 
				op->lchild = pp;
				
			rootp = op;
			flag = 2;
		} else {
			restoretoken();
			goto _OK;
		}
	}

	if(flag == 0) {
		/* not logical_or_expr */
		return 0;
	}
	if(flag == 2){
		printf("parse error before %s\n", yytext);
		exit(0);
	}

_OK:	
	if(lexprs == 1)
		*p = pp;
	else 
		*p = rootp;
	return 1;
}



/*
conditional_expr	
	: logical_or_expr	
	| logical_or_expr '?' logical_or_expr ':' conditional_expr	
	;
*/
int  conditional_expr(CType **p)
{
	CType *pp;
	CType *pp1;
	CType *pp2;
	
	if(logical_or_expr(&pp)) {
		if(gettoken() == '?') {
			if(logical_or_expr(&pp1)) {
				if(gettoken() == ':') {
					if(conditional_expr(&pp2)) {
						
						CType* op = create_op_ctype();
						op->op_t = COND_OP;
						op->lchild = pp;
						op->rchild = pp1;
						op->child3 = pp2;
						*p = op;
						return 1;
					} else {
						// error
						goto _error;
					}	
				} else {
					// error
					goto _error;
				}
			} else {
				// error
				goto _error;
			}
		} else {
			restoretoken();
			*p = pp;
			return 1;
		}
	}

	return 0;	// not conditional_expr

_error:
	printf("%d: parse error before %s\n", line, yytext);
	exit(0);
	return 1;
}

/*
constant_expr	
	: conditional_expr	
	;
*/

int  constant_expr(CType ** p)
{
	if(conditional_expr(p) != 0)
		return 1;
	
	return 0;   // not constant_expr
}

/*
assignment_operator	
	: '='	
	| MUL_ASSIGN	
	| DIV_ASSIGN	
	| MOD_ASSIGN
	| ADD_ASSIGN	
	| SUB_ASSIGN	
	| LEFT_ASSIGN	
	| RIGHT_ASSIGN	
	| AND_ASSIGN	
	| XOR_ASSIGN	
	| OR_ASSIGN	
	;

*/
inline bool assignment_operator(int type)
{

	switch(type) {
		case '=':
		case MUL_ASSIGN:
		case DIV_ASSIGN:
		case MOD_ASSIGN:
		case ADD_ASSIGN:
		case SUB_ASSIGN:
		case LEFT_ASSIGN:
		case RIGHT_ASSIGN:
		case AND_ASSIGN:
		case XOR_ASSIGN:
		case OR_ASSIGN:
			return T;
		break;
	}

	return F;
}


#if 1

void gen_mem2reg(int dst_breg, CType *tmp, int src_breg, int off)
{
	char * src_breg_str = g_reg_str[src_breg];
	char * dst_breg_str = g_reg_str[dst_breg];

	if(tmp->size_of < 4) {
		// like short = char; short = unsigned char;
		// unsigned short = char; unsigned short = unsigned char;
		// int = char; int = unsigned char; 
		// unsigned int = char; unsigned int = unsigned char; 
		// int = short; int = unsigned short; 
		// unsigned int = short; unsigned int = unsigned short;
		int tmp_is_unsigned = 0;
		if(tmp->types.normal.d_attr.type_spec & TS_USIGNED)
			tmp_is_unsigned = 1;
				
		if(tmp->size_of == 2) {
			if(tmp_is_unsigned) {
				if(off > 0)
					oprintf("\tMOVZX\t%s,WORD [%s+%d]\n", dst_breg_str, src_breg_str, off);
				else if(off < 0)
					oprintf("\tMOVZX\t%s,WORD [%s-%d]\n", dst_breg_str, src_breg_str, -off);
				else
					oprintf("\tMOVZX\t%s,WORD [%s]\n", dst_breg_str, src_breg_str);
			} else {
				if(off > 0)
					oprintf("\tMOVSX\t%s,WORD [%s+%d]\n", dst_breg_str, src_breg_str, off);
				else if(off < 0)
					oprintf("\tMOVSX\t%s,WORD [%s-%d]\n", dst_breg_str, src_breg_str, -off);
				else
					oprintf("\tMOVSX\t%s,WORD [%s]\n", dst_breg_str, src_breg_str);						
			}
			
		} else if (tmp->size_of == 1) {
			if(tmp_is_unsigned) {
				if(off > 0)
					oprintf("\tMOVZX\t%s,BYTE [%s+%d]\n", dst_breg_str, src_breg_str, off);
				else if(off < 0)
					oprintf("\tMOVZX\t%s,BYTE [%s-%d]\n", dst_breg_str, src_breg_str, -off);
				else
					oprintf("\tMOVZX\t%s,BYTE [%s]\n", dst_breg_str, src_breg_str);						
			} else { 
				if(off > 0)
					oprintf("\tMOVSX\t%s,BYTE [%s+%d]\n", dst_breg_str, src_breg_str, off);
				else if(off < 0)
					oprintf("\tMOVSX\t%s,BYTE [%s-%d]\n", dst_breg_str, src_breg_str, -off);
				else
					oprintf("\tMOVSX\t%s,BYTE [%s]\n", dst_breg_str, src_breg_str); 
			}							
		} else 
			goto _error;
	
	} else if( tmp->size_of > 4){
		error(" source oprand size > 4");			
	} else { // i==4
		if(off > 0)
			oprintf("\tMOV\t%s,[%s+%d]\n", dst_breg_str, src_breg_str, off);
		else if(off < 0)
			oprintf("\tMOV\t%s,[%s-%d]\n", dst_breg_str, src_breg_str, -off);
		else
			oprintf("\tMOV\t%s,[%s]\n", dst_breg_str, src_breg_str);	
	}

	return;

_error:
	printf("__LINE__ %d ", __LINE__);
	error("This should not happen....");
	return;	
	
}


int gen_ctype2reg(int dst_breg, CType *tmp)
{
	CType * l = tmp;
	int ret = 0;

	if(is_ctype_normal(l) || is_ctype_pointer(l) 
		|| is_ctype_array(l) || is_ctype_function(l)) {
		if(l->storage_type == STORAGE_CONST){
		} else if(l->storage_type == STORAGE_LOCAL 
				|| l->storage_type == STORAGE_PARAM) {
			gen_mem2reg(dst_breg, l, EBP, l->offset);
		} else if(l->storage_type == STORAGE_GLOBAL || l->storage_type == STORAGE_STATIC) {
			error("not OK yet...\n");
		} else if(l->storage_type == STORAGE_REG) {
			if(l->lvalue)
				gen_mem2reg(dst_breg, l, l->reg >> REG_SHF, 0);
			else
				oprintf("\tMOV\t%s,%s\n", g_reg_str[dst_breg], g_reg_str[l->reg >> REG_SHF]);
			freereg(l->reg);
		} else if(l->storage_type == STORAGE_REGOFF) {
			gen_mem2reg(dst_breg, l, l->reg >> REG_SHF, l->offset);
		} else {
			printf("bad storage type %d\n", l->storage_type);
			exit(0);
		}
		ret = STORAGE_REG;
	}else if(is_ctype_struct(l) || is_ctype_union(l)) { 
		if(l->storage_type == STORAGE_LOCAL 
				|| l->storage_type == STORAGE_PARAM) {
			if(l->offset > 0)
				oprintf("\tLEA\t%s,[EBP+%d]\n", g_reg_str[dst_breg], l->offset);
			else if(l->offset < 0)
				oprintf("\tLEA\t%s,[EBP-%d]\n", g_reg_str[dst_breg], -l->offset);
			else 
				oprintf("\tLEA\t%s,[EBP]\n", g_reg_str[dst_breg]);	
			l->offset = 0;
		} else if(l->storage_type == STORAGE_GLOBAL || l->storage_type == STORAGE_STATIC) {
			error("not OK yet...\n");
		} else if(l->storage_type == STORAGE_REG || l->storage_type == STORAGE_REGOFF) {
			oprintf("\tMOV\t%s,%s\n", g_reg_str[dst_breg], g_reg_str[l->reg >> REG_SHF]);
			freereg(l->reg);
		} else {
			printf("bad storage type %d\n", l->storage_type);
			exit(0);
		}
		ret = STORAGE_REGOFF;
	} else {
		printf("%d: error: wrong type...\n", line);
		exit(0);
	}

	return ret;
}

void gen_general_assign(CType *pp, int dst_breg, CType *tmp, int src_breg)
{
	bool need_adjust = F;
	int i = tmp->size_of;
	int off1 = pp->offset;
	int off2 = tmp->offset;
	int reg_idx = getreg(REG_FLAG_NORMAL, 4);

	if(g_ajust_offset_needed && dst_breg == EBP)
		need_adjust = T;

	if(reg_idx < 0) {   // out of GP-register, EDX is used
	                    // and EDX is not needed to be freed in this situation, because it's freed 'naturally'  
		reg_idx = (EDX<<REG_SHF)| 0xffff;
	}
		
	char * src_breg_str = g_reg_str[src_breg];
	char * dst_breg_str = g_reg_str[dst_breg];

	if(i > 64) {
		// if size is too large, we will use like memcpy
		g_func_ctx.is_EDI_used = T;
		g_func_ctx.is_ESI_used = T;

		oprintf("\tPUSH\tECX\n");
		oprintf("\tPUSH\tEAX\n");
		oprintf("\tPUSH\tESI\n");
		oprintf("\tPUSH\tEDI\n");
		oprintf("\tMOV\tECX,%d\n", i/4);

		if(off2 > 0)
			oprintf("\tMOV\t%s,[%s+%d]\n", g_reg_str[ESI], src_breg_str, off2);
		else if(off2 < 0)
			oprintf("\tMOV\t%s,[%s-%d]\n", g_reg_str[ESI], src_breg_str, -off2);
		else
			oprintf("\tMOV\t%s,[%s]\n", g_reg_str[ESI], src_breg_str);

		if(off1 > 0)
			oprintf("\tMOV\t%s,[%s+%d]\n", g_reg_str[EDI], dst_breg_str, off1);
		else if(off1 < 0)
			oprintf("\tMOV\t%s,[%s-%d]\n", g_reg_str[EDI], dst_breg_str, -off1);
		else 
			oprintf("\tMOV\t%s,[%s]\n", g_reg_str[EDI], dst_breg_str);

#if 0		
		oprintf("\tREP;MOVSL\n");
#else
		char *str = itoa(g_label, 0, 10);
		oprintf("L%s:\n", str);
		oprintf("\tMOV\tEAX,[ESI]\n");
		oprintf("\tMOV\t[EDI],EAX\n");
		oprintf("\tADD\tEDI,4\n");
		oprintf("\tADD\tESI,4\n");
		oprintf("\tDEC\tECX\n");
		oprintf("\tJNZ\tL%s\n", str);
		free(str);
		g_label++;
#endif
		if(i & 3) {
			oprintf("\tMOV\tECX,%d\n", i&3);
#if 0			
			oprintf("\tREP;MOVSB\n"); 
#else
			str = itoa(g_label, 0, 10);
			oprintf("L%s:\n", str);
			oprintf("\tMOV\tAL,BYTE [ESI]\n");
			oprintf("\tMOV\tBYTE [EDI],AL\n");
			oprintf("\tINC\tEDI\n");
			oprintf("\tINC\tESI\n");
			oprintf("\tDEC\tECX\n");
			oprintf("\tJNZ\tL%s\n", str);
			free(str);
			g_label++;	
#endif			
		}

		oprintf("\tPOP\tEDI\n");
		oprintf("\tPOP\tESI\n");
		oprintf("\tPOP\tEAX\n");
		oprintf("\tPOP\tECX\n");

	} else {
	for(; i>0; i-=4) {
		if(i < 4) {
			if(pp->size_of > tmp->size_of) {
				// like short = char; short = unsigned char;
				// unsigned short = char; unsigned short = unsigned char;
				// int = char; int = unsigned char; 
				// unsigned int = char; unsigned int = unsigned char; 
				// int = short; int = unsigned short; 
				// unsigned int = short; unsigned int = unsigned short;
				int tmp_is_unsigned = 0;
				if(tmp->types.normal.d_attr.type_spec & TS_USIGNED)
						tmp_is_unsigned = 1;
						
				if(pp->size_of == 4 && tmp->size_of == 2) {
					if(tmp_is_unsigned) {
						if(off2 > 0)
							oprintf("\tMOVZX\t%s,WORD [%s+%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, off2);
						else if(off2 < 0)
							oprintf("\tMOVZX\t%s,WORD [%s-%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, -off2);
						else
							oprintf("\tMOVZX\t%s,WORD [%s]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str);
					} else {
						if(off2 > 0)
							oprintf("\tMOVSX\t%s,WORD [%s+%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, off2);
						else if(off2 < 0)
							oprintf("\tMOVSX\t%s,WORD [%s-%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, -off2);
						else
							oprintf("\tMOVSX\t%s,WORD [%s]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str);						
					}

					if(off1 > 0)
						oprintf(need_adjust? "\tMOV\t[%s+(X)%d],%s\n":"\tMOV\t[%s+%d],%s\n", 
							dst_breg_str, off1, g_reg_str[reg_idx >> REG_SHF]);
					else if(off1 < 0)
						oprintf(need_adjust? "\tMOV\t[%s-(X)%d],%s\n":"\tMOV\t[%s-%d],%s\n", 
							dst_breg_str, -off1, g_reg_str[reg_idx >> REG_SHF]);
					else
						oprintf(need_adjust? "\tMOV\t[%s(X)],%s\n":"\tMOV\t[%s],%s\n", 
							dst_breg_str, g_reg_str[reg_idx >> REG_SHF]);
					
				} else if (pp->size_of == 4 && tmp->size_of == 1) {
					if(tmp_is_unsigned) {
						if(off2 > 0)
							oprintf("\tMOVZX\t%s,BYTE [%s+%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, off2);
						else if(off2 < 0)
							oprintf("\tMOVZX\t%s,BYTE [%s-%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, -off2);
						else
							oprintf("\tMOVZX\t%s,BYTE [%s]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str);						
					} else { 
						if(off2 > 0)
							oprintf("\tMOVSX\t%s,BYTE [%s+%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, off2);
						else if(off2 < 0)
							oprintf("\tMOVSX\t%s,BYTE [%s-%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, -off2);
						else
							oprintf("\tMOVSX\t%s,BYTE [%s]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str); 
					}
					if(off1 > 0)
						oprintf(need_adjust? "\tMOV\t[%s+(X)%d], %s\n":"\tMOV\t[%s+%d], %s\n", 
							dst_breg_str, off1, g_reg_str[reg_idx >> REG_SHF]);
					else if(off1 < 0)
						oprintf(need_adjust? "\tMOV\t[%s-(X)%d], %s\n":"\tMOV\t[%s-%d], %s\n", 
							dst_breg_str, -off1, g_reg_str[reg_idx >> REG_SHF]);
					else
						oprintf(need_adjust? "\tMOV\t[%s(X)], %s\n":"\tMOV\t[%s], %s\n", 
							dst_breg_str, g_reg_str[reg_idx >> REG_SHF]);							
				} else if (pp->size_of == 2 && tmp->size_of == 1) {
					if(tmp_is_unsigned) {
						if(off2 > 0)
							oprintf("\tMOVZX\t%s,BYTE [%s+%d]\n", g_sub16_reg_str[reg_idx >> REG_SHF], src_breg_str, off2);
						else if(off2 < 0)
							oprintf("\tMOVZX\t%s,BYTE [%s-%d]\n", g_sub16_reg_str[reg_idx >> REG_SHF], src_breg_str, -off2);
						else
							oprintf("\tMOVZX\t%s,BYTE [%s]\n", g_sub16_reg_str[reg_idx >> REG_SHF], src_breg_str);						
					} else {
						if(off2 > 0)
							oprintf("\tMOVSX\t%s,BYTE [%s+%d]\n", g_sub16_reg_str[reg_idx >> REG_SHF], src_breg_str, off2);
						else if(off2 < 0)
							oprintf("\tMOVSX\t%s,BYTE [%s-%d]\n", g_sub16_reg_str[reg_idx >> REG_SHF], src_breg_str, -off2);
						else
							oprintf("\tMOVSX\t%s,BYTE [%s]\n", g_sub16_reg_str[reg_idx >> REG_SHF], src_breg_str);						
					}
					if(off1 > 0)
						oprintf(need_adjust? "\tMOV\t[%s+(X)%d],%s\n":"\tMOV\t[%s+%d],%s\n", 
							dst_breg_str, off1, g_sub16_reg_str[reg_idx >> REG_SHF]);
					else if(off1 < 0)
						oprintf(need_adjust? "\tMOV\t[%s-(X)%d],%s\n":"\tMOV\t[%s-%d],%s\n", 
							dst_breg_str, -off1, g_sub16_reg_str[reg_idx >> REG_SHF]);
					else
						oprintf(need_adjust? "\tMOV\t[%s(X)],%s\n":"\tMOV\t[%s],%s\n", 
							dst_breg_str, g_sub16_reg_str[reg_idx >> REG_SHF]);					
				} else 
					goto _error;
				
			} else if (pp->size_of < tmp->size_of) {
				// like char = short; char = unsigned short
				//  unsigned char = short;  unsigned char = unsigned short;
				assert(tmp->size_of == 2);
				if(off2 > 0)
					oprintf("\tMOVZX\t%s,WORD [%s+%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, off2);
				else if(off2 < 0)
					oprintf("\tMOVZX\t%s,WORD [%s-%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, -off2);
				else
					oprintf("\tMOVZX\t%s,WORD [%s]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str);
				
				if(off1 > 0)
					oprintf(need_adjust? "\tMOV\t[%s+(X)%d],%s\n":"\tMOV\t[%s+%d],%s\n", dst_breg_str, off1, g_sub8l_reg_str[reg_idx >> REG_SHF]);
				else if(off1 < 0)
					oprintf(need_adjust? "\tMOV\t[%s-(X)%d],%s\n":"\tMOV\t[%s-%d],%s\n", dst_breg_str, -off1, g_sub8l_reg_str[reg_idx >> REG_SHF]);
				else
					oprintf(need_adjust? "\tMOV\t[%s(X)],%s\n":"\tMOV\t[%s],%s\n", dst_breg_str, g_sub8l_reg_str[reg_idx >> REG_SHF]);				
			} else {  // pp->size_of == tmp->size_of
				// like char = char; short = short;
				// like unsigned char = unsigned char; unsigned short = unsigned short;
				if(pp->size_of == 1) {
					if(off2 > 0)
						oprintf("\tMOVZX\t%s,BYTE [%s+%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, off2);
					else if(off2 < 0)
						oprintf("\tMOVZX\t%s,BYTE [%s-%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, -off2);
					else
						oprintf("\tMOVZX\t%s,BYTE [%s]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str);					

					if(off1 > 0)
						oprintf(need_adjust? "\tMOV\t[%s+(X)%d],%s\n":"\tMOV\t[%s+%d],%s\n", 
							dst_breg_str, off1, g_sub8l_reg_str[reg_idx >> REG_SHF]);
					else if(off1 < 0)
						oprintf(need_adjust? "\tMOV\t[%s-(X)%d],%s\n":"\tMOV\t[%s-%d],%s\n", 
							dst_breg_str, -off1, g_sub8l_reg_str[reg_idx >> REG_SHF]);
					else
						oprintf(need_adjust? "\tMOV\t[%s(X)],%s\n":"\tMOV\t[%s],%s\n", 
							dst_breg_str, g_sub8l_reg_str[reg_idx >> REG_SHF]);

				} else if (pp->size_of == 2) {
					assert((pp->types.normal.d_attr.type_spec & TS_SHORT)  != 0);
					if((pp->types.normal.d_attr.type_spec == TS_USHORT) ||
						(pp->types.normal.d_attr.type_spec == TS_USHORT_INT)) {
						if(off2 > 0)
							oprintf("\tMOVZX\t%s,WORD [%s+%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, off2);
						else if(off2 < 0)
							oprintf("\tMOVZX\t%s,WORD [%s-%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, -off2);
						else
							oprintf("\tMOVZX\t%s,WORD [%s]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str);						
					} else {
						if(off2 > 0)
							oprintf("\tMOV\t%s,[%s+%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, off2);
						else if(off2 < 0)
							oprintf("\tMOV\t%s,[%s-%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, -off2);
						else
							oprintf("\tMOV\t%s,[%s]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str);						
					}
					if(off1 > 0)
						oprintf(need_adjust? "\tMOV\t[%s+(X)%d],%s\n":"\tMOV\t[%s+%d],%s\n", 
							dst_breg_str, off1, g_sub16_reg_str[reg_idx >> REG_SHF]);
					else if(off1 < 0)
						oprintf(need_adjust? "\tMOV\t[%s-(X)%d],%s\n":"\tMOV\t[%s-%d],%s\n", 
							dst_breg_str, -off1, g_sub16_reg_str[reg_idx >> REG_SHF]);
					else
						oprintf(need_adjust? "\tMOV\t[%s(X)],%s\n":"\tMOV\t[%s],%s\n", 
							dst_breg_str, g_sub16_reg_str[reg_idx >> REG_SHF]);					
				} else 
					goto _error;
			}
			
		} else if( i > 4){
			if(off2 > 0)
				oprintf("\tMOV\t%s,[%s+%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, off2);
			else if(off2 < 0)
				oprintf("\tMOV\t%s,[%s-%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, -off2);
			else
				oprintf("\tMOV\t%s,[%s]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str);			

			if(off1 > 0)
				oprintf(need_adjust? "\tMOV\t[%s+(X)%d],%s\n":"\tMOV\t[%s+%d],%s\n", 
					dst_breg_str, off1, g_reg_str[reg_idx >> REG_SHF]);
			else if(off1 < 0)
				oprintf(need_adjust? "\tMOV\t[%s-(X)%d],%s\n":"\tMOV\t[%s-%d],%s\n", 
					dst_breg_str, -off1, g_reg_str[reg_idx >> REG_SHF]);
			else
				oprintf(need_adjust? "\tMOV\t[%s(X)],%s\n":"\tMOV\t[%s],%s\n", 
					dst_breg_str, g_reg_str[reg_idx >> REG_SHF]);			
		} else { // i==4
			if(off2 > 0)
				oprintf("\tMOV\t%s,[%s+%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, off2);
			else if(off2 < 0)
				oprintf("\tMOV\t%s,[%s-%d]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str, -off2);
			else
				oprintf("\tMOV\t%s,[%s]\n", g_reg_str[reg_idx >> REG_SHF], src_breg_str);	
			
			if(pp->size_of == 4) {
				if(off1 > 0)
					oprintf(need_adjust? "\tMOV\t[%s+(X)%d],%s\n":"\tMOV\t[%s+%d],%s\n", dst_breg_str, off1, g_reg_str[reg_idx >> REG_SHF]);
				else if(off1 < 0)
					oprintf(need_adjust? "\tMOV\t[%s-(X)%d],%s\n":"\tMOV\t[%s-%d],%s\n", dst_breg_str, -off1, g_reg_str[reg_idx >> REG_SHF]);
				else
					oprintf(need_adjust? "\tMOV\t[%s(X)],%s\n":"\tMOV\t[%s],%s\n", dst_breg_str, g_reg_str[reg_idx >> REG_SHF]);				
			} else if(pp->size_of == 2) {
				if(off1 > 0)
					oprintf(need_adjust? "\tMOV\t[%s+(X)%d],%s\n":"\tMOV\t[%s+%d],%s\n", dst_breg_str, off1, g_sub16_reg_str[reg_idx >> REG_SHF]);
				else if(off1 < 0)
					oprintf(need_adjust? "\tMOV\t[%s-(X)%d],%s\n":"\tMOV\t[%s-%d],%s\n", dst_breg_str, -off1, g_sub16_reg_str[reg_idx >> REG_SHF]);
				else
					oprintf(need_adjust? "\tMOV\t[%s(X)],%s\n":"\tMOV\t[%s],%s\n", dst_breg_str, g_sub16_reg_str[reg_idx >> REG_SHF]);				
				
			} else if(pp->size_of == 1) {
				if(off1 > 0)
					oprintf(need_adjust? "\tMOV\t[%s+(X)%d],%s\n":"\tMOV\t[%s+%d],%s\n", dst_breg_str, off1, g_sub8l_reg_str[reg_idx >> REG_SHF]);
				else if(off1 < 0)
					oprintf(need_adjust? "\tMOV\t[%s-(X)%d],%s\n":"\tMOV\t[%s-%d],%s\n", dst_breg_str, -off1, g_sub8l_reg_str[reg_idx >> REG_SHF]);
				else
					oprintf(need_adjust? "\tMOV\t[%s(X)],%s\n":"\tMOV\t[%s],%s\n", dst_breg_str, g_sub8l_reg_str[reg_idx >> REG_SHF]);				
				
			} else if(pp->size_of > 4){
				// this should be last 4 bytes of struct or union
				if(off1 > 0)
					oprintf(need_adjust? "\tMOV\t[%s+(X)%d],%s\n":"\tMOV\t[%s+%d],%s\n", dst_breg_str, off1, g_reg_str[reg_idx >> REG_SHF]);
				else if(off1 < 0)
					oprintf(need_adjust? "\tMOV\t[%s-(X)%d],%s\n":"\tMOV\t[%s-%d],%s\n", dst_breg_str, -off1, g_reg_str[reg_idx >> REG_SHF]);
				else
					oprintf(need_adjust? "\tMOV\t[%s(X)],%s\n":"\tMOV\t[%s],%s\n", dst_breg_str, g_reg_str[reg_idx >> REG_SHF]);				
				
			} else {
				goto _error;
			}
		}
		off2 += 4;
		off1 += 4;
	}
	}
	freereg(reg_idx);
	return;

_error:
	printf("__LINE__ %d ", __LINE__);
	error("This should not happen....");
	return;	
	
}
#endif

/*
   generate code for 'pp' '=' 'tmp' 
*/
void gen_assign(CType *pp, CType *tmp)
{
	bool need_ajust = F;
	
	if(!pp->lvalue) {
		printf("line %d: error: invalid lvalue in assignment\n", line);		
		exit(0);
	}

	if(g_ajust_offset_needed) {
		if(pp->storage_type != STORAGE_LOCAL)
			error("This should not happen!!!!!!");
		need_ajust = T;
	}
	// printf("debug %d = %d\n", pp->storage_type, tmp->storage_type);
	
	if(tmp->storage_type == STORAGE_CONST) {
#if   0				
		out2[0] = '\0';
		printf("\r\n ********\r\n");
		print_ctype(pp, out2);
		printf(out2);
		printf("\n\r");
#endif

		if(pp->storage_type == STORAGE_LOCAL || 
			pp->storage_type == STORAGE_PARAM) {
#ifdef TEST_OUTPUT
			char *mov_s;
 
			if(pp->size_of == 1)
				mov_s = "MOV\tBYTE ";
			else if(pp->size_of == 2)
				mov_s = "MOV\tWORD ";
			else if(pp->size_of == 4)
				mov_s = "MOV\tDWORD ";
			else {
				/* TODO: struct or union assigment */
			}

			if(pp->offset > 0)
				oprintf(need_ajust?"\t%s[EBP+(X)%d],0x%x\n":"\t%s[EBP+%d],0x%x\n", 
					mov_s, pp->offset, tmp->v->content.ull);
			else if(pp->offset < 0)
				oprintf(need_ajust?"\t%s[EBP-(X)%d],0x%x\n":"\t%s[EBP-%d],0x%x\n", 
					mov_s, -pp->offset, tmp->v->content.ull);
			else
				oprintf(need_ajust?"\t%s[EBP(X)],0x%x\n":"\t%s[EBP],0x%x\n",
					mov_s, tmp->v->content.ull);
				
#endif			
		} else if(pp->storage_type == STORAGE_REG) {
#ifdef TEST_OUTPUT
			char *mov_s;
			if(pp->size_of == 1) {
				mov_s = "MOV\tBYTE "; 
				tmp->v->content.ull &= 0xff;
			}
			else if(pp->size_of == 2) {
				mov_s = "MOV\tWORD "; 
				tmp->v->content.ull &= 0xffff;
			}
			else if(pp->size_of == 4)
				mov_s = "MOV\tDWORD ";
			else {
				/* TODO: struct or union assigment */
			}

			oprintf("\t%s[%s], 0x%x\n", mov_s, g_reg_str[pp->reg >> REG_SHF], tmp->v->content.ull);
#endif

		} else if(pp->storage_type == STORAGE_REGOFF) {
#ifdef TEST_OUTPUT
			char *mov_s;
			if(pp->size_of == 1) {
				mov_s = "MOV\tBYTE "; 
				tmp->v->content.ull &= 0xff;
			}
			else if(pp->size_of == 2) {
				mov_s = "MOV\tWORD "; 
				tmp->v->content.ull &= 0xffff;
			}
			else if(pp->size_of == 4)
				mov_s = "MOV\tDWORD ";
			else {
				/* TODO: struct or union assigment */
			}

			if(pp->offset == 0) 
				oprintf("\t%s[%s],0x%x\n", mov_s,
					g_reg_str[pp->reg >> REG_SHF], tmp->v->content.ull);
			else if(pp->offset > 0)
				oprintf("\t%s[%s+%d],0x%x\n", mov_s, g_reg_str[pp->reg >> REG_SHF], pp->offset, 
					 tmp->v->content.ull);
			else
				oprintf("\t%s[%s-%d],0x%x\n", mov_s, g_reg_str[pp->reg >> REG_SHF], -pp->offset, 
					 tmp->v->content.ull);		
#endif			
		} else if(pp->storage_type == STORAGE_STKREG) {
#ifdef TEST_OUTPUT
			if(pp->offset > 0)
				oprintf("\tMOV\t%s,[EBP+%d]\n", g_reg_str[EDX], pp->offset);
			else if(pp->offset < 0)
				oprintf("\tMOV\t%s,[EBP-%d]\n", g_reg_str[EDX], -pp->offset);
			else
				oprintf("\tMOV\t%s,[EBP]\n", g_reg_str[EDX]);
			
			char *mov_s;
			if(pp->size_of == 1) {
				mov_s = "MOV\tBYTE "; 
				tmp->v->content.ull &= 0xff;
			}
			else if(pp->size_of == 2) {
				mov_s = "MOV\tWORD "; 
				tmp->v->content.ull &= 0xffff;
			}
			else if(pp->size_of == 4)
				mov_s = "MOV\tDWORD ";
			else {
				/* TODO: struct or union assigment */
			}

			oprintf("\t%s[%s],0x%x\n", mov_s, g_reg_str[EDX], tmp->v->content.ull);
#endif	
		}

	}
	else if(tmp->storage_type == STORAGE_LOCAL 
		|| tmp->storage_type== STORAGE_PARAM) {
		if(pp->storage_type == STORAGE_LOCAL
			|| pp->storage_type== STORAGE_PARAM) {
#ifdef TEST_OUTPUT
			/*
			  *  struct can assign to struct, or only size <= 4 (including pointer).
			  *  Now we not support 64bits type, eg. long long type.
			  */
			assert((pp->size_of == tmp->size_of) || (pp->size_of <= 4 && tmp->size_of <= 4) );
			gen_general_assign(pp, EBP, tmp, EBP);
#endif			
		} else if(pp->storage_type == STORAGE_REG){
#ifdef TEST_OUTPUT
			gen_general_assign(pp, pp->reg >> REG_SHF, tmp, EBP);
#endif		
		} else if(pp->storage_type == STORAGE_STKREG) {
#ifdef TEST_OUTPUT
			g_func_ctx.is_ESI_used = T;
			if(pp->offset > 0)
				oprintf("\tMOV\t%s,[EBP+%d]\n", g_reg_str[ESI], pp->offset);
			else if(pp->offset < 0)
				oprintf("\tMOV\t%s,[EBP-%d]\n", g_reg_str[ESI], -pp->offset);
			else
				oprintf("\tMOV\t%s,[EBP]\n", g_reg_str[ESI]);
			
			int save_offset = pp->offset;
			pp->offset = 0;
			pp->storage_type = STORAGE_REG;
			gen_general_assign(pp, ESI, tmp, EBP);
			pp->offset = save_offset;
			pp->storage_type = STORAGE_STKREG;
#endif	
		} else if(pp->storage_type == STORAGE_REGOFF) {
#ifdef TEST_OUTPUT
			gen_general_assign(pp, pp->reg >> REG_SHF, tmp, EBP);
#endif	
		}
	} else if(tmp->storage_type == STORAGE_REG){
		if(pp->storage_type == STORAGE_LOCAL
			|| pp->storage_type== STORAGE_PARAM) {

#ifdef TEST_OUTPUT
			if(tmp->lvalue) {
				gen_general_assign(pp, EBP, tmp, tmp->reg >> REG_SHF);
				
			} else {
				char *reg_str;
				if(pp->size_of == 4)
					reg_str = g_reg_str[tmp->reg >> REG_SHF]; 
				else if(pp->size_of == 2)
					reg_str = g_sub16_reg_str[tmp->reg >> REG_SHF];
				else if(pp->size_of == 1)
					reg_str = g_sub8l_reg_str[tmp->reg >> REG_SHF]; 
				else 
					goto _error;

				if(pp->offset > 0)
					oprintf(need_ajust? "\tMOV\t[EBP+(X)%d],%s\n":"\tMOV\t[EBP+%d],%s\n", 
						pp->offset, reg_str);
				else if(pp->offset< 0)
					oprintf(need_ajust? "\tMOV\t[EBP-(X)%d],%s\n":"\tMOV\t[EBP-%d],%s\n", 
						-pp->offset, reg_str);
				else
					oprintf(need_ajust? "\tMOV\t[EBP(X)],%s\n":"\tMOV\t[EBP],%s\n", reg_str);
			}
#endif			
		}
		else if (pp->storage_type == STORAGE_REG || pp->storage_type == STORAGE_REGOFF){
#ifdef TEST_OUTPUT
			if(tmp->lvalue) {
				gen_general_assign(pp, pp->reg >> REG_SHF, tmp, tmp->reg >> REG_SHF);
				
			} else {
				char *reg_str;
				if(tmp->size_of == 4) {	
					if(pp->size_of == 4)
						reg_str = g_reg_str[tmp->reg >> REG_SHF]; 
					else if(pp->size_of == 2)
						reg_str = g_sub16_reg_str[tmp->reg >> REG_SHF];
					else if(pp->size_of == 1)
						reg_str = g_sub8l_reg_str[tmp->reg >> REG_SHF];
					else
						goto _error;
					
					if(pp->offset > 0)
						oprintf("\tMOV\t[%s+%d],%s\n", g_reg_str[pp->reg >> REG_SHF], pp->offset, reg_str);
					else if(pp->offset< 0)
						oprintf("\tMOV\t[%s-%d],%s\n", g_reg_str[pp->reg >> REG_SHF], -pp->offset, reg_str);
					else
						oprintf("\tMOV\t[%s],%s\n", g_reg_str[pp->reg >> REG_SHF], reg_str);
				
				}	
				else if(tmp->size_of == 2)
					reg_str = g_sub16_reg_str[tmp->reg >> REG_SHF];
				else if(tmp->size_of == 1)
					reg_str = g_sub8l_reg_str[tmp->reg >> REG_SHF]; 
				else 
					goto _error;

				if(tmp->size_of == 2 || tmp->size_of == 1) {
					if(pp->offset > 0)
						oprintf("\tMOV\t[%s+%d],%s\n", g_reg_str[pp->reg >> REG_SHF], pp->offset, reg_str);
					else if(pp->offset< 0)
						oprintf("\tMOV\t[%s-%d],%s\n", g_reg_str[pp->reg >> REG_SHF], -pp->offset, reg_str);
					else
						oprintf("\tMOV\t[%s],%s\n", g_reg_str[pp->reg >> REG_SHF], reg_str);	
				}			
			}
#endif			
		} else if(pp->storage_type == STORAGE_STKREG) {
			printf("TODO %d, %d\n", __LINE__, pp->storage_type);

#ifdef TEST_OUTPUT
#if 0  // this looks should not happen, the code is left here
			g_func_ctx.is_ESI_used = T;
			oprintf("\tMOV\t%s,%d[EBP]\n", g_reg_str[ESI], pp->offset);
			pp->offset = 0;
			pp->storage_type = STORAGE_REG;
			gen_general_assign(pp, ESI, tmp, EBP);

			if(tmp->lvalue) {
				gen_general_assign(pp, ESI, tmp, tmp->reg >> REG_SHF);
				
			} else { 
				if(pp->size_of == 4)
					oprintf("\tMOV\t%d[EBP], %s\n", pp->offset, g_reg_str[tmp->reg >> REG_SHF]); 
				else if(pp->size_of == 2)
					oprintf("\tMOV\t%d[EBP], %s\n", pp->offset, g_sub16_reg_str[tmp->reg >> REG_SHF]);
				else if(pp->size_of == 1)
					oprintf("\tMOV\t%d[EBP], %s\n", pp->offset, g_sub8l_reg_str[tmp->reg >> REG_SHF]); 
				else 
					goto _error;

			}
#endif			
#endif	
		} else {
			printf("TODO %d, %d\n", __LINE__, pp->storage_type);
		}
	}
	else if(tmp->storage_type == STORAGE_REGOFF){
		if(pp->storage_type == STORAGE_LOCAL
			|| pp->storage_type== STORAGE_PARAM) {

#ifdef TEST_OUTPUT
			gen_general_assign(pp, EBP, tmp, tmp->reg >> REG_SHF);
#endif
		}
		else if (pp->storage_type == STORAGE_REG){
			gen_general_assign(pp, pp->reg >> REG_SHF, tmp, tmp->reg >> REG_SHF);
		} else {
			printf("@@@@@@\n");
		}
	} else if(tmp->storage_type == STORAGE_STKREG) {
		// 'tmp' rans out of register
		g_func_ctx.is_ESI_used = T;
			
		if(pp->storage_type == STORAGE_LOCAL
			|| pp->storage_type== STORAGE_PARAM) {

#ifdef TEST_OUTPUT
			if(tmp->lvalue) {
				if(tmp->offset > 0)
					oprintf("\tMOV\t%s,[EBP+%d]\n", g_reg_str[ESI], tmp->offset);
				else if(tmp->offset < 0)
					oprintf("\tMOV\t%s,[EBP-%d]\n", g_reg_str[ESI], -tmp->offset);
				else 
					oprintf("\tMOV\t%s,[EBP]\n", g_reg_str[ESI], tmp->offset);
				
				int save_offset = tmp->offset;
				tmp->offset = 0;
				tmp->storage_type = STORAGE_REG;				
				gen_general_assign(pp, EBP, tmp, ESI);
				tmp->offset = save_offset;
				tmp->storage_type = STORAGE_STKREG;
				
			} else {
				char *reg_str;
				if(tmp->offset > 0)
					oprintf("\tMOV\t%s,[EBP+%d]\n", g_reg_str[EDX], tmp->offset);
				else if(tmp->offset < 0)
					oprintf("\tMOV\t%s,[EBP-%d]\n", g_reg_str[EDX], -tmp->offset);
				else 
					oprintf("\tMOV\t%s,[EBP]\n", g_reg_str[EDX], tmp->offset);
				
				if(pp->size_of == 4)
					reg_str = g_reg_str[EDX]; 
				else if(pp->size_of == 2)
					reg_str = g_sub16_reg_str[EDX];
				else if(pp->size_of == 1)
					reg_str = g_sub8l_reg_str[EDX]; 
				else 
					goto _error;
				
				if(pp->offset > 0)
					oprintf(need_ajust?"\tMOV\t[EBP+(X)%d],%s\n":"\tMOV\t[EBP+%d],%s\n", 
						pp->offset, reg_str);
				else if(pp->offset < 0)
					oprintf(need_ajust?"\tMOV\t[EBP-(X)%d],%s\n":"\tMOV\t[EBP-%d],%s\n", 
						-pp->offset, reg_str);
				else 
					oprintf(need_ajust?"\tMOV\t[EBP(X)],%s\n":"\tMOV\t[EBP],%s\n", reg_str);		
			}
#endif			
		}
		else if (pp->storage_type == STORAGE_REG){
			if(tmp->offset > 0)
				oprintf("\tMOV\t%s,[EBP+%d]\n", g_reg_str[ESI], tmp->offset);
			else if(tmp->offset < 0)
				oprintf("\tMOV\t%s,[EBP-%d]\n", g_reg_str[ESI], -tmp->offset);
			else
				oprintf("\tMOV\t%s,[EBP]\n", g_reg_str[ESI], tmp->offset);
			
			int save_offset = tmp->offset;
			tmp->offset = 0;
			tmp->storage_type = STORAGE_REG;			
			gen_general_assign(pp, pp->reg >> REG_SHF, tmp, ESI);
			tmp->offset = save_offset;
			tmp->storage_type = STORAGE_STKREG;
		} else if(pp->storage_type == STORAGE_STKREG) {
#ifdef TEST_OUTPUT
			if(tmp->lvalue) {
				g_func_ctx.is_EDI_used = T;
				if(pp->offset > 0)
					oprintf("\tMOV\t%s,[EBP+%d]\n", g_reg_str[EDI], pp->offset);
				else if(pp->offset < 0)
					oprintf("\tMOV\t%s,[EBP-%d]\n", g_reg_str[EDI], -pp->offset);
				else
					oprintf("\tMOV\t%s,[EBP]\n", g_reg_str[EDI], pp->offset);				
				int save_offset1 = pp->offset;
				pp->offset = 0;
				pp->storage_type = STORAGE_REG;
				if(tmp->offset > 0)
					oprintf("\tMOV\t%s,[EBP+%d]\n", g_reg_str[ESI], tmp->offset);
				else if(tmp->offset < 0)
					oprintf("\tMOV\t%s,[EBP-%d]\n", g_reg_str[ESI], -tmp->offset);
				else
					oprintf("\tMOV\t%s,[EBP]\n", g_reg_str[ESI], tmp->offset);				
				int save_offset2 = tmp->offset;
				tmp->offset = 0;
				tmp->storage_type = STORAGE_REG;				
				gen_general_assign(pp, EDI, tmp, ESI);
				pp->offset = save_offset1;
				tmp->offset = save_offset2;
				tmp->storage_type = pp->storage_type = STORAGE_STKREG;
			} else {

				if(tmp->offset > 0)
					oprintf("\tMOV\t%s,[EBP+%d]\n", g_reg_str[EDX], tmp->offset);
				else if(tmp->offset < 0)
					oprintf("\tMOV\t%s,[EBP-%d]\n", g_reg_str[EDX], -tmp->offset);
				else
					oprintf("\tMOV\t%s,[EBP]\n", g_reg_str[EDX], tmp->offset);
				
				if(pp->offset > 0)
					oprintf("\tMOV\t%s,[EBP+%d]\n", g_reg_str[ESI], pp->offset);
				else if(pp->offset < 0)
					oprintf("\tMOV\t%s,[EBP-%d]\n", g_reg_str[ESI], -pp->offset);
				else
					oprintf("\tMOV\t%s,[EBP]\n", g_reg_str[ESI], pp->offset);

				if(pp->size_of == 4)
					oprintf("\tMOV\t[ESI],%s\n", g_reg_str[EDX]); 
				else if(pp->size_of == 2)
					oprintf("\tMOV\t[ESI],%s\n", g_sub16_reg_str[EDX]);
				else if(pp->size_of == 1)
					oprintf("\tMOV\t[ESI],%s\n", g_sub8l_reg_str[EDX]); 
				else 
					goto _error;
			}		
#endif	
		} 

	}

	return;

_error:
	printf("__LINE__ %d ", __LINE__);
	error("This should not happen....");
	return;
}


void implicit_convert(CType **p1, CType **p2)
{
/*
C语言隐式类型转换规则
1. 如果其中一个操作数为long double类型,则另一个操作数被转换为long double.
2. 否则,如果其中一个操作数为double, 则另一个操作数被转换为double.
3. 否则,如果其中一个操作数为float, 则另一个操作数也转换为float.
4. 否则,两个操作数进行 "整型升级":
    a. 如果其中一个操作数为unsigned long int, 则另一个操作数也被视为unsigned long int.
    b. 否则,如果其中一个操作数为long int,而另一个操作数类型是unsigned int, 
        并且long int能够表示unsigned int的所有值,则另一个操作数也被视为long int;
        如果long int不能表示unsigned int的所有值,则两个数都被视为unsigned long int.
    c. 否则, 如果其中一个操作数是long int,则另一个操作数也被视为long int.
    d. 否则, 如果其中一个操作数是unsigned int, 则另一个操作数也被视为unsigned int.
    e. 否则, 两个操作数都被视为int.

*/

}

CType * gen_cast(CType *ct)
{
	
}

/*
 *  gencode of logical, if 'ct' is not 0, goto 'true_label', else goto 'false_label'
 */
void * gen_logical(CType *ct, int true_label, int false_label)
{
	CType * l = NULL;
	generate_tmp_ctype(&l, ct);

	if(is_ctype_normal(l)) {
		if(l->storage_type == STORAGE_CONST){
		}
		else if(l->storage_type == STORAGE_LOCAL 
				|| l->storage_type == STORAGE_PARAM) {
			char * ops;
			if(l->size_of == 4)
				ops = "CMP\tDWORD";
			else if(l->size_of == 2)
				ops = "CMP\tWORD";
			else if(l->size_of == 1)
				ops = "CMP\tBYTE";
			else
				error("This should not happen.");
				
			if(l->offset > 0)
				oprintf("\t%s [EBP+%d],0\n", ops, l->offset); 
			else if(l->offset < 0)
				oprintf("\t%s [EBP-%d],0\n", ops, -l->offset);
			else 
				oprintf("\t%s [EBP],0\n", ops);

			if(true_label)
				oprintf("\tJNE\tL%s\n", itoa(true_label, 0, 10));
			if(false_label)
				oprintf("\tJE\tL%s\n", itoa(false_label, 0, 10));

		} else if(l->storage_type == STORAGE_GLOBAL || l->storage_type == STORAGE_STATIC) {
			error("not OK yet...\n");
		} else if(l->storage_type == STORAGE_REG) {
			char * ops;
			if(l->size_of == 4)
				ops = "CMP\tDWORD";
			else if(l->size_of == 2)
				ops = "CMP\tWORD";
			else if(l->size_of == 1)
				ops = "CMP\tBYTE";
			else
				error("This should not happen.");

			if(l->lvalue)
				oprintf("\t%s\t[%s],0\n", ops, g_reg_str[l->reg >> REG_SHF]);
			else 
				oprintf("\tCMP\t%s,0\n", g_reg_str[l->reg >> REG_SHF]);
			freereg(l->reg);
				
			if(true_label)
				oprintf("\tJNE\tL%s\n", itoa(true_label, 0, 10));
			if(false_label)
				oprintf("\tJE\tL%s\n", itoa(false_label, 0, 10));

		} else {
			printf("bad storage type %d\n", l->storage_type);
			exit(0);
		}
	} else {
		printf("%d: error: wrong type argument to unary not\n", line);
		exit(0);
	}
	free(l);
}


CType * gen_binop(CType *ct)
{
	if(ct == NULL) 
		return NULL;

	if(ct->datatype != DATATYPE_OP) {
		return ct;
	}
	int tmp_is_unsigned = 0;

	CType * tmp = genop(ct->lchild);
	CType * tmp2 = genop(ct->rchild);
	implicit_convert(&ct->lchild, &ct->rchild);
	CType * l = NULL;
	generate_tmp_ctype(&l, tmp);
	CType * l2 = NULL;
	generate_tmp_ctype(&l2, tmp2);
	int reg_idx, reg_idx2;
	char *reg_str;

	if(tmp->types.normal.d_attr.type_spec & TS_USIGNED)
		tmp_is_unsigned = 1;

	char *opstr;
	switch(ct->op_t){
		case '+':
			opstr = "ADD";
			break;
		case '-':
			opstr = "SUB";
			break;
		case '*':
			opstr = tmp_is_unsigned?"MUL":"IMUL";
			break;
		case '&':
			opstr = "AND";
			break;
		case '|':
			opstr = "OR";
			break;
		case '^':
			opstr = "XOR";
			break;
		case LEFT_OP:      // <<
			opstr = "SHL";
			break;
		case RIGHT_OP:     // >>
			opstr = "SAR";
			break;
		default:
			error("BAD op\n");
	}

	if(is_ctype_normal(l2)) {
		if(l2->storage_type == STORAGE_CONST){
			
		} else if(l2->storage_type == STORAGE_LOCAL || l2->storage_type == STORAGE_PARAM) {
			reg_idx = getreg(REG_FLAG_NORMAL, 4);
			if(reg_idx < 0) {
				error("Out of regs.....\n");
			}
			gen_mem2reg(reg_idx >> REG_SHF, l2, EBP, l2->offset);
		} else if(l2->storage_type == STORAGE_GLOBAL || l2->storage_type == STORAGE_STATIC) {
			error("not OK yet...\n");
		} else if(l2->storage_type == STORAGE_REG) {
			if(l2->lvalue)
				gen_mem2reg(l2->reg >> REG_SHF, l2, l2->reg >> REG_SHF, 0);
			reg_idx = l2->reg;			
		} else {
			printf("bad storage type %d\n", l2->storage_type);
			exit(0);
		}
	} else {
		printf("%d: error: wrong type argument to op %d\n", line, ct->op_t);
		exit(0);
	}

	if(is_ctype_normal(l)) {
		if(l->storage_type == STORAGE_CONST){
		}
		else if(l->storage_type == STORAGE_LOCAL 
				|| l->storage_type == STORAGE_PARAM) {
			reg_idx2 = getreg(REG_FLAG_NORMAL, 4);
			if(reg_idx2 < 0) {
				error("Out of regs.....\n");
			}
			gen_mem2reg(reg_idx2 >> REG_SHF, l, EBP, l->offset);
			oprintf("\t%s\t%s, %s\n", opstr, g_reg_str[reg_idx2 >> REG_SHF], 
									g_reg_str[reg_idx >> REG_SHF]);
			freereg(reg_idx);
			l->link.next = l->link.prev = NULL;
			l->get_from = NULL;
			l->reg = reg_idx2;
			l->storage_type = STORAGE_REG;
			l->offset = 0;
			l->lvalue = 0;
		} else if(l->storage_type == STORAGE_GLOBAL || l->storage_type == STORAGE_STATIC) {
			error("not OK yet...\n");
		} else if(l->storage_type == STORAGE_REG) {	
			if(l->lvalue) {
				gen_mem2reg(l->reg >> REG_SHF, l, l->reg >> REG_SHF, 0);
				oprintf("\t%s\t%s, %s\n", opstr, g_reg_str[l->reg >> REG_SHF], 
										g_reg_str[reg_idx >> REG_SHF]);
			} else {
				oprintf("\t%s\t%s, %s\n", opstr, g_reg_str[l->reg >> REG_SHF], 
										g_reg_str[reg_idx >> REG_SHF]);
			}	
			freereg(reg_idx);
			l->link.next = l->link.prev = NULL;
			l->get_from = NULL;
			l->storage_type = STORAGE_REG;
			l->offset = 0;
			l->lvalue = 0;

		} else {
			printf("bad storage type %d\n", l->storage_type);
			exit(0);
		}
		return l;
	} else {
		printf("%d: error: wrong type argument to op %d\n", line, ct->op_t);
		exit(0);
	}		
}


CType * gen_binop2(CType *ct)
{
	if(ct == NULL) 
		return NULL;

	if(ct->datatype != DATATYPE_OP) {
		return ct;
	}
	int tmp_is_unsigned = 0;

	CType * tmp = genop(ct->lchild);
	CType * tmp2 = genop(ct->rchild);
	implicit_convert(&ct->lchild, &ct->rchild);
	CType * l = NULL;
	generate_tmp_ctype(&l, tmp);
	CType * l2 = NULL;
	generate_tmp_ctype(&l2, tmp2);
	int reg_idx, reg_idx2;
	char *reg_str;
	bool eax_saved = F;

	if(tmp->types.normal.d_attr.type_spec & TS_USIGNED)
		tmp_is_unsigned = 1;

	char *opstr;
	switch(ct->op_t){
		case '/':
			opstr = tmp_is_unsigned?"DIV":"IDIV";
			break;
		case '%':
			opstr = tmp_is_unsigned?"DIV":"IDIV";
			break;
		default:
			error("BAD op\n");
	}

	oprintf("\tMOV\tEDX, 0\n");
	if(is_ctype_normal(l)) {
		if(l->storage_type == STORAGE_CONST){
			
		} else if(l->storage_type == STORAGE_LOCAL || l->storage_type == STORAGE_PARAM) {
			reg_idx = getreg(REG_FLAG_NORMAL, 4);
			if(reg_idx < 0) {
				error("Out of regs.....\n");
			}
			if(reg_idx != ((EAX << REG_SHF)|0xffff)){
				freereg(reg_idx);
				reg_idx == ((EAX << REG_SHF)|0xffff);
				eax_saved = T;
				// TODO : EAX in use, need save EAX
				error("EAX is used");
			} 
			gen_mem2reg(reg_idx >> REG_SHF, l, EBP, l->offset);
		} else if(l->storage_type == STORAGE_GLOBAL || l->storage_type == STORAGE_STATIC) {
			error("not OK yet...\n");
		} else if(l->storage_type == STORAGE_REG) {
			if(l->lvalue)
				gen_mem2reg(l->reg >> REG_SHF, l, l->reg >> REG_SHF, 0);
			reg_idx = l->reg;
			if(reg_idx != ((EAX << REG_SHF)|0xffff)){
				// TODO : EAX in use, save EAX
				error("EAX is used");
				eax_saved = T;
				freereg(reg_idx);
				reg_idx == ((EAX << REG_SHF)|0xffff);				
			}	
		} else {
			printf("bad storage type %d\n", l->storage_type);
			exit(0);
		}
	} else {
		printf("%d: error: wrong type argument to op %d\n", line, ct->op_t);
		exit(0);
	}
	if(tmp_is_unsigned == 0)
		oprintf("\tCDQ\n");   // extend sign from EAX to EDX
	
	if(is_ctype_normal(l2)) {
		if(l2->storage_type == STORAGE_CONST){
			
		} else if(l2->storage_type == STORAGE_LOCAL || l2->storage_type == STORAGE_PARAM) {
			reg_idx2 = getreg(REG_FLAG_NORMAL, 4);
			if(reg_idx2 < 0) {
				error("Out of regs.....\n");
			}
			gen_mem2reg(reg_idx2 >> REG_SHF, l2, EBP, l2->offset);
		} else if(l2->storage_type == STORAGE_GLOBAL || l2->storage_type == STORAGE_STATIC) {
			error("not OK yet...\n");
		} else if(l2->storage_type == STORAGE_REG) {
			if(l2->lvalue)
				gen_mem2reg(l2->reg >> REG_SHF, l2, l2->reg >> REG_SHF, 0);
			reg_idx2 = l2->reg;	
		} else {
			printf("bad storage type %d\n", l2->storage_type);
			exit(0);
		}
	} else {
		printf("%d: error: wrong type argument to op %d\n", line, ct->op_t);
		exit(0);
	}

	oprintf("\t%s\t%s\n", opstr, g_reg_str[reg_idx2 >> REG_SHF]);
	freereg(reg_idx2);
	
	l->link.next = l->link.prev = NULL;
	l->get_from = NULL;
	if(ct->op_t == '/')
		l->reg = (EAX << REG_SHF)|0xffff;
	else { // '%'
		l->reg = (EDX << REG_SHF)|0xffff; 
		freereg(reg_idx);
	}
	l->storage_type = STORAGE_REG;
	l->offset = 0;
	l->lvalue = 0;

	return l;
}

CType * genop(CType * ct)
{
	CType *l;
	CType *r;
		
	if(ct == NULL) 
		return NULL;

	if(ct->datatype != DATATYPE_OP) {
		return ct;
	} 
		
	if(ct->op_t == '=') {
		r = genop(ct->rchild);
		l = genop(ct->lchild);
		
		gen_assign(l, r);
		freereg(l->reg);
		return r;
	} else if(ct->op_t == '.') {
		r = ct->rchild;
		l = genop(ct->lchild);
		CType * pctype = NULL;
		generate_tmp_ctype(&pctype, l);
		freereg(l->reg);
		l = pctype;

		if(!(is_ctype_struct(l) || is_ctype_union(l))) {
			// error
			printf("error: request for member '%s' in something not a structure or union\n",
				(char *)r);
			exit(0);
		}
					
		CType *value;
		if(symtab_lookup(l->types.normal.d_attr.st_un->types.struct_union_enum.table, (char*)r, 
					SYM_TYPE_OBJECT, &value) == R_SUCCESS) {
			/*
			  * Usually Structure definition is in global stage, 
			  * so it's members usually set to be STORAGE_GLOBAL.
			  * Here the struct members's storage type should be set same as the struct 
			  */
			int storage_type = l->storage_type;
			/*
			  * And save offset of 'pctype', since generated new 'pctype' need re-calculate its offset 
			  */
			int offset = l->offset;
			
			generate_tmp_ctype(&l, value);
			
			if(storage_type == STORAGE_REG) { 
				/* if structure address is in register, 
				  * then offset is same as in structure, 'pctype' will be like 8[eax] 
				  */
				l->reg = getreg(REG_FLAG_NORMAL, 4);
				l->storage_type = STORAGE_REGOFF;
			} else if(storage_type == STORAGE_STKREG) {
				printf("###############\n");
			} else if(storage_type == STORAGE_LOCAL ||
						storage_type == STORAGE_PARAM ||
						storage_type == STORAGE_REGOFF ){
				l->storage_type = storage_type;
				l->offset += offset;
			} else {
				printf("##########%d\n", storage_type);
			}
			free((void*)r);
			return l;
		} else {
			printf("error: struct or union has no item named '%s'\n", (char*)r);
			exit(0);
		}

		return NULL;
	} else if(ct->op_t == PTR_OP) { // ->
		r = ct->rchild;
		l = genop(ct->lchild);
		CType * pctype = NULL;
		generate_tmp_ctype(&pctype, l);
		int reg = l->reg;	/* save reg if l is STORAGE_REG */
		l = pctype;
		
		struct list_head *lh;
		struct list_head *curr;
		
		lh = l->link.next->next;
		curr =  l->link.next;

		if(l->link.prev != &(l->link)) {
			/* the poinetr ctype is created from '&', so try to free it */
			CType * elt = list_entry(curr, CType, link);
			free_ctype(elt);
		}
		if(l->link.prev == curr) {	
			l->link.prev = &(l->link);
		}
	
		l->link.next = lh;
		if(is_ctype_struct(l) || is_ctype_union(l)) {
			CType *value;
			if(symtab_lookup(l->types.normal.d_attr.st_un->types.struct_union_enum.table, 
				(char*)r, SYM_TYPE_OBJECT, &value) == R_SUCCESS) {
				int storage_type = l->storage_type;
				int offset = l->offset;

				if(storage_type == STORAGE_LOCAL ||
						storage_type == STORAGE_PARAM) {
						
					int reg_idx = getreg(REG_FLAG_NORMAL, 4);  /* pointer type */
					if(reg_idx >= 0) {
#ifdef  TEST_OUTPUT
						if(offset > 0)
							oprintf("\tMOV\t%s,[EBP+%d]\n", g_reg_str[reg_idx >> REG_SHF], offset); 
						else if(offset < 0)
							oprintf("\tMOV\t%s,[EBP-%d]\n", g_reg_str[reg_idx >> REG_SHF], -offset);
						else 
							oprintf("\tMOV\t%s,[EBP]\n", g_reg_str[reg_idx >> REG_SHF]);
						
#endif
						l = NULL;
						generate_tmp_ctype(&l, value);
						/* we do not know where the structure is stored, local or global,
						  * but the address will be 'pctype''s offset+ [reg].
						  * And offset is 'pctype' is same as offset in structure.
						  */
						l->storage_type = STORAGE_REGOFF;  
						l->reg = reg_idx;
						l->lvalue = 1; 
					} else {
						printf("*** out of reg4 ***\n");
					}
				}else if(storage_type == STORAGE_REG) {
				
				}else if(storage_type == STORAGE_REGOFF) {
					// printf("reg off 0x%x\n", reg);   // debug
#ifdef  TEST_OUTPUT
					if(offset > 0)
						oprintf("\tMOV\t%s,[%s+%d]\n", g_reg_str[reg >> REG_SHF], 
								g_reg_str[reg >> REG_SHF], offset);
					else if(offset < 0)
						oprintf("\tMOV\t%s,%d[%s-%d]\n", g_reg_str[reg >> REG_SHF],  
								g_reg_str[reg >> REG_SHF], -offset);						
					else
						oprintf("\tMOV\t%s,[%s]\n", g_reg_str[reg >> REG_SHF], 
								g_reg_str[reg >> REG_SHF]);						
#endif
					l = NULL;
					generate_tmp_ctype(&l, value);
					/* we do not know where the structure is stored, local or global,
					  * but the address will be 'pctype''s offset+ [reg].
					  * And offset is 'pctype' is same as offset in structure.
					  */
					l->storage_type = STORAGE_REGOFF;  
					l->reg = reg;
					l->lvalue = 1; 
					
				} else {
					printf("????????????????\n");
				}

				free((void *)r);
				return l;
			} else {
				printf("error: struct or union has no item named '%s'\n", (char*)r);
				exit(0);
			}
		} else {
			printf("error: request for member '%s' in not a pointer to structure or union\n",
					(char*)r);
			exit(0);
		}
		return NULL;
	} else if(ct->op_t == INC_OP || ct->op_t == DEC_OP) { // i++ 

		g_pre_inc_code.start = &(OUTBUF[OUTBUFIDX]);
		l = genop(ct->lchild);
		CType * pctype = NULL;
		generate_tmp_ctype(&pctype, l);
		l = pctype;
		
		if(l->lvalue == 0) {
			printf("%d: error: invalid lvalue in increment\n", line);
			exit(0);
		}
			
		if(!is_ctype_pointer(l) && !is_ctype_normal(l)) {
			// error
			printf("error: wrong type argument to increment\n");
			exit(0);
		}

		/* first save content of 'l' */
		if(l->storage_type == STORAGE_LOCAL || l->storage_type == STORAGE_PARAM) {
			int reg_idx = getreg(REG_FLAG_NORMAL, 4);
			if(reg_idx < 0) {
				error("Out of regs.....\n");
			}
			if(l->offset > 0)
				oprintf("\tMOV\t%s,[EBP+%d]\n", g_reg_str[reg_idx >> REG_SHF], l->offset); 
			else if(l->offset < 0)
				oprintf("\tMOV\t%s,[EBP-%d]\n", g_reg_str[reg_idx >> REG_SHF], -l->offset);
			else
				oprintf("\tMOV\t%s,[EBP]\n", g_reg_str[reg_idx >> REG_SHF]);
			l->reg = reg_idx;
			l->storage_type = STORAGE_REG;
			l->offset = 0;

		} else if(l->storage_type == STORAGE_GLOBAL || l->storage_type == STORAGE_STATIC) {
			error("not OK yet...\n");
		} else if(l->storage_type == STORAGE_REG) {
			int reg_idx = getreg(REG_FLAG_NORMAL, 4);
			if(reg_idx < 0) {
				error("Out of regs.....\n");
			}
			if(l->lvalue)
				oprintf("\tMOV\t%s,[%s]\n", g_reg_str[reg_idx >> REG_SHF], g_reg_str[l->reg >> REG_SHF]);
			else 
				oprintf("\tMOV\t%s,%s\n", g_reg_str[reg_idx >> REG_SHF], g_reg_str[l->reg >> REG_SHF]);
			freereg(l->reg);
			l->reg = reg_idx;
			l->storage_type = STORAGE_REG;
			l->offset = 0;
		} else {
			printf("bad storage type %d\n", l->storage_type);
			exit(0);
		}
		g_pre_inc_code.end = &(OUTBUF[OUTBUFIDX]);   
		/* TODO: we can remove these code if only "i++", not "a=i++ " 
		    after assignment_expr()
		*/
		
		/* then do the INC&DEC */
		CType *tmp = create_op_ctype();
		if(ct->op_t == INC_OP)
			tmp->op_t = PRE_INC_OP;
		else 
			tmp->op_t = PRE_DEC_OP;
		tmp->lchild = ct->lchild;
		tmp = genop(tmp);
		freereg(tmp->reg);

		l->lvalue = 0;
		return l;		
	} else if(ct->op_t == '&') {
		if(ct->lchild && ct->rchild) {
			return gen_binop(ct);
		}
		
		if(ct->lchild != NULL && ct->lchild->datatype == DATATYPE_OP 
			&& ct->lchild->op_t == '*') {   // like  &(*p);
			CType *tmp = ct->lchild->lchild;
			if(tmp != NULL && ct->lchild->rchild == NULL) {
				tmp = genop(tmp);
				if(!is_ctype_pointer(tmp)) {
					printf("error: subscripted value is neither array nor pointer\n");
					exit(0);
				}
				return tmp; 
			}
		}
			
		l = genop(ct->lchild);
		CType * pctype = NULL;
		generate_tmp_ctype(&pctype, l);
		l = pctype;
		
		CType * tmp;
		tmp = create_ctype();
		tmp->datatype = DATATYPE_POINTER;
		tmp->types.pointer.is_from_addrop = T;

		tmp->link.next = l->link.next;
		l->link.next = &(tmp->link);
		if(l->link.prev == &(l->link)) {
			l->link.prev = &(tmp->link);
		}
		
		l->types.normal.bitfield_size = 0;	/* bitfield_size not needed */ 

		/*
		  * Now 'pp' will be address of somthing, so save original 'pp' variable  
		  */
		int o_offset = l->offset;
		int storage_t = l->storage_type;
		l->offset = 0;
		l->size_of = sizeof_ctype(l, F);

		if(storage_t == STORAGE_LOCAL
			|| storage_t == STORAGE_PARAM) {
					
			l->reg = getreg(REG_FLAG_NORMAL, 4);

			if(l->reg >= 0) {
				l->storage_type = STORAGE_REG;
				l->lvalue = 0;
				
#if  0				
				out2[0] = '\0';
				printf("\r\n ********\r\n");
				print_ctype(pp, out2);
				printf(out2);
				printf("\n\r");
#endif	

#ifdef  TEST_OUTPUT
				if(o_offset > 0)
					oprintf("\tLEA\t%s,[EBP+%d]\n", g_reg_str[l->reg >> REG_SHF], o_offset);
				else if(o_offset < 0)
					oprintf("\tLEA\t%s,[EBP-%d]\n", g_reg_str[l->reg >> REG_SHF], -o_offset);
				else
					oprintf("\tLEA\t%s,[EBP]\n", g_reg_str[l->reg >> REG_SHF]);
#endif
				 
			} else {
				int stk_reg = getStackReg(); // getStackReg() should never failed 

				l->storage_type = STORAGE_STKREG;
				l->size_of = sizeof_ctype(l, F);
				l->offset = g_func_ctx.stack_reg[stk_reg].offset;
				l->lvalue = 0;
					
#ifdef  TEST_OUTPUT
				if(o_offset > 0)
					oprintf("\tLEA\t%s,[EBP+%d]\n", g_reg_str[EDX], o_offset); 
				else if(o_offset < 0)
					oprintf("\tLEA\t%s,[EBP-%d]\n", g_reg_str[EDX], -o_offset); 
				else
					oprintf("\tLEA\t%s,[EBP]\n", g_reg_str[EDX]); 
				
				if(l->offset > 0)
					oprintf("\tMOV\t[EBP+%d],%s\n", l->offset, g_reg_str[EDX]);
				else if(l->offset < 0)
					oprintf("\tMOV\t[EBP-%d],%s\n", -l->offset, g_reg_str[EDX]);
				else
					oprintf("\tMOV\t[EBP],%s\n", g_reg_str[EDX]);
#endif								
			} 
		}else if(storage_t == STORAGE_REG) {
			printf("////////////\n");
		} else if(storage_t == STORAGE_REGOFF) {
			// l->reg  no change
			l->storage_type = STORAGE_REG;
			l->lvalue = 0;

#ifdef  TEST_OUTPUT
			if(o_offset > 0)
				oprintf("\tADD\t%s,%d\n", g_reg_str[l->reg >> REG_SHF], o_offset);  				
#endif
		}
		return l;
	} else if(ct->op_t == '*') { 
		if(ct->lchild && ct->rchild) {
			return gen_binop(ct);
		}

		CType *pp = genop(ct->lchild);
		struct list_head *l, *curr;

		CType * pctype = NULL;
		generate_tmp_ctype(&pctype, pp);
		pp = pctype;

		if(is_ctype_array(pp) || is_ctype_pointer(pp)) {
			int storage_t = pp->storage_type;
			int o_offset = pp->offset;
			
			l = pp->link.next->next;
			curr =  pp->link.next;
			
			if(pp->link.prev != &(pp->link)) {
				/* the poinetr ctype is created from '&', so try to free it */
				CType * elt = list_entry(curr, CType, link);
				free_ctype(elt);
			}
			if(pp->link.prev == curr) { 	
				pp->link.prev = &(pp->link);
			}
			
			pp->link.next = l;


#ifdef  TEST_OUTPUT
			if(storage_t == STORAGE_LOCAL
				|| storage_t == STORAGE_PARAM) {

				pp->reg = getreg(REG_FLAG_NORMAL, 4); 
				if( pp->reg >= 0) {
					pp->storage_type = STORAGE_REG;
					pp->size_of = sizeof_ctype(pp, F);
					pp->offset = 0;
					pp->lvalue = 1;
						
#ifdef  TEST_OUTPUT  
					if(o_offset > 0)
						oprintf("\tMOV\t%s,[EBP+%d]\n", g_reg_str[pp->reg >> REG_SHF], o_offset); 
					else if(o_offset < 0)
						oprintf("\tMOV\t%s,[EBP-%d]\n", g_reg_str[pp->reg >> REG_SHF], -o_offset); 
					else
						oprintf("\tMOV\t%s,[EBP]\n", g_reg_str[pp->reg >> REG_SHF]);

#endif			
				} else { // out of register
					int stk_reg = getStackReg(); // getStackReg() should never failed 

					pp->storage_type = STORAGE_STKREG;
					pp->size_of = sizeof_ctype(pp, F);
					pp->offset = g_func_ctx.stack_reg[stk_reg].offset;
					pp->lvalue = 1;

					if(o_offset > 0)
						oprintf("\tMOV\t%s,[EBP+%d]\n", g_reg_str[EDX], o_offset);
					else if(o_offset < 0)
						oprintf("\tMOV\t%s,[EBP-%d]\n", g_reg_str[EDX], -o_offset);
					else
						oprintf("\tMOV\t%s,[EBP]\n", g_reg_str[EDX]);

					if(pp->offset > 0)
						oprintf("\tMOV\t[EBP+%d],%s\n", pp->offset, g_reg_str[EDX]);
					else if(pp->offset < 0)
						oprintf("\tMOV\t[EBP-%d],%s\n", -pp->offset, g_reg_str[EDX]);
					else
						oprintf("\tMOV\t[EBP],%s\n", g_reg_str[EDX]);

				} 				
			} else if(storage_t == STORAGE_REG){
				if(is_ctype_struct(pp) || is_ctype_union(pp)) {
					// do nothing
				}
				else {
					oprintf("\tMOV\t%s,[%s]\n", g_reg_str[pp->reg >> REG_SHF], g_reg_str[pp->reg >> REG_SHF]);
				}
			} else if(storage_t == STORAGE_REGOFF){
				printf("&&&&&&&&&&&\n"); 
			}
#endif

			return pp;
		} else {
			// error
			printf("error: subscripted value '%s' is neither array nor pointer\n", pp->name);
			exit(0);
		}

		return NULL;
	} else if(ct->op_t == '+') { 
		if(ct->lchild && ct->rchild == NULL) {
			return ct->lchild;
		} else {
			if(ct->lchild && ct->rchild) {
				return gen_binop(ct);
			} else 
				error("error: oops");
		}
	} else if(ct->op_t == '-' || ct->op_t == '~') {
		if(ct->op_t == '-' && ct->lchild && ct->rchild) {
			return gen_binop(ct);
		}
		
		const char *op_s; 
		if(ct->op_t == '-')
			op_s = "NEG";
		else
			op_s = "NOT";
		if(ct->lchild && ct->rchild == NULL) {
			CType * tmp = genop(ct->lchild);
			CType * l = NULL;
			generate_tmp_ctype(&l, tmp);
			if(is_ctype_normal(l)) {
				if(l->storage_type == STORAGE_LOCAL || l->storage_type == STORAGE_PARAM) {
					int reg_idx = getreg(REG_FLAG_NORMAL, 4);
					if(reg_idx < 0) {
						error("Out of regs.....\n");
					}
					gen_mem2reg(reg_idx >> REG_SHF, l, EBP, l->offset);

					#if 1
					oprintf("\t%s\t%s\n", op_s, g_reg_str[reg_idx >> REG_SHF]);
					#else
					if(l->size_of == 4)
						oprintf("\t%s\t%s\n", op_s, g_reg_str[reg_idx >> REG_SHF]); 
					else if(l->size_of == 2)
						oprintf("\t%s\t%s\n", op_s, g_sub16_reg_str[reg_idx >> REG_SHF]);
					else if(l->size_of == 1)
						oprintf("\t%s\t%s\n", op_s, g_sub8l_reg_str[reg_idx >> REG_SHF]);
					else 
						error("not possible\n"); 
					#endif
						
					l->reg = reg_idx;
					l->storage_type = STORAGE_REG;
					l->offset = 0;
					l->lvalue = 0;
				} else if(l->storage_type == STORAGE_GLOBAL || l->storage_type == STORAGE_STATIC) {
					error("not OK yet...\n");
				} else if(l->storage_type == STORAGE_REG) {
					if(l->lvalue)
						gen_mem2reg(l->reg >> REG_SHF, l, l->reg >> REG_SHF, 0);

					#if 1
					oprintf("\t%s\t%s\n", op_s, g_reg_str[l->reg >> REG_SHF]);
					#else
					if(l->size_of == 4)
						oprintf("\t%s\t%s\n", op_s, g_reg_str[l->reg >> REG_SHF]); 
					else if(l->size_of == 2)
						oprintf("\t%s\t%s\n", op_s,g_sub16_reg_str[l->reg >> REG_SHF]);
					else if(l->size_of == 1)
						oprintf("\t%s\t%s\n", op_s,g_sub8l_reg_str[l->reg >> REG_SHF]);
					else 
						error("not possible\n");
					#endif

					l->storage_type = STORAGE_REG;
					l->offset = 0;
				} else {
					printf("bad storage type %d\n", l->storage_type);
					exit(0);
				}
				return l;
			} else {
				printf("%d: error: wrong type argument to unary minus\n", line);
				exit(0);
			}
		} else if(ct->lchild && ct->rchild && ct->op_t == '-'){
			
		} else {
			error("This should not happen\n");
		}
	} 
	else if(ct->op_t == '>'
			|| ct->op_t == '<'
			|| ct->op_t == LE_OP
			|| ct->op_t == GE_OP
			|| ct->op_t == EQ_OP
			|| ct->op_t == NE_OP) {

		char *cmpstr;
		switch(ct->op_t){
			case '>':
				cmpstr = "SETG";
				break;
			case '<':
				cmpstr = "SETB";
				break;
			case LE_OP:
				cmpstr = "SETLE";
				break;
			case GE_OP:
				cmpstr = "SETGE";
				break;
			case EQ_OP:
				cmpstr = "SETE";
				break;
			case NE_OP:
				cmpstr = "SETNE";
				break;
		}
		CType * tmp = genop(ct->lchild);
		CType * tmp2 = genop(ct->rchild);
		implicit_convert(&ct->lchild, &ct->rchild);
		CType * l = NULL;
		generate_tmp_ctype(&l, tmp);
		CType * l2 = NULL;
		generate_tmp_ctype(&l2, tmp2);
		int reg_idx;
		char *reg_str;
		
		if(is_ctype_normal(l2)) {
			if(l2->storage_type == STORAGE_CONST){
				
			} else if(l2->storage_type == STORAGE_LOCAL || l2->storage_type == STORAGE_PARAM) {
				reg_idx = getreg(REG_FLAG_NORMAL, 4);
				if(reg_idx < 0) {
					error("Out of regs.....\n");
				}
				gen_mem2reg(reg_idx >> REG_SHF, l2, EBP, l2->offset);
			} else if(l2->storage_type == STORAGE_GLOBAL || l2->storage_type == STORAGE_STATIC) {
				error("not OK yet...\n");
			} else if(l2->storage_type == STORAGE_REG) {
				if(l2->lvalue)
					gen_mem2reg(l2->reg >> REG_SHF, l2, l2->reg >> REG_SHF, 0);
				reg_idx = l2->reg;			
			} else {
				printf("bad storage type %d\n", l2->storage_type);
				exit(0);
			}
		} else {
			printf("%d: error: wrong type argument to op %d\n", line, ct->op_t);
			exit(0);
		}
		if(l2->size_of == 4) {
			reg_str = g_reg_str[reg_idx >> REG_SHF];
		} else if(l2->size_of == 2) {
			reg_str = g_sub16_reg_str[reg_idx >> REG_SHF];
		} else if(l2->size_of == 1) {
			reg_str = g_sub8l_reg_str[reg_idx >> REG_SHF];
		} else {
			error("size not OK, somthing wrong.");
		}

		if(is_ctype_normal(l)) {
			if(l->storage_type == STORAGE_CONST){
			}
			else if(l->storage_type == STORAGE_LOCAL 
					|| l->storage_type == STORAGE_PARAM) {
				char * ops;
				if(l->size_of == 4)
					ops = "CMP\tDWORD";
				else if(l->size_of == 2)
					ops = "CMP\tWORD";
				else if(l->size_of == 1)
					ops = "CMP\tBYTE";
				else
					error("This should not happen.");
					
				if(l->offset > 0)
					oprintf("\t%s [EBP+%d],%s\n", ops, l->offset, reg_str); 
				else if(l->offset < 0)
					oprintf("\t%s [EBP-%d],%s\n", ops, -l->offset, reg_str);
				else 
					oprintf("\t%s [EBP],%s\n", ops, reg_str);
				freereg(reg_idx);

				oprintf("\t%s\tAL\n", cmpstr);
				oprintf("\tMOVZX\tEAX,AL\n");

				l->link.next = l->link.prev = NULL;
				l->get_from = NULL;
				l->reg = (EAX << REG_SHF)|0xf;
				l->size_of = 1;
				l->storage_type = STORAGE_REG;
				l->offset = 0;
				l->lvalue = 0;
			} else if(l->storage_type == STORAGE_GLOBAL || l->storage_type == STORAGE_STATIC) {
				error("not OK yet...\n");
			} else if(l->storage_type == STORAGE_REG) {
				char * ops;
				if(l->size_of == 4)
					ops = "CMP\tDWORD";
				else if(l->size_of == 2)
					ops = "CMP\tWORD";
				else if(l->size_of == 1)
					ops = "CMP\tBYTE";
				else
					error("This should not happen.");

				if(l->lvalue)
					oprintf("\t%s [%s], %s\n", ops, g_reg_str[l->reg >> REG_SHF], reg_str);
				else 
					oprintf("\t%s %s, %s\n", ops, g_reg_str[l->reg >> REG_SHF], reg_str);
				freereg(reg_idx);
				freereg(l->reg);
					
				oprintf("\t%s\tAL\n", cmpstr);
				oprintf("\tMOVZX\tEAX,AL\n");
					
				l->link.next = l->link.prev = NULL;
				l->get_from = NULL;
				l->reg = (EAX << REG_SHF)|0xf;
				l->size_of = 1;
				l->storage_type = STORAGE_REG;
				l->offset = 0;
				l->lvalue = 0;
			} else {
				printf("bad storage type %d\n", l->storage_type);
				exit(0);
			}
			return l;
		} else {
			printf("%d: error: wrong type argument to op %d\n", line, ct->op_t);
			exit(0);
		}		
	} else if(ct->op_t == '!') { 
		CType * tmp = genop(ct->lchild);
		CType * l = NULL;
		generate_tmp_ctype(&l, tmp);

		if(is_ctype_normal(l)) {
			if(l->storage_type == STORAGE_CONST){
			}
			else if(l->storage_type == STORAGE_LOCAL 
					|| l->storage_type == STORAGE_PARAM) {
				char * ops;
				if(l->size_of == 4)
					ops = "CMP\tDWORD";
				else if(l->size_of == 2)
					ops = "CMP\tWORD";
				else if(l->size_of == 1)
					ops = "CMP\tBYTE";
				else
					error("This should not happen.");
					
				if(l->offset > 0)
					oprintf("\t%s [EBP+%d],0\n", ops, l->offset); 
				else if(l->offset < 0)
					oprintf("\t%s [EBP-%d],0\n", ops, -l->offset);
				else 
					oprintf("\t%s [EBP],0\n", ops);

				oprintf("\tSETE\tAL\n");
				oprintf("\tMOVZX\tEAX,AL\n");

				l->link.next = l->link.prev = NULL;
				l->get_from = NULL;
				l->reg = (EAX << REG_SHF)|0xf;
				l->size_of = 1;
				l->storage_type = STORAGE_REG;
				l->offset = 0;
				l->lvalue = 0;
			} else if(l->storage_type == STORAGE_GLOBAL || l->storage_type == STORAGE_STATIC) {
				error("not OK yet...\n");
			} else if(l->storage_type == STORAGE_REG) {
				char * ops;
				if(l->size_of == 4)
					ops = "CMP\tDWORD";
				else if(l->size_of == 2)
					ops = "CMP\tWORD";
				else if(l->size_of == 1)
					ops = "CMP\tBYTE";
				else
					error("This should not happen.");

				if(l->lvalue)
					oprintf("\t%s [%s],0\n", ops, g_reg_str[l->reg >> REG_SHF]);
				else 
					oprintf("\t%s %s,0\n", ops, g_reg_str[l->reg >> REG_SHF]);
				freereg(l->reg);
					
				oprintf("\tSETE\tAL\n");
				oprintf("\tMOVZX\tEAX,AL\n");
					
				l->link.next = l->link.prev = NULL;
				l->get_from = NULL;
				l->reg = (EAX << REG_SHF)|0xf;
				l->size_of = 1;
				l->storage_type = STORAGE_REG;
				l->offset = 0;
				l->lvalue = 0;
			} else {
				printf("bad storage type %d\n", l->storage_type);
				exit(0);
			}
			return l;
		} else {
			printf("%d: error: wrong type argument to unary not\n", line);
			exit(0);
		}		
	} else if(ct->op_t == OPBRACKET) { // []
		r = genop(ct->rchild);
		l = genop(ct->lchild);
		CType * pctype = NULL;
		generate_tmp_ctype(&pctype, l);
		freereg(l->reg);
		l = pctype;
		
		if(!(is_ctype_array(l) || is_ctype_pointer(l))) {
			// error
			printf("error: subscripted value is neither array nor pointer\n");
			exit(0);
		}
		if(is_ctype_normal(r) && 
			(r->types.normal.d_attr.type_spec |(INT_MASK | CHAR_MASK))) {

			// generate operation [] (bracket) 
			gen_opb(l, r);
			freereg(r->reg);
			return l;
		} else {
			// error
			printf("error: array subscript is not an integer\n");
			exit(0);
		}		

	} else if(ct->op_t == PRE_INC_OP || ct->op_t == PRE_DEC_OP) { // ++i
		l = genop(ct->lchild);
		CType * pctype = NULL;
		generate_tmp_ctype(&pctype, l);
		l = pctype;

		if(l->lvalue == 0) {
			printf("%d: error: invalid lvalue in increment\n", line);
			exit(0);
		}
			
		if(is_ctype_pointer(l)) {
			CType * pp = NULL;
			generate_tmp_ctype(&pp, l);

			struct list_head *curr, *li;
			li = pp->link.next->next;
			curr =  pp->link.next;
			if(pp->link.prev == curr) { 	
				pp->link.prev = &(pp->link);
			}
			pp->link.next = li;
			pp->size_of = sizeof_ctype(pp, F);  /* get the size of the element the pointer point to */

			if(pp->size_of <= 0) {
				printf("size error %d.....\n", pp->size_of);
				exit(0);
			}

			if(l->storage_type == STORAGE_LOCAL || l->storage_type == STORAGE_PARAM) {
				int reg_idx = getreg(REG_FLAG_NORMAL, 4);
				if(reg_idx < 0) {
					error("Out of regs.....\n");
				}
				if(l->offset > 0)
					oprintf("\tLEA\t%s,[EBP+%d]\n", g_reg_str[reg_idx >> REG_SHF], l->offset); 
				else if(l->offset < 0)
					oprintf("\tLEA\t%s,[EBP-%d]\n", g_reg_str[reg_idx >> REG_SHF], -l->offset);
				else
					oprintf("\tLEA\t%s,[EBP]\n", g_reg_str[reg_idx >> REG_SHF]);

				if(pp->size_of == 1) {
					if(ct->op_t == PRE_INC_OP)
						oprintf("\tINC\tDWORD [%s]\n", g_reg_str[reg_idx >> REG_SHF]);
					else 
						oprintf("\tDEC\tDWORD [%s]\n", g_reg_str[reg_idx >> REG_SHF]);
				} else if(pp->size_of > 1) {
					if(ct->op_t == PRE_INC_OP)
						oprintf("\tADD\tDWORD [%s],%d\n", g_reg_str[reg_idx >> REG_SHF], pp->size_of);
					else
						oprintf("\tSUB\tDWORD [%s],%d\n", g_reg_str[reg_idx >> REG_SHF], pp->size_of);
				}
				
				freereg(reg_idx);

			} else if(l->storage_type == STORAGE_GLOBAL || l->storage_type == STORAGE_STATIC) {
				error("not OK yet...\n");
			} else if(l->storage_type == STORAGE_REG) {
				if(pp->size_of == 1) {
					if(ct->op_t == PRE_INC_OP)
						oprintf("\tINC\tDWORD [%s]\n", g_reg_str[l->reg >> REG_SHF]);
					else 
						oprintf("\tDEC\tDWORD [%s]\n", g_reg_str[l->reg >> REG_SHF]);
				} else if(pp->size_of > 1) {
					if(ct->op_t == PRE_INC_OP)
						oprintf("\tADD\tDWORD [%s],%d\n", g_reg_str[l->reg >> REG_SHF], pp->size_of);
					else
						oprintf("\tSUB\tDWORD [%s],%d\n", g_reg_str[l->reg >> REG_SHF], pp->size_of);
				}				
			} else {
				printf("This should not happen.....st.%d\n", l->storage_type);
				exit(0);
			}
			return l;

		} else if(is_ctype_normal(l)) {
			char *ins = NULL;

			if(l->size_of == 1) {
				if(ct->op_t == PRE_INC_OP)
					ins = "INC\tBYTE";
				else
					ins = "DEC\tBYTE";
			} else if(l->size_of == 2) {
				if(ct->op_t == PRE_INC_OP)
					ins = "INC\tWORD";
				else
					ins = "DEC\tWORD";
			} else if(l->size_of == 4) {
				if(ct->op_t == PRE_INC_OP)
					ins = "INC\tDWORD";
				else
					ins = "DEC\tDWORD";
			} else {
				printf("size error %d.....\n", l->size_of);
				exit(0);
			}

			if(l->storage_type == STORAGE_LOCAL || l->storage_type == STORAGE_PARAM) {
				int reg_idx = getreg(REG_FLAG_NORMAL, 4);
				if(reg_idx < 0) {
					error("Out of regs.....\n");
				}

				if(l->offset > 0)
					oprintf("\tLEA\t%s,[EBP+%d]\n", g_reg_str[reg_idx >> REG_SHF], l->offset); 
				else if(l->offset < 0)
					oprintf("\tLEA\t%s,[EBP-%d]\n", g_reg_str[reg_idx >> REG_SHF], -l->offset);
				else
					oprintf("\tLEA\t%s,[EBP]\n", g_reg_str[reg_idx >> REG_SHF]);
				oprintf("\t%s [%s]\n", ins, g_reg_str[reg_idx >> REG_SHF]);
				freereg(reg_idx);

			} else if(l->storage_type == STORAGE_GLOBAL || l->storage_type == STORAGE_STATIC) {
				error("not OK yet...\n");
			} else if(l->storage_type == STORAGE_REG) {
				oprintf("\t%s [%s]\n", ins, g_reg_str[l->reg >> REG_SHF]);
			} else {
				printf("This should not happen.....st.%d\n", l->storage_type);
				exit(0);
			}
			return l;
		}else {
			// error
			printf("error: wrong type argument to increment\n");
			exit(0);
		}
	} else if(ct->op_t == SIZEOF_OP) { // sizeof
		CType *t1 = genop(ct->lchild);
		CType *tmp= create_ctype();
		tmp->symtype = SYM_TYPE_NO_NAME_VAL;
		tmp->datatype = DATATYPE_NORMAL;
		// tmp->address = 
		// store this val to .data

		tmp->v = create_c_val();
		
		tmp->types.normal.d_attr.has_type_spec = T;
		tmp->types.normal.d_attr.type_spec = TS_LONG;
		tmp->v->content.ull = sizeof_ctype(t1, F);
		build_ctype(tmp);
		tmp->storage_type = STORAGE_CONST;
		tmp->lvalue = 0;
		return tmp;
	} else if(ct->op_t == CAST_OP) { 
		CType *tmp = genop(ct->lchild);
		return gen_cast(tmp);
	} else if(ct->op_t == '/' || ct->op_t == '%') { 
		return gen_binop2(ct);
	} else if(ct->op_t == CALL_OP) {
		call_rop *crop = (call_rop *)ct->rchild;
		CType *pctype = ct->lchild;
		CType **ap = crop->ap;
		int i;
		int n = crop->cnts;
		
		bool has_ellipsis = F;
		CType *t;
		struct list_head * lk = pctype->link.next;
		t = list_entry(lk, CType, link);
		int size = 0;        /* current offset based on ESP when push parameters */
		bool is_return_big_sz = F;

#ifdef TEST_OUTPUT 
		char *stk_instr_addr = &(OUTBUF[OUTBUFIDX]);
		int stk_instr_len = 32;
		memset(&(OUTBUF[OUTBUFIDX]),' ',32);
		OUTBUF[OUTBUFIDX+31] = '\n';
		OUTBUFIDX += 32;
		
#endif

		do { // check return type size 
			CType * rct = NULL;
			generate_tmp_ctype(&rct, pctype);

			struct list_head *lh;
			lh = rct->link.next->next;
			rct->link.next = lh;			 // function return value ctype
			rct->size_of = sizeof_ctype(rct, F);

			CType *c_tmp;
			struct list_head *cur = pctype->link.next;
			c_tmp = list_entry(cur, CType, link);

			if(rct->size_of > 4) {  // the callee return big size
				is_return_big_sz = T;
				size += 4;
				int offset = 0;

				if(rct->size_of <= g_func_ctx.hidden_local_value_sz) {
					/* there already exists a hidden value, size is enough */
					offset = g_func_ctx.hidden_local_value_offset;
				} else {
					/*
					  alloc a bigger hidden value in caller's stack 
					*/ 
					int sz = g_func_ctx.cur_statck_size;
					if(sz & (ALIGNMENT-1)) {
						if(rct->size_of <= (ALIGNMENT - ( sz & (ALIGNMENT-1)))) {
							rct->offset = sz;
							sz += rct->size_of;
						} else {
							sz += ALIGNMENT - ( sz & (ALIGNMENT-1));
							rct->offset = sz;
							sz += rct->size_of;
						}
					} else {
						/* size currently is aligned */
						rct->offset = sz;
						sz += rct->size_of;
					} 
					g_func_ctx.cur_statck_size = sz;
					rct->offset = -g_func_ctx.cur_statck_size;
					offset = g_func_ctx.hidden_local_value_offset 
						   = -g_func_ctx.cur_statck_size;
					g_func_ctx.hidden_local_value_sz = rct->size_of;
				}
				
				/* Then move address of hidden value in front of the first parameter */
#ifdef  TEST_OUTPUT
				int reg = getreg(REG_FLAG_NORMAL, 4);
				if(reg == -1) {
					printf("error: *** TODO: out of regs\n");
					exit(0);
				}

				if(g_func_ctx.hidden_local_value_offset > 0)
					oprintf("\tLEA\t%s,[EBP+%d]\n", g_reg_str[reg >> REG_SHF], 
					                 g_func_ctx.hidden_local_value_offset);
				else if(g_func_ctx.hidden_local_value_offset < 0)
					oprintf("\tLEA\t%s,[EBP-%d]\n", g_reg_str[reg >> REG_SHF], 
					                 -g_func_ctx.hidden_local_value_offset);
				else
					oprintf("\tLEA\t%s,[EBP]\n", g_reg_str[reg >> REG_SHF]);

				
				oprintf("\tMOV\t[ESP],%s\n", g_reg_str[reg >> REG_SHF]); 
				freereg(reg);
#endif
			}
			free(rct);
		}while(0);
		
		// check argu type
		for(i = 0; i < t->types.function.nr_param && i < n; i++) {
			CType *ct = t->types.function.param_list[i];
			if(ap[i]->datatype == DATATYPE_OP) {
				ap[i] = genop(ap[i]);
			}
			
			if(ct->datatype == DATATYPE_ELLIPSIS) {
				has_ellipsis = T;
				assert(i+1 == t->types.function.nr_param);
				break;
			}

			if(is_ctype_compatible(ct, ap[i]) == F) {
				// error
				printf("error: incompatible type for argument %d\n", i+1);
				out2[0] = 0;
				printf("param\n");
				print_ctype(ct,out2);
				printf(out2);
				out2[0] = 0;
				printf("in type\n");
				print_ctype(ap[i],out2);
				printf(out2);

				printf("---\n");
				
				OUTBUF[OUTBUFIDX] = '\0';
				printf(OUTBUF);  // print instr buf now

				exit(0);
			}
			
			/* gen code for parameter push */
			do {
				CType * tmp = create_ctype();
				CType_cpy(tmp, ap[i], F);
				tmp->storage_type = STORAGE_REGOFF;
				tmp->reg = MAKE_REG32(ESP);
				tmp->lvalue = 1;
				// calculate offset
				if(size & (ALIGNMENT-1)) {
					if(tmp->size_of <= (ALIGNMENT - ( size & (ALIGNMENT-1)))) {
						tmp->offset = size;
						size += tmp->size_of;
					} else {
						size += ALIGNMENT - ( size & (ALIGNMENT-1));
						tmp->offset = size;
						size += tmp->size_of;
					}
				} else {
					/* size currently is aligned */
					tmp->offset = size;
					size += tmp->size_of;
				} 

				gen_assign(tmp, ap[i]); 
				freereg(ap[i]->reg);
			}while(0);
		}

		if(has_ellipsis || (t->types.function.nr_param > 1 &&
			(t->types.function.param_list[t->types.function.nr_param-1])->datatype 
				== DATATYPE_ELLIPSIS)) {
			if(n+1 < t->types.function.nr_param) {
				printf("error: too few arguments to function\n");
				exit(0);
			}

			while(i < n) {

				if(ap[i]->datatype == DATATYPE_OP) {
					ap[i] = genop(ap[i]);
				}
			
				/* gen code for parameter push */
				do {
					CType * tmp = create_ctype();
					CType_cpy(tmp, ap[i], F);
					tmp->storage_type = STORAGE_REGOFF;
					tmp->reg = MAKE_REG32(ESP);
					tmp->lvalue = 1;
					// calculate offset
					if(size & (ALIGNMENT-1)) {
						if(tmp->size_of <= (ALIGNMENT - ( size & (ALIGNMENT-1)))) {
							tmp->offset = size;
							size += tmp->size_of;
						} else {
							size += ALIGNMENT - ( size & (ALIGNMENT-1));
							tmp->offset = size;
							size += tmp->size_of;
						}
					} else {
						/* size currently is aligned */
						tmp->offset = size;
						size += tmp->size_of;
					} 

					gen_assign(tmp, ap[i]); 
					freereg(ap[i]->reg);
				}while(0);
				i++;
			}
		}
		else {
			if(n > t->types.function.nr_param) {
				printf("error: too many arguments to function\n");
				exit(0);
			} else if (n < t->types.function.nr_param) {
				printf("error: too few arguments to function\n");
				exit(0);
			}
		}

		if(size & (ALIGNMENT-1))
			size += ALIGNMENT - (size & (ALIGNMENT-1));

#ifdef TEST_OUTPUT
		if(size) {
			int ret;
			ret = sprintf(stk_instr_addr, 
				"\tSUB\tESP,0x%x\n",size); 
			stk_instr_addr[ret] = ' '; 
		}

		oprintf("\tCALL\t%s\n", pctype->name);
#endif

#ifdef TEST_OUTPUT
		if(size) {
			oprintf("\tADD\tESP,0x%x\n", size); 
		}
#endif

		CType * rct = NULL;
		generate_tmp_ctype(&rct, pctype);
		rct->symtype = SYM_TYPE_NONAME;

		struct list_head *lh;
		lh = rct->link.next->next;
		rct->link.next = lh;             // function return value ctype
		rct->reg = getreg(REG_FLAG_NORMAL, 4);
		if((rct->reg >> REG_SHF) != EAX) {
			/* function return value is in EAX */
			// TODO: return structure ...
#ifdef TEST_OUTPUT
			oprintf("\tMOV\t%s,%s\n", g_reg_str[rct->reg >> REG_SHF], g_reg_str[EAX]);
#endif
		}
		if(is_return_big_sz) {
			rct->storage_type = STORAGE_REGOFF; 
			rct->lvalue = 1;
		}
		else {
			rct->storage_type = STORAGE_REG;
			rct->lvalue = 0; 
		}
		rct->size_of = sizeof_ctype(rct, F);
		rct->offset = 0;		

		return rct;
	}else if(ct->op_t == '^'
		|| ct->op_t == '|'
		|| ct->op_t == LEFT_OP
		|| ct->op_t == RIGHT_OP) { 
		return gen_binop(ct);
	} else if(ct->op_t == MUL_ASSIGN
			|| ct->op_t == DIV_ASSIGN 
			|| ct->op_t == MOD_ASSIGN 
			|| ct->op_t == ADD_ASSIGN 
			|| ct->op_t == SUB_ASSIGN 
			|| ct->op_t == LEFT_ASSIGN 
			|| ct->op_t == RIGHT_ASSIGN 
			|| ct->op_t == AND_ASSIGN 
			|| ct->op_t == XOR_ASSIGN 
			|| ct->op_t == OR_ASSIGN) {
			
		CType* op = create_op_ctype();
		op->op_t = '=';

		switch(ct->op_t){
			case MUL_ASSIGN:
				ct->op_t = '*';
				break;
			case DIV_ASSIGN:
				ct->op_t = '/';
				break;
			case MOD_ASSIGN:
				ct->op_t = '%';
				break;
			case ADD_ASSIGN:
				ct->op_t = '+';
				break;
			case SUB_ASSIGN:
				ct->op_t = '-';
				break;
			case LEFT_ASSIGN:
				ct->op_t = LEFT_OP;
				break;
			case RIGHT_ASSIGN:
				ct->op_t = RIGHT_OP;
				break;
			case AND_ASSIGN:
				ct->op_t = AND_OP;
				break;
			case XOR_ASSIGN:
				ct->op_t = '^';
				break;
			case OR_ASSIGN:
				ct->op_t = OR_OP;
				break;
		}
		op->lchild = ct->lchild;
		op->rchild = ct;
		return genop(op);
	} else if(ct->op_t == AND_OP || ct->op_t == OR_OP){  // &&  ||
		int label1 = g_label++;
		int label2 = g_label++;
		int label3 = g_label++;

		if(ct->op_t == AND_OP) {
			CType *tmp = genop(ct->lchild);
			gen_logical(tmp, 0, label2);
			tmp = genop(ct->rchild);
			gen_logical(tmp, 0, label2); 
		} else {
			CType *tmp = genop(ct->lchild);
			gen_logical(tmp, label2, 0);
			tmp = genop(ct->rchild);
			gen_logical(tmp, label2, 0);
		}

		int reg_idx = getreg(REG_FLAG_NORMAL, 4);
		if(reg_idx < 0) {
			error("Out of regs.....\n");
		}
		if(ct->op_t == AND_OP) {
			oprintf("L%s:\n", itoa(label1, 0, 10));
			oprintf("\tMOV\t%s,1\n", g_reg_str[reg_idx >> REG_SHF]);
			oprintf("\tJMP\tL%s\n",itoa(label3, 0, 10));
			oprintf("L%s:\n", itoa(label2, 0, 10));
			oprintf("\tMOV\t%s,0\n", g_reg_str[reg_idx >> REG_SHF]);
			oprintf("L%s:\n", itoa(label3, 0, 10)); 
		} else {
			oprintf("L%s:\n", itoa(label1, 0, 10));
			oprintf("\tMOV\t%s,0\n", g_reg_str[reg_idx >> REG_SHF]);
			oprintf("\tJMP\tL%s\n",itoa(label3, 0, 10));
			oprintf("L%s:\n", itoa(label2, 0, 10));
			oprintf("\tMOV\t%s,1\n", g_reg_str[reg_idx >> REG_SHF]);
			oprintf("L%s:\n", itoa(label3, 0, 10));
		}

		CType *l = create_ctype();
		l->types.normal.d_attr.has_type_spec = T;
		l->types.normal.d_attr.type_spec = INT_MASK;
		l->reg = reg_idx;
		l->size_of = 4;
		l->storage_type = STORAGE_REG;
		l->offset = 0;
		l->lvalue = 0;
		return l;
	} else if(ct->op_t == COND_OP) {  // ?:
		int label1 = g_label++;
		int label2 = g_label++;

		CType *tmp = genop(ct->lchild);
		gen_logical(tmp, 0, label1);
		CType *tmp2 = NULL;
		generate_tmp_ctype(&tmp2, genop(ct->rchild));		
		int st_t1 = gen_ctype2reg(EDX, tmp2);

		oprintf("\tJMP\tL%s\n",itoa(label2, 0, 10));
		oprintf("L%s:\n", itoa(label1, 0, 10));
		CType *tmp3 = NULL;
		generate_tmp_ctype(&tmp3, genop(ct->child3));		
		int st_t2 = gen_ctype2reg(EDX, tmp3);
		oprintf("L%s:\n", itoa(label2, 0, 10));

		if(!is_ctype_compatible(tmp2, tmp3)) {
			printf("%d:error: type not compatbale in condition expr\n", line);
			exit(0);
		}

		assert(st_t1 == st_t2);
		implicit_convert(&tmp2, &tmp3);
		tmp2->reg = ((EDX << REG_SHF)|0xffff);
		tmp2->storage_type = st_t2;
		tmp2->lvalue = 0;
		
		return tmp2;		
	}

	return NULL;
}

void gen_jmp_tbl(CType *ct, statement_ctx *p)
{
	CType * l = NULL;
	generate_tmp_ctype(&l, ct);
	int reg_idx;
	jmp_table_t *jt = p->jt;

#define JT_BUF_SZ  4096
	static char jt_buf[JT_BUF_SZ] = {0};
	
	if(l->storage_type == STORAGE_LOCAL 
			|| l->storage_type == STORAGE_PARAM) {	
		reg_idx = getreg(REG_FLAG_NORMAL, 4);
		if(reg_idx < 0) {
			error("Out of regs.....\n");
		}
		gen_mem2reg(reg_idx >> REG_SHF, l, EBP, l->offset);

	} else if(l->storage_type == STORAGE_GLOBAL || l->storage_type == STORAGE_STATIC) {
		error("not OK yet...\n");
	} else if(l->storage_type == STORAGE_REG) {
		if(l->lvalue) {
			gen_mem2reg(l->reg >> REG_SHF, l, l->reg >> REG_SHF, 0);
		}
		reg_idx = l->reg;
	} else {
		printf("bad storage type %d\n", l->storage_type);
		exit(0);
	}

	int i=0;
	for(; i<jt->cnt; i++) {
		if(jt->label[i]) {
			oprintf("L%s:\n", itoa(g_label++, 0, 10));
			oprintf("\tCMP\t%s, %d\n", g_reg_str[reg_idx >> REG_SHF], jt->constant[i]);
			oprintf("\tJNE\tL%s\n", itoa(g_label, 0, 10));
			oprintf("\tJMP\tL%s\n", itoa(jt->label[i], 0, 10));
		}
		else
			break;
	}

	oprintf("L%s:\n", itoa(g_label++, 0, 10));
	if(jt->default_label){
		oprintf("\tJMP\tL%s\n", itoa(jt->default_label, 0, 10));
	}
	oprintf("\tJMP\tL%s\n", itoa(p->out_label, 0, 10));
	freereg(reg_idx);

	/* then insert 'code' of jmp table before 'p->start_switch_code' */
	jt_buf[0] = 0;
	int len;
	if((len = strlen(p->end_switch_code)) > (JT_BUF_SZ-1)) {
		error("JMP table buf size too small\n");
	}

	strcpy(jt_buf, p->end_switch_code);
#if 0
	/* debug*/
	printf("********\n");
	printf("%s\n", jt_buf);
	printf("********\n");
#endif

	/* move code from 'p->start_switch_code' to 'p->start_switch_code+len'*/
	memmove((p->start_switch_code+len), p->start_switch_code, 
		(p->end_switch_code-p->start_switch_code+1));
	memcpy(p->start_switch_code, jt_buf, len);
}

/*
assignment_expr	
	: conditional_expr	
	| unary_expr assignment_operator assignment_expr
	;

*/
int assignment_expr(CType **p)
{
	int type;
	CType *pp;

	type = gettoken();
	if(type == '(') {
		CType  *new;
		if((new = type_name()) != NULL) {
			//tmp->xxx = size_of(ctype);
			if(gettoken() != ')') {
				printf("error: parse error before '%s'\n", yytext);
				exit(0);
			}
			if(cast_expr(&pp)) {
				// make sure *pp can be cast to *new
				if(is_cast_available(pp, new)) {
					free_ctype(pp);
				
				} else {
					// error
					printf("error: can't cast from.. to ..\n");
					exit(0);
				}
			} else {
				// error
				printf("error: parse error from '%s'\n", yytext);
				exit(0);
			}
			*p = new;
			goto cond_exp;
		} else {		/* still unary_expr() */
			restoretoken2("(");
			goto _unary;
		}
	} else {

		restoretoken();
_unary:		
		if(unary_expr(&pp)) {

			*p = pp;
		} 
		else {
			return 0;    // not assignment_expr
		}
	}

	/* here is unary_expr */
	type = gettoken();
	if(assignment_operator(type) == T) {
		CType *tmp;
		if(assignment_expr(&tmp)) {
			/* is compatible of 'tmp' and 'pp', ??????? */

			/* 'pp' '=' 'tmp' */
#ifdef DEBUG				
			out_buffer[0] = '\0';
			print_ctype(pp, out_buffer);
			printf(out_buffer);
			printf("\n\r");
			
			out_buffer[0] = '\0';
			print_ctype(tmp, out_buffer);
			printf(out_buffer);
			printf("\n\r");
#endif
			// generate expression
			CType* op = create_op_ctype();
			op->op_t = type;

			op->lchild = pp;
			op->rchild = tmp;

			*p = op; 
			return 1;
		} else {
			error("error: not an assignment expr");
		}
	}

	restoretoken();
	
cond_exp:

	first_cast_expr_is_parsed = T;
	cast_expr_ret = *p;
	
	if(conditional_expr(&pp)) {
		if(!StackEmpty(&addr_op_stk) && pp->datatype == DATATYPE_OP
			&& pp->op_t == '*')
			*p = pp;   /* what does this mean ????? */
		else 
			*p = pp;
	} else {
		//error
		printf("error: not conditional_expr\n");
		exit(0);		
	}
#ifdef   DEBUG				
	out_buffer[0] = '\0';
	print_ctype(*p, out_buffer);
	printf(out_buffer);
	printf("\n\r");
#endif

	return 1;
}


/*
expr	
	: assignment_expr	
	| expr ',' assignment_expr	
	;
*/

int expr(CType **p, bool later_gen)
{
	int n = 0;
	static int cnt = 0;
	
	while(assignment_expr(p)) {
		cnt++;
		n++;
		if(gettoken() == ',') {
			if(later_gen == F) {
				*p = genop(*p);
				if(*p) freereg((*p)->reg); 
			}
		} else {
			restoretoken();
			return 1;
		}
	}

	if(n == 0) {
		return 0;    // not assignment_expr
	}

	printf("error: parse error before %s", yytext);
	exit(0);
}



/*

labeled_statement	
	: 
	| identifier ':' statement	
	| CASE constant_expr ':' statement	
	| DEFAULT ':' statement	
	;

*/
int labeled_statement(statement_ctx *p)
{
	assert(p->sta_t == IN_SWITCH);

	/* we only care about 'CASE' and 'DEFAULT' here */
	int type = gettoken();
	if(type == CASE) {
		CType *pp;
		if(constant_expr(&pp)) {
			if(pp->symtype != SYM_TYPE_NO_NAME_VAL) {
				// error
				goto _error;
			}
			if(pp->datatype != DATATYPE_NORMAL && pp->storage_type != STORAGE_CONST) {
				printf("%d: case label does not reduce to an integer constant\n", line);
				exit(0);				
			}
			if(gettoken() != ':') {
				// error
				goto _error;
			}
			oprintf("L%s:\n", itoa(g_label, 0, 10));
			p->jt->label[p->jt->cnt] = g_label;
			p->jt->constant[p->jt->cnt] = pp->v->content.ull;
			p->jt->cnt++;
			g_label++;
			statement();
			return 1;
		} else {
			// error
			goto _error;
		}
	} else if(type == DEFAULT) {
		if(gettoken() != ':') {
			// error
			goto _error;
		}
		oprintf("L%s:\n", itoa(g_label, 0, 10));
		if(p->jt->default_label) {
			printf("%d: error: multiple default labels in one switch\n", line);
			exit(0);
		}
		p->jt->default_label = g_label;
		g_label++;
		statement();
		return 1;
	}

_error:
	printf("error: parse error before %s\n", yytext);
	exit(0);
}

/*

selection_statement	
	: IF '(' expr ')' statement	
	| IF '(' expr ')' statement ELSE statement	
	| SWITCH '(' expr ')' statement	
	;

*/
int selection_statement()
{
	int type = gettoken();
	CType *pp;
	int label1 = g_label++;
	int label2;

	if(type == IF) {
		if(gettoken() != '(') {
			// error
			goto _error;
		}
		if(expr(&pp, F) == 0) {
			// error
			goto _error;
		} else {
			gen_logical(pp, 0, label1);
		}
		if(gettoken() != ')') {
			// error
			goto _error;
		}
		if(statement() == 0) {
			// error
			goto _error;
		}
		if(gettoken() == ELSE) {
			label2 = g_label++;
			oprintf("\tJMP\tL%s\n",itoa(label2, 0, 10));
			oprintf("L%s:\n", itoa(label1, 0, 10)); 
			if(statement() == 0) {
				// error
				goto _error;
			}
			oprintf("L%s:\n", itoa(label2, 0, 10));
		} else {
			restoretoken();
		}
		return 1;
	} else if(type == SWITCH) {
		if(gettoken() != '(') {
			// error
			goto _error;
		}
		if(expr(&pp, F) == 0) {
			// error
			goto _error;
		}
		if(gettoken() != ')') {
			// error
			goto _error;
		}
		if(is_ctype_normal(pp)) {
			if((pp->types.normal.d_attr.type_spec & TS_FLOAT)
				|| (pp->types.normal.d_attr.type_spec & TS_DOUBLE)) {
				printf("%d: error: switch quantity not an integer..\n", line);
				exit(0);
			}
		} else {
			printf("%d: error: switch quantity not an integer.\n", line);
			exit(0);
		}
		
		statement_ctx *p;	 
		if(GetTop(&statement_ctx_stk, (SElemType*)&p) != R_SUCCESS) {
			printf("error: not in switch statement\n");
			exit(0);
		}
		else if(p->sta_t != IN_SWITCH) {
			printf("error: not in switch statement\n");
			exit(0);
		}

		p->start_switch_code = &(OUTBUF[OUTBUFIDX]);
		p->out_label = g_label++;
		if(statement() == 0) {
			// error
			goto _error;
		}

		oprintf("L%s:\n", itoa(p->out_label, 0, 10));		
		p->end_switch_code = &(OUTBUF[OUTBUFIDX]);

		gen_jmp_tbl(pp, p);

		return 1;
	}

_error:
	printf("error: parse error before %s\n", yytext);
	exit(0);
}

/*

iteration_statement	
	: WHILE '(' expr ')' statement	
	| DO statement WHILE '(' expr ')' ';'	
	| FOR '(' ';' ';' ')' statement	
	| FOR '(' ';' ';' expr ')' statement	
	| FOR '(' ';' expr ';' ')' statement	
	| FOR '(' ';' expr ';' expr ')' statement	
	| FOR '(' expr ';' ';' ')' statement	
	| FOR '(' expr ';' ';' expr ')' statement	
	| FOR '(' expr ';' expr ';' ')' statement	
	| FOR '(' expr ';' expr ';' expr ')' statement	
	;

*/
int iteration_statement()
{
	int type = gettoken();
	CType *pp;
	CType *later_gen = NULL;
	int label1 = g_label++;
	int label2 = g_label++;
	int for_cont_label = 0; 

	statement_ctx *p;
	if(find_sta_ctx(IN_LOOP, &p) != R_SUCCESS) {
		printf("error: not in loop statement, strange!\n");
		exit(0);
	}

	if(type == WHILE) {
		if(gettoken() != '(') {
			// error
			goto _error;
		}
		oprintf("L%s:\n", itoa(label1, 0, 10));
		p->continue_label = label1;
		p->out_label = label2;		
		if(expr(&pp, F) == 0) {
			// error
			goto _error;
		} 
		gen_logical(pp, 0, label2);
		if(gettoken() != ')') {
			// error
			goto _error;
		}
		if(statement() == 0) {
			// error
			goto _error;
		}	
		oprintf("\tJMP\tL%s\n",itoa(label1, 0, 10));
		oprintf("L%s:\n", itoa(label2, 0, 10));
		return 1;
	} else if(type == DO) {
		oprintf("L%s:\n", itoa(label1, 0, 10));
		for_cont_label = g_label++;
		p->continue_label = for_cont_label;
		p->out_label = label2;		
		if(statement() == 0) {
			// error
			goto _error;
		}
		if(gettoken()!= WHILE) {
			// error
			goto _error;
		}
		if(gettoken() != '(') {
			// error
			goto _error;
		}		
		oprintf("L%s:\n", itoa(for_cont_label, 0, 10)); 
		if(expr(&pp, F) == 0) {
			// error
			goto _error;
		}
		gen_logical(pp, label1, 0);	
		if(gettoken() != ')') {
			// error
			goto _error;
		}
		if(gettoken() != ';') {
			// error
			goto _error;
		}
		oprintf("L%s:\n", itoa(label2, 0, 10));		
		return 1;
	}else if(type == FOR){
		CType_list *ct_list;
		for_cont_label = g_label++;
		p->continue_label = for_cont_label;
		p->out_label = label2;
		
		if(gettoken() != '(') {
			// error
			goto _error;
		}
		if((ct_list = declaration(T, F)) != NULL) {
			printf("error: 'for' loop initial declaration used outside C99 mode\n");
			exit(0);
		}
		else if(gettoken() == ';') {
			oprintf("L%s:\n", itoa(label1, 0, 10));
			if(gettoken() == ';') {
				if(expr(&pp, T)) {
					/* for ( ; ; expr) */
					later_gen = pp;
				} 
				/* for ( ; ; ) */
				
				if(gettoken() != ')') {
					// error
					goto _error;
				}
			} else {
				restoretoken();
				if(expr(&pp, F) == 0) {
					// error
					goto _error;
				}
				pp = genop(pp);
				if(pp) freereg(pp->reg);
				if(gettoken() != ';') {
					// error
					goto _error;
				}
				gen_logical(pp, 0, label2);	
				if(expr(&pp, T)) {
					/* for ( ; expr; expr) */
					later_gen = pp;
				} 
				/* for ( ; expr; ) */				
				if(gettoken() != ')') {
					// error
					goto _error;
				}
			}
		} else {
			restoretoken();
			if(expr(&pp, F) == 0) {
				// error
				goto _error;
			}
			pp = genop(pp);
			if(pp) freereg(pp->reg);			
			if(gettoken() != ';') {
				// error
				goto _error;
			}
			oprintf("L%s:\n", itoa(label1, 0, 10));
			if(gettoken() == ';') {
				if(expr(&pp, T)) {
					/* for ( expr; ; expr) */
					later_gen = pp;
				} 
				/* for ( expr; ; ) */
				
				if(gettoken() != ')') {
					// error
					goto _error;
				}
			} else {
				restoretoken();
				if(expr(&pp, F) == 0) {
					// error
					goto _error;
				}
				pp = genop(pp);
				if(pp) freereg(pp->reg);
				if(gettoken() != ';') {
					// error
					goto _error;
				}
				
				gen_logical(pp, 0, label2);	
				if(expr(&pp, T)) {
					/* for ( expr; expr; expr) */
					later_gen = pp;
				} 
				/* for ( expr; expr; ) */
				if(gettoken() != ')') {
					// error
					goto _error;
				}
				
			}
		}
		
		if(statement() == 0) {
			// error
			goto _error;
		}

		if(later_gen) {
			oprintf("L%s:\n", itoa(for_cont_label, 0, 10));
			later_gen = genop(later_gen);
			if(later_gen) freereg((later_gen)->reg); 
		}
				
		oprintf("\tJMP\tL%s\n",itoa(label1, 0, 10));
		oprintf("L%s:\n", itoa(label2, 0, 10));
		return 1;
	}else {
		// should not happen
		goto _error;
	}

_error:
	printf("%d: error: parse error before %s\n", line, yytext);
	exit(0);

}

label_t *create_label(char *name)
{
    label_t * p;
	p = malloc(sizeof(label_t));
    if (p == NULL) {
       printf("error: Out of memory\n");
	   exit(-1);
    }
	memset(p, 0, sizeof(label_t));
	p->name = store_name(name, strlen(name));
	return p;
}

void free_label(label_t *p)
{
	assert(p->name != NULL);
	free(p->name);
	free(p);
}

statement_ctx *create_sta_ctx(int type)
{
    statement_ctx * p;
	p = malloc(sizeof(statement_ctx));
    if (p == NULL) {
       printf("error: Out of memory\n");
	   exit(-1);
    }
	memset(p, 0, sizeof(statement_ctx));
	INIT_LIST_HEAD(&(p->label_l));
	p->sta_t = type;
	if(type == IN_SWITCH) {
		jmp_table_t *jt;
		jt = malloc(sizeof(jmp_table_t));
		if (jt == NULL) {
		   printf("error: Out of memory\n");
		   exit(-1);
		}
		memset(jt, 0, sizeof(jmp_table_t));
		p->jt = jt;
	}
	return p;
}

void free_sta_ctx(statement_ctx *p)
{
	struct list_head *curr, *head;
	label_t *tmp;

	head = &(p->label_l);
	 
	for (curr = (head)->next; curr != (head); ) {
		tmp = list_entry(curr, label_t, link);
		curr = curr->next;
		free_label(tmp);
	}

	if(p->sta_t == IN_SWITCH) {
		free(p->jt);
	}	
	free(p);
}

int find_sta_ctx(int sta_t, statement_ctx **p)
{
    statement_ctx *s_ctx;
    SElemType *index;

    if(StackEmpty(&statement_ctx_stk)) {
	   printf("Stack empty\n");
       return R_UNSUCCESS;
    }
	
	index = statement_ctx_stk.top - 1;
    while(1) {
       s_ctx = (statement_ctx *)(*index);
	   //printf("-------- search table 0x%x\n", symtab);
	   if(s_ctx != NULL && s_ctx->sta_t == sta_t) {
	   	  *p = s_ctx;
          return R_SUCCESS;
	   }
	   if(index == statement_ctx_stk.base) break;
	   index--;
    }

	return R_UNSUCCESS;
}


/*

jump_statement	
	: GOTO identifier ';'	
	| CONTINUE ';'	
	| BREAK ';'	
	| RETURN ';'	
	| RETURN expr ';'	
	;

*/

int jump_statement()
{
	int type = gettoken();
	CType *pp;

	if(type == GOTO) {
		if(gettoken() != IDENTIFIER) {
			goto _error;
		}
		statement_ctx *p;	 
		if(GetBottom(&statement_ctx_stk, (SElemType*)&p) != R_SUCCESS) {
			printf("error: not possiable, 'goto' not in function\n");
			exit(0);
		} else if(p->sta_t != IN_FUNCTION) {
			printf("error: not possiable, 'goto' not in function\n");
			exit(0);
		}

		label_t * l = create_label(yytext);
		list_add(&(l->link), &(p->label_l));

		oprintf("\tJMP\t%s\n", yytext);
		return 1;
	} else if(type == CONTINUE) {
		if(gettoken() != ';') {
			goto _error;
		}
		return 1;
	} else if(type == BREAK) {
		if(gettoken() != ';') {
			goto _error;
		}
		return 1;
	} else if(type == RETURN) {
		if(gettoken() == ';') {
			
		} else {
			restoretoken();
			if(expr(&pp, F) == 0) {
				goto _error;
			}

			// TODO: gen_return
			
			freereg(pp->reg);
			if(gettoken() != ';') {
				goto _error;
			}
		}
		return 1;
	}

_error:
	printf("error: parse error before %s\n", yytext);
	exit(0);

}

void do_label(statement_ctx *ctx)
{ 
	struct list_head *curr;
	label_t *tmp;
	symvalue_t * value;
	
	list_for_each(curr, &(ctx->label_l)){ 	
		tmp = list_entry(curr, label_t, link);

		if(ctx->symt == NULL) {
			printf("%d: error: a label '%s' not found in function\n", line, tmp->name);
			exit(0);
		}
		if(symtab_lookup(ctx->symt, tmp->name, SYM_TYPE_LABEL, &value) != R_SUCCESS) {
			printf("%d: error: a label '%s' not found in function\n", line, tmp->name);
			exit(0);
		}		
	}
}


/*

expression_statement	
	: ';'	
	| expr ';'	
	;

statement	
	: 
	| labeled_statement	
	| compound_statement	
	| expression_statement	
	| selection_statement	
	| iteration_statement	
	| jump_statement	
	;

*/

int statement()
{
	int type;
	CType * pctype;
	statement_ctx *s_ctx;

	type = gettoken();
	if(type == IDENTIFIER) {
		char *id = store_name(yytext, yyleng);
		if(gettoken() == ':') {
			/* labled statement */
			statement_ctx *p;	 
			if(GetBottom(&statement_ctx_stk, (SElemType*)&p) != R_SUCCESS) {
				printf("error: label '%s' not in function\n", id);
				exit(0);
			} else if(p->sta_t != IN_FUNCTION) {
				printf("error: label '%s' not in function\n", id);
				exit(0);
			}
			if(p->symt == NULL) {
				printf("need create symtable\n");
				exit(0);
			}
			symvalue_t * value;
			if(symtab_lookup(p->symt, id, SYM_TYPE_LABEL, &value) == R_SUCCESS) {
				printf("error: a label '%s' found before\n", id);
				exit(0);
			} else {
				/* create a new label */
				CType *ct = create_ctype();
				ct->symtype = SYM_TYPE_LABEL;
				ct->name = store_name(id, strlen(id));
				symtab_define(p->symt,  ct, symexists_add, NULL, F);
				oprintf("%s:\n", ct->name);
			}
			
			if(statement() == 0) {
				printf("%d: error: no statement after label\n", line);
				exit(0);
			}
			free(id);
			return 1;
		} else {
			restoretoken();   // restore ':'
			restoretoken2(id);
			symvalue_t * value;
			if(symtabs_lookup(id, SYM_TYPE_TYPEDEF, &value) == R_SUCCESS
				&& symtabs_lookup(id, SYM_TYPE_OBJECT, &value) != R_SUCCESS) {			
				free(id);
				return 0;  // not statement
			}
			free(id);
			if(expr(&pctype, F)) {
				pctype = genop(pctype);
				if(pctype) freereg(pctype->reg);
				if(gettoken() != ';') {
					// error
					printf("_LINE_ %d: %d: parse error before '%s'\n", __LINE__, line, yytext);
					exit(0);
				}
				return 1;
			}
			return 0;  // not statement
		}
	} else if(type == CASE || type == DEFAULT) {
		statement_ctx *p;
		/*
		if(find_sta_ctx(IN_SWITCH, &p) != R_SUCCESS) {
			printf("error: '%s' not in switch statement\n", yytext);
			exit(0);
		}*/
		if(GetTop(&statement_ctx_stk, (SElemType*)&p) != R_SUCCESS) {
			printf("error: not in switch statement\n");
			exit(0);
		}
		else if(p->sta_t != IN_SWITCH) {
			printf("error: not in switch statement\n");
			exit(0);
		}
		
		restoretoken();
		labeled_statement(p);
		return 1;
	} else if(type == '{') {
		restoretoken();
		symtab_t * new_symt = NULL;
		compound_statement(new_symt);
		if(new_symt != NULL)
			symtab_destroy(&new_symt, free_ctype);
		return 1;
	} else if(type == ';') {
		// empty statement
		return 1;
	} else if(type == IF || type == SWITCH) {
		if(type == SWITCH) { 
			s_ctx = create_sta_ctx(IN_SWITCH);
			if(Push(&statement_ctx_stk, (SElemType)s_ctx) != R_SUCCESS) {	
				printf("Push to stack failed\n");
				exit(-1);    
			}
		}
		restoretoken();
		selection_statement();
		if(type == SWITCH) { 
			if(Pop(&statement_ctx_stk, (SElemType *)&s_ctx) != R_SUCCESS) {	
				printf("Push to stack failed\n");
				exit(-1);    
			}
			assert(s_ctx->sta_t == IN_SWITCH);
			free_sta_ctx(s_ctx);
		}
		return 1;
	} else if(type == WHILE || type == DO || type == FOR) {
		s_ctx = create_sta_ctx(IN_LOOP);
		if(Push(&statement_ctx_stk, (SElemType)s_ctx) != R_SUCCESS) {	
			printf("Push to stack failed\n");
			exit(-1);    
		}
		restoretoken();
		iteration_statement();
		if(Pop(&statement_ctx_stk, (SElemType *)&s_ctx) != R_SUCCESS) {	
			printf("Push to stack failed\n");
			exit(-1);    
		}
		assert(s_ctx->sta_t == IN_LOOP);
		free_sta_ctx(s_ctx);
		return 1;
	} else if(type == GOTO || type == CONTINUE || type == BREAK || type == RETURN) {
		if(type == BREAK) {
			statement_ctx *p;	 
			if(GetTop(&statement_ctx_stk, (SElemType*)&p) != R_SUCCESS) {
				printf("error: '%s' not in switch statement or loop\n", yytext);
				exit(0);
			}
			else if(p->sta_t != IN_SWITCH && p->sta_t != IN_LOOP) {
				printf("error: '%s' not in switch statement or loop\n", yytext);
				exit(0);
			}
			oprintf("\tJMP\tL%s\n", itoa(p->out_label, 0, 10));
		}
		else if(type == CONTINUE) {
			statement_ctx *p;
			if(find_sta_ctx(IN_LOOP, &p) != R_SUCCESS) {
				printf("error: '%s' not in loop statement\n", yytext);
				exit(0);
			}
			oprintf("\tJMP\tL%s\n", itoa(p->continue_label, 0, 10));
		}
		restoretoken();
		jump_statement();
		return 1;
	} else {
		restoretoken();
		if(expr(&pctype, F)) {
			pctype = genop(pctype);
			if(pctype) freereg(pctype->reg);

			if(gettoken() != ';') {
					// error
			}
			return 1;
		} else {
			return 0;  // not statement
		}
	}

	return 0;  // not statement
}

/*

compound_statement	
	: 
	| '{' '}'	
	| '{' statement_list '}'	
	| '{' declaration_list '}'	
	| '{' declaration_list statement_list '}'	
	;

statement_list	
	: statement	
	| statement_list statement	
	;

declaration_list	
	: declaration	
	| declaration_list declaration	
	;
	
*/

int compound_statement(symtab_t * in_symt)
{
	CType_list * ct_list;
	symtab_t * symt = NULL;
	int ret = 0;

	if(Push(&g_sym_stack, (SElemType)in_symt) != R_SUCCESS) {	// in_symt maybe NULL
		printf("Push to stack failed\n");
		exit(-1);    
	}
	
	if(gettoken() == '{') {
		while(1) {
			if((ct_list = declaration(T, F)) != NULL) {
				symvalue_t * value;
				struct list_head *curr;
				if(ct_list == (CType_list *)(!NULL)) continue;   /* FIXME  (!NULL) */

				free_ctype_list(ct_list);				
			} else if(statement()) {
					
			} else if(gettoken() == '}') {
				ret = 1;
				goto OUT;
			} else {
				printf("error: parse error before %s\n", yytext);
				exit(0);
			}
		}
	}

	restoretoken();

OUT:	
	 if(Pop(&g_sym_stack, (SElemType*)&symt) != R_SUCCESS) {
		printf("Pop from stack failed\n");
		exit(-1);	 
	 }
  
	return ret;   // 0 if not  compound_statement
}

#endif

int main(int argc, char **argv)
{
	int res;
	CType *pctype;
	symtab_t *symtab = NULL;
	CType_list * ct_list, *tmp;
	int type;

	g_pre_inc_code.start = NULL;
	g_pre_inc_code.end = NULL;
		
	if(InitStack(&g_sym_stack) != R_SUCCESS) {
		error("init stack failed\n");  
	}

	if(InitStack(&addr_op_stk) != R_SUCCESS) {
		error("init stack failed\n");  
	}
	
	++argv;
	--argc; /* skip over program name */

  
	if ( argc > 0 ) {
		if(strcmp(argv[0], "-h") == 0) {
			print_help();
			return;
		}
		yyin = fopen(argv[0], "r");
		if(yyin == 0) {
			error("Can not open file\n");
			return -1;
		}

	}
	else {
		yyin = fopen("t1.c", "r");
		if(yyin == 0) {
			error("Can not open file 'dcl.c'\n");
			return -1;
		}
	}

	//printf("Input C declaration here (if input error, using Backspace):\n\n\r");
  
	if(symtab_create(100, NULL, NULL, 1, &symtab) != R_SUCCESS) {
		printf("symtab_create failed\n");
		exit(-1);
	}
  
	if(Push(&g_sym_stack, (SElemType)symtab) != R_SUCCESS) {
		printf("Push to stack failed\n");
		exit(-1);    
	}

        g_cur_stage = STAGE_GLOBAL;
	while(1) {
		ct_list = declaration(T, F);
check_decl:
		if(ct_list !=  NULL) {
			symvalue_t * value;
			struct list_head *curr;
			if(ct_list == (CType_list *)(!NULL)) continue;   /* FIXME  (!NULL) */
			free_ctype_list(ct_list);

		}
		else {
			
			if((type = gettoken()) == IDENTIFIER || type == '*') {
				restoretoken();
				ct_list = declaration(F, F);
				goto  check_decl;
			}
			else if(*yytext  != '\0' )
				printf("%d: error: parse error before '%s'\n", line, yytext);
			break;
		}
	}

	
	if(Pop(&g_sym_stack, (SElemType*)&symtab) != R_SUCCESS) {
		printf("Pop from stack failed\n");
		exit(-1);    
	}
	assert(StackEmpty(&g_sym_stack) == T);
	assert(StackEmpty(&addr_op_stk) == T);
	
	FreeStack(&g_sym_stack);
	FreeStack(&addr_op_stk);
	symtab_destroy(&symtab, free_ctype);

	//printf("---File total lines %d\n", line);
	return 0;
}


