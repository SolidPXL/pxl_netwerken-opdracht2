SRCS := main test
all: $(SRCS)
CC = gcc
override CFLAGS = -lws2_32

$(SRCS): $(SRCS).c
	$(CC) $@.c -o $@.exe $(CFLAGS)