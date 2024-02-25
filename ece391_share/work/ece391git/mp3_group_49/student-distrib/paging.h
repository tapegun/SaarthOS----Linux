/*
 * Functions required for paging to work with VGA
 * 
 * References Used:
 *      Intel ISA-32 Manual section 3-9, Page 91
 */

#ifndef PAGING_H
#define PAGING_H

#define VGA_PT_IDX          184
#define VGA_BASE_ADDR       0xB8000
#define VGA_PD              0
#define USR_VGA_PD          0x21
#define KERNAL_ADDR         0x400000
#define KERNAL_PD_ENTRY     1
#define PD_SIZE             1024
#define PT_SIZE             1024

#define PAGE_ALIGN_OFFSET   12
#define DIR_BIT_OFF         22
#define TABLE_BMASK         0x3FF

#include "types.h"

/* Define helper structs here */

/* Page-Directory Entry (4-KByte Page) */
typedef struct pdir_entry_t {
    struct {
        uint32_t present : 1;       //Present
        uint32_t rw : 1;            //Read/Write
        uint32_t us : 1;            //User/supervisor (U/S) flag
        uint32_t pwt : 1;           //Page-level write-through (PWT) flag
        uint32_t pcd : 1;           //Page-level cache disable (PCD)
        uint32_t a : 1;             //Accessed (A) flag
        uint32_t reserved : 1;      //Reserved and available
        uint32_t ps : 1;            //Page Select
        uint32_t g: 1;              //Page Table Attribute Index
        uint32_t avail : 3;         //Available for system programmer’s use 11:9
        uint32_t addr : 20;         //Page Base Address 31:12
    } __attribute__ ((packed));
} pdir_entry_t;

/*Page-Table Entry (4-KByte Page)*/
typedef struct ptable_entry_t {
    struct {
        uint32_t present : 1;       //Present
        uint32_t rw : 1;            //Read/Write
        uint32_t us : 1;            //User/supervisor (U/S) flag
        uint32_t pwt : 1;           //Page-level write-through (PWT) flag
        uint32_t pcd : 1;           //Page-level cache disable (PCD)
        uint32_t a : 1;             //Accessed (A) flag
        uint32_t dirty : 1;         //Dirty (D) flag
        uint32_t reserved : 1;      //Reserved and available
        uint32_t g: 1;              //Page Table Attribute Index 
        uint32_t avail : 3;         //Available for system programmer’s use 11:9
        uint32_t addr : 20;         //Page Base Address 31:12
    } __attribute__ ((packed));
} ptable_entry_t;


/* Arrays for page_directory & page_entry */
extern pdir_entry_t page_directory[PD_SIZE];
extern ptable_entry_t page_table[PT_SIZE];


/* Macro To Set Address in Page Table */
#define PTAB_SET_ADDR(n, address)  \
    page_table[n].addr = ((unsigned int) address) >> PAGE_ALIGN_OFFSET


/* Macro To Set Address in Page Directory */
#define PDIR_SET_ADDR(n, address)  \
    page_directory[n].addr = ((unsigned int) address) >> PAGE_ALIGN_OFFSET

/* Initialize Paging */
void init_paging(void);

int32_t map_large(uint32_t* v_addr, uint32_t* p_addr);
int32_t map_vmem(uint8_t** start);
int32_t unmap_small(uint32_t* v_addr);
int32_t unmap_large(uint32_t* v_addr);

#define FLUSH_TLB() asm volatile ("movl %%cr3, %%eax \n movl %%eax, %%cr3"::: "%eax")

#endif /* PAGING_H */
