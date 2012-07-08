/* insns.h   header file for insns.c
 *
 * The Netwide Assembler is copyright (C) 1996 Simon Tatham and
 * Julian Hall. All rights reserved. The software is
 * redistributable under the licence given in the file "Licence"
 * distributed in the NASM archive.
 */

#ifndef NASM_INSNS_H
#define NASM_INSNS_H

#include <stdio.h>

/* long is 32 bits */
//typedef signed long		int32_t;
//typedef unsigned long		uint32_t;

typedef uint32_t opflags_t;

/* Size, and other attributes, of the operand */
#define BITS8     	0x00000001L
#define BITS16    	0x00000002L
#define BITS32    	0x00000004L
#define BITS64    	0x00000008L   /* x64 and FPU only */
#define BITS80    	0x00000010L   /* FPU only */
#define BITS128		0x20000000L
#define FAR       	0x00000020L   /* grotty: this means 16:16 or */
                                       /* 16:32, like in CALL/JMP */
#define NEAR      	0x00000040L
#define SHORT     	0x00000080L   /* and this means what it says :) */

#define TO        	0x00000100L   /* reverse effect in FADD, FSUB &c */
#define COLON     	0x00000200L   /* operand is followed by a colon */

#define IMMEDIATE	0x00002000L
#define MEMORY		0x0000c000L

/* Register classes */
#define REG_EA   	0x00009000L   /* 'normal' reg, qualifies as EA */
#define RM_GPR		0x00208000L   /* integer operand */
#define REG_GPR		0x00209000L   /* integer register */
#define REG8      	0x00209001L   /*  8-bit GPR  */
#define REG16     	0x00209002L   /* 16-bit GPR */
#define REG32     	0x00209004L   /* 32-bit GPR */
#define REG64     	0x00209008L   /* 64-bit GPR */
#define IP_REG    	0x00801000L   /* RIP or EIP register */
#define RIPREG    	0x00801008L   /* RIP */
#define EIPREG    	0x00801004L   /* EIP */
#define FPUREG    	0x01001000L   /* floating point stack registers */
#define FPU0      	0x01011000L   /* FPU stack register zero */
#define RM_MMX		0x02008000L   /* MMX operand */
#define MMXREG    	0x02009000L   /* MMX register */
#define RM_XMM		0x04008000L   /* XMM (SSE) operand */
#define XMMREG    	0x04009000L   /* XMM (SSE) register */
#define XMM0		0x04019000L   /* XMM register zero */
#define REG_CDT   	0x00101004L   /* CRn, DRn and TRn */
#define REG_CREG	0x00111004L   /* CRn */
#define REG_DREG	0x00121004L   /* DRn */
#define REG_TREG	0x00141004L   /* TRn */
#define REG_SREG	0x00401002L   /* any segment register */
#define REG_CS		0x00411002L   /* CS */
#define REG_DESS	0x00421002L   /* DS, ES, SS */
#define REG_FSGS	0x00441002L   /* FS, GS */
#define REG_SEG67	0x00481002L   /* Unimplemented segment registers */

#define REG_RIP		0x00801008L   /* RIP relative addressing */
#define REG_EIP		0x00801004L   /* EIP relative addressing */

/* Special GPRs */
#define REG_SMASK 	0x000f0000L   /* a mask for the following */
#define REG_ACCUM	0x00219000L   /* accumulator: AL, AX, EAX, RAX */
#define REG_AL		0x00219001L
#define REG_AX		0x00219002L
#define REG_EAX		0x00219004L
#define REG_RAX		0x00219008L
#define REG_COUNT	0x00229000L   /* counter: CL, CX, ECX, RCX */
#define REG_CL		0x00229001L
#define REG_CX		0x00229002L
#define REG_ECX		0x00229004L
#define REG_RCX		0x00229008L
#define REG_DL		0x00249001L   /* data: DL, DX, EDX, RDX */
#define REG_DX		0x00249002L
#define REG_EDX		0x00249004L
#define REG_RDX		0x00249008L
#define REG_HIGH	0x00289001L   /* high regs: AH, CH, DH, BH */

