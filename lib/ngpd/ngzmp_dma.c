/*
 * ngzmp_dma.c
 *
 *  Created on: 18/6/2021
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
#ifdef linux
#	include <unistd.h>
#endif
#include "ngpd.h"
#include "ngzmp_dma_protocol.h"
#include "zynqmp_protocol.h"
#include "zynqmp_api.h"

extern char ngpd_error_message[];

/**
 * @ingroup internal
 * Reset the DMA engines. 
 * Not usually called by use code, but use internaly. 
 *
 * @param path a handle to the top level of the ngpd system returned from {@link ngpd_config_ngzmp()}.
 * @param card is the number of the card in the ngpd system,
 *        0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 * @param stream_mask flag to notify which dma stream to reset defined as {@link NGZMP_DMA_STREAM_MASK}. Can be multiple.
 * @return {@link NGPD_OK} or a negative error code.
 */
int ngzmp_dma_reset(int path, int card, u_int32_t stream_mask) {
	int rc = 0;
	int this_path;
	int first_card, last_card;

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_reset: Invalid Path");
		return NGPD_ERROR;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
	{
		printf("Dummy:ngzmp_dma_reset(card=%d, stream_mask=%04X)\n", card, stream_mask);
		return 0;
	}

	if (card < 0)
	{
		first_card = 0;
		last_card = NGPDPath[path].num_cards-1;
	}
	else if (card >= NGPDPath[path].num_cards) //  Do sanity check on card number versus total in system
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_reset: illegal card specified (%d)", card);
		return NGPD_ERROR;
	}
	else
	{
		first_card = last_card = card;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_reset: Called on ADQ14 system");
		return NGPD_ERROR;
	}

	for (card=first_card; card <=last_card; card++)
	{
		if (NGPDPath[path].type == NGPDComposite)
		{
			this_path = NGPDPath[path].sub_path[card];
			if (this_path < 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_reset: resolved illegal sub path (path=%d)",path);
				return NGPD_ERROR;
			}
		}
		else
		{
			this_path = path;
		}
		rc = zynqmp_personality_write(NGPDPath[this_path].zynqmp, ZYNQMP_DMA_CMD_RESET, ZYNQMP_WIDTH_LONG, stream_mask, 0 , NULL, 0, NULL, NULL);
		if (rc != NGPD_OK)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_reset: Error resetting DMAs: return code %d:%s", rc, zynqmp_get_error_message());
			break;
		}
	}
		
	return rc;
}

/**
 * @ingroup internal
 * Build DMA descriptors for the specified stream using the details in the message. 
 * Not usually called by use code, but use internaly. 
 *
 * @param path a handle to the top level of the ngpd system returned from {@link ngpd_config_ngzmp()}.
 * @param card is the number of the card in the ngpd system,
 *        0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 * @param stream	DMA stream number to build the descriptors for. defined as {@link NGZMP_DMA_STREAM_BNUM.
 * @param msg		Messages of type {@link  ZYNQMP_DMA_MsgBuildDesc}
 * @return {@link NGPD_OK} or a negative error code.
 */
int ngzmp_dma_build_desc(int path, int card, u_int32_t stream, ZYNQMP_DMA_MsgBuildDesc *msg, int32 * num_desc)
{
	int rc = 0;
	int this_path;
	int first_card, last_card;

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_reset: Invalid Path");
		return NGPD_ERROR;
	}

	if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
	{
		printf("Dummy:ngzmp_dma_build_desc(card=%d, stream=%d)\n", card, stream);
		return 0;
	}

	if (card < 0)
	{
		first_card = 0;
		last_card = NGPDPath[path].num_cards-1;
	}
	else if (card >= NGPDPath[path].num_cards) //  Do sanity check on card number versus total in system
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_reset: illegal card specified (%d)", card);
		return NGPD_ERROR;
	}
	else
	{
		first_card = last_card = card;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_reset: Called on ADQ14 system");
		return NGPD_ERROR;
	}

	for (card=first_card; card <=last_card; card++)
	{
		if (NGPDPath[path].type == NGPDComposite)
		{
			this_path = NGPDPath[path].sub_path[card];
			if (this_path < 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_reset: resolved illegal sub path (path=%d)",path);
				return NGPD_ERROR;
			}
		}
		else
		{
			this_path = path;
		}

		if (stream < 1 || stream >= ZYNQMP_DMA_STREAM_NUM)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_reset: invalid dma stream number %d", stream);
			return NGPD_ERROR;
		}
		
		rc = zynqmp_personality_write(NGPDPath[this_path].zynqmp, ZYNQMP_DMA_CMD_BUILD_DESC, ZYNQMP_WIDTH_LONG, stream, sizeof(ZYNQMP_DMA_MsgBuildDesc)/sizeof(u_int32_t), (u_int8_t*)msg, 0, NULL, num_desc);
		if (rc != NGPD_OK)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_reset: Error resetting DMAs: return code %d:%s", rc, zynqmp_get_error_message());
			break;
		}
	}
		
	return rc;
}

/**
 * @ingroup internal
 * Request ZynqMP to build test patterns into the DMA buffers
 * Not usually called by use code, but use internaly. 
 *
 * @param path a handle to the top level of the ngpd system returned from {@link ngpd_config_ngzmp()}.
 * @param card is the number of the card in the ngpd system,
 *        0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 * @param stream	DMA stream number to build the descriptors for. defined as {@link NGZMP_DMA_STREAM_BNUM.
 * @param msg		Messages of type {@link  ZYNQMP_DMA_MsgBuildDesc}
 * @return {@link NGPD_OK} or a negative error code.
 */
int ngzmp_dma_build_test_pat(int path, int card, u_int32_t stream, ZYNQMP_DMA_MsgTestPat *msg)
{
	int rc = 0;
	int this_path;
	int first_card, last_card;

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_build_test_pat: Invalid Path");
		return NGPD_ERROR;
	}

	if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
	{
		printf("Dummy:ngzmp_dma_build_test_pat(card=%d, stream=%d)\n", card, stream);
		return 0;
	}

	if (card < 0)
	{
		first_card = 0;
		last_card = NGPDPath[path].num_cards-1;
	}
	else if (card >= NGPDPath[path].num_cards) //  Do sanity check on card number versus total in system
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_build_test_pat: illegal card specified (%d)", card);
		return NGPD_ERROR;
	}
	else
	{
		first_card = last_card = card;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_build_test_pat: Called on ADQ14 system");
		return NGPD_ERROR;
	}

	for (card=first_card; card <=last_card; card++)
	{
		if (NGPDPath[path].type == NGPDComposite)
		{
			this_path = NGPDPath[path].sub_path[card];
			if (this_path < 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_build_test_pat: resolved illegal sub path (path=%d)",path);
				return NGPD_ERROR;
			}
		}
		else
		{
			this_path = path;
		}

		if (stream < 1 || stream >= ZYNQMP_DMA_STREAM_NUM)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_build_test_pat: invalid dma stream number %d", stream);
			return NGPD_ERROR;
		}
		
		rc = zynqmp_personality_write(NGPDPath[this_path].zynqmp, ZYNQMP_DMA_CMD_BUILD_TEST_TEST_PAT, ZYNQMP_WIDTH_LONG, stream, sizeof(ZYNQMP_DMA_MsgTestPat)/sizeof(u_int32_t), (u_int8_t*)msg, 0, NULL, NULL);
		if (rc != NGPD_OK)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_build_test_pat: Error building test pattern type %d, %d:%s", msg->options, rc, zynqmp_get_error_message());
			break;
		}
	}
	return rc;
}

