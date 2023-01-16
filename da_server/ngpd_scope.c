/*----------------------------------------------------------------------
 *	NGPD scope mode functions
 *	By William Helsby
 *----------------------------------------------------------------------
 */

#include "header.h"
#include "ngpd.h"




/*----- Private function prototypes -----*/

int ngpd_read_scope_float(int, void *, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);
int ngpd_read_scope(int, void *, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);
int ngpd_scope_unif_enable(int path);
int ngpd_scope_unif_disable(int path);
int ngpd_scope_clear(int path, unsigned x, unsigned y, unsigned t, unsigned dx, unsigned dy, unsigned dt);
int ngpd_open_scope (int quals, int cn, int path);
// int ngpd_close (int quals, int cn, int handle);
int ngpd_scope_setup0_4(int quals, int cn, int path, int card, int stream, int chan);
int ngpd_scope_setup_any(int quals, int cn, int path, int card, int stream, int chan);
int ngpd_scope_setup5(int quals, int cn, int path, int card);
int ngpd_scope_enable(int quals, int cn, int path);
int ngpd_scope_disable(int quals, int cn, int path);
int ngpd_search_scope(int quals, int cn, int path, int card, int stream, u_int32 mask, u_int32 min, u_int32 max);
int ngpd_scope_set_options_cmd(int quals, int cn, int path, int card);
int ngpd_scope_setup_all(int quals, int cn, int path, int card);

char * ngpd_scope_input(int quals, int cn, int path);
int ngpd_scope_save(int quals, int cn, int path, char *fname);


PARSE_TREE ngpd_scope_tree[] = {
	PT_COMMAND ("open","%d float = %d",
		    "Open debug Scope mode data, return unified read handle",
		    "Path Number/Make Float data with appropriate signed conversions",
		    ngpd_open_scope),
	PT_COMMAND ("set-options", "%d %d set-streams %d = %d",
		    "Setup Scope mode option",
		    "Path Number/Card (-1 for all)/Switch between 1, 2, 4 and 6 stream mode and 4, 8, 10 streams for NGZMP",
		    ngpd_scope_set_options_cmd),
	PT_COMMAND ("set-all-chan", "%d %d sync-boxes filter = %d",
			"Setup scope mode to view data from all channels",
			"Path/Card (-1 for all cards)/Synchronises boxes (where possible)/Take data from filter (default raw input from ADC)",
			ngpd_scope_setup_all ),


/*	PT_COMMAND ("set-options", "%d %d delay-start force-extra extra-0 radial sync set-streams %d = %d",
		    "Setup Scope mode option",
		    "Path Number/Card (-1 for all)/Delay start until Count Enb signal/Force extra delay on specified cards/Extra delay on card 0 only/"
		    "Use radial scope trigger for XSPRESS4 rack version/Use default synchronisation based on current sync mode/Switch between 4, 8 and 16 stream mode for XSPRESS3V7 onwards",
		    ngpd_scope_set_options),
*/
	PT_COMMAND ("setup0-4", "%d %d %d + %d test-pat inp filter dtrig-diff dtrig-out bsub-out bsub-int meas-data meas-height meas-fall-time meas-tail-sum output-height output-fall-time output-tail-count output-tail-sum "
				"dig-3bit dig-5bit src %d alt %d = %d",
			"Setup NGPD scope mode streams 0-4",
			"Path/Card (-1 for all)/Stream to configure (0..4)/Channel number/"
			"Test pattern generator/ADC input/Data output from filter/Data output from differentail trigger/Differential used in Diff trigger/Data output from baseline subtraction/Baseline value for subtraction/"
			"Measure input data/Measure output : height/Measure output : Fall time/Measure output tail sum/Data Output: Height/Data Output: Fall Time/Data Output:Tail Count/Data Output:Tail Sum/"
			"Selects 3 digital bits from each of all the 5 other streams/Selects 5 bits of digital data from first 3 streams/Specify source number/Specify Alternate number/",
			ngpd_scope_setup0_4),
	PT_COMMAND ("setup5", "%d %d dig-3bit dig-5bit src %d alt %d = %d",
			"Setup NGPD scope mode stream 5",
			"Path/Card (-1 for all)/"
			"Selects 3 digital bits from each of all the 5 other streams/Selects 5 bits of digital data from first 3 streams/Specify source number/Specify Alternate number/",
			ngpd_scope_setup5),

	PT_COMMAND ("setup-any", "%d %d %d + %d test-pat inp filter dtrig-diff dtrig-out bsub-out bsub-int meas-data meas-height meas-fall-time meas-tail-sum output-height output-fall-time output-tail-count output-tail-sum "
				"dig-3bit dig-5bit dig-4bit dig-mixed src %d alt %d = %d",
			"Setup NGPD or NGZMP scope mode streams",
			"Path/Card (-1 for all)/Stream to configure/Channel number/"
			"Test pattern generator/ADC input/Data output from filter/Data output from differentail trigger/Differential used in Diff trigger/Data output from baseline subtraction/Baseline value for subtraction/"
			"Measure input data/Measure output : height/Measure output : Fall time/Measure output tail sum/Data Output: Height/Data Output: Fall Time/Data Output:Tail Count/Data Output:Tail Sum/"
			"Selects 3 digital bits from each of all the 5 other streams (not on ZynqMP)/Selects 5 bits of digital data from first 3 streams/"
			"Selects 4 digital bit from streams 0..7, used on stream 8 and 9/Select 1 digital bit from stream 0..3, 2 from 4..8/Specify source number/Specify Alternate number/",
			ngpd_scope_setup_any),

/*
	PT_COMMAND ("enable", "%d",
			"Enable Scope mode DMA (also automatically enabled by any setup)", 
			"Path Number",
			ngpd_scope_enable),
	PT_COMMAND ("disable", "%d",
			"Disable Scope mode DMA (automatically enabled by any setup)", 
			"Path Number",
			ngpd_scope_disable),
*/
	PT_COMMAND ("search", "%d %d %d + %d %d %d bit0 not-bit0 bit1 not-bit1 bit2 not-bit2 save %s "
				"min-time-true %d max-time-true %d min-time-false %d max-time-false %d  limit %d count "
				"other-bit0 %d not-other-bit0 %d other-bit1 %d not-other-bit1 %d other-bit2 %d not-other-bit2 %d other-offset0 %d other-offset1 %d other-offset2 %d "
				"rise0 rise1 rise2 rise %d fall %d fall0 fall1 average %s dig-mask %d not-mask %d = %d",
				"Search scope mode data for condition",
				"XSPRESS3 Path number/Card/Stream (1..5)/"
				"Bitwise And Mask/Min value/Max value/"
				"Check bit 0 of associated digital  is true/Check bit 0 is false/Check bit 1 is true/Check bit 1 is false/Check bit 2 is true/Check bit 2 is false/"
				"File name to save 4096 points/"
				"Trigger when match is true for <= cycles/Trigger when match is true for >= cycles (min > max for time in window)/"
				"Trigger when match is flase for <= cycles/Trigger when match is flase for >= cycles (min > max for time in window)/Limit Number of matches/Count matches, don't print/"
				"Check bit 0 of digital data from another stream is true/Check bit 0 of digital data from another stream is false/"
				"Check bit 1 of digital data from another stream is true/Check bit 1 of digital data from another stream is false/"
				"Check bit 2 of digital data from another stream is true/Check bit 2 of digital data from another stream is false/"
				"Time Offset for other bit0/Time Offset for other bit1/Time Offset for other bit2/"
				"Rising edge on bit0/Rising edge on bit1/Rising edge on bit2/Rising edge on specified bit number 0..4/Falling edge on specifeid bit number 0..4/Falling edge on bit0/Falling edge on bit1/Average into named module/"
				"Digital mask True to allow bits 3 and 4 to be tested (X4)/Digital mask to test specified bit are FALSE to allow bits 3 and 4 to be tested (X4)/",
				ngpd_search_scope),
/*
	PT_COMMAND ("search-events", "%d %d %d + %d count count-missing = %d",
				"Search scope mode data containing event signal from trigger-B our or servo-out",
				"Path/Card/First event stream/Second event stream/Count All issues but do not print/Count but do not print missing OTD",
				ngpd_search_scope_events),
	PT_COMMAND ("search-ts", "%d %d %d %d most-neg %d ts12 = %d",
				"Search event List Time stamps to look for unexpected changes",
				"Path/Card/Stream (must be outputing Event timestamp/Most Positive time stamp difference to allow/Most negative difference to allow (default -10)/Use 12 bits of ti,me stamp (default all 14)",
				ngpd_search_scope_xtk_ts),
	PT_COMMAND("load-mod", "%d %s first-card %d num-cards %d total-cards %d first-dig mod-first-stream %d = %d",
			"Load scope mode data saved by ngpd.server into scope module",
			"System number to link or create xsp3_scope<num> (usually 0)/File name which can contain %d to be replaced by card number/"
			"First card (default 0)/Num of card files to read (default 1)/Total number of cards if creating module/First stream of file is digial data/"
			"First stream in module to load",
			ngpd_scope_load),


	PT_COMMAND ("setup-inp", "%d do-upper sync = %s",
			"Setup scope mode on all generations of XSPRESS3 onwards to record ADC data",
			"Path/Do upper channels when required (more channels than scope mode streams, e.g. 8 channel Xspress3)/Use box to box triggering to synchronise boxes",
			ngpd_scope_input),
*/
	PT_COMMAND ("setup-save", "%d %s from-mod = %d",
			"Save setup of scope mode to a script to allow reloading",
			"Path/File name for script/Take settings from module rather than hardware",
			ngpd_scope_save ),

	PT_END
};


