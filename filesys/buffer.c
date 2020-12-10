// void
// check_and_write(struct block* fs_device, block_sector_t sector_idx, uint8_t *buffer, off_t bytes_written, int sector_ofs, int chunk_size){   
//     struct bce* cache;
//     if((cache=check_buffer(sector_idx))!=NULL){
//         buffer_write(cache,buffer, bytes_written, sector_ofs, chunk_size);
//     }
//     else{
//         if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
//         {
//             /* Write full sector directly to disk. */
//             block_write (fs_device, sector_idx, buffer + bytes_written);
//         }
//         else 
//         {
//             cache = bce_alloc(sector_idx,inode);
//             cache->dirty = true;
//             /* If the sector contains data before or after the chunk
//               we're writing, then we need to read in the sector
//               first.  Otherwise we start with a sector of all zeros. */
//             if (sector_ofs > 0 || chunk_size < sector_left) 
//               block_read (fs_device, sector_idx, cache->buffercache);
//             else
//               memset (cache->buffercache, 0, BLOCK_SECTOR_SIZE);
//             memcpy (cache->buffercache + sector_ofs, buffer + bytes_written, chunk_size);
//         }
//       }
// void
// check_and_read (struct block* fs_device, block_sector_t sector_idx, uint8_t *buffer, off_t bytes_read, int sector_ofs, int chunk_size)
//     struct bce* cache;
//     if((cache=check_buffer(sector_idx))!=NULL){
//     buffer_read(cache,buffer, bytes_read, sector_ofs, chunk_size);
//     // cache->inode = inode;
//     }
//     else{
//     if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
//         {
//         /* Read full sector directly into caller's buffer. */
//         // 나중에 비효율적이면 고쳐보자!
//         block_read (fs_device, sector_idx, buffer + bytes_read);
//         }
//     else 
//         {
//         cache = bce_alloc(sector_idx,inode);
//         /* Read sector into bounce buffer, then partially copy
//             into caller's buffer. */
//         // if (bounce == NULL) 
//         //     {
//         //     bounce = malloc (BLOCK_SECTOR_SIZE);
//         //     if (bounce == NULL)
//         //         break;
//         //     }
//         block_read (fs_device, sector_idx, cache->buffercache);
//         memcpy (buffer + bytes_read, cache->buffercache + sector_ofs, chunk_size);
//         }
//     }

// void
// grow_file(struct inode* inode, int offset, int size){
//  block_sector_t sectors = bytes_to_sectors(offset+size);
//  block_sector_t inode_sectors = bytes_to_sectors(inode_length(inode));
//  needed_sectors = sectors - inode_sectors;
//     if (inode_sectors >= sectors){
//         return true;
//     }

//     if(sectors>inode_sectors){
//         else if (sectors < DOUBLE_SEC_BOUNDARY) {
//             if (inode.data->double_indirect_sec == -1) {
//                 free_map_allocate (1, &inode.data->double_indirect_sec);
//                 double_index = DIV_ROUND_UP(sectors, 128);
//                 for (i=0;i<double_index;i++) {

//                 }
//             }
//         }

// void
// set_level (struct inode* inode) {
//     off_t length = inode->data.length;
//     size_t sectors = bytes_to_sectors (length);
//     if (sectors <= 124)
//       inode->level = 1;
//     else if (sectors <=252)
//       inode->level = 2;
//     else if (sectors <= 16000)
//       inode->level = 3;
//     else
//       inode->level = NULL;
// }
// void
// set_inode_method (block_sector_t sector, off_t length, ) {
//     if (length <= 124)
//         make_direct_sec_table(struct inode_disk* disk_inode, int sectors,char * zeros,bool* success)
//     else if (length <=252)
//         method_2
//     else if (length <= 16000)
//         method_3
//     else
//         exit;
// }
//remove read write 고쳐야함!
//구현해야 할 함수
//length를 통해 direct, indirect, double indirect 구분하는 함수 V
//inode나 inode disk에 lenght관련 flag를 저장 V
//inode_byte_to_sector를 만들어야 함 ! V


