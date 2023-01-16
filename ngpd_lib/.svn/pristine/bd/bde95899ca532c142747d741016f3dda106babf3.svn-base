/*

	Neutron Gamma Pulse Discriminator in Zynq MP Test Program
	By William Helsby

	Command line arguments for look-up test program. Uses the separate 'args'
	module.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <sys/types.h>
#include <os9types.h>
#include "errors.h"
#include "args.h"

#include "ngzmp.h"
#include "ngzmptest.h"

int nr_mandatory_arguments = 0;
const char *author = "W.I. Helsby";
const char *title = "Neutron Gamma Pulse Discriminator in Zynq MP Test Program";
extern int num_devices;
int num_devices;

/*----- private function prototypes -----*/

static int set_dev (char *), set_loops (char *), set_logfile (char *), set_reporting (char *);
static int set_poll_send(char *str);
static int set_hist_fwhm(char *str);

/*----- set functions for option flags -----*/

#define SET(attr,code) 	static int set_##attr (char *s) { if (!s) code else return 1; return 0; }
#define SETVAL(attr,var) 	static int set_##attr (char *s) { if (!s) return 1; else sscanf (s, "%d", &(var)); return 0; }
#define SETSTR(attr,var) 	static int set_##attr (char *s) { if (!s) return 1; else strcpy(var, s); return 0; }
#define SETDVAL(attr,var) 	static int set_##attr (char *s) { if (!s) return 1; else sscanf (s, "%lg", &(var)); return 0; }

SET (all, test_regs = test_bram = 0xFFFFFFFF;)
static int set_hist_auto(char * s)
{
	if (!s)
	{
		test_hist_auto = 1;
		test_hist_rw = TEST_HIST_REGS_READ;
		test_hist_hist = TEST_HIST_HIST_RAMP | TEST_HIST_HIST_RANDOM | TEST_HIST_HIST_RAMP_TPG | TEST_HIST_HIST_RAND_TPG | TEST_HIST_HIST_ROLL_TPG;
		return 0;
	}
	else
		return 1;
}

	
	SET (regs, test_regs = 0xFFFFFFFF;)
		SET (regs_chan, test_regs |= TEST_REGS_CHAN_REGS;)
		SET (regs_glob, test_regs |= TEST_REGS_GLOB_REGS;)
		SET (regs_filt, test_regs |= TEST_REGS_FILTER;)
