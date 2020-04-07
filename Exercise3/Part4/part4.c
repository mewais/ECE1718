#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#define two_hex_offset 0x100
#define one_hex_offset 0x10
#define US_TO_MS 1000
#define BUFFER_SIZE 128
#define KEY_SIZE 3
#define SW_SIZE 7

unsigned int parse_character_hex (char c)
{
        if ('0' <= c && '9'>=c) {return((int)(c - '0'));}
        if ('a' <= c && 'f'>=c) {return((int)(c - 'a' + 10));}
        if ('A' <= c && 'F'>=c) {return((int)(c - 'A' + 10));}
        return 0;
}

int main()
{
	int sw_fd, hex_fd, ledr_fd, key_fd, value;
	char SW_BUF [SW_SIZE];
	int total = 0;
	char HEX_BUF [BUFFER_SIZE];
	char LEDR_BUF[BUFFER_SIZE];
	char KEY_BUF[KEY_SIZE];
	sw_fd = open("/dev/SW",O_RDWR);
	hex_fd = open("/dev/HEX",O_RDWR);
	ledr_fd = open("/dev/LEDR",O_RDWR);
	key_fd = open("/dev/KEY",O_RDWR);
	if (sw_fd == -1 || hex_fd == -1 || ledr_fd == -1 || key_fd == -1)
	{
		printf ("Error opening device files\n");
		return(-1);
	}
	sprintf(LEDR_BUF,"%x",0);
	sprintf(HEX_BUF,"%d",0);
	write(ledr_fd,LEDR_BUF,strlen(LEDR_BUF)+1);
	write(hex_fd,HEX_BUF,strlen(HEX_BUF)+1);
	while (1)
	{
		while(read(key_fd,KEY_BUF,2)!=0);
		if (KEY_BUF[0] != '0')
		{
			//Format of read from sw_fd is 0xNNN where N is each number from the switch, extract SW_BUF 2 to 4 to get the three numbers and
			//multiply with two and one hex offset (0x100 and 0x10 respectively). Read 6 characters (including EOS char)

			while(read(sw_fd,SW_BUF,6)!=0);
			value = parse_character_hex(SW_BUF[2])*two_hex_offset + parse_character_hex(SW_BUF[3])*one_hex_offset + parse_character_hex(SW_BUF[4]);
			sprintf(LEDR_BUF,"%x",value);
			write(ledr_fd,LEDR_BUF,strlen(LEDR_BUF)+1);
			total += value;
			sprintf(HEX_BUF,"%d",total);
			write(hex_fd,HEX_BUF,strlen(HEX_BUF)+1);
		}
		usleep(US_TO_MS);
	}
	return(0);
}
