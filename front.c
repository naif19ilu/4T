#include "front.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

#define INTRO_ANSI "\x1b[?1049h\x1b[?25l\x1b[H"
#define OUTRO_ANSI "\x1b[?1049l\x1b[?25h"

#define TRUE       1
#define FALSE      0

struct front
{
	struct termios deftty;
};

struct font
{
};

static volatile sig_atomic_t Quit = FALSE, Resize = FALSE;

static void intro_ (struct front*);
static void outro_ (struct front*);

static void signal_handler (int);

void frontend_execute (const char *taskname, const char *fontname, const int time)
{
	struct front front;
	intro_(&front);

	while (!Quit)
	{
		char pressed;
		read(STDIN_FILENO, &pressed, 1);

		if (pressed == 'q') { break; }

		printf("pressed: %c %d\n", pressed, pressed);
		fflush(stdout);
	}

	outro_(&front);
}

static void intro_ (struct front *front)
{
	/* Signal hanlder
	 * - In case of abortion attempt
	 *   the program will try to save
	 *   progress made so far
	 *
	 * - In case of terminal resizing
	 *   the program will redraw everything
	 *   and will try to continue
	 *
	 * - CTRL-Z is ignored
	 */
	struct sigaction s;
	s.sa_handler = signal_handler;
	sigemptyset(&s.sa_mask);

	sigaction(SIGINT,   &s, NULL);
	sigaction(SIGQUIT,  &s, NULL);
	sigaction(SIGHUP,   &s, NULL);
	sigaction(SIGWINCH, &s, NULL);

	sigset_t block;
	sigemptyset(&block);

	sigaddset(&block, SIGTSTP);
	sigprocmask(SIG_BLOCK, &block, NULL);

	/* Terminal configuration
	 *  - do not deal with CTRL-S/CTRL-Q (full buffers)
	 *  - no string post processing
	 *  - no displaying characters & provide chars as they're typed
	 *  - only read one characters to be read
	 */
	tcgetattr(STDIN_FILENO, &front->deftty);
	struct termios custty = front->deftty;

	custty.c_iflag &= ~(IXON | IXOFF);
	custty.c_oflag &= ~(OPOST);
	custty.c_lflag &= ~(ICANON | ECHO);
	custty.c_cc[VMIN] = 1;

	tcsetattr(STDIN_FILENO, TCSANOW, &custty);

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
	switch (s)
	{
		case SIGINT  :
		case SIGQUIT :
		case SIGHUP  : { Quit   = TRUE; break; }
		case SIGWINCH: { Resize = TRUE; break; }
	}
}
