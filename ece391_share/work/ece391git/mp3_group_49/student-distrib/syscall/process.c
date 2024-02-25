#include "process.h"

#include <drivers/fs.h>
#include <drivers/terminal.h>
#include <lib.h>
#include <x86_desc.h>
#include <paging.h>

#define STACK_OFF       4
#define ELF_SIZE        4
#define MAX_SHELLS      3
#define MAX_PROCESS     6

static uint32_t pid = 0;    // temporary pid counter

static fops_t stdin_ops = {terminal_open, terminal_close, terminal_read, NULL};
static fops_t stdout_ops = {terminal_open, terminal_close, NULL, terminal_write};

/*
 * create_process
 * DESCRIPTION: create PCB entry, map the memory and set TSS values
 * INPUTS: command -- input command from syscall execute
 * OUTPUTS: none
 * RETURN VALUE: -1 on invalid input, status on success
 * SIDE EFFECTS: none
 */
int32_t create_process(const uint8_t* command)
{
    uint8_t filename[FNAME_MAX];
    uint8_t args[BUF_SIZE];
    int i = 0;
    int j = 0;
    dentry_t dentry;
    int8_t status;
    /* elf header magic number from https://wiki.osdev.org/ELF */
    char elf_magic[ELF_SIZE] = {0x7F, 'E', 'L', 'F'};
    char elf_buf[ELF_SIZE];

    /* null command check */
    if (!command){
        return FFAIL;
    }

    if (pid >= MAX_PROCESS){
    //if(pid >= MAX_SHELLS){
        return FFAIL;
    }

    /* create a filename string */
    while (command[i] != ' ' && command[i] != NULL && i<FNAME_MAX){
        filename[i] = command[i];
        i++;
    }
    /* null terminate it if size != max */
    if (i != FNAME_MAX){
        filename[i] = NULL;
    }

    i++;

    /* copy args from the remainder of command */
    while (command[i] != NULL && i<BUF_SIZE){
        args[j] = command[i];
        i++;
        j++;
    }

    /* ensure null termination */
    if (j != BUF_SIZE){
        args[j] = NULL;
    }

    /* check if dentry for that filename exists */
    if (read_dentry_by_name(filename, &dentry)){
        return FFAIL;
    }

    /* elf check */
    read_data(dentry.inode_num, 0, (uint8_t*)elf_buf, ELF_SIZE);
    for(i=0; i<ELF_SIZE; i++){
        if(elf_buf[i] != elf_magic[i]){
            return FFAIL;
        }
    }

    /* prepare to access file content */
    map_large((uint32_t*)PROGRAM_SEGMENT, (uint32_t*)(MB_8 + (pid)*MB_4));
    
    read_data(dentry.inode_num, 0, (uint8_t*)PROGRAM_ADDRESS, LARGE_SIZE);
    uint32_t file_entry_point = *((uint32_t*)(PROGRAM_ADDRESS + ELF_ENTRY_OFFSET));

    /* populate PCB struct for process */
    pcb_t* pcb = (pcb_t*)(MB_8 - KB_8 - KB_8*pid);
    pcb->pid = pid;
    pcb->parent_pid = get_pcb_ptr()->pid;

    memset(pcb->args, 0, 128);
    strncpy((int8_t*)pcb->args, (int8_t*)args, j);

    /* set file desc array values for STDIN and STDOUT, clear the rest */
    for (i = 0; i < FILE_DESC_SIZE; i++) {
        switch(i){
            case STDIN:
                pcb->file_desc_array[i].file_ops = &stdin_ops;
                pcb->file_desc_array[i].flags = IN_USE;
                break;
            case STDOUT:
                pcb->file_desc_array[i].file_ops = &stdout_ops;
                pcb->file_desc_array[i].flags = IN_USE;
                break;
            default:
                pcb->file_desc_array[i].flags = 0;
        }
    }

    /* populate tss */
    tss.ss0 = KERNEL_DS;
    tss.esp0 = MB_8 - KB_8*pid - STACK_OFF;

    /* increment pid and run process; decrement it after termination */
    pid++;

    /* push use data segment, user stack pointer,
     * flags, user code segment, entry point, then iret */
    asm volatile("           \n\
        movl %%ebp, %0       \n\
        movl %%esp, %1       \n\
        pushl $0x2B          \n\
        pushl $0x83ffffc     \n\
        pushfl               \n\
        pop %%eax            \n\
        orl $0x200, %%eax    \n\
        push %%eax           \n\
        pushl $0x23          \n\
        pushl %3             \n\
        call get_ret_addr    \n\
        iret                 \n\
        pop %2"
        : "=m"(pcb->parent_ebp), "=m"(pcb->parent_esp), "=m"(status)
        : "r"(file_entry_point)
        : "%eax"
    );

    pid--;

    return status;
}

/*
 * get_ret_addr
 * DESCRIPTION: get the address of the next instruction
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: 
 */
void get_ret_addr()
{
    uint32_t ret_eip;

    // get the return address from the stack. 
    asm volatile ("         \n\
    mov 20(%%esp), %0"
    : "=r"(ret_eip));

    // add 1 so it skips over the iret;
    ret_eip++;

    // don't pass in pcb pointers cause this should be easy to call from asm
    pcb_t* pcb = (pcb_t*)(MB_8 - KB_8 - KB_8*pid);
    pcb->execute_return = ret_eip;
}

/*
 * get_pcb_ptr
 * DESCRIPTION: get PCB pointer from stack
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: PCB pointer
 * SIDE EFFECTS: none
 */
pcb_t* get_pcb_ptr()
{
    pcb_t* pcb;

    /* get the PCB pointer passed from assembly */
    asm volatile("              \n\
        andl %%esp, %%eax       \n\
        movl %%eax, %0"
        : "=r"(pcb)
        : "a"(PCB_MASK)
    );

    return pcb;
}

/*
 * end_process
 * DESCRIPTION: kill process, remove from PCB
 * INPUTS: status -- value specified to return to halt
 * OUTPUTS: none
 * RETURN VALUE: 0 on success, -1 on bad pcb config
 * SIDE EFFECTS: unmaps the program memory
 */
int32_t end_process(uint8_t status) {

    pcb_t* pcb = get_pcb_ptr();

    /* restart the shell if the user quits the last layer */
    if (!pcb->execute_return) {
        if (pcb->pid == 0) {
            pid = 0;            // this feels like a hack...
            create_process((uint8_t*)"shell");
        }
    }

    if (pcb->is_vidmapped)
        unmap_small((uint32_t*)USER_VMEM);

    // unmap the current process memory before going back to the execute that called it
    // remap original program memory
    map_large((uint32_t*)PROGRAM_SEGMENT, (uint32_t*)(MB_8 + (get_pcb_ptr()->parent_pid)*MB_4));

    tss.esp0 = pcb->parent_esp;

    /* give the control back */
    asm volatile("              \
        mov %0, %%ebp           \n\
        mov %1, %%esp           \n\
        push %%ebx              \n\
        jmp *%%eax"
        : 
        :"m"(pcb->parent_ebp), "m"(pcb->parent_esp), "b"(status), "a"(pcb->execute_return)
    );
    return FSUCCESS;
}
