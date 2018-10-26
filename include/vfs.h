// vfs.h

#ifndef VFS_H
#define VFS_H

#include "file.h"

struct Vfs_state;

int vfs_init(unsigned int disk_size);

int vfs_allocate_block(unsigned int size);

struct Vfs_file *vfs_create_file(const char *file_name);

struct Vfs_file *vfs_create_dir(const char *file_name);

int vfs_write(struct Vfs_file *file, const void *data, unsigned int size);

int vfs_append(struct Vfs_file *file, const void *data, unsigned int size);

int vfs_read(struct Vfs_file *file);

int vfs_print_file_info(const struct Vfs_file *file);

struct Vfs_state* vfs_get_state();

int vfs_dump_disk(const char *path);

void vfs_get_error();

void vfs_free();

#endif // VFS_H