/* 	
	ngpd_cal_triggers.c
	ngpd calibration library
	Created: wih73   11/08/2022
	Derived from da.server ngpd_calib_triggers.c
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


//int ngpd_measure_event_baseline(int * wspace, int i, int pre, int post, X3FloatAve * baseline, X3FloatAve *lhs, X3FloatAve *rhs, int num_lhs, int num_rhs);
int ngpd_cal_measure_pulse(int16_t * ana_ptr, int offset, int pre, int post, double *lhs, double *rhs, double *height, int num_lhs, int num_rhs, int ana_inc, int dig_inc);
int ngpd_cal_divide_average(MOD_IMAGE3D *mod, int row, int t, int n);
int ndpg_cal_accumulate_xtk(NGPDMeasureTrigger *set, NGPDMeasureTriggerThread *thread_specific, int do_upper, int src, int offset, int mod_t);

int (*ngpd_cal_test_quit)();


#define ADC_SCALE 2


void *ngpd_measure_trigger_do(void *set_vp);

static void ngpd_cal_measure_trigger_unlink(NGPDMeasureTrigger *set)
{
	int i, phase;
		
	for (phase=0; phase<NGPD_CAL_MAX_PHASES; phase++)
	{
		for (i=0; i<NGPD_MAX_CHANS; i++)
		{
			if (set->private->persist_mod[phase][i] != NULL)
			{
				munlink(set->private->persist_head[phase][i]);
				set->private->persist_mod[phase][i] = NULL;
			}
			if (set->private->ave_mod[phase][i] != NULL)
			{
				munlink(set->private->ave_head[phase][i]);
				set->private->ave_mod[phase][i] = NULL;
			}
		}
		if (set->private->num_ave_mod[phase] != NULL)
		{
			munlink(set->private->num_ave_head[phase]);
			set->private->num_ave_mod[phase] = NULL;
		}
		if (set->private->mca_mod[phase] != NULL)
		{
			munlink(set->private->mca_head[phase]);
			set->private->mca_mod[phase] = NULL;
		}
		if (set->private->xtk_all_mod[phase] != NULL)
		{
			munlink(set->private->xtk_all_head[phase]);
			set->private->xtk_all_mod[phase] = NULL;
		}
		if (set->private->xtk_neutron_mod[phase] != NULL)
		{
			munlink(set->private->xtk_neutron_head[phase]);
			set->private->xtk_neutron_mod[phase] = NULL;
		}
		if (set->private->xtk_gamma_mod[phase] != NULL)
		{
			munlink(set->private->xtk_gamma_head[phase]);
			set->private->xtk_gamma_mod[phase] = NULL;
		}
		if (set->private->num_ave_xtk_mod[phase] != NULL)
		{
			munlink(set->private->num_ave_xtk_head[phase]);
			set->private->num_ave_xtk_mod[phase] = NULL;
		}
	}
}

/**
 * @ingroup calibration
 * Build a structure with default settings to be passed to {@link ngpd_cal_measure_trigger()}.
 * The settings structure is created with a call to this function. The default settings can then be changed by the calling code 
 * before this pointer is passed to {@link ngpd_cal_measure_trigger()}.
 * @n
 * Afterwards the structure is freed by calling {@link ngpd_cal_measure_trigger_delete()}
 * @param path 	A handle to the top level of the ngpd system returned from any of the config functions  {@link ngpd_config_adq14} or {@link ngpd_config_ngzmp()}.
 * @return A pointer to the allocated structure or NULL on error, with error message available from ngpd_cal_get_error_message()
**/

NGPDMeasureTrigger *ngpd_cal_measure_trigger_default(int path)
{
	NGPDMeasureTrigger *set;
	int i;
	int num_chan;

	num_chan = ngpd_get_num_chan(path);
	if (num_chan <= 0)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger_default: Cannot read num_chan from path %d, %s", path, ngpd_get_error_message());
		return NULL;
	}

	if ((set=(NGPDMeasureTrigger *)malloc(sizeof(NGPDMeasureTrigger))) == NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger_default: Cannot malloc space for setup");
		return NULL;
	}
	
	memset(set, 0, sizeof(NGPDMeasureTrigger));
	if ((set->min_event = (double*)malloc(sizeof(double)*NGPD_MAX_CHANS)) == NULL)
		goto exit_error;
	if ((set->max_event = (double*)malloc(sizeof(double)*NGPD_MAX_CHANS)) == NULL)
		goto exit_error;
	if ((set->min_hgt_neutron = (double*)malloc(sizeof(double)*NGPD_MAX_CHANS)) == NULL)
		goto exit_error;
	if ((set->max_hgt_neutron = (double*)malloc(sizeof(double)*NGPD_MAX_CHANS)) == NULL)
		goto exit_error;
	if ((set->private=(NGPDMeasureTriggerPrivate*)malloc(sizeof(NGPDMeasureTriggerPrivate))) == NULL)
		goto exit_error;
	memset(set->private, 0, sizeof(NGPDMeasureTriggerPrivate));
	pthread_mutex_init(&set->private->mutex, NULL);
	pthread_cond_init(&set->private->cond_var, NULL);
	
	for (i=0; i<NGPD_MAX_NUM_CARDS; i++)
		set->private->thread_specific[i].parent = set;

	set->path = path;
	set->num_chan = num_chan;
	set->num_cards = ngpd_get_num_cards(path);
	set->private->chans_per_card = ngpd_get_chans_per_card(path);
	set->private->scope_mod = ngpd_scope_get_mod(path); 

	set->pre=25;
	set->post=170;
	set->num_pass=1;
	set->persist_num_y = 1200;
	set->num_phase=1;
	set->auto_neutron_height=0;
	set->mca_scale=1.0/16.0;		// Scale 0..65535 downto to 0..4095 */

	set->first_chan = 0;
	set->last_chan  = num_chan-1;
	for (i=0;i<NGPD_MAX_CHANS; i++)
	{
		set->min_event[i] = -1.0;
		set->max_event[i] = -1.0;
		set->min_hgt_neutron[i] = -1.0;
		set->max_hgt_neutron[i] = -1.0;
	}
	return set;
exit_error:
	free(set->private);
	free(set->min_event);
	free(set->max_event);
	free(set->min_hgt_neutron);
	free(set->max_hgt_neutron);
	free(set);
	return NULL;
}
/**
 * @ingroup calibration
 * Free a structure of settings built by {@link ngpd_cal_measure_trigger_default()}
 * @param set 	Pointer to NGPDMeasureTrigger created by {@link ngpd_cal_measure_trigger_default()}
 * @return none
 */
void ngpd_cal_measure_trigger_delete(NGPDMeasureTrigger *set)
{
	pthread_mutex_destroy(&set->private->mutex);
	pthread_cond_destroy(&set->private->cond_var);
	free(set->private);
	free(set->min_event);
	free(set->max_event);
	free(set->min_hgt_neutron);
	free(set->max_hgt_neutron);
	free(set);
}

static int ngpd_cal_measure_trigger_wait_state(NGPDMeasureTrigger *set, enum NGPDMeasureTriggerState state)
{
	int i, match;
	int rc;
	// printf("ngpd_cal_measure_trigger_wait_state(state=%d)\n", state);
	rc = pthread_mutex_lock(&set->private->mutex);
	if (rc != 0)
		return rc;
	while (1)
	{
		match = 1;
		for (i=set->private->glob_first_card; i<= set->private->glob_last_card; i++)
		{
			if (set->private->thread_specific[i].state != state)
			{
				match = 0;
				break;
			}
		}
		if (match)
			break;
		
		rc = pthread_cond_wait(&set->private->cond_var, &set->private->mutex);
		if (rc != 0)
			break;
			
	}
	pthread_mutex_unlock(&set->private->mutex);
	return rc;
}

static int ngpd_cal_measure_trigger_signal_state(NGPDMeasureTrigger *set, enum NGPDMeasureTriggerState state)
{
	int i;
	int rc;
	// printf("ngpd_cal_measure_trigger_signal_state(state=%d)\n", state);
	rc = pthread_mutex_lock(&set->private->mutex);
	if (rc != 0)
		return rc;
	for (i=set->private->glob_first_card; i<= set->private->glob_last_card; i++)
		set->private->thread_specific[i].state = state;
		
	rc = pthread_cond_broadcast(&set->private->cond_var);
	pthread_mutex_unlock(&set->private->mutex);
	return rc;
}


