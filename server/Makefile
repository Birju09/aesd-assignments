CROSS_COMPILE = aarch64-none-linux-gnu-
NATIVE = gcc
TARGET = aesdsocket
SRC = aesdsocket.c
OBJECTS = aesdsocket.o
CFLAGS = -Wall -Werror

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(SRC) -g -o $(TARGET) $(CFLAGS) 

clean:
	rm -rf $(OBJECTS) $(TARGET)