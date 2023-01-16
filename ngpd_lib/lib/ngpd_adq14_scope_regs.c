#if NGPD_CONF_ADQ14
#include "ADQAPI.h"
#endif
#include <stdio.h>
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



/**
 * @ingroup debug
 * Setup scope mode options.
 *
 * For debug purposes, the system provides the capability to stream multiple 16 bits streams to memory per card, recording raw ADC data or data from various points through the processing.
 * Usually the scope mode recording starts as soon as the run bit is asserted. However, to synchronise across cards, 
 * it is possible to make scope mode wait until the Veto (Count enb) signal is asserted.
 * This can be routed from board to board using to be determined

 * @param path a handle to the top level of the ngpd system returned from {@link ngpd_config()}.
 * @param card is the number of the card in the ngpd system,
 *        0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *        where this value has been passed to {@link ngpd_config()} at ngpd system configuration.
 *        If card is less than 0 then all cards are selected.
 * @param options Scope mode options defined by Xsp3ScopeOptions
 * @return 0 or a negative error code.
 */
int ngpd_set_scope_options(int path, int card, NGPDScopeOptions *options)
{
	int first_card, last_card;
	u_int32_t scope_cont;
	int layout;
	int rc;

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_scope_options: invalid path %d", path);
		return NGPD_ERROR;
	}

	if (NGPDPath[path].debug)
		printf("ngpd_set_scope_options: Called with Card %d, nstreams=%d\n", card, options->nstreams);
	if (card < 0)
	{
		first_card = 0;
		last_card = NGPDPath[path].num_cards - 1;
	} else {
		if (card >= NGPDPath[path].num_cards) 
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_scope_options: invalid card %d", card);
			return NGPD_ERROR;
		}
		first_card = card;
		last_card = card;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		if (NGPDPath[path].scope_mod != NULL)
			layout = NGPDPath[path].scope_mod->head.layout;
		for (card = first_card; card <= last_card; card++) 
		{
			if ((rc=ngpd_read_glob_regs(path, card, NGPD_SCOPE_CONT, 1, &scope_cont)) != 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_scope_options: Error Reading scope control register");
				return NGPD_ERROR;
			}
			if (options->nstreams > 0)
			{
				switch (options->nstreams)
				{
				case 1:
					scope_cont &= ~NGPD_SCOPE_CONT_NUM_STREAMS(0xFF);
					scope_cont |= NGPD_SCOPE_CONT_NUM_STREAMS(NGPD_SCOPE_NUM_STREAMS1);
					layout = NGPD_SCOPE_LAYOUT_1;
					break;

				case 2:
					scope_cont &= ~NGPD_SCOPE_CONT_NUM_STREAMS(0xFF);
					scope_cont |= NGPD_SCOPE_CONT_NUM_STREAMS(NGPD_SCOPE_NUM_STREAMS2);
					layout = NGPD_SCOPE_LAYOUT_2;
					break;

				case 4:
					scope_cont &= ~NGPD_SCOPE_CONT_NUM_STREAMS(0xFF);
					scope_cont |= NGPD_SCOPE_CONT_NUM_STREAMS(NGPD_SCOPE_NUM_STREAMS4);
					layout = NGPD_SCOPE_LAYOUT_4;
					break;

				case 6:
					scope_cont &= ~NGPD_SCOPE_CONT_NUM_STREAMS(0xFF);
					scope_cont |= NGPD_SCOPE_CONT_NUM_STREAMS(NGPD_SCOPE_NUM_STREAMS6);
					layout = NGPD_SCOPE_LAYOUT_6;
					break;

				default:
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_scope_options: Unsupported number of streams %d, currently 1, 2,3 ,4 anad 6 for ADQ14", options->nstreams);
					return NGPD_ERROR;
				}
			}
			if (NGPDPath[path].scope_mod != NULL)
				NGPDPath[path].scope_mod->head.layout = layout;
		}
		if (NGPDPath[path].scope_mod != NULL)
			ngpd_scope_calc_num_t(NGPDPath[path].scope_mod);
		if ((rc=ngpd_write_glob_regs(path, card, NGPD_SCOPE_CONT, 1, &scope_cont)) != 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_scope_options: Error Writing scope control register");
			return NGPD_ERROR;
		}
	}
	else
	{
		if (NGPDPath[path].scope_mod != NULL)
			layout = NGPDPath[path].scope_mod->head.layout;
		for (card = first_card; card <= last_card; card++) 
		{
			if ((rc=ngpd_read_glob_regs(path, card, NGZMP_SCOPE_CONT, 1, &scope_cont)) != 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_scope_options: Error Reading scope control register");
				return NGPD_ERROR;
			}
			if (options->nstreams > 0)
			{
				switch (options->nstreams)
				{
				case 4:
					scope_cont &= ~NGZMP_SCOPE_CONT_NUM_STREAMS(0xFF);
					scope_cont |= NGZMP_SCOPE_CONT_NUM_STREAMS(0);
					layout = NGZMP_SCOPE_LAYOUT_4;
					break;

				case 8:
					scope_cont &= ~NGZMP_SCOPE_CONT_NUM_STREAMS(0xFF);
					scope_cont |= NGZMP_SCOPE_CONT_NUM_STREAMS(1);
					layout = NGZMP_SCOPE_LAYOUT_8;
					break;

				case 10:
					scope_cont &= ~NGZMP_SCOPE_CONT_NUM_STREAMS(0xFF);
					scope_cont |= NGZMP_SCOPE_CONT_NUM_STREAMS(2);
					layout = NGZMP_SCOPE_LAYOUT_10;
					break;

				default:
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_scope_options: Unsupported number of streams %d, currently 4, 8 and 10 for ZynqMP", options->nstreams);
					return NGPD_ERROR;
				}
			}
			if ((rc=ngpd_write_glob_regs(path, card, NGZMP_SCOPE_CONT, 1, &scope_cont)) != 0)
			{
				char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
				strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_scope_options: Error Writing scope control register rc=%d: %s", rc, old_msg);
				ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
				return rc;
			}
		}
		if (NGPDPath[path].scope_mod != NULL)
		{
			NGPDPath[path].scope_mod->head.layout = layout;
			ngpd_scope_calc_num_t(NGPDPath[path].scope_mod);
		}
	}
	return 0;
}

