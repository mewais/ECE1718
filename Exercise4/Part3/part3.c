#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#define STOPWATCH_SIZE 8
#define KEY_SIZE 3
#define SW_SIZE 7

static int start_stop = 0;
static int sw_fd, key_fd, stopwatch_fd;
static char SW_BUF [SW_SIZE];
static char KEY_BUF[KEY_SIZE];
static char STOPWATCH_BUF[STOPWATCH_SIZE];

unsigned int parse_character_hex (char c)
{
        if ('0' <= c && '9'>=c) {return((int)(c - '0'));}
        if ('a' <= c && 'f'>=c) {return((int)(c - 'a' + 10));}
        if ('A' <= c && 'F'>=c) {return((int)(c - 'A' + 10));}
        return 0;
}

void handle_int(int dummy)
{
	sprintf(STOPWATCH_BUF, "stop");
	write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE+1);
	sprintf(STOPWATCH_BUF, "00:00:00");
	write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE+1);
	close(sw_fd);
	close(key_fd);
	close(stopwatch_fd);
	exit(0);
}

int main()
{
	sw_fd = open("/dev/SW",O_RDONLY);
	key_fd = open("/dev/KEY",O_RDONLY);
	stopwatch_fd = open("/dev/stopwatch",O_RDWR);
	if (sw_fd == -1 || key_fd == -1 || stopwatch_fd == -1)
	{
		printf ("Error opening device files\n");
		return(-1);
	}
	signal(SIGINT, handle_int);
	sprintf(STOPWATCH_BUF, "stop");
	write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE+1);
	sprintf(STOPWATCH_BUF, "59:59:99");
	write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE+1);
	printf("Set the stopwatch and press KEY0 when ready\n");
	while (1)
	{
		while(read(key_fd,KEY_BUF,2)!=0);
		if (KEY_BUF[0] == '1')
		{
			if (start_stop == 0)
			{
				// write start to the stopwatch
				sprintf(STOPWATCH_BUF, "run");
			}
			else
			{
				// write stop to the stopwatch
				sprintf(STOPWATCH_BUF, "stop");
			}
			write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE+1);
			// change the start_stop value for next time
			start_stop ^= 1;
		}
		else if (KEY_BUF[0] != '0')
		{
			//Format of read from sw_fd is 0xNNN where N is each number from the switch, extract SW_BUF 2 to 4 to get the three numbers and
			//multiply with two and one hex offset (0x100 and 0x10 respectively). Read 6 characters (including EOS char)

			while(read(sw_fd,SW_BUF,6)!=0);
			int key_value = parse_character_hex(SW_BUF[2])*0x100 + parse_character_hex(SW_BUF[3])*0x10 + parse_character_hex(SW_BUF[4]);
			while(read(stopwatch_fd,STOPWATCH_BUF,STOPWATCH_SIZE+1)!=0);
			if (KEY_BUF[0] == '2' && key_value <= 99)
			{
				STOPWATCH_BUF[6] = key_value/10 + '0';
				STOPWATCH_BUF[7] = key_value%10 + '0';
			}
			else if (KEY_BUF[0] == '4' && key_value <= 59)
			{
				STOPWATCH_BUF[3] = key_value/10 + '0';
				STOPWATCH_BUF[4] = key_value%10 + '0';
			}
			else if (KEY_BUF[0] == '8' && key_value <= 59)
			{
				STOPWATCH_BUF[0] = key_value/10 + '0';
				STOPWATCH_BUF[1] = key_value%10 + '0';
			}
			write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE+1);
		}
		usleep(1000);
	}
	sprintf(STOPWATCH_BUF, "stop");
	write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
	sprintf(STOPWATCH_BUF, "00:00:00");
	write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
	close(sw_fd);
	close(key_fd);
	close(stopwatch_fd);
	return(0);
}
