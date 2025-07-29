#include "cxa.h"

#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#define DESC          "4T   [-t <task>] [flags]"
#define WIDEST_FONT   8
#define SET_SIZE      11

#define FD_STDOUT     1
#define FD_STDIN      0

#define DEFAULT_T     30
#define DEFAULT_f     "bulbhead"

struct font
{
	char *set[SET_SIZE][WIDEST_FONT];
	unsigned char rows, cols;
};

#include "fonts/bulbhead.inc"

struct fourt
{
	struct termios defterm;
	struct font    font;
	unsigned int   wcols, wrows;
	int            flags;

	struct
	{
		char *task, *font, *info;
		int time;
	} args;
};

static void intro (struct fourt*);
static void outro (struct fourt*);

static void start_clock (struct fourt*);
static void pick_font (struct font*, const char*);

int main (int argc, char **argv)
{
	struct fourt ft;
	memset(&ft, 0, sizeof(ft));

	ft.args.time = DEFAULT_T;
	ft.args.font = DEFAULT_f;

	struct CxaFlag flags[] =
	{
		CXA_SET_STR("task", "task you'll be working on",      &ft.args.task, CXA_FLAG_TAKER_YES, 't'),
		CXA_SET_STR("font", "font to display time left",      &ft.args.font, CXA_FLAG_TAKER_MAY, 'f'),
		CXA_SET_STR("info", "display info about a task",      &ft.args.task, CXA_FLAG_TAKER_MAY, 'i'),
		CXA_SET_STR("prev", "preview font (needs font name)", &ft.args.font, CXA_FLAG_TAKER_YES, 'p'),
		CXA_SET_INT("time", "time you'll work (30 default)",  &ft.args.time, CXA_FLAG_TAKER_MAY, 'T'),
		CXA_SET_CHR("help", "display this message :)",        NULL,          CXA_FLAG_TAKER_NON, 'h'),
		CXA_SET_CHR("list", "list font names",                NULL,          CXA_FLAG_TAKER_NON, 'L'),
		CXA_SET_END
	};

	cxa_clean(cxa_execute((unsigned char) argc, argv, flags, "4T"));
	if ((ft.args.task == NULL) || (flags[4].meta & CXA_FLAG_SEEN_MASK))
	{
		cxa_print_usage(DESC, flags);
	}

	intro(&ft);
	start_clock(&ft);
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

	ft->flags = fcntl(FD_STDIN, F_GETFL);
	fcntl(FD_STDIN, F_SETFL, ft->flags | O_NONBLOCK);

	const int startingmsglen = 16;
	const unsigned int drow = (ft->wrows >> 1), dcol = ((ft->wcols - startingmsglen) >> 1);

	printf("\x1b[%d;%dHstarting in  ...", drow, dcol);
	fflush(stdout);

	for (char i = 3; i > 0; i--)
	{
		printf("\x1b[%d;%dH%d", drow, dcol + 12, i);
		fflush(stdout);
		sleep(1);
	}

	printf("\x1b[2J\x1b");
	fflush(stdout);
}

static void outro (struct fourt *ft)
{
	fcntl(FD_STDIN, F_SETFL, ft->flags);
	tcsetattr(FD_STDIN, TCSANOW, &ft->defterm);
	printf("\x1b[?25h");
	fflush(stdout);
}

static void start_clock (struct fourt *fourt)
{
	if (fourt->args.time <= 0) { fourt->args.time = DEFAULT_T; }

	unsigned int seconds = (unsigned int) fourt->args.time * 60;
	pick_font(&fourt->font, fourt->args.font);

	char quit = getchar();
	while ((seconds > 0) && ((quit = getchar()) != 'q'))
	{
		const unsigned int h = seconds / 3600;
		const unsigned int m = (seconds % 3600) / 60;
		const unsigned int s = (seconds % 60);

		    printf("\r%02d:%02d:%02d", h, m, s);
			fflush(stdout);

		sleep(1);
		seconds--;
	}
}

static void pick_font (struct font *font, const char *asked)
{
	/* Whatever is given to you via argv is null terminated
	 * so we do not need to compare at most N bytes
	 */
	if (strcmp(asked, "bulbhead")) { *font = f_bulbhead; }
	else { *font = f_bulbhead; }
}
