/* 	
	ngpd_cal_setup.c
	ngpd calibration library
	Created: wih73   28/10/2021
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "datamod.h"
#include "ngpd.h"
#include "ngpd_cal.h"

extern char ngpd_cal_error_message[NGPD_MAX_ERROR_MESSAGE+2];


/**
@ingroup calibration
	Returns an error message describing the last error detected by the ngpd_cal functions.
	@return A statically allocated 0 terminated error message string.
*/
char * ngpd_cal_get_error_message()
{
	return ngpd_cal_error_message;
}
/** 
@defgroup cal_setup		Functions to setup scope mode multiplexer to capture required data for auto calibration
@ingroup calibration
These functions aim to work for both the ADQ14 and ZynqMP generations. The idea would be to extend them to handle any future generations or major variant within a generation.
They are used within the auto-calibration code, but are available to use code wanting detailed debug control.
*/
/**
@addtogroup cal_setup
@{
*/
/**
 * Setup scope mode to capture analogue signal only from all channels.
 * This is supported for both the ADQ14 and ZynqMP generations.
 * @param path 			A handle to the top level of the ngpd system returned from any of the config functions  {@link ngpd_config_adq14} or {@link ngpd_config_ngzmp()}.
 * @param card 			Card number 0..num_cards-1 or -1 to setup all cards in system.
 * @param sync_boxes	Synchronise acquisition across all cards when firmware supports this. For future expansion.
 * @param src			Data source as described by {@link NGZMP_SCOPE_SEL_MACROS}
 * @return		Returns 0 on success or -ve on error with error message available from ngpd_cal_get_error_message()
*/
int ngpd_cal_setup_all_ana(int path,  int card, int sync_boxes, int src)
{
	NGPDGeneration generation = ngpd_get_generation(path);
	int stream;
	int rc;
	NGPDScopeOptions scope_options;
	NGPDScopeModule * scope_mod;

	scope_mod = ngpd_scope_get_mod(path);
	if (scope_mod == NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_ana: Cannot determine scope module pointer for path %d : %s", path, ngpd_get_error_message());
		return -1;
	}

	if (generation == NGPDGenADQ14)
	{
		int chans_per_card = ngpd_get_chans_per_card(path);

		scope_options.nstreams = chans_per_card;

#if 0
		if (sync_boxes)
			scope_options.flags = NGPDScopeOpt_Synchronised;	// Turn on delayed start mode, to try to sync boxes.
		else
			scope_options.flags = 0; 	// Turn off delayed start mode, not required.
#endif
		rc = ngpd_set_scope_options(path, -1, &scope_options);
		if (rc < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_ana: Error setting scope options nstreams= %d: %s\n", scope_options.nstreams, ngpd_get_error_message());
			return -1;
		}

		for (stream=0; stream<chans_per_card; stream++)
		{
			if ((rc=ngpd_scope_setup_stream(path, card, stream, stream, src, 0)) < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_ana: Error setting scope stream %d: %s\n", stream, ngpd_get_error_message());
				return -1;
			}
		}
	}
	else if (generation == NGPDGenZynqMP1)
	{
		int chans_per_card = ngpd_get_chans_per_card(path);

		scope_options.nstreams = 8;

#if 0
		if (sync_boxes)
			scope_options.flags = NGPDScopeOpt_Synchronised;	// Turn on delayed start mode, to try to sync boxes.
		else
			scope_options.flags = 0; 	// Turn off delayed start mode, not required.
#endif
		rc = ngpd_set_scope_options(path, -1, &scope_options);
		if (rc < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_ana: Error setting scope options nstreams= %d: %s\n", scope_options.nstreams, ngpd_get_error_message());
			return -1;
		}
		for (stream=0; stream<8; stream++)
		{
			int chan = stream;
			chan %= chans_per_card;
			if ((rc=ngpd_scope_setup_stream(path, card, stream, chan, src, 0)) < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_ana: Error setting scope stream %d: %s", stream, ngpd_get_error_message());
				return -1;
			}
		}
	}
	else
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_ana: Error Unknown NGPD generation %d\n", generation);
		return -1;
	}
	return 0;
}
/**
 * Setup scope mode to capture analogue signal and digital signals from trigger output or measure block.
 * This is supported for both the ADQ14 and ZynqMP generations.
 * For the ZynqMP 8 channel, limitation in memory bandwidth mean that in 10 stream scope mode with all 8 analogue stream only 2 digital bits can be recorded per stream. 
 * Since 3 digital signals are required per stream, scope mode must be used in 8 stream mode to capture 6 analogue streams and 2 of digital streams, 
 * recording 5 digital bits per analogue stream.
 * Hence to cover all the channels, the calibration has to be done in two chunks, channels 0..3 and then we 4..7
 * The need to work in 2 chunks is reported in need_upper.
 * @param path 			A handle to the top level of the ngpd system returned from any of the config functions  {@link ngpd_config_adq14} or {@link ngpd_config_ngzmp()}.
 * @param card 			Card number 0..num_cards-1 or -1 to setup all cards in system.
 * @param use_measure	Code to use trigger or measure data as {@link NGPD_CAL_USE_MEASURE_MACROS}
 * @param for_xtk		If set, setup for measuring XTK, reading all 8 channels, but accepting only 4 bits of digital
 * @param do_upper		Set to 0 for first set of channels. Call again with do_upper=1 to remaining channels if *need_uppers reports this is necessary
 * @param first_stream	Pointer to report the first stream to be scanned data. Determine if needed and associated channel from ngpd_cal_setup_trig_or_measure_get_chan()
 * @param last_stream	Pointer to report the last stream to be scanned data. Determine if needed and associated channel from ngpd_cal_setup_trig_or_measure_get_chan()
 * @param trig_offset	Pointer to return bit position within digital bits for the trigger signal. To be added to base position from ngpd_scope_stream_details()
 * @param wide_a_offset	Pointer to return bit position within digital bits for the wide_a signal, or -1 if not available
 * @param wide_b_offset	Pointer to return bit position within digital bits for the wide_b signal, or -1 if not available
 * @param neutron_offset Pointer to return bit position within digital bits for the neutron signal (from measure) or -1 when not available.
 * @param sync_boxes	Synchronise acquisition across all cards when firmware supports this. For future expansion.
 * @param need_upper	Pointer to return 1 when the channels must be read in 2 sets, by calling again with do_upper=1.
 * @return		Returns 0 on success or -ve on error with error message available from ngpd_cal_get_error_message()
*/

