#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ngpd.h"
#include "ngzmptest.h"
#include "errors.h"
#include "zynqmp_lib_driver.h"

#define NUM_WORDS_SHORT 64
#define NUM_WORDS_MEDIUM 2048
#define NUM_WORDS_LONG (2<<28)

int build_tp_data(int path, int card, int num_pb_streams);

int dma_tests(int path)
{
	int card = 0;
	int i, num_pb_streams;
/*
	if (test_dma & TEST_DMA_DISPLAY)
		dma_display(path);
	if (test_dma & TEST_DMA_UPLOAD_SHORT)
		dma_upload_short(path);
	if (test_dma & TEST_DMA_READ_SIZE)
		dma_read_size(path);
	if (test_dma & TEST_DMA_RUN_PLAYBACK)
		dma_run_playback(path);

	if (test_dma & TEST_DMA_READ_SCOPE)
		dma_test_read_scope(path, 0, 0);
	if (test_dma & TEST_DMA_READ_SCOPE_TP)
		dma_test_read_scope(path, 1, 0);
	if (test_dma & TEST_DMA_READ_SCOPE_TP_NO_CHUNK)
		dma_test_read_scope(path, 1, NGPD_READ_SCOPE_NO_CHUNK);
	if (test_dma & TEST_DMA_READ_SCOPE_TP_FLOW_CONT)
		dma_test_read_scope(path, 1, NGPD_READ_SCOPE_FLOW_CONTROL);
*/
	if (test_dma & TEST_DMA_SCOPE_PB)
	{
		for (i=0; i<4; i++)
		{
			num_pb_streams = 1 << i;
			dma_test_scope_tp(path, card, 0, num_pb_streams);
		}
	}
	if (test_dma & TEST_DMA_SCOPE_TP)
		dma_test_scope_tp(path, card, 1, 0);

	return 0;
}
#if 0
#define NUM_WORDS_READ (32*32)
int dma_display(int path)
{
	int i, j, k, rc;
	u_int32_t rd_buff[NUM_WORDS_READ];
	unsigned int first, bytes_received;

	for (i=0; i<20; i++)
	{
		if (i==0)
			first = 0;
		else
			first = NUM_WORDS_READ << (i-1);

		printf("Calling ngpd_dma_read_playback(%d, %d, %d...)\n", path, first, NUM_WORDS_READ);
		rc = ngpd_dma_read_playback(path, first, NUM_WORDS_READ, rd_buff, &bytes_received);
		if (rc != 0)
			printf("Error calling ngpd_dma_read_playback: %s\n", ngpd_get_error_message());
		printf("Start offset=%X, bytes received=%d\n", first, bytes_received);
		for (j=0; j<64; j++)
		{
			printf("%08X:", first+j/2);
			for (k=0;k<16; k++)
				printf(" %08X", rd_buff[j*16+k]);
			printf("\n");
		}
	}
	return 0;
}

int dma_read_size(int path)
{
	int rc;
//	int j;
	u_int32_t *rd_buff;
	unsigned int first, bytes_received;
	size_t rd_size;
	rd_buff = (u_int32_t *)malloc((size_t)2*1024*1024*1024);
	
	printf("Testing reading different block sizes\n");

	for (rd_size=32*32; rd_size<=512*1024*1024; rd_size <<= 1)
	{
		first=0;
		printf("Calling ngpd_dma_read_playback(%d, %d, %zu...)\n", path, first, rd_size);
		rc = ngpd_dma_read_playback(path, first, rd_size, rd_buff, &bytes_received);
		if (rc != 0)
			printf("Error calling ngpd_dma_read_playback: message=%s, rc=%d\n", ngpd_get_error_message(), rc);
		printf("Size=%zx, bytes received=%d\n", rd_size, bytes_received);

/* 		for (j=0; j<64; j++)
		{
			printf("%08X:", first+j/2);
			for (k=0;k<16; k++)
				printf(" %08X", rd_buff[j*16+k]);
			printf("\n");
		}
*/
	}
	return 0;
}

