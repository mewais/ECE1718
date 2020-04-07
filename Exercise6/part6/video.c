#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>             // for __init, see code
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>          // for copy_to_user, see code
#include <linux/miscdevice.h>       // for misc_device_register and struct miscdev
#include <asm/io.h>
#include <asm/uaccess.h>
#include "../address_map_arm.h"

#define DEV_NAME "video"
#define SUCCESS 0
#define MAX_SIZE 1024
char msg [MAX_SIZE+1];
// Declare global variables needed to use the pixel buffer
void *LW_virtual; // used to access FPGA light-weight bridge
volatile int * pixel_ctrl_ptr; // virtual address of pixel buffer controller
volatile int * char_ctrl_ptr; // virtual address of pixel buffer controller
int device_registered = 0;
ushort * pixel_buffer, * back_pixel_buffer; // used for virtual address of pixel buffer
ushort * char_buffer; // used for virtual address of pixel buffer
int resolution_x, resolution_y; // VGA screen size
int char_x, char_y; // VGA screen size
int resolution_m, resolution_n; // VGA screen size

// Declare variables and prototypes needed for a character device driver
static int device_open (struct inode *, struct file *);
static int device_release (struct inode *, struct file *);
static ssize_t device_read (struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations video_fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};

static struct miscdevice video = { 
    .minor = MISC_DYNAMIC_MINOR, 
    .name = DEV_NAME,
    .fops = &video_fops,
    .mode = 0666
};

void get_screen_specs(volatile int * pixel_ctrl_ptr, volatile int * char_ctrl_ptr)
{
	int res = pixel_ctrl_ptr[2];
	resolution_x = res & 0xFFFF;
	resolution_y = res >> 16;
	res = pixel_ctrl_ptr[3];
	resolution_n = (res >> 16) & 0xFF;
	resolution_m = res >> 24;

	pixel_ctrl_ptr[1] = 0xC0000000;

	res = char_ctrl_ptr[2];
	char_x = res & 0xFFFF;
	char_y = res >> 16;
	printk(KERN_INFO "X: %i, Y: %i", char_x, char_y);
}

void sync(void)
{
	ushort * temp;
	pixel_ctrl_ptr[0]=1;
	while ((pixel_ctrl_ptr[3] & 1) == 1);
	temp = back_pixel_buffer;
	back_pixel_buffer = pixel_buffer;
	pixel_buffer = temp;
}

void clear_screen(void)
{
	int i, j;
	int address;
	for (i = 0; i < resolution_x; i++)
	{
		for (j = 0; j < resolution_y; j++)
		{
			address = i + (j << (resolution_n));
			back_pixel_buffer[address] = 0x0;
		}
	}
}

void clear_text(void)
{
	int i, j;
	int address;
	for (i = 0; i < char_x; i++)
	{
		for (j = 0; j < char_y; j++)
		{
			address = i + (j << 7);
			char_buffer[address] = ' ';
		}
	}
}

void plot_pixel(int x, int y, short int color)
{
	int address = 0;
	if (x < resolution_x && 0 <= x && y < resolution_y && 0 <= y)
	{
		address = x + (y << (resolution_n));
		back_pixel_buffer[address] = color;
	}
}

void plot_line(int xs, int ys, int xe, int ye, int color)
{
	int is_steep = (abs(ye - ys) > abs(xe - xs));
	int deltax;
	int deltay;
	int error;
	int y;
	int y_step;
	int x;
	int address;

	if (is_steep)
	{
		int t = xs;
		xs = ys;
		ys = t;
		t = xe;
		xe = ye;
		ye = t;
	}
	if (xe < xs)
	{
		int t = xs;
		xs = xe;
		xe = t;
		t = ys;
		ys = ye;
		ye = t;
	}

	deltax = xe - xs;
	deltay = abs(ye - ys);
	error = -(deltax/2);

	y = ys;
	if (ys < ye)
		y_step = 1;
	else
		y_step = -1;

	for (x = xs; x < xe; x++)
	{
		if (is_steep)
		{
			if (y < resolution_x && 0 <= y && x < resolution_y && 0 <= x)
			{
				address = y + (x << (resolution_n));
				back_pixel_buffer[address] = color;
			}
		}
		else
		{
			if (x < resolution_x && 0 <= x && y < resolution_y && 0 <= y)
			{
				address = x + (y << (resolution_n));
				back_pixel_buffer[address] = color;
			}
		}
		error += deltay;
		if (error >= 0)
		{
			y += y_step;
			error -= deltax;
 		}
	}
}

