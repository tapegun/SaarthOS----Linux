
/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
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
	
/* Global Variables */
unsigned char buttons;
unsigned int ledstatus;
unsigned int readledval;
unsigned int readledconf;
unsigned char readpacket[3];

/* array of seven-segment display codes */
char displaychars[16] = {0xE7, 0x06, 0xCB, 0x8F, 0x2E, 0xAD, 0xED, 0x86, 0xEF,
	0xAF, 0xEE, 0x6D, 0xE1, 0x4F, 0xE9, 0xE8};

	
/* local helper functions */
int tuxctl_ioctl_tuxinit(struct tty_struct* tty);
int tuxctl_ioctl_set_led (struct tty_struct* tty, unsigned long arg);
int tuxctl_ioctl_read_led (struct tty_struct* tty, unsigned long arg);
int tuxctl_ioctl_buttons(struct tty_struct* tty, unsigned long arg);
int tuxctl_ioctl_led_request(struct tty_struct* tty);
void parse_led_read_0(void);
void parse_led_read_1(void);

/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
	unsigned char left, down;
    unsigned a, b, c;

    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];

    //printk("packet : %x %x %x\n", a, b, c);
	
	switch (a) {
	case MTCP_BIOC_EVENT:
		/* parse packet into buttons global var */
		b = ~b;
		c = ~c;
		down = c;
		down = (down >> 2) % 2;
		left = c;
		left = (left >> 1) % 2;
		buttons = (b & 0x0F); //load low bits into buttons global var
		buttons = (c << 4) | buttons; //put high bits into buttons
		buttons = buttons & 0x9F;
		buttons = buttons | (left << 6); //switch 'left' and 'down' bits
		buttons = buttons | (down << 5);
		break;
	case MTCP_RESET:
		/* reinitialize tux controller and set LEDs using saved state */
		tuxctl_ioctl_tuxinit(tty);				// re-init
		tuxctl_ioctl_set_led(tty, ledstatus);	// set LEDs
		break;
	case 0x50:
		readpacket[0] = a;
		readpacket[1] = b;
		readpacket[2] = c;
		parse_led_read_0();
		break;
	case 0x51:
		readpacket[0] = a;
		readpacket[1] = b;
		readpacket[2] = c;
		parse_led_read_0();
		break;
	case 0x52:
		readpacket[0] = a;
		readpacket[1] = b;
		readpacket[2] = c;
		parse_led_read_0();
		break;
	case 0x53:
		readpacket[0] = a;
		readpacket[1] = b;
		readpacket[2] = c;
		parse_led_read_0();
		break;
	case 0x54:
		readpacket[0] = a;
		readpacket[1] = b;
		readpacket[2] = c;
		parse_led_read_1();
		break;
	case 0x55:
		readpacket[0] = a;
		readpacket[1] = b;
		readpacket[2] = c;
		parse_led_read_1();
		break;
	case 0x56:
		readpacket[0] = a;
		readpacket[1] = b;
		readpacket[2] = c;
		parse_led_read_1();
		break;
	case 0x57:
		readpacket[0] = a;
		readpacket[1] = b;
		readpacket[2] = c;
		parse_led_read_1();
		break;
	default:
		return;
	}
	/* clear packet data */
	a = 0x00;
	b = 0x00;
	c = 0x00;
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
	case TUX_INIT_CONTROLLER:
		return tuxctl_ioctl_tuxinit(tty);
	case TUX_BUTTONS:
		return tuxctl_ioctl_buttons(tty, arg);
	case TUX_SET_LED:
		return tuxctl_ioctl_set_led(tty, arg);
	case TUX_READ_LED:
		return tuxctl_ioctl_read_led(tty, arg);
		break;
	case TUX_LED_REQUEST:
		tuxctl_ioctl_led_request(tty);
		break;
	case TUX_LED_ACK:
		printk("ack: conf=%d\n", readledconf);
		if (readledconf == 1)
			return 0;
		else
			return -1;
		break;
	default:
	    return -EINVAL;
    }
	return 0;
}


void
parse_led_read_0(void)
{
	unsigned int dp, temp, i;
	unsigned a, b, c;
    a = readpacket[0]; // Avoid printk() sign extending the 8-bit
    b = readpacket[1]; // values when printing them.
    c = readpacket[2];
	
	readledval = 0;		// clear current contents of global var


	// check which character is in led0
	temp = (int)b;					// combine b and A0
	if (!(a % 2))
		temp = temp & 0x7F;
	temp = temp & 0xEF;

	for (i = 0; i < 16; i++)
	{
		if ((char)temp == displaychars[i])
			break;
	}

	// set correct bits in readledval
	readledval = readledval | i;
	
	/* set dp bits */
	temp = (int)b;
	dp = ((temp >> 4) % 2);
	if (dp)
		readledval = readledval | 0x01000000;
	

	// check which character is in led1
	temp = (int)c;					// combine c and A1
	if (!((a >> 1) % 2))
		temp = temp & 0x7F;
	temp = temp & 0xEF;
	for (i = 0; i < 16; i++)
	{
		if ((char)temp == displaychars[i])
			break;
	}
	// set correct bits in readledval
	readledval = readledval | (i << 4);
	
	/* set dp bits */
	temp = (int)c;
	dp = ((temp >> 4) % 2);
	if (dp)
		readledval = readledval | 0x02000000;
	return;
}

