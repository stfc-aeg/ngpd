#if NGPD_CONF_ADQ14
#include "ADQAPI.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef LINUX
#include <sys/time.h>
#endif
#ifdef _MSC_VER
typedef struct timeval {
    long tv_sec;
    long tv_usec;
} timeval;
#endif
#include "ngpd.h"

int ngpd_dma_read_scope_chunk(int path, int card, u_int32_t scope_start, u_int32_t scope_num512, u_int32_t scope_out_skip, u_int32_t dpc, u_int32_t pause_words, int options);

int ngpd_dma_upload(int path, int card, int offset, int num_words, u_int32_t *data)
{
	int rc;
	int playback_size;
	u_int32_t x, num512, dpc;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_upload: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenZynqMP1)
	{
		return ngzmp_write_dma_buff(path, card, NGZMP_DMA_STREAM_BNUM_PLAYBACK0, offset, num_words, data);
	}
#if NGPD_CONF_ADQ14

	if ((rc=ngpd_read_glob_regs(path, card, NGPD_DATA_PATH, 1, &dpc)))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_upload:Error reading data path control register");
		return rc;
	}
	dpc &= ~NGPD_DP_RUN & ~NGPD_DP_RUN_SCOPE_OUTPUT;

	if ((rc=ngpd_write_glob_regs(path, card, NGPD_DATA_PATH, 1, &dpc)))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_upload:Error writing data path control register");
		return rc;
	}

	ADQ_SetStreamConfig(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1, 1);		// Disable the DRAM FIFO while Playback is being used.
	if ((offset & 0xF) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_upload: 32 bit word offset = %d must be divisible by 16", offset);
		return -1;
	}
	offset >>= 4;
	if ((num_words & 0xF) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_upload: Number of 32 bit words = %d must be divisible by 16", num_words);
		return -1;
	}
	num512 = num_words >> 4;
	playback_size = 1 + NGPDPath[path].playback_last_addr-NGPDPath[path].playback_first_addr;
	if (offset < 0 || offset >= playback_size  || num512 < 0 || offset+num512 > playback_size)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_upload: 512 bit word offset = %d + number of 512 bit words = %u does not fit in DMA buffer of %d 512 bit words", offset, num512, playback_size);
		return -1;
	}
	x = NGPDPath[path].playback_first_addr+offset;
	if ((rc=ngpd_write_glob_regs(path, card, NGPD_UPLOAD_FIRST_ADDR, 1, &x)))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_upload:Error writing first address register");
		return rc;
	}
	x = NGPDPath[path].playback_last_addr;
	if ((rc=ngpd_write_glob_regs(path, card, NGPD_UPLOAD_LAST_ADDR, 1, &x)))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_upload:Error writing last address register");
		return rc;
	}
	if ((rc=ngpd_write_glob_regs(path, card, NGPD_UPLOAD_WORD_COUNT, 1, &num512)))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_upload:Error writing word count register");
		return rc;
	}
	if (NGPDPath[path].debug > 0)
	{
		int start, nw, chunk;
		if (NGPDPath[path].debug > 1)
			chunk = 1024;
		else
			chunk = 1024*1024;
		for (nw=num_words, start=0; nw > 0; nw-=chunk, start+= chunk)
		{
			if (nw < chunk)
				chunk = nw;
			printf("ngpd_dma_upload: Writing %d of %9d, remaining=%9d\r", chunk, num_words, nw);
			ADQ_WriteBlockUserRegister(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 2, NGPD_UPLOAD_DATA, data+start, chunk*sizeof(u_int32_t), 0);
		}
		printf("Finished upload playback data\n");
		return 0;
	}
	return !ADQ_WriteBlockUserRegister(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 2, NGPD_UPLOAD_DATA, data, num_words*sizeof(u_int32_t), 0);	
#else
	printf("This version compiled without ADQ14 support. Build with NGPD_CONF_ADQ14 set\n");
	return -1;
#endif
}

int ngpd_dma_read_playback(int path, int offset, int num_words, u_int32_t *data, unsigned int *bytes_receivedP)
{
	int rc;
	int playback_size;
	u_int32_t num512;
	unsigned int bytes_received;

	/* The data alignment appears to be 512 bit words = 16 x 32 bit words. But also multiples of 32 */
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_playback: Invalid path=%d", path);
		return -1;
	}
#if NGPD_CONF_ADQ14
	ADQ_SetStreamConfig(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1, 1);		// Disable the DRAM FIFO while Playback is being used.

    ADQ_DisarmTrigger(NGPDPath[path].adq_cu, NGPDPath[path].adq_num);
    // ADQ_SetStreamStatus(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 0);

	if ((offset & 0x1FF) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_playback: 32 bit word offset = %d must be divisible by 16*32", offset);
		return -1;
	}
	offset >>= 4;
	if ((num_words & 0x1FF) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_playback: Number of 32 bit words = %d must be divisible by 16*32", num_words);
		return -1;
	}
	num512 = num_words >> 4;
	playback_size = 1 + NGPDPath[path].playback_last_addr-NGPDPath[path].playback_first_addr;
	if (offset < 0 || offset >= playback_size  || num512 < 0 || offset+num512 > playback_size)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_playback: 512 bit word offset = %d + number of 512 bit words = %u does not fit in DMA buffer of %d 512 bit words", offset, num512, playback_size);
		return -1;
	}
	printf("Calling ADQ_MemoryDump(first=0x%08X, last=%08X\n", NGPDPath[path].playback_first_addr+offset, NGPDPath[path].playback_first_addr+offset+num512-1);
	rc = !ADQ_MemoryDump(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, NGPDPath[path].playback_first_addr+offset, NGPDPath[path].playback_first_addr+offset+num512-1, (unsigned char *)data, &bytes_received, 0);	
	if (bytes_receivedP != NULL)
		*bytes_receivedP = bytes_received;
	return rc;
