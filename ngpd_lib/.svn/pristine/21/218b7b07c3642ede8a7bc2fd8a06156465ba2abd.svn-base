/*
 * ngzmp_hist.c
 *
 *  Created on: 5/8/2021
 *      Author: wih73
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <unistd.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>

#include "ngpd.h"
#include "ngzmp_dma_protocol.h"
#include "zynqmp_protocol.h"
#include "zynqmp_api.h"

extern char ngpd_error_message[];


int ngzmp_hist_read_reg(int path, int card, int offset, int num_regs, u_int32_t *data)
{
	int rc;
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_reg: Invalid path=%d", path);
		return -1;
	}

	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_reg: Not supported on ADQ14");
		return -1;
	}
	else
	{
		int this_path;
		if (card < 0 || card >= NGPDPath[path].num_cards)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_reg: Card=%d out of range 0...%d", card, NGPDPath[path].num_cards-1);
			return -1;
		}
		if (NGPDPath[path].type == NGPDComposite)
		{
			this_path = NGPDPath[path].sub_path[card];
			if (this_path <0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_reg: Card=%d resolved illegal sub_path=%d", card, this_path);
				return -1;
			}
		}
		else
		{
			this_path = path;
		}
		if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
			return 0;
		rc = zynqmp_access_read(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_HIST, ZYNQMP_WIDTH_LONG, offset, num_regs, 1, (u_int8_t *)data);
		if (rc < 0)
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_reg: %s : error=%d", zynqmp_get_error_message(), rc);
		return rc;
	}
}

int ngzmp_hist_write_reg(int path, int card, int offset, int num_regs, u_int32_t *data)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_write_reg: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_write_reg: Not supported on ADQ14");
		return -1;
	}
	else
	{
		int this_path;
		int rc;
		if (card < 0)
		{
			for (card=0; card<NGPDPath[path].num_cards; card++)
			{
				if ((rc = ngzmp_hist_write_reg(path, card, offset, num_regs, data)) < 0)
					return rc;
			}
			return 0;
		}
		else if (card >= NGPDPath[path].num_cards)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_write_reg: Card=%d out of range 0...%d", card, NGPDPath[path].num_cards-1);
			return -1;
		}
		if (NGPDPath[path].type == NGPDComposite)
		{
			this_path = NGPDPath[path].sub_path[card];
			if (this_path <0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_write_reg: Card=%d resolved illegal sub_path=%d", card, this_path);
				return -1;
			}
		}
		else
		{
			this_path = path;
		}
		if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
			return 0;
		rc = zynqmp_access_write(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_HIST, ZYNQMP_WIDTH_LONG, offset, num_regs, 1, (u_int8_t *)data);
		if (rc < 0 )
		{
			strncpy(ngpd_error_message, zynqmp_get_error_message(), NGPD_MAX_ERROR_MESSAGE);
			ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
		}
		return rc;
	}
}

/**
 * @ingroup full_control
 * Setup diagnostig histogrammer..
 *
 * @param path a handle to the top level of the ngpd system returned from {@link ngpd_config_ngzmp()}.
 * @param chan 	Channel number with system or -1 to copy to all channels
 * @param hist_conf	Pointer to histogram structure.
 * @return {@link NGPD_OK} or a negative error code.
 */
int ngzmp_hist_write_chan_config(int path, int chan, NGZMPHistConf *hist_conf)
{
	int rc = 0;
	u_int32_t hc[2];

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_write_chan_config: Invalid Path");
		return NGPD_ERROR;
	}

	hc[0] = 0;
	hc[1] = 0;
	if (hist_conf->separate_ngp) hc[0] |= NGZMP_HCH_SEPARATE_NGP;
	if (hist_conf->enb_hgt_sum)  hc[0] |= NGZMP_HCH_ENB_HGT_SUM;
	if (hist_conf->enb_hgt_fall) hc[0] |= NGZMP_HCH_ENB_HGT_FALL;
	if (hist_conf->discard_pu)   hc[0] |= NGZMP_HCH_DISCARD_PU;

	hc[0] |= NGZMP_HCH_NBITS_HGT_SET(hist_conf->nbits_height);
	hc[0] |= NGZMP_HCH_HGT_SHIFT_SET(hist_conf->shift_height);

	hc[1] |= NGZMP_HCT_NBITS_FALL_SET(hist_conf->nbits_fall_time);
	hc[1] |= NGZMP_HCT_FALL_SHIFT_SET(hist_conf->shift_fall_time);
	hc[1] |= NGZMP_HCT_NBITS_TSUM_SET(hist_conf->nbits_tail_sum);
	hc[1] |= NGZMP_HCT_TSUM_SHIFT_SET(hist_conf->shift_tail_sum);

	rc = ngpd_write_chan_regs(path, chan, NGZMP_REGION_REGS, NGZMP_CHAN_HIST_CONT_HGT, 2, hc);
	return rc;
}