/* special type of immediate operand */
#define UNITY		0x00012000L   /* for shift/rotate instructions */
#define SBYTE		0x00022000L   /* for op r16/32,immediate instrs. */

/* special types of EAs */
#define MEM_OFFS	0x0001c000L   /* simple [address] offset - absolute! */

/* special flags */
#define SAME_AS		0x40000000L


#define MAX_OPERANDS 4

enum opcode {
	I_AAA,
	I_AAD,
	I_AAM,
	I_AAS,
	I_ADC,
	I_ADD,
	I_ADDPD,
	I_ADDPS,
	I_ADDSD,
	I_ADDSS,
	I_ADDSUBPD,
	I_ADDSUBPS,
	I_AND,
	I_ANDNPD,
	I_ANDNPS,
	I_ANDPD,
	I_ANDPS,
	I_ARPL,
	I_BB0_RESET,
	I_BB1_RESET,
	I_BLENDPD,
	I_BLENDPS,
	I_BLENDVPD,
	I_BLENDVPS,
	I_BOUND,
	I_BSF,
	I_BSR,
	I_BSWAP,
	I_BT,
	I_BTC,
	I_BTR,
	I_BTS,
	I_CALL,
	I_CBW,
	I_CDQ,
	I_CDQE,
	I_CLC,
	I_CLD,
	I_CLFLUSH,
	I_CLGI,
	I_CLI,
	I_CLTS,
	I_CMC,
	I_CMP,
	I_CMPEQPD,
	I_CMPEQPS,
	I_CMPEQSD,
	I_CMPEQSS,
	I_CMPLEPD,
	I_CMPLEPS,
	I_CMPLESD,
	I_CMPLESS,
	I_CMPLTPD,
	I_CMPLTPS,
	I_CMPLTSD,
	I_CMPLTSS,
	I_CMPNEQPD,
	I_CMPNEQPS,
	I_CMPNEQSD,
	I_CMPNEQSS,
	I_CMPNLEPD,
	I_CMPNLEPS,
	I_CMPNLESD,
	I_CMPNLESS,
	I_CMPNLTPD,
	I_CMPNLTPS,
	I_CMPNLTSD,
	I_CMPNLTSS,
	I_CMPORDPD,
	I_CMPORDPS,
	I_CMPORDSD,
	I_CMPORDSS,
	I_CMPPD,
	I_CMPPS,
	I_CMPSB,
	I_CMPSD,
	I_CMPSQ,
	I_CMPSS,
	I_CMPSW,
	I_CMPUNORDPD,
	I_CMPUNORDPS,
	I_CMPUNORDSD,
	I_CMPUNORDSS,
	I_CMPXCHG,
	I_CMPXCHG16B,
	I_CMPXCHG486,
	I_CMPXCHG8B,
	I_COMISD,
	I_COMISS,
	I_COMPD,
	I_COMPS,
	I_COMSD,
	I_COMSS,
	I_CPUID,
	I_CPU_READ,
	I_CPU_WRITE,
	I_CQO,
	I_CRC32,
	I_CVTDQ2PD,
	I_CVTDQ2PS,
	I_CVTPD2DQ,
	I_CVTPD2PI,
	I_CVTPD2PS,
	I_CVTPH2PS,
	I_CVTPI2PD,
	I_CVTPI2PS,
	I_CVTPS2DQ,
	I_CVTPS2PD,
	I_CVTPS2PH,
	I_CVTPS2PI,
	I_CVTSD2SI,
	I_CVTSD2SS,
	I_CVTSI2SD,
	I_CVTSI2SS,
	I_CVTSS2SD,
	I_CVTSS2SI,
	I_CVTTPD2DQ,
	I_CVTTPD2PI,
	I_CVTTPS2DQ,
	I_CVTTPS2PI,
	I_CVTTSD2SI,
	I_CVTTSS2SI,
	I_CWD,
	I_CWDE,
	I_DAA,
	I_DAS,
	I_DB,
	I_DD,
	I_DEC,
	I_DIV,
	I_DIVPD,
	I_DIVPS,
	I_DIVSD,
	I_DIVSS,
	I_DMINT,
	I_DO,
	I_DPPD,
	I_DPPS,
	I_DQ,
	I_DT,
	I_DW,
	I_EMMS,
	I_ENTER,
	I_EQU,
	I_EXTRACTPS,
	I_EXTRQ,
	I_F2XM1,
	I_FABS,
	I_FADD,
	I_FADDP,
	I_FBLD,
	I_FBSTP,
	I_FCHS,
	I_FCLEX,
	I_FCMOVB,
	I_FCMOVBE,
	I_FCMOVE,
	I_FCMOVNB,
	I_FCMOVNBE,
	I_FCMOVNE,
	I_FCMOVNU,
	I_FCMOVU,
	I_FCOM,
	I_FCOMI,
	I_FCOMIP,
	I_FCOMP,
	I_FCOMPP,
	I_FCOS,
	I_FDECSTP,
	I_FDISI,
	I_FDIV,
	I_FDIVP,
	I_FDIVR,
	I_FDIVRP,
	I_FEMMS,
	I_FENI,
	I_FFREE,
	I_FFREEP,
	I_FIADD,
	I_FICOM,
	I_FICOMP,
	I_FIDIV,
	I_FIDIVR,
	I_FILD,
	I_FIMUL,
	I_FINCSTP,
	I_FINIT,
	I_FIST,
	I_FISTP,
	I_FISTTP,
	I_FISUB,
	I_FISUBR,
	I_FLD,
	I_FLD1,
	I_FLDCW,
	I_FLDENV,
	I_FLDL2E,
	I_FLDL2T,
	I_FLDLG2,
	I_FLDLN2,
	I_FLDPI,
	I_FLDZ,
	I_FMADDPD,
	I_FMADDPS,
	I_FMADDSD,
	I_FMADDSS,
	I_FMNADDPD,
	I_FMNADDPS,
	I_FMNADDSD,
	I_FMNADDSS,
	I_FMNSUBPD,
	I_FMNSUBPS,
	I_FMNSUBSD,
	I_FMNSUBSS,
	I_FMSUBPD,
	I_FMSUBPS,
	I_FMSUBSD,
	I_FMSUBSS,
	I_FMUL,
	I_FMULP,
	I_FNCLEX,
	I_FNDISI,
	I_FNENI,
	I_FNINIT,
	I_FNOP,
	I_FNSAVE,
	I_FNSTCW,
	I_FNSTENV,
	I_FNSTSW,
	I_FPATAN,
	I_FPREM,
	I_FPREM1,
	I_FPTAN,
	I_FRCZPD,
	I_FRCZPS,
	I_FRCZSD,
	I_FRCZSS,
	I_FRNDINT,
	I_FRSTOR,
	I_FSAVE,
	I_FSCALE,
	I_FSETPM,
	I_FSIN,
	I_FSINCOS,
	I_FSQRT,
	I_FST,
	I_FSTCW,
	I_FSTENV,
	I_FSTP,
	I_FSTSW,
	I_FSUB,
	I_FSUBP,
	I_FSUBR,
	I_FSUBRP,
	I_FTST,
	I_FUCOM,
	I_FUCOMI,
	I_FUCOMIP,
	I_FUCOMP,
	I_FUCOMPP,
	I_FWAIT,
	I_FXAM,
	I_FXCH,
	I_FXRSTOR,
	I_FXSAVE,
	I_FXTRACT,
	I_FYL2X,
	I_FYL2XP1,
	I_GETSEC,
	I_HADDPD,
	I_HADDPS,
	I_HINT_NOP0,
	I_HINT_NOP1,
	I_HINT_NOP10,
	I_HINT_NOP11,
	I_HINT_NOP12,
	I_HINT_NOP13,
	I_HINT_NOP14,
	I_HINT_NOP15,
	I_HINT_NOP16,
	I_HINT_NOP17,
	I_HINT_NOP18,
	I_HINT_NOP19,
	I_HINT_NOP2,
	I_HINT_NOP20,
	I_HINT_NOP21,
	I_HINT_NOP22,
	I_HINT_NOP23,
	I_HINT_NOP24,
	I_HINT_NOP25,
	I_HINT_NOP26,
	I_HINT_NOP27,
	I_HINT_NOP28,
	I_HINT_NOP29,
	I_HINT_NOP3,
	I_HINT_NOP30,
	I_HINT_NOP31,
	I_HINT_NOP32,
	I_HINT_NOP33,
	I_HINT_NOP34,
	I_HINT_NOP35,
	I_HINT_NOP36,
	I_HINT_NOP37,
	I_HINT_NOP38,
	I_HINT_NOP39,
	I_HINT_NOP4,
	I_HINT_NOP40,
	I_HINT_NOP41,
	I_HINT_NOP42,
	I_HINT_NOP43,
	I_HINT_NOP44,
	I_HINT_NOP45,
	I_HINT_NOP46,
	I_HINT_NOP47,
	I_HINT_NOP48,
	I_HINT_NOP49,
	I_HINT_NOP5,
	I_HINT_NOP50,
	I_HINT_NOP51,
	I_HINT_NOP52,
	I_HINT_NOP53,
	I_HINT_NOP54,
	I_HINT_NOP55,
	I_HINT_NOP56,
	I_HINT_NOP57,
	I_HINT_NOP58,
	I_HINT_NOP59,
	I_HINT_NOP6,
	I_HINT_NOP60,
	I_HINT_NOP61,
	I_HINT_NOP62,
	I_HINT_NOP63,
	I_HINT_NOP7,
	I_HINT_NOP8,
	I_HINT_NOP9,
	I_HLT,
	I_HSUBPD,
	I_HSUBPS,
	I_IBTS,
	I_ICEBP,
	I_IDIV,
	I_IMUL,
	I_IN,
	I_INC,
	I_INCBIN,
	I_INSB,
	I_INSD,
	I_INSERTPS,
	I_INSERTQ,
	I_INSW,
	I_INT,
	I_INT01,
	I_INT03,
	I_INT1,
	I_INT3,
	I_INTO,
	I_INVD,
	I_INVLPG,
	I_INVLPGA,
	I_IRET,
	I_IRETD,
	I_IRETQ,
	I_IRETW,
	I_JCXZ,
	I_JECXZ,
	I_JMP,
	I_JMPE,
	I_JRCXZ,
	I_LAHF,
	I_LAR,
	I_LDDQU,
	I_LDMXCSR,
	I_LDS,
	I_LEA,
	I_LEAVE,
	I_LES,
	I_LFENCE,
	I_LFS,
	I_LGDT,
	I_LGS,
	I_LIDT,
	I_LLDT,
	I_LMSW,
	I_LOADALL,
	I_LOADALL286,
	I_LODSB,
	I_LODSD,
	I_LODSQ,
	I_LODSW,
	I_LOOP,
	I_LOOPE,
	I_LOOPNE,
	I_LOOPNZ,
	I_LOOPZ,
	I_LSL,
	I_LSS,
	I_LTR,
	I_LZCNT,
	I_MASKMOVDQU,
	I_MASKMOVQ,
	I_MAXPD,
	I_MAXPS,
	I_MAXSD,
	I_MAXSS,
	I_MFENCE,
	I_MINPD,
	I_MINPS,
	I_MINSD,
	I_MINSS,
	I_MONITOR,
	I_MONTMUL,
	I_MOV,
	I_MOVAPD,
	I_MOVAPS,
	I_MOVD,
	I_MOVDDUP,
	I_MOVDQ2Q,
	I_MOVDQA,
	I_MOVDQU,
	I_MOVHLPS,
	I_MOVHPD,
	I_MOVHPS,
	I_MOVLHPS,
	I_MOVLPD,
	I_MOVLPS,
	I_MOVMSKPD,
	I_MOVMSKPS,
	I_MOVNTDQ,
	I_MOVNTDQA,
	I_MOVNTI,
	I_MOVNTPD,
	I_MOVNTPS,
	I_MOVNTQ,
	I_MOVNTSD,
	I_MOVNTSS,
	I_MOVQ,
	I_MOVQ2DQ,
	I_MOVSB,
	I_MOVSD,
	I_MOVSHDUP,
	I_MOVSLDUP,
	I_MOVSQ,
	I_MOVSS,
	I_MOVSW,
	I_MOVSX,
	I_MOVUPD,
	I_MOVUPS,
	I_MOVZX,
	I_MPSADBW,
	I_MUL,
	I_MULPD,
	I_MULPS,
	I_MULSD,
	I_MULSS,
	I_MWAIT,
	I_NEG,
	I_NOP,
	I_NOT,
	I_OR,
	I_ORPD,
	I_ORPS,
	I_OUT,
	I_OUTSB,
	I_OUTSD,
	I_OUTSW,
	I_PABSB,
	I_PABSD,
	I_PABSW,
	I_PACKSSDW,
	I_PACKSSWB,
	I_PACKUSDW,
	I_PACKUSWB,
	I_PADDB,
	I_PADDD,
	I_PADDQ,
	I_PADDSB,
	I_PADDSIW,
	I_PADDSW,
	I_PADDUSB,
	I_PADDUSW,
	I_PADDW,
	I_PALIGNR,
	I_PAND,
	I_PANDN,
	I_PAUSE,
	I_PAVEB,
	I_PAVGB,
	I_PAVGUSB,
	I_PAVGW,
	I_PBLENDVB,
	I_PBLENDW,
	I_PCMOV,
	I_PCMPEQB,
	I_PCMPEQD,
	I_PCMPEQQ,
	I_PCMPEQW,
	I_PCMPESTRI,
	I_PCMPESTRM,
	I_PCMPGTB,
	I_PCMPGTD,
	I_PCMPGTQ,
	I_PCMPGTW,
	I_PCMPISTRI,
	I_PCMPISTRM,
	I_PCOMB,
	I_PCOMD,
	I_PCOMQ,
	I_PCOMUB,
	I_PCOMUD,
	I_PCOMUQ,
	I_PCOMUW,
	I_PCOMW,
	I_PDISTIB,
	I_PERMPD,
	I_PERMPS,
	I_PEXTRB,
	I_PEXTRD,
	I_PEXTRQ,
	I_PEXTRW,
	I_PF2ID,
	I_PF2IW,
	I_PFACC,
	I_PFADD,
	I_PFCMPEQ,
	I_PFCMPGE,
	I_PFCMPGT,
	I_PFMAX,
	I_PFMIN,
	I_PFMUL,
	I_PFNACC,
	I_PFPNACC,
	I_PFRCP,
	I_PFRCPIT1,
	I_PFRCPIT2,
	I_PFRSQIT1,
	I_PFRSQRT,
	I_PFSUB,
	I_PFSUBR,
	I_PHADDBD,
	I_PHADDBQ,
	I_PHADDBW,
	I_PHADDD,
	I_PHADDDQ,
	I_PHADDSW,
	I_PHADDUBD,
	I_PHADDUBQ,
	I_PHADDUBW,
	I_PHADDUDQ,
	I_PHADDUWD,
	I_PHADDUWQ,
	I_PHADDW,
	I_PHADDWD,
	I_PHADDWQ,
	I_PHMINPOSUW,
	I_PHSUBBW,
	I_PHSUBD,
	I_PHSUBDQ,
	I_PHSUBSW,
	I_PHSUBW,
	I_PHSUBWD,
	I_PI2FD,
	I_PI2FW,
	I_PINSRB,
	I_PINSRD,
	I_PINSRQ,
	I_PINSRW,
	I_PMACHRIW,
	I_PMACSDD,
	I_PMACSDQH,
	I_PMACSDQL,
	I_PMACSSDD,
	I_PMACSSDQH,
	I_PMACSSDQL,
	I_PMACSSWD,
	I_PMACSSWW,
	I_PMACSWD,
	I_PMACSWW,
	I_PMADCSSWD,
	I_PMADCSWD,
	I_PMADDUBSW,
	I_PMADDWD,
	I_PMAGW,
	I_PMAXSB,
	I_PMAXSD,
	I_PMAXSW,
	I_PMAXUB,
	I_PMAXUD,
	I_PMAXUW,
	I_PMINSB,
	I_PMINSD,
	I_PMINSW,
	I_PMINUB,
	I_PMINUD,
	I_PMINUW,
	I_PMOVMSKB,
	I_PMOVSXBD,
	I_PMOVSXBQ,
	I_PMOVSXBW,
	I_PMOVSXDQ,
	I_PMOVSXWD,
	I_PMOVSXWQ,
	I_PMOVZXBD,
	I_PMOVZXBQ,
	I_PMOVZXBW,
	I_PMOVZXDQ,
	I_PMOVZXWD,
	I_PMOVZXWQ,
	I_PMULDQ,
	I_PMULHRIW,
	I_PMULHRSW,
	I_PMULHRWA,
	I_PMULHRWC,
	I_PMULHUW,
	I_PMULHW,
	I_PMULLD,
	I_PMULLW,
	I_PMULUDQ,
	I_PMVGEZB,
	I_PMVLZB,
	I_PMVNZB,
	I_PMVZB,
	I_POP,
	I_POPA,
	I_POPAD,
	I_POPAW,
	I_POPCNT,
	I_POPF,
	I_POPFD,
	I_POPFQ,
	I_POPFW,
	I_POR,
	I_PPERM,
	I_PREFETCH,
	I_PREFETCHNTA,
	I_PREFETCHT0,
	I_PREFETCHT1,
	I_PREFETCHT2,
	I_PREFETCHW,
	I_PROTB,
	I_PROTD,
	I_PROTQ,
	I_PROTW,
	I_PSADBW,
	I_PSHAB,
	I_PSHAD,
	I_PSHAQ,
	I_PSHAW,
	I_PSHLB,
	I_PSHLD,
	I_PSHLQ,
	I_PSHLW,
	I_PSHUFB,
	I_PSHUFD,
	I_PSHUFHW,
	I_PSHUFLW,
	I_PSHUFW,
	I_PSIGNB,
	I_PSIGND,
	I_PSIGNW,
	I_PSLLD,
	I_PSLLDQ,
	I_PSLLQ,
	I_PSLLW,
	I_PSRAD,
	I_PSRAW,
	I_PSRLD,
	I_PSRLDQ,
	I_PSRLQ,
	I_PSRLW,
	I_PSUBB,
	I_PSUBD,
	I_PSUBQ,
	I_PSUBSB,
	I_PSUBSIW,
	I_PSUBSW,
	I_PSUBUSB,
	I_PSUBUSW,
	I_PSUBW,
	I_PSWAPD,
	I_PTEST,
	I_PUNPCKHBW,
	I_PUNPCKHDQ,
	I_PUNPCKHQDQ,
	I_PUNPCKHWD,
	I_PUNPCKLBW,
	I_PUNPCKLDQ,
	I_PUNPCKLQDQ,
	I_PUNPCKLWD,
	I_PUSH,
	I_PUSHA,
	I_PUSHAD,
	I_PUSHAW,
	I_PUSHF,
	I_PUSHFD,
	I_PUSHFQ,
	I_PUSHFW,
	I_PXOR,
	I_RCL,
	I_RCPPS,
	I_RCPSS,
	I_RCR,
	I_RDM,
	I_RDMSR,
	I_RDPMC,
	I_RDSHR,
	I_RDTSC,
	I_RDTSCP,
	I_RESB,
	I_RESD,
	I_RESO,
	I_RESQ,
	I_REST,
	I_RESW,
	I_RET,
	I_RETF,
	I_RETN,
	I_ROL,
	I_ROR,
	I_ROUNDPD,
	I_ROUNDPS,
	I_ROUNDSD,
	I_ROUNDSS,
	I_RSDC,
	I_RSLDT,
	I_RSM,
	I_RSQRTPS,
	I_RSQRTSS,
	I_RSTS,
	I_SAHF,
	I_SAL,
	I_SALC,
	I_SAR,
	I_SBB,
	I_SCASB,
	I_SCASD,
	I_SCASQ,
	I_SCASW,
	I_SFENCE,
	I_SGDT,
	I_SHL,
	I_SHLD,
	I_SHR,
	I_SHRD,
	I_SHUFPD,
	I_SHUFPS,
	I_SIDT,
	I_SKINIT,
	I_SLDT,
	I_SMI,
	I_SMINT,
	I_SMINTOLD,
	I_SMSW,
	I_SQRTPD,
	I_SQRTPS,
	I_SQRTSD,
	I_SQRTSS,
	I_STC,
	I_STD,
	I_STGI,
	I_STI,
	I_STMXCSR,
	I_STOSB,
	I_STOSD,
	I_STOSQ,
	I_STOSW,
	I_STR,
	I_SUB,
	I_SUBPD,
	I_SUBPS,
	I_SUBSD,
	I_SUBSS,
	I_SVDC,
	I_SVLDT,
	I_SVTS,
	I_SWAPGS,
	I_SYSCALL,
	I_SYSENTER,
	I_SYSEXIT,
	I_SYSRET,
	I_TEST,
	I_UCOMISD,
	I_UCOMISS,
	I_UD0,
	I_UD1,
	I_UD2,
	I_UMOV,
	I_UNPCKHPD,
	I_UNPCKHPS,
	I_UNPCKLPD,
	I_UNPCKLPS,
	I_VERR,
	I_VERW,
	I_VMCALL,
	I_VMCLEAR,
	I_VMLAUNCH,
	I_VMLOAD,
	I_VMMCALL,
	I_VMPTRLD,
	I_VMPTRST,
	I_VMREAD,
	I_VMRESUME,
	I_VMRUN,
	I_VMSAVE,
	I_VMWRITE,
	I_VMXOFF,
	I_VMXON,
	I_WAIT,
	I_WBINVD,
	I_WRMSR,
	I_WRSHR,
	I_XADD,
	I_XBTS,
	I_XCHG,
	I_XCRYPTCBC,
	I_XCRYPTCFB,
	I_XCRYPTECB,
	I_XCRYPTOFB,
	I_XLAT,
	I_XLATB,
	I_XOR,
	I_XORPD,
	I_XORPS,
	I_XSHA1,
	I_XSHA256,
	I_XSTORE,
	I_CMOVcc,
	I_Jcc,
	I_SETcc,
	I_none = -1

};

