CC = gcc

CFLAGS = -std=gnu99 -Wall

TARGET = emu

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	rm $(TARGET)