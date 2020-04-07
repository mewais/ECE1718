#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define VIDEO_SIZE 64
#define ACCEL_SIZE 30
#define ALPHA 0.000025

int screen_x, screen_y;
static int video_FD, accel_FD; // file descriptor
static char VIDEO_BUF[VIDEO_SIZE];
static char ACCEL_BUF[ACCEL_SIZE];

float avg_x = 0.0;
float avg_y = 0.0;

void handle_int(int dummy)
{
	sprintf(VIDEO_BUF, "clear");
	write(video_FD, VIDEO_BUF, 6);
	sprintf(VIDEO_BUF, "erase");
	write(video_FD, VIDEO_BUF, 6);
	close(video_FD);
	close(accel_FD);
	exit(0);
}

void drawCircleLines(int xc, int yc, int x, int y)
{
	sprintf(VIDEO_BUF, "line %i,%i %i,%i %i", xc+y, yc+x, xc-y, yc+x, 33333);
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);
	sprintf(VIDEO_BUF, "line %i,%i %i,%i %i", xc+x, yc+y, xc-x, yc+y, 33333);
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);
	sprintf(VIDEO_BUF, "line %i,%i %i,%i %i", xc+x, yc-y, xc-x, yc-y, 33333);
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);
	sprintf(VIDEO_BUF, "line %i,%i %i,%i %i", xc+y, yc-x, xc-y, yc-x, 33333);
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);
}

void drawFilledCircle(int xc, int yc, int r)
{
	int x = 0, y = r;
    int d = 3 - 2 * r;
    drawCircleLines(xc, yc, x, y);
    while (y >= x)
    {
        x++;
        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
		{
            d = d + 4 * x + 6;
		}
        drawCircleLines(xc, yc, x, y);
    }
}



int parseNumbers(char* string, int* x, int* y, int* z, int* single_tap, int* double_tap)
{
	int index = 0;
	int scale = 0;
	int neg_x = 0;
	int neg_y = 0;
	int neg_z = 0;
	int parsed_x = 0;
	int parsed_y = 0;
	int parsed_z = 0;
	int activity = 0;
	
	*double_tap = string[index] == 'f';
	*single_tap = string[index] == 'd' || string[index] == 'f' || string[index] == '4' || string[index]=='c' || string[index]=='5';
	activity = (string[index] == '9' || string[index] == 'f' || string[index] == 'd');
	index += 2;

	if (!activity)
	{
		return 0;
	}
	while (string[index] == ' ' || string[index] == '\t')
	{
		index++;
	}
	while (string[index] != ' ' && string[index] != '\t')
	{
		if (string[index] == '-')
		{
			neg_x = 1;
			index++;
		}
		else
		{
			parsed_x *= 10;
			parsed_x += string[index++] - '0';
		}
	}
	while (string[index] == ' ' || string[index] == '\t')
	{
		index++;
	}
	while (string[index] != ' ' && string[index] != '\t')
	{
		if (string[index] == '-')
		{
			neg_y = 1;
			index++;
		}
		else
		{
			parsed_y *= 10;
			parsed_y += string[index++] - '0';
		}
	}
	while (string[index] == ' ' || string[index] == '\t')
	{
		index++;
	}
	while (string[index] != ' ' && string[index] != '\t')
	{
		if (string[index] == '-')
		{
			neg_z = 1;
			index++;
		}
		else
		{
			parsed_z *= 10;
			parsed_z += string[index++] - '0';
		}
	}
	while (string[index] == ' ' || string[index] == '\t')
	{
		index++;
	}
	while (string[index] != ' ' && string[index] != '\t' && string[index] != '\n' && string[index] != '\0')
	{
		scale *= 10;
		scale += string[index++] - '0';
	}
	*x = neg_x? -parsed_x * scale : parsed_x * scale;
	*y = neg_y? -parsed_y * scale : parsed_y * scale; 
	*z = neg_z? -parsed_z * scale : parsed_z * scale; 
	return 1;
}

