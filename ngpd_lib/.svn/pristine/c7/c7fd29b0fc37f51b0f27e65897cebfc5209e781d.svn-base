/*

	PCI Histogramming memory  Board Test Program
	By William Helsby

	Command line arguments for look-up test program. Uses the separate 'args'
	module.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <sys/types.h>
#include <os9types.h>
#include "errors.h"
#include "args.h"

#include "ngpd.h"
#include "ngpdtest.h"

int nr_mandatory_arguments = 0;
const char *author = "W.I. Helsby";
const char *title = "Neutron Gamma Discriminator Test Program";
int num_devices;


/*----- private function prototypes -----*/

static int set_card (char *), set_num_cards (char *), set_loops (char *), set_logfile (char *), set_reporting (char *);

/*----- set functions for option flags -----*/

#define SET(attr,code) 	static int set_##attr (char *s) { if (!s) code else return 1; return 0; }
#define SETVAL(attr,var) 	static int set_##attr (char *s) { if (!s) return 1; else sscanf (s, "%d", &(var)); return 0; }
#define SETSTR(attr,var) 	static int set_##attr (char *s) { if (!s) return 1; else strcpy(var, s); return 0; }
#define SETDVAL(attr,var) 	static int set_##attr (char *s) { if (!s) return 1; else sscanf (s, "%g", &(var)); return 0; }

SET (all, test_regs = test_bram = 0xFFFFFFFF;)

	SET (regs, test_regs = 0xFFFFFFFF;)
		SET (regs_chan, test_regs |= TEST_REGS_CHAN_REGS;)
		SET (regs_glob, test_regs |= TEST_REGS_GLOB_REGS;)
		

	SET (bram, test_bram = 0xFFFFFFFF;)
		SET (bram_width, 	test_bram |= TEST_BRAM_WIDTH;)
		SET (bram_memory,	test_bram |= TEST_BRAM_MEMORY;)

	SET (dma, test_dma= ~(TEST_DMA_DISPLAY | TEST_DMA_READ_SIZE);)
		SET (dma_upload_short, test_dma |= TEST_DMA_UPLOAD_SHORT;)
		SET (dma_upload_full, test_dma |= TEST_DMA_UPLOAD_FULL;)

	SET (dma_display, test_dma |= TEST_DMA_DISPLAY;)
	SET (dma_read_size, test_dma |= TEST_DMA_READ_SIZE;)
	SET (dma_run_playback, test_dma |= TEST_DMA_RUN_PLAYBACK;)
	SET (dma_read_scope, test_dma |= TEST_DMA_READ_SCOPE;)
	SET (dma_read_scope_tp, test_dma |= TEST_DMA_READ_SCOPE_TP;)
	SET (dma_read_scope_tp_nc, test_dma |= TEST_DMA_READ_SCOPE_TP_NO_CHUNK;)
	SET (dma_read_scope_tp_fc, test_dma |= TEST_DMA_READ_SCOPE_TP_FLOW_CONT;)
		
	SET (dma_scope_tp,		test_dma |= TEST_DMA_SCOPE_TP;)
	SET(manual, manual_test = 1;)
	SET(count, dont_print_errors=1;)
	SET(2s, test_2stream = 1;)
/*	SET(gen_scope_inp, test_gen_scope = TEST_GEN_SCOPE_INP;)
	SET(gen_scope_all_dig, test_gen_scope = TEST_GEN_SCOPE_ALL_DIG;)
	SET(gen_scope_mixed, test_gen_scope = TEST_GEN_SCOPE_MIXED;)
*/
SET (noexit, exit_on_error = 0;)
SET (verbose, reporting = 2;)
SET (quiet, reporting = 0;)
SETVAL(loop_reset, loop_reset)
SETVAL(scope_out_skip, scope_out_skip)

/*----- list of options and arguments for 'args' module -----*/