#else
	printf("This version compiled without ADQ14 support. Build with NGPD_CONF_ADQ14 set\n");
	return -1;
#endif
}


int ngpd_set_run_flags(int path, int run_flags)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_run_flags: Invalid path=%d", path);
		return -1;
	}
	NGPDPath[path].run_flags = run_flags;

	return 0;
}

int ngpd_get_run_flags(int path)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_get_run_flags: Invalid path=%d", path);
		return -1;
	}
	return NGPDPath[path].run_flags;
}

int ngpd_set_playback_streams(int path, int num_streams, int num_t)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_playback_streams: Invalid path=%d", path);
		return -1;
	}
	if (num_streams==1 || num_streams==2 || num_streams==4 || num_streams==8)
	{
		NGPDPath[path].playback_num_streams = num_streams;
		if (num_t == 0)
			NGPDPath[path].playback_num_t = NGPDPath[path].playback_num_bytes/(sizeof(u_int16_t)*NGPDPath[path].playback_num_streams);
		else
		{
			if (num_t > NGPDPath[path].playback_num_bytes/(sizeof(u_int16_t)*NGPDPath[path].playback_num_streams) )
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_playback_streams: Request num=t=%d does not fit in memory max num_t=%d", num_t,
					(int)(NGPDPath[path].playback_num_bytes/(sizeof(u_int16_t)*NGPDPath[path].playback_num_streams)) );
				return -1;
			}
			NGPDPath[path].playback_num_t = num_t;
		}
		return 0;
	}
	else
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_playback_streams: Invalid num of streams=%d, expected 1, 2, 4 or 8", num_streams);
		return -1;
	}
	return 0;
}

int ngpd_get_playback_streams(int path)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_get_playback_streams: Invalid path=%d", path);
		return -1;
	}
	return NGPDPath[path].playback_num_streams;
}

/**
 * @ingroup debug
 * Set the number of stall states per data word when outputing playback data.

 * This is usually set to 1 to output new data on every cycle, bt could be slowed to emulate using sample skip from the ADC, to evalutae using lower clock rates.
 *
 * @param path 				A handle to the top level of the NGPD system returned from {@link ngpd_config()}.
 * @param scope_out_skip	Number of stall states, 1 => continuous data, 2=> every other cycle, up to {@link NGPD_SCOPEOUT_SKIP_MAX}
 * @return 0 on success or a negative error code.
 */

int ngpd_dma_set_playback_skip(int path, int playback_skip)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_set_scope_out_skip: Invalid path=%d", path);
		return -1;
	}
	if (playback_skip < 1 || playback_skip > NGPD_SCOPEOUT_SKIP_MAX)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_set_playback_skip: Invalid skip value %d, expected 1..%d\n", playback_skip, NGPD_SCOPEOUT_SKIP_MAX);
		return -1;
	}
	NGPDPath[path].playback_skip = playback_skip;

	return 0;
}
/**
 * @ingroup debug
 * Get the number of stall states per data word when outputing playback data.
 *
 * @param path 				A handle to the top level of the NGPD system returned from {@link ngpd_config()}.
 * @param scope_out_skip	Number of stall states, 1 => continuous data, 2=> every other cycle, up to {@link NGPD_SCOPEOUT_SKIP_MAX}
 * @return 0 on success or a negative error code.
 */
int ngpd_dma_get_playback_skip(int path)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_get_playback_skip: Invalid path=%d", path);
		return -1;
	}
	return NGPDPath[path].playback_skip;
}


/**
 * @ingroup debug
 * Set the number of stall states per data word when outputing scope mode data.

 * This is used to throttle the rate of data outptu so that the server can library can keep up with emptying the buffers.
 *
 * @param path 				A handle to the top level of the NGPD system returned from {@link ngpd_config()}.
 * @param scope_out_skip	Number of stall states, 1 => continuous data, 2=> every other cycle, up to {@link NGPD_SCOPEOUT_SKIP_MAX}
 * @return 0 on success or a negative error code.
 */

int ngpd_dma_set_scope_out_skip(int path, int scope_out_skip)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_set_scope_out_skip: Invalid path=%d", path);
		return -1;
	}
	if (scope_out_skip < 1 || scope_out_skip > NGPD_SCOPEOUT_SKIP_MAX)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_set_scope_out_skip: Invalid skip value %d, expected 1..%d\n", scope_out_skip, NGPD_SCOPEOUT_SKIP_MAX);
		return -1;
	}
	NGPDPath[path].scope_out_skip = scope_out_skip;

	return 0;
}
/**
 * @ingroup debug
 * Get the number of stall states per data word when outputing scope mode data.
 *
 * @param path 				A handle to the top level of the NGPD system returned from {@link ngpd_config()}.
 * @param scope_out_skip	Number of stall states, 1 => continuous data, 2=> every other cycle, up to {@link NGPD_SCOPEOUT_SKIP_MAX}
 * @return 0 on success or a negative error code.
 */
int ngpd_dma_get_scope_out_skip(int path)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_get_scope_out_skip: Invalid path=%d", path);
		return -1;
	}
	return NGPDPath[path].scope_out_skip;
}

/**
 * @ingroup toplevel
 * Start the system including starting DMA engines if required. 
 *
 * The run_flags set by {@link ngpd_set_run_flags()} are used to control various settings
 * @param path a handle to the top level of the xspress3 system returned from {@link ngpd_config()}.
 * @param pb_start	Override default start of data to send in playback testing, usually specify 0.
 * @param pb_num512	Override default number of 512 bit words of playback data to send in playback testing, usually specify 0 to use default
 * @return 0 on success or a negative error code.
 */

