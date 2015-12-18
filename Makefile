HDR = execute.h program.h public.h
SRC = execute.c main.c parse.c program.c

sish : ${SRC} ${HDR}
	gcc -g -Wall ${SRC} -o sish