/**
 * @ingroup internal
 * Start the DMA engines. 
 * Not usually called by use code, but use internaly. The desriptor must have been built first.
 *
 * @param path a handle to the top level of the ngzmp system returned from {@link ngpd_config_ngzmp()}.
 * @param card is the number of the card in the ngpd system,
 *        0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 * @param stream_mask flag to notify which dma stream to reset defined as {@link NGZMP_DMA_STREAM_MASK}. Can be multiple.
 * @param msg		Start command options message of type {@link ZYNQMP_DMA_MsgStart}
 * @return {@link NGPD_OK} or a negative error code.
 */
int ngzmp_dma_start(int path, int card, u_int32_t stream_mask, ZYNQMP_DMA_MsgStart *msg)
{
	int rc = 0;
	int this_path;
	int first_card, last_card;

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_start: Invalid Path");
		return NGPD_ERROR;
	}

	if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
	{
		printf("Dummy:ngzmp_dma_start(card=%d, stream_mask=0x%04X)\n", card, stream_mask);
		return 0;
	}

	if (card < 0)
	{
		first_card = 0;
		last_card = NGPDPath[path].num_cards-1;
	}
	else if (card >= NGPDPath[path].num_cards) //  Do sanity check on card number versus total in system
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_start: illegal card specified (%d)", card);
		return NGPD_ERROR;
	}
	else
	{
		first_card = last_card = card;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_start: Called on ADQ14 system");
		return NGPD_ERROR;
	}

	for (card=first_card; card <=last_card; card++)
	{
		if (NGPDPath[path].type == NGPDComposite)
		{
			this_path = NGPDPath[path].sub_path[card];
			if (this_path < 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_start: resolved illegal sub path (path=%d)",path);
				return NGPD_ERROR;
			}
		}
		else
		{
			this_path = path;
		}
		
		rc = zynqmp_personality_write(NGPDPath[this_path].zynqmp, ZYNQMP_DMA_CMD_START, ZYNQMP_WIDTH_LONG, stream_mask, sizeof(ZYNQMP_DMA_MsgStart)/sizeof(u_int32_t), (u_int8_t*)msg, 0, NULL, NULL);
		if (rc != NGPD_OK)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_start: Error starting DMAs: return code %d:%s", rc, zynqmp_get_error_message());
			break;
		}
	}
		
	return rc;
}

/**
 * @ingroup internal
 * Get the status and settings of all the DMA streams in Xspress3mini and Xspress4
 *
 * @param path a handle to the top level of the ngpd system returned from {@link ngpd_config_ngzmp()}.
 * @param card is the number of the card in the ngpd system,
 *        0 for a single card system and up to ({@link xsp3_get_num_cards()} - 1) for a multi-card system,
 *        where this value has been passed to {@link ngpd_config_ngzmp()} at ngpd system configuration.
 * @param statusBlock a {@link NGZMP_DMA_StatusBlock} structure to receive the status.
 * @return {@link #NGPD_OK NGPD_OK} or a negative error code.
 */
int ngzmp_get_dma_status_block(int path, int card, ZYNQMP_DMA_StatusBlock *status_block) 
{
	int this_path;
	int rc;

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_get_dma_status_block: Invalid Path");
		return NGPD_ERROR;
	}
	// Do sanity check on card number versus total in system
	if ((card < 0) || (card >= NGPDPath[path].num_cards)) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_get_dma_status_block: illegal card specified (%d)", card);
		return NGPD_ERROR;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_get_dma_status_block: Called on ADQ14 system");
		return NGPD_ERROR;
	}

	if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
	{
		printf("Dummy:ngzmp_get_dma_status_block(card=%d)\n", card);
		return 0;
	}

	if (NGPDPath[path].type == NGPDComposite)
	{
		this_path = NGPDPath[path].sub_path[card];
		if (this_path < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_get_dma_status_block: resolved illegal sub path (path=%d)",path);
			return NGPD_ERROR;
		}
	}
	else
	{
		this_path = path;
	}
	rc = zynqmp_access_read(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_DMA_STATUS, ZYNQMP_WIDTH_LONG, 0, sizeof(ZYNQMP_DMA_StatusBlock)/sizeof(u_int32_t), 1, (u_int8_t *)status_block);
	if (rc != NGPD_OK)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_get_dma_status_block: zynqmp_access_read() returns %d:%s", rc, zynqmp_get_error_message());
		return rc;
	}

	return NGPD_OK;

}

int ngzmp_dma_config_memory(int path, int card, int layout, int just_read, int force)
{
	int rc = NGPD_OK;
	ZYNQMP_DMA_StatusBlock status_block;
	int this_path;

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) 
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_config_memory: invalid path %d", path);
		return NGPD_ERROR;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_config_memory: Called on ADQ14 system");
		return NGPD_ERROR;
	}
	if (card < 0)
	{
		for (card=0; card<NGPDPath[path].num_cards; card++)
			rc = ngzmp_dma_config_memory(path, card, layout, just_read, force);
		return rc;
	}
	// Do sanity check on card number versus total in system
	if (card < 0 || card >= NGPDPath[path].num_cards) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_config_memory: illegal card specified (%d)", card);
		return NGPD_ERROR;
	}

	if (NGPDPath[path].type == NGPDComposite) {
		this_path = NGPDPath[path].sub_path[card];
		if (this_path < 0) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_config_memory: resolved illegal sub path (path=%d)", path);
			return NGPD_ERROR;
		}
	}
	else
	{
		this_path = path;
	}

	if (!force && layout == NGPDPath[this_path].mem_layout)
		return 0;

	if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
	{
		printf("Dummy:ngzmp_dma_config_memory(card=%d)\n", card);
		NGPDPath[path].nbytes_scope_per_card = ((size_t)2)*1024*1024*1024;
		NGPDPath[this_path].nbytes_scope_per_card = ((size_t)2)*1024*1024*1024;
		NGPDPath[path].playback_num_bytes = 1*1024*1024*1024;
		NGPDPath[this_path].playback_num_bytes = 1*1024*1024*1024;
		return 0;
	}
	if (!just_read)
	{
		rc = zynqmp_personality_write(NGPDPath[this_path].zynqmp, ZYNQMP_DMA_CMD_CONFIG_MEMORY, ZYNQMP_WIDTH_LONG, (u_int32_t)layout, 0 , NULL, 0, NULL, NULL);
		if (rc != NGPD_OK)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_config_memory: Error configuring memory using code %08X: return code %d:%s", layout, rc, zynqmp_get_error_message());
		}
		NGPDPath[path].mem_layout = layout;
		NGPDPath[this_path].mem_layout = layout;
	}
	rc = ngzmp_get_dma_status_block(path, card, &status_block);
	if (rc != 0)
	{
		char err_copy[NGPD_MAX_ERROR_MESSAGE + 2];
		strcpy(err_copy, ngpd_get_error_message());
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_config_memory: %s : returns %d", err_copy, rc);
		return rc;
	}
	NGPDPath[path].nbytes_scope_per_card = status_block.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0].data_size;
	NGPDPath[this_path].nbytes_scope_per_card = status_block.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0].data_size;
	NGPDPath[path].playback_num_bytes = status_block.stream_def[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].data_size;
	NGPDPath[this_path].playback_num_bytes = status_block.stream_def[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].data_size;
	return rc;
}


