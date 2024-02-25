#include "terminal.h"
#include "keyboard.h"

#include <lib.h>

#define MAX_BUF_FILL    128


static int8_t terminal_buf[MAX_BUF_FILL];


// helper vars for terminal using keyboard
static short in_terminal = 0;       // see if we even need to keep track of backspacing too much... set the to true if you wanna 
                                    // not allow it to backspace at a certain point... also set the depth to 0


/* SYSCALL FUNCTIONS */

/* terminal_read
 * Read up to size-1 characters (127 max) before the the enter key is pressed.
 * A newline character is automatically added.
 * Inputs: int8_t* buf - where inputted characters are stored
 *         uint32_t n - number of characters to read (max buf size minus 1) 
 * Return Value: number of characters read on success, -1 on failure
 * Function: reads keyboard input 
 */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes)
{
    /* null parameter check */
    in_terminal = 1;
    if (buf == NULL){
		return FFAIL;
	}
    if(nbytes > MAX_BUF_FILL){
        nbytes = ((int32_t)MAX_BUF_FILL)-1;
    }
    uint32_t i = 0;
    while (i <= nbytes && (terminal_buf[i] = keyboard_get_key())) {
        if (terminal_buf[i] == '\n'){
            putc('\n');
            break;
        }

        if (terminal_buf[i] == '\b') {
            /* handle backspace */
            if(i <= 0){
                continue;
            }
            putc('\b');
            i--;     
            terminal_buf[i] = NULL;
            continue;
        }else{
            ((int8_t*)buf)[i] = terminal_buf[i];
            putc(((int8_t*)buf)[i]);
            i++;
        }
            // find the location of the cursor using the get function I made in lib.h
            // change the value in video mem of that location to ' '
            // change the cursor using the set function I made
            // actually in case we aren't the first backspace in the set, we can just delete the previous character in the buffers
    }
    int x = 0;
    for(; x < MAX_BUF_FILL; x++){
        terminal_buf[x] = 0;
    }
    /* emd the line and keep filling */
    ((int8_t*)buf)[i++] = '\n';
    for(x = i + 1; x < MAX_BUF_FILL; x++){
        ((int8_t*)buf)[x] = 0;
    }
    in_terminal = 0;
    return i;
}


/* terminal_write   
 * write characters to terminal
 * Inputs: int8_t* buf - buffer of characters to write
 *         uint32_t n - number of characters to write
 * Return Value: 0 on success, -1 on failure
 * Function: writes to screen. Only stops after n chars written. 
 */
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes)
{
    int i = 0;
    /* write nbytes from buffer */
    for(; i < nbytes; i++){
        putc(((int8_t *)buf)[i]);
    }
    return FSUCCESS;
}

/* terminal_open
 * open terminal device
 * Inputs: uint8_t* filename
 * Outputs: none
 * Return Value: 0 on success
 * Function: none.
 */
int32_t terminal_open(const uint8_t* filename)
{
    return FSUCCESS;
}

/* terminal_close
 * close terminal device
 * Inputs: int32_t fd
 * Outputs: none
 * Return Value: 0 on success
 * Function: none.
 */
int32_t terminal_close(int32_t fd)
{
    return FFAIL;
}
