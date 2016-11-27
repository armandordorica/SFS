#include "sfs_api.h"
#include "disk_emu.h"
#include <string.h> 
#include <unistd.h>
#define BLOCK_SIZE 1024
#define NUM_DIR_PTR 12


typedef struct Inodes{ 
  /*The file mode which determines the file type and how the file's owner, its group, and others can access the file.*/
  int mode; 
  /*A link count telling how many hard links point to the inode.*/ 
  int linkCount; 
  /*The User ID of the file's owner.*/
  int uid; 
  /*The Group ID of the file.*/ 
  int gid; 
  /*The size of the file in bytes.*/ 
  int size; 
  /*The pointers are called from another struct pointersBlock*/
  PointersBlock inode_blockOfOointers; 
  /*mode 
  link count
  uid
  gid
  size 
  pointers (up to 12)
  indirect pointer)*/

}inode; 

typedef struct PointersBlock {
  /*array containing the direct pointers*/ 
  int direct_ptr[NUM_DIR_PTR]; 
  /*index containing the indirect pointer*/ 
  int indirect_ptr; 
}pointersBlock; 


typedef struct Directory{ 
}directory; 

typedef struct SuperBlock{ 
}superBlock; 

void mksfs(int fresh){

}
int sfs_get_next_file_name(char *fname){
  return 0;
}
int sfs_get_file_size(char* path){
  return 0;
}
int sfs_fopen(char *name){
  return 0;
}
int sfs_fclose(int fileID){
  return 0;
}
int sfs_frseek(int fileID, int loc){
  return 0;
}
int sfs_fwseek(int fileID, int loc){
  return 0;
}
int sfs_fwrite(int fileID, char *buf, int length){
  return 0;
}
int sfs_fread(int fileID, char *buf, int length){
  return 0;
}
int sfs_remove(char *file){
  return 0;
}
