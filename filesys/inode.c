#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "threads/synch.h"

// #include "filesys/buffer.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define DIRECT_SEC_BOUNDARY 123
#define INDIRECT_SEC_BOUNDARY 251
#define DOUBLE_SEC_BOUNDARY 160000

static char zeros[BLOCK_SECTOR_SIZE];            


struct inode_disk
  {
    bool dir_flag;
    off_t length;
    unsigned magic;
    block_sector_t direct_table[123];
    block_sector_t indirect_sec;
    block_sector_t double_indirect_sec;
  };

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
    struct lock write_lock;
    int read;
    off_t pos;
  };
struct semaphore cache_lock;
void func_write_behind(void){
    while(true){
        timer_sleep(50);
        buffer_flush_all();
    }
}
// void func_read_ahead(void* sector){
//   check_buffer(&sector)
// }

void
cache_list_init(void){
    list_init(&cache_sector_list);
    sema_init(&cache_lock,1);
    thread_create("working_thread:write_behind", 0, func_write_behind, NULL);
}
block_sector_t
get_sector(struct inode* inode){
  return inode->sector;
}

struct bce*
create_bce(block_sector_t sector_idx,struct inode* inode){
    // printf("create_bce = %d\n",  sizeof(struct inode_disk));
    struct bce* cache = malloc(sizeof(struct bce));
    cache->sector_idx=sector_idx;
    cache->dirty=0;
    cache->accessed=0;
    // cache->thread=thread_current();
    cache->inode=inode;
    list_push_back(&cache_sector_list,&cache->elem);
    return cache;
}

struct bce*
check_buffer (block_sector_t sector_idx) 
{
    struct bce* bce;
    struct list_elem* e;
    for(e=list_begin(&cache_sector_list);e!=list_end(&cache_sector_list); e=list_next(e)){
        bce = list_entry(e,struct bce, elem);
        if (bce->sector_idx == sector_idx)
            return bce;
    }
    return NULL;
}
void
buffer_read(struct bce* cache, uint8_t *buffer, off_t bytes_read, int sector_ofs, int chunk_size){
  sema_down(&cache_lock);
  void* buffercache = cache->buffercache;
  cache->accessed=true;
  memcpy (buffer + bytes_read, buffercache + sector_ofs, chunk_size);
  sema_up(&cache_lock);
}

struct bce*
bce_alloc (block_sector_t sector_idx,struct inode* inode){
  sema_down(&cache_lock);
  struct bce* bce;
  if (list_size(&cache_sector_list)==64){
    bce= find_victim_cache();
    if(bce->dirty){
        buffer_flush(bce);
    }
    bce->sector_idx=sector_idx;
    bce->inode=inode;
    bce->accessed=0;
    } 
  else {
    bce=create_bce(sector_idx,inode);
    uint8_t *bounce = malloc (BLOCK_SECTOR_SIZE);
    bce->buffercache = bounce;
  }
  sema_up(&cache_lock);
  return bce;
}


struct bce* 
find_victim_cache (void){ 
  struct bce* evict;
  struct list_elem* e;
    // next_bce_elem을 처음에 NULL 로 함수 밖에서 지정
  while (true) {
    if (next_bce_elem == NULL){
        e = list_begin(&cache_sector_list); 
    }
    else{
        e = next_bce_elem;
    }
    if (list_next(e) == list_tail(&cache_sector_list)){
        next_bce_elem = list_begin(&cache_sector_list);
    }
    else{
        next_bce_elem = list_next(e);
    }
    evict = list_entry(e,struct bce, elem);


    if (evict->accessed) {
        evict->accessed = false;
    }
    else
        break;
  }
  return evict;
}