/**
 * @ingroup debug
 * Tell ZynqMP driver to display a region of DMA buffer
 * Debug function for use with console connection to ZynqMP.
 *
 * @param path a handle to the top level of the ngpd system returned from {@link ngpd_config_ngzmp()}.
 * @param card is the number of the card in the ngpd system,
 *        0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 * @param stream_mask flag to notify which dma stream to reset defined as {@link NGZMP_DMA_STREAM_MASK}. Can be multiple.
 * @return {@link NGPD_OK} or a negative error code.
 */
int ngzmp_dma_print_data(int path, int card, u_int32_t stream, ZYNQMP_DMA_MsgPrint *msg)
{
	int rc = 0;
	int this_path;
	int first_card, last_card;

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_print: Invalid Path");
		return NGPD_ERROR;
	}

	if (card < 0)
	{
		first_card = 0;
		last_card = NGPDPath[path].num_cards-1;
	}
	else if (card >= NGPDPath[path].num_cards) //  Do sanity check on card number versus total in system
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_print: illegal card specified (%d)", card);
		return NGPD_ERROR;
	}
	else
	{
		first_card = last_card = card;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_print: Called on ADQ14 system");
		return NGPD_ERROR;
	}

	for (card=first_card; card <=last_card; card++)
	{
		if (NGPDPath[path].type == NGPDComposite)
		{
			this_path = NGPDPath[path].sub_path[card];
			if (this_path < 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_print: resolved illegal sub path (path=%d)",path);
				return NGPD_ERROR;
			}
		}
		else
		{
			this_path = path;
		}
		
		rc = zynqmp_personality_write(NGPDPath[this_path].zynqmp, ZYNQMP_DMA_CMD_PRINT_DATA, ZYNQMP_WIDTH_LONG, stream, sizeof(ZYNQMP_DMA_MsgPrint)/sizeof(u_int32_t), (u_int8_t*)msg, 0, NULL, NULL);
		if (rc != NGPD_OK)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_print: Error displaying DMA buffer data: return code %d:%s", rc, zynqmp_get_error_message());
			break;
		}
	}
		
	return rc;
}

