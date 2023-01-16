/*
 * hist_hist.c
 *
 *  Created on: Jan 2021
 *      Author: wih
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "args.h"
#include "errors.h"


#include "ngpd.h"
#include "ngzmp.h"
#include "ngzmptest.h"
#include "zynqmp_lib_driver.h"
#include "ngzmp_lib_driver.h"
#include "ngzmp_driver.h"


#define HIST_BLOCK_SIZE (2*1024*1024)
#define EVENT_BLOCK_SIZE (128*1024)
#define TEST_CLEAR (-1)
int run_test_hist_hist(int path, int card, int num_pass, char *name, int (*generate_tp)(int , int,  u_int32_t *, int, int, int, int), int use_read_dma, int gen_options);
int hist_test_pattern_to_expected(u_int32_t *event_list, u_int32_t **expected, int num_lines, int num_streams, int64_t stream_totals[AXI_HIST_MAX_STREAMS]);
int check_histogram_data(int path, int card, u_int32_t *rbuf, u_int32_t **expected, int num_streams, int stream_stride, int use_dma, int64_t stream_totals[AXI_HIST_MAX_STREAMS]);
int hist_generate_tp_ramp(int num_lines, int tp_line, u_int32_t *event_list, int pass, int first, int last, int gen_options);
int hist_generate_tp_random(int num_lines, int tp_line, u_int32_t *event_list, int pass,  int first, int last, int gen_options);
int hist_generate_tp_xspress4(int num_lines, int tp_line, u_int32_t *event_list, int pass,  int first, int last, int gen_options);
int write_histogram_data(int path, int card, char *fname, u_int32_t *rbuf, int num_streams, int stream_stride, int use_dma);
int hist_display_progress(int path, int card);
int hist_exercise_clear_and_read(int path, int card, AXIHistConfig *hist_conf, u_int32_t *rbuf, int hist_test_words, int repeat);
int hist_insert_delays(int path, int card, int type, u_int32_t *event_list, int  test_pattern_lines);
void * hist_insert_delay_thread(void *vp);
int hist_thin(int path, int card, u_int32_t *event_list, u_int32_t *expected[AXI_HIST_MAX_STREAMS], int test_pattern_lines, int num_streams, int64_t *stream_totals);
int hist_generate_tp_ramp_row_hammer(int num_lines, int tp_line, u_int32_t *event_list, int pass, int first, int last, int gen_options);
int hist_generate_tp_ramp_row_hammer2(int num_lines, int tp_line, u_int32_t *event_list, int pass, int first, int last, int gen_options);
int cal_expected(int stream, int *exp_base, int hist_size, int size_code, int occ_code, int events_per_stream);

int run_test_hist_tpg_rand(int path, int card, int num_pass, int use_read_dma);
int run_test_hist_tpg_roll(int path, int card, int num_pass, int use_read_dma);
int check_histogram_tpg_data(int path, int card, u_int32_t *rbuf, int mode, int words_per_loop, int num_loops, int num_streams, int stream_stride, int use_dma);
int run_test_hist_tpg_ramp(int path, int card, int num_pass, char *name, int (*generate_tp)(int , int, u_int32_t *, int, int , int, int ), int use_read_dma);
int check_histogram_roll_tpg_data(int path, int card, u_int32_t *rbuf, int num_streams, int hist_size, int size_code, int occ_code, int events_per_stream, int use_dma);
int optimise_for_substreams(u_int32_t *event_list, int num_lines, AXIHistConfig *hist_conf);

AXIHistTPSpectra xspress4_tp;

typedef struct
{
	int path, card;
	int type;
	u_int32_t *buffer;
	int num_lines;
	int start_index;
	u_int32_t mem_start, mem_num;
} HistDelayGen;

int test_hist_default_xspress4()
{
	int i;
	xspress4_tp.num_bins = 4096;
	xspress4_tp.num_peaks = 1;
	xspress4_tp.peak[0].posn = 589;
	xspress4_tp.peak[0].height = 1.0;
	for (i=0; i<AXI_HIST_TP_MAX_PEAKS; i++)
		xspress4_tp.peak[i].fwhm = 20;

	xspress4_tp.pileup2 = 0.02;
	xspress4_tp.background = 0.001;
	xspress4_tp.average_events_per_tf = 1000;
	return 0;
}

int hist_hist_tests(int path, int card)
{
	int i, j, k;
	mprintf(1, "HIST_HIST: Running Histogrammer histogram tests\n");
	if (test_hist_hist & TEST_HIST_HIST_RAMP)
		run_test_hist_hist(path, card, 1, "Ramp", hist_generate_tp_ramp, !(test_hist_opt & TEST_HIST_OPT_CPU_READ), 0);
	if (test_hist_auto)
	{
		run_test_hist_hist(path, card, 1, "Random-Full ", hist_generate_tp_random, !(test_hist_opt & TEST_HIST_OPT_CPU_READ), 0);
		if (hist_rand_size)
			run_test_hist_hist(path, card, 1, "Random-Small ", hist_generate_tp_random, !(test_hist_opt & TEST_HIST_OPT_CPU_READ), hist_rand_size);
		else
			run_test_hist_hist(path, card, 1, "Random-500k", hist_generate_tp_random, !(test_hist_opt & TEST_HIST_OPT_CPU_READ), 500000);
	}
	else
	{
		if (test_hist_hist & TEST_HIST_HIST_RANDOM)
			run_test_hist_hist(path, card, 1, "Random", hist_generate_tp_random, !(test_hist_opt & TEST_HIST_OPT_CPU_READ), hist_rand_size);
	}
	if (test_hist_hist & TEST_HIST_HIST_RAMP_RHP)
		run_test_hist_hist(path, card, 1, "Ramp with row hammer protection", hist_generate_tp_ramp_row_hammer, !(test_hist_opt & TEST_HIST_OPT_CPU_READ), 0);
	if (test_hist_hist & TEST_HIST_HIST_RAMP_RH2)
		run_test_hist_hist(path, card, 60, "Ramp row hammer testing", hist_generate_tp_ramp_row_hammer2, !(test_hist_opt & TEST_HIST_OPT_CPU_READ), 0);
	
	if (test_hist_hist & TEST_HIST_HIST_XSPRESS4)
	{
		if (!(test_hist_hist & (TEST_HIST_HIST_XSPRESS1US | TEST_HIST_HIST_XSPRESS10US | TEST_HIST_HIST_XSPRESS1MS | TEST_HIST_HIST_XSPRESS100MS)))
			test_hist_hist |= TEST_HIST_HIST_XSPRESS1MS;	// Default to 1ms time frams if not specified

		if (!(test_hist_hist & (TEST_HIST_HIST_XSPRESSMN	| TEST_HIST_HIST_XSPRESSZR | TEST_HIST_HIST_XSPRESSMIX)))
			test_hist_hist |= TEST_HIST_HIST_XSPRESSMN; // Default to Mn if none specified

		for (i=0; i<4; i++)
		{
			if ( i == 0 && (test_hist_hist & TEST_HIST_HIST_XSPRESS1US))
				xspress4_tp.average_events_per_tf = 1;	// Consider 1 MHz x 10 us frame = 10 events
			else if ( i == 1 && (test_hist_hist & TEST_HIST_HIST_XSPRESS10US))
				xspress4_tp.average_events_per_tf = 10;	// Consider 1 MHz x 10 us frame = 10 events
			else if (i == 2 && (test_hist_hist & TEST_HIST_HIST_XSPRESS1MS))
				xspress4_tp.average_events_per_tf = 1000;
			else if (i == 3 && (test_hist_hist & TEST_HIST_HIST_XSPRESS100MS))
				xspress4_tp.average_events_per_tf = 100000;
			else
				continue;
			for (j=0; j<3; j++)
			{
				if (j==0 && (test_hist_hist & TEST_HIST_HIST_XSPRESSMN))
				{
					xspress4_tp.num_peaks= 1;
					xspress4_tp.peak[0].posn = 589;
					xspress4_tp.peak[0].height = 1.0;
					xspress4_tp.peak[0].fwhm = 20;
					xspress4_tp.pileup2 = 0.02;
				}
				else if (j==1 && (test_hist_hist & TEST_HIST_HIST_XSPRESSZR))
				{
					xspress4_tp.num_peaks= 1;
					xspress4_tp.peak[0].posn = 1570;
					xspress4_tp.peak[0].height = 1.0;
					xspress4_tp.peak[0].fwhm = 35;
					xspress4_tp.pileup2 = 0.02;
				}
				else if (j==2 && (test_hist_hist & TEST_HIST_HIST_XSPRESSMIX))
				{
					xspress4_tp.num_peaks= 4;
					xspress4_tp.peak[0].posn = 640; // Fe
					xspress4_tp.peak[0].height = 1.;
					xspress4_tp.peak[0].fwhm = 20;
					xspress4_tp.peak[1].posn = 805;	// Cu
					xspress4_tp.peak[1].height = 0.5;
					xspress4_tp.peak[0].fwhm = 25;
					xspress4_tp.peak[2].posn = 862;	// Zn
					xspress4_tp.peak[2].height = 0.5;
					xspress4_tp.peak[2].fwhm = 25;
					xspress4_tp.peak[3].posn = 1750;	// Mo
					xspress4_tp.peak[3].height = 2;
					xspress4_tp.peak[3].fwhm = 35;
					xspress4_tp.pileup2 = 0.005;
				}
				else
					continue;
				mprintf(1, "Simulated spectra with %d event per frame and peaks:\n", xspress4_tp.average_events_per_tf);
				for (k=0; k<xspress4_tp.num_peaks; k++)
					mprintf(1, ".... Posn = %4d, rel hgt=%f, FWHM=%d\n", xspress4_tp.peak[k].posn, xspress4_tp.peak[k].height, xspress4_tp.peak[k].fwhm);
				run_test_hist_hist(path, card, 1, "Xspress4", hist_generate_tp_xspress4, !(test_hist_opt & TEST_HIST_OPT_CPU_READ), 0);
			}
		}
	}
	if (test_hist_hist & TEST_HIST_HIST_RAMP_TPG)
		run_test_hist_tpg_ramp(path, card, 1, "Ramp", hist_generate_tp_ramp, !(test_hist_opt & TEST_HIST_OPT_CPU_READ));
	if (test_hist_hist & TEST_HIST_HIST_RAND_TPG)
		run_test_hist_tpg_rand(path, card, 8, !(test_hist_opt & TEST_HIST_OPT_CPU_READ));
	if (test_hist_hist & TEST_HIST_HIST_ROLL_TPG)
		run_test_hist_tpg_roll(path, card, 8, !(test_hist_opt & TEST_HIST_OPT_CPU_READ));
		
	return 0;
}
int run_test_hist_hist(int path, int card, int num_pass, char *name, int (*generate_tp)(int , int, u_int32_t *, int, int , int, int ), int use_read_dma, int gen_options)
{
	AXIHistConfig hist_conf;
	int rc;
	u_int32_t *expected[AXI_HIST_MAX_STREAMS];
	int64_t stream_totals[AXI_HIST_MAX_STREAMS];
	u_int32_t *rbuf;
	int i;
	ZYNQMP_DMA_StatusBlock dma_status;
	int io_errors = 0;
	int pass;
	int test_pattern_lines, lines_remaining;
	u_int32_t * event_list;
	int tp_line;
	int32_t dma_status_reg=0;
	ZYNQMP_DMA_MsgBuildDesc msg_build_desc;
	u_int32_t axi_hist_gpio_control=0;
	ZYNQMP_DMA_MsgStart msg_start;
	struct timeval t_start_dma, t_now;
	double t_hist, t_hist2;
	struct timespec delay, remaining;
	int errors=0;
	int stream_stride;
	u_int32_t axi_hist_gpio1_status, axi_hist_gpio2_status;
	u_int64_t hist_busy_time;
	u_int32_t total_events;
	int repeat;
	int timeout=0;
	int num_clears = 0;
	int thin = 0;
	int delays = 0;
	u_int32_t tpg_cont[3];
	int num_words;
	mprintf(1, "Running Histogram test %s\n", name);

	rc = ngzmp_hist_read_config(path, card, &hist_conf);
	if (rc != 0)
	{
		io_errors++;
		printf("Error reading AXI Histogrammer config\n");
	}
	if (hist_conf.NumStreams > AXI_HIST_MAX_STREAMS)
	{
		fprintf(stderr, "Number of streams =%d > AXI_HIST_MAX_STREAMS=%d, please recompile with larger AXI_HIST_MAX_STREAMS\n", hist_conf.NumStreams, AXI_HIST_MAX_STREAMS);
		exit(1);
	}

	if (hist_conf.StreamAtBottom)
		stream_stride = hist_conf.HistWordsPerAXIWord;
	else
		stream_stride = hist_conf.HistWordsPerStream;
	for (i=0; i<hist_conf.NumStreams; i++)
	{
		if ((expected[i]= malloc(sizeof(int)*HIST_BLOCK_SIZE)) == NULL)
		{
			fprintf(stderr, "test_hist_hist: Out of memory allocating blocks for histogram\n");
			exit(1);
		}
	}
	if ((rbuf=malloc(sizeof(u_int32_t)*HIST_BLOCK_SIZE)) == NULL)
	{
		fprintf(stderr, "test_hist_hist: Out of memory allocating read block for histogram\n");
		exit(1);
	}

	if ((event_list  = malloc(EVENT_BLOCK_SIZE*AXI_HIST_TEST_NUM_STREAMS*sizeof(u_int32_t))) == NULL)
	{
		fprintf(stderr, "test_hist_hist: Out of memory allocating blocks for event list\n");
		exit(1);
	}

	if ((rc=zynqmp_read_dma_status_block(path, card, 0, sizeof(ZYNQMP_DMA_StatusBlock)/sizeof(u_int32_t), (u_int32_t *)&dma_status)) < 0)
	{
		io_errors++;
		mprintf(MP_ERR," ERROR Reading DMA status block\n");
	}
	mprintf(3, "Memory for test pattern = %lu\n", dma_status.stream_def[NGZMP_DMA_STREAM_BNUM_HIST_TEST].data_size);

	for (pass=test_hist_first_pass; pass<num_pass; pass++)
	{
		mprintf(2, ".... Histogram test %s, pass=%d\n", name, pass);
		for (i=0; i<hist_conf.NumStreams; i++)
		{
			memset(expected[i], 0, sizeof(int)*HIST_BLOCK_SIZE);
			stream_totals[i] = 0;
		}
		test_pattern_lines = dma_status.stream_def[NGZMP_DMA_STREAM_BNUM_HIST_TEST].data_size/(AXI_HIST_TEST_NUM_STREAMS*sizeof(u_int32_t));
		if (limit_tp_lines > 0 && test_pattern_lines > limit_tp_lines)
			test_pattern_lines = limit_tp_lines;
		total_events = 0;
		mprintf(3, ".... Generating test pattern\n");
		for (tp_line=0, lines_remaining=test_pattern_lines; lines_remaining>0; )
		{
			int num_lines;
			if (lines_remaining > EVENT_BLOCK_SIZE)
				num_lines = EVENT_BLOCK_SIZE;
			else
				num_lines = lines_remaining;
			mprintf(4, "........ Generating %d lines from %d\n", num_lines, tp_line);
			total_events += generate_tp(num_lines, tp_line, event_list, pass, tp_line==0, num_lines==lines_remaining, gen_options);
			if (test_hist_opt & TEST_HIST_OPT_SUBSTREAMS)
				optimise_for_substreams(event_list, num_lines, &hist_conf);
			if ((rc=zynqmp_write_dma_buff(path, card, NGZMP_DMA_STREAM_BNUM_HIST_TEST, tp_line*AXI_HIST_TEST_NUM_STREAMS, num_lines*AXI_HIST_TEST_NUM_STREAMS, event_list)) < 0)
			{
				io_errors++;
				mprintf(MP_ERR,"ERROR writing test pattern to memory\n");
			}

			hist_test_pattern_to_expected(event_list, expected, num_lines, hist_conf.NumStreams, stream_totals);
			tp_line += num_lines;
			lines_remaining -= num_lines;
		}
		total_events *= hist_conf.NumStreams/8;
		for (repeat=0; repeat<test_hist_repeat; repeat++)
		{
			if (test_hist_opt & TEST_HIST_OPT_THIN)
			{
				if (test_hist_repeat == 1)
				{
					hist_thin(path, card, event_list, expected, test_pattern_lines,  hist_conf.NumStreams, stream_totals);
					thin = 1;
				}
				else if (repeat == test_hist_repeat/2)
				{
					hist_thin(path, card, event_list, expected, test_pattern_lines,  hist_conf.NumStreams, stream_totals);
					thin = 1;
				}
			}
			if (test_hist_opt & TEST_HIST_OPT_DELAYS)
			{
				if (test_hist_repeat == 1)
				{
					hist_insert_delays(path, card, 1, event_list, test_pattern_lines);
					delays = 1;
				}
				else
				{
					if (repeat == test_hist_repeat/4 || repeat == 3*(test_hist_repeat/4))
					{
						hist_insert_delays(path, card, 1, event_list, test_pattern_lines);
						delays = 1;
					}
					else if (repeat == test_hist_repeat/2 )
					{
						hist_insert_delays(path, card, 0, event_list, test_pattern_lines);
						delays = 0;
					}
				}
			}	
			mprintf(3, ".... Clearing memory repeat=%d. delays=%d, thin=%d\n", repeat, delays, thin);
			num_clears = 0;
			num_words = hist_conf.StreamAtBottom?hist_conf.AXIWordsPerStream*hist_conf.NumStreams:hist_conf.AXIWordsPerStream; 
			if (ngzmp_hist_clear_start(path, card, 0, num_words, 1) != 0)
			{
				printf("Error staring clear\n");
				io_errors++;
			}
			if (ngzmp_hist_clear_wait(path, card) != 0)
			{
				printf("Error waiting for clear to finish\n");
				io_errors++;
				num_io_errors += io_errors;
				return 0;
			}
			mprintf(3, ".... Reseting DMA\n");
			if ((rc=zynqmp_dma(path, card, ZYNQMP_DMA_CMD_RESET, 1<<NGZMP_DMA_STREAM_BNUM_HIST_TEST, NULL, NULL, NULL)) < 0)
			{
				mprintf(MP_ERR, "ERROR reseting hist test DMA\n");
				io_errors++;
			}
			memset(&msg_build_desc, 0, sizeof(ZYNQMP_DMA_MsgBuildDesc)) ;
			msg_build_desc.src_stream = NGZMP_DMA_STREAM_BNUM_HIST_TEST;
			msg_build_desc.addr = 0;
			msg_build_desc.size_bytes = test_pattern_lines*sizeof(u_int32_t)*AXI_HIST_TEST_NUM_STREAMS ;

			if ((rc=zynqmp_dma(path, card, ZYNQMP_DMA_CMD_BUILD_DESC, NGZMP_DMA_STREAM_BNUM_HIST_TEST, &msg_build_desc, NULL, NULL)) < 0)
			{
				mprintf(MP_ERR, "ERROR Building descriptors for hist test DMA\n");
				io_errors++;
			}
			
			axi_hist_gpio_control = 0;
			if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x400, 1, &axi_hist_gpio_control)) < 0)
			{
				mprintf(MP_ERR, "ERROR writing to GPIO for hist test DMA\n");
				io_errors++;
			}

			tpg_cont[0] = 0;
			tpg_cont[1] = 0;
			tpg_cont[2] = 0;

			if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, AXI_HIST_TESTPAT_CONT, 3, tpg_cont)) < 0)
			{
				mprintf(MP_ERR, "ERROR writing to Test pattern Control registers\n");
				io_errors++;
			}

			axi_hist_gpio_control = NGZMP_HIST_GPIO_ENABLE | NGZMP_HIST_GPIO_SEL_TEST;
			if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x400, 1, &axi_hist_gpio_control)) < 0)
			{
				mprintf(MP_ERR, "ERROR writing to GPIO for hist test DMA\n");
				io_errors++;
			}
			mprintf(3, ".... Starting DMA to send test patterns to histogrammer.\n");
			if (manual_test) { printf("Press RETURN to start DMA repeat=%d, delays=%d, thin=%d\n", repeat, delays, thin); fflush(stdout); getchar(); }
			memset(&msg_start, 0, sizeof(ZYNQMP_DMA_MsgStart));
			if ((rc=zynqmp_dma(path, card, ZYNQMP_DMA_CMD_START, 1<<NGZMP_DMA_STREAM_BNUM_HIST_TEST, &msg_start, NULL, NULL)) < 0)
			{
				mprintf(MP_ERR, "ERROR Starting hist test DMA\n");
				io_errors++;
			}
			gettimeofday(&t_start_dma, NULL);
			delay.tv_sec = 0;
			delay.tv_nsec=1E6;
			do
			{
				if ((rc=ngzmp_dma_read_status(path, card, 1<<NGZMP_DMA_STREAM_BNUM_HIST_TEST, &dma_status_reg)) < 0)
				{
					mprintf(MP_ERR, "ERROR Reading status from hist test DMA\n");
					io_errors++;
				}
				if (reporting >= 5)
					printf(".... DMA status=%08X\n", dma_status_reg);
				gettimeofday(&t_now, NULL);
				t_hist = t_now.tv_sec-t_start_dma.tv_sec;
				t_hist += 1E-6*(t_now.tv_usec-t_start_dma.tv_usec);

				if (dma_status_reg & 2)
					break;
				if (test_hist_opt & (TEST_HIST_OPT_EXERCISE | TEST_HIST_OPT_EX_FIXED))
				{
					rc =hist_exercise_clear_and_read(path, card, &hist_conf, rbuf, HIST_BLOCK_SIZE, repeat);
					if (rc >= 0)
						num_clears += rc;
				}
				else
				{
					remaining = delay;
					while (nanosleep(&remaining, &remaining) == EINTR);
				}

			} while (t_hist < 70);
			if (t_hist > 70)
			{
				mprintf(MP_ERR, "Timout waiting for hist test DMA to become idle\n");
				timeout++;
			}
			mprintf(3, "DMA finished after %g seconds\n", t_hist);
			do
			{
				if ((rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x402, 1, &axi_hist_gpio2_status)) < 0)
				{
					mprintf(MP_ERR, "ERROR Reading status from hist GPIO\n");
					io_errors++;
				}
				if (reporting >= 5)
					printf(".... GPIO Hist status=%08X\n", axi_hist_gpio2_status);
				gettimeofday(&t_now, NULL);
				t_hist = t_now.tv_sec-t_start_dma.tv_sec;
				t_hist += 1E-6*(t_now.tv_usec-t_start_dma.tv_usec);

				if ((axi_hist_gpio2_status & (1<<30)) == 0)
					break;
				remaining = delay;
				while (nanosleep(&remaining, &remaining) == EINTR);

			} while (t_hist < 80);
			if (t_hist > 80)
			{
				mprintf(MP_ERR, "Timout waiting for histogramming to finish\n");
				timeout++;
			}
			if (timeout)
				hist_display_progress(path, card);
			if ((rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x400, 1, &axi_hist_gpio1_status)) < 0 || (rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x402, 1, &axi_hist_gpio2_status)) < 0)
			{
				mprintf(MP_ERR, "ERROR Reading status from hist GPIO\n");
				io_errors++;
			}
			hist_busy_time = (((u_int64_t)(axi_hist_gpio2_status&0x3FFFFFFF)) << 32) |  axi_hist_gpio1_status;
			t_hist2 = hist_clk_period*hist_busy_time;
			mprintf(MP_PRINT, "Histogramming finished: Pass=%d, Repeat=%d, delays=%d, thin=%d. Software timer= %g , firmware timer=%g seconds, num_clears=%d => Histrogram rate=%g\n", 
				pass, repeat, delays, thin, t_hist, t_hist2, num_clears, total_events/t_hist2  );
			if (report_file != NULL)
				fprintf(report_file, "%s : rate=%g", name, total_events/t_hist2);
			if (fabs(t_hist-t_hist2) > 0.1+0.005*t_hist)
			{
				mprintf(MP_ERR, "*ERROR* Software and firmware timing differ significantly. Currently assuming input clock period=%g\n", hist_clk_period);
				mprintf(MP_ERR, "If this is wrong, consider the hist-clk option to change clk period\n", hist_clk_period);
			}
			if (manual_test) { printf("Press RETURN to read data\n"); getchar();}
			errors += check_histogram_data(path, card, rbuf, expected, hist_conf.NumStreams, stream_stride, use_read_dma, stream_totals);
			if (test_hist_write[0] != 0)
				write_histogram_data(path, card, test_hist_write, rbuf, hist_conf.NumStreams, stream_stride, use_read_dma);
			if (test_hist_repeat>1 && errors > 0 && exit_on_error)
			{
				printf("Stopping due to errors\n");
				exit(1);
			}
		}
	}
	for (i=0; i<hist_conf.NumStreams; i++)
		free(expected[i]);
	free(event_list);
	free(rbuf);
	num_io_errors += io_errors;
	if (report_file != NULL)
		fprintf(report_file, "Finished histogram tests %s with %d errors and %d io_errors accessing data via %s\n", name, errors, io_errors, use_read_dma?"DMA read from PL to PS":"CPU read from PL");
	printf("Finished histogram tests %s with %d errors and %d io_errors accessing data via %s\n", name, errors, io_errors, use_read_dma?"DMA read from PL to PS":"CPU read from PL");
	return 0;
}

int hist_test_pattern_to_expected(u_int32_t *event_list, u_int32_t **expected, int num_lines, int num_streams, int64_t stream_totals[AXI_HIST_MAX_STREAMS])
{
	int line, i, stream;
	int num_outside=0;
	for (line=0; line<num_lines; line++)
	{
		for (stream=0; stream<num_streams; stream++)
		{
			int pos;
			i = stream % AXI_HIST_TEST_NUM_STREAMS;	// If there are more hist streams (e.g. 16) plan to repeat the 256 bit test pattern
			if (event_list[i] & AXI_HIST_TEST_VALID)
			{
				pos = AXI_HIST_TEST_GET_OFFSET(event_list[i]);
				if (pos >= 0 && pos < HIST_BLOCK_SIZE)
				{
					(expected[stream][pos])++;
					stream_totals[stream]++;
				}
				else
					num_outside++;
			}
		}
		event_list += AXI_HIST_TEST_NUM_STREAMS;
	}
	return num_outside;
}

int check_histogram_data(int path, int card, u_int32_t *rbuf, u_int32_t **expected, int num_streams, int stream_stride, int use_dma, int64_t stream_totals[AXI_HIST_MAX_STREAMS])
{
	int errors=0, io_errors=0;
	int rc;
	int i;
	int stream;
	u_int64_t actual_totals[AXI_HIST_MAX_STREAMS];
	u_int64_t actual_all=0, expected_all=0;
	int errors_this_stream;
	int read_pass;
	
	for (stream=0; stream<num_streams; stream++)
	{
		read_pass = 0;
		do  
		{
			actual_totals[stream] = 0;
			errors_this_stream = 0;
			if (use_dma)
			{
				if ((rc=ngzmp_hist_read_stream_dma(path, card, stream, 0, HIST_BLOCK_SIZE, rbuf, 0)) < 0)
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
				if ((rc=ngzmp_hist_read_stream(path, card, stream, 0, HIST_BLOCK_SIZE, rbuf)) < 0)
				{
					io_errors++;
					if (io_errors < max_errors)
						mprintf(MP_ERR, "ERROR: zynqmp_read returned error %d\n", rc);
					else if (io_errors == max_errors)
						mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
				}
			}

			for (i=0; i<HIST_BLOCK_SIZE; i++)
			{
				actual_totals[stream] += rbuf[i];
				if (expected[stream][i] != rbuf[i])
				{
					if (errors == 0 && reporting == 0)
					{
						int j;
						printf("ERROR: Stream=%d, Offset %d=%08X: Expected %08X, Read %08X\n", stream, i, i, expected[stream][i], rbuf[i]);
						for (j=i&0xFFFFFFF8; j < (i|7); j++)
						{
							if (expected[stream][j] != rbuf[j])
								printf("Wrong:   Stream=%d, Offset %d=%08X: Expected %08X, Read %08X\n", stream, j, j, expected[stream][j], rbuf[j]);
							else
								printf("Correct: Stream=%d, Offset %d=%08X: Expected %08X, Read %08X\n", stream, j, j, expected[stream][j], rbuf[j]);
						}
					}
					errors++;
					errors_this_stream++;
					if (errors < max_errors)
						mprintf(MP_ERR, "ERROR: Stream=%d, Offset %d=%08X: Expected %08X, Read %08X\n", stream, i, i, expected[stream][i], rbuf[i]);
					else if (errors == max_errors)
						mprintf(MP_ERR, "Too many errors. Counting\n");
				}
				else if (reporting >=5 && i < 100)
					printf("Correct: Stream=%d, Offset %d=%08X: Read %08X\n", stream, i, i, rbuf[i]);
			}
			if (actual_totals[stream] != stream_totals[stream])
				mprintf(MP_ERR, "ERROR:   Stream %d total: Expected %ld, received %ld, extra counts=%ld\n", stream, stream_totals[stream], actual_totals[stream], actual_totals[stream]-stream_totals[stream]);
			else if (reporting >= 3)
				mprintf(MP_PRINT, "Correct: Stream %d total: Expected %ld, received %ld\n", stream, stream_totals[stream], actual_totals[stream]);
			if (errors_this_stream || read_pass >  0)
			{
					printf("Read pass %d.  Stream %d : %d errors\n", read_pass,stream, errors_this_stream);
					read_pass++;
			}
		} while (errors_this_stream && read_pass == 1);
		actual_all += actual_totals[stream];
		expected_all += stream_totals[stream];
	}

	if (errors != 0 || io_errors != 0)
	{
		mprintf(MP_ERREX, "Found %d errors and %d IO error checking histogram data. Expected total=%ld, actual_total=%ld\n", errors, io_errors, expected_all, actual_all);
		num_io_errors += io_errors;
		num_errors += errors;
	}
	return errors;
}


int hist_generate_tp_ramp(int num_lines, int tp_line, u_int32_t *event_list, int pass, int first, int last, int gen_options)
{
	int stream;
	static int row_len[AXI_HIST_TEST_NUM_STREAMS];
	static int row_start[AXI_HIST_TEST_NUM_STREAMS];
	static int max_row_len[AXI_HIST_TEST_NUM_STREAMS];
	static int col[AXI_HIST_TEST_NUM_STREAMS];
	int i;

	if (tp_line == 0)
	{
		for (stream=0; stream<AXI_HIST_TEST_NUM_STREAMS; stream++)
		{
			max_row_len[stream] = 60 + stream;
			row_len[stream] = max_row_len[stream];
			row_start[stream] = 0;
			col[stream] = 0;
		}
	}
	for (stream=0; stream<AXI_HIST_TEST_NUM_STREAMS; stream++)
	{
		for (i=0; i<num_lines; i++)
		{
			event_list[stream+AXI_HIST_TEST_NUM_STREAMS*i] = (row_start[stream]+col[stream]) | AXI_HIST_TEST_VALID | AXI_HIST_TEST_FIRST;
			col[stream]++;
			if (col[stream] >= row_len[stream])
			{
				col[stream] =  0;
				row_len[stream]--;
				if (row_len[stream] == 0)
				{
					row_start[stream] += max_row_len[stream];
					max_row_len[stream] ++;
					row_len[stream] = max_row_len[stream];  
					if (row_start[stream] + max_row_len[stream] > HIST_BLOCK_SIZE)
					{
						printf("Hist test pattern wrapped to start of block at line=%d, stream=%d\n", i, stream);
						row_start[stream] = 0;
					}
				}
			}
		}
	}
	return num_lines*8;
}

/* Artificially modify the pattern to try to reduce the risk of row hammer */
#define NUM_ROW_ADDRESSES 65536
#define GET_ROW(x) (((x)>>10)&0xFFFF)
int hist_generate_tp_ramp_row_hammer(int num_lines, int tp_line, u_int32_t *event_list, int pass, int first, int last, int gen_options)
{
	int stream;
	static int row_len[AXI_HIST_TEST_NUM_STREAMS];
	static int row_start[AXI_HIST_TEST_NUM_STREAMS];
	static int max_row_len[AXI_HIST_TEST_NUM_STREAMS];
	static int col[AXI_HIST_TEST_NUM_STREAMS];
	int i;
	static int *activations[AXI_HIST_TEST_NUM_STREAMS];
	int dram_row;

	if (tp_line == 0)
	{
		for (stream=0; stream<AXI_HIST_TEST_NUM_STREAMS; stream++)
		{
			max_row_len[stream] = 60 + stream;
			row_len[stream] = max_row_len[stream];
			row_start[stream] = 0;
			col[stream] = 0;
			if ((activations[stream]=calloc(NUM_ROW_ADDRESSES, sizeof(int))) == NULL)
			{
				printf("Out of memory\n");
				exit(1);
			}
		}
	}
	for (stream=0; stream<AXI_HIST_TEST_NUM_STREAMS; stream++)
	{
		for (i=0; i<num_lines; i++)
		{
			event_list[stream+AXI_HIST_TEST_NUM_STREAMS*i] = (row_start[stream]+col[stream]) | AXI_HIST_TEST_VALID | AXI_HIST_TEST_FIRST;
			dram_row = GET_ROW(event_list[stream+AXI_HIST_TEST_NUM_STREAMS*i]);
			activations[stream][dram_row]++;
			if (activations[stream][dram_row] > 100000)
			{
				int j;
				if (dram_row > 0)
				{
					for (j=0; j<16 && i<num_lines-1; j++)
					{
						i++;
						event_list[stream+AXI_HIST_TEST_NUM_STREAMS*i] = ((dram_row-1)<<10) | AXI_HIST_TEST_VALID | AXI_HIST_TEST_FIRST;
					}
				}
				printf("Inserting dummy events stream %d, at offset 0x%08X\n", stream, ((dram_row+1)<<10) | AXI_HIST_TEST_VALID );
				for (j=0; j<16 && i<num_lines-1; j++)
				{
					i++;
					event_list[stream+AXI_HIST_TEST_NUM_STREAMS*i] = ((dram_row+1)<<10) | AXI_HIST_TEST_VALID | AXI_HIST_TEST_FIRST;
				}
				activations[stream][dram_row] = 0;
			}
			col[stream]++;
			if (col[stream] >= row_len[stream])
			{
				col[stream] =  0;
				row_len[stream]--;
				if (row_len[stream] == 0)
				{
					row_start[stream] += max_row_len[stream];
					max_row_len[stream] ++;
					row_len[stream] = max_row_len[stream];  
					if (row_start[stream] + max_row_len[stream] > HIST_BLOCK_SIZE)
					{
						printf("Hist test pattern wrapped to start of block at line=%d, stream=%d\n", i, stream);
						row_start[stream] = 0;
					}
				}
			}
		}
	}
	if (last)
	{
		for (i=0; i<AXI_HIST_TEST_NUM_STREAMS; i++)
			free(activations[i]);
	}
	return num_lines*8;
}


