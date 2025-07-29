#include "cxa.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#define DESC        "4T   [-t <task>] [flags]"
#define FD_STDOUT   1
#define FD_STDIN    0

struct fourt
{
	struct termios defterm;
	unsigned int   wcols, wrows;

	struct
	{
		char *task, *font, *info;
		int time;
	} args;
};


static void intro (struct fourt*);
static void outro (struct fourt*);

int main (int argc, char **argv)
{
	struct fourt ft;
	memset(&ft, 0, sizeof(ft));

	ft.args.time = 30;
	struct CxaFlag flags[] =
	{
		CXA_SET_STR("task", "task you'll be working on",      &ft.args.task, CXA_FLAG_TAKER_YES, 't'),
		CXA_SET_STR("font", "font to display time left",      &ft.args.font, CXA_FLAG_TAKER_MAY, 'f'),
		CXA_SET_STR("info", "display info about a task",      &ft.args.task, CXA_FLAG_TAKER_MAY, 'i'),
		CXA_SET_STR("prev", "preview font (needs font name)", &ft.args.font, CXA_FLAG_TAKER_YES, 'p'),
		CXA_SET_INT("time", "time you'll work (30 default)",  &ft.args.time, CXA_FLAG_TAKER_MAY, 'T'),
		CXA_SET_CHR("help", "display this message :)",        NULL,          CXA_FLAG_TAKER_NON, 'h'),
		CXA_SET_END
	};

	cxa_clean(cxa_execute((unsigned char) argc, argv, flags, "4T"));
	if ((ft.args.task == NULL) || (flags[4].meta & CXA_FLAG_SEEN_MASK))
	{
		cxa_print_usage(DESC, flags);
	}

	intro(&ft);
	outro(&ft);
	return 0;
}

static void intro (struct fourt *ft)
{
	struct winsize w;
	ioctl(FD_STDOUT, TIOCGWINSZ, &w);
	
	ft->wrows = (unsigned int) w.ws_row;
	ft->wcols = (unsigned int) w.ws_col;

	printf("\x1b[2J\x1b[H\x1b[0\x1b[?25l");
	fflush(stdout);

	tcgetattr(FD_STDIN, &ft->defterm);
	struct termios conf = ft->defterm;

	conf.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(FD_STDIN, TCSANOW, &conf);

	const int startingmsglen = 16;
	const unsigned int drow = (ft->wrows >> 1), dcol = ((ft->wcols - startingmsglen) >> 1);

	printf("\x1b[%d;%dHstarting in  ...", drow, dcol);
	fflush(stdout);

	for (char i = 5; i > 0; i--)
	{
		printf("\x1b[%d;%dH%d", drow, dcol + 12, i);
		fflush(stdout);

		clock_t stop = clock() + CLOCKS_PER_SEC;
		while (clock() < stop) {}
	}

	printf("\x1b[2J\x1b");
	fflush(stdout);
}

static void outro (struct fourt *ft)
{
	tcsetattr(FD_STDIN, TCSANOW, &ft->defterm);
	printf("\x1b[?25h");
	fflush(stdout);
}
