BASE_CC = gcc
CFLAGS = -Wall -Wextra
OUTPUT_FILE = writer
CC=$(CROSS_COMPILE)$(BASE_CC)

all: $(OUTPUT_FILE)

writer: writer.c
	$(CC) $(CFLAGS) -o $(OUTPUT_FILE) writer.c
clean:
	rm -f $(OUTPUT_FILE)