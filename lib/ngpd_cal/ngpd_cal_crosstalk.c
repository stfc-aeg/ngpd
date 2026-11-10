/* 	
	ngpd_cal_triggers.c
	ngpd calibration library
	Created: wih73   11/08/2022
	Derived from da.server ngpd_calib_triggers.c
	8/7/2026 W. Helsby: It appears that the conclusion that ngpd_cal_measure_trigger() had to be 2 pass was wrong. 
		By making NGZMP_SCOPE_ALT_DIFF_OUT_DIG4 which discards the armed signal, ther are now 4 digital signals per channel, so using 10 streams we can see 
		all 8 channels plsu the 4*8= 32bits of digital that are required for calib triggers.
		But we also no longer need_upper, so the same code can do measure crosstalk, provided its threading works correctly.
		So this file is being deprecated unless I find a problem finishing ngpd_cal_measure_trigger()

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
int ndpg_cal_accumulate_crosstalk_xtk(NGPDMeasureCrosstalk *set, int do_upper, int src, int offset, int mod_t);

int (*ngpd_cal_test_quit)();


#define ADC_SCALE 2


void *ngpd_measure_crosstalk_do(void *set_vp);

/**
 * @ingroup calibration
 * Build a structure with default settings to be passed to {@link ngpd_cal_measure_crosstalk()}.
 * The settings structure is created with a call to this function. The default settings can then be changed by the calling code 
 * before this pointer is passed to {@link ngpd_cal_measure_crosstalk()}.
 * @n
 * Afterwards the structure is freed by calling {@link ngpd_cal_measure_crosstalk_delete()}
 * @param path 	A handle to the top level of the ngpd system returned from any of the config functions  {@link ngpd_config_adq14} or {@link ngpd_config_ngzmp()}.
 * @return A pointer to the allocated structure or NULL on error, with error message available from ngpd_cal_get_error_message()
**/

NGPDMeasureCrosstalk *ngpd_cal_measure_crosstalk_default(int path)
{
	NGPDMeasureCrosstalk *set;
	int i;
	int num_chan;
	
	num_chan = ngpd_get_num_chan(path);
	if (num_chan <= 0)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk_default: Cannot read num_chan from path %d, %s", path, ngpd_get_error_message());
		return NULL;
	}

	if ((set=(NGPDMeasureCrosstalk *)malloc(sizeof(NGPDMeasureCrosstalk))) == NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk_default: Cannot malloc space for setup");
		return NULL;
	}
	
	memset(set, 0, sizeof(NGPDMeasureCrosstalk));
	if ((set->min_event = (double*)malloc(sizeof(double)*NGPD_MAX_CHANS)) == NULL)
		goto exit_error;
	if ((set->max_event = (double*)malloc(sizeof(double)*NGPD_MAX_CHANS)) == NULL)
		goto exit_error;
	if ((set->min_hgt_neutron = (double*)malloc(sizeof(double)*NGPD_MAX_CHANS)) == NULL)
		goto exit_error;
	if ((set->max_hgt_neutron = (double*)malloc(sizeof(double)*NGPD_MAX_CHANS)) == NULL)
		goto exit_error;
	
	set->path = path;
	set->num_chan = num_chan;
	set->num_cards = ngpd_get_num_cards(path);
	set->chans_per_card = ngpd_get_chans_per_card(path);
	set->scope_mod = ngpd_scope_get_mod(path); 

	set->pre=25;
	set->post=170;
	set->num_pass=1;
	set->persist_num_y = 1200;
	set->trig_min_otd=-1;
	set->trig_max_otd=-1;
	set->num_phase=1;
	set->scale_sigma = 5;
	set->auto_neutron_height=0;
	set->mca_scale=1.0/16.0;		// Scale 0..65535 downto to 0..4095 */
	set->max_width = 1;
	set->max_fine_time = 1;

	for (i=0;i<NGPD_MAX_CHANS; i++)
		set->variable_width_mode[i] = 0;

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
	free(set->min_event);
	free(set->max_event);
	free(set->min_hgt_neutron);
	free(set->max_hgt_neutron);
	free(set);
	return NULL;
}
/**
 * @ingroup calibration
 * Free a structure of settings built by {@link ngpd_cal_measure_crosstalk_default()}
 * @param set 	Pointer to NGPDMeasureCrosstalk created by {@link ngpd_cal_measure_crosstalk_default()}
 * @return none
 */