struct itemplate {
    enum opcode opcode;		/* the token, passed from "parser.c" */
    int operands;		/* number of operands */
    opflags_t opd[MAX_OPERANDS]; /* bit flags for operand types */
    const char *code;		/* the code it assembles to */
    uint32_t flags;		/* some flags */
    const char *str;
};

/* Disassembler table structure */
/* If n == -1, then p points to another table of 256
   struct disasm_index, otherwise p points to a list of n
   struct itemplates to consider. */
struct disasm_index {
    const void *p;
    int n;
};

/* Tables for the assembler and disassembler, respectively */
extern const struct itemplate * const nasm_instructions[];
extern const struct disasm_index itable[256];

/*
 * this define is used to signify the end of an itemplate
 */
#define ITEMPLATE_END {-1,-1,{-1,-1,-1},NULL,0, NULL}

/*
 * Instruction template flags. These specify which processor
 * targets the instruction is eligible for, whether it is
 * privileged or undocumented, and also specify extra error
 * checking on the matching of the instruction.
 *
 * IF_SM stands for Size Match: any operand whose size is not
 * explicitly specified by the template is `really' intended to be
 * the same size as the first size-specified operand.
 * Non-specification is tolerated in the input instruction, but
 * _wrong_ specification is not.
 *
 * IF_SM2 invokes Size Match on only the first _two_ operands, for
 * three-operand instructions such as SHLD: it implies that the
 * first two operands must match in size, but that the third is
 * required to be _unspecified_.
 *
 * IF_SB invokes Size Byte: operands with unspecified size in the
 * template are really bytes, and so no non-byte specification in
 * the input instruction will be tolerated. IF_SW similarly invokes
 * Size Word, and IF_SD invokes Size Doubleword.
 *
 * (The default state if neither IF_SM nor IF_SM2 is specified is
 * that any operand with unspecified size in the template is
 * required to have unspecified size in the instruction too...)
 */