int hist_generate_tp_ramp_row_hammer2(int num_lines, int tp_line, u_int32_t *event_list, int pass, int first, int last, int gen_options)
{
	int stream;
	static int row_len[AXI_HIST_TEST_NUM_STREAMS];
	static int row_start[AXI_HIST_TEST_NUM_STREAMS];
	static int max_row_len[AXI_HIST_TEST_NUM_STREAMS];
	static int col[AXI_HIST_TEST_NUM_STREAMS];
	int i;
	static int *activations[AXI_HIST_TEST_NUM_STREAMS];
	int dram_col, dram_row;
	int pass_inner = pass  % 12;
	int pass_outer = pass/12;
	int stream_mask;

	if (pass_outer == 0)
		stream_mask = 0xFFFF;
	else
	{
		stream = tp_line/ 1024*1024;
		stream_mask = (1<< pass_outer)-1;
		stream <<= stream % AXI_HIST_TEST_NUM_STREAMS;
		stream = (stream & 0xFF) | ((stream & 0xFF00)>>8);
	}
	if (tp_line == 0)
	{
		mprintf(2, "...... Outer pass= %d => starting stream mask= %02X\n", pass_outer, stream_mask);
		if (pass_inner < 4)
		{
			dram_row = 1 + pass_inner*17;
			mprintf(2, "...... Inner pass = %d => 1 ROW = %d\n", pass_inner, dram_row); 
		}
		else if (pass_inner < 8)
		{
			dram_row = (pass_inner-4)*17+1;
			mprintf(2, "...... Inner pass = %d => 2 neighbouring rows = %d,%d\n", pass_inner, dram_row, dram_row+1); 
		}
		else
		{
			dram_row = (pass_inner-8)*17+1;
			mprintf(2, "...... Inner pass = %d => 2 rows 1 gap = %d,%d\n", pass_inner, dram_row, dram_row+2); 
		}
		for (stream=0; stream<AXI_HIST_TEST_NUM_STREAMS; stream++)
		{
			max_row_len[stream] = 60 + stream;
			row_len[stream] = max_row_len[stream];
			row_start[stream] = 0;
			col[stream] = 0;
			if ((activations[stream]=calloc(NUM_ROW_ADDRESSES, sizeof(int))) == NULL)
			{
				printf("Out of memory\n");
				exit(1);
			}
		}
	}
	for (stream=0; stream<AXI_HIST_TEST_NUM_STREAMS; stream++)
	{
		if (stream_mask & 1 << stream)
		{
			for (i=0; i<num_lines; i++)
			{
				int offset = (row_start[stream]+col[stream]);
				dram_col = offset & 0x3FF;
				dram_row = GET_ROW(offset);
				if (pass_inner < 4)
				{
					dram_row = 1 + pass_inner*17;
				}
				else if (pass_inner < 8)
				{
					dram_row &= 1;
					dram_row += (pass_inner-4)*17+1;
				}
				else
				{
					dram_row = (dram_row&1) << 1;
					dram_row += (pass_inner-8)*17+1;
				}
				event_list[stream+AXI_HIST_TEST_NUM_STREAMS*i] = dram_row << 10 | dram_col | AXI_HIST_TEST_VALID | AXI_HIST_TEST_FIRST;
				activations[stream][dram_row]++;
				if (0 && activations[stream][dram_row] > 100000)
				{
					int j;
					if (dram_row > 0)
					{
						for (j=0; j<16 && i<num_lines-1; j++)
						{
							i++;
							event_list[stream+AXI_HIST_TEST_NUM_STREAMS*i] = ((dram_row-1)<<10) | AXI_HIST_TEST_VALID | AXI_HIST_TEST_FIRST;
						}
					}
					printf("Inserting dummy events stream %d, at offset 0x%08X\n", stream, ((dram_row+1)<<10) | AXI_HIST_TEST_VALID );
					for (j=0; j<16 && i<num_lines-1; j++)
					{
						i++;
						event_list[stream+AXI_HIST_TEST_NUM_STREAMS*i] = ((dram_row+1)<<10) | AXI_HIST_TEST_VALID | AXI_HIST_TEST_FIRST;
					}
					activations[stream][dram_row] = 0;
				}
				col[stream]++;
				if (col[stream] >= row_len[stream])
				{
					col[stream] =  0;
					row_len[stream]--;
					if (row_len[stream] == 0)
					{
						row_start[stream] += max_row_len[stream];
						max_row_len[stream] ++;
						row_len[stream] = max_row_len[stream];  
						if (row_start[stream] + max_row_len[stream] > HIST_BLOCK_SIZE)
						{
							printf("Hist test pattern wrapped to start of block at line=%d, stream=%d\n", i, stream);
							row_start[stream] = 0;
						}
					}
				}
			}
		}
		else
		{
			for (i=0; i<num_lines; i++)
				event_list[stream+AXI_HIST_TEST_NUM_STREAMS*i] = 0;
		}
	}
	if (last)
	{
		for (i=0; i<AXI_HIST_TEST_NUM_STREAMS; i++)
			free(activations[i]);
	}
	return num_lines*8;
}
					  
