/* 	
	ngpd_cal_triggers.c
	ngpd calibration library
	Created: wih73   11/08/2022
	Derived from da.server ngpd_calib_triggers.c
*/
/* Calibration of trigger threshold by measuring/calculating noise on differentials.
The issue is getting "clean" examples of noise with no events. Rely in the user setting up with no source.
Then can differentiate , histogram, std dev etc.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
 
#include "datamod.h"
#include "ngpd.h"
#include "ngpd_cal.h"

extern char ngpd_cal_error_message[NGPD_MAX_ERROR_MESSAGE+2];
int (*ngpd_cal_test_quit)();

int app_ngpd_measure_diff_hist (int cn, int path, int diff_num, double fraction, int decades, int num_settings, int mask_0_diff);


#define NGPD_CAL_DIFF_VARY_SEP 100
#define SCALE_ADC_UNITS	2

static char * hist_names[NGPD_CAL_MAX_DIFFS] = { "hist_diff_trig", };
static char * noise_names[NGPD_CAL_MAX_DIFFS] = {"noise_diff_trig"};

/**
 * @ingroup calibration
 * Build a structure with default settings to be passed to {@link ngpd_cal_hist_diffs()}.
 * The settings structure is created with a call to this function. The default settings can then be changed by the calling code 
 * before this pointer is passed to {@link ngpd_cal_hist_diffs()}.
 * @n
 * Afterwards the structure is freed by calling {@link ngpd_cal_hist_diffs_delete()}
 * @param path 	A handle to the top level of the ngpd system returned from any of the config functions  {@link ngpd_config_adq14} or {@link ngpd_config_ngzmp()}.
 * @return A pointer to the allocated structure or NULL on error, with error message available from ngpd_cal_get_error_message()
**/

NgpdCalHistDiffs * ngpd_cal_hist_diffs_default(int path)
{
	int num_chan;
	NgpdCalHistDiffs * set;
	int i;

	num_chan = ngpd_get_num_chan(path);
	if (num_chan <= 0)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs_default: Cannot read num_chan from path %d, %s", path, ngpd_get_error_message());
		return NULL;
	}
	set = (NgpdCalHistDiffs *)malloc(sizeof(NgpdCalHistDiffs));
	if (set == NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs_default: Cannot allocate space for settings");
		return NULL;
	}
	memset(set, 0, sizeof(NgpdCalHistDiffs));
	set->private = (NgpdCalHistDiffsPrivate *)malloc(sizeof(NgpdCalHistDiffsPrivate));
	if (set->private == NULL)
	{
		free(set);
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs_default: Cannot allocate space for private settings");
		return NULL;
	}
	memset(set->private, 0, sizeof(NgpdCalHistDiffsPrivate));
	set->private->glob_progress_total = 1;
	set->private->partial_progress_total = 1;
	set->path = path;
	set->num_chan = num_chan;
	set->first_chan = 0;
	set->last_chan = num_chan-1;
	set->chunk_size = 1024;
	set->num_pass = 1;
	set->persist_num_y = 1000;
	set->col_sat = 0;
	set->hist_num_x = 1024;
	set->diff_trig_from_hw=1;
	for (i=0; i<NGPD_CAL_MAX_DIFFS; i++)
	{
		set->diff_sep[i] = -1;
		set->errant_thres[i] = -1;
	}
	set->mode = Normal;
	return set;
}
/**
 * @ingroup calibration
 * Free a structure of settings built by {@link ngpd_cal_hist_diffs_default()}
 * @param set 	Pointer to NgpdCalHistDiffs created by {@link ngpd_cal_hist_diffs_default()}
 * @return none
 */
void ngpd_cal_hist_diffs_delete(NgpdCalHistDiffs *set)
{
	free(set->private);
	free(set);
}

