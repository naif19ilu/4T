#include "front.h"
#include "common.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#define INTRO_ANSI             "\x1b[?1049h\x1b[?25l\x1b[H"
#define OUTRO_ANSI             "\x1b[?1049l\x1b[?25h"

#define WIDEST_FONT            17
#define FONT_CHARSET_SIZE      11

/* Number of characters defined within a font_t
 * to be displayed in screen xx:xx:xx (8)
 */
#define RENDER_CHARSET_SIZE    8
/* Besides the time left/passed we need to display
 * other information, this number indicates how many
 * lines are going to be used
 */
#define EXTRA_RENDERED_LINES   3

#define COLON_INDEX            10

struct font_t
{
	char *set[FONT_CHARSET_SIZE][WIDEST_FONT];
	unsigned short height, width;
};

#include "fontset.h"

struct front
{
	struct termios deftty;
	struct font_t  font;
	const char     *fontname, *taskname;
	unsigned int   s_total, s_workd;
	unsigned short w_height, w_width;
};

static volatile sig_atomic_t Resize     = FALSE;
static bool_t                Terminated = FALSE;

static inline void get_window_dimensions (unsigned short *w_height, unsigned short *w_width)
{
	struct winsize szs;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &szs);

	*w_height = (unsigned short) szs.ws_row;
	*w_width  = (unsigned short) szs.ws_col;
}

static inline void compute_rendering_origin (struct font_t *font, const unsigned short w_height, const unsigned short w_width, unsigned short *ori_y,  unsigned short *ori_x)
{
	*ori_y = (w_height - font->height) >> 1;
	*ori_x = (w_width  - font->width * RENDER_CHARSET_SIZE) >> 1;
}

enum state
{
	state_wkg = 0,
	state_psd = 1,
};

/* besides of saying what type of metric is, it also provides
 * the offset at which the value should be rendered
 * hh:mm:ss
 * |  |  ` sixth one
 * |  ` third character to be redered
 * 0 offset
 */
enum temps
{
	temps_hur = 0,
	temps_min = 3,
	temps_sec = 6,
};

static const char *const States[] =
{
	"working",
	"paused "
};

static void intro_ (struct termios*);
static void outro_ (struct termios*);

static void signal_handler (int);
static struct font_t pick_final_font (const char*);

static void main_loop (struct front*);
static void fits_in (struct front*, const unsigned short, const unsigned short, const bool_t);

static void render_constant (struct font_t*, const unsigned short, const unsigned, const char*);
static void render_dynamic (struct font_t*, const unsigned int, const unsigned short, const unsigned short, const enum temps);

void frontend_execute (const char *taskname, const char *fontname, const int time)
{
	struct front front = {
		.font     = pick_final_font(fontname),
		.fontname = fontname,
		.taskname = taskname,
		.s_total  = time * 60,
		.s_workd  = 0
	};

	intro_(&front.deftty);
	main_loop(&front);

	if (!Terminated) outro_(&front.deftty);
}

void frontend_list_available_fonts (void)
{
	printf("%s - list of available fonts\n", PROGRAM_NAME);
	for (unsigned short i = 0; i < NO_FONTS; i++)
		printf(" * %s\n", FontNames[i]);
}

void frontend_do_preview (const char *fontname)
{
	struct front front = { .font = pick_final_font(fontname) };
	fits_in(&front, FONT_CHARSET_SIZE + 1, 0, FALSE);

	if (Terminated) return;

	for (unsigned short line = 0; line < front.font.height; line++)
		printf("%*s%s%s%s%s%s%s%s%s%s%s\n\r",
		front.font.width * 2,
		front.font.set[ 0][line],
		front.font.set[ 1][line],
		front.font.set[ 2][line],
		front.font.set[ 3][line],
		front.font.set[ 4][line],
		front.font.set[ 5][line],
		front.font.set[ 6][line],
		front.font.set[ 7][line],
		front.font.set[ 8][line],
		front.font.set[ 9][line],
		front.font.set[10][line]
		);
}

static void intro_ (struct termios *deftty)
{
	/* Signal hanlder
	 * All brute force methods to exit
	 * the program will be ignore (as
	 * long as it is possible), the only
	 * signal the program will handle is
	 * when the terminal gets resized
	 */
	struct sigaction s;
	s.sa_handler = signal_handler;
	sigemptyset(&s.sa_mask);

	sigaction(SIGWINCH, &s, NULL);

	sigset_t block;
	sigemptyset(&block);

	sigaddset(&block, SIGTSTP);
	sigaddset(&block, SIGINT);
	sigaddset(&block, SIGQUIT);
	sigaddset(&block, SIGHUP);
	sigprocmask(SIG_BLOCK, &block, NULL);

	/* Terminal configuration
	 *  - do not deal with CTRL-S/CTRL-Q (full buffers)
	 *  - no string post processing
	 *  - no displaying characters & provide chars as they're typed
	 *  - only read one characters to be read
	 */
	tcgetattr(STDIN_FILENO, deftty);
	struct termios custty = *deftty;

	custty.c_iflag &= ~(IXON | IXOFF);
	custty.c_oflag &= ~(OPOST);
	custty.c_lflag &= ~(ICANON | ECHO);
	custty.c_cc[VMIN] = 1;

	tcsetattr(STDIN_FILENO, TCSANOW, &custty);

	printf(INTRO_ANSI);
	fflush(stdout);
}

