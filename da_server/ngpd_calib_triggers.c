#include "header.h"
#include "peak_fit.h"
#include "ngpd.h"


//int ngpd_measure_event_baseline(int * wspace, int i, int pre, int post, X3FloatAve * baseline, X3FloatAve *lhs, X3FloatAve *rhs, int num_lhs, int num_rhs);
int ngpd_measure_pulse(u_int16_t * ana_ptr, int offset, int pre, int post, double *lhs, double *rhs, double *height, int num_lhs, int num_rhs, int ana_inc, int dig_inc);
void ngpd_accumulate_ave(int16_t * ana_ptr, u_int16_t * dig_ptr,  int offset, MOD_IMAGE3D *mod, int mod_t, int pre, int post, double baseline, int ana_inc, int dig_inc, u_int16_t trig_mask, u_int16_t wide_mask_a, u_int16_t wide_mask_b);


#define MAX_PHASES 3
#define ADC_SCALE 2

typedef struct 
{
	int path, cn, thread_index;
	MOD_IMAGE3D *ave_mod[MAX_PHASES][NGPD_MAX_CHANS];
	mh_com *ave_head[MAX_PHASES][NGPD_MAX_CHANS];
	MOD_IMAGE3D *persist_mod[MAX_PHASES][NGPD_MAX_CHANS];
	mh_com *persist_head[MAX_PHASES][NGPD_MAX_CHANS];
	MOD_IMAGE *num_ave_mod[MAX_PHASES];
	mh_com *num_ave_head[MAX_PHASES];
	MOD_IMAGE3D *mca_mod[MAX_PHASES];
	mh_com *mca_head[MAX_PHASES];
	MOD_IMAGE *ave_by_width_mod[MAX_PHASES][NGPD_MAX_CHANS];
	mh_com *ave_by_width_head[MAX_PHASES][NGPD_MAX_CHANS];
	MOD_IMAGE *num_ave_by_width_mod[MAX_PHASES];
	mh_com *num_ave_by_width_head[MAX_PHASES];

	int do_persist, do_ave;
	int pre, post;
	int first_chan, last_chan;
	NGPDScopeModule * scope_mod;

	int no_clear;
	int num_pass;
	int col_sat;
	int persist_num_y;
	int extra_pre, extra_post;
	int trig_min_otd, trig_max_otd;
	int phase, num_phase;
	char *playback;
	int auto_neutron_height;
	double scale_sigma;
	int first_card, last_card;
	int use_measure, chans_per_card;
	double mca_scale;
	double *min_event, *max_event;
	double *min_hgt_neutron, *max_hgt_neutron;
	int max_width, max_fine_time;
	int first_stream, last_stream,  trig_offset, wide_offset_a, wide_offset_b, neutron_offset;
	char variable_width_mode[NGPD_MAX_CHANS];
	int need_upper;
} NGPDMeasureTrigger;

void *ngpd_measure_trigger_do(void *set_vp);

