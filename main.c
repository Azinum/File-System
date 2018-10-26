// main.c - (virtual/Fake) File System
// File system running on RAM

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vfs.h"

int main(int argc, char** argv) {
   if (vfs_init(4096) != 0) {
      vfs_get_error();
   }
   
   struct Vfs_file *file = vfs_create_file("test.txt");

   const char *to_write = "QWERTYUIOPASDFGHJKLZXCVBNM,.qwertyuiopasdfghjklzxcvbnm,.1234567890-!#€%&/()=?";

   if (vfs_write(file, to_write, strlen(to_write)) != 0) {
      vfs_get_error();
   }

   {
      const char *to_write = "0987654321.0987654321..0987654321...0987654321....0987654321.....";
      vfs_append(file, to_write, strlen(to_write));
   }

   vfs_read(file);

   vfs_print_file_info(file);

   vfs_dump_disk("disks/tmp.disk");
   vfs_free();
   return 0;
}