/**
 * @ingroup full_control
 * retrieve setup from diagnostig histogrammer..
 *
 * @param path a handle to the top level of the ngpd system returned from {@link ngpd_config_ngzmp()}.
 * @param chan 	Channel number with system 
 * @param hist_conf	Pointer to histogram structure.
 * @return {@link NGPD_OK} or a negative error code.
 */
int ngzmp_hist_read_chan_config(int path, int chan, NGZMPHistConf *hist_conf)
{
	int rc = 0;
	u_int32_t hc[2];

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_chan_config: Invalid Path");
		return NGPD_ERROR;
	}

	rc = ngpd_read_chan_regs(path, chan, NGZMP_REGION_REGS, NGZMP_CHAN_HIST_CONT_HGT, 2, hc);
	if (rc < 0)
	{
		char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
		strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
		old_msg[NGPD_MAX_ERROR_MESSAGE] = 0;
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_chan_config: Cannot read hist config registers : %s", old_msg);
		return rc;
	}
	memset(hist_conf, 0, sizeof(NGZMPHistConf));

	hist_conf->separate_ngp = !!(hc[0] & NGZMP_HCH_SEPARATE_NGP);
	hist_conf->enb_hgt_sum  = !!(hc[0] & NGZMP_HCH_ENB_HGT_SUM);
	hist_conf->enb_hgt_fall = !!(hc[0] & NGZMP_HCH_ENB_HGT_FALL);
	hist_conf->discard_pu   = !!(hc[0] & NGZMP_HCH_DISCARD_PU);

	hist_conf->nbits_height = NGZMP_HCH_NBITS_HGT_GET(hc[0]);
	hist_conf->shift_height = NGZMP_HCH_HGT_SHIFT_GET(hc[0]);

	hist_conf->nbits_fall_time = NGZMP_HCT_NBITS_FALL_GET(hc[1]);
	hist_conf->shift_fall_time = NGZMP_HCT_FALL_SHIFT_GET(hc[1]);
	hist_conf->nbits_tail_sum  = NGZMP_HCT_NBITS_TSUM_GET(hc[1]);
	hist_conf->shift_tail_sum  = NGZMP_HCT_TSUM_SHIFT_GET(hc[1]);

	return 0;
}