int main(int argc, char *argv[])
{
	// Open the character device driver
	if ((video_FD = open("/dev/video", O_RDWR)) == -1)
	{
		printf("Error opening /dev/video: %s\n", strerror(errno));
		return -1;
	}
	if ((accel_FD = open("/dev/accel", O_RDONLY)) == -1)
	{
		printf("Error opening /dev/accel: %s\n", strerror(errno));
		return -1;
	}
	
	// Set screen_x and screen_y by reading from the drive
	while(read(video_FD, VIDEO_BUF, 7)!=0);
	screen_y = ((VIDEO_BUF[0] - '0') * 100) + ((VIDEO_BUF[1] - '0') * 10) + VIDEO_BUF[2] - '0';
	screen_x = ((VIDEO_BUF[4] - '0') * 100) + ((VIDEO_BUF[5] - '0') * 10) + VIDEO_BUF[6] - '0';

	sprintf(VIDEO_BUF, "clear");
	write(video_FD, VIDEO_BUF, 6);
	sprintf(VIDEO_BUF, "erase");
	write(video_FD, VIDEO_BUF, 6);
	drawFilledCircle(screen_x/2, screen_y/2, 10);
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);
	sprintf(VIDEO_BUF, "sync");
	write(video_FD, VIDEO_BUF, VIDEO_SIZE);

	int xc = 0;
	int yc = 0;
	int zc = 0;
	int st_timeout = 0;
	int dt_timeout = 0;
	while(1)
	{
		int st, dt;
		while(read(accel_FD, ACCEL_BUF, ACCEL_SIZE) != 0);
		ACCEL_BUF[64] = '\0';
		st_timeout = st_timeout == 0 ? 0 : st_timeout - 1;
		dt_timeout = dt_timeout == 0 ? 0 : dt_timeout - 1;
		if (parseNumbers(ACCEL_BUF, &xc, &yc, &zc, &st, &dt))
		{
			avg_x = avg_x * ALPHA + xc * (1 - ALPHA);
			avg_y = avg_y * ALPHA + yc * (1 - ALPHA);
			sprintf(VIDEO_BUF, "clear");
			write(video_FD, VIDEO_BUF, 6);
			sprintf(VIDEO_BUF, "erase");
			write(video_FD, VIDEO_BUF, 6);
			drawFilledCircle((screen_x/2) + avg_x/10, (screen_y/2) + avg_y/10, 10);
			write(video_FD, VIDEO_BUF, VIDEO_SIZE);
			sprintf(VIDEO_BUF, "text 0,0 %i, %i, %i%s %s", xc, yc, zc, st_timeout? ", ST" : "", dt_timeout? ", DT" : "");
			write(video_FD, VIDEO_BUF, VIDEO_SIZE);
			sprintf(VIDEO_BUF, "sync");
			write(video_FD, VIDEO_BUF, VIDEO_SIZE);
			if (st)
			{
				st_timeout = 12;
			}
			if (dt)
			{
				dt_timeout = 12;
			}
			usleep(80000);
		}
		else
		{
			avg_x = avg_x * ALPHA + xc * (1 - ALPHA);
			avg_y = avg_y * ALPHA + yc * (1 - ALPHA);
			sprintf(VIDEO_BUF, "clear");
			write(video_FD, VIDEO_BUF, 6);
			sprintf(VIDEO_BUF, "erase");
			write(video_FD, VIDEO_BUF, 6);
			drawFilledCircle((screen_x/2) + avg_x/10, (screen_y/2) + avg_y/10, 10);
			write(video_FD, VIDEO_BUF, VIDEO_SIZE);
			sprintf(VIDEO_BUF, "text 0,0 %i, %i, %i%s %s", xc, yc, zc, st_timeout? ", ST" : "", dt_timeout? ", DT" : "");
			write(video_FD, VIDEO_BUF, VIDEO_SIZE);
			sprintf(VIDEO_BUF, "sync");
			write(video_FD, VIDEO_BUF, VIDEO_SIZE);
			if (st)
			{
				st_timeout = 12;
			}
			if (dt)
			{
				dt_timeout = 12;
			}
			usleep(80000);
		}
	}

	close(video_FD);
	close(accel_FD);
	return 0;
}