/**
 * @ingroup calibration
 * Run the system in scope mode searching for pulses detected by the firmware event trigger and accumulate the data.
 * The data is collected usually from the output of the differential trigger. The shape of the event and of the 3 timing signals are collected and averaged. 
 * The averaged data can be viewed for debug and are used by {@link ngpd_cal_tail_subtract()} to calculate the tail subtraction data when this feature is enabled 
 * and by {@link ngpd_cal_adjust_trigger_timing()} to adjust the setting of the firmware trigger so the firmware trigger so that the digital signals are aligned correctly with the 
 * digital representation of the analogue data.
 * 
 * Optionally data can be collected from the pulse measurement block of the FPGA which can use the tail subtracted data. See the use_measure field in the settings.
 * The output data is stored in data modules in /dev/shm, formatted so they can be viewed using imgd or linked to from other applications.
 * The function can be used to collect all events together or to try to separate neutrons and gammas on the basis of the pulse height only.
 * 
 * The settings are determined by creating default settings with {@link ngpd_cal_measure_trigger_default()}.
 * This sets the path to the specified value and determines the number of channels in the system. 
 * The fields first_chan and last_chan are set so by default all channels are processed, 
 * though these can be changed to limit the range.
 * 
 * Usually the averaged data will be required. Specify the root of the data module name in ave_name. The function will append _phase<phase_num>_<chan_num> to make the module names.
 * Also set do_ave=1. The data module is arranged as a 3-d data module with the x axis being time (sample), the y or row being the event and trigger signals 
 * see {@link NGPD_CAL_SHAPE_MOD_MACROS} and the 3rd dimension 0=all events, 1=neutrons or 2=gammas.  
 * By default the function will average all events detected by the firmware. 
 * The fields min_event and max_event allow the range of events to be included in the average to be set on a per channel basis. 
 * It is often useful to exclude any grossly oversize saturated events from the averages. It also may be useful to exclude low level nusiance noise triggers using min_event.
 * By default all events types are averaged into the "all" plane of the data module, however if the min_hgt_neutron and max_hgt_neutron arrays are set, 
 * the function separates events into neutrons or others (gammas) on the basis of pulse height only. 
 * The  min_hgt_neutron and max_hgt_neutron fields can be specified by the calling code. In this case the data is processed in a single phase. 
 * Alternatively, if the auto_neutron_height field is set, the software runs in two phases. Teh first phase averages all event together by collects a MCA. It then automatically sets 
 * a window around the neutron peak pulse height for use in phase2, which separates neutrons and gammas.
 
 * The system can also collect 2-d plots which emulated persistence mode of an oscilloscope to aid debugging, allowing errant events to be seen. 
 * Set the root name in persist_name and set do_persist=1. The data modules are a 2-d plot with the horizontal axis time and the vertical axis voltage. 
 * The data modules are 3-d, with the 3rd dimension being 0=all events, 1=neutrons or 2=gammas.
 
 * @param set 	A pointer to the settings of type {@link NGPDMeasureTrigger}
 * @return		Returns 0 on success or -ve on error with error message available from ngpd_cal_get_error_message()
* */
int ngpd_cal_measure_trigger(NGPDMeasureTrigger *set)
{
	char mod_name[NGPD_MAX_MODNAME+20];
	int i, j, k, pass, phase, do_upper, chan, card;
	int flags;
	int total_passes = set->num_pass;
	int *iptr;
	int rc;
	char * ave_mod_labels[] = {"Signal", "Trigger", "WideA Meas", "WideB Base", "Fit", "Corrected", "Diff1", NULL };

	if (set->auto_neutron_height)
	{
		set->num_phase = 2;
		total_passes ++;
	}
	else
		set->num_phase = 1;		// If if the future there is some other need for more than 1 phase, this needs to be done differently.

	if (ngpd_cal_measure_trigger_validate(set))
		goto exit_error;

	for (phase=0; phase < set->num_phase; phase++)
	{
		sprintf(mod_name, "mca_phase%d_%d", phase+1, set->num_chan);
		set->private->mca_mod[phase] = id_mkmod3d_err_msg(mod_name, 4096, NGPD_CAL_MOD_NUM_T, set->num_chan, "ADC Value", "All/Neutron/Gamma", "Channel", 
				NULL, DATA_LONG, &(set->private->mca_head[phase]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
		if (set->private->mca_mod[phase] == NULL)
			goto exit_error;
		sprintf(set->private->mca_mod[phase]->head.x_label, "ADC Values/%g", 1.0/set->mca_scale);
		if (!set->no_clear)
			id_clear_mod((MOD_IMAGE *)set->private->mca_mod[phase]);

		if (set->do_persist)
		{
			for (i=set->first_chan;i<=set->last_chan; i++)
			{
				sprintf(mod_name, "%s_phase%d_%02d", set->persist_name, phase+1, i);
				set->private->persist_mod[phase][i] = id_mkmod3d_err_msg(mod_name, set->pre+set->post, set->persist_num_y, NGPD_CAL_MOD_NUM_T, "Time", "ADC Value", "All/Neutron/Gamma", 
					NULL, DATA_LONG, &(set->private->persist_head[phase][i]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
				if (set->private->persist_mod[phase][i] == NULL)
					goto exit_error;
				if (!set->no_clear)
					id_clear_mod((MOD_IMAGE *)(set->private->persist_mod[phase][i]));
			}
		}
		if (set->do_ave)
		{
			for (i=set->first_chan;i<=set->last_chan; i++)
			{
				sprintf(mod_name, "%s_phase%d_%02d", set->ave_name, phase+1, i);
				set->private->ave_mod[phase][i] = id_mkmod3d_err_msg(mod_name, set->pre+set->post, NGPD_CAL_MOD_NUM_ROW, NGPD_CAL_MOD_NUM_T, "Time", "ADC Value scaled 0...63353", "All/Neutron/Gamma", 
				 ave_mod_labels, DATA_DOUBLE, &(set->private->ave_head[phase][i]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
				if (set->private->ave_mod[phase][i] == NULL)
					goto exit_error;
				if (!set->no_clear)
					id_clear_mod((MOD_IMAGE *)(set->private->ave_mod[phase][i]));

			}
			sprintf(mod_name, "num_ave_phase%d_%d", phase+1, set->num_chan);
			set->private->num_ave_mod[phase] = id_mkmod_err_msg(mod_name, set->num_chan, NGPD_CAL_MOD_NUM_T, "Chan", "All/Neutron/Gamma",  DATA_LONG, &(set->private->num_ave_head[phase]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
			if (set->private->num_ave_mod[phase] == NULL)
				goto exit_error;
			if (!set->no_clear)
				id_clear_mod(set->private->num_ave_mod[phase]);
		}
		if (set->do_xtk)
		{
			sprintf(mod_name, "%s_phase%d_all%dx%d", set->xtk_name, phase+1, set->num_chan, set->num_chan);
			set->private->xtk_all_mod[phase] = id_mkmod3d_err_msg(mod_name, set->pre+set->post, set->num_chan, set->num_chan, "Time", "Dest Channel", "Src Channel",
				 NULL, DATA_DOUBLE, &(set->private->xtk_all_head[phase]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
			if (set->private->xtk_all_mod[phase] == NULL)
				goto exit_error;
			if (!set->no_clear)
				id_clear_mod((MOD_IMAGE *)(set->private->xtk_all_mod[phase]));

			sprintf(mod_name, "%s_phase%d_neutron%dx%d", set->xtk_name, phase+1, set->num_chan, set->num_chan);
			set->private->xtk_neutron_mod[phase] = id_mkmod3d_err_msg(mod_name, set->pre+set->post, set->num_chan, set->num_chan, "Time", "Dest Channel", "Src Channel",
				 NULL, DATA_DOUBLE, &(set->private->xtk_neutron_head[phase]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
			if (set->private->xtk_neutron_mod[phase] == NULL)
				goto exit_error;
			if (!set->no_clear)
				id_clear_mod((MOD_IMAGE *)(set->private->xtk_neutron_mod[phase]));

			sprintf(mod_name, "%s_phase%d_gamma%dx%d", set->xtk_name, phase+1, set->num_chan, set->num_chan);
			set->private->xtk_gamma_mod[phase] = id_mkmod3d_err_msg(mod_name, set->pre+set->post, set->num_chan, set->num_chan, "Time", "Dest Channel", "Src Channel",
				 NULL, DATA_DOUBLE, &(set->private->xtk_gamma_head[phase]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
			if (set->private->xtk_gamma_mod[phase] == NULL)
				goto exit_error;
			if (!set->no_clear)
				id_clear_mod((MOD_IMAGE *)(set->private->xtk_gamma_mod[phase]));

			sprintf(mod_name, "num_ave_xtk_phase%d_%d", phase+1, set->num_chan);
			set->private->num_ave_xtk_mod[phase] = id_mkmod3d_err_msg(mod_name, set->num_chan, set->num_chan, NGPD_CAL_MOD_NUM_T, "Dest Chan", "Src Chan", "All/Neutron/Gamma",
				NULL, DATA_LONG, &(set->private->num_ave_xtk_head[phase]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
			if (set->private->num_ave_xtk_mod[phase] == NULL)
				goto exit_error;
			if (!set->no_clear)
				id_clear_mod(set->private->num_ave_xtk_mod[phase]);
		}
	}


	set->private->save_flags = ngpd_get_run_flags(set->path);
	flags = (set->private->save_flags & ~NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS) | NGPD_RUN_FLAGS_SCOPEMODE | NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD;
	ngpd_set_run_flags(set->path, flags);

	set->private->glob_first_card = set->first_chan/set->private->chans_per_card;
	set->private->glob_last_card  = set->last_chan/set->private->chans_per_card;

	if ((rc=ngpd_cal_setup_trig_or_measure(set->path, -1, set->use_measure, set->do_xtk, 0, &set->private->first_stream, &set->private->last_stream,  &set->private->trig_offset, &set->private->wide_offset_a,  &set->private->wide_offset_b, &set->private->neutron_offset, 0, &set->private->need_upper)) < 0)
	{
		goto exit_error;
	}
	set->private->use_multi_thread = set->num_cards >= 1 && set->disable_multi_thread==0;

	if (set->private->use_multi_thread)
	{
		for (i=0;i<set->num_cards; i++)
		{
			set->private->thread_specific[i].thread_index = i;
			set->private->thread_specific[i].first_card = i;
			set->private->thread_specific[i].last_card = i;
			__atomic_store_n(&set->private->thread_specific[i].glob_progress_total, total_passes*3*(1+set->private->need_upper), __ATOMIC_SEQ_CST);
			__atomic_store_n(&set->private->thread_specific[i].progress_total, 1, __ATOMIC_SEQ_CST);	// To stop divide 0 if polled early
			set->private->thread_specific[i].state = MT_NotStarted;
		}
		for (card=set->private->glob_first_card; card<=set->private->glob_last_card; card++)
		{
			pthread_create(&(set->private->thread_specific[card].thread), NULL, ngpd_measure_trigger_do, &set->private->thread_specific[card]);
		}
	}
	else
	{
		set->private->thread_specific[0].thread_index = 0;
		set->private->thread_specific[0].first_card = set->private->glob_first_card;
		set->private->thread_specific[0].last_card  = set->private->glob_last_card;
		__atomic_store_n(&set->private->thread_specific[i].glob_progress_total, total_passes*3*(1+set->private->need_upper), __ATOMIC_SEQ_CST);
		__atomic_store_n(&set->private->thread_specific[i].progress_total, 1, __ATOMIC_SEQ_CST);	// To stop divide 0 if polled early
	}


	for (phase=0; phase<set->num_phase; phase++)
	{
		for (pass = 0; pass < ((set->private->phase==set->num_phase-1)?set->num_pass:1); pass++)
		{
			for (do_upper=0; do_upper<=set->private->need_upper; do_upper++)
			{
				if (set->private->use_multi_thread)
					ngpd_cal_measure_trigger_wait_state(set, MT_Idle);
				set->private->phase = phase;
				set->private->pass = pass;
				set->private->do_upper = do_upper;

				if (set->private->use_multi_thread)
					ngpd_cal_measure_trigger_signal_state(set, MT_Prepare);

				if (set->private->need_upper)
				{
					printf( "Note calling ngpd_calib_setup_trig_or_measure(card=%d, do_upper=%d) from ngpd_measure_trigger_do\n", card, do_upper);
					/* FIXME: Overwriting set->first|last_stream here is not OK unless a control thread synchronises all threads */
					/* Actually OK as these do not change at each call */
					if ((rc=ngpd_cal_setup_trig_or_measure(set->path, -1, set->use_measure, set->do_xtk, set->private->do_upper, &set->private->first_stream, &set->private->last_stream,  &set->private->trig_offset, &set->private->wide_offset_a,  &set->private->wide_offset_b, &set->private->neutron_offset, 0, NULL)) < 0)
						goto exit_threads;
				}
				if (set->private->use_multi_thread)
				{
					u_int32_t scope_status;
					ngpd_cal_measure_trigger_wait_state(set, MT_Ready);
					rc = ngpd_dma_system_start(set->path, -1, 0, 0, NULL);
					if (rc < 0)
					{
						snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger: ERROR starting system: %s", ngpd_get_error_message());
						goto exit_threads;
					}
					rc = ngpd_dma_wait_scope(set->path, -1, &scope_status);
					if (rc < 0) {
						snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger: ERROR waiting for scope DMAs %s", ngpd_get_error_message());
						goto exit_threads;
					}
					if (scope_status != 0)
					{
						snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger:ERROR: Scope status appears wrong at end of run = %08X", scope_status);
						goto exit_threads;
					}		
					ngpd_cal_measure_trigger_signal_state(set, MT_StartRead);
					ngpd_cal_measure_trigger_wait_state(set, MT_FinishedRead);
					ngpd_cal_measure_trigger_signal_state(set, MT_StartProcessing);
					ngpd_cal_measure_trigger_wait_state(set, MT_Idle);
					if (ngpd_cal_test_quit != NULL && ngpd_cal_test_quit())
					{
						printf("Main thread STOPPED By ^c\n");
						goto exit_threads;
					}
				}
				else				
				{
					if ((int)ngpd_measure_trigger_do(&(set->private->thread_specific[0])) < 0)
						goto exit_error;
				}					

				for (i=set->first_chan; i<= set->last_chan; i++)
				{
					if (set->min_hgt_neutron[i] >= 0.0 || set->max_hgt_neutron[i] >=0.0)
					{
						int *nnum, *gnum, *anum;
						nnum = (int *)id_get_ptr(set->private->mca_mod[phase], 0, NGPD_CAL_MOD_T_NEUTRON, i);
						gnum = (int *)id_get_ptr(set->private->mca_mod[phase], 0, NGPD_CAL_MOD_T_GAMMA, i);
						anum = (int *)id_get_ptr(set->private->mca_mod[phase], 0, NGPD_CAL_MOD_T_ALL, i);
						for (j=0; j<4096; j++)
							*anum++ = *gnum++ + *nnum++;
					}
				}
				if (set->do_ave)
				{
					for (i=set->first_chan; i<= set->last_chan; i++)
					{
						if (set->min_hgt_neutron[i] >= 0.0 || set->max_hgt_neutron[i] >=0.0)
						{
							double *nsum, *gsum, *asum;
							int *nnum, *gnum, *anum;
							for (j=NGPD_CAL_MOD_ROW_SIG; j<=NGPD_CAL_MOD_ROW_WIDE_B; j++)
							{
								nsum = (double *)id_get_ptr(set->private->ave_mod[phase][i], 0, j, NGPD_CAL_MOD_T_NEUTRON);
								gsum = (double *)id_get_ptr(set->private->ave_mod[phase][i], 0, j, NGPD_CAL_MOD_T_GAMMA);
								asum = (double *)id_get_ptr(set->private->ave_mod[phase][i], 0, j, NGPD_CAL_MOD_T_ALL);
								for (k=0;k<set->pre+set->post; k++)
									*asum++ = *gsum++ + *nsum++;
							}
							nnum = (int *)id_get_ptr(set->private->num_ave_mod[phase], i, NGPD_CAL_MOD_T_NEUTRON, 0);
							gnum = (int *)id_get_ptr(set->private->num_ave_mod[phase], i, NGPD_CAL_MOD_T_GAMMA, 0);
							anum = (int *)id_get_ptr(set->private->num_ave_mod[phase], i, NGPD_CAL_MOD_T_ALL, 0);
							*anum = *gnum + *nnum;
						}
						
						for (j=0; j<NGPD_CAL_MOD_NUM_T; j++)
						{
							int n;
							if ((n=set->private->num_ave_mod[phase]->data[i+set->num_chan*j]) > 0)
							{
								for (k=0; k<=NGPD_CAL_MOD_ROW_WIDE_B; k++)
								{
									ngpd_cal_divide_average(set->private->ave_mod[phase][i], k, j, n);
								}
							}
						}				
					}
				}
				if (set->do_xtk)
				{
					for (i=set->first_chan; i<= set->last_chan; i++)
					{
						if (set->min_hgt_neutron[i] >= 0.0 || set->max_hgt_neutron[i] >=0.0)
						{
							double *nsum, *gsum, *asum;
							int *nnum, *gnum, *anum;
							for (j=set->first_chan; j<=set->last_chan; j++)
							{
								nsum = (double *)id_get_ptr(set->private->xtk_neutron_mod[phase], 0, j, i);
								gsum = (double *)id_get_ptr(set->private->xtk_gamma_mod[phase], 0, j, i);
								asum = (double *)id_get_ptr(set->private->xtk_all_mod[phase], 0, j, i);
								for (k=0;k<set->pre+set->post; k++)
									*asum++ = *gsum++ + *nsum++;
								nnum = (int *)id_get_ptr(set->private->num_ave_xtk_mod[phase], j, i, NGPD_CAL_MOD_T_NEUTRON);
								gnum = (int *)id_get_ptr(set->private->num_ave_xtk_mod[phase], j, i, NGPD_CAL_MOD_T_GAMMA);
								anum = (int *)id_get_ptr(set->private->num_ave_xtk_mod[phase], j, i, NGPD_CAL_MOD_T_ALL);
								*anum = *gnum + *nnum;
								if (*anum> 0)
									ngpd_cal_divide_average(set->private->xtk_all_mod[phase], j, i, *anum);
								if (*gnum> 0)
									ngpd_cal_divide_average(set->private->xtk_gamma_mod[phase], j, i, *gnum);
								if (*nnum> 0)
									ngpd_cal_divide_average(set->private->xtk_neutron_mod[phase], j, i, *nnum);
							}
						}				
						else
						{
							int *anum;
							for (j=set->first_chan; j<=set->last_chan; j++)
							{
								anum = (int *)id_get_ptr(set->private->num_ave_xtk_mod[phase], j, i, NGPD_CAL_MOD_T_ALL);
								if (*anum> 0)
									ngpd_cal_divide_average(set->private->xtk_all_mod[phase], j, i, *anum);
							}
						}
					}
				}
					
				if (set->auto_neutron_height && phase == 0)
				{
					/* Scan mca looking for max and update min/max event */
					int max, max_posn;
					double sum_f, sum_fx;
					printf("End of Phase %d, Adjusting min/max to find K-alpha\n", phase+1);
					for (chan=set->first_chan; chan<= set->last_chan; chan++)
					{
						iptr = (int *)id_get_ptr(set->private->mca_mod[phase], 0, NGPD_CAL_MOD_T_ALL, chan);
						max = 0;
						for (i=0;i<4096;i++)
						{
							if (*iptr > max)
							{
								max = *iptr;
								max_posn = i;
							}
							iptr++;
						}
						printf("Chan %d found max %d at %d\n", chan, max, max_posn);
						iptr = (int *)id_get_ptr(set->private->mca_mod[phase], 0, NGPD_CAL_MOD_T_ALL, chan);
						sum_f = sum_fx = 0.0;
						for (i=(int)(0.90*max_posn); i<=(int)(1.1*max_posn); i++)
						{
							sum_f  += iptr[i];
							sum_fx += iptr[i] * i;
						}
						max_posn = sum_fx/sum_f;
						set->min_hgt_neutron[chan] = (0.9*max_posn)/set->mca_scale;
						set->max_hgt_neutron[chan] = (1.1*max_posn)/set->mca_scale;
						printf("Chan %d: Min = %g, Max=%g\n", chan, set->min_hgt_neutron[chan], set->max_hgt_neutron[chan]);
					}
				}
			} /* End for do_upper */
		} /* End for pass */
	} /* End for phase */

	if (set->private->use_multi_thread )
	{
		int this_rc=0;
		ngpd_cal_measure_trigger_signal_state(set, MT_Stop);
		
		for (card=set->private->glob_first_card; card<=set->private->glob_last_card; card++)
		{
			void *ret_void;
			pthread_join(set->private->thread_specific[card].thread, &ret_void);
			if ((int)(ret_void) < 0)
				this_rc = -1;
		}
		if (this_rc < 0)
			goto exit_error;
	}


	ngpd_cal_measure_trigger_unlink(set);

	ngpd_set_run_flags(set->path, set->private->save_flags);

	return 0;
exit_threads:
	if (set->private->use_multi_thread )
	{
		ngpd_cal_measure_trigger_signal_state(set, MT_Stop);
		
		for (card=set->private->glob_first_card; card<=set->private->glob_last_card; card++)
		{
			void *ret_void;
			pthread_join(set->private->thread_specific[card].thread, &ret_void);
		}
	}

exit_error:
	ngpd_cal_measure_trigger_unlink(set);
	return -1;
}

/**
 * @ingroup calibration
 * Start a background thread to run ngpd_cal_measure_trigger(). 
 * Progress can be monitored using ngpd_cal_measure_trigger_progress() and then thread joined using ngpd_cal_measure_trigger_join()
 * @param set	Settings 
*/
int ngpd_cal_measure_trigger_start(NGPDMeasureTrigger *set)
{	
	int rc;
	rc = pthread_create(&set->private->control_thread, NULL, (void * (*)(void *))ngpd_cal_measure_trigger, set);
	if (rc < 0)
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger_start: Cannot start thread, rc=%d\n", rc);
	return rc;
}

/**
 * @ingroup calibration
 * Wait for the background thread running ngpd_cal_measure_trigger() to terminate and join thread
 * Progress before this can be monitored using ngpd_cal_measure_trigger_progress()
 * @param set	Settings 
*/
int ngpd_cal_measure_trigger_join(NGPDMeasureTrigger *set)
{	
	int rc;
	void *ret_void;

	rc = pthread_join(set->private->control_thread, &ret_void);
	if (rc < 0)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger_join: Cannot join thread, rc=%d\n", rc);
		return -1;
	}
	else if (ret_void != NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger_join: worker thread returned error=%p\n", ret_void);
		return (int)ret_void;
	}
	return 0;
}

/**
 * @ingroup internal
 * Validate the setting in the data structure of type {@link NGPDMeasureTrigger}.
 * This is called by {@link ngpd_cal_measure_trigger()} but could be called by the user code first if required.
 * @param set 	A pointer to the settings of type {@link NGPDMeasureTrigger}
 * @return		Returns 0 on success or -ve on error with error message available from ngpd_cal_get_error_message()
 */
int ngpd_cal_measure_trigger_validate(NGPDMeasureTrigger *set)
{
	if (set->pre < 1 || set->pre > 2000)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger:Expected pre in range 1..2000 not %d", set->pre);
		return -1;
	}
	if (set->post < 1 || set->post > 10000)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger: Expected post in range 1..10000 not %d", set->post);
		return -1;
	}

	if (set->col_sat < 0)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger: expected colour saturation control > 0, not %d", set->col_sat);
		return -1;
	}
	if (set->first_chan < 0 || set->first_chan>= set->num_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger: expected %d <= first_chan <= %d , not %d", 0, set->num_chan-1, set->first_chan);
		return -1;
	}

	if (set->last_chan < 0 || set->last_chan >= set->num_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger: expected %d <= last <= %d , not %d", 0, set->num_chan-1, set->last_chan);
		return -1;
	}
	if (set->last_chan < set->first_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger: Expected first_chan <=last_chan but first_chan=%d, last_chan = %d", set->first_chan, set->last_chan);
		return -1;
	}

	if (set->num_phase < 1 || set->num_phase > 3)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger: expected %d <= num_phase <= %d , not %d", 1, 3, set->num_phase);
		return -1;
	}

	if (!set->do_ave && ! set->do_persist)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger: Please enable at least 1 display mode");
		return -1;
	}
	if (set->playback != NULL && set->user_service == NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger: Playback command required but user_service function not specified");
		return -1;
	}				

	return 0;
}
/**
 * @ingroup internal
 * This internal function does the work of running ngpd_measure_trigger(). It arranged so it can be called as thread function to allow a thread per card.
  * @param set	Settings 
 */

void *ngpd_measure_trigger_do(void * thread_vp)
{
	NGPDMeasureTriggerThread *thread_specific = (struct ngpd_measure_trigger_thread_t *) thread_vp;
	NGPDMeasureTrigger *set= thread_specific->parent;
	int i, j, chan, card;
	u_int16_t *dig_ptr;
	int16_t *ana_ptr;
	int *iptr;
	double *fptr;
	double lhs, rhs;
	// int pass;
	int persist_baseline;
	double average_baseline;
	int start_gap, stop_gap;
	char cmd[2048];
	int wide_start[NGPD_SCOPE_MAX_STREAMS];
	u_int16_t trig_mask, wide_mask_a, wide_mask_b, neutron_mask;
	int stream, rc;
	int ana_inc, dig_inc;
	int base_bit_posn;
	double hgt_for_mca;
	int separate_neutrons, mod_t;
	u_int32_t scope_status;

	while (1)
	{
		if (set->private->use_multi_thread)
		{
			printf("ngpd_measure_trigger_do:first_card=%d, last_card=%d triggering state=%d\n", thread_specific->first_card, thread_specific->last_card,  MT_Idle);
			pthread_mutex_lock(&set->private->mutex);
			thread_specific->state = MT_Idle;
			pthread_cond_broadcast(&set->private->cond_var);
			while (thread_specific->state != MT_Prepare && thread_specific->state != MT_Stop)
				pthread_cond_wait(&set->private->cond_var, &set->private->mutex);	
			pthread_mutex_unlock(&set->private->mutex);
			if (thread_specific->state == MT_Stop)
				break;
		}
		printf("Thread Index=%d : Card %d..%d : Source Channels = %d ..%d, pass %d\n", thread_specific->thread_index, 
			thread_specific->first_card, thread_specific->last_card, 0, set->private->chans_per_card-1, set->private->pass);
		if (set->playback && (set->private->phase ==0 || set->private->pass >0) && set->private->do_upper == 0)
		{
			for (card=thread_specific->first_card; card<=thread_specific->last_card; card++)
			{
				sprintf(cmd, set->playback, card, set->private->pass, set->private->pass, set->private->pass, set->private->pass, set->private->pass, set->private->pass, set->private->pass,
								set->private->pass, set->private->pass, set->private->pass, set->private->pass, set->private->pass, set->private->pass, set->private->pass, 
								set->private->pass, set->private->pass, set->private->pass);
				if (set->user_service(set->cn, cmd, 0, NULL, NULL) < 0)
				{
					snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger:  Cannot run playback command '%s'", cmd);
					goto exit_error;
				}
			}
		}
		__atomic_store_n(&thread_specific->glob_progress_now, (set->private->pass+set->private->phase)*(1+set->private->need_upper)*3+set->private->do_upper*3, __ATOMIC_SEQ_CST);
		__atomic_store_n(&thread_specific->progress_now, 0, __ATOMIC_SEQ_CST);
		__atomic_store_n(&thread_specific->progress_total, 1, __ATOMIC_SEQ_CST);
		
		if (set->private->use_multi_thread)
		{
			pthread_mutex_lock(&set->private->mutex);
			thread_specific->state = MT_Ready;
			pthread_cond_broadcast(&set->private->cond_var);
			while (thread_specific->state != MT_StartRead && thread_specific->state != MT_Stop)
				pthread_cond_wait(&set->private->cond_var, &set->private->mutex);	
			pthread_mutex_unlock(&set->private->mutex);
			if (thread_specific->state == MT_Stop)
				break;
		}
		else
		{
			rc = ngpd_dma_system_start(set->path, -1, 0, 0, NULL);
			if (rc < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger: ERROR starting system: %s", ngpd_get_error_message());
				goto exit_error;
			}
			rc = ngpd_dma_wait_scope(set->path, -1, &scope_status);
			if (rc < 0) {
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger: ERROR waiting for scope DMAs %s", ngpd_get_error_message());
				goto exit_error;
			}
			if (scope_status != 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger:ERROR: Scope status appears wrong at end of run = %08X", scope_status);
				goto exit_error;
			}		
		}
		__atomic_store_n(&thread_specific->glob_progress_now, (set->private->pass+set->private->phase)*(1+set->private->need_upper)*3+set->private->do_upper*3+1, __ATOMIC_SEQ_CST);
		__atomic_store_n(&thread_specific->progress_now, 0, __ATOMIC_SEQ_CST);
		__atomic_store_n(&thread_specific->progress_total, thread_specific->last_card-thread_specific->first_card+1, __ATOMIC_SEQ_CST);
		for (card=thread_specific->first_card; card<=thread_specific->last_card; card++)
		{
			thread_specific->progress_now = card-thread_specific->first_card;
			rc = ngpd_dma_read_scope(set->path, card, 0, 0, 0);
			if (rc < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger: ERROR reading scope data %s", ngpd_get_error_message());
				goto exit_error;
			}
		}
		if (set->private->use_multi_thread)
		{
			pthread_mutex_lock(&set->private->mutex);
			thread_specific->state = MT_FinishedRead;
			pthread_cond_broadcast(&set->private->cond_var);
			while (thread_specific->state != MT_StartProcessing && thread_specific->state != MT_Stop)
				pthread_cond_wait(&set->private->cond_var, &set->private->mutex);	
			pthread_mutex_unlock(&set->private->mutex);
			if (thread_specific->state == MT_Stop)
				break;
		}
		__atomic_store_n(&thread_specific->glob_progress_now, (set->private->pass+set->private->phase)*(1+set->private->need_upper)*3+set->private->do_upper*3+2, __ATOMIC_SEQ_CST);
		__atomic_store_n(&thread_specific->progress_now, 0, __ATOMIC_SEQ_CST);
		__atomic_store_n(&thread_specific->progress_total, (thread_specific->last_card-thread_specific->first_card+1)*(set->private->last_stream-set->private->first_stream+1), __ATOMIC_SEQ_CST);
		for (card=thread_specific->first_card; card<=thread_specific->last_card; card++)
		{
			for (stream=set->private->first_stream; stream<=set->private->last_stream; stream++)
			{
				int num_raw_trig=0, num_pileup_pre=0, num_pileup_post=0;
				__atomic_store_n(&thread_specific->progress_now, stream-set->private->first_stream+(set->private->last_stream-set->private->first_stream+1)*(card-thread_specific->first_card), __ATOMIC_SEQ_CST);
				chan = ngpd_cal_setup_trig_or_measure_get_chan(set->path, set->use_measure, set->private->do_upper, stream);
				if (chan < 0)
					continue;
				chan += card*set->private->chans_per_card;
				if (chan >= set->first_chan && chan <= set->last_chan)
				{
					separate_neutrons = set->min_hgt_neutron[chan]>=0 && set->max_hgt_neutron[chan] >= 0;
					ana_ptr = (int16_t *)ngpd_scope_mod_get_ptr(set->private->scope_mod, card, stream);
					ngpd_scope_stream_details(set->private->scope_mod, card, stream, NULL, &base_bit_posn, &dig_ptr, NULL, NULL, NULL, &ana_inc, &dig_inc);
					//					sv_printf(set->cn, "# Stream %d:Chan %d: Determined num_fine=%d, fine_posn=%d, dig_ptr=%p, fine_ptr=%p\n", stream, chan, num_fine, fine_posn, dig_ptr, fine_ptr);
					if (dig_ptr == NULL || ana_ptr == NULL)
					{
						snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger:ERROR reading scope data pointers:'%s'", ngpd_get_error_message());
						goto exit_error;
					}
					if (set->private->phase != 0)
					{
						fptr = (double *)id_get_ptr(set->private->ave_mod[set->private->phase-1][chan], 0, NGPD_CAL_MOD_ROW_WIDE_A, 0);
						for (i=0; i<set->pre+set->post; i++)
							if (fptr[i] > 0.5)
								break;
						wide_start[stream] = i+1-set->pre;
					}

					trig_mask = 1 << (base_bit_posn+set->private->trig_offset);
					if (set->private->wide_offset_a < 0)
						wide_mask_a = 0;
					else
						wide_mask_a =  1 << (base_bit_posn+set->private->wide_offset_a);

					if (set->private->wide_offset_b < 0)
						wide_mask_b = 0;
					else
						wide_mask_b =  1 << (base_bit_posn+set->private->wide_offset_b);

					if (set->private->neutron_offset < 0)
						neutron_mask = 0;
					else
						neutron_mask =  1 << (base_bit_posn+set->private->neutron_offset);

					start_gap = set->pre+set->extra_pre;
					stop_gap  = set->post+set->extra_post;
					printf("Card %d, Stream %d, chan %d. TRIG_MASK=%04X, WIDE_MASK_A=%04X, WIDE_MASK_B=%04X, set->use_measure=%d\n", card, stream, chan, trig_mask, wide_mask_a, wide_mask_b, set->use_measure);
					for (i=start_gap; i<set->private->scope_mod->head.num_t-stop_gap-1; i++)
					{
						if ((dig_ptr[dig_inc*i] & trig_mask)  && (dig_ptr[dig_inc*(i-1)] & trig_mask) == 0) /* Find Rising edge of trig Event signal, should be a lone rising pulse */
						{
							int omit = 0;
							num_raw_trig++;
							if (1) /* Clean by finding extra triggers */
							{
								for (j=-(set->pre+set->extra_pre); j<-1; j++) /* Check for any early hits */
								{
									if (dig_ptr[dig_inc*(i+j)] & trig_mask)
									{
										omit++;
										num_pileup_pre++;
										break;
									}
								}
								for (j=1; j< set->post+set->extra_post; j++) /* search for any late triggers */
								{
									if (dig_ptr[dig_inc*(i+j)] & trig_mask)
									{
										omit++;
										num_pileup_post++;
										break;
									}
								}
								if (omit)
									continue;
							}
							ngpd_cal_measure_pulse(ana_ptr, i, set->pre, set->post, &lhs, &rhs, &hgt_for_mca, set->pre/3, set->post/4, ana_inc, dig_inc);
							// sv_printf(set->cn, "# Found event Chan %d , t=%d;lhs=%g, rhs=%g, hgt=%f\n", chan, i, lhs, rhs, rhs-lhs);
							/* lhs, rhs, min_event, min_hgt_neutron should all be on 0..65535 scaling */
							if (set->min_event[chan] >= 0 && hgt_for_mca < set->min_event[chan])
								continue;
							if (set->max_event[chan] > 0 && hgt_for_mca > set->max_event[chan])
								continue;
							if (separate_neutrons)
							{
								if ( hgt_for_mca>= set->min_hgt_neutron[chan] && hgt_for_mca <= set->max_hgt_neutron[chan])
									mod_t = NGPD_CAL_MOD_T_NEUTRON;
								else
									mod_t = NGPD_CAL_MOD_T_GAMMA;
							}
							else
								mod_t = NGPD_CAL_MOD_T_ALL;

							persist_baseline = (lhs-32*set->persist_num_y/10);
							average_baseline = lhs;

							j = hgt_for_mca*set->mca_scale;
							if (j < 0) j = 0;
							else if (j > 4095) j= 4095;
							if (j >= 0 && j < 4096)
							{
								iptr = (int *) id_get_ptr(set->private->mca_mod[set->private->phase], j, mod_t, chan);
								(*iptr)++;
							}
							if (set->do_persist)
							{
								ngpd_cal_accumulate_persist(ana_ptr, dig_ptr, i, set->private->persist_mod[set->private->phase][chan], mod_t, set->pre, set->post, persist_baseline, 32, set->col_sat, trig_mask, wide_mask_a, wide_mask_b, ana_inc, dig_inc);
							}
							if (set->do_ave)
							{
								ngpd_cal_accumulate_ave(ana_ptr, dig_ptr, i, set->private->ave_mod[set->private->phase][chan], mod_t, set->pre, set->post, average_baseline, ana_inc, dig_inc, trig_mask, wide_mask_a, wide_mask_b );
								set->private->num_ave_mod[set->private->phase]->data[chan+set->private->num_ave_mod[set->private->phase]->head.num_x*mod_t]++;
							}
							if (set->do_xtk)
							{
								ndpg_cal_accumulate_xtk(set, thread_specific, set->private->do_upper, chan, i, mod_t);
							}
						} /* End IF Found event */
					} /* For i searching for reset */
					if (num_raw_trig < 1000)
						printf("# Warning only found %d triggers on channel %d\n", num_raw_trig, chan);
					else if (num_pileup_pre+num_pileup_post > num_raw_trig/2)
						printf("# Large fraction of events rejected on channel %d: num_raw_trig=%d, num_pileup_pre=%d, num_pileup_post=%d\n", chan, num_raw_trig, num_pileup_pre, num_pileup_post);
				} /* If Chan in range */
				if (ngpd_cal_test_quit != NULL && ngpd_cal_test_quit())
				{
					printf("STOPPED By ^c\n");
					goto exit_error;
				}
			} /* For stream */
			if (ngpd_cal_test_quit != NULL && ngpd_cal_test_quit())
			{
				printf("STOPPED By ^c\n");
				break;
			}
		} /* For Slot  */
		if (set->private->use_multi_thread)
		{
			pthread_mutex_lock(&set->private->mutex);
			thread_specific->state = MT_Idle;
			pthread_mutex_unlock(&set->private->mutex);
			pthread_cond_broadcast(&set->private->cond_var);
		}
		else
			break;
	} /* while (1) */
	printf("# Thread index %d finished: Card %d...%d , Stream %d...%d\n", thread_specific->thread_index, thread_specific->first_card, thread_specific->last_card, set->private->first_stream, set->private->last_stream);
	return (void *)(int64_t)0;

exit_error: 
	if (set->private->use_multi_thread)
	{
		pthread_mutex_lock(&set->private->mutex);
		thread_specific->state = MT_Idle;
		pthread_mutex_unlock(&set->private->mutex);
		pthread_cond_broadcast(&set->private->cond_var);
	}

	return (void *)(int64_t)-1;
}
/**
 * Calculate and return the progress being made by a background worker thread performing ngpd_cal_measure_trigger
 @param set		Pointer to the NGPDMeasureTrigger used to start the worker
 @return A double 0 -> 1.0 as a measures of progress.
 */
double ngpd_cal_measure_trigger_progress(NGPDMeasureTrigger *set)
{
	double progress = 1.0;
	if (set->private->use_multi_thread)
	{
		int card;
		printf("Multi-thread card first_card=%d, last_card=%d\n", set->private->glob_first_card, set->private->glob_last_card);
		for (card=set->private->glob_first_card; card<=set->private->glob_last_card; card++)
		{
			double p;
			p = (double)__atomic_load_n (&set->private->thread_specific[card].glob_progress_now, __ATOMIC_SEQ_CST)/(double)__atomic_load_n (&set->private->thread_specific[card].glob_progress_total, __ATOMIC_SEQ_CST);
			p += (double)__atomic_load_n (&set->private->thread_specific[card].progress_now, __ATOMIC_SEQ_CST)/
				((double)__atomic_load_n (&set->private->thread_specific[card].progress_total, __ATOMIC_SEQ_CST)*(double)__atomic_load_n (&set->private->thread_specific[card].glob_progress_total, __ATOMIC_SEQ_CST));
			printf("Multi-thread card=%d: progress=%g from %d of %d + %d of %d\n", card, p, set->private->thread_specific[card].glob_progress_now, set->private->thread_specific[card].glob_progress_total,
			set->private->thread_specific[card].progress_now, set->private->thread_specific[card].progress_total);
			if (p < progress)
				progress = p;
		}
	}
	else
	{
		int card = 0;
		progress = (double)__atomic_load_n (&set->private->thread_specific[card].glob_progress_now, __ATOMIC_SEQ_CST)/(double)__atomic_load_n (&set->private->thread_specific[card].glob_progress_total, __ATOMIC_SEQ_CST);
		progress += (double)__atomic_load_n (&set->private->thread_specific[card].progress_now, __ATOMIC_SEQ_CST)/
				((double)__atomic_load_n (&set->private->thread_specific[card].progress_total, __ATOMIC_SEQ_CST)*(double)__atomic_load_n (&set->private->thread_specific[card].glob_progress_total, __ATOMIC_SEQ_CST));
	}
	return progress;
}
/**
 * @ingroup calibration
 * Build a structure with default settings to be passed to {@link ngpd_cal_tail_subtract()}.
 * The settings structure is created with a call to this function. The default settings can then be changed in the calling code 
 * before this pointer is passed to {@link ngpd_cal_tail_subtract()}.
 * Afterwards the structure is freed by calling {@link ngpd_cal_tail_subtract_delete()}
 * @param path 	A handle to the top level of the ngpd system returned from any of the config functions  {@link ngpd_config_adq14} or {@link ngpd_config_ngzmp()}.
 * @param root_ave_name 	Pointer to a string giving the name of the averaged data modules created by {@link ngpd_cal_measure_trigger()}
 * @param root_fname		Pointer to string to give filenames to save the tail shapes, NULL to disable saving files.
 * @return 					A pointer to the allocated structure or NULL on error, with error message available from ngpd_cal_get_error_message()
 */
NgpdCalTailSubtract *ngpd_cal_tail_subtract_default(int path, char *root_ave_name, char *root_fname)
{
	NgpdCalTailSubtract * set;
	int num_chan;
	
	num_chan = ngpd_get_num_chan(path);
	if (num_chan < 0)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_tail_subtract_default: Cannot read num_chan from path %d, %s", path, ngpd_get_error_message());
		return NULL;
	}
	
	set = (NgpdCalTailSubtract *) malloc(sizeof(NgpdCalTailSubtract));
	if (set == NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_tail_subtract_default: Cannot malloc space for setup");
		return NULL;
	}
	memset(set, 0, sizeof(NgpdCalTailSubtract));
	set->path = path;
	set->num_chan = num_chan;
	set->first_chan = 0;
	set->last_chan = num_chan-1;
	set->fit_ramp= - 1;
	set->scale_tail = 1.0;
	set->end_offset = 0;
	set->root_ave_name = strdup(root_ave_name);
	if (root_fname != NULL)
		set->root_fname = strdup(root_fname);
	return set;
}
/**
 * @ingroup calibration
 * Free a structure of settings built by {@link ngpd_cal_tail_subtract_default()}
 * @param set 	Pointer to NgpdCalTailSubtract created by {@link ngpd_cal_tail_subtract_default()}
 * @return none
 */
void ngpd_cal_tail_subtract_delete(NgpdCalTailSubtract *set)
{
	free(set->root_ave_name);
	free(set->root_fname);
	free(set);
}
/**
 * @ingroup calibration
 * Process the averaged neutron and gamma pulse shape from {@link ngpd_cal_measure_trigger} to calculate the tail shapes to be loaded into the  firmware for tail subtraction.
 * To use this data must be collected using {@link ngpd_cal_measure_trigger} setup to separate neutrons and gammas.
 * The default setting are created with ngpd_cal_tail_subtract_default with the root_ave_name set to the last phase of ngpd_cal_measure_trigger. 
 * So either: ngpd_cal_measure_trigger is use with a single phase, min_hgt_neutron and max_hgt_neutron are set manually and the root_name = e.g. "ave_phase1".
 * Or ngpd_cal_measure_trigger is used with auto_neutron_height=1 and run 2 phases. The root_ave_name = e.g "ave_name_pase2"
 * Often the calculated tail shapes are saved to file using the root_fname.
 
 * @param set	Settings of type {@link NgpdCalTailSubtract} initialised by {@link ngpd_cal_tail_subtract_default()} and adjusted as necessary.
 * @return		Returns 0 on success or -ve on error with error message available from ngpd_cal_get_error_message()
*/
int ngpd_cal_tail_subtract (NgpdCalTailSubtract *set)
{
	char ave_mod_name[NGPD_MAX_MODNAME+20];
	char mod_name[NGPD_MAX_MODNAME+20];
	MOD_IMAGE3D *ave_mod=NULL;
	mh_com *ave_head;
	MOD_IMAGE3D *tail_mod=NULL;
	mh_com *tail_head;
	int num_x;
	int i, j, chan, x;
	double wspace[NGPD_MEASURE_TAIL_SUB_SIZE];
	u_int32_t *tail;
	double *fptr;
//	double *fp2, *fp3, f;
//	int num_lhs, num_rhs;
//	double baseline, lhs, rhs, lhs4, rhs4;
//	double grad_residual;
//	double sig_pre_event;
	char fname[1024];
	const char *cp;
    u_int16 attr_rev = mkattrevs(MA_REENT,1);
    u_int16 type_lang = mktypelang(MT_DATA, ML_ANY);
	int event_posn;
	double max_height;
	FILE *ofp;
	int err;
	int rc;
	int neutron;

	if (set->first_chan < 0 || set->first_chan>= set->num_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_tail_subtract: expected %d <= first_chan <= %d , not %d\n", 0, set->num_chan-1, set->first_chan);
		return -1;
	}
	if (set->last_chan < 0 || set->last_chan>= set->num_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_tail_subtract: expected %d <= last_chan <= %d , not %d\n", 0, set->num_chan-1, set->last_chan);
		return -1;
	}
	if (set->last_chan < set->first_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_tail_subtract: Expected first_chan <=last_chan but first_chan=%d, last_chan = %d", set->first_chan, set->last_chan);
		return -1;
	}

	sprintf(mod_name, "tail_subtract%d", set->num_chan);
	tail_mod = id_mkmod3d_err_msg (mod_name, NGPD_MEASURE_TAIL_SUB_SIZE, 2, set->num_chan, "Time", "Gamma/Neutron", "Channel",  NULL, 0, &(tail_head), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
	if (tail_mod == NULL)
		return -1;

	strncpy(ave_mod_name, set->root_ave_name, NGPD_MAX_MODNAME);
	ave_mod_name[NGPD_MAX_MODNAME] = 0;
	i=strlen(ave_mod_name);
	if (i > 1 && ave_mod_name[i-1] == '_')
		ave_mod_name[i-1] = 0;
	for (chan=set->first_chan; chan <= set->last_chan; chan++)
	{
		sprintf(mod_name, "%s_%02d", ave_mod_name, chan);
		cp = mod_name;
	   	err = _os_link(&cp, &ave_head, (void **)&ave_mod, &type_lang, &attr_rev);
	    if (err != 0)
	    {
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_tail_subtract: Cannot open average module name %s\n", mod_name);
			goto exit_error;
	    }

    	if (IMG_MOD_GET_TYPE(ave_mod) != IMG_MOD_3D || ave_mod->head.num_y != NGPD_CAL_MOD_NUM_ROW || ave_mod->head.num_t != NGPD_CAL_MOD_NUM_T)
    	{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_tail_subtract: Average module name %s is wrong type or size. Expected 3-d and num_y=%d, num_t=%d not %d, %d\n", mod_name, NGPD_CAL_MOD_NUM_ROW, NGPD_CAL_MOD_NUM_T, ave_mod->head.num_y, ave_mod->head.num_t);
			munlink(ave_head);
			goto exit_error;
    	}
		printf("# Processing chan %d from module %s\n", chan, mod_name);
		num_x = ave_mod->head.num_x;
		fptr = (double *)id_get_ptr(ave_mod, 0, NGPD_CAL_MOD_ROW_TRIG, NGPD_CAL_MOD_T_ALL);
		for (i=0; i< num_x; i++)
			if (fptr[i] > 0.5)
				break;
		event_posn = i ;

		if (i == num_x)
		{
			printf("# WARNING : Cannot find event end point on channel %d. Omitting\n", chan);
			munlink (ave_head);
			continue;
		}
		for (neutron=0; neutron<2; neutron++)
		{
/*
		if (fit_ramp >= 0)
		{
			if (event_end > fit_ramp)
			{
				sv_printf(cn, "! ERROR: Event end at position %d is after beginning of fit ramp %d. This cannot be sensible\n", event_end, fit_ramp);
				munlink(ave_head);
				goto exit_error;
			}
			if (fit_ramp > ave_mod->head.num_x-4)
			{
				sv_printf(cn, "! ERROR fitting leakage ramp from start point to RHS. Expected start point near middle of data module not %d\n", fit_ramp);
				munlink(ave_head);
				goto exit_error;
			}
			else
			{
				char command[100];
				double lhs;
				sprintf(command, "maths leastsqr-fit '%s' %d %d %d fit %d extrapolate", mod_name, fit_ramp, ave_mod->head.num_x-1, X3_MOD_ROW_SIG, X3_MOD_ROW_FIT);
				if (user_service(cn, command, 0, NULL, NULL) < 0)
				{
					sv_printf(cn, "! Error proceesing command '%s' fitting to module\n", command);
					munlink(ave_head);
					goto exit_error;
				}
				fptr = (double *)(ave_mod->data);
				fp2  = ((double *)(ave_mod->data)) + num_x*X3_MOD_ROW_FIT;
				fp3  = ((double *)(ave_mod->data)) + num_x*X3_MOD_ROW_CORRECTED;
				lhs = fp2[0];
				for (i=0;i<num_x;i++)
					fp3[i] = fptr[i]-(fp2[i]-lhs);
				j = event_start-1;
				if (j > 10)
				{
					short_tot = 0.0;
					for (i=0; i<j; i++)
						short_tot += fp3[i];
					short_tot /= j;
					for (i=0; i<num_x; i++)
						fp3[i] -= short_tot;
				}
			}
		}

		if (fit_ramp < 0)
			fptr = (double *)(ave_mod->data);
		else
			fptr = ((double *)(ave_mod->data))+num_x*X3_MOD_ROW_CORRECTED;
*/
			if (neutron)
				fptr = (double *)id_get_ptr(ave_mod, 0, NGPD_CAL_MOD_ROW_SIG, NGPD_CAL_MOD_T_NEUTRON);
			else
				fptr = (double *)id_get_ptr(ave_mod, 0, NGPD_CAL_MOD_ROW_SIG, NGPD_CAL_MOD_T_GAMMA);

			max_height = fptr[event_posn];
			if (max_height == 0)
				printf("Chan %d: neutron=%d: Warning event height %g No data. Note need to separate n and gamma\n", chan, neutron, max_height);
			else if (max_height < 100)
				printf("Chan %d: neutron=%d: Warning event height %g seems too small\n", chan, neutron, max_height);

			for (j=0, i=event_posn+set->end_offset; i<num_x && j < NGPD_MEASURE_TAIL_SUB_SIZE ; i++, j++)
			{
				wspace[j] = fptr[i];
			}

			while (j < NGPD_MEASURE_TAIL_SUB_SIZE)
			{
				wspace[j] = 0.0;
				j++;
			}
			tail = (u_int32 *)id_get_ptr(tail_mod, 0, neutron, chan);
			for (i=0; i<NGPD_MEASURE_TAIL_SUB_SIZE; i++)
			{
				x = (wspace[i]/max_height)*0x20000;
				x *= set->scale_tail;		// For testing only
				if ( x > 0x1ffff)
					x =0x1ffff;
				if (x < -0x1ffff)
					x = -0x1ffff;
				tail[i] = x;
			}
		}
		if (set->root_fname != NULL)
		{
			sprintf(fname, "%s_%02d.dat", set->root_fname, chan);
			if ((ofp=fopen(fname, "w")) == NULL)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_tail_subtract: Cannot open output file %s\n", fname);
				goto exit_error;
			}
			tail = id_get_ptr(tail_mod, 0, 0, chan);
			if (fwrite(tail, sizeof(int32), NGPD_MEASURE_TAIL_SUB_SIZE*2, ofp) != NGPD_MEASURE_TAIL_SUB_SIZE*2)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_tail_subtract:  ERROR writing file %s\n", fname);
				fclose(ofp);
				goto exit_error;
			}
			fclose(ofp);
		}
		if ((rc=ngpd_write_tail_subtract(set->path, chan, (int32_t*) tail)) < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_tail_subtract: Error writing tail subtract BRAM, chan %d : %s\n", chan, ngpd_get_error_message());
			goto exit_error;
		}
		munlink(ave_head);
		ave_mod = NULL;
	}
	if (tail_mod != NULL)
		munlink(tail_head);

	return 0;

exit_error:
	if (tail_mod != NULL)
		munlink(tail_head);

	return -1;

}
/**
 * @ingroup calibration
 * Build a structure with default settings to be passed to {@link ngpd_cal_adjust_trigger_timing()}.
 * The settings structure is created with a call to this function. The default settings can then be changed in the calling code 
 * before this pointer is passed to {@link ngpd_cal_adjust_trigger_timing()}.
 * Afterwards the structure is freed by calling {@link ngpd_cal_adjust_trigger_timing_delete()}
 * @param path 	A handle to the top level of the ngpd system returned from any of the config functions  {@link ngpd_config_adq14} or {@link ngpd_config_ngzmp()}.
 * @param ave_name 		Pointer to a string giving the name of the averaged data modules created by {@link ngpd_cal_measure_trigger()}
 * @param res_name		Pointer to string to give module name to save the measurements of the event timing
 * @return 				A pointer to the allocated structure or NULL on error, with error message available from ngpd_cal_get_error_message()
 */

NgpdCalAdjustTiming *ngpd_cal_adjust_trigger_timing_default (int path, char *ave_name, char *res_name)
{
	NgpdCalAdjustTiming * set;
	int num_chan;
	int i;
	
	num_chan = ngpd_get_num_chan(path);
	if (num_chan < 0)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing_default: Cannot read num_chan from path %d, %s", path, ngpd_get_error_message());
		return NULL;
	}
	set = (NgpdCalAdjustTiming *)malloc(sizeof(NgpdCalAdjustTiming));
	if (set == NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing_default: Cannot malloc space for setup");
		return NULL;
	}
	memset(set, 0, sizeof(NgpdCalAdjustTiming));
	set->path = path;
	set->num_chan = num_chan;
	set->first_chan = 0;
	set->last_chan = num_chan-1;
	set->ave_name = strdup(ave_name);
	set->res_name = strdup(res_name);
	for (i=0; i<NGPD_NUM_STRETCHED_TRIG; i++)
	{
		set->sig_first_thres[i]  = 0.005;
		set->sig_last_thres[i]   = 0.015;
		set->wide_first_thres[i] = 0.5;
		set->wide_last_thres[i]  = 0.5;
		set->wide_first_offset[i]= -2;
		set->wide_last_offset[i] = 10;
	}
	set->no_clear = 0;
	return set;
}
/**
 * @ingroup calibration
 * Free a structure of settings built by {@link ngpd_cal_adjust_trigger_timing_default()}
 * @param set 	Pointer to NGPDMeasureTrigger created by {@link ngpd_cal_adjust_trigger_timing_default()}
 * @return none
 */
void ngpd_cal_adjust_trigger_timing_delete(NgpdCalAdjustTiming *set)
{
	free(set->ave_name);
	free(set->res_name);
	free(set);
}

/**
 * @ingroup internal
 * Validate the setting in the data structure of type {@link NgpdCalAdjustTiming}.
 * This is called by {@link ngpd_cal_adjust_trigger_timing()} but could be called by the user code first if required.
 * @param set 	A pointer to the settings of type {@link NgpdCalAdjustTiming}
 * @return 0 on success, -1 on error.
 */
int ngpd_cal_adjust_trigger_timing_validate(NgpdCalAdjustTiming *set)
{
	int i;
	if (set->num_chan < 1)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing: Expected num_chan >=1, not %d", set->num_chan);
		return -1;
	}

	if (set->first_chan < 0 || set->first_chan >= set->num_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing: Expected 0 <= first_chan <= %d, not %d", set->num_chan-1, set->first_chan);
		return -1;
	}
	if (set->last_chan < 0 || set->last_chan >= set->num_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing: Expected 0 <= last_chan <= %d, not %d", set->num_chan-1, set->last_chan);
		return -1;
	}
	if (set->last_chan < set->first_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing: Expected first_chan <=last_chan but first_chan=%d, last_chan = %d", set->first_chan, set->last_chan);
		return -1;
	}
		
	for (i=0; i<NGPD_NUM_STRETCHED_TRIG; i++)
	{
		if (set->sig_first_thres[i] <= 0.0 || set->sig_first_thres[i] >= 1.0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing: Expected 0 < sig_first_thres[%d] < 1.0, not %g", i, set->sig_first_thres[i]);
			return -1;
		}
		if (set->sig_last_thres[i] <= 0.0 || set->sig_last_thres[i] >= 1.0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing: Expected 0 < sig_last_thres[%d] < 1.0, not %g", i, set->sig_last_thres[i]);
			return -1;
		}
		if (set->wide_first_thres[i] <= 0.0 || set->wide_first_thres[i] >= 1.0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing: Expected 0 < wide_first_thres[%d] < 1.0, not %g", i, set->wide_first_thres[i]);
			return -1;
		}
		if (set->wide_last_thres[i] <= 0.0 || set->wide_last_thres[i] >= 1.0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing: Expected 0 < wide_last_thres[%d] < 1.0, not %g", i, set->wide_last_thres[i]);
			return -1;
		}
	}
	if (strlen(set->ave_name) == 0)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing: Cannot Accept zero length ave module name\n");
		return -1;
	}
	if (set->fit_ramp)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing: fit_ramp feature not implemented yet\n");
		return -1;
	}
		
	return 0;
}
/**
 * @ingroup calibration
 * This function uses the averaged event shape and timing signals collect by {@link ngpd_cal_measure_trigger()} to adjust the trigger timing.
 * The timing is adjusted by setting the data delay and the delay and stretch times for the digital signals.
 * First averaged data must be collected using ngpd_cal_measure_trigger(). This must be configured to collect neutrons and gammas averaged together. 
 * However, it is useful to set min/max event sizes to exclude errant events.
 * The default settings are allocated by calling ngpd_cal_adjust_trigger_timing_default(). The root module name for the averaged data module 
 * and the name for the results data module are specified in this call.
 * The function works by scanning the shape of the averaged signal to find the maximum. The delay are adjusted to place the main trigger at the maximum of the peak.
 * The averaged shape is the scanned to find the start and  end of the pulse at a threshold of typically 0.5% of max for start and 1.5 % of max for the end.
 * The two wide digital trigger a and b are then positioned to the required offset (+ve or -ve) from these start and end points.
 * The wide a signal can alternatively be set as a single cycle pulse. See {@link NgpdCalAdjustTiming}
 * These thresholds and offsets can be adjusted by overwriting fields in the setting between calling ngpd_cal_adjust_trigger_timing_default() and ngpd_cal_adjust_trigger_timing().
 * After setting the trigger, the results can be checked by calling  ngpd_cal_measure_trigger() again and then call ngpd_cal_adjust_trigger_timing with just_check=1.
 
 * @param 	set		Setting structure allocated by ngpd_cal_adjust_trigger_timing_default().
 * @return		Returns 0 on success or -ve on error with error message available from ngpd_cal_get_error_message()
*/ 

int ngpd_cal_adjust_trigger_timing(NgpdCalAdjustTiming *set)
{
	MOD_IMAGE3D *ave_mod[NGPD_MAX_CHANS];
	mh_com *ave_head[NGPD_MAX_CHANS];
	MOD_IMAGE *res_mod=NULL;
	mh_com *res_head;
	const char *cp;
	int i, j;
	int chan;
	char mod_name[NGPD_MAX_MODNAME+20];
    u_int16 attr_rev = mkattrevs(MA_REENT,1);
    u_int16 type_lang = mktypelang(MT_DATA, ML_ANY);
	int sig_first_time[NGPD_NUM_STRETCHED_TRIG], sig_last_time[NGPD_NUM_STRETCHED_TRIG] ;
	int wide_first_time[NGPD_NUM_STRETCHED_TRIG], wide_last_time[NGPD_NUM_STRETCHED_TRIG];
	int num_x;
	double data_max;
	double *fptr;
	int sig_peak_time;
	int trig_time;
	int *iptr;
	int trig_dt, wide_first_dt[NGPD_NUM_STRETCHED_TRIG], wide_last_dt[NGPD_NUM_STRETCHED_TRIG];
	int wide_len[NGPD_NUM_STRETCHED_TRIG];
	NGPDDiffTrigger trig;
	int min_delay;

	if (ngpd_cal_adjust_trigger_timing_validate(set) < 0)
		return -1;

	i = strlen(set->ave_name);
	if (set->ave_name[i-1] == '_')
		set->ave_name[i-1] = 0;

	for (i=0; i < NGPD_MAX_CHANS; i++)
		ave_mod[i] = NULL;

	res_mod = id_mkmod_err_msg (set->res_name, set->num_chan, 12, "Chan", "Function",  DATA_LONG, &res_head, ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
	if (res_mod == NULL)
		goto exit_error;

	if (!set->no_clear)
	{
		iptr = (int *)res_mod->data;
		id_clear_mod(res_mod);
	}

	for (chan=set->first_chan; chan<=set->last_chan; chan++)
	{
		if (ngpd_read_diff_trigger(set->path, chan, &trig) < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing: error extracting diff trigger setup on chan %d\n", chan);
			goto exit_error;
		}
	
		sprintf(mod_name, "%s_%02d", set->ave_name, chan);
		cp = mod_name;
	    if (_os_link(&cp, &(ave_head[chan]), (void **)&(ave_mod[chan]), &type_lang, &attr_rev))
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing: Cannot link to data module %s\n", mod_name);
			goto exit_error;
		}
		num_x = ave_mod[chan]->head.num_x;
		iptr = (int *)res_mod->data+chan;

		if (ave_mod[chan]->head.num_y != NGPD_CAL_MOD_NUM_ROW || ave_mod[chan]->head.data_type != DATA_DOUBLE)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing: Module for channel %d has wrong num_y=%d or data type %d\n", chan,
					ave_mod[chan]->head.num_y, ave_mod[chan]->head.data_type );
			goto exit_error;
		}
		fptr = (double *)id_get_ptr(ave_mod[chan], 0, NGPD_CAL_MOD_ROW_TRIG, NGPD_CAL_MOD_T_ALL);

		for (i=0; i<num_x; i++)
			if (fptr[i] >= 0.9)
				break;
		trig_time = i;

		if (trig_time == num_x)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing: Missing CFD signal on channel %d, skipping\n", chan);
			continue;
		}

		fptr = (double *)id_get_ptr(ave_mod[chan], 0, NGPD_CAL_MOD_ROW_SIG, NGPD_CAL_MOD_T_ALL);
#if 0
		if (set->fit_ramp > 0)
		{
			if (set->fit_ramp > trig_time)
			{
				printf("WARNING: On Channel %d, fit_ramp=%d > trig_time=%d, run with longer pre or shorted fit-ramp\n", chan, set->fit_ramp, trig_time);
			}
			else
			{
				char command[100];
				double short_tot, *fp2, *fp3;
				double left_val, right_val;
				int this_fit_ramp;
				
				fp2 = (double *)id_get_ptr(ave_mod[chan], 0, NGPD_CAL_MOD_ROW_SIG, NGPD_CAL_MOD_T_ALL);
				left_val = fp2[0];
				right_val = fp2[num_x-1];
				for (i=0;i<num_x; i++)
					if (fp2[i] > 0.5*right_val)
						break;
				
				if (i-25 < num_x/4)
				{
					printf("WARNING: Channel %d: Rising edge position %d is very early, leaves too little space for fit ramp\n", chan, i);
				}
				else
				{
					this_fit_ramp = fit_ramp;
					if (this_fit_ramp > i - 25)
					{
						printf(cn, "Note Channel %d: Early event position %d, so shortening fit ramp to %d\n", chan, i, fit_ramp);
						this_fit_ramp = i-25;
					}
					sprintf(command, "maths leastsqr-fit '%s' %d %d %d fit %d extrapolate", mod_name, 0, this_fit_ramp, X3_MOD_ROW_SIG, X3_MOD_ROW_FIT);
					if (user_service(cn, command, 0, NULL, NULL) < 0)
					{
						sv_printf(cn, "! Error proceesing command '%s' fitting to module\n", command);
						munlink(ave_head);
						goto exit_error;
					}
					fptr = (double *)id_get_ptr(ave_mod[chan], 0, NGPD_CAL_MOD_ROW_SIG, NGPD_CAL_MOD_T_ALL);
					fp2  = (double *)id_get_ptr(ave_mod[chan], 0, NGPD_CAL_MOD_ROW_FIT, NGPD_CAL_MOD_T_ALL);
					fp3  = (double *)id_get_ptr(ave_mod[chan], 0, NGPD_CAL_MOD_ROW_CORRECTED, NGPD_CAL_MOD_T_ALL);

					for (i=0;i<num_x;i++)
						fp3[i] = fptr[i]-fp2[i];

					j = fit_ramp-10;
					if (j > 10)
					{
						short_tot = 0.0;
						for (i=0; i<j; i++)
							short_tot += fp3[i];
						short_tot /= j;
						for (i=0; i<num_x; i++)
							fp3[i] -= short_tot;
					}
					fptr  = (double *)id_get_ptr(ave_mod[chan], 0, NGPD_CAL_MOD_ROW_CORRECTED, NGPD_CAL_MOD_T_ALL);
				}
			}
		}
#endif
		data_max = 0.0;
		for (i=0; i<num_x; i++)
		{
			if (fptr[i] > data_max)
			{
				data_max = fptr[i];
				sig_peak_time = i;
			}
		}

		for (j=0; j<NGPD_NUM_STRETCHED_TRIG; j++)
		{
			for (i=0; i<num_x; i++)
				if (fptr[i] >= set->sig_first_thres[j]*data_max)
					break;
			sig_first_time[j] = i;

			for (i=num_x-1; i>=0; i--)
				if (fptr[i] > set->sig_last_thres[j]*data_max)
					break;
			sig_last_time[j] = i;
		}

		for (j=0; j<NGPD_NUM_STRETCHED_TRIG; j++)
		{
			fptr = (double *)id_get_ptr(ave_mod[chan], 0, NGPD_CAL_MOD_ROW_WIDE_A+j, NGPD_CAL_MOD_T_ALL);
			for (i=0; i<num_x; i++)
				if (fptr[i] >= set->wide_first_thres[j])
					break;
			wide_first_time[j] = i;

			if (wide_first_time[j] == num_x)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing: Missing Wide signal on channel %d, skipping\n", chan);
				continue;
			}

			for (i=num_x-1; i >0; i--)
				if (fptr[i] >= set->wide_last_thres[j])
					break;
	//		sv_printf(cn, "# Chan %d : Found OTD last at i=%d, fptr[i] = %g\n", chan, i, fptr[i]);
			wide_last_time[j] = i;
		}

		*iptr = sig_peak_time;
		iptr += set->num_chan;
		*iptr = trig_time;
		iptr += set->num_chan;

		for (j=0; j<NGPD_NUM_STRETCHED_TRIG; j++)
		{
			*iptr = sig_first_time[j];
			iptr += set->num_chan;
			*iptr = sig_last_time[j];
			iptr += set->num_chan;
			*iptr = wide_first_time[j];
			iptr += set->num_chan;
			*iptr = wide_last_time[j];
			iptr += set->num_chan;
		}
/*
		*iptr = sig_last_time+1-sig_first_time;
		iptr += set->num_chan;
		*iptr = wide_last_time+1-wide_first_time;
		iptr += set->num_chan;
*/
		trig_dt = sig_peak_time-trig_time;
		for (j=0; j<NGPD_NUM_STRETCHED_TRIG; j++)
		{
			wide_first_dt[j] = sig_first_time[j]+set->wide_first_offset[j] - wide_first_time[j];
			wide_last_dt[j]  = sig_last_time[j]+set->wide_last_offset[j] - wide_last_time[j];
			wide_len[j]      = wide_last_time[j]+1-wide_first_time[j];
		
			if (wide_len[j] <= 2)
			{
				printf("Warning: Wide event signal length on channel %d is very small, %d, check length after adjustment\n", chan, wide_len[j]);
			}
		}
		trig.delay_a += wide_first_dt[0]/2;
		trig.width_a -= wide_first_dt[0]/2;
		trig.delay_b += wide_first_dt[1]/2;
		trig.width_b -= wide_first_dt[1]/2;
		trig.trig_delay += trig_dt;

		min_delay = trig.delay_a*2;
		if (trig.delay_b*2 < min_delay) min_delay = trig.delay_b*2;
		if (trig.data_delay < min_delay) min_delay = trig.data_delay;
		if (trig.trig_delay < min_delay) min_delay = trig.trig_delay;

		trig.data_delay -= min_delay;
		trig.trig_delay -= min_delay;
		trig.delay_a -= min_delay/2;
		trig.delay_b -= min_delay/2;

		trig.width_a += wide_last_dt[0]/2;
		trig.width_b += wide_last_dt[1]/2;

/*		if (trig.data_delay < 0)
		{
			sv_printf(cn, "# Warning, required data delay=%d, limiting to 0\n", trig.data_delay);
			trig.trig_delay -= trig.data_delay;
			trig.stretch -= trig.data_delay/2;
			trig.data_delay = 0;
		}
		else */
		if (trig.data_delay > NGPD_DIFF_TRIG_MAX_DATA_DELAY)
		{
			int dt = trig.data_delay-NGPD_DIFF_TRIG_MAX_DATA_DELAY;
			printf("Warning, required data delay=%d, limiting to %d\n", trig.data_delay, NGPD_DIFF_TRIG_MAX_DATA_DELAY);
			trig.data_delay = NGPD_DIFF_TRIG_MAX_DATA_DELAY;
			trig.trig_delay -= dt;
			trig.width_a -= dt/2;
			trig.width_b -= dt/2;
			if (trig.trig_delay < 0)
			{
				printf("Warning, But adjusted trig delay=%d, limiting to 0\n", trig.data_delay);
				trig.trig_delay = 0;
			}
		}
		if (trig.trig_delay > NGPD_DIFF_TRIG_MAX_TRIG_DELAY)
		{
			int dt = trig.trig_delay - NGPD_DIFF_TRIG_MAX_TRIG_DELAY;
			printf("Warning, required trig delay=%d, limiting to %d\n", trig.data_delay, NGPD_DIFF_TRIG_MAX_TRIG_DELAY);
			trig.trig_delay = NGPD_DIFF_TRIG_MAX_TRIG_DELAY;
			trig.data_delay -= dt;
			trig.width_a -= dt/2;
			trig.width_b -= dt/2;
		}
		if (trig.data_delay < 0)
		{
			printf("Warning, required data delay=%d, limiting to 0\n", trig.data_delay);
			trig.data_delay = 0;
		}
		else if (trig.data_delay > NGPD_DIFF_TRIG_MAX_DATA_DELAY)
		{
			printf("Warning, required data delay=%d, limiting to %d\n", trig.data_delay, NGPD_DIFF_TRIG_MAX_DATA_DELAY);
			trig.data_delay = NGPD_DIFF_TRIG_MAX_DATA_DELAY;
		}
		if (trig.width_a < 1)
			trig.width_a = 1;
		else if (trig.width_a > NGPD_DIFF_TRIG_MAX_TRIG_STRETCH)
			trig.width_a = NGPD_DIFF_TRIG_MAX_TRIG_STRETCH;

		if (trig.width_b < 1)
			trig.width_b = 1;
		else if (trig.width_b > NGPD_DIFF_TRIG_MAX_TRIG_STRETCH)
			trig.width_b = NGPD_DIFF_TRIG_MAX_TRIG_STRETCH;
		if (set->wide_a_pulse)
			trig.width_a = 1;

		if (!set->just_check)
		{
			if (ngpd_write_diff_trigger(set->path, chan, &trig) < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_adjust_trigger_timing: ERROR setting diff trigger setup on chan %d: %s\n", chan, ngpd_get_error_message());
				goto exit_error;
			}

		}
		printf("Chan %d Changes: trig_dt=%d, Wide-A First=%d, Wide-A Last=%d, Wide-B First=%d, Wide-B Last=%d\n", chan, trig_dt, wide_first_dt[0], wide_last_dt[0], wide_first_dt[1], wide_last_dt[1]);
	}
	if (res_mod != NULL)
		munlink(res_head);
	for (i=0; i< NGPD_MAX_CHANS; i++)
	{
		if (ave_mod[i] != NULL)
			munlink(ave_head[i]);
	}
	return 0;

exit_error:
	for (i=0; i< NGPD_MAX_CHANS; i++)
	{
		if (ave_mod[i] != NULL)
			munlink(ave_head[i]);
	}
	if (res_mod != NULL)
		munlink(res_head);
	return -1;
}