int ngpd_cal_setup_trig_or_measure(int path,  int card, int use_measure, int for_xtk, int do_upper, int *first_stream, int *last_stream,  int *trig_offset, int *wide_a_offset, int *wide_b_offset, int *neutron_offset, int sync_boxes, int *need_upper)
{
	NGPDGeneration generation = ngpd_get_generation(path);
	int stream;
	u_int32_t scope_src;
	u_int32_t scope_chan;
	u_int32_t scope_alt;
	int rc;
	NGPDScopeOptions scope_options;
	NGPDScopeModule * scope_mod;

	scope_mod = ngpd_scope_get_mod(path);
	if (scope_mod == NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_trig_or_measure: Cannot determine scope module pointer for path %d : %s\n", path, ngpd_get_error_message());
		return -1;
	}

	if (generation == NGPDGenADQ14)
	{
		int chans_per_card = ngpd_get_chans_per_card(path);

		if (chans_per_card == 1)
			scope_options.nstreams = 2;
		else
			scope_options.nstreams = 3;

#if 0
		if (sync_boxes)
			scope_options.flags = NGPDScopeOpt_Synchronised;	// Turn on delayed start mode, to try to sync boxes.
		else
			scope_options.flags = 0; 	// Turn off delayed start mode, not required.
#endif
		rc = ngpd_set_scope_options(path, -1, &scope_options);
		if (rc < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_trig_or_measure: Error setting scope options nstreams= %d: %s\n", scope_options.nstreams, ngpd_get_error_message());
			return -1;
		}

		for (stream=0; stream<chans_per_card; stream++)
		{
			scope_src = use_measure?NGPD_SCOPE_SEL0TO5_MEASURE:NGPD_SCOPE_SEL0TO5_DIFF_TRIG_OUT;
			scope_alt = (use_measure==NGPD_CAL_USE_MEASURE_CORRECTED)?5:0;
			if ((rc=ngpd_scope_setup_stream(path, card, stream, stream, scope_src, scope_alt)) < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_trig_or_measure: Error setting scope stream %d: %s\n", stream, ngpd_get_error_message());
				return -1;
			}
		}
		if ((rc=ngpd_scope_setup_stream(path, card, scope_options.nstreams-1, 0, NGPD_SCOPE_SEL5_DIG_5BIT, 0)) < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_trig_or_measure:  Error setting scope stream %d: %s\n", 5, ngpd_get_error_message());
			return -1;
		}
		*first_stream = 0;
		*last_stream = chans_per_card-1;
		if (use_measure)
		{
			*trig_offset = 0;
			*wide_a_offset = -1;
			*wide_b_offset = -1;
			*neutron_offset = -1;
		}
		else
		{
			*trig_offset = 2;
			*wide_a_offset = 3;
			*wide_b_offset = 4;
			*neutron_offset = -1;
		}
		if (need_upper != NULL)
			*need_upper = 0;
	}
	else if (generation == NGPDGenZynqMP1)
	{
		int chans_per_card = ngpd_get_chans_per_card(path);
		int dma;
		if (!for_xtk && 0)		/* With updated FW should never need to do_upper */
		{
			if (need_upper != NULL)
			{
				if (chans_per_card > 6)
					*need_upper = 1;
				else
					*need_upper = 0;
			}
			scope_options.nstreams = 8;

#if 0
			if (sync_boxes)
				scope_options.flags = NGPDScopeOpt_Synchronised;	// Turn on delayed start mode, to try to sync boxes.
			else
				scope_options.flags = 0; 	// Turn off delayed start mode, not required.
#endif
			rc = ngpd_set_scope_options(path, -1, &scope_options);
			if (rc < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_trig_or_measure:  Error setting scope options nstreams= %d: %s\n", scope_options.nstreams, ngpd_get_error_message());
				return -1;
			}
			scope_src = use_measure?NGZMP_SCOPE_SEL_ANA_MEASURE:NGZMP_SCOPE_SEL_ANA_DIFF_TRIG_OUT;
			scope_alt = (use_measure==NGPD_CAL_USE_MEASURE_CORRECTED)?NGZMP_SCOPE_ALT_MEAS_CORRECTED:0;
			for (dma=0; dma<2; dma++)
			{
				for (stream=0; stream<3; stream++)
				{
					int chan = stream+dma*3+do_upper*4;
					chan %= chans_per_card;
					if ((rc=ngpd_scope_setup_stream(path, card, stream+dma*4, chan, scope_src, scope_alt)) < 0)
					{
						snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_trig_or_measure: Error setting scope stream %d: %s\n", stream+dma*4, ngpd_get_error_message());
						return -1;
					}
				}
				if ((rc=ngpd_scope_setup_stream(path, card, 3+dma*4, 0, NGZMP_SCOPE_SEL37_DIG_5BIT, 0)) < 0)
				{
					snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_trig_or_measure: setting scope stream %d: %s\n",  3+dma*4, ngpd_get_error_message());
					return -1;
				}
			}
			*first_stream = 0;
			*last_stream = 6;
			if (use_measure)
			{
				*trig_offset = 0;
				*wide_a_offset = -1;
				*wide_b_offset = -1;
				*neutron_offset = -1;
			}
			else
			{
				*trig_offset = 2;
				*wide_a_offset = 3;
				*wide_b_offset = 4;
				*neutron_offset = -1;
			}
		}
		else
		{		/* this version used to measure XTK */
			if (need_upper != NULL)
				*need_upper = 0;
			scope_options.nstreams = 10;

#if 0
		// Will want this, but may/will need work on firmware
			if (sync_boxes)
				scope_options.flags = NGPDScopeOpt_Synchronised;	// Turn on delayed start mode, to try to sync boxes.
			else
				scope_options.flags = 0; 	// Turn off delayed start mode, not required.
#endif
			rc = ngpd_set_scope_options(path, -1, &scope_options);
			if (rc < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_trig_or_measure:  Error setting scope options nstreams= %d: %s\n", scope_options.nstreams, ngpd_get_error_message());
				return -1;
			}
			scope_src = use_measure?NGZMP_SCOPE_SEL_ANA_MEASURE:NGZMP_SCOPE_SEL_ANA_DIFF_TRIG_OUT;
			if (use_measure == NGPD_CAL_USE_MEASURE_NONE)
				scope_alt = NGZMP_SCOPE_ALT_DIFF_OUT_DIG4;
			else
				scope_alt = (use_measure==NGPD_CAL_USE_MEASURE_CORRECTED)?NGZMP_SCOPE_ALT_MEAS_CORRECTED:0;
			for (dma=0; dma<2; dma++)
			{
				for (stream=0; stream<4; stream++)
				{
					int chan = stream+dma*4;
					chan %= chans_per_card;
					if ((rc=ngpd_scope_setup_stream(path, card, stream+dma*4, chan, scope_src, scope_alt)) < 0)
					{
						snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_trig_or_measure: Error setting scope stream %d: %s\n", stream+dma*4, ngpd_get_error_message());
						return -1;
					}
				}
			}
			if ((rc=ngpd_scope_setup_stream(path, card, 8, 0, NGZMP_SCOPE_SEL8_DIG_4BIT, 0)) < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_trig_or_measure: setting scope stream %d: %s\n",  8, ngpd_get_error_message());
				return -1;
			}
			if ((rc=ngpd_scope_setup_stream(path, card, 9, 0, NGZMP_SCOPE_SEL9_DIG_4BIT, 0)) < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_trig_or_measure: setting scope stream %d: %s\n",  9, ngpd_get_error_message());
				return -1;
			}
			*first_stream = 0;
			*last_stream = 7;
			if (use_measure)
			{
				*trig_offset = 0;
				*wide_a_offset = -1;
				*wide_b_offset = -1;
				*neutron_offset = -1;
			}
			else
			{
				*trig_offset = 1;
				*wide_a_offset = 2;
				*wide_b_offset = 3;
				*neutron_offset = -1;
			}
		}
			
	}
	else
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_trig_or_measure:  Unknown NGPD generation %d\n", generation);
		return -1;
	}
	return 0;
}
/**
 * Determine which analogue channel is stored on a given scope mode stream when setup using ngpd_cal_setup_trig_or_measure();
 * The system is configured using ngpd_cal_setup_trig_or_measure(). This reports the first and last stream which stream are used.
 * The calling software scan streams in this range calling this function to see which analogue channel is stored in this stream, if any..
 * This is supported for both the ADQ14 and ZynqMP generations.
 * @param path 			A handle to the top level of the ngpd system returned from any of the config functions  {@link ngpd_config_adq14} or {@link ngpd_config_ngzmp()}.
 * @param use_measure	Consistent wit call to ngpd_cal_setup_trig_or_measure()
 * @param do_upper		Consistent wit call to ngpd_cal_setup_trig_or_measure()
 * @param stream		Stream number
 * @return 				Channel number within card (0..chans_per_card-1) or -1 if this stream does not have analogue data. This is not an error, skip to next stream.
 */
