#include "paging.h"
#include "lib.h"
#include "syscall/process.h"

static ptable_entry_t user_vid_table[PT_SIZE] __attribute((aligned(4096)));


/*
 * init paging
 * DESCRIPTION: Initializes 1KB VGA Memory and 4MB Kernal Memory
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: Maps Virtual Addresses for VGA and Kernal code 
 *               to physical memory.
 */
void init_paging() {
    //All Pages initially Set to Not Present -- see x86_desc.S

    int i;
    for (i = 0; i < PT_SIZE; i++){
        memset(&user_vid_table[i], 0, sizeof(ptable_entry_t));
    }

    //Initialize Page Table Entry for VGA      
    page_table[VGA_PT_IDX].present = 1;         
    page_table[VGA_PT_IDX].rw = 1;
    PTAB_SET_ADDR(VGA_PT_IDX, VGA_BASE_ADDR);
    
    //Initialize Page Directory Entry for VGA
    page_directory[VGA_PD].present = 1;
    page_directory[VGA_PD].rw = 1;
    page_directory[VGA_PD].ps = 0; //4kb page
    PDIR_SET_ADDR(VGA_PD, page_table);

    //Initialize Page Directory Entry for VGA
    page_directory[USR_VGA_PD].present = 1;
    page_directory[USR_VGA_PD].rw = 1;
    page_directory[USR_VGA_PD].us = 1;
    page_directory[USR_VGA_PD].ps = 0; //4kb page
    PDIR_SET_ADDR(USR_VGA_PD, user_vid_table);

    //Initialize 4MB Page for Kernal Code
    page_directory[1].present = 1;
    page_directory[1].rw = 1;
    page_directory[1].ps = 1; //4mb page
    page_directory[1].g = 1; //dont flush kerneal code from virtual memory
    PDIR_SET_ADDR(KERNAL_PD_ENTRY, KERNAL_ADDR);

    //IA-32 Intel Page 57

    /* Page Size Extensions (bit 4 of CR4). Enables 4-MByte pages when set; restricts page
     * to 4 KBytes when clear. 
     */
    asm volatile ("         \n\
        movl %cr4, %eax     \n\
        orl $0x10, %eax     \n\
        movl %eax, %cr4"
    );


    /* Moves Physical address of the base of the page directory Into CR3*/
    asm volatile ("         \n\
        movl %0, %%eax      \n\
        movl %%eax, %%cr3"
        :
        : "r" (page_directory)
    );

    /*Enable PE and PG in CR0*/
    asm volatile ("             \n\
        movl %cr0, %eax         \n\
        orl $0x80000001, %eax   \n\
        movl %eax, %cr0"
    );
}

/*
 * map large
 * DESCRIPTION: Map virtual address to physical address
 * INPUTS:  v_addr -- virtual address to map from
 *          p_addr -- physical address to map to
 * OUTPUTS: none
 * RETURN VALUE: 0 for success
 * RESOURCES: https://wiki.osdev.org/Paging
 * SIDE EFFECTS: maps a new large page to the addresses specified.
 */
int32_t map_large(uint32_t* v_addr, uint32_t* p_addr)
{
    uint32_t dir_index = (uint32_t)v_addr >> DIR_BIT_OFF;

    /* fill page dir table accordingly */
    page_directory[dir_index].present = 1;
    page_directory[dir_index].rw = 1;
    page_directory[dir_index].us = 1;
    page_directory[dir_index].ps = 1; //4mb page
    PDIR_SET_ADDR(dir_index, p_addr);

    /* clear cache */
    FLUSH_TLB();

    return FSUCCESS;
}


/*
 * map_vmem
 * DESCRIPTION: Map video memory to physical address
 * INPUTS:  start -- location to map vmem to
 * OUTPUTS: none
 * RETURN VALUE: 0 for success
 * RESOURCES: https://wiki.osdev.org/Paging
 * SIDE EFFECTS: none
 */
int32_t map_vmem(uint8_t** start)
{
    /* set view as active and map */
    user_vid_table[0].present = 1;         
    user_vid_table[0].rw = 1;
    user_vid_table[0].us = 1;
    user_vid_table[0].addr = ((unsigned int) VGA_BASE_ADDR) >> PAGE_ALIGN_OFFSET;

    /* clear cache */
    FLUSH_TLB();

    *start = (uint8_t*)USER_VMEM;
    return FSUCCESS;
    
}


/*
 * unmap_small
 * DESCRIPTION: Unmap short virtual address from physical address
 * INPUTS:  v_addr -- virtual address to unmap
 * OUTPUTS: none
 * RETURN VALUE: 0 for success
 * RESOURCES: https://wiki.osdev.org/Paging
 * SIDE EFFECTS: none
 */
int32_t unmap_small(uint32_t* v_addr)
{
    uint32_t dir_index = (uint32_t)v_addr >> DIR_BIT_OFF;

    /* required for small */
    uint32_t table_index = ((uint32_t)v_addr >> PAGE_ALIGN_OFFSET) & (TABLE_BMASK);
    pdir_entry_t* table = (pdir_entry_t*)(page_directory[dir_index].addr << PAGE_ALIGN_OFFSET);

    /* label as removed from page directory */
    table[table_index].present = 0;

    /* clear cache */
    FLUSH_TLB();

    return FSUCCESS;
}

/*
 * unmap_large
 * DESCRIPTION: Unmap large virtual address from physical address
 * INPUTS:  v_addr -- virtual address to unmap
 * OUTPUTS: none
 * RETURN VALUE: 0 for success
 * RESOURCES: https://wiki.osdev.org/Paging
 * SIDE EFFECTS: none
 */
int32_t unmap_large(uint32_t* v_addr)
{
    uint32_t dir_index = (uint32_t)v_addr >> DIR_BIT_OFF;

    /* label as removed from page directory */
    page_directory[dir_index].present = 0;

    /* clear cache */
    FLUSH_TLB();

    return FSUCCESS;
}