int
ngpd_open_scope (int quals, int cn, int path)
{
	int handle = -1;
	int num_cards;
	NGPDScopeModule *mod;
	int to_float = quals & 1;
	int nstreams;
		
	if ((mod = ngpd_scope_get_mod(path)) == NULL)
		return -1;
	nstreams = ngpd_scope_mod_get_nstreams(mod);
	if (to_float)
	{
		handle = unified_open (cn, path, mod->head.num_t, nstreams, mod->head.num_cards, UNIF_DATA_FLOAT,
				"NGPD System - Scope Mode",
				ngpd_read_scope_float, NULL, NULL, NULL, NULL,
				ngpd_scope_clear,
				ngpd_scope_unif_enable,
				ngpd_scope_unif_disable);
	}
	else
	{
		handle = unified_open (cn, path, mod->head.num_t, nstreams, mod->head.num_cards, UNIF_DATA_SHORT,
				"NGPD System - Scope Mode",
				ngpd_read_scope, NULL, NULL, NULL, NULL,
				ngpd_scope_clear,
				ngpd_scope_unif_enable,
				ngpd_scope_unif_disable);
	}
	if (handle < 0)
	{
		sv_printf (cn, "! Unified open command failed.\n");
		return -1;
	}
	ngpd_unif_handle[handle].valid = 1;
	ngpd_unif_handle[handle].ngpd_path = path;
	ngpd_unif_handle[handle].type = NGPDHistType_Scope;
	return handle;
}


/*
ngpd_close (int quals, int cn, int handle)
{
	unified_close (handle);
	return 0;
}
*/
/*----- miscellaneous other functions -----*/


int
ngpd_scope_unif_enable(int sys)
{
	return 0;
}

int
ngpd_scope_unif_disable(int sys)
{
	return 0;
}
int
ngpd_scope_clear(int sys,  unsigned x, unsigned y, unsigned t,
							 unsigned dx, unsigned dy, unsigned dt)
{
	int tf_size;
	return 0;
}

int
ngpd_read_scope (int path, void *to, unsigned sx, unsigned sy, unsigned st,
	unsigned dx, unsigned dy, unsigned dt)
{
	unsigned i, y, t;
	int16_t *buffer = (int16_t *)to;
	int16_t *ptr;
	int rc = -1;
	NGPDScopeModule *mod;
	int inc;

	if ((mod = ngpd_scope_get_mod(path)) == NULL)
		return -1;


	for (t=st; t<st+dt; t++)
	{
		ngpd_scope_update_mod(path, t, sx, dx);
		for (y=sy; y<sy+dy; y++)
		{
			inc = ngpd_scope_mod_get_inc(mod, y);
			ptr = ngpd_scope_mod_get_ptr(mod, t, y);
			if (ptr == NULL)
				return -1;
			ptr += inc*sx;
			for (i=0; i<dx; i++)
			{
				*buffer++ = *ptr;
				ptr += inc;
			}
		}
	}
	return 0;
}

int
ngpd_read_scope_float (int path, void *to, unsigned sx, unsigned sy, unsigned st,
	unsigned dx, unsigned dy, unsigned dt)
{
	unsigned i, y, t;
	float *buffer = (float *)to;
	u_int16_t *uptr;
	int16_t *sptr;
	NGPDScopeStream stype;
	NGPDScopeModule *mod;
	int inc;

	if ((mod = ngpd_scope_get_mod(path)) == NULL)
		return -1;

	for (t=st; t<st+dt; t++)
	{
		ngpd_scope_update_mod(path, t, sx, dx);

		for (y=sy; y<sy+dy; y++)
		{
			uptr = ngpd_scope_mod_get_ptr(mod, t, y);
			if (uptr == NULL)
				return -1;
			if ((stype=ngpd_scope_stream_details(mod, t, y, NULL, NULL, NULL, NULL, NULL, NULL, &inc, NULL)) < 0)
				return -1;
			uptr += inc*sx;
			sptr = (int16_t *) uptr;
			if (stype == NGPDScopeStream_Signed)
			{
				for (i=0; i<dx; i++)
				{
					*buffer++ = *sptr;
					sptr += inc;
				}
			}
			else
			{
				for (i=0; i<dx; i++)
				{
					*buffer++ = *uptr;
					uptr += inc;
				}
			}
		}
	}
	return 0;
}
int ngpd_scope_set_options_cmd(int quals, int cn, int path, int card)
{
	int rc;
	NGPDScopeOptions options;
	options.flags = 0;
	options.nstreams = 0;

#if 0
	if (quals & 1)
	{
		options.flags = NGPDScopeOpt_DelayStart;
		switch (quals & 6)
		{
		case 0:	break;
		case 2: options.flags |= NGPDScopeOpt_ForceExtraDelay; break;
		case 4: options.flags |= NGPDScopeOpt_ExtraDelayOn0; break;
		default:
			sv_printf(cn, "! ERROR: Do not specify force-extra and extra-0\n");
			return -1;
		}
	}
	else
	{
		if (quals & 6)
		{
			sv_printf(cn, "! ERROR: extra delay qualifiers can only be used with delay-start\n");
			return -1;
		}
	}
	if (quals & 8)
	{
		if (quals & 7)
		{
			sv_printf(cn, "! ERROR: With radial option do not specify other delay start qualifiers\n");
			return -1;
		}
		options.flags = NGPDScopeOpt_RadialStart;
	}
	if (quals & 0x10)
	{
		if (quals & 0xF)
		{
			sv_printf(cn, "! ERROR: With synchronised option do not specify other delay start qualifiers\n");
			return -1;
		}
		options.flags = NGPDScopeOpt_Synchronised;
	}
#endif 
	if (quals & 0x1)
		options.nstreams = Qualifier[0].IntArg;

	sv_printf(cn,"# Setting scope options.flags=0x%X, options.nstreams=0x%X\n", options.flags, options.nstreams);
	if ((rc=ngpd_set_scope_options(path, card, &options)) < 0)
	{
		sv_printf(cn, "! ERROR specifying scope options : %s\n", ngpd_get_error_message());
		return rc;
	}
	return ngpd_scope_resize_read_handle(cn, path);
}

