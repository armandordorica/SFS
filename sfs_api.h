//Functions you should implement. 
//Return -1 for error besides mksfs

void mksfs(int fresh); //good
int sfs_get_next_file_name(char *fname); //good 
int sfs_get_file_size(char* path); //good
int sfs_fopen(char *name); //good
int sfs_fclose(int fileID); //good
int sfs_frseek(int fileID, int loc); //good
int sfs_fwseek(int fileID, int loc); //good
int sfs_fwrite(int fileID, char *buf, int length); //good
int sfs_fread(int fileID, char *buf, int length); //good 
int sfs_remove(char *file); //good 

int name_to_inode_number (char* name);
char* getFileExtension (char* testString); 
int get_free_fd();
int get_free_inode_idx();
int get_free_root_slot();



/*Need to add 

#define MAXFILENAME 21
#define MAXFILENAME 20
#define MAX_EXTENSION 3

*/ 