//		SET (regs_dma,  test_regs |= TEST_REGS_DMA_REGS;)
	SET (bram, test_bram = 0xFFFFFFF;)
		SET (bram_width, test_bram |= TEST_BRAM_WIDTH;)
		SET (bram_mem,   test_bram |= TEST_BRAM_MEMORY;)

		SET(rdma_read_lut, test_regs |= TEST_REGS_READ_RDMA_LUT;)

	SET (regs_hist, test_hist_rw |= TEST_HIST_REGS_READ|TEST_HIST_REGS_RW;)
		SET (regs_hist_read, test_hist_rw |= TEST_HIST_REGS_READ;)
		SET (regs_hist_rw, test_hist_rw |= TEST_HIST_REGS_RW;)


		SET (hist_mem_rw, test_hist_rw |= TEST_HIST_MEM_RW;)
		SET (hist_mem_dma, test_hist_rw |= TEST_HIST_MEM_DMA;)
		SET (hist_mem_tpg_dma, test_hist_rw |= TEST_HIST_MEM_TPG_DMA;)
		SET (hist_mem_small, test_hist_rw |= TEST_HIST_MEM_SMALL;)
			SET (hist_mem_small_dma, test_hist_rw |= TEST_HIST_MEM_SMALL_DMA;)
			SET (hist_mem_small_region, test_hist_rw |= TEST_HIST_MEM_SMALL_REGION;)

		SET (hist_tp_mem_rw, test_hist_rw |= TEST_HIST_TP_MEM_RW;)
		SET (hist_pb_mem_rw, test_hist_rw |= TEST_HIST_PB_MEM_RW;)
		SET (hist_rbuf_mem_rw, test_hist_rw |= TEST_HIST_RBUF_MEM_RW;)

		SET (hist_mem_stream, test_hist_rw |= TEST_HIST_MEM_STREAM;)
		SET (hist_mem_row_len, test_hist_rw |= TEST_HIST_MEM_ROW_LEN;)


		SET (hist_clear, test_hist_hist |= TEST_HIST_CLEAR;)
		
			SET (hist_clear_imm, test_hist_hist |= TEST_HIST_CLEAR_CHECK_IMM;)
			SET (hist_clear_ind, test_hist_hist |= TEST_HIST_CLEAR_IND_STREAM;)
			SET (hist_clear_mult, test_hist_hist |= TEST_HIST_CLEAR_MULT_STREAM;)

		SET (hist_hist, test_hist_hist |= TEST_HIST_HIST; )
			SET (hist_ramp, test_hist_hist |= TEST_HIST_HIST_RAMP; )
			SET (hist_random, test_hist_hist |= TEST_HIST_HIST_RANDOM; )
			SET (hist_ramp_rhp, test_hist_hist |= TEST_HIST_HIST_RAMP_RHP; )
			SET (hist_ramp_rh2, test_hist_hist |= TEST_HIST_HIST_RAMP_RH2; )
		SETVAL (hist_rand_size, hist_rand_size)
		SET (hist_xspress4, test_hist_hist |= TEST_HIST_HIST_XSPRESS4; )
			SET (hist_1us,  test_hist_hist |= TEST_HIST_HIST_XSPRESS1US; )
			SET (hist_10us,  test_hist_hist |= TEST_HIST_HIST_XSPRESS10US; )
			SET (hist_1ms,   test_hist_hist |= TEST_HIST_HIST_XSPRESS1MS;)
			SET (hist_100ms, test_hist_hist |= TEST_HIST_HIST_XSPRESS100MS;)

			SET (hist_mn, test_hist_hist |= TEST_HIST_HIST_XSPRESSMN;)
			SET (hist_zr, test_hist_hist |= TEST_HIST_HIST_XSPRESSZR;)
			SET (hist_mix, test_hist_hist |= TEST_HIST_HIST_XSPRESSMIX;)
		SET (hist_hist_tpg, test_hist_hist |= TEST_HIST_HIST_TPG; )
			SET (hist_ramp_tpg, test_hist_hist |= TEST_HIST_HIST_RAMP_TPG; )
			SET (hist_rand_tpg, test_hist_hist |= TEST_HIST_HIST_RAND_TPG; )
			SET (hist_roll_tpg, test_hist_hist |= TEST_HIST_HIST_ROLL_TPG; )
			
		SET (hist_exercise,	test_hist_opt |= TEST_HIST_OPT_EXERCISE; )
			SET (hist_ex_clear,	test_hist_opt |= TEST_HIST_OPT_EX_CLEAR; )
			SET (hist_ex_read,	test_hist_opt |= TEST_HIST_OPT_EX_READ; )

			SET (hist_ex_long_clear,	test_hist_opt |= TEST_HIST_OPT_EX_LONG_CLEAR; )
			SET (hist_ex_long_read,		test_hist_opt |= TEST_HIST_OPT_EX_LONG_READ; )
			SET (hist_ex_short_clear,	test_hist_opt |= TEST_HIST_OPT_EX_SHORT_CLEAR; )
			SET (hist_ex_short_read,	test_hist_opt |= TEST_HIST_OPT_EX_SHORT_READ; )
			SET (hist_ex_short_discard,	test_hist_opt |= TEST_HIST_OPT_EX_SHORT_DISCARD; )
			SET (hist_ex_long_discard,	test_hist_opt |= TEST_HIST_OPT_EX_LONG_DISCARD; )

		SET (hist_thin,	test_hist_opt |= TEST_HIST_OPT_THIN; )
		SET (hist_check_cleared,	test_hist_opt |= TEST_HIST_OPT_CHECK_CLEARED; )
		SET (sub_streams, test_hist_opt |= TEST_HIST_OPT_SUBSTREAMS; )
		
		SET (hist_delays,	test_hist_opt |= TEST_HIST_OPT_DELAYS; )
		SETDVAL (hist_background, xspress4_tp.background)
	
		SET (hist_cpu_read, test_hist_opt |= TEST_HIST_OPT_CPU_READ; )
	SET (readout_mode, test_readout_mode = 0xFFFFFFFF;)

	SET (speed, test_speed=0xFFFFFFFF;)
		SET(speed_regs, test_speed |= TEST_SPEED_REGS;)
		SET(speed_bram, test_speed |= TEST_SPEED_BRAM;)
		SET(speed_pb_write, test_speed |= TEST_SPEED_PB_WRITE;)
		SET(speed_scope_read, test_speed |= TEST_SPEED_SCOPE_READ;)
		SET(speed_hist_mem_rw,   test_speed |= TEST_SPEED_HIST_MEM_RW;)
		
	SET (dma_buf_wr_rd,		test_dma_buff |= TEST_DMA_BUF_WR_RD; )

	SET(dma, test_dma = 0xFFFFFFFF;)
		SET(dma_fill, test_dma |= TEST_DMA_FILL_ALL;)
		SET(dma_vary, test_dma |= TEST_DMA_VARY_LEN;)
		SET(dma_scope_tp, test_dma |= TEST_DMA_SCOPE_TP;)
		SET(dma_scope_pb, test_dma |= TEST_DMA_SCOPE_PB;)

	SET (prbs_pn9, test_prbs |= TEST_PRBS_PN9; )
	
	SET(manual, manual_test = 1;)
	SET(count, dont_print_errors=1;)
	SET (reset_phy, test_reset |= TEST_RESET_PHY;)
	
	SET(10g_tx,			test_10g_tx |= 0xFFFFFFFF;)
	SET(10g_tx_scope,	test_10g_tx |= TEST_10G_TX_SCOPE;)
	SET(10g_tx_debug,	test_10g_tx |= TEST_10G_TX_DEBUG;)

	SET(gen_scope_inp, test_gen_scope = TEST_GEN_SCOPE_INP;)
	SET(gen_scope_all_dig, test_gen_scope = TEST_GEN_SCOPE_ALL_DIG;)
	SET(gen_scope_mixed, test_gen_scope = TEST_GEN_SCOPE_MIXED;)

	SET(spi, test_spi = 0xFFFFFFFF;)
		SET(spi_dga, test_spi |= TEST_SPI_DGA;)
		SET(spi_adc, test_spi |= TEST_SPI_ADC;)
			SET(spi_adc_glob, test_spi |= TEST_SPI_ADC_GLOB;)

		SET(spi_clk, test_spi |= TEST_SPI_CLK;)
	SET(i2c, test_i2c = 0xFFFFFFFF;)
		SET(i2c_offset_dac, test_i2c |= TEST_I2C_OFFSET_DAC; )
		SET(i2c_preamp_adt7410, test_i2c |= TEST_I2C_PREAMP_ADT7410;)
		SET(i2c_adc_adt7410, 	test_i2c |= TEST_I2C_ADC_ADT7410;)
		SET(i2c_psu_pexp, 		test_i2c |= TEST_I2C_PSU_PEXP; )
		SET(i2c_sfp_pexp, 		test_i2c |= TEST_I2C_SFP_PEXP; )
		SET(i2c_lmk61e2, 		test_i2c |= TEST_I2C_CLK_LMK61E2; )
		SET(i2c_si5324, 		test_i2c |= TEST_I2C_ESSCLK_SI5324; )
		SET(i2c_enclustra, 		test_i2c |= TEST_I2C_ENCLUSTRA; )
		SET(i2c_ucd90160,		test_i2c |= TEST_I2C_UCD90160; )
	SET (list_sys_mon, 			test_sys_mons |= TEST_SYS_MON_LIST; )
	SET(read_hist, test_read_hist = 0xFFFFFFFF;)
		SET(read3d, test_read_hist |= TEST_READ_HIST_3D;)
		SET(read4d, test_read_hist |= TEST_READ_HIST_4D;)
		SET(read_chan, test_read_hist |= TEST_READ_HIST_CHAN;)

	SETVAL(hist_first_pass, test_hist_first_pass)