void
parse_led_read_1(void)
{
	unsigned int dp, temp, i;
	unsigned a, b, c;
    a = readpacket[0]; // Avoid printk() sign extending the 8-bit
    b = readpacket[1]; // values when printing them.
    c = readpacket[2];
	
	// check which character is in led2
	temp = (int)b;					// combine b and A2
	if (!(a % 2))
		temp = temp & 0x7F;
	temp = temp & 0xEF;

	for (i = 0; i < 16; i++)
	{
		if ((char)temp == displaychars[i])
			break;
	}

	// set correct bits in readledval
	readledval = readledval | (i << 8);
	
	/* set dp bits */
	temp = (int)b;
	dp = ((temp >> 4) % 2);
	if (dp)
		readledval = readledval | 0x04000000;
	
	// check which character is in led1
	temp = (int)c;					// combine c and A3
	if (!((a >> 1) % 2))
		temp = temp & 0x7F;
	temp = temp & 0xEF;
	for (i = 0; i < 16; i++)
	{
		if ((char)temp == displaychars[i])
			break;
	}
	// set correct bits in readledval
	readledval = readledval | (i << 12);
	
	/* set dp bits */
	temp = (int)c;
	dp = ((temp >> 4) % 2);
	if (dp)
		readledval = readledval | 0x08000000;
		
	/* set led on bits */
	if ((((readledval >> 24) % 2) == 1) || ((readledval & 0xF) != 0))
		readledval = readledval | 0x00010000;
	if ((((readledval >> 25) % 2) == 1) || ((readledval & (0xF << 4)) != 0))
		readledval = readledval | 0x00020000;
	if ((((readledval >> 26) % 2) == 1) || ((readledval & (0xF << 8)) != 0))
		readledval = readledval | 0x00040000;
	if ((((readledval >> 27) % 2) == 1) || ((readledval & (0xF << 12)) != 0))
		readledval = readledval | 0x00080000;
	
	readledconf = 1;	//	 acknowlege receipt of packets
	return;
}

int
tuxctl_ioctl_led_request(struct tty_struct* tty)
{
	char buf[1];
	buf[0] = MTCP_POLL_LEDS;
	tuxctl_ldisc_put(tty, buf, 1);
	return 0;
}

int
tuxctl_ioctl_tuxinit(struct tty_struct* tty)
{
	char buf[1];
	buf[0] = MTCP_BIOC_ON;
	tuxctl_ldisc_put(tty, buf, 1);
	return 0;
}

int
tuxctl_ioctl_buttons(struct tty_struct* tty, unsigned long arg)
{
	int ret;
	unsigned int temp;
	if ((int*)arg == NULL)
		return -EFAULT;
	temp = 0;
	temp = temp | buttons;
	ret = copy_to_user((int*)arg, &temp, 4);
	if (ret > 0)
		return -EFAULT;
	else
		return 0;
	//*(int*)arg = buttons;
}

int
tuxctl_ioctl_read_led (struct tty_struct* tty, unsigned long arg)
{
	int ret;
	if ((int*)arg == NULL)
		return -EFAULT;
	ret = copy_to_user((int*)arg, &readledval, 4);
	readledconf = 0;
	readledval = 0;
	if (ret > 0)
		return -EFAULT;
	else
		return 0;
}

int
tuxctl_ioctl_set_led (struct tty_struct* tty, unsigned long arg)
{
	unsigned int  dp, onLED, ledVal, count, ledHex;
	int bufferSize, i;
	char temp;
	char buffer[10];
	
	// put LED display into user-mode
	char buf[1];
	buf[0] = MTCP_LED_USR;
	tuxctl_ldisc_put(tty, buf, 1);

	// parse argument 
	dp = ((0xF << 24) & arg) >> 24;
	onLED = ((0xF << 16) & arg) >> 16;
	ledVal = (0xFFFF & arg);
	
	// calculate size of buffer
	bufferSize = 2;
	count = onLED;
	for (i = 0; i < 4; i++)
	{
		if (count % 2)
		{
			bufferSize++;
		}
		count = count >> 1;
	}
		
	// create buffer to transmit to Tux Controller
	buffer[0] = MTCP_LED_SET;
	buffer[1] = onLED;
	for (i = 0; i < 4; i++)
	{
		if (onLED % 2)
		{
			// if LED should be on get value 
			ledHex = ((0xF << (i * 4)) & ledVal)>>(i*4);
			temp = displaychars[ledHex];
			
			// check if dp should be turned on 
			if (dp % 2)
			{
				temp = (temp | 0x10);
			}
			buffer[2 + i] = temp;
		}
		
		onLED = onLED >> 1;
		dp = dp >> 1;
	}
	
	// save led state in ledstatus global variable
	ledstatus = arg;

	// write LED values to Tux Controller 
	tuxctl_ldisc_put(tty, buffer, bufferSize);
	return 0;
}
