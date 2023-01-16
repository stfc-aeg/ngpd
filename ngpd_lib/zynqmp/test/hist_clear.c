/*
 * hist_reg_test.c
 *
 *  Created on: Jan 2021
 *      Author: wih
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "args.h"
#include "errors.h"


#include "ngzmp.h"
#include "ngpd.h"
#include "ngzmptest.h"
#include "zynqmp_lib_driver.h"
#include "ngzmp_lib_driver.h"
#include "ngzmp_driver.h"


#define RW_BLOCK_SIZE (1024*1024)
#define AXI_HIST_MAX_STREAMS 256
int check_clear_data(int path, int card, AXIHistConfig *hist_conf, u_int32_t *rbuf, int stream, int start_word, int total_words, int first_zero, int num_zero, int *errors);
int hist_clear_fill_inc(int path, int card, AXIHistConfig *hist_conf);
int test_hist_clear_check_imm(int path, int card);
int test_hist_clear_ind_stream(int path, int card);
int test_hist_clear_mult_stream(int path, int card);
int test_dma_buff_rd_small(int path, int card, int stream, int num_streams, char *name);
int test_dma_buff_wr_small(int path, int card, int stream, int num_streams, char *name);
int test_buff_rd_small(int path, int card, int addr_space, size_t total_mem_words, char *name);
int test_buff_wr_small(int path, int card, int addr_space, size_t total_mem_words, char *name);

int test_hist_memory(int path, int card)
{
	u_int32_t *rbuf, *wbuf,  *p, x;
	int i;
	AXIHistConfig hist_conf;
	int rc;
	int io_errors = 0;
	int errors = 0;
	size_t total_mem_words;
	size_t words_remaining;
	int offset;

	printf("Testing Read/Write to hist memory\n");

	rc = ngzmp_hist_read_config(path, card, &hist_conf);
	if (rc != 0)
	{
		io_errors++;
		printf("Error reading AXI Histogrammer config\n");
	}

	total_mem_words = ((u_int64_t)1)<<(hist_conf.NBitsAddrMem-2);

	if ((wbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	if ((rbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	words_remaining = total_mem_words;
	x = 0;
	offset = 0;

	while (	words_remaining)
	{
		int num;
		if (words_remaining > RW_BLOCK_SIZE)
			num = RW_BLOCK_SIZE;
		else
			num = words_remaining;
		
		p = wbuf;
		for (i=0; i<num; i++)
		{
			*p++ = x;
			x++;
		}
		if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_PL_RAM, offset, num, wbuf)) < 0)
		{
			io_errors++;
			if (io_errors < max_errors)
				printf("ERROR: zynqmp_write returned error %d\n", rc);
			else if (io_errors == max_errors)
				printf("Too many IO ERRORS: Counting\n");
		}
		words_remaining -= num;
		offset += num;
	}

	words_remaining = total_mem_words;
	x = 0;
	offset = 0;
	while (	words_remaining)
	{
		int num;
		if (words_remaining > RW_BLOCK_SIZE)
			num = RW_BLOCK_SIZE;
		else
			num = words_remaining;
		
		p = wbuf;
		for (i=0; i<num; i++)
		{
			*p++ = x;
			x++;
		}
		if ((rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_PL_RAM, offset, num, rbuf)) < 0)
		{
			io_errors++;
			if (io_errors < max_errors)
				printf("ERROR: zynqmp_read returned error %d\n", rc);
			else if (io_errors == max_errors)
				printf("Too many IO ERRORS: Counting\n");
		}

		for (i=0; i<num;i++)
		{
			if (wbuf[i] != rbuf[i])
			{
				errors++;
				if (errors < max_errors)
					printf("ERROR: Offset %d=%08X: Wrote %08X, Read %08X\n", offset+i, offset+i, wbuf[i], rbuf[i]);
				else if (errors == max_errors)
					printf("Too many errors. Counting\n");
			}
		}
		words_remaining -= num;
		offset += num;
	}

	if (errors != 0 || io_errors != 0)
	{
		printf("Found %d errors and %d IO error testing Write/Read to Histogram PL MEmroy\n", errors, io_errors);
		num_errors += errors;
		num_io_errors += io_errors;
	}
	else
		printf("Finished Hist PL memory write/read test with no errors\n");
	free(wbuf);
	free(rbuf);

	return 0;
}

/* Testing reading of Hist stream when the data is spread out by the stream being interleaved */
int test_hist_read_stream(int path, int card)
{
	u_int32_t *rbuf, *wbuf,  *p, expected;
	int i;
	AXIHistConfig hist_conf;
	int rc;
	int io_errors = 0;
	int errors = 0;
	int offset;
	int col, stream;
	int num;

	printf("Testing Read/Write to hist memory using the interleaved/sparse function ngzmp_hist_read_stream()\n");

	rc = ngzmp_hist_read_config(path, card, &hist_conf);
	if (rc != 0)
	{
		io_errors++;
		printf("Error reading AXI Histogrammer config\n");
	}
	if (hist_conf.StreamAtBottom == 0)
	{
		printf("Stream bits are at top so data is not interleaved\n");
		return 0;
	}

	if ((wbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	if ((rbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	offset = 0;

	p = wbuf;
	for (i=0; i<RW_BLOCK_SIZE/(hist_conf.HistWordsPerAXIWord*hist_conf.NumStreams); i++)
	{
		for (stream=0; stream<hist_conf.NumStreams; stream++)
		{
			for (col=0; col<hist_conf.HistWordsPerAXIWord; col++)
			{
				*p++ = col+hist_conf.HistWordsPerAXIWord*i+ 0x1000000*stream;
			}
		}
	}
	if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_PL_RAM, offset, RW_BLOCK_SIZE, wbuf)) < 0)
	{
		io_errors++;
		if (io_errors < max_errors)
			mprintf(MP_ERR, "ERROR: zynqmp_write returned error %d\n", rc);
		else if (io_errors == max_errors)
			mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
	}


	for (stream=0; stream<hist_conf.NumStreams; stream++)
	{
		if ((rc=ngzmp_hist_read_stream(path, card, stream, 0, RW_BLOCK_SIZE/hist_conf.NumStreams, rbuf)) < 0)
		{
			io_errors++;
			if (io_errors < max_errors)
				mprintf(MP_ERR, "ERROR: zynqmp_read returned error %d\n", rc);
			else if (io_errors == max_errors)
				mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
		}

		for (i=0; i<RW_BLOCK_SIZE/hist_conf.NumStreams;i++)
		{
			expected = i + 0x1000000*stream;
			if (rbuf[i] != expected)
			{
				errors++;
				if (errors < max_errors)
					mprintf(MP_ERR, "ERROR: Stream=%d, Hist Bin %d=%08X: Wrote %08X, Read %08X\n", stream, i, i, expected, rbuf[i]);
				else if (errors == max_errors)
					mprintf(MP_ERR, "Too many errors. Counting\n");
			}
		}
	}

	for (offset=0; offset<65; offset++)
	{
		for (num=1; num<135; num++)
		{
			mprintf(3, ".... Testing read from start word=0x%04X, num=0x%04X\n", offset, num);
			for (stream=0; stream<hist_conf.NumStreams; stream++)
			{
				memset(rbuf, 0, (num+10)*sizeof(u_int32_t));
				if ((rc=ngzmp_hist_read_stream(path, card, stream, offset, num, rbuf)) < 0)
				{
					io_errors++;
					if (io_errors < max_errors)
						mprintf(MP_ERR, "ERROR: zynqmp_read returned error %d\n", rc);
					else if (io_errors == max_errors)
						mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
				}

				for (i=0; i<num;i++)
				{
					expected = i+offset + 0x1000000*stream;
					if (rbuf[i] != expected)
					{
						errors++;
						if (errors < max_errors)
							mprintf(MP_ERR, "ERROR: Stream=%d, Hist Bin %d=%08X: Wrote %08X, Read %08X\n", stream, i+offset, i+offset, expected, rbuf[i]);
						else if (errors == max_errors)
							mprintf(MP_ERR, "Too many errors. Counting\n");
					}
				}
			}
		}
	}

	if (errors != 0 || io_errors != 0)
	{
		mprintf(MP_ERREX, "Found %d errors and %d IO error testing Write/Read to Histogram PL Memory by stream\n", errors, io_errors);
		num_errors += errors;
		num_io_errors += io_errors;
	}
	else
		printf("Finished Hist PL memory write/read by stream test with no errors\n");
	free(wbuf);
	free(rbuf);

	return 0;
}
#define MAX_ROW_LEN 9
#define MAX_NUM_STREAMS 5
/* Testing reading of Hist stream with other values of row_lem and row_stride to exercise the deive driver for future use */
int test_hist_read_row_len(int path, int card)
{
	u_int32_t *rbuf, *wbuf,  *p, expected, x;
	int i;
	AXIHistConfig hist_conf;
	int rc;
	int io_errors = 0;
	int errors = 0;
	int offset;
	int col, stream;
	int num;
	int row_len, num_streams;
	int max_index[MAX_NUM_STREAMS];

	printf("Testing Read/Write to hist memory using the interleaved/sparse function READ_ROW\n");

	rc = ngzmp_hist_read_config(path, card, &hist_conf);
	if (rc != 0)
	{
		io_errors++;
		printf("Error reading AXI Histogrammer config\n");
	}
	if (hist_conf.StreamAtBottom == 0)
	{
		printf("Stream bits are at top so data is not interleaved\n");
		return 0;
	}

	if ((wbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	if ((rbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	for (row_len=1; row_len<=MAX_ROW_LEN; row_len++)
	{
		for (num_streams=2; num_streams<=MAX_NUM_STREAMS; num_streams++)
		{
			mprintf(2, ".. Testing by row readout with row_len=%d, num_stream=%d, row_stride=%d\n", row_len, num_streams, row_len*num_streams);
			p = wbuf;
			for (i=0; i<RW_BLOCK_SIZE; i++)
			{
				if (row_len > 1)
					col = i % row_len;
				else 
					col = 0;
				stream = (i/row_len) % num_streams;
				x = col+ (i/(row_len*num_streams))*row_len;
				max_index[stream] = x;
				*p++ = x + 0x1000000*stream;
			}
			if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_PL_RAM, 0, RW_BLOCK_SIZE, wbuf)) < 0)
			{
				io_errors++;
				if (io_errors < max_errors)
					mprintf(MP_ERR, "ERROR: zynqmp_write returned error %d\n", rc);
				else if (io_errors == max_errors)
					mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
			}


			for (stream=0; stream<num_streams; stream++)
			{
				memset(rbuf,0, RW_BLOCK_SIZE*sizeof(u_int32_t));
				mprintf(4, ".... max_index[%d]=%d, RW_BLOCK=%d, num_tests=%d\n", stream, max_index[stream], RW_BLOCK_SIZE, RW_BLOCK_SIZE/num_streams);
				if ((rc=ngzmp_hist_read_row(path, card, row_len, num_streams, stream, 0, max_index[stream]+1, rbuf)) < 0)
				{
					io_errors++;
					if (io_errors < max_errors)
						mprintf(MP_ERR, "ERROR: zynqmp_read returned error %d\n", rc);
					else if (io_errors == max_errors)
						mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
				}
				for (i=0; i<=max_index[stream];i++)
				{
					expected = i + 0x1000000*stream;
					if (rbuf[i] != expected)
					{
						errors++;
						if (errors < max_errors)
							mprintf(MP_ERR, "ERROR: Stream=%d, Hist Bin %d=%08X: Wrote %08X, Read %08X\n", stream, i, i, expected, rbuf[i]);
						else if (errors == max_errors)
							mprintf(MP_ERR, "Too many errors. Counting\n");
					}
				}
			}
			for (offset=0; offset<65; offset++)
			{
				for (num=1; num<135; num++)
				{
					mprintf(3, ".... Testing read from start word=0x%04X, num=0x%04X\n", offset, num);
					for (stream=0; stream<num_streams; stream++)
					{
						memset(rbuf, 0, (num+10)*sizeof(u_int32_t));
						if ((rc=ngzmp_hist_read_row(path, card, row_len, num_streams, stream, offset, num, rbuf)) < 0)
						{
							io_errors++;
							if (io_errors < max_errors)
								mprintf(MP_ERR, "ERROR: zynqmp_read returned error %d\n", rc);
							else if (io_errors == max_errors)
								mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
						}

						for (i=0; i<num;i++)
						{
							expected = i+offset + 0x1000000*stream;
							if (rbuf[i] != expected)
							{
								errors++;
								if (errors < max_errors)
									mprintf(MP_ERR, "ERROR: Stream=%d, Hist Bin %d=%08X: Wrote %08X, Read %08X\n", stream, i+offset, i+offset, expected, rbuf[i]);
								else if (errors == max_errors)
									mprintf(MP_ERR, "Too many errors. Counting\n");
							}
						}
						if (errors >0 )
							exit(1);
					}
				}
			}
		}
	}
	if (errors != 0 || io_errors != 0)
	{
		printf("Found %d errors and %d IO error testing Write/Read to Histogram PL Memory by stream\n", errors, io_errors);
		num_errors += errors;
		num_io_errors += io_errors;
	}
	else
		printf("Finished Hist PL memory write/read by stream test with no errors\n");
	free(wbuf);
	free(rbuf);

	return 0;
}



int test_hist_tp_memory(int path, int card)
{
	return test_dma_buff_rw(path, card, NGZMP_DMA_STREAM_BNUM_HIST_TEST, 1, "Hist Test pattern");
}	 
int test_hist_rbuf_memory(int path, int card)
{
	return test_dma_buff_rw(path, card, NGZMP_DMA_STREAM_BNUM_HIST_READ, 1, "Hist Readback");
}	 
int test_playback_memory(int path, int card)
{
	return test_dma_buff_rw(path, card, NGZMP_DMA_STREAM_BNUM_PLAYBACK0, 2, "Playback");
}



int test_dma_buff_rw(int path, int card, int stream, int num_streams, char *name)
{
	u_int32_t *rbuf, *wbuf,  *p, x;
	int i;
	int rc;
	int io_errors = 0;
	int errors = 0;
	size_t total_mem_words;
	size_t words_remaining;
	int offset;
	ZYNQMP_DMA_StatusBlock dma_status;
	int pb_buff;


	mprintf(1, "Testing Read/Write to DMA buffer %s\n", name);

	if ((rc=zynqmp_read_dma_status_block(path, card, 0, sizeof(ZYNQMP_DMA_StatusBlock)/sizeof(u_int32_t), (u_int32_t *)&dma_status)) < 0)
	{
		io_errors++;
		mprintf(MP_ERR," ERROR Reading DMA status block\n");
	}
	printf("Memory for playback = %lu\n", dma_status.stream_def[stream].data_size);


	if ((wbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	if ((rbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	for (pb_buff=0; pb_buff<num_streams; pb_buff++)
	{
		total_mem_words = dma_status.stream_def[stream+pb_buff].data_size/sizeof(u_int32_t);
		words_remaining = total_mem_words;
		x = 0;
		offset = 0;

		while (	words_remaining)
		{
			int num;
			if (words_remaining > RW_BLOCK_SIZE)
				num = RW_BLOCK_SIZE;
			else
				num = words_remaining;
			
			p = wbuf;
			for (i=0; i<num; i++)
			{
				*p++ = x;
				x++;
			}
			if ((rc=zynqmp_write_dma_buff(path, card, stream+pb_buff, offset, num, wbuf)) < 0)
			{
				io_errors++;
				if (io_errors < max_errors)
					printf("ERROR: zynqmp_write_dma_buff returned error %d\n", rc);
				else if (io_errors == max_errors)
					printf("Too many IO ERRORS: Counting\n");
			}
			words_remaining -= num;
			offset += num;
		}

		words_remaining = total_mem_words;
		x = 0;
		offset = 0;
		while (	words_remaining)
		{
			int num;
			if (words_remaining > RW_BLOCK_SIZE)
				num = RW_BLOCK_SIZE;
			else
				num = words_remaining;
			
			p = wbuf;
			for (i=0; i<num; i++)
			{
				*p++ = x;
				x++;
			}
			if ((rc=zynqmp_read_dma_buff(path, card, stream+pb_buff, offset, num, rbuf)) < 0)
			{
				io_errors++;
				if (io_errors < max_errors)
					printf("ERROR: zynqmp_read_dma_buff returned error %d\n", rc);
				else if (io_errors == max_errors)
					printf("Too many IO ERRORS: Counting\n");
			}

			for (i=0; i<num;i++)
			{
				if (wbuf[i] != rbuf[i])
				{
					errors++;
					if (errors < max_errors)
						printf("ERROR: Offset %d=%08X: Wrote %08X, Read %08X\n", offset+i, offset+i, wbuf[i], rbuf[i]);
					else if (errors == max_errors)
						printf("Too many errors. Counting\n");
				}
			}
			words_remaining -= num;
			offset += num;
		}
	}
	if (errors != 0 || io_errors != 0)
	{
		printf("Found %d errors and %d IO error testing Write/Read to %s buffer\n", errors, io_errors, name);
		num_errors += errors;
		num_io_errors += io_errors;
	}
	else
		printf("Finished write/read test to %s with no errors\n", name);
	free(wbuf);
	free(rbuf);

	return 0;
}

int test_dma_buff_rw_small(int path, int card, int stream, int num_streams, char *name)
{
	test_dma_buff_rd_small(path, card, stream, num_streams, name);
	test_dma_buff_wr_small(path, card, stream, num_streams, name);
	return 0;
}
int test_buff_rw_small(int path, int card, int address_space, size_t total_mem_words, char *name)
{
	test_buff_rd_small(path, card, address_space, total_mem_words, name);
	test_buff_wr_small(path, card, address_space, total_mem_words, name);
	return 0;
}

int test_dma_buff_rd_small(int path, int card, int stream, int num_streams, char *name)
{
	u_int32_t *rbuf, *wbuf,  *p, x;
	int i;
	int rc;
	int io_errors = 0;
	int errors = 0;
	size_t total_mem_words;
	size_t words_remaining;
	int offset;
	ZYNQMP_DMA_StatusBlock dma_status;
	int pb_buff;
	int pass;
	int alignment=0;


	mprintf(1, "Testing Write then read small/unaligned regions of DMA buffer %s\n", name);

	if ((rc=zynqmp_read_dma_status_block(path, card, 0, sizeof(ZYNQMP_DMA_StatusBlock)/sizeof(u_int32_t), (u_int32_t *)&dma_status)) < 0)
	{
		io_errors++;
		mprintf(MP_ERR," ERROR Reading DMA status block\n");
	}
	printf("Memory for playback = %lu\n", dma_status.stream_def[stream].data_size);


	if ((wbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	if ((rbuf = malloc((RW_BLOCK_SIZE+2)*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	for (pb_buff=0; pb_buff<num_streams; pb_buff++)
	{
		total_mem_words = dma_status.stream_def[stream+pb_buff].data_size/sizeof(u_int32_t);
		words_remaining = total_mem_words;
		x = 0;
		offset = 0;

		while (	words_remaining)
		{
			int num;
			if (words_remaining > RW_BLOCK_SIZE)
				num = RW_BLOCK_SIZE;
			else
				num = words_remaining;
			
			p = wbuf;
			for (i=0; i<num; i++)
			{
				*p++ = x;
				x++;
			}
			if ((rc=zynqmp_write_dma_buff(path, card, stream+pb_buff, offset, num, wbuf)) < 0)
			{
				io_errors++;
				if (io_errors < max_errors)
					printf("ERROR: zynqmp_write_dma_buff returned error %d\n", rc);
				else if (io_errors == max_errors)
					printf("Too many IO ERRORS: Counting\n");
			}
			words_remaining -= num;
			offset += num;
		}

		for (pass=0; pass<256; pass++)
		{
	//		words_remaining = total_mem_words;
			words_remaining = (pass % 8)+1;
			offset = (pass/8) % 8;
			x = pass/64;
			if (x & 1)
				words_remaining += RW_BLOCK_SIZE-4;
			if (x & 2)
				offset += 1024-4;
			x = offset;
			alignment = 0;
			mprintf(3, ".... Pass %d: offset=%d, words_remaining=%d\n", pass, offset, words_remaining); 
			while (	words_remaining)
			{
				int num;
				if (words_remaining > RW_BLOCK_SIZE)
					num = RW_BLOCK_SIZE;
				else
					num = words_remaining;
				
				p = wbuf;
				for (i=0; i<num; i++)
				{
					*p++ = x;
					x++;
				}
				if ((rc=zynqmp_read_dma_buff(path, card, stream+pb_buff, offset, num, rbuf+alignment)) < 0)
				{
					io_errors++;
					if (io_errors < max_errors)
						printf("ERROR: zynqmp_read_dma_buff returned error %d\n", rc);
					else if (io_errors == max_errors)
						printf("Too many IO ERRORS: Counting\n");
				}

				for (i=0; i<num;i++)
				{
					if (wbuf[i] != rbuf[i+alignment])
					{
						errors++;
						if (errors < max_errors)
							printf("ERROR: Offset %d=%08X: Wrote %08X, Read %08X\n", offset+i, offset+i, wbuf[i], rbuf[i+alignment]);
						else if (errors == max_errors)
							printf("Too many errors. Counting\n");
					}
				}
				words_remaining -= num;
				offset += num;
			}
		}
	}
	if (errors != 0 || io_errors != 0)
	{
		printf("Found %d errors and %d IO error testing Write/Read small/unaligned to %s buffer\n", errors, io_errors, name);
		num_errors += errors;
		num_io_errors += io_errors;
	}
	else
		printf("Finished write/read small/unaligned test to %s with no errors\n", name);
	free(wbuf);
	free(rbuf);

	return 0;
}

int test_dma_buff_wr_small(int path, int card, int stream, int num_streams, char *name)
{
	u_int32_t *buf,  *p, x;
	int i;
	int rc;
	int io_errors = 0;
	int errors = 0;
	size_t total_mem_words;
	size_t words_remaining;
	int offset;
	ZYNQMP_DMA_StatusBlock dma_status;
	int pb_buff;
	int pass;
	int write_num_words, write_offset;
	u_int32_t expected;

	mprintf(1, "Testing Write to small regions of DMA buffer %s\n", name);

	if ((rc=zynqmp_read_dma_status_block(path, card, 0, sizeof(ZYNQMP_DMA_StatusBlock)/sizeof(u_int32_t), (u_int32_t *)&dma_status)) < 0)
	{
		io_errors++;
		mprintf(MP_ERR," ERROR Reading DMA status block\n");
	}

	if ((buf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	for (pb_buff=0; pb_buff<num_streams; pb_buff++)
	{
		total_mem_words = dma_status.stream_def[stream+pb_buff].data_size/sizeof(u_int32_t);
		if (total_mem_words > 2*RW_BLOCK_SIZE)
			total_mem_words = 2*RW_BLOCK_SIZE;

		for (pass=0; pass<256; pass++)
		{
			words_remaining = total_mem_words;
			offset = 0;

			p = buf;
			for (i=0; i<RW_BLOCK_SIZE; i++)
			{
				*p++ = 0xDEADBEEF;
			}
			while (	words_remaining)
			{
				int num;
				if (words_remaining > RW_BLOCK_SIZE)
					num = RW_BLOCK_SIZE;
				else
					num = words_remaining;
				
				if ((rc=zynqmp_write_dma_buff(path, card, stream+pb_buff, offset, num, buf)) < 0)
				{
					io_errors++;
					if (io_errors < max_errors)
						printf("ERROR: zynqmp_write_dma_buff returned error %d\n", rc);
					else if (io_errors == max_errors)
						printf("Too many IO ERRORS: Counting\n");
				}
				words_remaining -= num;
				offset += num;
			}

			write_num_words = (pass % 8)+1;
			write_offset = (pass/8) % 8;
			x = pass/64;
			if (x & 1)
				write_num_words += RW_BLOCK_SIZE-4;
			if (x & 2)
				write_offset += 1024-4;
			x = write_offset;
			mprintf(3, ".... Pass %d: offset=%d, words_remaining=%d\n", pass, write_offset, write_num_words); 

			words_remaining = write_num_words;
			offset = write_offset;
			while (	words_remaining)
			{
				int num;
				if (words_remaining > RW_BLOCK_SIZE)
					num = RW_BLOCK_SIZE;
				else
					num = words_remaining;
				
				p = buf;
				for (i=0; i<num; i++)
				{
					*p++ = x;
					x++;
				}
				if ((rc=zynqmp_write_dma_buff(path, card, stream+pb_buff, offset, num, buf)) < 0)
				{
					io_errors++;
					if (io_errors < max_errors)
						printf("ERROR: zynqmp_write_dma_buff returned error %d\n", rc);
					else if (io_errors == max_errors)
						printf("Too many IO ERRORS: Counting\n");
				}
				words_remaining -= num;
				offset += num;
			}
			words_remaining = total_mem_words;
			offset = 0;
							
			while (	words_remaining)
			{
				int num;
				if (words_remaining > RW_BLOCK_SIZE)
					num = RW_BLOCK_SIZE;
				else
					num = words_remaining;

				if ((rc=zynqmp_read_dma_buff(path, card, stream+pb_buff, offset, num, buf)) < 0)
				{
					io_errors++;
					if (io_errors < max_errors)
						printf("ERROR: zynqmp_read_dma_buff returned error %d\n", rc);
					else if (io_errors == max_errors)
						printf("Too many IO ERRORS: Counting\n");
				}
				for (i=0; i<num;i++)
				{
					if (offset < write_offset || offset >= write_offset+write_num_words)
						expected = 0xDEADBEEF;
					else
						expected = offset;
					if (buf[i] != expected)
					{
						errors++;
						if (errors < max_errors)
							printf("ERROR: Offset %d=%08X: Wrote %08X, Read %08X\n", offset, offset, expected, buf[i]);
						else if (errors == max_errors)
							printf("Too many errors. Counting\n");
					}
					offset++;
				}
				words_remaining -= num;
			}
		}
	}
	if (errors != 0 || io_errors != 0)
	{
		printf("Found %d errors and %d IO error testing Writing small regions to %s buffer\n", errors, io_errors, name);
		num_errors += errors;
		num_io_errors += io_errors;
	}
	else
		printf("Finished Writing small regions to %s with no errors\n", name);
	free(buf);

	return 0;
}

int test_buff_rd_small(int path, int card, int addr_space, size_t total_mem_words, char *name)
{
	u_int32_t *rbuf, *wbuf,  *p, x;
	int i;
	int rc;
	int io_errors = 0;
	int errors = 0;
	size_t words_remaining;
	int offset;
	int pass;

	mprintf(1, "Testing Write then read small/unaligned regions of address region %s\n", name);

	if ((wbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	if ((rbuf = malloc((RW_BLOCK_SIZE+2)*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	x = 0;
	offset = 0;

	if (total_mem_words > 4*RW_BLOCK_SIZE)
		total_mem_words = 4*RW_BLOCK_SIZE;

	words_remaining = total_mem_words;
	while (	words_remaining)
	{
		int num;
		if (words_remaining > RW_BLOCK_SIZE)
			num = RW_BLOCK_SIZE;
		else
			num = words_remaining;
		
		p = wbuf;
		for (i=0; i<num; i++)
		{
			*p++ = x;
			x++;
		}
		if ((rc=zynqmp_write(path, card, addr_space, offset, num, wbuf)) < 0)
		{
			io_errors++;
			if (io_errors < max_errors)
				printf("ERROR: zynqmp_write(offset=%d, num=%d) returned error %d\n", offset, num, rc);
			else if (io_errors == max_errors)
				printf("Too many IO ERRORS: Counting\n");
		}
		words_remaining -= num;
		offset += num;
	}

	for (pass=0; pass<256; pass++)
	{
//		words_remaining = total_mem_words;
		words_remaining = (pass % 8)+1;
		offset = (pass/8) % 8;
		x = pass/64;
		if (x & 1)
			words_remaining += RW_BLOCK_SIZE-4;
		if (x & 2)
			offset += 1024-4;
		x = offset;
		mprintf(3, ".... Pass %d: offset=%d, words_remaining=%d\n", pass, offset, words_remaining); 
		while (	words_remaining)
		{
			int num;
			if (words_remaining > RW_BLOCK_SIZE)
				num = RW_BLOCK_SIZE;
			else
				num = words_remaining;
			
			p = wbuf;
			for (i=0; i<num; i++)
			{
				*p++ = x;
				x++;
			}
			if ((rc=zynqmp_read(path, card, addr_space, offset, num, rbuf)) < 0)
			{
				io_errors++;
				if (io_errors < max_errors)
					printf("ERROR: zynqmp_read returned error %d\n", rc);
				else if (io_errors == max_errors)
					printf("Too many IO ERRORS: Counting\n");
			}

			for (i=0; i<num;i++)
			{
				if (wbuf[i] != rbuf[i])
				{
					errors++;
					if (errors < max_errors)
						printf("ERROR: Offset %d=%08X: Wrote %08X, Read %08X\n", offset+i, offset+i, wbuf[i], rbuf[i]);
					else if (errors == max_errors)
						printf("Too many errors. Counting\n");
				}
			}
			words_remaining -= num;
			offset += num;
		}
	}
	if (errors != 0 || io_errors != 0)
	{
		printf("Found %d errors and %d IO error testing Write/Read small/unaligned to %s address space\n", errors, io_errors, name);
		num_errors += errors;
		num_io_errors += io_errors;
	}
	else
		printf("Finished write/read small/unaligned test to %s address space with no errors\n", name);
	free(wbuf);
	free(rbuf);

	return 0;
}

int test_buff_wr_small(int path, int card, int addr_space, size_t total_mem_words, char *name)
{
	u_int32_t *buf,  *p, x;
	int i;
	int rc;
	int io_errors = 0;
	int errors = 0;
	size_t words_remaining;
	int offset;
	int pass;
	int write_num_words, write_offset;
	u_int32_t expected;

	mprintf(1, "Testing Write to small regions of address space %s\n", name);

	if ((buf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	if (total_mem_words > 2*RW_BLOCK_SIZE)
		total_mem_words = 2*RW_BLOCK_SIZE;

	for (pass=0; pass<256; pass++)
	{
		words_remaining = total_mem_words;
		offset = 0;

		p = buf;
		for (i=0; i<RW_BLOCK_SIZE; i++)
		{
			*p++ = 0xDEADBEEF;
		}
		while (	words_remaining)
		{
			int num;
			if (words_remaining > RW_BLOCK_SIZE)
				num = RW_BLOCK_SIZE;
			else
				num = words_remaining;
			
			if ((rc=zynqmp_write(path, card, addr_space, offset, num, buf)) < 0)
			{
				io_errors++;
				if (io_errors < max_errors)
					printf("ERROR: zynqmp_write returned error %d\n", rc);
				else if (io_errors == max_errors)
					printf("Too many IO ERRORS: Counting\n");
			}
			words_remaining -= num;
			offset += num;
		}

		write_num_words = (pass % 8)+1;
		write_offset = (pass/8) % 8;
		x = pass/64;
		if (x & 1)
			write_num_words += RW_BLOCK_SIZE-4;
		if (x & 2)
			write_offset += 1024-4;
		x = write_offset;
		mprintf(3, ".... Pass %d: offset=%d, words_remaining=%d\n", pass, write_offset, write_num_words); 

		words_remaining = write_num_words;
		offset = write_offset;
		while (	words_remaining)
		{
			int num;
			if (words_remaining > RW_BLOCK_SIZE)
				num = RW_BLOCK_SIZE;
			else
				num = words_remaining;
			
			p = buf;
			for (i=0; i<num; i++)
			{
				*p++ = x;
				x++;
			}
			if ((rc=zynqmp_write(path, card, addr_space, offset, num, buf)) < 0)
			{
				io_errors++;
				if (io_errors < max_errors)
					printf("ERROR: zynqmp_write returned error %d\n", rc);
				else if (io_errors == max_errors)
					printf("Too many IO ERRORS: Counting\n");
			}
			words_remaining -= num;
			offset += num;
		}
		words_remaining = total_mem_words;
		offset = 0;
						
		while (	words_remaining)
		{
			int num;
			if (words_remaining > RW_BLOCK_SIZE)
				num = RW_BLOCK_SIZE;
			else
				num = words_remaining;

			if ((rc=zynqmp_read(path, card, addr_space, offset, num, buf)) < 0)
			{
				io_errors++;
				if (io_errors < max_errors)
					printf("ERROR: zynqmp_read returned error %d\n", rc);
				else if (io_errors == max_errors)
					printf("Too many IO ERRORS: Counting\n");
			}
			for (i=0; i<num;i++)
			{
				if (offset < write_offset || offset >= write_offset+write_num_words)
					expected = 0xDEADBEEF;
				else
					expected = offset;
				if (buf[i] != expected)
				{
					errors++;
					if (errors < max_errors)
						printf("ERROR: Offset %d=%08X: Wrote %08X, Read %08X\n", offset, offset, expected, buf[i]);
					else if (errors == max_errors)
						printf("Too many errors. Counting\n");
				}
				offset++;
			}
			words_remaining -= num;
		}
	}
	if (errors != 0 || io_errors != 0)
	{
		printf("Found %d errors and %d IO error testing Writing small regions to %s address space\n", errors, io_errors, name);
		num_errors += errors;
		num_io_errors += io_errors;
	}
	else
		printf("Finished Writing small regions to %s address space with no errors\n", name);
	free(buf);

	return 0;
}


int test_hist_memory_speed(int path, int card)
{
	u_int32_t *rbuf, *wbuf,  *p, x;
	int i;
	AXIHistConfig hist_conf;
	int rc;
	int io_errors = 0;
	int errors = 0;
	size_t total_mem_words;
	size_t words_remaining;
	int offset;
	struct timeval before, after_read, after_write;
	double t_write, t_read;

	printf("Testing Read/Write to hist memory\n");

	rc = ngzmp_hist_read_config(path, card, &hist_conf);
	if (rc != 0)
	{
		io_errors++;
		printf("Error reading AXI Histogrammer config\n");
	}

	total_mem_words = ((u_int64_t)1)<<(hist_conf.NBitsAddrMem-2);

	if ((wbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	if ((rbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	words_remaining = total_mem_words;
	x = 0;
	offset = 0;

	p = wbuf;
	for (i=0; i<RW_BLOCK_SIZE; i++)
	{
		*p++ = x;
		x++;
	}
	gettimeofday(&before, NULL);
	while (	words_remaining)
	{
		int num;
		if (words_remaining > RW_BLOCK_SIZE)
			num = RW_BLOCK_SIZE;
		else
			num = words_remaining;
		
		if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_PL_RAM, offset, num, wbuf)) < 0)
		{
			io_errors++;
			if (io_errors < max_errors)
				printf("ERROR: zynqmp_write returned error %d\n", rc);
			else if (io_errors == max_errors)
				printf("Too many IO ERRORS: Counting\n");
		}
		words_remaining -= num;
		offset += num;
	}
	gettimeofday(&after_write, NULL);
	words_remaining = total_mem_words;
	x = 0;
	offset = 0;
	while (	words_remaining)
	{
		int num;
		if (words_remaining > RW_BLOCK_SIZE)
			num = RW_BLOCK_SIZE;
		else
			num = words_remaining;
		
		p = wbuf;
		if ((rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_PL_RAM, offset, num, rbuf)) < 0)
		{
			io_errors++;
			if (io_errors < max_errors)
				printf("ERROR: zynqmp_read returned error %d\n", rc);
			else if (io_errors == max_errors)
				printf("Too many IO ERRORS: Counting\n");
		}

		words_remaining -= num;
		offset += num;
	}
	gettimeofday(&after_read, NULL);

	t_write = after_write.tv_sec-before.tv_sec+1E-6*(after_write.tv_usec-before.tv_usec);
	t_read = after_read.tv_sec-after_write.tv_sec+1E-6*(after_read.tv_usec-after_write.tv_usec);
	printf("Time taken Write=%g, read=%g\n", t_write, t_read);
	printf("Data Rate Write=%g, Read=%g\n", sizeof(u_int32_t)*total_mem_words/t_write, sizeof(u_int32_t)*total_mem_words/t_read);
	if (errors != 0 || io_errors != 0)
	{
		printf("Found %d errors and %d IO error testing Write/Read to Histogram PL Memroy\n", errors, io_errors);
		num_errors += errors;
		num_io_errors += io_errors;
	}
	else
		printf("Finished Hist PL memory write/read test with no errors\n");

	free(wbuf);
	free(rbuf);
	return 0;
}

int test_hist_clear(int path, int card)
{
	if (test_hist_hist & TEST_HIST_CLEAR_CHECK_IMM)
		test_hist_clear_check_imm(path, card);
	if (test_hist_hist & TEST_HIST_CLEAR_IND_STREAM)
		test_hist_clear_ind_stream(path, card);
	if (test_hist_hist & TEST_HIST_CLEAR_MULT_STREAM)
		test_hist_clear_mult_stream(path, card);
	return 0;
}

int test_hist_clear_check_imm(int path, int card)
{
	AXIHistConfig hist_conf;
	int rc;
	int pass;
	size_t total_mem_words;
	size_t hist_words_per_stream;
	int axi_words_per_stream;
	int hist_words_per_axi_word;
	int stream, start_word, order, num_words, word_adjust;
	int nbits_axi_word_per_stream;
	u_int32_t *rbuf;
	int check_start, num_to_check;
	int errors = 0, io_errors=0;
	int i;

	rc = ngzmp_hist_read_config(path, card, &hist_conf);
	if (rc != 0)
	{
		io_errors++;
		printf("Error reading AXI Histogrammer config\n");
	}
	if (hist_conf.StreamAtBottom && 0)
	{
		mprintf(MP_ERR, "test_hist_clear_check_imm: Does not wrok with StreamAtBottom Memory layout\n");
		return 0;
	}
	if (hist_conf.NumStreams > AXI_HIST_MAX_STREAMS)
	{
		fprintf(stderr, "Number of streams =%d > AXI_HIST_MAX_STREAMS=%d, please recompile with larger AXI_HIST_MAX_STREAMS\n", hist_conf.NumStreams, AXI_HIST_MAX_STREAMS);
		exit(1);
	}
	total_mem_words = ((u_int64_t)1)<<(hist_conf.NBitsAddrMem-2);
	hist_words_per_stream = total_mem_words / hist_conf.NumStreams;
	hist_words_per_axi_word = hist_conf.NBitsDataAXI/hist_conf.NBitsDataHist;
	axi_words_per_stream = hist_words_per_stream/hist_words_per_axi_word;

	nbits_axi_word_per_stream = hist_conf.NBitsAddrIn;

	for (i=hist_conf.NBitsDataAXI/8; i>1; i>>=1)
		nbits_axi_word_per_stream--;
	if ((rbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	for (pass=0; pass<1; pass++)
	{
		mprintf(1, "Hist-clear on single streams, pass=%d\n", pass);
		hist_clear_fill_inc(path, card, &hist_conf);
		for (stream=0; stream<hist_conf.NumStreams; stream++)
		{
			start_word=stream%5;
			mprintf(2, ".... Clearing and checking Stream=%d\n", stream);
// 			for (order=0; order<nbits_axi_word_per_stream-1; order++)
 			for (order=0; order<6; order++)
			{
				word_adjust = ((stream/5+order) % 5)-2;
				num_words = 1<<order;
				num_words += word_adjust;
				if (num_words < 1)
					num_words = 1;
				else if (num_words+start_word > axi_words_per_stream)
				{
					num_words = axi_words_per_stream-start_word;
					if (num_words < 0)
					{
						printf("Coding error, ran out of space at stream=%d, start_word=%d, order=%d, num_words=%d, axi_words_per_stream=%d, nbits_axi_word_per_stream=%d\n",
								stream, start_word, order, num_words, axi_words_per_stream, nbits_axi_word_per_stream);
						exit(1);
					}
				}

				if (hist_conf.StreamAtBottom)
					mprintf(3, "...... Clearing from Hist Word 0x%08X for 0x%08X relative to stream and then checking\n",start_word*hist_words_per_axi_word, num_words*hist_words_per_axi_word);
				else
					mprintf(3, "...... Clearing from Hist Word 0x%08X for 0x%08X and then checking\n", (start_word+axi_words_per_stream*stream)*hist_words_per_axi_word, num_words*hist_words_per_axi_word);
				if (manual_test) 
				{
					printf("Press return to start clear from word Word 0x%08X for 0x%08X\n", (start_word+axi_words_per_stream*stream)*hist_words_per_axi_word, num_words*hist_words_per_axi_word); 
					getchar();
				}
				if (ngzmp_hist_clear_stream_start(path, card, stream, start_word, num_words) != 0)
				{
					printf("Error staring clear\n");
					io_errors++;
				}
				if (ngzmp_hist_clear_wait(path, card) != 0)
				{
					printf("Error waiting for clear to finish, %s\n", ngpd_get_error_message());
					return 0;
				}
				check_start = start_word-7;
				if (check_start < 0) check_start  = 0;
				num_to_check = num_words+14;
				if (num_to_check+check_start > axi_words_per_stream)
					num_to_check = axi_words_per_stream-check_start;
				if (check_start+num_to_check > start_word+num_words+7)
					num_to_check = start_word+num_words+7-check_start;

				check_clear_data(path, card, &hist_conf, rbuf, stream, check_start*hist_words_per_axi_word, num_to_check*hist_words_per_axi_word, start_word*hist_words_per_axi_word, num_words*hist_words_per_axi_word, &errors);

				start_word += num_words+7;
			}
		}
	}
	if (errors != 0 || io_errors != 0)
	{
		printf("Found %d errors and %d IO error checking Clear data \n", errors, io_errors);
		num_errors += errors;
		num_io_errors += io_errors;
	}
	free(rbuf);
	return 0;
}


int test_hist_clear_ind_stream(int path, int card)
{
	AXIHistConfig hist_conf;
	int rc;
	int pass;
	size_t total_mem_words;
	size_t hist_words_per_stream;
	int axi_words_per_stream;
	int hist_words_per_axi_word;
	int stream, start_word, order, num_words, word_adjust;
	int nbits_axi_word_per_stream;
	u_int32_t *rbuf;
	int check_start, num_to_check;
	int errors = 0, io_errors=0;
	int i;

	rc = ngzmp_hist_read_config(path, card, &hist_conf);
	if (rc != 0)
	{
		io_errors++;
		printf("Error reading AXI Histogrammer config\n");
	}
	if (hist_conf.StreamAtBottom && 0)
	{
		mprintf(MP_ERR, "test_hist_clear_ind_stream: Does not wrok with StreamAtBottom Memory layout\n");
		return 0;
	}
	if (hist_conf.NumStreams > AXI_HIST_MAX_STREAMS)
	{
		fprintf(stderr, "Number of streams =%d > AXI_HIST_MAX_STREAMS=%d, please recompile with larger AXI_HIST_MAX_STREAMS\n", hist_conf.NumStreams, AXI_HIST_MAX_STREAMS);
		exit(1);
	}
	total_mem_words = ((u_int64_t)1)<<(hist_conf.NBitsAddrMem-2);
	hist_words_per_stream = total_mem_words / hist_conf.NumStreams;
	hist_words_per_axi_word = hist_conf.NBitsDataAXI/hist_conf.NBitsDataHist;
	axi_words_per_stream = hist_words_per_stream/hist_words_per_axi_word;

	nbits_axi_word_per_stream = hist_conf.NBitsAddrIn;

	for (i=hist_conf.NBitsDataAXI/8; i>1; i>>=1)
		nbits_axi_word_per_stream--;
	if ((rbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	
	for (pass=0; pass<1; pass++)
	{
		mprintf(1, "Hist-clear on single streams\n");
		hist_clear_fill_inc(path, card, &hist_conf);
		for (stream=0; stream<hist_conf.NumStreams; stream++)
		{
			start_word=stream%5;
			mprintf(2, ".... Clear Stream=%d\n", stream);
 			for (order=0; order<nbits_axi_word_per_stream-1; order++)
// 			for (order=0; order<6; order++)
			{
				word_adjust = ((stream/5+order) % 5)-2;
				num_words = 1<<order;
				num_words += word_adjust;
				if (num_words < 1)
					num_words = 1;
				else if (num_words+start_word > axi_words_per_stream)
				{
					num_words = axi_words_per_stream-start_word;
					if (num_words < 0)
					{
						printf("Coding error, ran out of space at stream=%d, start_word=%d, order=%d, num_words=%d, axi_words_per_stream=%d, nbits_axi_word_per_stream=%d\n",
								stream, start_word, order, num_words, axi_words_per_stream, nbits_axi_word_per_stream);
						exit(1);
					}
				}

				mprintf(3, "...... Clearing from Hist Word 0x%08X for 0x%08X\n", (start_word+axi_words_per_stream*stream)*hist_words_per_axi_word, num_words*hist_words_per_axi_word);
				if (ngzmp_hist_clear_stream_start(path, card, stream, start_word, num_words) != 0)
				{
					printf("Error staring clear\n");
					io_errors++;
				}
				if (ngzmp_hist_clear_wait(path, card) != 0)
				{
					printf("Error waiting for clear to finish : %s\n", ngpd_get_error_message());
					return 0;
				}
				start_word += num_words+7;
			}
		}
		/* Now repeat, checking for data */
		for (stream=0; stream<hist_conf.NumStreams; stream++)
		{
			start_word=stream%5;
			mprintf(2, ".... Checking Stream=%d\n", stream);
 			for (order=0; order<nbits_axi_word_per_stream-1; order++)
			{
				word_adjust = ((stream/5+order) % 5)-2;
				num_words = 1<<order;
				num_words += word_adjust;
				if (num_words < 1)
					num_words = 1;
				else if (num_words+start_word > axi_words_per_stream)
				{
					num_words = axi_words_per_stream-start_word;
					if (num_words < 0)
					{
						printf("Coding error, ran out of space at stream=%d, start_word=%d, order=%d, num_words=%d, axi_words_per_stream=%d, nbits_axi_word_per_stream=%d\n",
								stream, start_word, order, num_words, axi_words_per_stream, nbits_axi_word_per_stream);
						exit(1);
					}
				}
				check_start = start_word-7;
				if (check_start < 0) check_start  = 0;
				num_to_check = num_words+14;
				if (num_to_check+check_start > axi_words_per_stream)
					num_to_check = axi_words_per_stream-check_start;
				if (check_start+num_to_check > start_word+num_words+7)
					num_to_check = start_word+num_words+7-check_start;
				mprintf(3, "...... Checking Clear from Hist Word 0x%08X for 0x%08X\n", (start_word+axi_words_per_stream*stream)*hist_words_per_axi_word, num_words*hist_words_per_axi_word);
				check_clear_data(path, card, &hist_conf, rbuf, stream, check_start*hist_words_per_axi_word, num_to_check*hist_words_per_axi_word, start_word*hist_words_per_axi_word, num_words*hist_words_per_axi_word, &errors);

				start_word += num_words+7;
			}
		}
	}
	if (errors != 0 || io_errors != 0)
	{
		printf("Found %d errors and %d IO error checking Clear data \n", errors, io_errors);
		num_errors += errors;
		num_io_errors += io_errors;
	}
	free(rbuf);
	return 0;
}

int test_hist_clear_mult_stream(int path, int card)
{
	AXIHistConfig hist_conf;
	int rc;
	int pass;
	size_t total_mem_words;
	size_t hist_words_per_stream;
	int axi_words_per_stream;
	int hist_words_per_axi_word;
	int stream, start_word, order, num_words, word_adjust;
	int nbits_axi_word_per_stream;
	u_int32_t *rbuf;
	int check_start, num_to_check;
	int errors = 0, io_errors=0;
	int i;

	rc = ngzmp_hist_read_config(path, card, &hist_conf);
		
	if (rc != 0)
	{
		io_errors++;
		printf("Error reading AXI Histogrammer config\n");
	}
	if (hist_conf.StreamAtBottom && 0)
	{
		mprintf(MP_ERR, "test_hist_clear_mult_stream: Does not wrok with StreamAtBottom Memory layout\n");
		return 0;
	}
	if (hist_conf.NumStreams > AXI_HIST_MAX_STREAMS)
	{
		fprintf(stderr, "Number of streams =%d > AXI_HIST_MAX_STREAMS=%d, please recompile with larger AXI_HIST_MAX_STREAMS\n", hist_conf.NumStreams, AXI_HIST_MAX_STREAMS);
		exit(1);
	}
	total_mem_words = ((u_int64_t)1)<<(hist_conf.NBitsAddrMem-2);
	hist_words_per_stream = total_mem_words / hist_conf.NumStreams;
	hist_words_per_axi_word = hist_conf.NBitsDataAXI/hist_conf.NBitsDataHist;
	axi_words_per_stream = hist_words_per_stream/hist_words_per_axi_word;

	nbits_axi_word_per_stream = hist_conf.NBitsAddrIn;

	for (i=hist_conf.NBitsDataAXI/8; i>1; i>>=1)
		nbits_axi_word_per_stream--;
	if ((rbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	
	for (pass=0; pass<10; pass++)
	{
		start_word=pass%5;
		mprintf(2,".... Hist-clear on multiple streams, pass=%d start_word offset=%d\n", pass);
		hist_clear_fill_inc(path, card, &hist_conf);
		for (order=0; order<nbits_axi_word_per_stream-1; order++)
		{
			word_adjust = ((pass/5+order) % 5)-2;
			num_words = 1<<order;
			num_words += word_adjust;
			if (num_words < 1)
				num_words = 1;
			else if (num_words+start_word > axi_words_per_stream)
			{
				num_words = axi_words_per_stream-start_word;
				if (num_words < 0)
				{
					printf("Coding error, ran out of space start_word=%d, order=%d, num_words=%d, axi_words_per_stream=%d, nbits_axi_word_per_stream=%d\n",
							start_word, order, num_words, axi_words_per_stream, nbits_axi_word_per_stream);
					exit(1);
				}
			}

			if (ngzmp_hist_clear_all_start(path, card, start_word, num_words) != 0)
			{
				printf("Error staring clear\n");
				io_errors++;
			}
			if (ngzmp_hist_clear_wait(path, card) != 0)
			{
				printf("Error waiting for clear to finish\n");
				return 0;
			}
			start_word += num_words+7;
		}
		/* Now repeat, checking for data */
		for (stream=0; stream<hist_conf.NumStreams; stream++)
		{
			start_word=pass%5;
 			for (order=0; order<nbits_axi_word_per_stream-1; order++)
			{
				word_adjust = ((pass/5+order) % 5)-2;
				num_words = 1<<order;
				num_words += word_adjust;
				if (num_words < 1)
					num_words = 1;
				else if (num_words+start_word > axi_words_per_stream)
				{
					num_words = axi_words_per_stream-start_word;
					if (num_words < 0)
					{
						printf("Coding error, ran out of space at start_word=%d, order=%d, num_words=%d, axi_words_per_stream=%d, nbits_axi_word_per_stream=%d\n",
								start_word, order, num_words, axi_words_per_stream, nbits_axi_word_per_stream);
						exit(1);
					}
				}
				check_start = start_word-7;
				if (check_start < 0) check_start  = 0;
				num_to_check = num_words+14;
				if (num_to_check+check_start > axi_words_per_stream)
					num_to_check = axi_words_per_stream-check_start;
				if (check_start+num_to_check > start_word+num_words+7)
					num_to_check = start_word+num_words+7-check_start;
				mprintf(3, "...... Checking Clear from Hist Word 0x%08X for 0x%08X\n", (start_word+axi_words_per_stream*stream)*hist_words_per_axi_word, num_words*hist_words_per_axi_word);
				check_clear_data(path, card, &hist_conf, rbuf, stream, check_start*hist_words_per_axi_word, num_to_check*hist_words_per_axi_word, start_word*hist_words_per_axi_word, num_words*hist_words_per_axi_word, &errors);

				start_word += num_words+7;
			}
		}
	}
	if (errors != 0 || io_errors != 0)
	{
		printf("Found %d errors and %d IO error checking Clear data \n", errors, io_errors);
		num_errors += errors;
		num_io_errors += io_errors;
	}
	free(rbuf);
	return 0;
}


int hist_clear_fill_inc(int path, int card, AXIHistConfig *hist_conf)
{
	u_int32_t *wbuf,  *p, x;
	int i;
	int rc;
	int io_errors = 0;
	size_t total_mem_words;
	size_t words_remaining;
	int offset;

	printf(".. Writing incrementing pattern\n");

	total_mem_words = ((u_int64_t)1)<<(hist_conf->NBitsAddrMem-2);

	if ((wbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}

	words_remaining = total_mem_words;
	x = 0;
	offset = 0;

	while (	words_remaining)
	{
		int num;
		if (words_remaining > RW_BLOCK_SIZE)
			num = RW_BLOCK_SIZE;
		else
			num = words_remaining;
		
		p = wbuf;
		for (i=0; i<num; i++)
		{
			*p++ = x;
			x++;
		}
		if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_PL_RAM, offset, num, wbuf)) < 0)
		{
			io_errors++;
			if (io_errors < max_errors)
				printf("ERROR: zynqmp_write returned error %d\n", rc);
			else if (io_errors == max_errors)
				printf("Too many IO ERRORS: Counting\n");
		}
		words_remaining -= num;
		offset += num;
	}
	free(wbuf);
	return 0;
}

int check_clear_data(int path, int card,  AXIHistConfig *hist_conf, u_int32_t *rbuf, int stream, int start_word, int total_words, int first_zero, int num_zero, int *errors)
{
	int start_errors= *errors, io_errors=0;
	int rc;
	int i;
	u_int32_t expected;

	while (	total_words)
	{
		int num;
		if (total_words > RW_BLOCK_SIZE)
			num = RW_BLOCK_SIZE;
		else
			num = total_words;
		
//		if ((rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_PL_RAM, start_word, num, rbuf)) < 0)
		if ((rc=ngzmp_hist_read_stream(path, card, stream, start_word, num, rbuf)) < 0)
		{
			io_errors++;
			if (io_errors < max_errors)
				printf("ERROR: zynqmp_read returned error %d\n", rc);
			else if (io_errors == max_errors)
				printf("Too many IO ERRORS: Counting\n");
		}

		for (i=0; i<num;i++)
		{
			if (start_word < first_zero || start_word >= first_zero+num_zero)
			{
				if (hist_conf->StreamAtBottom)
				{
					expected = start_word % hist_conf->HistWordsPerAXIWord;
					expected += stream*hist_conf->HistWordsPerAXIWord;
					expected += (start_word/hist_conf->HistWordsPerAXIWord)*(hist_conf->HistWordsPerAXIWord*hist_conf->NumStreams);
				}
				else
				{
					expected = start_word+stream*hist_conf->HistWordsPerStream;
				}
			}
			else
				expected = 0;
			if (expected != rbuf[i])
			{
				(*errors)++;
				if (*errors < max_errors)
					mprintf(MP_ERR, "ERROR: Stream %d: Offset %d=%08X: Expected %08X, Read %08X\n", stream, start_word, start_word, expected, rbuf[i]);
				else if (*errors == max_errors)
					mprintf(MP_ERR, "Too many errors. Counting\n");
			}
			else if (reporting >= 4)
				printf("Correct: Offset %d=%08X: Read %08X\n", start_word, start_word, rbuf[i]);
			start_word++;
		}
		total_words -= num;
	}

	if (*errors-start_errors != 0 || io_errors != 0)
	{
		mprintf(MP_ERR, "Found %d errors and %d IO error checking Clear data \n", *errors-start_errors, io_errors);
		num_io_errors += io_errors;
	}
	return io_errors;
}

int test_hist_memory_dma(int path, int card)
{
	u_int32_t *rbuf, *wbuf,  *p, x;
	int i;
	AXIHistConfig hist_conf;
	int rc;
	int io_errors = 0;
	int errors = 0;
	size_t total_mem_words;
	size_t words_remaining;
	int offset;
	int pass;
	ZYNQMP_DMA_MsgPrintDesc msg_print_desc;

	mprintf(1, "Testing Write to hist memory followed by read via hist DMA channel\n");

	rc = ngzmp_hist_read_config(path, card, &hist_conf);
	if (rc != 0)
	{
		io_errors++;
		mprintf(MP_ERR, "Error reading AXI Histogrammer config\n");
	}

	total_mem_words = ((u_int64_t)1)<<(hist_conf.NBitsAddrMem-2);

	if ((wbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		mprintf(MP_ERR, "Out of memory\n");
		exit(1);
	}

	if ((rbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		mprintf(MP_ERR, "Out of memory\n");
		exit(1);
	}

	words_remaining = total_mem_words;
	x = 0;
	offset = 0;

	while (	words_remaining)
	{
		int num;
		if (words_remaining > RW_BLOCK_SIZE)
			num = RW_BLOCK_SIZE;
		else
			num = words_remaining;
		
		p = wbuf;
		for (i=0; i<num; i++)
		{
			*p++ = x;
			x++;
		}
		if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_PL_RAM, offset, num, wbuf)) < 0)
		{
			io_errors++;
			if (io_errors < max_errors)
				mprintf(MP_ERR, "ERROR: zynqmp_write returned error %d\n", rc);
			else if (io_errors == max_errors)
				mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
		}
		words_remaining -= num;
		offset += num;
	}

	for (pass=0; pass<512; pass++)
	{
#if 0
		if (pass == 0)
			words_remaining = total_mem_words;
		else
			words_remaining = RW_BLOCK_SIZE-1024+(pass/16);
		offset = pass%16;
#else
		if (pass == 0)
			words_remaining = total_mem_words;
		else
//			words_remaining = RW_BLOCK_SIZE-1024+(pass/16);
			words_remaining = RW_BLOCK_SIZE-8+(pass%32);
		offset = 1024*(pass % 32) +pass/32;
#endif
		x = offset;
		mprintf(2, ".... Pass %d start=%d, num=%d\n", pass, offset, words_remaining);
		if (manual_test){ printf("Press return to run test\n"); getchar(); }
		while (	words_remaining)
		{
			int num;
			if (words_remaining > RW_BLOCK_SIZE)
				num = RW_BLOCK_SIZE;
			else
				num = words_remaining;
			
			p = wbuf;
			for (i=0; i<num; i++)
			{
				*p++ = x;
				x++;
			}
			if ((rc=ngzmp_hist_read_dma(path, card, offset, num, rbuf, AXI_HIST_RC_ACROSS_STREAMS)) < 0)
			{
				io_errors++;
				if (io_errors < max_errors)
					mprintf(MP_ERR, "ERROR: zynqmp_read returned error %s\n", ngpd_get_error_message());
				else if (io_errors == max_errors)
					mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
			}
			memset(&msg_print_desc ,0, sizeof(ZYNQMP_DMA_MsgPrintDesc));
			msg_print_desc.first_desc = 0;
			msg_print_desc.num_desc = 1;
			if (reporting >= 4)
				zynqmp_dma(path, 0, ZYNQMP_DMA_CMD_PRINT_DESC, NGZMP_DMA_STREAM_BNUM_HIST_READ, &msg_print_desc, NULL, NULL)  ;
			for (i=0; i<num;i++)
			{
				if (wbuf[i] != rbuf[i])
				{
					errors++;
					if (errors < max_errors)
						mprintf(MP_ERR, "ERROR: Offset %d=%08X: Wrote %08X, Read %08X\n", offset+i, offset+i, wbuf[i], rbuf[i]);
					else if (errors == max_errors)
						mprintf(MP_ERR, "Too many errors. Counting\n");
				}
			}
			words_remaining -= num;
			offset += num;
		}
	}

	if (errors != 0 || io_errors != 0)
	{
		mprintf(MP_ERR, "Found %d errors and %d IO error testing Write/Read via DMA to Histogram PL Memory\n", errors, io_errors);
		num_errors += errors;
		num_io_errors += io_errors;
	}
	else
		mprintf(1, "Finished Hist PL memory write/read via DMA test with no errors\n");
	free(wbuf);
	free(rbuf);

	return 0;
}

/* Test Histogram memory using test pattern gerenator part of AXIHist to write and DMA to read. Does not Hist DMA buffer to be in CPU address space.
*/

int test_hist_memory_tpgen_dma(int path, int card)
{
	u_int32_t *rbuf, *p, expected;
	int i, j;
	AXIHistConfig hist_conf;
	int rc;
	int io_errors = 0;
	int errors = 0;
	size_t total_mem_words;
	size_t words_remaining;
	int offset;
	int wr_pass, rd_pass;
	int hist_words_per_axi_word;
	int axi_words_per_stream;
	int stream;
	u_int32_t tpgen;
	ZYNQMP_DMA_MsgPrintDesc msg_print_desc;
	int hist_words_per_stream;
	int do_clear;
	int num_pass=2;
	int write_all_streams=1;
	int read_across_streams=0;
	u_int32_t rd_flags=0;
	int num_words;

	mprintf(1, "Testing TPGEN write to hist memory followed by read via hist DMA channel\n");

	rc = ngzmp_hist_read_config(path, card, &hist_conf);
	if (rc != 0)
	{
		io_errors++;
		mprintf(MP_ERR, "Error reading AXI Histogrammer config\n");
	}

	total_mem_words = ((u_int64_t)1)<<(hist_conf.NBitsAddrMem-2);
	hist_words_per_axi_word = hist_conf.NBitsDataAXI/hist_conf.NBitsDataHist;
	hist_words_per_stream = total_mem_words/hist_conf.NumStreams;
	axi_words_per_stream = total_mem_words/(hist_words_per_axi_word*hist_conf.NumStreams);

	if ((rbuf = malloc(RW_BLOCK_SIZE*sizeof(u_int32_t))) == NULL)
	{
		mprintf(MP_ERR, "Out of memory\n");
		exit(1);
	}
	if (hist_conf.StreamAtBottom)
		num_pass = 4;

	for (wr_pass=0; wr_pass<num_pass; wr_pass++)
	{
		do_clear = !(wr_pass & 1);
		if (hist_conf.StreamAtBottom)
		{
			read_across_streams = wr_pass/2;
			write_all_streams = read_across_streams;
			if (read_across_streams)
				rd_flags = AXI_HIST_RC_ACROSS_STREAMS;
			else
				rd_flags = 0;
			mprintf(2, ".. Write pass %d, do_clear=%d, Read/Write across streams=%d\n", wr_pass, do_clear, read_across_streams);
		}
		else
		{
			mprintf(2, ".. Write pass %d, do_clear=%d\n", wr_pass, do_clear);
		}
		if (hist_conf.StreamAtBottom && write_all_streams == 0)
		{
			for (stream=0; stream<hist_conf.NumStreams; stream++)
			{
				if (do_clear)
				{
					if (manual_test) {printf("Press return to start clear\n"); getchar(); };
					if ((rc=ngzmp_hist_clear_stream_start(path, card, stream, 0, axi_words_per_stream)) < 0)
					{
						io_errors++;
						if (io_errors < max_errors)
							mprintf(MP_ERR, "ERROR: ngzmp_hist_clear_start_tpgen returned error %s\n", ngpd_get_error_message());
						else if (io_errors == max_errors)
							mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
					}
				}
				else
				{
					if (manual_test) {printf("Press return to write test pattern\n"); getchar(); };
					if ((rc=ngzmp_hist_clear_start_tpgen(path, card, stream, axi_words_per_stream, write_all_streams)) < 0)
					{
						io_errors++;
						if (io_errors < max_errors)
							mprintf(MP_ERR, "ERROR: ngzmp_hist_clear_start_tpgen returned error %s\n", ngpd_get_error_message());
						else if (io_errors == max_errors)
							mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
					}
				}
				if (ngzmp_hist_clear_wait(path, card) != 0)
				{
					io_errors++;
					if (io_errors < max_errors)
						mprintf(MP_ERR, "ERROR: ngzmp_hist_clear_wait returned error %s\n", ngpd_get_error_message());
					else if (io_errors == max_errors)
						mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
				}
			}
		}
		else
		{
			if (do_clear)
			{
				if (manual_test) {printf("Press return to start clear all streams\n"); getchar(); };
				if ((rc=ngzmp_hist_clear_all_start(path, card, 0, axi_words_per_stream)) < 0)
				{
					io_errors++;
					if (io_errors < max_errors)
						mprintf(MP_ERR, "ERROR: ngzmp_hist_clear_start_tpgen returned error %s\n", ngpd_get_error_message());
					else if (io_errors == max_errors)
						mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
				}
			}
			else
			{
				if (manual_test) {printf("Press return to write test pattern with write_all_streams=%d\n", write_all_streams); getchar(); };
				num_words = hist_conf.StreamAtBottom?axi_words_per_stream*hist_conf.NumStreams:axi_words_per_stream; 
				if ((rc=ngzmp_hist_clear_start_tpgen(path, card, 0, num_words, write_all_streams)) < 0)
				{
					io_errors++;
					if (io_errors < max_errors)
						mprintf(MP_ERR, "ERROR: ngzmp_hist_clear_start_tpgen returned error %s\n", ngpd_get_error_message());
					else if (io_errors == max_errors)
						mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
				}
			}
			if (ngzmp_hist_clear_wait(path, card) != 0)
			{
				io_errors++;
				if (io_errors < max_errors)
					mprintf(MP_ERR, "ERROR: ngzmp_hist_clear_wait returned error %s\n", ngpd_get_error_message());
				else if (io_errors == max_errors)
					mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
			}
		}

		for (rd_pass=0; rd_pass<32; rd_pass++)
		{
			int num_stream_read;
			num_stream_read = hist_conf.NumStreams;
			if (hist_conf.StreamAtBottom && read_across_streams)
				num_stream_read=1;
			if (0 && manual_test){ printf("Press return to read data pass %d\n", rd_pass); getchar(); }
			for (stream=0; stream<num_stream_read; stream++)
			{
				if (rd_pass == 0)
					words_remaining = axi_words_per_stream;
				else
					words_remaining = RW_BLOCK_SIZE/hist_words_per_axi_word-8+(rd_pass%17);
				offset = rd_pass * 0x1001;

				mprintf(3, ".... Pass %d start=%d, num=%d, stream=%d\n", rd_pass, offset, words_remaining, stream);
				if (do_clear)
				{
					tpgen = 0;
				}
				else
				{
					if (hist_conf.StreamAtBottom && read_across_streams==0)
						tpgen = 1;	// Same data sent to all stream in this mode.
					else
						tpgen = 1 << stream;
					for (i=0; i<offset; i++)
						tpgen = tpgen << 1 |  ((tpgen >> 31 & 1) ^ (tpgen >> 29&1));
				}

				while (	words_remaining)
				{
					int num;
					if (words_remaining > RW_BLOCK_SIZE/hist_words_per_axi_word)
						num = RW_BLOCK_SIZE/hist_words_per_axi_word;
					else
						num = words_remaining;
					if (hist_conf.StreamAtBottom && read_across_streams)
					{
						// mprintf(3, ".... calling ngzmp_hist_read_dma(offset=%08X, num=%08X, flags=%08X\n", offset*hist_words_per_axi_word+stream*hist_words_per_stream, num*hist_words_per_axi_word, rd_flags );
						if ((rc=ngzmp_hist_read_dma(path, card, offset*hist_words_per_axi_word+stream*hist_words_per_stream, num*hist_words_per_axi_word, rbuf, rd_flags)) < 0)
						{
							io_errors++;
							if (io_errors < max_errors)
								mprintf(MP_ERR, "ERROR: ngzmp_hist_read_dma returned error %s\n", ngpd_get_error_message());
							else if (io_errors == max_errors)
								mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
						}
					}
					else
					{
						if ((rc=ngzmp_hist_read_stream_dma(path, card, stream, offset*hist_words_per_axi_word, num*hist_words_per_axi_word, rbuf, 0)) < 0)
						{
							io_errors++;
							if (io_errors < max_errors)
								mprintf(MP_ERR, "ERROR: ngzmp_hist_read_stream_dma returned error %s\n", ngpd_get_error_message());
							else if (io_errors == max_errors)
								mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
						}
					}
					memset(&msg_print_desc ,0, sizeof(ZYNQMP_DMA_MsgPrintDesc));
					msg_print_desc.first_desc = 0;
					msg_print_desc.num_desc = 1;
					if (reporting >= 4)
						zynqmp_dma(path, 0, ZYNQMP_DMA_CMD_PRINT_DESC, NGZMP_DMA_STREAM_BNUM_HIST_READ, &msg_print_desc, NULL, NULL)  ;
					p = rbuf;
					if (do_clear)
					{
						for (i=0; i<num;i++)
						{
							for (j=0; j<hist_words_per_axi_word; j++)
							{
								if (0 != *p)
								{
									errors++;
									if (errors < max_errors)
										mprintf(MP_ERR, "ERROR: stream %d Word Offset %d=%08X: Wrote %08X, Read %08X\n", stream, 
											(offset+i)*hist_words_per_axi_word+j, (offset+i)*hist_words_per_axi_word+j, 0, *p);
									else if (errors == max_errors)
										mprintf(MP_ERR, "Too many errors. Counting\n");
								}
								p++;
							}
						}
					}
					else
					{
						for (i=0; i<num;i++)
						{
							for (j=0; j<hist_words_per_axi_word; j++)
							{
								if (j == 0)
									expected = tpgen;
								else if (j == 1)
									expected = ~tpgen;
								else expected = tpgen ^ (j*257);
								if (expected != *p)
								{
									errors++;
									if (errors < max_errors)
										mprintf(MP_ERR, "ERROR: stream %d Word Offset %d=%08X: Wrote %08X, Read %08X\n", stream, (offset+i)*hist_words_per_axi_word+j, (offset+i)*hist_words_per_axi_word+j, 
											expected, *p);
									else if (errors == max_errors)
										mprintf(MP_ERR, "Too many errors. Counting\n");
								}
								p++;
							}
							tpgen = tpgen << 1 |  ((tpgen >> 31 & 1) ^ (tpgen >> 29&1));
						}
					}
					words_remaining -= num;
					offset += num;
				}
			}
		}
	}
	if (errors != 0 || io_errors != 0)
	{
		mprintf(MP_ERR, "Found %d errors and %d IO error testing TPGEN Write/Read via DMA to Histogram PL Memory\n", errors, io_errors);
		num_errors += errors;
		num_io_errors += io_errors;
	}
	else
		mprintf(1, "Finished Hist PL memory test pattern write DMA read t with no errors\n");
	free(rbuf);

	return 0;
}
