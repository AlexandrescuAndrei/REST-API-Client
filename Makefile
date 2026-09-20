# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Files
SRC = client.c helper.c requests.c parson.c buffer.c
OBJ = $(SRC:.c=.o)
EXEC = client

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXEC)