//	SET(clear_hist, test_clear_hist = 0xFFFFFFFF;)
		SET(clear_all, test_clear_hist |= TEST_CLEAR_HIST_ALL;)
		SET(clear_part, test_clear_hist |= TEST_CLEAR_HIST_PART;)

	SET (hist_speed, test_hist_speed = 0xFFFFFFFF;)
	SET (scaler_read,  test_scaler_read = 0xFFFFFFFF;)

	SETVAL(pack_gap, override_packet_gap)
	SETVAL(seed, test_pat_seed)
	SETVAL(pack_size, packet_size)

	SETVAL(hist_repeat, test_hist_repeat)
	SETVAL(test_10g_tx_debug_opt, test_10g_tx_debug_opt)
	SETVAL(hist_events, limit_tp_lines)
	
	SET(ignore_psu_ok, test_ignore_psu_ok = 1;)

	SET (hist_clk250, hist_clk_period = 4E-9;)
	SET (hist_clk300, hist_clk_period = 3.333E-9; )

SET(axi_c2c_reset, test_axi_c2c |= TEST_AXI_C2C_RESET;)

SET (sys_mon, test_sys_mon = 1;)

SET (noexit, exit_on_error = 0;)
SET (verbose, reporting = 2;)
SET (quiet, reporting = 0;)
SETVAL(loop_reset, loop_reset)
SETSTR (hist_write, test_hist_write) 
SETSTR(report_name, test_report_fname)

