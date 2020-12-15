#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
/* A directory. */
struct dir 
  {
    struct inode *inode;                /* Backing store. */
    off_t pos;                          /* Current position. */
  };

/* A single directory entry. */
struct dir_entry 
  {
    block_sector_t inode_sector;        /* Sector number of header. */
    char name[NAME_MAX + 1];            /* Null terminated file name. */
    bool in_use;                        /* In use or free? */
  };

/* Creates a directory with space for ENTRY_CNT entries in the
   given SECTOR.  Returns true if successful, false on failure. */
bool
dir_create (block_sector_t sector, size_t entry_cnt)
{
  return inode_create (sector, entry_cnt * sizeof (struct dir_entry),1);
}

/* Opens and returns the directory for the given INODE, of which
   it takes ownership.  Returns a null pointer on failure. */
struct dir *
dir_open (struct inode *inode) 
{
  struct dir *dir = calloc (1, sizeof *dir);
  if (inode != NULL && dir != NULL)
    {
      dir->inode = inode;
      dir->pos = 0;
      return dir;
    }
  else
    {
      inode_close (inode);
      free (dir);
      return NULL; 
    }
}

/* Opens the root directory and returns a directory for it.
   Return true if successful, false on failure. */
struct dir *
dir_open_root (void)
{
  // printf("open root\n");
  return dir_open (inode_open (ROOT_DIR_SECTOR));
}

/* Opens and returns a new directory for the same inode as DIR.
   Returns a null pointer on failure. */
struct dir *
dir_reopen (struct dir *dir) 
{
  return dir_open (inode_reopen (dir->inode));
}

/* Destroys DIR and frees associated resources. */
void
dir_close (struct dir *dir) 
{
  if (dir != NULL)
    {
      inode_close (dir->inode);
      free (dir);
    }
  // printf("dir_close success\n");
}

/* Returns the inode encapsulated by DIR. */
struct inode *
dir_get_inode (struct dir *dir) 
{
  return dir->inode;
}

/* Searches DIR for a file with the given NAME.
   If successful, returns true, sets *EP to the directory entry
   if EP is non-null, and sets *OFSP to the byte offset of the
   directory entry if OFSP is non-null.
   otherwise, returns false and ignores EP and OFSP. */
static bool
lookup (const struct dir *dir, const char *name,
        struct dir_entry *ep, off_t *ofsp) 
{
  struct dir_entry e;
  size_t ofs;
  
  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
       {
    if (e.in_use && !strcmp (name, e.name)) 
      {
        if (ep != NULL)
          *ep = e;
        if (ofsp != NULL)
          *ofsp = ofs;
        return true;
      }
       }
  // printf("name = %s\n",name);
  return false;
}

/* Searches DIR for a file with the given NAME
   and returns true if one exists, false otherwise.
   On success, sets *INODE to an inode for the file, otherwise to
   a null pointer.  The caller must close *INODE. */
bool
dir_lookup (const struct dir *dir, const char *name,
            struct inode **inode) 
{
  struct dir_entry e;
  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  if (lookup (dir, name, &e, NULL))
    *inode = inode_open (e.inode_sector);
  else{
    *inode = NULL;
  }
  return *inode != NULL;
}

/* Adds a file named NAME to DIR, which must not already contain a
   file by that name.  The file's inode is in sector
   INODE_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or a disk or memory
   error occurs. */
bool
dir_add (struct dir *dir, const char *name, block_sector_t inode_sector)
{
  struct dir_entry e;
  off_t ofs;
  bool success = false;
  ASSERT (dir != NULL);
  ASSERT (name != NULL);
  /* Check NAME for validity. */
  if (*name == '\0' || strlen (name) > NAME_MAX)
    return false;

  /* Check that NAME is not in use. */
  if (lookup (dir, name, NULL, NULL))
    goto done;

  /* Set OFS to offset of free slot.
     If there are no free slots, then it will be set to the
     current end-of-file.
     
     inode_read_at() will only return a short read at end of file.
     Otherwise, we'd need to verify that we didn't get a short
     read due to something intermittent such as low memory. */
  for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
       ofs += sizeof e) 
    if (!e.in_use)
      break;

  /* Write slot. */
  // printf("before writing");
  e.in_use = true;
  strlcpy (e.name, name, sizeof e.name);
  e.inode_sector = inode_sector;
  success = inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
 done:
  // printf("dir_add success \n");
  return success;
}