void
buffer_flush(struct bce* bce){
    block_write (fs_device, bce->sector_idx, bce->buffercache);
    bce->dirty=0;
}
void
buffer_flush_all(void){
  
  struct list_elem *e;
  struct bce *bce;
  for(e=list_begin(&cache_sector_list);e!=list_end(&cache_sector_list);e=list_next(e)){
    bce=list_entry(e,struct bce,elem);
    if(bce->dirty)
      buffer_flush(bce);   
  }
}
void
buffer_write(struct bce* cache, uint8_t *buffer, off_t bytes_written, int sector_ofs, int chunk_size){
  sema_down(&cache_lock);
  void* buffercache = cache->buffercache;
  cache->dirty=true;
  cache->accessed=true;
  memcpy (buffercache + sector_ofs,buffer + bytes_written, chunk_size);
  sema_up(&cache_lock);
}
void
make_direct_sec_table(struct inode_disk* disk_inode, size_t sectors,const  void * zeros,bool* success){
  for(int i=0;i<DIRECT_SEC_BOUNDARY;i++){
    // printf("here\n");]
    if(i<sectors){
      free_map_allocate (1, &disk_inode->direct_table[i]);
      // printf("sec=%d\n",disk_inode->direct_table[i]);
      block_write (fs_device, disk_inode->direct_table[i], zeros);
    }
    else{
      disk_inode->direct_table[i]=-1;
    }
  }
  disk_inode->indirect_sec=-1;
  disk_inode->double_indirect_sec=-1;
  *success = true; 
}

// void
// make_direct_sec_table_at (struct inode_disk* disk_inode, size_t sectors, int start_index, const  void * zeros,bool* success){
//   for(int i=start_index;i<124;i++){
//     // printf("here\n");]
//     if(i<sectors){
//       free_map_allocate (1, &disk_inode->direct_table[i]);
//       // printf("sec=%d\n",disk_inode->direct_table[i]);
//       block_write (fs_device, disk_inode->direct_table[i], zeros);
//     }
//     else{
//       disk_inode->direct_table[i]=-1;
//     }
//   }
//   disk_inode->indirect_sec=-1;
//   disk_inode->double_indirect_sec=-1;
//   *success = true; 
// }

void
make_indirect_sec_table(struct inode_disk* disk_inode, size_t sectors,const void * zeros, bool* success){
  block_sector_t indirect_block[128];
  free_map_allocate (1, &disk_inode->indirect_sec);
  // printf("make_indirect_sec_table: sectors %d\n",disk_inode->indirect_sec);
  for(int j=0;j<128;j++){
    if(j<sectors-DIRECT_SEC_BOUNDARY){
      free_map_allocate (1, &indirect_block[j]);
      // printf("make_indirect_sec_table : indirect_block %d\n",indirect_block[j]);
      block_write (fs_device,indirect_block[j], zeros);
    }
    else{
      // printf("??????????????/\n");
      indirect_block[j]=-1;
    }
  }
  block_write (fs_device, disk_inode->indirect_sec, indirect_block);
  disk_inode->double_indirect_sec=-1;
  *success = true;
}



void
make_double_indirect_sec_table(struct inode_disk* disk_inode, size_t sectors,const void * zeros,bool* success){
  int double_sector=sectors-INDIRECT_SEC_BOUNDARY;
  int double_index=double_sector/128;
  int remain = double_sector-double_index*128;
  // printf("remain = %d\n",remain);
  //double_index만큼 double_indirect_block을 채움
  free_map_allocate (1, &disk_inode->double_indirect_sec);
  block_sector_t double_indirect_block[128];
  for(int k=0;k<double_index;k++){
    free_map_allocate (1, &double_indirect_block[k]);
    block_sector_t indirect_block[128];
    for(int j=0;j<128;j++){
      free_map_allocate (1,&indirect_block[j]);
      block_write (fs_device,indirect_block[j], zeros);
    }
    block_write (fs_device,double_indirect_block[k], indirect_block);
  }
  //remain에 대해서는 double_indirect_block의 한 인덱스를 할당받아서 넣어줌
  free_map_allocate (1, &double_indirect_block[double_index]);
  block_sector_t indirect_block[128];
  for(int j=0;j<128;j++){
      if(j<remain){
        free_map_allocate (1,&indirect_block[j]);
        block_write (fs_device,indirect_block[j], zeros);
      }
      else{
        // printf("?????????????????????\n");
        //아래가 문제....''
        indirect_block[j]=-1;
      }
    }
  for(int i = double_index+1;i<128;i++){
    double_indirect_block[i]=-1;
  }
  block_write (fs_device,double_indirect_block[double_index], indirect_block);
  block_write (fs_device,disk_inode->double_indirect_sec, double_indirect_block);
  *success = true; 
}
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

