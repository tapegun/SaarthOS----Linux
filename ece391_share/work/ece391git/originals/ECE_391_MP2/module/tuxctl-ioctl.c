/*
 * tuxctl-ioctl.c
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

char setLed(unsigned long arg, char dp);

static spinlock_t lock_tux = SPIN_LOCK_UNLOCKED;
unsigned button_info;
int ack;
unsigned long time_backup;

#define debug(str, ...) printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)

/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in
 * tuxctl-ld.c. It calls this function, so all warnings there apply
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet) {
    unsigned a, b, c;

    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];

    switch (a)
    {
        case MTCP_ACK:
        {
          spin_lock(&lock_tux);
          ack = 0;
          spin_unlock(&lock_tux);
          break;
        }

        case MTCP_BIOC_EVENT:
        {
          char switch1 = 0x00; // helpers to switch left and down bit back
          char switch2 = 0x00;
          char temp = c;
          int cba = temp & 0x09;
          char abc = b;
          switch1 = (temp & 0x04)>>1;
          switch2 = (temp & 0x02)<<1;
          cba = cba | switch1;
          cba = cba | switch2;
          cba = cba<<4;
          abc = abc & 0x0F;
          spin_lock(&lock_tux);
          button_info = cba + abc;
          spin_unlock(&lock_tux);
          break;
        }

        case MTCP_RESET:
        {
          unsigned long temporary;
          int index = 0;
          unsigned long temp1;
          char bitmask;
          int a;
          unsigned long temp;
          char buffer[10];
          buffer[0] = MTCP_LED_USR;
          buffer[1] = MTCP_BIOC_ON;
          a = tuxctl_ldisc_put(tty, buffer, 2);
          if (a != 0)
          {
            return;
          }
          spin_lock(&lock_tux);
          temp = time_backup;
          if (ack == 1)
          {
            spin_unlock(&lock_tux);
            return;
          }
          spin_unlock(&lock_tux);

          spin_lock(&lock_tux);
          temporary = time_backup;
          button_info = 0;
          // ack = 1;
          if (ack == 1)
          {
            spin_unlock(&lock_tux);
            return;
          }
          spin_unlock(&lock_tux);
          temp1 = temporary>>16;
          bitmask = temp1 & 0x0F;
          buffer[0] = MTCP_LED_SET;
          buffer[1] = 0x0F;
          index = 2;
          if (bitmask & 0x08) // set command of leds in order
          {
            char led3 = setLed(temporary & 0x0F, temporary>>20 & 0x10);
            // char led3 = 0xFF;
            buffer[index] = led3;
            index++;
          }
          else
          {
            char led3 = setLed(0xFF, temporary>>20 & 0x10);
            // char led3 = 0xFF;
            buffer[index] = led3;
            index++;
          }
          if (bitmask & 0x04)
          {
            char led2 = setLed(temporary>>4 & 0x0F, temporary>>21 & 0x10);
            // char led2 = 0xFF;
            buffer[index] = led2;
            index++;
          }
          else
          {
            char led2 = setLed(0xFF, temporary>>21 & 0x10);
            // char led3 = 0xFF;
            buffer[index] = led2;
            index++;
          }
          if (bitmask & 0x02)
          {
            char led1 = setLed(temporary>>8 & 0x0F, temporary>>22 & 0x10);
            // char led1 = 0xFF;
            buffer[index] = led1;
            index++;
          }
          else
          {
            char led1 = setLed(0xFF, temporary>>22 & 0x10);
            // char led3 = 0xFF;
            buffer[index] = led1;
            index++;
          }
          if (bitmask & 0x01)
          {
            char led0 = setLed(temporary>>12 & 0x0F, temporary>>23 & 0x10);
            // char led0 = 0xFF;
            buffer[index] = led0;
            index++;
          }
          else
          {
            char led0 = setLed(0xFF, temporary>>23 & 0x10);
            // char led3 = 0xFF;
            buffer[index] = led0;
            index++;
          }
          tuxctl_ldisc_put(tty, buffer, index); //index);
          spin_lock(&lock_tux);
          ack = 1;
          spin_unlock(&lock_tux);
          break;
        }
    }
    return;
    /*printk("packet : %x %x %x\n", a, b, c); */
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

