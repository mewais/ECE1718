#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/mman.h>
#include <pthread.h>
#include <linux/input.h>
#include <signal.h>
#define AUDIO_BASE 0xFF203000
#define AUDIO_SPAN 0x100
#define PI 3.14159265
#define SAMPLING_RATE 8000
#define SAMPLES_PER_NOTE 2400
#define RADS_PER_TIMESTEP (PI*2/SAMPLING_RATE)
#define MAX_VOLUME 0x7fffffff
#define FADE_FACTOR 0.9998
#define KEY_RELEASED 0
#define KEY_PRESSED 1

float FREQS[] =	{261.626,
				 277.183,
				 293.665,
				 311.127,
				 329.628,
				 349.228,
				 369.994,
				 391.995,
				 415.305,
				 440.000,
				 466.164,
				 493.883,
				 523.251};

int fade[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
int key_fd = 0;
int fd;
int* audio_base;

pthread_t tid;
pthread_mutex_t mutex_tone_volume;

void handle_int(int dummy)
{
	pthread_cancel(tid);
	munmap(audio_base, AUDIO_SPAN);
    close(fd);
	close(key_fd);
	exit(0);
}

void *keyboard_thread () 
{
	struct input_event ev;
	int event_size = sizeof (struct input_event);
	while (1)
	{
		if (read (key_fd, &ev, event_size) < event_size) {
			continue;
		}
		pthread_mutex_lock (&mutex_tone_volume);
		if (ev.type == EV_KEY && ev.value == KEY_PRESSED)
		{
			switch (ev.code)
			{
				case 0x10:
					fade[0] = MAX_VOLUME;
					break;
				case 0x3:
					fade[1] = MAX_VOLUME;
					break;
				case 0x11:
					fade[2] = MAX_VOLUME;
					break;
				case 0x4:
					fade[3] = MAX_VOLUME;
					break;
				case 0x12:
					fade[4] = MAX_VOLUME;
					break;
				case 0x13:
					fade[5] = MAX_VOLUME;
					break;
				case 0x6:
					fade[6] = MAX_VOLUME;
					break;
				case 0x14:
					fade[7] = MAX_VOLUME;
					break;
				case 0x7:
					fade[8] = MAX_VOLUME;
					break;
				case 0x15:
					fade[9] = MAX_VOLUME;
					break;
				case 0x8:
					fade[10] = MAX_VOLUME;
					break;
				case 0x16:
					fade[11] = MAX_VOLUME;
					break;
				case 0x17:
					fade[12] = MAX_VOLUME;
					break;
			}
		}
		else if (ev.type == EV_KEY && ev.value == KEY_RELEASED)
		{
			switch (ev.code)
			{
				case 0x10:
					fade[0] = 0;
					break;
				case 0x3:
					fade[1] = 0;
					break;
				case 0x11:
					fade[2] = 0;
					break;
				case 0x4:
					fade[3] = 0;
					break;
				case 0x12:
					fade[4] = 0;
					break;
				case 0x13:
					fade[5] = 0;
					break;
				case 0x6:
					fade[6] = 0;
					break;
				case 0x14:
					fade[7] = 0;
					break;
				case 0x7:
					fade[8] = 0;
					break;
				case 0x15:
					fade[9] = 0;
					break;
				case 0x8:
					fade[10] = 0;
					break;
				case 0x16:
					fade[11] = 0;
					break;
				case 0x17:
					fade[12] = 0;
					break;
			}
		}
		pthread_mutex_unlock (&mutex_tone_volume);
	}
}


int main(int argc, char** argv)
{
	int err;
	if (argv[1] == NULL)
	{
		printf("Path not specified\n");
		return -1;
	}
	if ((key_fd = open (argv[1], O_RDONLY | O_NONBLOCK ) ) == -1 )
	{
		printf ("Could not open keyboard \n");
		return -1;
	}
	if ((fd = open( "/dev/mem", (O_RDWR | O_SYNC))) == -1)
	{
		printf ("ERROR could not open /dev/mem\n");
		close(key_fd);
		return -1;
	}
	audio_base = (int *) mmap (NULL, AUDIO_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, AUDIO_BASE);
	if (audio_base == MAP_FAILED)
	{
		printf ("ERROR: mmap() failed...\n");
		close (fd);
		close(key_fd);
		return -1;
	}
	if ((err = pthread_create(&tid, NULL, &keyboard_thread, NULL)) != 0 )
	{
		printf("thread creation failed \n");
		close (fd);
		close(key_fd);
		munmap(audio_base, AUDIO_SPAN);
		return -1;
	}

	signal(SIGINT, handle_int);
	
	int credits = 0;
	audio_base[0x10] = audio_base[0x10] | 8;
	usleep(100000);
	audio_base[0x10] = audio_base[0x10] & (~8);
	while(1)
	{
		for (int j = 0; j < SAMPLING_RATE; j++)
		{
			while (credits == 0)
			{
				unsigned int fs = audio_base[0x11];
				unsigned int fsl = fs >> 24;
				unsigned int fsr = (fs >> 16) & 0xFF;
				credits = fsl < fsr ? fsl : fsr;
			}
			int next_val = 0;
			pthread_mutex_lock (&mutex_tone_volume);
			for (int i = 0; i < 13; i++)
			{
				next_val += (fade[i] * sin(RADS_PER_TIMESTEP * j * FREQS[i])) / 13;
				fade[i] = fade[i] * FADE_FACTOR;
			}
			pthread_mutex_unlock (&mutex_tone_volume);
			audio_base[0x12] = next_val;
			audio_base[0x13] = next_val;
			credits --;
		}
	}
	
	pthread_cancel(tid);
	munmap(audio_base, AUDIO_SPAN);
    close(fd);
	close(key_fd);
    return 0;
}