int ngpd_cal_hist_diffs_validate(NgpdCalHistDiffs *set)
{
	int i;
	if (set->col_sat < 0)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: expected colour saturation control > 0, not %d", set->col_sat);
		return -1;
	}
	if (set->first_chan < 0 || set->first_chan>= set->num_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: expected %d <= first_chan <= %d , not %d", 0, set->num_chan-1, set->first_chan);
		return -1;
	}
	if (set->last_chan < 0 || set->last_chan >= set->num_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: expected %d <= last <= %d , not %d", 0, set->num_chan-1, set->last_chan);
		return -1;
	}
	if (set->last_chan < set->first_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: Expected first_chan <=last_chan but first_chan=%d, last_chan = %d", set->first_chan, set->last_chan);
		return -1;
	}
	if (set->chunk_size < 100 || set->chunk_size > 65536)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: Expected chunk_size in range 100..65536 not %d\n", set->chunk_size);
		return -1;
	}
		
	if (set->persist_errant)
	{
		if (set->errant_thres[0] <= 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: If using persist_errant, please specify errant_thres > 0, not %d", set->errant_thres[0]);
			return -1;
		}
		if (set->persist_name[0] == 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: If using persist_errant, please specifiy  and persistance module name for errant hists");
			return -1;
		}
	}
	if (set->mode == VarySep)
	{
		for (i=0;i<NGPD_CAL_MAX_DIFFS; i++)
		{
			if (set->diff_sep[i] < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: If using mode=VarySep, please specifiy all diff_sep elements, note diff_sep[%d]=%d",
						i, set->diff_sep[i] );
				return -1;
			}
		}
	}	
	if (set->playback != NULL && set->user_service == NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: Playback command required but user_service function not specified");
		return -1;
	}				

	return 0;
}

/**
 * @ingroup calibration
 * Run the system in scope mode collecting data as seen by the differential trigger, after the filter, calculate differences and then histogram them to assess noise levels.
 * The data is collected from the output of the differential trigger. At this point the FIR input filter has been applied to the data. Differences between values a specified number 
 * of samples are formed to emulate the numerical differentiation. These differences, which will be compared to the event trigger, are histogrammed.
 * Usually this done with no source of radiation so the histogram gives a measure of the noise as seen by the event trigger. The histograms can be measured using ngpd_cal_measure_diff_hist().
 * The results of the measurement can be used to set the trigger threshold using ngpd_cal_adjust_trigger_thresholds().
 * To use the function, first create a set of default settings using ngpd_cal_hist_diffs_default(). Then adjust the settings as required before calling this function.
 * Finally call ngpd_cal_hist_diffs_delete() to free the allocated memory.
 * The function can be used in 2 mode, Normal Mode (default) or VarySep mode.
 * In normal mode the difference are formed for the trigger engine. For the current design, this is a single first differential, though the provision for more than one differential is 
 * provided for future expansion. Using the default settings, diff_trig_from_hw=1 so the current separation between the two samples is read from the trigger firmware.
 * This can be over-ridden by setting diff_trig_from_hw=1 and then setting diff_sep[NGPD_CAL_DIFF_TRIG] as required. Either  way the results will be stored into /dev/shm/hist_diff_trig
 * To compare the effect on noise performance or using different separations, set mode=VarySep and filling diff_sep[0...NGPD_CAL_MAX_DIFFS-1] with separations to test. 
 * The data will be stored into /dev/shm/hist_diff_sep<set->diff_sep[i]>
 * The data is stored in data modules with the x axis (of size 0..hist_num_x-1) being the differences offset so difference 0 is at middle bin.
 * The data module row (y) is the channel number.
 * This function usually runs just one burst of scope mode, though set->num_pass can be increased to increase this to improve statistics.
 
 * @param set 	A pointer to the settings of type {@link NgpdCalHistDiffs}
 * @return		Returns 0 on success or -ve on error with error message available from ngpd_cal_get_error_message()
 */
