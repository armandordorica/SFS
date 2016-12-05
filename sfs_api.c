#include "sfs_api.h"
#include "disk_emu.h"
#include <string.h> 
#include <unistd.h>
#include <stdio.h> 

#define BLOCK_SIZE 1024
#define MAGIC_NUMBER 0xACBD0005
#define NUM_DIR_PTR 12

#define DISK_NAME "Armando_Ordorica_SFS"
#define SB_START_ADDRESS 0 
#define ROOT_INODE_START_ADDRESS 0 

#define USED 1 
#define FREE 0 //change to availale or empty 
#define NUM_BLOCKS 1024
#define FS_SIZE BLOCK_SIZE*NUM_BLOCKS
#define MAX_INODES 150
#define MAX_FILES 16
#define NUM_DIRPTR 12
#define MAX_FILE_NAME_SIZE BLOCK_SIZE*NUM_DIR_PTR
#define INODE_T_START_ADDRESS 2 
#define ROOT_START_ADDRESS 24 
#define INODE_MAP_START_ADDRESS 34
#define BLOCK_MAP_START_ADDRESS 35
#define INODE_NUM_BLOCKS (MAX_INODES*sizeof(inode_t)/BLOCK_SIZE) + 1
#define ROOT_NUM_BLOCKS (MAX_INODES-1)*sizeof(dir_entry_t)/NUM_BLOCKS + 1
#define ROOTFD 0


/*USEFUL FUNCTIONS 
Writes a series of blocks to the disk from the buffer  
int write_blocks(int start_address, int num, void *buffer)
*/

typedef struct block_ptr {
  /*array containing the direct pointers*/ 
  int direct_ptr[NUM_DIR_PTR]; 
  /*index containing the indirect pointer*/ 
  int indirect_ptr; 
} block_ptr_t; 


