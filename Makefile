CC = gcc

CFLAGS = -std=gnu99 -Wall  -F/Library/Frameworks

LIB = -framework SDL2


TARGET = emu

all: $(TARGET)

$(TARGET): emu.o helpers.o proc.o
	$(CC) $(CFLAGS) $^ -o $@ $(LIB) 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -g

clean:
	rm $(TARGET) *.o
