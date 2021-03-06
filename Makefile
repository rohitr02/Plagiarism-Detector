OUTPUT=compare
CFLAGS=-g -Wall -Werror -fsanitize=undefined -fsanitize=address -std=c99
LFLAGS = -pthread -lm

all: $(OUTPUT)

compare: compare.c compare.h
	gcc $(CFLAGS) -o $@ $< $(LFLAGS)

clean:
	rm -f *.o $(OUTPUT)
