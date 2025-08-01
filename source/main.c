/* 4T - terminal task time tracker
 * Aug 1 2025
 * Main file
 */
#include "cxa.h"

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

/* basic program information
 */
#define PROGRAM_NAME   "4T"
#define BASIC_USAGE    "4T [-t <task>] [flags]"

/* font related stuff
 * WIDEST_FONT: 'hollywood' font is 17 bytes long
 * ALPHA_SIZE:  number of characters within the font's charset (0-9:)
 */
#define WIDEST_FONT   17
#define ALPHA_SIZE    11

/* Argument defaults
 */
#define DEFAULT_FONT   "short"
#define DEFAULT_TIME   30

#define STDIN_FD    0
#define STDOUT_FD   1

struct font
{
	char *set[ALPHA_SIZE][WIDEST_FONT];
	unsigned short rows, cols;
};

#include "fonts/braced.inc"
#include "fonts/bulbhead.inc"
#include "fonts/fraktur.inc"
#include "fonts/hollywood.inc"
#include "fonts/larry3d.inc"
#include "fonts/raw.inc"
#include "fonts/rectangles.inc"
#include "fonts/short.inc"

struct xargs
{
	char *task;
	char *font;
	unsigned int time;
};

struct FT
{
	struct font  font;
	struct xargs args;
	char *quote;
	unsigned short wrows, wcols;
};

static void get_and_check_dimensions (struct FT*, const bool);

int main (int argc, char **argv)
{
	struct FT ft;
	memset(&ft, 0, sizeof(ft));

	ft.args.font = DEFAULT_FONT;
	ft.args.time = DEFAULT_TIME;

	struct CxaFlag flags[] =
	{
		CXA_SET_STR("task", "task you will be working on",             &ft.args.task, CXA_FLAG_TAKER_YES, 't'),
		CXA_SET_STR("font", "font to be displayed (short by default)", &ft.args.font, CXA_FLAG_TAKER_YES, 'f'),
		CXA_SET_END,
	};

	cxa_clean(cxa_execute((unsigned char) argc, argv, flags, PROGRAM_NAME));
	if ((flags[0].meta & CXA_FLAG_SEEN_MASK) == CXA_FLAG_WASNT_SEEN || (*ft.args.task == 0)) { cxa_print_usage(BASIC_USAGE, flags); }

	get_and_check_dimensions(&ft, false);
	return 0;
}

static void get_and_check_dimensions (struct FT *ft, const bool wasignal)
{
	struct winsize w;
    ioctl(STDOUT_FD, TIOCGWINSZ, &w);

	ft->wrows = w.ws_row;
	ft->wcols = w.ws_col;

	printf("%d rows\n%d cols\n", ft->wrows, ft->wcols);
}
