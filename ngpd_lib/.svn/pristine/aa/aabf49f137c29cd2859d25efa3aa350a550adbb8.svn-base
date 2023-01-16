#include "ngpd.h"
#if NGPD_CONF_ADQ14
#include "ADQAPI.h"
#endif
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#ifdef LINUX
#include <sys/time.h>
#endif


#include "zynqmp_api.h"
#include "zynqmp_protocol.h"
#include "zynqmp_api.h"

static int ngpd_save_filter_type(int path, int chan, NGPDFilter *filter)
{
	int this_path, chan_of_card;
	int rc;
	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_save_filter_type: Invalid path=%d", path);
		return -1;
	}
	
	if (chan < 0)
	{
		for (chan=0; chan<NGPDPath[path].num_chan; chan++)
			ngpd_save_filter_type(path, chan, filter);
	}
	else 
	{
		if ((rc=ngpd_resolve_path_chan_card(path, chan, &this_path, &chan_of_card, NULL)) < 0)
			return rc;
		NGPDPath[this_path].filter[chan_of_card] = *filter;
	}
	return 0;
}
int ngpd_get_filter_type(int path, int chan, NGPDFilter *filter)
{
	int this_path, chan_of_card;
	int rc;
	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_get_filter_type: Invalid path=%d", path);
		return -1;
	}
	
	if ((rc=ngpd_resolve_path_chan_card(path, chan, &this_path, &chan_of_card, NULL)) < 0)
	{
		char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
		strcpy(old_msg, ngpd_error_message);
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_get_filter_type: Invalid channel : %s", old_msg);
		ngpd_error_message[NGPD_MAX_ERROR_MESSAGE]=0;
		return rc;
	}
	*filter = NGPDPath[this_path].filter[chan_of_card];
	return 0;
}

int ngpd_filter_load_integer(int path, int chan, int num_words, u_int32_t *data, u_int32_t *statusP)
{
	int rc;
	u_int32_t status, coeff;
	int i, attempt;
	int card=0;
	NGPDFilter filter = {NGPDFilter_Unknown, 0, 0, 0.0};
	
	if (statusP != NULL)
		*statusP = 0xFFFFFFFF;
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_integer: Invalid path=%d", path);
		return -1;
	}
	if (num_words  != NGPD_FILT_NUM_COEFF)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_integer:expected number of coefficients=%d, not %d", NGPD_FILT_NUM_COEFF, num_words);
		return -1;
	}
	ngpd_save_filter_type(path, chan, &filter);
	if (chan < 0)
	{
		for (chan=0;chan<NGPDPath[path].num_chan; chan++)
			rc = ngpd_filter_load_integer(path, chan, num_words, data, statusP);
		return rc;
	}

	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
#if NGPD_CONF_ADQ14
		for (attempt=0; attempt<10; attempt++)
		{
			if ((rc=ngpd_read_glob_regs(path, card, NGPD_GLOB_SCOPE_STATUS, 1, &status)))
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_integer:Error reading status register");
				return rc;
			}
			if (!(status & NGPD_FILTER_STATUS_RELOAD_TREADY))
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_integer: Filter Coeff port report not ready, status=%08X\n", status);
				continue;
			}
			for (i=0;i<num_words;i++)
			{
				coeff = NGPD_FILTER_COEF(data[i]);
				if (i == num_words-1)
					coeff |= NGPD_FILTER_TLAST;
				if ((rc=ngpd_write_chan_regs(path, chan, NGPD_REGION_REGS, NGPD_CHAN_FILTER, 1, &coeff)) < 0)
				{
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_integer:Error writing Coefficient register");
					return rc;
				}
					
				if ((rc=ngpd_read_glob_regs(path, card, NGPD_GLOB_SCOPE_STATUS, 1, &status)))
				{
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_integer:Error reading status register");
					return rc;
				}
				if (status & NGPD_FILTER_STATUS_TLAST_UNEXPECTED)
				{
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_integer: detected unexpected tlast at word %d of %d", i, num_words);
					printf("%s\n", ngpd_error_message);
					break;
				}
				if (status & NGPD_FILTER_STATUS_TLAST_MISSING)
				{
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_integer: detected missing tlast at word %d of %d", i, num_words);
					printf("%s\n", ngpd_error_message);
					break;
				}
			}
			if ((status & (NGPD_FILTER_STATUS_TLAST_UNEXPECTED | NGPD_FILTER_STATUS_TLAST_MISSING)) == 0)
			{
				if (statusP != NULL)
					*statusP = status;
				return 0;
			}
		}
		if (statusP != NULL)
			*statusP = status;
		return -1;
#else
		printf("This build does not support ADQ14\n");
		return -1;
#endif
	}
	else
	{
		int this_path, chan_of_card;
		if ((rc=ngpd_resolve_path_chan_card(path, chan, &this_path, &chan_of_card, NULL)) < 0)
		{
			char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_integer: Invalid channel : %s", old_msg);
			ngpd_error_message[NGPD_MAX_ERROR_MESSAGE]=0;
			return rc;
		}
		if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
		{
			printf("Dummy: ngpd_filter_load_integer(card=%d, chan_of_card=%d)\n", card, chan_of_card);
			return 0;
		}
		rc = zynqmp_personality_write(NGPDPath[this_path].zynqmp, ZYNQMP_DMA_CMD_LOAD_FILTER,ZYNQMP_WIDTH_LONG, chan_of_card, num_words, (u_int8_t *)data, 1, &status, NULL);
		if (rc < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_integer: zynqmp_personality_writefails: %s", zynqmp_get_error_message());
			ngpd_error_message[NGPD_MAX_ERROR_MESSAGE]=0;
			return rc;
		}
		if (statusP != NULL)
			*statusP = status;
		if (status & NGZMP_FILTER_STATUS_TLAST_UNEXPECTED(chan_of_card))
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_integer: Unexpected tlast, status=%08X", status);
			return NGPD_ERROR;
		}
		else if (status & NGZMP_FILTER_STATUS_TLAST_MISSING(chan_of_card))
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_integer: Missing tlast, status=%08X", status);
			return NGPD_ERROR;
		}
		return NGPD_OK;
	}
}