block_sector_t
inode_byte_to_sector (const struct inode *inode, off_t pos) 
{
  size_t pos_sectors = pos/BLOCK_SECTOR_SIZE;
  struct inode_disk disk_inode = inode->data;
  if (pos_sectors <DIRECT_SEC_BOUNDARY){
    // printf("inode_byte_to_sector=%d\n",pos_sectors);
    // printf("inode_byte_to_sector=%d\n",(block_sector_t)disk_inode.direct_table[pos_sectors]);

    return (block_sector_t) disk_inode.direct_table[pos_sectors];
  }
  else if (pos_sectors<INDIRECT_SEC_BOUNDARY){
    struct bce* cache;
    block_sector_t indirect[128];

    if((cache=check_buffer(disk_inode.indirect_sec))==NULL){
      cache = bce_alloc(disk_inode.indirect_sec,inode);
      block_read (fs_device, disk_inode.indirect_sec, cache->buffercache);
    }
    // if(indirect[pos_sectors -DIRECT_SEC_BOUNDARY]<0||indirect[pos_sectors -DIRECT_SEC_BOUNDARY]>100000){
    //   exit(-1);
    // }

    memcpy(indirect,cache->buffercache,BLOCK_SECTOR_SIZE);
    //         printf("inode_byte_to_sector=%d\n",disk_inode.indirect_sec);
    // printf("inode_byte_to_sector=%d\n",pos_sectors -DIRECT_SEC_BOUNDARY);
    // printf("inode_byte_to_sector=%d\n",(block_sector_t)indirect[pos_sectors -DIRECT_SEC_BOUNDARY]);

    return  (block_sector_t) indirect[pos_sectors -DIRECT_SEC_BOUNDARY];
  }
  else if (pos_sectors <DOUBLE_SEC_BOUNDARY){
    // printf("double\n");
    struct bce* cache;
      block_sector_t double_indirect[128];
      block_sector_t indirect[128];

    if((cache=check_buffer(disk_inode.double_indirect_sec))!=NULL){
      int index = (pos_sectors-INDIRECT_SEC_BOUNDARY)/128;
      memcpy(double_indirect,cache->buffercache,128*4);

      block_sector_t indirect_sec= double_indirect[index];
      block_sector_t remain = (pos_sectors-INDIRECT_SEC_BOUNDARY)-index*128;
      struct bce* indirect_cache;
      if((indirect_cache=check_buffer(indirect_sec))==NULL){
        indirect_cache = bce_alloc(indirect_sec,inode);
        block_read (fs_device, indirect_sec, indirect_cache->buffercache);
      }
      
      memcpy(indirect,indirect_cache->buffercache,512);
      // printf("inode_byte_to_sector=%d\n",disk_inode.double_indirect_sec);
      // printf("inode_byte_to_sector=%d\n",(remain)%128);
      // printf("inode_byte_to_sector=%d\n",(block_sector_t)indirect[(remain)%128]);

      return (block_sector_t) indirect[(remain)%128];
    }
    else{
          // printf("double\n");

      cache=bce_alloc(disk_inode.double_indirect_sec,inode);
      block_read (fs_device, disk_inode.double_indirect_sec, cache->buffercache);
      int index = (pos_sectors-INDIRECT_SEC_BOUNDARY)/128;
      memcpy(double_indirect,cache->buffercache,128*4);

      block_sector_t indirect_sec= double_indirect[index];
      block_sector_t remain = (pos_sectors-INDIRECT_SEC_BOUNDARY)-index*128;
      struct bce* indirect_cache;
      if((indirect_cache=check_buffer(indirect_sec))==NULL){
        indirect_cache = bce_alloc(indirect_sec,inode);
        block_read (fs_device, indirect_sec, indirect_cache->buffercache);
      }
      memcpy(indirect,indirect_cache->buffercache,512);
      // printf("inode_byte_to_sector=%d\n",disk_inode.double_indirect_sec);
      // printf("inode_byte_to_sector=%d\n",(remain)%128);
      // printf("inode_byte_to_sector=%d\n",(block_sector_t)indirect[(remain)%128]);

      return (block_sector_t) indirect[(remain)%128];
    }
  }
  else
    return -1;
}


