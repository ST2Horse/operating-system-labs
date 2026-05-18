CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LDFLAGS = -pthread

TARGET = pv_lab
SRCS = src/main.c \
       src/common.c \
       src/producer_consumer.c \
       src/reader_writer.c \
       src/dining_philosophers.c \
       src/smokers.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
