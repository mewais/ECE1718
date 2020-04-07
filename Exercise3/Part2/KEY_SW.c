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
#define EDGE_BIT_OFFSET 3
static int device_open (struct inode *, struct file *);
static int device_release (struct inode *, struct file *);

static ssize_t KEY_device_read (struct file *, char *, size_t, loff_t *);

static ssize_t SW_device_read (struct file *, char *, size_t, loff_t *);

static struct file_operations KEY_fops = {
    .owner = THIS_MODULE,
    .read = KEY_device_read,
    // .write = KEY_device_write,
    .open = device_open,
    .release = device_release
};

static struct file_operations SW_fops = {
    .owner = THIS_MODULE,
    .read = SW_device_read,
    // .write = SW_device_write,
    .open = device_open,
    .release = device_release
};

#define SUCCESS 0

static struct miscdevice KEY = { 
    .minor = MISC_DYNAMIC_MINOR, 
    .name = "KEY",
    .fops = &KEY_fops,
    .mode = 0666
};

static struct miscdevice SW = { 
    .minor = MISC_DYNAMIC_MINOR, 
    .name = "SW",
    .fops = &SW_fops,
    .mode = 0666
};

static int KEY_registered = 0;
static int SW_registered = 0;

#define MAX_SIZE 256                // we assume that no message will be longer than this
static char KEY_msg[MAX_SIZE];  // the character array that can be read or written
static char SW_msg[MAX_SIZE];  // the character array that can be read or written

volatile int * KEY_addr;
volatile int * SW_addr;
void * LW_virtual;

static int __init start_module(void)
{
	int err;
	LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
	KEY_addr = (volatile int *) (LW_virtual + KEY_BASE);
	SW_addr = (volatile int *) (LW_virtual + SW_BASE);
    err = misc_register (&KEY);
    if (err < 0) {
        printk (KERN_ERR "misc_register() for KEY failed\n");
    }
    else {
        printk (KERN_INFO "driver KEY registered\n");
        KEY_registered = 1;
    }
	err = misc_register (&SW);
	if (err < 0) {
        printk (KERN_ERR "misc_register() for SW failed\n");
    }
    else {
        printk (KERN_INFO "driver SW registered\n");
        SW_registered = 1;
    }

    return err;
}

static void __exit stop_module(void)
{
    if (KEY_registered) {
        misc_deregister (&KEY);

        printk (KERN_INFO "driver KEY de-registered\n");
    }
	
	if (SW_registered) {
        misc_deregister (&SW);

        printk (KERN_INFO "driver SW de-registered\n");
	}
	iounmap(LW_virtual);
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

/* Called when a process reads from chardev. Provides character data from chardev_msg.
 * Returns, and sets *offset to, the number of bytes read. */
static ssize_t KEY_device_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
	size_t bytes;
	int read_value;
	read_value = KEY_addr[EDGE_BIT_OFFSET];
	sprintf(KEY_msg, "%x\n",read_value);
	KEY_addr[EDGE_BIT_OFFSET] = read_value;
    bytes = strlen (KEY_msg) - (*offset);    // how many bytes not yet sent?
    bytes = bytes > length ? length : bytes;     // too much to send all at once?
    
    if (bytes)
        if (copy_to_user (buffer, &KEY_msg[*offset], bytes) != 0)
            printk (KERN_ERR "Error: copy_to_user unsuccessful");
    *offset = bytes;    // keep track of number of bytes sent to the user
    return bytes;
}

static ssize_t SW_device_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
	size_t bytes;
	sprintf(SW_msg, "0x%03x\n",*SW_addr);
    bytes = strlen (SW_msg) - (*offset);    // how many bytes not yet sent?
    bytes = bytes > length ? length : bytes;     // too much to send all at once?
    
    if (bytes)
        if (copy_to_user (buffer, &SW_msg[*offset], bytes) != 0)
            printk (KERN_ERR "Error: copy_to_user unsuccessful");
    *offset = bytes;    // keep track of number of bytes sent to the user
    return bytes;
}

MODULE_LICENSE("GPL");
module_init (start_module);
module_exit (stop_module);