int ngpd_scope_resize_read_handle(int cn, int path)
{
	NGPDScopeModule *mod;
	int nstreams;
	int num_t;
	int ncards;
	
	mod = ngpd_scope_get_mod(path);
	if (mod == NULL)
	{
		sv_printf(cn, "! ERROR ngpd_scope_resize_read_handle: %s\n", ngpd_get_error_message());
		return -1;
	}
	
	nstreams = ngpd_scope_mod_get_nstreams(mod);
	ncards = ngpd_get_num_cards(path);
	num_t = mod->head.num_t;
	if (nstreams < 1 || ncards < 1 || num_t < 1)
	{
		sv_printf(cn, "! ERROR ngpd_scope_resize_read_handle: nstream=%d, ncards=%d, num_t=%d => %s\n", nstreams, ncards, num_t, ngpd_get_error_message());
		return -1;
	}
	return ngpd_unif_resize(cn, path, NGPDHistType_Scope, num_t, nstreams, ncards, 0);
}	
#if 0
int ngpd_scope_disable(int quals, int cn, int path)
{
	int flags;

	flags = ngpd_get_run_flags(path);
	if (flags < 0)
	{
		sv_printf(cn, "! ERROR: Cannot read run flags: %s\n", ngpd_get_error_message());
		return -1;
	}
	flags &= ~XSP3_RUN_FLAGS_SCOPE;

	return ngpd_set_run_flags(path, flags);
}	

int ngpd_scope_enable(int quals, int cn, int path)
{
	int flags;

	flags = ngpd_get_run_flags(path);
	if (flags < 0)
	{
		sv_printf(cn, "! ERROR: Cannot read run flags: %s\n", ngpd_get_error_message());
		return -1;
	}
	flags |= XSP3_RUN_FLAGS_SCOPE;

	return ngpd_set_run_flags(path, flags);
}	
#endif

#define BIT_X 0
#define BIT_0 1
#define BIT_1 2
#define BIT_R 3
#define BIT_F 4