/* Removes any entry for NAME in DIR.
   Returns true if successful, false on failure,
   which occurs only if there is no file with the given NAME. */
bool
dir_remove (struct dir *dir, const char *name) 
{
  struct dir_entry e;
  struct inode *inode = NULL;
  bool success = false;
  off_t ofs;

  ASSERT (dir != NULL);
  ASSERT (name != NULL);

  /* Find directory entry. */
  if (!lookup (dir, name, &e, &ofs))
    goto done;

  /* Open inode. */
  inode = inode_open (e.inode_sector);

  if (inode == NULL)
    goto done;

  /* Erase directory entry. */
  e.in_use = false;
  if (inode_write_at (dir->inode, &e, sizeof e, ofs) != sizeof e) 
    goto done;

  /* Remove inode. */
  inode_remove (inode);
  success = true;

 done:
  inode_close (inode);
  return success;
}

/* Reads the next directory entry in DIR and stores the name in
   NAME.  Returns true if successful, false if the directory
   contains no more entries. */
bool
dir_readdir (struct dir *dir, char name[NAME_MAX + 1])
{
  struct dir_entry e;
  while (inode_read_at (dir->inode, &e, sizeof e, dir->pos) == sizeof e) 
    {
      dir->pos += sizeof e;
      if (e.in_use)
        {
          strlcpy (name, e.name, NAME_MAX + 1);
          return true;
        } 
    }
  return false;
}
// struct dir* 
// parse_path (char *path_name, char *file_name,bool create) {
//     // printf("parse_path : %s\n",path_name);
//     struct dir *dir;
//     char *token,*savePtr,*next;
//     char path[strlen(path_name)+1];
//     bool check =false;
//     memcpy(path,path_name,strlen(path_name)+1);
//     if (path == NULL || file_name == NULL || strlen(path) == 0){
//       return NULL;
//     }
//     printf("file_name at start is %s \n", path_name);
//     token = strtok_r (path, "/", &savePtr);
//     printf("parse_path_1 \n");
//     printf("name is %s \n", token);
//     if (path[0] == '/'){
//       dir = dir_open_root();
//       token=strtok_r (path, "/", &savePtr); 
//       check=true;
//     }
//     else if (thread_current()->dir_now==NULL){
//       // printf("here\n");
//         dir = dir_open_root();
//     }
//     else
//       dir = thread_current()->dir_now;

//     while (token!=NULL) {
//       // printf("token = %s\n",token);
//       struct inode* inode=NULL;
//       next = strtok_r (NULL, "/", &savePtr);
//       printf("token = %s\n",next);

//       if(next==NULL){
//         if(dir_lookup(dir, token, &inode)){
//           if(!create){
//             dir_close(dir);
//             return NULL;
//           }
//           else{
//               memcpy(file_name, token, strlen(token) + 1);
//               return dir;
//           }
//         }
//         else{
//           memcpy(file_name, token, strlen(token) + 1);
//           printf("file_name = %s\n",file_name);
//           return dir;
//         }
//       }
//       else{
//         if(!dir_lookup(dir, token, &inode)){
//           if(!is_dir(inode)){
//             dir_close(dir);
//             return NULL;     
//           }
//         }
//       }
//       dir_close(dir);
//       dir = dir_open(inode);
//       token=next; 
//     }

