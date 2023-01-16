#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ngpd.h"
#include "ngpdtest.h"

int test_chan_regs(int path)
{
	int rc, i, j;
	u_int32_t x, y;
	int errors = 0;		
	int io_errors=0;


	printf("Test Channel Registers 1 at a time\n");

	for (i=0; i<num_chan; i++) 
	{ // all possible channels
		for (j=0; j<NGPD_CHAN_NUM; j++)
		{
			x = i*9+j;
			rc = ngpd_write_chan_regs(path, i, NGPD_REGION_REGS, j, 1, &x);
			if (rc != 0)
			{
				printf("Error on writing Register at channel %d offset %d: %d\n", i, j, rc);
				io_errors++;
			}
		}
	}

	for (i=0; i<num_chan; i++) 
	{ // all possible channels
		for (j=0; j<NGPD_CHAN_NUM; j++)
		{
			x = i*9+j;
			rc = ngpd_read_chan_regs(path, i, NGPD_REGION_REGS, j, 1, &y);
			if (rc != 0)
			{
				printf("Error on reading Register at channel %d offset %d: %d\n", i, j, rc);
				io_errors++;
			}
			if (x != y)
			{
				errors++;
				if (errors < max_errors)
					printf("ERROR: Chan  %d, Register %d. Wrote %08X, read %08X\n", i, j, x, y);
				else if (errors == max_errors)
					printf("Too Many Errors, counting\n");
				
			}
		}
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed Channel Register data tests\n");
	else
		printf("Channel Register data tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}


int test_chan_bram(int path)
{
	int rc, i, j;
	u_int32_t x;
	u_int32_t em, rdm, ec, rdc, raddr;
	int errors = 0;		
	int io_errors=0;


	printf("Test Channel BRAM 1 at a time\n");

	for (i=0; i<num_chan; i++) 
	{ // all possible channels
		j = 0;
		rc = ngpd_write_chan_regs(path, i, NGPD_REGION_DPRAM, NGPD_CHAN_DPADDR, 1, (u_int32_t *)&j);
		if (rc != 0)
		{
			printf("Error on BRAM address Register at channel %d offset %d: %d\n", i, j, rc);
			io_errors++;
		}
		for (j=0; j<NGPD_MEASURE_TAIL_THRES_SIZE; j++)
		{
			x = j*9+i;
			rc = ngpd_write_chan_regs(path, i, NGPD_REGION_DPRAM, NGPD_CHAN_MEASURE_TAIL_THRES_M, 1, &x);
			if (rc != 0)
			{
				printf("Error on writing BRAM M-Data Register at channel %d offset %d: %d\n", i, j, rc);
				io_errors++;
			}
			x ^= 0xFFFFFFFF;
			rc = ngpd_write_chan_regs(path, i, NGPD_REGION_DPRAM, NGPD_CHAN_MEASURE_TAIL_THRES_C, 1, &x);
			if (rc != 0)
			{
				printf("Error on writing BRAM C-Data Register at channel %d offset %d: %d\n", i, j, rc);
				io_errors++;
			}
		}
	}

	for (i=0; i<num_chan; i++) 
	{ // all possible channels
		j = 0;
		rc = ngpd_write_chan_regs(path, i, NGPD_REGION_DPRAM, NGPD_CHAN_DPADDR, 1, (u_int32_t *)&j);
		if (rc != 0)
		{
			printf("Error on BRAM address Register at channel %d offset %d: %d\n", i, j, rc);
			io_errors++;
		}
		for (j=0; j<NGPD_MEASURE_TAIL_THRES_SIZE; j++)
		{
			em = j*9+i;
			ec = em ^ 0xFFFFFFFF;
			rc = ngpd_read_chan_regs(path, i, NGPD_REGION_DPRAM, NGPD_CHAN_DPADDR, 1, &raddr);
			if (rc != 0)
			{
				printf("Error on BRAM address Register at channel %d offset %d: %d\n", i, j, rc);
				io_errors++;
			}
			rc = ngpd_read_chan_regs(path, i, NGPD_REGION_DPRAM, NGPD_CHAN_MEASURE_TAIL_THRES_M, 1, &rdm);
			if (rc != 0)
			{
				printf("Error on reading BRAM M-Data Register at channel %d offset %d: %d\n", i, j, rc);
				io_errors++;
			}
			rc = ngpd_read_chan_regs(path, i, NGPD_REGION_DPRAM, NGPD_CHAN_MEASURE_TAIL_THRES_C, 1, &rdc);
			if (rc != 0)
			{
				printf("Error on reading BRAM C_Data Register at channel %d offset %d: %d\n", i, j, rc);
				io_errors++;
			}
			if (j != raddr || em !=  rdm || ec != rdc)
			{
				errors++;
				if (errors < max_errors)
					printf("ERROR: Chan  %d, Tail Sum Thres BRAM expected address %d, actual=%d. Wrote %08X, %08X read %08X, %08X\n", i, j, raddr, em, ec, rdm, rdc);
				else if (errors == max_errors)
					printf("Too Many Errors, counting\n");
			}
		}
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed Channel BRAM data tests\n");
	else
		printf("Channel BRAM data tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}
int test_chan_tail_sub_one(int path)
{
	int rc, i, j;
	u_int32_t x;
	u_int32_t em, rdm, ec, rdc, raddr;
	int errors = 0;		
	int io_errors=0;


	printf("Test Channel Tail subtract BRAM single access at a time\n");

	for (i=0; i<num_chan; i++) 
	{ // all possible channels
		j = 0;
		rc = ngpd_write_chan_regs(path, i, NGPD_REGION_DPRAM, NGPD_CHAN_DPADDR, 1, (u_int32_t *)&j);
		if (rc != 0)
		{
			printf("Error on BRAM address Register at channel %d offset %d: %d\n", i, j, rc);
			io_errors++;
		}
		for (j=0; j<NGPD_MEASURE_TAIL_SUB_SIZE; j++)
		{
			x = j*9+i;
			rc = ngpd_write_chan_regs(path, i, NGPD_REGION_DPRAM, NGPD_CHAN_MEASURE_TAIL_SUB_EVEN, 1, &x);
			if (rc != 0)
			{
				printf("Error on writing BRAM M-Data Register at channel %d offset %d: %d\n", i, j, rc);
				io_errors++;
			}
			x ^= 0x3FFFF;
			rc = ngpd_write_chan_regs(path, i, NGPD_REGION_DPRAM, NGPD_CHAN_MEASURE_TAIL_SUB_ODD, 1, &x);
			if (rc != 0)
			{
				printf("Error on writing BRAM C-Data Register at channel %d offset %d: %d\n", i, j, rc);
				io_errors++;
			}
		}
	}

	for (i=0; i<num_chan; i++) 
	{ // all possible channels
		j = 0;
		rc = ngpd_write_chan_regs(path, i, NGPD_REGION_DPRAM, NGPD_CHAN_DPADDR, 1, (u_int32_t *)&j);
		if (rc != 0)
		{
			printf("Error on BRAM address Register at channel %d offset %d: %d\n", i, j, rc);
			io_errors++;
		}
		for (j=0; j<NGPD_MEASURE_TAIL_SUB_SIZE; j++)
		{
			em = j*9+i;
			ec = em ^ 0x3FFFF;
			rc = ngpd_read_chan_regs(path, i, NGPD_REGION_DPRAM, NGPD_CHAN_DPADDR, 1, &raddr);
			if (rc != 0)
			{
				printf("Error on BRAM address Register at channel %d offset %d: %d\n", i, j, rc);
				io_errors++;
			}
			rc = ngpd_read_chan_regs(path, i, NGPD_REGION_DPRAM, NGPD_CHAN_MEASURE_TAIL_SUB_EVEN, 1, &rdm);
			if (rc != 0)
			{
				printf("Error on reading BRAM M-Data Register at channel %d offset %d: %d\n", i, j, rc);
				io_errors++;
			}
			rc = ngpd_read_chan_regs(path, i, NGPD_REGION_DPRAM, NGPD_CHAN_MEASURE_TAIL_SUB_ODD, 1, &rdc);
			if (rc != 0)
			{
				printf("Error on reading BRAM C_Data Register at channel %d offset %d: %d\n", i, j, rc);
				io_errors++;
			}
			if (j != raddr || em !=  rdm || ec != rdc)
			{
				errors++;
				if (errors < max_errors)
					printf("ERROR: Chan  %d, Tail Sum Thres BRAM expected address %d, actual=%d. Wrote %08X, %08X read %08X, %08X\n", i, j, raddr, em, ec, rdm, rdc);
				else if (errors == max_errors)
					printf("Too Many Errors, counting\n");
			}
		}
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed Channel Tail subtract BRAM single access data tests\n");
	else
		printf("Channel Tail subtract BRAM single access data tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}

int test_chan_tail_sub_bram(int path)
{
	int rc, i, j;
	int errors = 0;		
	int io_errors=0;
	int32_t correct[2*NGPD_MEASURE_TAIL_SUB_SIZE];
	int32_t actual[2*NGPD_MEASURE_TAIL_SUB_SIZE];

	printf("Testing Channel Tail subtract BRAM block write\n");

	for (i=0; i<num_chan; i++) 
	{ // all possible channels
		for (j=0; j< NGPD_MEASURE_TAIL_SUB_SIZE*2; j++)
			correct[j] = j*3+i;

		rc = ngpd_write_tail_subtract(path, i, correct);
		if (rc != 0)
		{
			printf("Error on chan %d writing Tail subtract BRAM : %s\n", i, ngpd_get_error_message());
			io_errors++;
		}
	}

	for (i=0; i<num_chan; i++) 
	{ // all possible channels
		for (j=0; j< NGPD_MEASURE_TAIL_SUB_SIZE*2; j++)
			correct[j] = j*3+i;

		rc = ngpd_read_tail_subtract(path, i, actual);
		if (rc != 0)
		{
			printf("Error on chan %d Reading Tail subtract BRAM : %s\n", i, ngpd_get_error_message());
			io_errors++;
		}
		for (j=0; j<NGPD_MEASURE_TAIL_SUB_SIZE*2; j++)
		{
			if (actual[j] != correct[j])
			{
				errors++;
				if (errors < max_errors)
					printf("ERROR: Chan  %d, Tail Subtract BRAM address %d: Wrote %08X, read %08X\n", i, j, correct[j], actual[j]);
				else if (errors == max_errors)
					printf("Too Many Errors, counting\n");
			}
		}
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed Channel BRAM data tests\n");
	else
		printf("Channel Tail subtract BRAM data tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}


int test_chan_regs2(int path)
{
	int rc, i, j, k, n;
	u_int32_t x, y;
	int errors = 0;		
	int io_errors=0;
	u_int32_t wr_data[4];
 
	printf("Test Channel Registers 1..4 at a time\n");

	for (i=0; i<num_chan; i++) 
	{ // all possible channels
		for (j=0; j<NGPD_CHAN_NUM-1;)	// Omit the DP addr register
		{
			for (n=1;j+n<=NGPD_CHAN_NUM-1 && n<=4; n++)	
			{
				for (k=0;k<n;k++)
					wr_data[k] = (i*9+j+k)*0x07050301;
				printf("chan %d: From %d for %d\n", i, j, n);
				rc = ngpd_write_chan_regs(path, i, NGPD_REGION_REGS, j, n, wr_data);
				if (rc != 0) {
					printf("Error on writing Register at channel %d offset %d: %d\n", i, j, rc);
					io_errors++;
				}
				j+= n;
			}
		}
	}

	for (i=0; i<num_chan; i++) 
	{ // all possible channels
		for (j=0; j<NGPD_CHAN_NUM-1; j++)
		{
			x = (i*9+j)*0x07050301;
			rc = ngpd_read_chan_regs(path, i, NGPD_REGION_REGS, j, 1, &y);
			if (rc != 0) {
				printf("Error on reading Register at channel %d offset %d: %d\n", i, j, rc);
				io_errors++;
			}

			if (x != y)
			{
				errors++;
				if (errors < max_errors)
					printf("ERROR: Chan  %d, Register %d. Wrote %08X, read %08X\n", i, j, x, y);
				else if (errors == max_errors)
					printf("Too Many Errors, counting\n");
				
			}
		}
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed Channel Register data tests\n");
	else
		printf("Channel Register data tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}

int test_glob_regs(int path, int card)
{
	int rc, j;
	u_int32_t x, y;
	int errors = 0;		
	int io_errors=0;


	printf("Test Global Registers 1 at a time\n");

	for (j=0; j<NGPD_GLOB_NUM; j++)
	{
		x = j;
		rc = ngpd_write_glob_regs(path, card, j+NGPD_GLOB_BASE, 1, &x);
		if (rc != 0)
		{
			printf("Error on writing Register at offset %d: %d\n", j+NGPD_GLOB_BASE, rc);
			io_errors++;
		}
	}

	for (j=0; j<NGPD_GLOB_NUM; j++)
	{
		x = j;
		rc = ngpd_read_glob_regs(path, card, j+NGPD_GLOB_BASE, 1, &y);
		if (rc != 0)
		{
			printf("Error on reading Register at offset %d: %d\n",  j+NGPD_GLOB_BASE, rc);
			io_errors++;
		}
		if (x != y)
		{
			errors++;
			if (errors < max_errors)
				printf("ERROR: GlolRegs, Register %d. Wrote %08X, read %08X\n", j, x, y);
			else if (errors == max_errors)
				printf("Too Many Errors, counting\n");
			
		}
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed Global Register data tests\n");
	else
		printf("Global Register data tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}

int test_reg_range(int path, int card)
{
	int i;
	u_int32_t x;
	int rc;

	printf("Trying address range to get write to logic analyser\n");
	for (i=6; i<22; i++)
	{
		x = i;
		rc = ngpd_write_glob_regs(path, card, 1<<i, 1, &x);
		printf("i=%d, rc=%d\n", i, rc);
	}
	return rc;
}

	