int dma_run_playback(int path)
{
	u_int32_t pb_skip=8;
	int card = -1;
	dma_upload_medium(path);

	ngpd_write_glob_regs(path, NGPD_PLAYBACK_SKIP, 1, &pb_skip);
	
	ngpd_set_run_flags(path, NGPD_RUN_FLAGS_PLAYBACK | NGPD_RUN_FLAGS_PLAYBACK_CONTINUOUS | NGPD_RUN_FLAGS_SCOPEMODE);

	ngpd_dma_system_start(path, card, 0, NUM_WORDS_MEDIUM/16, NULL);
	return 0;
}

int dma_test_read_scope(int path, int use_tp, int read_options)
{
	int rc;
	int run_flags = 0;
	int io_errors=0;
	NGPDScopeModule * scope_mod = ngpd_scope_get_mod(path);
	int card = -1;

	if (scope_mod == NULL)
	{
		printf("Cannot gte pointer to scope module ; %s\n", ngpd_get_error_message());
		return -1;
	}
	ngpd_scope_set_streams(scope_mod, 4);
	fill_scope_mod(scope_mod);

	if (test_2stream)
		run_flags |= NGPD_RUN_FLAGS_SCOPE_OUT_2_STREAM;
	if (use_tp)
		run_flags |= NGPD_RUN_FLAGS_SCOPE_OUT_TEST;

	ngpd_set_run_flags(path, run_flags);
	if (scope_out_skip != 0)
	{
		ngpd_dma_set_scope_out_skip(path, scope_out_skip);
	}
	rc = ngpd_dma_read_scope(path, card, 0, 0, read_options); // 
	if (rc < 0)
	{
		printf("ngpd_dma_read_scope returns error: %s\n", ngpd_get_error_message());
		io_errors++;
	}

	if (use_tp)
		check_scope_read_tp_data(scope_mod);
	num_io_errors += io_errors;
	return 0;
}


