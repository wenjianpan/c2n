all:
#	flex -o scan.c scan.l
	gcc -g stack.c symtab.c num.c main.c Geni386.c insnsa.c -o cparser.exe
	