#ifndef __CTYPE_H__
#define __CTYPE_H__

#include "list.h"

#define MAXTOKEN  100

typedef int bool;
#define T 1
#define F 0

#define R_SUCCESS     0
#define R_UNSUCCESS  -1
#define R_NOMEMORY   -2
#define R_NOTFOUND   -3
#define R_EXISTS     -4
#define R_OVERFLOW   -5

#define MAXPOINTERS 100

#define DECLARATOR_DIRECT    1
#define DECLARATOR_ABSTRACT  2
#define DECLARATOR_BOTH      (DECLARATOR_DIRECT|DECLARATOR_ABSTRACT)

typedef struct {
    int q_const;
	int q_volatile;
}qualifier;

struct _CType;

#define SIGNED_MASK 0x1
#define UNSIGNED_MASK 0x2
#define CHAR_MASK 0x4
#define SHORT_MASK 0x8
#define INT_MASK 0x10
#define LONG_MASK 0x20
#define FLOAT_MASK 0x80
#define DOUBLE_MASK 0x100
#define LONG_LONG_MASK 0x60
#define STRUCT_MASK 0x200
#define UNION_MASK 0x400
#define ENUM_MASK 0x800
#define SUE_MASK  0xE00    // TS_STRUCT | TS_UNION | TS_ENUM
#define VOID_MASK 0x1000

enum type_spec {	// basic type 
/*
           void|enum|union|struct|double|float|long|long|int|short|char|unsigned|signed|
bit         12     11      10       9         8        7       6      5      4     3       2         1            0 
*/
	TS_SIGNED = 1,
	TS_USIGNED = 2,
	TS_CHAR = 4,
	TS_SCHAR,
	TS_UCHAR,
	TS_SHORT = 8,
	TS_SSHORT,
	TS_USHORT,
	TS_INT = 16,
	TS_SINT,
	TS_UINT,	
	TS_SHORT_INT = 24,
	TS_SSHORT_INT,
	TS_USHORT_INT,
	TS_LONG = 32,
	TS_SLONG,
	TS_ULONG,
	TS_LONG_INT = 48,
	TS_SLONG_INT,
	TS_ULONG_INT,
	TS_LONG_LONG = 96,
	TS_SLONG_LONG,
	TS_ULONG_LONG,
	TS_LONG_LONG_INT = 112,
	TS_SLONG_LONG_INT,
	TS_ULONG_LONG_INT,
	TS_FLOAT = 128,
	TS_DOUBLE = 256,
	TS_LONG_DOUBLE = 288,
	TS_STRUCT = 512,
	TS_UNION = 1024,
	TS_ENUM = 2048,
	TS_VOID = 4096
};

typedef struct {
	bool has_stor_spec;
	bool has_type_spec;
	bool is_typedef_name;     /* type specifier is a typedef_name  */
	bool has_enum_val;
	int  stor_spec;
	int  type_spec;
	char *tag;                  /* If type_spec == STRUCT or UNION or ENUM, tag is the tag name */
	struct _CType *st_un;
	int  enum_val;
	qualifier qua;
}dcl_specifiers_attr;

#define DATATYPE_NORMAL   0x1001   /* datatype is int, float, double, struct, union, enum etc. */
#define DATATYPE_ARRAY    0x1002
#define DATATYPE_FUNCTION 0x1003
#define DATATYPE_POINTER  0x1004
#define DATATYPE_ELLIPSIS 0x1005
#define DATATYPE_STRUCT  0x1006    /* struct or union or enum itself */
#define DATATYPE_UNION    0x1007
#define DATATYPE_ENUM      0x1008
#define DATATYPE_OP          0x1111   /* for syntax tree generation */

