BASE_CC = gcc
CFLAGS ?= -g -Wall -Wextra
LDFLAGS ?= -lpthread -lrt
TARGET ?= aesdsocket
CC ?= $(CROSS_COMPILE)$(BASE_CC)

all: $(TARGET)

aesdsocket: aesdsocket.c thread_queue.c
	$(CC) $(CFLAGS) -I . -o $(TARGET) aesdsocket.c thread_queue.c $(LDFLAGS)
clean:
	rm -f $(TARGET)