#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
#define MAX_VOLUME 0x3fffffff
#define FADE_FACTOR 0.9998
#define KEY_RELEASED 0
#define KEY_PRESSED 1
#define beta 9
#define MAX_RECORDING_SIZE 2400000
#define MIN_FILTER_VOLUME 0
#define MAX_FILTER_VOLUME 125

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

int32_t fade[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int32_t space_bar_held_flag[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int64_t recording_l [MAX_RECORDING_SIZE];
int64_t recording_r [MAX_RECORDING_SIZE];
int32_t recording_size=0;
int32_t recording_state = 0;
int32_t space_bar_state = 0;
int32_t playback_state = 0;
int32_t key_fd = 0;
int32_t fd;
int32_t* audio_base;
int64_t high_volume;
int64_t low_volume;
int64_t ema_value_left;
int64_t ema_value_right;
int32_t initialized;
int64_t hfs_value_left;
int64_t hfs_value_right;

pthread_t tid;
pthread_mutex_t mutex_tone_volume;

void update_ema (int64_t new_sample_left, int64_t new_sample_right)
{
	if (initialized == 0)
	{
		initialized = 1;
		ema_value_left = new_sample_left / 2;
		ema_value_right = new_sample_right / 2;
	}
	else
	{
		ema_value_left = ema_value_left / 10 * beta + new_sample_left / 10 * (10 - beta);
		ema_value_right = ema_value_right / 10 * beta + new_sample_right / 10 * (10 - beta);
	}
	hfs_value_left = new_sample_left - ema_value_left;
	hfs_value_right = new_sample_right - ema_value_right;
}

void handle_int(int32_t dummy)
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
	int32_t event_size = sizeof (struct input_event);
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
					space_bar_held_flag[0] = 0;
					break;
				case 0x3:
					fade[1] = MAX_VOLUME;
					space_bar_held_flag[1] = 0;
					break;
				case 0x11:
					fade[2] = MAX_VOLUME;
					space_bar_held_flag[2] = 0;
					break;
				case 0x4:
					fade[3] = MAX_VOLUME;
					space_bar_held_flag[3] = 0;
					break;
				case 0x12:
					fade[4] = MAX_VOLUME;
					space_bar_held_flag[4] = 0;
					break;
				case 0x13:
					fade[5] = MAX_VOLUME;
					space_bar_held_flag[5] = 0;
					break;
				case 0x6:
					fade[6] = MAX_VOLUME;
					space_bar_held_flag[6] = 0;
					break;
				case 0x14:
					fade[7] = MAX_VOLUME;
					space_bar_held_flag[7] = 0;
					break;
				case 0x7:
					fade[8] = MAX_VOLUME;
					space_bar_held_flag[8] = 0;
					break;
				case 0x15:
					fade[9] = MAX_VOLUME;
					space_bar_held_flag[9] = 0;
					break;
				case 0x8:
					fade[10] = MAX_VOLUME;
					space_bar_held_flag[10] = 0;
					break;
				case 0x16:
					fade[11] = MAX_VOLUME;
					space_bar_held_flag[11] = 0;
					break;
				case 0x17:
					fade[12] = MAX_VOLUME;
					space_bar_held_flag[12] = 0;
					break;
				case 0x39:
					space_bar_state =1;
					break;
				case 0x1e:
					if (low_volume < MAX_FILTER_VOLUME)
					{
						low_volume = low_volume + 1;
					}
					printf("New volume for low frequency is %llu\n", low_volume);
					break;
                case 0x2c:
					if (low_volume > MIN_FILTER_VOLUME)
					{
						low_volume = low_volume - 1;
					}
                    printf("New volume for low frequency is %llu\n", low_volume);
					break;
                case 0x1f:
					if (high_volume < MAX_FILTER_VOLUME)
					{
						high_volume = high_volume + 1;
					}
                    printf("New volume for high frequency is %llu\n", high_volume);
					break;
                case 0x2d:
					if (high_volume > MIN_FILTER_VOLUME)
					{
						high_volume = high_volume - 1;
					}
                    printf("New volume for high frequency is %llu\n", high_volume);
					break;
				case 0x18:
					if (recording_state == 0)
					{
						if (playback_state == 1)
						{
							playback_state = 0;
							printf("Finished Playback\n");
						}
						printf("Recording\n");
						recording_size = 0;
						recording_state = 1;
					}
					else
					{
						printf("Finished Recording\n");
						recording_state = 0;
					}
					break;
				case 0x19:
					if (playback_state == 0)
					{
						if (recording_state == 1)
						{
							recording_state = 0;
							printf("Finished Recording\n");
						}
						printf("Playing Back\n");
						playback_state = 1;
					}
					else
					{
						printf("Finished Playback\n");
						playback_state = 0;
					}
			}
		}
		else if (ev.type == EV_KEY && ev.value == KEY_RELEASED && space_bar_state == 0)
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
		else if (ev.type == EV_KEY && ev.value == KEY_RELEASED && space_bar_state == 1)
		{
			switch(ev.code)
			{
				case 0x10:
					space_bar_held_flag[0] = 1;
					break;
				case 0x3:
					space_bar_held_flag[1] = 1;
					break;
				case 0x11:
					space_bar_held_flag[2] = 1;
					break;
				case 0x4:
					space_bar_held_flag[3] = 1;
					break;
				case 0x12:
					space_bar_held_flag[4] = 1;
					break;
				case 0x13:
					space_bar_held_flag[5] = 1;
					break;
				case 0x6:
					space_bar_held_flag[6] = 1;
					break;
				case 0x14:
					space_bar_held_flag[7] = 1;
					break;
				case 0x7:
					space_bar_held_flag[8] = 1;
					break;
				case 0x15:
					space_bar_held_flag[9] = 1;
					break;
				case 0x8:
					space_bar_held_flag[10] = 1;
					break;
				case 0x16:
					space_bar_held_flag[11] = 1;
					break;
				case 0x17:
					space_bar_held_flag[12] = 1;
					break;
				case 0x39:
					space_bar_state = 0;
					for(int32_t i=0; i<13; i++)
					{
						if (space_bar_held_flag[i] == 1)
						{
							fade[i]=0;
						}
					}
					break;
			}
		}
		pthread_mutex_unlock (&mutex_tone_volume);
	}
}


