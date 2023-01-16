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
#include <stdlib.h>
#endif


#include "ngpd.h"
#include "zynqmp_protocol.h"
#include "zynqmp_api.h"

int ngpd_read_chan_regs(int path, int chan, int region, int offset, int num_regs, u_int32_t *data)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_chan_regs: Invalid path=%d", path);
		return -1;
	}
	/* offset bits: Full width 19
		But currently the highest working bit is bit 13

	 13    1 => Chan, 0 => global
	 12 Chan
	 11 Region 1 =. BRAM, 0 => regs
	 10..0  Upto 2048 register or BRAM addresses
	 */
	
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
#if NGPD_CONF_ADQ14
		offset |= 1 << 13 | (chan & 1) << 12 | (region & 0x1) << 11;
		return -!ADQ_ReadBlockUserRegister(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 2, offset, data, num_regs*sizeof(u_int32_t), 1);
#else
		printf("ngpd_read_chan_regs: ADQ14 not configured in this build\n");
		return -1;
#endif
	}
	else
	{
		int this_path, chan_of_card;
		int rc;
		if ((rc=ngpd_resolve_path_chan_card(path, chan, &this_path, &chan_of_card, NULL)) < 0)
		{
			char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_chan_regs: Invalid channel : %s", old_msg);
			ngpd_error_message[NGPD_MAX_ERROR_MESSAGE]=0;
			return rc;
		}
		if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
		{
			if (region == NGZMP_REGION_REGS)
				memcpy(data, NGPDPath[this_path].dummy_chan_reg[chan_of_card]+offset, sizeof(u_int32_t)*num_regs);
			else
				memset(data, 0, sizeof(u_int32_t)*num_regs);
			return 0;
		}
			
		offset = chan_of_card << (NGZMP_ADDR_SLICE_CHAN-2) | region << (NGZMP_ADDR_SLICE_REGION-2) | offset; 
		return zynqmp_access_read(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_REGS, ZYNQMP_WIDTH_LONG, offset, num_regs, 1, (u_int8_t *)data);
	}
}

int ngpd_read_glob_regs(int path, int card, int offset, int num_regs, u_int32_t *data)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_glob_regs: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
#if NGPD_CONF_ADQ14
		return -!ADQ_ReadBlockUserRegister(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 2, offset, data, num_regs*sizeof(u_int32_t), 1);
#else
		printf("ngpd_read_glob_regs: ADQ14 not configured in this build\n");
		return -1;
#endif
	}
	else
	{
		int this_path;
		if (card < 0 || card >= NGPDPath[path].num_cards)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_glob_regs: Card=%d out of range 0...%d", card, NGPDPath[path].num_cards-1);
			return -1;
		}
		if (NGPDPath[path].type == NGPDComposite)
		{
			this_path = NGPDPath[path].sub_path[card];
			if (this_path <0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_glob_regs: Card=%d resolved illegal sub_path=%d", card, this_path);
				return -1;
			}
		}
		else
		{
			this_path = path;
		}
		if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
		{
			if (offset+num_regs > NGZMP_GLOB_NUM)
				memset(data, 0, sizeof(u_int32_t)*num_regs);
			else
				memcpy(data, NGPDPath[this_path].dummy_glob_reg+offset, sizeof(u_int32_t)*num_regs);
			return 0;
		}
		offset = 1 << (NGZMP_ADDR_BIT_GLOB_REGS-2) | offset;
		return zynqmp_access_read(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_REGS, ZYNQMP_WIDTH_LONG, offset, num_regs, 1, (u_int8_t *)data);
	}
}