#if 0

#if 0
int ngpd_adjust_trigger_b_threshold_ringing (int quals, int cn, int path, char *ave_name, char *res_name)
{
	int sys;
	MOD_IMAGE *ave_mod[NGPD_MAX_CHANS];
	mh_com *ave_head[NGPD_MAX_CHANS];
	MOD_IMAGE *res_mod=NULL;
	mh_com *res_head;
	MOD_IMAGE *thres_mod=NULL;
	mh_com *thres_head;
	int first_chan, last_chan, num_chan;
	char *cp;
	int i, j;
	int chan;
	char mod_name[NGPD_MAX_MODNAME+20];
    u_int16 attr_rev = mkattrevs(MA_REENT,1);
    u_int16 type_lang = mktypelang(MT_DATA, ML_ANY);
	int num_x;
	double data_rhs;
	double *fptr, *fbase;
	double diff1, *d1ptr, *d2ptr, *d1filt;
	int cfd_first_time, cfd_last_time;
	double cfd_first_thres = 0.5;
	double cfd_last_thres  = 0.5;
	int sep_offset, sep_max, rc;
	Xspress3_TriggerB trig_b;
	int num_ave, sep1, sep2;
	double max_diff1, max_ring1, max_ring2, min_diff, max_diff2;
	int max_posn1, min_posn, max_ring_posn1, max_ring_posn2;
	int half_len1, half_len2;
	int no_clear = 0;
	double cur_energy=5.89, max_energy=20.0, min_energy=3.5;
	int noise_thres1, ring_thres1;
	int noise_thres2, ring_thres2;
	int max_thres1, max_thres2;
	int thres1, thres2;
	float * sigma_ptr, sigma1, sigma2;
	int do_filt=0;
	int pass, num_pass;
	int min_delay, max_delay;
	int del_min, del_max;
	double scale_min, scale_max;
	char * script_name;
	FILE *script_fp=NULL;
	int least_sqr;

	num_chan = 	ngpd_get_num_chan(path);
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

	if (quals & 8)
		cur_energy = Qualifier[3].DoubleArg;

	if (quals & 0x10)
		max_energy = Qualifier[4].DoubleArg;

	if (quals & 0x20)
	{
		do_filt = 1;
		script_name = Qualifier[5].StringArg;
		if ((script_fp=fopen(script_name, "w")) == NULL)
		{
			sv_printf(cn, "! ERROR : Cannot open sctipt file '%s'\n", script_name);
			return -1;
		}
	}
	least_sqr = !(quals & 0x40);
	ngpd_ringing_debug= !!(quals & 0x80);
	if (quals & 0x100)
		ngpd_ringing_debug = 2;
	if (first_chan <0 || first_chan >= num_chan || last_chan <0 || last_chan >= num_chan || first_chan > last_chan)
	{
		sv_printf(cn, "! ERROR: Please specify valid channel range in 0..%d not %d to %d\n", num_chan-1, first_chan, last_chan);
		return -1;
	}
	if (strlen(ave_name) == 0)
	{
		sv_printf(cn, "! Cannot Accept zero length ave module name length\n");
		return -1;
	}
	i = strlen(ave_name);
	if (ave_name[i-1] == '_')
		ave_name[i-1] = 0;

	for (i=0; i < NGPD_MAX_CHANS; i++)
		ave_mod[i] = NULL;

	res_mod = app_mkmod_head (cn, res_name, num_chan, 2, "Chan", "Function",  DATA_DOUBLE, &res_head);
	if (res_mod == NULL)
		goto exit_error;

	if (!no_clear)
	{
		clear_mod(res_mod);
	}

	if ((rc=ngpd_trigger_b_get_diff_params(path, &sep_offset, &sep_max)) < 0)
	{
		sv_printf(cn, " ! ERROR Reading trigger B hardware limits: %s\n", ngpd_get_error_message());
		goto exit_error;
	}
	if ((rc=ngpd_trigger_b_get_ringing_params(path, &min_delay, &max_delay)) < 0)
	{
		sv_printf(cn, " ! ERROR Reading trigger B hardware limits: %s\n", ngpd_get_error_message());
		goto exit_error;
	}
	if (max_delay == 0)
		do_filt = 0;
	num_pass = do_filt?2:1;

	cp = "ngpd_thresholds";
    if (_os_link(&cp, &thres_head, (void **)&thres_mod, &type_lang, &attr_rev))
	{
		sv_printf (cn,"# Cannot link to data module %s, so will not adjust thresholds\n", cp);
		thres_head = NULL;
	}
	sigma_ptr = (float *)thres_mod->data;

	for (chan=first_chan; chan<=last_chan; chan++)
	{
		sprintf(mod_name, "%s_%02d", ave_name, chan);
		cp = mod_name;
	    if (_os_link(&cp, &(ave_head[chan]), (void **)&(ave_mod[chan]), &type_lang, &attr_rev))
		{
			sv_printf (cn,"! Cannot link to data module %s\n", mod_name);
			goto exit_error;
		}
		num_x = ave_mod[chan]->head.num_x;

		if (ave_mod[chan]->head.num_y != X3_MOD_NUM_ROW || ave_mod[chan]->head.data_type != X3_AVE_MOD_TYPE)
		{
			sv_printf(cn, "! ERROR: Module for channel %d has wrong num_y=%d or data type %d\n", chan,
					ave_mod[chan]->head.num_y, ave_mod[chan]->head.data_type );
			goto exit_error;
		}
		fbase = (double *)ave_mod[chan]->data;

		fptr = fbase+X3_MOD_ROW_CFD*num_x;
		for (i=0; i<num_x; i++)
			if (fptr[i] >= cfd_first_thres)
				break;
		cfd_first_time = i;

		for (i=num_x-1; i >0; i--)
			if (fptr[i] >= cfd_first_thres)
				break;
		cfd_last_time = i;

		if (ngpd_get_trigger_b(path, chan, &trig_b) < 0)
		{
			sv_printf(cn, "! ERROR extracting trigger b setup on chan %d\n", chan);
			goto exit_error;
		}

		sep1 = trig_b.sep1 + sep_offset;
		sep2 = trig_b.sep2 + sep_offset;
		num_ave = 1 << trig_b.avemode;
		half_len1 = (sep1+num_ave-1)/2;
		half_len2 = (sep2)/2;
		fptr = fbase+X3_MOD_ROW_SIG*num_x +sep1+num_ave-1;
		d1ptr = fbase + X3_MOD_ROW_DIFF1 * num_x + half_len1;
		for (i=sep1+num_ave-1; i<num_x; i++)
		{
			diff1 = 0;
			for (j=0; j<num_ave;j++)
				diff1 += fptr[-j];
			for (j=0; j<num_ave;j++)
				diff1 -= fptr[-sep1-j];
			*d1ptr = diff1;
			d1ptr++;
			fptr++;
		}
		for (pass=0; pass<num_pass; pass++)
		{
			if (pass == 1)
			{ 
				/* second pass */
				d1ptr = fbase + X3_MOD_ROW_DIFF1 * num_x;
				d1filt = fbase + X3_MOD_ROW_DIFF1_FILT * num_x;
				sv_printf(cn, "# Creating filtered differential scale=%g, del=%d, scale=%g, del=%d\n", scale_min, del_min, scale_max, del_max);
				for (i=0; i<num_x; i++)
				{
					*d1filt = *d1ptr;
					if (i >= del_min)
						*d1filt += scale_min*d1ptr[-del_min];

					if (i >= del_max)
						*d1filt += scale_max*d1ptr[-del_max];
					d1ptr++;
					d1filt++;
				}
				d1ptr = fbase + X3_MOD_ROW_DIFF1_FILT * num_x;
				d2ptr = fbase + X3_MOD_ROW_DIFF2_FILT * num_x + half_len2;
			}
			else
			{
				d1ptr = fbase + X3_MOD_ROW_DIFF1 * num_x;
				d2ptr = fbase + X3_MOD_ROW_DIFF2 * num_x + half_len2;
			}
			/* The timing between diff1 and diff2 is important as the 2nd zero cross in diff 2 is used to find the minimum in diff 1 */
			/* Hence d2ptr start offset by half_len2 */
			for (i=0; i+sep2<num_x; i++)
			{
				*d2ptr++ = d1ptr[sep2]-d1ptr[0];
				d1ptr++;
			}

			if (pass == 1)
			{ 
				d1ptr = fbase + X3_MOD_ROW_DIFF1_FILT * num_x;
				d2ptr = fbase + X3_MOD_ROW_DIFF2_FILT * num_x;
			}
			else
			{
				d1ptr = fbase + X3_MOD_ROW_DIFF1 * num_x;
				d2ptr = fbase + X3_MOD_ROW_DIFF2 * num_x;
			}
			max_diff1 = 0;
			max_diff2 = 0;
			for (i=0; i<num_x-(sep1+num_ave); i++)
			{
				if (*d1ptr > max_diff1)
				{
					max_diff1 = *d1ptr;
					max_posn1 = i;
				}
				if (*d2ptr > max_diff2)
					max_diff2 = *d2ptr;
				d1ptr++;
				d2ptr++;
			}
			if (pass == 1)
			{ 
				d1ptr = fbase + X3_MOD_ROW_DIFF1_FILT * num_x;
				d2ptr = fbase + X3_MOD_ROW_DIFF2_FILT * num_x;
			}
			else
			{
				d1ptr = fbase + X3_MOD_ROW_DIFF1 * num_x;
				d2ptr = fbase + X3_MOD_ROW_DIFF2 * num_x;
			}
			for (i=max_posn1-2; i<num_x-1; i++)
			{
				if (i >= 0)
				{
					if (d2ptr[i] > 0 && d2ptr[i+1] <= 0.0)
						break;		/* Found zero crossing falling that is trigger */
				}
			}
			i++;
			while (i<num_x-1)
			{
				if (d2ptr[i] < 0 && d2ptr[i+1] >= 0.0)
					break; /* Found zero crossing rising that is trigger  minimum*/
				i++;
			}
			min_posn = i-1;
	//		sv_printf(cn, "# searching for min from %d to %d\n", min_posn, min_posn+2);
			min_diff = d1ptr[i-1];
			for (j=0;j<3; j++)
			{
				if (d1ptr[i+j] < min_diff)
				{
					min_diff = d1ptr[i+j];
					min_posn = i+j;
				}
			}
			max_ring1 = 0.0;
			max_ring2 = 0.0;
			while (i<num_x-1)
			{
				if (d2ptr[i] > max_ring2)
				{
					max_ring2 = d2ptr[i];
					max_ring_posn2 = i;
				}
				i++;
			}
			i = min_posn;
			while (i<num_x-1)
			{
				if (d1ptr[i] > max_ring1)
				{
					max_ring1 = d1ptr[i];
					max_ring_posn1 = i;
				}
				i++;
			}
			if (pass == 0 && do_filt)
			{
				if (least_sqr)
				{
					ngpd_ringing_least_squares(cn, ave_mod[chan],  min_delay, max_delay, max_posn1, max_diff1, min_posn, max_ring_posn1, &del_min, &del_max, &scale_min, &scale_max);
				}
				else
				{
					del_min = min_posn-max_posn1;
					del_max = max_ring_posn1-max_posn1;
					if (del_min >= min_delay && del_min <= max_delay && min_diff < 0)
						scale_min = -min_diff/max_diff1;
					else
					{
						del_min = min_delay;
						scale_min = 0.0;
					}

					if (del_max >= min_delay && del_max <= max_delay && max_diff1 > 0)
						scale_max = -max_ring1/max_diff1;
					else
					{
						del_max = min_delay;
						scale_max = 0.0;
					}
				}
				ngpd_set_trigger_b_ringing(path, chan, scale_min, del_min, scale_max, del_max);
				if (script_fp != NULL)
					fprintf(script_fp, "ngpd set-ringing %d %d %g %d %g %d\n", path, chan, scale_min, del_min, scale_max, del_max);
			}
			if (num_pass == 1)
				sv_printf(cn, "# Chan %d: Found ringing on diff1 at %d, value=%g, diff2 @ %d, value=%g\n", chan, max_ring_posn1, max_ring1, max_ring_posn2, max_ring2);
			else if (pass == 0)
				sv_printf(cn, "# Chan %d: Raw  Found ringing on diff1 at %d, value=%g, diff2 @ %d, value=%g\n", chan, max_ring_posn1, max_ring1, max_ring_posn2, max_ring2);
			else
				sv_printf(cn, "# Chan %d: Filt Found ringing on diff1 at %d, value=%g, diff2 @ %d, value=%g\n", chan, max_ring_posn1, max_ring1, max_ring_posn2, max_ring2);

			if (pass == num_pass-1)
			{
				d1ptr = (double *) (res_mod->data);
				d1ptr += chan;
				*d1ptr = max_ring1;
				d2ptr = d1ptr+res_mod->head.num_x;
				*d2ptr = max_ring2;
				if (thres_mod != NULL)
				{

					sigma1 = sigma_ptr[chan+X3THRES_TRIG_B_SIGMA1*thres_mod->head.num_x];
					sigma2 = sigma_ptr[chan+X3THRES_TRIG_B_SIGMA2*thres_mod->head.num_x];
					if (sigma1 > 0 && sigma2 > 0)
					{
						noise_thres1 = 5.0*sigma1;
						ring_thres1  = max_energy/cur_energy*max_ring1 + 2.5*sigma1;
						max_thres1 = min_energy/cur_energy*max_diff1-1.5*sigma1;
						thres1 = (noise_thres1>ring_thres1)?noise_thres1:ring_thres1;

						noise_thres2 = 5.0*sigma2;
						ring_thres2  = max_energy/cur_energy*max_ring2 + 2.5*sigma2;
						max_thres2 = min_energy/cur_energy*max_diff2-1.5*sigma2;
						thres2 = (noise_thres2>ring_thres2)?noise_thres2:ring_thres2;
						if (thres1 > max_thres1 || thres2 > max_thres2)
						{
							sv_printf(cn, "# Chan %d: Sigma 1=%g, sigma2=%g. Maximum signal diff1=%g, diff2=%g\n", chan, sigma1, sigma2, max_diff1, max_diff2);
							sv_printf(cn, "# .... Threshold to clear ringing/noise too large for min energy\n");
							if (thres1 <= max_thres1)
							{
								sv_printf(cn, "# .... Acceptable values for arm threshold\n");
								ngpd_trigger_b_change(cn, path, chan, X2_CHANGE_ARM_THRES, thres1, 0, 0);
								ngpd_trigger_b_change(cn, path, chan, X2_CHANGE_END_THRES, thres1/2, 0, 0);
							}
							if (thres2 <= max_thres2)
							{
								sv_printf(cn, "# .... Acceptable values for re-arm threshold\n");
								ngpd_trigger_b_change(cn, path, chan, X2_CHANGE_REARM_THRES, thres2, 0, 0);
							}

							sv_printf(cn, "# .... Threshold: %d %d %d but to detect min require %d %d %d\n", thres1, thres1/2, thres2, max_thres1, max_thres1/2, max_thres2);
						}
						else
						{
							sv_printf(cn, "# .... Adjusted threshold: %d %d %d\n", chan, thres1, thres1/2, thres2);
							ngpd_trigger_b_change(cn, path, chan, X2_CHANGE_ARM_THRES, thres1, 0, 0);
							ngpd_trigger_b_change(cn, path, chan, X2_CHANGE_END_THRES, thres1/2, 0, 0);
							ngpd_trigger_b_change(cn, path, chan, X2_CHANGE_REARM_THRES, thres2, 0, 0);
						}
					}
					else
					{
						sv_printf(cn, "# .... Note sigma not available for channel %d\n", chan);
					}
				}
			}
		}
	}
		
	if (script_fp != NULL)
		fclose(script_fp);
	if (res_mod != NULL)
		munlink(res_head);
	for (i=0; i< NGPD_MAX_CHANS; i++)
	{
		if (ave_mod[i] != NULL)
			munlink(ave_head[i]);
	}
	if (thres_mod != NULL)
		munlink(thres_head);
	return 0;

exit_error:
	if (script_fp != NULL)
		fclose(script_fp);
	for (i=0; i< NGPD_MAX_CHANS; i++)
	{
		if (ave_mod[i] != NULL)
			munlink(ave_head[i]);
	}
	if (res_mod != NULL)
		munlink(res_head);
	if (thres_mod != NULL)
		munlink(thres_head);
	return -1;
}
#define NGPD_MAX_NUM_RINGING_TERMS 2
static MOD_IMAGE *cur_mod;
static double *diff1, *diff2;
static int filt_posn[NGPD_MAX_NUM_RINGING_TERMS];
static int debug_cn;
static int tail_start;
static int npts_tail;
static double max_diff1=1.0;
static int num_calls;

