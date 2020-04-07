#include <linux/fs.h>               // struct file, struct file_operations
#include <linux/init.h>             // for __init, see code
#include <linux/module.h>           // for module init and exit macros
#include <linux/miscdevice.h>       // for misc_device_register and struct miscdev
#include <linux/uaccess.h>          // for copy_to_user, see code
#include <asm/io.h>                 // for mmap
#include "../address_map_arm.h"
/* Kernel character device driver. By default, this driver provides the text "Hello from 
 * chardev" when /dev/chardev is read (for example, cat /dev/chardev). The text can be changed 
 * by writing a new string to /dev/chardev (for example echo "New message" > /dev/chardev).
 * This version of the code uses copy_to_user and copy_from_user, to send data to, and receive
 * date from, user-space */

static int device_open (struct inode *, struct file *);
static int device_release (struct inode *, struct file *);

static ssize_t HEX_device_write (struct file *, const char *, size_t, loff_t *);

static ssize_t LEDR_device_write (struct file *, const char *, size_t, loff_t *);

static struct file_operations HEX_fops = {
    .owner = THIS_MODULE,
    //.read = KEY_device_read,
    .write = HEX_device_write,
    .open = device_open,
    .release = device_release
};

static struct file_operations LEDR_fops = {
    .owner = THIS_MODULE,
    // .read = SW_device_read,
    .write = LEDR_device_write,
    .open = device_open,
    .release = device_release
};

#define SUCCESS 0

static struct miscdevice LEDR = { 
    .minor = MISC_DYNAMIC_MINOR, 
    .name = "LEDR",
    .fops = &LEDR_fops,
    .mode = 0666
};

static struct miscdevice HEX = { 
    .minor = MISC_DYNAMIC_MINOR, 
    .name = "HEX",
    .fops = &HEX_fops,
    .mode = 0666
};

static int HEX_registered = 0;
static int LEDR_registered = 0;

#define MAX_SIZE 256                // we assume that no message will be longer than this
static char HEX_msg[MAX_SIZE];  // the character array that can be read
static char LEDR_msg[MAX_SIZE];  // the character array that can be read

volatile int * HEX_3_0_addr;
volatile int * HEX_5_4_addr;
volatile int * LEDR_addr;
void * LW_virtual;

static int __init start_module(void)
{
	int err;
	LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
	HEX_3_0_addr = (int *) (LW_virtual) + (HEX3_HEX0_BASE/4);
	HEX_5_4_addr = (int *) (LW_virtual) + (HEX5_HEX4_BASE/4);
	LEDR_addr    = (int *) (LW_virtual) + (LEDR_BASE/4);
	*LEDR_addr = 0;
	*HEX_3_0_addr = 0;
	*HEX_5_4_addr = 0;
    err = misc_register (&LEDR);
    if (err < 0) {
        printk (KERN_ERR "misc_register() for LEDR failed\n");
    }
    else {
        printk (KERN_INFO "driver LEDR registered\n");
        LEDR_registered = 1;
    }
	err = misc_register (&HEX);
	if (err < 0) {
        printk (KERN_ERR "misc_register() for HEX failed\n");
    }
    else {
        printk (KERN_INFO "driver HEX registered\n");
        HEX_registered = 1;
    }

    return err;
}

static void __exit stop_module(void)
{
    *LEDR_addr = 0;
    *HEX_3_0_addr = 0;
    *HEX_5_4_addr = 0;
    if (LEDR_registered) {
        misc_deregister (&LEDR);

        printk (KERN_INFO "driver LEDR de-registered\n");
    }
	
	if (HEX_registered) {
        misc_deregister (&HEX);

        printk (KERN_INFO "driver HEX de-registered\n");
    }
}

/* Called when a process opens chardev */
static int device_open(struct inode *inode, struct file *file)
{
    return SUCCESS;
}

/* Called when a process closes chardev */
static int device_release(struct inode *inode, struct file *file)
{
    return 0;
}

unsigned int parse_character_hex (char c)
{
	if ('0' <= c && '9'>=c) {return((int)(c - '0'));}
	if ('a' <= c && 'f'>=c) {return((int)(c - 'a' + 10));}
	if ('A' <= c && 'F'>=c) {return((int)(c - 'A' + 10));}
	return 0;
}

/* Called when a process writes to chardev. Provides character data from chardev_msg.
*/
static ssize_t LEDR_device_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
    size_t bytes;
	int value;
	int i;
	bytes = length;

	if (bytes > MAX_SIZE - 1)    // can copy all at once, or not?
		bytes = MAX_SIZE - 1;
	if (copy_from_user (LEDR_msg, buffer, bytes) != 0)
		printk (KERN_ERR "Error: copy_from_user unsuccessful");
	value = 0;
	for (i=0; i<bytes-1; i++)
	{
		value = value * 16 + parse_character_hex(LEDR_msg[i]);
	}
	*LEDR_addr = value;
	return bytes;
}

int get_hex(int value)
{
	switch (value)
	{
		case 0:
		{
			return 63;
		}
		case 1:
		{
			return 6;
		}
		case 2:
		{
			return 91;
		}
		case 3:
		{
			return 79;
		}
		case 4:
		{
			return 102;
		}
		case 5:
		{
			return 109;
		}
		case 6:
		{
			return 124;
		}
		case 7:
		{
			return 7;
		}
		case 8:
		{
			return 127;
		}
		case 9:
		{
			return 111;
		}
	}
	return 0;
}

static ssize_t HEX_device_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
    size_t bytes;
	int value_high=0;
	int value_low=0;
	int index=0;
	bytes = length;

	if (bytes > MAX_SIZE - 1)    // can copy all at once, or not?
		bytes = MAX_SIZE - 1;
	if (copy_from_user (HEX_msg, buffer, bytes) != 0)
		printk (KERN_ERR "Error: copy_from_user unsuccessful");
	if (bytes >=7)
	{
		value_high += (get_hex(HEX_msg[index++] - '0'))<<8;
	}
	else
	{
		value_high += (get_hex(0))<<8;
	}
	if (bytes >=6)
	{
		value_high += (get_hex(HEX_msg[index++] - '0'));
	}
	else
	{
		value_high += (get_hex(0));
	}
	if (bytes >=5)
	{
		value_low += (get_hex(HEX_msg[index++] - '0'))<<24;
	}
	else
	{
		value_low += (get_hex(0))<<24;
	}
	if (bytes >=4)
	{
		value_low += (get_hex(HEX_msg[index++] - '0'))<<16;
	}
	else
	{
		value_low += (get_hex(0))<<16;
	}
	if (bytes >=3)
	{
		value_low += (get_hex(HEX_msg[index++] - '0'))<<8;
	}
	else
	{
		value_low += (get_hex(0))<<8;
	}
	if (bytes >=2)
	{
		value_low += (get_hex(HEX_msg[index++] - '0'));
	}
	else
	{
		value_low += (get_hex(0));
	}
	*HEX_5_4_addr = value_high;
	*HEX_3_0_addr = value_low;
	return bytes;
}



MODULE_LICENSE("GPL");
module_init (start_module);
module_exit (stop_module);