int ngpd_search_scope(int quals, int cn, int path, int card, int stream, u_int32 mask, u_int32 min, u_int32 max)
{
	u_int16_t *ana, *dig;
	int num_x=16384;
	int i, j, t, n, fn;
	u_int16_t x, ana_masked;
	char fname[1000];
	char command[1500];
	int do_save;
	int use_mask=0;
	int bit[5]= {BIT_X, BIT_X, BIT_X, BIT_X, BIT_X };
	char *bit_names[]={"X", "0", "1", "R", "F"};
	int masked, match;
	int min_time=0, max_time=0;
	int time_trig=0;
	int timer=-1;
	int trigger;
	int match_limit = -1;
	int dont_print=0;
	NGPDScopeModule * scope_mod;
	int upath;
	int save_pre = 600;
	int save_post = 100;
	int other_stream[3];
	int other_offset[3] = {0, 0, 0};
	int other_bit_posn[3];
	u_int16_t *other_dig[3] = {NULL, NULL, NULL};
	int bit_other[3] = {BIT_X, BIT_X, BIT_X};
	MOD_IMAGE *mod=NULL;
	mh_com * mod_head;
	int mod_pre=100, mod_post=100;
	char *mod_name=NULL;
	int do_ave=0;
	int num_ave = 0;
	scope_mod = ngpd_scope_get_mod(path); 
	int ana_inc, dig_inc, other_dig_inc;
	int num_dig, bit_posn;
	int nstreams;
	int dig_mask;
	int first_card, last_card;
	int total_n = 0;

	if (scope_mod == NULL)
	{
		sv_printf(cn, "! ERROR: Scope data module not available :%s\n", ngpd_get_error_message());
		return -1;
	}

	if (TotalArgumentCount == 3)
		use_mask = 0;
	else if (TotalArgumentCount == 6)
		use_mask = 1;
	else
	{
		sv_printf(cn, "! ERROR, Please specify either all of mask, min, max or none\n");
		return -1;
	}
	if (quals & 1)
		bit[0] = BIT_1;
	if (quals & 2)
		bit[0] = BIT_0;

	if (quals & 4)
		bit[1] = BIT_1;
	if (quals & 8)
		bit[1] = BIT_0;

	if (quals & 0x10)
		bit[2] = BIT_1;
	if (quals & 0x20)
		bit[2] = BIT_0;

	if (quals & 0x40)
	{
		strcpy(fname, Qualifier[6].StringArg);
		do_save=1;
	}
	else
		do_save=0;

	if (quals & 0x80)
	{
		time_trig=1;	/* Time while trigger is true */
		min_time = Qualifier[7].IntArg;
	}
	if (quals & 0x100)
	{
		time_trig=1;	/* Time while trigger is true */
		max_time = Qualifier[8].IntArg;
	}
	if (quals & 0x200)
	{
		time_trig=2;	/* Time while trigger is flase */
		min_time = Qualifier[9].IntArg;
	}
	if (quals & 0x400)
	{
		time_trig=2;	/* Time while trigger is flase */
		max_time = Qualifier[10].IntArg;
	}
	if (quals & 0x800)
		match_limit =Qualifier[11].IntArg;

	dont_print = !!(quals & 0x1000);

	if (quals & 0x2000)
	{
		bit_other[0] = BIT_1;
		other_stream[0] = Qualifier[13].IntArg;
	}
	if (quals & 0x4000)
	{
		bit_other[0] = BIT_0;
		other_stream[0] = Qualifier[14].IntArg;
	}

	if (quals & 0x8000)
	{
		bit_other[1] = BIT_1;
		other_stream[1] = Qualifier[15].IntArg;
	}
	if (quals & 0x10000)
	{
		bit_other[1] = BIT_0;
		other_stream[1] = Qualifier[16].IntArg;
	}

	if (quals & 0x20000)
	{
		bit_other[2] = BIT_1;
		other_stream[2] = Qualifier[17].IntArg;
	}
	if (quals & 0x40000)
	{
		bit_other[2] = BIT_0;
		other_stream[2] = Qualifier[18].IntArg;
	}

	if (quals & 0x80000)
		other_offset[0] = Qualifier[19].IntArg;
	if (quals & 0x100000)
		other_offset[1] = Qualifier[20].IntArg;
	if (quals & 0x200000)
		other_stream[2] = Qualifier[21].IntArg;

	if (quals & 0x400000)
		bit[0] = BIT_R;
	if (quals & 0x800000)
		bit[1] = BIT_R;
	if (quals & 0x1000000)
		bit[2] = BIT_R;
	if (quals & 0x2000000)
	{
		i = Qualifier[25].IntArg;
		if (i <0 || i> 4)
		{
			sv_printf(cn, "! ERROR Expected rising bit number 0..4, not %d\n", i);
			return -1;
		}
		bit[i] = BIT_R;
	}
	if (quals & 0x4000000)
	{
		i = Qualifier[26].IntArg;
		if (i <0 || i> 4)
		{
			sv_printf(cn, "! ERROR Expected falling bit number 0..4, not %d\n", i);
			return -1;
		}
		bit[i] = BIT_F;
	}

	if (quals & 0x8000000)
		bit[0] = BIT_F;
	if (quals & 0x10000000)
		bit[1] = BIT_F;

	if (quals & 0x20000000)
	{
		mod_name = Qualifier[29].StringArg;
		do_ave = 1;
	}
	if (quals & 0x40000000)
	{
		dig_mask = Qualifier[30].IntArg;
		for (i=0;i<5; i++)
		{
			if (dig_mask & (1 << i))
			{
				printf("Checking bit %d = 1\n", i);
				bit[i] = BIT_1;
			}
		}
	}
	if (quals & 0x80000000)
	{
		dig_mask = Qualifier[31].IntArg;
		for (i=0;i<5; i++)
		{
			if (dig_mask & (1 << i))
			{
				printf("Checking bit %d = 1\n", i);
				bit[i] = BIT_0;
			}
		}
	}
	if (card < 0)
	{
		first_card = 0;
		last_card = ngpd_get_num_cards(path)-1;
	}
	else
	{
		first_card = card;
		last_card = card;
	}
	fn = 0;
	for (card=first_card; card<= last_card; card++)
	{
		ana = ngpd_scope_mod_get_ptr(scope_mod, card, stream);
		nstreams = ngpd_scope_mod_get_nstreams(scope_mod);

		if (do_ave)
		{
			mod = app_mkmod_head (cn, mod_name, mod_pre+mod_post, nstreams, "Time", "stream", 2, &mod_head);
			if (mod == NULL)
				return -1; 
			clear_mod(mod);
		}

		if (ngpd_scope_stream_details(scope_mod, card, stream, &num_dig, &bit_posn, &dig, NULL, NULL, NULL, &ana_inc, &dig_inc) < 0)
		{
			sv_printf(cn, "ERROR: Cannot find info about requested stream and card: %s\n", ngpd_get_error_message());
			return -1;
		}

		if (dig == NULL || ana == NULL)
		{
			sv_printf(cn, "# ERROR determining data pointers: %s\n", ngpd_get_error_message());
			return -1;
		}
		for (j=0; j<5; j++)
		{
			if (bit[j] != BIT_X)
			{
				if (j >= num_dig)
					sv_printf(cn, "# WARNING Digital bit %d is not available, only %d bits are avaliable, are you sure?\n", j, num_dig);
			}
		}

		if (do_save)
		{
			upath = ngpd_open_scope (1, cn, path);
		}

		for (j=0; j<3; j++)
		{
			if (bit_other[j] != BIT_X)
			{
				if (ngpd_scope_stream_details(scope_mod, card, other_stream[j], &num_dig, other_bit_posn+j, other_dig+j, NULL, NULL, NULL, NULL, &other_dig_inc) < 0)
				{
					sv_printf(cn, "ERROR: Cannot find info about other %d stream and card: %s\n", j, ngpd_get_error_message());
					return -1;
				}
				if (j >= num_dig)
					sv_printf(cn, "# WARNING Digital bit %d is not available on other %d, stream %d, only %d bits are avaliable, are you sure?\n", j, j, other_stream[j], num_dig);
				sv_printf(cn, "# Additional test for stream %d, base bit posn=%d, bit offset=%d, value=%s, time offset=%d\n", other_stream[j], other_bit_posn[j], j, bit_names[bit_other[j]], other_offset[j]);
			}
		}
		sv_printf(cn, "#\tNum\tPosn\tRow\tCol\n");
		n = 0;
		
		ngpd_scope_update_mod(path, card, 0, -1);
		for (i=0; i<scope_mod->head.num_t; i++)
		{
			match = 1;
			ana_masked = ana[ana_inc*i] & mask;
			if (use_mask && (ana_masked < min || ana_masked > max))
				match = 0;
			x = dig[dig_inc*i] >> bit_posn;
			for (j=0; j<5; j++)
			{
				if (bit[j] != BIT_X)
				{
					if (bit[j] == BIT_1 && (x & (1 << j)) == 0)
						match = 0;
					if (bit[j] == BIT_0 && (x & (1 << j)))
						match = 0;
					if (i > 0 && bit[j] == BIT_R && !((x & (1 << j)) && !(dig[dig_inc*(i-1)] & (1 << (bit_posn+j)) )))
						match = 0;
					if (i > 0 && bit[j] == BIT_F && !(!(x & (1 << j)) && (dig[dig_inc*(i-1)] & (1 << (bit_posn+j)) )))
						match = 0;
				}
			}
			for (j=0; j<3; j++)
			{
				if (bit_other[j] != BIT_X)
				{
					if (i+other_offset[j] >= 0 && i+other_offset[j] < scope_mod->head.num_t)
					{
						x = other_dig[j][other_dig_inc*(i+other_offset[j])];
						if (bit_other[j] == BIT_1 &&  (x & (1 << (other_bit_posn[j]+j))) == 0)
							match = 0;
						if (bit_other[j] == BIT_0 && (x & (1 << (other_bit_posn[j]+j))))
							match = 0;
					}
				}
			}

			if (time_trig)
			{
				trigger = 0;
				/* Run timer while true */
				if (match && time_trig ==1 || !match && time_trig == 2)
				{
					if (timer == -1)
						timer = 1;
					else
						timer++;
				}
				else
				{
					if (timer > 0)
					{
						if (min_time && max_time && min_time > max_time)
						{
							/* Window comparison */
							if (timer >= max_time && timer <= min_time)
								trigger = 1;
						}
						else
						{
							if (min_time && timer <= min_time || max_time && timer >= max_time)
							{
	/*								sv_printf(cn, "# Trigger timer=%d, min_time=%d, max_time=%d\n", timer, min_time, max_time); */
								trigger = 1;
							}
						}
					}
					timer = -1;
				}
			}
			else
				trigger = match;

			if (trigger)
			{
				if (!dont_print)
				{
					if (use_mask)
						sv_printf(cn, "#\t%d\t%d\t%d\t%d\t%d\n", n, i, i/num_x, i%num_x, ana_masked);
					else
						sv_printf(cn, "#\t%d\t%d\t%d\t%d\n", n, i, i/num_x, i%num_x);
				}
				if (do_save && i > save_pre && i < scope_mod->head.num_t-save_post)
				{
					sprintf(command, "read %d %d %d %d %d 1 from %d to-local-file '%s_%03d.asc' asc-col", i-save_pre, 0, card,
									save_pre+save_post, nstreams, upath, fname, fn++);
					user_service (cn, command, 0, NULL, NULL);
				}
				n++;
				if (do_ave && i > mod_pre && i < scope_mod->head.num_t-mod_post)
				{
					double *dptr;
					u_int16_t *up;
					int16_t *sp;
					for (j=0; j<nstreams; j++)
					{
						int inc;
						dptr = (double *)id_get_ptr(mod, 0, j, 0);
						if (ngpd_scope_stream_details(scope_mod, card, j, NULL, NULL, NULL, NULL, NULL, NULL, &inc, NULL) == NGPDScopeStream_Signed)
						{
							sp = ngpd_scope_mod_get_ptr(scope_mod, card, j);
							sp += inc*(i-mod_pre);
							for (t=-mod_pre; t<mod_post; t++)
							{
								*dptr++ += *sp;
								sp += inc;
							}
						}
						else
						{
							up = ngpd_scope_mod_get_ptr(scope_mod, card, j);
							up += inc*(i-mod_pre);
							for (t=-mod_pre; t<mod_post; t++)
							{
								*dptr++ += *up;
								up += inc;
							}
						}
					}
					num_ave++;
				}
				if (match_limit> 0 && n >match_limit)
					break;
			}
			if (sv_quit)
				break;
		}
		sv_printf(cn, "# Card %d: matches=%d\n", card, n);
		total_n += n;
		if (sv_quit)
			break;
	}
	if (do_ave && num_ave > 0)
	{
		double *dptr;
		sv_printf(cn, "# Dividing %d averages\n", num_ave);
		dptr = (double *)id_get_ptr(mod, 0, 0, 0);
		for (j=0; j<nstreams*mod->head.num_x; j++)
			*dptr ++ /= num_ave;
		munlink(mod_head);
	}
	return total_n;
}

#if 0
#define DEFAULT_SIZE 67031660

