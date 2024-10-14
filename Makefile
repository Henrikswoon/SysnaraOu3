CC = gcc
CFLAGS = -g -std=gnu11 -Werror  -Wall -Wextra -Wpedantic -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition
SOURCES = mdu.c queue.c du_worker.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = mdu

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

clean:
	rm -f $(TARGET) *.o