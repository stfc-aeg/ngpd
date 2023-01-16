/*
 * ngzmp_dma.c
 *
 *  Created on: 18/6/2021
 *      Author: wih73
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "ngpd.h"
#include "ngzmptest.h"

extern char ngpd_error_message[];
extern int manual_test;


int ngzmp_system_start_count_enb(int path, int card, int num_pb_streams, int num_streams, int run_flags, int pb_num_t)
{
	int rc;
	int first_card, last_card;
	u_int32_t stream_mask, pb_cont;
	ZYNQMP_DMA_MsgBuildDesc msgBuildDesc;
	char err_copy[NGPD_MAX_ERROR_MESSAGE + 2];
	u_int32_t dpc = 0; /* may change if want to send Hist data via SDRAM */
	u_int32_t num_lwords, scope_cont, num_scope_words;
	ZYNQMP_DMA_MsgStart msgStart;
	int debug;
	int scope, num_scope;
	int num_cards=1;
	u_int32_t data_path_or, data_path_and;

	debug = 1 ; // Xsp3Sys[path].debug;

	first_card = 0;
	last_card = 0;
	if (debug)
	{
		printf("Starting system cards %d .. %d, num_cards=%d with:", first_card, last_card, /* Xsp3Sys[path]. */ num_cards);
		if (/*Xsp3Sys[path]*/ run_flags & NGPD_RUN_FLAGS_PLAYBACK)
			printf(" Playback");
		if (/*Xsp3Sys[path].*/ run_flags & NGPD_RUN_FLAGS_SCOPEMODE)
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
	if (manual_test)
	{
		printf("\n....Finished reset, press return to build descriptors and start DMAs\n");
		getchar();
	}
	for (card = first_card; card <= last_card; card++) 
	{
		if (/*Xsp3Sys[path]. */ run_flags & NGPD_RUN_FLAGS_PLAYBACK)
		{
			if (debug > 1)
				printf(".... Configuring playback DMA\n");

			memset(&msgBuildDesc, 0, sizeof(msgBuildDesc));
			msgBuildDesc.src_stream = NGZMP_DMA_STREAM_BNUM_PLAYBACK0;
			if (pb_num_t > 0)
			{
				/* For test cases limit the size of the playback data */
				msgBuildDesc.size_bytes = 2*num_pb_streams*pb_num_t;
			}
			rc = ngzmp_dma_build_desc(path, card, NGZMP_DMA_STREAM_BNUM_PLAYBACK0, &msgBuildDesc, NULL);
			if (rc != 0) {
				strcpy(err_copy, ngpd_get_error_message());
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error Building Playback descriptors %s", err_copy);
				ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
				return rc;
			}
			switch (num_pb_streams)
			{
			case 1:	pb_cont = NGZMP_PLAYBACK_ENABLE | NGZMP_PLAYBACK_NUM_STREAMS(0); break;
			case 2:	pb_cont = NGZMP_PLAYBACK_ENABLE | NGZMP_PLAYBACK_NUM_STREAMS(1); break;
			case 4:	pb_cont = NGZMP_PLAYBACK_ENABLE | NGZMP_PLAYBACK_NUM_STREAMS(2); break;
			case 8:	pb_cont = NGZMP_PLAYBACK_ENABLE | NGZMP_PLAYBACK_NUM_STREAMS(3); break;
			default:
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Unsupported number of playback stream %d", num_pb_streams);
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
		if (/*Xsp3Sys[path].*/ run_flags & NGPD_RUN_FLAGS_SCOPEMODE) 
		{
			if (debug > 1)
				printf(".... Configuring Scope mode\n");

			ZYNQMP_DMA_StatusBlock statusBlock;
			rc = ngzmp_get_dma_status_block(path, card, &statusBlock);
			if (rc != 0) {
				strcpy(err_copy, ngpd_get_error_message());
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error Reading Status Block %s", err_copy);
				ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
				return rc;
			}

			rc = ngpd_read_glob_regs(path, card, NGZMP_SCOPE_CONT, 1, &scope_cont);

			switch (num_streams)
			{
			case 4:
				scope_cont = NGZMP_SCOPE_CONT_NUM_STREAMS(0);
				num_scope = 1;
				num_scope_words = statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0].data_size/(16);
				break;

			case 8:
				scope_cont = NGZMP_SCOPE_CONT_NUM_STREAMS(1);
				num_scope = 2;
				num_scope_words = statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0].data_size/(16*2);
				break;

			case 10:
				scope_cont = NGZMP_SCOPE_CONT_NUM_STREAMS(2);
				num_scope = 3;
				num_scope_words = statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0].data_size/(40);
				break;

			default:
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Unsupported number of scopemode stream %d", num_streams);
				return -1;
			}
			num_scope_words	&= 0xFFFFFFFE;		// force 64 byte alignment to mix with descriptors and cachelines.
			if ((rc = ngpd_write_glob_regs(path, card, NGZMP_SCOPE_CONT, 1, &scope_cont)) < 0 ||  (rc= ngpd_write_glob_regs(path, card, NGZMP_SCOPE_NUMWORDS, 1, &num_scope_words)) < 0 )
			{
				strcpy(err_copy, ngpd_get_error_message());
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error writing Scope Control %s", err_copy);
				ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
				return rc;
			}
			printf("... Total scope data size=%ld, num_scope_words=%d\n", statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0].data_size, num_scope_words);
			stream_mask = 0;
			for (scope=0;scope<num_scope; scope++)
			{
				memset(&msgBuildDesc, 0, sizeof(msgBuildDesc));
				msgBuildDesc.src_stream = NGZMP_DMA_STREAM_BNUM_SCOPE0;	// Caclculate all byte sizes and offsets relative to stream 0
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
			num_lwords = statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0].data_size / 4;

/*			if (Xsp3Sys[path].total_lwords_per_card != num_lwords * Xsp3Sys[path].features.num_scope_dma)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_system_start: Number of Lwords has changed since configuration. For Xsp3Sys=%d, From DMA status block=%d",
						Xsp3Sys[path].total_lwords_per_card, num_lwords * Xsp3Sys[path].features.num_scope_dma);
				return XSP3_ERROR;
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
		}
		/* When finished, also need to prepare scalers DMA Here */
#if 0
		if ( ((Xsp3Sys[path].run_flags & XSP3_RUN_FLAGS_SCALERS) && !Xsp3Sys[path].features.soft_scalers)  || 
			(Xsp3Sys[path].features.generation == XspressGen3Mini && (Xsp3Sys[path].run_flags & XSP3_RUN_FLAGS_HIST) && !(Xsp3Sys[path].readout_mode & Xsp3mRd_SendHistList)) )
		{

			rc = ngpd_read_glob_regs(path, card, XSP4_GLOB_TIMING_FIXED, 1, &fixed_tf);
			if (rc != 0) {
				strcpy(err_copy, ngpd_get_error_message());
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error reading Global Fixed Time Frame reg : %s", err_copy);
				ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
				return rc;
			}
		}
		if ((Xsp3Sys[path].run_flags & XSP3_RUN_FLAGS_SCALERS) && !Xsp3Sys[path].features.soft_scalers) 
		{
			int thisPath;

			if (Xsp3Sys[path].type == FEM_COMPOSITE)
				thisPath = Xsp3Sys[path].sub_path[card];
			else
				thisPath = path;

			memset(&msgBuildDesc, 0, sizeof(msgBuildDesc));
			msgBuildDesc.src_stream = XSP3_DMA_STREAM_BNUM_SCALERS;
			if (Xsp3Sys[path].features.generation == XspressGen3Mini)
			{
				msgBuildDesc.block_size= sizeof(u_int32_t)*(XSP3M_HW_NUM_SCALERS*Xsp3Sys[thisPath].num_chan);
				msgBuildDesc.size_bytes=sizeof(u_int32_t)*(XSP3M_HW_NUM_SCALERS*Xsp3Sys[thisPath].num_chan) * Xsp3Sys[thisPath].scaler_num_tf_fw;
			}
			else
			{
				msgBuildDesc.block_size= sizeof(u_int32_t)*(2+XSP3_HW_DEFINED_SCALERS*Xsp3Sys[thisPath].num_chan);
				msgBuildDesc.size_bytes=sizeof(u_int32_t)*(2+XSP3_HW_DEFINED_SCALERS*Xsp3Sys[thisPath].num_chan) * Xsp3Sys[thisPath].scaler_num_tf;
			}
			if (debug > 1)
			{
				printf(".... Building scaler descriptors for %d time frames, block size=%d, total=%d\n", Xsp3Sys[thisPath].scaler_num_tf_fw, msgBuildDesc.block_size, msgBuildDesc.size_bytes);
			} else if (debug ==1 && card == first_card)
				printf(" Scalers for %d time frames", Xsp3Sys[thisPath].scaler_num_tf);
			rc = xsp3_dma_build_desc(path, card, XSP3_DMA_STREAM_BNUM_SCALERS, &msgBuildDesc, NULL);
			if (rc != 0) {
				strcpy(err_copy, ngpd_get_error_message());
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_system_start: Error Building Scalers descriptors : %s", err_copy);
				ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
				return rc;
			}
			memset((void *)&msgStart, 0, sizeof(msgStart));
			msgStart.first_desc = XSP3_GLOB_TIMB_GET_FIXED(fixed_tf);
			if ((Xsp3Sys[path].run_flags & XSP3_RUN_FLAGS_CIRCULAR_BUFFER))
				msgStart.options |= XSP3_DMA_START_CIRCULAR;
			rc = xsp3_dma_start(path, card, XSP3_DMA_STREAM_BNUM_SCALERS, &msgStart);
			if (rc != 0) {
				strcpy(err_copy, ngpd_get_error_message());
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_system_start: Error Starting Scalers descriptors : %s",
						err_copy);
				return rc;
			}
		}
		if (Xsp3Sys[path].features.generation == XspressGen3Mini)
		{
			int thisPath;

			if (Xsp3Sys[path].type == FEM_COMPOSITE)
				thisPath = Xsp3Sys[path].sub_path[card];
			else
				thisPath = path;
//				int nbits_eng = 12;
			if ((Xsp3Sys[path].run_flags & XSP3_RUN_FLAGS_HIST) && !(Xsp3Sys[path].readout_mode & Xsp3mRd_SendHistList))
			{
				/* Prepare DMA to store complete usually 4k spectra into DRAM  - Now allow this to be stored in Zynq in the memory layout */
				memset(&msgBuildDesc, 0, sizeof(msgBuildDesc));
				msgBuildDesc.src_stream = XSP3M_DMA_STREAM_BNUM_HIST_FRAMES;
//					msgBuildDesc.block_size= sizeof(u_int32_t)*Xsp3Sys[thisPath].num_chan << nbits_eng;
				msgBuildDesc.size_bytes= 0;	// Use all memory
				if (debug > 1)
				{
					printf(".... Building hist frames descriptors for %d time frames, block size=%d, total=%d\n", Xsp3Sys[thisPath].scaler_num_tf, msgBuildDesc.block_size, msgBuildDesc.size_bytes);
				} else if (debug ==1 && card == first_card)
					printf(" Hist Frames for %d time frames", Xsp3Sys[thisPath].scaler_num_tf);
				rc = xsp3_dma_build_desc(path, card, XSP3M_DMA_STREAM_BNUM_HIST_FRAMES, &msgBuildDesc, NULL);
				if (rc != 0) {
					strcpy(err_copy, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_system_start: Error Building Hist Frames descriptors : %s",
							err_copy);
					return rc;
				}
				memset((void *)&msgStart, 0, sizeof(msgStart));
				msgStart.first_desc = XSP3_GLOB_TIMB_GET_FIXED(fixed_tf);
				if ((Xsp3Sys[path].run_flags & XSP3_RUN_FLAGS_CIRCULAR_BUFFER))
					msgStart.options |= XSP3_DMA_START_CIRCULAR;
				rc = xsp3_dma_start(path, card, XSP3M_DMA_STREAM_BNUM_HIST_FRAMES, &msgStart);
				if (rc != 0) {
					strcpy(err_copy, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_system_start: Error Starting Hist Frames descriptors : %s",
							err_copy);
					return rc;
				}
			}
			if ((Xsp3Sys[path].run_flags & XSP3_RUN_FLAGS_DIAG_HIST) || (Xsp3Sys[path].readout_mode & Xsp3mRd_SendHistList) )
			{
				memset(&msgBuildDesc, 0, sizeof(msgBuildDesc));
				msgBuildDesc.src_stream = XSP3M_DMA_STREAM_BNUM_HIST_LIST;
				msgBuildDesc.block_size= 0; // Use default
				msgBuildDesc.size_bytes= 0;	// Use all memory
				if ((Xsp3Sys[path].readout_mode & Xsp3mRd_SendHistList))
					msgBuildDesc.block_size= XSP3M_HIST_LIST_BURST_EVENTS*sizeof(u_int16_t);  // Use 4096 short word blocks for when sending by TCP

				if (debug > 1)
				{
					printf(".... Building hist list descriptors block size=%d, total=%d\n", msgBuildDesc.block_size, msgBuildDesc.size_bytes);
				} else if (debug ==1 && card == first_card)
					printf(" Hist List ");
				rc = xsp3_dma_build_desc(path, card, XSP3M_DMA_STREAM_BNUM_HIST_LIST, &msgBuildDesc, NULL);
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
		data_path_or = NGPD_DP_RUN;
		data_path_and= 0xFFFFFFFF;
		printf("Starting system with data_path_and=%08X, data_path_or = 0x%08X\n", data_path_and, data_path_or);
		rc = ngpd_rmw_glob_regs(path, card, NGZMP_DATA_PATH, data_path_and, data_path_or, NULL);
		if (rc != 0) {
			strcpy(err_copy, ngpd_get_error_message());
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_system_start_count_enb: Error RMW DATA_DATA %s", err_copy);
			ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
			return rc;
		}
	}
	return NGPD_OK;
}