int ngpd_rmw_glob_regs(int path, int card,  int offset, u_int32_t and_mask, u_int32_t or_mask, u_int32_t *ret_value)
{
	u_int32_t new_value;
	int rc;
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_rmw_glob_regs: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
#if NGPD_CONF_ADQ14
		rc = -!ADQ_ReadBlockUserRegister(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 2, offset, &new_value, sizeof(u_int32_t), 1);
		if (rc < 0)
			return rc;
		new_value = (new_value & and_mask) | or_mask;
		rc =  -!ADQ_WriteBlockUserRegister(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 2, offset, &new_value, sizeof(u_int32_t), 1);
		if (rc < 0)
			return rc;
		if (ret_value != NULL)
			*ret_value = new_value;
		return 0;
#else
		printf("ngpd_rmw_glob_regs: ADQ14 not configured in this build\n");
		return -1;
#endif
	}
	else
	{
		int this_path;
		if (card < 0 || card >= NGPDPath[path].num_cards)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_rmw_glob_regs: Card=%d out of range 0...%d", card, NGPDPath[path].num_cards-1);
			return -1;
		}
		if (NGPDPath[path].type == NGPDComposite)
		{
			this_path = NGPDPath[path].sub_path[card];
			if (this_path <0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_rmw_glob_regs: Card=%d resolved illegal sub_path=%d", card, this_path);
				return -1;
			}
		}
		else
		{
			this_path = path;
		}
		if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
		{
			printf ("ngpd_rmw_glob_regs: Card=%d, Offset=%d : and=0x%08X, or=%08X\n", card, and_mask, or_mask);
			return 0;
		}
		offset = 1 << (NGZMP_ADDR_BIT_GLOB_REGS-2) | offset;
		return (int) zynqmp_access_rmw(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_REGS, ZYNQMP_WIDTH_LONG, offset, and_mask, or_mask, (u_int8_t *)ret_value);
	}
}


int ngpd_write_chan_regs(int path, int chan, int region, int offset, int num_regs, u_int32_t *data)
{
	int rc;
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_chan_regs: Invalid path=%d", path);
		return -1;
	}
	if (chan < 0)
	{
		for (chan=0; chan<NGPDPath[path].num_chan; chan++)
		{
			rc = ngpd_write_chan_regs(path, chan, region, offset, num_regs, data);
			if (rc < 0)
				return rc;
		}
		return 0;
	}

	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
#if NGPD_CONF_ADQ14
		offset |= 1 << 13 | (chan & 1) << 12 | (region & 0x1) << 11;
		return -!ADQ_WriteBlockUserRegister(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 2, offset, data, num_regs*sizeof(u_int32_t), 1);
#else
		printf("ngpd_write_chan_regs: ADQ14 not configured in this build\n");
		return -1;
#endif
	}
	else
	{
		int this_path, chan_of_card;
		int rc;
		if ((rc=ngpd_resolve_path_chan_card(path, chan, &this_path, &chan_of_card, NULL)) < 0)
		{
			char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
			strcpy(old_msg, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_chan_regs: Invalid channel : %s", old_msg);
			ngpd_error_message[NGPD_MAX_ERROR_MESSAGE]=0;
			return rc;
		}
		if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
		{
			if (region == NGZMP_REGION_REGS)
				memcpy(NGPDPath[this_path].dummy_chan_reg[chan_of_card]+offset, data, sizeof(u_int32_t)*num_regs);
			return 0;
		}

		offset = chan_of_card << (NGZMP_ADDR_SLICE_CHAN-2) | region << (NGZMP_ADDR_SLICE_REGION-2) | offset; 
		rc =  zynqmp_access_write(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_REGS, ZYNQMP_WIDTH_LONG, offset, num_regs, 1, (u_int8_t *)data);
		if (rc < 0)
		{
			strncpy(ngpd_error_message, zynqmp_get_error_message(), NGPD_MAX_ERROR_MESSAGE);
			ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
		}
		return rc;
	}
}


int ngpd_write_glob_regs(int path, int card, int offset, int num_regs, u_int32_t *data)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_glob_regs: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
#if NGPD_CONF_ADQ14
		return -!ADQ_WriteBlockUserRegister(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 2, offset, data, num_regs*sizeof(u_int32_t), 1);
#else
		printf("ngpd_write_glob_regs: ADQ14 not configured in this build\n");
		return -1;