int hist_generate_tp_random(int num_lines, int tp_line, u_int32_t *event_list, int pass, int first, int last, int gen_options)
{
	int stream;
	int i;
	int nbins=HIST_BLOCK_SIZE;
	if (gen_options >0 && gen_options < HIST_BLOCK_SIZE)
		nbins = gen_options;
	for (stream=0; stream<AXI_HIST_TEST_NUM_STREAMS; stream++)
	{
		for (i=0; i<num_lines; i++)
			event_list[stream+AXI_HIST_TEST_NUM_STREAMS*i] = (random() % nbins) | AXI_HIST_TEST_VALID | AXI_HIST_TEST_FIRST;
	}
	return num_lines*8;
}
/* Build test patterns which emulated XSPRESS4 spectra with multiple time frames
	The behaviour is modified by multiple globals.
	Each spectrum is 4096 bin, with a k alpha +beta peak peak and 2x pileup peak,and some background 
	See
	#define AXI_HIST_TP_MAX_PEAKS	4
	typedef struct
	{
		int num_bins;				//!< Number of energy bins, typcially 4096 for Xspress3/4
		int num_peaks;			//!< Number of K-alpha peaks (k-beta infered at 1.1 x position 0.2 * height)
		struct 
		{
			int posn;		//!<	k-alpha peak position (bins)
			double height;
			int fwhm;		//!<	k-alpha peak FWHM (bins)
		} peak[AXI_HIST_TP_MAX_PEAKS];
		double pileup2;		//!<	number of counts in 2x pileup peak compared to k alpha (typically 0.2)
		double background	//!<	total number of counts backgound compared to k alpha (typically 0.05)
	} AXIHistTPSpectra;


	*/

