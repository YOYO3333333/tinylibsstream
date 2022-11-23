CC = gcc
CFLAGS = -fPIC -Wall -Wextra -Werror -pedantic -std=c99 -g

SRC = tiny.c
OBJS = $(SRC:.c=.o)
LIB = libstream.a

all: library

library: $(LIB)
$(LIB): $(OBJS)
	ar crs $@ $^

clean:
	$(RM) *.o *.a
