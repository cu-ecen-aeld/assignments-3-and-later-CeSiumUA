BASE_CC = gcc
CFLAGS = -g -Wall -Wextra
LDFLAGS = -lpthread -lrt
OUTPUT_FILE = aesdsocket
CC=$(CROSS_COMPILE)$(BASE_CC)

all: $(OUTPUT_FILE)

aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) -I . -o $(OUTPUT_FILE) aesdsocket.c thread_queue.c $(LDFLAGS)
clean:
	rm -f $(OUTPUT_FILE)