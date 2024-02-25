#include "fs.h"

#include <lib.h>
#include <syscall/process.h>

static fops_t dir_ops = {directory_open, directory_close, directory_read, directory_write};
static fops_t file_ops = {file_open, file_close, file_read, file_write};

/* fs_test
 * read some data and statistics from the file system
 * does not take in or return anything, just prints to screen */
void fs_test()
{
    int i;
    int j;

    boot_block_t* fs_start = (boot_block_t*) FS_START;


    printf("num dir entries: %d\n", fs_start->dir_count);
    printf("num inodes: %d\n", fs_start->inode_count);
    printf("num data blocks: %d\n", fs_start->data_count);


    for (j = 0; j < fs_start->dir_count; j++){
        printf("name of file %d: ", j);
        for (i = 0; i < FNAME_MAX; i++) {
            putc(fs_start->dir_entries[j].filename[i]);
        }
        putc('\n');
    }

    inode_t* inodes = (inode_t*)(FS_START + BLOCK_SIZE);
    data_block_t* data_blocks = (data_block_t*)(inodes + fs_start->inode_count);

    inode_t frame_inode;
    data_block_t frame_data_1;

    printf(" inode num of frame0.txt: %d\n", fs_start->dir_entries[10].inode_num);

    frame_inode = inodes[fs_start->dir_entries[10].inode_num];
    frame_data_1 = data_blocks[frame_inode.data_block_num[0]];
    printf("text: ");
    
    for (i = 0; i < FNAME_MAX; i++) {
        putc(frame_data_1.data[i]);
    }
}


/* file_read
 * read data from an open file
 * Inputs:  int32_t fd     - open fd of file to read
 *          int32_t nbytes - number of bytes to read
 * Outputs: void* buf       - Buffer to write the read data to.
 * Return Value: int32_t 0 on success, -1 on failure
 * Function: fills in the buffer with data from the file, starting
 * at the saved position file and ending after either the endi of
 * the file or number of bytes requested has been reached. */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes)
{
    uint32_t bytes_read;
    pcb_t* pcb;

    pcb = get_pcb_ptr();

    /* read and count bytes in the process */
    bytes_read = read_data(pcb->file_desc_array[fd].inode, pcb->file_desc_array[fd].file_pos, buf, nbytes);
    pcb->file_desc_array[fd].file_pos += bytes_read;
    return bytes_read;
}

/* file_write
 * write data to an open file
 * Inputs:  int32_t fd      - open fd of file to write
 *          void* buf       - buffer of data to write
 *          int32_t nbytes  - number of bytes to write
 * Outputs: none.
 * Return Value: 0 on success, -1 on failure
 * Function: Writing is not allowed. Always fails with -1. */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes)
{
    return FFAIL;
}

/* file_open
 * open a file by name
 * Inputs:  uint8_t* filename   - name of file to open
 * Outputs: none
 * Return Value: fd of the file on success, -1 on failure
 * Function: Opens a file by name */
int32_t file_open(const uint8_t* filename)
{
    dentry_t dentry;
    pcb_t* pcb = get_pcb_ptr();
    int i;
    int fd = -1;

    /* check if dentry exists */
    if (read_dentry_by_name(filename, &dentry))
        return FFAIL;

    /* try to find an empty file desciptor, fail otherwise */
    for (i = 0; i < FILE_DESC_SIZE; i++) {
        if (pcb->file_desc_array[i].flags == !IN_USE) {
            fd = i;
            break;
        }
    }
    if (fd == -1){
        return FFAIL;
    }

    /* add process */
    pcb->file_desc_array[fd].inode = dentry.inode_num;
    pcb->file_desc_array[fd].flags = IN_USE;
    pcb->file_desc_array[fd].file_pos = 0;

    pcb->file_desc_array[fd].file_ops = &file_ops;

    return fd;
}