#if 0
/**
 * @ingroup fullcontrol
 * This is called typically on an RX DMA Stream e.g. XSP3_DMA_STREAM_BNUM_SCALERS, XSP3_DMA_STREAM_BNUM_SCOPE0, XSP3_DMA_STREAM_BNUM_SCOPE1 or XSP3_DMA_STREAM_BNUM_10G_TO_DRAM 
 * to see how much data has been received. It returns the number of successfully, correctly completed DMA descriptors that have been received. 
 * For XSP3_DMA_STREAM_BNUM_SCALERS this is the number of time frames of scalers that have been received.
 * The preferred method to monitor progress is to call {@link xsp3_scaler_check_progress_details} which uses this function for systems where appropriate.
 * @n@n
 * The following describes the layout of the structure.
 * @snippet xspress3_dma_protocol.h XSP3_DMA_MSG_CHECK_DESC
 *
 * option=0 => do default checks for the specified stream and is usually appropriate
 * For more control set options as:
 * @snippet xspress3_dma_protocol.h XSP3_DMA_MSG_CHECK_DESC
 *
 * For XSPRESS3Mini and XSPRESS4 more information is available. For the scalar and BRAM histogrammer spectra, if the frame rate is too high
 * some frames get dropped, but the DMA continues. The number of correctly completed descriptors is returned. 
 * The total number of completed descriptors (including descriptors after any dropped frames) is returned in completed_desc
 * The frame number reached in the last descriptor is stored in last_frame.
 * @n
 * @param path 				A handle to the top level of the xspress3 system returned from {@link xsp3_config()}.
 * @param card 				The number of the card in the xspress3 system,
 *        					0 for a single card system and up to ({@link xsp3_get_num_cards()} - 1) for a multi-card system,
 *        					where this value has been passed to {@link xsp3_config()} at xspress3 system configuration.
 * @param stream 			Stream number check descriptors for defined in {@link XSP3_DMA_STREAM_NUMBER}.
 * @param msg				Pointer to {@link XSP3_DMA_MsgCheckDesc} to control which descriptors are checked and against what options.
 * @param completed_desc 	Pointer to return the number of completed descriptors including any after dropped frames (XSPRESS3Mini adn XSPRESS4 only)
 * @param last_frame		Pointer to return the frame number in the last completed descriptor (XSPRESS3Mini adn XSPRESS4 only)
 * @param status			Pointer to return the AXI DMA Desc status word from the first failing descriptor
 * @return Number of correctly completed  descriptors (often frames) or a negative error code.
 */
