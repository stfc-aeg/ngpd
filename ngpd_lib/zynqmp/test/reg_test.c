/*
 * reg_test.c
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


int test_chan_regs(int path)
{
	int rc, i, j;
	u_int32_t x, y;
	int errors = 0;		
	int io_errors=0;


	mprintf(1, "TEST CHANNEL REGISTERS\n");

	for (i=0; i<num_chan; i++) 
	{ // all possible channels
		for (j=0; j<32; j++)
		{
			if (regs_present & 1 << j)
			{
				x = i*9+j;
				if (manual_test)
				{
					printf("About to write 0x%08X to chan %d, offset %d, press return to continue\n", x, i, j);
					getchar();
				}
				rc = ngpd_write_chan_regs(path, i, NGZMP_REGION_REGS, j, 1, &x);
				if (rc != 0) {
					mprintf(MP_ERR, "Error on writing Register at channel %d offset %d: %d\n", i, j, rc);
					io_errors++;
				}
			}
		}
	}

	for (i=0; i<num_chan; i++) 
	{ // all possible channels
		for (j=0; j<32; j++)
		{
			if (regs_present & 1 << j)
			{
				x = i*9+j;
				rc = ngpd_read_chan_regs(path, i, NGZMP_REGION_REGS, j, 1, &y);
				if (rc != 0) {
					mprintf(MP_ERR, "Error on reading Register at channel %d offset %d: %d\n", i, j, rc);
					io_errors++;
				}

				if (x != y)
				{
					errors++;
					if (errors < max_errors)
						mprintf(MP_ERR, "ERROR: Chan  %d, Register %d. Wrote %08X, read %08X\n", i, j, x, y);
					else if (errors == max_errors)
						mprintf(MP_ERR, "Too Many Errors, counting\n");
					
				}
			}
		}
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed Channel Register data tests\n");
	else
		mprintf(MP_ERR, "Channel Register data tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
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


	mprintf(1, "TESTING GLOBAL REGISTERS\n");

	for (j=0; j<32; j++)
	{
		if (1)
		{
			x = j*0x07050301;
			rc = ngpd_write_glob_regs(path, card, j, 1, &x);
			if (rc != 0) {
				mprintf(MP_ERR, "Error on writing Global Register at offset %d: %d\n", j, rc);
				io_errors++;
			}
		}
	}

	for (j=0; j<32; j++)
	{
		if (1)
		{
			x = j*0x07050301;
			rc = ngpd_read_glob_regs(path,card, j, 1, &y);
			if (rc != 0) {
				printf("Error on reading Global Register at offset %d: %d\n", j, rc);
				io_errors++;
			}

			if (x != y)
			{
				errors++;
				if (errors < max_errors)
					mprintf(MP_ERR, "ERROR: Global Register %d. Wrote %08X, read %08X\n", j, x, y);
				else if (errors == max_errors)
					mprintf(MP_ERR, "Too Many Errors, counting\n");
				
			}
		}
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed Global Register data tests\n");
	else
		mprintf(MP_ERR, "Global Register data tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}

int test_regs_filter(int path)
{
	int chan, i;
	u_int32_t coeff[NGZMP_FILT_NUM_COEFF], status;
	int errors = 0;		
	int io_errors=0;
	int pass, rc;

	for (i=0; i<NGZMP_FILT_NUM_COEFF; i++)
		coeff[i] = i;

	mprintf(1, "LOAD-FILTER\n");
	for (chan=0; chan<num_chan; chan++)
	{
		for (pass=0; pass<3; pass++)
		{
			if (pass == 1 && 1)
			{
				/* recovering from out of step is difficult without a reset, or possiblly force configuration */
				mprintf(2,".... Loading filter channel %d, pass 1, inserting extra write to filter\n", chan);
				ngpd_write_chan_regs(path, chan, NGZMP_REGION_REGS, NGZMP_CHAN_FILTER, 1, coeff+chan);
			}
			else
			{
				mprintf(2,".... Loading filter channel %d, pass %d\n", chan, pass);
			}
			if (manual_test)
			{
				printf("Press return to load filter on channel %d\n", chan);
				getchar();
			}
			rc = ngpd_filter_load_integer(path, chan, NGZMP_FILT_NUM_COEFF, coeff, &status);
			if (rc < 0)
			{
				io_errors++;
				mprintf(MP_ERR, "Error calling ngpd_filter_load_integer, rc=%d\n", rc);
			}
			else if ((status & (NGZMP_FILTER_STATUS_TLAST_MISSING(chan) | NGZMP_FILTER_STATUS_TLAST_UNEXPECTED(chan) | NGZMP_FILTER_STATUS_MISSING_TREADY(chan) )) != 0 )
			{
				errors++;
				if (errors < max_errors)
					mprintf(MP_ERR, "ERROR: ngpd_filter_load_integer: Status error detected loading chan %d, status= 0x%08X\n", chan, status);
				else if (errors == max_errors)
					mprintf(MP_ERR, "Too Many Errors, counting\n");
			}
		}
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed load filter tests\n");
	else
		mprintf(MP_ERR, "Load filter tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}
