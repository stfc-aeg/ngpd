/*
 * femApiTest.c
 *
 *  Created on: 2 Dec 2011
 *      Author: tcn
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ADQAPI.h"

#include "ngpd.h"
#include "ngpdtest.h"
#include "errors.h"
#include "args.h"

int test_regs;
int test_bram;
int test_dma;
int manual_test;
int dont_print_errors;
int loop_reset=1;
int nr_loops=1;
int test_reset;
int test_2stream;
int test_spi;
int test_i2c;
int limit_bram=0;
char *progname;

u_int32_t revision;
int num_chan;
int num_errors, num_io_errors;
int error_tests;
int max_errors=200;
int nof_adq;
int nof_failed_adq;

int num_cards=1, first_card;
int scope_out_skip;
int api_rev = 0;
int *fw_rev;
unsigned int adq_num=1;
int read_glob_regs(int path, int card);
u_int32_t regs_present=~(1<<NGPD_CHAN_FILTER);

int main(int argc, char* argv[]) 
{
	int total_errors = 0, total_io_errors=0;
	int path=0;
	int loop, full_loops;
    int debug = 1;
	int card=0;
	progname = argv[0];
	do_args(argc, argv);
	if (reporting > 1) debug=2;

	path= ngpd_config_ngzmp(num_cards,  NULL, -1, NULL, -1, debug, first_card, 1,  0);
	if (path < 0)
	{
		printf("ngpd_config failed: %s\n", ngpd_get_error_message());
		exit(1);
	}
	for (full_loops=0;full_loops<loop_reset; full_loops++)
	{
		num_chan = ngpd_get_num_chan(path);
		for (card=0; card<num_cards; card++)
			readFixedRegs(path, card);
		// read_glob_regs(path);
		// test_reg_range(path);
		for (loop=0; loop<nr_loops || nr_loops ==0; loop++)
		{
			num_errors = 0;
			num_io_errors = 0;

			if (test_regs & TEST_REGS_CHAN_REGS)
			{
				test_chan_regs(path);
			}
			if (test_regs & TEST_REGS_FILTER)
				test_regs_filter(path);
			if (test_regs & TEST_REGS_GLOB_REGS)
			{
				for (card=0; card<num_cards; card++)
					test_glob_regs(path, card);
			}
			if (test_spi)
				spi_regs_test(path);
			if (test_i2c)
				i2c_tests(path);
			if (test_bram & TEST_BRAM_MEMORY)
				test_bram_wrrd(path);
			if (test_bram & TEST_BRAM_WIDTH )
				test_bram_width(path);

			msg_discard();
			mprintf(0, "Finished ADQ14 Neutron Gamma Discriminator test loop %d with %d errors, IO errors=%d, failing tests=%d\n", loop+1, num_errors, num_io_errors, error_tests);
			if (exit_on_error && (num_errors > 0 || num_io_errors > 0))
				return -1;

		}
		total_errors += num_errors;
		total_io_errors += num_io_errors;
		if (loop_reset > 1)
		{
			mprintf(0, "Finished FULL loop pass %d: Total Errors=%d, Total IO Errors=%d\n", full_loops, total_errors, total_io_errors);
		}
	}
	printf("Finished tests with %d errors and %d io errors, erroring tests=%d\n", total_errors, total_io_errors, error_tests);
	return 0;
}

int readFixedRegs(int path, int card)
{
	int rc;
	int passed = 1;
	u_int32_t rev;

	printf("Read Back fixed revision \n");


	rc = ngpd_read_glob_regs(path, card, NGZMP_REVISION, 1, &rev);
	if (rc != 0)
	{
		printf("Error on calling ngpd_read_glob_regs(): %d\n", rc);
		passed = 0;
	}
	printf("Revision=0x%08X\n", rev);
	return !passed;
}

int read_glob_regs(int path, int card)
{
	u_int32_t x;
	int i, rc;

	for (i=0; i< 64; i++)
	{
		rc = ngpd_read_glob_regs(path, card, i, 1, &x);
		printf("%d\t%d\t%08X\n", i, rc, x);
	}
	return 0;
}