#endif
#define BUFF_WORDS (1024*128)
int check_scope_read_tp_data(int path, int card)
{
	int t, dt, s, nt, ns;
	u_int16_t *p, expected;
	int64_t io_errors=0, errors=0;
	int dma, rc;
	ZYNQMP_DMA_StatusBlock statusBlock;
	int num_scope_dma;
	u_int16_t *buff[3];
	u_int64_t remaining;
	int chunk_bytes;
	u_int64_t chunk_start;
	u_int32_t scope_cont;
	u_int32_t chan_sel[2];

	rc = ngzmp_get_dma_status_block(path, card, &statusBlock);
	if (rc != 0)
	{
		mprintf(MP_ERR, "check_scope_read_tp_data: Error Reading Status Block %s\n", ngpd_get_error_message());
		io_errors++;
	}

	rc = ngpd_read_glob_regs(path, card, NGZMP_SCOPE_CONT, 1, &scope_cont);
	if (rc != 0)
	{
		mprintf(MP_ERR, "check_scope_read_tp_data: Error Reading Scope Cont %s\n", ngpd_get_error_message());
		io_errors++;
	}
	num_scope_dma = NGZMP_SCOPE_CONT_GET_NUM_STREAMS(scope_cont)+1;

	rc = ngpd_read_glob_regs(path, card, NGZMP_SCOPE_CHAN_SEL0, 2, chan_sel);
	if (rc != 0)
	{
		mprintf(MP_ERR, "check_scope_read_tp_data: Error Reading Scope Chan sel %s\n", ngpd_get_error_message());
		io_errors++;
	}
	for (dma=0; dma<num_scope_dma; dma++)
	{
		if ((buff[dma] = (u_int16_t *)malloc(BUFF_WORDS*sizeof(u_int16_t))) == NULL)
		{
			printf("Out of memory\n");
			exit(1);
		}
	}

	remaining = statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0].transfer_size;
	mprintf(1, ".... checking data from %d DMA, transfer size=%lld\n", num_scope_dma, remaining);
	chunk_start = 0;
	t = 0;
	while (remaining > 0)
	{
		if (remaining > BUFF_WORDS*sizeof(u_int16_t))
			chunk_bytes = BUFF_WORDS*sizeof(u_int16_t);
		else
			chunk_bytes = remaining;
		nt = chunk_bytes/(4*sizeof(u_int16_t));		
		for (dma=0; dma<num_scope_dma; dma++)
		{
			int num_lwords = chunk_bytes/sizeof(u_int32_t);
			int bytes_offset = chunk_start;
			if (dma == 2)
			{
				num_lwords /=2;
				bytes_offset /=2;
			}
			mprintf(3, ".... Reading DMA=%d, chunk byte start=%lld=0x%08llX, transfer_start=%lld=0x%llX, size=%lld=0x%08llX, nt=%d\n", 
				dma, bytes_offset, bytes_offset, statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0+dma].transfer_start, statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0+dma].transfer_start,
				 num_lwords*sizeof(u_int32_t), num_lwords*sizeof(u_int32_t), nt);
			if ((rc=zynqmp_read_dma_buff(path, card, NGZMP_DMA_STREAM_BNUM_SCOPE0, (bytes_offset+statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0+dma].transfer_start)/sizeof(u_int32_t), num_lwords, (u_int32_t *)buff[dma])) < 0)
			{
				mprintf(MP_ERR, "check_scope_read_tp_data: Error Reading Scope DMA buffer %s\n", ngpd_get_error_message());
				io_errors++;
			}
		}
		for (dt=0; dt<nt; dt++)
		{
			for (dma=0; dma<num_scope_dma; dma++)
			{
				p = (u_int16_t *)buff[dma];
				if (dma ==2)
				{
					ns = 2;
					p += dt*2;
				}
				else
				{
					ns = 4;
					p += dt*4;
				}
				for (s=0; s<ns; s++)
				{
					expected = 0x7FFF & (t*(s+dma*4+1));
					if (NGZMP_SCOPE_CHAN_SEL_GET(s+dma*4,chan_sel[0], chan_sel[1])&1)
						expected |= 0x8080;
					if (p[s] != expected)
					{
						errors++;
						if (errors < max_errors)
							mprintf(MP_ERR, "ERROR: t=%6d=0x%06X=(%d, %d), stream %d: Read %d, expected %d\n", t, t, t % 16384, t/16384, s+4*dma, p[s], expected);
						else if (errors == max_errors)
							mprintf(MP_ERR, "Too many errors, counting\n");
					}
					else if (reporting > 3)
						printf("Correct: t=%6d=0x%06X=(%d, %d), stream %d: Read %d\n", t, t, t % 16384, t/16384, s+4*dma, p[s]);
				}
			}
			t++;
		}
		chunk_start += chunk_bytes;
		remaining -= chunk_bytes;
	}
	if (io_errors == 0 && errors == 0)
		mprintf(1, "Checked scope mode buffer OK\n");
	else
	{
		mprintf(MP_ERR, "Found %lld errors and %lld io_errors checking scope mode buffer against test patterns\n", errors, io_errors);
	}
	num_errors += errors;
	num_io_errors += io_errors;
	for (dma=0; dma<num_scope_dma; dma++)
		free(buff[dma]);
	return 0;
}