#define NUM_RANDOM (4096*128)



double *axi_hist_tp_build_spectra(AXIHistTPSpectra *tp)
{
	double * spectra_clean, *spectra;
	int i, j;
	double total;
	FILE *fp;

	spectra_clean =(double *)malloc(sizeof(double)*tp->num_bins);
	if (spectra_clean == NULL)
		return NULL;
	
	spectra =(double *)malloc(sizeof(double)*tp->num_bins);
	if (spectra == NULL)
	{
		free(spectra_clean);
		return NULL;
	}
	for (i=0; i<tp->num_bins; i++)
	{
		spectra_clean[i] = 0.0;
		spectra[i] = 0.0;
	}
	for (j=0; j<tp->num_peaks && j < AXI_HIST_TP_MAX_PEAKS; j++)
	{
		double sigma = tp->peak[j].fwhm/2.355;
		double two_ss = 2*sigma*sigma;
		for (i=0; i<tp->num_bins; i++)
		{
			double x = i-tp->peak[j].posn;
			double y = exp(-x*x/two_ss)*tp->peak[j].height;
			spectra_clean[i] += y;
			x = i-1.1*tp->peak[j].posn;
			y = exp(-x*x/two_ss)*tp->peak[j].height * 0.2;
			spectra_clean[i] += y;
		}
	}

	for (i=0; i<tp->num_bins; i++)
		spectra[i] = spectra_clean[i];

	for (j=0; j<tp->num_bins; j++)
	{
		for (i=0; i<tp->num_bins; i++)
		{
			if (i+j < tp->num_bins)
				spectra[i+j] += spectra_clean[i]*spectra_clean[j]*tp->pileup2;
		}
	}

	total = 0.0;
	for (i=0; i<tp->num_bins; i++)
	{
		spectra[i] += tp->background;
		total += spectra[i];
	}
	for (i=0; i<tp->num_bins; i++)
		spectra[i] /= total;

	if ((fp=fopen("spectra.txt", "w")) != NULL)
	{
		for (i=0; i<tp->num_bins; i++)
			fprintf(fp, "%d\t%g\t%g\n", i, spectra_clean[i], spectra[i]);
		fclose(fp);
	}
	free (spectra_clean);
	return spectra;
}		

int hist_generate_tp_xspress4(int num_lines, int tp_line, u_int32_t *event_list, int pass, int first, int last, int gen_options)
{
	int stream;
	int i, j;
static double *spectra;
static	int *lut;
	double total;
	int tf, max_tf;

	max_tf = HIST_BLOCK_SIZE/xspress4_tp.num_bins;
	if (first)
	{
		spectra=axi_hist_tp_build_spectra(&xspress4_tp);
		if (spectra == NULL)
		{
			mprintf(MP_ERR, "OUT of Memory building spectrum base test pattern\n");
			return -1;
		}

		if ((lut=(int *)malloc(sizeof(int)*NUM_RANDOM)) == NULL)
		{
			free(spectra);
			mprintf(MP_ERR, "OUT of Memory building spectrum base test pattern\n");
			return -1;
		}
		total=0.0;

		for (i=1; i<xspress4_tp.num_bins; i++)
			spectra[i] += spectra[i-1];
		
		for (i=0, j=0;  i<NUM_RANDOM && j<xspress4_tp.num_bins; j++)
		{
			while (total < spectra[j] && i < NUM_RANDOM)
			{
//				printf("i=%d, j=%d, spectra[j]=%g, total=%g\n", i, j, spectra[j], total); 
				total += 1.0/NUM_RANDOM;
				lut[i++] = j;
			}
		}
		while (i<NUM_RANDOM)
			lut[i++] = 0;
		free (spectra);
		spectra=NULL;
	}
	for (stream=0; stream<AXI_HIST_TEST_NUM_STREAMS; stream++)
	{
		for (i=0; i<num_lines; i++)
		{
			tf = i/xspress4_tp.average_events_per_tf;
			tf %= max_tf;
			event_list[stream+AXI_HIST_TEST_NUM_STREAMS*i] = (lut[rand() % NUM_RANDOM]+xspress4_tp.num_bins*tf) | AXI_HIST_TEST_VALID | AXI_HIST_TEST_FIRST;
		}
	}
	if (last)
	{
		/*
		for (i=num_lines-4; i<num_lines; i++)
		{
			printf("line=%d:", i);
			for (stream=0; stream<AXI_HIST_TEST_NUM_STREAMS; stream++)
				printf(" %08X", event_list[stream+AXI_HIST_TEST_NUM_STREAMS*i]);
			printf("\n");
		} */
		free (lut);
		lut = NULL;
	}
	return num_lines*8;
}


int write_histogram_data(int path, int card, char *fname, u_int32_t *rbuf, int num_streams, int stream_stride, int use_dma)
{
	int io_errors=0;
	int rc;
	int i, tf;
	int stream;
	FILE *ofp;

	if ((ofp=fopen(fname, "w")) == NULL)
	{
		fprintf(stderr, "ERROR: Cannot open output file '%s'\n", fname);
		return -1;
	}
	else
	{
		for (i=0; i<4096; i++)
			fprintf(ofp,"\t%d", i);
		fprintf(ofp,"\n");
		for (stream=0; stream<num_streams; stream++)
		{
			if (use_dma)
			{
				if ((rc=ngzmp_hist_read_stream_dma(path, card, stream, 0, HIST_BLOCK_SIZE, rbuf, 0)) < 0)
				{
					io_errors++;
					if (io_errors < max_errors)
						printf("ERROR: ngzmp_hist_read_dma returned error %s\n", ngpd_get_error_message());
					else if (io_errors == max_errors)
						printf("Too many IO ERRORS: Counting\n");
				}
			}
			else
			{
				if ((rc=ngzmp_hist_read_stream(path, card, stream, 0, HIST_BLOCK_SIZE, rbuf)) < 0)
				{
					io_errors++;
					if (io_errors < max_errors)
						printf("ERROR: zynqmp_read returned error %d\n", rc);
					else if (io_errors == max_errors)
						printf("Too many IO ERRORS: Counting\n");
				}
			}
//			for (tf=0;tf<HIST_BLOCK_SIZE/4096;tf++)
			for (tf=0;tf<16;tf++)
			{
				fprintf(ofp, "TF %d,Stream %d:", tf, stream);
				for (i=0; i<4096; i++)
					fprintf(ofp,"\t%d", rbuf[i+4096*tf]);
				fprintf(ofp, "\n");
			}
		}
		fclose(ofp);
	}
	return 0;
}


int hist_display_progress(int path, int card)
{
	ZYNQMP_DMA_MsgCheckDesc msg;
	u_int32_t completed_desc, last_frame, status;
	int rc;

	memset(&msg, 0, sizeof(ZYNQMP_DMA_MsgCheckDesc));
	rc = ngzmp_dma_check_desc(path, card, NGZMP_DMA_STREAM_BNUM_HIST_TEST, &msg, NULL, &completed_desc, &last_frame, &status);

	mprintf(MP_PRINT, "rc=%d, Completed desc=%d, last status=%08X\n", rc, completed_desc, status);

	return rc;
}

int hist_exercise_clear_and_read(int path, int card, AXIHistConfig *hist_conf, u_int32_t *rbuf, int hist_block_words, int repeat)
{
	int test_block_hist_words;
	int test_clear_start_word;
	int io_errors=0;
	int stream;
	int rc;
	int num_clears=0;
	int i;
	int long_clear = 0;
	int omit_clears=0;
	static int sent_warning;
		
	if (repeat == 0)
		sent_warning = 0;
	if (hist_block_words < HIST_BLOCK_SIZE)
		test_clear_start_word = HIST_BLOCK_SIZE;
	else
		test_clear_start_word = hist_block_words;
	if (hist_conf->HistWordsPerStream < HIST_BLOCK_SIZE+hist_block_words)
	{
		if (hist_block_words >= hist_conf->HistWordsPerStream)
		{
			if (!sent_warning)
			{
				mprintf(MP_ERR, "hist_exercise_clear_and_read:Warning:  no space for dummy exercising buffer hist_conf->HistWordsPerStream=%d, HIST_BLOCK_SIZE=%d\n", hist_conf->HistWordsPerStream, HIST_BLOCK_SIZE);
				sent_warning = 1;
			}
			omit_clears = 1;
			test_block_hist_words = 0;
		}
		else
		{
			test_block_hist_words = hist_conf->HistWordsPerStream-hist_block_words;
		}
	}
	else
		test_block_hist_words = HIST_BLOCK_SIZE;

	if (!omit_clears)
	{
		if (((test_hist_opt & TEST_HIST_OPT_EX_CLEAR) && repeat % 3 == 0) || (test_hist_opt & TEST_HIST_OPT_EX_LONG_CLEAR))
		{
//			printf("Large Dummy clear from 0x%08X for %08X AXI words\n", test_clear_start_word/hist_conf->HistWordsPerAXIWord, test_block_hist_words/hist_conf->HistWordsPerAXIWord);
			if (ngzmp_hist_clear_all_start(path, card, test_clear_start_word/hist_conf->HistWordsPerAXIWord, test_block_hist_words/hist_conf->HistWordsPerAXIWord) != 0)
			{
				printf("Error staring clear\n");
				io_errors++;
			}
			num_clears = 1;
			long_clear = 1;
		}
		else if (((test_hist_opt & TEST_HIST_OPT_EX_CLEAR) && repeat % 3 == 1) ||	(test_hist_opt & TEST_HIST_OPT_EX_SHORT_CLEAR))
		{
			for (i=0; i<10; i++)
			{
				int nw = (rand() % 20)+1;
				int start = (rand() % 20)+1;
//				printf("Dummy clear from 0x%08X for %08X AXI words\n", test_clear_start_word/hist_conf->HistWordsPerAXIWord+start, nw);
				if (ngzmp_hist_clear_all_start(path, card, test_clear_start_word/hist_conf->HistWordsPerAXIWord+start, nw) != 0)
				{
					printf("Error staring clear\n");
					io_errors++;
				}
				if (ngzmp_hist_clear_wait(path, card) != 0)
				{
					printf("hist_exercise_clear_and_read: Error waiting for short clear to finish\n");
					return -1;
				}
				num_clears ++;
			}
		}
	}
	if (((test_hist_opt & TEST_HIST_OPT_EX_READ) && repeat % 5 == 0) ||	(test_hist_opt & (TEST_HIST_OPT_EX_LONG_READ|TEST_HIST_OPT_EX_LONG_DISCARD)))
	{
		for (stream=0; stream<hist_conf->NumStreams; stream++)
		{
			if (test_hist_opt & TEST_HIST_OPT_EX_LONG_DISCARD)
			{
				if ((rc=ngzmp_hist_read_discard(path, card, stream*hist_conf->HistWordsPerStream, HIST_BLOCK_SIZE)) < 0)
				{
					io_errors++;
					if (io_errors < max_errors)
						printf("ERROR: ngzmp_hist_read_discard returned error %s\n", ngpd_get_error_message());
					else if (io_errors == max_errors)
						printf("Too many IO ERRORS: Counting\n");
				}
			}
			else
			{
				if ((rc=ngzmp_hist_read_dma(path, card, stream*hist_conf->HistWordsPerStream, HIST_BLOCK_SIZE, rbuf, AXI_HIST_RC_ACROSS_STREAMS)) < 0)
				{
					io_errors++;
					if (io_errors < max_errors)
						printf("ERROR: ngzmp_hist_read_dma returned error %s\n", ngpd_get_error_message());
					else if (io_errors == max_errors)
						printf("Too many IO ERRORS: Counting\n");
				}
			}
		}
	}
	else if (((test_hist_opt & TEST_HIST_OPT_EX_READ) && (repeat % 5 != 1 || repeat % 5 == 3) )	|| (test_hist_opt & (TEST_HIST_OPT_EX_SHORT_READ|TEST_HIST_OPT_EX_SHORT_DISCARD)) )
	{
		for (i=0; i<10;i++)
		{
			int nw = (rand() % 20)+1;
			int start = (rand() % 20)+1;
			if (rand() % 3 == 2)
				nw += 2000;
			for (stream=0; stream<hist_conf->NumStreams; stream++)
			{
				if (test_hist_opt & TEST_HIST_OPT_EX_SHORT_DISCARD)
				{
					if ((rc=ngzmp_hist_read_discard(path, card, stream*hist_conf->HistWordsPerStream+start, nw)) < 0)
					{
						io_errors++;
						if (io_errors < max_errors)
							printf("ERROR: ngzmp_hist_read_discard returned error %s\n", ngpd_get_error_message());
						else if (io_errors == max_errors)
							printf("Too many IO ERRORS: Counting\n");
					}
				}
				else
				{
					if ((rc=ngzmp_hist_read_dma(path, card, stream*hist_conf->HistWordsPerStream+start, nw, rbuf, AXI_HIST_RC_ACROSS_STREAMS)) < 0)
					{
						io_errors++;
						if (io_errors < max_errors)
							printf("hist_exercise_clear_and_read:ERROR: ngzmp_hist_read_dma returned error %s\n", ngpd_get_error_message());
						else if (io_errors == max_errors)
							printf("Too many IO ERRORS: Counting\n");
					}
				}
			}
		}
	}
	if (long_clear)	
	{
		if (ngzmp_hist_clear_wait(path, card) != 0)
		{
			printf("hist_exercise_clear_and_read: Error waiting for large clear to finish\n");
			return -1;
		}
	}
	if (io_errors > 0)
		return -io_errors;
	return num_clears;
}


