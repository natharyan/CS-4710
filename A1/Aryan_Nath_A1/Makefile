CC = gcc
CFLAGS = -Wall -O2 -pthread
TARGET = main
SRC = Aryan_Nath_A1.c
OBJ = $(SRC:.c=.o)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJ) $(TARGET)
