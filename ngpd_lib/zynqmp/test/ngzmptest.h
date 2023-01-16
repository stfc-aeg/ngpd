extern u_int32_t revision;
extern u_int32_t regs_present;
extern int num_chan;
extern int num_cards;
extern int test_dma;
extern int test_dma_buff;

extern unsigned int num_errors, num_io_errors;
extern unsigned int max_errors;
//extern XSP3Path Xsp3Sys[XSP3_MAX_PATH];
extern int test_regs;
extern int test_hist_rw;
extern int test_hist_hist;
extern int test_hist_opt;
extern int test_bram;
extern int limit_bram;
extern int test_speed;
extern int test_reset;
extern int test_10g_tx, override_packet_gap;
extern int test_pat_seed;
extern int test_gen_scope; 
extern int test_read_hist;
extern int test_clear_hist;
extern int test_scaler_read;
extern int test_axi_c2c;
extern int test_ignore_psu_ok;
extern int test_prbs;
extern int test_sys_mons;
extern int hist_rand_size;
extern int test_hist_first_pass;
extern char test_report_fname[];
extern FILE *report_file;
extern int nr_loops;

extern int manual_test, dont_print_errors, loop_reset;
extern char *progname;
extern int packet_size;
extern int test_10g_tx_debug_opt;
extern int test_hist_speed;
extern int first_card;
extern int num_scope_dma;
extern int test_spi;
extern int test_i2c;
extern int test_sys_mon;
extern int test_readout_mode;
extern int test_poll_send;
extern char test_hist_write[];
extern int test_hist_repeat;
extern double hist_clk_period;
extern int limit_tp_lines;
extern int test_hist_auto;

#ifndef NGPD_MAX_ERROR_MESSAGE
#define NGPD_MAX_ERROR_MESSAGE 200
#endif

#define TEST_HIST_REGS_READ			1	
#define TEST_HIST_REGS_RW			2
#define TEST_HIST_MEM_RW			0x10
#define TEST_HIST_TP_MEM_RW			0x20
#define TEST_HIST_PB_MEM_RW			0x40
#define TEST_HIST_RBUF_MEM_RW		0x80
#define TEST_HIST_MEM_STREAM		0x100
#define TEST_HIST_MEM_ROW_LEN		0x200

#define TEST_HIST_MEM_DMA			0x01000000
#define TEST_HIST_MEM_TPG_DMA		0x02000000
#define TEST_HIST_MEM_SMALL			0x30000000
#define TEST_HIST_MEM_SMALL_DMA		0x10000000
#define TEST_HIST_MEM_SMALL_REGION	0x20000000


#define TEST_HIST_CLEAR				0xFF
#define TEST_HIST_CLEAR_CHECK_IMM	0x01
#define TEST_HIST_CLEAR_IND_STREAM	0x02
#define TEST_HIST_CLEAR_MULT_STREAM	0x04

#define TEST_HIST_HIST				0xFFFFFF00

#define TEST_HIST_HIST_RAMP			0x0000100
#define TEST_HIST_HIST_RANDOM		0x0000200
#define TEST_HIST_HIST_XSPRESS1US	0x0000400
#define TEST_HIST_HIST_XSPRESS10US	0x0000800
#define TEST_HIST_HIST_XSPRESS1MS	0x0001000
#define TEST_HIST_HIST_XSPRESS100MS	0x0002000

#define TEST_HIST_HIST_XSPRESSMN	0x0010000
#define TEST_HIST_HIST_XSPRESSZR	0x0020000
#define TEST_HIST_HIST_XSPRESSMIX	0x0040000

#define TEST_HIST_HIST_XSPRESS4		0x007FC00

#define TEST_HIST_HIST_RAMP_RHP		0x0080000
#define TEST_HIST_HIST_RAMP_RH2		0x0100000

#define TEST_HIST_HIST_RAMP_TPG		0x1000000
#define TEST_HIST_HIST_RAND_TPG		0x2000000
#define TEST_HIST_HIST_SUMMED_TPG	0x4000000
#define TEST_HIST_HIST_ROLL_TPG		0x8000000
#define TEST_HIST_HIST_TPG			0xF000000

#define TEST_HIST_OPT_SUBSTREAMS		0x00000001