/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
// static block_sector_t
// byte_to_sector (const struct inode *inode, off_t pos) 
// {
//   ASSERT (inode != NULL);
//   if (pos < inode->data.length)
//     return inode->data.start + pos / BLOCK_SECTOR_SIZE;
//   else
//     return -1;
// }

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool dir_flag)
{
  // printf("inode_create sec = %d \n",sector);
  //   printf("inode_create len = %d \n",length);

  struct inode_disk *disk_inode = NULL;
  bool success = false;
  ASSERT (length >= 0);
  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length);
      //dir인지 file인 확인해주는 Flag
      disk_inode->dir_flag = dir_flag;
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      // printf("sec=%d\n",sector);
      if(sectors==0){
        make_direct_sec_table(disk_inode,1, zeros, &success);
      }
      else if(sectors<=DIRECT_SEC_BOUNDARY){
        make_direct_sec_table(disk_inode,sectors, zeros, &success);
      }
      else if(sectors<=INDIRECT_SEC_BOUNDARY){
        make_direct_sec_table(disk_inode,(size_t)DIRECT_SEC_BOUNDARY, zeros, &success);
        make_indirect_sec_table(disk_inode,sectors, zeros, &success);
      }
      else if(sectors<=DOUBLE_SEC_BOUNDARY){
        // printf("inode_create double\n")
        make_direct_sec_table(disk_inode,(size_t)DIRECT_SEC_BOUNDARY, zeros, &success);
        //for indirect
        make_indirect_sec_table(disk_inode,INDIRECT_SEC_BOUNDARY, zeros, &success);
        //for double indirect
        make_double_indirect_sec_table(disk_inode,sectors,zeros,&success);
      }
      else{
        // printf("inode_create error : file size too big!!\n");
        success =false;
      }
      // struct bce* inode_disk_cache=bce_alloc(sector,NULL);
      // buffer_write(inode_disk_cache,disk_inode,0,0,BLOCK_SECTOR_SIZE);
      block_write (fs_device, sector, disk_inode);
      // printf("disk_inode.length : %d \n", disk_inode->length);
      free (disk_inode);

    }

  // printf("%d length1 \n", disk_inode->length);


  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{

  struct list_elem *e;
  struct inode *inode;
  // printf("opening sector %d \n", sector);
  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          // printf("inode open2 sector: %d\n",inode->sector);
          // printf("inode open2 cnt: %d\n",inode->open_cnt);

          return inode; 
        }
    }
  // printf("inode_open\n");
  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  inode->pos = 0;
  //block read 맞을까?
  block_read (fs_device, inode->sector, &inode->data);
      //   struct bce* inode_disk_cache=bce_alloc(inode->sector,inode);

      // buffer_write(inode_disk_cache,&inode->data,0,0,BLOCK_SECTOR_SIZE);

  lock_init(&inode->write_lock);
  inode->read = inode->data.length;
          //   printf("inode open sector: %d\n",inode->sector);
          // printf("inode open cnt: %d\n",inode->open_cnt);

  // printf("after open inode length: %d \n",inode->data.length);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  // printf("inode reopen sector: %d\n",inode->sector);
  // printf("inode reopen cnt: %d\n",inode->open_cnt);

  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  // printf("inode close sector: %d\n",inode->sector);
  // printf("inode close cnt: %d\n",inode->open_cnt);

  /* Ignore null pointer. */
  if (inode == NULL)
    return;
  //close하면 
  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
                // printf("here...?\n");
      // printf("inode_close\n");
      inode->pos = 0;
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      //이부분이 뭘까 
      if (inode->removed) 
        {
          // printf("here...?\n");
          int sectors = (inode->data.length)/BLOCK_SECTOR_SIZE+1;
          int remove;
          for(int i = 0;i<sectors;i++){
            remove = inode_byte_to_sector(inode,i*512);
            free_map_release(remove,1);
          }
          free_map_release (inode->sector, 1);
        }

      struct bce* inode_disk_cache;
      if((inode_disk_cache=check_buffer(inode->sector))!=NULL){
        if(inode_disk_cache->dirty)
          buffer_write(inode_disk_cache,&inode->data,0,0,BLOCK_SECTOR_SIZE);
      }
      else{
        // printf("here2\n");
        block_write(fs_device,inode->sector,&inode->data);
      }

      free (inode); 
          // printf("inode_close success\n");

    }
  // else
  //   block_write(fs_device,inode->sector,&inode->data);

}


