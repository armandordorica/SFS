
#include "disk_emu.h"
#include <string.h> 
#include <unistd.h>
#include <stdio.h> 
#include "sfs_api.h"

#define BLOCK_SIZE 1024
#define MAGIC_NUMBER 0xACBD0005
/*The header starts with a so-called magic number,
 identifying the file as an executable file (to prevent the accidental execution 
 of a file not in this format)*/
/*Then come the sizes of the various pieces of the file, the address at which execution starts,
 and some flag bits. */
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

typedef struct block_ptr_t {
  /*array containing the direct pointers*/ 
  int direct_ptr[NUM_DIR_PTR]; 
  /*index containing the indirect pointer*/ 
  int indirect_ptr; 
} block_ptr_t; 

typedef struct inode_t{ 
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

#define SIZE_OF_INODE_STRUCT  sizeof(inode_t)
#define INODE_NUM_BLOCKS MAX_INODES*SIZE_OF_INODE_STRUCT/BLOCK_SIZE + 1
#define ROOT_NUM_BLOCKS (MAX_INODES-1)*sizeof(dir_entry_t)/NUM_BLOCKS + 1
#define ROOTFD 0
#define IND_PTR_SIZE 4



/*USEFUL FUNCTIONS 
Writes a series of blocks to the disk from the buffer  
int write_blocks(int start_address, int num, void *buffer)
*/



/*Each inode entry contains the file attributes, except the name. 
The first I-node points to the block containing the root directory of the file system. 
*/





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

/*The super block contains critical information about the layout 
of the file system, such as the number of inodes and the number of disk blocks. */
typedef struct super_block{ 
  unsigned int magic; 
  unsigned int block_size; 
  unsigned int fs_size; 
  unsigned int inode_table_len; 
  unsigned int root_dir_inode; 
} super_block_t; 

int max(int A, int B);
int get_block_ptr (block_ptr_t* block_pointers, int block_num);
void free_pointers_block(block_ptr_t* pointers);
void rem_file_from_dir (char* filename);

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
    if(fd_table[fileID].status == FREE){
      return -1; 
    }

    /*Change the status of the entry in the fd_table with the fileID passed as argument to FREE*/ 
    fd_table[fileID].status = FREE; 
    fd_table[fileID].inode_idx = -1; //EOF 

  return 0;
}
int sfs_frseek(int fileID, int loc){
  if(fd_table[fileID].status == FREE){
    return -1; 
  }
  if (loc > inode_table[fd_table[fileID].inode_idx].size){
    loc = inode_table[fd_table[fileID].inode_idx].size;
  }

  fd_table[fileID].r_ptr = loc; 
  return 0;
}
int sfs_fwseek(int fileID, int loc){
  if(fd_table[fileID].status == FREE){
    return -1; 
  }
  if (loc > inode_table[fd_table[fileID].inode_idx].size){
    loc = inode_table[fd_table[fileID].inode_idx].size;
  }

  fd_table[fileID].w_ptr = loc; 
  return 0;
}
int sfs_fwrite(int fileID, char *buf, int length){

  inode_t* inode; 
  char block[BLOCK_SIZE]; 
  int offset, w_ptr, block_num, block_ptr, bytes_last_block, last_full_block;
  /*Check in open file table for fileID. If you find it, continue. Else, return -1.
    Check if (offset+ length) is smaller than the block size. This means that it fits, so no need for 
    other blocks. 
    call the read block function and pass it the block index. 
    use mem_cpy to copy from the buffer as your source to the block buffer as your destination and include 
    how many bytes will be copied. 
    mem_cpy has to account for index within the data block, so that you don't write to the wrong address. 
    update the write pointer with the length of the caracter you just wrote. 
    Update the buffer index by subtracting from the block size, the block offset you added. 
    Let n be the number of full blocks between the current and last block.
    for i=0 to n,
      current_block = first_block_index + 1; 
      write_to_block(block_index+1, buff + buff_index); 
      buff_index += 1024; 
  */

      if ((fd_table[fileID].status == FREE) || (fd_table[fileID].w_ptr + length > MAX_FILES)){
        printf("Cannot write to this file \n"); 
        return -1; 
      }

      inode = &inode_table[fd_table[fileID].inode_idx]; 
      w_ptr = fd_table[fileID].w_ptr;
      block_num = w_ptr / BLOCK_SIZE;
      offset = w_ptr % BLOCK_SIZE; 

      block_ptr = get_block_ptr(&(inode->block_ptr), block_num); 

      if ((offset + length) <= BLOCK_SIZE){
        /*The data fits in the current block, no need to go for the next one*/
        

        /*If no blocks were available, return an error*/
        if(block_ptr == -1){ 
          printf("No blocks available\n"); 
          return -1; 
        }else{
          /*Else, write on the part of the current block that is free*/
          read_blocks(block_ptr, 1, block); 
          memcpy(&block[offset], buf, length); 
          write_blocks(block_ptr, 1, block);

          /*Update the size of the file*/
          inode->size = max(w_ptr + length, inode->size);
          fd_table[fileID].w_ptr = w_ptr +length; 

          write_blocks(INODE_T_START_ADDRESS, INODE_NUM_BLOCKS, inode_table); 
          return length; 
        }

        
      }
        bytes_last_block = (offset + length) %BLOCK_SIZE;
        last_full_block = ((offset +  length)/BLOCK_SIZE) + block_num;

        read_blocks(block_ptr, 1, block); 
        memcpy(buf, &block[offset], BLOCK_SIZE - offset);
        write_blocks(block_ptr, 1, block);

        buf += BLOCK_SIZE - offset; 
        block_num++;

        while(block_num<last_full_block){
            block_ptr = get_block_ptr(&(inode->block_ptr), block_num);
            if (block_ptr == -1){
              printf("Not able to retrieve block pointer. Error. \n");
            }
            memcpy(block, buf, bytes_last_block); 
            write_blocks(INODE_T_START_ADDRESS, INODE_NUM_BLOCKS, inode_table);       
            
            buf += BLOCK_SIZE; 
            block_num++; 
        }

        /*Writing last partial block*/
        block_ptr = get_block_ptr(&(inode->block_ptr), block_num); 
        if(block_ptr == -1){ 
          return -1;
        }
        read_blocks(block_ptr, 1, block); 
        memcpy(block, buf, bytes_last_block);
        write_blocks(block_ptr, 1, block); 

        /*Updating file size, write pointer, and inode table with new block pointers*/
        inode->size = max(w_ptr + length, inode->size); 
        fd_table[fileID].w_ptr = w_ptr + length; 
        /*Update inode table with new block pointers*/
        write_blocks(INODE_T_START_ADDRESS, INODE_NUM_BLOCKS, inode_table);




  return length;
}


