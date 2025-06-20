// fs_sync.h - Sincronização
#ifndef FS_SYNC_H
#define FS_SYNC_H

void fs_sync_lock(const char* file);
void fs_sync_unlock(const char* file);

#endif