#endif
	}
	else
	{
		int this_path;
		int rc;
		if (card < 0)
		{
			for (card=0; card<NGPDPath[path].num_cards; card++)
			{
				if ((rc = ngpd_write_glob_regs(path, card, offset, num_regs, data)) < 0)
					return rc;
			}
			return 0;
		}
		else if (card >= NGPDPath[path].num_cards)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_glob_regs: Card=%d out of range 0...%d", card, NGPDPath[path].num_cards-1);
			return -1;
		}
		if (NGPDPath[path].type == NGPDComposite)
		{
			this_path = NGPDPath[path].sub_path[card];
			if (this_path <0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_glob_regs: Card=%d resolved illegal sub_path=%d", card, this_path);
				return -1;
			}
		}
		else
		{
			this_path = path;
		}
		if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
		{
			memcpy(NGPDPath[this_path].dummy_glob_reg+offset, data, sizeof(u_int32_t)*num_regs);
			return 0;
		}
		offset = 1 << (NGZMP_ADDR_BIT_GLOB_REGS-2) | offset;
		rc = zynqmp_access_write(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_REGS, ZYNQMP_WIDTH_LONG, offset, num_regs, 1, (u_int8_t *)data);
		if (rc < 0 )
		{
			strncpy(ngpd_error_message, zynqmp_get_error_message(), NGPD_MAX_ERROR_MESSAGE);
			ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
		}
		return rc;
	}
}

int ngpd_write_chan_cont(int path, int chan, u_int32_t chan_cont)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_chan_cont: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_chan_cont: Not supported on ADC14");
		return -1;
	}
	return ngpd_write_chan_regs(path, chan, NGZMP_REGION_REGS, NGPD_CHAN_CONT, 1, &chan_cont);
}

int ngpd_read_chan_cont(int path, int chan, u_int32_t *chan_cont)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_chan_cont: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_chan_cont: Not supported on ADC14");
		return -1;
	}
	return ngpd_read_chan_regs(path, chan, NGZMP_REGION_REGS, NGPD_CHAN_CONT, 1, chan_cont);
}

int ngpd_write_diff_trigger(int path, int chan, NGPDDiffTrigger * trig)
{
	u_int32_t trig_abc[3];

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_diff_trigger: Invalid path=%d", path);
		return -1;
	}
	if (trig->thres < NGPD_DIFF_TRIG_MIN_THRES || trig->thres > NGPD_DIFF_TRIG_MAX_THRES)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_diff_trigger: Invalid threshold=%d, expected %d...%d", trig->thres, NGPD_DIFF_TRIG_MIN_THRES, NGPD_DIFF_TRIG_MAX_THRES);
		return -1;
	}
	if (trig->sep < NGPD_DIFF_TRIG_MIN_SEP || trig->sep > NGPD_DIFF_TRIG_MAX_SEP)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_diff_trigger: Invalid Differential separation=%d, expected %d...%d", trig->sep, NGPD_DIFF_TRIG_MIN_SEP, NGPD_DIFF_TRIG_MAX_SEP);
		return -1;
	}
	if (trig->data_delay < NGPD_DIFF_TRIG_MIN_DATA_DELAY || trig->data_delay > NGPD_DIFF_TRIG_MAX_DATA_DELAY)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_diff_trigger: Invalid data delay=%d, expected %d...%d", trig->data_delay, NGPD_DIFF_TRIG_MIN_DATA_DELAY, NGPD_DIFF_TRIG_MAX_DATA_DELAY);
		return -1;
	}
	if (trig->trig_delay < NGPD_DIFF_TRIG_MIN_TRIG_DELAY || trig->trig_delay > NGPD_DIFF_TRIG_MAX_TRIG_DELAY)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_diff_trigger: Invalid trigger delay=%d, expected %d...%d", trig->trig_delay, NGPD_DIFF_TRIG_MIN_TRIG_DELAY, NGPD_DIFF_TRIG_MAX_TRIG_DELAY);
		return -1;
	}
	if (trig->width_a < NGPD_DIFF_TRIG_MIN_TRIG_STRETCH || trig->width_a > NGPD_DIFF_TRIG_MAX_TRIG_STRETCH)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_diff_trigger: Invalid Stretched trigger A stretch=%d, expected %d...%d", trig->width_a, NGPD_DIFF_TRIG_MIN_TRIG_STRETCH, NGPD_DIFF_TRIG_MAX_TRIG_STRETCH);
		return -1;
	}
	if (trig->delay_a < NGPD_DIFF_TRIG_MIN_DELAY_AB || trig->delay_a > NGPD_DIFF_TRIG_MAX_DELAY_AB)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_diff_trigger: Invalid stretched trigger A delay=%d, expected %d...%d", trig->delay_a, NGPD_DIFF_TRIG_MIN_DELAY_AB, NGPD_DIFF_TRIG_MAX_DELAY_AB);
		return -1;
	}
	if (trig->width_b < NGPD_DIFF_TRIG_MIN_TRIG_STRETCH || trig->width_b > NGPD_DIFF_TRIG_MAX_TRIG_STRETCH)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_diff_trigger: Invalid stretched trigger B stretch=%d, expected %d...%d", trig->width_b, NGPD_DIFF_TRIG_MIN_TRIG_STRETCH, NGPD_DIFF_TRIG_MAX_TRIG_STRETCH);
		return -1;
	}
	if (trig->delay_b < NGPD_DIFF_TRIG_MIN_DELAY_AB || trig->delay_b > NGPD_DIFF_TRIG_MAX_DELAY_AB)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_diff_trigger: Invalid stretched trigger B delay=%d, expected %d...%d", trig->delay_b, NGPD_DIFF_TRIG_MIN_DELAY_AB, NGPD_DIFF_TRIG_MAX_DELAY_AB);
		return -1;
	}

	trig_abc[0] = NGPD_DIFF_TRIG_A_THRES(trig->thres) | NGPD_DIFF_TRIG_A_SEP(trig->sep-NGPD_DIFF_TRIG_MIN_SEP) | NGPD_DIFF_TRIG_A_DATA_DELAY (trig->data_delay);
	trig_abc[1] = NGPD_DIFF_TRIG_B_TRIG_DELAY(trig->trig_delay) | NGPD_DIFF_TRIG_B_STRETCH_A(trig->width_a) | NGPD_DIFF_TRIG_B_DELAY_A(trig->delay_a);
	trig_abc[2] =    											  NGPD_DIFF_TRIG_C_STRETCH_B(trig->width_b) | NGPD_DIFF_TRIG_C_DELAY_B(trig->delay_b);

	return ngpd_write_chan_regs(path, chan, NGPD_REGION_REGS, NGPD_CHAN_DIFF_TRIG_A, 3, trig_abc);
}

