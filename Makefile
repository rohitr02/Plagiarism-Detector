OUTPUT=compare
CFLAGS=-g -Wall -Werror -fsanitize=undefined -fsanitize=address -std=c99
LFLAGS = -pthread

all: $(OUTPUT)

compare: compare.c compare.h
	gcc $(CFLAGS) -o $@ $< $(LFLAGS) -lm

clean:
	rm -f *.o $(OUTPUT)