#define XSP_NUM_RINGING_PASS 100
int ngpd_ringing_residuals(int *m, int *n, double *x, double *fvec) 
{
	int i, j;
	double scale_err = 1.0;
	double *d1, *d2;
	double sumsqr;

	d1 = diff1+tail_start;
	d2 = diff2+tail_start;
	for (i=0; i<npts_tail; i++)
		fvec[i] = d1[i];
	for (i=0; i<npts_tail; i++)
		fvec[i+npts_tail] = d2[i];

	for (j=0; j<*n; j++)
	{
		for (i=0; i<npts_tail; i++)
			fvec[i] += d1[i-filt_posn[j]]*x[j];
		for (i=0; i<npts_tail; i++)
			fvec[i+npts_tail] += d2[i-filt_posn[j]]*x[j];
	}
	sumsqr = 0.0;
	for (i=0; i<2*npts_tail; i++)
	{
//		printf("%d\t%g\n", i, fvec[i]);		
		if (fvec[i] > 0)
		{
			scale_err = fvec[i]/(max_diff1/10);	// This should reach 1 when ringing reaches + 10%
			scale_err = 10*scale_err*scale_err+1;
			fvec[i] *= scale_err;
		}
		if (i < npts_tail)
			fvec[i] *= 2.0;	// make 1st differentail more significant
		sumsqr+= fvec[i] * fvec[i];
	}
	if (ngpd_ringing_debug > 1)
		printf("Call %d posn[0]=%d, scale=%g, posn[1]=%d, scale=%g => sumsqr=%g\n", num_calls, filt_posn[0], x[0], filt_posn[1], x[1], sumsqr);
	num_calls++;
	return 0;
}

