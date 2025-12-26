# C Compiler
CC := gcc

# C Flag(s)
CFLAG := -o

# Output prgram
PROG := bin/btmap.exe

all: main.c
	$(CC) $^ $(CFLAG) $(PROG)

run: $(PROG)
	./$^ -h
