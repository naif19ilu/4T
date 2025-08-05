#include "front.h"
#include "common.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

#define INTRO_ANSI "\x1b[?1049h\x1b[?25l\x1b[H"
#define OUTRO_ANSI "\x1b[?1049l\x1b[?25h"


struct front
{
	struct termios deftty;
};

struct font
{
};

static volatile sig_atomic_t Resize = FALSE;

static void intro_ (struct front*);
static void outro_ (struct front*);

static void signal_handler (int);

void frontend_execute (const char *taskname, const char *fontname, const int time)
{
	struct front front;
	intro_(&front);

	struct timeval tv;
	fd_set inset;

	bool_t quit = FALSE;

	static const char *const a[] =
	{
		"waiting.  ",
		"waiting.. ",
	};

	int a_ = 1;

	while (!quit)
	{
		tv.tv_sec  = 1;
		tv.tv_usec = 0;

		FD_ZERO(&inset);
		FD_SET(STDIN_FILENO, &inset);

		const int ret = select(STDOUT_FILENO, &inset, NULL, NULL, &tv);
		if (ret == -1 && Resize)
		{
			printf("needs to resize\n\r");
			Resize = FALSE;
			fflush(stdout);
			continue;
		}

		if (FD_ISSET(STDIN_FILENO, &inset))
		{
			const char key = fgetc(stdin);

			printf("\x1b[2J\x1b[Hpressed: %c\n", key);
			if (key == 'q') quit = TRUE;
		}
		else
		{
			printf("%s %d\n\r", a[(a_ % 3) - 1], a_);
			a_++;
			if (a_==3) a_=1;
		}
		fflush(stdout);
	}

	outro_(&front);
}

static void intro_ (struct front *front)
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
	(void) s;
	Resize = TRUE;
}
