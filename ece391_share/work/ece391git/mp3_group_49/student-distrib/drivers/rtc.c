#include "rtc.h"

#include <lib.h>
#include <interrupts/i8259.h>
#include <syscall/process.h>

#define INDEX               0x70
#define CONFIG              0x71
#define REG_A               0x8A
#define REG_B               0x8B

#define NMI                 0x80
/* 
 * rate can be between 0-15
 * 0 to disable
 * 15 for slowest interrupts
 * 1,2 can be unstable based on hardware
 * use 3 for the fastest and safest max rate
 * using a maximum of 6 because of QUEMU restrictions
 * frequency = 32768 >> (rate - 1)
 */
#define MAX_RATE            6
#define FREQ_MAX            1024    /* rate=6 => freq=1024 */
#define FREQ_MIN            2       /* rate=15 => freq=2 */
#define INTERVAL            1000
#define CLEAR_TOP           0x0F
#define CLEAR_BOTTOM        0xF0
#define RTC_DEFAULT_FREQ    0x40    /* rate=6, frequency=1024Hz */
#define LOAD_REG_B_BITS     0XB
#define LOAD_REG_C_BITS     0XC

#define PIC_PIN_RTC         8

#define RTC_OPEN_FREQ       2
#define CLEAR               0

/* int interrupted states */
#define WAITING             0
#define INT_HIT             1

/* virtualized rtc variables, initialized during rtc_open */
static int vfreq;                               /* virtual frequency container */
static int icounter = 0;                        /* virtual interrupt counter */
volatile static int interrupted = WAITING;       /* "flag" for rtc_read */

/* uncomment to turn on debug mode for init_rtc */
// #define RTC_DEBUG


static fops_t rtc_ops = {rtc_open, rtc_close, rtc_read, rtc_write};

/*
 * init_rtc
 * DESCRIPTION: sets up rtc and sets the frequency rate
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void init_rtc() {
    int rate;
    char old_config;
    /* send the current rate (bottom 4 bits) to the controller */
    rate &= (int) CLEAR_TOP;

    /* Turn on the RTC with the default 1024Hz Rate */
    cli();
    outb(NMI | (int) LOAD_REG_B_BITS, INDEX);
    old_config = inb(CONFIG);
    outb(NMI | (int) LOAD_REG_B_BITS, INDEX);
    outb(old_config | ((int) RTC_DEFAULT_FREQ), CONFIG);
    sti();

    /*
     * set RTC frequency to its maximum 1024Hz (rate = 6)
     * this is used for virtualization
     * 
     * note that this is not required since max=default
     * but having this code segment makes it more adaptable
     */
    rate = MAX_RATE;
    cli();
    outb(REG_A, INDEX);
    /* read the current config because we only need to change the lower 4 bits */
    old_config = inb(CONFIG);
    outb(REG_A, INDEX);
    outb((old_config & (int)CLEAR_BOTTOM) + rate, CONFIG);
    sti();
    enable_irq(PIC_PIN_RTC);
}

/*
 * handle_rtc
 * DESCRIPTION: handles the rtc for test_interrupt as an example
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void handle_rtc() {
    /* 
     * throw away the contents of register C
     * so that the interrupts can be received
     */
    outb(LOAD_REG_C_BITS, INDEX);
    inb(CONFIG);

    /* virtualized rtc counter gets a hit */
    icounter++;
    if(vfreq*icounter >= FREQ_MAX){
        /* set flag for read_rtc */
        interrupted = INT_HIT;
        /* keep counter in check */
        icounter -= FREQ_MAX/vfreq;
    }

    /* do something every INTERVAL for testing */
    #ifdef RTC_DEBUG
        static int debug_track = 0;         /* debug interrupt counter */
        /*
         * pseudo-virtualization for of rtc for testing purposes
         * this code should only run if only testing RTC
         * run test_interrupts at every INTERVAL rtc interrupt
         * test_interrupts defined in lib.c
         */
        debug_track++;
        if(debug_track == ((int) INTERVAL)){
            test_interrupts();
            debug_track = 0;
        }
    #endif

    /* send eoi to PIC's RTC pin */
    send_eoi(PIC_PIN_RTC);
}

/* start of driver functions */

/*
 * rtc_open
 * DESCRIPTION: sets the virtual frequency to 2Hz
 * INPUTS: filename -- not used
 * OUTPUTS: none
 * RETURN VALUE: 0 on success
 * SIDE EFFECTS: clears the virtualized interrupt counter
 */
int32_t rtc_open(const uint8_t* filename){
    int i, fd;

    pcb_t* pcb = get_pcb_ptr();

    /* set global frequency */
    vfreq = RTC_OPEN_FREQ;
    icounter = 0;

    /* try to update fd, fail otherwise */
    for (i = 0; i < FILE_DESC_SIZE; i++) {
        if (!pcb->file_desc_array[i].flags) {
            fd = i;
            break;
        }
    }
    if (fd == -1){
        return FFAIL;
    }

    /* populate PCB accordingly */
    pcb->file_desc_array[fd].inode = NULL;
    pcb->file_desc_array[fd].flags = IN_USE;
    pcb->file_desc_array[fd].file_pos = 0;

    pcb->file_desc_array[fd].file_ops = &rtc_ops;

    return fd;
}

/*
 * rtc_read
 * DESCRIPTION: wait until the next RTC interrupt happens
 * INPUTS: fd     -- not used
 *         buf    -- not used
 *         nbytes -- not used
 * OUTPUTS: none
 * RETURN VALUE: 0 on success
 * SIDE EFFECTS: waits until interrupt
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes){
    /* set flag to help distinguish the next interrupt at virtual frequency */
    interrupted = WAITING;
    /* loop until enough interrupts happen */
    do{
        /* pause the processor until the next interrupt. */
        asm volatile("hlt");
    }while(interrupted == WAITING);
    
    return FSUCCESS;
}

/*
 * rtc_write
 * DESCRIPTION: write the frequency from the given buffer to RTC
 * INPUTS: fd     -- not used
 *         buf    -- buffer containing the frequency to set
 *         nbytes -- not used
 * OUTPUTS: none
 * RETURN VALUE: 0 on success, -1 on invalid input
 * SIDE EFFECTS: none
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes){
    uint32_t in_freq;

    /* parameter validation */
    if(buf == NULL){
        return FFAIL;
    }

    /* read new frequency, validate it */
    in_freq = *(uint32_t *)buf;

    /* check if the frequency is a valid int and power of 2 */
    if((in_freq & (in_freq-1)) != 0){
        /* power of 2 check */
        return FFAIL;
    }
    if((in_freq < FREQ_MIN) || (in_freq > FREQ_MAX)){
        /* bound check */
        return FFAIL;
    }

    /* change the virtual frequency */
    vfreq = in_freq;
    return FSUCCESS;
}

/*
 * rtc_close
 * DESCRIPTION: change global variables for virtual interrupts
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: 0 on success
 * SIDE EFFECTS: none
 */
int32_t rtc_close(int32_t fd){
    pcb_t* pcb;
    
    pcb = get_pcb_ptr();
    
    /* check if its already closed */
    if (!pcb->file_desc_array[fd].flags){
        return FFAIL;
    }

    /* mark as free */
    pcb->file_desc_array[fd].flags = !IN_USE;
    return FSUCCESS;
}