int ngzmp_hist_read_config(int path, int card, AXIHistConfig * config)
{
	int rc = 0;
	u_int32_t regs[4];
	int this_path;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_config: Invalid path=%d", path);
		return -1;
	}

	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_config: Not supported on ADQ14");
		return -1;
	}
	if (card < 0)
	{
		for (card=0; card<NGPDPath[path].num_cards; card++)
		{
			rc = ngzmp_hist_read_config(path, card, config);
			if (rc < 0)
				return rc;
		}
		return 0;
	}
	else if (card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_config: Card=%d out of range 0...%d", card, NGPDPath[path].num_cards-1);
		return -1;
	}
	if (NGPDPath[path].type == NGPDComposite)
	{
		this_path = NGPDPath[path].sub_path[card];
		if (this_path <0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_config: Card=%d resolved illegal sub_path=%d", card, this_path);
			return -1;
		}
	}
	else
	{
		this_path = path;
	}

	rc = ngzmp_hist_read_reg(path, card, AXI_HIST_IDENT_REG, 4, regs);
	if (rc < 0 )
	{
		char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
		strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
		old_msg[NGPD_MAX_ERROR_MESSAGE] = 0;
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_config: Cannot read hist config registers : %s", old_msg);
		return rc;
	}
	if (regs[0] != AXI_HIST_IDENTIFIER)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_config: Error reading identifier register: read 0x%08X, expedted 0x%08X\n", regs[0], AXI_HIST_IDENTIFIER);
		return -1;
	}

	NGPDPath[this_path].hist_config.NBitsDataHist	= 1 << AXI_HIST_CG0_NBITS_DATA(regs[1]);
	NGPDPath[this_path].hist_config.NBitsDataAXI	= 1 << AXI_HIST_CG0_NBITS_DATA_AXI(regs[1]);
	NGPDPath[this_path].hist_config.NumStreams		= AXI_HIST_CG0_NUM_STREAMS(regs[1]);
	NGPDPath[this_path].hist_config.Major 			= AXI_HIST_CG0_MAJOR(regs[1]);
	NGPDPath[this_path].hist_config.Minor 			= AXI_HIST_CG0_MINOR(regs[1]);
	NGPDPath[this_path].hist_config.NBitsAddrTopFixed = AXI_HIST_CG1_NBITS_ADDR_TOP(regs[2]);
	NGPDPath[this_path].hist_config.NBitsStream		= AXI_HIST_CG1_NBITS_STREAM(regs[2]);
	NGPDPath[this_path].hist_config.NBitsBankAndGroup= AXI_HIST_CG1_GET_NBITS_BANKANDGROUP(regs[2]);
	NGPDPath[this_path].hist_config.NBitsAddrMem	= AXI_HIST_CG1_GET_NBITS_ADDR(regs[2]);
	NGPDPath[this_path].hist_config.NBitsAddrIn		= AXI_HIST_CG1_GET_NBITS_ADDR_IN(regs[2]);
	NGPDPath[this_path].hist_config.StreamAtBottom  = AXI_HIST_CG1_GET_STREAM_AT_BOTTOM(regs[2]);
	NGPDPath[this_path].hist_config.HistWordsPerAXIWord = NGPDPath[this_path].hist_config.NBitsDataAXI/NGPDPath[this_path].hist_config.NBitsDataHist;
	NGPDPath[this_path].hist_config.TotalMemWords 	= (((u_int64_t)1)<<NGPDPath[this_path].hist_config.NBitsAddrMem)/(NGPDPath[this_path].hist_config.NBitsDataHist/8);
	NGPDPath[this_path].hist_config.HistWordsPerStream = NGPDPath[this_path].hist_config.TotalMemWords/NGPDPath[this_path].hist_config.NumStreams;
	NGPDPath[this_path].hist_config.AXIWordsPerStream = NGPDPath[this_path].hist_config.HistWordsPerStream/NGPDPath[this_path].hist_config.HistWordsPerAXIWord;
	NGPDPath[this_path].hist_config.NBitsSubStream		= AXI_HIST_CG2_GET_NBITS_SUBSTREAM(regs[3]);
	NGPDPath[this_path].hist_config.HistTPGPresent		= AXI_HIST_CG2_GET_TPG_PRESENT(regs[3]);
	NGPDPath[this_path].hist_config.Cache				= AXI_HIST_CG2_GET_CACHE(regs[3]);
	NGPDPath[this_path].hist_config.ReadWaitWrite		= AXI_HIST_CG2_GET_READWAITWRITE(regs[3]);

	if (path != this_path)
		NGPDPath[path].hist_config = NGPDPath[this_path].hist_config;
	NGPDPath[this_path].hist_config_read = 1;

	if (config != NULL)
		*config = NGPDPath[this_path].hist_config;

	return 0;
}

/**
	Clear region of histogram memory in a single stream (generally use all stream version)

@param path			Path to ngzmp device including access to histogrammer registers.
@param card			Unused on ZynqMP, allow consistent API with x86_64 version
@param stream		Stream Number 0.. NumStreams-1
@param start_word	First word to clear, measured within streams s aaddress psace, in AXI words.
@param num_words	Number of AXI word to clerar within stream.
@return   			0 on success or -1 on error
*/
int ngzmp_hist_clear_stream_start(int path, int card, int stream, int start_word, int num_words)
{
	u_int32_t cont_start_num[3];
	int first_card, last_card, this_path;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_stream_start: Invalid path=%d", path);
		return -1;
	}

	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_stream_start: Not supported on ADQ14");
		return -1;
	}

	if (card < 0)
	{
		first_card = 0;
		last_card = NGPDPath[path].num_cards-1;
	}
	else if (card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_stream_start: Card=%d out of range 0...%d", card, NGPDPath[path].num_cards-1);
		return -1;
	}
	else
		first_card = last_card = card;

	for (card=first_card; card<=last_card; card++)
	{
		if (NGPDPath[path].type == NGPDComposite)
		{
			this_path = NGPDPath[path].sub_path[card];
			if (this_path <0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_stream_start: Card=%d resolved illegal sub_path=%d", card, this_path);
				return -1;
			}
		}
		else
		{
			this_path = path;
		}
		if (!NGPDPath[this_path].hist_config_read)
		{
			ngzmp_hist_read_config(path, card, NULL);
		}
		if (NGPDPath[this_path].hist_config.StreamAtBottom)
		{
			start_word *= NGPDPath[this_path].hist_config.NumStreams;
			start_word += stream;
		}
		else
		{
			start_word += stream * NGPDPath[this_path].hist_config.AXIWordsPerStream;
		}
		cont_start_num[0] = 0;

		cont_start_num[1] = start_word;
		cont_start_num[2] = num_words;
		if (ngzmp_hist_write_reg(path, card, AXI_HIST_CLEAR_CONT, 3, cont_start_num))
		{
			char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
			strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
			old_msg[NGPD_MAX_ERROR_MESSAGE] = 0;
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_start: Error writing clear control registers : %s", old_msg);
			return -1;
		}
	}
	return 0;
}
/**
	Start clear region of histogram memory all streams.

@param path			Path to ngzmp device including access to histogrammer registers.
@param card			Unused on ZynqMP, allow consistent API with x86_64 version
@param start_word	First word to clear, measured within streams  address space, in AXI words.
@param num_words	Number of AXI word to clerar within stream. Setting start=0, num=0 causes full memory to be cleared.
@return   			0 on success or -1 on error
*/