/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  // printf("inode_read_at = %d\n",inode->sector);
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = inode_byte_to_sector (inode, offset);
      // printf("%d hi \n", sector_idx);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode->read - offset;
      // printf("inode_left : %d \n", inode_left);
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      // printf("size = %d\n",inode->data.length);
      if (chunk_size <= 0)
        break;
      struct bce* cache;
      if((cache=check_buffer(sector_idx))!=NULL){
        buffer_read(cache,buffer, bytes_read, sector_ofs, chunk_size);
        // cache->inode = inode;
      }
      else{
        if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
            {
            /* Read full sector directly into caller's buffer. */
            // 나중에 비효율적이면 고쳐보자!
            block_read (fs_device, sector_idx, buffer + bytes_read);
            }
        else 
            {
            cache = bce_alloc(sector_idx,inode);
            /* Read sector into bounce buffer, then partially copy
                into caller's buffer. */
            // if (bounce == NULL) 
            //     {
            //     bounce = malloc (BLOCK_SECTOR_SIZE);
            //     if (bounce == NULL)
            //         break;
            //     }
            block_read (fs_device, sector_idx, cache->buffercache);
            memcpy (buffer + bytes_read, cache->buffercache + sector_ofs, chunk_size);
            }
      }
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
//   free (bounce);

  return bytes_read;
}
bool
grow_file(struct inode* inode, off_t offset, off_t size){ 
 bool success=false;
 int sectors = bytes_to_sectors(offset+size);

 int inode_sectors = bytes_to_sectors(inode_length(inode));
 struct bce* cache;
  if (inode_sectors == sectors){
    success=true;
  }
  else{
    if(sectors<=DIRECT_SEC_BOUNDARY){
      for(int i= inode_sectors;i<sectors;i++){
        free_map_allocate (1, &inode->data.direct_table[i]);
        block_write (fs_device, inode->data.direct_table[i], zeros);
      }
      success=true;
      // printf("growfile_direct\n");
    }
    else if(sectors<=INDIRECT_SEC_BOUNDARY){
      block_sector_t indirect_block[128];
      if(inode->data.indirect_sec==-1){
        free_map_allocate (1, &inode->data.indirect_sec);
        cache = bce_alloc(inode->data.indirect_sec,inode);
        for(int i= inode_sectors;i<DIRECT_SEC_BOUNDARY;i++){
          free_map_allocate (1, &inode->data.direct_table[i]);
          block_write (fs_device, inode->data.direct_table[i], zeros);
        }
        for(int j=0;j<128;j++){
          if(j<sectors-DIRECT_SEC_BOUNDARY){
            free_map_allocate (1, &indirect_block[j]);
            block_write(fs_device,indirect_block[j],zeros);
          }
          else{
            indirect_block[j]=-1;
          }
        }
      }
      else{
        if((cache=check_buffer(inode->data.indirect_sec))!=NULL){
          memcpy(indirect_block,cache->buffercache,BLOCK_SECTOR_SIZE); 
        }
        else{
          cache = bce_alloc(inode->data.indirect_sec,inode);
          memcpy(indirect_block,cache->buffercache,BLOCK_SECTOR_SIZE); 
        }
        for(int i=inode_sectors-DIRECT_SEC_BOUNDARY;i<sectors-DIRECT_SEC_BOUNDARY;i++){
          free_map_allocate(1,&indirect_block[i]);
          block_write(fs_device,indirect_block[i],zeros);
        }
      }
      buffer_write(cache,indirect_block,0,0,BLOCK_SECTOR_SIZE);
      success=true;
    }
    else if (sectors <=DOUBLE_SEC_BOUNDARY) {
      if(inode_sectors<=DIRECT_SEC_BOUNDARY){
        for(int i= inode_sectors;i<DIRECT_SEC_BOUNDARY;i++){
          free_map_allocate (1, &inode->data.direct_table[i]);
          block_write (fs_device, inode->data.direct_table[i], zeros);
        }
        block_sector_t indirect_block[128];
        free_map_allocate (1, &inode->data.indirect_sec);
        cache = bce_alloc(inode->data.indirect_sec,inode);
        for(int j=0;j<128;j++){
          free_map_allocate (1, &indirect_block[j]);
          block_write(fs_device,indirect_block[j],zeros);
        }
        buffer_write(cache,indirect_block,0,0,BLOCK_SECTOR_SIZE);

      }
      else if(inode_sectors<=INDIRECT_SEC_BOUNDARY){  
        block_sector_t indirect_block[128];
        free_map_allocate (1, &inode->data.indirect_sec);
        cache = bce_alloc(inode->data.indirect_sec,inode);
        for(int j=inode_sectors-DIRECT_SEC_BOUNDARY;j<128;j++){
          free_map_allocate (1, &indirect_block[j]);
          block_write(fs_device,indirect_block[j],zeros);
        }
        buffer_write(cache,indirect_block,0,0,BLOCK_SECTOR_SIZE);
      }
      struct bce* double_cache;
      if (inode->data.double_indirect_sec == -1)
        make_double_indirect_sec_table(&inode->data,sectors,zeros,&success);
      else{
        block_sector_t double_indirect_block[128];
        if((double_cache=check_buffer(inode->data.double_indirect_sec))!=NULL){
          memcpy(double_indirect_block,double_cache->buffercache,BLOCK_SECTOR_SIZE); 
        }
        else{
          double_cache = bce_alloc(inode->data.double_indirect_sec,inode);
          memcpy(double_indirect_block,double_cache->buffercache,BLOCK_SECTOR_SIZE); 
        }

        int start_index = (inode_sectors-INDIRECT_SEC_BOUNDARY-1)/128;
        int start_remain = inode_sectors-INDIRECT_SEC_BOUNDARY-1 - (start_index)*128;
        int finish_index = (sectors-INDIRECT_SEC_BOUNDARY-1)/128;
        int finish_remain = sectors-INDIRECT_SEC_BOUNDARY-1 - finish_index*128;
        for(int i=start_index;i<=finish_index;i++){  
          block_sector_t indirect_block[128];
          int start;
          if(i==start_index)
            start=start_remain;
          else
            start=0;
          struct bce* indirect_cache;
          if((indirect_cache=check_buffer(double_indirect_block[i]))!=NULL){
            memcpy(indirect_block,cache->buffercache,BLOCK_SECTOR_SIZE);
            if(i<finish_index){
              for(int j = start; j<128; j++){
                free_map_allocate (1, &indirect_block[j]);
                block_write(fs_device,indirect_block[j],zeros);    
              }
            }
            else{
              for(int j = start; j<finish_remain; j++){
                free_map_allocate (1, &indirect_block[j]);
                block_write(fs_device,indirect_block[j],zeros);    
              }
            }
            buffer_write(indirect_cache,indirect_block,0,0,BLOCK_SECTOR_SIZE);
          }
          else{
            block_read(fs_device,double_indirect_block[i],indirect_block);
            if(i<finish_index){
              for(int j = start; j<128; j++){
                free_map_allocate (1, &indirect_block[j]);
                block_write(fs_device,indirect_block[j],zeros);    
              }
            }
            else{
              for(int j = start; j<finish_remain; j++){
                free_map_allocate (1, &indirect_block[j]);
                block_write(fs_device,indirect_block[j],zeros);    
              }
            }
          }
        }
        buffer_write(cache,double_indirect_block,0,0,BLOCK_SECTOR_SIZE);
      }
      success=true;
    }
  }
  inode->data.length=offset+size;
  inode->read = offset+size;
  struct bce* inode_disk_cache;
    //  printf("grow_file %d\n",inode->read );

  // printf("here\n");
  return success;
}
      



