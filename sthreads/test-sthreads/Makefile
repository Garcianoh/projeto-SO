# Diretórios
INCLUDE_DIR = ../include
LIB_DIR = ../sthread_lib

# Compilador e flags
CC = gcc
CFLAGS = -g -O0 -Wall -I$(INCLUDE_DIR) -I$(LIB_DIR)
LDFLAGS = $(LIB_DIR)/libsthread.a

# Alvo principal
all: supermercado

# Compilar o teste supermercado
supermercado: 10supermercado.c
	$(CC) $(CFLAGS) -o supermercado 10supermercado.c $(LDFLAGS)

# Limpar objetos e binários
clean:
	rm -f supermercado *.o
