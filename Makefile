OUTPUT=compare
CFLAGS=-g -Wall -Werror -fsanitize=undefined -fsanitize=address -std=c99

all: $(OUTPUT)

compare: compare.c compare.h
	gcc $(CFLAGS) -o $@ $<

clean:
	rm -f *.o $(OUTPUT)
