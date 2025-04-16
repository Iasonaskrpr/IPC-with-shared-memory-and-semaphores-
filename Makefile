CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -lrt -pthread
TARGETS := parent child

.PHONY: all run clean

all: $(TARGETS)

parent: Source/Parent.c
	$(CC) $(CFLAGS) -o $@ $<

child: Source/Child.c
	$(CC) $(CFLAGS) -o $@ $<

run: all
	@echo "Starting parent process..."
	./parent

clean:
	@echo "Cleaning up..."
	rm -f $(TARGETS)