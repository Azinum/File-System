// vfs.c

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hash.h"
#include "vfs.h"

#define array_length(array) sizeof(array) / sizeof(array[0])

struct Vfs_state {
   char* disk;
   unsigned int disk_size;
   unsigned int error;
   struct Vfs_file *current_directory;
};

enum Vfs_errors {
   VFS_ERR_NONE,
   VFS_ERR_INVALID_DISK,
   VFS_ERR_INVALID_STATE,
   VFS_ERR_INVALID_WRITE,
   VFS_ERR_MEMORY,
   VFS_ERR_ALLOC_FILE,
   VFS_ERR_WRITE,
   VFS_ERR_INVALID_FILE,
   VFS_ERR_EMPTY_FILE
};

const char *vfs_errors[] = {
   "",
   "Invalid disk",
   "Invalid Vfs state",
   "Bad write address",
   "Out of memory",
   "Failed to allocate file",
   "Failed to write to file",
   "Bad file",
   "File is empty"
};

const char *vfs_file_types[] = {
   "Unknown",
   "File",
   "Directory"
};

static struct Vfs_state vfs_state;

int vfs_raw_write(unsigned int addr, void *buff, unsigned int size);
struct Vfs_file *vfs_allocate_file(const char *file_name, int type);


int vfs_init(unsigned int disk_size) {
   vfs_state.disk = malloc(sizeof(char) * disk_size);
   if (!vfs_state.disk) {
      vfs_state.error = VFS_ERR_INVALID_DISK;
      return -1;
   }

   vfs_state.disk_size = disk_size;
   vfs_state.error = 0;
   // vfs_state.current_directory = vfs_allocate_file("root", T_DIR);
   // vfs_write(vfs_state.current_directory, "", 0);
   return 0;
}

int vfs_flush(unsigned int start, unsigned int end) {
   if (start > end ||
         start > vfs_state.disk_size ||
         end > vfs_state.disk_size) {
      vfs_state.error = VFS_ERR_INVALID_WRITE;
      return -1;
   }

   for (unsigned int i = start; i < end; i++) {
      vfs_state.disk[i] = 0;
   }
   return 0;
}

int vfs_raw_write(unsigned int addr, void *buff, unsigned int size) {
   if (!vfs_state.disk) {
      vfs_state.error = VFS_ERR_INVALID_DISK;
      return -1;
   }

   if ((addr + size) >= vfs_state.disk_size) {
      vfs_state.error = VFS_ERR_INVALID_WRITE;
      return -1;
   }

   memcpy(&vfs_state.disk[addr], buff, size);
   return 0;
}

/*
   Scan through the disk from start to end.
   If there is a free block then jump to the end of it and continue.
*/
int vfs_allocate_block(unsigned int size) {
   if (!vfs_state.disk) {
      vfs_state.error = VFS_ERR_INVALID_DISK;
      return -1;
   }

   char* disk = vfs_state.disk;

   unsigned int free_space = 0;

   for (unsigned int i = 0; i < vfs_state.disk_size; i++) {
      if (free_space >= size) {
         return i - (size + (free_space % size));
      }
      switch (disk[i]) {
         case BLOCK_USED: {
            free_space = 0;
            i += sizeof(struct Data_block);
         }
            break;

         case BLOCK_FREE: {
            free_space += sizeof(struct Data_block);
            i += sizeof(struct Data_block);
            vfs_flush(i - sizeof(struct Data_block), i); // Flush to remove any data in that block
         }
            break;

         case BLOCK_FILE_HEADER: {
            free_space = 0;
            i += sizeof(struct Vfs_file);
         }
            break;

         case BLOCK_FILE_HEADER_FREE: {
            free_space += sizeof(struct Vfs_file);
            i += sizeof(struct Vfs_file);
            vfs_flush(i - sizeof(struct Vfs_file), i);
         }
            break;
         
         case 0: {
            free_space += 1;
         }
            break;
      }
   }

   return -1;
}

struct Vfs_file *vfs_allocate_file(const char *file_name, int type) {
   struct Vfs_file *file = NULL;

   int addr = vfs_allocate_block(sizeof(struct Vfs_file));

   if (addr < 0) {
      vfs_state.error = VFS_ERR_MEMORY;
      return NULL;
   }

   file = (struct Vfs_file*)&vfs_state.disk[addr];

   file->block_type = BLOCK_FILE_HEADER;
   file->type = type;
   file->addr = 0;
   file->size = 0;

   unsigned long name_len = strlen(file_name);

   for (unsigned long i = 0; i < array_length(file->file_name) && i < name_len; i++) {
      file->file_name[i] = file_name[i];
   }

   file->hash = hash(file->file_name, array_length(file->file_name));

   vfs_raw_write(addr, file, sizeof(struct Vfs_file));

   return file;
}

struct Vfs_file *vfs_create_file(const char *file_name) {
   struct Vfs_file *file = vfs_allocate_file(file_name, T_FILE);

