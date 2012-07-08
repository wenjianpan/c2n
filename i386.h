#ifndef   __INCLUDE_I386__
#define   __INCLUDE_I386__

struct InstrPre{
	char bytes[4];
	int  cnt;       /* 0-4 */
} InstrPre;

struct Opcode{
	char bytes[3];  /* 1,2,3 */
	int  cnt;
} Opcode;

struct ModRM{
	char Mod:2;
	char Reg_Opc:3;
	char RM:3;
} ModRM;

struct SIB{
	char scale:2;
	char index:3;
	char base:3;
}SIB;

struct Displacement{
	char bytes[4];
	int  cnt;     /* 1,2,4 */
} Displacement;

struct Immediate{
	char bytes[4];
	int  cnt;     /* 1,2,4 */
} Immediate;


typedef struct IA32Instr {
	struct InstrPre instrpre;
	struct Opcode opcode;
	struct ModRM modrm;
	struct SIB sib;
	struct Displacement displacement;
	struct Immediate immediate;
}IA32Instr;

/*
  IA32 register usage
  
8-bit      16-bit
AH AL      AX
BH BL      BX
CH CL      CX
DH DL      DX
               BP
               SI
               DI
               SP
for regs like EAX, 0xffff means EAX is used, 0xf means AL, 0xf0 means AH, 0xff means AX 
  */
#define REG_FLAG_NORMAL   0          // used for getreg(flag)

#define REG_SHF   16

#define FUNC_STACK_FRAME   8        // intel near call stack frame size

enum IA32_Reg_Type {
	EAX = 0,
	EBX,
	ECX,
	EDX,
	ESI,
	EDI,
	EBP,
	ESP,
	EIP,
	CS,
	DS,
	SS,
	ES,
	FS,
	GS,
	EFLAGS
};

extern char* g_reg_str[];
extern char* g_sub16_reg_str[];
extern char* g_sub8h_reg_str[];
extern char* g_sub8l_reg_str[];

typedef struct IA32Regs {
	unsigned long EAX;
	unsigned long EBX;
	unsigned long ECX;
	unsigned long EDX;
	unsigned long ESI;
	unsigned long EDI;
	unsigned long EBP;
	unsigned long ESP;
	unsigned long EFLAGS;
	unsigned long EIP;
	unsigned short CS;
	unsigned short DS;
	unsigned short SS;
	unsigned short ES;
	unsigned short FS;
	unsigned short GS;
}IA32Regs;


extern IA32Regs  g_regs;


/*
  reg_size 1,2,4 bytes
  return reg index: index = (index << 16)|(0xf or 0xf0 or 0xff or 0xffff)
  When caller get index, it use (index >> 16) as reg index,
  if reg_size is 1, low 8 bits are used for identify AH&AL 
*/
#define MAKE_REG32(i)   ((i << 16)|0xffff)
int getreg(int, int);
void freereg(int);

int getStackReg();
void freeStackReg(int);

enum oprand_type {
	IS_REG = 1,
	IS_MEM,
	IS_CONST
};

enum oprand_sz {
	NOT_NEEDED = 0,
	BYTE = 1,
	WORD = 2,
	DWORD = 4
};

#endif