int xsp3_dma_check_desc(int path, int card, u_int32_t stream, XSP3_DMA_MsgCheckDesc *msg, u_int32_t *completed_desc, u_int32_t *last_frame, u_int32_t *status) {
	int rc = XSP3_OK;
	int thisPath;
	u_int32_t sub_command = XSP3_DMA_CMD_CHECK_RX_DESC;
	int size = sizeof(XSP3_DMA_MsgCheckDesc) / sizeof(u_int32_t);
	u_int32_t details[3];

	if (path < 0 || path >= XSP3_MAX_PATH || !Xsp3Sys[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_dma_check_desc: invalid path %d", path);
		return XSP3_INVALID_PATH;
	}
	// Do sanity check on card number versus total in system
	if ((card < 0) || (card >= Xsp3Sys[path].num_cards)) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_dma_check_desc: illegal card specified (%d)", card);
		return XSP3_ILLEGAL_CARD;
	}

	if (Xsp3Sys[path].type == FEM_COMPOSITE) {
		thisPath = Xsp3Sys[path].sub_path[card];
		if (thisPath < 0) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_dma_check_desc: resolved illegal sub path (path=%d)", path);
			return XSP3_ILLEGAL_SUBPATH;
		}
	} else {
		thisPath = path;
	}

	if (stream < 0 || stream > Xsp3Sys[path].features.max_real_dma_stream) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_dma_check_desc: invalid dma stream number %d", stream);
		return XSP3_INVALID_DMA_STREAM;
	}

	rc = xspress3FemPersonalityWrite(Xsp3Sys[thisPath].femHandle, sub_command, (1 << stream), size, (u_int32_t*) msg, 3, details);
	if (rc < 0) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_dma_check_desc: Error writing stream 0x%x: return code %d",
				stream, rc);
		return rc;
	}
	if (completed_desc != NULL) *completed_desc = details[0];
	if (last_frame != NULL) *last_frame = details[1];
	if (status != NULL) *status = details[2];
	return rc;
}