int ngpd_dma_system_start(int path, int card, u_int32_t pb_start, u_int32_t pb_num512, NGPDITFGSetup *itfg_setup)
{
	//	char old_message[NGPD_MAX_ERROR_MESSAGE+2];
	int rc;
	u_int32_t dpc, pb_last;
	u_int32_t playback_size;
	u_int32_t scope_cont, scope_regs[3];
	NGPDScopeModule *scope_mod;
	struct timeval start, now;
	unsigned int buffers_filled;
	double t_from_start, t_from_end;
	int use_itfg=1;
	NGPDITFGSetup dummy_setup = {-1.0, 0, 1};
	u_int32_t prev_status, status=0xFFFFFFFF;
	u_int32_t prev_cycles=0, cycles_status;
	int trigger_from_run = 1;

	if (itfg_setup == NULL)
		itfg_setup = &dummy_setup;
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenZynqMP1)
		return ngzmp_system_start(path, card, pb_num512, itfg_setup);
	// printf("ngpd_dma_system_start: with run_flags=%08X\n", NGPDPath[path].run_flags);
#if NGPD_CONF_ADQ14
	if ((rc=ngpd_read_glob_regs(path, card, NGPD_DATA_PATH, 1, &dpc)) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Reading data path register");
		return -1;
	}
	dpc &= ~NGPD_DP_RUN & ~NGPD_DP_RUN_SCOPE_OUTPUT & ~NGPD_DP_DATA_SRC_INPUT(255) & ~NGPD_DP_DATA_SRC_OUTPUT(255) & ~NGPD_DP_PLAYBACK_ENB & ~NGPD_DP_PLAYBACK_CONT & NGPD_DP_RUN_TRIG_FROM_RUN;
	if (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_PLAYBACK)
	{
		if (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_PLAYBACK_ALT)
			dpc |= 	NGPD_DP_DATA_SRC_INPUT(NGPD_DP_DATA_SRC_INP_PLAYBACK_ALT) | NGPD_DP_PLAYBACK_ENB;
		else
			dpc |= 	NGPD_DP_DATA_SRC_INPUT(NGPD_DP_DATA_SRC_INP_PLAYBACK) | NGPD_DP_PLAYBACK_ENB;
	}
	dpc |= NGPD_DP_DATA_SRC_OUTPUT(NGPD_DP_DATA_SRC_OUT_RUN);
	if (trigger_from_run)
		dpc |= NGPD_DP_RUN_TRIG_FROM_RUN;

	if (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_PLAYBACK_CONTINUOUS)
		dpc |= NGPD_DP_PLAYBACK_CONT;

	if ((rc=ngpd_write_glob_regs(path, card, NGPD_DATA_PATH, 1, &dpc)) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Writing data path register");
		return -1;
	}
	CHECKADQ(ADQ_StopStreaming(NGPDPath[path].adq_cu, NGPDPath[path].adq_num));
	CHECKADQ(ADQ_SetStreamStatus(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 0)); 

	if (itfg_setup->trig_mode == NGPDTrigSW)
	{
		CHECKADQ(ADQ_SetTriggerMode(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1));	// SWTrig
	}
	else
	{
		CHECKADQ(ADQ_SetTriggerMode(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 2));	// External Trigger 1
	}

	CHECKADQ(ADQ_SetTriggerEdge(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1, 1));   // SWTrig Rising
	if ((NGPDPath[path].run_flags & NGPD_RUN_FLAGS_PLAYBACK) || (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_SCOPEMODE))
	{
		ADQ_SetStreamConfig(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1, 1);		// Disable the DRAM FIFO while Playback or scope mode is being used.
	}
	else
	{
		ADQ_SetStreamConfig(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1, 0);		// Enable the DRAM FIFO while Playback is not being used.
	}
	if (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_PLAYBACK)
	{
		if (pb_start == 0)
			pb_start = NGPDPath[path].playback_first_addr;
		if (pb_num512 == 0)
			pb_num512 =  NGPDPath[path].playback_num_t/32;			
//			pb_num512 = 1 + NGPDPath[path].playback_last_addr-NGPDPath[path].playback_first_addr;
		playback_size = 1 + NGPDPath[path].playback_last_addr-NGPDPath[path].playback_first_addr;
		if (pb_start < NGPDPath[path].playback_first_addr || pb_start > NGPDPath[path].playback_last_addr || pb_num512 > playback_size)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Invalid playback data range : start=%u, num512=%u, buffer_first=%u, buffer_last=%u", 
				pb_start, pb_num512, NGPDPath[path].playback_first_addr, NGPDPath[path].playback_last_addr);
			return -1;
		}
		pb_last=pb_start+pb_num512-1;
		if ((rc=ngpd_write_glob_regs(path, card, NGPD_PLAYBACK_FIRST_ADDR, 1, &pb_start)) != 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Writing playback first addr register");
			return -1;
		}
		if ((rc=ngpd_write_glob_regs(path, card, NGPD_PLAYBACK_LAST_ADDR, 1, &pb_last)) != 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Writing playback last addr register");
			return -1;
		}
		if ((rc=ngpd_write_glob_regs(path, card, NGPD_PLAYBACK_SKIP, 1, &(NGPDPath[path].playback_skip))) < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Writing playback skip register");
			return -1;
		}
	}
	if ((rc=ngpd_read_glob_regs(path, card, NGPD_SCOPE_CONT, 1, &scope_cont)) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Reading scope control register");
		return -1;
	}
	if (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_SCOPEMODE)
	{
		if ((rc=ngpd_write_glob_regs(path, card, NGPD_SCOPE_FIRST_ADDR, 1, &(NGPDPath[path].scope_first_addr))) != 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Writing scope mode first addr register");
			return -1;
		}
		if ((rc=ngpd_write_glob_regs(path, card, NGPD_SCOPE_LAST_ADDR, 1, &(NGPDPath[path].scope_last_addr))) != 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Writing scope mode last addr register");
			return -1;
		}
		scope_cont |= NGPD_SCOPE_CONT_ENABLE;
		if ((scope_mod=NGPDPath[path].scope_mod) != NULL)
		{
			if (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS)
			{
				int nstreams;
				nstreams = ngpd_scope_mod_get_nstreams(scope_mod);
				scope_cont &= ~NGPD_SCOPE_CONT_NUM_STREAMS(255);
				switch (nstreams)
				{
				case 1: scope_cont |= NGPD_SCOPE_CONT_NUM_STREAMS(NGPD_SCOPE_NUM_STREAMS1);
					break;
				case 2: scope_cont |= NGPD_SCOPE_CONT_NUM_STREAMS(NGPD_SCOPE_NUM_STREAMS2);
					break;
				case 4: scope_cont |= NGPD_SCOPE_CONT_NUM_STREAMS(NGPD_SCOPE_NUM_STREAMS4);
					break;
				case 6: scope_cont |= NGPD_SCOPE_CONT_NUM_STREAMS(NGPD_SCOPE_NUM_STREAMS6);
					break;
				default : snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Unknown number of streams %d\n", nstreams);
					return -1;
				}
				scope_regs[0] = scope_mod->head.card[0].chan_sel[0];
				scope_regs[1] = scope_mod->head.card[0].src_sel[0];
				scope_regs[2] = scope_mod->head.card[0].alternate[0];
				if ((rc=ngpd_write_glob_regs(path, card, NGPD_SCOPE_CHAN_SEL, 3, scope_regs)) != 0)
				{
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Reading scope control register");
					return -1;
				}
				printf("Scope cont= 0x%08X\n", scope_cont);
				printf("Chan Sel  = 0x%08X\n", scope_mod->head.card[0].chan_sel[0]);
				printf("Src Sel   = 0x%08X\n", scope_mod->head.card[0].src_sel[0]);
				printf("Alternate = 0x%08X\n", scope_mod->head.card[0].alternate[0]);
			}
			else if (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD)
			{
				int nstreams;
				switch (NGPD_SCOPE_CONT_GET_NUM_STREAMS(scope_cont))
				{
				case NGPD_SCOPE_NUM_STREAMS1: nstreams = 1;
					break;
				case NGPD_SCOPE_NUM_STREAMS2: nstreams = 2;
					break;
				case NGPD_SCOPE_NUM_STREAMS4: nstreams = 4;
					break;
				case NGPD_SCOPE_NUM_STREAMS6: nstreams = 6;
					break;
				default : snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Unknown number of streams CODE %d\n", NGPD_SCOPE_CONT_GET_NUM_STREAMS(scope_cont));
					return -1;
				}
				ngpd_scope_set_streams(scope_mod, nstreams);
//				printf("Setting Nstreams= %d then reading registers into module\n", nstreams);
				if ((rc=ngpd_read_glob_regs(path, card, NGPD_SCOPE_CHAN_SEL, 3, scope_regs)) != 0)
				{
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Reading scope control register");
					return -1;
				}
				scope_mod->head.card[0].chan_sel[0] = scope_regs[0];
				scope_mod->head.card[0].chan_sel[1] = 0;
				scope_mod->head.card[0].src_sel[0] = scope_regs[1];
				scope_mod->head.card[0].src_sel[1] = 0;
				scope_mod->head.card[0].alternate[0] = scope_regs[2];
				scope_mod->head.card[0].alternate[1] = 0;
			}
		}
	}
	else
	{
		scope_cont &= ~NGPD_SCOPE_CONT_ENABLE;
	}

	if ((rc=ngpd_write_glob_regs(path, card, NGPD_SCOPE_CONT, 1, &scope_cont)) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error writing scope control register");
		return -1;
	}
	
	if (itfg_setup->col_time > 0)
	{
		size_t num_buff;
		num_buff = 64;		// But due to maximum number of queued tranfers being 32, not worth allocating so many buffers.

		printf("ngpd_dma_system_start:Allocating %d buffers of size %d bytes in Kernel space\n", (int)num_buff, NGPD_MAX_BUFFER );

		while (num_buff > 2)
		{
			rc = ADQ_SetTransferBuffers(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, num_buff, NGPD_MAX_BUFFER);
			if (rc == 1)
				break;
			num_buff >>= 1;
		}
		if (rc == 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Cannot allocate DMA buffers");
			return -1;
		}

		//Enable streaming
		CHECKADQ(ADQ_SetStreamStatus(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1));  // Enable streaming mode
		CHECKADQ(ADQ_SetStreamConfig(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 2, 1)); // Enable raw streaming without packet headers
		CHECKADQ(ADQ_SetStreamConfig(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 3,  1)); // Set channel mask to enable channel 1 only in run mode

		CHECKADQ(ADQ_StartStreaming(NGPDPath[path].adq_cu, NGPDPath[path].adq_num));
		if (!trigger_from_run)
			CHECKADQ(ADQ_SWTrig(NGPDPath[path].adq_cu, NGPDPath[path].adq_num));

		printf("Collecting data, please wait...\n");
		if (use_itfg)
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
			if ((rc=ngpd_write_glob_regs(path, card, NGPD_ITFG_TIME, 3, itfg_regs)) != 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Writing ITFG Time register");
				return -1;
			}
		}

		gettimeofday(&start, NULL);
		prev_status = NGPD_SCOPE_STATUS_ITFG_RUNNING;

		dpc |= NGPD_DP_RUN;
		if ((rc=ngpd_write_glob_regs(path, card, NGPD_DATA_PATH, 1, &dpc)) != 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Writing data path register");
			return -1;
		}
		t_from_end = 0.0;
		t_from_start = 0.0;
		do
		{
			CHECKADQ(ADQ_GetTransferBufferStatus(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, &buffers_filled));
			if (use_itfg)
			{
				if (prev_status)
				{
					if ((rc=ngpd_read_glob_regs(path, card, NGPD_GLOB_SCOPE_STATUS, 1, &status)) != 0)
					{
						snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Writing ITFG Time register");
						return -1;
					}
					if ((rc=ngpd_read_glob_regs(path, card, NGPD_GLOB_CYCLES_STATUS, 1, &cycles_status)) != 0)
					{
						snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Writing ITFG Time register");
						return -1;
					}
					if (itfg_setup->cycles > 1 && prev_cycles != cycles_status)
						printf("Cycles Remaining=%u          \r", cycles_status);
					prev_cycles = cycles_status;
					if (status != prev_status && 0)
						printf("Status=%X\n", status);

					if (!(status & NGPD_SCOPE_STATUS_ITFG_RUNNING))
					{
						printf("\nITFG finished\n");
						gettimeofday(&start, NULL);
						prev_status = 0;
						ADQ_FlushDMA(NGPDPath[path].adq_cu, NGPDPath[path].adq_num);
					}
					t_from_end = 0;
				}
				else
				{
					gettimeofday(&now, NULL);
					t_from_end = now.tv_sec-start.tv_sec + 1E-6*(now.tv_usec-start.tv_usec);
				}
			}
			else
			{
				gettimeofday(&now, NULL);
				t_from_start = now.tv_sec-start.tv_sec + 1E-6*(now.tv_usec-start.tv_usec);
//				printf("t_from_start=%g, coltime=%g, buffers_filled=%u, status=0x%08X\n", t_from_start, col_time, buffers_filled, status);
			}

			if (buffers_filled > 0)
			{
				if ((rc=ngpd_hist_process(path, buffers_filled)) < 0)
				{
					dpc &= ~NGPD_DP_RUN;
					if ((rc=ngpd_write_glob_regs(path, card, NGPD_DATA_PATH, 1, &dpc)) != 0)
					{
						snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Writing data path register");
						return -1;
					}
					CHECKADQ(ADQ_StopStreaming(NGPDPath[path].adq_cu, NGPDPath[path].adq_num));
					goto error;
				}
			}
					
		} while ((use_itfg && t_from_end < 0.001) || (!use_itfg && t_from_start < itfg_setup->col_time));

		CHECKADQ(ADQ_StopStreaming(NGPDPath[path].adq_cu, NGPDPath[path].adq_num));
		dpc &= ~NGPD_DP_RUN;
		if ((rc=ngpd_write_glob_regs(path, card, NGPD_DATA_PATH, 1, &dpc)) != 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Writing data path register");
			return -1;
		}
	}
	else
	{
		/* Start system for run without histogramming */
		dpc |= NGPD_DP_RUN;
		if ((rc=ngpd_write_glob_regs(path, card, NGPD_DATA_PATH, 1, &dpc)) != 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_system_start: Error Writing data path register");
			return -1;
		}
	}
	return 0;