/* Hist data gap
 The idea is to put gaps in the data stream to the histogrammer to exercise bubbles in the data supply, including region where we swap from data starvation to backpressure.

 Total buffer is 2 Gbytes, => 2048/(8*4) = 64 Meg lines
 To refill the FIFOs after a long gap would take about > 512 lines, suggest 1024.
 Allows 64 k (different) gaps.
 There are 255 different gaps, but half are after the fifo has been allow to empty, so don't need the biggest gaps with this, perhaps only  32 smaller gaps

 64k /(127+32)  = 412 attempts at each

 So consider permuting first gap, (All 159 types) x 8 small + 8 medium, 4 delay x 21 (ish) different block sizes

*/

int hist_insert_delays(int path, int card, int type, u_int32_t *event_list, int test_pattern_lines)
{
	HistDelayGen hd[4];
	int i;
	int tp_lines, num_lines;
	int mem_chunk=test_pattern_lines/4;
	tp_lines = EVENT_BLOCK_SIZE;

	if (type)
		mprintf(2, "Inserting delays\n");
	else
		mprintf(2, "Removing delays\n");

	for (i=0; i<4; i++)
	{
		num_lines = tp_lines;
		if (num_lines > EVENT_BLOCK_SIZE/4)
			num_lines = EVENT_BLOCK_SIZE/4;
		hd[i].path = path;
		hd[i].card = card;
		hd[i].type = type;
		hd[i].num_lines = num_lines;
		hd[i].buffer = event_list;
		event_list += AXI_HIST_TEST_NUM_STREAMS*num_lines;
		hd[i].start_index = i;
		hd[i].mem_start = mem_chunk*i;
		hd[i].mem_num = mem_chunk;
	}
	for (i=0; i<4; i++)
	{
		hist_insert_delay_thread(hd+i);
	}
	return 0;
}
void * hist_insert_delay_thread(void *vp)
{
	HistDelayGen *hd = (HistDelayGen *)vp;
	int remaining = hd->mem_num;
	int tp_line = hd->mem_start;
	int chunk;
	int rc;
	int io_errors;
	int gap = 256;
	int i, j;
	int toggle=0;
	int delay0=1, delay1=1;
	int delay;
	u_int32_t *line;
	int gap_group=0;

	while (remaining > 0)
	{
		chunk = remaining;
		if (chunk > hd->num_lines)
			chunk = hd->num_lines;

		if ((rc=zynqmp_read_dma_buff(hd->path, hd->card, NGZMP_DMA_STREAM_BNUM_HIST_TEST, tp_line*AXI_HIST_TEST_NUM_STREAMS, chunk*AXI_HIST_TEST_NUM_STREAMS, hd->buffer)) < 0)
		{
			io_errors++;
			mprintf(MP_ERR,"ERROR writing test pattern to memory\n");
		}
		line = hd->buffer;
		for (i=0;i<chunk; i++)
		{
			for (j=0;j<AXI_HIST_TEST_NUM_STREAMS; j++)
				line[j] &= 0xBFFFFFFF;
			if (hd->type && --gap == 0)
			{
				if (toggle == 0)
				{
					delay = delay0;
				}
				else
				{
					delay = delay1;
					delay0++;
					if (delay0 == 160)
					{
						delay0 = 1;
						delay1++;
						if (delay1 == 9)
							delay1 = 64;
						else if (delay1 == 73)
							delay1 = 128;
						else if (delay1 == 132)
						{
							delay1 = 1;
							gap_group++;
						}
					}
				}
				toggle = ! toggle;
				for (j=0;j<AXI_HIST_TEST_NUM_STREAMS; j++)
				{
					if (delay & 1 << j)
						line[j] |= AXI_HIST_TEST_DELAY;
				}

				gap = 512-80 + 16*(gap_group+hd->start_index*4);
				if (0 && tp_line+i < 100000) printf("Line %d: delay=%d, gap=%d, gap_group=%d\n", tp_line+i, delay, gap, gap_group);
			}
			line += AXI_HIST_TEST_NUM_STREAMS;
		}

		if ((rc=zynqmp_write_dma_buff(hd->path, hd->card, NGZMP_DMA_STREAM_BNUM_HIST_TEST, tp_line*AXI_HIST_TEST_NUM_STREAMS, chunk*AXI_HIST_TEST_NUM_STREAMS, hd->buffer)) < 0)
		{
			io_errors++;
			mprintf(MP_ERR,"ERROR writing test pattern to memory\n");
		}
		tp_line += chunk;
		remaining -= chunk;
	}

	return (void *) hd;
}


/*
 	Read modify write the event list in memory to remove hist enable signals so that some streams are use much less frequently

	0 : Continous
 	1 :	Miss 1 in 11
	2 : Miss 1 in 5
	3 : Miss 1 in 3
	4	Miss 1 in 2
	5	Send 1 in 3
	6 	Send 1 in 7
	7 : Send 1 in 13 

*/	
int hist_thin(int path, int card, u_int32_t *event_list, u_int32_t *expected[AXI_HIST_MAX_STREAMS], int test_pattern_lines, int num_streams, int64_t stream_totals[AXI_HIST_MAX_STREAMS] ) 
{
	int remaining = test_pattern_lines;
	int tp_line = 0;
	int chunk;
	u_int32_t *line;
	int stream;
	int roll_count=0;
	int roll = 0;
	int io_errors = 0;
	int i, j, rc;

	for (stream=0; stream<num_streams; stream++)
	{
		memset(expected[stream], 0, sizeof(u_int32_t)*HIST_BLOCK_SIZE);
		stream_totals[stream] = 0;
	}
	while (remaining > 0)
	{
		chunk = remaining;
		if (chunk > EVENT_BLOCK_SIZE)
			chunk = EVENT_BLOCK_SIZE;

		if ((rc=zynqmp_read_dma_buff(path, card, NGZMP_DMA_STREAM_BNUM_HIST_TEST, tp_line*AXI_HIST_TEST_NUM_STREAMS, chunk*AXI_HIST_TEST_NUM_STREAMS, event_list)) < 0)
		{
			io_errors++;
			mprintf(MP_ERR,"ERROR reading test pattern to memory\n");
		}
		line = event_list;
		for (i=0;i<chunk; i++)
		{
			for (stream=0; stream<AXI_HIST_TEST_NUM_STREAMS; stream++)
				line[stream] &= 0x7FFFFFFF;
			for (stream=0; stream<AXI_HIST_TEST_NUM_STREAMS; stream++)
			{
				j = (stream+roll) % AXI_HIST_TEST_NUM_STREAMS;
				switch (j)
				{
				case 0: 
					line[stream] |= AXI_HIST_TEST_VALID;
					break;
				case 1: 
					if (i % 11 != 10)
						line[stream] |= AXI_HIST_TEST_VALID;
					break;
				case 2:
					if (i % 5 != 4)
						line[stream] |= AXI_HIST_TEST_VALID;
					break;
				case 3:
					if (i % 3 != 2)
						line[stream] |= AXI_HIST_TEST_VALID;
					break;
				case 4:
					if (i % 2 != 1)
						line[stream] |= AXI_HIST_TEST_VALID;
					break;
				case 5:
					if (i % 3 == 2)
						line[stream] |= AXI_HIST_TEST_VALID;
					break;
				case 6:
					if (i % 7 == 3)
						line[stream] |= AXI_HIST_TEST_VALID;
					break;
				case 7:
					if (i % 13 == 4)
						line[stream] |= AXI_HIST_TEST_VALID;
					break;
				default:
					printf("Coding error j=%d @ %s:%d\n", j, __FILE__, __LINE__);
					exit(1);
				}
			}
			if (++roll_count == 100)
			{
				roll_count = 0;
				roll = (roll+1) % AXI_HIST_TEST_NUM_STREAMS; 
			}
			line += AXI_HIST_TEST_NUM_STREAMS;
		}

		if ((rc=zynqmp_write_dma_buff(path, card, NGZMP_DMA_STREAM_BNUM_HIST_TEST, tp_line*AXI_HIST_TEST_NUM_STREAMS, chunk*AXI_HIST_TEST_NUM_STREAMS, event_list)) < 0)
		{
			io_errors++;
			mprintf(MP_ERR,"ERROR writing test pattern to memory\n");
		}
		hist_test_pattern_to_expected(event_list, expected, chunk, num_streams, stream_totals);
		tp_line += chunk;
		remaining -= chunk;
	}
	if (reporting >= 5)
	{
		for (i=0;i<num_streams; i++)
			printf("Stream %d : New total= %ld\n", i, stream_totals[i]);
	}

	return 0;
}


