#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <sys/mman.h>

#define AUDIO_BASE 0xFF203000
#define AUDIO_SPAN 0x100
#define PI 3.14159265
#define SAMPLING_RATE 8000
#define SAMPLES_PER_NOTE 2400
#define RADS_PER_TIMESTEP (PI*2/SAMPLING_RATE)
#define MAX_VOLUME 0x7fffffff



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

int* audio_base;

int main(int argc, char** argv)
{
	int fd;
	int enable[13] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}; 
	if ((fd = open( "/dev/mem", (O_RDWR | O_SYNC))) == -1)
	{
		printf ("ERROR could not open /dev/mem\n");
		return -1;
	}
	audio_base = (int *) mmap (NULL, AUDIO_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, AUDIO_BASE);
	if (audio_base == MAP_FAILED)
	{
		printf ("ERROR: mmap() failed...\n");
		close (fd);
		return -1;
	}

	if (argc == 2)
	{
		for (int i = 0; i < 13; i++)
		{
			if (argv[1][i] != '0' && argv[1][i] != '1')
			{
				printf("Wrong Input Value.\n");
				return 0;
			}
			enable[i] = argv[1][i] - '0';
		}
	}

	int credits = 0;
	audio_base[0x10] = audio_base[0x10] | 8;
	usleep(100000);
	audio_base[0x10] = audio_base[0x10] & (~8);
	// while(1)
	// {
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
			for (int i = 0; i < 13; i++)
			{
				if (enable[i])
					next_val += (MAX_VOLUME * sin(RADS_PER_TIMESTEP * j * FREQS[i])) / 13;
			}
			audio_base[0x12] = next_val;
			audio_base[0x13] = next_val;
			credits --;
		}
	// }

	munmap(audio_base, AUDIO_SPAN);
    close(fd);
    return 0;
}
