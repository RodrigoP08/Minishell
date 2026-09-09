#compilador gcc
CC = gcc

TARGET = mini_shell

all: $(TARGET)

$(TARGET): shell.o
	$(CC) -o $(TARGET) shell.o
	
shell.o: shell.c
	$(CC) -c shell.c

clean:
	rm -f $(TARGET) *.o
