#include <linux/fs.h>               // struct file, struct file_operations
#include <linux/init.h>             // for __init, see code
#include <linux/module.h>           // for module init and exit macros
#include <linux/miscdevice.h>       // for misc_device_register and struct miscdev
#include <linux/uaccess.h>          // for copy_to_user, see code
#include <asm/io.h>                 // for mmap
#include <linux/interrupt.h>
#include "../address_map_arm.h"
#include "../interrupt_ID.h"

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
#define MAX_COUNT 360000
/* Kernel character device driver. By default, this driver provides the text "Hello from 
 * stopwatch" when /dev/stopwatch is read (for example, cat /dev/stopwatch). The text can be changed 
 * by writing a new string to /dev/stopwatch (for example echo "New message" > /dev/stopwatch).
 * This version of the code uses copy_to_user and copy_from_user, to send data to, and receive
 * date from, user-space */

static int device_open (struct inode *, struct file *);
static int device_release (struct inode *, struct file *);
static ssize_t device_read (struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

volatile int *TIMER_PTR;
volatile int *HEX_3_0_ptr, *HEX_5_4_ptr; // virtual addresses
void * LW_virtual;
int disp =1;

int cur_count = MAX_COUNT - 1;
int digit0 = 0;
int digit1 = 0;
int digit2 = 0;
int digit3 = 0;
int digit4 = 0;
int digit5 = 0;

static struct file_operations stopwatch_fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
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

irq_handler_t irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	int hs = cur_count%100;
	int s = (cur_count / 100)%60;
	int m = (cur_count / 6000);
	
	//reset interrupt
	TIMER_PTR[0]=0;
	if (cur_count != 0)
	{
		cur_count --;
	}
	digit0=hs%10;
	digit1=hs/10;
	digit2=s%10;
	digit3=s/10;
	digit4=m%10;
	digit5=m/10;
	// set_hex_display
	if(disp)
	{
		*HEX_5_4_ptr = get_hex(digit4) + (get_hex(digit5)<<8);
		*HEX_3_0_ptr = get_hex(digit0) + (get_hex(digit1)<<8) + (get_hex(digit2)<<16) + (get_hex(digit3)<<24);
	}
	else
	{
		*HEX_5_4_ptr = 0;
		*HEX_3_0_ptr = 0;
	}
	return (irq_handler_t) IRQ_HANDLED;
}

static int __init start_stopwatch(void)
{
    int err = misc_register (&stopwatch);
    if (err < 0)
	{
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", DEV_NAME);
		return err;
    }
    else
	{
        printk (KERN_INFO "/dev/%s driver registered\n", DEV_NAME);
    }
	err = request_irq (TIMER0_IRQ, (irq_handler_t) irq_handler, IRQF_SHARED, "timer_0_irq_handler", (void *) (irq_handler));
	if (err)
	{
        misc_deregister (&stopwatch);
		printk(KERN_ERR "Failed to initialize TIMER0 Interrupt");
		return err;
	}
    stopwatch_registered = 1;
	LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
	HEX_3_0_ptr = LW_virtual + HEX3_HEX0_BASE; // virtual address for HEX3-0 port
	HEX_5_4_ptr = LW_virtual + HEX5_HEX4_BASE;
	TIMER_PTR = LW_virtual + TIMER0_BASE;
	TIMER_PTR[3]=0xF;			// The higher part of counter start value
	TIMER_PTR[2]=0x4240;		// The lower part of counter start value
	TIMER_PTR[1]=7;				// Enable the Interrupt, enable auto reset/continue, and start
    return 0;
}

