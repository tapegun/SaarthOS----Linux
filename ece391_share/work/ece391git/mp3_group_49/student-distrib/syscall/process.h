#ifndef _PROCESS_H
#define _PROCESS_H

#include <types.h>

#define PCB_MASK            0xffffe000

#define PROGRAM_SEGMENT     0x08000000
#define PROGRAM_ADDRESS     0x08048000
#define USER_VMEM           0x8400000

#define LARGE_SIZE          0xffffffff

#define ELF_ENTRY_OFFSET    0x18

#define FILE_DESC_SIZE      8

#define BUF_SIZE            128

#define STDIN               0
#define STDOUT              1

#define IN_USE              1

/* file operations struct */
typedef struct fops_t {
    int32_t (*open)(const uint8_t* filename);
    int32_t (*close)(int32_t fd);
    int32_t (*read)(int32_t fd, void* buf, int32_t nbytes);
    int32_t (*write)(int32_t fd, const void* buf, int32_t nbytes);
} fops_t;

/* file descriptors struct */
typedef struct file_desc_t {
    fops_t* file_ops;
    uint32_t inode;
    uint32_t file_pos;
    uint32_t flags;
} file_desc_t;

/* Process Control Block struct */
typedef struct pcb_t {
    file_desc_t file_desc_array[FILE_DESC_SIZE];
    int32_t is_vidmapped;
    uint8_t args[128];
    uint32_t execute_return;
    int32_t pid;
    int32_t parent_pid;
    uint32_t parent_esp;
    uint32_t parent_ebp;
} pcb_t;

/* create and add process to PCB */
int32_t create_process(const uint8_t* command);

/* kill and remove process from PCB */
int32_t end_process(uint8_t status);

/* access PCB pointer */
pcb_t* get_pcb_ptr();

#endif