int ngpd_cal_setup_trig_or_measure_get_chan(int path, int use_measure, int do_upper, int stream)
{
	NGPDGeneration generation = ngpd_get_generation(path);
	int nstreams;
	int chans_per_card = ngpd_get_chans_per_card(path);

	if (generation == NGPDGenADQ14)
	{
		if (chans_per_card == 1)
			nstreams = 2;
		else
			nstreams = 3;

		if (stream < 0 || stream >= nstreams-1)
			return -1;
		return stream;
	}
	else if (generation == NGPDGenZynqMP1)
	{
		NGPDScopeModule *mod = ngpd_scope_get_mod(path);
		if (mod == NULL)
			return -1;
		if (mod->head.layout == NGZMP_SCOPE_LAYOUT_10)
		{
			if (stream <0 || stream >= 8)
				return -1;
			return stream;
		}
		else
		{
			if (stream <0 || stream == 3 || stream >= 5)
				return -1;
			if (stream <= 2)
				return stream+do_upper*4;
			if (stream == 4)
				return 3+do_upper*4;
		}
	}
	return -1;
}


/**
 * Setup scope mode to capture analogue signal and digital signals from all channels, with as many digital bits as possible.
 * This is supported for both the ADQ14 and ZynqMP generations.
 * For the ZynqMP 8 channel, limitations in memory bandwidth mean that in 10 stream scope mode with all 8 analogue stream only 2 digital bits can be recorded per stream. 

 * @param path 			A handle to the top level of the ngpd system returned from any of the config functions  {@link ngpd_config_adq14} or {@link ngpd_config_ngzmp()}.
 * @param card 			Card number 0..num_cards-1 or -1 to setup all cards in system.
 * @param scope_src		Data source as described by {@link NGZMP_SCOPE_SEL_MACROS}
 * @param first_stream	Pointer to report the first stream to be scanned data.
 * @param last_stream	Pointer to report the last stream to be scanned data.
 * @param trig_offset	Pointer to return bit position within digital bits for the trigger signal. To be added to base position from ngpd_scope_stream_details()
 * @param wide_a_offset	Pointer to return bit position within digital bits for the wide_a signal, or -1 if not available
 * @param wide_b_offset	Pointer to return bit position within digital bits for the wide_b signal, or -1 if not available
 * @param neutron_offset Pointer to return bit position within digital bits for the neutron signal (from measure) or -1 when not available.
 * @param sync_boxes	Synchronise acquisition across all cards when firmware supports this. For future expansion.
 * @return		Returns 0 on success or -ve on error with error message available from ngpd_cal_get_error_message()
*/