#define IF_SM     0x00000001UL  /* size match */
#define IF_SM2    0x00000002UL  /* size match first two operands */
#define IF_SB     0x00000004UL  /* unsized operands can't be non-byte */
#define IF_SW     0x00000008UL  /* unsized operands can't be non-word */
#define IF_SD     0x0000000CUL  /* unsized operands can't be non-dword */
#define IF_SQ     0x00000010UL  /* unsized operands can't be non-qword */
#define IF_SO     0x00000014UL  /* unsized operands can't be non-oword */
#define IF_SMASK  0x0000001CUL  /* mask for unsized argument size */
#define IF_AR0	  0x00000020UL  /* SB, SW, SD applies to argument 0 */
#define IF_AR1	  0x00000040UL  /* SB, SW, SD applies to argument 1 */
#define IF_AR2	  0x00000060UL  /* SB, SW, SD applies to argument 2 */
#define IF_AR3	  0x00000080UL  /* SB, SW, SD applies to argument 2 */
#define IF_ARMASK 0x000000E0UL  /* mask for unsized argument spec */
#define IF_PRIV   0x00000100UL  /* it's a privileged instruction */
#define IF_SMM    0x00000200UL  /* it's only valid in SMM */
#define IF_PROT   0x00000400UL  /* it's protected mode only */
#define IF_NOLONG 0x00000800UL  /* it's not available in long mode */
#define IF_UNDOC  0x00001000UL  /* it's an undocumented instruction */
#define IF_FPU    0x00002000UL  /* it's an FPU instruction */
#define IF_MMX    0x00004000UL  /* it's an MMX instruction */
#define IF_3DNOW  0x00008000UL  /* it's a 3DNow! instruction */
#define IF_SSE    0x00010000UL  /* it's a SSE (KNI, MMX2) instruction */
#define IF_SSE2   0x00020000UL  /* it's a SSE2 instruction */
#define IF_SSE3   0x00040000UL  /* it's a SSE3 (PNI) instruction */
#define IF_VMX	  0x00080000UL  /* it's a VMX instruction */
#define IF_LONG   0x00100000UL	/* long mode instruction */
#define IF_SSSE3  0x00200000UL  /* it's an SSSE3 instruction */
#define IF_SSE4A  0x00400000UL  /* AMD SSE4a */
#define IF_SSE41  0x00800000UL  /* it's an SSE4.1 instruction */
#define IF_SSE42  0x00800000UL  /* HACK NEED TO REORGANIZE THESE BITS */
#define IF_SSE5   0x00800000UL  /* HACK NEED TO REORGANIZE THESE BITS */
#define IF_PMASK  0xFF000000UL  /* the mask for processor types */
#define IF_PLEVEL 0x0F000000UL  /* the mask for processor instr. level */
                                        /* also the highest possible processor */