error:
	return -1;
#else
	printf("This version compiled without ADQ14 support. Build with NGPD_CONF_ADQ14 set\n");
	return -1;
#endif
}

int ngpd_dma_read_scope(int path, int card, u_int32_t scope_start, u_int32_t scope_num512, int options)
{
//	char old_message[NGPD_MAX_ERROR_MESSAGE+2];
	size_t num_buff;
	int rc;
	u_int32_t dpc;
	u_int32_t scope_size;
	int two_stream;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_scope: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenZynqMP1)
	{
		return ngpd_scope_update_mod(path, card, 0, -1);	// For read of all scope mode data
	}

#if NGPD_CONF_ADQ14
	two_stream= !!(NGPDPath[path].run_flags & NGPD_RUN_FLAGS_SCOPE_OUT_2_STREAM);

	if ((rc=ngpd_read_glob_regs(path, card, NGPD_DATA_PATH, 1, &dpc)) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_scope: Error Reading data path register");
		return -1;
	}
	dpc &= ~NGPD_DP_RUN & ~NGPD_DP_RUN_SCOPE_OUTPUT & ~NGPD_DP_DATA_SRC_OUTPUT(255) & ~NGPD_DP_PLAYBACK_ENB & ~NGPD_DP_PLAYBACK_CONT & ~NGPD_DP_RUN_TRIG_FROM_RUN;

	if (NGPDPath[path].run_flags & NGPD_RUN_FLAGS_SCOPE_OUT_TEST)
		dpc |= 	NGPD_DP_DATA_SRC_OUTPUT(NGPD_DP_DATA_SRC_OUT_TEST_PAT);
	else
		dpc |= 	NGPD_DP_DATA_SRC_OUTPUT(NGPD_DP_DATA_SRC_OUT_SCOPE_OUT);

	printf("Writing data path register=0x%X and scope skip=0x%X. 2 stream=%d\n", dpc, NGPDPath[path].scope_out_skip, two_stream);
	if ((rc=ngpd_write_glob_regs(path, card, NGPD_DATA_PATH, 1, &dpc)) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_scope: Error Writing data path register");
		return -1;
	}

	CHECKADQ(ADQ_StopStreaming(NGPDPath[path].adq_cu, NGPDPath[path].adq_num));	
	CHECKADQ(ADQ_SetStreamStatus(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 0)); 
	CHECKADQ(ADQ_SetTriggerMode(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1));	// SWTrig
	CHECKADQ(ADQ_SetTriggerEdge(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1, 1));   // SWTrig Rising

	ADQ_SetStreamConfig(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1, 1);		// Disable the DRAM FIFO while Playback is being used.

	if (scope_start == 0)
		scope_start = NGPDPath[path].scope_first_addr;
	if (scope_num512 == 0)
	scope_num512 = 1 + NGPDPath[path].scope_last_addr-NGPDPath[path].scope_first_addr;
	scope_size = 1 + NGPDPath[path].scope_last_addr-NGPDPath[path].scope_first_addr;
	if (scope_start < NGPDPath[path].scope_first_addr || scope_start > NGPDPath[path].scope_last_addr || scope_num512 > scope_size)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_scope: Invalid scope data range : start=%u, num512=%u, buffer_first=%u, buffer_last=%u", 
				scope_start, scope_num512, NGPDPath[path].scope_first_addr, NGPDPath[path].scope_last_addr);
		return -1;
	}