static void __exit stop_stopwatch(void)
{
    if (stopwatch_registered) {
		TIMER_PTR[1] = 0;
		TIMER_PTR[2] = 0;
		TIMER_PTR[3] = 0;
		*HEX_5_4_ptr = 0;
		*HEX_3_0_ptr = 0;
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

    stopwatch_msg[0] = digit5 + '0';
    stopwatch_msg[1] = digit4 + '0';
    stopwatch_msg[2] = ':';
    stopwatch_msg[3] = digit3 + '0';
    stopwatch_msg[4] = digit2 + '0';
    stopwatch_msg[5] = ':';
    stopwatch_msg[6] = digit1 + '0';
    stopwatch_msg[7] = digit0 + '0';
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

/* Called when a process writes to stopwatch. Stores the data received into stopwatch_msg, and 
 * returns the number of bytes stored. */
static ssize_t device_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
    size_t bytes;
	char start_str[] = "run";
	char stop_str[] = "stop";
	char disp_str[] = "disp";
	char nodisp_str[] = "nodisp";
    bytes = length;

    if (bytes > MAX_SIZE - 1)    // can copy all at once, or not?
        bytes = MAX_SIZE - 1;
    if (copy_from_user (stopwatch_msg, buffer, bytes) != 0)
        printk (KERN_ERR "Error: copy_from_user unsuccessful");
    stopwatch_msg[bytes-1] = '\0';    // NULL terminate
	// Find out the command we got
	if (strcmp(stopwatch_msg, start_str) == 0)
	{
		TIMER_PTR[1]=7; //Enable the Interrupt, enable auto reset/continue, and start
		return bytes;
	}
	if (strcmp(stopwatch_msg, stop_str) == 0)
	{
		TIMER_PTR[1]=11;//Enable the Interrupt, enable auto reset/continue, and stop
		return bytes;
	}
	if (strcmp(stopwatch_msg, disp_str) == 0)
	{
		// If we disp/nodisp while it is paused, need to update HEX here
		*HEX_5_4_ptr = get_hex(digit4) + (get_hex(digit5)<<8);
		*HEX_3_0_ptr = get_hex(digit0) + (get_hex(digit1)<<8) + (get_hex(digit2)<<16) + (get_hex(digit3)<<24);
		disp = 1;
		return bytes;
	}
	if (strcmp(stopwatch_msg, nodisp_str) == 0)
	{
		// If we disp/nodisp while it is paused, need to update HEX here
		*HEX_5_4_ptr = 0;
		*HEX_3_0_ptr = 0;
		disp = 0;
		return bytes;
	}
	// Either it is in the MM:SS:DD format or we got a wrong input
	if (bytes-1 == 8 && stopwatch_msg[2] == ':' && stopwatch_msg[5] == ':')
	{
		int value = 0;
		if (stopwatch_msg[0] < '0' || stopwatch_msg[0] > '9' ||
			stopwatch_msg[1] < '0' || stopwatch_msg[1] > '9' ||
			stopwatch_msg[3] < '0' || stopwatch_msg[3] > '9' ||
			stopwatch_msg[4] < '0' || stopwatch_msg[4] > '9' ||
			stopwatch_msg[6] < '0' || stopwatch_msg[6] > '9' ||
			stopwatch_msg[7] < '0' || stopwatch_msg[7] > '9')
		{
			// bogus value, forget it.
			return bytes;
		}
		digit5 = stopwatch_msg[0] - '0';
		value += digit5 * 10;
		digit4 = stopwatch_msg[1] - '0';
		value += digit4;
		value *= 60;
		digit3 = stopwatch_msg[3] - '0';
		value += digit3 * 10;
		digit2 = stopwatch_msg[4] - '0';
		value += digit2;
		value *= 100;
		digit1 = stopwatch_msg[6] - '0';
		value += digit1 * 10;
		digit0 = stopwatch_msg[7] - '0';
		value += digit0;
		cur_count = value;
		// If we're paused, need to update HEX now
		*HEX_5_4_ptr = get_hex(digit4) + (get_hex(digit5)<<8);
		*HEX_3_0_ptr = get_hex(digit0) + (get_hex(digit1)<<8) + (get_hex(digit2)<<16) + (get_hex(digit3)<<24);
	}
	// If we get here, got invalid command and ignored it
	return bytes;
}

MODULE_LICENSE("GPL");
module_init (start_stopwatch);
module_exit (stop_stopwatch);
