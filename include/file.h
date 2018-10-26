// file.h

#ifndef FILE_H
#define FILE_H

#define BLOCK_SIZE 32

enum File_types {
   T_FILE = 1,
   T_DIR
};

enum Block_types {
   BLOCK_USED = 1,
   BLOCK_FREE,
   BLOCK_FILE_HEADER,
   BLOCK_FILE_HEADER_FREE
};

struct Vfs_file {
   char block_type;  // BLOCK_FILE_HEADER, BLOCK_FILE_FREE
   char file_name[24];
   unsigned long hash;  // Hashed file name
   char type;  // T_FILE, T_DIR
   unsigned int addr;   // Data block address to the file 
   unsigned int size;
};

struct Data_block {
   char block_type;  // BLOCK_USED, BLOCK_FREE
   unsigned int next_block_addr;
   char block[BLOCK_SIZE];
};

#endif // FILE_H