int ngpd_ringing_least_squares(int cn, MOD_IMAGE *ave_mod,  int min_delay, int max_delay, int max_posn, double max_diff1_in, int min_posn_in, int max_ring_posn1_in, int *del_min, int *del_max, double *scale_min, double *scale_max)
{
	int i;
	double *workspace= NULL;
	double *fvec=NULL;
	double fit_params[NGPD_MAX_NUM_RINGING_TERMS*XSP_NUM_RINGING_PASS];
	int *int_ws=NULL, int_ws_size;
	int lmdif_m, lmdif_n;
	int p_l_work;
	int l_work;
	double tol;
	int info[XSP_NUM_RINGING_PASS];
	double residuals[XSP_NUM_RINGING_PASS];
	int best_pass, pass, num_pass=33;
	double sumsqr, min_residual;
	int posns[XSP_NUM_RINGING_PASS][NGPD_MAX_NUM_RINGING_TERMS];
	int num_terms=2;

	debug_cn = cn;
	filt_posn[0] = min_posn_in;
	filt_posn[1] = max_ring_posn1_in;
	npts_tail = 60;
	max_diff1 = max_diff1_in;
	diff1 = (double*)id_get_ptr(ave_mod, 0, X3_MOD_ROW_DIFF1, 0);
	diff2 = (double*)id_get_ptr(ave_mod, 0, X3_MOD_ROW_DIFF2, 0);

	if (diff1[min_posn_in] < 0)
	{
		for (i=min_posn_in; diff1[i] < 0 && i >0; i--);
		tail_start = i+1;
	}
	else
		tail_start = min_posn_in;

	if (npts_tail+tail_start > ave_mod->head.num_x)
		npts_tail = ave_mod->head.num_x-tail_start;

	if (ngpd_ringing_debug)
		sv_printf(cn, "# tail_start=%d,nptrs_tail=%d\n", tail_start, npts_tail);


	lmdif_n = NGPD_MAX_NUM_RINGING_TERMS;
	lmdif_m = npts_tail*2;

	l_work =  lmdif_m*lmdif_n+5*lmdif_n+lmdif_m;	

	int_ws_size = lmdif_n;

	fvec        = malloc(lmdif_m*sizeof(double));
	workspace = malloc (l_work*sizeof(double));
	int_ws    = malloc (int_ws_size*sizeof(int));

	if (fvec == NULL || workspace == NULL || int_ws == NULL)
	{
		sv_printf(cn, "! ERROR: Cannot malloc memory for x vector\n");
		goto exit_error;
	}

	cur_mod = ave_mod;
	best_pass = 0;

	if (ngpd_ringing_debug)
		sv_printf(cn, "# Trialing fits around min_posn=%d, max_posn=%d, max_ring_posn1=%d\n", min_posn_in, max_posn, max_ring_posn1_in);
	for (pass=0; pass < num_pass; pass++)
	{
		filt_posn[0] = min_posn_in-max_posn;
		filt_posn[1] = max_ring_posn1_in-max_posn;
		filt_posn[0] += (pass % 3)-1;
		filt_posn[1] += pass/3 - 5;
		for (i=0; i< num_terms; i++)
		{
			if (filt_posn[i] < min_delay || filt_posn[i] > max_delay)
				break;
		}

		if (i < num_terms || filt_posn[0] == filt_posn[1])
		{
			residuals[pass] = -1.0;
			continue;
		}
		fit_params[0+lmdif_n*pass] = -diff1[min_posn_in]/max_diff1;
		fit_params[1+lmdif_n*pass] = -diff1[max_ring_posn1_in]/max_diff1;

		if (ngpd_ringing_debug > 1 )
		{
			sv_printf(cn, "# Pass %d: seed %g @ %d, %g @ %d\n", pass, fit_params[0+lmdif_n*pass], filt_posn[0], fit_params[1+lmdif_n*pass], filt_posn[1]);
		}
		for (i=0;i < num_terms; i++)
			posns[pass][i] = filt_posn[i];
		num_calls = 0;
		tol=0.000001;

		/* Find best parameters */
		lmdif1_(ngpd_ringing_residuals, &lmdif_m, &lmdif_n, fit_params+lmdif_n*pass, fvec, &tol, info+pass, int_ws, workspace, &l_work);

		sumsqr= 0.0;
		for (i=0; i<lmdif_m; i++)
		{
			double scale_err;
			if (fvec[i] > 0)
			{
				scale_err = fvec[i]/(max_diff1/10);	// This should reach 1 when ringing reaches + 10%
				scale_err = 10*scale_err*scale_err+1;
				fvec[i] *= scale_err;
			}
			if (i < npts_tail)
				fvec[i] *= 2.0;	// make 1st differentail more significant

			sumsqr += fvec[i]*fvec[i];
		}
		residuals[pass] = sumsqr;
		if (ngpd_ringing_debug)
		{
			i = 1;
			sv_printf(cn, "# Pass %d, Solver returns %d after %d calls, tol =%g, residual=%g\n", pass, info[pass], num_calls, tol, sumsqr);
			sv_printf(cn, "# Note machine precission = %g, sqrt(machine prec) = %g\n", dpmpar_(&i), sqrt(dpmpar_(&i)));
			sv_printf(cn, "# Pass %d: result %g @ %d, %g @ %d => residual=%g\n", pass, fit_params[0+lmdif_n*pass], filt_posn[0], fit_params[1+lmdif_n*pass], filt_posn[1], sumsqr);
		}
	}
/*	for (i=0; i<num_pass && residuals[i] < 0; i++);
	
	if (i== num_pass)
	{
		sv_printf(cn, "# WARNING Cannot find any usable filters WHY?\n");
		best_pass = 0;
	}
	else
	{
		
		min_residual = residuals[i];
*/
	min_residual = DBL_MAX;
	best_pass = 0;
	for (i=0; i<num_pass; i++)
	{
		if (residuals[i] >= 0.0 && residuals[i] < min_residual)
		{
			best_pass = i;
			min_residual = residuals[i];
		}
	}
	if (ngpd_ringing_debug)
		sv_printf(cn, "# Picked pass %d, min_residual %g filter : %g @ %d, %g @ %d\n", best_pass, residuals[best_pass],  fit_params[0+lmdif_n*best_pass], posns[best_pass][0], fit_params[1+lmdif_n*best_pass], posns[best_pass][1] );
	*del_min = posns[best_pass][0];
	*del_max = posns[best_pass][1];
	if (*del_min >= min_delay && *del_min <= max_delay )
		*scale_min = fit_params[2*best_pass+0];
	else
	{
		*del_min = min_delay;
		*scale_min = 0.0;
	}

	if (*del_max >= min_delay && *del_max <= max_delay)
		*scale_max = fit_params[2*best_pass+1];
	else
	{
		*del_max = min_delay;
		*scale_max = 0.0;
	}
	free(fvec);
	free(workspace);
	free(int_ws);

	return 0;

exit_error:

	free(fvec);
	free(workspace);
	free(int_ws);

	return -1;
}
#endif

