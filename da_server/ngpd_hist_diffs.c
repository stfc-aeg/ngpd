/* Calibration of trigger threshold by measuring/calculating noise on differentials.
The issue is getting "clean" examples of noise with no events. Rely in the user setting uop with no source.
Then can differentiate , histogram, std dev etc.
*/

#include "header.h"
#include "ngpd.h"

int app_ngpd_measure_diff_hist (int cn, int path, int diff_num, double fraction, int decades, int num_settings, int mask_0_diff);

static int measure_chunk_baseline_ends(u_int16_t * chunk_ptr, int chunk, float *lhs, float *rhs, int num_lhs, int num_rhs, int scope_inc);

#define MAX_DIFFS (16)
#define DIFF_TRIG 	0
#define DIFF_VARY_SEP 100
#define SCALE_ADC_UNITS	2

static char * hist_names[MAX_DIFFS] = { "hist_diff_trig", };
static char * noise_names[MAX_DIFFS] = {"noise_diff_trig"};

int ngpd_histogram_diffs (int quals, int cn, int path)
{
	NGPDScopeModule * scope_mod;
	MOD_IMAGE *hist_mod[MAX_DIFFS];
	mh_com *hist_head[MAX_DIFFS];
	MOD_IMAGE3D *persist_mod[NGPD_MAX_CHANS];
	mh_com *persist_head[NGPD_MAX_CHANS];
	int num_chan, num_cards;
	int do_persist=0;
	int chunk = 1024;
	int first_chan, last_chan;
	char persist_name[NGPD_MAX_MODNAME+2], hist_name[NGPD_MAX_MODNAME+2];
	char mod_name[NGPD_MAX_MODNAME+20];
	int i, j, k, chan, card, stream;
	int start;
	int no_clear = 0;
	int *iptr, *hist_ptr;
	int num_lhs, num_rhs;
	float baseline, lhs, rhs, lhs4, rhs4;
	int pass, num_pass=1;
	int scale_down_reset=8;
	int found_reset;
	double event_tolerance=40;
	int col_sat = 0;
	int persist_baseline;
	int prev_event;
	int persist_num_y = 1000;
	int start_gap, stop_gap;
	int extra;
	char *playback=NULL;
	double scale_sigma = 5;
	int diff_sep[MAX_DIFFS];
	int hist_num_x = 1024;
	int chunk_start;
	int num_chunks, num_good[NGPD_MAX_CHANS];
	int diff_num;
	int diff, off_diff;
	int errant_thres[MAX_DIFFS];
	int persist_errant=0;
	int errant;
	int chans_per_card;
	int rc;
	u_int16_t *dig_ptr, *ana_base, *ana_ptr;
	int save_flags, flags;
	enum {
		Normal, VarySep 
	} mode = Normal;
	NGPDDiffTrigger diff_trig;
	NGPDScopeOptions scope_options;
	int scope_inc;
	int first_stream, last_stream;
	int trig_offset, wide_a_offset, wide_b_offset, neutron_offset;
	int diff_trig_from_hw=0;
	int sep_offset=NGPD_DIFF_TRIG_MIN_SEP, sep_max=NGPD_DIFF_TRIG_MAX_SEP;
	int possible_chunks;
	int mod_num_t;
	u_int32_t scope_status;

	for (i=0; i<MAX_DIFFS; i++)
	{
		diff_sep[i] = -1;
		errant_thres[i] = -1;
	}

	num_chan = ngpd_get_num_chan(path);
	if (num_chan <= 0)
	{
		sv_printf(cn, "! ERROR: Cannot read num_chan from path %d, %s\n", path, ngpd_get_error_message());
		return -1;
	}
	num_cards = ngpd_get_num_cards(path);
	chans_per_card = ngpd_get_chans_per_card(path);
	scope_mod = ngpd_scope_get_mod(path);
	if (scope_mod == NULL)
	{
		sv_printf(cn, "! ERROR: Scope data module not available :%s\n", ngpd_get_error_message());
		return -1;
	}


	first_chan = 0;
	last_chan  = num_chan-1;

	if (quals & 1)
		chunk=Qualifier[0].IntArg;

	if (chunk < 100 || chunk > 65536)
	{
		sv_printf(cn, "! ERROR: Expected chunk in range 100..65536 not %d\n", chunk);
		return -1;
	}

	if (quals & 2)
	{
		do_persist=1;
		strncpy(persist_name, Qualifier[1].StringArg, NGPD_MAX_MODNAME);
		persist_name[NGPD_MAX_MODNAME]=0;
	}

	if (quals & 0x4)
		num_pass = Qualifier[2].IntArg;

	if (quals & 0x8)
		col_sat = Qualifier[3].IntArg;

	if (col_sat < 0)
	{
		sv_printf(cn, "! ERROR : expected colour saturation control > 0, not %d\n", col_sat);
		return -1;
	}
	if (quals & 0x10)
	{
		chan = Qualifier[4].IntArg;
		if (chan < 0 || chan >= num_chan)
		{
			sv_printf(cn, "! ERROR : expected %d <= chan <= %d , not %d\n", 0, num_chan-1, chan);
			return -1;
		}
		first_chan = chan;
		last_chan = chan;
	}
	if (quals & 0x20)
		event_tolerance = Qualifier[5].DoubleArg;

	if (quals & 0x40)
		playback = Qualifier[6].StringArg;

	if (quals & 0x80)
	{
		persist_errant = 1;
		errant_thres[0] = Qualifier[7].IntArg;
		if (do_persist == 0)
		{
			sv_printf(cn, "! ERROR: Please specifiy persistance module name for errant hists\n");
			return -1;
		}
	}

	if (quals & 0x100)
		diff_sep[DIFF_TRIG] = Qualifier[8].IntArg;
	else
		diff_trig_from_hw=1;

	if (quals & 0x200)
	{
		mode = VarySep;
		for (j=0;j<MAX_DIFFS;j++)
		{
			diff_sep[j] = j+sep_offset;
		}
		diff_trig_from_hw = 0;
	}

	if (quals & 0x400)
		first_chan = Qualifier[10].IntArg;
	if (quals & 0x800)
		last_chan = Qualifier[11].IntArg;

	if (first_chan < 0 || last_chan < first_chan || last_chan >= num_chan)
	{
		sv_printf(cn, "! ERROR : expected %d <= first_chan <= last_chan <= %d , not first_chan=%d, last_chan=%d\n", 0, num_chan-1, first_chan, last_chan);
		return -1;
	}

	if (diff_trig_from_hw)
	{
		/* Scan all the channel to see if any have otd-fast enabled */
		for (chan=first_chan; chan<=last_chan; chan++)
		{
			if (ngpd_read_diff_trigger(path, chan, &diff_trig) < 0)
			{
				sv_printf(cn, "! ERROR extracting Diff trigger setup on chan %d\n", chan);
				return -1;
			}
			if (chan == first_chan)
			{
				diff_sep[DIFF_TRIG] = diff_trig.sep;
			}
			else
			{
				if (diff_sep[DIFF_TRIG] != diff_trig.sep)
				{
					sv_printf(cn, "! ERROR inconsistent trigger filter setting between channel %d and %d. Work in blocks of channels if necessary\n", first_chan, chan);
					return -1;
				}
			}

		}
	}
	for (i=0;i<NGPD_MAX_CHANS; i++)
		persist_mod[i] = NULL;

	for (i=0;i<MAX_DIFFS; i++)
		hist_mod[i] = NULL;

	if (do_persist)
	{
		for (i=first_chan;i<=last_chan; i++)
		{
			sprintf(mod_name, "%s_%02d", persist_name, i);
			persist_mod[i] = app_mkmod3d (cn, mod_name, chunk, persist_num_y, 1, "Time", "ADC Value", "Unused", NULL,  0, &(persist_head[i]));
			if (persist_mod[i] == NULL)
				goto exit_error;
			if (!no_clear)
			{
				clear_mod((MOD_IMAGE *)persist_mod[i]);
			}
		}
	}
	for (i=0; i < MAX_DIFFS; i++)
	{
		if (diff_sep[i] > 0)
		{
			switch (mode)
			{
			case Normal:
				strcpy(mod_name, hist_names[i]);
				break;
			case VarySep:
				sprintf(mod_name, "hist_diff_sep%04d", diff_sep[i]);
				break;
			}
			hist_mod[i] = app_mkmod_head (cn, mod_name, hist_num_x, num_chan, "ADC Value (2 ADU/bin)", "Channel",  0, &(hist_head[i]));
			if (hist_mod[i] == NULL)
				goto exit_error;
		}
	}

	for (i=0; i<MAX_DIFFS; i++)
	{
		if (hist_mod[i] != NULL && !no_clear)
		{
			for (chan=first_chan; chan<=last_chan; chan++)
			{
				iptr = (int *) id_get_ptr(hist_mod[i], 0, chan, 0);
				for (j=0; j<hist_mod[i]->head.num_x; j++)
					*iptr ++ = 0;
			}
		}
	}

	num_chunks = 0;
	for (i=0;i<NGPD_MAX_CHANS; i++)
		num_good[i] = 0;

	save_flags = ngpd_get_run_flags(path);
	flags = (save_flags & ~NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS) | NGPD_RUN_FLAGS_SCOPEMODE | NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD;
	ngpd_set_run_flags(path, flags);
	for (pass = 0; pass < num_pass; pass++)
	{
		if ((rc=ngpd_calib_setup_all_chan(cn, path, -1, NGZMP_SCOPE_SEL_ANA_DIFF_TRIG_OUT,	&first_stream, &last_stream, &trig_offset, &wide_a_offset, &wide_b_offset, &neutron_offset, 1)) < 0)
		{
			sv_printf(cn, "! Error setting scope streams : %s\n", ngpd_get_error_message());
			return -1;
		}

		if (playback)
		{
			char cmd[2048];
			sprintf(cmd, playback, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass);
			if (user_service(cn, cmd, 0, NULL, NULL) < 0)
			{
				sv_printf(cn, "! Cannot run playback command '%s'\n", cmd);
				goto exit_error;
			}
		}
		rc = ngpd_dma_system_start(path, -1, 0, 0, NULL);
		if (rc < 0)
		{
			sv_printf(cn, "! ERROR starting system: %s\n", ngpd_get_error_message());
			goto exit_error;
		}
		rc = ngpd_dma_wait_scope(path, -1, &scope_status);
		if (rc < 0) {
			sv_printf(cn,"! ERROR waiting for scope DMAs %s\n", ngpd_get_error_message());
			goto exit_error;
		}
		if (scope_status != 0)
		{
			sv_printf(cn, "! ERROR: Scope status appears wrong at end of run = %08X\n", scope_status);
			return -1;
		}
		for (card=0; card < num_cards; card++)
		{
			rc = ngpd_dma_read_scope(path, card, 0, 0, 0);
			if (rc < 0)
			{
				sv_printf(cn,"! ERROR reading scope data from card %d : %s\n", card, ngpd_get_error_message());
				goto exit_error;
			}
		}
		chunk_start = 0; /* The tsting of chunks by rhs-lhs is no longer valid. For now assume that a chunk is OK and use it */
		for (i=0; i<scope_mod->head.num_t; i++)
		{
			if (chunk_start >= 0)
			{
				if (i - chunk_start == chunk)
				{
					/* We have a chunk to use */
					num_chunks++;
					for (card=0; card < num_cards; card++)
					{
						for (stream=first_stream; stream<=last_stream; stream++)
						{
							chan = card*chans_per_card+stream-first_stream;
							if (chan < first_chan || chan > last_chan)
								continue;
							scope_inc = ngpd_scope_mod_get_inc(scope_mod, stream); // Moved to after ngpd_calib_setup_inp_all_resets
							ana_base = ngpd_scope_mod_get_ptr(scope_mod, card, stream);
							ana_ptr = ana_base + scope_inc*chunk_start;

							num_good[chan]++;
							errant = 0;
							for (diff_num=0; diff_num<MAX_DIFFS; diff_num++)
							{
								int sep=diff_sep[diff_num];
								int end;
								if (sep <0)
									continue;
								hist_ptr= (int *)(hist_mod[diff_num]->data)+hist_mod[diff_num]->head.num_x*chan;

								end = chunk-(sep);

								ana_ptr = ana_base + scope_inc*(chunk_start+num_lhs);
								for (j=num_lhs; j< end; j++)
								{
									diff = ana_ptr[scope_inc*sep] - ana_ptr[0];
									if (errant_thres[diff_num] > 0 && abs(diff) > errant_thres[diff_num])
										errant++;
									ana_ptr += scope_inc;

									off_diff = diff + hist_num_x/2;
									if (off_diff <0) off_diff = 0;
									if (off_diff >= hist_num_x)
										off_diff = hist_num_x-1;
									hist_ptr[off_diff]++;
								}
							}
							if ((do_persist && !persist_errant) || (do_persist && persist_errant && errant))
							{
								persist_baseline = lhs - persist_num_y/2;
								ngpd_accumulate_persist(ana_base, NULL, chunk_start, persist_mod[chan], 0, 0, chunk, persist_baseline, 32000/persist_num_y, col_sat, 0, 0, 0, scope_inc, 0);
							}
						} /* End For stream */
					} /* End For card */
					chunk_start = i;
				} /* If usable chunk */
			} /* End if have Valid Chunk start */
			if (sv_quit)
			{
				sv_printf(cn, "STOPPED By ^c\n");
				goto exit_error;
			}
		} /* End For i scanning for reset free chunks */
	} /* End For Pass */
	ngpd_set_run_flags(path, save_flags);

	/*
	possible_chunks = scope_mod->head.num_t/chunk;
	possible_chunks *= num_pass;

	if (num_chunks < possible_chunks/10)
	{
	sv_printf(cn, "! ERROR: From scanning RESETS found only %d chunks out of possible %d, so rate is too high or RESET STUCK HIGH\n", num_chunks, possible_chunks);
	goto exit_error;
	}
	if (num_chunks < possible_chunks/3)
	{
	sv_printf(cn, "# Warning: From scanning RESETS found only %d chunks out of possible %d, so suggests that rate too high or RESET STUCK HIGH\n", num_chunks, possible_chunks);
	}
	else 
	{
	sv_printf(cn, "# From scanning resets used %d chunks out of a possible %d\n", num_chunks, possible_chunks);
	}

	i=0;
	possible_chunks = scope_mod->head.num_t/chunk;
	possible_chunks *= num_pass;
	for (chan=first_chan; chan<=last_chan; chan++)
	{
	if (num_good[chan] < possible_chunks/3)
	{
	sv_printf(cn, "! ERROR Channel %d found only %d out of %d chunks, reduce rate or investigate leakage rate and tolerance\n", chan, num_good[chan], possible_chunks);
	i++;
	}
	else
	{
	sv_printf(cn, "#  OK Channel %d : Used %4d out of %4d chunks\n", chan, num_good[chan], possible_chunks);
	}

	}
	if (i > 0)
	goto exit_error;
	*/

	for (i=0; i<NGPD_MAX_CHANS; i++)
	{
		if (persist_mod[i] != NULL)
			munlink(persist_head[i]);
	}
	for (i=0; i<MAX_DIFFS; i++)
	{
		if (hist_mod[i] != NULL)
			munlink(hist_head[i]);
	}
	return 0;


exit_error:

	for (i=0; i<NGPD_MAX_CHANS; i++)
	{
		if (persist_mod[i] != NULL)
			munlink(persist_head[i]);
	}
	for (i=0; i<MAX_DIFFS; i++)
	{
		if (hist_mod[i] != NULL)
			munlink(hist_head[i]);
	}
	return -1;
}


