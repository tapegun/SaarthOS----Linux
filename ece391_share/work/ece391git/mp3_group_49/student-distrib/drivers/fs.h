/* fs.h - Defines for file system driver
 * vim:ts=4 noexpandtab
 */

#ifndef _FS_H
#define _FS_H

#include <types.h>

#define BLOCK_SIZE  4096
#define FNAME_MAX   32

#define DENT_RTC        0
#define DENT_FILE       1
#define DENT_DIR        2

/* directory entry type. */
typedef struct dentry_t {
    uint8_t filename[FNAME_MAX];
    uint32_t filetype;
    uint32_t inode_num;
    uint8_t reserved[24];
} dentry_t;

/* inode type */
typedef struct inode_t {
    uint32_t length;
    uint32_t data_block_num[1023];
} inode_t;

/* boot block type. only used one */
typedef struct boot_block_t {
    uint32_t dir_count;
    uint32_t inode_count;
    uint32_t data_count;
    uint8_t reserved[52];
    dentry_t dir_entries[63];
} boot_block_t;

/* data block type, so indexing is easier */
typedef struct data_block_t {
    uint8_t data[BLOCK_SIZE];
} data_block_t;

/* address of file system loaded by GRUB */
unsigned int FS_START;

/* syscalls for files */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t file_open(const uint8_t* filename);
int32_t file_close(int32_t fd);

/* syscalls for directories */
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes);
int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t directory_open(const uint8_t* filename);
int32_t directory_close(int32_t fd);

/* other file system routines */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

int32_t read_data2(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
int32_t file_read_2(int32_t fd, void* buf, int32_t nbytes);

void fs_test();


#endif  /* _FS_H */
