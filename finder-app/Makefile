CROSS_COMPILE = $(CROSS_COMPILE)gcc
NATIVE = gcc
TARGET = writer
SRC = writer.c

all: 
	$(NATIVE) $(SRC) -o $(OBJECTS) $(TARGET)

arm:
	$(CROSS_COMPILE) $(SRC) -o $(OBJECTS) $(TARGET)

clean:
	rm -rf $(OBJECTS) $(TARGET)