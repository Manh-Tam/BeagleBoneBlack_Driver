# Compiler and flags
CC:=arm-linux-gnueabihf-gcc
CFLAGS:=-static

# Source and object files
SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)

# Default target
TARGET := gpio_button_poll_app

#$^: equivalent to all dependencies
#$@: equivalent to 
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# $<: equivalent to one dependency %.c
# $@: equivalent to target %.o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

all: $(TARGET)

run: $(TARGET)
	./$(TARGET)

scp: $(TARGET)
	scp $(TARGET) debian@192.168.0.172:/home/debian/workspace/gpio_button_poll/

clean:
	rm -rf $(OBJ) $(TARGET)

.PHONY: all clean run