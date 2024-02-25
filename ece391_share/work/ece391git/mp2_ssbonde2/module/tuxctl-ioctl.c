/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"

#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)


int __tux_init__(struct tty_struct* tty);
int __tux_buttons__(struct tty_struct* tty, unsigned long arg);
int __tux_set_led__(struct tty_struct* tty, unsigned long arg);

unsigned char buttons;
unsigned int LED;
unsigned char readp_toggleacket[3];

static spinlock_t tux_lock = SPIN_LOCK_UNLOCKED;  // general purpose lock for this sys call.

/* array of seven-segment display codes */
char numbers_disp[16] = {0xE7, 0x06, 0xCB, 0x8F, 0x2E, 0xAD, 0xED, 0x86, 0xEF, 0xAF, 0xEE, 0x6D, 0xE1, 0x4F, 0xE9, 0xE8};



	

// /************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
    unsigned char l_val, d_val;   // left value and down value  these two are enough to encoded our information about direction.
    unsigned a, b, c;

    char buf[2];

    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];

    //printk("packet : %x %x %x\n", a, b, c);
	
	switch (a) {
	case MTCP_BIOC_EVENT:



    // RTDC for how to format the buttons.. we are using unsigned for buttons because 8 byte value.  
		b = ~b;
		c = ~c;
		d_val = c;
		d_val = (d_val >> 2) % 2;
		l_val = c;
		l_val = (l_val >> 1) % 2;
		buttons = (b & 0x0F); 
		buttons = (c << 4) | buttons; 
		buttons = buttons & 0x9F;
		buttons = buttons | (l_val << 6);
		buttons = buttons | (d_val << 5);
		break;

	case MTCP_RESET:          // reset with init and led
	  buf[0] = MTCP_BIOC_ON;
	  buf[1] = MTCP_LED_USR;
	  tuxctl_ldisc_put(tty, buf, 2); // start the machine and LED again 
		break;
	default:
		return;
	}
	return;
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{
    switch (cmd) {
	case TUX_INIT:{return __tux_init__(tty);}
	case TUX_BUTTONS:{return __tux_buttons__(tty, arg);}
	case TUX_SET_LED:{return __tux_set_led__(tty, arg);}
	case TUX_LED_ACK:           // didn't have time.
	case TUX_LED_REQUEST:
	case TUX_READ_LED:
	default:
	    return -EINVAL;
    }
}	

/*
 * __tux_init__
 *   DESCRIPTION: function that initializes the tux controller
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */

int __tux_init__(struct tty_struct* tty)
{
  char buf[1];
	buf[0] = MTCP_BIOC_ON;
  tuxctl_ldisc_put(tty, buf, 1);
	return 0;
}


/*
 * __tux_buttons__
 *   DESCRIPTION: function that interfaces with the buttons
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int __tux_buttons__(struct tty_struct* tty, unsigned long arg){
  spin_lock(&tux_lock);
  int tmp = buttons;
  if(copy_to_user((int *)arg, &tmp, sizeof(int)) != 0){
    spin_unlock(&tux_lock);       // don't keep the lock
    return -EFAULT;
  }
  spin_unlock(&tux_lock);
  return 0;
}	

/*
 * __tux_set_led__
 *   DESCRIPTION: function that interfaces with the LED
 *   INPUTS: tty struct and the arg
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int __tux_set_led__(struct tty_struct* tty, unsigned long arg){
  unsigned int  dp_toggle, LED_toggle, ledVal, count, ledHex;
	int bufLength, i;
	char temp;
	char buffer[10];
	
	char temp_buf[1];
	temp_buf[0] = MTCP_LED_USR;          // prepare the LED to take input
	tuxctl_ldisc_put(tty, temp_buf, 1);

	dp_toggle = (arg & 0x0F000000)>>24;
	LED_toggle = (arg & 0x000F0000)>>16;
	ledVal = (0xFFFF & arg);  // do the bitwise and to get a copy 
	
	bufLength = 2;
	count = LED_toggle;
	for (i = 0; i < 4; i++) // the different LED spots
	{
		if (count % 2)  
		{
			bufLength++;     // finding the size of the buffer 
		}
		count = count >> 1;
	}
	buffer[0] = MTCP_LED_SET;     // loading primary values according to the documentation
	buffer[1] = LED_toggle;
	for (i = 0; i < 4; i++)
	{
		if (LED_toggle % 2)
		{
			ledHex = ((0xF << (i * 4)) & ledVal)>>(i*4); // calculating the hex then finding it in our array that holds the values
			temp = numbers_disp[ledHex];                // now that we have the mapped value  we should load this into our buffer
			
			if (dp_toggle % 2)      // see if dp needs to  be on 
			{
				temp = (temp | 0x10);               // if so modify the value we load into our buffer by using the bitwise or operator
			}
			buffer[2 + i] = temp;                 //loading into our buffer
		}
		
		LED_toggle = LED_toggle >> 1;           // shift both vars to the right
		dp_toggle = dp_toggle >> 1;
	}
	
	LED = arg;

	tuxctl_ldisc_put(tty, buffer, bufLength);
	return 0;
}	

