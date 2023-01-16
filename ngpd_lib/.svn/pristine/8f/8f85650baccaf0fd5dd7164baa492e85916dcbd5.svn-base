/*
 * dram_test.c
 *
 *  Created on: 16/6/2016
 *      Author: wih73
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xspress3.h"
#include "xspress3test.h"
#include "xspress4_lib_driver.h"
#include "xspress4_protocol.h"

#define MAX_BLOCK_WORDS (128*1024)

int test_v7_dram(int path)
{
	int rc, i;
	u_int32_t *send_buf, *recv_buf, x;
	int errors = 0;		
	int io_errors=0;
	int total_dram_words= 512*1024*1024/4;
	int num_blocks = total_dram_words/MAX_BLOCK_WORDS;
	int block;
	int addr_space = XSP4_BUS_RDRAM;

	if ((send_buf = (u_int32_t *)malloc(MAX_BLOCK_WORDS*sizeof(u_int32_t))) == NULL)
	{
		fprintf(stderr, "No memory\n");
		exit(1);
	}
	if ((recv_buf = (u_int32_t *)malloc(MAX_BLOCK_WORDS*sizeof(u_int32_t))) == NULL)
	{
		fprintf(stderr, "No memory\n");
		exit(1);
	}

	printf("Write & read DRAM locations\n");

	x = 0;
	for (block=0; block<num_blocks; block++)
	{ 
		printf(".... Writing Block %4d of %4d\r", block, num_blocks);
		fflush(stdout);
		for (i=0;i<MAX_BLOCK_WORDS; i++)
		{
			send_buf[i] = x;
			x += 0x07050301;
		}
		rc = xsp4_write(path, addr_space, block*MAX_BLOCK_WORDS, MAX_BLOCK_WORDS, send_buf);
		if (rc != XSP3_OK)
		{
			printf("IO Error on writing DRAM at region %d offset %d: %d\n", addr_space, block*MAX_BLOCK_WORDS, rc);
			io_errors++;
		}
	}

	x = 0;
	printf("\n");
	for (block=0; block<num_blocks; block++)
	{ 
		printf(".... Reading Block %4d of %4d\r", block, num_blocks);
		fflush(stdout);
		for (i=0;i<MAX_BLOCK_WORDS; i++)
		{
			send_buf[i] = x;
			x += 0x07050301;
		}
		rc = xsp4_read(path, addr_space, block*MAX_BLOCK_WORDS, MAX_BLOCK_WORDS, recv_buf);
		if (rc != XSP3_OK)
		{
			printf("IO Error on reading DRAM at region %d offset %d: %d\n", addr_space, block*MAX_BLOCK_WORDS, rc);
			io_errors++;
		}
		for (i=0;i<MAX_BLOCK_WORDS; i++)
		{
			if (send_buf[i] != recv_buf[i])
			{
				errors++;
				if (errors < max_errors)
					printf("ERROR: Region %d, offset %d. Wrote %08X, read %08X\n", addr_space, block*MAX_BLOCK_WORDS+i, send_buf[i], recv_buf[i]);
				else if (errors == max_errors)
					printf("Too Many Errors, counting\n");
			}
		}
	}
	free(send_buf);
	free(recv_buf);
	printf("\n");
	if (errors == 0 && io_errors == 0)
		printf("Passed DRAM data tests\n");
	else
		printf("DRAM tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}
