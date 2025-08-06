#include "cxa.h"
#include "front.h"
#include "common.h"

#include <stdio.h>
#include <string.h>

#define FLAG_TASK_DESC "task name (mandatory)"
#define FLAG_FONT_DESC "font (default: small)"
#define FLAG_TIME_DESC "work time in mins (default: 30)"
#define FLAG_LIST_DESC "list all available fonts"

#define FLAG_FONT_DEFT "small"
#define FLAG_TIME_DEFT 30
#define FLAG_TASK_DEFT ""

#define PROGRAM_USAGE  "4T --task <taskname> [flags]"

struct program
{
	struct
	{
		char *task, *font;
		int  time;
	} args;
};

static void set_default_flags (struct program*);
static void list_available_fonts (void);

int main (int argc, char **argv)
{
	struct program prg;
	set_default_flags(&prg);

	struct CxaFlag flags[] =
	{
		CXA_SET_STR("task", FLAG_TASK_DESC, &prg.args.task, CXA_FLAG_TAKER_YES, 't'),
		CXA_SET_STR("font", FLAG_FONT_DESC, &prg.args.font, CXA_FLAG_TAKER_YES, 'f'),
		CXA_SET_STR("time", FLAG_TIME_DESC, &prg.args.time, CXA_FLAG_TAKER_YES, 'T'),
		CXA_SET_CHR("list", FLAG_LIST_DESC, NULL,           CXA_FLAG_TAKER_NON, 'L'),

		CXA_SET_END
	};

	cxa_clean(cxa_execute((unsigned char) argc, argv, flags, PROGRAM_NAME));

	if (flags[3].meta & CXA_FLAG_SEEN_MASK)
	{
		frontend_list_available_fonts();
		return 0;
	}

	if ((*prg.args.task == 0) || !(flags[0].meta & CXA_FLAG_SEEN_MASK))
	{
		cxa_print_usage(PROGRAM_USAGE, flags);	
		return 0;
	}
	
	frontend_execute(prg.args.task, prg.args.font, prg.args.time);
	return 0;
}

static void set_default_flags (struct program *prg)
{
	memset(prg, 0, sizeof(*prg));
	prg->args.font = FLAG_FONT_DEFT;
	prg->args.time = FLAG_TIME_DEFT;
	prg->args.task = FLAG_TASK_DEFT;
}