int ngpd_read_diff_trigger(int path, int chan, NGPDDiffTrigger * trig)
{
	u_int32_t trig_abc[3];
	int rc;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_diff_trigger: Invalid path=%d", path);
		return -1;
	}
	rc = ngpd_read_chan_regs(path, chan, NGPD_REGION_REGS, NGPD_CHAN_DIFF_TRIG_A, 3, trig_abc);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_diff_trigger:Error reading trigger registers");
		return rc;
	}
	trig->thres = NGPD_DIFF_TRIG_A_GET_THRES(trig_abc[0]);
	trig->sep = NGPD_DIFF_TRIG_A_GET_SEP(trig_abc[0])+NGPD_DIFF_TRIG_MIN_SEP;
	trig->data_delay = NGPD_DIFF_TRIG_A_GET_DATA_DELAY(trig_abc[0]);
	trig->trig_delay = NGPD_DIFF_TRIG_B_GET_TRIG_DELAY(trig_abc[1]);
	trig->width_a = NGPD_DIFF_TRIG_B_GET_STRETCH_A(trig_abc[1]);
	trig->delay_a = NGPD_DIFF_TRIG_B_GET_DELAY_A(trig_abc[1]);
	trig->width_b = NGPD_DIFF_TRIG_C_GET_STRETCH_B(trig_abc[2]);
	trig->delay_b = NGPD_DIFF_TRIG_C_GET_DELAY_B(trig_abc[2]);
	
	return 0;
}


int ngpd_write_baseline_subtract(int path, int chan, NGPDBaseSubtract * bsub)
{
	u_int32_t bsub_regs[2];

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_baseline_subtract: Invalid path=%d", path);
		return -1;
	}
	if (bsub->div_cont < 0 || bsub->div_cont > 7)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_baseline_subtract: Expected divide control in range0..7 not %d", bsub->div_cont);
		return -1;
	}
	bsub_regs[0] = NGPD_BASELINE_A_FIXED(bsub->fixed) | NGPD_BASELINE_A_DIV_CONT(bsub->div_cont);
	if (bsub->use_fixed)
		bsub_regs[0] |= NGPD_BASELINE_A_USE_FIXED;
	bsub_regs[1] = NGPD_BASELINE_B_ERR_LIM(bsub->error_limit);

	return ngpd_write_chan_regs(path, chan, NGPD_REGION_REGS, NGPD_CHAN_BASESUB_A, 2, bsub_regs);
}

