CC = gcc
CFLAGS = -Wall -Wextra -std=c11

TARGET = memory_lab
SRCS = src/main.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
