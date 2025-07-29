#include "cxa.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#define MAX(a, b)   ((a) > (b) ? (a) : (b))

/* Refers to the name of the major project which is making use of the
 * library, set on 'cxa_execute' function
 */
static char *Project = "unnamed";

/* Refers to the last flag seen as long as the flag needs an argument
 * even if it is optional
 */
static struct CxaFlag *LastSeen = NULL;

/* 0. Stores the index where the flag with certain shortname can be found within 'flags' array
 * 1. Stores the longname length
 */
static short QuickInf[26 * 2 + 10][2];

static char *get_name_of_argtype (const CxaFlagMeta meta)
{
	if ((meta & CXA_FLAG_TAKER_MASK) == CXA_FLAG_ARG_GIVEN_NON)
	{
		return "";
	}
	switch ((meta & CXA_ARG_TYPE_MASK))
	{
		case CXA_FLAG_ARG_TYPE_STR: return "-string";
		case CXA_FLAG_ARG_TYPE_CHR: return "-character";
		case CXA_FLAG_ARG_TYPE_SHT: return "-short";
		case CXA_FLAG_ARG_TYPE_INT: return "-int";
		case CXA_FLAG_ARG_TYPE_LNG: return "-long";
		case CXA_FLAG_ARG_TYPE_DBL: return "-double";
	}
	return NULL;
}

static void error_undefined_shortname (const char name, const char *group)
{
	const char *const fmt =
	"cxa:%s:\x1b[31merror:\x1b[0m undefined flag provided\n"
	"   '-%c' cannot be recognized as a program's argument\n"
	"   found in '%s' group\n";
	fprintf(stderr, fmt, Project, name, group);
	exit(EXIT_FAILURE);
}

static void error_multi_taker_in_group (const char first, const char second, const char *group)
{
	const char *const fmt =
	"cxa:%s:\x1b[31merror:\x1b[0m more than one flag takes argument in this group\n"
	"   both '-%c' and '-%c' take argument (found in '%s' group)\n"
	"   fix: ... -%c <arg> -%c <arg>\n";
	fprintf(stderr, fmt, Project, first, second, group, first, second);
	exit(EXIT_FAILURE);
}

static void error_undefined_longname (const char *name, const size_t length)
{
	const char *const fmt =
	"cxa:%s:\x1b[31merror:\x1b[0m undefined flag provided\n"
	"   '--%.*s' cannot be recognized as a program's argument\n";
	fprintf(stderr, fmt, Project, (int) length, name);
	exit(EXIT_FAILURE);
}

static void error_missing_argument (const char *longname, const char shortname, const CxaFlagMeta meta)
{
	const char *const fmt =
	"cxa:%s:\x1b[31merror:\x1b[0m missing argument\n"
	"   '--%s' (%c) is missing its argument of type <%s>\n";
	fprintf(stderr, fmt, Project, longname, shortname, get_name_of_argtype(meta) + 1);
	exit(EXIT_FAILURE);
}

static void check_names (struct CxaFlag*);
static short get_quick_access_for (const char);

static void handle_short_flag (struct CxaFlag*, const char*, const size_t);
static void handle_long_flag (struct CxaFlag*, const char*);

static void check_flag_has_its_arg (void);
static void handle_freeword (const char*, struct Cxa*);

static void store_positional_argument (struct Cxa*, const char*);

struct Cxa *cxa_execute (const unsigned char argc, char **argv, struct CxaFlag *flags, const char *projectName)
{
	struct Cxa *cxa = (struct Cxa*) calloc(1, sizeof(struct Cxa));
	cxa->positional = (char**)      calloc(CXA_POS_ARGS_GROWTH_FAC, sizeof(char*));
	cxa->len        = 0;
	cxa->cap        = CXA_POS_ARGS_GROWTH_FAC;

	assert(cxa && "CANNOT ALLOC");
	assert(cxa->positional && "CANNOT ALLOC");

	Project = (char*) projectName;
	check_names(flags);

	bool endOfArgs = false;

	for (unsigned char i = 1; i < argc; i++)
	{
		const char *this = argv[i];
		const size_t len = strlen(this);

		if (endOfArgs)
		{
			handle_freeword(this, cxa);
			continue;
		}
		else if (len == 2 && *this == '-' && this[1] == '-')
		{
			endOfArgs = true;
			continue;
		}
		else if (len >  2 && *this == '-' && this[1] == '-')
		{
			check_flag_has_its_arg();
			handle_long_flag(flags, this + 2);
		}
		else if (len >  1 && *this == '-')
		{
			check_flag_has_its_arg();
			handle_short_flag(flags, this, len);
		}
		else
		{
			handle_freeword(this, cxa);
		}
	}

	check_flag_has_its_arg();
	return cxa;
}

void cxa_print_usage (const char *desc, const struct CxaFlag *flags)
{
	printf("\n\x1b[1mUsage\x1b[0m: %s - %s %s\n", Project, __DATE__, __TIME__);
	printf("%s\n\n", desc);
	printf("flags:\n");

	unsigned short largestname = 0;
	unsigned short largestdesc = 0;

	for (unsigned int i = 0; flags[i].longname; i++)
	{
		largestname = MAX(largestname, strlen(flags[i].longname));
		largestdesc = MAX(largestdesc, strlen(flags[i].description));
	}

	largestname += 2;
	largestdesc += 2;

	static const char *const argstr[] =
	{
		"NON",
		"MAY",
		"YES"
	};

	for (unsigned int i = 0; flags[i].longname; i++)
	{
		const CxaFlagMeta meta = flags[i].meta;

		printf(
			"  \x1b[2m-\x1b[0m%c or \x1b[2m--\x1b[0m%-*s%-*s(arg? %s%s)\n",
			flags[i].shortname, largestname, flags[i].longname,
			largestdesc, flags[i].description, argstr[meta & CXA_FLAG_TAKER_MASK],
			get_name_of_argtype(meta)
		);
	}

	putchar(10);
}