#endif

void ngpd_cal_accumulate_ave(int16_t * ana_ptr, u_int16_t * dig_ptr,  int offset, MOD_IMAGE3D *mod, int mod_t, int pre, int post, double baseline, int ana_inc, int dig_inc, u_int16_t trig_mask, u_int16_t wide_mask_a, u_int16_t wide_mask_b)
{
	int j;
	int y;
	int num_x = mod->head.num_x;
	double *fptr;
	fptr = (double *) id_get_ptr(mod, 0, 0, mod_t);

	for (j= -pre; j<post; j++)
	{
		y = ana_ptr[ana_inc*(offset+j)];
		*fptr += ADC_SCALE*y-baseline;

		if (dig_ptr[dig_inc*(offset+j)] & trig_mask)
			fptr[NGPD_CAL_MOD_ROW_TRIG*num_x]++;

		if (dig_ptr[dig_inc*(offset+j)] & wide_mask_a)
			fptr[NGPD_CAL_MOD_ROW_WIDE_A*num_x]++;

		if (dig_ptr[dig_inc*(offset+j)] & wide_mask_b)
			fptr[NGPD_CAL_MOD_ROW_WIDE_B*num_x]++;
		fptr++;
	}
}

/* FIXME: This will not work in cases when do_upper is required to get the data in 2 attempts
*/
int ndpg_cal_accumulate_xtk(NGPDMeasureTrigger *set, NGPDMeasureTriggerThread *thread_specific, int do_upper, int src, int offset, int mod_t)
{
	int16_t * ana_ptr, *p;
	u_int16_t * dig_ptr;
	double baseline;
	int ana_inc, dig_inc;
	int base_bit_posn;
	MOD_IMAGE3D *xtk_mod;
	int i, dst;
	int card, stream;
	double *dp;
	int *nump;
	
	switch(mod_t)
	{
	case NGPD_CAL_MOD_T_ALL:
		xtk_mod = set->private->xtk_all_mod[set->private->phase];
		break;
		
	case NGPD_CAL_MOD_T_NEUTRON:
		xtk_mod = set->private->xtk_neutron_mod[set->private->phase];
		break;
	
	case NGPD_CAL_MOD_T_GAMMA:
		xtk_mod = set->private->xtk_gamma_mod[set->private->phase];
		break;
	default:
		printf("Coding error: ndpg_cal_accumulate_xtk mod_t=%d\n", mod_t);
		return -1;
	}
	for (card=thread_specific->first_card; card<=thread_specific->last_card; card++)
	{
		for (stream=set->private->first_stream; stream<=set->private->last_stream; stream++)
		{
			dst = ngpd_cal_setup_trig_or_measure_get_chan(set->path, set->use_measure, do_upper, stream);
			if (dst < 0)
				continue;
			dst += card*set->private->chans_per_card;
			if (dst >= set->first_chan && dst <= set->last_chan)
			{
				ana_ptr = (int16_t *)ngpd_scope_mod_get_ptr(set->private->scope_mod, card, stream);
				ngpd_scope_stream_details(set->private->scope_mod, card, stream, NULL, &base_bit_posn, &dig_ptr, NULL, NULL, NULL, &ana_inc, &dig_inc);
				if (dig_ptr == NULL || ana_ptr == NULL)
				{
					snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_trigger:ERROR reading scope data pointers:'%s'", ngpd_get_error_message());
					return -1;
				}
				dp = (double *) id_get_ptr((void *)xtk_mod, 0, dst, src);
				baseline = 0.0;
				ana_ptr += ana_inc*(offset-set->pre);
				dig_ptr += dig_inc*(offset-set->pre);
				p = ana_ptr;
				for (i=0; i<set->pre/3; i++)
				{
					baseline += *p; 
					p += ana_inc;
				}
				baseline /= (set->pre/3);
				p = ana_ptr;
				for (i=0; i<set->pre+set->post; i++)
				{
					*dp++ += ADC_SCALE*(*p - baseline);
					p += ana_inc;
				}
				nump = (int *)id_get_ptr(set->private->num_ave_xtk_mod[set->private->phase], dst, src, mod_t);
				(*nump)++;
			}
		}
	}
	return 0;
}
	
