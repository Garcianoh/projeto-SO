# Makefile para o sistema de ficheiros e teste
CC=gcc
CFLAGS=-Wall -g -pthread -I../sthread_lib

SRC=fs_daemon.c fs_ipc.c fs_core.c fs_cache.c fs_cow.c fs_replica.c fs_sync.c blockdev.c
OBJ=$(SRC:.c=.o)

all: fs_daemon test_fs

fs_daemon: $(OBJ)
	$(CC) $(CFLAGS) -o fs_daemon $(OBJ)

test_fs: test_fs.c fs_ipc.c
	$(CC) $(CFLAGS) -o test_fs test_fs.c fs_ipc.c

clean:
	rm -f *.o fs_daemon test_fs