int ngpd_read_baseline_subtract(int path, int chan, NGPDBaseSubtract * bsub)
{
	u_int32_t bsub_regs[2];
	int rc;
	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_baseline_subtract: Invalid path=%d", path);
		return -1;
	}
	rc =  ngpd_read_chan_regs(path, chan, NGPD_REGION_REGS, NGPD_CHAN_BASESUB_A, 2, bsub_regs);
	if (rc < 0)
		return rc;
	
	bsub->fixed = NGPD_BASELINE_A_GET_FIXED(bsub_regs[0]);
	bsub->div_cont = NGPD_BASELINE_A_GET_DIV_CONT(bsub_regs[0]);
	bsub->use_fixed = !!(bsub_regs[0] & NGPD_BASELINE_A_USE_FIXED);
	bsub->error_limit = NGPD_BASELINE_B_GET_ERR_LIM(bsub_regs[1]);

	return 0;
}
 
int ngpd_write_measure(int path, int chan, NGPDMeasure * measure)
{
	u_int32_t measure_regs[5] = {0, 0, 0, 0 ,0 };
	int frac;
	int rc;
	u_int32_t i;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_measure: Invalid path=%d", path);
		return -1;
	}
	if (measure->tail_sum_delay < 0 || measure->tail_sum_delay > NGPD_MEASURE_SUM_DELAY_MAX)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_measure: sum_delay value %d, expected 0...%d", measure->tail_sum_delay, NGPD_MEASURE_SUM_DELAY_MAX);
		return -1;
	}

	if (measure->tail_sum_num < 1 || measure->tail_sum_num > NGPD_MEASURE_SUM_NUM_MAX)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_measure: number to sum value %d, expected 1...%d", measure->tail_sum_num, NGPD_MEASURE_SUM_NUM_MAX);
		return -1;
	}
	if (measure->fall_time_frac < 0.0 || measure->fall_time_frac > 1.0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_measure: fall time fraction %g, expected 0 ... 1.0", measure->fall_time_frac);
		return -1;
	}
	if (measure->min_height <0 || measure->min_height > NGPD_MEASURE_HEIGHT_MAX)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_measure: min_height=%d, expected 0 ... %d", measure->min_height, NGPD_MEASURE_HEIGHT_MAX);
		return -1;
	}
	if (measure->max_height <0 || measure->max_height > NGPD_MEASURE_HEIGHT_MAX)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_measure: max_height=%d, expected 0 ... %d", measure->max_height, NGPD_MEASURE_HEIGHT_MAX);
		return -1;
	}
	if (measure->max_height < measure->min_height)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_measure: expected min_height (%d) < max_height(%d)", measure->min_height, measure->max_height);
		return -1;
	}

	if (measure->min_fall_time <0 || measure->min_fall_time > NGPD_MEASURE_FALL_TIME_MAX)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_measure: min_fall_time=%d, expected 0 ... %d", measure->min_fall_time, NGPD_MEASURE_FALL_TIME_MAX);
		return -1;
	}
	if (measure->max_fall_time <0 || measure->max_fall_time > NGPD_MEASURE_FALL_TIME_MAX)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_measure: max_fall_time=%d, expected 0 ... %d", measure->max_fall_time, NGPD_MEASURE_FALL_TIME_MAX);
		return -1;
	}
	if (measure->max_fall_time < measure->min_fall_time)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_measure: expected min_fall_time (%d) < max_fall_time(%d)", measure->min_fall_time, measure->max_fall_time);
		return -1;
	}

	if (measure->min_tail_count <0 || measure->min_tail_count > NGPD_MEASURE_TAIL_COUNT_MAX)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_measure: min_tail_count=%d, expected 0 ... %d", measure->min_tail_count, NGPD_MEASURE_TAIL_COUNT_MAX);
		return -1;
	}
		
	frac= (int)floor(measure->fall_time_frac * 0x40000 + 0.5);
	if (frac < 0)
		frac = 0;
	else if (frac > 0x3FFFF)
		frac = 0x3FFFF;

	NGPDPath[path].histogram.ignores = 0;
	if (measure->use_abs_trig)
		measure_regs[0] = NGPD_MEAS_A_USE_ABS_TRIG;
	if (measure->adaptive_tail_sum)
		measure_regs[0] |= NGPD_MEAS_A_ADAPTIVE_TAIL_SUM;
	if (measure->enable_tail_subtract)
		measure_regs[0] |= NGPD_MEAS_A_IGNORE_ENB_TAIL_SUB;
	if (measure->tail_subtract_test)
		measure_regs[0] |= NGPD_MEAS_A_IGNORE_TAIL_SUB_TEST;	
	if (measure->tail_subtract_test_neutron)
		measure_regs[0] |= NGPD_MEAS_A_IGNORE_TAIL_SUB_TEST_N;


	if (measure->ignore_fall_time)
	{
		measure_regs[0] |= NGPD_MEAS_A_IGNORE_FALL_TIME;
		NGPDPath[path].histogram.ignores |= NGPDHistIgnoreFallTime;
	}
	if (measure->ignore_tail_sum)
	{
		measure_regs[0] |= NGPD_MEAS_A_IGNORE_TAIL_SUM;
		NGPDPath[path].histogram.ignores |= NGPDHistIgnoreTailSum;
	}
	measure_regs[1] = NGPD_MEAS_B_TAIL_FRAC(frac);
	measure_regs[2] = NGPD_MEAS_C_SUM_DELAY(measure->tail_sum_delay) | NGPD_MEAS_C_SUM_NUM(measure->tail_sum_num);
	measure_regs[3] = NGPD_MEAS_D_MIN_HEIGHT(measure->min_height) | NGPD_MEAS_D_MAX_HEIGHT(measure->max_height);
	measure_regs[4] = NGPD_MEAS_E_MIN_FALL_TIME(measure->min_fall_time) | NGPD_MEAS_E_MAX_FALL_TIME(measure->max_fall_time) | NGPD_MEAS_E_MIN_TAIL_COUNT(measure->min_tail_count);

	rc = ngpd_write_chan_regs(path, chan, NGPD_REGION_REGS, NGPD_CHAN_MEASURE_A, 5, measure_regs);
	if (rc == 0)
	{
		if (NGPDPath[path].generation == NGPDGenADQ14)
		{
			i = 0;
			rc = ngpd_write_chan_regs(path, chan, NGPD_REGION_DPRAM, NGPD_CHAN_DPADDR, 1, &i);
			for (i=0;i<NGPD_MEASURE_TAIL_THRES_SIZE; i++)
			{
				u_int32_t mc[2];
				mc[0]= (u_int32_t)(measure->tail_thres_m[i]);
				mc[1]= (u_int32_t)(measure->tail_thres_c[i]);
				rc = ngpd_write_chan_regs(path, chan, NGPD_REGION_DPRAM, NGPD_CHAN_MEASURE_TAIL_THRES_M, 2, mc);
			}
		}
		else
		{
			rc = ngpd_write_chan_regs(path, chan, NGZMP_REGION_TAIL_THRES_M, 0, NGPD_MEASURE_TAIL_THRES_SIZE, (u_int32_t *)(measure->tail_thres_m));
			if (rc == 0)
				rc = ngpd_write_chan_regs(path, chan, NGZMP_REGION_TAIL_THRES_C, 0, NGPD_MEASURE_TAIL_THRES_SIZE, (u_int32_t *)(measure->tail_thres_c));
		}
	}
	NGPDPath[path].histogram.min_tail_count = measure->min_tail_count;
	return rc;
}