/*----- list of options and arguments for 'args' module -----*/

optlist_t optlist[] =
{
/*-- Switch on all the available tests --*/

{	"all", 'a',			"Perform all the tests",				set_all },

/*-- Register tests --*/

{	"registers", 'r',	"Test all the registers",										set_regs },
{	"regs-chan", 0,		".. Test write and read from channel registers",				set_regs_chan },
{	"regs-glob", 0,		".. Test write and read from global register",					set_regs_glob },
{	"regs-filt", 0,		".. Test write of filter",										set_regs_filt },
//{	"regs-dma", 0,		".. Test write/read into DMA registers",						set_regs_dma },
{	"bram",	'b',		"Test BRAM read/write and width",								set_bram  },
{	"bram-width",	0,	"...Test BRAM width",											set_bram_width  },
{	"bram-mem",	0,		"...Test BRAM write/read",										set_bram_mem  },
//{	"limit-bram",	0,	".. Limit size of buffer used for BRAM test to trigger for Chipscope", set_limit_bram },

{	"rdma-read-lut", 0, ".. Read RDMA Farm Mode LUT",									set_rdma_read_lut },
{	"readout-mode", 0,	"Test simple set and get of readout mode",						set_readout_mode },

{	"hist-auto",	0, 	"Run comprehensive test on histogrammer, depending on build",	set_hist_auto },
{	"hist-regs", 0,		".. Test registers in histogram block",							set_regs_hist },
{	"hist-regs-rd", 0,	".... Read fixed registers in histogram block",					set_regs_hist_read },
{	"hist-regs-rw", 0,	".... Read/Write registers in histogram block",					set_regs_hist_rw },

{	"hist-pb-rw", 0,	".. Test read/write Playback memory (ZynqMP PS)",				set_hist_pb_mem_rw },
{	"hist-rbuf-rw", 0,	".. Test read/write Hist read buff memory (ZynqMP PS)",			set_hist_rbuf_mem_rw },
{	"hist-tp-rw", 0,	".. Test read/write to Hist test pattern memory (ZynqMP PS)",	set_hist_tp_mem_rw },
{	"hist-mem-rw", 0,	".. Read/Write PL DRAM used by histogram block",				set_hist_mem_rw },
{	"hist-mem-dma", 0,	".. Write then read by DMA histogram buffer",					set_hist_mem_dma },
{	"hist-mem-tpg", 0,	".. TP gen write then read by DMA histogram buffer",			set_hist_mem_tpg_dma },
{	"hist-mem-small", 0,".. Write then read of small and unaligned sections  ",			set_hist_mem_small },
{	"hist-mem-small-dma", 0,".... Write then read of small and unaligned parts of DMA buffers ",	set_hist_mem_small_dma },
{	"hist-mem-small-region", 0,".... Write then read of small and unaligned part of PS addr space ",	set_hist_mem_small_region },
{	"hist-mem-small-dma", 0,".. Write then read of small regions ",							set_hist_mem_small },
{	"hist-mem-stream",	0,	".. Hist memory read by stream",							set_hist_mem_stream },
{	"hist-row-len",		0,	".. Exercise many cases of row length and row stride",		set_hist_mem_row_len },


{	"hist-clear", 		0, 	".. Clear tests on  histogram block",							set_hist_clear },
{	"hist-clear-imm", 	0, 	".... Clear tests check immediately after clear",				set_hist_clear_imm },
{	"hist-clear-ind", 	0,	".... Clear tests on individual streams in histogram block",	set_hist_clear_ind },
{	"hist-clear-mult", 	0,	".... Clear tests on all streams at same time ",				set_hist_clear_mult },
{	"hist", 			0,	".. All Histogram tests on hist block",							set_hist_hist },
{	"hist-ramp", 		0,	".... Histogram tests using ramping locations",					set_hist_ramp },
{	"hist-ramp-rhp", 	0,	".... using ramping locations with extra row hammer protection",	set_hist_ramp_rhp },
{	"hist-ramp-rh2", 	0,	".... using ramping locations to force row hammer ",		set_hist_ramp_rh2 },
{	"hist-random", 		0,	".... Histogram tests uniformly randomly distributed locations",set_hist_random },
{	"hist-xspress4", 	0,	".... All Histogram test making simulated Xspress4 spectra",	set_hist_xspress4 },
{	"hist-1us", 		0,	"......  Simulate   1 us time frames (1 events/frame)",			set_hist_1us },
{	"hist-10us", 		0,	"......  Simulate  10 us time frames (10 events/frame)",		set_hist_10us },
{	"hist-1ms", 		0,	"......  Simulate   1 ms time frames (1000 events/frame)",		set_hist_1ms },
{	"hist-100ms", 		0,	"......  Simulate 100 ms time frames (100,000 events/frame)",	set_hist_100ms },
{ 	"hist-exercise",	0,	"...... Exercise clears and reads while histograming",			set_hist_exercise },
{ 	"hist-ex-clear",	0,	"........ Exercise clears while histograming",					set_hist_ex_clear },
{ 	"hist-ex-read", 	0,	"........ Exercise reads while histograming",					set_hist_ex_read },
{ 	"hist-long-clear",	0,"........ Exercise LONG clears while histograming",				set_hist_ex_long_clear },
{ 	"hist-long-read", 	0,"........ Exercise LONG reads while histograming",				set_hist_ex_long_read },
{ 	"hist-short-clear",	0,"........ Exercise short clears while histograming",			set_hist_ex_short_clear },
{ 	"hist-short-read", 	0,"........ Exercise short reads while histograming",			set_hist_ex_short_read },
{ 	"hist-long-discard", 0,"........ Exercise long reads but discard data",				set_hist_ex_long_discard },
{ 	"hist-short-discard", 0,"........ Exercise short reads but discard data",			set_hist_ex_long_discard },
{	"hist-delays", 		0,	"...... Add delays to histogram stream",						set_hist_delays },
{	"hist-thin",   		0,	"...... Remove HistValid on some streams to thin out data",		set_hist_thin },
{	"hist-check-cleared", 0,	".. Check data has cleared before starting histogram",			set_hist_check_cleared },

{	"hist-mn", 		0,	"......  Simulate Mn Kalpha+kbeta peak",						set_hist_mn },
{	"hist-zr", 		0,	"......  Simulate Zr Kalpha+kbeta peak",						set_hist_zr },
{	"hist-mix", 	0,	"......  Simulate mixture of Fe, Cu, Zn and Mo",				set_hist_mix },
{	"hist-background",0,".... Set background level compared to peak height of 1.0",		set_hist_background },
{	"hist-fwhm",	0,	".... Set FWHM (bins) of peaks",								set_hist_fwhm },
{	"hist-repeat",	0, 	".... Specify number of repeats of Hist test with same test pattern",	set_hist_repeat },
{	"hist-write", 0,	".. Set filename to write out histogrammed data",					set_hist_write },
{	"hist-cpu-read", 0,	". Histogram tests use direct CPU read of PL DRAM (instead of DMA)",set_hist_cpu_read },
{	"hist-tpg",		0,  ".... Histogram tests using hist event test pattern generators",	set_hist_hist_tpg },
{	"hist-ramp-tpg",	0,  "...... Histogram tests using hist event ramp TPG",				set_hist_ramp_tpg },
{	"hist-rand-tpg",	0,  "...... Histogram tests using hist event PRBS TPG",				set_hist_rand_tpg },
{	"hist-roll-tpg",	0,  "...... Histogram tests using hist event Rolling shutter TPG",	set_hist_roll_tpg },
{	"hist-clk-250",		0,  "Histogram input clock use 200 MHz playback, 250 MHz in run mode",	set_hist_clk250 },
{	"hist-clk-300",		0,  ".. Switch to 300 MHz on the histogram firmware tester",		set_hist_clk300 },
{	"hist-events",		0,	".. Limit number of hist events sent",							set_hist_events },
{ 	"hist-sub-streams",	0, 	"Set First of Group signals to optimise use of sub-streams when supported",	set_sub_streams },
{	"hist-rand-size",	0,	".. Limit number of bins used in hist-random test",				set_hist_rand_size }, 
{	"hist-first-pass", 0,	".. Start hist tests at pass >0 (currently hist-rand-tpg only)",set_hist_first_pass },
{	"speed-hist-mem", 0,	"Test read/write speed PL DRAM used by histogram block",		set_speed_hist_mem_rw },

{	"scope-tp",	0,		"Test Scope mode read of scope test pattern generator",				set_dma_scope_tp },
{	"scope-pb",	0,		"Test Scope mode read of playback data",							set_dma_scope_pb },
{	"prbs-pn9",	0,		"Test ADC JESD data 9 bit PRBS to scope mode DMA",					set_prbs_pn9  },
{	"spi",		0,		"Read/Write test to all SPI registers",								set_spi},
{	"spi-dga",	0,		".... Read/Write test to DGA SPI registers",						set_spi_dga},
{	"spi-adc",	0,		".... Read/Write test to ADC SPI registers",						set_spi_adc},
{	"spi-adc-glob",	0,	"...... Read/Write test to ADC Global registers",					set_spi_adc_glob},
{	"spi-clk",	0,		".... Read/Write test to CLK SPI registers",						set_spi_clk},
{	"i2c", 		0,		"Read/Write tests to i2c devices",									set_i2c },
{	"i2c-offset", 0,	".... Read/Write tests to i2c offset DACs on the preamp",			set_i2c_offset_dac },
{	"i2c-preamp-temp",0,".... Read/Write tests ADT7410 on the preamp",						set_i2c_preamp_adt7410 },
{	"i2c-adc-temp", 0,	".... Read/Write tests ADT7410 on the ADC board",					set_i2c_adc_adt7410 },
{	"i2c-psu-pexp", 0,	".... Read/Write tests PSU Port expander",							set_i2c_psu_pexp },
{	"i2c-sfp-pexp", 0,	".... Read/Write tests SFP Port expander",							set_i2c_sfp_pexp },
{	"i2c-lmk61e2", 0,	".... Read tests LMK61E2",											set_i2c_lmk61e2 },
{	"i2c-si5324", 0,	".... Read tests ESS Clk SI5324",									set_i2c_si5324 },
{	"i2c-enclustra", 0,	".... Try to read 2 posible I2C addrs for Enclustar EEPROM",		set_i2c_enclustra },
{	"i2c-ucd90160", 0,	".... Read raw volatges back from the 2 off UCD90160",				set_i2c_ucd90160 },
{	"list-sys-mon",	0,	"List system monitor values to display",							set_list_sys_mon },
/*-- Miscellaneous features --*/
{	"manual", 0, 		"Prompt 'Press RETURN' at various points", set_manual},
{	"count", 0, 		"Do not print errors, just count them", set_count},
{	"loop", 'l',		"Perform all the tests several times",	set_loops },
{	"loop-reset",   0,	"Loop tests including open/close to force reset on MIDAS", set_loop_reset},
{	"noexit", 'n',		"Don't exit if there is an error",		set_noexit },
{	"reporting", 'p',	"Report level, quiet, normal, verbose, debugging", set_reporting },
{	"verbose", 'v',		"Provide verbose messages",				set_verbose },
{	"quiet", 'q',		"Provide minimal messages",				set_quiet },
{	"logfile", 'f',		"Log errors to file (implies -noexit)",	set_logfile },
{	"report-file", 0, 	"File name to save report on test and performance",					set_report_name },
{	NULL, 0,			NULL,									NULL }
};