int ngpd_scope_load(int quals, int cn, int sys_num, char *fname_root)
{
	int card, first_card=0, num_cards=1;
	int i, t;
	int total_cards=0;
	char fname[1024];
	char mname[100], *namep;
	int fd;
	int file_streams=-1;
	int stream;
	int first_dig;
	struct stat file_stat;
	off_t file_num_t, mod_num_t;
	int num_t;
	u_int16_t *buff=NULL, *src, *dst;
	int mod_num_streams;
	NGPDScopeModule * scope_mod=NULL;
	mh_com *mod_head;
    u_int16 attr_rev = mkattrevs(MA_REENT,1);
    u_int16 type_lang = mktypelang(MT_DATA, ML_ANY);
    error_code err;
	int mod_inc;
	int mod_first_stream=1;
	int num_streams=-1;
	char *cp;
	off_t offset, n;

	if (quals & 1)
		first_card = Qualifier[0].IntArg;

	if (quals & 2)
		num_cards = Qualifier[1].IntArg;

	if (quals & 4)
		total_cards = Qualifier[2].IntArg;

	first_dig = !!(quals & 8);

	if (first_dig)
		mod_first_stream = 0;

	if (quals & 0x10)
		mod_first_stream = Qualifier[4].IntArg;

	if (total_cards == 0)
		total_cards = first_card+num_cards;

	sprintf(mname, "ngpd_scope%d", sys_num);
	namep = mname;
   	err = _os_link(&namep, &mod_head, (void **)&scope_mod, &type_lang, &attr_rev);
	if (err != 0)
		scope_mod = NULL;
	else
	{
		if (scope_mod->head.num_cards < total_cards)
		{
			sv_printf(cn, "! ERROR: Scope module exists but with space for only %d cards, requested total_cards=%d, first=%d, num=%d\n", scope_mod->head.num_cards, total_cards, first_card, num_cards);
			munlink(mod_head);
			return -1;
		}
		mod_num_streams = ngpd_scope_mod_get_nstreams(scope_mod);
		mod_inc =  ngpd_scope_mod_get_inc(scope_mod);
		mod_num_t = scope_mod->head.num_t;
	}

	for (i=0; i<num_cards; i++)
	{
		card = i+first_card;

		sprintf(fname, fname_root, card);

		if ((fd=open(fname, O_RDONLY)) < 0)
		{
			sv_printf(cn, "# ERROR: Cannot open file '%s'\n", fname);
			return -1;
		}
		if (fstat(fd, &file_stat) < 0)
		{
			sv_printf(cn, "# ERROR: Cannot stat file '%s'\n", fname);
			return -1;
		}
		if (i == 0)
		{
			if (file_streams <= 0)
			{
				file_streams = (file_stat.st_size/2)/DEFAULT_SIZE;
				file_num_t = DEFAULT_SIZE;
			}
			else
			{
				file_num_t = file_stat.st_size/(file_streams*2);
				if (file_streams*file_num_t*2 != file_stat.st_size)
					sv_printf(cn, "# Warning : File size = %ld does NOT divide into %d streams. Are you sure?\n", file_stat.st_size, file_streams);
			}
			if (scope_mod == NULL)
			{
				scope_mod = ngpd_scope_mod_create(mname, total_cards, 3*file_num_t, &mod_head, XSP3_SCOPE_MOD_LAYOUT_2X3);
				if (scope_mod == NULL)
				{
					close(fd);
					sv_printf(cn, "! ERROR: Cannot Create scope data module '%s'\n", mname);
					return -1;
				}
				mod_num_streams = ngpd_scope_mod_get_nstreams(scope_mod);
				mod_inc =  ngpd_scope_mod_get_inc(scope_mod);
				mod_num_t = scope_mod->head.num_t;
			}
		}
		else
		{
			if (file_streams*file_num_t*2 != file_stat.st_size)
				sv_printf(cn, "# Warning : File sizes do not seem to match or size = %ld does NOT divide into %d streams. Are you sure?\n", file_stat.st_size, file_streams);
		}
		if (buff == NULL)
		{
			if ((buff=(u_int16_t *)malloc(file_num_t*sizeof(u_int16_t))) == NULL)
			{
				sv_printf(cn, "! ERROR: Cannot malloc buffer to read data\n");
				close(fd);
				return -1;
			}
		}
												
		if (first_dig == 0)
		{
			dst = ngpd_scope_mod_get_ptr(scope_mod, card, 0);
			for (t=0; t<mod_num_t;t++)
			{
				*dst = 0;
				dst += mod_inc;
			}
		}
		if (num_streams == -1)
			num_streams = first_dig?file_streams-1:file_streams;

		if (mod_first_stream < 0 || num_streams < 1 || mod_first_stream+num_streams > mod_num_streams)
		{
			sv_printf(cn, "# ERROR : first =%d + num=%d does not fit in module with %d streams\n", mod_first_stream, num_streams, mod_num_streams);
			munlink(mod_head);
			close(fd);
			return -1;
		}
		for (stream=0;stream<num_streams; stream++)
		{
			ssize_t r;
			offset = file_num_t*sizeof(u_int16_t)*stream;
			n = file_num_t*sizeof(u_int16_t);
			cp = (char *)buff;
			lseek(fd, offset, SEEK_SET);
			do
			{
				r = read(fd, cp, n);
				if (r < 0)
				{
					sv_printf(cn, "! ERROR: error reading file '%s'\n", fname);
					close(fd);
					munlink(mod_head);
					return -1;
				}
				n -= r;
				cp += r;
			} while (n > 0);

			src = buff;
			dst = ngpd_scope_mod_get_ptr(scope_mod, card, stream+mod_first_stream);
			num_t=(mod_num_t<file_num_t)?mod_num_t:file_num_t;
			sv_printf(cn, "# Writing file stream %d to module stream %d, file_num_t=%d, mod_num_t=%d, num_t=%d, mod_inc=%d\n", stream, mod_first_stream+stream, file_num_t, mod_num_t, num_t, mod_inc);
			for (t=0; t<num_t;t++)
			{
				*dst = *src++;
				dst += mod_inc;
			}
			while (t < mod_num_t)
			{
				*dst = 0;
				dst += mod_inc;
				t++;
			}
		}
		close(fd);
	}
	if (buff != NULL)
		free(buff);
		
	munlink (mod_head);
	return 0;
}
#endif