int run_test_hist_tpg_rand(int path, int card, int num_pass, int use_read_dma)
{
	AXIHistConfig hist_conf;
	int rc;
	u_int32_t *rbuf;
	int io_errors = 0;
	int pass;
	u_int32_t axi_hist_gpio_control=0;
	struct timeval t_start_dma, t_now;
	double t_hist, t_hist2;
	struct timespec delay, remaining;
	int errors=0;
	int stream_stride;
	u_int32_t tpg_status, axi_hist_gpio1_status, axi_hist_gpio2_status;
	u_int64_t hist_busy_time;
	u_int64_t total_events;
	int repeat;
	int timeout=0;
	int num_clears = 0;
	int tpg_size;
	int words_one_loop;
	int num_loops;
	u_int32_t tpg_cont[3];
	int num_words;

	mprintf(1, "Running Histogram tests using PRBS random test pattern generator\n");

	rc = ngzmp_hist_read_config(path, card, &hist_conf);
	if (rc != 0)
	{
		io_errors++;
		printf("Error reading AXI Histogrammer config\n");
	}
	if (hist_conf.NumStreams > AXI_HIST_MAX_STREAMS)
	{
		fprintf(stderr, "Number of streams =%d > AXI_HIST_MAX_STREAMS=%d, please recompile with larger AXI_HIST_MAX_STREAMS\n", hist_conf.NumStreams, AXI_HIST_MAX_STREAMS);
		exit(1);
	}

	if (hist_conf.StreamAtBottom)
		stream_stride = hist_conf.HistWordsPerAXIWord;
	else
		stream_stride = hist_conf.HistWordsPerStream;

	if ((rbuf=malloc(sizeof(u_int32_t)*HIST_BLOCK_SIZE)) == NULL)
	{
		fprintf(stderr, "test_hist_hist: Out of memory allocating read block for histogram\n");
		exit(1);
	}

	for (pass=test_hist_first_pass; pass<num_pass; pass++)
	{
		tpg_size=pass%8;
		words_one_loop = 1;
		switch (tpg_size)
		{
		case 0: words_one_loop = (1<<9) - 1; break;
		case 1: words_one_loop = (1<<10) - 1; break;
		case 2: words_one_loop = (1<<11) - 1; break;
		case 3: words_one_loop = (1<<12) - 1; break;
		case 4: words_one_loop = (1<<15) - 1; break;
		case 5: words_one_loop = (1<<18) - 1; break;
		case 6: words_one_loop = (1<<20) - 1; break;
		case 7: words_one_loop = (1<<23) - 1; break;
		}
		num_loops = ((int64_t)40000000)*100/(words_one_loop*hist_conf.NumStreams);
		if ( num_loops < 2)
			num_loops = 2;
		mprintf(2, ".... Histogram test with TPG size code=%d, word_one_loop=%d, num_loops=%d\n", tpg_size, words_one_loop, num_loops);
		for (repeat=0; repeat<test_hist_repeat; repeat++)
		{
			mprintf(3, ".... Clearing memory repeat=%d\n", repeat);
			num_clears = 0;
			num_words = hist_conf.StreamAtBottom?hist_conf.AXIWordsPerStream*hist_conf.NumStreams:hist_conf.AXIWordsPerStream; 
			if (ngzmp_hist_clear_start(path, card, 0, num_words, 1) != 0)
			{
				printf("Error staring clear\n");
				io_errors++;
			}
			if (ngzmp_hist_clear_wait(path, card) != 0)
			{
				printf("Error waiting for clear to finish\n");
				return 0;
			}
			if (test_hist_opt & TEST_HIST_OPT_CHECK_CLEARED)
			{
				errors += check_histogram_tpg_data(path, card, rbuf, TEST_CLEAR, words_one_loop, num_loops, hist_conf.NumStreams, stream_stride, use_read_dma);
			}
			mprintf(3, ".... Preparing test pattern generator size code=%d, word_one_loop=%d, num_loops=%d\n", tpg_size, words_one_loop, num_loops);
			if (manual_test) { printf("Press RETURN to start DMA repeat=%d,\n", repeat); fflush(stdout); getchar(); }

			tpg_cont[0] = AXI_HIST_TC_HIST_TPG(AXI_HIST_TPG_RAND);
			tpg_cont[1] = num_loops*words_one_loop;
			tpg_cont[2] = tpg_size;

			axi_hist_gpio_control = 0;
			if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x400, 1, &axi_hist_gpio_control)) < 0)
			{
				mprintf(MP_ERR, "ERROR writing to GPIO for hist test DMA\n");
				io_errors++;
			}
			if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, AXI_HIST_TESTPAT_CONT, 3, tpg_cont)) < 0)
			{
				mprintf(MP_ERR, "ERROR writing to GPIO for hist test DMA\n");
				io_errors++;
			}

			axi_hist_gpio_control = NGZMP_HIST_GPIO_ENABLE; // Using internal TPG should not need external test data 
			if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x400, 1, &axi_hist_gpio_control)) < 0)
			{
				mprintf(MP_ERR, "ERROR writing to GPIO for hist test DMA\n");
				io_errors++;
			}

			gettimeofday(&t_start_dma, NULL);
			delay.tv_sec = 0;
			delay.tv_nsec=1E6;
			do
			{
				if ((rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_HIST, AXI_HIST_TEST_STATUS, 1, &tpg_status)) < 0)
				{
					mprintf(MP_ERR, "ERROR Reading status from hist TPG\n");
					io_errors++;
				}
				if (reporting >= 5)
					printf(".... DMA status=%08X\n", tpg_status);
				gettimeofday(&t_now, NULL);
				t_hist = t_now.tv_sec-t_start_dma.tv_sec;
				t_hist += 1E-6*(t_now.tv_usec-t_start_dma.tv_usec);

				if (!(tpg_status & AXI_HIST_TSTATUS_TPG_BUSY))
					break;
				if (test_hist_opt & (TEST_HIST_OPT_EXERCISE | TEST_HIST_OPT_EX_FIXED))
				{
					rc =hist_exercise_clear_and_read(path, card, &hist_conf, rbuf, words_one_loop+1, repeat);
					if (rc >= 0)
						num_clears += rc;
				}
				else
				{
					remaining = delay;
					while (nanosleep(&remaining, &remaining) == EINTR);
				}

			} while (t_hist < 220);
			if (t_hist > 220)
			{
				mprintf(MP_ERR, "Timout waiting for hist test pattern generator to become idle\n");
				timeout++;
			}
			mprintf(3, "TPG finished after %g seconds\n", t_hist);
			do
			{
				if ((rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x402, 1, &axi_hist_gpio2_status)) < 0)
				{
					mprintf(MP_ERR, "ERROR Reading status from hist GPIO\n");
					io_errors++;
				}
				if (reporting >= 5)
					printf(".... GPIO Hist status=%08X\n", axi_hist_gpio2_status);
				gettimeofday(&t_now, NULL);
				t_hist = t_now.tv_sec-t_start_dma.tv_sec;
				t_hist += 1E-6*(t_now.tv_usec-t_start_dma.tv_usec);

				if ((axi_hist_gpio2_status & (1<<30)) == 0)
					break;
				remaining = delay;
				while (nanosleep(&remaining, &remaining) == EINTR);

			} while (t_hist < 240);
			if (t_hist > 240)
			{
				mprintf(MP_ERR, "Timout waiting for histogramming to finish\n");
				timeout++;
			}
			if ((rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x400, 1, &axi_hist_gpio1_status)) < 0 || (rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x402, 1, &axi_hist_gpio2_status)) < 0)
			{
				mprintf(MP_ERR, "ERROR Reading status from hist GPIO\n");
				io_errors++;
			}
			hist_busy_time = (((u_int64_t)(axi_hist_gpio2_status&0x3FFFFFFF)) << 32) |  axi_hist_gpio1_status;
			t_hist2 = hist_clk_period*hist_busy_time;
			total_events = num_loops*words_one_loop;
			total_events *= hist_conf.NumStreams;
			mprintf(MP_PRINT, "Histogramming finished: Pass=%d, Repeat=%d. Software timer= %g , firmware timer=%g seconds, num_clears=%d => Histrogram rate=%g\n", 
				pass, repeat, t_hist, t_hist2, num_clears, total_events/t_hist2  );
			if (report_file != NULL)
				fprintf(report_file, "TPG-Rand: size code=%d, size=%d : Rate= %g\n", tpg_size, words_one_loop, total_events/t_hist2);
			if (fabs(t_hist-t_hist2) > 0.1+0.005*t_hist)
			{
				mprintf(MP_ERR, "*ERROR* Software and firmware timing differ significantly. Currently assuming input cloc kperiod=%g\n", hist_clk_period);
				mprintf(MP_ERR, "If this is wrong, consisder the hist-clk option to change clk period\n", hist_clk_period);
			}

			errors += check_histogram_tpg_data(path, card, rbuf, 0, words_one_loop, num_loops, hist_conf.NumStreams, stream_stride, use_read_dma);
			if (test_hist_write[0] != 0)
				write_histogram_data(path, card, test_hist_write, rbuf, hist_conf.NumStreams, stream_stride, use_read_dma);
			if (test_hist_repeat>1 && errors > 0 && exit_on_error)
			{
				printf("Stopping due to errors\n");
				exit(1);
			}
		}
	}
	free(rbuf);
	num_io_errors += io_errors;
	if (report_file != NULL)
		fprintf(report_file, "Finished histogram tests using RAND TPG with %d errors and %d io_errors accessing data via %s\n", errors, io_errors, use_read_dma?"DMA read from PL to PS":"CPU read from PL");
	printf("Finished histogram tests using RAND TPG with %d errors and %d io_errors accessing data via %s\n", errors, io_errors, use_read_dma?"DMA read from PL to PS":"CPU read from PL");
	return 0;
}

int check_histogram_tpg_data(int path, int card, u_int32_t *rbuf, int mode, int words_one_loop, int num_loops, int num_streams, int stream_stride, int use_dma)
{
	int errors=0, io_errors=0;
	int rc;
	int i;
	int stream;
	u_int64_t actual_totals[AXI_HIST_MAX_STREAMS];
	u_int64_t actual_all=0, expected_all=0;
	int errors_this_stream;
	int read_pass;
	int words_remaining, num_words;
	int offset;
	int expected;

	for (stream=0; stream<num_streams; stream++)
	{
		read_pass = 0;
		do  
		{
			actual_totals[stream] = 0;
			errors_this_stream = 0;
			words_remaining = words_one_loop+1;
			if (words_remaining < HIST_BLOCK_SIZE)
				words_remaining = HIST_BLOCK_SIZE;
			offset = 0;
			do
			{
				num_words = words_remaining;
				if (num_words > HIST_BLOCK_SIZE)
					num_words = HIST_BLOCK_SIZE;

				if (use_dma)
				{
					if ((rc=ngzmp_hist_read_stream_dma(path, card, stream, offset, num_words, rbuf, 0)) < 0)
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
					if ((rc=ngzmp_hist_read_stream(path, card, stream, offset, num_words, rbuf)) < 0)
					{
						io_errors++;
						if (io_errors < max_errors)
							mprintf(MP_ERR, "ERROR: zynqmp_read returned error %d\n", rc);
						else if (io_errors == max_errors)
							mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
					}
				}

				for (i=0; i<num_words; i++)
				{
					actual_totals[stream] += rbuf[i];

					if (i+offset == 0 || i+offset > words_one_loop || mode == TEST_CLEAR)
						expected = 0;
					else
						expected = num_loops;
					if (expected != rbuf[i])
					{
						if (errors == 0 && reporting == 0)
						{
							printf("ERROR: Stream=%d, Offset %d=%08X: Expected %08X, Read %08X\n", stream, i, i, expected, rbuf[i]);
						}
						errors++;
						errors_this_stream++;
						if (errors < max_errors)
							mprintf(MP_ERR, "ERROR: Stream=%d, Offset %d=%08X: Expected %08X, Read %08X Difference 0x%08X\n", stream, i, i, expected, rbuf[i], rbuf[i]-expected);
						else if (errors == max_errors)
							mprintf(MP_ERR, "Too many errors. Counting\n");
					}
					else if (reporting >=5)
						printf("Correct: Stream=%d, Offset %d=%08X: Read %08X\n", stream, i, i, rbuf[i]);
				}
				if (errors_this_stream || read_pass >  0)
				{
						printf("Read pass %d.  Stream %d : %d errors\n", read_pass,stream, errors_this_stream);
						read_pass++;
				}
				words_remaining -= num_words;
				offset += num_words;
			} while (words_remaining > 0);
			if (mode == TEST_CLEAR)
			{
				if (actual_totals[stream] != 0)
					mprintf(MP_ERR, "ERROR:   Stream %d testing cleared: Expected 0, received %ld, extra counts=%ld\n", stream,  actual_totals[stream], actual_totals[stream]-(int64_t)words_one_loop*num_loops);
				else if (reporting >= 3)
					mprintf(MP_PRINT, "Correct: Stream %d total: Expected %ld, received %ld\n", stream, (int64_t)0, actual_totals[stream]);
			}
			else
			{
				if (actual_totals[stream] != (int64_t)words_one_loop*num_loops)
					mprintf(MP_ERR, "ERROR:   Stream %d total: Expected %ld, received %ld, extra counts=%ld\n", stream, (int64_t)words_one_loop*num_loops, actual_totals[stream], actual_totals[stream]-(int64_t)words_one_loop*num_loops);
				else if (reporting >= 3)
					mprintf(MP_PRINT, "Correct: Stream %d total: Expected %ld, received %ld\n", stream, (int64_t)words_one_loop*num_loops, actual_totals[stream]);
			}
		} while (errors_this_stream && read_pass == 1);
		actual_all += actual_totals[stream];
		expected_all += (int64_t)words_one_loop*num_loops;
	}

	if (errors != 0 || io_errors != 0)
	{
		mprintf(MP_ERREX, "Found %d errors and %d IO error checking histogram data. Expected total=%ld, actual_total=%ld\n", errors, io_errors, expected_all, actual_all);
		num_io_errors += io_errors;
		num_errors += errors;
	}
	return errors;
}


