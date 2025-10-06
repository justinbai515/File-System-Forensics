GCC:=gcc
FLAGS:=-Wall -g -Wextra

all: clean runscan

runscan: read_ext2.o
	$(GCC) $(FLAGS) read_ext2.o -o runscan runscan.c

%.o: %.c
	$(GCC) $(FLAGS) -c $< -o $@

clean:
	@rm -f *.o runscan

.PHONY: all runscan clean