#define TEST_HIST_OPT_CHECK_CLEARED		0x00020000
#define TEST_HIST_OPT_EX_LONG_DISCARD  	0x00040000
#define TEST_HIST_OPT_EX_SHORT_DISCARD 	0x00080000
#define TEST_HIST_OPT_EX_SHORT_CLEAR  	0x00100000
#define TEST_HIST_OPT_EX_SHORT_READ		0x00200000
#define TEST_HIST_OPT_EX_LONG_CLEAR		0x00400000
#define TEST_HIST_OPT_EX_LONG_READ		0x00800000
#define TEST_HIST_OPT_EX_FIXED			0x00FC0000
#define TEST_HIST_OPT_EX_CLEAR			0x01000000
#define TEST_HIST_OPT_EX_READ			0x02000000
#define TEST_HIST_OPT_EXERCISE			0x03000000
#define TEST_HIST_OPT_THIN				0x08000000
#define TEST_HIST_OPT_DELAYS			0x40000000
#define TEST_HIST_OPT_CPU_READ			0x80000000

#define TEST_SPEED_REGS				1
#define TEST_SPEED_BRAM				2
#define TEST_SPEED_PB_WRITE			4
#define TEST_SPEED_SCOPE_READ		8
#define TEST_SPEED_HIST_MEM_RW 0x10

#define TEST_REGS_CHAN_REGS			1
#define TEST_REGS_GLOB_REGS			2
#define TEST_REGS_FILTER			4

#define TEST_REGS_DMA_REGS			0x10

#define TEST_REGS_READ_RDMA_LUT     0x1000

#define TEST_BRAM_WIDTH				1
#define TEST_BRAM_MEMORY			2

#define TEST_DMA_BUF_WR_RD     		1

#define TEST_RESET_PHY				1

#define TEST_10G_TX_SCOPE			1
#define TEST_10G_TX_DEBUG			2

#define TEST_GEN_SCOPE_INP			1
#define TEST_GEN_SCOPE_ALL_DIG		2
#define TEST_GEN_SCOPE_MIXED		3
		
#define TEST_SPI_DGA				0x000F
#define TEST_SPI_ADC				0x00F0
#define TEST_SPI_ADC_GLOB			0x0010
#define TEST_SPI_CLK				0xF000


#define TEST_READ_HIST_3D			0x1
#define TEST_READ_HIST_4D			0x2
#define TEST_READ_HIST_CHAN			0x4

#define TEST_CLEAR_HIST_ALL			0x1
#define TEST_CLEAR_HIST_PART		0x2
#define TEST_HIST_SPEED				0xFFFF

#define TEST_DMA_FILL_ALL			1
#define TEST_DMA_VARY_LEN			2

#define TEST_DMA_SCOPE_TP			0x10
#define TEST_DMA_SCOPE_PB			0x20

#define TEST_AXI_C2C_RESET			1

#define TEST_I2C_OFFSET_DAC			0x0001
#define TEST_I2C_PREAMP_ADT7410		0x0002
#define TEST_I2C_ADC_ADT7410		0x0004
#define TEST_I2C_PSU_PEXP			0x0008
#define TEST_I2C_SFP_PEXP			0x0010
#define TEST_I2C_CLK_LMK61E2		0x0020
#define TEST_I2C_ESSCLK_SI5324		0x0040
#define TEST_I2C_ENCLUSTRA			0x0080
#define TEST_I2C_UCD90160			0x0100

#define TEST_SYS_MON_LIST				 1


#define TEST_PRBS_PN9				0x0001

int test_hist_read_config(int path, int card);
int test_hist_reg_wr_rd(int path, int card);

int test_hist_memory(int path, int card);
int test_hist_memory_speed(int path, int card);
int test_hist_memory_tpgen_dma(int path, int card);
int test_hist_read_stream(int path, int card);
int test_hist_read_row_len(int path, int card);

int test_hist_clear(int path, int card);
int hist_hist_tests(int path, int card);
int test_hist_tp_memory(int path, int card);
int test_playback_memory(int path, int card);
int test_hist_rbuf_memory(int path, int card);
int test_dma_buff_rw(int path, int card, int stream, int num_streams, char *name);
int test_hist_memory_dma(int path, int card);
int test_dma_buff_rw_small(int path, int card, int stream, int num_streams, char *name);
int test_buff_rw_small(int path, int card, int address_space, size_t total_mem_words, char *name);
int test_hist_default_xspress4();

int test_chan_regs(int path);
int test_glob_regs(int path, int card);
int test_regs_filter(int path);

int test_bram_width(int path);
int test_bram_wrrd(int path);
int dma_tests(int path);
int dma_test_scope_tp(int path, int card, int use_tp, int num_pb_streams);
int i2c_tests(int path);
int prbs_dma_tests(int path);
int sys_mon_tests(int path);

extern AXIHistTPSpectra xspress4_tp;
