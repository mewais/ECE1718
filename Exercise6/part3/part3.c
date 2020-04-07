#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
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
	int y = 0, last_y = 0;
	int dir = 0;
	
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

	// Use pixel commands to color some pixels on the screen
	while(1)
	{
		sprintf(buffer, "line %i,%i %i,%i %i", 100, last_y, 200, last_y, 0);
		write(video_FD, buffer, video_BYTES);
		sprintf(buffer, "line %i,%i %i,%i %i", 100, y, 200, y, 2000);
		write(video_FD, buffer, video_BYTES);
		sprintf(buffer, "sync");
		write(video_FD, buffer, video_BYTES);
		last_y = y;
		if (dir == 0)
		{
			y++;
			if (y == screen_y-1)
			{
				dir = 1;
			}
		}
		else
		{
			y--;
			if (y == 0)
			{
				dir = 0;
			}
		}
	}
	
	close (video_FD);
	return 0;
}