int ngpd_cal_setup_all_chan(int path,  int card, int scope_src, int *first_stream, int *last_stream,  int *trig_offset, int *wide_a_offset, int *wide_b_offset, int *neutron_offset, int sync_boxes)
{
	NGPDGeneration generation = ngpd_get_generation(path);
	int stream;
	u_int32_t scope_chan;
	u_int32_t scope_alt;
	int rc;
	NGPDScopeOptions scope_options;
	NGPDScopeModule * scope_mod;

	scope_mod = ngpd_scope_get_mod(path);
	if (scope_mod == NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_chan: Cannot determine scope module pointer for path %d : %s", path, ngpd_get_error_message());
		return -1;
	}

	if (generation == NGPDGenADQ14)
	{
		int chans_per_card = ngpd_get_chans_per_card(path);

		if (chans_per_card == 1)
			scope_options.nstreams = 2;
		else
			scope_options.nstreams = 4;

#if 0
		if (sync_boxes)
			scope_options.flags = NGPDScopeOpt_Synchronised;	// Turn on delayed start mode, to try to sync boxes.
		else
			scope_options.flags = 0; 	// Turn off delayed start mode, not required.
#endif
		rc = ngpd_set_scope_options(path, -1, &scope_options);
		if (rc < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_chan: Error setting scope options nstreams= %d: %s", scope_options.nstreams, ngpd_get_error_message());
			return -1;
		}

		for (stream=0; stream<chans_per_card; stream++)
		{
			if ((rc=ngpd_scope_setup_stream(path, card, stream, stream, scope_src, 0)) < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_chan: Error setting scope stream %d: %s", stream, ngpd_get_error_message());
				return -1;
			}
		}
		if ((rc=ngpd_scope_setup_stream(path, card, scope_options.nstreams-1, 0, NGPD_SCOPE_SEL5_DIG_5BIT, 0)) < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_chan: Error setting scope stream %d: %s", 5, ngpd_get_error_message());
			return -1;
		}
		*first_stream = 0;
		*last_stream = chans_per_card-1;

		if (scope_src == NGPD_SCOPE_SEL0TO5_MEASURE)
		{
			*trig_offset = 0;
			*wide_a_offset = -1;
			*wide_b_offset = -1;
			*neutron_offset = -1;
		}
		else if (scope_src == NGPD_SCOPE_SEL0TO5_DIFF_TRIG_OUT)
		{
			*trig_offset = 2;
			*wide_a_offset = 3;
			*wide_b_offset = 4;
			*neutron_offset = -1;
		}
		else
		{
			*trig_offset = -1;
			*wide_a_offset = -1;
			*wide_b_offset = -1;
			*neutron_offset = -1;
		}
	}
	else if (generation == NGPDGenZynqMP1)
	{
		int chans_per_card = ngpd_get_chans_per_card(path);

		scope_options.nstreams = 10;

#if 0
		if (sync_boxes)
			scope_options.flags = NGPDScopeOpt_Synchronised;	// Turn on delayed start mode, to try to sync boxes.
		else
			scope_options.flags = 0; 	// Turn off delayed start mode, not required.
#endif
		rc = ngpd_set_scope_options(path, -1, &scope_options);
		if (rc < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_chan:  Error setting scope options nstreams= %d: %s", scope_options.nstreams, ngpd_get_error_message());
			return -1;
		}
		for (stream=0; stream<8; stream++)
		{
			int chan = stream;
			chan %= chans_per_card;
			if ((rc=ngpd_scope_setup_stream(path, card, stream, chan, scope_src, 0)) < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_chan:  Error setting scope stream %d: %s", stream, ngpd_get_error_message());
				return -1;
			}
		}
		if ((rc=ngpd_scope_setup_stream(path, card, 8, 0, NGZMP_SCOPE_SEL8_DIG_4BIT, 0)) < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_chan:  Error setting scope stream %d: %s", 8, ngpd_get_error_message());
			return -1;
		}
		if ((rc=ngpd_scope_setup_stream(path, card, 9, 0, NGZMP_SCOPE_SEL9_DIG_4BIT, 0)) < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_chan:  Error setting scope stream %d: %s", 9, ngpd_get_error_message());
			return -1;
		}
		*first_stream = 0;
		*last_stream = 7;
		if (scope_src == NGZMP_SCOPE_SEL_ANA_MEASURE)
		{
			*trig_offset = 0;
			*wide_a_offset = -1;
			*wide_b_offset = -1;
			*neutron_offset = -1;
		}
		else if (scope_src == NGZMP_SCOPE_SEL_ANA_DIFF_TRIG_OUT)
		{
			*trig_offset = 2;
			*wide_a_offset = 3;
			*wide_b_offset = -1;
			*neutron_offset = -1;
		}
		else
		{
			*trig_offset = -1;
			*wide_a_offset = -1;
			*wide_b_offset = -1;
			*neutron_offset = -1;
		}
	}
	else
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_chan:  Unknown NGPD generation %d", generation);
		return -1;
	}
	return 0;
}
/**
 * Determine which analogue channel is stored on a given scope mode stream when setup using ngpd_cal_setup_all_chan().
 * The system is configured using ngpd_cal_setup_all_chan(). This reports the first and last stream which stream are used.
 * The calling software scan streams in this range calling this function to see which analogue channel is stored in this stream, if any..
 * This is supported for both the ADQ14 and ZynqMP generations.
 * @param path 			A handle to the top level of the ngpd system returned from any of the config functions  {@link ngpd_config_adq14} or {@link ngpd_config_ngzmp()}.
 * @param use_measure	Consistent with call to ngpd_cal_setup_all_chan()
 * @param stream		Stream number
 * @return 				Channel number within card (0..chans_per_card-1) or -1 if this stream does not have analogue data. This is not an error, skip to next stream.
 */
int ngpd_cal_setup_all_chan_get_chan(int path, int scope_src, int stream)
{
	NGPDGeneration generation = ngpd_get_generation(path);
	int nstreams;
	int chans_per_card = ngpd_get_chans_per_card(path);

	if (generation == NGPDGenADQ14)
	{
		if (chans_per_card == 1)
			nstreams = 2;
		else
			nstreams = 3;

		if (stream < 0 || stream >= nstreams-1)
			return -1;
		return stream;
	}
	else if (generation == NGPDGenZynqMP1)
	{
		if (stream <0 ||stream >= 8)
			return -1;
		return stream;
	}
	return -1;
}

/**
@}
*/