int ngzmp_system_start(int path, int card, int pb_num_t, NGPDITFGSetup *itfg_setup)
{
	int rc;
	int first_card, last_card;
	u_int32_t stream_mask, pb_cont;
	ZYNQMP_DMA_MsgBuildDesc msgBuildDesc;
	char err_copy[NGPD_MAX_ERROR_MESSAGE + 2];
	u_int32_t dpc = 0; /* may change if want to send Hist data via SDRAM */
	//u_int32_t num_lwords;
	u_int32_t scope_cont, num_scope_words;
	ZYNQMP_DMA_MsgStart msgStart;
	int debug;
	int scope, num_scope;
	u_int32_t data_path_or, data_path_and;
	ZYNQMP_DMA_StatusBlock status_block;
	int num_scope_streams=4;

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) 
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start: invalid path %d", path);
		return NGPD_ERROR;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start: Called on ADQ14 system");
		return NGPD_ERROR;
	}

	if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
	{
		printf("Dummy:ngzmp_system_start(card=%d)\n", card);
		return 0;
	}

	debug = NGPDPath[path].debug;

	first_card = 0;
	last_card = NGPDPath[path].num_cards-1;
	if (debug)
	{
		printf("Starting system cards %d .. %d, num_cards=%d with:", first_card, last_card, NGPDPath[path].num_cards);
		if (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_PLAYBACK)
			printf(" Playback");
		if (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_SCOPEMODE)
			printf(" Scope Mode");
		if (debug > 1)
		{
			printf("\n"); // More info will follow
		} 
	}
	for (card = first_card; card <= last_card; card++) 
	{
		/* First stop and reset DMAs on all cards to stop source of further UDP/TCP data */
		if (debug > 1)
			printf(".... Stopping and reseting card = %d\n", card);
		dpc = 0;
	
		rc =  ngpd_write_glob_regs(path, card, NGZMP_DATA_PATH, 1, &dpc);
		if (rc != 0) {
			strcpy(err_copy, ngpd_get_error_message());
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error writing data path register %s", 	err_copy);
			ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
			return rc;
		}
		stream_mask = NGZMP_DMA_STREAM_MASK_PLAYBACK0 | NGZMP_DMA_STREAM_MASK_SCOPE0 | NGZMP_DMA_STREAM_MASK_SCOPE1 | NGZMP_DMA_STREAM_MASK_SCOPE2 |
						NGZMP_DMA_STREAM_MASK_HIST_TEST | NGZMP_DMA_STREAM_MASK_HIST_READ;

		rc = ngzmp_dma_reset(path, card, stream_mask);
		if (rc != 0) {
			strcpy(err_copy, ngpd_get_error_message());
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error Reseting all DMAs %s", err_copy);
			ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
			return rc;
		}
	}
	if (itfg_setup->col_time > 0)
	{
		for (card = first_card; card <= last_card; card++) 
		{
			u_int32_t itfg_regs[3];
			itfg_regs[0] = (int)floor(1000000*itfg_setup->col_time+0.5);
			itfg_regs[1] = NGPD_ITFG_SET_TRIG_MODE(itfg_setup->trig_mode);
			if (itfg_setup->cycles > 0)
				itfg_regs[2] = itfg_setup->cycles;
			else
				itfg_regs[2] = 1;
			if (itfg_regs[0]  < 1)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Collection time must be > 1 us, not %g", itfg_setup->col_time);
				return -1;
			}
			printf("ITFG regs=%d, %d, %d\n", itfg_regs[0], itfg_regs[1], itfg_regs[2] );
			if ((rc=ngpd_write_glob_regs(path, card, NGZMP_ITFG_TIME, 3, itfg_regs)) != 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Writing ITFG Time register");
				return -1;
			}
		}
	}

	for (card = first_card; card <= last_card; card++) 
	{
		if (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_PLAYBACK)
		{
			if (debug > 1)
				printf(".... Configuring playback DMA\n");

			memset(&msgBuildDesc, 0, sizeof(msgBuildDesc));
			msgBuildDesc.src_stream = NGZMP_DMA_STREAM_BNUM_PLAYBACK0;
			if (pb_num_t > 0)
			{
				/* For test cases limit the size of the playback data */
				msgBuildDesc.size_bytes = 2*NGPDPath[path].playback_num_streams*pb_num_t;
			}
			msgBuildDesc.size_bytes = sizeof(u_int16_t)*NGPDPath[path].playback_num_streams*NGPDPath[path].playback_num_t;
			msgBuildDesc.size_bytes &= 0xFFFFFFFE0L;	// Force 256 bit = 32 byte alignment.

			rc = ngzmp_dma_build_desc(path, card, NGZMP_DMA_STREAM_BNUM_PLAYBACK0, &msgBuildDesc, NULL);
			if (rc != 0) {
				strcpy(err_copy, ngpd_get_error_message());
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error Building Playback descriptors %s", err_copy);
				ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
				return rc;
			}
			switch (NGPDPath[path].playback_num_streams)
			{
			case 1:	pb_cont = NGZMP_PLAYBACK_ENABLE | NGZMP_PLAYBACK_NUM_STREAMS(0); break;
			case 2:	pb_cont = NGZMP_PLAYBACK_ENABLE | NGZMP_PLAYBACK_NUM_STREAMS(1); break;
			case 4:	pb_cont = NGZMP_PLAYBACK_ENABLE | NGZMP_PLAYBACK_NUM_STREAMS(2); break;
			case 8:	pb_cont = NGZMP_PLAYBACK_ENABLE | NGZMP_PLAYBACK_NUM_STREAMS(3); break;
			default:
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Unsupported number of playback stream %d", NGPDPath[path].playback_num_streams);
				return -1;
			}
			rc = ngpd_write_glob_regs(path, card, NGZMP_PLAYBACK, 1, &pb_cont);
			if (rc < 0)
			{
				strcpy(err_copy, ngpd_get_error_message());
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error setting Rlack control register: %s", err_copy);
				ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
				return rc;
			}
			memset(&msgStart, 0, sizeof(msgStart));
			msgStart.options = ZYNQMP_DMA_START_CIRCULAR;
			rc = ngzmp_dma_start(path, card, NGZMP_DMA_STREAM_MASK_PLAYBACK0, &msgStart);
			if (rc != 0) {
				strcpy(err_copy, ngpd_get_error_message());
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error Starting Playback DMA %s", err_copy);
				ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
				return rc;
			}
		}
		if (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_SCOPEMODE) 
		{
			if (debug > 1)
				printf(".... Configuring Scope mode\n");

			rc = ngpd_read_glob_regs(path, card, NGZMP_SCOPE_CONT, 1, &scope_cont);
			if (rc < 0)
			{
				strcpy(err_copy, ngpd_get_error_message());
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error reading Scope Control %s", err_copy);
				ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
				return rc;
			}
			if (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS)
			{
				int nstreams;
				nstreams = ngpd_scope_mod_get_nstreams(NGPDPath[path].scope_mod);
				scope_cont &= ~NGPD_SCOPE_CONT_NUM_STREAMS(255);
				switch (nstreams)
				{
				case 4: scope_cont |= NGZMP_SCOPE_CONT_NUM_STREAMS(0);
					break;
				case 8: scope_cont |= NGZMP_SCOPE_CONT_NUM_STREAMS(1);
					break;
				case 10: scope_cont |= NGZMP_SCOPE_CONT_NUM_STREAMS(2);
					break;
				default : snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Unknown number of streams %d\n", nstreams);
					return -1;
				}
				rc = ngpd_write_glob_regs(path, card, NGZMP_SCOPE_CONT, 1, &scope_cont);
				if (rc < 0)
				{
					strcpy(err_copy, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error writing Scope Control %s", err_copy);
					ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
					return rc;
				}
				if ((rc=ngpd_write_glob_regs(path, card, NGZMP_SCOPE_CHAN_SEL0, 6, NGPDPath[path].scope_mod->head.card[0].chan_sel)) != 0)
				{
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Reading scope control register");
					return -1;
				}
			}
			else if (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD)
			{
				if ((rc=ngpd_read_glob_regs(path, card, NGZMP_SCOPE_CHAN_SEL0, 6, NGPDPath[path].scope_mod->head.card[0].chan_sel)) != 0)
				{
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Reading scope control register");
					return -1;
				}
			}
			rc = ngzmp_get_dma_status_block(path, card, &status_block);
			if (rc != 0) {
				strcpy(err_copy, ngpd_get_error_message());
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error Reading Status Block %s", err_copy);
				ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
				return rc;
			}

			switch ( NGZMP_SCOPE_CONT_GET_NUM_STREAMS(scope_cont))
			{
			case 0:
				num_scope_streams = 4;
				num_scope = 1;
				num_scope_words = status_block.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0].data_size/(16);
				break;

			case 1:
				num_scope_streams = 8;
				num_scope = 2;
				num_scope_words = status_block.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0].data_size/(16*2);
				break;

			case 2:
				num_scope_streams = 10;
				num_scope = 3;
				num_scope_words = status_block.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0].data_size/(40);
				break;

			default:
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Unsupported number of scopemode stream %d", NGZMP_SCOPE_CONT_GET_NUM_STREAMS(scope_cont));
				return -1;
			}
			if (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD)
			{
				ngpd_scope_set_streams(NGPDPath[path].scope_mod, num_scope_streams);
			}
			num_scope_words	&= 0xFFFFFFFC;		// force 64 byte alignment to mix with descriptors and cachelines.
			if ((rc= ngpd_write_glob_regs(path, card, NGZMP_SCOPE_NUMWORDS, 1, &num_scope_words)) < 0 )
			{
				strcpy(err_copy, ngpd_get_error_message());
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error writing Scope Control %s", err_copy);
				ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
				return rc;
			}
			printf("... Total scope data size=%ld, num_scope_words=%d\n", status_block.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0].data_size, num_scope_words);
			stream_mask = 0;
			for (scope=0;scope<num_scope; scope++)
			{
				memset(&msgBuildDesc, 0, sizeof(msgBuildDesc));
				msgBuildDesc.src_stream = NGZMP_DMA_STREAM_BNUM_SCOPE0;	// Calculate all byte sizes and offsets relative to stream 0
				msgBuildDesc.size_bytes = (scope==2)?num_scope_words*8:num_scope_words*16;
				msgBuildDesc.addr = num_scope_words*16*scope;

				rc = ngzmp_dma_build_desc(path, card, NGZMP_DMA_STREAM_BNUM_SCOPE0+scope, &msgBuildDesc, NULL);
				if (rc != 0) {
					strcpy(err_copy, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error Building Scope mode descriptors %s",  err_copy);
					ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
					return rc;
				}
				stream_mask |= 1 << (NGZMP_DMA_STREAM_BNUM_SCOPE0+scope);
			}
/*			num_lwords = status_block.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0].data_size / 4;
			if (NGPDPath[path].total_lwords_per_card != num_lwords * NGPDPath[path].features.num_scope_dma)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_system_start: Number of Lwords has changed since configuration. For NGPDPath=%d, From DMA status block=%d",
						NGPDPath[path].total_lwords_per_card, num_lwords * NGPDPath[path].features.num_scope_dma);
				return NGPD_ERROR;
			}
*/
			memset(&msgStart, 0, sizeof(msgStart));
			rc = ngzmp_dma_start(path, card, stream_mask, &msgStart);
			if (rc != 0) {
				strcpy(err_copy, ngpd_get_error_message());
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error starting Scope mode DMA: %s", err_copy);
				ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
				return rc;
			}
			ngpd_scope_mark_all(path, card, 0);	// Mark scope module contents as invalid so imgd or da.server knows to fetch the data
		}
		/* When finished, also need to prepare scalers DMA Here */