int ngpd_read_measure(int path, int chan, NGPDMeasure * measure)
{
	u_int32_t measure_regs[5] = {0, 0, 0, 0, 0};
	int frac;
	u_int32_t i;
	int rc;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_measure: Invalid path=%d", path);
		return -1;
	}
	if (ngpd_read_chan_regs(path, chan, NGPD_REGION_REGS, NGPD_CHAN_MEASURE_A, 5, measure_regs) < 0)
	{
		char old_message[NGPD_MAX_ERROR_MESSAGE+2];
		strcpy(old_message, ngpd_error_message);
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_measure: Error reading measure registers: %s", old_message);
		return -1;
	}

	
	frac= (int)floor(measure->fall_time_frac * 0x40000 + 0.5);
	if (frac < 0)
		frac = 0;
	else if (frac > 0x3FFFF)
		frac = 0x3FFFF;

	measure->use_abs_trig = !!(	measure_regs[0] & NGPD_MEAS_A_USE_ABS_TRIG);
	measure->adaptive_tail_sum = !!(measure_regs[0] & NGPD_MEAS_A_ADAPTIVE_TAIL_SUM);
	measure->ignore_fall_time = !!(measure_regs[0] & NGPD_MEAS_A_IGNORE_FALL_TIME);
	measure->ignore_tail_sum = !!(measure_regs[0] & NGPD_MEAS_A_IGNORE_TAIL_SUM);
	measure->enable_tail_subtract = !!(measure_regs[0] & NGPD_MEAS_A_IGNORE_ENB_TAIL_SUB);
	measure->tail_subtract_test = !!(measure_regs[0] & NGPD_MEAS_A_IGNORE_TAIL_SUB_TEST);
	measure->tail_subtract_test_neutron = !!(measure_regs[0] & NGPD_MEAS_A_IGNORE_TAIL_SUB_TEST_N);

	frac = NGPD_MEAS_B_GET_TAIL_FRAC(measure_regs[1]);
	measure->fall_time_frac = (double)frac/0x40000;
	measure->tail_sum_delay = NGPD_MEAS_C_GET_SUM_DELAY(measure_regs[2]);
	measure->tail_sum_num = NGPD_MEAS_C_GET_SUM_NUM(measure_regs[2]);

	measure->min_height = NGPD_MEAS_D_GET_MIN_HEIGHT(measure_regs[3]); 
	measure->max_height = NGPD_MEAS_D_GET_MAX_HEIGHT(measure_regs[3]);
	measure->min_fall_time = NGPD_MEAS_E_GET_MIN_FALL_TIME(measure_regs[4] );
	measure->max_fall_time = NGPD_MEAS_E_GET_MAX_FALL_TIME(measure_regs[4] );
	measure->min_tail_count = NGPD_MEAS_E_GET_MIN_TAIL_COUNT(measure_regs[4] );
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		i = 0;
		rc = ngpd_write_chan_regs(path, chan, NGPD_REGION_DPRAM, NGPD_CHAN_DPADDR, 1, &i);
		for (i=0;i<NGPD_MEASURE_TAIL_THRES_SIZE; i++)
		{
			u_int32_t mc[2];
			rc = ngpd_read_chan_regs(path, chan, NGPD_REGION_DPRAM, NGPD_CHAN_MEASURE_TAIL_THRES_M, 2, mc);
			measure->tail_thres_m[i] = (int)mc[0];
			measure->tail_thres_c[i] = (int)mc[1];
		}
	}
	else
	{
		rc = ngpd_read_chan_regs(path, chan, NGZMP_REGION_TAIL_THRES_M, 0, NGPD_MEASURE_TAIL_THRES_SIZE, (u_int32_t *)(measure->tail_thres_m));
		if (rc == 0)
			rc = ngpd_read_chan_regs(path, chan, NGZMP_REGION_TAIL_THRES_C, 0, NGPD_MEASURE_TAIL_THRES_SIZE, (u_int32_t *)(measure->tail_thres_c));
	}
	return rc;
}