int ngpd_cal_divide_average(MOD_IMAGE3D *mod, int row, int t, int n)
{
	int i;
	double *fptr;

	if (mod == NULL || n == 0)
		return -1;

	fptr = (double *)id_get_ptr(mod, 0, row, t);
	for (i=0; i<mod->head.num_x; i++)
		*fptr++ /= n;
	return 0;
}

int ngpd_cal_measure_pulse(int16_t * ana_ptr, int offset, int pre, int post, double *lhs, double *rhs, double *height, int num_lhs, int num_rhs, int ana_inc, int dig_inc)
{
	int i;
	int16_t *p;
	int num_x = pre+post;
	int sum_lhs, sum_rhs, hgt;

	hgt = 0;
	p = ana_ptr+ana_inc*(offset-pre);
	for (i=0; i<num_x; i++)
	{
		if (*p > hgt)
			hgt = *p;
		p += ana_inc;
	}
	sum_lhs = 0;
	p = ana_ptr+ana_inc*(offset-pre);
	for (i=0;i<num_lhs; i++)
	{
		sum_lhs += *p;
		p +=ana_inc;
	}
	p = ana_ptr+ana_inc*(offset-pre+num_x-num_rhs);
	sum_rhs = 0;
	for (i=0; i<num_rhs; i++)
	{
		sum_rhs += *p;
		p +=ana_inc;
	}
	*lhs    = (double)ADC_SCALE*sum_lhs/num_lhs;
	*rhs    = (double)ADC_SCALE*sum_rhs/num_rhs;
	*height = (double)ADC_SCALE*hgt - *lhs;
	return 0;
}
#define GREY_RESET 4