int run_test_hist_tpg_ramp(int path, int card, int num_pass, char *name, int (*generate_tp)(int , int, u_int32_t *, int, int , int, int ), int use_read_dma)
{
	AXIHistConfig hist_conf;
	int rc;
	u_int32_t *expected[AXI_HIST_MAX_STREAMS];
	int64_t stream_totals[AXI_HIST_MAX_STREAMS];
	u_int32_t *rbuf;
	int i;
	int io_errors = 0;
	int pass;
	int test_pattern_lines, lines_remaining;
	u_int32_t * event_list;
	int tp_line;
	u_int32_t axi_hist_gpio_control=0;
	struct timeval t_start_dma, t_now;
	double t_hist, t_hist2;
	struct timespec delay, remaining;
	int errors=0;
	int stream_stride;
	u_int32_t axi_hist_gpio1_status, axi_hist_gpio2_status;
	u_int64_t hist_busy_time;
	u_int32_t total_events;
	int repeat, num_clears;
	int timeout=0;
	u_int32_t tpg_cont[3], tpg_status;
	int num_words;

	mprintf(1, "Running Histogram test %s\n", name);

	rc = ngzmp_hist_read_config(path, card, &hist_conf);
	if (rc != 0)
	{
		io_errors++;
		printf("Error reading AXI Histogrammer config\n");
	}
	if (hist_conf.NumStreams > AXI_HIST_MAX_STREAMS)
	{
		fprintf(stderr, "Number of streams =%d > AXI_HIST_MAX_STREAMS=%d, please recompile with larger AXI_HIST_MAX_STREAMS\n", hist_conf.NumStreams, AXI_HIST_MAX_STREAMS);
		exit(1);
	}

	if (hist_conf.StreamAtBottom)
		stream_stride = hist_conf.HistWordsPerAXIWord;
	else
		stream_stride = hist_conf.HistWordsPerStream;

	for (i=0; i<hist_conf.NumStreams; i++)
	{
		if ((expected[i]= malloc(sizeof(int)*HIST_BLOCK_SIZE)) == NULL)
		{
			fprintf(stderr, "test_hist_hist: Out of memory allocating blocks for histogram\n");
			exit(1);
		}
	}
	if ((rbuf=malloc(sizeof(u_int32_t)*HIST_BLOCK_SIZE)) == NULL)
	{
		fprintf(stderr, "test_hist_hist: Out of memory allocating read block for histogram\n");
		exit(1);
	}

	if ((event_list  = malloc(EVENT_BLOCK_SIZE*AXI_HIST_TEST_NUM_STREAMS*sizeof(u_int32_t))) == NULL)
	{
		fprintf(stderr, "test_hist_hist: Out of memory allocating blocks for event list\n");
		exit(1);
	}

	for (pass=0; pass<num_pass; pass++)
	{
		mprintf(2, ".... Histogram test %s, pass=%d\n", name, pass);
		for (i=0; i<hist_conf.NumStreams; i++)
		{
			memset(expected[i], 0, sizeof(int)*HIST_BLOCK_SIZE);
			stream_totals[i] = 0;
		}
		test_pattern_lines = 512*1024*1024/8*2;
		total_events = 0;
		mprintf(3, ".... Generating test pattern\n");
		for (tp_line=0, lines_remaining=test_pattern_lines; lines_remaining>0; )
		{
			int num_lines;
			if (lines_remaining > EVENT_BLOCK_SIZE)
				num_lines = EVENT_BLOCK_SIZE;
			else
				num_lines = lines_remaining;
			mprintf(4, "........ Generating %d lines from %d\n", num_lines, tp_line);
			total_events += generate_tp(num_lines, tp_line, event_list, pass, tp_line==0, num_lines==lines_remaining, 0);

			hist_test_pattern_to_expected(event_list, expected, num_lines, hist_conf.NumStreams, stream_totals);
			tp_line += num_lines;
			lines_remaining -= num_lines;
		}
		total_events *= hist_conf.NumStreams/8;
		for (repeat=0; repeat<test_hist_repeat; repeat++)
		{
			mprintf(3, ".... Clearing memory repeat=%d\n", repeat);
			num_clears = 0;
			num_words = hist_conf.StreamAtBottom?hist_conf.AXIWordsPerStream*hist_conf.NumStreams:hist_conf.AXIWordsPerStream; 
			if (ngzmp_hist_clear_start(path, card, 0, num_words, 1) != 0)
			{
				printf("Error staring clear\n");
				io_errors++;
			}
			if (ngzmp_hist_clear_wait(path, card) != 0)
			{
				printf("Error waiting for clear to finish\n");
				io_errors++;
				num_io_errors += io_errors;
				return 0;
			}

			tpg_cont[0] = AXI_HIST_TC_HIST_TPG(AXI_HIST_TPG_RAMP);
			tpg_cont[1] = test_pattern_lines;
			tpg_cont[2] = HIST_BLOCK_SIZE;

			mprintf(3, ".... Starting DMA to send test patterns to histogrammer.\n");
			if (manual_test) { printf("Press RETURN to start DMA repeat=%d\n", repeat); fflush(stdout); getchar(); }
			axi_hist_gpio_control = 0;
			if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x400, 1, &axi_hist_gpio_control)) < 0)
			{
				mprintf(MP_ERR, "ERROR writing to GPIO for hist test DMA\n");
				io_errors++;
			}
			if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, AXI_HIST_TESTPAT_CONT, 3, tpg_cont)) < 0)
			{
				mprintf(MP_ERR, "ERROR writing to GPIO for hist test DMA\n");
				io_errors++;
			}
			axi_hist_gpio_control = NGZMP_HIST_GPIO_ENABLE;

			if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x400, 1, &axi_hist_gpio_control)) < 0)
			{
				mprintf(MP_ERR, "ERROR writing to GPIO for hist test DMA\n");
				io_errors++;
			}

			gettimeofday(&t_start_dma, NULL);
			delay.tv_sec = 0;
			delay.tv_nsec=1E6;

			do
			{
				if ((rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_HIST, AXI_HIST_TEST_STATUS, 1, &tpg_status)) < 0)
				{
					mprintf(MP_ERR, "ERROR Reading status from hist TPG\n");
					io_errors++;
				}
				if (reporting >= 5)
					printf(".... DMA status=%08X\n", tpg_status);
				gettimeofday(&t_now, NULL);
				t_hist = t_now.tv_sec-t_start_dma.tv_sec;
				t_hist += 1E-6*(t_now.tv_usec-t_start_dma.tv_usec);

				if (!(tpg_status & AXI_HIST_TSTATUS_TPG_BUSY))
					break;
				if (test_hist_opt & (TEST_HIST_OPT_EXERCISE | TEST_HIST_OPT_EX_FIXED))
				{
					rc =hist_exercise_clear_and_read(path, card, &hist_conf, rbuf, HIST_BLOCK_SIZE, repeat);
					if (rc >= 0)
						num_clears += rc;
				}
				else
				{
					remaining = delay;
					while (nanosleep(&remaining, &remaining) == EINTR);
				}

			} while (t_hist < 50);

			if (t_hist > 50)
			{
				mprintf(MP_ERR, "Timout waiting for hist test DMA to become idle\n");
				timeout++;
			}
			mprintf(3, "TPG finished after %g seconds\n", t_hist);
			do
			{
				if ((rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x402, 1, &axi_hist_gpio2_status)) < 0)
				{
					mprintf(MP_ERR, "ERROR Reading status from hist GPIO\n");
					io_errors++;
				}
				if (reporting >= 5)
					printf(".... GPIO Hist status=%08X\n", axi_hist_gpio2_status);
				gettimeofday(&t_now, NULL);
				t_hist = t_now.tv_sec-t_start_dma.tv_sec;
				t_hist += 1E-6*(t_now.tv_usec-t_start_dma.tv_usec);

				if ((axi_hist_gpio2_status & (1<<30)) == 0)
					break;
				remaining = delay;
				while (nanosleep(&remaining, &remaining) == EINTR);

			} while (t_hist < 60);
			if (t_hist > 60)
			{
				mprintf(MP_ERR, "Timout waiting for histogramming to finish\n");
				timeout++;
			}
			if (timeout)
				hist_display_progress(path, card);
			if ((rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x400, 1, &axi_hist_gpio1_status)) < 0 || (rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x402, 1, &axi_hist_gpio2_status)) < 0)
			{
				mprintf(MP_ERR, "ERROR Reading status from hist GPIO\n");
				io_errors++;
			}
			hist_busy_time = (((u_int64_t)(axi_hist_gpio2_status&0x3FFFFFFF)) << 32) |  axi_hist_gpio1_status;
			t_hist2 = hist_clk_period*hist_busy_time;
			mprintf(MP_PRINT, "Histogramming finished: Pass=%d, Repeat=%d. Software timer= %g , firmware timer=%g seconds, num_clears=%d => Histrogram rate=%g\n", 
				pass, repeat, t_hist, t_hist2, num_clears, total_events/t_hist2  );
			if (report_file != NULL)
				fprintf(report_file, "TPG-ramp: Rate = %g\n", total_events/t_hist2);
			if (fabs(t_hist-t_hist2) > 0.1+0.005*t_hist)
			{
				mprintf(MP_ERR, "*ERROR* Software and firmware timing differ significantly. Currently assuming input cloc kperiod=%g\n", hist_clk_period);
				mprintf(MP_ERR, "If this is wrong, consisder the hist-clk option to change clk period\n", hist_clk_period);
			}

			errors += check_histogram_data(path, card, rbuf, expected, hist_conf.NumStreams, stream_stride, use_read_dma, stream_totals);
			if (test_hist_write[0] != 0)
				write_histogram_data(path, card, test_hist_write, rbuf, hist_conf.NumStreams, stream_stride, use_read_dma);
			if (test_hist_repeat>1 && errors > 0 && exit_on_error)
			{
				printf("Stopping due to errors\n");
				exit(1);
			}
		}
	}
	for (i=0; i<hist_conf.NumStreams; i++)
		free(expected[i]);
	free(event_list);
	free(rbuf);
	num_io_errors += io_errors;
	if (report_file != NULL)
		fprintf(report_file, "Finished histogram tests %s with %d errors and %d io_errors accessing data via %s\n", name, errors, io_errors, use_read_dma?"DMA read from PL to PS":"CPU read from PL");
	printf("Finished histogram tests %s with %d errors and %d io_errors accessing data via %s\n", name, errors, io_errors, use_read_dma?"DMA read from PL to PS":"CPU read from PL");
	return 0;
}


int run_test_hist_tpg_roll(int path, int card, int num_pass, int use_read_dma)
{
	AXIHistConfig hist_conf;
	int rc;
	u_int32_t *rbuf;
	int io_errors = 0;
	int pass;
	u_int32_t axi_hist_gpio_control=0;
	struct timeval t_start_dma, t_now;
	double t_hist, t_hist2;
	struct timespec delay, remaining;
	int errors=0;
	int stream_stride;
	u_int32_t tpg_status, axi_hist_gpio1_status, axi_hist_gpio2_status;
	u_int64_t hist_busy_time;
	u_int64_t events_per_stream, total_events;
	int repeat;
	int timeout=0;
	int num_clears = 0;
	int tpg_size, tpg_occ;
	int words_one_loop;
	int num_loops;
	u_int32_t tpg_cont[3];
	int num_words;
	int hist_size;
	
	mprintf(1, "Running Histogram tests using Rolling shutter like PRBS random test pattern generator\n");

	rc = ngzmp_hist_read_config(path, card, &hist_conf);
	if (rc != 0)
	{
		io_errors++;
		printf("Error reading AXI Histogrammer config\n");
	}
	if (hist_conf.NumStreams > AXI_HIST_MAX_STREAMS)
	{
		fprintf(stderr, "Number of streams =%d > AXI_HIST_MAX_STREAMS=%d, please recompile with larger AXI_HIST_MAX_STREAMS\n", hist_conf.NumStreams, AXI_HIST_MAX_STREAMS);
		exit(1);
	}
	if ((hist_conf.HistTPGPresent & AXI_HIST_TPG_HAS_ROLLING) == 0)
	{
		mprintf(MP_ERR, "This build does not include the rolling shutter Test pattern generator\n");
		return 0;
	}

	if (hist_conf.StreamAtBottom)
		stream_stride = hist_conf.HistWordsPerAXIWord;
	else
		stream_stride = hist_conf.HistWordsPerStream;

	if ((rbuf=malloc(sizeof(u_int32_t)*HIST_BLOCK_SIZE)) == NULL)
	{
		fprintf(stderr, "test_hist_hist: Out of memory allocating read block for histogram\n");
		exit(1);
	}

	for (pass=0; pass<num_pass; pass++)
	{
		tpg_size=pass%4;
		tpg_occ = (3+pass/4) % 7;
		words_one_loop = (1<<24)-1;
		num_loops = 20000000/words_one_loop;
		if (num_loops < 2)
			num_loops = 2;
		events_per_stream = num_loops*words_one_loop;
		hist_size = AXI_HIST_TPG_ROLL_CALC_SIZE(tpg_size);
		mprintf(2, ".... Histogram test with Roll TPG size code=%d,=> size=%d, Occupancy code=%d, words_per_loop=%d, num_loops=%d\n", tpg_size, hist_size, tpg_occ, words_one_loop, num_loops);
		for (repeat=0; repeat<test_hist_repeat; repeat++)
		{
			mprintf(3, ".... Clearing memory repeat=%d\n", repeat);
			num_clears = 0;
			num_words = hist_conf.StreamAtBottom?hist_conf.AXIWordsPerStream*hist_conf.NumStreams:hist_conf.AXIWordsPerStream; 
			if (ngzmp_hist_clear_start(path, card, 0, num_words, 1) != 0)
			{
				printf("Error staring clear\n");
				io_errors++;
			}
			if (ngzmp_hist_clear_wait(path, card) != 0)
			{
				printf("Error waiting for clear to finish\n");
				return 0;
			}
			if (test_hist_opt & TEST_HIST_OPT_CHECK_CLEARED)
			{
				errors += check_histogram_tpg_data(path, card, rbuf, TEST_CLEAR, words_one_loop, num_loops, hist_conf.NumStreams, stream_stride, use_read_dma);
			}
			mprintf(3, ".... Preparing test pattern generator size code=%d, hist_size=%d,  word_one_loop=%d, num_loops=%d\n", tpg_size, hist_size, words_one_loop, num_loops);
			if (manual_test) { printf("Press RETURN to start DMA repeat=%d,\n", repeat); fflush(stdout); getchar(); }

			tpg_cont[0] = AXI_HIST_TC_HIST_TPG(AXI_HIST_TPG_ROLLING);
			tpg_cont[1] = events_per_stream;
			tpg_cont[2] = AXI_HIST_TPG_ROLL_SET_SIZE(tpg_size) | AXI_HIST_TPG_ROLL_SET_OCC(tpg_occ);

			axi_hist_gpio_control = 0;
			if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x400, 1, &axi_hist_gpio_control)) < 0)
			{
				mprintf(MP_ERR, "ERROR writing to GPIO for hist test DMA\n");
				io_errors++;
			}
			if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, AXI_HIST_TESTPAT_CONT, 3, tpg_cont)) < 0)
			{
				mprintf(MP_ERR, "ERROR writing to GPIO for hist test DMA\n");
				io_errors++;
			}

			axi_hist_gpio_control = NGZMP_HIST_GPIO_ENABLE; // Using internal TPG should not need external test data 
			if ((rc=zynqmp_write(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x400, 1, &axi_hist_gpio_control)) < 0)
			{
				mprintf(MP_ERR, "ERROR writing to GPIO for hist test DMA\n");
				io_errors++;
			}

			gettimeofday(&t_start_dma, NULL);
			delay.tv_sec = 0;
			delay.tv_nsec=1E6;
			do
			{
				if ((rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_HIST, AXI_HIST_TEST_STATUS, 1, &tpg_status)) < 0)
				{
					mprintf(MP_ERR, "ERROR Reading status from hist TPG\n");
					io_errors++;
				}
				if (reporting >= 5)
					printf(".... DMA status=%08X\n", tpg_status);
				gettimeofday(&t_now, NULL);
				t_hist = t_now.tv_sec-t_start_dma.tv_sec;
				t_hist += 1E-6*(t_now.tv_usec-t_start_dma.tv_usec);

				if (!(tpg_status & AXI_HIST_TSTATUS_TPG_BUSY))
					break;
				if (test_hist_opt & (TEST_HIST_OPT_EXERCISE | TEST_HIST_OPT_EX_FIXED))
				{
					rc =hist_exercise_clear_and_read(path, card, &hist_conf, rbuf, HIST_BLOCK_SIZE, repeat);
					if (rc >= 0)
						num_clears += rc;
				}
				else
				{
					remaining = delay;
					while (nanosleep(&remaining, &remaining) == EINTR);
				}

			} while (t_hist < 150);
			if (t_hist > 150)
			{
				mprintf(MP_ERR, "Timout waiting for hist test pattern generator to become idle\n");
				timeout++;
			}
			mprintf(3, "TPG finished after %g seconds\n", t_hist);
			do
			{
				if ((rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x402, 1, &axi_hist_gpio2_status)) < 0)
				{
					mprintf(MP_ERR, "ERROR Reading status from hist GPIO\n");
					io_errors++;
				}
				if (reporting >= 5)
					printf(".... GPIO Hist status=%08X\n", axi_hist_gpio2_status);
				gettimeofday(&t_now, NULL);
				t_hist = t_now.tv_sec-t_start_dma.tv_sec;
				t_hist += 1E-6*(t_now.tv_usec-t_start_dma.tv_usec);

				if ((axi_hist_gpio2_status & (1<<30)) == 0)
					break;
				remaining = delay;
				while (nanosleep(&remaining, &remaining) == EINTR);

			} while (t_hist < 200);
			if (t_hist > 200)
			{
				mprintf(MP_ERR, "Timout waiting for histogramming to finish\n");
				timeout++;
			}
			if ((rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x400, 1, &axi_hist_gpio1_status)) < 0 || (rc=zynqmp_read(path, card, ZYNQMP_ADDR_SPACE_HIST, 0x402, 1, &axi_hist_gpio2_status)) < 0)
			{
				mprintf(MP_ERR, "ERROR Reading status from hist GPIO\n");
				io_errors++;
			}
			hist_busy_time = (((u_int64_t)(axi_hist_gpio2_status&0x3FFFFFFF)) << 32) |  axi_hist_gpio1_status;
			t_hist2 = hist_clk_period*hist_busy_time;
			total_events = events_per_stream;
			total_events *= hist_conf.NumStreams;
			mprintf(MP_PRINT, "Histogramming finished: Pass=%d, Repeat=%d. Software timer= %g , firmware timer=%g seconds, num_clears=%d => Histrogram rate=%g\n", 
				pass, repeat, t_hist, t_hist2, num_clears, total_events/t_hist2  );
			if (report_file)
				fprintf(report_file, "TPG-Roll: size code=%d => size=%d, Occupancy code=%d, Rate = %g\n", tpg_size, hist_size, tpg_occ, total_events/t_hist2);
			if (fabs(t_hist-t_hist2) > 0.1+0.005*t_hist)
			{
				mprintf(MP_ERR, "*ERROR* Software and firmware timing differ significantly. Currently assuming input cloc kperiod=%g\n", hist_clk_period);
				mprintf(MP_ERR, "If this is wrong, consisder the hist-clk option to change clk period\n", hist_clk_period);
			}

			errors += check_histogram_roll_tpg_data(path, card, rbuf, hist_conf.NumStreams, hist_size, tpg_size, tpg_occ, events_per_stream, use_read_dma);
			if (test_hist_write[0] != 0)
				write_histogram_data(path, card, test_hist_write, rbuf, hist_conf.NumStreams, stream_stride, use_read_dma);
			if (test_hist_repeat>1 && errors > 0 && exit_on_error)
			{
				printf("Stopping due to errors\n");
				exit(1);
			}
		}
	}
	free(rbuf);
	num_io_errors += io_errors;
	if (report_file != NULL)
		fprintf(report_file, "Finished histogram tests using ROLLING TPG with %d errors and %d io_errors accessing data via %s\n", errors, io_errors, use_read_dma?"DMA read from PL to PS":"CPU read from PL");
	printf("Finished histogram tests using ROLLING TPG with %d errors and %d io_errors accessing data via %s\n", errors, io_errors, use_read_dma?"DMA read from PL to PS":"CPU read from PL");
	return 0;
}