void ngpd_cal_measure_crosstalk_delete(NGPDMeasureCrosstalk *set)
{
	free(set->min_event);
	free(set->max_event);
	free(set->min_hgt_neutron);
	free(set->max_hgt_neutron);
	free(set);
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
 * The settings are determined by creating default settings with {@link ngpd_cal_measure_crosstalk_default()}.
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
 
 * @param set 	A pointer to the settings of type {@link NGPDMeasureCrosstalk}
 * @return		Returns 0 on success or -ve on error with error message available from ngpd_cal_get_error_message()
* */
int ngpd_cal_measure_crosstalk(NGPDMeasureCrosstalk *set)
{
	NGPDMeasureCrosstalk *settings[NGPD_MAX_NUM_CARDS];
	char mod_name[NGPD_MAX_MODNAME+20];
	int i, j, k, phase, chan, card;
	int first_card, last_card;
	int flags, save_flags;


//	u_int16_t *dig_ptr, *ana_ptr;
	int *iptr;
	int rc;
	char * ave_mod_labels[] = {"Signal", "Trigger", "WideA Meas", "WideB Base", "Fit", "Corrected", "Diff1", NULL };

	memset(settings, 0, sizeof(settings));
	for (card=0; card<NGPD_MAX_NUM_CARDS; card++)
	{
		settings[card] = (NGPDMeasureCrosstalk *)malloc(sizeof(NGPDMeasureCrosstalk));
		if (settings[card] == NULL)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk: Cannot malloc space for setup");
			goto exit_error;
		}
	}

	if (set->auto_neutron_height && set->num_phase==1)
		set->num_phase = 2;
		
	if (ngpd_cal_measure_crosstalk_validate(set))
		goto exit_error;

	for (phase=0; phase < set->num_phase; phase++)
	{
		sprintf(mod_name, "mca_phase%d_%d", phase+1, set->num_chan);
		set->mca_mod[phase] = id_mkmod3d_err_msg(mod_name, 4096, NGPD_CAL_MOD_NUM_T, set->num_chan, "ADC Value", "All/Neutron/Gamma", "Channel", 
				NULL, DATA_LONG, &(set->mca_head[phase]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
		if (set->mca_mod[phase] == NULL)
			goto exit_error;
		sprintf(set->mca_mod[phase]->head.x_label, "ADC Values/%g", 1.0/set->mca_scale);
		if (!set->no_clear)
			id_clear_mod((MOD_IMAGE *)set->mca_mod[phase]);

		if (set->do_persist)
		{
			for (i=set->first_chan;i<=set->last_chan; i++)
			{
				sprintf(mod_name, "%s_phase%d_%02d", set->persist_name, phase+1, i);
				set->persist_mod[phase][i] = id_mkmod3d_err_msg(mod_name, set->pre+set->post, set->persist_num_y, NGPD_CAL_MOD_NUM_T, "Time", "ADC Value", "All/Neutron/Gamma", 
					NULL, DATA_LONG, &(set->persist_head[phase][i]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
				if (set->persist_mod[phase][i] == NULL)
					goto exit_error;
				if (!set->no_clear)
					id_clear_mod((MOD_IMAGE *)(set->persist_mod[phase][i]));
			}
		}
		if (set->do_ave)
		{
			for (i=set->first_chan;i<=set->last_chan; i++)
			{
				sprintf(mod_name, "%s_phase%d_%02d", set->ave_name, phase+1, i);
				set->ave_mod[phase][i] = id_mkmod3d_err_msg(mod_name, set->pre+set->post, NGPD_CAL_MOD_NUM_ROW, NGPD_CAL_MOD_NUM_T, "Time", "ADC Value scaled 0...63353", "All/Neutron/Gamma", 
				 ave_mod_labels, DATA_DOUBLE, &(set->ave_head[phase][i]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
				if (set->ave_mod[phase][i] == NULL)
					goto exit_error;
				if (!set->no_clear)
					id_clear_mod((MOD_IMAGE *)(set->ave_mod[phase][i]));

			}
			sprintf(mod_name, "num_ave_phase%d_%d", phase+1, set->num_chan);
			set->num_ave_mod[phase] = id_mkmod_err_msg(mod_name, set->num_chan, NGPD_CAL_MOD_NUM_T, "Chan", "All/Neutron/Gamma",  DATA_LONG, &(set->num_ave_head[phase]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
			if (set->num_ave_mod[phase] == NULL)
				goto exit_error;
			if (!set->no_clear)
				id_clear_mod(set->num_ave_mod[phase]);
		}
		if (set->do_xtk)
		{
			sprintf(mod_name, "%s_phase%d_all%dx%d", set->xtk_name, phase+1, set->num_chan, set->num_chan);
			set->xtk_all_mod[phase] = id_mkmod3d_err_msg(mod_name, set->pre+set->post, set->num_chan, set->num_chan, "Time", "Dest Channel", "Src Channel",
				 NULL, DATA_DOUBLE, &(set->xtk_all_head[phase]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
			if (set->xtk_all_mod[phase] == NULL)
				goto exit_error;
			if (!set->no_clear)
				id_clear_mod((MOD_IMAGE *)(set->xtk_all_mod[phase]));

			sprintf(mod_name, "%s_phase%d_neutron%dx%d", set->xtk_name, phase+1, set->num_chan, set->num_chan);
			set->xtk_neutron_mod[phase] = id_mkmod3d_err_msg(mod_name, set->pre+set->post, set->num_chan, set->num_chan, "Time", "Dest Channel", "Src Channel",
				 NULL, DATA_DOUBLE, &(set->xtk_neutron_head[phase]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
			if (set->xtk_neutron_mod[phase] == NULL)
				goto exit_error;
			if (!set->no_clear)
				id_clear_mod((MOD_IMAGE *)(set->xtk_neutron_mod[phase]));

			sprintf(mod_name, "%s_phase%d_gamma%dx%d", set->xtk_name, phase+1, set->num_chan, set->num_chan);
			set->xtk_gamma_mod[phase] = id_mkmod3d_err_msg(mod_name, set->pre+set->post, set->num_chan, set->num_chan, "Time", "Dest Channel", "Src Channel",
				 NULL, DATA_DOUBLE, &(set->xtk_gamma_head[phase]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
			if (set->xtk_gamma_mod[phase] == NULL)
				goto exit_error;
			if (!set->no_clear)
				id_clear_mod((MOD_IMAGE *)(set->xtk_gamma_mod[phase]));

			sprintf(mod_name, "num_ave_xtk_phase%d_%d", phase+1, set->num_chan);
			set->num_ave_xtk_mod[phase] = id_mkmod3d_err_msg(mod_name, set->num_chan, set->num_chan, NGPD_CAL_MOD_NUM_T, "Dest Chan", "Src Chan", "All/Neutron/Gamma",
				NULL, DATA_LONG, &(set->num_ave_xtk_head[phase]), ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE);
			if (set->num_ave_xtk_mod[phase] == NULL)
				goto exit_error;
			if (!set->no_clear)
				id_clear_mod(set->num_ave_xtk_mod[phase]);
		}
	}


	save_flags = ngpd_get_run_flags(set->path);
	flags = (save_flags & ~NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS) | NGPD_RUN_FLAGS_SCOPEMODE | NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD;
	ngpd_set_run_flags(set->path, flags);

	first_card = set->first_chan/set->chans_per_card;
	last_card  = set->last_chan/set->chans_per_card;

	for (phase=0; phase<set->num_phase; phase++)
	{
		if ((rc=ngpd_cal_setup_all_chan(set->path, -1, set->use_measure?NGPD_SCOPE_SEL0TO5_BASELINE_SUB:NGPD_SCOPE_SEL0TO5_DIFF_TRIG_OUT, 
				&set->first_stream, &set->last_stream,  &set->trig_offset, &set->wide_offset_a,  &set->wide_offset_b, &set->neutron_offset, 1)) < 0)
		{
			goto exit_error;
		}

		memcpy(settings[0], set, sizeof(NGPDMeasureCrosstalk));
		settings[0]->phase = phase;
		if (set->num_cards > 1 && set->disable_multi_thread==0)	/* Moved all this here so info from ngpd_calib_setup_trig_or_servo is correctly copied to all threads */
		{
			for (i=1;i<set->num_cards; i++)
				memcpy(settings[i], settings[0], sizeof(NGPDMeasureCrosstalk));

			for (i=0;i<set->num_cards; i++)
			{
				settings[i]->thread_index = i;
				settings[i]->first_card = i;
				settings[i]->last_card = i;
			}
		}
		else
		{
			settings[0]->thread_index = 0;
			settings[0]->first_card = first_card;
			settings[0]->last_card  = last_card;
		}

		if ((int)ngpd_measure_crosstalk_do(set) < 0)
			goto exit_error;

		for (i=set->first_chan; i<= set->last_chan; i++)
		{
			if (set->min_hgt_neutron[i] >= 0.0 || set->max_hgt_neutron[i] >=0.0)
			{
				int *nnum, *gnum, *anum;
				nnum = (int *)id_get_ptr(set->mca_mod[phase], 0, NGPD_CAL_MOD_T_NEUTRON, i);
				gnum = (int *)id_get_ptr(set->mca_mod[phase], 0, NGPD_CAL_MOD_T_GAMMA, i);
				anum = (int *)id_get_ptr(set->mca_mod[phase], 0, NGPD_CAL_MOD_T_ALL, i);
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
						nsum = (double *)id_get_ptr(set->ave_mod[phase][i], 0, j, NGPD_CAL_MOD_T_NEUTRON);
						gsum = (double *)id_get_ptr(set->ave_mod[phase][i], 0, j, NGPD_CAL_MOD_T_GAMMA);
						asum = (double *)id_get_ptr(set->ave_mod[phase][i], 0, j, NGPD_CAL_MOD_T_ALL);
						for (k=0;k<set->pre+set->post; k++)
							*asum++ = *gsum++ + *nsum++;
					}
					nnum = (int *)id_get_ptr(set->num_ave_mod[phase], i, NGPD_CAL_MOD_T_NEUTRON, 0);
					gnum = (int *)id_get_ptr(set->num_ave_mod[phase], i, NGPD_CAL_MOD_T_GAMMA, 0);
					anum = (int *)id_get_ptr(set->num_ave_mod[phase], i, NGPD_CAL_MOD_T_ALL, 0);
					*anum = *gnum + *nnum;
				}
				
				for (j=0; j<NGPD_CAL_MOD_NUM_T; j++)
				{
					int n;
					if ((n=set->num_ave_mod[phase]->data[i+set->num_chan*j]) > 0)
					{
						for (k=0; k<=NGPD_CAL_MOD_ROW_WIDE_B; k++)
						{
							ngpd_cal_divide_average(set->ave_mod[phase][i], k, j, n);
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
						nsum = (double *)id_get_ptr(set->xtk_neutron_mod[phase], 0, j, i);
						gsum = (double *)id_get_ptr(set->xtk_gamma_mod[phase], 0, j, i);
						asum = (double *)id_get_ptr(set->xtk_all_mod[phase], 0, j, i);
						for (k=0;k<set->pre+set->post; k++)
							*asum++ = *gsum++ + *nsum++;
						nnum = (int *)id_get_ptr(set->num_ave_xtk_mod[phase], j, i, NGPD_CAL_MOD_T_NEUTRON);
						gnum = (int *)id_get_ptr(set->num_ave_xtk_mod[phase], j, i, NGPD_CAL_MOD_T_GAMMA);
						anum = (int *)id_get_ptr(set->num_ave_xtk_mod[phase], j, i, NGPD_CAL_MOD_T_ALL);
						*anum = *gnum + *nnum;
						if (*anum> 0)
							ngpd_cal_divide_average(set->xtk_all_mod[phase], j, i, *anum);
						if (*gnum> 0)
							ngpd_cal_divide_average(set->xtk_gamma_mod[phase], j, i, *gnum);
						if (*nnum> 0)
							ngpd_cal_divide_average(set->xtk_neutron_mod[phase], j, i, *nnum);
					}
				}				
				else
				{
					int *anum;
					for (j=set->first_chan; j<=set->last_chan; j++)
					{
						anum = (int *)id_get_ptr(set->num_ave_xtk_mod[phase], j, i, NGPD_CAL_MOD_T_ALL);
						if (*anum> 0)
							ngpd_cal_divide_average(set->xtk_all_mod[phase], j, i, *anum);
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
				iptr = (int *)id_get_ptr(set->mca_mod[phase], 0, NGPD_CAL_MOD_T_ALL, chan);
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
				iptr = (int *)id_get_ptr(set->mca_mod[phase], 0, NGPD_CAL_MOD_T_ALL, chan);
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
	} /* End for phase */

	for (phase=0; phase<NGPD_CAL_MAX_PHASES; phase++)
	{
		for (i=0; i<NGPD_MAX_CHANS; i++)
		{
			if (set->persist_mod[phase][i] != NULL)
				munlink(set->persist_head[phase][i]);
			if (set->ave_mod[phase][i] != NULL)
				munlink(set->ave_head[phase][i]);
		}
		if (set->num_ave_mod[phase] != NULL)
			munlink(set->num_ave_head[phase]);
		if (set->mca_mod[phase] != NULL)
			munlink(set->mca_head[phase]);
	}

	ngpd_set_run_flags(set->path, save_flags);

	return 0;


exit_error:
	for (phase=0; phase<NGPD_CAL_MAX_PHASES; phase++)
	{
		for (i=0; i<NGPD_MAX_CHANS; i++)
		{
			if (set->persist_mod[phase][i] != NULL)
				munlink(set->persist_head[phase][i]);
			if (set->ave_mod[phase][i] != NULL)
				munlink(set->ave_head[phase][i]);
		}
		if (set->num_ave_mod[phase] != NULL)
			munlink(set->num_ave_head[phase]);
		if (set->mca_mod[phase] != NULL)
			munlink(set->mca_head[phase]);
		if (set->xtk_all_mod[phase] != NULL)
			munlink(set->xtk_all_mod[phase]);
		if (set->xtk_neutron_mod[phase] != NULL)
			munlink(set->xtk_neutron_mod[phase]);
		if (set->xtk_gamma_mod[phase] != NULL)
			munlink(set->xtk_gamma_mod[phase]);
		if (set->num_ave_xtk_mod[phase] != NULL)
			munlink(set->num_ave_xtk_head[phase]);
	}
	return -1;
}
/**
 * @ingroup internal
 * Validate the setting in the data structure of type {@link NGPDMeasureCrosstalk}.
 * This is called by {@link ngpd_cal_measure_crosstalk()} but could be called by the user code first if required.
 * @param set 	A pointer to the settings of type {@link NGPDMeasureCrosstalk}
 * @return		Returns 0 on success or -ve on error with error message available from ngpd_cal_get_error_message()
 */
int ngpd_cal_measure_crosstalk_validate(NGPDMeasureCrosstalk *set)
{
	if (set->pre < 1 || set->pre > 2000)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk:Expected pre in range 1..2000 not %d", set->pre);
		return -1;
	}
	if (set->post < 1 || set->post > 10000)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk: Expected post in range 1..10000 not %d", set->post);
		return -1;
	}

	if (set->col_sat < 0)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk: expected colour saturation control > 0, not %d", set->col_sat);
		return -1;
	}
	if (set->first_chan < 0 || set->first_chan>= set->num_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk: expected %d <= first_chan <= %d , not %d", 0, set->num_chan-1, set->first_chan);
		return -1;
	}

	if (set->last_chan < 0 || set->last_chan >= set->num_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk: expected %d <= last <= %d , not %d", 0, set->num_chan-1, set->last_chan);
		return -1;
	}
	if (set->last_chan < set->first_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk: Expected first_chan <=last_chan but first_chan=%d, last_chan = %d", set->first_chan, set->last_chan);
		return -1;
	}

	if (set->num_phase < 1 || set->num_phase > 3)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk: expected %d <= num_phase <= %d , not %d", 1, 3, set->num_phase);
		return -1;
	}

	if (!set->do_ave && ! set->do_persist)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk: Please enable at least 1 display mode");
		return -1;
	}
	if (set->playback != NULL && set->user_service == NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk: Playback command required but user_service function not specified");
		return -1;
	}				

	return 0;
}
/**
 * @ingroup internal
 * This internal function does the work of running ngpd_measure_crosstalk(). It arranged so it can be called as thread function to allow a thread per card.
  * @param set	Settings 
 */

void *ngpd_measure_crosstalk_do(void * set_vp)
{
	NGPDMeasureCrosstalk *set= (NGPDMeasureCrosstalk *)set_vp;
	int i, j, chan, card;
	u_int16_t *dig_ptr;
	int16_t *ana_ptr;
	int *iptr;
	double *fptr;
	double lhs, rhs;
	int pass;
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

	for (pass = 0; pass < ((set->phase==set->num_phase-1)?set->num_pass:1); pass++)
	{
		printf("Thread Index=%d : Card %d..%d : Source Channels = %d ..%d, pass %d\n", set->thread_index, set->first_card, set->last_card, 0, set->chans_per_card-1, pass);
		if (set->playback && (set->phase ==0 || pass >0))
		{
			sprintf(cmd, set->playback, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass);
			if (set->user_service(set->cn, cmd, 0, NULL, NULL) < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk:  Cannot run playback command '%s'", cmd);
				goto exit_error;
			}
		}

		if (set->first_card != set->last_card)
			card = -1;
		else
			card = set->first_card;

		rc = ngpd_dma_system_start(set->path, card, 0, 0, NULL);
		if (rc < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk: ERROR starting system: %s", ngpd_get_error_message());
			goto exit_error;
		}
		rc = ngpd_dma_wait_scope(set->path, card, &scope_status);
		if (rc < 0) {
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk: ERROR waiting for scope DMAs %s", ngpd_get_error_message());
			goto exit_error;
		}
		if (scope_status != 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk:ERROR: Scope status appears wrong at end of run = %08X", scope_status);
			goto exit_error;
		}
			
		for (card=set->first_card; card<=set->last_card; card++)
		{
			rc = ngpd_dma_read_scope(set->path, card, 0, 0, 0);
			if (rc < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk: ERROR reading scope data %s", ngpd_get_error_message());
				goto exit_error;
			}
			for (stream=set->first_stream; stream<=set->last_stream; stream++)
			{
				int num_raw_trig=0, num_pileup_pre=0, num_pileup_post=0;
				chan = ngpd_cal_setup_all_chan_get_chan(set->path, set->use_measure?NGPD_SCOPE_SEL0TO5_BASELINE_SUB:NGPD_SCOPE_SEL0TO5_DIFF_TRIG_OUT, stream);
				if (chan < 0)
					continue;
				chan += card*set->chans_per_card;
				if (chan >= set->first_chan && chan <= set->last_chan)
				{
					separate_neutrons = set->min_hgt_neutron[chan]>=0 && set->max_hgt_neutron[chan] >= 0;
					ana_ptr = (int16_t *)ngpd_scope_mod_get_ptr(set->scope_mod, card, stream);
					ngpd_scope_stream_details(set->scope_mod, card, stream, NULL, &base_bit_posn, &dig_ptr, NULL, NULL, NULL, &ana_inc, &dig_inc);
					//					sv_printf(set->cn, "# Stream %d:Chan %d: Determined num_fine=%d, fine_posn=%d, dig_ptr=%p, fine_ptr=%p\n", stream, chan, num_fine, fine_posn, dig_ptr, fine_ptr);
					if (dig_ptr == NULL || ana_ptr == NULL)
					{
						snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_measure_crosstalk:ERROR reading scope data pointers:'%s'", ngpd_get_error_message());
						goto exit_error;
					}
					if (set->phase != 0)
					{
						fptr = (double *)id_get_ptr(set->ave_mod[set->phase-1][chan], 0, NGPD_CAL_MOD_ROW_WIDE_A, 0);
						for (i=0; i<set->pre+set->post; i++)
							if (fptr[i] > 0.5)
								break;
						wide_start[stream] = i+1-set->pre;
					}

					trig_mask = 1 << (base_bit_posn+set->trig_offset);
					if (set->wide_offset_a < 0)
						wide_mask_a = 0;
					else
						wide_mask_a =  1 << (base_bit_posn+set->wide_offset_a);

					if (set->wide_offset_b < 0)
						wide_mask_b = 0;
					else
						wide_mask_b =  1 << (base_bit_posn+set->wide_offset_b);

					if (set->neutron_offset < 0)
						neutron_mask = 0;
					else
						neutron_mask =  1 << (base_bit_posn+set->neutron_offset);

					start_gap = set->pre+set->extra_pre;
					stop_gap  = set->post+set->extra_post;
					printf("measure_crosstalk: Card %d, Stream %d, chan %d. TRIG_MASK=%04X, WIDE_MASK_A=%04X, WIDE_MASK_B=%04X, set->use_measure=%d\n", card, stream, chan, trig_mask, wide_mask_a, wide_mask_b, set->use_measure);
					for (i=start_gap; i<set->scope_mod->head.num_t-stop_gap-1; i++)
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
								iptr = (int *) id_get_ptr(set->mca_mod[set->phase], j, mod_t, chan);
								(*iptr)++;
							}
							if (set->do_persist)
							{
								ngpd_cal_accumulate_persist(ana_ptr, dig_ptr, i, set->persist_mod[set->phase][chan], mod_t, set->pre, set->post, persist_baseline, 32, set->col_sat, trig_mask, wide_mask_a, wide_mask_b, ana_inc, dig_inc);
							}
							if (set->do_ave)
							{
								ngpd_cal_accumulate_ave(ana_ptr, dig_ptr, i, set->ave_mod[set->phase][chan], mod_t, set->pre, set->post, average_baseline, ana_inc, dig_inc, trig_mask, wide_mask_a, wide_mask_b );
								set->num_ave_mod[set->phase]->data[chan+set->num_ave_mod[set->phase]->head.num_x*mod_t]++;
							}
							if (set->do_xtk)
							{
								ndpg_cal_accumulate_crosstalk_xtk(set, 0, chan, i, mod_t);
							}
						} /* End IF Found event */
					} /* For i searching for reset */
					if (num_raw_trig < 1000)
						printf("# Warning only found %d triggers on channel %d\n", num_raw_trig, chan);
					else if (num_pileup_pre+num_pileup_post > num_raw_trig/2)
						printf("# Large fraction of events rejected on channel %d: num_raw_trig=%d, num_pileup_pre=%d, num_pileup_post=%d\n", chan, num_raw_trig, num_pileup_pre, num_pileup_post);
				} /* If Chan in range */
			} /* For stream */
			if (ngpd_cal_test_quit != NULL && ngpd_cal_test_quit())
			{
				printf("STOPPED By ^c\n");
				goto exit_error;
			}
		} /* For Slot  */
	} /* For Pass */
	printf("# Thread index %d finished: Card %d...%d , Stream %d...%d\n", set->thread_index, set->first_card, set->last_card, set->first_stream, set->last_stream);
	return (void *)(int64_t)0;

exit_error: return (void *)(int64_t)-1;
}

int ndpg_cal_accumulate_crosstalk_xtk(NGPDMeasureCrosstalk *set, int do_upper, int src, int offset, int mod_t)
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
		xtk_mod = set->xtk_all_mod[set->phase];
		break;
		
	case NGPD_CAL_MOD_T_NEUTRON:
		xtk_mod = set->xtk_neutron_mod[set->phase];
		break;
	
	case NGPD_CAL_MOD_T_GAMMA:
		xtk_mod = set->xtk_gamma_mod[set->phase];
		break;
	default:
		printf("Coding error: ndpg_cal_accumulate_xtk mod_t=%d\n", mod_t);
		return -1;
	}
	for (card=set->first_card; card<=set->last_card; card++)
	{
		for (stream=set->first_stream; stream<=set->last_stream; stream++)
		{
			dst = ngpd_cal_setup_all_chan_get_chan(set->path, set->use_measure?NGPD_SCOPE_SEL0TO5_BASELINE_SUB:NGPD_SCOPE_SEL0TO5_DIFF_TRIG_OUT, stream);
			if (dst < 0)
				continue;
			dst += card*set->chans_per_card;
			if (dst >= set->first_chan && dst <= set->last_chan)
			{
				ana_ptr = (int16_t *)ngpd_scope_mod_get_ptr(set->scope_mod, card, stream);
				ngpd_scope_stream_details(set->scope_mod, card, stream, NULL, &base_bit_posn, &dig_ptr, NULL, NULL, NULL, &ana_inc, &dig_inc);
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
					*dp++ += *p - baseline;
					p += ana_inc;
				}
				nump = (int *)id_get_ptr(set->num_ave_xtk_mod[set->phase], dst, src, mod_t);
				(*nump)++;
			}
		}
	}
	return 0;
}
