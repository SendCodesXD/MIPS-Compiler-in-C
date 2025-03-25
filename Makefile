all:
	gcc vm.h vm.c instruction.h instruction.c utils.h utils.c constants.h macro.h macro.c symbol.h symbol.c data.h data.c assembler.c -o assembler -g -O0 -DMYDEBUG

zip: clean
	zip src.zip vm.h vm.c instruction.h instruction.c utils.h utils.c constants.h macro.h macro.c symbol.h symbol.c data.h data.c assembler.c mips.txt Makefile macros.asm

clean:
	rm -f assembler symboltable.txt execute.txt src.zip