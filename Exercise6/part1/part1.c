#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#define video_BYTES 11 // number of characters to read from /dev/video

int screen_x, screen_y;

int main(int argc, char *argv[]){
	int video_FD; // file descriptor
	char buffer[video_BYTES]; // buffer for data read from /dev/video
	int i, j;
	
	// Open the character device driver
	if ((video_FD = open("/dev/video", O_RDWR)) == -1)
	{
		printf("Error opening /dev/video: %s\n", strerror(errno));
		return -1;
	}

	// Set screen_x and screen_y by reading from the driver
	while(read(video_FD, buffer, 7)!=0);
	screen_y = ((buffer[0] - '0') * 100) + ((buffer[1] - '0') * 10) + buffer[2] - '0';
	screen_x = ((buffer[4] - '0') * 100) + ((buffer[5] - '0') * 10) + buffer[6] - '0';

	sprintf(buffer, "clear");
	write(video_FD, buffer, 6);

	// Use pixel commands to color some pixels on the screen
	for (i = 0; i < screen_x; i++)
	{
		for (j = 0; j < screen_y; j++)
		{
			sprintf(buffer, "pixel %i,%i %i", i, j, 777);
			write(video_FD, buffer, 17);
		}
	}
	close (video_FD);
	return 0;
}