int ngpd_cal_hist_diffs (NgpdCalHistDiffs *set)
{
	NGPDScopeModule * scope_mod;
	MOD_IMAGE *hist_mod[NGPD_CAL_MAX_DIFFS];
	mh_com *hist_head[NGPD_CAL_MAX_DIFFS];
	MOD_IMAGE3D *persist_mod[NGPD_MAX_CHANS];
	mh_com *persist_head[NGPD_MAX_CHANS];
	int num_cards;
	char mod_name[NGPD_MAX_MODNAME+20];
	int i, j, chan, card, stream;
	int *iptr, *hist_ptr;
	float  lhs=0.0;
	int pass;
	int persist_baseline;
	int chunk_start;
	int num_chunks, num_good[NGPD_MAX_CHANS];
	int diff_num;
	int diff, off_diff;
	int errant;
	int chans_per_card;
	int rc=-1;
	u_int16_t *ana_base, *ana_ptr;
	int save_flags, flags;
	NGPDDiffTrigger diff_trig;
	int scope_inc;
	int first_stream, last_stream;
	int trig_offset, wide_a_offset, wide_b_offset, neutron_offset;
	u_int32_t scope_status;

	num_cards = ngpd_get_num_cards(set->path);
	chans_per_card = ngpd_get_chans_per_card(set->path);
	scope_mod = ngpd_scope_get_mod(set->path);
	if (scope_mod == NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: Scope data module not available : %s", ngpd_get_error_message());
		return -1;
	}

	if (set->diff_trig_from_hw)
	{
		/* Scan all the channel to see if any have otd-fast enabled */
		for (chan=set->first_chan; chan<=set->last_chan; chan++)
		{
			if (ngpd_read_diff_trigger(set->path, chan, &diff_trig) < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: ERROR extracting Diff trigger setup on chan %d", chan);
				return -1;
			}
			if (chan == set->first_chan)
			{
				set->diff_sep[NGPD_CAL_DIFF_TRIG] = diff_trig.sep;
			}
			else
			{
				if (set->diff_sep[NGPD_CAL_DIFF_TRIG] != diff_trig.sep)
				{
					snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: inconsistent trigger filter setting between channel %d and %d. Work in blocks of channels if necessary\n", set->first_chan, chan);
					return -1;
				}
			}
		}
	}
	for (i=0;i<NGPD_MAX_CHANS; i++)
		persist_mod[i] = NULL;

	for (i=0;i<NGPD_CAL_MAX_DIFFS; i++)
		hist_mod[i] = NULL;

	if (set->persist_name[0] != 0)
	{
		for (i=set->first_chan;i<=set->last_chan; i++)
		{
			sprintf(mod_name, "%s_%02d", set->persist_name, i);
			persist_mod[i] = id_mkmod3d_err_msg (mod_name, set->chunk_size, set->persist_num_y, 1, "Time", "ADC Value", "Unused", NULL,  DATA_LONG, &(persist_head[i]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
			if (persist_mod[i] == NULL)
				goto exit_error;
			if (!set->no_clear)
			{
				id_clear_mod((MOD_IMAGE *)persist_mod[i]);
			}
		}
	}
	for (i=0; i < NGPD_CAL_MAX_DIFFS; i++)
	{
		if (set->diff_sep[i] > 0)
		{
			switch (set->mode)
			{
			case Normal:
				strcpy(mod_name, hist_names[i]);
				break;
			case VarySep:
				sprintf(mod_name, "hist_diff_sep%04d", set->diff_sep[i]);
				break;
			}
			hist_mod[i] = id_mkmod_err_msg (mod_name, set->hist_num_x, set->num_chan, "ADC Value (2 ADU/bin)", "Channel",  DATA_LONG, &(hist_head[i]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
			if (hist_mod[i] == NULL)
				goto exit_error;
		}
	}

	for (i=0; i<NGPD_CAL_MAX_DIFFS; i++)
	{
		if (hist_mod[i] != NULL && !set->no_clear)
		{
			for (chan=set->first_chan; chan<=set->last_chan; chan++)
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

	save_flags = ngpd_get_run_flags(set->path);
	flags = (save_flags & ~NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS) | NGPD_RUN_FLAGS_SCOPEMODE | NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD;
	ngpd_set_run_flags(set->path, flags);
	/* Global progress split into Run scope mode, read data, process data so 3*num_pass */
	__atomic_store_n(&set->private->glob_progress_total, set->num_pass*3, __ATOMIC_SEQ_CST);

	for (pass = 0; pass < set->num_pass; pass++)
	{
		__atomic_store_n(&set->private->glob_progress_now, 3*pass, __ATOMIC_SEQ_CST);
		__atomic_store_n(&set->private->partial_progress_now, 0, __ATOMIC_SEQ_CST);
		if ((rc=ngpd_cal_setup_all_chan(set->path, -1, NGZMP_SCOPE_SEL_ANA_DIFF_TRIG_OUT,	&first_stream, &last_stream, &trig_offset, &wide_a_offset, &wide_b_offset, &neutron_offset, 1)) < 0)
			goto exit_error;

		if (set->playback != NULL)
		{
			char cmd[2048];
			sprintf(cmd, set->playback, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass);
			if (set->user_service(set->cn, cmd, 0, NULL, NULL) < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: Cannot run playback command '%s'", cmd);
				rc = -1;
				goto exit_error;
			}
		}
		rc = ngpd_dma_system_start(set->path, -1, 0, 0, NULL);
		if (rc < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: Error starting system: %s", ngpd_get_error_message());
			goto exit_error;
		}
		rc = ngpd_dma_wait_scope(set->path, -1, &scope_status);
		if (rc < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: ERROR waiting for scope DMAs %s\n", ngpd_get_error_message());
			goto exit_error;
		}
		if (scope_status != 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: Scope status appears wrong at end of run = %08X\n", scope_status);
			goto exit_error;
		}
			
		__atomic_store_n(&set->private->glob_progress_now, 3*pass+1, __ATOMIC_SEQ_CST);
		__atomic_store_n(&set->private->partial_progress_now, 0, __ATOMIC_SEQ_CST);
		rc = ngpd_dma_read_scope(set->path, -1, 0, 0, 0);	// Read all cards to allow library to multi-thread if required
		if (rc < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs: ERROR reading scope data from all cards : %s\n", ngpd_get_error_message());
			goto exit_error;
		}
		chunk_start = 0; /* The testing of chunks by rhs-lhs is no longer valid. For now assume that a chunk_size is OK and use it */
		__atomic_store_n(&set->private->glob_progress_now, 3*pass+2, __ATOMIC_SEQ_CST);
		__atomic_store_n(&set->private->partial_progress_total, scope_mod->head.num_t, __ATOMIC_SEQ_CST);
		for (i=0; i<scope_mod->head.num_t; i++)
		{
			__atomic_store_n(&set->private->partial_progress_now, i, __ATOMIC_SEQ_CST);
			if (chunk_start >= 0)
			{
				if (i - chunk_start == set->chunk_size)
				{
					/* We have a chunk_size to use */
					num_chunks++;
					for (card=0; card < num_cards; card++)
					{
						for (stream=first_stream; stream<=last_stream; stream++)
						{
							chan = card*chans_per_card+stream-first_stream;
							if (chan < set->first_chan || chan > set->last_chan)
								continue;
							scope_inc = ngpd_scope_mod_get_inc(scope_mod, stream); // Moved to after ngpd_calib_setup_inp_all_resets
							ana_base = ngpd_scope_mod_get_ptr(scope_mod, card, stream);
							ana_ptr = ana_base + scope_inc*chunk_start;

							num_good[chan]++;
							errant = 0;
							for (diff_num=0; diff_num<NGPD_CAL_MAX_DIFFS; diff_num++)
							{
								int sep=set->diff_sep[diff_num];
								int end;
								if (sep <0)
									continue;
								hist_ptr= (int *)(hist_mod[diff_num]->data)+hist_mod[diff_num]->head.num_x*chan;

								end = set->chunk_size-sep;

								ana_ptr = ana_base + scope_inc*chunk_start;
								for (j=0; j< end; j++)
								{
									diff = ana_ptr[scope_inc*sep] - ana_ptr[0];
									if (set->errant_thres[diff_num] > 0 && abs(diff) > set->errant_thres[diff_num])
										errant++;
									ana_ptr += scope_inc;

									off_diff = diff + set->hist_num_x/2;
									if (off_diff <0) off_diff = 0;
									if (off_diff >= set->hist_num_x)
										off_diff = set->hist_num_x-1;
									hist_ptr[off_diff]++;
								}
							}
							if ((set->persist_name[0] && !set->persist_errant) || (set->persist_name[0] && set->persist_errant && errant))
							{
								persist_baseline = lhs - set->persist_num_y/2;
								ngpd_cal_accumulate_persist(ana_base, NULL, chunk_start, persist_mod[chan], 0, 0, set->chunk_size, persist_baseline, 32000/set->persist_num_y, set->col_sat, 0, 0, 0, scope_inc, 0);
							}
						} /* End For stream */
					} /* End For card */
					chunk_start = i;
				} /* If usable chunk_size */
			} /* End if have Valid chunk_size start */
			if (ngpd_cal_test_quit != NULL && ngpd_cal_test_quit())
			{
				printf("STOPPED By ^c\n");
				goto exit_error;
			}
		} /* End For i scanning for reset free chunks */
	} /* End For Pass */
	ngpd_set_run_flags(set->path, save_flags);

	/*
	possible_chunks = scope_mod->head.num_t/chunk_size;
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
	possible_chunks = scope_mod->head.num_t/chunk_size;
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
	rc = 0;

exit_error:
	for (i=0; i<NGPD_MAX_CHANS; i++)
	{
		if (persist_mod[i] != NULL)
			munlink(persist_head[i]);
	}
	for (i=0; i<NGPD_CAL_MAX_DIFFS; i++)
	{
		if (hist_mod[i] != NULL)
			munlink(hist_head[i]);
	}
	return rc;
}
/**
 * Calculate and return the progress being made by a background worker thread performing ngpd_cal_hist_diffs()
 @param set		Pointer to the NgpdCalHistDiffs used to start the worker
 @return A double 0 -> 1.0 as a measures of progress.
 */

double ngpd_cal_hist_diffs_progress(NgpdCalHistDiffs *set)
{
	double progress;
	progress = (double)__atomic_load_n (&set->private->glob_progress_now, __ATOMIC_SEQ_CST)/(double)__atomic_load_n (&set->private->glob_progress_total, __ATOMIC_SEQ_CST);
	progress += (double)__atomic_load_n (&set->private->partial_progress_now, __ATOMIC_SEQ_CST)/
			((double)__atomic_load_n (&set->private->partial_progress_total, __ATOMIC_SEQ_CST)*(double)__atomic_load_n (&set->private->glob_progress_total, __ATOMIC_SEQ_CST));
	return progress;
}

/**
 * @ingroup calibration
 * Start a background thread to run ngpd_cal_hist_diffs(). 
 * Progress can be monitored using ngpd_cal_hist_diffs_progress() and then thread joined using ngpd_cal_hist_diffs_join()
 * @param set	Settings 
*/
int ngpd_cal_hist_diffs_start(NgpdCalHistDiffs *set)
{	
	int rc;
	rc = pthread_create(&set->private->control_thread, NULL, (void * (*)(void *))ngpd_cal_hist_diffs, set);
	if (rc < 0)
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs_start: Cannot start thread, rc=%d\n", rc);
	return rc;
}

/**
 * @ingroup calibration
 * Wait for the background thread running ngpd_cal_hist_diffs() to terminate and join thread
 * Progress before this can be monitored using ngpd_cal_hist_diffs_progress()
 * @param set	Settings 
*/
int ngpd_cal_hist_diffs_join(NgpdCalHistDiffs *set)
{	
	int rc;
	void *ret_void;

	rc = pthread_join(set->private->control_thread, &ret_void);
	if (rc < 0)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs_join: Cannot join thread, rc=%d\n", rc);
		return -1;
	}
	else if (ret_void != NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_hist_diffs_join: worker thread returned error=%p\n", ret_void);
		return (int)ret_void;
	}
	return 0;
}


/**
 * @ingroup calibration
 * Measure the difference histogram formed by ngpd_cal_hist_diffs() to assess the noise levels.
 *	
 * The data is read form data modules in /dev/shm. The name is inferred from the diff_num which either identifies which differential within the trigger engine 
 * or specifies NGPD_CAL_DIFF_VARY_SEP. 
 * The histogram is processed to form the mean and standard deviation from the weighted sum.
 * The cumulative sum is also formed to find the threshold to give threshold which noise triggers on a specified fraction of samples. 
 * This is repeated for 10, 100, 1000 times less likely using the decades parameter.
 * If the ADC input saturates for some of the time, there can be long runs of 0 differences, which cause a peak in the histogram are probably not from valid data. 
 * The mask_0_diff parameter specifies that this is masked out.
 * When ngpd_cal_hist_diffs has been used in mod=Normal and diff_num=NGPD_CAL_DIFF_TRIG, the results of the analysis are stored in a data module noise_diff_trig.
 * The layout of the module is defined by {@link NGPD_CAL_HIST_MOD_MACROS} including up to NGPD_CAL_MAX_DECADES of fraction, fraction/10, fraction/100 ...
 * Using VarySep mode and diff_num==NGPD_CAL_DIFF_VARY_SEP the data module vary_sep_noise is created storing just the standard deviation and threshold for fraction results, 
 * for each of the separations tested.
 * @param path 			A handle to the top level of the ngpd system returned from any of the config functions  {@link ngpd_config_adq14} or {@link ngpd_config_ngzmp()}.
 * @param diff_num		When ngpd_cal_hist_diffs has been used in mode Normal, this is the code to identify the differential with the trigger engine. Currently only NGPD_CAL_DIFF_TRIG.
 *						When used in VarySep mode, this should be set to NGPD_CAL_DIFF_VARY_SEP.
 * @param fraction  	Fraction (typically 0.001) used to calculate threshold for this fraction of samples to cause a noise trigger.
 * @param decades		Number of decades 0.. NGPD_CAL_MAX_DECADES to calculate threshold for from the cumulative sum. 
 * @param num_settings  Specifies the number of settings to test in VarSep mode, otherwise set a 1.
 * @param mask_0_diff	Enable masking of peaks at 0 difference which can be caused by long runs of under or over range ADC values.
 * @return		Returns 0 on success or -ve on error with error message available from ngpd_cal_get_error_message()
*/

int ngpd_cal_measure_diff_hist (int path, int diff_num, double fraction, int decades, int num_settings, int mask_0_diff)
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
	const char *cp;
	int setting;
	char mod_name[NGPD_MAX_MODNAME+20];
	int vary_sep_mode=0;
	int sep;
			
	num_chan = ngpd_get_num_chan(path);
	if (num_chan <= 0)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_diff_hist: Cannot read num_chan from path %d, %s", path, ngpd_get_error_message());
		return -1;
	}
	first_chan = 0;
	last_chan  = num_chan-1;
	if (diff_num == NGPD_CAL_DIFF_VARY_SEP)
	{
		diff_num = 0;
		vary_sep_mode = 1;
	}
	if (diff_num <0 || diff_num >= NGPD_CAL_MAX_DIFFS)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_diff_hist: Expected diff num in range 0 ... %d, not %d", diff_num, NGPD_CAL_MAX_DIFFS-1);
		return -1;
	}
	if (decades > NGPD_CAL_MAX_DECADES)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_diff_hist: Expected decades int range 0... %d, not %d", NGPD_CAL_MAX_DECADES, decades);
		return -1;
	}
	if (vary_sep_mode)
	{
		noise_mod = id_mkmod_err_msg ("vary_sep_noise", num_chan, 2*num_settings, "Chan", "Function",  DATA_FLOAT, &noise_head, ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
	}
	else
		noise_mod = id_mkmod_err_msg (noise_names[diff_num], num_chan, 2+NGPD_CAL_MAX_DECADES, "Chan", "Function",  DATA_FLOAT, &noise_head, ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
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
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_diff_hist: Cannot link to data module %s\n", cp);
			goto exit_error;
		}

		if (hist_mod->head.num_y != num_chan)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_diff_hist:  Module %s does not have 1 row per channel. num_chan=%d num_y=%d\n", cp, num_chan, hist_mod->head.num_y);
			goto exit_error;
		}

		fptr= (float *)id_get_ptr(noise_mod, first_chan, setting, 0);

		for (chan=first_chan; chan<=last_chan; chan++)
		{
			num_x = hist_mod->head.num_x;
			iptr = (u_int32_t *)id_get_ptr(hist_mod, 0, chan, 0);
			
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
			if (vary_sep_mode)
				fptr[0] = SCALE_ADC_UNITS*stdev;
			else
			{
				fptr[NGPD_CAL_HIST_RES_MEAN*num_chan]  = SCALE_ADC_UNITS*mean;
				fptr[NGPD_CAL_HIST_RES_STDEV*num_chan] = SCALE_ADC_UNITS*stdev;
			}
			printf("# Chan %d: Mean=%g, stdev=%g\n", chan, SCALE_ADC_UNITS*mean, SCALE_ADC_UNITS*stdev); 
			target = sum_f * fraction;
			for (j=0; j<decades; j++)
			{
				sum_f = 0;
				iptr = (u_int32_t *)hist_mod->data+chan*num_x+num_x-1;
				for (i=num_x; i > 0; i--)
				{
					sum_f += *iptr--;
					if (sum_f > target)
						break;
				}
				if (vary_sep_mode)
					fptr[num_chan * num_settings] = SCALE_ADC_UNITS*(i-num_x/2);
				else
					fptr[(NGPD_CAL_HIST_RES_FRAC1+j)*num_chan] = SCALE_ADC_UNITS*(i-num_x/2);
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

/**
 * @ingroup calibration
 * Build default settings to use with ngpd_cal_adjust_trigger_thresholds().
 * Afterwards the structure is freed by calling {@link ngpd_cal_adjust_trigger_thresholds_delete()}
 * @param path 	A handle to the top level of the ngpd system returned from any of the config functions  {@link ngpd_config_adq14} or {@link ngpd_config_ngzmp()}.
 * @return A pointer to the allocated structure or NULL on error, with error message available from ngpd_cal_get_error_message()
 */
NgpdCalAdjustTrigThres * ngpd_cal_adjust_trigger_thresholds_default(int path)
{
	int num_chan;
	NgpdCalAdjustTrigThres * set;
	
	num_chan = ngpd_get_num_chan(path);
	if (num_chan <=0)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_thresholds_default: Cannot read num_chan from path %d, %s", path, ngpd_get_error_message());
		return NULL;
	}
	set = (NgpdCalAdjustTrigThres *)malloc(sizeof(NgpdCalAdjustTrigThres));
	if (set == NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_thresholds_default: Cannot allocate space for settings");
		return NULL;
	}
	memset(set, 0, sizeof(NgpdCalAdjustTrigThres));
	set->path = path;
	set->num_chan = num_chan;
	set->first_chan = 0;
	set->last_chan = num_chan-1;	
	set->first_thres = NGPD_CAL_THRES_DIFF_TRIG;
	set->last_thres  = NGPD_CAL_THRES_DIFF_TRIG;
	set->thres_type[NGPD_CAL_THRES_DIFF_TRIG]   = ScaleSigma;
	set->thres_val[NGPD_CAL_THRES_DIFF_TRIG]    = 5.0;
	return set;
}
/**
 * @ingroup calibration
 * Free a structure of settings built by {@link ngpd_cal_adjust_trigger_thresholds_default()}
 * @param set 	Pointer to NgpdCalAdjustTrigThres created by {@link ngpd_cal_adjust_trigger_thresholds_default()}
 * @return none
 */
void ngpd_cal_adjust_trigger_thresholds_delete(NgpdCalAdjustTrigThres *set)
{
	free(set);
}
/**
 * @ingroup internal
 * Validate the setting in the data structure of type {@link NgpdCalAdjustTrigThres}.
 * This is called by {@link ngpd_cal_adjust_trigger_thresholds()} but could be called by the user code first if required.
 * @param set 	A pointer to the settings of type {@link NgpdCalAdjustTrigThres}
 * @return 0 on success, -1 on error with error message available from ngpd_cal_get_error_message().
 */

int ngpd_cal_adjust_trigger_thresholds_validate(NgpdCalAdjustTrigThres *set)
{
	if (set->first_chan < 0 || set->first_chan>= set->num_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_thresholds: expected %d <= first_chan <= %d , not %d", 0, set->num_chan-1, set->first_chan);
		return -1;
	}
	if (set->last_chan < 0 || set->last_chan >= set->num_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_thresholds: expected %d <= last <= %d , not %d", 0, set->num_chan-1, set->last_chan);
		return -1;
	}
	if (set->last_chan < set->first_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_thresholds: Expected first_chan <=last_chan but first_chan=%d, last_chan = %d", set->first_chan, set->last_chan);
		return -1;
	}
	return 0;
}

/**
 * @ingroup calibration
 * Automatically set the trigger threshold per channel on the basis of the noise level measured from ngpd_cal_hist_diffs.
 * First use ngpd_cal_hist_diffs to measure the histogram of teh differences seen at the required differential separation, with no input radiation.
 * Then create default settings with ngpd_cal_adjust_trigger_thresholds_default and adjust as required.
 * Then call ngpd_cal_adjust_trigger_thresholds(). This will call ngpd_cal_measure_diff_hist() to measure the noise and then set the trigger thresholds from the results.
 * The threshold can either from the standard deviation of the noise * a constant using thres_type=ScaleSigma ar to trigger on a specified (very small) fraction of samples using
 * thres_type==Fraction. Using ScaleSigma, thres_val is the scaling, typically 5.0. Using Fraction thres_val is thre fraction of samples, typically 0.0001 or less.
 * The resulting thresholds are stored in ngpd_thresholds.
 * @param set 	A pointer to the settings of type {@link NgpdCalAdjustTrigThres}
 * @return 0 on success, -1 on error with error message available from ngpd_cal_get_error_message().
 */

int ngpd_cal_adjust_trigger_thresholds(NgpdCalAdjustTrigThres *set)
{
	MOD_IMAGE *noise_mod=NULL;
	mh_com *noise_head;
	MOD_IMAGE *thres_mod=NULL;
	mh_com *thres_head;
    u_int16 attr_rev = mkattrevs(MA_REENT,1);
    u_int16 type_lang = mktypelang(MT_DATA, ML_ANY);
	const char *cp;
	int i, j;
	int chan;
	float *sptr, *dptr, *sigma_ptr;
	int thres;
	double fraction;
	const char *thres_name[] = {"DiffTrig"};
	NGPDDiffTrigger diff_trig;

	if (ngpd_cal_adjust_trigger_thresholds_validate(set) < 0)
		return -1;

	thres_mod = id_mkmod_err_msg ("ngpd_thresholds", set->num_chan, NGPD_CAL_NUM_THRES+NGPD_CAL_NUM_SIGMA, "Chan", "Function",  DATA_FLOAT, &thres_head, 
									ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
	if (thres_mod == NULL)
		goto exit_error;

	if (!set->no_clear)
	{
		for (chan=set->first_chan;chan<=set->last_chan; chan++)
		{
			for (j=0; j<thres_mod->head.num_y; j++)
			{
				dptr = (float *)id_get_ptr(thres_mod, chan, j, 0);
				*dptr = 0;
			}
		}
	}

	for (i=set->first_thres;i<=set->last_thres; i++)
	{
		if (set->thres_type[i] != Unchanged)
		{
			int which_diff[NGPD_CAL_NUM_THRES] = { NGPD_CAL_DIFF_TRIG};

			if (set->thres_type[i] == Fraction)
				fraction = set->thres_val[i];
			else
				fraction = 0.001;

			if (ngpd_cal_measure_diff_hist(set->path, which_diff[i], fraction, 3, 1, 1) < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger: Problem measuring histogram. Run hist-diffs first");
				goto exit_error;
			}

			cp = noise_names[which_diff[i]];
		    if (_os_link(&cp, &noise_head, (void **)&noise_mod, &type_lang, &attr_rev))
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger: Cannot link to data module %s", noise_names[which_diff[i]]);
				goto exit_error;
			}
			if (noise_mod->head.num_x != set->num_chan)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger: Noise module %s width %d does not match num_chan=%d",
								noise_names[which_diff[i]], noise_mod->head.num_x, set->num_chan); 
				goto exit_error;
			}
			dptr= (float *)id_get_ptr(thres_mod, set->first_chan, i, 0);
			sigma_ptr = (float *)(thres_mod->data);
			for (chan=set->first_chan; chan <= set->last_chan; chan++)
			{
				if (ngpd_read_diff_trigger(set->path, chan, &diff_trig) < 0)
				{
					snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger: ERROR extracting differential trigger setup on chan %d", chan);
					goto exit_error;
				}
				if (set->thres_type[i] == ScaleSigma)
				{
					sptr= (float *)id_get_ptr(noise_mod, chan, NGPD_CAL_HIST_RES_STDEV, 0);
					if (i == NGPD_CAL_THRES_DIFF_TRIG)
						sigma_ptr[chan+set->num_chan*NGPD_CAL_THRES_DIFF_TRIG_SIGMA] = *sptr;
				}
				else
					sptr= (float *)id_get_ptr(noise_mod, chan, NGPD_CAL_HIST_RES_FRAC1, 0);
				thres = set->thres_val[i] * *sptr;
				*dptr++ = thres;
				switch (i)
				{
				case NGPD_CAL_THRES_DIFF_TRIG:
					diff_trig.thres = thres;
					break;
				}
				if (ngpd_write_diff_trigger(set->path, chan, &diff_trig) < 0)
				{
					snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger: ERROR writing differential trigger setup on chan %d", chan);
					goto exit_error;
				}
			}
			munlink(noise_head);
			noise_mod = NULL;
		}
	}
	printf("# Chan");
	for (i=set->first_thres;i<=set->last_thres; i++)
	{
		if (set->thres_type[i] != Unchanged)
			printf("\t%s", thres_name[i]);
	}
	printf("\n");

	for (chan=set->first_chan; chan <= set->last_chan; chan++)
	{
		printf("# %d", chan);
		for (i=set->first_thres;i<=set->last_thres; i++)
		{
			dptr= (float *)(thres_mod->data) + set->num_chan*i + chan;
			if (set->thres_type[i] != Unchanged)
				printf("\t%d", (int) (*dptr));
		}
		printf("\n");
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

