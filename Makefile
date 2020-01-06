CC = gcc

CFLAGS = -std=gnu99 -Wall  -F/Library/Frameworks

LIB = -framework SDL2


TARGET = emu

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c $(LIB) 

clean:
	rm $(TARGET)