void write_text(int x, int y, char* str)
{
	int i = 0;
	int address = 0;
	while (str[i] != '\0')
	{
		address = (y << 7) + x++;
		if (x == char_x)
		{
			x = 0;
			y++;
		}
		char_buffer[address] = str[i++];
	}
}

int parse_num(char* string, int* start_idx, char end_char)
{
	int value = 0;
	while (string[*start_idx] != end_char)
	{
		if (string[*start_idx] > '9' || string[*start_idx] < '0')
		{
			printk(KERN_ERR "Format string invalid");
			return 0;
		}
		else
		{
			value = (value * 10) + string[(*start_idx)++] - '0';
		}
	}
	return value;
}

/* Code to initialize the video driver */
static int __init start_video(void)
{
	// initialize the dev_t, cdev, and class data structures
	int err = misc_register(&video);
	if (err < 0)
	{
		printk(KERN_ERR "/dev/%s: misc_register() failed\n", DEV_NAME);
		return err;
	}
    printk(KERN_INFO "/dev/%s driver registered\n", DEV_NAME);
	// generate a virtual address for the FPGA lightweight bridge
	LW_virtual = ioremap_nocache (0xFF200000, 0x00005000);
	if (LW_virtual == 0)
	{
		printk(KERN_ERR "Error: ioremap_nocache returned NULL\n");
		misc_deregister(&video);
		return -1;
	}

	// Create virtual memory access to the pixel buffer controller
	pixel_ctrl_ptr = (unsigned int *) (LW_virtual + 0x00003020);
	char_ctrl_ptr = (unsigned int *) (LW_virtual + 0x00003030);
	get_screen_specs (pixel_ctrl_ptr, char_ctrl_ptr); // determine X, Y screen size

	// Create virtual memory access to the pixel buffer
	pixel_buffer = ioremap_nocache (0xC8000000, 0x0003FFFF);
	if (pixel_buffer == 0)
	{
		printk (KERN_ERR "Error: ioremap_nocache returned NULL\n");
		iounmap(LW_virtual);
		misc_deregister(&video);
		return -1;
	}
	
	// Create virtual memory access to the back pixel buffer
	back_pixel_buffer = ioremap_nocache (0xC0000000, 0x0003FFFF);
	//back_pixel_buffer = pixel_buffer;
	if (back_pixel_buffer == 0)
	{
		printk (KERN_ERR "Error: ioremap_nocache returned NULL\n");
		iounmap(LW_virtual);
		iounmap(pixel_buffer);
		misc_deregister(&video);
		return -1;
	}
	
	// Create virtual memory access to the pixel buffer
	char_buffer = ioremap_nocache (0xC9000000, 0x0003FFFF);
	if (pixel_buffer == 0)
	{
		printk (KERN_ERR "Error: ioremap_nocache returned NULL\n");
		iounmap(LW_virtual);
		iounmap(pixel_buffer);
		iounmap(back_pixel_buffer);
		misc_deregister(&video);
		return -1;
	}
	device_registered = 1;

	/* Erase the pixel buffer */
	clear_screen();
	clear_text();
	return 0;
}

static void __exit stop_video(void)
{
	if (device_registered)
	{
		clear_screen();
		clear_text();

		/* unmap the physical-to-virtual mappings */
		iounmap (LW_virtual);
		iounmap (pixel_buffer);
		iounmap (back_pixel_buffer);
		iounmap (char_buffer);

		/* Remove the device from the kernel */
		misc_deregister(&video);
	}
}

static int device_open(struct inode *inode, struct file *file)
{
	return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
	char info[9];
	size_t bytes;

	sprintf(info, "%3d %3d", resolution_y, resolution_x);

	bytes = strlen(info) - (*offset);
    bytes = bytes > length ? length : bytes;     // too much to send all at once?
	if (bytes)
		if (copy_to_user(buffer, &info, strlen(info)) != 0)
			printk(KERN_ERR "Error: copy_to_user unsuccessful");

	*offset = bytes;
	return bytes;
}