int ngpd_measure_diff_hist (int quals, int cn, int path)
{
	if ((quals & 1) || (quals & 0x3) == 0)
		return app_ngpd_measure_diff_hist(cn, path, DIFF_TRIG, 0.0001, 3, 1, !(quals & 0x4));
	if (quals & 0x2)
		app_ngpd_measure_diff_hist(cn, path, DIFF_VARY_SEP, 0.000001, 1, 16, !(quals & 0x4));

	return 0;
}
#define MAX_DECADES 3
#define HIST_RES_MEAN    0 
#define HIST_RES_STDEV   1
#define HIST_RES_FRAC1   2
#define HIST_RES_FRAC10  3
#define HIST_RES_FRAC100 4

int app_ngpd_measure_diff_hist (int cn, int path, int diff_num, double fraction, int decades, int num_settings, int mask_0_diff)
{
	MOD_IMAGE *hist_mod;
	mh_com *hist_head;
	MOD_IMAGE *noise_mod=NULL;
	mh_com *noise_head;
    u_int16 attr_rev = mkattrevs(MA_REENT,1);
    u_int16 type_lang = mktypelang(MT_DATA, ML_ANY);
	u_int32 * iptr;
	float *fptr;
	u_int32 sum_f, target;
	int64_t sum_fx, sum_fxx;
	int64_t x;
	double mean, var, stdev;
	int i, j, chan, num_chan, num_x;
	int first_chan, last_chan;
	int no_clear=0;
	char *cp;
	int setting;
	char mod_name[NGPD_MAX_MODNAME+20];
	int vary_sep_mode=0;
	int sep;
			
	num_chan = ngpd_get_num_chan(path);
	if (num_chan <= 0)
	{
		sv_printf(cn, "! ERROR: Cannot read num_chan from path %d, %s\n", path, ngpd_get_error_message());
		return -1;
	}
	first_chan = 0;
	last_chan  = num_chan-1;
	if (diff_num == DIFF_VARY_SEP)
	{
		diff_num = 0;
		vary_sep_mode = 1;
	}
	if (diff_num <0 || diff_num >= MAX_DIFFS)
	{
		sv_printf(cn, "! Coding ERROR:app_ngpd_measure_diff_hist, diff num %d %s:%d\n", diff_num, __FILE__, __LINE__);
		return -1;
	}
	if (decades > MAX_DECADES)
	{
		sv_printf(cn, "! Coding ERROR:app_ngpd_measure_diff_hist, decades %d %s:%d\n", decades, __FILE__, __LINE__);
		return -1;
	}
	if (vary_sep_mode)
	{
		noise_mod = app_mkmod_head (cn, "vary_sep_noise", num_chan, 2*num_settings, "Chan", "Function",  1, &noise_head);
	}
	else
		noise_mod = app_mkmod_head (cn, noise_names[diff_num], num_chan, 2+MAX_DECADES, "Chan", "Function",  1, &noise_head);
	if (noise_mod == NULL)
		goto exit_error;

	if (!no_clear)
	{
		fptr = (float *)noise_mod->data;
		for (j=0; j<num_chan*noise_mod->head.num_y; j++)
			*fptr ++ = 0;
	}
	for (setting=0; setting < num_settings; setting++)
	{
		if (vary_sep_mode)
		{
			sep = NGPD_DIFF_TRIG_MIN_SEP+setting;
			sprintf(mod_name, "hist_diff_sep%04d", sep);
			cp = mod_name;
		}
		else
			cp = hist_names[diff_num];
	    if (_os_link(&cp, &hist_head, (void **)&hist_mod, &type_lang, &attr_rev))
		{
			sv_printf (cn,"! Cannot link to data module %s\n", cp);
			goto exit_error;
		}

		if (hist_mod->head.num_y != num_chan)
		{
			sv_printf(cn, "! ERROR: Module %s does not have 1 row per channel. num_chan=%d num_y=%d\n", num_chan, hist_mod->head.num_y);
			goto exit_error;
		}

		fptr= (float *)id_get_ptr(noise_mod, first_chan, setting, 0);

		for (chan=first_chan; chan<=last_chan; chan++)
		{
			num_x = hist_mod->head.num_x;
			iptr = (int *)id_get_ptr(hist_mod, 0, chan, 0);
			
			sum_f   = 0;
			sum_fx  = 0;
			sum_fxx = 0;
			for (i=0; i<num_x; i++)
			{
				x = i-num_x/2;
				if (x == 0 && mask_0_diff)
				{
					int ave = (iptr[-1]+iptr[1])/2;
					sum_f += ave;
					sum_fx += ((int64_t)ave) * x ;
					sum_fxx += ((int64_t)ave) * x * x  ;
				}
				else
				{
					sum_f += *iptr;
					sum_fx += ((int64_t)*iptr) * x ;
					sum_fxx += ((int64_t)*iptr) * x * x  ;
				}
				iptr++;
			}
			if (sum_f > 0)
			{
				mean = (double)sum_fx/sum_f;
				var  = (double)sum_fxx/sum_f - mean*mean;
				if (var >0)
					stdev = sqrt(var);
				else 
					stdev = 0;
			}
			else
			{
				mean = 0;
				stdev = 0;
			}
	/*		sv_printf(cn, "# sum_f=%u, sum_fx=%lld\n", sum_f, sum_fx); */
			if (vary_sep_mode)
				fptr[0] = SCALE_ADC_UNITS*stdev;
			else
			{
				fptr[HIST_RES_MEAN*num_chan]  = SCALE_ADC_UNITS*mean;
				fptr[HIST_RES_STDEV*num_chan] = SCALE_ADC_UNITS*stdev;
			}
			sv_printf(cn, "# Chan %d: Mean=%g, stdev=%g\n", chan, SCALE_ADC_UNITS*mean, SCALE_ADC_UNITS*stdev); 
			target = sum_f * fraction;
			for (j=0; j<decades; j++)
			{
				sum_f = 0;
				iptr = (int *)hist_mod->data+chan*num_x+num_x-1;
				for (i=num_x; i > 0; i--)
				{
					sum_f += *iptr--;
					if (sum_f > target)
						break;
				}
				if (vary_sep_mode)
					fptr[num_chan * num_settings] = SCALE_ADC_UNITS*(i-num_x/2);
				else
					fptr[(HIST_RES_FRAC1+j)*num_chan] = SCALE_ADC_UNITS*(i-num_x/2);
				target /= 10;
			}

			fptr++;
		}

		if (hist_mod != NULL)
			munlink(hist_head);
		hist_mod = NULL;
	}
	if (noise_mod != NULL)
		munlink(noise_head);
	return 0;

exit_error:
	if (hist_mod != NULL)
		munlink(hist_head);
	if (noise_mod != NULL)
		munlink(noise_head);
	return -1;
}


