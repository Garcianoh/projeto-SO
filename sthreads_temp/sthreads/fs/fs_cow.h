// fs_cow.h - Copy-on-write
#ifndef FS_COW_H
#define FS_COW_H

void* fs_cow_get_block(const char* file, int block);

#endif
