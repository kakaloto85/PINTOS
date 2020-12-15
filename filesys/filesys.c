#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
// #include "filesys/buffer.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();
  // printf("hi 1 \n");
  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  buffer_flush_all();
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{
  block_sector_t inode_sector = 0;
  // printf('filesys_create name %s size %d\n',name,initial_size);
  //dir_open_root가 아니라, 원하는 directory에 create하도록 만들어야됨!
  // 그러면 항상 0번째 sector index에 생성하는건가?
      char file_name[strlen(name)+1];
      // printf("name = %s\n",name);
    struct dir *dir = parse_path(name, file_name,1);
      // printf("name = %s\n",file_name);

  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size,0)
                  && dir_add (dir, file_name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);
  // printf("filesys_create success\n");
  return success;
}
bool
filesys_create_dir (const char *name) {
    // char directory[strlen(name)];
      block_sector_t inode_sector = 0;
    

    char file_name[strlen(name)+1];
    struct dir *dir = parse_path(name, file_name,1);
    bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && dir_create (inode_sector, 0)
                  && dir_add (dir, file_name, inode_sector));
    if (!success && inode_sector != 0) 
        free_map_release (inode_sector, 1);
    if(dir!=NULL)
      dir_close (dir);
    
    return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  //dir_open_root가 아니라, 원하는 directory에서 open하도록 만들어야됨!
      char file_name[strlen(name)+1];
    struct dir *dir = parse_path(name, file_name,0);
  // printf("filename : %s\n", file_name);
  struct inode *inode = NULL;
  if(!strcmp(file_name,"rootdir")){
    return file_open (inode_open (ROOT_DIR_SECTOR));
  }
  if (dir != NULL){
    //dir에서 name에 해당하는 File의 inode를 리턴함
    dir_lookup (dir, file_name, &inode);
  }
  
  dir_close (dir);
  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir); 

  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  thread_current()->dir_now = dir_open_root();
  free_map_close ();
  printf ("done.\n");
}