int ngpd_filter_load_rect(int path, int chan, int num_ave)
{
	u_int32_t data[NGPD_FILT_NUM_COEFF];
	int i;
	int rc;
	NGPDFilter filter = {NGPDFilter_Rectangle, num_ave, 0, 0.0};
#if 1
	u_int32_t coeff=0x10000/num_ave;
#else
	u_int32_t coeff=0x20000/num_ave;
	if (num_ave == 1)
		coeff = 0x1FFFF;
#endif
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_rect: Invalid path=%d", path);
		return -1;
	}

	for (i=0;i<NGPD_FILT_NUM_COEFF; i++)
		data[i] = 0;


	/* num_ave     Coeffs
		1			1
		2			1 0.5
		3			1 1
		4           1 1 0.5
		*/
	for (i=0;i<=num_ave/2; i++)
	{
		if (i == num_ave/2 && (num_ave &1) == 0)
			data[NGPD_FILT_NUM_COEFF-1-i] = coeff/2;
		else
			data[NGPD_FILT_NUM_COEFF-1-i] = coeff;
		printf("i=%d, data=%d\n", i, data[NGPD_FILT_NUM_COEFF-1-i]);
	}
	rc = ngpd_filter_load_integer(path, chan, NGPD_FILT_NUM_COEFF, data, NULL);
	if (rc == 0)
		ngpd_save_filter_type(path, chan, &filter);
	return rc;
}

int ngpd_filter_load_double(int path, int chan, int num_words, double *data)
{
	u_int32_t i_data[NGPD_FILT_NUM_COEFF];
	int i;
	double total, scale, coeff;


	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_double: Invalid path=%d", path);
		return -1;
	}
	if (num_words  != NGPD_FILT_NUM_COEFF)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_double:expected number of coefficients=%d, not %d", NGPD_FILT_NUM_COEFF, num_words);
		return -1;
	}
	total = 0.0;
	for (i=0; i<num_words-1; i++)
		total += data[i];

	total = total*2+data[num_words-1];
#if 1
	scale = (double)0x10000/total;
#else
	scale = (double)0x20000/total;
#endif
	for (i=0; i<num_words; i++)
	{
		coeff = floor(data[i]*scale+0.5);
		if (coeff > 0x1FFFF)
			i_data[i] = 0x1FFFF;
		else if (coeff < -0x1FFFF)
			i_data[i] = (u_int32_t)(-0x1FFFF);
		else
			i_data[i] = (u_int32_t)coeff;
	}
	return ngpd_filter_load_integer(path, chan, num_words, i_data, NULL);
}

int ngpd_filter_load_exp(int path, int chan, double Tsamples)
{
	double data[NGPD_FILT_NUM_COEFF], t;
	int i;
	int rc;
	NGPDFilter filter = {NGPDFilter_Exponential, 0, 0, Tsamples};

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_exp: Invalid path=%d", path);
		return -1;
	}

	for (i=0;i<NGPD_FILT_NUM_COEFF; i++)
	{
		t = NGPD_FILT_NUM_COEFF-1-i;
		data[i] = exp(-t/Tsamples);
	}
	rc = ngpd_filter_load_double(path, chan, NGPD_FILT_NUM_COEFF, data);
	if (rc == 0)
		ngpd_save_filter_type(path, chan, &filter);
	return rc;
}

int ngpd_filter_load_gaus(int path, int chan, double sigma)
{
	double data[NGPD_FILT_NUM_COEFF], t;
	int i;
	int rc;
	NGPDFilter filter = {NGPDFilter_Gaussian, 0, 0, sigma};

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_gaus: Invalid path=%d", path);
		return -1;
	}

	for (i=0;i<NGPD_FILT_NUM_COEFF; i++)
	{
		t = NGPD_FILT_NUM_COEFF-1-i;
		data[i] = exp(-t*t/(2*sigma*sigma));
	}
	rc = ngpd_filter_load_double(path, chan, NGPD_FILT_NUM_COEFF, data);
	if (rc == 0)
		ngpd_save_filter_type(path, chan, &filter);
	return rc;
}


int ngpd_filter_load_trapeziod(int path, int chan, int wid_top, int wid_bot)
{
	double data[NGPD_FILT_NUM_COEFF];
	int i, j;
	int rc;
	double m;
	NGPDFilter filter = {NGPDFilter_Trapezoidal, wid_top, wid_bot, 0.0};

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_trapeziod: Invalid path=%d", path);
		return -1;
	}

	if (wid_bot <= wid_top)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_filter_load_trapeziod: require wid_top=%d to be less than wid_bot=%d", wid_top, wid_bot);
		return -1;
	}
	m = 2.0/(wid_bot-wid_top);
	for (i=0;i<NGPD_FILT_NUM_COEFF; i++)
	{
		j = NGPD_FILT_NUM_COEFF-1-i;
		if (j < wid_top/2)
			data[i] = 1.0;
		else if (j < wid_bot/2)
			data[i]= (j-wid_bot/2)*m;
		else
			data[i] = 0.0;
	}
	rc = ngpd_filter_load_double(path, chan, NGPD_FILT_NUM_COEFF, data);
	if (rc == 0)
		ngpd_save_filter_type(path, chan, &filter);
	return rc;
}
