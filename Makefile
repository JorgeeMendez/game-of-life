CC     = gcc
CFLAGS = -O3 -Wall -Wextra -std=c11
TARGET = lifegame

all: $(TARGET)

$(TARGET): main.o lifegame.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c lifegame.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET) $(TARGET).exe

.PHONY: all clean