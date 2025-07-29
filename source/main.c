#include "cxa.h"

#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#define DESC          "4T   [-t <task>] [flags]"
#define WIDEST_FONT   7
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
#include "fonts/raw.inc"

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

enum time_type
{
	time_t_seconds,
	time_t_minutes,
	time_t_hours
};

static void intro (struct fourt*);
static void outro (struct fourt*);

static void start_clock (struct fourt*);
static void pick_font (struct font*, const char*);

static void draw_colons (struct font*, const unsigned int, const unsigned int);
static void display_pair (struct font*, const unsigned int, const enum time_type, const unsigned int, const unsigned int);

static void display_font_names (void);

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
	if (flags[6].meta & CXA_FLAG_SEEN_MASK)
	{
		display_font_names();
		return 0;
	}

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

	char quit;
	const unsigned int start_r = ((fourt->wrows - fourt->font.rows    ) >> 1);
	const unsigned int start_c = ((fourt->wcols - fourt->font.cols * 8) >> 1);

	draw_colons(&fourt->font, start_r, start_c);

	while ((seconds > 0) && ((quit = getchar()) != 'q'))
	{
		const unsigned int h = seconds / 3600;
		const unsigned int m = (seconds % 3600) / 60;
		const unsigned int s = (seconds % 60);

		display_pair(&fourt->font, h, time_t_hours  , start_r, start_c);
		display_pair(&fourt->font, m, time_t_minutes, start_r, start_c);
		display_pair(&fourt->font, s, time_t_seconds, start_r, start_c);

		sleep(1);
		seconds--;
	}
}

static void pick_font (struct font *font, const char *asked)
{
	/* Whatever is given to you via argv is null terminated
	 * so we do not need to compare at most N bytes
	 */
	if (!strcmp(asked, "bulbhead")) { *font = f_bulbhead; }
	else if (!strcmp(asked, "raw")) { *font = f_raw; }
	else { *font = f_bulbhead; }
}

static void draw_colons (struct font *font, const unsigned int start_r, const unsigned int start_c)
{
	unsigned int coffset = start_c + font->cols * 2;

	for (unsigned char i = 0; i < 2; i++)
	{
		for (unsigned short line = 0; line < font->rows; line++)
		{
			printf("\x1b[%d;%dH%s", start_r + line, coffset, font->set[10][line]);
		}
		coffset += font->cols * 3;
	}
}

static void display_pair (struct font *font, const unsigned int left, const enum time_type kind, const unsigned int start_r, const unsigned int start_c)
{
	const unsigned int d1 = left / 10;
	const unsigned int d2 = left % 10;

	unsigned int coffset = start_c;
	switch (kind)
	{
		case time_t_hours:   break;
		case time_t_minutes: coffset += font->cols * 3; break;
		case time_t_seconds: coffset += font->cols * 6; break;
	}

	for (register char i = 0; i < 2; i++)
	{
		for (unsigned short line = 0; line < font->rows; line++)
		{
			printf("\x1b[%d;%dH%s%s", start_r + line, coffset, font->set[d1][line], font->set[d2][line]);
			fflush(stdout);
		}
	}

	if (kind == time_t_seconds) { return; }
}

static void display_font_names (void)
{
	const unsigned int no = 11;
	static const char *const fonts[] =
	{
		"braced",
		"bulbhead (default)",
		"fraktur",
		"hollywood",
		"italic",
		"larry3d",
		"raw",
		"rectangles",
		"short",
		"small",
		"wavy"
	};

	printf("\n\t4T List of fonts available!\n");
	for (unsigned int i = 0; i < no; i++)
	{
		printf("\t - %s\n", fonts[i]);
	}
	printf("\tremember to use -f flag to use any of these (-f <font> or --flag=<font> or --flag <font>\n\n");
}