int ngpd_scope_setup0_4(int quals, int cn, int path, int card, int stream, int chan)
{
	u_int32_t src, alt=0;
	int rc;
	NGPDGeneration generation;
	NGPDScopeModule *mod;
	int to_float = quals & 1;
	int nstreams;
		

	if ((generation = ngpd_get_generation(path)) < 0)
	{
		sv_printf(cn, "! ERROR Determining ngpd generation : %s\n", ngpd_get_error_message());
		return -1;
	}

	if ((mod = ngpd_scope_get_mod(path)) == NULL)
		return -1;

	if (stream < 0 || stream > 4)
	{
		sv_printf(cn, "! ERROR: Please specify stream in range 0..4, not %d\n", stream);
		return -1;
	}
	if (mod->head.layout == NGPD_SCOPE_LAYOUT_1)
	{
		if (stream > 0)
		{
			sv_printf(cn, "! ERROR: Current module layout is for 1 streams, so stream %d is not available. Use ngpd scope set-options ... set-streams if necessary or use streams 0..0\n", stream);
			return -1;
		}
	}
	if (mod->head.layout == NGPD_SCOPE_LAYOUT_2)
	{
		if (stream >= 1)
		{
			sv_printf(cn, "! ERROR: Current module layout is for 2 streams, so stream %d is not available. Use ngpd scope set-options ... set-streams if necessary or use streams 0..1\n", stream);
			sv_printf(cn, "! ERROR: Note that in 2 stream mode output stream 1 comes from stream 5 (usually 5 bit digital but can be analog)\n");
			return -1;
		}
	}
	if (mod->head.layout == NGPD_SCOPE_LAYOUT_4)
	{
		if (stream >= 3)
		{
			sv_printf(cn, "! ERROR: Current module layout is for 4 streams, so stream %d is not available. Use ngpd scope set-options ... set-streams if necessary or use streams 0..3\n", stream);
			return -1;
		}
	}
	if (mod->head.layout == NGPD_SCOPE_LAYOUT_6)
	{
		if (stream >= 5)
		{
			sv_printf(cn, "! ERROR: Current module layout is for 6 streams, so stream %d is not available. Use ngpd scope set-options ... set-streams if necessary or use streams 0..5\n", stream);
			return -1;
		}
	}
	if (chan < 0 || chan >= NGPD_MAX_CHANS_PER_CARD)
	{
		sv_printf(cn, "! ERROR: Please specify channel in range 0..%d, not %d\n", NGPD_MAX_CHANS_PER_CARD-1, chan);
		return -1;
	}

	switch (quals & 0x3FFFF)
	{
	case       1: src = NGPD_SCOPE_SEL0TO5_TEST_PAT; break;
	case       2: src =  NGPD_SCOPE_SEL0TO5_INPUT;  break;
	case       4: src = NGPD_SCOPE_SEL0TO5_FILTER; break;
	case       8: src = NGPD_SCOPE_SEL0TO5_DIFF_TRIG_DIFF; break;
	case    0x10: src = NGPD_SCOPE_SEL0TO5_DIFF_TRIG_OUT; break;
	case    0x20: src = NGPD_SCOPE_SEL0TO5_BASELINE_SUB; break;
	case    0x40: src = NGPD_SCOPE_SEL0TO5_BSUB_INTERNAL; break;
	case    0x80: src = NGPD_SCOPE_SEL0TO5_MEASURE; alt = 0;  break;
	case   0x100: src = NGPD_SCOPE_SEL0TO5_MEASURE; alt = 2; break;
	case   0x200: src = NGPD_SCOPE_SEL0TO5_MEASURE; alt = 3; break;	
	case   0x400: src = NGPD_SCOPE_SEL0TO5_MEASURE; alt = 4; break;	
	case   0x800: src = NGPD_SCOPE_SEL0TO5_DATA_OUTPUT; alt = 0; break;		
	case  0x1000: src = NGPD_SCOPE_SEL0TO5_DATA_OUTPUT; alt = 1; break;		
	case  0x2000: src = NGPD_SCOPE_SEL0TO5_DATA_OUTPUT; alt = 2; break;			
	case  0x4000: src = NGPD_SCOPE_SEL0TO5_DATA_OUTPUT; alt = 3; break;			

	case  0x8000: src = NGPD_SCOPE_SEL5_DIG_3BIT; alt = 0; break;			
	case 0x10000: src = NGPD_SCOPE_SEL5_DIG_5BIT; alt = 0; break;			
	case 0x20000: src = Qualifier[17].IntArg; break;
	default: sv_printf(cn, "! ERROR please 1 and only 1 scope source\n");
		return -1;
	}
	if (quals & 0x40000)
		alt = Qualifier[18].IntArg;

	rc = ngpd_scope_setup_stream(path, card, stream, (u_int32_t) chan, src, alt);
	if (rc != 0)
		sv_printf(cn, "! ERROR: Setting scope stream %d: %s\n", stream, ngpd_get_error_message());
	return rc;
}


int ngpd_scope_setup5(int quals, int cn, int path, int card)
{
	u_int32_t src, alt=0;
	int rc;
	NGPDGeneration generation;
	int stream = 5;
	NGPDScopeModule *mod;

	if ((generation = ngpd_get_generation(path)) < 0)
	{
		sv_printf(cn, "! ERROR Determining ngpd generation : %s\n", ngpd_get_error_message());
		return -1;
	}
	if ((mod =ngpd_scope_get_mod(path)) == NULL)
		return -1;

	switch (quals & 0x7)
	{	
	case 0x1:    src = NGPD_SCOPE_SEL5_DIG_3BIT; break;
	case 0x2:    src = NGPD_SCOPE_SEL5_DIG_5BIT; break;
	case 0x4:    src = Qualifier[2].IntArg; break;
	default: sv_printf(cn, "! ERROR please 1 and only 1 scope source\n");
		return -1;
	}
	if (quals & 0x8)
		alt = Qualifier[3].IntArg;

	rc = ngpd_scope_setup_stream(path, card, stream, (u_int32_t) 0, src, alt);
	if (rc != 0)
		sv_printf(cn, "! ERROR: Setting scope stream %d: %s\n", stream, ngpd_get_error_message());
	return rc;
}

int ngpd_scope_setup_any(int quals, int cn, int path, int card, int stream, int chan)
{
	u_int32_t src, alt=0;
	int rc;
	NGPDGeneration generation;
	NGPDScopeModule *mod;
	int to_float = quals & 1;
	int nstreams;
	int chans_per_card;	

	if ((generation = ngpd_get_generation(path)) < 0)
	{
		sv_printf(cn, "! ERROR Determining ngpd generation : %s\n", ngpd_get_error_message());
		return -1;
	}

	if ((mod = ngpd_scope_get_mod(path)) == NULL)
		return -1;

	nstreams = ngpd_scope_mod_get_nstreams(mod);
	chans_per_card = ngpd_get_chans_per_card(path);

	if (stream < 0 || stream >= nstreams)
	{
		sv_printf(cn, "! ERROR: stream %d is out of range 0...%d for current scope mode configuration, layout-%d\n", stream, nstreams-1, mod->head.layout);
		return -1;
	}
	if (mod->head.layout == NGPD_SCOPE_LAYOUT_1)
	{
		if (stream > 0)
		{
			sv_printf(cn, "! ERROR: Current module layout is for 1 streams, so stream %d is not available. Use ngpd scope set-options ... set-streams if necessary or use streams 0..0\n", stream);
			return -1;
		}
	}
	if (mod->head.layout == NGPD_SCOPE_LAYOUT_2)
	{
		if (stream >= 1)
		{
			sv_printf(cn, "! ERROR: Current module layout is for 2 streams, so stream %d is not available. Use ngpd scope set-options ... set-streams if necessary or use streams 0..1\n", stream);
			sv_printf(cn, "! ERROR: Note that in 2 stream mode output stream 1 comes from stream 5 (usually 5 bit digital but can be analog)\n");
			return -1;
		}
	}
	if (mod->head.layout == NGPD_SCOPE_LAYOUT_4)
	{
		if (stream >= 3)
		{
			sv_printf(cn, "! ERROR: Current module layout is for 4 streams, so stream %d is not available. Use ngpd scope set-options ... set-streams if necessary or use streams 0..3\n", stream);
			return -1;
		}
	}
	if (mod->head.layout == NGPD_SCOPE_LAYOUT_6)
	{
		if (stream >= 5)
		{
			sv_printf(cn, "! ERROR: Current module layout is for 6 streams, so stream %d is not available. Use ngpd scope set-options ... set-streams if necessary or use streams 0..5\n", stream);
			return -1;
		}
	}

	if (chan < 0 || chan >= chans_per_card )
	{
		sv_printf(cn, "! ERROR: Please specify channel in range 0..%d, not %d\n", chans_per_card-1, chan);
		return -1;
	}
	if (generation == NGPDGenADQ14)
	{
		switch (quals & 0xFFFFF)
		{
		case       1: src = NGPD_SCOPE_SEL0TO5_TEST_PAT; break;
		case       2: src =  NGPD_SCOPE_SEL0TO5_INPUT;  break;
		case       4: src = NGPD_SCOPE_SEL0TO5_FILTER; break;
		case       8: src = NGPD_SCOPE_SEL0TO5_DIFF_TRIG_DIFF; break;
		case    0x10: src = NGPD_SCOPE_SEL0TO5_DIFF_TRIG_OUT; break;
		case    0x20: src = NGPD_SCOPE_SEL0TO5_BASELINE_SUB; break;
		case    0x40: src = NGPD_SCOPE_SEL0TO5_BSUB_INTERNAL; break;
		case    0x80: src = NGPD_SCOPE_SEL0TO5_MEASURE; alt = 0;  break;
		case   0x100: src = NGPD_SCOPE_SEL0TO5_MEASURE; alt = 2; break;
		case   0x200: src = NGPD_SCOPE_SEL0TO5_MEASURE; alt = 3; break;	
		case   0x400: src = NGPD_SCOPE_SEL0TO5_MEASURE; alt = 4; break;	
		case   0x800: src = NGPD_SCOPE_SEL0TO5_DATA_OUTPUT; alt = 0; break;		
		case  0x1000: src = NGPD_SCOPE_SEL0TO5_DATA_OUTPUT; alt = 1; break;		
		case  0x2000: src = NGPD_SCOPE_SEL0TO5_DATA_OUTPUT; alt = 2; break;			
		case  0x4000: src = NGPD_SCOPE_SEL0TO5_DATA_OUTPUT; alt = 3; break;			

		case  0x8000: src = NGPD_SCOPE_SEL5_DIG_3BIT; alt = 0; break;			
		case 0x10000: src = NGPD_SCOPE_SEL5_DIG_5BIT; alt = 0; break;			
		case 0x20000:
		case 0x40000: sv_printf(cn, "! ERROR not supported on ADQ14\n"); return -1;
		case 0x80000: src = Qualifier[19].IntArg; break;
		default: sv_printf(cn, "! ERROR please 1 and only 1 scope source\n");
			return -1;
		}
	}
	else
	{
		switch (quals & 0xFFFFF)
		{
		case       1: src = NGZMP_SCOPE_SEL_ANA_TEST_PAT; break;
		case       2: src = NGZMP_SCOPE_SEL_ANA_INPUT;  break;
		case       4: src = NGZMP_SCOPE_SEL_ANA_FILTER; break;
		case       8: src = NGZMP_SCOPE_SEL_ANA_DIFF_TRIG_DIFF; break;
		case    0x10: src = NGZMP_SCOPE_SEL_ANA_DIFF_TRIG_OUT; break;
		case    0x20: src = NGZMP_SCOPE_SEL_ANA_BASELINE_SUB; break;
		case    0x40: src = NGZMP_SCOPE_SEL_ANA_BSUB_INTERNAL; break;
		case    0x80: src = NGZMP_SCOPE_SEL_ANA_MEASURE; alt = 0;  break;
		case   0x100: src = NGZMP_SCOPE_SEL_ANA_MEASURE; alt = 2; break;
		case   0x200: src = NGZMP_SCOPE_SEL_ANA_MEASURE; alt = 3; break;	
		case   0x400: src = NGZMP_SCOPE_SEL_ANA_MEASURE; alt = 4; break;	
		case   0x800: src = NGZMP_SCOPE_SEL_ANA_DATA_OUTPUT; alt = 0; break;		
		case  0x1000: src = NGZMP_SCOPE_SEL_ANA_DATA_OUTPUT; alt = 1; break;		
		case  0x2000: src = NGZMP_SCOPE_SEL_ANA_DATA_OUTPUT; alt = 2; break;			
		case  0x4000: src = NGZMP_SCOPE_SEL_ANA_DATA_OUTPUT; alt = 3; break;			

		case  0x8000: sv_printf(cn, "! ERROR not supported on ZYNQMP\n"); return -1; 
		case 0x10000: src = NGZMP_SCOPE_SEL37_DIG_5BIT; alt = 0; break;			
		case 0x20000: src = NGZMP_SCOPE_SEL8_DIG_4BIT; alt= 0; break;
		case 0x40000: src = NGZMP_SCOPE_SEL9_DIG_4X14X2BIT; alt = 0; break;			
		case 0x80000: src = Qualifier[19].IntArg; break;
		default: sv_printf(cn, "! ERROR please 1 and only 1 scope source\n");
			return -1;
		}
	}
	if (quals & 0x100000)
		alt = Qualifier[20].IntArg;

	rc = ngpd_scope_setup_stream(path, card, stream, (u_int32_t) chan, src, alt);
	if (rc != 0)
		sv_printf(cn, "! ERROR: Setting scope stream %d: %s\n", stream, ngpd_get_error_message());
	return rc;
}