   if (!file) {
      vfs_state.error = VFS_ERR_ALLOC_FILE;
      return NULL;
   }

   return file;
}

struct Vfs_file *vfs_create_dir(const char *file_name) {
   struct Vfs_file *file = vfs_allocate_file(file_name, T_DIR);

   if (!file) {
      vfs_state.error = VFS_ERR_ALLOC_FILE;
      return NULL;
   }

   return file;
}

int vfs_write(struct Vfs_file *file, const void *data, unsigned int size) {
   if (!file) {
      vfs_state.error = VFS_ERR_INVALID_WRITE;
      return -1;
   }

   unsigned int bytes_written = 0;
   unsigned int blocks_to_write = (size / BLOCK_SIZE) + 1;
   int block_addr = 0;
   const char *read_data = (const char*)data;

   // printf("Blocks to write: %i (size:%i)\n", blocks_to_write, size);

   block_addr = vfs_allocate_block(sizeof(struct Data_block));
   struct Data_block *data_block = NULL;

   if (block_addr < 0) {
      vfs_state.error = VFS_ERR_MEMORY;
      return -1;
   }

   data_block = (struct Data_block*)&vfs_state.disk[block_addr];
   data_block->block_type = BLOCK_USED;
   data_block->next_block_addr = 0;
   file->addr = block_addr;

   for (int i = 0; i < (BLOCK_SIZE) && bytes_written < size ; i++) {
      data_block->block[i] = read_data[bytes_written++];
   }

   for (unsigned int i = 0; i < blocks_to_write - 1; i++) {
      block_addr = vfs_allocate_block(sizeof(struct Data_block));
      if (block_addr < 0) {
         vfs_state.error = VFS_ERR_MEMORY;
         return -1;
      }
      data_block->next_block_addr = block_addr;

      data_block = (struct Data_block*)&vfs_state.disk[block_addr];
      data_block->block_type = BLOCK_USED;
      data_block->next_block_addr = 0;

      for (int i = 0; i < (BLOCK_SIZE) && bytes_written < size ; i++) {
         data_block->block[i] = read_data[bytes_written++];
      }
   }
   
   file->size = size;
   return 0;
}


// [0, 1, 2]
// 
// Do the same thing as vfs_write but first go to the last block
int vfs_append(struct Vfs_file *file, const void *data, unsigned int size) {
   if (!file) {
      vfs_state.error = VFS_ERR_INVALID_WRITE;
      return -1;
   }

   unsigned int addr = file->addr;
   int block_count = (file->size / BLOCK_SIZE) + 1;

   // Find the last written block in this file
   while (block_count--) {
      struct Data_block* data_block = (struct Data_block*)&vfs_state.disk[addr];
      addr = data_block->next_block_addr;

      if (block_count == 1) { // Last block, start writing data here
         printf("Last block addr:%i\n", addr);

         for (int i = addr; i < addr + BLOCK_SIZE; i++) {
            printf("%c", vfs_state.disk[i]);
         }
         puts("\n");
      }
   };  

   return 0;
}

int vfs_read(struct Vfs_file *file) {
   if (!file) {
      vfs_state.error = VFS_ERR_INVALID_FILE;
      return -1;
   }

   if (file->size == 0) {
      vfs_state.error = VFS_ERR_EMPTY_FILE;
      return -1;
   }

   unsigned int addr = file->addr;
   int block_count = (file->size / BLOCK_SIZE) + 1;

   while (block_count--) {
      struct Data_block* data_block = (struct Data_block*)&vfs_state.disk[addr];

      for (int i = 0; i < array_length(data_block->block); i++) {
         printf("%c", data_block->block[i]);
      }

      addr = data_block->next_block_addr;

      if (addr == 0) {
         puts("");
         break;
      }
   }

   return 0;
}

int vfs_print_file_info(const struct Vfs_file *file) {
   if (!file) {
      vfs_state.error = VFS_ERR_INVALID_FILE;
      return -1;
   }

   printf(
      "File info\n"
      "  Name: %s\n"
      "  Type: %s\n"
      "  Data address: %i\n"
      "  Size: %i\n"
      "  Hash: %lu\n",
      file->file_name,
      vfs_file_types[file->type],
      file->addr,
      file->size,
      file->hash
   );

   return 0;
}

struct Vfs_state* vfs_get_state() {
   return &vfs_state;
}

int vfs_dump_disk(const char *path) {
   if (!vfs_state.disk) {
      vfs_state.error = VFS_ERR_INVALID_DISK;
      return -1;
   }

   FILE *file = fopen(path, "w");

   if (file) {
      fwrite(vfs_state.disk, 1, vfs_state.disk_size, file);
      fclose(file);
      return 0;
   }

   printf("Failed to write to file '%s'\n", path);
   return -1;
}

void vfs_get_error() {
   printf("Error (%i) %s\n", vfs_state.error, vfs_errors[vfs_state.error]);
}

void vfs_free() {
   if (vfs_state.disk != NULL) {
      free(vfs_state.disk);
   }
}