static void outro_ (struct termios *deftty)
{
	tcsetattr(STDIN_FILENO, TCSANOW, deftty);
	printf(OUTRO_ANSI);
	fflush(stdout);
}

static void signal_handler (int s)
{
	(void) s;
	Resize = TRUE;
}

static struct font_t pick_final_font (const char *name)
{
	/* since 'given' is a string given via argv, it is assumed to be
	 * null-byte terminated, therefore we do not need to worry about
	 * variable lengths
	 */
	     if (!strcmp(name, "bulbhead"))   { return f_bulbhead;   }
	else if (!strcmp(name, "braced"))     { return f_braced;     }
	else if (!strcmp(name, "fraktur"))    { return f_fraktur;    }
	else if (!strcmp(name, "hollywood"))  { return f_hollywood;  }
	else if (!strcmp(name, "larry3d"))    { return f_larry3d;    }
	else if (!strcmp(name, "raw"))        { return f_raw;        }
	else if (!strcmp(name, "rectangles")) { return f_rectangles; }
	else if (!strcmp(name, "short"))      { return f_short;      }
	else
	{
		static const char *const errmsg =
		"%s: error: '%s' is not defined as a font\n"
		" make sure it exists by checking all available fonts\n"
		" $ %s --list\n";
		fprintf(stderr, errmsg, PROGRAM_NAME, name, PROGRAM_NAME);
		exit(EXIT_FAILURE);
	}
}

static void main_loop (struct front *front)
{
	bool_t quit = FALSE, pause = FALSE, render_1 = TRUE;

	struct timeval tv;
	fd_set inset;

	unsigned short ori_y, ori_x;
	enum state state = state_wkg;

	while (!quit && !Terminated)
	{
		tv.tv_sec  = 1 - render_1;
		tv.tv_usec = 0;

		FD_ZERO(&inset);
		FD_SET(STDIN_FILENO, &inset);

		const int ret = select(STDIN_FILENO + 1, &inset, NULL, NULL, &tv);
		if ((ret == -1 && Resize) || render_1)
		{
			printf("\x1b[2J");

			fits_in(front, RENDER_CHARSET_SIZE, EXTRA_RENDERED_LINES, TRUE);
			if (Terminated == TRUE) break;

			compute_rendering_origin(&front->font, front->w_height, front->w_width, &ori_y, &ori_x);
			render_constant(&front->font, ori_y, ori_x, front->taskname);

			Resize   = FALSE;
			render_1 = FALSE;
			continue;
		}

		if (FD_ISSET(STDIN_FILENO, &inset))
		{
			switch (fgetc(stdin))
			{
				case 'q': quit  = TRUE; break;
				case ' ': pause = !pause; state = 1 - state; break;
				case '+': break;
				case 'L': break;
			}

			continue;
		}

		render_dynamic(&front->font, front->s_workd, ori_y, ori_x, temps_sec);
		if (front->s_workd++ == front->s_total) break;
	}
}

static void fits_in (struct front* front, const unsigned short setsz, const unsigned short plsrws, const bool_t timerunning)
{
	get_window_dimensions(&front->w_height, &front->w_width);

	const unsigned short w_needed = front->font.width  * setsz;
	const unsigned short h_needed = front->font.height + plsrws;

	if (w_needed < front->w_width && h_needed < front->w_height) return;

	static const char *const errmsg =
	"%s:error: cannot continue since the dimensions are too small\n"
	" minimum dimensions: %d rows by %d columns\n"
	" current dimensions: %d rows by %d columns\n"
	" all progress (if any) will be saved!\n";

	if (timerunning) { outro_(&front->deftty); }

	fprintf(stderr, errmsg, PROGRAM_NAME, w_needed, h_needed, front->w_height, front->w_width);
	fflush(stderr);

	Terminated = TRUE;
}

static void render_constant (struct font_t *font, const unsigned short ori_y, const unsigned ori_x, const char *task)
{
	const unsigned short coffset[] =
	{
		font->width * 2,
		font->width * 5,
	};

	for (unsigned short i = 0; i < 2; i++)
		for (unsigned short line = 0; line < font->height; line++)
			printf("\x1b[5m\x1b[%d;%dH%s\x1b[0m", ori_y + line, ori_x + coffset[i], font->set[COLON_INDEX][line]);

	const unsigned short loffset = ori_y + font->height + 2;
	
	printf("\x1b[%d;%dHworking on \x1b[1m%s\x1b[0m",            loffset + 0, ori_x, task);
	printf("\x1b[%d;%dH\x1b[2mpress 'q' to save & quit\x1b[0m", loffset + 1, ori_x);
	printf("\x1b[%d;%dHstate: %s",                              loffset + 2, ori_x, States[state_wkg]);

	fflush(stdout);
}

static void render_dynamic (struct font_t *font, const unsigned int val, const unsigned short ori_y, const unsigned short ori_x, const enum temps temps)
{
	const unsigned short idxs[] = { (unsigned short) val / 10, (unsigned short) val % 10};
	const unsigned short offs[] = { ori_x + temps * font->width, ori_x + (temps + 1) * font->width };

	for (unsigned short line = 0; line < font->height; line++)
		printf("\x1b[%d;%dH%s\x1b[%d;%dH%s",
		ori_y + line, offs[0], font->set[idxs[0]][line],
		ori_y + line, offs[1], font->set[idxs[1]][line]
		);

	fflush(stdout);
}
