#include <linux/miscdevice.h>       // for misc_device_register and struct miscdev
#include <linux/interrupt.h>
#include <linux/module.h>           // for module init and exit macros
#include <linux/fs.h>               // struct file, struct file_operations
#include <asm/io.h>                 // for mmap
#include "../address_map_arm.h"
#include "../interrupt_ID.h"

#define SUCCESS 0
#define MAX_SIZE 256                // we assume that no message will be longer than this

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

#define TIMER_FREQ	100000000	// 100MHz

static int device_open (struct inode *, struct file *);
static int device_release (struct inode *, struct file *);

static struct file_operations siggen_fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release
};

static struct miscdevice siggen = { 
    .minor = MISC_DYNAMIC_MINOR, 
    .name = "siggen",
    .fops = &siggen_fops,
    .mode = 0666
};

static int device_registered = 0;

volatile int * KEY_addr;
volatile int * SW_addr;
volatile int * HEX_3_0_addr;
volatile int * LEDR_addr;
volatile int * TIMER_addr;
volatile int * JP1_addr;
void * LW_virtual;

// Get the value for the hex display
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

// Set the led display to the switch value, and the hex display
// to the freq.
void set_hex_led(int sw_value)
{
	int real_value;

	*LEDR_addr = sw_value;
	real_value = sw_value >> 6;
	real_value *= 10;
	real_value += 10;
	*HEX_3_0_addr = (get_hex(real_value/100) << 16) + (get_hex((real_value%100)/10) << 8) + get_hex(real_value%10);
	// printk(KERN_INFO "Freq set to %i\n", real_value);
}

// Set the timer value to the required freq value
void set_timer0(int sw_value)
{
	int real_value;
	int counter_value;
	
	real_value = sw_value >> 6;
	real_value *= 10;
	real_value += 10;
	counter_value = TIMER_FREQ/real_value;
	counter_value /= 2;
	TIMER_addr[3]=counter_value >> 16;			// The higher part of counter start value
	TIMER_addr[2]=counter_value && 0xFFFF;		// The lower part of counter start value
	TIMER_addr[1]=5;				// Enable the Interrupt, disable auto reset/continue, and start
	// printk(KERN_INFO "Timer set to %i\n", counter_value);
}

// Interrupt handler of Timer0
irq_handler_t irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	//reset interrupt
	TIMER_addr[0]=0;
	// Switch JP1
	JP1_addr[1] = 1;
	JP1_addr[0] ^= 1;
	// Setup timer with the value of the keys
	set_timer0(*SW_addr);
	// reset values of HEX and LEDs
	set_hex_led(*SW_addr);
	return (irq_handler_t) IRQ_HANDLED;
}

static int __init start_module(void)
{
	int err;

	// Register the device
	err = misc_register (&siggen);
    if (err < 0) {
        printk (KERN_ERR "misc_register() for siggen failed\n");
		return err;
    }
    else {
        printk (KERN_INFO "driver siggen registered\n");
    }

	// Register the interrupt
	err = request_irq (TIMER0_IRQ, (irq_handler_t) irq_handler, IRQF_SHARED, "timer_0_irq_handler", (void *) (irq_handler));
	if (err)
	{
        misc_deregister (&siggen);
		printk(KERN_ERR "Failed to initialize TIMER0 Interrupt");
		return err;
	}
    device_registered = 1;

	// Setup pointers
	LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
	KEY_addr = (volatile int *) (LW_virtual + KEY_BASE);
	SW_addr = (volatile int *) (LW_virtual + SW_BASE);
	HEX_3_0_addr = (volatile int *) (LW_virtual + HEX3_HEX0_BASE);
	LEDR_addr    = (volatile int *) (LW_virtual + LEDR_BASE);
	TIMER_addr = (volatile int *) (LW_virtual + TIMER0_BASE);
	JP1_addr = (volatile int *) (LW_virtual + JP1_BASE);

	// Setup timer with the initial value of the keys
	set_timer0(*SW_addr);
	
	// reset values of HEX and LEDs
	set_hex_led(*SW_addr);

    return 0;
}

static void __exit stop_module(void)
{
    if (device_registered) {
		*LEDR_addr = 0;
		*HEX_3_0_addr = 0;
		TIMER_addr[1] = 0;
		TIMER_addr[2] = 0;
		TIMER_addr[3] = 0;
		JP1_addr[0] = 0;
		JP1_addr[1] = 0;
		iounmap(LW_virtual);
		free_irq (TIMER0_IRQ, (void*) irq_handler);
        misc_deregister (&siggen);
        printk (KERN_INFO "driver siggen de-registered\n");
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

MODULE_LICENSE("GPL");
module_init (start_module);
module_exit (stop_module);
