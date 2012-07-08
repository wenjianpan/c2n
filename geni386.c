#include "I386.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "pcc.h"
#include "insns.h"

IA32Regs  g_regs;

char* g_reg_str[] = {
	"EAX",
	"EBX",
	"ECX",
	"EDX",
	"ESI",
	"EDI",
	"EBP",
	"ESP",
	"EIP",
	"CS",
	"DS",
	"SS",
	"ES",
	"FS",
	"GS",
	"EFLAGS"
};

char* g_sub16_reg_str[] = {
	"AX",
	"BX",
	"CX",
	"DX",
	"SI",
	"DI",
	"BP",
	"SP",
	"IP"
};

char* g_sub8h_reg_str[] = {
	"AH",
	"BH",
	"CH",
	"DH"
};

char* g_sub8l_reg_str[] = {
	"AL",
	"BL",
	"CL",
	"DL"
};

void dump_regs()
{
	int i;
	unsigned long *p = (unsigned long*)(&g_regs);

	for (i = EAX; i <= ECX; i++)  // general popurse register EDX is used when out of register 
	{
		printf("0x%x ", p[i]);
	}
	printf("\n");
}

/*
  reg_size 1,2,4 bytes
  return reg index: index = (index << 16)|(0xf or 0xf0 or 0xff or 0xffff)
  When caller get index, it use (index >> 16) as reg index,
  if reg_size is 1, low 8 bits are used for identify AH&AL 
*/

int getreg(int flag, int reg_size)
{
	int i;
	unsigned long *p = (unsigned long*)(&g_regs);

	if(flag == REG_FLAG_NORMAL) {
		for (i = EAX; i <= ECX; i++)  // general popurse register EDX is used when out of register 
		{
			switch(reg_size) {
				case 1:
					if(p[i] & 0xf == 0) {
						p[i] = 0xf;
						return (i << REG_SHF)|0xf; 
					} else if(p[i] & 0xf0 == 0) {
						p[i] = 0xf;
						return (i << REG_SHF)|0xf0; 
					}
					break;
				case 2:
					if(p[i] & 0xff == 0xff) {
						p[i] = 0xff;
						return (i << REG_SHF)|0xff; 
					}
					break;
				case 4:
					if(p[i] == 0) {
						p[i] = 0xffff;
						return (i << REG_SHF)|0xffff; 
					}
					break;
				default:
					printf("error: flag reg_size %d\n", reg_size);
					exit(0);
			}
		}
		printf("error: no regs!\n");
		exit(0);
		return -1;
	}

	printf("error: flag abnormal %d\n", flag);
	exit(0);
}

void freereg(int reg_idx)
{
	unsigned long *p = (unsigned long*)(&g_regs);
	int idx = reg_idx >> REG_SHF;
	int bytes = reg_idx & 0xffff;

	//printf("free 0x%x\n", reg_idx);
	if(reg_idx == 0)
		return;
	
	if(idx >= EAX && idx <= ECX) {
		p[idx] = p[idx] & (~bytes);
	} 
	else if (idx == EDX){
		// Do nothing ?
	}else {
		// TODO
	}
}


extern func_ctx g_func_ctx;

int getStackReg()
{
	int i;
	int fisrt_unallocated = -1;
	
	for(i=0; i<MAX_STACK_REGS; i++) {
		if(g_func_ctx.stack_reg[i].offset == NA && fisrt_unallocated == -1) 
			fisrt_unallocated = i;  // not allocated, we will allocate the first one 
		else if(g_func_ctx.stack_reg[i].offset <= 0) {
			if(g_func_ctx.stack_reg[i].used == 0){
				// allocated but freed
				return i;
			}
		}
	}

	if(fisrt_unallocated == -1) {
		printf("Really bad!! out of registers...\n");
		exit(0);
	}

	g_func_ctx.stack_reg[fisrt_unallocated].offset = g_func_ctx.cur_statck_size;
	g_func_ctx.cur_statck_size += 4;   // 32bits
	g_func_ctx.stack_reg[fisrt_unallocated].used = 1;

	return fisrt_unallocated;
	
}

void freeStackReg(int idx)
{
	assert(g_func_ctx.stack_reg[idx].offset <= 0);  /* for IA32 */
	assert(g_func_ctx.stack_reg[idx].used == 1);

	g_func_ctx.stack_reg[idx].used = 0;
}


/*
#define ADDRESS_MODE(rtn_ptr, offset, reg_s)    \
                       \
do{                                    \
  if(offset > 0)                       \
    rtn_ptr = "["#reg_s"+%d]";        \
  else if(offset < 0)                  \
    rtn_ptr = "["#reg_s"%d]";         \
  else                                 \
    rtn_ptr = "["#reg_s"]";           \
}while(0)


void gen_i(enum opcode opcode, int instr_idx, int oprand_cnt,
		int dstoff, int dstreg, enum oprand_type dst_rm, enum oprand_sz dstsize,
		int srcoff, int srcreg, enum oprand_type src_rm, enum oprand_sz srcsize, long constant)
{
	const char *opcode_str;
	char addr_mos[256];

	addr_mos[0] = '\t';
	addr_mos[1] = '\0';
    opcode_str = nasm_instructions[opcode][0].str;
	strcat(addr_mos, opcode_str);
	
	if(oprand_cnt == 0) {
		oprintf("%s", addr_mos);
	} else if(oprand_cnt == 1) {
		if(src_rm == IS_CONST) {

		} else if(src_rm == IS_REG) {

		} else if(src_rm == IS_MEM) {

		} else {
			// error
		}
		
	} else if(oprand_cnt == 2){
		if(src_rm == IS_CONST && dst_rm == IS_REG) {

		} else if(src_rm == IS_CONST && dst_rm == IS_MEM) {

		} else if(src_rm == IS_REG && dst_rm == IS_REG) {

		} else if(src_rm == IS_REG && dst_rm == IS_MEM) {

		} else if(src_rm == IS_MEM && dst_rm == IS_REG) {

		} else {
			// error
		}

	} else {
		// error 
	}
	//oprintf("\tMOV\t%s,%s\n", g_reg_str[reg_idx >> REG_SHF], o_offset);
	oprintf("\r\n");
}
*/

