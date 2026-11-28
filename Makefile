CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
TARGET = lifegame
TEST   = tests/test_lifegame

all: $(TARGET)

$(TARGET): main.o lifegame.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c lifegame.h
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST): tests/test_lifegame.c lifegame.o lifegame.h
	$(CC) $(CFLAGS) -I. -o $@ tests/test_lifegame.c lifegame.o

test: $(TARGET) $(TEST)
	./$(TEST)

clean:
	rm -f *.o $(TARGET) $(TARGET).exe $(TEST) $(TEST).exe

.PHONY: all test clean