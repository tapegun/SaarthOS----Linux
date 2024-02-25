
#ifndef TUXCTL_H
#define TUXCTL_H

#define TUX_SET_LED _IOR('E', 0x10, unsigned long)
#define TUX_READ_LED _IOW('E', 0x11, unsigned long*)
#define TUX_BUTTONS _IOW('E', 0x12, unsigned long*)
#define TUX_INIT_CONTROLLER _IOR('E', 0x13, unsigned long*)
#define TUX_LED_REQUEST _IOR('E', 0x14, unsigned long*)
#define TUX_LED_ACK _IOR('E', 0x15, unsigned long*)

#endif
