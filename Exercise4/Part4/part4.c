#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#define STOPWATCH_SIZE 9
#define KEY_SIZE 3
#define SW_SIZE 7

static int sw_fd, key_fd, stopwatch_fd;
static char SW_BUF[SW_SIZE];
static char KEY_BUF[KEY_SIZE];
static char STOPWATCH_BUF[STOPWATCH_SIZE];
static char USER_STOPWATCH[STOPWATCH_SIZE];
static int started_game = 0;
static int answer_count = 0;
static time_t start, end;
static double cpu_time_used;

unsigned int parse_character_hex (char c)
{
        if ('0' <= c && '9'>=c) {return((int)(c - '0'));}
        if ('a' <= c && 'f'>=c) {return((int)(c - 'a' + 10));}
        if ('A' <= c && 'F'>=c) {return((int)(c - 'A' + 10));}
        return 0;
}

void handle_int(int dummy)
{
	if (started_game)
	{
		time(&end);
		cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
		// For whatever reason, CLOCKS_PER_SEC isn't correctly defined on our ARM Linux, This is an empirical value 
		// to get the correct number of seconds.
		cpu_time_used *= 1000000;
		printf("You bailed, you managed to answer a total of %i questions in a total of %f seconds, averaging %f seconds per question.\n", answer_count, cpu_time_used, cpu_time_used/answer_count);
	}
	else
	{
		printf("You bailed.\n");
	}
	sprintf(STOPWATCH_BUF, "stop");
	write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
	sprintf(STOPWATCH_BUF, "00:00:00");
	write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
	close(sw_fd);
	close(key_fd);
	close(stopwatch_fd);
	exit(0);
}

int main()
{
	time_t t;

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
	write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
	sprintf(STOPWATCH_BUF, "59:59:99");
	write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
	printf("Let's play a game.\nFirst, set the stopwatch and press KEY0 when ready\n");
	
	while (1)
	{
		while(read(key_fd,KEY_BUF,2)!=0);
		if (KEY_BUF[0] == '1')
		{
			// Save the stopwatch value
			STOPWATCH_BUF[STOPWATCH_SIZE-1] = '\0';
			strcpy(USER_STOPWATCH, STOPWATCH_BUF);
			srand((unsigned) time(&t));
			break;
		}
		else if (KEY_BUF[0] != '0')
		{
			//Format of read from sw_fd is 0xNNN where N is each number from the switch, extract SW_BUF 2 to 4 to get the three numbers and
			//multiply with two and one hex offset (0x100 and 0x10 respectively). Read 6 characters (including EOS char)

			while(read(sw_fd,SW_BUF,6)!=0);
			int key_value = parse_character_hex(SW_BUF[2])*0x100 + parse_character_hex(SW_BUF[3])*0x10 + parse_character_hex(SW_BUF[4]);
			while(read(stopwatch_fd,STOPWATCH_BUF,STOPWATCH_SIZE)!=0);
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
			write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
		}
		usleep(1000);
	}

	int cont = 1;
	int correct;
	// Maximum value of random numbers, increments with iterations
	int max_value = 10;
	time(&start);
	started_game = 1;
	while(cont)
	{
		// Build a problem
		int first;
		int second;
		int op;
		int answer;
		int entered_answer = 0;
		first = rand() % (max_value - 1);
		second = rand() % (max_value++ - 1);
		op = rand() % 3;		// 0 for +, 1 for -, 2 for *, 3 for /
		answer = op == 0? first + second : op == 1? first - second : op == 2? first * second : first / second;
		printf("%i %c %i = ", first, op == 0? '+' : op == 1? '-' : op == 2? '*' : '/', second);
		// Start the counter
		write(stopwatch_fd, USER_STOPWATCH, STOPWATCH_SIZE);
		sprintf(STOPWATCH_BUF, "run");
		write(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE);
		correct = 0;
		// Check for answers
		while(cont && !correct)
		{
			scanf("%i", &entered_answer);
			
			while(read(stopwatch_fd, STOPWATCH_BUF, STOPWATCH_SIZE) != 0);
			STOPWATCH_BUF[STOPWATCH_SIZE-1] = '\0';
			cont = strcmp(STOPWATCH_BUF, "00:00:00");
			
			if (entered_answer == answer)
			{
				if (cont)
				{
					correct = 1;
					answer_count++;
				}
				else
				{
					printf("Too late.\n");
				}
			}
			else
			{
				if (cont)
				{
					correct = 0;
					printf("Wrong answer, try again: ");
					fflush(stdout);
				}
				else
				{
					printf("Wrong answer.\n");
				}
			}
		}
	}
	// If we're out here it's game over
	time(&end);
	cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	// For whatever reason, CLOCKS_PER_SEC isn't correctly defined on our ARM Linux, This is an empirical value 
	// to get the correct number of seconds.
	cpu_time_used *= 1000000;
	printf("You answered a total of %i questions in a total of %f seconds, averaging %f seconds per question.\n", answer_count, cpu_time_used, cpu_time_used/answer_count);

	close(sw_fd);
	close(key_fd);
	close(stopwatch_fd);
	return(0);
}
