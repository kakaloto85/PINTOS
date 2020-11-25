#include "vm/page.h"
#include "threads/vaddr.h"

unsigned 
hash_key_func (const struct hash_elem *e, void *aux UNUSED){
    const struct spte* spte = hash_entry(e,struct spte,elem);
    // return hash_bytes (&spte->upage, sizeof spte->upage);
    return hash_int(spte->upage);
}

bool 
hash_less_key_func (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED){
    const struct spte* spte1 = hash_entry(a,struct spte,elem);
    const struct spte* spte2 = hash_entry(b,struct spte,elem);
    if(spte1->upage<spte2->upage)
        return true;
    else
        return false;
}

void
init_spt(struct hash* spt){
  hash_init(spt, hash_key_func, hash_less_key_func, NULL); 
}

struct spte*
create_spte(void * upage, enum status state) {
    //언제 free해줘야하나
    struct spte *spte = (struct spte*)malloc(sizeof(struct spte));
    spte->upage=pg_round_down(upage);
    spte->state=state;
    spte->swap_location = -1;
    spte->writable=false;
    return spte;
}


struct spte* 
get_spte(struct hash* spt,void* fault_addr){
    struct spte spte;
    spte.upage=pg_round_down(fault_addr);
    struct hash_elem* get = hash_find (spt,&spte.elem);

    if(get ==NULL){
        // printf("hi \n");
        return NULL;
    }
    else{
        return hash_entry(get,struct spte, elem);
    }
}
void spte_free(struct hash_elem *e, void *aux){
    free(hash_entry(e,struct spte, elem));
    }

void
destroy_spt(struct thread* thread){
    hash_destroy(&thread->spt, spte_free); 
}

bool 
create_spte_from_exec(struct file *file, int32_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes,bool writable)
{
  struct spte *spte = malloc(sizeof(struct spte));
  if (!spte)
    return false;

  spte->upage = pg_round_down(upage);  
  spte->state = EXEC_FILE;

  spte->file = file;
  spte->offset = ofs;
  spte->read_bytes = read_bytes;
  spte->zero_bytes = zero_bytes;
  spte->writable = writable;
  return (hash_insert(&thread_current()->spt, &spte->elem) == NULL);
}
bool 
create_spte_from_mmf(struct file *file, int32_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes,bool writable,struct mmap_file* mmf)
{
  struct spte *spte = malloc(sizeof(struct spte));
  if (!spte)
    return false;
  spte->upage = pg_round_down(upage);  
  spte->state = MM_FILE;
  spte->mmap_file = mmf;
  spte->file = file;
  spte->offset = ofs;
  
  spte->read_bytes = read_bytes;
  spte->zero_bytes = zero_bytes;
  spte->writable = true;
      list_push_back(&mmf->spte_list,&spte->mmf_elem);

  return (hash_insert(&thread_current()->spt, &spte->elem) == NULL);
}
