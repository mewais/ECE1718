#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#define BOX_DIM 5
#define video_BYTES 64 // number of characters to read from /dev/video
#define STOPWATCH_SIZE 9
#define KEY_SIZE 3

int screen_x, screen_y;
static char buffer[video_BYTES]; // buffer for data read from /dev/video
static char KEY_BUF[KEY_SIZE];
static char STOPWATCH_BUF[STOPWATCH_SIZE];
static int video_FD, key_FD, stopwatch_FD; // file descriptor

void handle_int(int dummy)
{
	sprintf(buffer, "clear");
	write(video_FD, buffer, 6);
	sprintf(STOPWATCH_BUF, "stop");
	write(stopwatch_FD, STOPWATCH_BUF, STOPWATCH_SIZE);
	sprintf(STOPWATCH_BUF, "disp");
	write(stopwatch_FD, STOPWATCH_BUF, STOPWATCH_SIZE);
	sprintf(STOPWATCH_BUF, "00:00:00");
	write(stopwatch_FD, STOPWATCH_BUF, STOPWATCH_SIZE);
	close (video_FD);
	close (key_FD);
	close (stopwatch_FD);
	exit(0);
}

int main(int argc, char *argv[]){
	int x_pos[64];
	int y_pos[64];
	int old_x_pos[64];
	int old_y_pos[64];
	int x_dir[64];
	int y_dir[64];
	int i = 0;
	int count = 8;
	int speed = 0;
	int cont = 0;
	srand(time(0));

	// Open the character device driver
	if ((video_FD = open("/dev/video", O_RDWR)) == -1)
	{
		printf("Error opening /dev/video: %s\n", strerror(errno));
		return -1;
	}
	if ((key_FD = open("/dev/KEY", O_RDONLY)) == -1)
	{
		printf("Error opening /dev/KEY: %s\n", strerror(errno));
		return -1;
	}
	if ((stopwatch_FD = open("/dev/stopwatch", O_RDWR)) == -1)
	{
		printf("Error opening /dev/stopwatch: %s\n", strerror(errno));
		return -1;
	}
	signal(SIGINT, handle_int);

	// Set screen_x and screen_y by reading from the driver
	while(read(video_FD, buffer, 7)!=0);
	screen_y = ((buffer[0] - '0') * 100) + ((buffer[1] - '0') * 10) + buffer[2] - '0';
	screen_x = ((buffer[4] - '0') * 100) + ((buffer[5] - '0') * 10) + buffer[6] - '0';

	sprintf(buffer, "clear");
	write(video_FD, buffer, 6);
	sprintf(STOPWATCH_BUF, "stop");
	write(stopwatch_FD, STOPWATCH_BUF, STOPWATCH_SIZE);
	sprintf(STOPWATCH_BUF, "nodisp");
	write(stopwatch_FD, STOPWATCH_BUF, STOPWATCH_SIZE);
	sprintf(STOPWATCH_BUF, "00:00:00");
	write(stopwatch_FD, STOPWATCH_BUF, STOPWATCH_SIZE);

	for (i = 0; i < count; i++)
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
		// Erase old
		for (i = 0; i < count; i++)
		{
			sprintf(buffer, "box %i,%i %i,%i %i", old_x_pos[i], old_y_pos[i], old_x_pos[i] + BOX_DIM, old_y_pos[i] + BOX_DIM, 0);
			write(video_FD, buffer, video_BYTES);
			if (i > 0)
			{
				sprintf(buffer, "line %i,%i %i,%i %i", old_x_pos[i-1] + BOX_DIM/2, old_y_pos[i-1] + BOX_DIM/2, old_x_pos[i] + BOX_DIM/2, old_y_pos[i] + BOX_DIM/2, 0);
				write(video_FD, buffer, video_BYTES);
				if (i == count-1)
				{
					sprintf(buffer, "line %i,%i %i,%i %i", old_x_pos[0] + BOX_DIM/2, old_y_pos[0] + BOX_DIM/2, old_x_pos[i] + BOX_DIM/2, old_y_pos[i] + BOX_DIM/2, 0);
					write(video_FD, buffer, video_BYTES);
				}
			}
		}
		// Get Keys
		while(read(key_FD,KEY_BUF,2)!=0);
		if (KEY_BUF[0] == '1')
		{
			if (speed < 6)
				speed++;
		}
		else if (KEY_BUF[0] == '2')
		{
			if (speed > -18)
				speed--;
		}
		else if (KEY_BUF[0] == '4')
		{
			if (count < 64)
			{
				x_pos[count] = rand() % (screen_x - BOX_DIM);
				old_x_pos[count] = x_pos[count];
				y_pos[count] = rand() % (screen_y - BOX_DIM);
				old_y_pos[count] = y_pos[count];
				x_dir[count] = rand() % 2? 1 : -1;
				y_dir[count] = rand() % 2? 1 : -1;
				count++;
			}
		}
		else if (KEY_BUF[0] == '8')
		{
			if (count > 3)
			{
				count--;
			}
		}
		// Redraw Same if low speed
		if (speed < 0)
		{
			int base = (int) pow(2.0, -speed);
			sprintf(STOPWATCH_BUF, "%02d:%02d:%02d", (base / 100) / 60, (base / 100) % 60, base % 100);
			// printf("Test: %i, %s\n", speed, STOPWATCH_BUF);
			write(stopwatch_FD, STOPWATCH_BUF, STOPWATCH_SIZE);
			sprintf(STOPWATCH_BUF, "run");
			write(stopwatch_FD, STOPWATCH_BUF, STOPWATCH_SIZE);
			cont = 1;
			while (cont)
			{
				// Draw New
				for (i = 0; i < count; i++)
				{
					sprintf(buffer, "box %i,%i %i,%i %i", x_pos[i], y_pos[i], x_pos[i] + BOX_DIM, y_pos[i] + BOX_DIM, 20000);
					write(video_FD, buffer, video_BYTES);
					if (i > 0)
					{
						sprintf(buffer, "line %i,%i %i,%i %i", x_pos[i-1] + BOX_DIM/2, y_pos[i-1] + BOX_DIM/2, x_pos[i] + BOX_DIM/2, y_pos[i] + BOX_DIM/2, 63000);
						write(video_FD, buffer, video_BYTES);
						if (i == count-1)
						{
							sprintf(buffer, "line %i,%i %i,%i %i", x_pos[0] + BOX_DIM/2, y_pos[0] + BOX_DIM/2, x_pos[i] + BOX_DIM/2, y_pos[i] + BOX_DIM/2, 63000);
							write(video_FD, buffer, video_BYTES);
						}
					}
				}
				sprintf(buffer, "sync");
				write(video_FD, buffer, video_BYTES);
				while(read(stopwatch_FD, STOPWATCH_BUF, STOPWATCH_SIZE) != 0);
				STOPWATCH_BUF[STOPWATCH_SIZE-1] = '\0';
				cont = strcmp(STOPWATCH_BUF, "00:00:00");
			}
		}
		else
		{
			// Draw New at normal speed
			for (i = 0; i < count; i++)
			{
				sprintf(buffer, "box %i,%i %i,%i %i", x_pos[i], y_pos[i], x_pos[i] + BOX_DIM, y_pos[i] + BOX_DIM, 20000);
				write(video_FD, buffer, video_BYTES);
				if (i > 0)
				{
					sprintf(buffer, "line %i,%i %i,%i %i", x_pos[i-1] + BOX_DIM/2, y_pos[i-1] + BOX_DIM/2, x_pos[i] + BOX_DIM/2, y_pos[i] + BOX_DIM/2, 63000);
					write(video_FD, buffer, video_BYTES);
					if (i == count-1)
					{
						sprintf(buffer, "line %i,%i %i,%i %i", x_pos[0] + BOX_DIM/2, y_pos[0] + BOX_DIM/2, x_pos[i] + BOX_DIM/2, y_pos[i] + BOX_DIM/2, 63000);
						write(video_FD, buffer, video_BYTES);
					}
				}
			}
			sprintf(buffer, "sync");
			write(video_FD, buffer, video_BYTES);
		}
		// Update Locations
		for (i = 0; i < count; i++)
		{
			old_x_pos[i] = x_pos[i];
			old_y_pos[i] = y_pos[i];
			x_pos[i] += x_dir[i] * 2 * (speed <= 0? 1 : speed);
			y_pos[i] += y_dir[i] * 2 * (speed <= 0? 1 : speed);
			if (x_pos[i] + BOX_DIM >= screen_x - 1)
			{
				int adj = x_pos[i] + BOX_DIM - screen_x + 1;
				x_pos[i] = screen_x - 1 - adj - BOX_DIM;
				x_dir[i] = -x_dir[i];
			}
			if (x_pos[i] <= 0)
			{
				int adj = x_pos[i];
				x_pos[i] = adj;
				x_dir[i] = -x_dir[i];
			}
			if (y_pos[i] + BOX_DIM >= screen_y - 1)
			{
				int adj = y_pos[i] + BOX_DIM - screen_y + 1;
				y_pos[i] = screen_y - 1 - adj - BOX_DIM;
				y_dir[i] = -y_dir[i];
			}
			if (y_pos[i] <= 0)
			{
				int adj = y_pos[i];
				y_pos[i] = adj;
				y_dir[i] = -y_dir[i];
			}
		}
	}
	
	close (key_FD);
	close (video_FD);
	close (stopwatch_FD);
	return 0;
}
