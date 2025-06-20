// blockdev.h - Emulação de disco
#ifndef BLOCKDEV_H
#define BLOCKDEV_H

void blockdev_read(const char* file, int block, void* buf);
void blockdev_write(const char* file, int block, void* buf);

#endif