#define SYM_TYPE_OBJECT      0x1
#define SYM_TYPE_TYPEDEF    0x2
#define SYM_TYPE_NONAME    0x4
#define SYM_TYPE_TAG            0x8	/* strucut a {int i};  'a' is the TAG, with  DATATYPE_STRUCT/UNION/ENUM */
#define SYM_TYPE_NO_NAME_VAL   0x10 /* if is this type,  Ctype is read only value, char, integer, string.. */
#define SYM_TYPE_NO_NAME_TMP_VAL 0x20
#define SYM_TYPE_LABEL     0x40

#define STR_CHAR  1
#define STR_LONG  2	

#define STORAGE_UNKNOW   0
#define STORAGE_GLOBAL     1
#define STORAGE_LOCAL        2
#define STORAGE_STATIC      4
#define STORAGE_PARAM       8
// below are used for code generation
#define STORAGE_REG           16
#define STORAGE_CONST       32
#define STORAGE_STKREG     64    

#define STORAGE_REGOFF     128      // like 4(%eax)

typedef struct {
	struct list_head link;
	char string_t;		/* just for free(),  if string literal is  STR_CHAR or STR_LONG */
	union {
		char c;              /* not used , only for debug */
		unsigned long long ull;
		long double ld;
		char *s;
		long *ls;
		
	} content;
} value_t;

struct symtab;

typedef struct _CType{
	struct list_head link;
	char *name;
	int datatype;
	int symtype;
	struct _CType *get_from;   /* typedef from or get from other type, or tmp_val get from other ctype */
	struct _CType *parent;     /* This is point to its parent */
	struct _CType *child;         /* this is only used for array initializer */
	int idx;                                /* used for initializer */

	int op_t;                 /* for syntax tree generation */
	struct  _CType *lchild, *rchild;      /* for syntax tree generation */
	struct  _CType *child3;   /* only used in '?:' */
		
	union types{
	   struct {
		qualifier  qua;	  /* only two kind of type qualifier: const and volatile */
		bool  is_from_addrop;	  /* ctype pointer is created from '&' */
	   }pointer;
	   
	   struct {
		  struct _CType **param_list;
		  int nr_param;
		  struct symtab* table;
		  bool is_old_style;       /* old stype param list, i.e. identifier list */
		  bool has_same_id;     /* old style param list may have same id name */
		  bool is_definition;
	   }function;
	   
	   struct { 
	   	  bool is_pointer_alike;	  /* pointer alike array,  a[] */
		  bool is_p_alike_init;
	   	  int array_size;  /* item counts, e.g. long a[10], array_size is 10, not 40(bytes) */
	   }array;

	   struct  {
	      dcl_specifiers_attr d_attr;
	      int    bitfield_size;
	   }normal;
	   
	   struct  {
	   	  struct _CType **item_list;
		  int items;
		  struct symtab* table;
		  struct _CType  *attach_to;   /* if it has no tag name, it attach to ctype for free memory */
		  struct _CType  *associate_with;   /* is used for value assign when initializer */
		  bool is_empty;    /* like struct s {}; */
	   }struct_union_enum;
	   
	} types;

	bool is_abstract;
	int lvalue;	                /* this value can be assigned */
	int line;
	value_t *v;	       

        /********* for code generation *********/
		
	/* size of this Ctype */
	unsigned int  size_of;         
	/* offset in the stack or in data section ? 
	     offset based on base reg. like EBP in intel Arch.
	     offset in struct or union is > 0
	     offset in stack:
	     1. if stack is growing down (like intel.)  

                                0xffffffff
	      |   |        |
	      |   |        |   <-- ebp +   parameters
	      v   |        |   <-- ebp   
	           |        |   <-- ebp -   local vars
	           |        |
	           |        |   0x00000000
	         
	         local variable (in stack) offset is < 0, function parameter variable offset is > 0
	     2. if stack is growing up
	         local variable (in stack) offset is > 0, function parameter variable offset is < 0
	  */
	int  offset;           
	/* global, static or local, or parameter, 
	     or only in a reg, or constant 
	     reg and constant type is only used for code generation */
	int  storage_type;        
	int  reg;
	
	/********* end of for code generation *********/
	
} CType;


#endif