int ngpd_measure_trigger(int quals, int cn, int path)
{
	NGPDMeasureTrigger settings[NGPD_MAX_NUM_CARDS], *set;
	int num_chan, num_cards;
	char persist_name[NGPD_MAX_MODNAME+2], ave_name[NGPD_MAX_MODNAME+2];
	char mod_name[NGPD_MAX_MODNAME+20];
	int i, j, k, chan, card;
//	u_int16_t *dig_ptr, *ana_ptr;
	int *iptr, *p;
	double *fptr, *fp2;
//	double baseline, lhs, rhs, lhs4, rhs4;
	int pass;
	int first_card, last_card;
//	int found_reset;
//	int persist_baseline;
//	double average_baseline;
	int prev_event;
//	int start_gap, stop_gap;
	int phase;
	char cmd[2048];
	int wide_end[NGPD_SCOPE_MAX_STREAMS];
//	u_int16_t event_mask, otd_mask;
	int chans_per_card;
	int stream, rc;
	int flags, save_flags;
	int disable_multi_thread=0;
	pthread_t threads[NGPD_MAX_NUM_CARDS];
	double min_event[NGPD_MAX_CHANS], max_event[NGPD_MAX_CHANS];
	double min_hgt_neutron[NGPD_MAX_CHANS], max_hgt_neutron[NGPD_MAX_CHANS];
	NGPDScopeOptions scope_options;
	char * ave_mod_labels[] = {"Signal", "Trigger", "WideA Meas", "WideB Base", "Fit", "Corrected", "Diff1", NULL };

	set = &settings[0];
	set->do_persist=0;
	set->do_ave=0;
	set->pre=25;
	set->post=170;
	set->no_clear = 0;
	set->num_pass=1;
	set->col_sat = 0;
	set->persist_num_y = 1200;
	set->trig_min_otd=-1;
	set->trig_max_otd=-1;
	set->num_phase=1;
	set->playback=NULL;
	set->scale_sigma = 5;
	set->auto_neutron_height=0;
	set->mca_scale=1.0/16.0;		// Scale 0..65535 downto to 0..4095 */
	set->min_event = min_event;
	set->max_event = max_event;
	set->min_hgt_neutron = min_hgt_neutron;
	set->max_hgt_neutron = max_hgt_neutron;
	set->max_width = 1;
	set->max_fine_time = 1;

	for (i=0;i<NGPD_MAX_CHANS; i++)
		set->variable_width_mode[i] = 0;
		
	num_chan = ngpd_get_num_chan(path);
	if (num_chan <= 0)
	{
		sv_printf(cn, "! ERROR: Cannot read num_chan from path %d, %s\n", path, ngpd_get_error_message());
		return -1;
	}
	num_cards = ngpd_get_num_cards(path);
	set->chans_per_card = ngpd_get_chans_per_card(path);
	set->scope_mod = ngpd_scope_get_mod(path); 

	set->first_chan = 0;
	set->last_chan  = num_chan-1;

	if (quals & 1)
		set->pre=Qualifier[0].IntArg;
	if (quals & 2)
		set->post= Qualifier[1].IntArg;

	set->extra_pre = set->post;	// Check full fall time before start */
	set->extra_post = set->pre;	// Check full rise time after end */

	if (set->pre < 1 || set->pre > 2000)
	{
		sv_printf(cn, "! ERROR: Expected pre in range 1..2000 not %d\n", set->pre);
		return -1;
	}
	if (set->post < 1 || set->post > 10000)
	{
		sv_printf(cn, "! ERROR: Expected post in range 1..10000 not %d\n", set->post);
		return -1;
	}

	if (quals & 4)
	{
		set->do_persist=1;
		strncpy(persist_name, Qualifier[2].StringArg, NGPD_MAX_MODNAME);
		persist_name[NGPD_MAX_MODNAME]=0;
	}
	if (quals & 8)
	{
		set->do_ave=1;
		strncpy(ave_name, Qualifier[3].StringArg, NGPD_MAX_MODNAME);
		ave_name[NGPD_MAX_MODNAME]=0;
	}
	if (quals & 0x10)
		set->num_pass = Qualifier[4].IntArg;

	if (quals & 0x20)
		set->persist_num_y = Qualifier[5].IntArg;

	if (quals & 0x40)
		set->col_sat = Qualifier[6].IntArg;

	if (set->col_sat < 0)
	{
		sv_printf(cn, "! ERROR : expected colour saturation control > 0, not %d\n", set->col_sat);
		return -1;
	}
	if (quals & 0x80)
	{
		chan = Qualifier[7].IntArg;
		if (chan < 0 || chan>= num_chan)
		{
			sv_printf(cn, "! ERROR : expected %d <= chan <= %d , not %d\n", 0, num_chan-1, chan);
			return -1;
		}
		set->first_chan = chan;
		set->last_chan = chan;
	}

	if (quals & 0x100)
	{
		for (i=0;i<NGPD_MAX_CHANS; i++)
			min_event[i] = Qualifier[8].IntArg;
	}
	else
	{
		for (i=0;i<NGPD_MAX_CHANS; i++)
			min_event[i] = -1.0;
	}
	
	if (quals & 0x200)
	{
		for (i=0;i<NGPD_MAX_CHANS; i++)
			max_event[i] = Qualifier[9].IntArg;
/*		set->mca_scale = 800/Qualifier[9].IntArg;
		if (set->mca_scale == 0)
			set->mca_scale = 1;
		sv_printf(cn, "# Note scaling MCA energy values by %d\n", set->mca_scale);
*/
	}
	else
	{
		for (i=0;i<NGPD_MAX_CHANS; i++)
			max_event[i] = -1;
	}

	if (quals & 0x400)
	{
		for (i=0;i<NGPD_MAX_CHANS; i++)
			min_hgt_neutron[i] = Qualifier[10].IntArg;
	}
	else
	{
		for (i=0;i<NGPD_MAX_CHANS; i++)
			min_hgt_neutron[i] = -1.0;
	}
	
	if (quals & 0x800)
	{
		for (i=0;i<NGPD_MAX_CHANS; i++)
			max_hgt_neutron[i] = Qualifier[11].IntArg;
	}
	else
	{
		for (i=0;i<NGPD_MAX_CHANS; i++)
			max_hgt_neutron[i] = -1;
	}

	if (quals & 0x1000)
		set->extra_pre = Qualifier[12].IntArg;
	if (quals & 0x2000)
		set->extra_post = Qualifier[13].IntArg;
	if (quals & 0x4000)
		set->playback = Qualifier[14].StringArg;

	if (quals & 0x8000)
		set->num_phase = 2;

	if (quals & 0x10000)
		set->num_phase = 3;
	if (quals & 0x20000)
		set->auto_neutron_height = 1;

	set->use_measure = !!(quals & 0x40000);
	disable_multi_thread = !!(quals & 0x80000);
	if (quals & 0x100000)
	{
		chan = Qualifier[20].IntArg;
		if (chan < 0 || chan >= num_chan)
		{
			sv_printf(cn, "! ERROR : expected %d <= first <= %d , not %d\n", 0, num_chan-1, chan);
			return -1;
		}
		set->first_chan = chan;
	}
	if (quals & 0x200000)
	{
		chan = Qualifier[21].IntArg;
		if (chan < 0 || chan>= num_chan)
		{
			sv_printf(cn, "! ERROR : expected %d <= last <= %d , not %d\n", 0, num_chan-1, chan);
			return -1;
		}
		set->last_chan = chan;
	}
	if (quals & 0x400000)
		set->max_width = Qualifier[22].IntArg;

	if (quals & 0x800000)
		set->max_fine_time = Qualifier[23].IntArg;

	if (set->auto_neutron_height && set->num_phase==1)
		set->num_phase = 2;
	if (!set->do_ave && ! set->do_persist)
	{
		sv_printf(cn, "! ERROR : Please enable at least 1 display mode\n");
		return -1;
	}
	for (phase=0;phase<MAX_PHASES; phase++)
	{
		for (i=0;i<NGPD_MAX_CHANS; i++)
		{
			set->persist_mod[phase][i] = NULL;
			set->ave_mod[phase][i] = NULL;
			set->ave_by_width_mod[phase][i] = NULL;
		}
		set->num_ave_mod[phase] = NULL;
		set->mca_mod[phase] = NULL;
		set->num_ave_by_width_mod[phase] = NULL;
	}

	for (phase=0; phase < set->num_phase; phase++)
	{
		sprintf(mod_name, "mca_phase%d_%d", phase+1, num_chan);
		set->mca_mod[phase] = app_mkmod3d (cn, mod_name, 4096, NGPD_MOD_NUM_T, num_chan, "ADC Value", "All/Neutron/Gamma", "Channel", NULL, 0, &(set->mca_head[phase]));
		if (set->mca_mod[phase] == NULL)
			goto exit_error;
		sprintf(set->mca_mod[phase]->head.x_label, "ADC Values/%g", 1.0/set->mca_scale);
		if (!set->no_clear)
			clear_mod((MOD_IMAGE *)set->mca_mod[phase]);

		if (set->do_persist)
		{
			for (i=set->first_chan;i<=set->last_chan; i++)
			{
				sprintf(mod_name, "%s_phase%d_%02d", persist_name, phase+1, i);
				set->persist_mod[phase][i] = app_mkmod3d (cn, mod_name, set->pre+set->post, set->persist_num_y, NGPD_MOD_NUM_T, "Time", "ADC Value", "All/Neutron/Gamma", NULL, 0, &(set->persist_head[phase][i]));
				if (set->persist_mod[phase][i] == NULL)
					goto exit_error;
				if (!set->no_clear)
					clear_mod((MOD_IMAGE *)(set->persist_mod[phase][i]));
			}
		}
		if (set->do_ave)
		{
			for (i=set->first_chan;i<=set->last_chan; i++)
			{
				sprintf(mod_name, "%s_phase%d_%02d", ave_name, phase+1, i);
				set->ave_mod[phase][i] = app_mkmod3d (cn, mod_name, set->pre+set->post, NGPD_MOD_NUM_ROW, NGPD_MOD_NUM_T, "Time", "ADC Value scaled 0...63353", "All/Neutron/Gamma", ave_mod_labels, 2, &(set->ave_head[phase][i]));
				if (set->ave_mod[phase][i] == NULL)
					goto exit_error;
				if (!set->no_clear)
					clear_mod((MOD_IMAGE *)(set->ave_mod[phase][i]));

			}
			sprintf(mod_name, "num_ave_phase%d_%d", phase+1, num_chan);
			set->num_ave_mod[phase] = app_mkmod_head (cn, mod_name, num_chan, NGPD_MOD_NUM_T, "Chan", "All/Neutron/Gamma",  0, &(set->num_ave_head[phase]));
			if (set->num_ave_mod[phase] == NULL)
				goto exit_error;
			if (!set->no_clear)
				clear_mod(set->num_ave_mod[phase]);
		}
	}

	set->path = path;
	set->cn = cn;

	save_flags = ngpd_get_run_flags(set->path);
	flags = (save_flags & ~NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS) | NGPD_RUN_FLAGS_SCOPEMODE | NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD;
	ngpd_set_run_flags(set->path, flags);

	first_card = set->first_chan/set->chans_per_card;
	last_card  = set->last_chan/set->chans_per_card;

	for (phase=0; phase<set->num_phase; phase++)
	{
		if ((rc=ngpd_calib_setup_trig_or_measure(cn, set->path, -1, set->use_measure, 0, &set->first_stream, &set->last_stream,  &set->trig_offset, &set->wide_offset_a,  &set->wide_offset_b, &set->neutron_offset, 0, &set->need_upper)) < 0)
		{
			sv_printf(cn, "! ERROR setting scope multiplexers : %s\n", ngpd_get_error_message());
			return rc;
		}

		settings[0].phase = phase;
		if (num_cards > 1 && disable_multi_thread==0)	/* Moved all this here so info from ngpd_calib_setup_trig_or_servo is correctly copied to all threads */
		{
			for (i=1;i<num_cards; i++)
				settings[i] = settings[0];

			for (i=0;i<num_cards; i++)
			{
				settings[i].thread_index = i;
				settings[i].first_card = i;
				settings[i].last_card = i;
			}
		}
		else
		{
			settings[0].thread_index = 0;
			settings[0].first_card = first_card;
			settings[0].last_card  = last_card;
		}

		if (num_cards > 1 && disable_multi_thread==0)
		{
			int this_rc=0;
			for (card=first_card; card<=last_card; card++)
			{
				pthread_create(threads+card, NULL, ngpd_measure_trigger_do, settings+card);
			}
			for (card=first_card; card<=last_card; card++)
			{
				void *ret_void;
				pthread_join(threads[card], &ret_void);
				if ((int)(ret_void) < 0)
					this_rc = -1;
			}
			if (this_rc < 0)
				goto exit_error;
		}
		else
		{
			if ((int)ngpd_measure_trigger_do(set) < 0)
				goto exit_error;
		}
		for (i=set->first_chan; i<= set->last_chan; i++)
		{
			if (set->min_hgt_neutron[i] >= 0.0 || set->max_hgt_neutron[i] >=0.0)
			{
				int *nnum, *gnum, *anum;
				nnum = (int *)id_get_ptr(set->mca_mod[phase], 0, NGPD_MOD_T_NEUTRON, i);
				gnum = (int *)id_get_ptr(set->mca_mod[phase], 0, NGPD_MOD_T_GAMMA, i);
				anum = (int *)id_get_ptr(set->mca_mod[phase], 0, NGPD_MOD_T_ALL, i);
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
					for (j=NGPD_MOD_ROW_SIG; j<=NGPD_MOD_ROW_WIDE_B; j++)
					{
						nsum = (double *)id_get_ptr(set->ave_mod[phase][i], 0, j, NGPD_MOD_T_NEUTRON);
						gsum = (double *)id_get_ptr(set->ave_mod[phase][i], 0, j, NGPD_MOD_T_GAMMA);
						asum = (double *)id_get_ptr(set->ave_mod[phase][i], 0, j, NGPD_MOD_T_ALL);
						for (k=0;k<set->pre+set->post; k++)
							*asum++ = *gsum++ + *nsum++;
					}
					nnum = (int *)id_get_ptr(set->num_ave_mod[phase], i, NGPD_MOD_T_NEUTRON, 0);
					gnum = (int *)id_get_ptr(set->num_ave_mod[phase], i, NGPD_MOD_T_GAMMA, 0);
					anum = (int *)id_get_ptr(set->num_ave_mod[phase], i, NGPD_MOD_T_ALL, 0);
					*anum = *gnum + *nnum;
				}
				
				for (j=0; j<NGPD_MOD_NUM_T; j++)
				{
					int n;
					if ((n=set->num_ave_mod[phase]->data[i+num_chan*j]) > 0)
					{
						for (k=0; k<=NGPD_MOD_ROW_WIDE_B; k++)
						{
							ngpd_divide_average(set->ave_mod[phase][i], k, j, n);
						}
					}
				}				
			}
		}
		if (set->auto_neutron_height && phase == 0)
		{
			/* Scan mca looking for max and update min/max event */
			int max, max_posn;
			double sum_f, sum_fx;
			sv_printf(cn, "# End of Phase %d, Adjusting min/max to find K-alpha\n", phase+1);
			for (chan=set->first_chan; chan<= set->last_chan; chan++)
			{
				iptr = (int *)id_get_ptr(set->mca_mod[phase], 0, NGPD_MOD_T_ALL, chan);
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
				sv_printf(cn, "# Chan %d found max %d at %d\n", chan, max, max_posn);
				iptr = (int *)id_get_ptr(set->mca_mod[phase], 0, NGPD_MOD_T_ALL, chan);
				sum_f = sum_fx = 0.0;
				for (i=(int)(0.90*max_posn); i<=(int)(1.1*max_posn); i++)
				{
					sum_f  += iptr[i];
					sum_fx += iptr[i] * i;
				}
				max_posn = sum_fx/sum_f;
				min_hgt_neutron[chan] = (0.9*max_posn)/set->mca_scale;
				max_hgt_neutron[chan] = (1.1*max_posn)/set->mca_scale;
				sv_printf(cn, "# Chan %d: Min = %g, Max=%g\n", chan, min_hgt_neutron[chan], max_hgt_neutron[chan]);
			}
		}
	} /* End for phase */

	for (phase=0; phase<MAX_PHASES; phase++)
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
	for (phase=0; phase<MAX_PHASES; phase++)
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
	return -1;
}

