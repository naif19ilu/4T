/* 4T - terminal task time tracker
 * Aug 1 2025
 * Main file
 */
#if defined(_WIN32) || defined(_WIN64)
	#error "4T is not available for Windows"
#endif

#include "cxa.h"

#include <err.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <termios.h>
#include <sys/ioctl.h>

#define PROGRAM_NAME                 "4T"
#define PROGRAM_USAGE                "4T [-t <taks>] [flags]"

#define FLAG_TASK_DESC               "task you will be working on (mandatory)"
#define FLAG_FONT_DESC               "font to display time remaining/passed by (short default)"
#define FLAG_TIME_DESC               "total amount of minutes working (30 default)"
#define FLAG_LIST_DESC               "lists all availbale fonts"
#define FLAG_PREV_DESC               "do a preview of <fontname> font"
#define FLAG_CLOCK_DESC              "show actual time instead of timer"
#define FLAG_UNDEF_DESC              "undefined timw working (count-up)"

#define FLAG_FONT_DEFAULT            "short"
#define FLAG_TIME_DEFAULT            30

#define FONT_WIDEST_SYMBOL           17
#define NO_FONTS                     8
#define FONT_CHARSET_SIZE            11
#define FONT_NO_DISPLAYED_CHARS      8

#define STDIN_FD                     0
#define STDOUT_FD                    1

#define PREDULE_TIME_S               0
#define EXTRA_RENDERED_LINES         4

#define STATE_WORKING                "working"
#define STATE_PAUSED                 "paused "

struct font
{
	char *set[FONT_CHARSET_SIZE][FONT_WIDEST_SYMBOL];
	unsigned short rows, cols;
	unsigned short srow, scol;
};

#include "fonts.inc"

enum temps_kind
{
	temps_is_seconds,
	temps_is_minutes,
	temps_is_hours,
};

enum state
{
	state_working  = 0,
	state_paused   = 1,
	state_noneeded = 2,
};

struct xargs_
{
	char *task;
	char *font;
	unsigned short time;
};

struct ft
{
	struct termios stdterm;
	struct xargs_ args;
	struct font font;
	int stdfcntl;
	unsigned short secworked;
	unsigned short winrows, wincols;
};

static const char *const States[] =
{
	"working",
	"paused "
};

static inline void obtain_window_dimensions (unsigned short *rows, unsigned short *cols)
{
	struct winsize w;
	assert(ioctl(STDOUT_FD, TIOCGWINSZ, &w) == 0);

	*rows = (unsigned short) w.ws_row;
	*cols = (unsigned short) w.ws_col;
}

static void set_defaults (struct ft*);
static void set_rendering_font (struct font*, const char*);

static void list_available_fonts (void);
static void make_sure_content_fits_in (struct ft*, const unsigned short, const unsigned short, const unsigned short, const bool);

static void do_font_preview (struct ft*);
static void config_terminal_intro (struct ft*);

static void config_terminal_outro (struct ft*);
static bool set_up_signals (void);

static void signal_daddy (const int);
static void task_prelude (const unsigned short, const unsigned);

static void start_timer (struct ft*, const bool, const bool);
static void render_constants (const struct font*);

static void render_dyanmics (const struct font*, const unsigned int, const enum temps_kind, const enum state);

int main (int argc, char **argv)
{
	struct ft ft;
	set_defaults(&ft);

	struct CxaFlag flags[] =
	{
		CXA_SET_STR("task",  FLAG_TASK_DESC,  &ft.args.task, CXA_FLAG_TAKER_YES, 't'),
		CXA_SET_STR("font",  FLAG_FONT_DESC,  &ft.args.font, CXA_FLAG_TAKER_YES, 'f'),
		CXA_SET_SHT("time",  FLAG_TIME_DESC,  &ft.args.time, CXA_FLAG_TAKER_YES, 'T'),
		CXA_SET_STR("prev",  FLAG_PREV_DESC,  &ft.args.font, CXA_FLAG_TAKER_MAY, 'p'),
		CXA_SET_CHR("list",  FLAG_LIST_DESC,  NULL,          CXA_FLAG_TAKER_NON, 'L'),
		CXA_SET_CHR("clock", FLAG_CLOCK_DESC, NULL,          CXA_FLAG_TAKER_NON, 'c'),
		CXA_SET_CHR("undef", FLAG_UNDEF_DESC, NULL,          CXA_FLAG_TAKER_NON, 'u'),

		CXA_SET_END
	};

	cxa_clean(cxa_execute((unsigned char) argc, argv, flags, PROGRAM_NAME));

	if (flags[4].meta & CXA_FLAG_SEEN_MASK)
	{
		list_available_fonts();
		return 0;
	}

	if (flags[3].meta & CXA_FLAG_SEEN_MASK)
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
	config_terminal_intro(&ft);

	if (set_up_signals() == false)
	{
		config_terminal_outro(&ft);
		err(EXIT_FAILURE, "fatal internal; cannot continue");
	}

	const bool c_seen = (flags[5].meta & CXA_FLAG_SEEN_MASK);
	const bool u_seen = (flags[6].meta & CXA_FLAG_SEEN_MASK);

	start_timer(&ft, c_seen, u_seen);

	config_terminal_outro(&ft);
	return 0;
}

