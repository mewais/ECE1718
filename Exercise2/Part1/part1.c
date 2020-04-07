#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "../address_map_arm.h"
#include <unistd.h>

/* Prototypes for functions used to access physical memory addresses */
int open_physical (int);
void * map_physical (int, unsigned int, unsigned int);
void close_physical (int);
int unmap_physical (void *, unsigned int);

/* This program increments the contents of the red LED parallel port */
int main(void)
{
	volatile int * HEX_DISPLAY_1;
	volatile int * HEX_DISPLAY_2;
	volatile int * KEYS;
	int fd = -1; // used to open /dev/mem
	void *LW_virtual; // physical addresses for light-weight bridge

	// Create virtual memory access to the FPGA light-weight bridge
	if ((fd = open_physical (fd)) == -1)
		return (-1);
	if (!(LW_virtual = map_physical (fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN)))
		return (-1);

	// Set virtual address pointer to I/O port
	unsigned int disp_1, disp_2, disp_3, disp_4;
	disp_1 = 0x454;
	disp_2 = 0x78793800;
	disp_3 = 0x6D5C3900;
	disp_4 = 0x71737D77;
	HEX_DISPLAY_1 = (int *) (LW_virtual) + HEX3_HEX0_BASE/4;
	HEX_DISPLAY_2 = (int *) (LW_virtual) + HEX5_HEX4_BASE/4;
	KEYS = (int *) (LW_virtual) + 0x5C/4;
	while (1)
	{
		*HEX_DISPLAY_2 = disp_1;
		*HEX_DISPLAY_1 = disp_2;
		disp_1 = (disp_1<<8) + (disp_2>>24);
		disp_2 = (disp_2<<8) + (disp_3>>24);
		disp_3 = (disp_3<<8) + (disp_4>>24);
		disp_4 = (disp_4<<8) + (disp_1>>16);
		disp_1 = (disp_1&0xFFFF);
		//disp_1 = disp_1 + ((disp_4&0xFF)<<16);
		//disp_4 = (disp_4>>8) + ((disp_3&0xFF)<<24);
		//disp_3 = (disp_3>>8) + ((disp_2&0xFF)<<24);
		//disp_2 = (disp_2>>8) + ((disp_1&0xFF)<<24);
		//disp_1 = (disp_1>>8);
		usleep(300000);
		if (((*KEYS)&0x1)==1)
		{
			*KEYS=1;
			while (((*KEYS)&0x1)==0)
			{
				usleep(10000);
			}
			*KEYS = 1;
		}
	}
	unmap_physical (LW_virtual, LW_BRIDGE_SPAN);
	close_physical (fd);
	return 0;
}

/* Open /dev/mem to give access to physical addresses */
int open_physical (int fd)
{
	if (fd == -1) // check if already open
		if ((fd = open( "/dev/mem", (O_RDWR | O_SYNC))) == -1)
		{
			printf ("ERROR: could not open \"/dev/mem\"...\n");
			return (-1);
		}
	return fd;
}

/* Close /dev/mem to give access to physical addresses */
void close_physical (int fd)
{
	close (fd);
}

/* Establish a virtual address mapping for the physical addresses starting
 * at base and extending by span bytes */
void* map_physical(int fd, unsigned int base, unsigned int span)
{
	void *virtual_base;
	// Get a mapping from physical addresses to virtual addresses
	virtual_base = mmap (NULL, span, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, base);
	if (virtual_base == MAP_FAILED)
	{
		printf ("ERROR: mmap() failed...\n");
		close (fd);
		return (NULL);
	}
	return virtual_base;
}
