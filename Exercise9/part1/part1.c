#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <signal.h>

#define ADC_BASE 0xFF204000
#define ADC_SPAN 0x100

int fd;
int* adc_base;

void handle_int(int dummy)
{
	munmap(adc_base, ADC_SPAN);
    close(fd);
	exit(0);
}

int main(void) {
	if ((fd = open( "/dev/mem", (O_RDWR | O_SYNC))) == -1)
	{
		printf ("ERROR could not open /dev/mem\n");
		return -1;
	}
	adc_base = (int *) mmap (NULL, ADC_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, ADC_BASE);
	if (adc_base == MAP_FAILED)
	{
		printf ("ERROR: mmap() failed...\n");
		close (fd);
		return -1;
	}
	signal(SIGINT, handle_int);

	while(1)
	{
		// Write once to start an ADC
		adc_base[0] = 1;
		// Read the required channels
		int value = adc_base[0];
		printf("Channel 0: %i\n", value);
		// Delay for a nanosecond
		usleep(1000);
	}

	munmap(adc_base, ADC_SPAN);
    close(fd);
    return 0;
}
