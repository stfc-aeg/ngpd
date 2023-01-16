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

int limit_bram=0;
char *progname;

u_int32_t revision;
int num_chan;
int num_errors, num_io_errors;
int error_tests;
int max_errors=200;
int nof_adq;
int nof_failed_adq;

int num_cards, first_card;
int scope_out_skip;
int api_rev = 0;
int *fw_rev;
unsigned int adq_num=1;
int read_glob_regs(int path, int card);

int main(int argc, char* argv[]) 
{
	int total_errors = 0, total_io_errors=0;
	int path=0;
	int loop, full_loops;
    int debug = 1;
	int rc;
	int card = 0;

	progname = argv[0];
	do_args(argc, argv);
	if (reporting > 1) debug=2;

	path = ngpd_config_adq14(adq_num, debug);
	if (path < 0)
	{
		printf("ngpd_config failed: %s\n", ngpd_get_error_message());
		exit(1);
	}
	for (full_loops=0;full_loops<loop_reset; full_loops++)
	{
		num_chan = ngpd_get_num_chan(path);
		readFixedRegs(path, card);
		// read_glob_regs(path, card);
		// test_reg_range(path, card);
		for (loop=0; loop<nr_loops || nr_loops ==0; loop++)
		{
			num_errors = 0;
			num_io_errors = 0;

			if (test_regs & TEST_REGS_CHAN_REGS)
			{
				test_chan_regs(path);
				test_chan_regs2(path);
			}
			if (test_regs & TEST_REGS_GLOB_REGS)
				test_glob_regs(path, card);



/*			if (test_bram & TEST_BRAM_WIDTH)
				testBRAMWidth(path); */

			if (test_bram & TEST_BRAM_MEMORY)
			{
				test_chan_bram(path);
				test_chan_tail_sub_one(path);
				test_chan_tail_sub_bram(path);
			}
			if (test_dma)
				dma_tests(path);
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
	u_int32_t regs;

	printf("Read Back fixed revision \n");


	rc = ngpd_read_glob_regs(path, card, NGPD_GLOB_REVISION, 1, &rev);
	if (rc != 0)
	{
		printf("Error on calling ngpd_read_glob_regs(): %d\n", rc);
		passed = 0;
	}
	rc = ngpd_read_glob_regs(path, card, NGPD_GLOB_SCOPE_STATUS, 1, &regs);
	if (rc != 0)
	{
		printf("Error on calling ngpd_read_glob_regs(): %d\n", rc);
		passed = 0;
	}
	printf("Revision=0x%08X, Scope Status=0x%08X\n", rev, regs);
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