/*
 * file_close
 * DESCRIPTION: close device abstracted as file
 * INPUTS: fd -- file descriptor for file to be closed
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
int32_t file_close(int32_t fd)
{
    pcb_t* pcb;
    pcb = get_pcb_ptr();

    /* update pcb to remove process */
    pcb->file_desc_array[fd].flags = !IN_USE;
    return FSUCCESS;
}



/* directory_read
 * read data from an open directory
 * Inputs:  int32_t fd     - open fd of directory to read
 *          int32_t nbytes - number of bytes to read
 * Outputs: void* buf       - Buffer to write the read data to.
 * Return Value: int32_t number of bytes read
 * Function: fills in the buffer with the names of the files in the directory. */
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes)
{
    dentry_t dentry;
    pcb_t* pcb;
    
    pcb = get_pcb_ptr();

    /* fail if not open */
    if (pcb->file_desc_array[fd].flags == !IN_USE)
        return 0;
    /* check index validity */
    if (read_dentry_by_index(pcb->file_desc_array[fd].file_pos / FNAME_MAX, &dentry))
        return 0;
    
    /* fill the buffer with the names of the files in dir */
    strncpy(buf, (int8_t*)dentry.filename, nbytes);
    pcb->file_desc_array[fd].file_pos += nbytes;
    return nbytes;
}

/* directory_write
 * write data to an open directory
 * Inputs:  int32_t fd      - open fd of file to directory
 *          void* buf       - buffer of data to write
 *          int32_t nbytes  - number of bytes to write
 * Outputs: none.
 * Return Value: -1 (always fails)
 * Function: Read only file system; always fails */
int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes)
{
    return FFAIL;
}


/* directory_open
 * open a directory by name
 * Inputs:  uint8_t* filename   - name of dir to open
 * Outputs: none
 * Return Value: fd of the file on success, -1 on failure
 * Function: Opens a directory by name */
int32_t directory_open(const uint8_t* filename)
{
    dentry_t dentry;
    pcb_t* pcb = get_pcb_ptr();
    int i;
    int fd = -1;

    /* check if the dir exists */
    if (read_dentry_by_name(filename, &dentry))
        return FFAIL;

    /* try to find an empty process */
    for (i = 0; i < FILE_DESC_SIZE; i++) {
        if (pcb->file_desc_array[i].flags == !IN_USE) {
            fd = i;
            break;
        }
    }
    if (fd == -1){
        return FFAIL;
    }

    /* populate the pcb, add process */
    pcb->file_desc_array[fd].flags = IN_USE;
    pcb->file_desc_array[fd].file_pos = 0;
    pcb->file_desc_array[fd].file_ops = &dir_ops;

    return fd;
}

/*
 * directory_close
 * DESCRIPTION: pseudo function for pcb ... file_ops
 * INPUTS: fd -- file descriptor
 * OUTPUTS: none
 * RETURN VALUE: -1 (always fails)
 * SIDE EFFECTS: none
 */
int32_t directory_close(int32_t fd)
{
    pcb_t* pcb;
    pcb = get_pcb_ptr();

    /* update pcb to remove process */
    pcb->file_desc_array[fd].flags = !IN_USE;
    return FFAIL;
}


/* read_dentry_by_name
 * get a d_entry from the directory
 * Inputs:  uint8_t* fname      - name of the file in directory
 * Outputs: dentry_t* dentry    - empty dentry_t to fill in
 * Return Value: int32_t 0 on success, -1 on failure
 * Function: searches for the given name in the directory and if found,
 * gets the data from that dentry */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry)
{
    int i;
    int index = -1;
    boot_block_t* fs_start = (boot_block_t*) FS_START;
    uint8_t z_fname[FNAME_MAX];

    /* parameter validation */
    if (!fname || !dentry){
        return FFAIL;
    }

    /* write the appropriate number of chars to fname */
    if (sizeof(fname) >= FNAME_MAX) {
        strncpy((int8_t*)z_fname, (int8_t*)fname, FNAME_MAX);
    }
    else {
        memset(z_fname, 0, FNAME_MAX);
        strncpy((int8_t*)z_fname, (int8_t*)fname, sizeof(fname));
    }

    /* try finding the file in dir */
    for (i = 0; i < fs_start->dir_count; i++) {
        if (!strncmp((int8_t*)fname, (int8_t*)fs_start->dir_entries[i].filename, FNAME_MAX))
            index = i;
    }
    if (index == -1)
        return FFAIL;

    read_dentry_by_index(index, dentry);

    return FSUCCESS;
}

