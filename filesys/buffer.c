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
