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

#define key_Q 0x10
#define key_2 0x3
#define key_W 0x11
#define key_3 0x4
#define key_E 0x12
#define key_R 0x13
#define key_5 0x6
#define key_T 0x14
#define key_6 0x7
#define key_Y 0x15
#define key_7 0x8
#define key_U 0x16
#define key_I 0x17
#define key_space 0x39
#define key_A 0x1e
#define key_Z 0x2c
#define key_S 0x1f
#define key_X 0x2d
#define key_O 0x18
#define key_P 0x19

#define AUDIO_BASE 0xFF203000
#define AUDIO_SPAN 0x100
//PI, the circle constant, not the desert
#define PI 3.14159265
#define SAMPLING_RATE 8000
#define SAMPLES_PER_NOTE 2400
//How many radians pass each sample if it goes 2 pi every second.
#define RADS_PER_TIMESTEP (PI*2/SAMPLING_RATE)
//This is the volume the keyboard plays at
#define MAX_VOLUME 0x3fffffff
//The per cycle fade of the pressed keys
#define FADE_FACTOR 0.9998
#define KEY_RELEASED 0
#define KEY_PRESSED 1
//The beta as defined in the EMA equation. We divide this by 10 so it is really a beta of 0.9
#define beta 9
#define MAX_RECORDING_SIZE 2400000
//Limits on the volume so that it doesn't go negative or too high in the positive direction
#define MIN_FILTER_VOLUME 0
#define MAX_FILTER_VOLUME 125
//Bank of frequencies corresponding to each piano note
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
//The fade array to say how loud each tone should be right now
int32_t fade[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//The space bar held flag is 1 if the key is released but the space bar is pressed meaning we should 
//remove that key's sound if the space bar is released
int32_t space_bar_held_flag[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//Array to hold our recordings, one for the left and right side since two mic inputs
int64_t recording_l [MAX_RECORDING_SIZE];
int64_t recording_r [MAX_RECORDING_SIZE];
//various flags are initialized
int32_t recording_size=0;
int32_t recording_state = 0;
int32_t space_bar_state = 0;
int32_t playback_state = 0;
int32_t key_fd = 0;
int32_t fd;
int32_t* audio_base;
//microphone specific values
int64_t high_volume;
int64_t low_volume;
int64_t ema_value_left;
int64_t ema_value_right;
int32_t initialized;
int64_t hfs_value_left;
int64_t hfs_value_right;

pthread_t keyboard_thread_id;
pthread_mutex_t mutex_tone_volume;

void update_ema (int64_t new_sample_left, int64_t new_sample_right)
{
	if (initialized == 0)
	{
		//for the first sample point assume half the signal 
		//is high frequency and half is low frequency, we will tune it as we get more points
		initialized = 1;
		ema_value_left = new_sample_left / 2;
		ema_value_right = new_sample_right / 2;
	}
	else
	{
		//Calculate the ema of the next step
		ema_value_left = ema_value_left / 10 * beta + new_sample_left / 10 * (10 - beta);
		ema_value_right = ema_value_right / 10 * beta + new_sample_right / 10 * (10 - beta);
	}
	//Calculate the HFS of the next step
	hfs_value_left = new_sample_left - ema_value_left;
	hfs_value_right = new_sample_right - ema_value_right;
}

void handle_int(int32_t dummy)
{
	//Handle Ctrl - C
	pthread_cancel(keyboard_thread_id);
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
		//Lock the volume control signals as they can be modified here or in the main thread
		pthread_mutex_lock (&mutex_tone_volume);
		if (ev.type == EV_KEY && ev.value == KEY_PRESSED)
		{
			switch (ev.code)
			{
				case key_Q:
					//Set the volume of the first frequency to max keyboard volume because the key is pressed
					fade[0] = MAX_VOLUME;
					//space bar held flag is 1 for values that are held because of the damp pedal that should 
					//be reset when the pedal is released. since we pressed it again we should clear the flag 
					//so that it is not reset if the space is released before the key
					space_bar_held_flag[0] = 0;
					break;
				case key_2:
					//same logic applies to all keys
					fade[1] = MAX_VOLUME;
					space_bar_held_flag[1] = 0;
					break;
				case key_W:
					fade[2] = MAX_VOLUME;
					space_bar_held_flag[2] = 0;
					break;
				case key_3:
					fade[3] = MAX_VOLUME;
					space_bar_held_flag[3] = 0;
					break;
				case key_E:
					fade[4] = MAX_VOLUME;
					space_bar_held_flag[4] = 0;
					break;
				case key_R:
					fade[5] = MAX_VOLUME;
					space_bar_held_flag[5] = 0;
					break;
				case key_5:
					fade[6] = MAX_VOLUME;
					space_bar_held_flag[6] = 0;
					break;
				case key_T:
					fade[7] = MAX_VOLUME;
					space_bar_held_flag[7] = 0;
					break;
				case key_6:
					fade[8] = MAX_VOLUME;
					space_bar_held_flag[8] = 0;
					break;
				case key_Y:
					fade[9] = MAX_VOLUME;
					space_bar_held_flag[9] = 0;
					break;
				case key_7:
					fade[10] = MAX_VOLUME;
					space_bar_held_flag[10] = 0;
					break;
				case key_U:
					fade[11] = MAX_VOLUME;
					space_bar_held_flag[11] = 0;
					break;
				case key_I:
					fade[12] = MAX_VOLUME;
					space_bar_held_flag[12] = 0;
					break;
				case key_space:
					//set the flag that we are holding the space bar
					space_bar_state =1;
					break;
				case key_A:
					//Raise the low frequency volume by 1
					if (low_volume < MAX_FILTER_VOLUME)
					{
						low_volume = low_volume + 1;
					}
					printf("New volume for low frequency is %llu\n", low_volume);
					break;
				case key_Z:
					//Lower the low frequency volume by 1
					if (low_volume > MIN_FILTER_VOLUME)
					{
						low_volume = low_volume - 1;
					}
					printf("New volume for low frequency is %llu\n", low_volume);
					break;
				case key_S:
					//Raise the high frequency volume by 1
					if (high_volume < MAX_FILTER_VOLUME)
					{
						high_volume = high_volume + 1;
					}
					printf("New volume for high frequency is %llu\n", high_volume);
					break;
				case key_X:
					//Lower the high frequency volume by 1
					if (high_volume > MIN_FILTER_VOLUME)
					{
						high_volume = high_volume - 1;
					}
					printf("New volume for high frequency is %llu\n", high_volume);
					break;
				case key_O:
					//toggle recording
					if (recording_state == 0)
					{
						//If not recording, turn it on and stop playback if it is running
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
						//If it is recording stop it
						printf("Finished Recording\n");
						recording_state = 0;
					}
					break;
				case key_P:
					//toggle Playback
					if (playback_state == 0)
					{
						//If not doing playback, turn it on and stop recording if it is running
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
						//If it is playing back stop it
						printf("Finished Playback\n");
						playback_state = 0;
					}
			}
		}
		else if (ev.type == EV_KEY && ev.value == KEY_RELEASED && space_bar_state == 0)
		{
			//A key is released but the space bar is not pressed, we will set its fade to 0 to stop it here
			switch (ev.code)
			{
				case key_Q:
					fade[0] = 0;
					break;
				case key_2:
					fade[1] = 0;
					break;
				case key_W:
					fade[2] = 0;
					break;
				case key_3:
					fade[3] = 0;
					break;
				case key_E:
					fade[4] = 0;
					break;
				case key_R:
					fade[5] = 0;
					break;
				case key_5:
					fade[6] = 0;
					break;
				case key_T:
					fade[7] = 0;
					break;
				case key_6:
					fade[8] = 0;
					break;
				case key_Y:
					fade[9] = 0;
					break;
				case key_7:
					fade[10] = 0;
					break;
				case key_U:
					fade[11] = 0;
					break;
				case key_I:
					fade[12] = 0;
					break;
			}
		}
		else if (ev.type == EV_KEY && ev.value == KEY_RELEASED && space_bar_state == 1)
		{
			//A key is released but the dampening pedal is on, if it is a note, 
			//we will just flag it as not pressed but do nothing to the sound
			switch(ev.code)
			{
				case key_Q:
					space_bar_held_flag[0] = 1;
					break;
				case key_2:
					space_bar_held_flag[1] = 1;
					break;
				case key_W:
					space_bar_held_flag[2] = 1;
					break;
				case key_3:
					space_bar_held_flag[3] = 1;
					break;
				case key_E:
					space_bar_held_flag[4] = 1;
					break;
				case key_R:
					space_bar_held_flag[5] = 1;
					break;
				case key_5:
					space_bar_held_flag[6] = 1;
					break;
				case key_T:
					space_bar_held_flag[7] = 1;
					break;
				case key_6:
					space_bar_held_flag[8] = 1;
					break;
				case key_Y:
					space_bar_held_flag[9] = 1;
					break;
				case key_7:
					space_bar_held_flag[10] = 1;
					break;
				case key_U:
					space_bar_held_flag[11] = 1;
					break;
				case key_I:
					space_bar_held_flag[12] = 1;
					break;
				case key_space:
					//The key released is the dampening pedal, set the flag off 
					//and turn off all tones that are released
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
		//Release the lock
		pthread_mutex_unlock (&mutex_tone_volume);
	}
}


int32_t main(int32_t argc, char** argv)
{
	//We initialize at full volume
	int32_t err;
	high_volume = 100;
	low_volume = 100;
	initialized = 0;
	//safely open the keyboard driver and /dev/mem for memory access
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
	//Get the base address of the audio hardware
	audio_base = (int32_t *) mmap (NULL, AUDIO_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, AUDIO_BASE);
	if (audio_base == MAP_FAILED)
	{
		printf ("ERROR: mmap() failed...\n");
		close (fd);
		close(key_fd);
		return -1;
	}
	//spawn a thread to listen to the keyboard
	if ((err = pthread_create(&keyboard_thread_id, NULL, &keyboard_thread, NULL)) != 0 )
	{
		printf("thread creation failed \n");
		close (fd);
		close(key_fd);
		munmap(audio_base, AUDIO_SPAN);
		return -1;
	}
	//add ctrl-c support
	signal(SIGINT, handle_int);
	//Start with no credits
	int32_t read_credits = 0;
	int32_t write_credits = 0;
	int32_t playback_val = 0;
	//reset the audio to have nothing to read in the mic and nothing written
	audio_base[0x10] = audio_base[0x10] | 12;
	usleep(100000);
	audio_base[0x10] = audio_base[0x10] & (~12);
	while(1)
	{
		for (int32_t j = 0; j < SAMPLING_RATE; j++)
		{
			while (write_credits == 0)
			{
				//Get the credits of the write portion (number of things to read)
				uint32_t fs = audio_base[0x11];
				uint32_t fsl = fs >> 24;
				uint32_t fsr = (fs >> 16) & 0xFF;
				write_credits = fsl < fsr ? fsl : fsr;
			}
			while (read_credits == 0)
			{
				//Get the credits of the read portion (number of slots available to write)
				uint32_t fs = audio_base[0x11];
				uint32_t fsl = (fs >> 8) & 0xFF;
				uint32_t fsr = fs & 0xFF;
				read_credits = fsl < fsr ? fsl : fsr;
			}
			int64_t next_val = 0;
			//lock the keyboard thread
			pthread_mutex_lock (&mutex_tone_volume);
			if (playback_state)
			{
				//get the next recorded value and play it
				audio_base[0x12] = recording_l[playback_val];
				audio_base[0x13] = recording_r[playback_val++];
				if (playback_val >= recording_size)
				{
					//We are at the end of the recording, flag it as such and clear the mic input
					//to clear the junk data we built up while doing the playback
					playback_val = 0;
					playback_state = 0;
					printf("Finished Playback\n");
					audio_base[0x10] = audio_base[0x10] | 12;
					pthread_mutex_unlock (&mutex_tone_volume);
					usleep(100000);
					pthread_mutex_lock (&mutex_tone_volume);
					audio_base[0x10] = audio_base[0x10] & (~12);
				}
				//decrement the write credits since we did a write
				write_credits--;
			}
			else
			{
				//add keyboard sounds
				for (int32_t i = 0; i < 13; i++)
				{
					//Move from frequency domain to time domain based on the current time step
					next_val += (fade[i] * sin(RADS_PER_TIMESTEP * j * FREQS[i])) / 13;
					//lower the key value by a multiple close to 1 so it slowly decreases 
					//in volume the longer the key is held
					fade[i] = fade[i] * FADE_FACTOR;
				}
				int64_t undivided_val_l = 0;
				int64_t undivided_val_r = 0;
				//update the ema
				update_ema(audio_base[0x12], audio_base[0x13]);
				//calculate the left and right audio by multiplying EMA and HFS with its volume and
				//scaling by dividing it by 100. Add in the keyboard value
				undivided_val_l = ema_value_left * low_volume + hfs_value_left * high_volume;
				undivided_val_l = undivided_val_l/100 + next_val;
				undivided_val_r = ema_value_right * low_volume + hfs_value_right * high_volume;
				undivided_val_r = undivided_val_r/100 + next_val;
				if (recording_size == MAX_RECORDING_SIZE)
				{
					//We have recorded the maximum possible amount
					printf("Finished Recording\n");
					recording_state = 0;
				}
				if (recording_state == 1)
				{
					//If we are recording, add this value to the array of recordings
					recording_l[recording_size] = (int32_t) (undivided_val_l);
					recording_r[recording_size++] = (int32_t) (undivided_val_r);
				}
				//write out the value to the hardware
				audio_base[0x12] = undivided_val_l;
				audio_base[0x13] = undivided_val_r;
				//decrement the credits
				read_credits--;
				write_credits--;
			}
			//release the keyboard lock
			pthread_mutex_unlock (&mutex_tone_volume);
		}
	}
	//cancel the thread
	pthread_cancel(keyboard_thread_id);
	//unmap the audio
	munmap(audio_base, AUDIO_SPAN);
	//close the character devices
	close(fd);
	close(key_fd);
	//return
	return 0;
}
