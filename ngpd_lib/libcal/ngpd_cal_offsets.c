/* 	
	ngpd_cal_offsets.c
	ngpd calibration library
	Created: wih73   25/10/2021
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <sys/types.h>

#include "datamod.h"
#include "ngpd.h"
#include "ngpd_cal.h"

char ngpd_cal_error_message[NGPD_MAX_ERROR_MESSAGE+2];
#define CAL_OFFSETS_NUM_AVE 65536

int ngpd_cal_offsets(int path,  void (*nprintf)(void *msg_data, char *, ...), void *msg_data, int first_chan, int last_chan, double target, int num_pass, int adjust_only, char *save_fname)
{
	char mname[NGPD_MAX_MODNAME+2];
	MOD_IMAGE *offsets_mod=NULL, *mean_mod=NULL;
	mh_com *offsets_head, *mean_head;
	int num_chan;
	int t, card, pass;
	int rc = -1;
	NGPDGeneration generation;
	u_int16_t offsets_buf[NGZMP_MAX_CHAN];
	u_int16_t dga_buf[NGZMP_MAX_CHAN];
	u_int32_t * offsets;
	int save_flags, flags;
	u_int32_t scope_status;
	int num_cards;
	int stream, num_streams, chan;
	u_int16_t *ana_ptr;
	int ana_inc;
	NGPDScopeModule *scope_mod;
	double mean, *dp;
	double gain;
	int new_offset;

	if ((num_chan=ngpd_get_num_chan(path)) < 0)
		return -1;
		
	num_cards = ngpd_get_num_cards(path);
	generation = ngpd_get_generation(path);
	if (generation == NGPDGenADQ14)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_offsets: Not supported on ADQ14 version");
		return -1;
	}
	if ((scope_mod = ngpd_scope_get_mod(path)) == NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_offsets: Cannot get scope module pointer: %s", ngpd_get_error_message());
		return -1;
	}

	if (first_chan < 0 || first_chan >= num_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_offsets: first_chan (=%d) out of range 0... %d", first_chan, num_chan-1);
		return -1;
	}
	if (last_chan < 0)
		last_chan = num_chan-1;
	else if (last_chan < first_chan || last_chan >= num_chan)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_offsets: last_chan (=%d) out of range %d...%d", last_chan, first_chan, num_chan-1);
		return -1;
	}
				
	save_flags = ngpd_get_run_flags(path);
	flags = (save_flags & ~NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS) | NGPD_RUN_FLAGS_SCOPEMODE | NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD;
	ngpd_set_run_flags(path, flags);

	sprintf(mname, "offsets%dx%d",  num_chan, num_pass+1);
	if ((offsets_mod=id_mkmod_err_msg ( mname, num_chan, num_pass+1, "Chan", "Pass", DATA_LONG, &offsets_head, ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE)) == NULL)
		goto exit_error;
	id_clear_mod(offsets_mod);
	
	sprintf(mname, "adc_mean%dx%d",  num_chan, num_pass);
	if ((mean_mod=id_mkmod_err_msg ( mname, num_chan, num_pass, "Chan", "Pass", DATA_DOUBLE, &mean_head, ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE)) == NULL)
		goto exit_error;
	id_clear_mod(mean_mod);
	
	if ((rc=ngpd_i2c_read_preamp_offset(path, 0,  num_chan, offsets_buf)) < 0)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_offsets: Cannot read offsets: %s", ngpd_get_error_message());
		goto exit_error;
	}
	if ((rc=ngzmp_spi_read_dga(path, 0, num_chan,  dga_buf)) < 0)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_offsets: Cannot read DGA settings : %s", ngpd_get_error_message());
		goto exit_error;
	}
	rc = ngpd_cal_setup_all_ana(path, -1, 1, NGZMP_SCOPE_SEL_ANA_INPUT);
	if (rc <0)
		goto exit_error;
		
	num_streams = ngpd_scope_mod_get_nstreams(scope_mod);
	
	offsets = id_get_ptr(offsets_mod, 0, 0, 0);
	
	for (chan=0; chan<num_chan; chan++)
		offsets[chan] = offsets_buf[chan];
	
	if (adjust_only == 0)
	{
		for (chan=first_chan; chan<=last_chan; chan++)
		{
			gain = 1/1.69;	// Average gain a DAC per ADU at 13 dB
			gain *= pow(10.0, (dga_buf[chan]-13)/20.0);
			new_offset = (33000-target)*gain;
//			printf("Seeding chan %d target=%g, gain=%g, new=%d\n", chan, target, gain, new_offset); 
			if (new_offset < 1000)
				new_offset = 1000;	// Very low level offsets seem to make of buffer noisy. Reconsider if need to work at this level.
			else if (new_offset > 65535)
				new_offset = 65535;
			offsets[chan] = new_offset;
			offsets_buf[chan] = new_offset;
		}
		if ((rc=ngpd_i2c_write_preamp_offset(path, first_chan, 1+last_chan-first_chan, offsets_buf)) < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_offsets: Cannot write starting offsets: %s", ngpd_get_error_message());
			goto exit_error;
		}
	}
	for (pass=0; pass<num_pass; pass++)
	{
		offsets = id_get_ptr(offsets_mod, 0, pass, 0);
		rc = ngpd_dma_system_start(path, -1, 0, 0, NULL);
		if (rc < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_offsets: ERROR starting system: %s", ngpd_get_error_message());
			goto exit_error;
		}
		rc = ngpd_dma_wait_scope(path, -1, &scope_status);
		if (rc < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_offsets: ERROR waiting for scope DMAs %s", ngpd_get_error_message());
			goto exit_error;
		}
		if (scope_status != 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_offsets : Scope status appears wrong at end of run = %08X", scope_status);
			return -1;
		}
		for (card=0; card < num_cards; card++)
		{
			int card_first, card_num;
			rc = ngpd_get_chans_on_card(path, card, &card_first, &card_num);
			if (rc <0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_offsets: ERROR checking channel on card : %s", ngpd_get_error_message());
				goto exit_error;
			}
			if (first_chan >= card_first+card_num || last_chan < card_first)
				continue;
			
			if ((rc=ngpd_scope_update_mod(path, card, 0, CAL_OFFSETS_NUM_AVE)) < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_offsets: reading scope data: %s", ngpd_get_error_message());
				goto exit_error;
			}
			for (stream=0; stream <num_streams; stream++)
			{
				chan = stream+card_first;		// FixME: Could change here if wanted more somplex stram to cahnnle mapping (e.g .with digital data)
				if (chan < first_chan || chan > last_chan)
					continue;
				ana_ptr = ngpd_scope_mod_get_ptr(scope_mod, card, stream);
				ana_inc = ngpd_scope_mod_get_inc(scope_mod, stream);
				if (ana_ptr == NULL || ana_inc <=0 )
				{
					snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_offsets: Cannot access scope data module ptr=%p, inc=%d", ana_ptr, ana_inc);
					goto exit_error;
				}
				for (t=0; t<CAL_OFFSETS_NUM_AVE; t++)
				{
					mean += *ana_ptr;
					ana_ptr += ana_inc;
				}
				mean /= CAL_OFFSETS_NUM_AVE;
				dp = (double *)id_get_ptr(mean_mod, chan, pass, 0);
				*dp = mean;
				gain = 1/1.69;	// Average gain a DAC per ADU at 13 dB
				gain *= pow(10.0, (dga_buf[chan]-13)/20.0);
				new_offset = offsets[chan]+gain*(mean-target);
//				printf("chan %d old=%g, target=%g, gain=%g, new=%d\n", chan, mean, target, gain, new_offset); 
				if (new_offset < 1000)
					new_offset = 1000;	// Very low level offsets seem to make of buffer noisy. Reconsider if need to work at this level.
				else if (new_offset > 65535)
					new_offset = 65535;
				offsets[chan+num_chan] = new_offset;
			}
		} /* End for card */
		for (chan=first_chan; chan <= last_chan; chan++)
			offsets_buf[chan] = offsets[chan+num_chan];
		if ((rc=ngpd_i2c_write_preamp_offset(path, first_chan, 1+last_chan-first_chan, offsets_buf)) < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_offsets: Cannot write new offsets: %s", ngpd_get_error_message());
			goto exit_error;
		}
	}
	if (save_fname != NULL)
	{
		FILE *ofp = fopen(save_fname, "w");
		if (ofp == NULL)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_offsets: Cannot open output file %s, errno=%d\n", save_fname, errno);
			goto exit_error;
		}
		for (chan=0; chan<num_chan; chan++)
			fprintf(ofp,  "%d\n", offsets[chan+num_chan]);
		fclose(ofp);
	}
			
	rc = 0;
exit_error:
	if (offsets_mod != NULL)
		munlink(offsets_head);
	if (mean_mod != NULL)
		munlink(mean_head);
	ngpd_set_run_flags(path, save_flags);
	return rc;
}