/* read_dentry_by_index
 * get a d_entry from the directory
 * Inputs:  uint32_t index      - index of file in the directory
 * Outputs: dentry_t* dentry    - empty dentry_t to fill in
 * Return Value: int32_t 0 on success, -1 on failure
 * Function: fills in the dentry with data from the dentry
 * at that index in the directory */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry)
{
    boot_block_t* fs_start = (boot_block_t*) FS_START;

    /* parameter validation */
    if (index >= fs_start->dir_count){
        return FFAIL;
    }
    if (!dentry){
        return FFAIL;
    }

    /* fill dentry with the corresponding values */
    strncpy((int8_t*)dentry->filename, (int8_t*)fs_start->dir_entries[index].filename, FNAME_MAX);
    dentry->inode_num = fs_start->dir_entries[index].inode_num;
    dentry->filetype = fs_start->dir_entries[index].filetype;

    return FSUCCESS;
}

/* read_data
 * reads data from an open file
 * Inputs:  uint32_t inode  - inode index to read data from
 *          uint32_t offset - location to start reading data from
 *          uint32_t length - number of bytes to read
 * Return Value: int32_t number of bytes read on success, -1 on error
 * Function: reads data from an inode */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
{
    int i;

    /* prepare pointers to file system segments */
    boot_block_t* fs_start = (boot_block_t*) FS_START;
    inode_t* inodes = (inode_t*)(FS_START + BLOCK_SIZE);
    data_block_t* data_blocks = (data_block_t*)(inodes + fs_start->inode_count);
    inode_t* inode_block = &inodes[inode];
    data_block_t* data_block;

    /* validate starting position isn't past end of file */
    if (offset > inode_block->length){
        return FFAIL;
    }
    /* validate final pos isn't past end of file */
    if (offset+length > inode_block->length){ 
        length = inode_block->length - offset;
    }

    /* prepare data block indices */
    uint32_t initial_data_block = offset/BLOCK_SIZE;
    uint32_t final_data_block = (offset+length)/BLOCK_SIZE;
    uint32_t last_block_bytes = (offset+length)%BLOCK_SIZE;
    uint32_t buf_index = 0;

    /* do the first unaligned block */
    data_block = &data_blocks[inode_block->data_block_num[initial_data_block]];
    if (initial_data_block == final_data_block){
        /* if there is only one block then don't go all the way to the end */
        memcpy(buf, data_block->data + offset%BLOCK_SIZE, length);
        buf_index += length;
    } else {
        /* standard */
        memcpy(buf, data_block->data + offset%BLOCK_SIZE, BLOCK_SIZE-offset%BLOCK_SIZE);
        buf_index += BLOCK_SIZE - offset%BLOCK_SIZE;
    }

    /* loop through the aligned blocks in the middle */
    for (i = initial_data_block+1; i < final_data_block; i++){
        data_block = &data_blocks[inode_block->data_block_num[initial_data_block+i]];
        memcpy(buf+buf_index, data_block->data, BLOCK_SIZE);
        buf_index += BLOCK_SIZE;
    }

    /* do the final block at the end */
    if (initial_data_block != final_data_block && buf_index < length) {
        data_block = &data_blocks[inode_block->data_block_num[final_data_block]];
        memcpy(buf+buf_index, data_block->data, last_block_bytes);
        buf_index += last_block_bytes;
    }

    return buf_index;
}
