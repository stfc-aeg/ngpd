/*
 * bram_test.c
 *
 *  Created on: 11/6/2021 
 *      Author: wih73
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "errors.h"

#include "ngpd.h"
#include "ngzmp.h"
#include "ngzmptest.h"

#define MAX_BRAM_SIZE 1024
#define CHUNK 256

int test_bram_wrrd(int path)
{
	int rc, i, j, k;
	u_int32_t *send_buf, *recv_buf, mask, x;
	int mem_size;
	int errors = 0;		
	int io_errors=0;

	if ((send_buf = (u_int32_t *)malloc(MAX_BRAM_SIZE*sizeof(u_int32_t))) == NULL)
	{
		fprintf(stderr, "No memory\n");
		exit(1);
	}
	if ((recv_buf = (u_int32_t *)malloc(MAX_BRAM_SIZE*sizeof(u_int32_t))) == NULL)
	{
		fprintf(stderr, "No memory\n");
		exit(1);
	}

	mprintf(1, "WRITE & READ BRAM MEMORY LOCATIONS\n");

	for (i=0; i<num_chan; i++) 
	{ // all possible channels
		for (j=1; j<NGZMP_NUM_REGIONS; j++)
		{ // all possible regions
			if ((mem_size=ngzmp_bram_size_table[j]) != 0)
			{
				if (ngzmp_bram_width_table[j] == 32)
					mask = 0xFFFFFFFF;
				else
					mask = (1 << ngzmp_bram_width_table[j])-1;
				mprintf(2, "Writing Chan %d, region %d, function %s\n", i, j, ngzmp_bram_name[j]);
				x = i*9+j;
				for (k=0; k<mem_size; k++) // all possible BRAM locations
				{
					send_buf[k] = mask & x;
					x++;
				}

				rc = ngpd_write_chan_regs(path, i, j, 0, mem_size, send_buf);
				if (rc != 0) {
					mprintf(MP_ERR, "IO Error on writing BRAM at channel %d region %d offset %d: %d\n", i, j, k, rc);
					io_errors++;
				}
			}
		}
	}

	for (i=0; i<num_chan; i++) 
	{ // all possible channels
		for (j=1; j<NGZMP_NUM_REGIONS; j++)
		{ // all possible regions
			if ((mem_size=ngzmp_bram_size_table[j]) != 0)
			{
				if (ngzmp_bram_width_table[j] == 32)
					mask = 0xFFFFFFFF;
				else
					mask = (1 << ngzmp_bram_width_table[j])-1;
				x = i*9+j;
				for (k=0; k<mem_size; k++) // all possible BRAM locations
				{
					send_buf[k] = mask & x;
					x++;
				}

				rc = ngpd_read_chan_regs(path, i, j, 0, mem_size, recv_buf);
				if (rc != 0) {
					mprintf(MP_ERR, "Error on reading BRAM at channel %d region %d offset %d: %d\n", i, j, k, rc);
					io_errors++;
					break;
				}
				for (k=0; k<mem_size; k++) // all possible BRAM locations
				{
					if (send_buf[k] != recv_buf[k])
					{
						errors++;
						if (errors < max_errors)
							mprintf(MP_ERR, "ERROR: Chan  %d, region %d, offset %d. Wrote %08X, read %08X\n", i, j, k, send_buf[k], recv_buf[k]);
						else if (errors == max_errors)
							mprintf(MP_ERR, "Too Many Errors, counting\n");
						
					}
				}
			}

		}
	}
	free(send_buf);
	free(recv_buf);
	if (errors == 0 && io_errors == 0)
		mprintf(1, "Passed BRAM data tests\n");
	else
		mprintf(MP_ERR, "BRAM tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}
//#undef XSP3_REGION_RAM_MAX
//#define XSP3_REGION_RAM_MAX 1
int test_bram_width(int path)
{
	int rc, i, j;
	u_int32_t mask;
	int mem_size;
	int errors = 0;		
	int io_errors=0;
	u_int32_t value;

	mprintf(1, "BRAM-WIDTH Write & read memory locations to determine width\n");

	for (i=0; i<num_chan; i++) 
	{ // all possible channels
		for (j=1; j<NGZMP_NUM_REGIONS; j++)
		{ // all possible regions
			if ((mem_size=ngzmp_bram_size_table[j]) != 0)
			{
				if (ngzmp_bram_width_table[j] == 32)
					mask = 0xFFFFFFFF;
				else
					mask = (1 << ngzmp_bram_width_table[j])-1;
				value=0xFFFFFFFF;
				rc = ngpd_write_chan_regs(path, i, j, 0, 1, &value);
				if (rc != 0) {
					mprintf(MP_ERR, "Error on calling ngzmp_write_chan_reg(): %d\n", rc);
					io_errors++;

				}

				rc = ngpd_read_chan_regs(path, i, j, 0, 1, &value);
				if (rc != 0) {
					mprintf(MP_ERR, "Error on calling ngzmp_read_chan_reg(): %d\n", rc);
					io_errors++;
				}
				if (value == mask)
					mprintf(3, "Chan %d, BRAM=%s, Wrote 0x%08X, Read 0x%08X\n", i, ngzmp_bram_name[j], 0xFFFFFFFF, value);
				else
				{
					errors++;
					mprintf(MP_ERR, "ERROR: Chan %d, BRAM=%s, Wrote 0x%08X, Read 0x%08X, Expected 0x%08X\n", i, ngzmp_bram_name[j], 0xFFFFFFFF, value, mask);
				}
			}
		}
	}
	if (errors == 0 && io_errors == 0)
		mprintf(1, "Passed BRAM Width tests\n");
	else
		mprintf(MP_ERR, "BRAM Widths tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}

#if 0
int test_dma_buff_wr_rd(int path, int card, int stream)
{
	int rc = 0;
	int errors=0, io_errors=0;
	u_int32_t *send_buf=NULL;
	u_int32_t *recv_buf=NULL;
	int i, pass;
	int block_size=1024*1024;

	printf("Testing xsp3m_write_dma_buff, xsp3m_write_dma_buff (1024x1024 words)\n");

	if ((send_buf = (u_int32_t *)malloc(block_size*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		io_errors++;
		return -1;
	}
	if ((recv_buf = (u_int32_t *)malloc(block_size*sizeof(u_int32_t))) == NULL)
	{
		printf("Out of memory\n");
		io_errors++;
		free(send_buf);
		return -1;
	}

	for (pass=0; pass<10; pass++)
	{
		for (i=0;i<block_size;i++)
			send_buf[i] = i+(pass<<24);

		rc = xsp3m_write_dma_buff(path, card, stream, pass, block_size, send_buf);
		if (rc != XSP3_OK)
		{
			io_errors++;
		}
		rc = xsp3m_read_dma_buff(path, card, stream, pass, block_size, recv_buf);
		if (rc != XSP3_OK)
		{
			io_errors++;
		}
		for (i=0;i<block_size;i++)
		{
			if (send_buf[i] != recv_buf[i])
			{
				errors++;
				if (errors < max_errors)
					printf("ERROR at offset %d, read 0x%08X, Expected 0x%08X\n", i, recv_buf[i], send_buf[i]);
				else if (errors == max_errors)
					printf("Too many errors. Counting\n");
			}
		}
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed DMA Buffer write/read tests\n");
	else
		printf("DMA Buffer write/read tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
	free(send_buf);
	free(recv_buf);
	return 0;
}

#endif