#if 0
		if ( ((NGPDPath[path].run_flags & NGPD_RUN_FLAGS_SCALERS) && !NGPDPath[path].features.soft_scalers)  || 
			(NGPDPath[path].features.generation == XspressGen3Mini && (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_HIST) && !(NGPDPath[path].readout_mode & Xsp3mRd_SendHistList)) )
		{

			rc = ngpd_read_glob_regs(path, card, NGZMP_GLOB_TIMING_FIXED, 1, &fixed_tf);
			if (rc != 0) {
				strcpy(err_copy, ngpd_get_error_message());
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error reading Global Fixed Time Frame reg : %s", err_copy);
				ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
				return rc;
			}
		}
		if ((NGPDPath[path].run_flags & NGPD_RUN_FLAGS_SCALERS) && !NGPDPath[path].features.soft_scalers) 
		{
			int this_path;

			if (NGPDPath[path].type == NGPDComposite)
				this_path = NGPDPath[path].sub_path[card];
			else
				this_path = path;

			memset(&msgBuildDesc, 0, sizeof(msgBuildDesc));
			msgBuildDesc.src_stream = NGPD_DMA_STREAM_BNUM_SCALERS;
			if (NGPDPath[path].features.generation == XspressGen3Mini)
			{
				msgBuildDesc.block_size= sizeof(u_int32_t)*(XSP3M_HW_NUM_SCALERS*NGPDPath[this_path].num_chan);
				msgBuildDesc.size_bytes=sizeof(u_int32_t)*(XSP3M_HW_NUM_SCALERS*NGPDPath[this_path].num_chan) * NGPDPath[this_path].scaler_num_tf_fw;
			}
			else
			{
				msgBuildDesc.block_size= sizeof(u_int32_t)*(2+NGPD_HW_DEFINED_SCALERS*NGPDPath[this_path].num_chan);
				msgBuildDesc.size_bytes=sizeof(u_int32_t)*(2+NGPD_HW_DEFINED_SCALERS*NGPDPath[this_path].num_chan) * NGPDPath[this_path].scaler_num_tf;
			}
			if (debug > 1)
			{
				printf(".... Building scaler descriptors for %d time frames, block size=%d, total=%d\n", NGPDPath[this_path].scaler_num_tf_fw, msgBuildDesc.block_size, msgBuildDesc.size_bytes);
			} else if (debug ==1 && card == first_card)
				printf(" Scalers for %d time frames", NGPDPath[this_path].scaler_num_tf);
			rc = xsp3_dma_build_desc(path, card, NGPD_DMA_STREAM_BNUM_SCALERS, &msgBuildDesc);
			if (rc != 0) {
				strcpy(err_copy, ngpd_get_error_message());
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_system_start: Error Building Scalers descriptors : %s", err_copy);
				ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
				return rc;
			}
			memset((void *)&msgStart, 0, sizeof(msgStart));
			msgStart.first_desc = NGPD_GLOB_TIMB_GET_FIXED(fixed_tf);
			if ((NGPDPath[path].run_flags & NGPD_RUN_FLAGS_CIRCULAR_BUFFER))
				msgStart.options |= NGPD_DMA_START_CIRCULAR;
			rc = xsp3_dma_start(path, card, NGPD_DMA_STREAM_BNUM_SCALERS, &msgStart);
			if (rc != 0) {
				strcpy(err_copy, ngpd_get_error_message());
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_system_start: Error Starting Scalers descriptors : %s",
						err_copy);
				return rc;
			}
		}
		if (NGPDPath[path].features.generation == XspressGen3Mini)
		{
			int this_path;

			if (NGPDPath[path].type == NGPDComposite)
				this_path = NGPDPath[path].sub_path[card];
			else
				this_path = path;
//				int nbits_eng = 12;
			if ((NGPDPath[path].run_flags & NGPD_RUN_FLAGS_HIST) && !(NGPDPath[path].readout_mode & Xsp3mRd_SendHistList))
			{
				/* Prepare DMA to store complete usually 4k spectra into DRAM  - Now allow this to be stored in Zynq in the memory layout */
				memset(&msgBuildDesc, 0, sizeof(msgBuildDesc));
				msgBuildDesc.src_stream = XSP3M_DMA_STREAM_BNUM_HIST_FRAMES;
//					msgBuildDesc.block_size= sizeof(u_int32_t)*NGPDPath[this_path].num_chan << nbits_eng;
				msgBuildDesc.size_bytes= 0;	// Use all memory
				if (debug > 1)
				{
					printf(".... Building hist frames descriptors for %d time frames, block size=%d, total=%d\n", NGPDPath[this_path].scaler_num_tf, msgBuildDesc.block_size, msgBuildDesc.size_bytes);
				} else if (debug ==1 && card == first_card)
					printf(" Hist Frames for %d time frames", NGPDPath[this_path].scaler_num_tf);
				rc = xsp3_dma_build_desc(path, card, XSP3M_DMA_STREAM_BNUM_HIST_FRAMES, &msgBuildDesc);
				if (rc != 0) {
					strcpy(err_copy, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_system_start: Error Building Hist Frames descriptors : %s",
							err_copy);
					return rc;
				}
				memset((void *)&msgStart, 0, sizeof(msgStart));
				msgStart.first_desc = NGPD_GLOB_TIMB_GET_FIXED(fixed_tf);
				if ((NGPDPath[path].run_flags & NGPD_RUN_FLAGS_CIRCULAR_BUFFER))
					msgStart.options |= NGPD_DMA_START_CIRCULAR;
				rc = xsp3_dma_start(path, card, XSP3M_DMA_STREAM_BNUM_HIST_FRAMES, &msgStart);
				if (rc != 0) {
					strcpy(err_copy, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_system_start: Error Starting Hist Frames descriptors : %s",
							err_copy);
					return rc;
				}
			}
			if ((NGPDPath[path].run_flags & NGPD_RUN_FLAGS_DIAG_HIST) || (NGPDPath[path].readout_mode & Xsp3mRd_SendHistList) )
			{
				memset(&msgBuildDesc, 0, sizeof(msgBuildDesc));
				msgBuildDesc.src_stream = XSP3M_DMA_STREAM_BNUM_HIST_LIST;
				msgBuildDesc.block_size= 0; // Use default
				msgBuildDesc.size_bytes= 0;	// Use all memory
				if ((NGPDPath[path].readout_mode & Xsp3mRd_SendHistList))
					msgBuildDesc.block_size= XSP3M_HIST_LIST_BURST_EVENTS*sizeof(u_int16_t);  // Use 4096 short word blocks for when sending by TCP

				if (debug > 1)
				{
					printf(".... Building hist list descriptors block size=%d, total=%d\n", msgBuildDesc.block_size, msgBuildDesc.size_bytes);
				} else if (debug ==1 && card == first_card)
					printf(" Hist List ");
				rc = xsp3_dma_build_desc(path, card, XSP3M_DMA_STREAM_BNUM_HIST_LIST, &msgBuildDesc);
				if (rc != 0) 
				{
					strcpy(err_copy, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_system_start: Error Building Hist List descriptors : %s",
							err_copy);
					return rc;
				}
				memset((void *)&msgStart, 0, sizeof(msgStart));
				rc = xsp3_dma_start(path, card, XSP3M_DMA_STREAM_BNUM_HIST_LIST, &msgStart);
				if (rc != 0) {
					strcpy(err_copy, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_system_start: Error Starting Hist List descriptors : %s",
							err_copy);
					return rc;
				}
			}
		}
