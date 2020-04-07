#include <linux/fs.h>               // struct file, struct file_operations
#include <linux/init.h>             // for __init, see code
#include <linux/module.h>           // for module init and exit macros
#include <linux/miscdevice.h>       // for misc_device_register and struct miscdev
#include <linux/uaccess.h>          // for copy_to_user, see code
#include <asm/io.h>                 // for mmap
#include <linux/interrupt.h>
#include "../address_map_arm.h"
#include "../interrupt_ID.h"

/* Kernel character device driver. By default, this driver provides the text "Hello from 
 * stopwatch" when /dev/stopwatch is read (for example, cat /dev/stopwatch). The text can be changed 
 * by writing a new string to /dev/stopwatch (for example echo "New message" > /dev/stopwatch).
 * This version of the code uses copy_to_user and copy_from_user, to send data to, and receive
 * date from, user-space */

static int device_open (struct inode *, struct file *);
static int device_release (struct inode *, struct file *);
static ssize_t device_read (struct file *, char *, size_t, loff_t *);

volatile int *TIMER_PTR;
void * LW_virtual;

int cur_count = 360000 - 1;

static struct file_operations stopwatch_fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .open = device_open,
    .release = device_release
};

#define SUCCESS 0
#define DEV_NAME "stopwatch"

static struct miscdevice stopwatch = { 
    .minor = MISC_DYNAMIC_MINOR, 
    .name = DEV_NAME,
    .fops = &stopwatch_fops,
    .mode = 0666
};
static int stopwatch_registered = 0;

#define MAX_SIZE 256                // we assume that no message will be longer than this
static char stopwatch_msg[MAX_SIZE];  // the character array that can be read or written

irq_handler_t irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	TIMER_PTR[0]=0;
	if (cur_count != 0) {
		cur_count --;
	}	
	// set_hex_display();
	//reset interrupt
	return (irq_handler_t) IRQ_HANDLED;
}

static int __init start_stopwatch(void)
{
    int err = misc_register (&stopwatch);
    if (err < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", DEV_NAME);
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", DEV_NAME);
        stopwatch_registered = 1;
    }
	LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
	TIMER_PTR = LW_virtual + TIMER0_BASE;
	err += request_irq (TIMER0_IRQ, (irq_handler_t) irq_handler, IRQF_SHARED, "timer_0_irq_handler", (void *) (irq_handler));
	TIMER_PTR[3]=0xF;
	TIMER_PTR[2]=0x4240;
	TIMER_PTR[1]=7;
    return err;
}

static void __exit stop_stopwatch(void)
{
    if (stopwatch_registered) {
		TIMER_PTR[1] = 0;
		iounmap (LW_virtual);
		free_irq (TIMER0_IRQ, (void*) irq_handler);
        misc_deregister (&stopwatch);
        printk (KERN_INFO "/dev/%s driver de-registered\n", DEV_NAME);
    }
}

/* Called when a process opens stopwatch */
static int device_open(struct inode *inode, struct file *file)
{
    return SUCCESS;
}

/* Called when a process closes stopwatch */
static int device_release(struct inode *inode, struct file *file)
{
    return 0;
}

/* Called when a process reads from stopwatch. Provides character data from stopwatch_msg.
 * Returns, and sets *offset to, the number of bytes read. */
static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    size_t bytes;

    int minutes = cur_count / (60*100);
    int seconds = (cur_count/100)%60;
    int hundredths = cur_count%100;

    stopwatch_msg[0] = minutes/10 + '0';
    stopwatch_msg[1] = minutes%10 + '0';
    stopwatch_msg[2] = ':';
    stopwatch_msg[3] = seconds/10 + '0';
    stopwatch_msg[4] = seconds%10 + '0';
    stopwatch_msg[5] = ':';
    stopwatch_msg[6] = hundredths/10 + '0';
    stopwatch_msg[7] = hundredths%10 + '0';
    stopwatch_msg[8] = '\n';
    stopwatch_msg[9] = 0;

    bytes = strlen (stopwatch_msg) - (*offset);    // how many bytes not yet sent?
    bytes = bytes > length ? length : bytes;     // too much to send all at once?
    
    if (bytes)
        if (copy_to_user (buffer, &stopwatch_msg[*offset], bytes) != 0)
            printk (KERN_ERR "Error: copy_to_user unsuccessful");
    *offset = bytes;    // keep track of number of bytes sent to the user
    return bytes;
}

MODULE_LICENSE("GPL");
module_init (start_stopwatch);
module_exit (stop_stopwatch);
