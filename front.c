#include "front.h"
#include "common.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#define INTRO_ANSI         "\x1b[?1049h\x1b[?25l\x1b[H"
#define OUTRO_ANSI         "\x1b[?1049l\x1b[?25h"

#define WIDEST_FONT        17
#define FONT_CHARSET_SIZE  11

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
	unsigned int   w_height, w_width;
	unsigned int   s_left, s_pssd;
};

static volatile sig_atomic_t Resize = FALSE;

static inline void get_window_dimensions (unsigned int *w_height, unsigned int *w_width)
{
	struct winsize szs;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &szs);

	*w_height = szs.ws_row;
	*w_width  = szs.ws_col;
}

static void intro_ (struct termios*, unsigned int*, unsigned int*);
static void outro_ (struct front*);

static void signal_handler (int);
static struct font_t pick_final_font (const char*);

static void main_loop (struct front*);

void frontend_execute (const char *taskname, const char *fontname, const int time)
{
	struct front front = {
		.font     = pick_final_font(fontname),
		.fontname = fontname,
		.taskname = taskname,
		.s_left   = time * 60,
		.s_pssd   = 0
	};
	intro_(&front.deftty, &front.w_height, &front.w_width);

	main_loop(&front);
	outro_(&front);
}

void frontend_list_available_fonts (void)
{
	printf("%s - list of available fonts\n", PROGRAM_NAME);
	for (unsigned short i = 0; i < NO_FONTS; i++)
		printf(" * %s\n", FontNames[i]);
}

static void intro_ (struct termios *deftty, unsigned int *w_height, unsigned int *w_width)
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
	get_window_dimensions(w_height, w_width);

	printf(INTRO_ANSI);
	fflush(stdout);
}

static void outro_ (struct front *front)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &front->deftty);

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
	bool_t quit = FALSE, pause = FALSE;

	struct timeval tv;
	fd_set inset;

	printf("%d %d", front->w_height, front->w_width);

	while (!quit)
	{
		tv.tv_sec  = 1;
		tv.tv_usec = 0;

		FD_ZERO(&inset);
		FD_SET(STDIN_FILENO, &inset);

		const int ret = select(STDOUT_FILENO, &inset, NULL, NULL, &tv);
		if (ret == -1 && Resize)
		{
			get_window_dimensions(&front->w_height, &front->w_width);
			printf("\x1b[2J\x1b[H%d %d", front->w_height, front->w_width);
			fflush(stdout);
			Resize = FALSE;
			continue;
		}

		if (FD_ISSET(STDIN_FILENO, &inset))
		{
			switch (fgetc(stdin))
			{
				case 'q': quit  = TRUE;   break;
				case ' ': pause = !pause; break;
				case '+': break;
				case 'L': break;
			}

			continue;
		}
		printf("\x1b[2J\x1b[Hnormal...");
		fflush(stdout);
	}
}