optlist_t optlist[] =
{
/*-- Switch on all the available tests --*/

{	"all", 'a',			"Perform all the tests",				set_all },

/*-- Register tests --*/

{	"registers", 'r',	"Test all the registers",								set_regs },
{	"regs-chan", 0,		".. Test channel registers",							set_regs_chan },
{	"regs-glob", 0,		".. Test global registers",								set_regs_glob },

/*-- BRAM read/write tests ----*/
{	"bram",		0,		"Test Read/Write of BRAMs", 							set_bram },
{	"bram-width",	0,	".. Test Read/Write of BRAMs Data Width",				set_bram_width },
{	"bram-memory",	0,	".. Test Read/Write of all BRAMs", 						set_bram_memory },
{	"dma",			0,	"Test all DMA functions",								set_dma },
{	"upload-short",	0,	".. Test DMA upload of short burst",					set_dma_upload_short },
{	"upload-full",	0,	".. Test DMA upload of full  buffer",					set_dma_upload_full },
{	"dma-disp",		0,	"Display blocks of DRAM",								set_dma_display },
{	"dma-read-size",0,	"Try reading different sized blocks",					set_dma_read_size },
{	"run-playback",	0,	"run playback DMA",										set_dma_run_playback },
{	"read-scope",	0,	"Read Scope mode data",									set_dma_read_scope },
{	"read-scope-tp",0,	"Read Scope mode data in test pattern Chunking (default)",			set_dma_read_scope_tp },
{	"read-scope-tp-nc",0,	"Read Scope mode data in test pattern mode, no chunking",			set_dma_read_scope_tp_nc },
{	"read-scope-tp-fc",0,	"Read Scope mode data in test pattern mode, pause flow control",			set_dma_read_scope_tp_fc },
{	"scope-out-skip", 0, "Override defualt scope out skip value",				set_scope_out_skip},
{	"scope-tp",		0,	"Run scope mode taking data from TP (when present)",	set_dma_scope_tp},
{ 	"2s",			0, "Enable 2 stream output of scope mode data",				set_2s},
/*-- Miscellaneous features --*/
{	"manual", 0, 		"Prompt 'Press RETURN' at various points", set_manual},
{	"count", 0, 		"Do not print errors, just count them", set_count},
{	"loop", 'l',		"Perform all the tests several times",	set_loops },
{	"loop-reset",   0,	"Loop tests including open/close to force reset on MIDAS", set_loop_reset},
{	"noexit", 'n',		"Don't exit if there is an error",		set_noexit },
{	"reporting", 'p',	"Report level, quiet, normal, verbose, debugging", set_reporting },
{	"verbose", 'v',		"Provide verbose messages",				set_verbose },
{	"quiet", 'q',		"Provide minimal messages",				set_quiet },
{	"logfile", 'f',		"Log errors to file (implies -noexit)",	set_logfile },
{	NULL, 0,			NULL,									NULL }
};

arglist_t arglist[] =
{
{	"first-card",		"Card index (default 0) of first card to test",			set_card },
{	"num-cards",		"Number of cards in the systemt",						set_num_cards },
{	NULL,				NULL,									NULL }
};

/*----- initialization function (called by 'args') -----*/

void
init_arguments (void)
{
	/* do nothing */
}

static int
set_loops (char *str)
{
	if (str == NULL)
		nr_loops = 0;		/* loop indefinitely */
	else
		nr_loops = atoi (str);
	return 0;
}


static int
set_logfile (char *str)
{
	FILE *fp;

	if (str == NULL) { fprintf (stderr, "%s: Please supply the name of a log-file.\n", progname); return 1; }
	if (logfile)
		fclose (logfile);

	fp = fopen (str, "w");
	if (fp != NULL)
	{
		logfile = fp;
		exit_on_error = 0;
		return 0;
	}
	else
	{
		fprintf (stderr, "%s: Couldn't open %s.\n", progname, str);
		return 1;
	}
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
		reporting = 0;
	else if (strcmp (str, "normal") == 0)
		reporting = 1;
	else if (strcmp (str, "verbose") == 0)
		reporting = 2;
	else if (strcmp (str, "debugging") == 0)
		reporting = 3;
	else if (isdigit (str[0]) && str[1] == '\0')
		reporting = str[0] - '0';
	else
	{
		fprintf (stderr, "%s: Reporting level can be set to 0..3 or `quiet', `normal',\n", progname);
		fprintf (stderr, "    `verbose' or `debugging' for minimal -> maximal messages.\n");
		return 1;
	}
	return 0;
}

static int
set_card (char *str)
{
	first_card=strtol(str, NULL, 0);
	return 0;
}

static int
set_num_cards (char *str)
{
	num_cards=strtol(str, NULL, 0);
	return 0;
}





