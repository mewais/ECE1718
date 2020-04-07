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

#define HEX_SIZE 16
#define CHAR_SIZE 8

#define NUM_HEX 6
#define NUM_LOWER_HEX 4
#define NUM_HIGHER_HEX 2

#define HEX_DISPLAY_0 63
#define HEX_DISPLAY_1 6
#define HEX_DISPLAY_2 91
#define HEX_DISPLAY_3 79
#define HEX_DISPLAY_4 102
#define HEX_DISPLAY_5 109
#define HEX_DISPLAY_6 124
#define HEX_DISPLAY_7 7
#define HEX_DISPLAY_8 127
#define HEX_DISPLAY_9 111

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
	HEX_3_0_addr = (volatile int *) (LW_virtual + HEX3_HEX0_BASE);
	HEX_5_4_addr = (volatile int *) (LW_virtual + HEX5_HEX4_BASE);
	LEDR_addr    = (volatile int *) (LW_virtual + LEDR_BASE);
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
		value = value * HEX_SIZE + parse_character_hex(LEDR_msg[i]);
	}
	*LEDR_addr = value;
	return bytes;
}

int get_hex(int value)
{
	switch (value)
	{
		case 0:
			return HEX_DISPLAY_0;
		case 1:
			return HEX_DISPLAY_1;
		case 2:
			return HEX_DISPLAY_2;
		case 3:
			return HEX_DISPLAY_3;
		case 4:
			return HEX_DISPLAY_4;
		case 5:
			return HEX_DISPLAY_5;
		case 6:
			return HEX_DISPLAY_6;
		case 7:
			return HEX_DISPLAY_7;
		case 8:
			return HEX_DISPLAY_8;
		case 9:
			return HEX_DISPLAY_9;
	}
	return 0;
}

static ssize_t HEX_device_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
    	size_t bytes;
	int value_high = 0;
	int value_low = 0;
	int i = 0;
	int index = 0;
	bytes = length;

	if (bytes > MAX_SIZE - 1)    // can copy all at once, or not?
		bytes = MAX_SIZE - 1;
	if (copy_from_user (HEX_msg, buffer, bytes) != 0)
		printk (KERN_ERR "Error: copy_from_user unsuccessful");

	// High part
	for (i = 0; i < NUM_HIGHER_HEX; i++) {
		value_high = value_high << CHAR_SIZE;
		if (NUM_HEX-i < bytes)	// If within our range, print
			value_high += get_hex(HEX_msg[index++] - '0');
		else			// Otherwise, print 0
			value_high += get_hex(0);
	}
	// Low part
	for (i = NUM_HIGHER_HEX; i < NUM_HEX; i++) {
		value_low = value_low << CHAR_SIZE;
		if (NUM_HEX-i < bytes)	// If within our range, print
			value_low += get_hex(HEX_msg[index++] - '0');
		else			// Otherwise, print 0
			value_low += get_hex(0);
	}
	*HEX_5_4_addr = value_high;
	*HEX_3_0_addr = value_low;
	return bytes;
}



MODULE_LICENSE("GPL");
module_init (start_module);
module_exit (stop_module);