void ngpd_cal_accumulate_persist(int16_t * ana_ptr, u_int16_t *dig_ptr,  int offset, MOD_IMAGE3D *mod, int mod_t, int pre, int post, int base_line, int divide, int cap_len, u_int16_t bmask0, u_int16_t bmask1, u_int16_t bmask2, int ana_inc, int dig_inc)
{
	u_int32_t * iptr;
	u_int32_t *p;
	int i, j, k;
	int x, y, last_y;
	int num_y = mod->head.num_y;
	int digital, prev_digital=0;
	static int grey_reset;
	int num_x= pre+post;
	int len, inc;
	int dig_y0, dig_y1;
	int num_digital=0;
	u_int16_t bmask;
	if (bmask0) num_digital++;
	if (bmask1) num_digital++;
	if (bmask2) num_digital++;

	iptr = id_get_ptr(mod, 0, 0, mod_t);

	for (j=-pre; j< post; j++)
	{
		x = j+pre;
		y = ADC_SCALE*ana_ptr[ana_inc*(offset+j)];
		if (dig_ptr != NULL)
			digital = dig_ptr[dig_inc*(offset+j)];
		else
			digital = 0;
	
		for (i=0; i<num_digital; i++)
		{
			if (num_digital == 1)
			{
				dig_y1=0;
				dig_y0=num_y-1;
				bmask = bmask0;
			}
			else
			{
				int dy=num_y/num_digital;
				dig_y1=dy/8+i*dy;
				dig_y0=7*dy/8+i*dy;
				switch (i)
				{
				case 0: bmask = bmask0; break;
				case 1: bmask = bmask1; break;
				case 2: bmask = bmask2; break;
				}
			}

			if ((digital & bmask) != (prev_digital & bmask))
			{

				if (cap_len == 0)
				{
					p = iptr+x + (dig_y1+grey_reset) * num_x;
					for (k=dig_y1+grey_reset; k<=dig_y0; k+= GREY_RESET)
					{
						(*p)++;
						p += num_x*GREY_RESET;
					}
					if ((digital & bmask) == 0)
						grey_reset = (grey_reset+1) % GREY_RESET;
				}
				else
				{
					p = iptr+x+dig_y1*num_x;
					for (k=dig_y1; k<=dig_y0; k++)
					{
						(*p)++;
						p += num_x;
					}
				}
			}
			else
			{
				p = iptr+x;
				if (digital & bmask)
					p += num_x*dig_y1;
				else
					p += num_x*dig_y0;
				(*p)++;
			}
		}
		prev_digital = digital;
		y -= base_line;
		y /= divide;
		if (y < 0) y = 0;
		if (y >= num_y)
			y = num_y-1;
		if (x != 0)
		{
			if (cap_len)
			{
				len = 1+abs(last_y-y);
				if (len > cap_len) len = cap_len;
				inc = 100/len;
				if (inc == 0) inc = 1;
			}
			else
				inc = 1;
/*			printf("x=%d, last_y=%d, mid point=%d, y=%d, inc=%d\n", x, last_y, (y+last_y)/2, y, inc); */
			if (last_y > y)
			{
#if 0
				p = iptr+x-1 + (num_y-1-last_y) * num_x;
				for (k=last_y; k>(last_y+y)/2; k--)
				{
					(*p)+= inc;
					p += num_x;
				}
				p ++;	/* move across 1 and back down a line */
/*				p += num_x;
				k--; */
				for (; k>y; k--)
				{
					(*p)+= inc;
					p += num_x;
				}
#else
				p = iptr+ x + (num_y-1-last_y) * num_x;
				for (k=last_y; k>=y; k--)
				{
					(*p)+= inc;
					p += num_x;
				}
#endif
			}
			else if (last_y == y)
			{
#if 0
				p = iptr+x-1 + (num_y-1-last_y) * num_x;
				(*p)+= inc;
#else
				p = iptr+x + (num_y-1-last_y) * num_x;
				(*p)+= inc;
#endif
			}
			else
			{
#if 0
				p = iptr+x-1 + (num_y-1-last_y) * num_x;
				for (k=last_y; k<=(last_y+y)/2; k++)
				{
					(*p)+= inc;
					p -= num_x;
				}
				p ++;	/* move across 1 and back down a line */
/*				p += num_x;
				k--; */
				for (; k< y; k++)
				{
					(*p)+= inc;
					p -= num_x;
				}
#else
				p = iptr+x + (num_y-1-last_y) * num_x;
				for (k=last_y; k<=y; k++)
				{
					(*p)+= inc;
					p -= num_x;
				}
#endif
			}
		}
		last_y = y;
	}
}