int sfs_fread(int fileID, char *buf, int length){
 /*Same variable declaration as fwrite*/ 
  inode_t* inode; 
  char block[BLOCK_SIZE]; 
  int offset, w_ptr, block_num, block_ptr, bytes_last_block, last_full_block;

  /*If file is not open yet*/
  if(fd_table[fileID].status == FREE){
    return -1; 
  }

  /*Retrieve corresponding inode from fd table*/
  inode = &inode_table[fd_table[fileID].inode_idx];

  return 0;
}



int sfs_remove(char *file){
  int inode_number; 
  inode_number = name_to_inode_number(file); 

  if(inode_number == -1){
    printf("It is not possible to remove this file\n"); 
    return -1; 
  }

  free_pointers_block(&(inode_table[inode_number].block_ptr));

  /*Free inode*/
  free_inodes[inode_number] = FREE; 
  write_blocks(INODE_MAP_START_ADDRESS, 1, free_inodes); 

  rem_file_from_dir(file); 

  /*Clearing the inode table*/
  memset(&inode_table[inode_number], 0, sizeof(inode_t)); 
  write_blocks(INODE_T_START_ADDRESS, INODE_NUM_BLOCKS, inode_table); 

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

int max(int A, int B){
  if(A>B){
    return A;
  }
  if(B>A){
    return B;
  }
  return 0; 
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

int get_block_ptr (block_ptr_t* block_pointers, int block_num){

  int ind_ptr_array[BLOCK_SIZE/IND_PTR_SIZE];
  /*Getting a direct pointer */
  if (block_num < NUM_DIR_PTR){
    if(block_pointers->direct_ptr[block_num] == 0){
      if ((get_free_block_idx()) == 0)
        return -1; 
    } else {
      return block_pointers->direct_ptr[block_num];
    }
  }

  if(block_pointers->indirect_ptr == 0){
    if(get_free_block_idx() == 0){
      printf("Indirect pointer has not been initialized\n"); 
      return -1; 
    }

    memset(ind_ptr_array, 0, BLOCK_SIZE); 
    write_blocks(block_pointers->indirect_ptr, 1, (void*)ind_ptr_array);
  }

  read_blocks(block_pointers->indirect_ptr, 1, (void*)ind_ptr_array);
  if(ind_ptr_array[block_num - NUM_DIR_PTR] == 0){
    if( get_free_block_idx()==0){
        return -1; 
    }
    write_blocks(block_pointers->indirect_ptr, 1, (void*)ind_ptr_array);
  }


  return ind_ptr_array[block_num - NUM_DIR_PTR]; 
}

void rem_file_from_dir (char* filename){

  
  int directory_size = inode_table[fd_table[ROOTFD].inode_idx].size;

  int i;
  for(i = 0; i<= MAX_INODES; i++){
    if(strcmp(root_dir[i].name, filename) == 0){

      //iterate until the file is found, then shift array  
      /*#include <string.h>

       void *memmove(void *dest, const void *src, size_t n);
      The  memmove()  function  copies n bytes from memory area src to memory
       area dest.  The memory areas may overlap: copying takes place as though
       the  bytes in src are first copied into a temporary array that does not
       overlap src or dest, and the bytes are then copied from  the  temporary
       array to dest.
      */

       memmove(&root_dir[i], &root_dir[i+1], sizeof(dir_entry_t)*(MAX_INODES-i-INODE_T_START_ADDRESS));

       dir_entry_t temp; 
       temp.status = FREE; 
       memcpy(&root_dir[MAX_INODES-INODE_T_START_ADDRESS], &temp, sizeof(dir_entry_t));

       //Take the shift it cache and write it to memory
       write_blocks(ROOT_START_ADDRESS, ROOT_NUM_BLOCKS, root_dir);

      /*reseting file descript table pointers*/
       fd_table[ROOTFD].w_ptr = 0; 
       fd_table[ROOTFD].r_ptr = 0; 

       inode_table[fd_table[ROOTFD].inode_idx].size = 0;

       /*Removing old blocks*/
       free_pointers_block(&inode_table[fd_table[ROOTFD].inode_idx].block_ptr);

       memset(&inode_table[fd_table[ROOTFD].inode_idx], 0, sizeof(inode_t));
       sfs_fwrite(ROOTFD, (char*)root_dir, directory_size- sizeof(dir_entry_t));

       /*Updating inode table after file has been removed*/ 
       write_blocks(INODE_T_START_ADDRESS, INODE_NUM_BLOCKS, inode_table);
       return; 
    }

  }
}

void free_pointers_block(block_ptr_t* pointers){
  int indirect_pointers[BLOCK_SIZE/IND_PTR_SIZE]; 
  int index, block_num; 

  for(block_num = 0; block_num <NUM_DIR_PTR; block_num++){
    index = pointers->direct_ptr[block_num]; 
    if(index == 0){
      write_blocks(BLOCK_MAP_START_ADDRESS, 1, free_blocks);
      return;  
    }

    index -= BLOCK_MAP_START_ADDRESS -1; 
    free_blocks[index] = FREE; 
  }

  read_blocks(pointers->indirect_ptr, 1, (void*)indirect_pointers); 
  for (block_num = 0; block_num < (BLOCK_SIZE/IND_PTR_SIZE); block_num++){
    index = indirect_pointers[block_num]; 
    if(index == 0){
      write_blocks(BLOCK_MAP_START_ADDRESS, 1, free_blocks); 
      return; 
    }
    index -= BLOCK_MAP_START_ADDRESS - 1; 
    free_blocks[index] = FREE; 

  }


}
int get_free_block_idx()
{

  int i; 
  for (i=0; i<NUM_BLOCKS; i++){
    if(free_blocks[i] == FREE){
      /*If free take it, and change its status*/
      free_blocks[i] = USED; 
      write_blocks(BLOCK_MAP_START_ADDRESS, 1, free_blocks);
      return BLOCK_MAP_START_ADDRESS + i + 1; 
    }
  }
  printf("No more free data blocks available.\n");
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
