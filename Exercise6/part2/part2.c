#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#define video_BYTES 64 // number of characters to read from /dev/video

int screen_x, screen_y;

int main(int argc, char *argv[]){
	int video_FD; // file descriptor
	char buffer[video_BYTES]; // buffer for data read from /dev/video
	
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
	sprintf(buffer, "line %i,%i %i,%i %i", 10, 10, 10, 100, 500);
	write(video_FD, buffer, video_BYTES);
	sprintf(buffer, "line %i,%i %i,%i %i", 10, 10, 100, 10, 1000);
	write(video_FD, buffer, video_BYTES);
	sprintf(buffer, "line %i,%i %i,%i %i", 10, 100, 100, 100, 1500);
	write(video_FD, buffer, video_BYTES);
	sprintf(buffer, "line %i,%i %i,%i %i", 100, 10, 100, 100, 2000);
	write(video_FD, buffer, video_BYTES);
	sprintf(buffer, "line %i,%i %i,%i %i", 100, 100, 10, 10, 2500);
	write(video_FD, buffer, video_BYTES);
	sprintf(buffer, "line %i,%i %i,%i %i", 100, 10, 10, 100, 3000);
	write(video_FD, buffer, video_BYTES);
	close (video_FD);
	return 0;
}