typedef struct inode{ 
  /*The file mode which determines the file type and how the file's owner, its group, and others can access the file.*/
  unsigned int mode; //change this to var name permissions 
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




typedef struct Directory{ 
}directory; 

typedef struct dir_entry{
  int status; 
  char name[MAX_FILE_NAME_SIZE]; 
  unsigned int inode_idx; 
} dir_entry_t;


typedef struct fd_table_t { 
  char status;
  unsigned int inode_idx; 
  unsigned int r_ptr; 
  unsigned int w_ptr; 
} fd_table_t;
/////

fd_table_t fd_table[MAX_FILES];
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
void clear_fd_table(){ 
  for(int i = 0; i< MAX_FILES; i++)
      fd_table[i].status = FREE;
}

void mksfs(int fresh){

     /*Step 1. Format the virtual disk implemented by the disk emulator */ 
     clear_fd_table();

  if(fresh){
    /*Create the file system from scratch*/ 

    printf("Create the file system from scratch\n");
    /*int init_fresh_disk(char *filename, int block_size, int num_blocks)*/
    init_fresh_disk(DISK_NAME, BLOCK_SIZE, NUM_BLOCKS);
    //fill everything with zeros 
    printf("size of super_block_t is: %lu\n", sizeof(super_block_t));

    bzero(&sb, sizeof(super_block_t));

    printf("size of inode_t is: %lu\n", sizeof(inode_t));
    bzero(&inode_table[0], sizeof(inode_t)*MAX_INODES);

    printf("sizeof(dir_entry_t) is: %lu\n",  sizeof(dir_entry_t));
    bzero(&root_dir[0], sizeof(dir_entry_t)*(MAX_INODES-1)); 
    bzero(&free_blocks[0], BLOCK_SIZE);
    bzero(&free_inodes[0], BLOCK_SIZE); 

    printf("sizeof(fd_table_t) is: %lu\n",  sizeof(fd_table_t));
    bzero(&fd_table[0], sizeof(fd_table_t)*MAX_FILES);

    

    //allocate the first block to the superblock
    printf("Initializing the superblock\n");

    sb.magic = MAGIC_NUMBER; 
    sb.block_size = BLOCK_SIZE; 
    sb.fs_size = NUM_BLOCKS*BLOCK_SIZE; 
    sb.inode_table_len = MAX_INODES; 
    sb.root_dir_inode = ROOT_INODE_START_ADDRESS; 
    /*/*Writes a series of blocks to the disk from the buffer          
int write_blocks(int start_address, int nblocks, void *buffer)*/
    write_blocks(SB_START_ADDRESS, 1, &sb);

    //allocate the second block to the inode table
    printf("Initializing inode table to second block\n");
    inode_table[0].mode = 0x755; 
    inode_table[0].linkCount = 0; 
    inode_table[0].uid = 0; 
    inode_table[0].gid = 0; 
    inode_table[0].size = 0;
    write_blocks(INODE_T_START_ADDRESS, INODE_NUM_BLOCKS, inode_table); 

    printf("Initializing root directlsory\n"); //21st block 
    int i; 
    //initializing the root directory cache 
    for (i = 0; i< MAX_INODES-1; i++){
      root_dir[i].status = FREE; 
    }

    write_blocks(ROOT_START_ADDRESS, ROOT_NUM_BLOCKS, root_dir);


    printf("Initializing free inodes\n"); //30th block 

        /*   memset - fill memory with a constant byte\
       #include <string.h>
       void *memset(void *s, int c, size_t n);
         The  memset()  function  fills  the  first  n  bytes of the memory area
       pointed to by s with the constant byte c.
       */
    memset(free_inodes, FREE, BLOCK_SIZE);
    free_inodes[0] = USED; 
    write_blocks(INODE_MAP_START_ADDRESS, 1, inode_table);

    printf("Initializing free blocks\n"); //31st block
    memset(free_blocks, FREE, BLOCK_SIZE); 
    write_blocks(BLOCK_MAP_START_ADDRESS, 1, free_blocks);

    //Initializing file descriptor table
    fd_table[ROOTFD].status = USED; 
    fd_table[ROOTFD].inode_idx = ROOT_INODE_START_ADDRESS;
    fd_table[ROOTFD].r_ptr = 0; 
    fd_table[ROOTFD].w_ptr = 0; 
  } else{
    /*load from directory*/ 
    clear_fd_table(); 
    /*Initializes an existing disk*/
    // int init_disk(char *filename, int block_size, int num_blocks)

    init_disk(DISK_NAME, BLOCK_SIZE, NUM_BLOCKS);

    /*reading blocks
    Reads a series of blocks from the disk into the buffer
    int read_blocks(int start_address, int nblocks, void *buffer)*/
    read_blocks(INODE_MAP_START_ADDRESS, 1, (void*)free_inodes);
    read_blocks(BLOCK_MAP_START_ADDRESS, 1, (void*)free_blocks);
    read_blocks(ROOT_START_ADDRESS, ROOT_NUM_BLOCKS, (void*)root_dir);
    read_blocks(INODE_T_START_ADDRESS, INODE_NUM_BLOCKS, (void*)inode_table);


      //Initializing file descriptor table
    fd_table[ROOTFD].status = USED; 
    fd_table[ROOTFD].inode_idx = ROOT_INODE_START_ADDRESS;
    fd_table[ROOTFD].w_ptr = inode_table[fd_table[ROOTFD].inode_idx].size;
    fd_table[ROOTFD].r_ptr = inode_table[fd_table[ROOTFD].inode_idx].size;

    return;

  }
}


int sfs_get_next_file_name(char *fname){
  printf("inside sfs_get_next_file_name function\n");
  int num_files = 0; 
  static int j = 0; 

  //Finding out how many files there are by looping through the inodes in the 
  //loop directory that contain a USED flag 
  for (int i = 0; i <= MAX_INODES; i++){
    if(root_dir[j].status == USED){
      num_files++; 
    }
  }

  while(j<num_files){
    //storing in the name in the root directory (directory entry struct)
    strcpy(fname, root_dir[j].name); 
    j++; 
    return 1; 
  }

  j=0; 
  return 0;
}
int sfs_get_file_size(char* path){
  /*Returns the size of a given file*/

  /*1. Pass the name of the file to name_to_inode_number function
    2. With the inode number, look it up into the inode table
    3. get its size 
    4. return this size*/
    int size;
    int inode_idx = name_to_inode_number(path); 
    if (inode_idx ==-1){
      return -1; //returns an error if the filename passed as argument does not find any matches
    }

    size = inode_table[inode_idx].size; 
    return size;
}
int sfs_fopen(char *name){
  /**Opens a file and returns an integer that corresponds to the index of the entry for the newly 
  opened file in the descriptor table. 
  If the file does not exist, it creates a new file and sets its size to 0. 
  If the file does exist, the file is opened in append mode (i.e. the write file pointer is set to the 
  end of the file and the read file pointer is set at the beginning of the file.*/
  int inode_idx, fd, i; 
  inode_t* inode; 
  dir_entry_t new_file; 


  //validation of input argument, i.e. name of file you're trying to open
  /*The file you're trying to open must: 
    1. Have only one dot in the naming (assuming that you can only use it to communicate file extension
    2. The name of the file is not too long 
    */
       if(getFileExtension(name) == -1){
        //If the filename is too long, get out.
        return -1; 
       } 
       getFileExtension(name); 

      fd = get_free_fd();
       /*get_free_fd() returns -1 if doesn't find a free fd in the file descriptor table. */
      if(fd == -1) {
        printf("No free file descriptor available in the file descriptor table. \n");
        return -1;
      }

       inode_idx = name_to_inode_number(name);
       if(inode_idx != -1){
        printf("File \"%s\" does not exist.\n", name); 
        printf("Creatinf file...\n");

        printf("Allocating inode...\n");
        inode_idx = get_free_inode_idx();
        inode = &inode_table[inode_idx];

        printf("Adding new file to directory...\n");
        new_file.status = USED;
        new_file.inode_idx = inode_idx;
        strcpy(new_file.name, name);

        printf("Getting a free slot in the cache...\n"); 
        int root_dir_idx = get_free_root_slot();
        root_dir[root_dir_idx] = new_file;

        printf("Updating root directoy\n"); 
        write_blocks(ROOT_START_ADDRESS, ROOT_NUM_BLOCKS, root_dir);
        fd_table[ROOTFD].r_ptr = inode_table[fd_table[ROOTFD].inode_idx].size;
        fd_table[ROOTFD].w_ptr = inode_table[fd_table[ROOTFD].inode_idx].size;

        sfs_fwrite(ROOTFD, (char*)&new_file, sizeof(dir_entry_t));



       }else {
        printf("File \"%s\" already exists. Loading from directory...\n", name);
        inode = &inode_table[inode_idx];fd_table[ROOTFD].r_ptr = inode_table[fd_table[ROOTFD].inode_idx].size;
        inode = &inode_table[inode_idx];fd_table[ROOTFD].w_ptr = inode_table[fd_table[ROOTFD].inode_idx].size;
        sfs_fwrite(ROOTFD, (char*)&new_file, sizeof(dir_entry_t));
        /*If the file does exist, the file is opened in append mode (i.e. the write file pointer is set to the 
  end of the file and the read file pointer is set at the beginning of the file.*/
       }

       fd_table[fd].inode_idx = inode_idx;
       fd_table[fd].r_ptr = inode->size;
       fd_table[fd].w_ptr = inode->size;

     



       

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

int name_to_inode_number (char* name){
  int i, inode_idx; 

  for(i = 0; i<= MAX_INODES; i++){
    if(strcmp(root_dir[i].name, name) == 0){
    inode_idx = root_dir[i].inode_idx; 
      return inode_idx; 
    }
  }

  return -1; 


}

char* getFileExtension (char* testString){

  printf("Test string is %s\n", testString);
  size_t sizeOfString = strlen(testString);
  char* fileType;
  fileType = strchr(testString, '.');

  //returns an error if the filename is too long 
  if(sizeOfString>MAX_FILE_NAME_SIZE){
    printf("Invalid size for file name. Try a shorter file name. \n"); 
    return -1; 
  } 
  
  printf("File extension is: %s\n", fileType);

  return fileType;

      /* char *strchr(const char *s, int c);
    strchr() function returns a pointer to the first occurrence of the character c in the string s

    char *strchr(const char *s, int c);
    The  strlen() function calculates the length of the string s, excluding
       the terminating null byte ('\0').
    The  strlen() function calculates the length of the string s, excluding
       the terminating null byte ('\0').

    */

}

int get_free_inode_idx()
{
    int i;
    for(i = 1; i < MAX_INODES; i++) {
        if(free_inodes[i] == FREE) {
            free_inodes[i] = USED;
            write_blocks(INODE_MAP_START_ADDRESS, 1, free_inodes);
            return i;
        }
    }
    return -1;
}

int get_free_root_slot() {
    int i;
    
    for(i = 0; i < MAX_INODES - 1; i++) {
        if(root_dir[i].status == FREE) {
            return i;
        }
    }
    return -1;
} 

int get_free_fd()
{
    int i;
    
    for(i = 1; i < MAX_FILES; i++) {
        if(fd_table[i].status == FREE) {
            fd_table[i].status = USED;
            return i;
        }
    }
    
    return -1;
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