#define NUM_THRES 	1
#define NUM_SIGMA	1

int ngpd_adjust_trigger_thresholds(int quals, int cn, int path)
{
	int sys;
	MOD_IMAGE *noise_mod=NULL;
	mh_com *noise_head;
	MOD_IMAGE *thres_mod=NULL;
	mh_com *thres_head;
    u_int16 attr_rev = mkattrevs(MA_REENT,1);
    u_int16 type_lang = mktypelang(MT_DATA, ML_ANY);
	int first_chan, last_chan, num_chan;
	char *cp;
	int i, j;
	int chan;
	int num_x;
	float *sptr, *dptr, *sigma_ptr;
	enum {Unchanged, ScaleSigma, Fraction } thres_type[NUM_THRES];
	double thres_val[NUM_THRES];
	int thres;
	double fraction;
	int no_clear = 0;
	int first_thres = NGPD_THRES_DIFF_TRIG;
	int last_thres  = NGPD_THRES_DIFF_TRIG;
	const char *thres_name[] = {"DiffTrig"};
	NGPDDiffTrigger diff_trig;

	num_chan = ngpd_get_num_chan(path);
	if (num_chan <= 0)
	{
		sv_printf(cn, "! ERROR: Cannot read num_chan from path %d, %s\n", path, ngpd_get_error_message());
		return -1;
	}
	first_chan = 0;
	last_chan  = num_chan-1;

	if (quals & 1)
	{
		first_chan = Qualifier[0].IntArg;
		last_chan  = Qualifier[0].IntArg;
	}

	if (quals & 2)
		first_chan = Qualifier[1].IntArg;

	if (quals & 4)
		last_chan = Qualifier[2].IntArg;


	thres_type[NGPD_THRES_DIFF_TRIG]   = ScaleSigma;
	thres_val[NGPD_THRES_DIFF_TRIG]    = 5.0;

	if ((quals & 0x18) == 0 && first_thres == NGPD_THRES_DIFF_TRIG)
	{
		sv_printf(cn, "# Using default settings, for Diff trigger :Arm thres=%g*sigma\n", thres_val[NGPD_THRES_DIFF_TRIG]);
	}
	
	for (thres=0; thres < NUM_THRES; thres++)
	{
		switch ((quals >>(3+2*thres)) & 0x3)
		{
		case 1:
			thres_type[thres] = ScaleSigma;
			thres_val[thres]  = Qualifier[6+2*thres].DoubleArg;
			break;
		case 2:
			thres_type[thres] = Fraction;
			thres_val[thres]  = Qualifier[7+2*thres].DoubleArg;
			break;
		case 3:
			sv_printf(cn, "! Please specify only one of scaler-sigma or fraction\n");
			return -1;
		}
	}

	thres_mod = app_mkmod_head (cn, "ngpd_thresholds", num_chan, NUM_THRES+NUM_SIGMA, "Chan", "Function",  1, &thres_head);
	if (thres_mod == NULL)
		goto exit_error;

	if (!no_clear)
	{
		for (chan=first_chan;chan<=last_chan; chan++)
		{
			for (j=0; j<thres_mod->head.num_y; j++)
			{
				dptr = (float *)id_get_ptr(thres_mod, chan, j, 0);
				*dptr = 0;
			}
		}
	}

	for (chan=first_chan; chan <= last_chan; chan++)
	{
		if (ngpd_read_diff_trigger(path, chan, &diff_trig) < 0)
		{
			sv_printf(cn, "! ERROR extracting differential trigger setup on chan %d\n", chan);
			goto exit_error;
		}
	}
	for (i=first_thres;i<=last_thres; i++)
	{
		if (thres_type[i] != Unchanged)
		{
			int which_diff[NUM_THRES] = { DIFF_TRIG};

			if (thres_type[i] == Fraction)
				fraction = thres_val[i];
			else
				fraction = 0.001;

			if (app_ngpd_measure_diff_hist(cn, path, which_diff[i], fraction, 3, 1, 1) < 0)
			{
				sv_printf(cn, "! Problem measuring histogram run hist-diffs first\n");
				goto exit_error;
			}

			cp = noise_names[which_diff[i]];
		    if (_os_link(&cp, &noise_head, (void **)&noise_mod, &type_lang, &attr_rev))
			{
				sv_printf (cn,"! Cannot link to data module %s\n", noise_names[which_diff[i]]);
				goto exit_error;
			}
			if (noise_mod->head.num_x != num_chan)
			{
				sv_printf(cn, "# ERROR: Noise module %s width %d does not match num_chan=%d\n",
								noise_names[which_diff[i]], noise_mod->head.num_x, num_chan); 
				goto exit_error;
			}
			dptr= (float *)id_get_ptr(thres_mod, first_chan, i, 0);
			sigma_ptr = (float *)(thres_mod->data);
			for (chan=first_chan; chan <= last_chan; chan++)
			{
				if (ngpd_read_diff_trigger(path, chan, &diff_trig) < 0)
				{
					sv_printf(cn, "! ERROR extracting differentiL trigger setup on chan %d\n", chan);
					goto exit_error;
				}
				if (thres_type[i] == ScaleSigma)
				{
					sptr= (float *)id_get_ptr(noise_mod, chan, HIST_RES_STDEV, 0);
					if (i == NGPD_THRES_DIFF_TRIG)
						sigma_ptr[chan+num_chan*NGPD_THRES_DIFF_TRIG_SIGMA] = *sptr;
				}
				else
					sptr= (float *)id_get_ptr(noise_mod, chan, HIST_RES_FRAC1, 0);
				thres = thres_val[i] * *sptr;
				*dptr++ = thres;
				switch (i)
				{
				case NGPD_THRES_DIFF_TRIG:
					diff_trig.thres = thres;
					break;
				}
				if (ngpd_write_diff_trigger(path, chan, &diff_trig) < 0)
				{
					sv_printf(cn, "! ERROR writing differential trigger setup on chan %d\n", chan);
					goto exit_error;
				}
			}
			munlink(noise_head);
			noise_mod = NULL;
		}
	}
	sv_printf(cn, "# Chan");
	for (i=first_thres;i<=last_thres; i++)
	{
		if (thres_type[i] != Unchanged)
			sv_printf(cn, "\t%s", thres_name[i]);
	}
	sv_printf(cn, "\n");

	for (chan=first_chan; chan <= last_chan; chan++)
	{
		sv_printf(cn, "# %d", chan);
		for (i=first_thres;i<=last_thres; i++)
		{
			dptr= (float *)(thres_mod->data) + num_chan*i + chan;
			if (thres_type[i] != Unchanged)
				sv_printf(cn, "\t%d", (int) (*dptr));
		}
		sv_printf(cn, "\n");
	}
	munlink(thres_head);

	return 0;

exit_error:
	if (thres_mod != NULL)
		munlink(thres_head);

	if (noise_mod != NULL)
		munlink(noise_head);
	return -1;
}

