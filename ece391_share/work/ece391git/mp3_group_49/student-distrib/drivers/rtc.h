/*
 * Functions related to the Real Time Clock (RTC)
 * aka IRQ8, pin0 on the slave PIC
 * 
 * common reference:
 *      https://wiki.osdev.org/RTC
 * 
 * frequency calculation:
 *      RTC frequency = 32768 >> (rate - 1)
 * note that rate can be between any value in [0,15]
 * quick note about rate={0,1,2}:
 *      0   -- expected behaviour: turn off periodic interrupts
 *      1,2 -- unstable
 */

#ifndef RTC_H
#define RTC_H

#include <types.h>

/* initialize RTC with default values */
void init_rtc(void);

/* handle RTC interrupts */
void handle_rtc(void);

/* 
 *  RTC DRIVER FUNCTIONS
 * note that not all parameters are not required
 * but they are passed in through the function
 * pointer in syscalls.c nevertheless [WIP]
 */

/* sets the virtual frequency to 2HZ */
extern int32_t rtc_open(const uint8_t* filename);

/* wait until the next RTC interrupt happens */
extern int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);

/* write the new rate from the given buffer to RTC */
extern int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);

/* turn off the RTC driver */
extern int32_t rtc_close(int32_t fd);

#endif /* RTC_H */