#endif
	}
	if (debug == 1)
		printf("\n");
	else if (debug > 1)
		printf(".... Writing DATA_PATH to enable run mode\n");
	for (card = last_card; card >= first_card; card--)  // Count backwards so that If card 0 has the intenal TFG, it is started last.
	{
		u_int32_t new_dpc;
		data_path_or = NGPD_DP_RUN;
		data_path_and= 0xFFFFFFFF;
		printf("Starting system with data_path_and=%08X, data_path_or = 0x%08X\n", data_path_and, data_path_or);
		rc = ngpd_rmw_glob_regs(path, card, NGZMP_DATA_PATH, data_path_and, data_path_or, &new_dpc);
		if (rc != 0) {
			strcpy(err_copy, ngpd_get_error_message());
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error RMW DATA_DATA %s", err_copy);
			ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
			return rc;
		}
		if (debug > 1)
			printf("New dpc=%08X\n", new_dpc);
	}
	return NGPD_OK;
}

int ngzmp_system_stop(int path, int card)
{
	int rc;
	int first_card, last_card;
	char err_copy[NGPD_MAX_ERROR_MESSAGE + 2];
	u_int32_t new_dpc;
	int debug;
	u_int32_t data_path_or, data_path_and;

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) 
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_stop: invalid path %d", path);
		return NGPD_ERROR;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_stop: Called on ADQ14 system");
		return NGPD_ERROR;
	}

	if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
	{
		printf("Dummy:ngzmp_system_stop(card=%d)\n", card);
		return 0;
	}

	debug = NGPDPath[path].debug;

	if (card < 0)
	{
		first_card = 0;
		last_card = NGPDPath[path].num_cards-1;
	}
	else
	{
		first_card = card;
		last_card = card;
	}
	if (debug)
	{
		printf("Stopping system cards %d .. %d, num_cards=%d\n", first_card, last_card, NGPDPath[path].num_cards);
	}
	for (card = first_card; card <= last_card; card++) 
	{
		/* First stop and reset DMAs on all cards to stop source of further UDP/TCP data */
		data_path_or = 0;
		data_path_and= ~NGPD_DP_RUN;
		if (debug)
			printf("Stopping system with data_path_and=%08X, data_path_or = 0x%08X\n", data_path_and, data_path_or);
		rc = ngpd_rmw_glob_regs(path, card, NGZMP_DATA_PATH, data_path_and, data_path_or, &new_dpc);
		if (rc != 0) {
			strcpy(err_copy, ngpd_get_error_message());
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_stop: Error writing data path register %s", 	err_copy);
			ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
			return rc;
		}
	}
	return 0;
}

/**
 * @ingroup fullcontrol
 * This is called typically on an RX DMA Stream e.g. NGPD_DMA_STREAM_BNUM_SCALERS, NGPD_DMA_STREAM_BNUM_SCOPE0, NGPD_DMA_STREAM_BNUM_SCOPE1 or NGPD_DMA_STREAM_BNUM_10G_TO_DRAM 
 * to see how much data has been received. It returns the number of successfully, correctly completed DMA descriptors that have been received. 
 * For streams where there is a concept of time frame or similar additional information can be returned
 * @n@n
 * option=0 => do default checks for the specified stream and is usually appropriate
 * For more control set options as {@link ZYNQMP_DMA_MsgCheckDesc}
 *
 * @param path 				A handle to the top level of the ngpd system returned from {@link ngpd_config_ngzmp()}.
 * @param card 				The number of the card in the xspress3 system,
 *        					0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 * @param stream 			Stream number check descriptors for defined in {@link NGPD_DMA_STREAM_NUMBER}.
 * @param msg				Pointer to {@link ZYNQMP_DMA_MsgCheckDesc} to control which descriptors are checked and against what options.
 * @param completed_desc 	Pointer to return the number of completed descriptors including any after dropped frames
 * @param last_frame		Pointer to return the frame number in the last completed descriptor
 * @param status			Pointer to return the AXI DMA Desc status word from the first failing descriptor
 * @return 					Number of correctly completed  descriptors (often frames) or a negative error code.
 */
int ngzmp_dma_check_desc(int path, int card, u_int32_t stream, ZYNQMP_DMA_MsgCheckDesc *msg, int32_t *good_desc, u_int32_t *completed_desc, u_int32_t *last_frame, u_int32_t *status) {
	int rc = NGPD_OK;
	int this_path;
	u_int32_t details[3];

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_check_desc: invalid path %d", path);
		return NGPD_ERROR;
	}
	// Do sanity check on card number versus total in system
	if ((card < 0) || (card >= NGPDPath[path].num_cards)) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_check_desc: illegal card specified (%d)", card);
		return NGPD_ERROR;
	}

	if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
	{
		printf("Dummy: ngzmp_dma_check_desc(card=%d, stream=%d\n", card, stream);
		return 1;
	}		
	if (NGPDPath[path].type == NGPDComposite)
	{
		this_path = NGPDPath[path].sub_path[card];
		if (this_path < 0) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_check_desc: resolved illegal sub path (path=%d)", path);
			return NGPD_ERROR;
		}
	}
	else
	{
		this_path = path;
	}


	rc = zynqmp_personality_write(NGPDPath[this_path].zynqmp, ZYNQMP_DMA_CMD_CHECK_RX_DESC, ZYNQMP_WIDTH_LONG, stream, sizeof(ZYNQMP_DMA_MsgCheckDesc)/sizeof(u_int32_t), (u_int8_t *)msg, 3, details, good_desc);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_check_desc: Error checking descriptors: return code %d:%s", rc, zynqmp_get_error_message());
		return rc;
	}
	if (completed_desc != NULL) *completed_desc = details[0];
	if (last_frame != NULL) *last_frame = details[1];
	if (status != NULL) *status = details[2];
	return rc;
}

#if 0