int32_t main(int32_t argc, char** argv)
{
	int32_t err;
	high_volume = 100;
	low_volume = 100;
	initialized = 0;
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
	audio_base = (int32_t *) mmap (NULL, AUDIO_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, AUDIO_BASE);
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
	
	int32_t read_credits = 0;
	int32_t write_credits = 0;
	int32_t playback_val = 0;
	audio_base[0x10] = audio_base[0x10] | 12;
	usleep(100000);
	audio_base[0x10] = audio_base[0x10] & (~12);
	while(1)
	{
		for (int32_t j = 0; j < SAMPLING_RATE; j++)
		{
			while (write_credits == 0)
			{
				uint32_t fs = audio_base[0x11];
				uint32_t fsl = fs >> 24;
				uint32_t fsr = (fs >> 16) & 0xFF;
				write_credits = fsl < fsr ? fsl : fsr;
			}
			while (read_credits == 0)
			{
				uint32_t fs = audio_base[0x11];
				uint32_t fsl = (fs >> 8) & 0xFF;
				uint32_t fsr = fs & 0xFF;
				read_credits = fsl < fsr ? fsl : fsr;
			}
			int64_t next_val = 0;
			pthread_mutex_lock (&mutex_tone_volume);
			if (playback_state)
			{
				audio_base[0x12] = recording_l[playback_val];
				audio_base[0x13] = recording_r[playback_val++];
				if (playback_val >= recording_size)
				{
					playback_val = 0;
					playback_state = 0;
					printf("Finished Playback\n");
					audio_base[0x10] = audio_base[0x10] | 12;
					usleep(100000);
					audio_base[0x10] = audio_base[0x10] & (~12);
				}
				write_credits--;
			}
			else
			{
				//add keyboard sounds
				for (int32_t i = 0; i < 13; i++)
				{
					next_val += (fade[i] * sin(RADS_PER_TIMESTEP * j * FREQS[i])) / 13;
					fade[i] = fade[i] * FADE_FACTOR;
				}
				int64_t undivided_val_l = 0;
				int64_t undivided_val_r = 0;
				update_ema(audio_base[0x12], audio_base[0x13]);
				undivided_val_l = ema_value_left * low_volume + hfs_value_left * high_volume;
				undivided_val_l = undivided_val_l/100 + next_val;
				undivided_val_r = ema_value_right * low_volume + hfs_value_right * high_volume;
				undivided_val_r = undivided_val_r/100 + next_val;
				if (recording_size == MAX_RECORDING_SIZE)
				{
					printf("Finished Recording\n");
					recording_state = 0;
				}
				if (recording_state == 1)
				{
					recording_l[recording_size] = (int32_t) (undivided_val_l);
					recording_r[recording_size++] = (int32_t) (undivided_val_r);
				}
				audio_base[0x12] = undivided_val_l;
				audio_base[0x13] = undivided_val_r;
				read_credits--;
				write_credits--;
			}
			pthread_mutex_unlock (&mutex_tone_volume);
		}
	}
	
	pthread_cancel(tid);
	munmap(audio_base, AUDIO_SPAN);
    close(fd);
	close(key_fd);
    return 0;
}