//direct table만드는 함수 V
//indirect table만드는 함수 V
//double indirect table만드는 함수 V

//remove, close --> 생각하기
// close 할 때는, buffer cache 에 있는 것들을 evict 하기!
// close는 지금은 start 부터 length 까지지만, 우리는 일일히로 해야할 것 같다.
// remove 자체는 flag 만 바꾸는 것이니까 딱히 생각 안해도 된다.
// 
// inode.c 로 옮기기
// sector 0, 1 은 안 겹치게 하기
// 디버깅

// dir 위한 inode 인지 file을 위한 inode인지 구분하는 flag 필요 => 한양대에서는 inode_disk에 수정하라고 되어있는데, inode에다가 넣어도 되지 않을까?
// inode_create 에서 dir inode 인지 file inode인지 구분하는 flag를 parameter로 받는다.

// exec으로 child process를 생성하고 실행할 때, 부모의 current directory를 받을 수 있도록 해야한다. => 즉, thread 안에다가 cur_dir을 새로운 field로 저장해야할 것이다.

// parse_path (한양대에서의 함수 이름) 함수 구현: 
// struct dir* parse_path (char *path_name, char *file_name) {
//     struct dir *dir;
//     if (path_name == NULL || file_name == NULL)
//     goto fail;
//     if (strlen(path_name) == 0)
//     return NULL;
//     /* PATH_NAME의 절대/상대경로에 따른 디렉터리 정보 저장 (구현)*/ 
//     => '/' 로 시작하면, dir = dir_open_root() 로 설정 (절대경로)
//     =>  else, thread_current 의 dir을 기준으로 시작 (상대경로) dir = thread_current ->dir_flag
//     char *token, *nextToken, *savePtr;
//     token = strtok_r (path_name, "/", &savePtr); ->  
//     nextToken = strtok_r (NULL, "/", &savePtr);
//     while (token != NULL && nextToken != NULL){
//     /* dir에서 token이름의 파일을 검색하여 inode의 정보를 저장*/ -> dir_lookup(dir, token, &inode) => dir_lookup 에서의 수정이 필요할 수도.. (여기서 . , .. 구분하는 것인가?)
//     /* inode가 파일일 경우 NULL 반환 */ =>  return NULL
//     /* dir의 디렉터리 정보를 메모리에서 해지 */ => dir_close(dir)
//     /* inode의 디렉터리 정보를 dir에 저장 */ =>. dir = next (dir_open(inode))  
//     /* token에 검색할 경로 이름 저장 */ => dir_entry->name + token ?
//     }
//     /* token의 파일 이름을 file_name에 저장 -> file_name = token
//     /* dir 정보 반환 */ -> return dir
// }
// 
// parse_path (한양대에서의 함수 이름) 함수 구현: 
// struct dir* parse_path (char *path_name, char *file_name) {
//     struct dir *dir;
//     if (path_name == NULL || file_name == NULL)
//     goto fail;
//     if (strlen(path_name) == 0)
//     return NULL;
//     if (path_name[0] == '/')
//          dir = dir_open_root();
// 
//     else
//          dir = thread_current() -> cur_dir;
//     char *token, *nextToken, *savePtr;
//     token = strtok_r (path_name, "/", &savePtr); 
//     nextToken = strtok_r (NULL, "/", &savePtr);
//     while (token != NULL && nextToken != NULL) {
//      struct *inode inode = NULL;
//          
//     if (!dir_lookup(dir, token, &inode) || inode->data.dir_flag == false) {
//          dir_close(dir);
//          return NULL;         
//     }
//     
//     dir = dir_open(inode);
//     /* inode의 디렉터리 정보를 dir에 저장 */ =>. dir = next (dir_open(inode))  
//     /* token에 검색할 경로 이름 저장 */ => dir_entry->name + token ?
//     }
//     /* token의 파일 이름을 file_name에 저장 -> file_name = token
//     /* dir 정보 반환 */ -> return dir
// }
// 