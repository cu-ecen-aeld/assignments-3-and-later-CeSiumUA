BASE_CC = gcc
CFLAGS = -g -Wall -Wextra
OUTPUT_FILE = aesdsocket
CC=$(CROSS_COMPILE)$(BASE_CC)

all: $(OUTPUT_FILE)

aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) -I . -o $(OUTPUT_FILE) aesdsocket.c thread_queue.c
clean:
	rm -f $(OUTPUT_FILE)