/**
 * @ingroup fullcontrol
 * This is called  on the RX DMA Streams XSP3_DMA_STREAM_BNUM_SCALERS, and XSP3M_DMA_STREAM_BNUM_HIST_FRAMES on Xspress3mini to monitor progress in circular buffer mode
 * It returns the number of successfully, correctly completed DMA descriptors that have been received. 
 * For XSP3_DMA_STREAM_BNUM_SCALERS this is the number of time frames of scalers that have been received.
 * The preferred method to monitor progress is to call {@link xsp3_scaler_check_progress_details} which uses this function for systems where appropriate.
 * @n@n
 *
 * For the scalar and BRAM histogrammer spectra, if the frame rate is too high some frames get dropped, but the DMA may continue. 
 * The number of correctly completed descriptors is returned. 
 * The total number of completed descriptors (including descriptors after any dropped frames) is returned in completed_desc
 * The frame number reached in the last descriptor is stored in last_frame.
 * @n
 * @param path 				A handle to the top level of the xspress3 system returned from {@link xsp3_config()}.
 * @param card 				The number of the card in the xspress3 system,
 *        					0 for a single card system and up to ({@link xsp3_get_num_cards()} - 1) for a multi-card system,
 *        					where this value has been passed to {@link xsp3_config()} at xspress3 system configuration.
 * @param stream 			Stream number check descriptors for defined in {@link XSP3_DMA_STREAM_NUMBER}.
 * @param completed_desc 	Pointer to return the number of completed descriptors including any after dropped frames (XSPRESS3Mini adn XSPRESS4 only)
 * @param last_frame		Pointer to return the frame number in the last completed descriptor (XSPRESS3Mini and XSPRESS4 only)
 * @param dma_status		Pointer to return the AXI DMA core status word.
 * @param desc_status		Pointer to return the AXI DMA Desc status word from the first failing descriptor.
 * @param desc_frame		Pointer to return the Time frame in the failing (or last_ descriptor)
 * @return 					Number of correctly completed frames or a negative error code.
 */