#define IF_PFMASK 0xF01FFF00UL  /* the mask for disassembly "prefer" */
#define IF_8086   0x00000000UL  /* 8086 instruction */
#define IF_186    0x01000000UL  /* 186+ instruction */
#define IF_286    0x02000000UL  /* 286+ instruction */
#define IF_386    0x03000000UL  /* 386+ instruction */
#define IF_486    0x04000000UL  /* 486+ instruction */
#define IF_PENT   0x05000000UL  /* Pentium instruction */
#define IF_P6     0x06000000UL  /* P6 instruction */
#define IF_KATMAI 0x07000000UL  /* Katmai instructions */
#define IF_WILLAMETTE 0x08000000UL      /* Willamette instructions */
#define IF_PRESCOTT   0x09000000UL      /* Prescott instructions */
#define IF_X86_64 0x0A000000UL	/* x86-64 instruction (long or legacy mode) */
#define IF_NEHALEM 0x0B000000UL  /* Nehalem instruction */
#define IF_X64	  (IF_LONG|IF_X86_64)
#define IF_IA64   0x0F000000UL  /* IA64 instructions (in x86 mode) */
#define IF_CYRIX  0x10000000UL  /* Cyrix-specific instruction */
#define IF_AMD    0x20000000UL  /* AMD-specific instruction */

#endif
