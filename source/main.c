#include "cxa.h"
#include <stdio.h>
#include <string.h>

#define DESC   "4T [-t <task>] [flags]"

struct FT
{
	struct { char *task, *font, *info; } args;
};

int main (int argc, char **argv)
{
	struct FT ft;
	memset(&ft, 0, sizeof(ft));

	struct CxaFlag flags[] =
	{
		CXA_SET_STR("task", "task you'll be working on",      &ft.args.task, CXA_FLAG_TAKER_YES, 't'),
		CXA_SET_STR("font", "font to display time left",      &ft.args.font, CXA_FLAG_TAKER_MAY, 'f'),
		CXA_SET_STR("info", "display info about a task",      &ft.args.task, CXA_FLAG_TAKER_MAY, 'i'),
		CXA_SET_STR("prev", "preview font (needs font name)", &ft.args.font, CXA_FLAG_TAKER_YES, 'p'),
		CXA_SET_CHR("help", "display this message :)",        NULL,          CXA_FLAG_TAKER_NON, 'h'),
		CXA_SET_END
	};

	cxa_clean(cxa_execute((unsigned char) argc, argv, flags, "4T"));
	if ((ft.args.task == NULL) || (flags[4].meta & CXA_FLAG_SEEN_MASK))
	{
		cxa_print_usage(DESC, flags);
	}
	return 0;
}