//	num_buff = 256;		// Maximum found in device driver
	num_buff = 64;		// But due to maximum number of queued tranfers being 32, not worth allocating so many buffers.

	printf("ngpd_dma_read_scope:Allocating %d buffers of size %d bytes in Kernel space\n", (int)num_buff, NGPD_MAX_BUFFER );

	while (num_buff > 2)
	{
		rc = ADQ_SetTransferBuffers(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, num_buff, NGPD_MAX_BUFFER);
		if (rc == 1)
			break;
		num_buff >>= 1;
	}
	if (rc == 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_scope: Cannot allocate DMA buffers");
		return -1;
	}
	if (options & NGPD_READ_SCOPE_FLOW_CONTROL)
	{
		u_int32_t scope_out_skip = (u_int32_t)NGPDPath[path].scope_out_skip;
		u_int32_t pause512words;
		if (num_buff > 30)
			pause512words = 30*NGPD_MAX_BUFFER/64;
		else
			pause512words = (num_buff/2)*NGPD_MAX_BUFFER/64;
//		u_int32_t pause512words = NGPD_MAX_BUFFER/64;
		if (NGPDPath[path].debug >=1)
			printf("Transfering scope mode data from 512 bit words region 0x%X for 0x%X  slowing using scope_out_skip=0x%X using pausing flow control event 0x%X words\n", 
				scope_start, scope_num512, scope_out_skip, pause512words );
		rc = ngpd_dma_read_scope_chunk(path, card, scope_start, scope_num512, scope_out_skip, dpc, pause512words, options);
	}
	else if (options & NGPD_READ_SCOPE_NO_CHUNK)
	{
		u_int32_t scope_out_skip = (u_int32_t)NGPDPath[path].scope_out_skip;
		if (NGPDPath[path].debug >=1)
			printf("Transfering scope mode data from 512 bit words region 0x%X for 0x%X  slowing using scope_out_skip=0x%X SINGLE CHUNK\n", 
				scope_start, scope_num512, scope_out_skip);
		rc = ngpd_dma_read_scope_chunk(path, card, scope_start, scope_num512, scope_out_skip, dpc, 0, options);
	}
	else
	{
		u_int32_t scope_out_skip = (u_int32_t)NGPDPath[path].scope_out_skip;
		if (NGPDPath[path].debug >=1)
			printf("Transfering scope mode data from 512 bit words region 0x%X for 0x%X  slowing using scope_out_skip=0x%X, reading in 0x%X Word chunks\n", 
				scope_start, scope_num512, scope_out_skip, (32)*NGPD_MAX_BUFFER/64 );
//		ngpd_dma_read_scope_chunk(path, 0, NGPD_MAX_BUFFER/64, scope_out_skip, dpc, 0, options);	// Extra little burst might "fix" scope output.--- makes it worse
		while (scope_num512 > 0)
		{
			int num512;
			int retry=0;
			if (num_buff > 32)
				num512 = (32)*NGPD_MAX_BUFFER/64;
			else
				num512 = (num_buff)*NGPD_MAX_BUFFER/64;
			if (scope_num512 < num512)
				num512 = scope_num512;
			do
			{
				rc = ngpd_dma_read_scope_chunk(path, card, scope_start, num512, scope_out_skip, dpc, 0, options);
			} while (rc != 0 &&  ++retry < 5);
			if (rc != 0)
				return rc;
			scope_start += num512;
			scope_num512 -= num512;
		}
	}
	return rc;
