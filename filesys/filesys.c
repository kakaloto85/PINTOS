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
  // thread_current()->dir_now = dir_open_root();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  buffer_flush_all();
  // flush_all();
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
  // else
  //   write_back(dir_get_inode(dir));

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
    // struct dir *prev_dir;
    struct dir *cur_dir;
    // printf("file_name : %s\n",file_name);
    // printf("dir returned /n");
    bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && dir_create (inode_sector, 0)
                  && dir_add (dir, file_name, inode_sector));
    // printf("inode_get_inumber:%d\n",inode_get_inumber(dir_get_inode(dir)));
    if (success) {
      // printf("here\n");
      cur_dir=dir_open(inode_open(inode_sector));
      dir_add (cur_dir, "..", inode_get_inumber(dir_get_inode(dir)));
      dir_add (cur_dir, ".", inode_sector);
      // write_back(dir_get_inode(cur_dir));
      dir_close (cur_dir); 
      // if(thread_current()->dir_now!=NULL){
      //   if(inode_get_inumber(dir_get_inode(thread_current()->dir_now))==inode_get_inumber(dir_get_inode(dir))){
      //             printf("here\n");

      //     thread_current()->dir_now=dir;
      //   }
      // }
    }

    if (!success && inode_sector != 0) 
        free_map_release (inode_sector, 1);
    if(dir!=NULL)
      dir_close (dir); 

    // if (!success)
    //   printf("create_dir success \n");
    return success;
}
// bool
// filesys_create_dir (const char *name) {
//     // char directory[strlen(name)];
//       block_sector_t inode_sector = 0;
    

//     char file_name[strlen(name)+1];
//     struct dir *dir = parse_path(name, file_name,1);
//     bool success = (dir != NULL
//                   && free_map_allocate (1, &inode_sector)
//                   && dir_create (inode_sector, 0)
//                   && dir_add (dir, file_name, inode_sector));
//     if (!success && inode_sector != 0) 
//         free_map_release (inode_sector, 1);
//     // else
//     //   write_back(dir_get_inode(dir));

//     if(dir!=NULL)
//       dir_close (dir);
    
//     return success;
// }

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  // printf("open\n");
  //dir_open_root가 아니라, 원하는 directory에서 open하도록 만들어야됨!
      char file_name[strlen(name)+1];
    struct dir *dir = parse_path(name, file_name,0);
    // printf("filesys_open dir: sector_number: %d \n", inode_get_inumber(dir_get_inode(dir)));
  // printf("filename : %s\n", file_name);
  // memcpy(name,file_name,strlen(name)+1);
  struct inode *inode = NULL;
  if(!strcmp(file_name,"rootdir")){
    return file_open (inode_open (ROOT_DIR_SECTOR));
  }
  if (dir != NULL){
    //dir에서 name에 해당하는 File의 inode를 리턴함
    dir_lookup (dir, file_name, &inode);
  }
  
  dir_close (dir);
  if(strlen(file_name)==0||strlen(file_name)>14){
    return NULL;
  }
  // 비어있는 것ㅇ을 열려하면 문제가 생기게
  // if (inode== NULL)
  //   return NULL;  // 추가했는데 고쳐야할듯..
  // else
  // struct file * file =file_open(inode);
  // if (file == NULL)
  //   exit(-1);
  // else
  //   return file; => file_open에 구현이 되어있다... 근데 왜 open 이 성공하는 것으로 나오지?
    return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  char file_name[strlen(name)+1];
  // struct dir *dir = dir_open_root ();
  struct dir* dir = parse_path(name, file_name,0);
  // printf("filesys_remove:%s\n",file_name);
  // struct inode *inode = NULL;
  // if(dir_lookup(dir, name, inode) && is_dir(inode))
  //   dir_readdir(i)
  bool success = dir != NULL && dir_remove (dir, file_name);
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
  // thread_current()->dir_now = dir_open_root();
  dir_add(dir_open_root(), "..", NULL);  // 이거는 NULL로 해야할수도
  // printf("before adding \n");
  dir_add(dir_open_root(),".", ROOT_DIR_SECTOR);
  // dir_close(dir_open_root());
  free_map_close ();
  printf ("done.\n");
}
// static void
// do_format (void)
// {
//   printf ("Formatting file system...");
//   free_map_create ();
//   if (!dir_create (ROOT_DIR_SECTOR, 16))
//     PANIC ("root directory creation failed");
//   thread_current()->dir_now = dir_open_root();
//   free_map_close ();
//   printf ("done.\n");
// }
