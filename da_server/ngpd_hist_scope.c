/*----------------------------------------------------------------------
 *	NGPD form histograms of scope mode data to assess noise and linearity
 *	By William Helsby
 *----------------------------------------------------------------------
 */

#include "header.h"
#include "ngpd.h"



/*----- Private global variables -----*/
/*----- global variables -----*/


/*----- Private function prototypes -----*/

#define MAX_EVENT_TIME 100

#define WSPACE_POINTS (128*1024)

#if 0
int ngpd_scope_count (int quals, int cn, int path)
{
	int sys;
	MOD_IMAGE *resets_mod=NULL;
	mh_com *resets_head=NULL;
	MOD_IMAGE *events_mod=NULL;
	mh_com *events_head=NULL;
	int num_chan, num_cards, chans_per_card;
	int first_chan, last_chan;
	char mod_name[NGPD_MAX_MODNAME+20];
	int i, chan;
	int *iptr, *p;
	float *fptr;
	int pass, num_pass=1;
	int upper4;
	int first_card, last_card, card;		
	int prev_reset, prev_event, event_time;
	int reset, event;
	NGPDScopeModule * scope_mod;
	u_int16_t *dig_ptr, reset_bit, event_bit;
	int stream;
	int rc;
	int flags;
	int first_stream, last_stream,  otd_offset, cfd_offset, trig_c_offset, reset_bit_offset, gr_bit_offset, need_upper4, bit_base_posn;
	int ana_inc;

	num_chan = ngpd_get_num_chan(path);
	if (num_chan < 0)
	{
		sv_printf(cn, "! Bad NGPD Path : %s\n", ngpd_get_error_message());
		return -1;
	}
	num_cards = ngpd_get_num_cards(path);
	scope_mod = ngpd_scope_get_mod(path); 
	chans_per_card = ngpd_get_chans_per_card(path);

	first_chan = 0;
	last_chan  = num_chan-1;

	if (quals & 0x1)
		num_pass = Qualifier[0].IntArg;

	if (quals & 2)
		first_chan = last_chan = Qualifier[1].IntArg;

	sprintf(mod_name, "resets_and_events");
	resets_mod = app_mkmod_head (cn, mod_name, num_chan, 2, "Chan", "Y",  0, &resets_head);
	if (resets_mod == NULL)
		goto exit_error;
	sprintf(mod_name, "event_time");
	events_mod = app_mkmod_head (cn, mod_name, MAX_EVENT_TIME, num_chan, "Event Time", "Chan",  0, &events_head);

	if (events_mod == NULL)
		goto exit_error;
	iptr = (int *)(resets_mod->data);
	for (i=0; i<num_chan; i++)
		iptr[i] = 0;

	iptr = (int *)events_mod->data;
	for (i=0; i< MAX_EVENT_TIME*num_chan; i++)
		*iptr ++ = 0;

	first_card = first_chan/chans_per_card;
	last_card  = last_chan/chans_per_card;
	flags = ngpd_get_run_flags(path);
	ngpd_set_run_flags(path, flags | ngpd_RUN_FLAGS_SCOPE);

	for (upper4=0; upper4<2; upper4++)
	{
		if (!ngpd_need_this_half(path, 0, first_chan, last_chan, upper4, chans_per_card))
			continue;

		if ((rc=ngpd_calib_setup_trig_or_servo(cn, path, upper4, 1, &first_stream, &last_stream,  &otd_offset, &cfd_offset, &trig_c_offset, &reset_bit_offset, &gr_bit_offset, &need_upper4, 0, 0)) < 0)
		{
			sv_printf(cn, "! Error setting scope multiplexers: %s\n", ngpd_get_error_message());
			return -1;
		}
		scope_inc = ngpd_scope_mod_get_inc(scope_mod); // Moved to after ngpd_calib_setup_trig_or_servo

		for (pass = 0; pass < num_pass; pass++)
		{
			
			rc = ngpd_system_start(path, -1);
			if (rc < 0)
			{
				sv_printf(cn, "! ERROR starting system: %s\n", ngpd_get_error_message());
				goto exit_error;
			}
			sleep(2);

			for (card=first_card; card<= last_card; card++)
			{
				rc = ngpd_read_scope_data(path, card);
				if (rc < 0) 
				{
					sv_printf(cn,"! ERROR reading scope data %s\n", ngpd_get_error_message());
					goto exit_error;
				}
				for (stream=first_stream; stream<=last_stream; stream++)
				{
					chan = card*chans_per_card+stream-first_stream+upper4*5;
					if (need_upper4 && stream-1+upper4*5 >= chans_per_card)
						continue;
					if (chan >= first_chan && chan <= last_chan)
					{
						sv_printf(cn, "# Chan=%d, for %d points\n", chan, scope_mod->head.num_t); 
						dig_ptr = NULL;
						ngpd_scope_stream_details(scope_mod, card, stream, NULL, &bit_base_posn, &dig_ptr, NULL, NULL, NULL);
						if (dig_ptr == NULL)
						{
							sv_printf(cn, "! ERROR readign scope data pointers:'%s'\n", ngpd_get_error_message());
							goto exit_error;
						}
						prev_reset = 0;
						prev_event = 0;
						event_time = 0;
						reset_bit = 1 << (bit_base_posn+reset_bit_offset);
						event_bit = 1 << (bit_base_posn+cfd_offset);
						for (i=0; i<scope_mod->head.num_t; i++)
						{
							reset = *dig_ptr & reset_bit;
							event = *dig_ptr & event_bit;
							if (reset && ! prev_reset)
								resets_mod->data[chan] ++;
							prev_reset = reset;
							if (event && ! prev_event)
								resets_mod->data[num_chan+chan] ++;
							prev_event = event;
							if (event)
								event_time++;
							else if (!event && event_time > 0)
							{
								if (event_time >= MAX_EVENT_TIME)
									event_time = MAX_EVENT_TIME-1;
								events_mod->data[event_time+chan*MAX_EVENT_TIME]++;
								event_time = 0;
							}
							dig_ptr += scope_inc;
						} /* For i searching for reset and event */
					} /* Is a required channel */
				} /* For stream */
				if (sv_quit)
				{
					sv_printf(cn, "STOPPED By ^c\n");
					goto exit_error;
				}
			} /* For each Card */ 
		} /* For Pass */
	}/* for upper/lower 4 */
	ngpd_set_run_flags(path, flags);

	if (resets_mod != NULL)
		munlink(resets_head);
	if (events_mod != NULL)
		munlink(events_head);
	return 0;


exit_error:

	if (resets_mod != NULL)
		munlink(resets_head);
	if (events_mod != NULL)
		munlink(events_head);
	return -1;
}
#endif
int ngpd_scope_hist (int quals, int cn, int path)
{
	int sys;
	MOD_IMAGE *hist_mod=NULL;
	mh_com *hist_head=NULL;
	int num_chan, num_cards;
	int first_chan, last_chan;
	int chans_per_card;
	char mod_name[NGPD_MAX_MODNAME+20];
	int i, j, chan;
	NGPDScopeModule * scope_mod;
	u_int16_t *dig_ptr, *ana_ptr;
	u_int32_t *iptr;
	float *fptr;
	int pass, num_pass=1;
	int upper4;
	int first_card, last_card, card;		
	int no_clear = 0;
	u_int32 x;
	MOD_IMAGE *noise_mod=NULL;
	mh_com *noise_head=NULL;
	int max;
	int lhs, rhs;
	int do_fw;
	u_int16_t reset_mask;
	int stream;
	int rc;
	int flags;
	int read_all=0;
	int ana_inc;
	u_int32_t scope_status;

	num_chan = ngpd_get_num_chan(path);
	if (num_chan < 0)
	{
		sv_printf(cn, "! Bad NGPD Path : %s\n", ngpd_get_error_message());
		return -1;
	}
	num_cards = ngpd_get_num_cards(path);
	scope_mod = ngpd_scope_get_mod(path); 
	chans_per_card = ngpd_get_chans_per_card(path);

	first_chan = 0;
	last_chan  = num_chan-1;

	if (quals & 0x1)
		num_pass = Qualifier[0].IntArg;

	if (quals & 2)
		first_chan = last_chan = Qualifier[1].IntArg;

	do_fw = !!(quals & 0x4);

	sprintf(mod_name, "ngpd_hist");
	hist_mod = app_mkmod_head (cn, mod_name, 16384, num_chan, "ADC Value", "Chan",  0, &hist_head);
	if (hist_mod == NULL)
		goto exit_error;

	if (do_fw)
	{
		sprintf(mod_name, "ngpd_noise");
		noise_mod = app_mkmod_head (cn, mod_name, num_chan, X2NOISE_NUM_ROWS, "Chan", "ADC Units", 1, &noise_head);
		if (noise_mod == NULL)
			goto exit_error;
	}
	if (!no_clear)
	{
		iptr = (u_int32_t *)(hist_mod->data);
		for (i=0; i<hist_mod->head.num_x * hist_mod->head.num_y; i++)
			iptr[i] = 0;
	}

	first_card = first_chan/chans_per_card;
	last_card  = last_chan/chans_per_card;

	flags = ngpd_get_run_flags(path);
	ngpd_set_run_flags(path, flags | NGPD_RUN_FLAGS_SCOPEMODE);

	if ((rc=ngpd_cal_setup_all_ana(path, -1, 1, NGZMP_SCOPE_SEL_ANA_INPUT)) < 0)
	{
		sv_printf(cn, "! ERROR setting scope mode multiplexers: %s\n", ngpd_get_error_message());
		return rc;
	}
	ana_inc = ngpd_scope_mod_get_inc(scope_mod, 0); // Moved to after ngpd_cal_setup_all_ana
	for (pass = 0; pass < num_pass; pass++)
	{
		rc = ngpd_dma_system_start(path, -1, 0, 0, NULL);
		if (rc < 0)
		{
			sv_printf(cn, "! ERROR starting system: %s\n", ngpd_get_error_message());
			goto exit_error;
		}

		rc = ngpd_dma_wait_scope(path, -1, &scope_status);
		if (rc < 0) {
			sv_printf(cn,"! ERROR waiting for scope DMAs %s\n", ngpd_get_error_message());
		}
		if (scope_status != 0)
		{
			sv_printf(cn, "! ERROR: Scope status appears wrong at end of run = %08X\n", scope_status);
			return -1;
		}

		if (first_card==0 && last_card == num_cards-1)
		{
			read_all = 1;
			rc = ngpd_dma_read_scope(path, -1, 0, 0, 0);
			if (rc < 0) 
			{
				sv_printf(cn,"! ERROR reading scope data from all cards %s \n", ngpd_get_error_message());
				goto exit_error;
			}
		}
		else
			read_all = 0;
		for (card=first_card; card<= last_card; card++)
		{
			if (read_all == 0)
			{
				rc = ngpd_dma_read_scope(path, card, 0, 0, 0);

				if (rc < 0) 
				{
					sv_printf(cn,"! ERROR reading scope data from card %d : %s \n", card, ngpd_get_error_message());
					goto exit_error;
				}
			}
			for (stream=0; stream<chans_per_card; stream++)
			{
				chan = card*chans_per_card+stream;
				if (chan >= first_chan && chan <= last_chan)
				{
					sv_printf(cn, "# Chan=%d, for %d points\n", chan, scope_mod->head.num_t); 
					ana_ptr = ngpd_scope_mod_get_ptr(scope_mod, card, stream);
					if (ana_ptr == NULL)
					{
						sv_printf(cn, "! ERROR getting scope data pointers:'%s'\n", ngpd_get_error_message());
						goto exit_error;
					}
					iptr = (u_int32_t *)hist_mod->data + chan * hist_mod->head.num_x;
					for (i=0; i<scope_mod->head.num_t; i++)
					{
						iptr[*ana_ptr >> 2]++;
						ana_ptr += ana_inc;
					}
				}
			} /* For stream */
			if (sv_quit)
			{
				sv_printf(cn, "STOPPED By ^c\n");
				goto exit_error;
			}
		} /* For each slot */ 
	} /* For Pass */
	ngpd_set_run_flags(path, flags);

	if (do_fw)
	{
		for (chan=first_chan; chan <=last_chan; chan++)
		{
			double sum_f, sum_fx, sum_fxx, var;
			fptr = (float *)noise_mod->data+chan;
			iptr =  (int *) hist_mod->data+chan*hist_mod->head.num_x;
			max = 0;
			sum_f   = 0.0;
			sum_fx  = 0.0;
			sum_fxx = 0.0;
			for (i=0;i<hist_mod->head.num_x; i++)
			{
				int f = iptr[i];
				double x = (double)i;
				if (f > max)
					max = f;
				sum_f   += f;
				sum_fx  += f*x;
				sum_fxx += f*x*x;
			}
			fptr[X2NOISE_MEAN*num_chan] = sum_fx/sum_f;
			var = sum_fxx/sum_f - (sum_fx/sum_f)*(sum_fx/sum_f);
			if (var > 0)
				fptr[X2NOISE_STDEV*num_chan] = sqrt(var);
			else
				fptr[X2NOISE_STDEV*num_chan] = 0.0;
			for (i=0;i<hist_mod->head.num_x; i++)
				if (iptr[i] > 0)
					break;
			lhs = i;
			for (i=hist_mod->head.num_x-1; i>= 0; i--)
				if (iptr[i] > 0)
					break;
			rhs = i;
			fptr[X2NOISE_MIN*num_chan] = lhs;
			fptr[X2NOISE_MAX*num_chan] = rhs;
			fptr[X2NOISE_CODE_SPREAD*num_chan] = rhs-lhs;

			for (j=0;j<4; j++)
			{
				max /=10;
				if (max < 1)
					break;
				for (i=0;i<hist_mod->head.num_x; i++)
					if (iptr[i] > max)
						break;
				lhs = i;
				for (i=hist_mod->head.num_x-1; i>= 0; i--)
					if (iptr[i] > max)
						break;
				rhs = i;
				fptr[(j+X2NOISE_FW10)*num_chan] = rhs-lhs;
			}
		}
	}

	if (hist_mod != NULL)
		munlink(hist_head);
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

int ngpd_measure_noise (int quals, int cn, int path)
{
	int sys;
	MOD_IMAGE *noise_mod=NULL;
	mh_com *noise_head=NULL;
	int num_chan, num_cards, chans_per_card;
	int first_chan, last_chan;
	char mod_name[NGPD_MAX_MODNAME+20];
	int i, chan;
	NGPDScopeModule * scope_mod;
	u_int16_t *dig_ptr, *ana_ptr;
	float *fptr;
	int pass, num_pass=1;
	int first_card, last_card, card;		
	int no_clear = 0;
	u_int32 x;
	double sum_x[XSP_MAX_CHAN], sum_xx[XSP_MAX_CHAN];
	double var;
	int stream, rc;
	int flags;
	int ana_inc;
	int read_all;
	u_int32_t scope_status;

	num_chan = ngpd_get_num_chan(path);
	if (num_chan < 0)
	{
		sv_printf(cn, "! Bad NGPD Path : %s\n", ngpd_get_error_message());
		return -1;
	}
	num_cards = ngpd_get_num_cards(path);
	scope_mod = ngpd_scope_get_mod(path); 
	chans_per_card = ngpd_get_chans_per_card(path);

	first_chan = 0;
	last_chan  = num_chan-1;

	if (quals & 0x1)
		num_pass = Qualifier[0].IntArg;

	if (quals & 2)
		first_chan = last_chan = Qualifier[1].IntArg;


	sprintf(mod_name, "ngpd_noise");
	noise_mod = app_mkmod_head (cn, mod_name, num_chan, X2NOISE_NUM_ROWS, "Chan", "ADC Units", 1, &noise_head);
	if (noise_mod == NULL)
		goto exit_error;

	if (!no_clear)
	{
		fptr = (float *)(noise_mod->data+num_chan*X2NOISE_MAX);
		for (i=0; i<noise_mod->head.num_x; i++)
			fptr[i] = 0;
		fptr = (float *)(noise_mod->data+num_chan*X2NOISE_MIN);	/* Used for Min */
		for (i=0; i<noise_mod->head.num_x; i++)
			fptr[i] = 65535;
		for (i=0;i<num_chan; i++)
		{
			sum_x[i] = 0.0;
			sum_xx[i] = 0.0;
		}
	}
	first_card = first_chan/chans_per_card;
	last_card  = last_chan/chans_per_card;
	flags = ngpd_get_run_flags(path);
	ngpd_set_run_flags(path, flags | NGPD_RUN_FLAGS_SCOPEMODE);

	if ((rc=ngpd_cal_setup_all_ana(path, -1, 1, NGZMP_SCOPE_SEL_ANA_INPUT)) < 0)
	{
		sv_printf(cn, "! ERROR setting scope mode multiplexers: %s\n", ngpd_get_error_message());
		return rc;
	}
	ana_inc = ngpd_scope_mod_get_inc(scope_mod, 0); // Moved to after ngpd_cal_setup_all_ana
	for (pass = 0; pass < num_pass; pass++)
	{
		rc = ngpd_dma_system_start(path, -1, 0, 0, NULL);
		if (rc < 0)
		{
			sv_printf(cn, "! ERROR starting system: %s\n", ngpd_get_error_message());
			goto exit_error;
		}

		rc = ngpd_dma_wait_scope(path, -1, &scope_status);
		if (rc < 0) {
			sv_printf(cn,"! ERROR waiting for scope DMAs %s\n", ngpd_get_error_message());
		}
		if (scope_status != 0)
		{
			sv_printf(cn, "! ERROR: Scope status appears wrong at end of run = %08X\n", scope_status);
			return -1;
		}

		if (first_card==0 && last_card == num_cards-1)
		{
			read_all = 1;
			rc = ngpd_dma_read_scope(path, -1, 0, 0, 0);
			if (rc < 0) 
			{
				sv_printf(cn,"! ERROR reading scope data %s \n", ngpd_get_error_message());
				goto exit_error;
			}
		}
		else
			read_all = 0;
		for (card=first_card; card<= last_card; card++)
		{
			if (read_all == 0)
			{
				rc = ngpd_dma_read_scope(path, card, 0, 0, 0);

				if (rc < 0) 
				{
					sv_printf(cn,"! ERROR reading scope data %s \n", ngpd_get_error_message());
					goto exit_error;
				}
			}
			for (stream=0; stream<chans_per_card; stream++)
			{
				chan = card*chans_per_card+stream;
				if (chan >= first_chan && chan <= last_chan)
				{
					sv_printf(cn, "# Chan=%d, for %d points\n", chan, scope_mod->head.num_t); 
					ana_ptr = ngpd_scope_mod_get_ptr(scope_mod, card, stream);
					if (ana_ptr == NULL)
					{
						sv_printf(cn, "! ERROR reading scope data pointers:'%s'\n", ngpd_get_error_message());
						goto exit_error;
					}
					fptr = (float *)noise_mod->data + chan;
					for (i=0; i<scope_mod->head.num_t; i++)
					{
						x = *ana_ptr >> 2;
						sum_x[chan] += x;
						sum_xx[chan] += x*x;
						if (fptr[X2NOISE_MIN*num_chan] > x)
							fptr[X2NOISE_MIN*num_chan] = x;
						if (fptr[X2NOISE_MAX*num_chan] < x)
							fptr[X2NOISE_MAX*num_chan] = x;
						ana_ptr += ana_inc;
					} /* For i scanning each workspace full of data */
					if (sv_quit)
					{
						sv_printf(cn, "STOPPED By ^c\n");
						goto exit_error;
					}
				} /* If correct channel */
			} /* For stream */
		} /* For each Card */ 
	} /* For Pass */
	ngpd_set_run_flags(path, flags);
	for (chan=first_chan; chan<=last_chan; chan++)
	{
		fptr = (float *)noise_mod->data + chan;
		fptr[X2NOISE_MEAN*num_chan] = sum_x[chan]/scope_mod->head.num_t;
		var =  sum_xx[chan]/scope_mod->head.num_t  - sum_x[chan]*sum_x[chan]/((double)scope_mod->head.num_t*(double)scope_mod->head.num_t);
		if (var > 0)
			fptr[X2NOISE_STDEV*num_chan] = sqrt(var);
		fptr[X2NOISE_CODE_SPREAD*num_chan] = fptr[X2NOISE_MAX*num_chan] - fptr[X2NOISE_MIN*num_chan];
	}

	if (noise_mod != NULL)
		munlink(noise_head);
	return 0;


exit_error:
	if (noise_mod != NULL)
		munlink(noise_head);
	return -1;
}

