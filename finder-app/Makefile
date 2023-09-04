CC = $(CROSS_COMPILE)gcc
NATIVE = gcc
TARGET = writer
SRC = writer.c

all: 
	$(NATIVE) $(SRC) -o $(OBJECTS) $(TARGET)

arm:
	$(CC) $(SRC) -o $(OBJECTS) $(TARGET)

clean:
	rm -rf $(OBJECTS) $(TARGET)