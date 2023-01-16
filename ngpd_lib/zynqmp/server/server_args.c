/*

	XSPRESS4 Zync Command Server
	By William Helsby

	Command line arguments. Uses the separate 'args' module.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <sys/types.h>
//#include <os9types.h>
#include "errors.h"
#include "args.h"
#include "commandProcessor.h"

int nr_mandatory_arguments = 0;
char *author = "W.I. Helsby";
char *title = "Xspress4 Zync Server";
extern char device_name[][80];
extern int num_devices;
int num_devices;
char device_name[20][80];
extern int dev_board;
int first_card;
/*----- private function prototypes -----*/

static int set_dev (char *), set_reporting (char *);

/*----- set functions for option flags -----*/

#define SET(attr,code) 	static int set_##attr (char *s) { if (!s) code else return 1; return 0; }
#define SETVAL(attr,var) 	static int set_##attr (char *s) { if (!s) return 1; else sscanf (s, "%d", &(var)); return 0; }
#define SETSTR(attr,var) 	static int set_##attr (char *s) { if (!s) return 1; else strcpy(var, s); return 0; }
#define SETDVAL(attr,var) 	static int set_##attr (char *s) { if (!s) return 1; else sscanf (s, "%g", &(var)); return 0; }

SET (quiet, msg_level = MSG_QUIET;)
SET (sparse, msg_level = MSG_SPARSE;)
SET (normal, msg_level = MSG_NORMAL;)
SET (verbose, msg_level = MSG_VERBOSE;)
SET(dev_board, dev_board=1;)
/*----- list of options and arguments for 'args' module -----*/

optlist_t optlist[] =
{
{	"reporting", 'p',	"Report level, quiet, spares, normal, verbose", set_reporting },
{	"quiet", 'q',		"Provide minimal messages",				set_quiet },
{	"sparse", 's',		"Provide verbose messages",				set_sparse },
{	"normal", 'n',		"Provide verbose messages",				set_normal },
{	"verbose", 'v',		"Provide verbose messages",				set_verbose },
{	"dev-board", 0,		"Dev Board, No I2C etc",				set_dev_board },
{	NULL, 0,			NULL,									NULL }
};

arglist_t arglist[] =
{
{	"first-card",		"Card index (default 0) of first card to test",			set_dev },
{	NULL,				NULL,									NULL }
};

/*----- initialization function (called by 'args') -----*/

void
init_arguments (void)
{
	/* do nothing */
}

static int
set_reporting (char *str)
{
	if (str == NULL)
	{
		fprintf (stderr, "%s: Use `-reporting=?' for help.\n", progname);
		return 1;
	}
	if (strcmp (str, "quiet") == 0)
		msg_level = MSG_QUIET;
	else if (strcmp (str, "sparse") == 0)
		msg_level = MSG_SPARSE;
	else if (strcmp (str, "normal") == 0)
		msg_level = MSG_NORMAL;
	else if (strcmp (str, "verbose") == 0)
		msg_level = MSG_VERBOSE;
	else if (isdigit (str[0]) && str[1] == '\0')
		msg_level = str[0] - '0';
	else
	{
		fprintf (stderr, "%s: Reporting level can be set to 0..3 or 'quiet', 'spares', `normal',\n", progname);
		fprintf (stderr, "    'verbose' for minimal -> maximal messages.\n");
		return 1;
	}
	return 0;
}

static int
set_dev (char *str)
{
	first_card=strtol(str, NULL, 0);
	return 0;
}