/**
 * @ingroup fullcontrol
 * Setup a single stream within the scope mode data muxes.
 *
 *
 * @param path 				A handle to the top level of the NGPD system returned from {@link ngpd_config()}.
 * @param scope_out_skip	Number of stall states, 1 => continuous data, 2=> every other cycle, up to {@link NGPD_SCOPEOUT_SKIP_MAX}
 * @return 0 on success or a negative error code.
 */

int ngpd_scope_setup_stream(int path, int card, int stream, int chan_sel, int src_sel, int alt)
{
	NGPDScopeModule *mod;
	u_int32_t regs[6];
	int rc;
	int first_card, last_card;
	int i;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_setup_stream: Invalid path=%d", path);
		return -1;
	}

	if (stream < 0 || stream >= NGPDPath[path].max_scope_streams)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_setup_stream: Invalid stream=%d, maximum=%d", stream, NGPDPath[path].max_scope_streams-1);
		return -1;
	}
	if (card < 0)
	{
		first_card = 0;
		last_card = NGPDPath[path].num_cards-1;
	}
	else if (card >=NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_setup_stream: card %d is out of range 0...%d", card, NGPDPath[path].num_cards-1);
		return -1;
	}
	else
	{
		first_card = card;
		last_card = card;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		if ((rc=ngpd_read_glob_regs(path, card, NGPD_SCOPE_CHAN_SEL, 3, regs)) != 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_setup_stream: Error Reading scope registers");
			return -1;
		}
		regs[0] &= ~NGPD_SCOPE_CHAN_SEL_SET(stream, 255);
		regs[0] |= NGPD_SCOPE_CHAN_SEL_SET(stream, chan_sel);
		regs[1] &= ~NGPD_SCOPE_SRC_SEL_SET(stream, 255);
		regs[1] |= NGPD_SCOPE_SRC_SEL_SET(stream, src_sel);
		regs[2] &= ~NGPD_SCOPE_ALTERNATE_SET(stream, 255);
		regs[2] |= NGPD_SCOPE_ALTERNATE_SET(stream, alt);

		mod = NGPDPath[path].scope_mod;
		if (mod != NULL)
		{
			mod->head.card[0].chan_sel[0] = regs[0];
			mod->head.card[0].src_sel[0] = regs[1];
			mod->head.card[0].alternate[0] = regs[2];
		}
	//	printf("ngpd_scope_setup_stream: Setting scope regs: chan_sel = %08X, src_sel=%08X, alt=%08X\n", regs[0], regs[1], regs[2]);
		if ((rc=ngpd_write_glob_regs(path, card, NGPD_SCOPE_CHAN_SEL, 3, regs)) != 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_setup_stream: Error Writing scope registers");
			return -1;
		}
	}
	else
	{
		for (card=first_card; card<=last_card; card++)
		{
			if ((rc=ngpd_read_glob_regs(path, card, NGZMP_SCOPE_CHAN_SEL0, 6, regs)) != 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_setup_stream: Error Reading scope registers");
				return -1;
			}
			i = stream/8;
			regs[0+i] &= ~NGZMP_SCOPE_CHAN_SEL_SET(stream, 255);
			regs[0+i] |=  NGZMP_SCOPE_CHAN_SEL_SET(stream, chan_sel);
			regs[2+i] &= ~NGZMP_SCOPE_SRC_SEL_SET(stream, 255);
			regs[2+i] |=  NGZMP_SCOPE_SRC_SEL_SET(stream, src_sel);
			regs[4+i] &= ~NGZMP_SCOPE_ALTERNATE_SET(stream, 255);
			regs[4+i] |=  NGZMP_SCOPE_ALTERNATE_SET(stream, alt);
			if ((rc=ngpd_write_glob_regs(path, card, NGZMP_SCOPE_CHAN_SEL0, 6, regs)) != 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_scope_setup_stream: Error writing scope registers rc=%d", rc);
				return -1;
			}
			mod = NGPDPath[path].scope_mod;
			if (mod != NULL)
			{
				for (i=0; i<2; i++)
				{
					mod->head.card[card].chan_sel[i] = regs[0+i];
					mod->head.card[0].src_sel[i] = regs[2+i];
					mod->head.card[0].alternate[i] = regs[4+i];
				}
			}
		}
	}
	
	return 0;
}