#if 0
int ngpd_search_scope_events(int quals, int cn, int path, int card, int stream0, int stream1)
{
	u_int16_t *ana, *dig0, *dig1;
	int num_x=16384;
	int t, i, j, k, n=0, fn;
	u_int16_t x;
	NGPDScopeModule * scope_mod;
	int inc;
	int num_dig0, bit_posn0;
	int num_dig1, bit_posn1;
	int nstreams;
	int otd_pos0, cfd_pos0, reset_pos0, gr_pos0; 
	int otd_pos1, cfd_pos1, reset_pos1, gr_pos1; 
	u_int16_t otd0=0, cfd0=0, reset0=0, otd1=0, cfd1=0, reset1=0, gr0=0, gr1=0;
	int rc;
	int use_offset_from_reset=0, offset=0, search_first_reset=0;
	int max_time_offset = 1000;
	int cfd_start[2]= {-1, -1};
	int cfd_stop[2]= {-1, -1};
	int otd_start[2]= {-1, -1};
	int otd_stop[2]= {-1, -1};
	int missing_otd[2] = {0, 0};
	int late_otd[2] = {0, 0};
	int dont_print_missing = quals  & 1;
	int dont_print = quals & 1;
	int start_during_gr[2];

	if (quals & 2)
		dont_print_missing = 1;
	scope_mod = ngpd_scope_get_mod(path); 

	if (scope_mod == NULL)
	{
		sv_printf(cn, "! ERROR: Scope data module not available :%s\n", ngpd_get_error_message());
		return -1;
	}
	inc = ngpd_scope_mod_get_inc(scope_mod);
	nstreams = ngpd_scope_mod_get_nstreams(scope_mod);

	if (TotalArgumentCount == 3)
		stream1 = -1;

	if ((rc=ngpd_scope_stream_has_event(scope_mod, card, stream0, &otd_pos0, &cfd_pos0, &reset_pos0, &gr_pos0)) < 0)
	{
		sv_printf(cn, "! ERROR: Error determining whether stream %d has event info: %s\n",  stream0, ngpd_get_error_message());
		return -1;
	}
	if (rc == 0)
	{
		sv_printf(cn, "! ERROR: Stream %d does not has event info: %s\n",  stream0);
		return -1;
	}

	if (stream1 >= 0)
	{
		if ((rc=ngpd_scope_stream_has_event(scope_mod, card, stream1, &otd_pos1, &cfd_pos1, &reset_pos1, &gr_pos1)) < 0)
		{
			sv_printf(cn, "! ERROR: Error determining whether stream %d has event info: %s\n",  stream1, ngpd_get_error_message());
			return -1;
		}
		if (rc == 0)
		{
			sv_printf(cn, "! ERROR: Stream %d does not has event info: %s\n",  stream1);
			return -1;
		}
	}

	if (ngpd_scope_stream_details(scope_mod, card, stream0, &num_dig0, &bit_posn0, &dig0, NULL, NULL, NULL) < 0)
	{
		sv_printf(cn, "ERROR: Cannot find info about requested stream %d and card: %s\n", stream0, ngpd_get_error_message());
		return -1;
	}
	if (stream1 >= 0)
	{
		if (ngpd_scope_stream_details(scope_mod, card, stream1, &num_dig1, &bit_posn1, &dig1, NULL, NULL, NULL) < 0)
		{
			sv_printf(cn, "ERROR: Cannot find info about requested stream %d and card: %s\n", stream1, ngpd_get_error_message());
			return -1;
		}
	}
//	ana = ngpd_scope_mod_get_ptr(scope_mod, card, stream);
	if (dig0 == NULL || (stream1 >= 0 && dig1 == NULL))
	{
		sv_printf(cn, "# ERROR determining data pointers: %s\n", ngpd_get_error_message());
		return -1;
	}

	if (stream1 >= 0)
	{
		use_offset_from_reset = 1;
		if (reset_pos0 < 0 || reset_pos1 < 0)
		{
			sv_printf(cn, "!  ERROR: Need to use end of reset to find offset between 2 sources, but cannot find reset bit on both resrt_pos0=%d, reset_pos1=%d\n", reset_pos0, reset_pos1);
			return -1;
		}
		search_first_reset = 1;
	}

	sv_printf(cn, "#\tNum\tPosn\tRow\tCol\n");
	n = 0;
	fn = 0;
	
	if (cfd_pos0 >= 0)   cfd0   = 1 << (bit_posn0+cfd_pos0);
	if (otd_pos0 >= 0)   otd0   = 1 << (bit_posn0+otd_pos0);
	if (reset_pos0 >= 0) reset0 = 1 << (bit_posn0+reset_pos0);
	if (gr_pos0 >= 0)    gr0    = 1 << (bit_posn0+gr_pos0);
	if (stream1 >= 0)
	{
		if (cfd_pos1 >= 0)   cfd1   = 1 << (bit_posn1+cfd_pos1);
		if (otd_pos1 >= 0)   otd1   = 1 << (bit_posn1+otd_pos1);
		if (reset_pos1 >= 0) reset1 = 1 << (bit_posn1+reset_pos1);
		if (gr_pos1 >= 0)    gr1    = 1 << (bit_posn1+gr_pos1);
	}
	for (t=0; t<scope_mod->head.num_t-1; t++)
	{
		if (use_offset_from_reset)
		{
			if ((dig0[t*inc] & reset0) && !(dig0[(t+1)*inc] & reset0))
			{
				for (j=0; j<max_time_offset; j++)
				{
					if (t-j >= 0 && (dig1[(t-j)*inc] & reset1) && !(dig1[(t-j+1)*inc] & reset1))
					{
						offset = -j;
						search_first_reset=0;
						break;
					}
					else if (t+j < scope_mod->head.num_t-1 && (dig1[(t+j)*inc] & reset1) && !(dig1[(t+j+1)*inc] & reset1))
					{
						offset = j;
						search_first_reset=0;
						break;
					}
				}
				if (j == max_time_offset)
					sv_printf(cn, "# Cannot find other stream reset near %d = (%d, %d)\n",  t, t/num_x, t%num_x);
			}
		}				
		if ((dig0[t*inc] & reset0) && !(dig0[(t+1)*inc] & reset0))
		{
			if (dig0[t*inc] & cfd0)
				cfd_start[0] = t;
			if (dig0[t*inc] & otd0)
				otd_start[0] = t;
		}
		if (stream1 >= 0 && (dig1[t*inc] & reset1) && !(dig1[(t+1)*inc] & reset1))
		{
			if (dig1[t*inc] & cfd1)
				cfd_start[1] = t;
			if (dig1[t*inc] & otd1)
				otd_start[1] = t;
		}

		if (!search_first_reset && !(dig0[t*inc] & reset0))
		{
			if (otd0 != 0)
			{
				for (i=0;i<2; i++)
				{
					u_int16_t cfd, otd, gr, *dig;
					int stream;
					if (i == 0)
					{
						otd = otd0;
						cfd = cfd0;
						dig = dig0;
						gr  = gr0;
						stream = stream0;
					}
					else
					{
						if (stream1 < 0)
							break;
						otd = otd1;
						cfd = cfd1;
						dig = dig1;
						gr  = gr1;
						stream = stream1;
					}
					if (!(dig[t*inc]&cfd) && (dig[(t+1)*inc]&cfd))
					{
						cfd_start[i] = t+1;
						start_during_gr[i] = !! (dig[t*inc]&gr);
					}
					if (!(dig[t*inc]&otd) && (dig[(t+1)*inc]&otd))
						otd_start[i] = t+1;
					if ((dig[t*inc]&cfd) && !(dig[(t+1)*inc]&cfd))
					{
						cfd_stop[i] = t;
						if (otd_start[i] == -1 && start_during_gr[i] == 0 && !(dig[t*inc]&gr))
						{
							if (!dont_print_missing)
								sv_printf(cn, "# Stream %d: Missing OTD near %d =(%d,%d)\n", stream, t, t/num_x, t%num_x);
							missing_otd[i]++;
						}
						if (dig[(t+1)*inc] & otd)
						{
							// CFD ended before OTD so leave to OTD to end event
						}
						else
						{
							otd_start[i] = -1;
							cfd_start[i] = -1;
						}
					}
					if ((dig[t*inc]&otd) && !(dig[(t+1)*inc]&otd))
					{
						otd_stop[i] = t;
						if (dig[t*inc] & cfd)
						{
							// OTD ended with in CFD, leave to end of CFD to check
						}
						else
						{
							if (cfd_start[i] == -1)
							{
								if (!dont_print)
									sv_printf(cn, "# Stream %d: Extra or late OTD near %d =(%d,%d)\n", stream, t, t/num_x, t%num_x);
								late_otd[i]++;
							}
							otd_start[i] = -1;
							cfd_start[i] = -1;
						}

					}
				}
			}
		}
	}
	sv_printf(cn, "# Stream %d: Missing OTD=%d, Late OTD=%d\n", stream0, missing_otd[0], late_otd[0]);
	if (stream1 >= 0)
		sv_printf(cn, "# Stream %d: Missing OTD=%d, Late OTD=%d\n", stream1, missing_otd[1], late_otd[1]);
	return n;
}
#endif
int ngpd_scope_save(int quals, int cn, int path, char *fname)
{
	NGPDScopeModule *mod;
	int stream, src, chan, alt;
	int ncards, card;
	FILE *fp;
	NGPDGeneration generation;
	int nstreams;
	int lstream;

	if ((mod = ngpd_scope_get_mod(path)) == NULL)
		return -1;


	ncards = ngpd_get_num_cards(path);
	if (ncards < 0)
	{
		sv_printf(cn, "! ERROR Determining number of cards : %s\n", ngpd_get_error_message());
		return -1;
	}

	if ((generation = ngpd_get_generation(path)) < 0)
	{
		sv_printf(cn, "! ERROR Determining ngpd generation : %s\n", ngpd_get_error_message());
		return -1;
	}
	if ((fp=fopen(fname, "w")) ==NULL)
	{
		sv_printf(cn, "! ERROR: Cannot open file '%s'\n", fname);
		return -1;
	}
	nstreams = ngpd_scope_mod_get_nstreams(mod); 

//		fprintf(fp, "ngpd scope set-options %d -1 sync set-streams %d\n", path, nstreams);
	for (card=0; card<ncards; card++)
	{
		for (lstream=0; lstream<nstreams; lstream++)
		{
			stream = lstream;
			chan = NGZMP_SCOPE_CHAN_SEL_GET(stream, mod->head.card[card].chan_sel[0], mod->head.card[card].chan_sel[1]);
			src = NGZMP_SCOPE_SRC_SEL_GET(stream, mod->head.card[card].src_sel[0], mod->head.card[card].src_sel[1]);
			alt = NGZMP_SCOPE_ALTERNATE_GET(stream, mod->head.card[card].alternate[0], mod->head.card[card].alternate[1]);

			fprintf(fp, "ngpd scope setup-any %d %d %d %d src %d alt %d\n", path, card, stream, chan, src, alt);
		}
	}

	fclose(fp);
	return 0;
}

int ngpd_scope_setup_all(int quals, int cn, int path, int card)
{
	int rc=-1;
	int sync_boxes = 0;
	
	sync_boxes = !!(quals & 1);
	switch (quals >> 1)
	{
	case 0:
		rc = ngpd_cal_setup_all_ana(path, card, sync_boxes, NGZMP_SCOPE_SEL_ANA_INPUT);
		break;
	case 1:
		rc = ngpd_cal_setup_all_ana(path, card, sync_boxes, NGZMP_SCOPE_SEL_ANA_FILTER);
		break;
	default:
		sv_printf(cn, "! ERROR: Unrecognised data soucre qualifiers\n");
		return -1;
	}
	
	if (rc < 0)
	{
		sv_printf(cn, "! ERROR: Cannot setup scope mode : %s\n", ngpd_get_error_message());
		return -1;
	}
	return ngpd_scope_resize_read_handle(cn, path);
}

	