error:
	return -1;
#else
	printf("This version compiled without ADQ14 support. Build with NGPD_CONF_ADQ14 set\n");
	return -1;
#endif
}

#if NGPD_CONF_ADQ14
int ngpd_dma_read_scope_chunk(int path, int card, u_int32_t scope_start, u_int32_t scope_num512, u_int32_t scope_out_skip, u_int32_t dpc, u_int32_t pause_words, int options)
{
	int rc;
	u_int32_t scope_last;
	u_int32_t status;
	unsigned int samples_to_collect, next_pause, samples_per_pause;
	unsigned int buffers_filled;
	u_int32_t *src, *dst;
	int i, j, k, m=0;
	int stream_b=1;			
	int two_stream;
	int poll_count;
	int cont_count;

	scope_last=scope_start+scope_num512-1;
	two_stream= !!(NGPDPath[path].run_flags & NGPD_RUN_FLAGS_SCOPE_OUT_2_STREAM);

	if (NGPDPath[path].debug >= 2)
		printf("Writing scope_start=%X, scope_last=%X, pause words=%08X, two stream=%d\n", scope_start, scope_last, pause_words, two_stream);
	if ((rc=ngpd_write_glob_regs(path, card, NGPD_SCOPEOUT_FIRST_ADDR, 1, &scope_start)) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_scope: Error Writing scope first addr register");
		return -1;
	}
	if ((rc=ngpd_write_glob_regs(path, card, NGPD_SCOPEOUT_LAST_ADDR, 1, &scope_last)) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_scope: Error Writing scope last addr register");
		return -1;
	}
	if ((rc=ngpd_write_glob_regs(path, card, NGPD_SCOPEOUT_PAUSE_WORDS, 1, &pause_words)) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_scope: Error Writing scope pause words register");
		return -1;
	}

	if (two_stream && scope_out_skip == 1)
		scope_out_skip= 2;
	if (two_stream)
		scope_out_skip |= NGPD_SCOPE_SKIP_TWO_STREAM;

	if (options & NGPD_READ_SCOPE_RESET_EACH_RUN)
		scope_out_skip |= NGPD_SCOPE_SKIP_RESET_EACH_RUN;
	if (options & NGPD_READ_SCOPE_RESET_LONG_ABORT)
		scope_out_skip |= NGPD_SCOPE_SKIP_LONG_ABORT;

	if ((rc=ngpd_write_glob_regs(path, card, NGPD_SCOPEOUT_SKIP, 1, &scope_out_skip)) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_scope: Error Writing scope skip path register");
		return -1;
	}

  	//Enable streaming
	CHECKADQ(ADQ_SetStreamStatus(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1));  // Enable streaming mode
	CHECKADQ(ADQ_SetStreamConfig(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 2, 1)); // Enable raw streaming without packet headers
	if (two_stream)
	{
		CHECKADQ(ADQ_SetStreamConfig(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 3,  3)); // Set channel mask to enable both channel in scope mode
	}
	else
	{
		CHECKADQ(ADQ_SetStreamConfig(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 3,  1)); // Set channel mask to enable just channel A
	}
	CHECKADQ(ADQ_StopStreaming(NGPDPath[path].adq_cu, NGPDPath[path].adq_num));
	ADQ_SetStreamConfig(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1, 1);		// Disable the DRAM FIFO while Playback is being used. Note the line that this is done appeats critical
	CHECKADQ(ADQ_StartStreaming(NGPDPath[path].adq_cu, NGPDPath[path].adq_num));
	CHECKADQ(ADQ_SWTrig(NGPDPath[path].adq_cu, NGPDPath[path].adq_num));
	
	dpc |= NGPD_DP_RUN_SCOPE_OUTPUT;
	if ((rc=ngpd_write_glob_regs(path, card, NGPD_DATA_PATH, 1, &dpc)) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_scope: Error Writing data path register");
		return -1;
	}
	dst = (u_int32_t *)ngpd_scope_mod_get_ptr(NGPDPath[path].scope_mod, 0, 0);
	dst += (scope_start-NGPDPath[path].scope_first_addr)*16;
	samples_to_collect = scope_num512*32;
	samples_per_pause = pause_words*32;
	if (pause_words && samples_per_pause < samples_to_collect)
		next_pause = samples_to_collect-samples_per_pause;
	else
		next_pause = 0;
	poll_count = 0;
	cont_count = 0;
	while (samples_to_collect > 0)
	{
		unsigned int samples_in_buffer;
		if (samples_to_collect <= next_pause)
		{
			if ((rc=ngpd_read_glob_regs(path, card, NGPD_GLOB_SCOPE_STATUS, 1, &status)) != 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_scope: Error reading scope status register");
				return -1;
			}
			if (status & NGPD_SCOPE_STATUS_SCOPEOUT_PAUSED)
			{
				if (++poll_count >= 1)
				{
					poll_count = 0;
					scope_out_skip &= ~NGPD_SCOPE_SKIP_CONTINUE;
					if ((rc=ngpd_write_glob_regs(path, card, NGPD_SCOPEOUT_SKIP, 1, &scope_out_skip)) != 0)
					{
						snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_scope: Error Writing scope skip to continue");
						return -1;
					}
					scope_out_skip |= NGPD_SCOPE_SKIP_CONTINUE;
					if ((rc=ngpd_write_glob_regs(path, card, NGPD_SCOPEOUT_SKIP, 1, &scope_out_skip)) != 0)
					{
						snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_read_scope: Error Writing scope skip to continue");
						return -1;
					}
					if (samples_per_pause < samples_to_collect)
						next_pause = samples_to_collect-samples_per_pause;
					else
						next_pause = 0;
					cont_count++;
					if (NGPDPath[path].debug >= 2)
						printf("samples_to_collect=%X, continue %d\n", samples_to_collect, cont_count);
				}
			}
		}
		CHECKADQ(ADQ_GetTransferBufferStatus(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, &buffers_filled));
#if 0 
/* this tried to leave 1 buffer margin between read and write, but did not help */
		if (buffers_filled == 1 && samples_to_collect-next_pause > NGPD_MAX_BUFFER/2 )
			continue; // Allow more than 1 buffer to be filled unless on last buffer 
#endif
		for (i=0; i<buffers_filled; i++)
		{
			rc = ADQ_CollectDataNextPage(NGPDPath[path].adq_cu, NGPDPath[path].adq_num);
			samples_in_buffer = ADQ_GetSamplesPerPage(NGPDPath[path].adq_cu, NGPDPath[path].adq_num);
			if (samples_to_collect < samples_in_buffer)
				samples_in_buffer = samples_to_collect; 

			if (ADQ_GetStreamOverflow(NGPDPath[path].adq_cu, NGPDPath[path].adq_num))
			{
				printf("ERROR: Streaming Overflow at samples_to_collect=%d, samples_in_buffer=%d, buffers_filled=%d !\n", samples_to_collect, samples_in_buffer, buffers_filled);
				ADQ_StopStreaming(NGPDPath[path].adq_cu, NGPDPath[path].adq_num);
				dpc &= ~NGPD_DP_RUN_SCOPE_OUTPUT;
				ngpd_write_glob_regs(path, card, NGPD_DATA_PATH, 1, &dpc);
				return NGPD_ERROR_OVERRUN;
				
			}		
			if (rc)
			{
				u_int32_t *s2;
				src = (u_int32_t *)ADQ_GetPtrStream(NGPDPath[path].adq_cu, NGPDPath[path].adq_num);
				s2 = src;
				if (NGPDPath[path].debug >= 3 && m++ <10)
				{
					for (j=0;j<16; j++)
						printf(" %8X", src[j]);
					printf("\n");
				}
				if (two_stream)
				{
					for (j=0; j<samples_in_buffer; j+=2*NGPD_ADQ14_TRANSFER_SIZE_BYTES/sizeof(u_int16_t))
					{
						for (stream_b=0; stream_b < 2; stream_b++)
						{
							for (k=0; k<NGPD_ADQ14_TRANSFER_SIZE_BYTES/sizeof(u_int32_t); k++)
								dst[2*k+stream_b] = *src++;
						}
						dst += NGPD_ADQ14_TRANSFER_SIZE_BYTES/2;
					}
					for (k=0; k<samples_in_buffer/2; k++)
					{
						if (k%4==0)
							*s2++ = 5000;
						else
							*s2++ = 0;
					}
				}
				else
				{
					memcpy(dst, src, samples_in_buffer*sizeof(u_int16_t));
					dst += samples_in_buffer/2;
				}
				samples_to_collect -= samples_in_buffer;
			}
			else
			{
				printf("ADQ_CollectDataNextPage failed\n");
				goto error;
			}
		}
	}
	dpc &= ~NGPD_DP_RUN_SCOPE_OUTPUT;
	ngpd_write_glob_regs(path, card, NGPD_DATA_PATH, 1, &dpc);
	CHECKADQ(ADQ_StopStreaming(NGPDPath[path].adq_cu, NGPDPath[path].adq_num));
	CHECKADQ(ADQ_SetStreamStatus(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 0));  // Disable streaming mode
	if (NGPDPath[path].debug >= 2)
		printf("buffers_filled = %d, cont_count=%d\n", buffers_filled, cont_count);

	return 0;	

error:
	dpc &= ~NGPD_DP_RUN_SCOPE_OUTPUT;
	ngpd_write_glob_regs(path, card, NGPD_DATA_PATH, 1, &dpc);
	ADQ_StopStreaming(NGPDPath[path].adq_cu, NGPDPath[path].adq_num);
	ADQ_SetStreamStatus(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 0);  // Disable streaming mode
	return -1;
}
#endif