void *ngpd_measure_trigger_do(void * set_vp)
{
	NGPDMeasureTrigger *set= (NGPDMeasureTrigger *)set_vp;
	int i, j, k, chan, card;
	u_int16_t *dig_ptr;
	int16_t *ana_ptr;
	int *iptr, *p;
	double *fptr, *fp2;
	double baseline, lhs, rhs, lhs4, rhs4;
	int pass;
	int persist_baseline;
	double average_baseline;
	int prev_event;
	int start_gap, stop_gap;
	char cmd[2048];
	int wide_start[NGPD_SCOPE_MAX_STREAMS];
	u_int16_t trig_mask, wide_mask_a, wide_mask_b, neutron_mask;
	int stream, rc;
	int ana_inc, dig_inc;
	int base_bit_posn;
	double scale_sigma;
	double hgt_for_mca;
	int separate_neutrons, mod_t;
	u_int32_t scope_status;
	int do_upper;

	for (pass = 0; pass < ((set->phase==set->num_phase-1)?set->num_pass:1); pass++)
	{
		sv_printf(set->cn, "# Thread Index=%d : Card %d..%d : Source Channels = %d ..%d, pass %d\n", set->thread_index, set->first_card, set->last_card, 0, set->chans_per_card-1, pass);
		if (set->playback && (set->phase ==0 || pass >0))
		{
			sprintf(cmd, set->playback, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass, pass);
			if (user_service(set->cn, cmd, 0, NULL, NULL) < 0)
			{
				sv_printf(set->cn, "! Cannot run playback command '%s'\n", cmd);
				goto exit_error;
			}
		}

		for (do_upper=0; do_upper<=set->need_upper; do_upper++)
		{
			if (set->first_card != set->last_card)
				card = -1;
			else
				card = set->first_card;
			if (set->need_upper)
			{
				sv_printf(set->cn, "# Note calling ngpd_calib_setup_trig_or_measure(card=%d, do_upper=%d) from ngpd_measure_trigger_do\n", card, do_upper);
				if ((rc=ngpd_calib_setup_trig_or_measure(set->cn, set->path, card, set->use_measure, do_upper, &set->first_stream, &set->last_stream,  &set->trig_offset, &set->wide_offset_a,  &set->wide_offset_b, &set->neutron_offset, 0, NULL)) < 0)
					goto exit_error;
			}
#if 0
			rc = ngpd_dma_system_start(set->path, 0, 0, NULL);
#else
			rc = ngpd_dma_system_start(set->path, card, 0, 0, NULL);
#endif
			if (rc < 0)
			{
				sv_printf(set->cn, "! ERROR starting system: %s\n", ngpd_get_error_message());
				goto exit_error;
			}
			rc = ngpd_dma_wait_scope(set->path, card, &scope_status);
			if (rc < 0) {
				sv_printf(set->cn,"! ERROR waiting for scope DMAs %s\n", ngpd_get_error_message());
				goto exit_error;
			}
			if (scope_status != 0)
			{
				sv_printf(set->cn, "! ERROR: Scope status appears wrong at end of run = %08X\n", scope_status);
				return -1;
			}
				
			for (card=set->first_card; card<=set->last_card; card++)
			{
				rc = ngpd_dma_read_scope(set->path, card, 0, 0, 0);
				if (rc < 0)
				{
					sv_printf(set->cn,"! ERROR reading scope data %s\n", ngpd_get_error_message());
					goto exit_error;
				}
				for (stream=set->first_stream; stream<=set->last_stream; stream++)
				{
					int num_raw_trig=0, num_pileup_pre=0, num_pileup_post=0;
					chan = ngpd_calib_setup_trig_or_measure_get_chan(set->cn, set->path, set->use_measure, do_upper, stream);
					if (chan < 0)
						continue;
					chan += card*set->chans_per_card;
					if (chan >= set->first_chan && chan <= set->last_chan)
					{
						separate_neutrons = set->min_hgt_neutron[chan]>=0 && set->max_hgt_neutron >= 0;
						ana_ptr = ngpd_scope_mod_get_ptr(set->scope_mod, card, stream);
						ngpd_scope_stream_details(set->scope_mod, card, stream, NULL, &base_bit_posn, &dig_ptr, NULL, NULL, NULL, &ana_inc, &dig_inc);
						//					sv_printf(set->cn, "# Stream %d:Chan %d: Determined num_fine=%d, fine_posn=%d, dig_ptr=%p, fine_ptr=%p\n", stream, chan, num_fine, fine_posn, dig_ptr, fine_ptr);
						if (dig_ptr == NULL || ana_ptr == NULL)
						{
							sv_printf(set->cn, "! ERROR reading scope data pointers:'%s'\n", ngpd_get_error_message());
							goto exit_error;
						}
						if (set->phase != 0)
						{
							fptr = (double *)id_get_ptr(set->ave_mod[set->phase-1][chan], 0, NGPD_MOD_ROW_WIDE_A, 0);
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
						sv_printf(set->cn, "# Card %d, Stream %d, chan %d. TRIG_MASK=%04X, WIDE_MASK_A=%04X, WIDE_MASK_B=%04X, set->use_measure=%d\n", card, stream, chan, trig_mask, wide_mask_a, wide_mask_b, set->use_measure);
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
									for (j=1; j< set->post+set->extra_post; j++) /* seearch for any late triggers */
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
								ngpd_measure_pulse(ana_ptr, i, set->pre, set->post, &lhs, &rhs, &hgt_for_mca, set->pre/3, set->post/4, ana_inc, dig_inc);
								// sv_printf(set->cn, "# Found event Chan %d , t=%d;lhs=%g, rhs=%g, hgt=%f\n", chan, i, lhs, rhs, rhs-lhs);
								/* lhs, rhs, min_event, min_hgt_neutron should all be on 0..65535 scaling */
								if (set->min_event[chan] >= 0 && hgt_for_mca < set->min_event[chan])
									continue;
								if (set->max_event[chan] > 0 && hgt_for_mca > set->max_event[chan])
									continue;
								if (separate_neutrons)
								{
									if ( hgt_for_mca>= set->min_hgt_neutron[chan] && hgt_for_mca <= set->max_hgt_neutron[chan])
										mod_t = NGPD_MOD_T_NEUTRON;
									else
										mod_t = NGPD_MOD_T_GAMMA;
								}
								else
									mod_t = NGPD_MOD_T_ALL;

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
									ngpd_accumulate_persist(ana_ptr, dig_ptr, i, set->persist_mod[set->phase][chan], mod_t, set->pre, set->post, persist_baseline, 32, set->col_sat, trig_mask, wide_mask_a, wide_mask_b, ana_inc, dig_inc);
								}
								if (set->do_ave)
								{
									ngpd_accumulate_ave(ana_ptr, dig_ptr, i, set->ave_mod[set->phase][chan], mod_t, set->pre, set->post, average_baseline, ana_inc, dig_inc, trig_mask, wide_mask_a, wide_mask_b );
									set->num_ave_mod[set->phase]->data[chan+set->num_ave_mod[set->phase]->head.num_x*mod_t]++;
								}
							} /* End IF Found event */
						} /* For i searching for reset */
						if (num_raw_trig < 1000)
							sv_printf(set->cn, "# Warning only found %d triggers on channel %d\n", num_raw_trig, chan);
						else if (num_pileup_pre+num_pileup_post > num_raw_trig/2)
							sv_printf(set->cn, "# Large fraction of events rejected on channel %d: num_raw_trig=%d, num_pileup_pre=%d, num_pileup_post=%d\n", chan, num_raw_trig, num_pileup_pre, num_pileup_post);
					} /* If Chan in range */
				} /* For stream */
				if (sv_quit)
				{
					sv_printf(set->cn, "STOPPED By ^c\n");
					goto exit_error;
				}
			} /* For Slot  */
		} /* For do_upper */
	} /* For Pass */
	sv_printf(set->cn, "# Thread index %d finished: Card %d...%d , Stream %d...%d\n", set->thread_index, set->first_card, set->last_card, set->first_stream, set->last_stream);
	return (void *)(int64_t)0;

