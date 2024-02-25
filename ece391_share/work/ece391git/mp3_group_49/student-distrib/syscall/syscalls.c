#include "syscalls.h"
#include "process.h"

#include <lib.h>
#include <drivers/fs.h>
#include <paging.h>
#include <drivers/terminal.h>
#include <drivers/rtc.h>
#include <x86_desc.h>

/*
 * halt
 * DESCRIPTION: terminate a process
 * INPUTS: status -- value specified to return to callee
 * OUTPUTS: none
 * RETURN VALUE: 0 on success, -1 on bad pcb config
 * SIDE EFFECTS: unmaps the program memory
 */
int32_t halt(uint8_t status)
{
    return end_process(status);
}

/*
 * execute
 * DESCRIPTION: load and execute a new program
 * INPUTS: command -- input command received from user
 * OUTPUTS: none
 * RETURN VALUE: -1 on invalid input, status on success
 * SIDE EFFECTS: none
 */
int32_t execute (const uint8_t* command)
{
    return create_process(command);
}

/*
 * read
 * DESCRIPTION: read data from keyboard, file, device, or directory
 * INPUTS:  fd -- file descriptor to read from
 *          buf -- buffer to read to
 *          nbytes -- number of bytes to read
 * OUTPUTS: none
 * RETURN VALUE: PCB pointer
 * SIDE EFFECTS: none
 */
int32_t read (int32_t fd, void* buf, int32_t nbytes)
{
    pcb_t* pcb = get_pcb_ptr();

    /* parameter validation */
    if (!buf)
        return FFAIL;
    if (fd < 0 || fd >= FILE_DESC_SIZE || fd == STDOUT)
        return FFAIL;
    if (pcb->file_desc_array[fd].flags == !IN_USE)
        return FFAIL;
    
    /* use the appropriate read function */
    return pcb->file_desc_array[fd].file_ops->read(fd, buf, nbytes);
}

/*
 * write
 * DESCRIPTION: write to terminal or device
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: number of bytes written successfully
 * SIDE EFFECTS: none
 */
int32_t write (int32_t fd, const void* buf, int32_t nbytes)
{
    pcb_t* pcb = get_pcb_ptr();

    /* parameter validation */
    if (!buf)
        return FFAIL;
    if (fd < 0 || fd >= FILE_DESC_SIZE || fd == STDIN)
        return FFAIL;
    if (pcb->file_desc_array[fd].flags == !IN_USE)
        return FFAIL;

    /* use the appropriate write function */
    return pcb->file_desc_array[fd].file_ops->write(fd, buf, nbytes);
}

/*
 * open
 * DESCRIPTION: access file system
 * INPUTS: filename -- file name (and directory)
 * OUTPUTS: none
 * RETURN VALUE: -1
 * SIDE EFFECTS: not implemented
 */
int32_t open (const uint8_t* filename)
{
    dentry_t dentry;
    int fd;

    /* parameter validation */
    if (!filename)
        return FFAIL;
    if (read_dentry_by_name(filename, &dentry)){
        return FFAIL;
    }

    /* set pcb functions based on directory entries */
    switch (dentry.filetype) {
        case(DENT_RTC):
            fd = rtc_open(filename);
            break;
        case(DENT_FILE):
            fd = directory_open(filename);
            break;
        case(DENT_DIR):
            fd = file_open(filename);
            break;
    }

    if (fd == -1)
        return FFAIL;

    return fd;
}

/*
 * close
 * DESCRIPTION: close specified fd and make it available
 * INPUTS: fd -- file descriptor to close
 * OUTPUTS: none
 * RETURN VALUE: -1 on fail, 0 on successful closing
 * SIDE EFFECTS: not implemented
 */
int32_t close (int32_t fd)
{
    pcb_t* pcb = get_pcb_ptr();

    /* parameter validation */
    if (fd < 0 || fd >= FILE_DESC_SIZE)
        return FFAIL;
    if (pcb->file_desc_array[fd].flags == !IN_USE){
        return FFAIL;
    }

    /* use the appropriate close function */
    return pcb->file_desc_array[fd].file_ops->close(fd);
}

/*
 * getargs
 * DESCRIPTION: read program's cmdl arguments into user level buffer
 * INPUTS:  buf -- user level buffer to write to
 *          nbytes -- number of bytes to read
 * OUTPUTS: none
 * RETURN VALUE: -1
 * SIDE EFFECTS: not implemented
 */
int32_t getargs (uint8_t* buf, int32_t nbytes)
{
    pcb_t* pcb = get_pcb_ptr();
    int length = strlen((int8_t*)pcb->args);

    /* parameter validation */
    if (!buf)
        return FFAIL;
    if (nbytes < 0)
        return FFAIL;

    /* cap length */
    if (length > nbytes)
        length = nbytes;

    /* copy args to buf and add null termination */
    strncpy((int8_t*)buf, (int8_t*)pcb->args, length);
    buf[length] = NULL;

    return FSUCCESS;
}

/*
 * vidmap
 * DESCRIPTION: map text-mode video memory into
 *              user space at pre-set virtual address
 * INPUTS: screen start -- pointer to the start of video memory
 * OUTPUTS: none
 * RETURN VALUE: -1
 * SIDE EFFECTS: not implemented
 */
int32_t vidmap (uint8_t** screen_start)
{
    pcb_t* pcb = get_pcb_ptr();

    /* parameter validation */
    if (!screen_start)
        return FFAIL;
    if (screen_start < (uint8_t**)PROGRAM_SEGMENT || screen_start > (uint8_t**)USER_VMEM)
        return FFAIL;

    /* update flag */
    pcb->is_vidmapped = 1;

    map_vmem(screen_start);
    return FSUCCESS;
}

/*
 * set_handler
 * DESCRIPTION: change default action taken when
 *              a signal is received
 * INPUTS:  signum          -- NYI
 *          handler_address -- NYI
 * OUTPUTS: none
 * RETURN VALUE: -1 on fail
 * SIDE EFFECTS: not implemented, optional
 */
int32_t set_handler (int32_t signum, void* handler_address)
{
    return FFAIL;
}

/*
 * sigreturn
 * DESCRIPTION: copy hardware context that was on
 *              user-level stack back onto processor
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: -1 on fail
 * SIDE EFFECTS: not implemented, optional
 */
int32_t sigreturn (void)
{
    return FFAIL;
}