int check_scope_read_pb_data(int path, int card, int num_pb_streams)
{
	int t, dt, s, nt, ns;
	u_int16_t *p, expected[NGZMP_SCOPE_MAX_STREAMS];
	int64_t io_errors=0, errors=0;
	int dma, rc;
	ZYNQMP_DMA_StatusBlock statusBlock;
	int num_scope_dma, num_scope_streams;
	u_int16_t *buff[3];
	u_int64_t remaining;
	int chunk_bytes;
	u_int64_t chunk_start;
	u_int32_t scope_cont;
	u_int32_t chan_sel[2];
	int stream_div=1;
	int pb_num_t=1;

	rc = ngzmp_get_dma_status_block(path, card, &statusBlock);
	if (rc != 0)
	{
		mprintf(MP_ERR, "check_scope_read_pb_data: Error Reading Status Block %s\n", ngpd_get_error_message());
		io_errors++;
	}

	rc = ngpd_read_glob_regs(path, card, NGZMP_SCOPE_CONT, 1, &scope_cont);
	if (rc != 0)
	{
		mprintf(MP_ERR, "check_scope_read_pb_data: Error Reading Scope Cont %s\n", ngpd_get_error_message());
		io_errors++;
	}
	num_scope_dma = NGZMP_SCOPE_CONT_GET_NUM_STREAMS(scope_cont)+1;

	rc = ngpd_read_glob_regs(path, card, NGZMP_SCOPE_CHAN_SEL0, 2, chan_sel);
	if (rc != 0)
	{
		mprintf(MP_ERR, "check_scope_read_pb_data: Error Reading Scope Chan sel %s\n", ngpd_get_error_message());
		io_errors++;
	}
	for (dma=0; dma<num_scope_dma; dma++)
	{
		if ((buff[dma] = (u_int16_t *)malloc(BUFF_WORDS*sizeof(u_int16_t))) == NULL)
		{
			printf("Out of memory\n");
			exit(1);
		}
	}
	switch (num_pb_streams)
	{
	case 1:
		stream_div = 8;	// Not used
		pb_num_t = statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].data_size/(sizeof(u_int16_t));
		break;
	case 2: 
		stream_div = 4;		// chan=0..3 <= pb 0, chan=3..7 <= pb 1
		pb_num_t = statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].data_size/(sizeof(u_int16_t)*2);
		break;

	case 4:
		stream_div = 2;	// chan=0,1 <= pb0, chan=2,3 <= pb1 ...
		pb_num_t = statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].data_size/(sizeof(u_int16_t)*4);
		break;

	case 8:
		stream_div = 1; // chan<n> <= Pb<n>
		pb_num_t = statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].data_size/(sizeof(u_int16_t)*8);
		break;

	default:
		printf("Coding error, unknown number of playback stream=%d\n", num_pb_streams);
		return -1;
	}
	mprintf(2, "... Note playback buffer size=%ld, pb_num_t=%d\n", statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_PLAYBACK0].data_size, pb_num_t);

	num_scope_streams = num_scope_dma*4;
	if (num_scope_streams > NGZMP_SCOPE_MAX_STREAMS)
		num_scope_streams = NGZMP_SCOPE_MAX_STREAMS;

	remaining = statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0].transfer_size;
	mprintf(2, ".... checking data from %d DMA, transfer size=%lld\n", num_scope_dma, remaining);
	chunk_start = 0;
	t = 0;
	while (remaining > 0)
	{
		if (remaining > BUFF_WORDS*sizeof(u_int16_t))
			chunk_bytes = BUFF_WORDS*sizeof(u_int16_t);
		else
			chunk_bytes = remaining;
		nt = chunk_bytes/(4*sizeof(u_int16_t));		
		for (dma=0; dma<num_scope_dma; dma++)
		{
			int num_lwords = chunk_bytes/sizeof(u_int32_t);
			int bytes_offset = chunk_start;
			if (dma == 2)
			{
				num_lwords /=2;
				bytes_offset /=2;
			}
			mprintf(3, ".... Reading DMA=%d, chunk start=%lld=0x%08llX, transfer_start=%lld=0x%llX, size=%lld=0x%08llX, dt=%d\n", 
				dma, chunk_start, chunk_start, statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0+dma].transfer_start, statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0+dma].transfer_start, chunk_bytes, chunk_bytes, nt);
			if ((rc=zynqmp_read_dma_buff(path, card, NGZMP_DMA_STREAM_BNUM_SCOPE0, (bytes_offset+statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0+dma].transfer_start)/sizeof(u_int32_t), num_lwords, (u_int32_t *)buff[dma])) < 0)
			{
				mprintf(MP_ERR, "check_scope_read_pb_data: Error Reading Scope DMA buffer %s\n", ngpd_get_error_message());
				io_errors++;
			}
		}
		for (dt=0; dt<nt; dt++)
		{
			if (num_pb_streams==1)
			{
				for (s=0; s<num_scope_streams; s++)
					expected[s] = t;
				expected[NGZMP_SCOPE_MAX_STREAMS-1] = 0;
			}
			else
			{
				u_int16_t x = t & 0xFFF;
				for (s=0; s<num_scope_streams; s++)
				{
					int pbs = s % NGZMP_CHANS_PER_CARD;
					pbs /= stream_div;
					expected[s] = x | pbs<<12;
				}
				expected[NGZMP_SCOPE_MAX_STREAMS-1] = 0;
			}

			for (dma=0; dma<num_scope_dma; dma++)
			{
				p = (u_int16_t *)buff[dma];
				if (dma ==2)
				{
					ns = 2;
					p += dt*2;
				}
				else
				{
					ns = 4;
					p += dt*4;
				}
				for (s=0; s<ns; s++)
				{
					if (p[s] != expected[s+4*dma])
					{
						errors++;
						if (errors < max_errors)
							mprintf(MP_ERR, "ERROR-PB: t=%6d=0x%06X=(%d, %d), stream %d: Read %d, expected %d\n", t, t, t % 16384, t/16384, s+4*dma, p[s], expected[s+4*dma]);
						else if (errors == max_errors)
							mprintf(MP_ERR, "Too many errors, counting\n");
					}
					else if (reporting > 3)
						printf("Correct: t=%6d=0x%06X=(%d, %d), stream %d: Read %d\n", t, t, t % 16384, t/16384, s+4*dma, p[s]);
				}
			}
			t = (t+1) % pb_num_t;
		}
		chunk_start += chunk_bytes;
		remaining -= chunk_bytes;
	}
	if (io_errors == 0 && errors == 0)
		mprintf(1, "Checked scope mode buffer OK using  num_pb_streams=%d, num_scope_stream=%d\n",  num_pb_streams, num_scope_streams);
	else
	{
		mprintf(MP_ERR, "Found %lld errors and %lld io_errors checking scope mode buffer against playback data with num_pb_streams=%d, num_scope_stream=%d\n", errors, io_errors, num_pb_streams, num_scope_streams);
	}
	num_errors += errors;
	num_io_errors += io_errors;
	for (dma=0; dma<num_scope_dma; dma++)
		free(buff[dma]);
	return 0;
}