int ngpd_dma_wait_scope(int path, int card, u_int32_t *statusP)
{
	double t, secs;
	int rc;
	u_int32_t scope_cont;
	u_int32_t status;
	struct timeval start, now;
	struct timespec delay;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_wait_scope: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenZynqMP1)
		return ngzmp_dma_wait_scope(path, card, statusP);

#if NGPD_CONF_ADQ14
	if (statusP != NULL)
		*statusP = 0;
	t = 512.0*1024*1024/500000000;

	if ((rc=ngpd_read_glob_regs(path, card, NGPD_SCOPE_CONT, 1, &scope_cont)) != 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_wait_scope: Error Reading scope control register");
		return -1;
	}

	switch (NGPD_SCOPE_CONT_GET_NUM_STREAMS(scope_cont))
	{
	case NGPD_SCOPE_NUM_STREAMS1: break;
	case NGPD_SCOPE_NUM_STREAMS2: t /= 2.0;
		break;
	case NGPD_SCOPE_NUM_STREAMS4: t /= 4.0;
		break;
	case NGPD_SCOPE_NUM_STREAMS6: t /= 6.0;
		break;
	}

	gettimeofday(&start, NULL);
	delay.tv_sec = 0;
	delay.tv_nsec = 10*1000*1000;	/* 10 ms delay */
	do
	{
		nanosleep(&delay, NULL);
		if ((rc=ngpd_read_glob_regs(path, card, NGPD_GLOB_SCOPE_STATUS, 1, &status)) != 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_wait_scope: Error Reading data path register");
			return -1;
		}
		if (!(status & NGPD_SCOPE_STATUS_RUNNING))
			break;
		gettimeofday(&now, NULL);
		secs = now.tv_sec-start.tv_sec + 1E-6*(now.tv_usec-start.tv_usec);
		if (secs > 1.5 * t)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_wait_scope: Timeout waiting for scope mode DMA to finish after %g s, expected %g s\n", secs, t);
			return -1;
		}
	} while (1);
	return 0;
#else
	printf("This version compiled without ADQ14 support. Build with NGPD_CONF_ADQ14 set\n");
	return -1;
#endif
}


int ngpd_dma_set_ext_trigger_thres(int path, int card, double trig_thres)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_dma_set_ext_trigger_thres: Invalid path=%d", path);
		return -1;
	}
#if NGPD_CONF_ADQ14

	return !ADQ_SetExtTrigThreshold(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1, trig_thres);
#else
	printf("This version compiled without ADQ14 support. Build with NGPD_CONF_ADQ14 set\n");
	return -1;
#endif

}
