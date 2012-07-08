extern int ext;

int  func()
{
	int c = 100;
	char a[100];
	int b = 12;
	int d;
	
	a[0] = 'c';
	d = c * b / 4;
	
	return 0;
}





$ readelf.exe -a test_elf.o
ELF Header:
  Magic:   7f 45 4c 46 01 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF32
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              REL (Relocatable file)
  Machine:                           Intel 80386
  Version:                           0x1
  Entry point address:               0x0
  Start of program headers:          0 (bytes into file)
  Start of section headers:          272 (bytes into file)
  Flags:                             0x0
  Size of this header:               52 (bytes)
  Size of program headers:           0 (bytes)
  Number of program headers:         0
  Size of section headers:           40 (bytes)
  Number of section headers:         7
  Section header string table index: 6

Section Headers:
  [Nr] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al

  [ 0]                   NULL            00000000 000000 000000 00      0   0  0

  [ 1] .text             PROGBITS        00000000 000040 000042 00  AX  0   0 32

  [ 2] .data             PROGBITS        00000000 0000a0 000000 00  WA  0   0 32

  [ 3] .bss              NOBITS          00000000 0000a0 000000 00  WA  0   0 32

  [ 4] .symtab           SYMTAB          00000000 0000a0 000030 10      5   2  4

  [ 5] .strtab           STRTAB          00000000 0000d0 000011 00      0   0  1

  [ 6] .shstrtab         STRTAB          00000000 0000e1 00002c 00      0   0  1

Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings)
  I (info), L (link order), G (group), x (unknown)
  O (extra OS processing required) o (OS specific), p (processor specific)

There are no section groups in this file.

There are no program headers in this file.

There are no relocations in this file.

There are no unwind sections in this file.

Symbol table '.symtab' contains 3 entries:
   Num:    Value  Size Type    Bind   Vis      Ndx Name
     0: 00000000     0 NOTYPE  LOCAL  DEFAULT  UND
     1: 00000000     0 FILE    LOCAL  DEFAULT  ABS test_elf.c
     2: 00000000    66 FUNC    GLOBAL DEFAULT    1 func

No version information found in this file.



$ objdump.exe -D test_elf.o

test_elf.o:     file format elf32-i386


Disassembly of section .text:

00000000 <func>:
   0:   55                      push   %ebp
   1:   89 e5                   mov    %esp,%ebp
   3:   81 ec 70 00 00 00       sub    $0x70,%esp
   9:   90                      nop
   a:   b8 64 00 00 00          mov    $0x64,%eax
   f:   89 45 fc                mov    %eax,-0x4(%ebp)
  12:   b8 0c 00 00 00          mov    $0xc,%eax
  17:   89 45 94                mov    %eax,-0x6c(%ebp)
  1a:   b8 63 00 00 00          mov    $0x63,%eax
  1f:   88 45 98                mov    %al,-0x68(%ebp)
  22:   8b 45 fc                mov    -0x4(%ebp),%eax
  25:   8b 4d 94                mov    -0x6c(%ebp),%ecx
  28:   0f af c1                imul   %ecx,%eax
  2b:   b9 04 00 00 00          mov    $0x4,%ecx
  30:   99                      cltd
  31:   f7 f9                   idiv   %ecx
  33:   89 45 90                mov    %eax,-0x70(%ebp)
  36:   b8 00 00 00 00          mov    $0x0,%eax
  3b:   e9 00 00 00 00          jmp    40 <func+0x40>
  40:   c9                      leave
  41:   c3                      ret