static ssize_t device_write(struct file *filp, const char* buffer, size_t length, loff_t *offset)
{
	size_t bytes;
	char clear_str[] = "clear";
	char erase_str[] = "erase";
	char sync_str[] = "sync";
	char pixel_str[] = "pixel";
	char line_str[] = "line";
	char box_str[] = "box";
	char text_str[] = "text";
	int cur_loc;
	int x, y, color;
	int x2, y2;
	char text[64];
	int index = 0;
    bytes = length;
	if (bytes > MAX_SIZE - 1)    // can copy all at once, or not?
        bytes = MAX_SIZE - 1;
    if (copy_from_user (msg, buffer, bytes) != 0)
        printk (KERN_ERR "Error: copy_from_user unsuccessful");
    msg[bytes-1] = '\0';    // NULL terminate
	// Find out the command we got
	if (strcmp(msg, clear_str) == 0)
	{
		clear_screen();
		return bytes;
	}
	else if (strcmp(msg, sync_str) == 0)
	{
		sync();
		return bytes;
	}
	else if (strcmp(msg, erase_str) == 0)
	{
		clear_text();
		return bytes;
	}
	else if (strncmp(msg, pixel_str, 5) == 0)
	{
		if (msg[5] != ' ')
		{
			printk(KERN_ERR "Format string invalid");
			return 0;
		}
		cur_loc = 6;
		x = parse_num(msg, &cur_loc, ',');
		cur_loc++;
		y = parse_num(msg, &cur_loc, ' ');
		cur_loc++;
		color = parse_num(msg, &cur_loc, '\0');
		printk(KERN_INFO "draw at (%d,%d) color = %d\n", x, y, color);
		plot_pixel(x, y, color);
	}
	else if (strncmp(msg, line_str, 4) == 0)
	{
		if (msg[4] != ' ')
		{
			printk(KERN_ERR "Format string invalid");
			return 0;
		}
		cur_loc = 5;
		x = parse_num(msg, &cur_loc, ',');
		cur_loc++;
		y = parse_num(msg, &cur_loc, ' ');
		cur_loc++;
		x2 = parse_num(msg, &cur_loc, ',');
		cur_loc++;
		y2 = parse_num(msg, &cur_loc, ' ');
		cur_loc++;
		color = parse_num(msg, &cur_loc, '\0');
		printk(KERN_INFO "draw at (%d,%d) to (%d,%d) color = %d\n", x, y, x2, y2, color);
		plot_line(x, y, x2, y2, color);
	}
	else if (strncmp(msg, box_str, 3) == 0)
	{
		if (msg[3] != ' ')
		{
			printk(KERN_ERR "Format string invalid");
			return 0;
		}
		cur_loc = 4;
		x = parse_num(msg, &cur_loc, ',');
		cur_loc++;
		y = parse_num(msg, &cur_loc, ' ');
		cur_loc++;
		x2 = parse_num(msg, &cur_loc, ',');
		cur_loc++;
		y2 = parse_num(msg, &cur_loc, ' ');
		cur_loc++;
		color = parse_num(msg, &cur_loc, '\0');
		printk(KERN_INFO "draw box at (%d,%d) to (%d,%d) color = %d\n", x, y, x2, y2, color);
		for (x = min(x, x2); x < max(x, x2); x++)
		{
			plot_line(x, y, x, y2, color);
		}
	}
	else if (strncmp(msg, text_str, 4) == 0)
	{
		if (msg[4] != ' ')
		{
			printk(KERN_ERR "Format string invalid");
			return 0;
		}
		cur_loc = 5;
		x = parse_num(msg, &cur_loc, ',');
		cur_loc++;
		y = parse_num(msg, &cur_loc, ' ');
		cur_loc++;
		while(msg[cur_loc] != '\0')
		{
			text[index++] = msg[cur_loc++];
		}
		text[index] = '\0';
		printk(KERN_INFO "writing text %s at (%d,%d)\n", text, x, y);
		write_text(x, y, text);
	}
	return bytes;
}

MODULE_LICENSE("GPL");
module_init (start_video);
module_exit (stop_video);
