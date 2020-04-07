#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <math.h>
#include <time.h>

#define VIDEO_SIZE 64
#define ADC_BASE 0xFF204000
#define ADC_SPAN 0x100

#define ADC_VOLTAGE_SCALE 4096
#define ADC_SWEEP_SCALE 100000	// us

int screen_x, screen_y;
int y_scale;
static int video_FD, mem_FD; // file descriptor
static char VIDEO_BUF[VIDEO_SIZE];
int* adc_base;

struct itimerspec interval_timer_start = 
{
	.it_interval = {.tv_sec=0,.tv_nsec=312500},
	.it_value = {.tv_sec=0,.tv_nsec=312500}
};
struct itimerspec interval_timer_stop = 
{
	.it_interval = {.tv_sec=0,.tv_nsec=0},
	.it_value = {.tv_sec=0,.tv_nsec=0}
};
timer_t interval_timer_id;

static int oscilloscope_index;
static int* oscilloscope_array;

void handle_sigint(int dummy)
{
	// Apparently, calling free from a signal handler is bad, and the OS will clean it up anyway.
	// free(oscilloscope_array);
	timer_settime (interval_timer_id, 0, &interval_timer_stop, NULL);
	munmap(adc_base, ADC_SPAN);
	sprintf(VIDEO_BUF, "clear");
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);
	sprintf(VIDEO_BUF, "erase");
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);
	sprintf(VIDEO_BUF, "sync");
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);
	close(video_FD);
    close(mem_FD);
	exit(0);
}

void handle_sigalrm(int dummy)
{
	if (oscilloscope_index <= screen_x)
	{
		// Trigger an ADC
		adc_base[0] = 1;
		// Read the value and store it
		oscilloscope_array[oscilloscope_index++] = adc_base[0] / y_scale;
		// Check if done
		if (oscilloscope_index == screen_x)
		{
			// Stop the timer
			timer_settime (interval_timer_id, 0, &interval_timer_stop, NULL);
			// Actually plot
			for (int x = 1; x < screen_x; x++)
			{
				sprintf(VIDEO_BUF, "line %i,%i %i,%i %04x", x-1, oscilloscope_array[x-1], x, oscilloscope_array[x], 0xF000);
				write(video_FD, VIDEO_BUF, VIDEO_SIZE);
			}
			sprintf(VIDEO_BUF, "sync");
			write(video_FD, VIDEO_BUF, VIDEO_SIZE);
		}
	}
}

int main(int argc, char *argv[]){
	if ((mem_FD = open( "/dev/mem", (O_RDWR | O_SYNC))) == -1)
	{
		printf ("ERROR could not open /dev/mem\n");
		return -1;
	}
	adc_base = (int *) mmap (NULL, ADC_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, mem_FD, ADC_BASE);
	if (adc_base == MAP_FAILED)
	{
		printf ("ERROR: mmap() failed...\n");
		close (mem_FD);
		return -1;
	}
	// Open the character device driver
	if ((video_FD = open("/dev/IntelFPGAUP/video", O_RDWR)) == -1)
	{
		printf("Error opening /dev/video: %s\n", strerror(errno));
		munmap(adc_base, ADC_SPAN);
		close (mem_FD);
		return -1;
	}

	signal(SIGINT, handle_sigint);

	// Set screen_x and screen_y by reading from the driver
	while(read(video_FD, VIDEO_BUF, 7)!=0);
	screen_x = ((VIDEO_BUF[0] - '0') * 100) + ((VIDEO_BUF[1] - '0') * 10) + VIDEO_BUF[2] - '0';
	screen_y = ((VIDEO_BUF[4] - '0') * 100) + ((VIDEO_BUF[5] - '0') * 10) + VIDEO_BUF[6] - '0';
	oscilloscope_array = malloc(screen_y*sizeof(int));

	sprintf(VIDEO_BUF, "clear");
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);
	sprintf(VIDEO_BUF, "erase");
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);
	sprintf(VIDEO_BUF, "sync");
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);

	// Draw Axis
	sprintf(VIDEO_BUF, "line %i,%i %i,%i %04x", 0, 239, 319, 239, 0x0AC0);
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);
	sprintf(VIDEO_BUF, "line %i,%i %i,%i %04x", 0, 0, 0, 239, 0x0AC0);
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);
	sprintf(VIDEO_BUF, "sync");
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);

	// Setup timer
	struct sigaction act;
	sigset_t set;
	sigemptyset (&set);
	sigaddset (&set, SIGALRM);
	act.sa_flags = 0;
	act.sa_mask = set;
	act.sa_handler = &handle_sigalrm;
	timer_create (CLOCK_MONOTONIC, NULL, &interval_timer_id);
	sigaction (SIGALRM, &act, NULL);

	// Determine Scales
	y_scale = floor(ADC_VOLTAGE_SCALE/screen_y);
	
	// Check waveform and plot
	int found_trigger = 0;
	int last = -1;
	int current = 0;
	while(!found_trigger)
	{
		// Trigger an ADC
		adc_base[0] = 1;
		current = adc_base[0];
		// Look for a trigger
		if (last != -1 && abs(last - current) > 3000)
		{
			found_trigger = 1;
			// Found trigger, delay until next reading.
			oscilloscope_array[0] = current;
			oscilloscope_index = 1;
			timer_settime (interval_timer_id, 0, &interval_timer_start, NULL);
		}
		last = current;
	}

	while(1);
	// End, should never be reached
	munmap(adc_base, ADC_SPAN);
	sprintf(VIDEO_BUF, "clear");
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);
	sprintf(VIDEO_BUF, "erase");
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);
	sprintf(VIDEO_BUF, "sync");
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);
	close(video_FD);
	close(mem_FD);
	return 0;
}
