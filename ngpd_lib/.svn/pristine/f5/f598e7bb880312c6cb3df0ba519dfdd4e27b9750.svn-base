extern u_int32_t revision;
extern u_int32_t regs_present;
extern int num_chan;
extern int reporting;

extern int num_errors, num_io_errors;
extern int max_errors;
extern int test_regs;
extern int test_bram;
extern int test_spi;
extern int test_dma;
extern int test_speed;
extern int test_reset;
extern int test_i2c;

extern int nr_loops;

extern int manual_test, dont_print_errors, loop_reset;
extern char *progname;
extern int first_card;
extern int num_cards;


#define TEST_REGS_CHAN_REGS			1
#define TEST_REGS_GLOB_REGS			2
#define TEST_REGS_FILTER			4

#define TEST_BRAM_WIDTH				1
#define TEST_BRAM_MEMORY			2
#define TEST_BRAM_MEMORY_XTK		4

#define TEST_DMA_DISPLAY     		1
#define TEST_DMA_READ_SIZE    	 	2
#define TEST_DMA_UPLOAD_SHORT		4
#define TEST_DMA_UPLOAD_FULL		8
#define TEST_DMA_RUN_PLAYBACK		0x10
#define TEST_DMA_READ_SCOPE			0x20
#define TEST_DMA_READ_SCOPE_TP		0x40
#define TEST_DMA_READ_SCOPE_TP_NO_CHUNK		0x80
#define TEST_DMA_READ_SCOPE_TP_FLOW_CONT 	0x100

#define TEST_DMA_SCOPE_TP			0x1000

#define TEST_GEN_SCOPE_INP			1
#define TEST_GEN_SCOPE_ALL_DIG		2
#define TEST_GEN_SCOPE_MIXED		3

/* Note these have to match ../zynqmp.test/ngzmptest.h */	
#define TEST_SPI_DGA				0x000F
#define TEST_SPI_ADC				0x00F0
#define TEST_SPI_ADC_GLOB			0x0010
#define TEST_SPI_CLK				0xF000

#define TEST_I2C_OFFSET_DAC			0x0001
#define TEST_I2C_PREAMP_ADT7410		0x0002
#define TEST_I2C_ADC_ADT7410		0x0004
#define TEST_I2C_PSU_PEXP			0x0008
#define TEST_I2C_SFP_PEXP			0x0010
#define TEST_I2C_CLK_LMK61E2		0x0020
#define TEST_I2C_ESSCLK_SI5324		0x0040
#define TEST_I2C_ENCLUSTRA			0x0080
#define TEST_I2C_UCD90160			0x0100

#define TEST_DMA_FILL_ALL			1
#define TEST_DMA_VARY_LEN			2

int test_bram_wrrd(int path);
int test_bram_width(int path);

int readFixedRegs(int path, int card);
int test_chan_regs(int path);

int test_glob_regs(int path, int card);
int test_regs_filter(int path);


int dma_run_playback(int path);
int dma_test_read_scope(int path, int use_tp, int no_chunks);


int dma_tests(int path);
int dma_upload_short(int path);
int dma_display(int path);
int dma_read_size(int path);
int fill_scope_mod(NGPDScopeModule *mod);
int check_scope_tp_data(NGPDScopeModule *mod);
int check_scope_read_tp_data(NGPDScopeModule *mod);
int spi_regs_test(int path);
int i2c_tests(int path);

extern int limit_bram;
extern int scope_out_skip;
extern int test_2stream;
extern int error_tests;