int ngzmp_hist_clear_all_start(int path, int card, int start_word, int num_words)
{
	u_int32_t cont_start_num[3];
	int first_card, last_card, this_path;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_all_start: Invalid path=%d", path);
		return -1;
	}

	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_all_start: Not supported on ADQ14");
		return -1;
	}

	if (card < 0)
	{
		first_card = 0;
		last_card = NGPDPath[path].num_cards-1;
	}
	else if (card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_all_start: Card=%d out of range 0...%d", card, NGPDPath[path].num_cards-1);
		return -1;
	}
	else
		first_card = last_card = card;

	for (card=first_card; card<=last_card; card++)
	{
		if (NGPDPath[path].type == NGPDComposite)
		{
			this_path = NGPDPath[path].sub_path[card];
			if (this_path <0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_all_start: Card=%d resolved illegal sub_path=%d", card, this_path);
				return -1;
			}
		}
		else
		{
			this_path = path;
		}
		if (!NGPDPath[this_path].hist_config_read)
		{
			ngzmp_hist_read_config(path, card, NULL);
		}
		if (start_word == 0 && num_words == 0)
			//	num_words = NGPDPath[path].hist_config.AXIWordsPerStream;  For NGZMP only part of the memory is used for histogram, the rest is for playback data
			num_words = NGZMP_TOTAL_HIST_SIZE_BYTES/((NGPDPath[this_path].hist_config.NBitsDataAXI/8)*NGPDPath[this_path].hist_config.NumStreams);
		if (NGPDPath[this_path].hist_config.StreamAtBottom)
		{
			start_word *= NGPDPath[this_path].hist_config.NumStreams;
			num_words *= NGPDPath[this_path].hist_config.NumStreams;
		}
		cont_start_num[0] = AXI_HIST_CC_CLEAR_ALL;

		cont_start_num[1] = start_word;
		cont_start_num[2] = num_words;
		if (ngzmp_hist_write_reg(path, card, AXI_HIST_CLEAR_CONT, 3, cont_start_num))
		{
			char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
			strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
			old_msg[NGPD_MAX_ERROR_MESSAGE] = 0;
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_all_start: Error writing clear control registers : %s", old_msg);
			return -1;
		}
	}
	return 0;
}

int ngzmp_hist_clear_start(int path, int card, int start_word, int num_words, int all_streams)
{
	u_int32_t cont_start_num[3];
	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_start: Invalid path=%d", path);
		return -1;
	}

	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_start: Not supported on ADQ14");
		return -1;
	}

	cont_start_num[0] = 0;
	if (all_streams)
		cont_start_num[0] |= AXI_HIST_CC_CLEAR_ALL;

	cont_start_num[1] = start_word;
	cont_start_num[2] = num_words;
	if (ngzmp_hist_write_reg(path, card, AXI_HIST_CLEAR_CONT, 3, cont_start_num))
	{
		char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
		strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
		old_msg[NGPD_MAX_ERROR_MESSAGE] = 0;
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_start: Error writing clear control registers : %s", old_msg);
		return -1;
	}
	return 0;
}

int ngzmp_hist_clear_start_tpgen(int path, int card, int start_word, int num_words, int all_streams)
{
	u_int32_t cont_start_num[3];
	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_start_tpgen: Invalid path=%d", path);
		return -1;
	}

	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_start_tpgen: Not supported on ADQ14");
		return -1;
	}

	cont_start_num[0] = AXI_HIST_CC_USE_TPG;
	if (all_streams)
		cont_start_num[0] |= AXI_HIST_CC_CLEAR_ALL;

	cont_start_num[1] = start_word;
	cont_start_num[2] = num_words;
	if (ngzmp_hist_write_reg(path, card, AXI_HIST_CLEAR_CONT, 3, cont_start_num))
	{
		char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
		strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
		old_msg[NGPD_MAX_ERROR_MESSAGE] = 0;
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_start_tpgen: Error writing clear control registers : %s", old_msg);
		return -1;
	}
	return 0;
}