int dma_test_scope_tp(int path, int card, int use_tp, int num_pb_streams)
{
	int rc;
	int first_code, last_code, nstreams_code, nstreams;
	int i;
	int run_flags;
	u_int32_t scope_cont, status;
	int errors=0;

	if (use_tp)
	{
		mprintf(1, "SCOPE-TP ... Testing scope mode capture of scope mode test pattern generator\n");
		// In TP test try 4, 8 and 10 streams
		first_code = 2;
		last_code = 2;
		num_pb_streams = 0;
	}
	else
	{
		mprintf(1, "SCOPE-PB ... Testing scope mode capture of playback data\n");
		first_code = 2;
		last_code = 2;
	}
	for (nstreams_code=first_code; nstreams_code<=last_code; nstreams_code++)
	{
		switch (nstreams_code)
		{
		case 0: nstreams = 4; break;
		case 1: nstreams = 8; break;
		case 2: nstreams = 10; break;
		}
		if (use_tp)
			mprintf(1, "Testing TPG to scope mode with number of scope streams =  nstreams=%d\n", nstreams);
		else
			mprintf(1, "Testing playback of %d streams to scope mode with number of scope streams =  nstreams=%d\n", num_pb_streams, nstreams);
		if (use_tp)
		{
			for (i=0; i<nstreams; i++)
				ngpd_scope_setup_stream(path, 0, i, 0, 0, 0);
		}
		else
		{
			u_int32_t chan_cont= NGZMP_CC_SRC_SET(NGZMP_CC_SRC_PB);
			for (i=0; i<nstreams; i++)
				ngpd_scope_setup_stream(path, 0, i, i % NGZMP_CHANS_PER_CARD, NGPD_SCOPE_SEL0TO5_INPUT, 0);
			ngpd_write_chan_regs(path, -1, NGZMP_REGION_REGS, NGPD_CHAN_CONT, 1, &chan_cont);
			build_tp_data(path, card, num_pb_streams);
		}

		run_flags = NGPD_RUN_FLAGS_SCOPEMODE | NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD;

		if (!use_tp)
			run_flags |= NGPD_RUN_FLAGS_PLAYBACK;

		if (manual_test)
		{
			if (use_tp)
				printf("Press Return to run test from test pattern generator with %d scope streams\n", nstreams);
			else
				printf("Press Return to run test from placback with %d streams to %d scope streams\n", num_pb_streams, nstreams);
			getchar();
		}
		rc = ngzmp_system_start_count_enb(path, card, num_pb_streams, nstreams, run_flags, 0);
		if (rc != 0)
			printf("ngpd_dma_system_start retuned error : %s\n", ngpd_get_error_message());
		
		ngpd_read_glob_regs(path, card, NGZMP_SCOPE_STATUS, 1, &status);
		printf("After start status = 0x%0X\n", status);
		if (manual_test)
		{
			printf("Press return to poll for DMA finished\n");
			getchar();
		}
		ngpd_dma_wait_scope(path, -1, NULL);
		ngpd_read_glob_regs(path, card, NGZMP_SCOPE_STATUS, 1, &status);
		printf("After run status = 0x%0X\n", status);
		if (status & NGZMP_SCOPE_STAT_SCOPE_RUNNING)
		{
			mprintf(MP_ERR, "ERROR: Scope Status still running when DMA finished, status=0x%08X\n", status);
			errors++;
		}
		if (status & NGZMP_SCOPE_STAT_SCOPE_OVERUN)
		{
			mprintf(MP_ERR, "ERROR: Scope Status OVER RUN detected, status=0x%08X\n", status);
			errors++;
		}
		if (use_tp==0 && (status & NGZMP_SCOPE_STAT_PB_UNDERUN)) 
		{
			mprintf(MP_ERR, "ERROR: Scope Status Playback UNDER RUN detected, status=0x%08X\n", status);
			errors++;
		}
		if (!use_tp)
		{
			u_int32_t dma_status;
			ZYNQMP_DMA_MsgPrintDesc msg;
			ngzmp_dma_read_status(path, card, NGZMP_DMA_STREAM_MASK_PLAYBACK0, &dma_status);
			printf("Playback DMA status=%08X\n", dma_status);
			memset(&msg, 0, sizeof(ZYNQMP_DMA_MsgPrintDesc));
			msg.num_desc=20;
			ngzmp_dma_print_desc(path, card, NGZMP_DMA_STREAM_BNUM_PLAYBACK0 , &msg);

		}

		if (manual_test)
		{
			printf("Press return to check data\n");
			getchar();
		}
		if (use_tp)
			check_scope_read_tp_data(path, card);
		else
			check_scope_read_pb_data(path, card, num_pb_streams);

		if (manual_test)
		{
			printf("Collected data with %d streams, press return\n", nstreams);
			getchar();
		}
	}
	if (errors > 0)
		mprintf(MP_ERREX, "Found %d status errors testing scope mode DMA\n", errors);
	num_errors  += errors;  
	return 0;
}

int build_tp_data(int path, int card, int num_pb_streams)
{
	int rc;
	ZYNQMP_DMA_MsgTestPat msg;
	int io_errors=0;
	memset(&msg, 0, sizeof(ZYNQMP_DMA_MsgTestPat));

	switch (num_pb_streams)
	{
	case 1:		msg.options = ZYNQMP_DMA_MSG_TP_PLAYBACK1; break;
	case 2:		msg.options = ZYNQMP_DMA_MSG_TP_PLAYBACK2; break;
	case 4:		msg.options = ZYNQMP_DMA_MSG_TP_PLAYBACK4; break;
	case 8:		msg.options = ZYNQMP_DMA_MSG_TP_PLAYBACK8; break;
	default:
			printf("Coding error: unexpected number of playback streams=%d\n", num_pb_streams);
			exit(1);
	}

	rc = ngzmp_dma_build_test_pat(path, card, NGZMP_DMA_STREAM_BNUM_PLAYBACK0, &msg);
	if (rc < 0 )
	{
		mprintf(MP_ERR, "Error building test patterns for playback: rc=%d\n", rc);
		io_errors++;
		return -1;
	}
	num_io_errors += io_errors;
	return 0;
}
