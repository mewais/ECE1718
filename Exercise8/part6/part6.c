#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
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
#define STORE_SIZE 20	// seconds
#define BUF_SIZE 64
#define STOPWATCH_SIZE 9

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
int audio_fd;
int sw_fd;
int led_fd;
int stopwatch_fd;
char AUDIO_BUF[BUF_SIZE];
char LEDR_BUF[BUF_SIZE];
char STOPWATCH_BUF[STOPWATCH_SIZE];
char OLD_STOPWATCH_BUF[STOPWATCH_SIZE];

pthread_t tid;
pthread_mutex_t mutex_tone_volume;
pthread_mutex_t record_playback;

int key_store[STORE_SIZE*100][13];
int record = 0;
int playback = 0;
int store_index = 0;

void handle_int(int dummy)
{
	pthread_cancel(tid);
	sprintf(STOPWATCH_BUF, "stop");
	write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
	sprintf(STOPWATCH_BUF, "00:00:00");
	write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
	sprintf(LEDR_BUF,"%x",0);
	write(led_fd,LEDR_BUF,strlen(LEDR_BUF)+1);
	close(key_fd);
	close(audio_fd);
	close(sw_fd);
	close(led_fd);
	close(stopwatch_fd);
	exit(0);
}


void *keyboard_thread () 
{
	struct input_event ev;
	int event_size = sizeof (struct input_event);
	while (1)
	{
		// Check for record and playback
		char SW_BUF [3];
		while(read(sw_fd,SW_BUF,2)!=0);
		pthread_mutex_lock (&record_playback);
		if (SW_BUF[0] == '1')
		{
			if (!playback)
			{
				record ^= 1;
				// If recording new
				if (record == 1)
				{
					// Cleanup the store
					for (int i = 0; i < STORE_SIZE; i++)
					{
						for (int j = 0; j < 13; j++)
						{
							key_store[i][j] = 0;
						}
					}
					printf("Recording...\n");
					sprintf(LEDR_BUF,"%x",1);
					write(led_fd,LEDR_BUF,strlen(LEDR_BUF)+1);
					sprintf(STOPWATCH_BUF, "run");
					write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
					while(read(stopwatch_fd, OLD_STOPWATCH_BUF, STOPWATCH_SIZE) != 0);
					OLD_STOPWATCH_BUF[STOPWATCH_SIZE-1] = '\0';
				}
				else
				{
					printf("Finished Recording\n");
					sprintf(STOPWATCH_BUF, "stop");
					write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
					sprintf(STOPWATCH_BUF, "59:59:99");
					write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
					sprintf(LEDR_BUF,"%x",0);
					write(led_fd,LEDR_BUF,strlen(LEDR_BUF)+1);
				}
				store_index = 0;
			}
		}
		else if (SW_BUF[0] == '2')
		{
			if (!record)
			{
				playback ^= 1;
				if (playback == 1)
				{
					printf("Playing back...\n");
					sprintf(STOPWATCH_BUF, "run");
					write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
					while(read(stopwatch_fd, OLD_STOPWATCH_BUF, STOPWATCH_SIZE) != 0);
					OLD_STOPWATCH_BUF[STOPWATCH_SIZE-1] = '\0';
					sprintf(LEDR_BUF,"%x",2);
					write(led_fd,LEDR_BUF,strlen(LEDR_BUF)+1);
				}
				else
				{
					printf("Finished Playback\n");
					sprintf(STOPWATCH_BUF, "stop");
					write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
					sprintf(STOPWATCH_BUF, "59:59:99");
					write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
					sprintf(LEDR_BUF,"%x",0);
					write(led_fd,LEDR_BUF,strlen(LEDR_BUF)+1);
				}
			}
		}
		// If we're not playing back a record
		if (!playback)
		{
			pthread_mutex_unlock (&record_playback);
			// Check if we have keyboard action
			if (read (key_fd, &ev, event_size) < event_size) {
				continue;
			}
			// If key presses, handle value
			if (ev.type == EV_KEY && ev.value == KEY_PRESSED)
			{
				pthread_mutex_lock (&mutex_tone_volume);
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
				// printf("%s %s %i\n", STOPWATCH_BUF, OLD_STOPWATCH_BUF, is_equal);
						break;
					case 0x7:
						fade[8] = MAX_VOLUME;
						break;
					case 0x15:
				printf("NO: %s %s\n", STOPWATCH_BUF, OLD_STOPWATCH_BUF);
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
				pthread_mutex_unlock (&mutex_tone_volume);
			}
			// If key released, handle value
			else if (ev.type == EV_KEY && ev.value == KEY_RELEASED)
			{
				pthread_mutex_lock (&mutex_tone_volume);
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
				pthread_mutex_unlock (&mutex_tone_volume);
			}
		}
		else
		{
			pthread_mutex_unlock (&record_playback);
		}
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
	if ((audio_fd = open( "/dev/IntelFPGAUP/audio", O_RDWR)) == -1)
	{
		printf ("ERROR could not open /dev/audio\n");
		close(key_fd);
		return -1;
	}
	if ((sw_fd = open( "/dev/KEY", O_RDONLY)) == -1)
	{
		printf ("ERROR could not open /dev/KEY\n");
		close(key_fd);
		close(audio_fd);
		return -1;
	}
	if ((led_fd = open( "/dev/LEDR", O_WRONLY)) == -1)
	{
		printf ("ERROR could not open /dev/LEDR\n");
		close(key_fd);
		close(audio_fd);
		close(sw_fd);
		return -1;
	}
	if ((stopwatch_fd = open( "/dev/stopwatch", O_RDWR)) == -1)
	{
		printf ("ERROR could not open /dev/stopwatch\n");
		close(key_fd);
		close(audio_fd);
		close(sw_fd);
		close(led_fd);
		return -1;
	}


	if ((err = pthread_create(&tid, NULL, &keyboard_thread, NULL)) != 0 )
	{
		printf("thread creation failed \n");
		close(key_fd);
		close(audio_fd);
		close(sw_fd);
		close(led_fd);
		close(stopwatch_fd);

		return -1;
	}

	// Handle ctrl + c
	signal(SIGINT, handle_int);
	
	// Setup audio
	sprintf(AUDIO_BUF, "init");
	write(audio_fd, AUDIO_BUF, 64);
	
	sprintf(STOPWATCH_BUF, "stop");
	write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
	sprintf(STOPWATCH_BUF, "59:59:99");
	write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);

	while(1)
	{
		for (int j = 0; j < SAMPLING_RATE; j++)
		{
			// How much space left in the buffer, wait until there's
			int next_val = 0;
			sprintf(AUDIO_BUF, "waitw");
			write(audio_fd, AUDIO_BUF, 64);
			// Check if the stopwatch has changed (i.e. 100th of second passed)
			pthread_mutex_lock (&record_playback);
			while(read(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE) != 0);
			STOPWATCH_BUF[STOPWATCH_SIZE-1] = '\0';
			int is_equal = strcmp(STOPWATCH_BUF, OLD_STOPWATCH_BUF);
			// Read the frequencies and actually play
			if (playback)
			{
				for (int i = 0; i < 13; i++)
				{
					// Get the saved frequencies
					next_val += (key_store[store_index][i] * sin(RADS_PER_TIMESTEP * j * FREQS[i])) / 13;
					// Record every 100th of a second if we are told to do so
					if (is_equal != 0 && i == 12 && store_index < STORE_SIZE*100)
					{
						strcpy(OLD_STOPWATCH_BUF, STOPWATCH_BUF);
						store_index++;
					}
				}
			}
			else
			{
				// Just play what the keyboard presses are
				pthread_mutex_lock (&mutex_tone_volume);
				for (int i = 0; i < 13; i++)
				{
					next_val += (fade[i] * sin(RADS_PER_TIMESTEP * j * FREQS[i])) / 13;
					fade[i] = fade[i] * FADE_FACTOR;
					// Record every 100th of a second if we are told to do so
					if (is_equal != 0 && record && store_index < STORE_SIZE*100)
					{
						strcpy(OLD_STOPWATCH_BUF, STOPWATCH_BUF);
						key_store[store_index][i] = fade[i];
						if (i == 12)
						{
							store_index++;
						}
					}
				}
				pthread_mutex_unlock (&mutex_tone_volume);
			}
			pthread_mutex_unlock (&record_playback);
			sprintf(AUDIO_BUF, "left %i", next_val);
			write(audio_fd, AUDIO_BUF, 64);
			sprintf(AUDIO_BUF, "right %i", next_val);
			write(audio_fd, AUDIO_BUF, 64);
		}
	}
	
	pthread_cancel(tid);
	sprintf(STOPWATCH_BUF, "stop");
	write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
	sprintf(STOPWATCH_BUF, "00:00:00");
	write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
    close(audio_fd);
	close(key_fd);
	close(sw_fd);
	close(led_fd);
	close(stopwatch_fd);
    return 0;
}
