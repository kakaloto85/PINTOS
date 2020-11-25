#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <devices/block.h>
#include "threads/thread.h"

enum status {
  SWAP_DISK, MEMORY, EXEC_FILE, MM_FILE
};
struct spte {  
    struct hash_elem elem;
    struct list_elem mmf_elem;
    enum status state; // SWAP_DISK, MEMORY
    void *upage; // virtual address of an user page
    bool writable; // check whether the frame is writable or not
    block_sector_t swap_location; //index for swap in/out
    //for lazy loading 
    struct file *file;
    struct mmap_file *mmap_file;
    int32_t offset;
    uint32_t read_bytes;
    uint32_t zero_bytes;
    //
};

struct mmap_file {
  int map_id;
  struct file* file; 
  struct list_elem elem;
  struct list spte_list;
};
// void spte_update(struct spte* spte);
unsigned hash_key_func (const struct hash_elem *e, void *aux);
bool hash_less_key_func (const struct hash_elem *a, const struct hash_elem *b, void *aux);
void init_spt(struct hash* spt);
struct spte* get_spte(struct hash* spt,void * fault_addr);
void destroy_spt(struct thread* thread);
void spte_free(struct hash_elem *e, void *aux);
bool create_spte_from_exec(struct file *file, int32_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes,bool writable);
struct spte*create_spte(void * upage, enum status state);

#endif /* vm/page.h */