int tuxctl_ioctl(struct tty_struct* tty, struct file* file,
                 unsigned cmd, unsigned long arg) {
    char buffer[10];
    switch (cmd)
    {
        case TUX_INIT:
        {
            int a;
            spin_lock(&lock_tux);
            button_info = 0;
            time_backup = 0;
            spin_unlock(&lock_tux);
            buffer[0] = MTCP_LED_USR;
            buffer[1] = MTCP_BIOC_ON;
            a = tuxctl_ldisc_put(tty, buffer, 2);
            spin_lock(&lock_tux);
            ack = 1;
            spin_unlock(&lock_tux);
            if (a != 0)
            {
              return -EINVAL;
            }
            // printk("%d", a);
            break;
        }

        case TUX_BUTTONS:
        {
            // printk("1");
            int *temp;
            spin_lock(&lock_tux);
            temp = (int *)arg;
            copy_to_user(temp, &button_info, sizeof(int));
            spin_unlock(&lock_tux);
            //*temp = *temp | button_info;
            break;
        }

        case TUX_SET_LED:
        {
            int a;
            int index = 0;
            char bitmask;
            unsigned long temp, temp1;
            // printk("set led\n");
            spin_lock(&lock_tux);
            time_backup = arg;
            if (ack == 1)
            {
              spin_unlock(&lock_tux);
              return -EINVAL;
            }
            spin_unlock(&lock_tux);
            temp1 = arg;
            temp = temp1>>16;
            bitmask = temp & 0x0F;
            buffer[0] = MTCP_LED_SET;
            buffer[1] = 0x0F; // always set all the led
            index = 2;
            if (bitmask & 0x08) // set command of leds in order
            {
              char led3 = setLed(temp1 & 0x0F, temp1>>20 & 0x10);
              // char led3 = 0xFF;
              buffer[index] = led3;
              index++;
            }
            else
            {
              char led3 = setLed(0xFF, temp1>>20 & 0x10);
              // char led3 = 0xFF;
              buffer[index] = led3;
              index++;
            }
            if (bitmask & 0x04)
            {
              char led2 = setLed(temp1>>4 & 0x0F, temp1>>21 & 0x10);
              // char led2 = 0xFF;
              buffer[index] = led2;
              index++;
            }
            else
            {
              char led2 = setLed(0xFF, temp1>>21 & 0x10);
              // char led3 = 0xFF;
              buffer[index] = led2;
              index++;
            }
            if (bitmask & 0x02)
            {
              char led1 = setLed(temp1>>8 & 0x0F, temp1>>22 & 0x10);
              // char led1 = 0xFF;
              buffer[index] = led1;
              index++;
            }
            else
            {
              char led1 = setLed(0xFF, temp1>>22 & 0x10);
              // char led3 = 0xFF;
              buffer[index] = led1;
              index++;
            }
            if (bitmask & 0x01)
            {
              char led0 = setLed(temp1>>12 & 0x0F, temp1>>23 & 0x10);
              // char led0 = 0xFF;
              buffer[index] = led0;
              index++;
            }
            else
            {
              char led0 = setLed(0xFF, temp1>>23 & 0x10);
              // char led3 = 0xFF;
              buffer[index] = led0;
              index++;
            }
            a = tuxctl_ldisc_put(tty, buffer, index);
            spin_lock(&lock_tux);
            ack = 1;
            spin_unlock(&lock_tux);
            // printk("%d", a);
            if (a != 0)
            {
              return -EINVAL;
            }
            break;
        }

        default:
        {
            return -EINVAL;
        }
    }
    return 0;
}


/*
 * setLed
 *   DESCRIPTION: Sets the hexdecimal values, along with dp into the format to be
 *                sent into tux
 *   INPUTS: arg: hexdecimal value want to print on LED
 *           dp: set dp on or off
 *   OUTPUTS: NONE
 *   RETURN VALUE: 1 byte command for the tux
 *   SIDE EFFECTS: NONE
 */
char setLed(unsigned long arg, char dp)
{
    char led;
    switch (arg)
    {
      case 0x00:
        led = 0xE7;
        led = led | dp;
        break;
      case 0x01:
        led = 0x06;
        led = led | dp;
        break;
      case 0x02:
        led = 0xCB;
        led = led | dp;
        break;
      case 0x03:
        led = 0x8F;
        led = led | dp;
        break;
      case 0x04:
        led = 0x2E;
        led = led | dp;
        break;
      case 0x05:
        led = 0xAD;
        led = led | dp;
        break;
      case 0x06:
        led = 0xED;
        led = led | dp;
        break;
      case 0x07:
        led = 0x86;
        led = led | dp;
        break;
      case 0x08:
        led = 0xEF;
        led = led | dp;
        break;
      case 0x09:
        led = 0xAF;
        led = led | dp;
        break;
      case 0x0A:
        led = 0xEE;
        led = led | dp;
        break;
      case 0x0B:
        led = 0x6D;
        led = led | dp;
        break;
      case 0x0C:
        led = 0xE1;
        led = led | dp;
        break;
      case 0x0D:
        led = 0x4F;
        led = led | dp;
        break;
      case 0x0E:
        led = 0xE9;
        led = led | dp;
        break;
      case 0x0F:
        led = 0xE8;
        led = led | dp;
        break;
      default:
        led = 0x00;
    }
    return led;
}