//     // printf("here\n");
//     memcpy(file_name,"rootdir", strlen("rootdir") + 1);
//     // printf("file_name = %s\n",file_name);
//     return dir;
// }
struct dir* 
parse_path (char *path_name, char *file_name,bool create) {
    // printf("parse_path : %s\n",path_name);
    struct dir *dir;
    char *token,*savePtr,*next;
    char path[strlen(path_name)+1];
    bool check =false;
    memcpy(path,path_name,strlen(path_name)+1);
    if (path == NULL || strlen(path) == 0){
      return NULL;
    }


    if (path[0] == '/'){
      dir = dir_open_root();
      token=strtok_r (path, "/", &savePtr); 
      check=true;
    }
    if (thread_current()->dir_now==NULL){
      // printf("here\n");
        dir = dir_open_root();
    }

    if(!strncmp(path,"..",2)){
      dir=thread_current()->dir_now;
      token=strtok_r (path, "/", &savePtr); 
      check=true;
    }
    else if(!strncmp(path,".",1)){
      dir=thread_current()->dir_now;
      token=strtok_r (path, "/", &savePtr); 
      check=true;
    }
    else{
      dir=dir_reopen(thread_current()->dir_now);
    }

    if(!check)
      token = strtok_r (path, "/", &savePtr);

    while (token!=NULL) {
      // printf("token = %s\n",token);
      struct inode* inode=NULL;
      next = strtok_r (NULL, "/", &savePtr);
      // // printf("token = %s\n",next);

      if(next==NULL){
        if(dir_lookup(dir, token, &inode)){
          if(create){
            dir_close(dir);
            return NULL;
          }
          else{
              memcpy(file_name, token, strlen(token) + 1);
              return dir;
          }
        }
        else{
          memcpy(file_name, token, strlen(token) + 1);
          // printf("file_name = %s\n",file_name);
          return dir;
        }
      }
      else{
        if(!dir_lookup(dir, token, &inode)){
          if(!is_dir(inode)){
            dir_close(dir);
            return NULL;     
          }
        }
      }
      dir_close(dir);
      dir = dir_open(inode);
      token=next; 

    }
    // printf("here\n");
    memcpy(file_name,"rootdir", strlen("rootdir") + 1);
    // printf("file_name = %s\n",file_name);
    return dir;
}
struct dir*
find_dir(char* path_name){
    struct dir *dir;
    char *token,*savePtr,*next;
    char path[strlen(path_name)+1];
    memcpy(path,path_name,strlen(path_name)+1);
    // if (path == NULL || strlen(path) == 0){
    //   return NULL;
    // }
    if (path[0] == '/'||thread_current()->dir_now==NULL){
        dir = dir_open_root();
        // strtok_r (path_name, "/", &savePtr); 
        // path_name = savePtr;
    }
    else{
      dir=dir_reopen(thread_current()->dir_now);
    }
    token = strtok_r (path, "/", &savePtr); 

    if(!strcmp(token,".")){
      dir=thread_current()->dir_now;
      token = strtok_r (NULL, "/", &savePtr);
    }
    else if(!strcmp(token,"..")){
      //이부분 고쳐야댐
      dir=thread_current()->dir_now;
      token = strtok_r (NULL, "/", &savePtr);
    }
    while (token!=NULL) {
      // printf("token = %s\n",token);
      struct inode* inode=NULL;
      // printf("token = %s\n",next);

      if(!dir_lookup(dir, token, &inode)|| !is_dir(inode)) {
            dir_close(dir);
            return NULL;         
        
      }
      dir_close(dir);
      dir = dir_open(inode);
            // printf("token = %s\n",token);

      token = strtok_r (NULL, "/", &savePtr);
    }
    return dir;
}


// check_path(char* path){
//     if (path == NULL ||strlen(path) == 0){
//       return NULL;
//     }

//     if (path[0] == '/'{
//         dir = dir_open_root();
//         strtok_r (path_name, "/", &savePtr); 
//         path = savePtr;
//     }
//     else if (thread_current()->dir_now==NULL){
//         dir = dir_open_root();
//     }
//     else if(!strncmp(path,"..",2)){
//       dir=thread_current()->dir_now;
//       token = strtok_r (NULL, "/", &savePtr);
//     }
//     else if(!strcmp(token,".",1)){
//       dir=thread_current()->dir_now;
//       token = strtok_r (NULL, "/", &savePtr);
//     }
//     else{
//       dir=dir_reopen(thread_current()->dir_now);
//     }

// }

// tokenize(char* path){
//   char *token, *next_ptr 
//   int length=0;
//   token=strtok_r(path,"/",&next_ptr);
//   while(token){
//     length++;
//     token=strtok_r(NULL,"/",&next_ptr);
//   }

// }
//   token = strtok_r(file_name," ", &next_ptr);
//   argv[0] = token;
//   while(token) 
//   { 
//     argc++;

//     token = strtok_r(NULL, " ", &next_ptr); 
//     argv[argc]=token;
//   }
