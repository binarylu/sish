HDR = $(wildcard *.h)
SRC = $(wildcard *.c)

.Phony: all clean

all : sish

clean :
	rm sish

sish : ${SRC} ${HDR}
	gcc -Wall ${SRC} -o $@