/**
 * @ingroup fullcontrol
 * This is called  on the RX DMA Streams NGPD_DMA_STREAM_BNUM_SCALERS, and XSP3M_DMA_STREAM_BNUM_HIST_FRAMES on Xspress3mini to monitor progress in circular buffer mode
 * It returns the number of successfully, correctly completed DMA descriptors that have been received. 
 * For NGPD_DMA_STREAM_BNUM_SCALERS this is the number of time frames of scalers that have been received.
 * The preferred method to monitor progress is to call {@link xsp3_scaler_check_progress_details} which uses this function for systems where appropriate.
 * @n@n
 *
 * For the scalar and BRAM histogrammer spectra, if the frame rate is too high some frames get dropped, but the DMA may continue. 
 * The number of correctly completed descriptors is returned. 
 * The total number of completed descriptors (including descriptors after any dropped frames) is returned in completed_desc
 * The frame number reached in the last descriptor is stored in last_frame.
 * @n
 * @param path 				A handle to the top level of the xspress3 system returned from {@link ngpd_config_ngzmp()}.
 * @param card 				The number of the card in the xspress3 system,
 *        					0 for a single card system and up to ({@link xsp3_get_num_cards()} - 1) for a multi-card system,
 *        					where this value has been passed to {@link ngpd_config_ngzmp()} at xspress3 system configuration.
 * @param stream 			Stream number check descriptors for defined in {@link NGPD_DMA_STREAM_NUMBER}.
 * @param completed_desc 	Pointer to return the number of completed descriptors including any after dropped frames (XSPRESS3Mini adn XSPRESS4 only)
 * @param last_frame		Pointer to return the frame number in the last completed descriptor (XSPRESS3Mini and XSPRESS4 only)
 * @param dma_status		Pointer to return the AXI DMA core status word.
 * @param desc_status		Pointer to return the AXI DMA Desc status word from the first failing descriptor.
 * @param desc_frame		Pointer to return the Time frame in the failing (or last_ descriptor)
 * @return 					Number of correctly completed frames or a negative error code.
 */
int64_t xsp3m_dma_check_desc(int path, int card, u_int32_t stream, u_int64_t *completed_desc, u_int64_t *last_frame, u_int32_t *dma_status, u_int32_t *desc_status, u_int64_t *desc_frame) 
{
	int rc = NGPD_OK;
	int this_path;
	u_int32_t sub_command = NGPD_DMA_CMD_CHECK_RX_DESC_CIRCULAR;
	NGPD_DMA_CheckCircular details;
	int size = sizeof(NGPD_DMA_CheckCircular) / sizeof(u_int32_t);

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3m_dma_check_desc: invalid path %d", path);
		return NGPD_INVALID_PATH;
	}
	// Do sanity check on card number versus total in system
	if ((card < 0) || (card >= NGPDPath[path].num_cards)) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3m_dma_check_desc: illegal card specified (%d)", card);
		return NGPD_ILLEGAL_CARD;
	}
	if (NGPDPath[path].features.generation != XspressGen3Mini)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3m_dma_check_desc: Only supported on Xspress3mini");
		return NGPD_ERROR;
	}
		
	if (stream != NGPD_DMA_STREAM_BNUM_SCALERS && stream != XSP3M_DMA_STREAM_BNUM_HIST_FRAMES) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3m_dma_check_desc: invalid dma stream number %d", stream);
		return NGPD_INVALID_DMA_STREAM;
	}
	if (NGPDPath[path].type == NGPDComposite) {
		this_path = NGPDPath[path].sub_path[card];
		if (this_path < 0) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3m_dma_check_desc: resolved illegal sub path (path=%d)", path);
			return NGPD_ILLEGAL_SUBPATH;
		}
	} else {
		this_path = path;
	}

	rc = xspress3FemPersonalityRead(NGPDPath[this_path].femHandle, sub_command, (1 << stream), sizeof(u_int32_t), 0, size, &details);
	if (rc < 0) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3m_dma_check_desc: Error writing stream 0x%x: return code %d",
				stream, rc);
		return rc;
	}
	if (completed_desc != NULL) *completed_desc = details.num_completed;
	if (last_frame != NULL) *last_frame   = details.last_tf;
	if (dma_status != NULL) *dma_status   = details.dma_status;
	if (desc_status != NULL) *desc_status = details.desc_status;
	if (desc_frame != NULL) *desc_frame   = details.desc_tf;

	return (int64_t)details.num_good;
}


/**
 * @ingroup debug
 * Setup the number of debug messages printed to the serial ports on the FEM directly from the embedded PowerPCs.
 * To view this data it is necessary to connect to the headers on the FEM.
 * Verbose messages slow down the system very significantly, so should only be enabled for detailed debugging.
 * 
 * For XSPRESS3Mini and XSPRESS4, the PPC2 message now controls the messages from xspress4d, which can be started in the foreground of an ssh session.
 * the PPC1 message control the messages sent to printk in the driver and visible on the console.
 *
 * @param path a handle to the top level of the xspress3 system returned from {@link ngpd_config_ngzmp()}.
 * @param card is the number of the card in the xspress3 system,
 *        0 for a single card system and up to ({@link xsp3_get_num_cards()} - 1) for a multi-card system,
 *        where this value has been passed to {@link ngpd_config_ngzmp()} at xspress3 system configuration.
 * @param ppc1		Flag to enable setting message level for PPC1 or Zynq driver printk
 * @param ppc2		Flag to enable setting message level for PPC2 or Zynq xspress4d
 * @param level		Message level to set
 * @return {@link #NGPD_OK NGPD_OK} or a negative error code.
 */
int xsp3_set_ppc_debuglevel(int path, int card, int ppc1, int ppc2, int level) {
	int rc = NGPD_OK;
	int first_card, last_card;
	int this_path;

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_set_ppc_debuglevel: invalid path %d", path);
		return NGPD_INVALID_PATH;
	}
	if (card < 0) {
		first_card = 0;
		last_card = NGPDPath[path].num_cards - 1;
	} else {
		if (card >= NGPDPath[path].num_cards) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_set_ppc_debuglevel: invalid card %d", card);
			return NGPD_ILLEGAL_CARD;
		}
		first_card = card;
		last_card = card;
	}

	for (card = first_card; card <= last_card; card++) {
		if (NGPDPath[path].type == NGPDComposite) 
		{
			this_path = NGPDPath[path].sub_path[card];
			if (this_path < 0) {
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_dma_check_desc: resolved illegal sub path (path=%d)", path);
				return NGPD_ILLEGAL_SUBPATH;
			}
		}
		else
			this_path = path;

		if (ppc1 == 1) {
			rc = xspress3FemPersonalityWrite(NGPDPath[this_path].femHandle, NGPD_DMA_CMD_SET_MSG_PPC1, level, 0, NULL, 0, NULL);
			if (rc < 0) {
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE,
						"xsp3_set_ppc_debuglevel: Error writing debug level %d: return code %d", level, rc);
				return rc;
			}
		}
		if (ppc2 == 1) {
			rc = xspress3FemPersonalityWrite(NGPDPath[this_path].femHandle, NGPD_DMA_CMD_SET_MSG_PPC2, level, 0, NULL, 0, NULL);
			if (rc < 0) {
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE,
						"xsp3_set_ppc_debuglevel: Error writing debug level %d: return code %d", level, rc);
				return rc;
			}
		}
	}
	return rc;
}
#endif
/**
 * @ingroup internal
 * This function polls the DMA descriptors for the number of scope DMA engines in use to check whether all the data has been transferred.
 * It is called from {@link ngpd_wait_scope()} to allow a common API
 * @param path 	A handle to the top level of the xspress3 system returned from {@link ngpd_config_ngzmp()}.
 * @param card 	The number of the card in the xspress3 system,
 *       		0 for a single card system and up to ({@link xsp3_get_num_cards()} - 1) for a multi-card system,
 *        		where this value has been passed to {@link ngpd_config_ngzmp()} at xspress3 system configuration.
 *        		If card <0 the function waits until all cards in the system have finished.
 * @return {@link #NGPD_OK NGPD_OK} or a negative error code.
 */
