CC=gcc
CFLAGS=-Wall
TARGET=oss user

all: $(TARGET) 

$(TARGET): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o $(TARGET)
