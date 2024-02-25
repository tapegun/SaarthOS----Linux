/*
 * Supported system calls as dictated by
 *      Appendix B of MP3 documentation
 * 
 * Numbered 1-10 as requested
 */


#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include <types.h>

/* required syscalls */
/* terminate a process */
int32_t halt (uint8_t status);
/* load and execute a new program */
int32_t execute (const uint8_t* command);
/* read data from keyboard, file, device, or directory */
int32_t read (int32_t fd, void* buf, int32_t nbytes);
/* write to terminal or device */
int32_t write (int32_t fd, const void* buf, int32_t nbytes);
/* access file system */
int32_t open (const uint8_t* filename);
/* close specified fd and make it available again */
int32_t close (int32_t fd);
/* read program's cmdl arguments into user level buffer */
int32_t getargs (uint8_t* buf, int32_t nbytes);
/* map text-mode video memory into user space at pre-set virtual address */
int32_t vidmap (uint8_t** screen_start);

/* extra credit syscalls */
/* change default action taken when a signal is received */
int32_t set_handler (int32_t signum, void* handler_address);
/* copy hardware context that was on user-level stack back onto processor */
int32_t sigreturn (void);

/* "number syscalls 1-10" */
enum syscall_list {
    SYS_HALT = 1,
    SYS_EXECUTE,
    SYS_READ,
    SYS_WRITE,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_GETARGS,
    SYS_VIDMAP,
    SYS_SET_HANDLER,
    SYS_SIGRETURN,
};


#endif
