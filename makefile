# C Compiler
CC := gcc

# C Flag(s)
CFLAG := -o

# Output prgram
PROG := branchTracking.exe
LOCAL := bin/

all: main.c
	$(CC) $^ $(CFLAG) $(PROG)
	mv $(PROG) $(LOCAL)

run: $(LOCAL)$(PROG)
	./$^

clean: $(LOCAL)*.exe
	rm -f $^