/* 4T - terminal task time tracker
 * Aug 1 2025
 * Main file
 */
#if defined(_WIN32) || defined(_WIN64)
	#error "4T is not available for Windows"
#endif

#include "cxa.h"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/ioctl.h>

#define PROGRAM_NAME                 "4T"
#define PROGRAM_USAGE                "4T [-t <taks>] [flags]"

#define FLAG_TASK_DESC               "task you will be working on (mandatory)"
#define FLAG_FONT_DESC               "font to display time remaining/passed by (short default)"
#define FLAG_TIME_DESC               "total amount of minutes working (30 default)"
#define FLAG_LIST_DESC               "lists all availbale fonts"
#define FLAG_PREV_DESC               "do a preview of <fontname> font"

#define FLAG_FONT_DEFAULT            "short"
#define FLAG_TIME_DEFAULT            30

#define FONT_WIDEST_SYMBOL           17
#define NO_FONTS                     8
#define FONT_CHARSET_SIZE            11
#define FONT_NO_DISPLAYED_CHARS      8

#define STDIN_FD                     0
#define STDOUT_FD                    1

struct font
{
	char *set[FONT_CHARSET_SIZE][FONT_WIDEST_SYMBOL];
	unsigned short rows, cols;
};

#include "const.inc"

struct xargs_
{
	char *task;
	char *font;
	unsigned short time;
};

struct ft
{
	struct xargs_ args;
	struct font font;
	const char *quote;
	unsigned short winrows, wincols;
};

static inline void obtain_window_dimensions (struct ft *ft)
{
	struct winsize w;
	assert(ioctl(STDOUT_FD, TIOCGWINSZ, &w) == 0);

	ft->winrows = (unsigned short) w.ws_row;
	ft->wincols = (unsigned short) w.ws_col;
}

static void set_defaults_and_quote (struct ft*);
static void set_rendering_font (struct font*, const char*);

static void list_available_fonts (void);
static void make_sure_content_fits_in (struct ft*, const unsigned short, const unsigned short, const unsigned short, const bool);

static void do_font_preview (struct ft*);

int main (int argc, char **argv)
{
	struct ft ft;
	set_defaults_and_quote(&ft);

	struct CxaFlag flags[] =
	{
		CXA_SET_STR("task", FLAG_TASK_DESC, &ft.args.task, CXA_FLAG_TAKER_YES, 't'),
		CXA_SET_STR("font", FLAG_FONT_DESC, &ft.args.font, CXA_FLAG_TAKER_YES, 'f'),
		CXA_SET_SHT("time", FLAG_TIME_DESC, &ft.args.time, CXA_FLAG_TAKER_YES, 'T'),
		CXA_SET_CHR("list", FLAG_LIST_DESC, NULL,          CXA_FLAG_TAKER_NON, 'L'),
		CXA_SET_STR("prev", FLAG_PREV_DESC, &ft.args.font, CXA_FLAG_TAKER_MAY, 'p'),
		CXA_SET_END
	};

	cxa_clean(cxa_execute((unsigned char) argc, argv, flags, PROGRAM_NAME));

	if (flags[3].meta & CXA_FLAG_SEEN_MASK)
	{
		list_available_fonts();
		return 0;
	}

	if (flags[4].meta & CXA_FLAG_SEEN_MASK)
	{
		do_font_preview(&ft);
		return 0;
	}

	if (!ft.args.task || *ft.args.task == 0)
	{
		cxa_print_usage(PROGRAM_USAGE, flags);
		return 0;
	}

	set_rendering_font(&ft.font, ft.args.font);
	return 0;
}

static void set_defaults_and_quote (struct ft *ft)
{
	memset(ft, 0, sizeof(*ft));
	ft->args.font = FLAG_FONT_DEFAULT;
	ft->args.time = FLAG_TIME_DEFAULT;

	srand(time(NULL));
	ft->quote = Quotes[rand() % NO_QUOTES];
}

static void set_rendering_font (struct font *font, const char *given)
{
	/* since 'given' is a string given via argv, it is assumed to be
	 * null-byte terminated, therefore we do not need to worry about
	 * variable lengths
	 */
	     if (!strcmp(given, "bulbhead"))   { *font = f_bulbhead;   }
	else if (!strcmp(given, "braced"))     { *font = f_braced;     }
	else if (!strcmp(given, "fraktur"))    { *font = f_fraktur;    }
	else if (!strcmp(given, "hollywood"))  { *font = f_hollywood;  }
	else if (!strcmp(given, "larry3d"))    { *font = f_larry3d;    }
	else if (!strcmp(given, "raw"))        { *font = f_raw;        }
	else if (!strcmp(given, "rectangles")) { *font = f_rectangles; }
	else if (!strcmp(given, "short"))      { *font = f_short;      }
	else
	{
		static const char *const errmsg =
		"%s: error: '%s' is not a defined font\n"
		" make sure it exists by checking all available fonts\n"
		" $ %s --list\n";
		fprintf(stderr, errmsg, PROGRAM_NAME, given, PROGRAM_NAME);
		exit(EXIT_FAILURE);
	}
}

static void list_available_fonts (void)
{
	static const char *const names[NO_FONTS] =
	{
    	"bulbhead",
		"braced",
		"fraktur",
		"hollywood",
		"larry3d",
		"raw",
		"rectangles",
		"short (default)",
	};

	printf("%s - list of available fonts:\n", PROGRAM_NAME);
	for (unsigned short i = 0; i < NO_FONTS; i++) printf("  - %s\n", names[i]);
}

static void make_sure_content_fits_in (struct ft *ft, const unsigned short setsz, const unsigned short erows, const unsigned short ecols, const bool viasig)
{
	obtain_window_dimensions(ft);

	const unsigned short rowsd = ft->font.rows + erows;
	const unsigned short colsd = ft->font.cols * setsz + ecols;

	if (rowsd > ft->winrows || colsd > ft->wincols)
	{
		const char *errmsg =
		"\x1b[2J\x1b[H%s:error: aboring now!\n"
		" terminal dimensions are not enough to display %s's content\n"
		" your progress (if any) will be saved now!\n"
		" minimum needed: %d rows and %d columns\n"
		" current values: %d rows and %d columns\n";

		if (viasig) { /* TODO enable canonical mode & echo */ }

		fprintf(stderr, errmsg, PROGRAM_NAME, PROGRAM_NAME, rowsd, colsd, ft->winrows, ft->wincols);
		exit(EXIT_FAILURE);
	}
}

static void do_font_preview (struct ft *ft)
{
	set_rendering_font(&ft->font, ft->args.font);
	make_sure_content_fits_in(ft, FONT_CHARSET_SIZE, 0, 0, false);

	for (unsigned short i = 0; i < ft->font.rows; i++)
	{
		printf("%s %s %s %s %s %s %s %s %s %s %s\n",
		ft->font.set[ 0][i],
		ft->font.set[ 1][i],
		ft->font.set[ 2][i],
		ft->font.set[ 3][i],
		ft->font.set[ 4][i],
		ft->font.set[ 5][i],
		ft->font.set[ 6][i],
		ft->font.set[ 7][i],
		ft->font.set[ 8][i],
		ft->font.set[ 9][i],
		ft->font.set[10][i]
		);
	}
}

