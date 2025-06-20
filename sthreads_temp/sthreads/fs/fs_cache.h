// fs_cache.h - Interface de cache
#ifndef FS_CACHE_H
#define FS_CACHE_H

void* fs_cache_read(const char* file, int offset, int size);
void fs_cache_write(const char* file, int offset, int size, void* data);

#endif