int ngzmp_dma_wait_scope(int path, int card, u_int32_t *statusP)
{
	char lower_msg[NGPD_MAX_ERROR_MESSAGE+2];
	ZYNQMP_DMA_StatusBlock status_block;
	ZYNQMP_DMA_MsgCheckDesc msg;
	int first_card, last_card;
	int stream;
	u_int32_t good_desc, completed_desc, status;
   	struct timespec delay, remaining;
	int timeout;
	int rc;
	int num_scope_dma;
	delay.tv_sec = 0;        /* seconds */
  	delay.tv_nsec = 100000000;       /* nanoseconds */
	u_int32_t scope_cont, scope_status;

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_wait_scope: invalid path %d", path);
		return NGPD_ERROR;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
	{
		printf("Dummy: ngzmp_dma_wait_scope(card=%d)\n", card);
#ifdef _MSC_VER
		Sleep(2000);
#else
		sleep (2);
#endif
		return 0;
	}		

	if (card < 0) {
		first_card = 0;
		last_card = NGPDPath[path].num_cards - 1;
	} else {
		if (card >= NGPDPath[path].num_cards) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_wait_scope: invalid card %d", card);
			return NGPD_ERROR;
		}
		first_card = card;
		last_card = card;
	}

	memset(&msg, 0, sizeof(ZYNQMP_DMA_MsgCheckDesc));

	for (card=first_card; card<=last_card; card++)
	{
		rc = ngpd_read_glob_regs(path, card, NGZMP_SCOPE_CONT, 1, &scope_cont);
		num_scope_dma = NGZMP_SCOPE_CONT_GET_NUM_STREAMS(scope_cont)+1;

		rc = ngzmp_get_dma_status_block(path, card, &status_block);
		if (rc != 0) {
			strcpy(lower_msg, ngpd_get_error_message());
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_wait_scope: Cannot get status block returns: %s", lower_msg);
			ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
			return rc;
		}
		for (stream=NGZMP_DMA_STREAM_BNUM_SCOPE0; stream < NGZMP_DMA_STREAM_BNUM_SCOPE0+num_scope_dma; stream++)
		{
			for(timeout=50; timeout> 0; timeout--)
			{
				// (status_block.stream_def[stream].data_size / 4) * 2;
				rc = ngzmp_dma_check_desc(path, card, stream, &msg, &good_desc, &completed_desc, NULL, &status);
				if (NGPDPath[path].debug)
					printf("Card %d: Done %d of %d, status=%08X\n", card, good_desc, status_block.stream_def[stream].defined_desc, status);
				if (rc < 0)
				{
					strcpy(lower_msg, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_wait_scope: ngzmp_dma_check_desc returns error: %s", lower_msg);
					ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
					return rc;
				}
				else if (good_desc == status_block.stream_def[stream].defined_desc)
					break;
#ifdef _MSC_VER
				Sleep(100);
#else
				remaining= delay;
				while (nanosleep(&remaining, &remaining) == -1 && errno == EINTR);
#endif
			}
			if (good_desc != status_block.stream_def[stream].defined_desc)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_wait_scope: Timeout waiting for DMA stream %d, path=%d, card=%d, good %d out of %d, completed=%d, status=%08X", 
						stream, path, card, good_desc, status_block.stream_def[stream].defined_desc, completed_desc, status);
				if (statusP != NULL)
					*statusP = 0;
				return NGPD_ERROR;
			}
		}
	}
	scope_status = 0;
	if (statusP != NULL)
	{
		for (card=first_card; card<=last_card; card++)
		{
			rc = ngpd_read_glob_regs(path, card, NGZMP_SCOPE_STATUS, 1, &status);
			scope_status |= (status & (NGZMP_SCOPE_STAT_SCOPE_RUNNING | NGZMP_SCOPE_STAT_SCOPE_OVERUN | NGZMP_SCOPE_STAT_PB_UNDERUN));
		}
		*statusP = scope_status;
	}
	return 0;
}
#if 0
/**
 * @ingroup internal
 * Instruct Zynq in XSPRESS3Mini or XSPRESS4 to read the status words from  a DMA descriptor
 * @n@n
 * The following describes the layout of the structure.
 * @snippet xspress3_dma_protocol.h NGPD_DMA_MSG_GET_DESC_STATUS
 *
 * @param path 		A handle to the top level of the xspress3 system returned from {@link ngpd_config_ngzmp()}.
 * @param card 		The number of the card in the xspress3 system,
 *        			0 for a single card system and up to ({@link xsp3_get_num_cards()} - 1) for a multi-card system,
 *        			where this value has been passed to {@link ngpd_config_ngzmp()} at xspress3 system configuration.
 * @param stream 	Stream number to build descriptors for defined in {@link NGPD_DMA_STREAM_NUMBER}.
 * @param msg		Pointer to the message of type NGPD_DMA_MsgBuildDesc.
 * @param status	Pointer ot return the status word returned by the Zynq DMA code.
 * @return {@link #NGPD_OK NGPD_OK} or a negative error code.
 */
int xsp3_dma_get_desc_status(int path, int card, u_int32_t stream, NGPD_DMA_MsgGetDescStatus *msg, u_int32_t *status) {
	int rc = NGPD_OK;
	int this_path;
	int size = sizeof(NGPD_DMA_MsgGetDescStatus) / sizeof(u_int32_t);
	u_int32_t sub_command = NGPD_DMA_CMD_GET_DESC_STATUS;

	if (path < 0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_dma_get_desc_status: invalid path %d", path);
		return NGPD_INVALID_PATH;
	}
	// Do sanity check on card number versus total in system
	if ((card < 0) || (card >= NGPDPath[path].num_cards)) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_dma_get_desc_status: illegal card specified (%d)", card);
		return NGPD_ILLEGAL_CARD;
	}

	if (NGPDPath[path].type == NGPDComposite) {
		this_path = NGPDPath[path].sub_path[card];
		if (this_path < 0) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_dma_get_desc_status: resolved illegal sub path (path=%d)", path);
			return NGPD_ILLEGAL_SUBPATH;
		}
	} else {
		this_path = path;
	}

	if (stream < 0 || stream > NGPDPath[path].features.max_real_dma_stream) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_dma_get_desc_status: invalid dma stream number %d", stream);
		return NGPD_INVALID_DMA_STREAM;
	}

	rc = xspress3FemPersonalityWrite(NGPDPath[this_path].femHandle, sub_command, (1 << stream), size, (u_int32_t*) msg, 1, status);
	return rc;
}
#endif