static void set_defaults (struct ft *ft)
{
	memset(ft, 0, sizeof(*ft));
	ft->args.font = FLAG_FONT_DEFAULT;
	ft->args.time = FLAG_TIME_DEFAULT;
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

static void make_sure_content_fits_in (struct ft *ft, const unsigned short setsz, const unsigned short erows, const unsigned short ecols, const bool isprev)
{
	obtain_window_dimensions(&ft->winrows, &ft->wincols);

	const unsigned short rowsd = ft->font.rows + erows;
	const unsigned short colsd = ft->font.cols * setsz + ecols;

	/* even though this should not be here, each time the terminal is resized
	 * the position where the first character should be displayed needs to be
	 * recomputed, this function is called each time the terminal is resiszed
	 * so this is the perfect place
	 */
	ft->font.srow = (ft->winrows - ft->font.rows) >> 1;
	ft->font.scol = (ft->wincols - ft->font.cols * FONT_NO_DISPLAYED_CHARS) >> 1;

	if (rowsd < ft->winrows && colsd < ft->wincols) { return; }

	const char *errmsg =
	"\x1b[2J\x1b[H%s:error: aboring now!\n"
	" terminal dimensions are not enough to display %s's content\n"
	" your progress (if any) will be saved now!\n"
	" minimum needed: %d rows and %d columns\n"
	" current values: %d rows and %d columns\n";

	if (isprev == false) { config_terminal_outro(ft); /* TODO save progress from here */ }

	fprintf(stderr, errmsg, PROGRAM_NAME, PROGRAM_NAME, rowsd, colsd, ft->winrows, ft->wincols);
	fflush(stderr);
	exit(EXIT_FAILURE);
}

static void do_font_preview (struct ft *ft)
{
	set_rendering_font(&ft->font, ft->args.font);
	make_sure_content_fits_in(ft, FONT_CHARSET_SIZE, 0, FONT_CHARSET_SIZE, true);

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

static void config_terminal_intro (struct ft *ft)
{
	printf("\x1b[2J\x1b[H\x1b[?25l");
	fflush(stdout);

	tcgetattr(STDIN_FD, &ft->stdterm);
	struct termios custom = ft->stdterm;

	custom.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FD, TCSANOW, &custom);

	ft->stdfcntl = fcntl(STDIN_FD, F_GETFL);
	fcntl(STDIN_FD, F_SETFL, ft->stdfcntl | O_NONBLOCK);
}

static void config_terminal_outro (struct ft *ft)
{
	printf("\x1b[2J\x1b[H\x1b[?25h");
	fflush(stdout);

	tcsetattr(STDIN_FD, TCSANOW, &ft->stdterm);
	fcntl(STDIN_FD, F_SETFL, ft->stdfcntl);
}

static bool set_up_signals (void)
{
	struct sigaction s;
	s.sa_handler = signal_daddy;
	sigemptyset(&s.sa_mask);

	errno = 0;
	int sigret = 0;
	sigret = sigaction(SIGINT,   &s, NULL);
	sigret = sigaction(SIGHUP,   &s, NULL);
	sigret = sigaction(SIGQUIT,  &s, NULL);
	sigret = sigaction(SIGTERM,  &s, NULL);
	sigret = sigaction(SIGWINCH, &s, NULL);
	sigret = sigaction(SIGTSTP,  &s, NULL);

	if (sigret) { return false; }
	return true;
}

static void signal_daddy (const int sg)
{
	switch (sg)
	{
		case SIGINT  :
		case SIGHUP  :
		case SIGQUIT :
		case SIGTERM : { break; }
		case SIGWINCH: { break; }
		case SIGTSTP : { break; }
	}
}

static void task_prelude (const unsigned short rows, const unsigned cols)
{
	static const char *const welcome[] =
	{
		"\x1b[%d;%dHstarting in %d\x1b[5m.  \x1b[0m",
		"\x1b[%d;%dHstarting in %d\x1b[5m.. \x1b[0m",
		"\x1b[%d;%dHstarting in %d\x1b[5m...\x1b[0m",
	};

	const unsigned short row = ((rows -  0) >> 1);
	const unsigned short col = ((cols - 16) >> 1);

	for (unsigned short i = 0, p = PREDULE_TIME_S; i < PREDULE_TIME_S; i++)
	{
		printf(welcome[i % 3], row, col, p--);
		fflush(stdout);
		sleep(1);
	}

	printf("\x1b[2J");
	fflush(stdout);
}

static void start_timer (struct ft *ft, const bool c_seen, const bool u_seen)
{
	make_sure_content_fits_in(ft, FONT_NO_DISPLAYED_CHARS, EXTRA_RENDERED_LINES, 0, false);
	task_prelude(ft->winrows, ft->wincols);
	render_constants(&ft->font);

	int s_pssdby = ft->args.time * 60, dt = (c_seen || u_seen) ? 1 : -1;
	int h_render = -1, m_render = -1, s_render = -1;

	if (c_seen)
	{
		time_t s1970;
		time(&s1970);
		struct tm *temps = localtime(&s1970);

		h_render = temps->tm_hour;
		m_render = temps->tm_min;
		s_render = temps->tm_sec;
		s_pssdby = temps->tm_sec;
	}
	if (u_seen) { s_pssdby = 0; }

	bool paused = false;
	enum state state = (c_seen) ? state_working : state_noneeded;

	while (1)
	{
		const char key = getchar();
		if (key == ' ')        { paused = !paused; if (c_seen) { state = (enum state) 1 - state; } }
		if (key == 'q')        { break; }
		if (paused && !c_seen) { sleep(1); continue; }

		if (c_seen)
		{
			s_render = s_pssdby;
			if (s_render == 60) { s_pssdby = 0; m_render++; }
			if (m_render == 60) { m_render = 0; h_render++; s_pssdby = 0; }
			if (h_render == 24) { s_pssdby = 0; m_render = 0; h_render = 0; }

			render_dyanmics(&ft->font, s_render, temps_is_seconds, state);
			render_dyanmics(&ft->font, m_render, temps_is_minutes, state);
			render_dyanmics(&ft->font, h_render, temps_is_hours, state);
		}
		else
		{
			h_render = (s_pssdby / 3600);
			m_render = (s_pssdby % 3600) / 60;
			s_render = (s_pssdby % 60);

			// TODO
			render_dyanmics(&ft->font, s_render, temps_is_seconds, state);
			render_dyanmics(&ft->font, m_render, temps_is_minutes, state);
			render_dyanmics(&ft->font, h_render, temps_is_hours, state);
		}

		sleep(1);
		s_pssdby += dt;

		if (!paused) { ft->secworked++; }
		if (s_pssdby == 0) { break; }
	}
}

static void render_constants (const struct font *font)
{
	const unsigned short offset[] =
	{
		font->scol + font->cols * 2,
		font->scol + font->cols * 5,
	};

	for (unsigned short j = 0; j < font->rows; j++)
		printf("\x1b[5m\x1b[%d;%dH%s\x1b[%d;%dH%s\x1b[0m",
		j + font->srow,
		offset[0],
		font->set[10][j],
		j + font->srow,
		offset[1],
		font->set[10][j]
		);

	const unsigned short newr = font->srow + font->rows + 1;
	printf("\x1b[%d;%dHworking on \x1b[1m%s\x1b[0m", newr, font->scol, "task");
	printf("\x1b[%d;%dHpress 'q' to quit and save", newr + 1, font->scol);
	fflush(stdout);
}

static void render_dyanmics (const struct font *font, const unsigned int temps, const enum temps_kind tk, const enum state cstt)
{
	static enum state lastt = state_noneeded;
	if (lastt != cstt)
	{
		printf("\x1b[%d;%dHstate: %s", font->srow + EXTRA_RENDERED_LINES + 1, font->scol, States[cstt]);
		lastt = cstt;
	}

	unsigned short offset = font->scol;

	switch (tk)
	{
		case temps_is_seconds: { offset += font->cols * 6; break; }
		case temps_is_minutes: { offset += font->cols * 3; break; }
		default: { break; }
	}

	const unsigned short d1 = temps / 10;
	const unsigned short d2 = temps % 10;

	for (unsigned short i = 0; i < font->rows; i++)
		printf("\x1b[%d;%dH%s%s", font->srow + i, offset, font->set[d1][i], font->set[d2][i]);

	fflush(stdout);
}
