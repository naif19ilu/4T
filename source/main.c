/* 4T - Terminal Task Time Tracker
 * 29 Jul 2025
 * main file
 */
#include "cxa.h"

#include <err.h>
#include <time.h>
#include <stdio.h>
#include <errno.h> 
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#define DESC          "4T   [-t <task>] [flags]"
#define WIDEST_FONT   17
#define ALPHA_SIZE    11
#define CHARS_DISPL   8

#define FD_STDOUT     1
#define FD_STDIN      0

#define DEFAULT_T     30
#define DEFAULT_f     "bulbhead"

#define NO_READS      17

/* Number of lines used to display name of the task,
 * message and quote plus 2 blank lines
 */
#define CTE_LINES     5

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

struct fourt
{
	struct termios defterm;
	struct font    font;
	unsigned int   wcols, wrows;
	unsigned int   secworked;
	int            stdflags;

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

static const char *const Reads[NO_READS] =
{
	"tengo reloj, no me interesa el tiempo porque se va y ya no puedo detenerlo",
	"el paso de las horas un delito sin culpable",
	"au fait, qui est ce qui dessine les nuages?",
	"no me soya la playa pero aqui estamos, a veces no escogemos donde terminamos",
	"hasta luego...",
	"ya va'ber tiempo pa' pensar, hoy solo queda reaccionar a 'como fue?'",
	"soulless friends <3",
	"...je veux vivre (2/2)",
	"el tiempo ni se gana ni se pierde",
	"aqui no se aceptan limites",
	"un jour, algun dia, one day",
	"lo esencial es invisible ante los ojos",
	"felices no estamos, pero estamos y es mas que algo",
	"no one else can do what you're here to do",
	"happy seeing you working :)",
	"voy a ser mejor que el que esta haciendo esto",
	"happy little clouds..."
};

static bool WannaQuitViaSignal = false;
static bool TerminalResized    = false;

inline static void get_terminal_dimensions (unsigned int *rows, unsigned int *cols)
{
	struct winsize w;
	ioctl(FD_STDOUT, TIOCGWINSZ, &w);	
	*rows = (unsigned int) w.ws_row;
	*cols = (unsigned int) w.ws_col;
}

inline static void check_space_is_enough (const unsigned int tr, const unsigned int tc, const struct font *f, const short factor)
{
	const unsigned int colsed = (f->cols + 1) * factor;
	if (((unsigned int) (f->rows + CTE_LINES) < tr) && (colsed < tc))
	{
		return;
	}

	fprintf(stderr, "4T: error: terminal dimensions are not big enough to render\n");
	fprintf(stderr, " solutions:\n");
	fprintf(stderr, "  1. increase the terminal size.\n");
	fprintf(stderr, "  2. decrease the font size.\n");
	exit(0);
}

static void intro (struct fourt*);
static void outro (struct fourt*);

static void start_clock (struct fourt*, const signed int);
static bool pick_font (struct font*, const char*);

static void display_constant_stuff (struct font*, const unsigned int, const unsigned int, const char*);
static void display_pair (struct font*, const unsigned int, const enum time_type, const unsigned int, const unsigned int);

static void display_font_names (void);
static void do_preview (const char*);

static void setup_signals (void);
static void signal_hanlder (const int);

int main (int argc, char **argv)
{
	struct fourt ft;
	memset(&ft, 0, sizeof(ft));

	ft.args.time = DEFAULT_T;
	ft.args.font = DEFAULT_f;

	struct CxaFlag flags[] =
	{
		CXA_SET_STR("task",   "task you'll be working on",      &ft.args.task, CXA_FLAG_TAKER_YES, 't'),
		CXA_SET_STR("font",   "font to display time left",      &ft.args.font, CXA_FLAG_TAKER_YES, 'f'),
		CXA_SET_STR("info",   "display info about a task",      &ft.args.task, CXA_FLAG_TAKER_MAY, 'i'),		// TODO
		CXA_SET_STR("prev",   "preview font (needs font name)", &ft.args.font, CXA_FLAG_TAKER_YES, 'p'),
		CXA_SET_INT("time",   "time you'll work (30 default)",  &ft.args.time, CXA_FLAG_TAKER_YES, 'T'),
		CXA_SET_CHR("help",   "display this message :)",        NULL,          CXA_FLAG_TAKER_NON, 'h'),
		CXA_SET_CHR("list",   "list font names",                NULL,          CXA_FLAG_TAKER_NON, 'L'),
		CXA_SET_CHR("undef",  "undefined time working",         NULL,          CXA_FLAG_TAKER_NON, 'u'),
		CXA_SET_END
	};

	cxa_clean(cxa_execute((unsigned char) argc, argv, flags, "4T"));

	if (flags[6].meta & CXA_FLAG_SEEN_MASK)                             { display_font_names();         return 0; }
	if (flags[3].meta & CXA_FLAG_SEEN_MASK)                             { do_preview(ft.args.font);     return 0; }
	if ((ft.args.task == NULL) || (flags[5].meta & CXA_FLAG_SEEN_MASK)) { cxa_print_usage(DESC, flags); return 0; }

	if (*ft.args.task == 0)
	{
		fprintf(stderr, "4T: error: make sure to name your task (--task)\n");
		fprintf(stderr, "everything you do should be worth enough to name it!\n");
		return 0;
	}

	setup_signals();

	intro(&ft);
	start_clock(&ft, ((flags[7].meta & CXA_FLAG_SEEN_MASK) == CXA_FLAG_WAS_SEEN) ? 1 : -1);

	outro(&ft);
	printf("worked: %d\n", ft.secworked);

	return 0;
}

static void intro (struct fourt *ft)
{
	get_terminal_dimensions(&ft->wrows, &ft->wcols);
	pick_font(&ft->font, ft->args.font);
	check_space_is_enough(ft->wrows, ft->wcols, &ft->font, CHARS_DISPL);

	printf("\x1b[2J\x1b[H\x1b[0\x1b[?25l");
	fflush(stdout);

	tcgetattr(FD_STDIN, &ft->defterm);
	struct termios conf = ft->defterm;

	conf.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(FD_STDIN, TCSANOW, &conf);

	ft->stdflags = fcntl(FD_STDIN, F_GETFL);
	fcntl(FD_STDIN, F_SETFL, ft->stdflags | O_NONBLOCK);

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
	fcntl(FD_STDIN, F_SETFL, ft->stdflags);
	tcsetattr(FD_STDIN, TCSANOW, &ft->defterm);
	printf("\x1b[H\x1b[2J\x1b[?25h");
	fflush(stdout);
}

static void start_clock (struct fourt *fourt, const signed int dt)
{
	if (fourt->args.time <= 0) { fourt->args.time = DEFAULT_T; }
	unsigned int spssdby = (dt == -1) ? (unsigned int) fourt->args.time * 60 : 0;

	char key;
	unsigned int start_r = ((fourt->wrows - fourt->font.rows              ) >> 1);
	unsigned int start_c = ((fourt->wcols - fourt->font.cols * CHARS_DISPL) >> 1);

	display_constant_stuff(&fourt->font, start_r, start_c, fourt->args.task);
	signed int hr = -1, min = -1;

	bool working = true, paused = false;
	while (working && !WannaQuitViaSignal)
	{
		if (TerminalResized)
		{
			printf("\x1b[2J\x1b[H");
			get_terminal_dimensions(&fourt->wrows, &fourt->wcols);
			pick_font(&fourt->font, fourt->args.font);
			check_space_is_enough(fourt->wrows, fourt->wcols, &fourt->font, CHARS_DISPL);

			start_r = ((fourt->wrows - fourt->font.rows              ) >> 1);
			start_c = ((fourt->wcols - fourt->font.cols * CHARS_DISPL) >> 1);
			display_constant_stuff(&fourt->font, start_r, start_c, fourt->args.task);


			TerminalResized = false;
			continue;
		}
		if (!paused)
		{
			const signed int h = spssdby / 3600;
			const signed int m = (spssdby % 3600) / 60;
			const signed int s = (spssdby % 60);

			if (h != hr)  { display_pair(&fourt->font, h, time_t_hours  , start_r, start_c); hr = h;  }
			if (m != min) { display_pair(&fourt->font, m, time_t_minutes, start_r, start_c); min = m; }
			display_pair(&fourt->font, s, time_t_seconds, start_r, start_c);

			sleep(1);
			spssdby += dt;
			fourt->secworked++;
		}

		if (spssdby == 0) { working = false; }
		switch ((key = getchar()))
		{
			case 'q': { fourt->secworked--; working = false; break; }
			case ' ': { paused = !paused; break; }
		}
	}
}

static bool pick_font (struct font *font, const char *asked)
{
	/* Whatever is given to you via argv is null terminated
	 * so we do not need to compare at most N bytes
	 */
	     if (!strcmp(asked, "rectangles")) { *font = f_rectangles; return true;  }
	else if (!strcmp(asked, "hollywood"))  { *font = f_hollywood;  return true;  }
	else if (!strcmp(asked, "bulbhead"))   { *font = f_bulbhead;   return true;  }
	else if (!strcmp(asked, "fraktur"))    { *font = f_fraktur;    return true;  }
	else if (!strcmp(asked, "larry3d"))    { *font = f_larry3d;    return true;  }
	else if (!strcmp(asked, "braced"))     { *font = f_braced;     return true;  }
	else if (!strcmp(asked, "short"))      { *font = f_short;      return true;  }
	else if (!strcmp(asked, "raw"))        { *font = f_raw;        return true;  }
	else                                   { *font = f_bulbhead;   return false; }
}

static void display_constant_stuff (struct font *font, const unsigned int start_r, const unsigned int start_c, const char *task)
{
	unsigned int coffset = start_c + font->cols * 2;
	for (unsigned char i = 0; i < 2; i++)
	{
		for (unsigned short line = 0; line < font->rows; line++)
		{
			printf("\x1b[2m\x1b[%d;%dH%s\x1b[0m", start_r + line, coffset, font->set[10][line]);
		}
		coffset += font->cols * 3;
	}

	const unsigned int taskrow = start_r + font->rows + 2;
	const unsigned int taskcol = start_c;

	printf("\x1b[%d;%dHworking on \x1b[1m%s\x1b[0m", taskrow, taskcol, task);
	printf("\x1b[%d;%dH\x1b[2mpress 'q' to quit and save!\x1b[0m", taskrow + 1, taskcol);

	srand(time(NULL));
	const unsigned int readp = (rand() % NO_READS);
	printf("\x1b[%d;%dH\x1b[2m\x1b[5m%s\x1b[0m", taskrow + 2, taskcol, Reads[readp]);
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
}

static void display_font_names (void)
{
	const unsigned int no = 8;
	static const char *const fonts[] =
	{
		"braced",
		"bulbhead (default)",
		"fraktur",
		"hollywood",
		"larry3d",
		"raw",
		"rectangles",
		"short"
	};

	printf("\n\t4T List of fonts available!\n");
	for (unsigned int i = 0; i < no; i++) { printf("\t - %s\n", fonts[i]); }

	printf("\tremember to use -f flag to use any of these (-f <font> or --flag=<font> or --flag <font>\n\n");
}

static void do_preview (const char *of)
{
	struct font font;
	unsigned int rows, cols;
	get_terminal_dimensions(&rows, &cols);

	const bool exists = pick_font(&font, of);
	check_space_is_enough(rows, cols, &font, ALPHA_SIZE);

	printf("\x1b[2J\x1b[H\n4T: preview of '%s' font\n", exists ? of : "bulbhead (wrong name given, so displaying default)");
	for (unsigned char i = 0; i < 11; i++)
	{
		for (unsigned short line = 0, pl = 4; line < font.rows; line++)
		{
			printf("\x1b[%d;4H%s %s %s %s %s %s %s %s %s %s %s",
			pl++,
			font.set[ 0][line],
			font.set[ 1][line],
			font.set[ 2][line],
			font.set[ 3][line],
			font.set[ 4][line],
			font.set[ 5][line],
			font.set[ 6][line],
			font.set[ 7][line],
			font.set[ 8][line],
			font.set[ 9][line],
			font.set[10][line]
			);
		}
	}
	printf("\n\n");
}

static void setup_signals (void)
{
	struct sigaction sa;
	sa.sa_handler = signal_hanlder;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	errno = 0;
	int sigret = 0;
	sigret = sigaction(SIGINT,   &sa, NULL);
	sigret = sigaction(SIGHUP,   &sa, NULL);
	sigret = sigaction(SIGQUIT,  &sa, NULL);
	sigret = sigaction(SIGTERM,  &sa, NULL);
	sigret = sigaction(SIGWINCH, &sa, NULL);

	if (sigret)
	{
		err(EXIT_FAILURE, "fatal internal; cannot continue");
	}
}

static void signal_hanlder (const int what)
{
	switch (what)
	{
		case SIGINT:
		case SIGHUP:
		case SIGQUIT:
		case SIGTERM:  { WannaQuitViaSignal = true; break; }
		case SIGWINCH: { TerminalResized = true;    break; }
	}
}
