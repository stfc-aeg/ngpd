/*
	Argument processing software.
	By Richard W.M. Jones.
	Command-line argument processing. This file is fairly generic and might be
	used elsewhere. It removes all the known command line options (quits with
	an error if it finds something it doesn't recognize), then processes the
	remaining 'ordinary' arguments separately. You have to supply several
	functions and an array.
*/

#ifndef _ARGS_H_
#define _ARGS_H_

/*----- types for the array of options and the array of other arguments -----*/

/* Notes: Call-back functions are passed NULL or a string if they were options
 * invoked with '-option=...' where the string is '...'. For arguments, the
 * call-back is passed the argument value, which is always non-NULL. If there
 * is an error in the call-back, print something on stderr and return >= 1,
 * else return 0. The help text is limited to 50 characters (any more will
 * be truncated). Don't put a \n at the end.
 */

typedef struct { const char *name, alias, *help; int (*call)(char *); } optlist_t;
typedef struct { const char *name, *help; int (*call)(char *); } arglist_t;

/*----- the user supplies these variables: -----*/
#ifdef __cplusplus
extern "C" {
#endif

extern optlist_t optlist[];
extern arglist_t arglist[];
extern int nr_mandatory_arguments;
extern const char *author, *title;

/*----- the user supplies this initialization function: -----*/

extern void init_arguments (void);

/*----- main must call this near the beginning: -----*/

void do_args (int argc, char *argv[]);
#ifdef __cplusplus
}
#endif

/*----- flags -----*/

#define ARGS_DOES_REP	0		/* if set, args writes 'rep' to repeat cmd */

#endif /* _ARGS_H_ */
