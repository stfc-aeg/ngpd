/*
 * femApiTest.c
 *
 *  Created on: 2 Dec 2011
 *      Author: tcn
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "ngpd.h"
#include "ngzmp_driver.h"
#include "ngzmptest.h"
#include "errors.h"
#include "args.h"

int test_regs;
int test_hist_rw;
int test_hist_hist;
int test_hist_opt;
int test_bram;
int test_speed;
int manual_test;
int dont_print_errors;
int loop_reset=1;
int nr_loops=1;
int test_reset;
int test_10g_tx;
int override_packet_gap=-1;
int test_pat_seed=0;
int test_gen_scope; 
int test_spi;
int packet_size = 8000;
int test_10g_tx_debug_opt=-1;
int test_read_hist;
int test_clear_hist;
int test_hist_speed;
int test_scaler_read;
int first_card;
int test_dma;
int test_dma_buff;
int test_axi_c2c;
int test_v7_program= -1;
int test_ignore_psu_ok;
int test_sys_mon;
int limit_bram=0;
int test_readout_mode;
int test_poll_send;
char test_hist_write[NAME_MAX+2];
char test_report_fname[NAME_MAX+2];
FILE *report_file;

int test_hist_repeat=1;
int test_i2c;
int test_prbs;
int test_sys_mons;
double hist_clk_period=5E-9;		// NGZMP uses 250 MHz in run mode, 200 MHz with internal clock for playback.
char *progname;
int limit_tp_lines;
int hist_rand_size;
int test_hist_first_pass;
int test_hist_auto;

u_int32_t revision;
u_int32_t regs_present=~(1<<NGPD_CHAN_FILTER);
int num_chan=NGZMP_CHANS_PER_CARD;
int num_cards=1;
unsigned int num_errors, num_io_errors;
unsigned int max_errors=200;
int num_scope_dma;

int main(int argc, char* argv[]) 
{
	int total_errors = 0, total_io_errors=0;
	int path;
	int loop, full_loops;
//	char femHostName[][XSP3_MAX_IP_CHARS] = {"192.168.0.2"};
	int ncards = 1;
	int card = 0;
//	int nchan = ncards*8;
 //   int femPort = 30123;
    int debug = 1;
	int num_tf=16384;
	int rc;
	int do_init=1;

	test_hist_default_xspress4();

	progname = argv[0];
	do_args(argc, argv);
	if (test_poll_send)
		do_init = 0;
	if (reporting > 1) debug=2;
	path = ngzmp_config_details(ncards, num_tf, NULL, -1, NULL, -1, 1, NULL, debug, first_card, do_init);
	if (path < 0)
	{
		fprintf(stderr, "Cannot open connection to NGPD. Errno=%d\n", errno);
		exit(errno);
	}
	if (test_report_fname[0] != 0)
	{
		if ((report_file = fopen(test_report_fname, "w")) == NULL)
		{
			fprintf(stderr, "Cannot open report file '%s', errno=%d\n", test_report_fname, errno);
			return errno;
		}
	}
	for (full_loops=0;full_loops<loop_reset; full_loops++)
	{
/*		if (test_gen_scope)
			generate_scope_data();
*/

		for (loop=0; loop<nr_loops || nr_loops ==0; loop++)
		{
			num_errors = 0;
			num_io_errors = 0;
			if (test_regs & TEST_REGS_CHAN_REGS)
				test_chan_regs(path);
			if (test_regs & TEST_REGS_GLOB_REGS)
				test_glob_regs(path, 0);
			if (test_regs & TEST_REGS_FILTER)
				test_regs_filter(path);
			if (test_bram & TEST_BRAM_WIDTH)
				test_bram_width(path);
			if (test_bram & TEST_BRAM_MEMORY)
				test_bram_wrrd(path);

			if (test_hist_rw & TEST_HIST_REGS_READ)
				test_hist_read_config(path, card);
			if (test_hist_rw & TEST_HIST_REGS_RW)
				test_hist_reg_wr_rd(path, card);
			if (test_hist_rw & TEST_HIST_MEM_RW)
				test_hist_memory(path, card);
			if (test_hist_rw & TEST_HIST_MEM_DMA)
				test_hist_memory_dma(path, card);
	
			if (test_hist_rw & TEST_HIST_MEM_TPG_DMA)
				test_hist_memory_tpgen_dma(path, card);

			if (test_hist_rw & TEST_HIST_TP_MEM_RW)
				test_hist_tp_memory(path, card);
			if (test_hist_rw & TEST_HIST_RBUF_MEM_RW)
				test_hist_rbuf_memory(path, card);
			if (test_hist_rw & TEST_HIST_PB_MEM_RW)
				test_playback_memory(path, card);
			if (test_hist_rw & TEST_HIST_MEM_SMALL_DMA)
				test_dma_buff_rw_small(path, card, NGZMP_DMA_STREAM_BNUM_HIST_TEST, 1, "Hist Test");
			if (test_hist_rw & TEST_HIST_MEM_SMALL_REGION)
				test_buff_rw_small(path, card, ZYNQMP_ADDR_SPACE_PS_LOW, ZYNQMP_PS_DRAM_PHYS_SIZE_LOW/sizeof(u_int32_t), "PS-low");
			if (test_hist_rw & TEST_HIST_MEM_STREAM)
				test_hist_read_stream(path, card);
			if (test_hist_rw & TEST_HIST_MEM_ROW_LEN)
				test_hist_read_row_len(path, card);

			if (test_hist_hist & TEST_HIST_CLEAR)
				test_hist_clear(path, card);

			if (test_hist_hist & TEST_HIST_HIST)
				hist_hist_tests(path, card);
			if (test_speed & TEST_SPEED_HIST_MEM_RW)
				test_hist_memory_speed(path, card);

			if (test_dma)
				dma_tests(path);
			if (test_prbs)
				prbs_dma_tests(path);
			if (test_spi)
				spi_regs_test(path);
			if (test_i2c)
				i2c_tests(path);
			if (test_sys_mons)
				sys_mon_tests(path);
			
			msg_discard();
			total_errors += num_errors;
			total_io_errors += num_io_errors;
			mprintf(0, "Finished NGPD test loop %d with %d errors, IO errors=%d, total so far=%d, %d\n", loop+1, num_errors, num_io_errors, total_errors, total_io_errors);
			if (exit_on_error && (num_errors > 0 || num_io_errors > 0))
				return -1;

		}
		if (loop_reset > 1)
		{
			mprintf(0, "Finished FULL loop pass %d: Total Errors=%d, Total IO Errors=%d\n", full_loops, total_errors, total_io_errors);
		}
	}
	printf("Finished tests with %d errors and %d io errors\n", total_errors, total_io_errors);
	if (report_file != NULL)
		fclose(report_file);
	return 0;
}