int ngzmp_hist_clear_wait(int path, int card)
{
	struct timespec delay, remaining;
	u_int32_t status;
	int polls=0;
	int first_card, last_card;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_wait: Invalid path=%d", path);
		return -1;
	}

	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_wait: Not supported on ADQ14");
		return -1;
	}

	delay.tv_sec = 0;
	delay.tv_nsec= 1E6;

	if (card < 0)
	{
		first_card = 0;
		last_card = NGPDPath[path].num_cards-1;
	}
	else if (card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_wait: Card=%d out of range 0...%d", card, NGPDPath[path].num_cards-1);
		return -1;
	}
	else
		first_card = last_card = card;

	for (card=first_card; card<=last_card; card++)
	{
		do
		{
			if (ngzmp_hist_read_reg(path, card, AXI_HIST_CLEAR_BUSY, 1, &status) < 0)
			{
				char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
				strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
				old_msg[NGPD_MAX_ERROR_MESSAGE] = 0;
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_all_start: Error writing clear control registers : %s", old_msg);
				return -1;
			}
			if (status == 0)
				break;
#ifdef _MSC_VER
			Sleep(1);
#else
			remaining = delay;
			while (nanosleep(&remaining, &remaining) == EINTR);
#endif
			polls++;
			if (polls > 2000)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_clear_wait: Timeout waiting for clear to finish on card %d", card);
				return -1;
			}
		} while (1);
	}
	return 0;
}

int ngzmp_hist_read_chan(int path, int chan, u_int32_t offset, u_int32_t size, u_int32_t *data)
{
	int rc;
	int this_path, this_card, chan_of_card;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_chan: Invalid path=%d", path);
		return -1;
	}

	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_chan: Not supported on ADQ14");
		return -1;
	}
	if ((rc=ngpd_resolve_path_chan_card(path, chan, &this_path, &chan_of_card, &this_card)) < 0)
	{
		char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
		strcpy(old_msg, ngpd_error_message);
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_chan: Invalid channel : %s", old_msg);
		ngpd_error_message[NGPD_MAX_ERROR_MESSAGE]=0;
		return rc;
	}
	return ngzmp_hist_read_stream(path, this_card, chan_of_card, offset, size, data);
}

int ngzmp_hist_read_stream(int path, int card, int stream, u_int32_t offset, u_int32_t size, u_int32_t *data)
{
	int rc;
	int this_path;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_stream: Invalid path=%d", path);
		return -1;
	}

	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_stream: Not supported on ADQ14");
		return -1;
	}
	if (card < 0 || card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_stream: Card=%d out of range 0...%d", card, NGPDPath[path].num_cards-1);
		return -1;
	}
	if (NGPDPath[path].type == NGPDComposite)
	{
		this_path = NGPDPath[path].sub_path[card];
		if (this_path <0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_stream: Card=%d resolved illegal sub_path=%d", card, this_path);
			return -1;
		}
	}
	else
	{
		this_path = path;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
		return 0;
	if (!NGPDPath[this_path].hist_config_read)
		ngzmp_hist_read_config(path, card, NULL);

	if (offset+size > NGPDPath[this_path].hist_config.HistWordsPerStream)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_stream: Region %u for %u does not fit in buffer %lu words", offset, size, NGPDPath[this_path].hist_config.HistWordsPerStream);
		return -1;
	}


	if (NGPDPath[this_path].hist_config.StreamAtBottom)
	{
		rc = zynqmp_access_read_intl(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_PL_RAM, ZYNQMP_WIDTH_LONG, offset, size, NGPDPath[this_path].hist_config.HistWordsPerAXIWord, 
				NGPDPath[this_path].hist_config.NumStreams, stream, (u_int8_t *) data);

	}
	else
	{
		offset += stream * NGPDPath[this_path].hist_config.HistWordsPerStream;
		rc = zynqmp_access_read(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_PL_RAM, ZYNQMP_WIDTH_LONG, offset, size, 1, (u_int8_t *)data);
	}
	if (rc < 0)
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_hist_read_stream: %s : error=%d", zynqmp_get_error_message(), rc);
	return rc;
}

