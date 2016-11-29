#include "sfs_api.h"
#include "disk_emu.h"
#include <string.h> 
#include <unistd.h>

#define BLOCK_SIZE 1024
#define MAGIC_NUMBER 0xACBD0005
#define NUM_DIR_PTR 12

#define DISK_NAME "Armando_Ordorica_SFS"
#define SB_START_ADDRESS 0 


#define NUM_BLOCKS 
#define FS_SIZE BLOCK_SIZE*NUM_BLOCKS
#define MAX_INODES 100


/*USEFUL FUNCTIONS 
Writes a series of blocks to the disk from the buffer  
int write_blocks(int start_address, int nblocks, void *buffer)
*/
typedef struct inode{ 
  /*The file mode which determines the file type and how the file's owner, its group, and others can access the file.*/
  unsigned int mode; 
  /*A link count telling how many hard links point to the inode.*/ 
  unsigned int linkCount; 
  /*The User ID of the file's owner.*/
  unsigned int uid; 
  /*The Group ID of the file.*/ 
  unsigned int gid; 
  /*The size of the file in bytes.*/ 
  unsigned int size; 
  /*The pointers are called from another struct pointersBlock*/
  /*mode 
  link count
  uid
  gid
  size 
  pointers (up to 12)
  indirect pointer)*/
  block_ptr_t block_ptr; 

} inode_t; 

typedef struct block_ptr {
  /*array containing the direct pointers*/ 
  int direct_ptr[NUM_DIR_PTR]; 
  /*index containing the indirect pointer*/ 
  int indirect_ptr; 
} block_ptr_t; 


typedef struct Directory{ 
}directory; 

typedef struct dir_entry{
  int status; 
  char name[MAXFILENAME]; 
  unsigned int rw_ptr; 
} dir_entry_t;

/////
typedef struct super_block{ 
  unsigned int magic; 
  unsigned int block_size; 
  unsigned int fs_size; 
  unsigned int inode_table_len; 
  unsigned int root_dir_inode; 
} super_block_t; 

char free_blocks[BLOCK_SIZE];
char free_inodes[BLOCK_SIZE];
super_block_t sb; 
inode_t inode_table[MAX_INODES]; 
dir_entry_t root_dir[MAX_INODES-1]; //Need to decrease 1 because the first
//inode is in the directory


void mksfs(int fresh){
  if(fresh){
    /*Create the file system from scratch*/ 
    /*Step 1. Format the virtual disk implemented by the disk emulator */
    printf("Create the file system from scratch\n");
    /*int init_fresh_disk(char *filename, int block_size, int num_blocks)*/
    init_fresh_disk(DISK_NAME, BLOCK_SIZE, NUM_BLOCKS);
    //fill everything with zeros 
    bzero(&sb, sizeof(super_block_t));
    bzero(&inode_table[0], sizeof(inode_t)*MAX_INODES);
    bzero(&root_dir[0], sizeof(dir_entry_t)*(MAX_INODES-1)); 
    bzero(&free_blocks[0], BLOCK_SIZE);
    bzero(&free_inodes[0], BLOCK_SIZE); 


  }


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

// void create_superblock(){
//   unsigned int *buff = malloc(BLOCK_SIZE);

//   if(buff==NULL){
//     return; 
//   }

// buff[0] = MAGIC_NUMBER; 
// buff[1] = BLOCK_SIZE; 
// buff[2] = FS_SIZE; 
// buff[3] = INODE_TABLE; 

// //using write_blocks function as provided by disk_emu.c 


// //deallocating memory previously allocated with malloc 
// /*Writes a series of blocks to the disk from the buffer  
// int write_blocks(int start_address, int nblocks, void *buffer)*/

// write_blocks(SB_START_ADDRESS, 1, &buff); 

// free(buff);

// }
