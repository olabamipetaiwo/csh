CC     = gcc
CFLAGS = -Wall -Wextra -g

all: probe csh

probe: probe.c
	$(CC) $(CFLAGS) -o probe probe.c

csh: lex.yy.c csh.c
	$(CC) $(CFLAGS) -Wno-unused-function -Wno-sign-compare -o csh lex.yy.c csh.c

lex.yy.c: lex.l
	flex lex.l

clean:
	rm -f probe csh lex.yy.c