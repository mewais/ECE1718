#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#define BOX_DIM 5
#define video_BYTES 64 // number of characters to read from /dev/video

int screen_x, screen_y;
static char buffer[video_BYTES]; // buffer for data read from /dev/video
static int video_FD; // file descriptor

void handle_int(int dummy)
{
	sprintf(buffer, "clear");
	write(video_FD, buffer, 6);
	close (video_FD);
	exit(0);
}

int main(int argc, char *argv[]){
	int x_pos[8];
	int y_pos[8];
	int old_x_pos[8];
	int old_y_pos[8];
	int x_dir[8];
	int y_dir[8];
	int i = 0;
	srand(time(0));

	// Open the character device driver
	if ((video_FD = open("/dev/video", O_RDWR)) == -1)
	{
		printf("Error opening /dev/video: %s\n", strerror(errno));
		return -1;
	}
	signal(SIGINT, handle_int);

	// Set screen_x and screen_y by reading from the driver
	while(read(video_FD, buffer, 7)!=0);
	screen_y = ((buffer[0] - '0') * 100) + ((buffer[1] - '0') * 10) + buffer[2] - '0';
	screen_x = ((buffer[4] - '0') * 100) + ((buffer[5] - '0') * 10) + buffer[6] - '0';

	sprintf(buffer, "clear");
	write(video_FD, buffer, 6);

	for (i = 0; i < 8; i++)
	{
		x_pos[i] = rand() % (screen_x - BOX_DIM);
		old_x_pos[i] = x_pos[i];
		y_pos[i] = rand() % (screen_y - BOX_DIM);
		old_y_pos[i] = y_pos[i];
		x_dir[i] = rand() % 2? 1 : -1;
		y_dir[i] = rand() % 2? 1 : -1;
	}

	// Use pixel commands to color some pixels on the screen
	while(1)
	{
		for (i = 0; i < 8; i++)
		{
			sprintf(buffer, "box %i,%i %i,%i %i", old_x_pos[i], old_y_pos[i], old_x_pos[i] + BOX_DIM, old_y_pos[i] + BOX_DIM, 0);
			write(video_FD, buffer, video_BYTES);
			if (i > 0)
			{
				sprintf(buffer, "line %i,%i %i,%i %i", old_x_pos[i-1] + BOX_DIM/2, old_y_pos[i-1] + BOX_DIM/2, old_x_pos[i] + BOX_DIM/2, old_y_pos[i] + BOX_DIM/2, 0);
				write(video_FD, buffer, video_BYTES);
				if (i == 7)
				{
					sprintf(buffer, "line %i,%i %i,%i %i", old_x_pos[0] + BOX_DIM/2, old_y_pos[0] + BOX_DIM/2, old_x_pos[i] + BOX_DIM/2, old_y_pos[i] + BOX_DIM/2, 0);
					write(video_FD, buffer, video_BYTES);
				}
			}
			sprintf(buffer, "box %i,%i %i,%i %i", x_pos[i], y_pos[i], x_pos[i] + BOX_DIM, y_pos[i] + BOX_DIM, 20000);
			write(video_FD, buffer, video_BYTES);
			if (i > 0)
			{
				sprintf(buffer, "line %i,%i %i,%i %i", x_pos[i-1] + BOX_DIM/2, y_pos[i-1] + BOX_DIM/2, x_pos[i] + BOX_DIM/2, y_pos[i] + BOX_DIM/2, 63000);
				write(video_FD, buffer, video_BYTES);
				if (i == 7)
				{
					sprintf(buffer, "line %i,%i %i,%i %i", x_pos[0] + BOX_DIM/2, y_pos[0] + BOX_DIM/2, x_pos[i] + BOX_DIM/2, y_pos[i] + BOX_DIM/2, 63000);
					write(video_FD, buffer, video_BYTES);
				}
			}
		}
		sprintf(buffer, "sync");
		write(video_FD, buffer, video_BYTES);
		for (i = 0; i < 8; i++)
		{
			old_x_pos[i] = x_pos[i];
			old_y_pos[i] = y_pos[i];
			x_pos[i] += x_dir[i];
			y_pos[i] += y_dir[i];
			if (x_pos[i] + BOX_DIM == screen_x - 1 || x_pos[i] == 0)
				x_dir[i] = -x_dir[i];
			if (y_pos[i] + BOX_DIM == screen_y - 1 || y_pos[i] == 0)
				y_dir[i] = -y_dir[i];
		}
	}
	
	close (video_FD);
	return 0;
}