void cxa_clean (struct Cxa *cxa)
{
	free(cxa->positional);
	free(cxa);
}

static void check_names (struct CxaFlag *flags)
{
	for (unsigned short i = 0; i < 62; i++)
	{
		QuickInf[i][0] = -1;
	}
	for (unsigned short i = 0; flags[i].longname; i++)
	{
		const char shortname = flags[i].shortname;
		const short key      = get_quick_access_for(shortname);
		
		assert(key != -1 && "PROGRAMMER: INVALID SHORTNAME");
		assert(QuickInf[key][0] == -1 && "PROGRAMMER: REPEATED SHORTNAMES");

		const char *longname = flags[i].longname;

		QuickInf[key][0] = i;
		QuickInf[key][1] = (short) strlen(longname);

		for (unsigned short j = i + 1; flags[j].longname; j++)
		{
			assert(strcmp(longname, flags[j].longname) != 0 && "PROGRAMMER: REPEATED LONGNAMES");
		}
	}
}

static short get_quick_access_for (const char a)
{
	if (isdigit(a)) { return a - '0'; }
	if (islower(a)) { return a - 'a' + 10; }
	if (isupper(a)) { return a - 'A' + 36; }

	return -1;
}

static void handle_short_flag (struct CxaFlag *flags, const char *given, const size_t len)
{
	for (size_t i = 1; i < len; i++)
	{
		const char name = given[i];
		const short key = get_quick_access_for(name);

		if (key == -1 || QuickInf[key][0] == -1)
		{
			error_undefined_shortname(name, given);
		}

		struct CxaFlag *flag = &flags[QuickInf[key][0]];
		flag->meta |= CXA_FLAG_WAS_SEEN;

		const bool takesArg = (flag->meta & CXA_FLAG_TAKER_MASK);

		if (takesArg != CXA_FLAG_TAKER_NON && LastSeen)
		{
			error_multi_taker_in_group(LastSeen->shortname, name, given);
		}
		if (takesArg != CXA_FLAG_TAKER_NON)
		{
			LastSeen = flag;
		}
	}
}

static void handle_long_flag (struct CxaFlag *flags, const char *given)
{
	char *followedArgument = NULL;
	size_t length = 0;

	for (; given[length]; length++)
	{
		if (given[length] == '=') { followedArgument = ((char*) given) + length + 1; break; }
	}

	for (unsigned short i = 0; flags[i].longname; i++)
	{
		const size_t defFlagLen = (size_t) QuickInf[get_quick_access_for(flags[i].shortname)][1];

		if (length == defFlagLen && !strncmp(given, flags[i].longname, length))
		{
			flags[i].meta |= CXA_FLAG_WAS_SEEN;

			if ((flags[i].meta & CXA_FLAG_TAKER_MASK) != CXA_FLAG_ARG_GIVEN_NON)
			{
				LastSeen = &flags[i];
			}
			if (LastSeen && followedArgument)
			{
				handle_freeword(followedArgument, NULL);
			}
			return;
		}
	}
	error_undefined_longname(given, length);
}

static void check_flag_has_its_arg (void)
{
	if (LastSeen && (LastSeen->meta & CXA_FLAG_ARG_GIVEN_MASK) == CXA_FLAG_ARG_GIVEN_YES)
	{
		error_missing_argument(LastSeen->longname, LastSeen->shortname, LastSeen->meta);
	}
	LastSeen = NULL;
}

static void handle_freeword (const char *word, struct Cxa *cxa)
{
	if (LastSeen == NULL)
	{
		store_positional_argument(cxa, word);
		return;
	}

	assert(LastSeen->destination != NULL && "NO DESTINATION ASSIGNED FOR A FLAG");
	errno = 0;

	switch (LastSeen->meta & CXA_ARG_TYPE_MASK)
	{
		case CXA_FLAG_ARG_TYPE_STR: *(char**)  LastSeen->destination = (char*)  word;                  break;
		case CXA_FLAG_ARG_TYPE_CHR: *(char*)   LastSeen->destination = (char)   *word;                 break;
		case CXA_FLAG_ARG_TYPE_SHT: *(short*)  LastSeen->destination = (short)  strtol(word, NULL, 0); break;
		case CXA_FLAG_ARG_TYPE_INT: *(int*)    LastSeen->destination = (int)    strtol(word, NULL, 0); break;
		case CXA_FLAG_ARG_TYPE_LNG: *(long*)   LastSeen->destination = (long)   strtol(word, NULL, 0); break;
		case CXA_FLAG_ARG_TYPE_DBL: *(double*) LastSeen->destination = (double) strtod(word, NULL);    break;
	}

	if (errno != 0)
	{
		error_missing_argument(LastSeen->longname, LastSeen->shortname, LastSeen->meta);
	}

	LastSeen->meta |= CXA_FLAG_ARG_GIVEN_YES;
	LastSeen = NULL;
}

static void store_positional_argument (struct Cxa *cxa, const char *pos)
{
	if (cxa->len == cxa->cap)
	{
		cxa->cap += CXA_POS_ARGS_GROWTH_FAC;
		cxa->positional = (char**) realloc(cxa->positional, cxa->cap * sizeof(char*));
		assert(cxa->positional && "CANNOT ALLOC");
	}
	cxa->positional[cxa->len++] = (char*) pos;
}