int ngpd_write_tail_subtract(int path, int chan, int32_t *data)
{
	int rc;
	u_int32_t i;
	int j;
	int32_t unpacked[NGPD_MEASURE_TAIL_SUB_SIZE];

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_tail_subtract: Invalid path=%d", path);
		return -1;
	}

	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		i = 0;
		if ((rc=ngpd_write_chan_regs(path, chan, NGPD_REGION_DPRAM, NGPD_CHAN_DPADDR, 1, &i)) < 0)
		{
			char old_message[NGPD_MAX_ERROR_MESSAGE+2];
			strcpy(old_message, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_tail_subtract: Error writing DPRAM address registers: %s", old_message);
			return rc;
		}

		for (j=0; j<NGPD_MEASURE_TAIL_SUB_SIZE; j++)
		{
			if ((rc=ngpd_write_chan_regs(path, chan, NGPD_REGION_DPRAM, NGPD_CHAN_MEASURE_TAIL_SUB_EVEN, 2, (u_int32_t *)(data+2*j))) < 0)
			{
				char old_message[NGPD_MAX_ERROR_MESSAGE+2];
				strcpy(old_message, ngpd_error_message);
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_tail_subtract: Error writing DPRAM data: %s", old_message);
				return rc;
			}
		}
	}
	else
	{
		for (j=0;j<2; j++)
		{
			for (i=0; i<NGPD_MEASURE_TAIL_SUB_SIZE; i++)
				unpacked[i] = data[2*i+j];
			if ((rc=ngpd_write_chan_regs(path, chan, NGZMP_REGION_TAIL_SUB0+j, 0, NGPD_MEASURE_TAIL_SUB_SIZE, (u_int32_t *)unpacked)) < 0)
			{
				char old_message[NGPD_MAX_ERROR_MESSAGE+2];
				strcpy(old_message, ngpd_error_message);
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_tail_subtract: Error writing DPRAM data: %s", old_message);
				return rc;
			}
		}
	}
	return 0;
}