/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  // printf("inode_write_at start %d\n",inode->sector);
  
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;
  if (inode->deny_write_cnt)
    return 0;
  int sectors = bytes_to_sectors(offset+size);
  int inode_sectors = bytes_to_sectors(inode_length(inode));
  if(inode_length(inode)<offset+size){
      lock_acquire(&inode->write_lock);
    grow_file(inode,offset,size);
      lock_release(&inode->write_lock);

  }

  while (size > 0) 
    {
      // printf("offset= %d\n",size);
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = inode_byte_to_sector (inode, offset);
      // printf("%d sector_idx at inode_write_at \n", sector_idx);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;
      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0){
        break;
      }
      struct bce* cache;
      if((cache=check_buffer(sector_idx))!=NULL){
        buffer_write(cache,buffer, bytes_written, sector_ofs, chunk_size);
        // cache->inode = inode;
      }
      else{
        if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
          {
            /* Write full sector directly to disk. */
            block_write (fs_device, sector_idx, buffer + bytes_written);
          }
        else 
          {
            cache = bce_alloc(sector_idx,inode);
            cache->dirty = true;

            /* If the sector contains data before or after the chunk
              we're writing, then we need to read in the sector
              first.  Otherwise we start with a sector of all zeros. */
            if (sector_ofs > 0 || chunk_size < sector_left) 
              block_read (fs_device, sector_idx, cache->buffercache);
            else
              memset (cache->buffercache, 0, BLOCK_SECTOR_SIZE);

            memcpy (cache->buffercache + sector_ofs, buffer + bytes_written, chunk_size);
            // block_write (fs_device, sector_idx, bounce);
            // cache->buffercache=bounce;
          }
      }
        /* Advance. */
        size -= chunk_size;
        offset += chunk_size;
        bytes_written += chunk_size;
    }
  // free (bounce);
  // printf("bytes_written at inode_write at: %d \n", bytes_written);
  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}
bool
is_dir(const struct inode* inode){
  // if(inode->data.dir_flag)
  //   return true;
  // else 
  //   return false;
  return inode->data.dir_flag;
}

void
write_back(struct inode* inode){
  block_write(fs_device,inode->sector,&inode->data);
}

void
flush_all(void){
  struct list_elem* e;
  for(e=list_begin(&open_inodes);e!=list_tail(&open_inodes);e=list_next(e)){
    write_back(list_entry(e,struct inode,elem));
}


}
bool
check_open(struct inode* inode){
  if(inode->open_cnt>2)
    return true;
  else
    return false;
}
off_t
bring_inode_pos(struct inode* inode) {
  return inode->pos;
}

// set_inode_pos(struct inode* inode, off_t pos) {
//   return inode->pos = pos;
// }