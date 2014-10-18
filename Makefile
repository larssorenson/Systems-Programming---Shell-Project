
#Use GNU compiler
cc = gcc -g
CC = g++ -g

LEX=lex
YACC=yacc

all: clean shell

lex.yy.o: shell.l 
	$(LEX) shell.l
	$(CC) -c lex.yy.c

y.tab.o: shell.y
	$(YACC) -d shell.y
	$(CC) -c y.tab.c

command.o: command.cc
	$(CC) -c command.cc

shell: y.tab.o lex.yy.o command.o read-line.o tty-raw-mode.o
	$(CC) -o shell lex.yy.o y.tab.o command.o read-line.o tty-raw-mode.o -lfl
	
tty-raw-mode.o: tty-raw-mode.c
	$(CC) -c tty-raw-mode.c

read-line.o: read-line.c
	$(CC) -c read-line.c
	
keyboard-example: keyboard-example.c tty-raw-mode.o
	$(CC) -o keyboard-example keyboard-example.c tty-raw-mode.o

read-line-example: read-line-example.c tty-raw-mode.o read-line.o
	$(CC) -o read-line-example read-line-example.c tty-raw-mode.o read-line.o

clean:
	rm -f lex.yy.c y.tab.c  y.tab.h shell *.o