arglist_t arglist[] =
{
{	"first-card",		"Card index (default 0) of first card to test",			set_dev },
{	NULL,				NULL,									NULL }
};

/*----- initialization function (called by 'args') -----*/

void
init_arguments (void)
{
	/* do nothing */
}

static int set_poll_send(char *str)
{
	if (str == NULL)
		test_poll_send = -1;
	else
		test_poll_send = atoi(str);
	return 0;
}

static int set_hist_fwhm(char *str)
{
	int i, fwhm;
	if (str == NULL)
		printf("-hist-fwhm requires an width in bins\n");
	else
	{
		fwhm=strtol(str, NULL, 0);
		for (i=0; i<AXI_HIST_TP_MAX_PEAKS; i++)
			xspress4_tp.peak[i].fwhm = fwhm;
	}
	return 0;
}


static int
set_loops (char *str)
{
	if (str == NULL)
		nr_loops = 0;		/* loop indefinitely */
	else
		nr_loops = atoi (str);
	return 0;
}


static int
set_logfile (char *str)
{
	FILE *fp;

	if (str == NULL) { fprintf (stderr, "%s: Please supply the name of a log-file.\n", progname); return 1; }
	if (logfile)
		fclose (logfile);

	fp = fopen (str, "w");
	if (fp != NULL)
	{
		logfile = fp;
		exit_on_error = 0;
		return 0;
	}
	else
	{
		fprintf (stderr, "%s: Couldn't open %s.\n", progname, str);
		return 1;
	}
}

static int
set_reporting (char *str)
{
	if (str == NULL)
	{
		fprintf (stderr, "%s: Use `-reporting=?' for help.\n", progname);
		return 1;
	}
	if (strcmp (str, "quiet") == 0)
		reporting = 0;
	else if (strcmp (str, "normal") == 0)
		reporting = 1;
	else if (strcmp (str, "verbose") == 0)
		reporting = 2;
	else if (strcmp (str, "debugging") == 0)
		reporting = 3;
	else if (isdigit (str[0]) && str[1] == '\0')
		reporting = str[0] - '0';
	else
	{
		fprintf (stderr, "%s: Reporting level can be set to 0..3 or `quiet', `normal',\n", progname);
		fprintf (stderr, "    `verbose' or `debugging' for minimal -> maximal messages.\n");
		return 1;
	}
	return 0;
}

static int
set_dev (char *str)
{
	first_card=strtol(str, NULL, 0);
	return 0;
}