int ngpd_read_tail_subtract(int path, int chan, int32_t *data)
{
	int rc;
	u_int32_t i;
	int j;
	int32_t unpacked[NGPD_MEASURE_TAIL_SUB_SIZE];

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_tail_subtract: Invalid path=%d", path);
		return -1;
	}

	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		i = 0;
		if ((rc=ngpd_write_chan_regs(path, chan, NGPD_REGION_DPRAM, NGPD_CHAN_DPADDR, 1, &i)) < 0)
		{
			char old_message[NGPD_MAX_ERROR_MESSAGE+2];
			strcpy(old_message, ngpd_error_message);
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_tail_subtract: Error writing DPRAM address registers: %s", old_message);
			return rc;
		}
		for (j=0; j<NGPD_MEASURE_TAIL_SUB_SIZE; j++)
		{
			if ((rc=ngpd_read_chan_regs(path, chan, NGPD_REGION_DPRAM, NGPD_CHAN_MEASURE_TAIL_SUB_EVEN, 2, (u_int32_t *)(data+2*j))) < 0)
			{
				char old_message[NGPD_MAX_ERROR_MESSAGE+2];
				strcpy(old_message, ngpd_error_message);
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_tail_subtract: Error reading DPRAM data: %s", old_message);
				return rc;
			}
		}
	}
	else
	{
		for (j=0;j<2; j++)
		{
			if ((rc=ngpd_read_chan_regs(path, chan, NGZMP_REGION_TAIL_SUB0+j, 0, NGPD_MEASURE_TAIL_SUB_SIZE, (u_int32_t *)unpacked)) < 0)
			{
				char old_message[NGPD_MAX_ERROR_MESSAGE+2];
				strcpy(old_message, ngpd_error_message);
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_tail_subtract: Error reading DPRAM data: %s", old_message);
				return rc;
			}
			for (i=0; i<NGPD_MEASURE_TAIL_SUB_SIZE; i++)
				data[2*i+j] = unpacked[i];
		}
	}
	return 0;
}

int ngpd_write_dae_pulse(int path, int chan, int stretch)
{
	u_int32_t dae_pulse;
	int rc;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_dae_pulse: Invalid path=%d", path);
		return -1;
	}
	if (stretch < 1 || stretch > 255)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_dae_pulse: Invalid stretch=%d, expected %d...%d", stretch, 1, 255);
		return -1;
	}
	dae_pulse = stretch;

	rc = ngpd_write_chan_regs(path, chan, NGPD_REGION_REGS, NGPD_CHAN_DAE_PULSE, 1, &dae_pulse);
	if (rc)
		return rc;
#if NGPD_CONF_ADQ14
	
	rc = -!ADQ_SetDirectionGPIOPort(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 0, 0xFFFF, 0);
	if (rc != 0)
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_dae_pulse: Error Setting GPIO data direction");
	return rc;
#else
	printf("This version compiled without ADQ14 support. Build with NGPD_CONF_ADQ14 set\n");
	return -1;
#endif
}

int ngpd_read_scope_status(int path, int card, u_int32_t *scope_status)
{
	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_scope_status: Invalid Path");
		return NGPD_ERROR;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
		return ngpd_read_glob_regs(path, card, NGPD_GLOB_SCOPE_STATUS, 1, scope_status);
	else
		return ngpd_read_glob_regs(path, card, NGZMP_SCOPE_STATUS, 1, scope_status);
}

