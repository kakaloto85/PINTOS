#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/block.h"
#include <list.h>
struct bitmap;

void inode_init (void);
bool inode_create (block_sector_t, off_t,bool);
struct inode *inode_open (block_sector_t);
struct inode *inode_reopen (struct inode *);
block_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);
struct list_elem* next_bce_elem;
struct list cache_sector_list;
void cache_list_init(void);
block_sector_t get_sector(struct inode* inode);

struct bce
    {
        struct list_elem elem;
        block_sector_t sector_idx;
        // struct thread *thread;
        struct inode* inode;
        void* buffercache;
        bool dirty;
        bool accessed;
        struct lock* bce_lock;
    };
struct bce* create_bce(block_sector_t sector_idx,struct inode* inode);
struct bce* check_buffer (block_sector_t sector_idx);
void buffer_read(struct bce* cache, uint8_t *buffer, off_t bytes_read, int sector_ofs, int chunk_size);
struct bce* bce_alloc (block_sector_t sector_idx,struct inode* inode);
struct bce* find_victim_cache (void);
void buffer_flush(struct bce* bce);
void buffer_flush_all(void);

void set_level (struct inode* inode);
block_sector_t inode_byte_to_sector (const struct inode *inode, off_t pos);
void buffer_write(struct bce* cache, uint8_t *buffer, off_t bytes_written, int sector_ofs, int chunk_size);
// void make_direct_sec_table(struct inode_disk* disk_inode, size_t sectors,const void * zeros,bool* success);
// void make_indirect_sec_table(struct inode_disk* disk_inode, size_t sectors,const void * zeros,bool* success);
// void make_double_indirect_sec_table(struct inode_disk* disk_inode, size_t sectors,const void * zeros,bool* success);
bool is_dir(const struct inode* inode);
void write_back(struct inode* inode);
void flush_all(void);

#endif /* filesys/inode.h */