int check_histogram_roll_tpg_data(int path, int card, u_int32_t *rbuf, int num_streams, int hist_size, int size_code, int occ_code, int events_per_stream, int use_dma)
{
	int errors=0, io_errors=0;
	int rc;
	int i;
	int stream;
	u_int64_t actual_totals[AXI_HIST_MAX_STREAMS];
	u_int64_t actual_all=0, expected_all=0;
	int errors_this_stream;
	int read_pass;
	int words_remaining, num_words;
	int offset;
	int expected, *exp_base;
	
	exp_base = (int *) malloc(hist_size*AXI_HIST_TPG_ROLL_NPIXELS*sizeof(int));
	if (exp_base == NULL)
	{
		mprintf(MP_ERR, "Out of memory.. continuining without full checking\n");
	}
	mprintf(3, "... Checking roll pattern hist_size=%d, size_code=%d, occ_code=%d\n", hist_size, size_code, occ_code);
	for (stream=0; stream<num_streams; stream++)
	{
		cal_expected(stream, exp_base, hist_size, size_code, occ_code, events_per_stream);
		read_pass = 0;
		do  
		{
			actual_totals[stream] = 0;
			errors_this_stream = 0;
			words_remaining = hist_size*AXI_HIST_TPG_ROLL_NPIXELS;
			if (words_remaining < HIST_BLOCK_SIZE)
				words_remaining = HIST_BLOCK_SIZE;
			offset = 0;
			mprintf(4, "... Checking roll pattern stream=%d, hist_size=%d, size_code=%d, occ_code=%d, offset=%d, words_remaining=%d\n", stream, hist_size, size_code, occ_code, offset, words_remaining);
			do
			{
				num_words = words_remaining;
				if (num_words > HIST_BLOCK_SIZE)
					num_words = HIST_BLOCK_SIZE;
				if (use_dma)
				{
					if ((rc=ngzmp_hist_read_stream_dma(path, card, stream, offset, num_words, rbuf, 0)) < 0)
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
					if ((rc=ngzmp_hist_read_stream(path, card, stream, offset, num_words, rbuf)) < 0)
					{
						io_errors++;
						if (io_errors < max_errors)
							mprintf(MP_ERR, "ERROR: zynqmp_read returned error %d\n", rc);
						else if (io_errors == max_errors)
							mprintf(MP_ERR, "Too many IO ERRORS: Counting\n");
					}
				}

				for (i=0; i<num_words; i++)
				{
					actual_totals[stream] += rbuf[i];

					if (exp_base != NULL)
					{
						if (i+offset >= hist_size*AXI_HIST_TPG_ROLL_NPIXELS)
							expected = 0;
						else
							expected = exp_base[i+offset];
						if (expected != rbuf[i])
						{
							if (errors == 0 && reporting == 0)
							{
								printf("ERROR: Stream=%d, Offset %d=%08X: Expected %08X, Read %08X\n", stream, i, i, expected, rbuf[i]);
							}
							errors++;
							errors_this_stream++;
							if (errors < max_errors)
								mprintf(MP_ERR, "ERROR: Stream=%d, Offset %d=%08X: Expected %08X, Read %08X Difference 0x%08X\n", stream, i+offset, i+offset, expected, rbuf[i], rbuf[i]-expected);
							else if (errors == max_errors)
								mprintf(MP_ERR, "Too many errors. Counting\n");
						}
						else if (reporting >=5)
							printf("Correct: Stream=%d, Offset %d=%08X: Read %08X\n", stream, i, i, rbuf[i]);
					}
				}
				if (errors_this_stream || read_pass >  0)
				{
						printf("Read pass %d.  Stream %d : %d errors\n", read_pass,stream, errors_this_stream);
						read_pass++;
				}
				words_remaining -= num_words;
				offset += num_words;
			} while (words_remaining > 0);
			if (actual_totals[stream] != (int64_t)events_per_stream)
				mprintf(MP_ERR, "ERROR:   Stream %d total: Expected %ld, received %ld, extra counts=%ld\n", stream, (int64_t)events_per_stream, actual_totals[stream], actual_totals[stream]-(int64_t)events_per_stream);
			else if (reporting >= 3)
				mprintf(MP_PRINT, "Correct: Stream %d total: Expected %ld, received %ld\n", stream, (int64_t)events_per_stream, actual_totals[stream]);
		} while (errors_this_stream && read_pass == 1);
		actual_all += actual_totals[stream];
		expected_all += (int64_t)events_per_stream;
	}

	if (errors != 0 || io_errors != 0)
	{
		mprintf(MP_ERREX, "Found %d errors and %d IO error checking histogram data. Expected total=%ld, actual_total=%ld\n", errors, io_errors, expected_all, actual_all);
		num_io_errors += io_errors;
		num_errors += errors;
	}
	free (exp_base);
	return errors;
}

int cal_expected(int stream, int *exp_base, int hist_size, int size_code, int occ_code, int events_per_stream)
{
	int prbs;
	int tp_stream = stream % AXI_HIST_TEST_NUM_STREAMS;
	int i, j;
	int pixel=0, eng;
	int inc_mask, eng_mask, pixel_shift;
	int feedback, offset;
	
	if (exp_base == NULL)
		return -1;
	inc_mask = 0xFF >> occ_code;
	if (inc_mask < 3)
		inc_mask = 3;
	eng_mask = 16<< (2*size_code);
	eng_mask --;
	pixel_shift = 2*size_code+6;
	prbs = 1 + tp_stream;
	memset(exp_base, 0, hist_size*AXI_HIST_TPG_ROLL_NPIXELS*sizeof(int));
	for (i=0; i<events_per_stream; i++)
	{
		pixel += ((prbs >> 16) & inc_mask)+1; 
		pixel &= 0xFF;
		eng = 0;
		for (j=0;j<4; j++)
		{
			eng += prbs & eng_mask;
			feedback = ((prbs >> 16) & 1) ^ ((prbs >> 21) & 1) ^ ((prbs >> 22) & 1) ^ ((prbs >> 23) & 1);
			prbs = 0xFFFFFF & ((prbs << 1) | feedback);
		}
		offset = eng + (pixel << pixel_shift);
		//printf("%dprbs=0x%06X\teng=%d\tpixel=%d\toffset=%d\n", i, prbs, eng, pixel, offset);
		exp_base[offset]++;
	}
/*	for (i=0; i<hist_size; i++)
		printf("%d\t%d\n", i, exp_base[i]);
	 */
	return 0;
}

int optimise_for_substreams(u_int32_t *event_list, int num_lines, AXIHistConfig *hist_conf)
{
	int i, j, stream;
	int repeat;
	u_int32_t addr_mask, current, *base, *ptr;
	int num_sub_streams = 1 << hist_conf->NBitsSubStream;
	int hist_words = hist_conf->HistWordsPerAXIWord;
	
	addr_mask = AXI_HIST_TEST_GET_OFFSET(0xFFFFFFFF);
	
	i=1;
	while (hist_words > 1)
	{
		addr_mask &= ~i;
		i<<=1; 
		hist_words >>= 1;
	}

//	printf("Determined Addr_mask = 0x%08X\n", addr_mask);
	
	if (num_sub_streams <= 1) 
		return 0;

	for (stream=0; stream<AXI_HIST_TEST_NUM_STREAMS; stream++)
		event_list[stream] |= AXI_HIST_TEST_FIRST;
	
	for (i=1; i<num_sub_streams; i++)
	{
		for (stream=0; stream<AXI_HIST_TEST_NUM_STREAMS; stream++)
		{
			repeat = 0;
			base = event_list+stream+i*AXI_HIST_TEST_NUM_STREAMS;
			current = *base & addr_mask;
			ptr = base-AXI_HIST_TEST_NUM_STREAMS;
			for (j=1; j<num_sub_streams && i-j >= 0; j++)
			{
				if (current == (*ptr & addr_mask))
				{
					repeat = 1;
					break;
				}
				ptr -= AXI_HIST_TEST_NUM_STREAMS;
			}
			if (repeat)
				*base |= AXI_HIST_TEST_FIRST;
			else
				*base &= ~AXI_HIST_TEST_FIRST;
		}
	}
	for (i=num_sub_streams; i<num_lines; i++)
	{
		for (stream=0; stream<AXI_HIST_TEST_NUM_STREAMS; stream++)
		{
			repeat = 0;
			base = event_list+stream+i*AXI_HIST_TEST_NUM_STREAMS;
			current = *base & addr_mask;
			ptr = base-AXI_HIST_TEST_NUM_STREAMS;
			for (j=1; j<num_sub_streams; j++)
			{
				if (current == (*ptr & addr_mask))
				{
					repeat = 1;
					break;
				}
				ptr -= AXI_HIST_TEST_NUM_STREAMS;
			}
			if (repeat)
				*base |= AXI_HIST_TEST_FIRST;
			else
				*base &= ~AXI_HIST_TEST_FIRST;
		}
	}
	return 0;
}