exit_error: return (void *)(int64_t)-1;
}

int ngpd_calc_tail_subtract (int quals, int cn, int path, char *root_ave_name, char *root_fname)
{
	MOD_IMAGE3D *ave_mod=NULL;
	mh_com *ave_head;
	MOD_IMAGE3D *tail_mod=NULL;
	mh_com *tail_head;
	int num_chan;
	int num_x;
	int first_chan, last_chan;
	char ave_mod_name[NGPD_MAX_MODNAME+20];
	char mod_name[NGPD_MAX_MODNAME+20];
	int i, j, k, chan, x;
	double wspace[NGPD_MEASURE_TAIL_SUB_SIZE];
	u_int32_t *tail;
	double *fptr, *fp2, *fp3, f;
	int num_lhs, num_rhs;
	double baseline, lhs, rhs, lhs4, rhs4;
	char fname[1024], *cp;
    u_int16 attr_rev = mkattrevs(MA_REENT,1);
    u_int16 type_lang = mktypelang(MT_DATA, ML_ANY);
	int event_posn;
	double max_height;
	double grad_residual;
	FILE *ofp;
	int err;
	int fit_ramp=-1;
	double scale_tail = 1.0;
	int end_offset=0;
	int rc;
	double sig_pre_event;
	int neutron;

	num_chan = ngpd_get_num_chan(path);
	first_chan = 0;
	last_chan  = num_chan-1;

	if (quals & 1)
	{
		first_chan = last_chan = Qualifier[0].IntArg;
		if (first_chan < 0 || first_chan>= num_chan)
		{
			sv_printf(cn, "! ERROR : expected %d <= src <= %d , not %d\n", 0, num_chan-1, first_chan);
			return -1;
		}
	}
	if (quals & 2)
	{
		fit_ramp = Qualifier[1].IntArg;
	}

	if (quals & 4)
		scale_tail = Qualifier[2].DoubleArg;

	if (quals & 8)
		end_offset = Qualifier[3].IntArg;

	if (quals & 0x10)
	{
		chan = Qualifier[4].IntArg;
		if (chan < 0 || chan >= num_chan)
		{
			sv_printf(cn, "! ERROR : expected %d <= first <= %d , not %d\n", 0, num_chan-1, chan);
			return -1;
		}
		first_chan = chan;
	}
	if (quals & 0x20)
	{
		chan = Qualifier[5].IntArg;
		if (chan < 0 || chan >= num_chan)
		{
			sv_printf(cn, "! ERROR : expected %d <= last <= %d , not %d\n", 0, num_chan-1, chan);
			return -1;
		}
		last_chan = chan;
	}

	if (root_fname != NULL)
	{
		sprintf(mod_name, "tail_subtract%d", num_chan);
		tail_mod = app_mkmod3d (cn, mod_name, NGPD_MEASURE_TAIL_SUB_SIZE, 2, num_chan, "Time", "Gamma/Neutron", "Channel",  NULL, 0, &(tail_head));
		if (tail_mod == NULL)
			return -1;
	}


	strncpy(ave_mod_name, root_ave_name, NGPD_MAX_MODNAME);
	ave_mod_name[NGPD_MAX_MODNAME] = 0;
	i=strlen(ave_mod_name);
	if (i > 1 && ave_mod_name[i-1] == '_')
		ave_mod_name[i-1] = 0;
	for (chan=first_chan; chan <= last_chan; chan++)
	{
		sprintf(mod_name, "%s_%02d", ave_mod_name, chan);
		cp = mod_name;
	   	err = _os_link(&cp, &ave_head, (void **)&ave_mod, &type_lang, &attr_rev);
	    if (err != 0)
	    {
			sv_printf(cn, "! ERROR: Cannot open average module name %s\n", mod_name);
			goto exit_error;
	    }

    	if (IMG_MOD_GET_TYPE(ave_mod) != IMG_MOD_3D || ave_mod->head.num_y != NGPD_MOD_NUM_ROW || ave_mod->head.num_t != NGPD_MOD_NUM_T)
    	{
			sv_printf(cn, "! ERROR: Average module name %s is wrong type or size. Expected 3-d and num_y=%d, num_t=%d not %d, %d\n", mod_name, NGPD_MOD_NUM_ROW, NGPD_MOD_NUM_T, ave_mod->head.num_y, ave_mod->head.num_t);
			munlink(ave_head);
			goto exit_error;
    	}
		sv_printf(cn, "# Processing chan %d from module %s\n", chan, mod_name);
		num_x = ave_mod->head.num_x;
		fptr = (double *)id_get_ptr(ave_mod, 0, NGPD_MOD_ROW_TRIG, NGPD_MOD_T_ALL);
		for (i=0; i< num_x; i++)
			if (fptr[i] > 0.5)
				break;
		event_posn = i ;

		if (i == num_x)
		{
			sv_printf(cn, "! WARNING : Cannot find event end point on channel %d. Omitting\n", chan);
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
				fptr = (double *)id_get_ptr(ave_mod, 0, NGPD_MOD_ROW_SIG, NGPD_MOD_T_NEUTRON);
			else
				fptr = (double *)id_get_ptr(ave_mod, 0, NGPD_MOD_ROW_SIG, NGPD_MOD_T_GAMMA);

			max_height = fptr[event_posn];

			for (j=0, i=event_posn+end_offset; i<num_x && j < NGPD_MEASURE_TAIL_SUB_SIZE ; i++, j++)
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
				x *= scale_tail;		// For testing only
				if ( x > 0x1ffff)
					x =0x1ffff;
				if (x < -0x1ffff)
					x = -0x1ffff;
				tail[i] = x;
			}
		}
		sprintf(fname, "%s_%02d.dat", root_fname, chan);
		if ((ofp=fopen(fname, "w")) == NULL)
		{
			sv_printf(cn, "! ERROR: Cannot open output file %s\n", fname);
			goto exit_error;
		}
		tail = id_get_ptr(tail_mod, 0, 0, chan);
		if (fwrite(tail, sizeof(int32), NGPD_MEASURE_TAIL_SUB_SIZE*2, ofp) != NGPD_MEASURE_TAIL_SUB_SIZE*2)
		{
			sv_printf(cn, "! ERROR writing file %s\n", fname);
			fclose(ofp);
			goto exit_error;
		}
		fclose(ofp);
		if ((rc=ngpd_write_tail_subtract(path, chan, (int32_t*) tail)) < 0)
		{
			sv_printf(cn, "! Error writing tail subtract BRAM, chan %d : %s\n", chan, ngpd_get_error_message());
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


int ngpd_adjust_trigger_timing (int quals, int cn, int path, char *ave_name, char *res_name)
{
	int sys;
	MOD_IMAGE3D *ave_mod[NGPD_MAX_CHANS];
	mh_com *ave_head[NGPD_MAX_CHANS];
	MOD_IMAGE *res_mod=NULL;
	mh_com *res_head;
	int first_chan, last_chan, num_chan;
	char *cp;
	int i, j;
	int chan;
	char mod_name[NGPD_MAX_MODNAME+20];
    u_int16 attr_rev = mkattrevs(MA_REENT,1);
    u_int16 type_lang = mktypelang(MT_DATA, ML_ANY);
	int num_x;
	double data_max;
	double *fptr, *fbase;
	int sig_peak_time;
	int trig_time;
	int sig_first_time[NGPD_NUM_STRETCHED_TRIG], sig_last_time[NGPD_NUM_STRETCHED_TRIG] ;
	int wide_first_time[NGPD_NUM_STRETCHED_TRIG], wide_last_time[NGPD_NUM_STRETCHED_TRIG];
	double wide_first_thres[NGPD_NUM_STRETCHED_TRIG] = {0.5, 0.5 };
	double wide_last_thres[NGPD_NUM_STRETCHED_TRIG]= {0.5, 0.5 };
	double sig_first_thres[NGPD_NUM_STRETCHED_TRIG] = {0.005, 0.005 };
	double sig_last_thres[NGPD_NUM_STRETCHED_TRIG]  = {0.015, 0.015 };
	int wide_first_offset[NGPD_NUM_STRETCHED_TRIG]  = {-2, -2};
	int wide_last_offset[NGPD_NUM_STRETCHED_TRIG] = {10, 10 };
	int no_clear = 0;
	int *iptr;
	int current, required;
	int trig_dt, wide_first_dt[NGPD_NUM_STRETCHED_TRIG], wide_last_dt[NGPD_NUM_STRETCHED_TRIG];
	int wide_len[NGPD_NUM_STRETCHED_TRIG];
	int fit_ramp = 0;
	NGPDDiffTrigger trig, old_trig;
	int just_check=0;
	int min_width;
	int min_delay;
	int wide_a_pulse;

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
		sig_first_thres[0] = Qualifier[3].DoubleArg;
	if (quals & 0x10)
		sig_last_thres[0]  = Qualifier[4].DoubleArg;
	if (quals & 0x20)
		wide_first_thres[0] = Qualifier[5].DoubleArg;
	if (quals & 0x40)
		wide_last_thres[0]  = Qualifier[6].DoubleArg;
	if (quals & 0x80)
		wide_first_offset[0] = Qualifier[7].IntArg;
	if (quals & 0x100)
		wide_last_offset[0]  = Qualifier[8].IntArg;

	if (quals & 0x200)
		sig_first_thres[1] = Qualifier[9].DoubleArg;
	if (quals & 0x400)
		sig_last_thres[1]  = Qualifier[10].DoubleArg;
	if (quals & 0x800)
		wide_first_thres[1] = Qualifier[11].DoubleArg;
	if (quals & 0x1000)
		wide_last_thres[1]  = Qualifier[12].DoubleArg;
	if (quals & 0x2000)
		wide_first_offset[1] = Qualifier[13].IntArg;
	if (quals & 0x4000)
		wide_last_offset[1]  = Qualifier[14].IntArg;

	wide_a_pulse = !!(quals & 0x8000);

	if (quals & 0x10000)
		fit_ramp = Qualifier[16].IntArg;

	if (quals & 0x20000)
		just_check = Qualifier[17].IntArg;

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

	res_mod = app_mkmod_head (cn, res_name, num_chan, 12, "Chan", "Function",  0, &res_head);
	if (res_mod == NULL)
		goto exit_error;

	if (!no_clear)
	{
		iptr = (int *)res_mod->data;
		clear_mod(res_mod);
	}

	for (chan=first_chan; chan<=last_chan; chan++)
	{
		if (ngpd_read_diff_trigger(path, chan, &trig) < 0)
		{
			sv_printf(cn, "! ERROR extracting diff trigger setup on chan %d\n", chan);
			goto exit_error;
		}
		old_trig = trig;
	
		sprintf(mod_name, "%s_%02d", ave_name, chan);
		cp = mod_name;
	    if (_os_link(&cp, &(ave_head[chan]), (void **)&(ave_mod[chan]), &type_lang, &attr_rev))
		{
			sv_printf (cn,"! Cannot link to data module %s\n", mod_name);
			goto exit_error;
		}
		num_x = ave_mod[chan]->head.num_x;
		iptr = (int *)res_mod->data+chan;

		if (ave_mod[chan]->head.num_y != NGPD_MOD_NUM_ROW || ave_mod[chan]->head.data_type != DATA_DOUBLE)
		{
			sv_printf(cn, "! ERROR: Module for channel %d has wrong num_y=%d or data type %d\n", chan,
					ave_mod[chan]->head.num_y, ave_mod[chan]->head.data_type );
			goto exit_error;
		}
		fptr = (double *)id_get_ptr(ave_mod[chan], 0, NGPD_MOD_ROW_TRIG, NGPD_MOD_T_ALL);

		for (i=0; i<num_x; i++)
			if (fptr[i] >= 0.9)
				break;
		trig_time = i;

		if (trig_time == num_x)
		{
			sv_printf(cn, "! ERROR: Missing CFD signal on channel %d, skipping\n", chan);
			continue;
		}

		fptr = (double *)id_get_ptr(ave_mod[chan], 0, NGPD_MOD_ROW_SIG, NGPD_MOD_T_ALL);
		if (fit_ramp > 0)
		{
			if (fit_ramp > trig_time)
			{
				sv_printf(cn, "# WARNING: On Channel %d, fit_ramp=%d > trig_time=%d, run with longer pre or shorted fit-ramp\n", chan, fit_ramp, trig_time);
			}
			else
			{
				char command[100];
				double short_tot, *fp2, *fp3;
				double left_val, right_val;
				int this_fit_ramp;
				
				fp2 = (double *)id_get_ptr(ave_mod[chan], 0, NGPD_MOD_ROW_SIG, NGPD_MOD_T_ALL);
				left_val = fp2[0];
				right_val = fp2[num_x-1];
				for (i=0;i<num_x; i++)
					if (fp2[i] > 0.5*right_val)
						break;
				
				if (i-25 < num_x/4)
				{
					sv_printf(cn, "# WARNING: Channel %d: Rising edge position %d is very early, leaves too little space for fit ramp\n", chan, i);
				}
				else
				{
					this_fit_ramp = fit_ramp;
					if (this_fit_ramp > i - 25)
					{
						sv_printf(cn, "# Note Channel %d: Early event position %d, so shortening fit ramp to %d\n", chan, i, fit_ramp);
						this_fit_ramp = i-25;
					}
					sprintf(command, "maths leastsqr-fit '%s' %d %d %d fit %d extrapolate", mod_name, 0, this_fit_ramp, X3_MOD_ROW_SIG, X3_MOD_ROW_FIT);
					if (user_service(cn, command, 0, NULL, NULL) < 0)
					{
						sv_printf(cn, "! Error proceesing command '%s' fitting to module\n", command);
						munlink(ave_head);
						goto exit_error;
					}
					fptr = (double *)id_get_ptr(ave_mod[chan], 0, NGPD_MOD_ROW_SIG, NGPD_MOD_T_ALL);
					fp2  = (double *)id_get_ptr(ave_mod[chan], 0, NGPD_MOD_ROW_FIT, NGPD_MOD_T_ALL);
					fp3  = (double *)id_get_ptr(ave_mod[chan], 0, NGPD_MOD_ROW_CORRECTED, NGPD_MOD_T_ALL);

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
					fptr  = (double *)id_get_ptr(ave_mod[chan], 0, NGPD_MOD_ROW_CORRECTED, NGPD_MOD_T_ALL);
				}
			}
		}

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
				if (fptr[i] >= sig_first_thres[j]*data_max)
					break;
			sig_first_time[j] = i;

			for (i=num_x-1; i>=0; i--)
				if (fptr[i] > sig_last_thres[j]*data_max)
					break;
			sig_last_time[j] = i;
		}

		for (j=0; j<NGPD_NUM_STRETCHED_TRIG; j++)
		{
			fptr = (double *)id_get_ptr(ave_mod[chan], 0, NGPD_MOD_ROW_WIDE_A+j, NGPD_MOD_T_ALL);
			for (i=0; i<num_x; i++)
				if (fptr[i] >= wide_first_thres[j])
					break;
			wide_first_time[j] = i;

			if (wide_first_time[j] == num_x)
			{
				sv_printf(cn, "! ERROR: Missing Wide signal on channel %d, skipping\n", chan);
				continue;
			}

			for (i=num_x-1; i >0; i--)
				if (fptr[i] >= wide_last_thres[j])
					break;
	//		sv_printf(cn, "# Chan %d : Found OTD last at i=%d, fptr[i] = %g\n", chan, i, fptr[i]);
			wide_last_time[j] = i;
		}

		*iptr = sig_peak_time;
		iptr += num_chan;
		*iptr = trig_time;
		iptr += num_chan;

		for (j=0; j<NGPD_NUM_STRETCHED_TRIG; j++)
		{
			*iptr = sig_first_time[j];
			iptr += num_chan;
			*iptr = sig_last_time[j];
			iptr += num_chan;
			*iptr = wide_first_time[j];
			iptr += num_chan;
			*iptr = wide_last_time[j];
			iptr += num_chan;
		}
/*
		*iptr = sig_last_time+1-sig_first_time;
		iptr += num_chan;
		*iptr = wide_last_time+1-wide_first_time;
		iptr += num_chan;
*/
		trig_dt = sig_peak_time-trig_time;
		for (j=0; j<NGPD_NUM_STRETCHED_TRIG; j++)
		{
			wide_first_dt[j] = sig_first_time[j]+wide_first_offset[j] - wide_first_time[j];
			wide_last_dt[j]  = sig_last_time[j]+wide_last_offset[j] - wide_last_time[j];
			wide_len[j]      = wide_last_time[j]+1-wide_first_time[j];
		
			if (wide_len[j] <= 2)
			{
				sv_printf(cn, "# Warning: Wide event signal length on channel %d is very small, %d, check length after adjustment\n", chan, wide_len[j]);
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
			sv_printf(cn, "# Warning, required data delay=%d, limiting to %d\n", trig.data_delay, NGPD_DIFF_TRIG_MAX_DATA_DELAY);
			trig.data_delay = NGPD_DIFF_TRIG_MAX_DATA_DELAY;
			trig.trig_delay -= dt;
			trig.width_a -= dt/2;
			trig.width_b -= dt/2;
			if (trig.trig_delay < 0)
			{
				sv_printf(cn, "# Warning, But adjusted trig delay=%d, limiting to 0\n", trig.data_delay);
				trig.trig_delay = 0;
			}
		}
		if (trig.trig_delay > NGPD_DIFF_TRIG_MAX_TRIG_DELAY)
		{
			int dt = trig.trig_delay - NGPD_DIFF_TRIG_MAX_TRIG_DELAY;
			sv_printf(cn, "# Warning, required trig delay=%d, limiting to %d\n", trig.data_delay, NGPD_DIFF_TRIG_MAX_TRIG_DELAY);
			trig.trig_delay = NGPD_DIFF_TRIG_MAX_TRIG_DELAY;
			trig.data_delay -= dt;
			trig.width_a -= dt/2;
			trig.width_b -= dt/2;
		}
		if (trig.data_delay < 0)
		{
			sv_printf(cn, "# Warning, required data delay=%d, limiting to 0\n", trig.data_delay);
			trig.data_delay = 0;
		}
		else if (trig.data_delay > NGPD_DIFF_TRIG_MAX_DATA_DELAY)
		{
			sv_printf(cn, "# Warning, required data delay=%d, limiting to %d\n", trig.data_delay, NGPD_DIFF_TRIG_MAX_DATA_DELAY);
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
		if (wide_a_pulse)
			trig.width_a = 1;

		if (!just_check)
		{
			if (ngpd_write_diff_trigger(path, chan, &trig) < 0)
			{
				sv_printf(cn, "! ERROR setting diff trigger setup on chan %d: %s\n", chan, ngpd_get_error_message());
				goto exit_error;
			}

		}
		sv_printf(cn, "# Chan %d Changes: trig_dt=%d, Wide-A First=%d, Wide-A Last=%d, Wide-B First=%d, Wide-B Last=%d\n", chan, trig_dt, wide_first_dt[0], wide_last_dt[0], wide_first_dt[1], wide_last_dt[1]);
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


double ngpd_lin_interp_time(MOD_IMAGE *ave_mod, int row, int iposn)
{
	double *p, a, b, c;
	
	if (iposn == 0)
		return 0.0;
	else if (iposn >= ave_mod->head.num_x-2)
		return (double) (ave_mod->head.num_x-2);
	
	p = (double *)id_get_ptr(ave_mod, 0, row, 0);

	a = p[iposn-1];
	b = p[iposn];
	c = p[iposn+1];

	if (c-2*b+a == 0)
		return (double)iposn;
	return (double)iposn + (a-b)/(c-2*b+a);
}
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


void ngpd_accumulate_ave(int16_t * ana_ptr, u_int16_t * dig_ptr,  int offset, MOD_IMAGE3D *mod, int mod_t, int pre, int post, double baseline, int ana_inc, int dig_inc, u_int16_t trig_mask, u_int16_t wide_mask_a, u_int16_t wide_mask_b)
{
	int i, j;
	int y, first_y;
	int num_x = mod->head.num_x;
	double *fptr;
	fptr = (double *) id_get_ptr(mod, 0, 0, mod_t);

	for (j= -pre; j<post; j++)
	{
		y = ana_ptr[ana_inc*(offset+j)];
		*fptr += ADC_SCALE*y-baseline;

		if (dig_ptr[dig_inc*(offset+j)] & trig_mask)
			fptr[NGPD_MOD_ROW_TRIG*num_x]++;

		if (dig_ptr[dig_inc*(offset+j)] & wide_mask_a)
			fptr[NGPD_MOD_ROW_WIDE_A*num_x]++;

		if (dig_ptr[dig_inc*(offset+j)] & wide_mask_b)
			fptr[NGPD_MOD_ROW_WIDE_B*num_x]++;
		fptr++;
	}
}


int ngpd_divide_average(MOD_IMAGE3D *mod, int row, int t, int n)
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

int ngpd_measure_pulse(u_int16_t * ana_ptr, int offset, int pre, int post, double *lhs, double *rhs, double *height, int num_lhs, int num_rhs, int ana_inc, int dig_inc)
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
}
#define GREY_RESET 4

void ngpd_accumulate_persist(int16_t * ana_ptr, u_int16_t *dig_ptr,  int offset, MOD_IMAGE3D *mod, int mod_t, int pre, int post, int base_line, int divide, int cap_len, u_int16_t bmask0, u_int16_t bmask1, u_int16_t bmask2, int ana_inc, int dig_inc)
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
	int digital_mask=0;
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