int64_t xsp3m_dma_check_desc(int path, int card, u_int32_t stream, u_int64_t *completed_desc, u_int64_t *last_frame, u_int32_t *dma_status, u_int32_t *desc_status, u_int64_t *desc_frame) 
{
	int rc = XSP3_OK;
	int thisPath;
	u_int32_t sub_command = XSP3_DMA_CMD_CHECK_RX_DESC_CIRCULAR;
	XSP3_DMA_CheckCircular details;
	int size = sizeof(XSP3_DMA_CheckCircular) / sizeof(u_int32_t);

	if (path < 0 || path >= XSP3_MAX_PATH || !Xsp3Sys[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3m_dma_check_desc: invalid path %d", path);
		return XSP3_INVALID_PATH;
	}
	// Do sanity check on card number versus total in system
	if ((card < 0) || (card >= Xsp3Sys[path].num_cards)) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3m_dma_check_desc: illegal card specified (%d)", card);
		return XSP3_ILLEGAL_CARD;
	}
	if (Xsp3Sys[path].features.generation != XspressGen3Mini)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3m_dma_check_desc: Only supported on Xspress3mini");
		return XSP3_ERROR;
	}
		
	if (stream != XSP3_DMA_STREAM_BNUM_SCALERS && stream != XSP3M_DMA_STREAM_BNUM_HIST_FRAMES) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3m_dma_check_desc: invalid dma stream number %d", stream);
		return XSP3_INVALID_DMA_STREAM;
	}
	if (Xsp3Sys[path].type == FEM_COMPOSITE) {
		thisPath = Xsp3Sys[path].sub_path[card];
		if (thisPath < 0) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3m_dma_check_desc: resolved illegal sub path (path=%d)", path);
			return XSP3_ILLEGAL_SUBPATH;
		}
	} else {
		thisPath = path;
	}

	rc = xspress3FemPersonalityRead(Xsp3Sys[thisPath].femHandle, sub_command, (1 << stream), sizeof(u_int32_t), 0, size, &details);
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
 * @param path a handle to the top level of the xspress3 system returned from {@link xsp3_config()}.
 * @param card is the number of the card in the xspress3 system,
 *        0 for a single card system and up to ({@link xsp3_get_num_cards()} - 1) for a multi-card system,
 *        where this value has been passed to {@link xsp3_config()} at xspress3 system configuration.
 * @param ppc1		Flag to enable setting message level for PPC1 or Zynq driver printk
 * @param ppc2		Flag to enable setting message level for PPC2 or Zynq xspress4d
 * @param level		Message level to set
 * @return {@link #XSP3_OK XSP3_OK} or a negative error code.
 */
int xsp3_set_ppc_debuglevel(int path, int card, int ppc1, int ppc2, int level) {
	int rc = XSP3_OK;
	int first_card, last_card;
	int thisPath;

	if (path < 0 || path >= XSP3_MAX_PATH || !Xsp3Sys[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_set_ppc_debuglevel: invalid path %d", path);
		return XSP3_INVALID_PATH;
	}
	if (card < 0) {
		first_card = 0;
		last_card = Xsp3Sys[path].num_cards - 1;
	} else {
		if (card >= Xsp3Sys[path].num_cards) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_set_ppc_debuglevel: invalid card %d", card);
			return XSP3_ILLEGAL_CARD;
		}
		first_card = card;
		last_card = card;
	}

	for (card = first_card; card <= last_card; card++) {
		if (Xsp3Sys[path].type == FEM_COMPOSITE) 
		{
			thisPath = Xsp3Sys[path].sub_path[card];
			if (thisPath < 0) {
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_dma_check_desc: resolved illegal sub path (path=%d)", path);
				return XSP3_ILLEGAL_SUBPATH;
			}
		}
		else
			thisPath = path;

		if (ppc1 == 1) {
			rc = xspress3FemPersonalityWrite(Xsp3Sys[thisPath].femHandle, XSP3_DMA_CMD_SET_MSG_PPC1, level, 0, NULL, 0, NULL);
			if (rc < 0) {
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE,
						"xsp3_set_ppc_debuglevel: Error writing debug level %d: return code %d", level, rc);
				return rc;
			}
		}
		if (ppc2 == 1) {
			rc = xspress3FemPersonalityWrite(Xsp3Sys[thisPath].femHandle, XSP3_DMA_CMD_SET_MSG_PPC2, level, 0, NULL, 0, NULL);
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
 * @ingroup toplevel
 * This function polls the DMA descriptors for the number of scope DMA engines in use to check whether all the data has been transferred.
 * @param path 	A handle to the top level of the xspress3 system returned from {@link xsp3_config()}.
 * @param card 	The number of the card in the xspress3 system,
 *       		0 for a single card system and up to ({@link xsp3_get_num_cards()} - 1) for a multi-card system,
 *        		where this value has been passed to {@link xsp3_config()} at xspress3 system configuration.
 *        		If card <0 the function waits until all cards in the system have finished.
 * @return {@link #XSP3_OK XSP3_OK} or a negative error code.
 */
int ngpd_dma_wait_scope(int path, int card, u_int32_t *statusP)
{
	char lower_msg[NGPD_MAX_ERROR_MESSAGE+2];
	ZYNQMP_DMA_StatusBlock statusBlock;
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
	u_int32_t scope_cont;

/*	if (path < 0 || path >= XSP3_MAX_PATH || !Xsp3Sys[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_wait_scope: invalid path %d", path);
		return XSP3_INVALID_PATH;
	}

	if (card < 0) {
		first_card = 0;
		last_card = Xsp3Sys[path].num_cards - 1;
	} else {
		if (card >= Xsp3Sys[path].num_cards) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_wait_scope: invalid card %d", card);
			return XSP3_ILLEGAL_CARD;
		}
		first_card = card;
		last_card = card;
	}

*/
	first_card = 0;
	last_card = 0;
	memset(&msg, 0, sizeof(ZYNQMP_DMA_MsgCheckDesc));

	for (card=first_card; card<=last_card; card++)
	{
		rc = ngpd_read_glob_regs(path, card, NGZMP_SCOPE_CONT, 1, &scope_cont);
		num_scope_dma = NGZMP_SCOPE_CONT_GET_NUM_STREAMS(scope_cont)+1;

		rc = ngzmp_get_dma_status_block(path, card, &statusBlock);
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
				// (statusBlock.stream_def[stream].data_size / 4) * 2;
				rc = ngzmp_dma_check_desc(path, card, stream, &msg, (u_int32_t *)&good_desc, &completed_desc, NULL, &status);
				printf("Done %d of %d, status=%08X\n", good_desc, statusBlock.stream_def[stream].defined_desc, status);
				if (rc < 0)
				{
					strcpy(lower_msg, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_wait_scope: ngzmp_dma_check_desc returns error: %s", lower_msg);
					ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
					return rc;
				}
				else if (good_desc == statusBlock.stream_def[stream].defined_desc)
					break;
				remaining= delay;
				while (nanosleep(&remaining, &remaining) == -1 && errno == EINTR);
			}
			if (good_desc != statusBlock.stream_def[stream].defined_desc)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_wait_scope: Timeout waiting for DMA stream %d, path=%d, card=%d, good %d out of %d, completed=%d, status=%08X", 
						stream, path, card, good_desc, statusBlock.stream_def[stream].defined_desc, completed_desc, status);
				return NGPD_ERROR;
			}
		}
	}
	return 0;
}
#if 0
/**
 * @ingroup internal
 * Instruct Zynq in XSPRESS3Mini or XSPRESS4 to read the status words from  a DMA descriptor
 * @n@n
 * The following describes the layout of the structure.
 * @snippet xspress3_dma_protocol.h XSP3_DMA_MSG_GET_DESC_STATUS
 *
 * @param path 		A handle to the top level of the xspress3 system returned from {@link xsp3_config()}.
 * @param card 		The number of the card in the xspress3 system,
 *        			0 for a single card system and up to ({@link xsp3_get_num_cards()} - 1) for a multi-card system,
 *        			where this value has been passed to {@link xsp3_config()} at xspress3 system configuration.
 * @param stream 	Stream number to build descriptors for defined in {@link XSP3_DMA_STREAM_NUMBER}.
 * @param msg		Pointer to the message of type XSP3_DMA_MsgBuildDesc.
 * @param status	Pointer ot return the status word returned by the Zynq DMA code.
 * @return {@link #XSP3_OK XSP3_OK} or a negative error code.
 */
int xsp3_dma_get_desc_status(int path, int card, u_int32_t stream, XSP3_DMA_MsgGetDescStatus *msg, u_int32_t *status) {
	int rc = XSP3_OK;
	int thisPath;
	int size = sizeof(XSP3_DMA_MsgGetDescStatus) / sizeof(u_int32_t);
	u_int32_t sub_command = XSP3_DMA_CMD_GET_DESC_STATUS;

	if (path < 0 || path >= XSP3_MAX_PATH || !Xsp3Sys[path].valid) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_dma_get_desc_status: invalid path %d", path);
		return XSP3_INVALID_PATH;
	}
	// Do sanity check on card number versus total in system
	if ((card < 0) || (card >= Xsp3Sys[path].num_cards)) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_dma_get_desc_status: illegal card specified (%d)", card);
		return XSP3_ILLEGAL_CARD;
	}

	if (Xsp3Sys[path].type == FEM_COMPOSITE) {
		thisPath = Xsp3Sys[path].sub_path[card];
		if (thisPath < 0) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_dma_get_desc_status: resolved illegal sub path (path=%d)", path);
			return XSP3_ILLEGAL_SUBPATH;
		}
	} else {
		thisPath = path;
	}

	if (stream < 0 || stream > Xsp3Sys[path].features.max_real_dma_stream) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "xsp3_dma_get_desc_status: invalid dma stream number %d", stream);
		return XSP3_INVALID_DMA_STREAM;
	}

	rc = xspress3FemPersonalityWrite(Xsp3Sys[thisPath].femHandle, sub_command, (1 << stream), size, (u_int32_t*) msg, 1, status);
	return rc;
}
#endif

