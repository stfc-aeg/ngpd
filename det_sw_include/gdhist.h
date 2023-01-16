#define PCI_VENDOR_ID_ALTERA 0x1172
#define PCI_DEVICE_ID_HISTMEM 4
#define PCI_DEVICE_ID_SCALER  5
#define PCI_DEVICE_ID_HISTMEM_RPE 6
#define PCI_DEVICE_ID_HISTMEM_RPE_PAGED 7
#define PCI_DEVICE_ID_HISTMEM_RPE_PAGED4K 	   8

#define PCI_DEVICE_ID_FREDA 				 0xA

#define PCI_DEVICE_ID_XSPRESS2 				0x10
#define PCI_DEVICE_ID_XSPRESS2_PLAYBACK		0x14

#define MAX_PATH 20
#define MAX_SLOT 16

#define XSPRESS_DETS_PER_SLOT 4
#define XSPRESS_FRAMES_PER_CHAN 256
#define XSPRESS_GRADES_PER_CHAN 16
#define XSPRESS_BINS_PER_MCA 4096

#define XSPRESS_SCALERS_PER_CHAN 4

#define CARRIER_NUM_LOCAL   100
#define GDHIST_MAX_REGIONS 2

typedef enum {PM_Local, PM_Midas_User, PM_Midas_Drvr, PM_Summed, PM_Interleaved} GDHCombType ;
typedef struct _GDHistPath {
	int valid;
	GDHCombType type;
	enum {HistMem, Scaler, RipPriEnc, Xspress2, Freda} dev_type;
	int fd;
	u_int32 * mem_base;
	u_int32 * io_base;
	u_int32 mem_size_words;
	u_int32 tp_size;
	int carrier_num, slot;
	int nbits_interleave, interleave_board_num;	
	int edge_mode;	/* For GDScaler ... */
	int use_dma;
	u_int32 nx[GDHIST_MAX_REGIONS], ny[GDHIST_MAX_REGIONS], nt[GDHIST_MAX_REGIONS];
	u_int32 reg_base[GDHIST_MAX_REGIONS];
	int num_regions;
	int sub_path[MAX_SLOT+1];
	int num_slots;
	int use_tp;
	/* For XSPRESS2+ */
	int num_dets, max_num_dets;
	u_int32 x2_csr;	/* CSR in GDHist BE for XSPRESS2+ */
	u_int32 x2_fev_cont;
	u_int32 x2_format;
	u_int32 x2_scope;
	int nbits_chan;	/* generally 2 bit (all 4 channels) are coded in y, but is special test cases 1 or 0 bits are coded */
	int revision;
	int freda_xiic_state;
	int freda_first_chip, freda_last_chip;
	} GDHistPath;

typedef struct trigger_b_setttings
{
	int avemode;
	int two_over_mode, enable;
	int disable_split, combined;
	int scaled_thres_mode;
	int arm_thres, end_thres, rearm_thres;
	int sep1, sep2, data_delay, event_time, over_thres_delay, over_thres_stretch, over_thres_trim;
} Xspress2_TriggerB;

typedef struct trigger_a_settings
{
	int thres0, thres1, thres2, thres3;
	int stretch, advance;
	int fixed_event_time;
	int avemode;
	int combined;
	int disable;
} Xspress2_TriggerA;


extern GDHistPath GDHist[];
extern u_int32 *GDHist_io_base;
extern u_int32 *GDHist_mem_base;


typedef struct _intl_struct
{
	int type;
	int rw_inc;
	int row_inc;
} Interleaving;
#define INTL_SIMPLE 	0
#define INTL_SCALER 	1
#define INTL_SUMMED		2
#define INTL_XSPRESS2	3

#define GDScal_NumChan		64
#define GDScal_NumTF		(1024*1024)

/* Working all in Lword increments  */

#define IOAddr_CSR  		0x00	/*  R/W CSR in both chip. 					*/
#define IOAddr_FECSR  		0x01	/* Read only copy of CSR in Front FPGA		*/
#define IOAddr_ClrAddr 		0x02	/* Write clear address						*/
#define IOAddr_ClrCount		0x03 	/* Write clear count and start clear 		*/

#define IOAddr_TFGTime		0x04	/* Read/Write internal TFG time register	*/
#define IOAddr_TFGStatus	0x05	/* Read internal TFG status register		*/
#define IOAddr_TFGStart		0x06	/* Write to start internal TFG				*/
#define IOAddr_TFGStop		0x07	/* Write to stop internal TFG				*/

#define IOAddr_ClrMonA0		0x8	/* Monitor clear progress all 4 banks..		*/
#define IOAddr_ClrMonA1		0x9
#define IOAddr_ClrMonA2		0xA
#define IOAddr_ClrMonA3		0xB
#define IOAddr_ClrMonC0		0xC
#define IOAddr_ClrMonC1		0xD
#define IOAddr_ClrMonC2		0xE
#define IOAddr_ClrMonC3		0xF

#define IOAddr_TPAddr		0x10	/* Read write test pattern memory current address (when not running) 	*/
#define IOAddr_TPData		0x11	/* R/W test pattern data to current memory location, with increment		*/
#define IOAddr_TPCycles		0x12	/* R/W Test pattern number of cycles register and counter.				*/
#define IOAddr_TPStart		0x13	/* Write starts test pattern burst										*/
#define IOAddr_TPStop		0x14	/* Write Stops test pattern 											*/
#define IOAddr_TPAltData	0x15	/* Read back of data from alternate test pattern generator.				*/
#define IOAddr_TPAltStart	0x16	/* Start Alternate tp generator (using input clock domain)				*/
#define IOAddr_LVDSOut		0x17	/* Data Register for data to drive LVDS output							*/
#define IOAddr_LVDSIn		0x18	/* Read back of LVDS Input.												*/
#define IOAddr_InpFIFO		0x19	/* Read back of Input FIFO.												*/
#define IOAddr_StartTimer   0x1A	/* Start Input clock timers 											*/
#define IOAddr_ReadTimers   0x1B    /* Read input clock  timers 											*/

#define IOAddr_Scal_TFReg 	0x04
#define IOAddr_Scal_TPDataB	0x15	/* R/W test pattern data to current memory location, with increment		*/
#define IOAddr_Scal_TPDataA	0x16	/* R/W test pattern data to current memory location, with increment		*/
#define IOAddr_Scal_LVDSOutB	0x17	/* Data Register for data to drive LVDS output							*/
#define IOAddr_Scal_LVDSOutA	0x18	/* Data Register for data to drive LVDS output							*/
#define IOAddr_Scal_CSTest		0x19

#define IOAddr_DiscCont		0x1C	/* Discriminator input control. 										*/
#define IOAddr_PosnCont		0x1D	/* Position Builder Control												*/
#define IOAddr_TFCont		0x1E	/*  Time Frame control and fixed time frame								*/
#define IOAddr_DAECont		0x1F	/* DAE1/2 timing/format control											*/

#define IOAddr_TFrameIn   	0x1C	/* Trime Frame counter from input on Xspress2							*/

#define SMALLEST_MEM_SIZE_WORDS (32*1024*1024)

#define MEM_SIZE_WORDS (64*1024*1024)
#define TP_SIZE			256
#define TP_PAUSE        0x80000000
/* Details of memory layout allow tests to pick on dificult end cases for hardware */

#define NumStreams 	4
#define BurstLength 8
#define RowLength	512
#define NumBanks	4
#define BankMult (NumStreams*BurstLength)
#define PCI_VENDOR_ID_ALTERA 0x1172
/* possible error codes */
#if defined(_OSK)
/* 0x900 through 0x9ff GDHist */
#define E_PMEM_BASE          ((9<<8)+000)    /* base error code for IFF */
#elif defined(_OS9000)
/* 0x90000 through 0x900ff IFF */
#define E_PMEM_BASE          ((9<<16)+000)   /* base error code for IFF */
#else
#define E_PMEM_BASE 1024
#endif /* _OS9000 */

#define EOS_BAD_SLOT  				(E_PMEM_BASE+0) 
#define EOS_NO_IO_PAGE 				(E_PMEM_BASE+1)
#define EOS_NO_MEM_PAGE 			(E_PMEM_BASE+2)
#define EOS_UNSUPPORTED_MEM_TYPE	(E_PMEM_BASE+3)
#define EOS_WRONG_VENDOR_ID			(E_PMEM_BASE+4)
#define EOS_NO_MIDAS_BRIDGE			(E_PMEM_BASE+5)
#define EOS_CLEAR_TIMEOUT			(E_PMEM_BASE+6)
#define EOS_BLOCK_TOO_BIG			(E_PMEM_BASE+0x10)
#define EOS_DMA_STOPPED				(E_PMEM_BASE+0x11)
#define EOS_DMA_HALTED				(E_PMEM_BASE+0x12)
#define EOS_DMA_PCI_ERROR			(E_PMEM_BASE+0x13)
#define EOS_DMA_VME_ERROR			(E_PMEM_BASE+0x14)
#define EOS_DMA_PROG_ERROR			(E_PMEM_BASE+0x15)
#define EOS_UNKSVC_ON_SCALER		(E_PMEM_BASE+0x16)
#define EOS_UNKSVC_ON_HIST			(E_PMEM_BASE+0x17)
#define EOS_UNKOWN_DEVICE			(E_PMEM_BASE+0x18)

#define GDH_RESET_FULL 1

#define EOS_BAD_PARAM				(E_PMEM_BASE+0x20)
#ifdef __LINUX__
#define EOS_HARDWARE                (E_PMEM_BASE+0x21)
#endif

#define NUMSLOTS	6		 /* Slot 0 is PCI bridge, 1..5 are PMCs 1..5 */
#define MAXBARS     6
#ifdef LOCAL_PCI
#define BAD_SLOT(slot) ((slot) != 0)
#else
#define BAD_SLOT(slot) ((slot) < 1 || (slot) >= NUMSLOTS)
#endif
#ifdef __LINUX__
#define MIDAS_MAX_BOARDS 9
#define MIDAS_CONFIG_BASE	0xF6000000
#define MIDAS_CONFIG_STEP   0x01000000
#define MEM_BASE            0xE0000000
#else
#define MIDAS_MAX_BOARDS 4
#define MIDAS_CONFIG_BASE	0xF9000000
#define MIDAS_CONFIG_STEP   0x01000000
#define MEM_BASE  ((u_char *)0xE0000000)
#endif
/* #define MEM_BASE_TRANS 0x20000000 */
#define VME_OFFSET_A32_CPU2VME		0		/* Offset from CPU view of VME A32 to Physical VME A32 address */


/* PCI configuration information formed durring iniz entry point of driver */
/* Stored in driver LuStats and readable by driver getstats                */

typedef struct
{
	u_int16 used;
	u_int16 io;		/* Memory Or IO Bar, 1=> IO Bar 	*/
	u_int32 size;	/* Size specified in bytes			*/
	u_int32 base;	/* Byte base address in PCI terms	*/
} BarInfo;

typedef struct
{
	u_int32 used;
	u_int16 vendor_id;
	u_int16 device_id;
	u_int32 * io_base;		/* Base address of Generic module IO area  		in CPU space		*/
	u_int32 * mem_base;		/* Base address of generic module memory space 	in CPU space		*/
	u_int32 * mem_base_pci;	/* Base Address of memory space ass seen from local PCI bus for DMA */
	BarInfo bar[MAXBARS];
} PCISlotInfo;

typedef struct
{
	u_int32 busy;							/* need to mark DMA as busy with ISR, but need to think how/when ??? */
	u_int32 nlines, pci_len, vme_len;		/* Copy of read 3D command packet */
	u_int32 line_inc;
	u_int32 nframes, frame_inc;
	u_int32 f2f_unaligned;
	u_int32 line, frame;					/* Counters, used in lu_stat to comunicate with ISR */
	u_int32 *vme_ptr, *frame_base, *line_base;
} Read3D;

/* Parameter Block for communication with device driver setstat */
typedef struct 
{
	int slot;			/* Midas carrier PMC slot number */
	u_int32 value;		/* For Register Write		*/
	u_int32 cycles;		/* For Setup of TP Generator */
	u_int32 start, num;	/* For Read/Write to Memory and TestPattern generator 	*/
#ifdef __LINUX__
	u_int32 x, y, t;
	u_int32 dx, dy, dt;
	u_int32 nx, ny;
#else
	u_int32 nlines, pci_len, vme_len;	/* For Read 3D, operation nlines of length pci_len, incrementing the CPU (VME side) vme_len */
	u_int32 nframes, frame_inc;			/* Read 3D reads nframes and add frame_inc at the beginning of each new frame */
	u_int32 f2f_unaligned;
#endif
	u_int32 line_inc;	/* Used in read 3D and in Scaler Test pattern and reused in XSPRESS2+ to access RAM table	*/
	u_int32 *data;		
/*	PCISlotInfo *slot_info; */
#ifdef __LINUX__
	Interleaving intl;
#endif
} GDHistPb;

/* User library entry points */
int 	GDHist_open(char * mem_name);

int 	GDHist_read(int path, u_int32 start_offset, u_int32 num_lwords, u_int32 *buffer);
int 	GDHist_read_slot(int path, int sub_slot, u_int32 start_offset, u_int32 num_lwords, u_int32 *buffer);
int 	GDHist_write(int path, u_int32 start_offset, u_int32 num_lwords, u_int32 *buffer);
int 	GDHist_write_slot(int path, int sub_slot, u_int32 start_offset, u_int32 num_lwords, u_int32 *buffer);
int 	GDHist_read3d(int path, u_int32 x, u_int32 y, u_int32 t, u_int32 dx, u_int32 dy, u_int32 dt, u_int32 *buffer, int region);
int 	__GDHist_read(int path, u_int32 start_offset, u_int32 num_lwords, u_int32 *buffer, Interleaving *intl);
int 	__GDHist_write(int path, u_int32 start_offset, u_int32 num_lwords, u_int32 *buffer, Interleaving *intl);
u_int32	GDHist_get_size(int path);
u_int32 GDHist_set_shape(int path, u_int32 nx, u_int32 ny);
int 	GDHist_clear_wait(int path, u_int32 start_offset, u_int32 num_lwords);
int 	GDHist_clear_start(int path, u_int32 start_offset, u_int32 num_lwords);
int 		GDHist_wait_for_clear(int path);
int 		GDHist_enable(int path);
int 		GDHist_disable(int path);
u_int32 GDHist_get_revision(int path);

/* Test program entry points */
int         GDHist_no_dma(int path);

u_int32 * 	GDHist_get_mem_base(int path);
u_int32 * 	GDHist_get_io_base(int path);

int 		GDHist_write_test_pat(int path, u_int32 start, u_int32 num, u_int32 *buffer, u_int32 cycles);
int 		GDHist_read_test_pat(int path, u_int32 start, u_int32 num, u_int32 *buffer);
int 		GDHist_read_alt_test_pat(int path, u_int32 start, u_int32 num, u_int32 *buffer);
int 		GDHist_start_tp(int path);
int 		GDHist_start_alt_tp(int path);
int			GDHist_stop_tp(int path);
int 		GDHist_wait_tp_finished(int path, int exercise_pci, int csr_clk);
int 		GDHist_wait_hist_finished(int path);
int 		GDHist_write_TPCycles(int path, u_int32 data);
u_int32 	GDHist_read_TPCycles(int path);

int 		GDHist_write_CSR(int path, u_int32 data);
u_int32 	GDHist_read_CSR(int path);
u_int32 	GDHist_read_FECSR(int path);
int 		GDHist_write_TPAddr(int path, u_int32 value);
u_int32 	GDHist_read_TPAddr(int path);
int 		GDHist_write_LVDSOut(int path, u_int32 data);
u_int32 	GDHist_read_LVDSOut(int path);
u_int32 	GDHist_read_LVDSIn(int path);
int 		GDHist_read_InpFIFO(int path, u_int32 num, u_int32 *buffer);
u_int32 	GDHist_run_clock_timers(int path);
int 		GDScal_set_tfg(int path, int tf);
int 		GDScal_write_test_pat(int path, u_int32 start, u_int32 num, u_int32 *buffer, u_int32 cycles, int sub_path, int row_inc);
int 		GDScal_read_test_pat(int path, u_int32 start, u_int32 num, u_int32 *buffer, int sub_slot);
int 		GDScal_write_TFReg(int path, u_int32 data);
u_int32 	GDScal_read_TFReg(int path);

int 		GDHist_IsScaler(int path);
int 		GDHist_IsHistMem(int path);
int 		GDHist_IsRipPriEnc(int path);
int 		GDScal_get_numchan(int path);
int 		GDHist_reset(int path, int full_reset);
/* XSPRESS2+ */ 
int 		GDHist_write_TFGTime(int path, u_int32 data);
u_int32 	GDHist_read_TFGTime(int path);
u_int32 	GDHist_read_TFGStatus(int path);
int 		GDHist_write_DiscCont(int path, u_int32 data);
u_int32 	GDHist_read_DiscCont(int path);
int 		GDHist_write_TFGCont(int path, u_int32 data);
u_int32 	GDHist_read_TFGCont(int path);
int 		GDHist_write_PosnCont(int path, u_int32 data);
u_int32 	GDHist_read_PosnCont(int path);
int 		GDHist_write_DAECont(int path, u_int32 data);
u_int32 	GDHist_read_DAECont(int path);
int 		GDHist_TFG_Start(int path);
int	 		GDHist_TFG_Stop(int path);
int 		GDHist_TFG_EnbIRQ(int path, u_int32 enb_mask, int procid, int signum);

int 		GDHist_get_nx(int path, int reg);
int 		GDHist_get_ny(int path, int reg);
int 		GDHist_get_nt(int path, int reg);
int 		GDHist_get_num_slots(int path);

/* XSPRESS2 + registers */
int 		GDHist_write_xspress2_reg(int path, int offset, u_int32 value);
u_int32 	GDHist_read_xspress2_reg(int path, int offset);
u_int32 	GDHist_read_xspress2_reg_copy(int path, int slot, int offset);
int 		GDHist_write_xspress2_fev_ram(int path, int chan, u_int32 start, u_int32 num, u_int32 *buffer, u_int32 cycles, int table);
int 		GDHist_read_xspress2_fev_ram(int path, int chan, u_int32 start, u_int32 num, u_int32 *buffer, int table);
int 		GDHist_xspress2_format_scope(int path, int num_x, int sub_slot, int cha, int srca, int chb, int srcb, int use_fev_tp_busy, u_int32 alternates);
int 		GDHist_xspress2_format_run(int path, int res_mode, int res_thres, int adc_cont, int disables, int use_fev_tp_busy, int aux_mode, int nbits_eng_lost);
int 		GDHist_xspress2_setup_input(int path, int thresh0, int thresh1, int thresh2, int thresh3, u_int32 fev_cont, u_int32 servo_cont);
int 		GDHist_xspress2_set_trigger_a(int path, int detector, int thresh0, int thresh1, int thresh2, int thresh3, int event_stretch, int fixed_event_time, int advance, int avemode, int disable);
int 		GDHist_xspress2_set_trigger_b(int path, int detector, Xspress2_TriggerB *trig_b);
int 		GDHist_xspress2_set_window(int path, int detector, int winLow, int winHigh);
int 		GDHist_xspress2_set_glitch(int path, int detector, u_int32 glitch, u_int32 glitch_b);
int 		GDHist_xspress2_set_glitch_thres(int path, int detector, int thres);
u_int32 	GDHist_read_xspress2_frame(int path, u_int32 *counters);
int 		GDHist_xspress2_get_resmode(int path, int *res_mode, int *res_thres);
int 		GDHist_xspress2_build_quotient(int path, int chan, double scaling);
int 		GDHist_xspress2_read_trigger_b(int path, int chan, Xspress2_TriggerB *trig_b);
int 		GDHist_xspress2_read_trigger_a(int path, int detector, Xspress2_TriggerA *trig_a);
int 		GDHist_nbits_grade(int pah, int res_mode);
int 		GDHist_nbits_adc(int path, int adc_cont);
int 		GDHist_xspress2_cal_event(int path, int cal_cont, int off23);
int 		GDHist_xspress2_set_all_event_window(int path, int detector, int winLow);

int 		GDHist_xspress2_reset_tail_rev(int path);
int		 	GDHist_xspress_max_servo_time(int path);
int 		GDHist_xspress2_setup_servo(int path, u_int32 servo_cont, u_int32 servo_cont_b, int enable);
int 		GDHist_xspress2_clear_servo_tail(int path, int chan);
int 		GDHist_xspress2_get_max_ave(int path);
int 		GDHist_xspress2_get_cal_event_version(int path);
int 		GDHist_xspress2_get_pwl_size(int path);
int 		GDHist_xspress2_setup_c2c(int path, int slot, u_int32 fev_cont_even, u_int32 fev_cont_odd);

typedef enum {None, Packed, Regs } AllEventType;

/* Functions to control Midas-50 DMA from user state */
int			midas_dma(u_int32 start, u_int32 num, int to_hist_memory, u_int32 *ptr);
int 		GDHist_write_freda_reg(int path, int offset, u_int32 value);
u_int32 	GDHist_read_freda_reg(int path, int offset);

int 		GDHist_freda_set_chipss(int path, int first, int last);
int 		GDHist_freda_get_chips(int path, int *first, int *last);

int 		GDHist_freda_xiic_write_dac(int path, int chip, int chan, int value);
int 		GDHist_freda_xiic_write_ioexp24(int path, int chip, int value);
int 		GDHist_freda_xiic_write_ioexp8(int path, int chip, int port, int value);
int 		GDHist_freda_xiic_ioexp_output(int path);
int 		GDHist_freda_xiic_read_ioexp(int path, int chip, int pin);
int 		GDHist_freda_xiic_read_dac(int path, int chip, int chan);
int 		GDHist_freda_xiic_write_chip(int path, int chip);
int 		GDHist_freda_xiic_read_chip(int path, int slot);

int 		GDHist_write_freda_vh_vl(int path, int first, int num, int which, u_int32 *ptr);
int 		GDHist_read_freda_vh_vl(int path, int first, int num, u_int32 *ptr);

/* Wrapped functions to call Midas Driver */
u_int32 *	midas_get_io_base(int fd, int slot_num);
u_int32 *	midas_get_mem_base(int fd, int slot_num);
error_code  midas_get_slot_info(int fd, int slot_num, PCISlotInfo *info);
error_code 	midas_write_csr(int fd, int slot_num, u_int32 value);
error_code  midas_write_tfreg(int fd, int slot_num, u_int32 value);
error_code 	midas_write_tpaddr(int fd, int slot_num, u_int32 value);
error_code  midas_write_lvds_out(int fd, int slot_num, u_int32 value);
error_code  midas_scal_write_lvds_out(int fd, int slot_num, u_int32 value, int which);

u_int32 	midas_read_csr(int fd, int slot_num);
u_int32 	midas_read_tfreg(int fd, int slot_num);
u_int32 	midas_read_cstest(int fd, int slot_num);

u_int32 	midas_read_fecsr(int fd, int slot_num);
u_int32 	midas_read_tpaddr(int fd, int slot_num);
u_int32 	midas_read_lvds_out(int fd, int slot_num);
u_int32 	midas_scal_read_lvds_out(int fd, int slot_num, int which);
u_int32 	midas_read_lvds_in(int fd, int slot_num);
u_int32 	midas_read_timers(int fd, int slot_num);

error_code  midas_write_testpat(int fd, int slot_num, u_int32 start, u_int32 num, u_int32 *buffer, u_int32 cycles);
error_code  midas_read_testpat(int fd, int slot_num, u_int32 start, u_int32 num, u_int32 *buffer);
error_code  midas_scal_write_testpat(int fd, int slot_num, u_int32 start, u_int32 num, u_int32 *buffer, u_int32 cycles, int row_inc);
error_code  midas_scal_read_testpat(int fd, int slot_num, u_int32 start, u_int32 num, u_int32 *buffer);


error_code  midas_clear_and_wait(int fd, int slot_num, u_int32 start, u_int32 num);
error_code 	midas_clear_start(int fd, int slot_num, u_int32 start, u_int32 num)	;
error_code 	midas_wait_for_clear(int fd, int slot_num);
u_int32     midas_wait_for_tp(int fd, int slot_num, u_int32 *ex_buf, u_int32 ex_count, u_int32 ex_offset, int csr_clk);
error_code 	midas_start_testpat(int fd, int slot_num);
error_code 	midas_start_alt_testpat(int fd, int slot_num);
error_code  midas_stop_testpat(int fd, int slot_num);
error_code  midas_read_inp_fifo(int fd, int slot_num, u_int32 num, u_int32 *buffer);
error_code 	midas_start_timer(int fd, int slot_num);

error_code  midas_read_mem(int fd, int slot, u_int32 start, u_int32 num, u_int32 *ptr, Interleaving *intl);
error_code  midas_write_mem(int fd, int slot, u_int32 start, u_int32 num, u_int32 *ptr, Interleaving *intl);
error_code	midas_read3d(int fd, int slot, u_int32 x, u_int32 y, u_int32 t, u_int32 dx, u_int32 dy, u_int32 dt, u_int32 nx, u_int32 ny,	u_int32  *buffer, Interleaving *intl, int reg_base);
error_code 	midas_write_xspress2_reg(int fd, int slot_num, u_int32 value, int offset);
u_int32 	midas_read_xspress2_reg(int fd, int slot_num, int offset);

error_code 	midas_write_xspress2_testpat(int fd, int slot_num, int chan, u_int32 start, u_int32 num, u_int32 *buffer, u_int32 cycles, int table);
error_code 	midas_read_xspress2_testpat(int fd, int slot_num, int chan, u_int32 start, u_int32 num, u_int32 *buffer, int table);

extern int  midas_csr_at_tp_first;
extern int  midas_csr_at_tp_over;


extern double GDHist_BERate, GDHist_FERate[];
extern int GDHist_debug;

#define INP_TIMER_COUNT 0x4000

#define CSR_ENB_HIST_INP 0x1
#define CSR_ENB_HIST_STR 0x2
#define CSR_RESET		 0x4
#define CSR_SWAB_MEMORY	 0x8
#define CSR_ENB_CLR_IRQ  0x10
#define CSR_ENB_TP_IRQ   0x20
#define CSR_ENB_TFG_EOR_IRQ  	0x10000
#define CSR_ENB_TFG_PAUSE_IRQ  	0x20000

#define CSR_BOARD_NUM(bnum)	 ((bnum)<<8)
#define CSR_NBITS_BOARD(nb)	 ((nb)<<12)
#define CSR_GET_BOARD_NUM(csr)	 (((csr)>>8)&0xF)
#define CSR_GET_NBITS_BOARD(csr)	 (((csr)>>12)&0x3)
#define CSR_EDGE_MODE(em)	 ((em)<<8)
#define CSR_LVDS_DATA_RUN   		0x0000 /* LVDS IO configured to RUN */
#define CSR_LVDS_OUT_FIXED   		0x4000 /* LVDS Outputs Fixed number from LVDS Test Register */
#define CSR_LVDS_OUT_TESTPAT      	0x8000 /* LVDS Outputs Test pattern from alternate TP generator */
#define CSR_LVDS_INTERNAL_TESTPAT 	0xC000 /* Alterate TP generator loops internally to input and drives LVDS output */
#define CSR_LVDS_MASK				0xC000 /* mask to get data mode */
#define CSR_FE_CLK_NORMAL		  	0x00000	/* Normal clock from incomming cable */
#define CSR_FE_CLK_INT44		  	0x10000	/* Use PCI clock to make 44 Mhz FE Clk */
#define CSR_FE_CLK_INT50		  	0x20000	/* Use PCI clock to make 50 Mhz FE Clk */
#define CSR_FE_CLK_INT66		  	0x30000	/* Use PCI clock to make 66 Mhz FE Clk */
#define CSR_FE_CLK_MASK				0x30000 /* Mask to be FE clock mode */
#define CSR_FE_CLK(ic)		  		((ic)<<16) /* Normal clock from incomming cable */
#define CSR_GET_INP_CLK(csr)        (((csr)>>16)&3)	
#define CSR_BYPASS_INPRDY           0x40000		/* Bypass mode to run loop-from tests in daisy chain */
#define CSR_INP_TIMING				0x2000000  /* Input clock timer is running */
#define CSR_ENB_TEST_PAT 0x80
#define CSR_TP_BUSY 	 0x80000000
#define CSR_HIST_BUSY 	 0x40000000
#define CSR_RESET_DONE 	 0x20000000

#define CSR_CLEARING 0x1000000
#define CSR_GET_CLEARING(x)  (((x)>>20)&0xF)

#define CSR_CLR_IRQ			0x02000000
#define CSR_TP_IRQ			0x04000000
#define CSR_RUN_IRQ			0x08000000
#define CSR_PAUSE_IRQ		0x10000000


#define FECSR_INP_TEST_IR 	 0x00100000
#define FECSR_COMB_NOTFULL 	 0x00200000
#define FECSR_TP_USE_ALT 	 0x00400000
#define FECSR_TP_ALT_RUN 	 0x00800000
#define FECSR_TP_ALT_WAIT 	 0x01000000
#define FECSR_INP_TIMING	 0x02000000  /* Input clock timer is running */
#define FECSR_GET_C_OUT(fecsr) (((fecsr)>>26)&3)
#define FECSR_DCM_RESET		 0x10000000
#define FECSR_DCM_LOCKED	 0x20000000

#define GDS_TFREG_TF(tf) ((tf)&0xFFFFF)
#define GDS_TFREG_COUNT_ENB    0x100000
#define GDS_TFREG_TF_INT	0x00000000
#define GDS_TFREG_TF_S1OR3	0x10000000
#define GDS_TFREG_TF_S2OR4	0x20000000
#define GDS_TFREG_NOXFER	0x40000000

#define GDS_CSR_EDGE_MODE(em)	(((em)&3)<<8)
#define GDS_CSR_EDGE_MODE_LEVEL 	(GDS_CSR_EDGE_MODE(0))
#define GDS_CSR_EDGE_MODE_EDGE 		(GDS_CSR_EDGE_MODE(1))

#define GDS_CSR_TEST_MODE_A(tm) 	(((tm)&3)<<12)
#define GDS_CSR_TEST_MODE_B(tm) 	(((tm)&3)<<14)

#define GDS_CSR_TEST_MODE_A_RUN     	(GDS_CSR_TEST_MODE_A(0))
#define GDS_CSR_TEST_MODE_A_INT_LOOP	(GDS_CSR_TEST_MODE_A(1))
#define GDS_CSR_TEST_MODE_A_OUT_REG     (GDS_CSR_TEST_MODE_A(2))
#define GDS_CSR_TEST_MODE_A_OUT_PAT     (GDS_CSR_TEST_MODE_A(3))

#define GDS_CSR_TEST_MODE_B_RUN     	(GDS_CSR_TEST_MODE_B(0))
#define GDS_CSR_TEST_MODE_B_INT_LOOP	(GDS_CSR_TEST_MODE_B(1))
#define GDS_CSR_TEST_MODE_B_OUT_REG     (GDS_CSR_TEST_MODE_B(2))
#define GDS_CSR_TEST_MODE_B_OUT_PAT     (GDS_CSR_TEST_MODE_B(3))

#define GDS_CSR_STEP_CS					4


/* possible Get/SetStat functions */

#ifdef __LINUX__
#define IOCTL_GDH			0xD0
#define GS_GET_SLOT_INFO 	_IOW(IOCTL_GDH, 0, GDHistPb)
#define GS_GET_IO_BASE	 	_IOW(IOCTL_GDH, 1, GDHistPb)
#define GS_GET_MEM_BASE	 	_IOW(IOCTL_GDH, 2, GDHistPb)
#define GS_READ_CSR		 	_IOW(IOCTL_GDH, 3, GDHistPb)
#define GS_READ_FECSR	 	_IOW(IOCTL_GDH, 4, GDHistPb)
#define GS_READ_TPADDR 	 	_IOW(IOCTL_GDH, 5, GDHistPb)
#define GS_READ_TPCYCLES  	_IOW(IOCTL_GDH, 6, GDHistPb)
#define GS_READ_LVDS_OUT	_IOW(IOCTL_GDH, 7, GDHistPb)
#define GS_READ_LVDS_IN		_IOW(IOCTL_GDH, 8, GDHistPb)
#define GS_READ_TIMERS      _IOW(IOCTL_GDH, 9, GDHistPb)
#define GS_READ_TFREG		_IOW(IOCTL_GDH, 10, GDHistPb)
/* #define GS_READ_TPCYCLE		_IOW(IOCTL_GDH, 11, GDHistPb))*/
#define GS_READ_CSTEST		_IOW(IOCTL_GDH, 12, GDHistPb)


#define SS_WRITE_CSR		_IOW(IOCTL_GDH, 13, GDHistPb)
#define SS_WRITE_TPADDR		_IOW(IOCTL_GDH, 14, GDHistPb)
#define SS_WRITE_TESTPAT	_IOW(IOCTL_GDH, 15, GDHistPb)
#define SS_READ_TESTPAT		_IOW(IOCTL_GDH, 16, GDHistPb)
#define SS_START_TESTPAT	_IOW(IOCTL_GDH, 17, GDHistPb)
#define SS_STOP_TESTPAT		_IOW(IOCTL_GDH, 18, GDHistPb)

#define SS_CLEAR_AND_WAIT	_IOW(IOCTL_GDH, 19, GDHistPb)
#define SS_CLEAR_START		_IOW(IOCTL_GDH, 20, GDHistPb)
#define SS_WAIT_FOR_CLEAR	_IOW(IOCTL_GDH, 21, GDHistPb)
#define SS_WAIT_FOR_TP		_IOW(IOCTL_GDH, 22, GDHistPb)

#define SS_WRITE_MEM		_IOW(IOCTL_GDH, 23, GDHistPb)
#define SS_READ_MEM			_IOW(IOCTL_GDH, 24, GDHistPb)

#define SS_WRITE_TFREG		_IOW(IOCTL_GDH, 25, GDHistPb)
#define SS_WRITE_TPCYCLE	_IOW(IOCTL_GDH, 26, GDHistPb)

#define SS_READ_MEM3D		_IOW(IOCTL_GDH, 27, GDHistPb)
#define SS_WRITE_LVDS_OUT	_IOW(IOCTL_GDH, 28, GDHistPb)
#define SS_READ_INPFIFO		_IOW(IOCTL_GDH, 29, GDHistPb)

#define SS_READ_ALT_TESTPAT	_IOW(IOCTL_GDH, 30, GDHistPb)
#define SS_START_ALT_TESTPAT _IOW(IOCTL_GDH, 31,GDHistPb)
#define SS_START_TIMER       _IOW(IOCTL_GDH, 32, GDHistPb)
#define SS_GDH_RESET		 _IOW(IOCTL_GDH, 33, GDHistPb)
#define GS_GET_REVISION		_IOW(IOCTL_GDH, 34, GDHistPb)
#define GS_GET_X2REVISIONS	_IOW(IOCTL_GDH, 35, GDHistPb)


/* Extra Gs/SS ioctls for RipPriEnc */
#define GS_READ_TFG_TIME	_IOW(IOCTL_GDH, 40, GDHistPb)
#define GS_READ_TFG_STATUS	_IOW(IOCTL_GDH, 41, GDHistPb)
#define GS_READ_DISC_CONT	_IOW(IOCTL_GDH, 42, GDHistPb)
#define GS_READ_POSN_CONT	_IOW(IOCTL_GDH, 43, GDHistPb)
#define GS_READ_TFG_CONT	_IOW(IOCTL_GDH, 44, GDHistPb)
#define GS_READ_DAE_CONT	_IOW(IOCTL_GDH, 45, GDHistPb)

#define SS_TFG_START		_IOW(IOCTL_GDH, 50, GDHistPb)
#define SS_TFG_STOP			_IOW(IOCTL_GDH, 51, GDHistPb)
#define SS_WRITE_TFG_TIME	_IOW(IOCTL_GDH, 52, GDHistPb)
#define SS_WRITE_DISC_CONT	_IOW(IOCTL_GDH, 53, GDHistPb)
#define SS_WRITE_POSN_CONT	_IOW(IOCTL_GDH, 54, GDHistPb)
#define SS_WRITE_TFG_CONT	_IOW(IOCTL_GDH, 55, GDHistPb)
#define SS_WRITE_DAE_CONT	_IOW(IOCTL_GDH, 56, GDHistPb)

#define SS_ENB_TFG_IRQ		_IOW(IOCTL_GDH, 60, GDHistPb)
/* And for XSPRESS2+ */
#define GS_READ_X2_REG		_IOW(IOCTL_GDH, 70, GDHistPb)
#define SS_WRITE_X2_REG		_IOW(IOCTL_GDH, 71, GDHistPb)
#define GS_READ_TFRAME_IN	_IOW(IOCTL_GDH, 72, GDHistPb)
#define SS_WRITE_FEV_TP		_IOW(IOCTL_GDH, 73, GDHistPb)
#define SS_READ_FEV_TP		_IOW(IOCTL_GDH, 74, GDHistPb)
#define GS_READ_X2_REG_COPY	_IOW(IOCTL_GDH, 75, GDHistPb)

/* And Freda */
#define SS_WRITE_FREDA_REG  _IOW(IOCTL_GDH, 80, GDHistPb)
#define GS_READ_FREDA_REG  	_IOW(IOCTL_GDH, 81, GDHistPb)
#else

#define GS_GET_SLOT_INFO 	512
#define GS_GET_IO_BASE	 	513
#define GS_GET_MEM_BASE	 	514
#define GS_READ_CSR		 	520
#define GS_READ_FECSR	 	521
#define GS_READ_TPADDR 	 	522
#define GS_READ_TPCYCLES  	523
#define GS_READ_LVDS_OUT	540
#define GS_READ_LVDS_IN		541
#define GS_READ_TIMERS      542
#define GS_READ_TFREG		543
/* #define GS_READ_TPCYCLE		544*/
#define GS_READ_CSTEST		545
#define GS_GET_REVISION     546
#define GS_GET_X2REVISION     547

#define SS_WRITE_CSR		550
#define SS_WRITE_TPADDR		551
#define SS_WRITE_TESTPAT	552
#define SS_READ_TESTPAT		553
#define SS_START_TESTPAT	554
#define SS_STOP_TESTPAT		555

#define SS_CLEAR_AND_WAIT	556
#define SS_CLEAR_START		557
#define SS_WAIT_FOR_CLEAR	558
#define SS_WAIT_FOR_TP		559

#define SS_WRITE_MEM		560
#define SS_READ_MEM			561

#define SS_WRITE_TFREG		562
#define SS_WRITE_TPCYCLE	563

#define SS_READ_MEM3D		570
#define SS_WRITE_LVDS_OUT	580
#define SS_READ_INPFIFO		581

#define SS_READ_ALT_TESTPAT	590
#define SS_START_ALT_TESTPAT 591
#define SS_START_TIMER       592
#define SS_GDH_RESET		 593

/* Extra Gs/SS ioctls for RipPriEnc */
#define GS_READ_TFG_TIME	600
#define GS_READ_TFG_STATUS	601
#define GS_READ_DISC_CONT	602
#define GS_READ_POSN_CONT	603
#define GS_READ_TFG_CONT	604
#define GS_READ_DAE_CONT	605

#define SS_TFG_START		610
#define SS_TFG_STOP			611
#define SS_WRITE_TFG_TIME	612
#define SS_WRITE_DISC_CONT	613
#define SS_WRITE_POSN_CONT	614
#define SS_WRITE_TFG_CONT	615
#define SS_WRITE_DAE_CONT	616

#define SS_ENB_TFG_IRQ		620

#define GS_READ_X2_REG      630
#define SS_WRITE_X2_REG     631
#define GS_READ_TFRAME_IN   632
#define SS_WRITE_FEV_TP		633
#define SS_READ_FEV_TP		634
#define GS_READ_X2_REG_COPY	635
#define SS_WRITE_FREDA_REG  700
#define GS_READ_FREDA_REG  	701

#endif
#ifdef linux
#define SI_RPETFG __SI_CODE(__SI_RT, 2)
#endif

#define TFG_CONT_DAE	 		(0<<30)
#define TFG_CONT_TFG2 			(1<<30)
#define TFG_CONT_FIXED 			(2<<30)
#define TFG_CONT_INTERNAL 		(3<<30)
#define TFG_CONT_PAUSE_BEFORE 	(1<<29)
#define TFG_CONT_ENB_EXT_START 	(1<<28)
#define TFG_CONT_EXT_START_FALL	(1<<27)
#define TFG_CONT_ENB_EXT_CONT	(1<<26)
#define TFG_CONT_EXT_CONT_FALL	(1<<25)

#define TFG_STAT_RUNNING	0x40000000
#define TFG_STAT_PAUSED 	0x80000000
#define TFG_MAXFRAMES		65536
#define TFG_TIME_UNIT		(1.0E-6)

typedef struct dev_cap_struct 
{
	int regs, tp, mem;
} DevCaps;

#define DEVCAP_MAX_DEVICE 	0x14

#define DEVCAP_MEM_HIST			1
#define DEVCAP_MEM_SCAL			2
#define DEVCAP_MEM_HIST_PAGED	3
#define DEVCAP_MEM_XSPRESS2 	4

#define DEVCAP_REG_CSR			1		/* Has CSR Register */
#define DEVCAP_REG_FECSR		2		/* Has FECSR Register */
#define DEVCAP_REG_CLEAR1		4		/* has current Clear DMA engine */
#define DEVCAP_REG_ITFG1		8		/* Has Ripple Priority Enc Internal TFG version 1 */
#define DEVCAP_REG_SCAL_TF   	0x10	/* Scaler time frame register */

#define DEVCAP_REG_LVDSOUT_HIST		 0x100
#define DEVCAP_REG_LVDSOUT_SCAL		 0x200
#define DEVCAP_REG_LVDSIN			 0x400
#define DEVCAP_REG_FIFOIN			 0x800
#define DEVCAP_REG_TIMERS			0x1000
#define DEVCAP_REG_SCAL_CSTEST		0x2000
#define DEVCAP_REG_PRI_ENC			0x4000
#define DEVCAP_REG_FIFOIN_X2		0x8000
#define DEVCAP_REG_FEVIRTEX_X2  	0x10000
#define DEVCAP_REG_TFRAME_IN    	0x20000
#define DEVCAP_REG_FEVIRTEX_FREDA	0x20000

#define DEVCAP_TP_ADDR			1
#define DEVCAP_TP_CYCLES		2
#define DEVCAP_TP_HIST_DATA		4
#define DEVCAP_TP_SCAL_DATA		8
#define DEVCAP_TP_START			0x10
#define DEVCAP_TP_STOP			0x20
#define DEVCAP_TP_WAIT			0x40
#define DEVCAP_TP_ALT_DATA		0x80
#define DEVCAP_TP_ALT_START		0x100


#define DEVCAP_TP_FEV_X2		0x200

#define DEVCAP_TP_HIST (DEVCAP_TP_ADDR | DEVCAP_TP_CYCLES | DEVCAP_TP_HIST_DATA | DEVCAP_TP_START | DEVCAP_TP_STOP | \
 DEVCAP_TP_WAIT | DEVCAP_TP_ALT_DATA | DEVCAP_TP_ALT_START)

#define DEVCAP_TP_SCAL (DEVCAP_TP_ADDR | DEVCAP_TP_CYCLES | DEVCAP_TP_SCAL_DATA | DEVCAP_TP_START | DEVCAP_TP_STOP | \
 DEVCAP_TP_WAIT)

#define DEVCAP_TP_RPE (DEVCAP_TP_ADDR | DEVCAP_TP_CYCLES | DEVCAP_TP_HIST_DATA | DEVCAP_TP_START | DEVCAP_TP_STOP | \
 DEVCAP_TP_WAIT)

#define DEVCAP_REG_HIST (DEVCAP_REG_CSR | DEVCAP_REG_FECSR | DEVCAP_REG_CLEAR1 | DEVCAP_REG_LVDSOUT_HIST |	\
				 		 DEVCAP_REG_LVDSIN | DEVCAP_REG_FIFOIN | DEVCAP_REG_TIMERS )

#define DEVCAP_REG_SCAL (DEVCAP_REG_CSR	| DEVCAP_REG_FECSR | DEVCAP_REG_CLEAR1 | DEVCAP_REG_LVDSOUT_SCAL | \
						 DEVCAP_REG_TIMERS | DEVCAP_REG_SCAL_CSTEST | DEVCAP_REG_SCAL_TF )

#define DEVCAP_REG_RPE (DEVCAP_REG_CSR | DEVCAP_REG_FECSR | DEVCAP_REG_CLEAR1 | DEVCAP_REG_ITFG1 | DEVCAP_REG_LVDSOUT_HIST |	\
						DEVCAP_REG_LVDSIN | DEVCAP_REG_FIFOIN | DEVCAP_REG_TIMERS | DEVCAP_REG_PRI_ENC)

#define DEVCAP_REG_X2 (DEVCAP_REG_CSR | DEVCAP_REG_FECSR | DEVCAP_REG_CLEAR1 | DEVCAP_REG_ITFG1      | \
                       DEVCAP_REG_FIFOIN_X2 | DEVCAP_REG_TIMERS | DEVCAP_REG_FEVIRTEX_X2 | DEVCAP_REG_TFRAME_IN)
						 /* test pattern generator is a single 2x32 bit wide test pat like scaler, thouigh actually uses input clock domian */
#define DEVCAP_TP_X2 (DEVCAP_TP_ADDR | DEVCAP_TP_CYCLES | DEVCAP_TP_SCAL_DATA | DEVCAP_TP_START | DEVCAP_TP_STOP | \
 DEVCAP_TP_WAIT | DEVCAP_TP_FEV_X2)

#define DEVCAP_REG_FREDA (DEVCAP_REG_CSR	| DEVCAP_REG_FECSR | DEVCAP_REG_CLEAR1 |  \
						 DEVCAP_REG_TIMERS | DEVCAP_REG_SCAL_CSTEST | DEVCAP_REG_SCAL_TF | DEVCAP_REG_FEVIRTEX_FREDA )


#define RPE_CONREG_ENB			0x0001
#define RPE_CONREG_2D			0x0002
#define RPE_CONREG_RADIAL		0x0004
#define RPE_CONREG_TR_DEL(x)	(((x)&0x1F)<<8)
#define RPE_CONREG_2D_TIME(x)	(((x)&0x3)<<16)




#define RPE_POSN_NBITS_X(x)		(((x)&0xF)<<0)
#define RPE_POSN_NBITS_Y(x)		(((x)&0xF)<<4)
#define RPE_POSN_NBITS_WID_X(x)	(((x)&0x7)<<8)
#define RPE_POSN_NBITS_WID_Y(x)	(((x)&0x7)<<12)
#define RPE_POSN_MAX_WID_X(x)	(((x)&0x1F)<<16)
#define RPE_POSN_MAX_WID_Y(x)	(((x)&0x1F)<<21)
#define RPE_POSN_MIN_WID_X(x)	(((x)&0x7)<<26)
#define RPE_POSN_MIN_WID_Y(x)	(((x)&0x7)<<29)

#define RPE_MASK_FORMAT			0x0000FFFF
#define RPE_MASK_WIDTH			0xFFFF0000

#define RPE_MAX_MAX_WID			31
#define RPE_MAX_MIN_WID			7

#define RPE_TF_MODE_DAE 		0x00000000
#define RPE_TF_MODE_TFG2 		0x40000000
#define RPE_TF_MODE_FIXED(tf)	(0x80000000|((tf)&0x03FFFFF))
#define RPE_TF_MODE_VETO 	    0x20000000

#define RPE_TF_MODE_ITFG 		0xC0000000

#define RPE_TF_MODE_MASK 		0xC0000000

#define RPE_DAE_MODE_GDAQ		0
#define RPE_DAE_MODE_DAE2		1
#define RPE_DAE_SETUP(x)		(((x)&0xFF)<<8)
#define RPE_DAE_STROBE(x)		(((x)&0xFF)<<16)
#define RPE_DAE_HOLD(x)			(((x)&0xFF)<<24)

/* XSPRESS2+ code */
#define X2_REGS_START  		64
#define X2_REGS_SIZE		64
#define X2_SERVO_CONT		0x00    /* In Revision 16 onwards Fev Cont is split 								*/
#define X2_CAL_CONT			0x01    /* In Revision 16 onwards Cal Cont replace common threshold THRES23			*/
#define X2_THRES01   		0x00   	/* Control register 1 threshold 0 and 1	for Chan 0..3, read CH01			*/
#define X2_THRES23   		0x01   	/* Control register 2 threshold 2 and 3	for Chan 0..3, read CH01			*/
#define X2_WINDOW0   		0x02   	/* Control register 3 window values for channel 0							*/
#define X2_FEV_CONT 		0x03   	/* Control register 4 (main Fevirtex control register) Wr0..3, Read0,1		*/
#define X2_FEV_TPMEM0		0x04	/* Chan 0 Test pattern memory read/write and inc address 					*/
#define X2_FEV_TPMEM1		0x05 	/* Chan 1 Test pattern memory read/write and inc address 					*/
#define X2_FEV_TPMEM2		0x06	/* Chan 2 Test pattern memory read/write and inc address 					*/
#define X2_FEV_TPMem3		0x07	/* Chan 3 Test pattern memory read/write and inc address 					*/
#define X2_FEV_TPADDR		0x08	/* Test pattern start address for all chan, write only						*/
#define X2_FEV_TPCYCLES		0x09	/* Test Pattern number of cycles (16 bit) write only for all channels		*/
#define X2_RAM_SELECT		0x0A	/* 8 bits of RAM select for lookup table and test pat generator				*/
#define X2_RAM_SELECT_CH23	0x0B	/* Read copy of RAM select in channel 23 chip								*/
#define X2_GLITCH0 			0x0C   	/* Glitch/reset control channel 0											*/
#define X2_GLITCH1 			0x0D   	/* Glitch/reset control channel 1											*/
#define X2_GLITCH2 			0x0E   	/* Glitch/reset control channel 2											*/
#define X2_GLITCH3 			0x0F   	/* Glitch/reset control channel 3											*/
#define X2_CNT_RES0 		0x10   	/* Reset counter for channel 0												*/
#define X2_CNT_RES1 		0x11   	/* Reset counter for channel 1 												*/
#define X2_CNT_RES2		 	0x12   	/* Reset counter for channel 2												*/
#define X2_CNT_RES3 		0x13   	/* Reset counter for channel 3												*/
#define X2_FRAME_NUM	 	0x14   	/* Control register 21 (live frame counter)									*/
#define X2_WINDOW1 			0x15   	/* Control register 22 window values for channel 1 (c.f.window 0)			*/
#define X2_WINDOW2 			0x16 	/* Control register 23 window values for channel 2							*/
#define X2_WINDOW3 			0x17	/* Control register 24 window values for channel 3 							*/
#define X2_THRES01_CH23		0x18	/* Overwrite/Read back of Thresh01 register copied in Chan 23				*/
#define X2_SERVO_CH23		0x18	/* Overwrite/Read back of Thresh01 register copied in Chan 23				*/
#define X2_THRES23_CH23		0x19	/* Overwrite/Read back of Thresh23 register copied in Chan 23				*/
#define X2_CAL_CONT_CH23	0x19    /* In Revision 16 onwards Cal Cont replace common threshold THRES23			*/

#define X2_FEV_CONT_CH23    0x1A	/* Overwrite/Read back of FEVirtex control register copied in Chan 23		*/
#define X2_FORMAT_CH01		0x1B	/* Format Control for Chan01 and 23											*/	
#define X2_FORMAT_CH23		0x1C	/* Format Control for Copy in Chan 23										*/
#define X2_SCOPE			0x1D	/* Scope Control for Chan01 and 23											*/	
#define X2_SCOPE_CH23		0x1E	/* Scope Control for Copy in Chan 23										*/

#define X2_RESET0			0x20	/* Reset Control for chan 0													*/
#define X2_RESET1			0x21	/* Reset Control for chan 1													*/
#define X2_RESET2			0x22	/* Reset Control for chan 2													*/
#define X2_RESET3			0x23	/* Reset Control for chan 3													*/

#define X2_GLITCHB0			0x24	/* Extra Glitch Control for chan 0											*/
#define X2_GLITCHB1			0x25	/* Extra Glitch Control for chan 1											*/
#define X2_GLITCHB2			0x26	/* Extra Glitch Control for chan 2											*/
#define X2_GLITCHB3			0x27	/* Extra Glitch Control for chan 3											*/

#define X2_THRES01_0		0x28	/* Individual Threshold control for V2 onwards								*/
#define X2_THRES01_1		0x29	/* Individual Threshold control for V2 onwards								*/
#define X2_THRES01_2		0x2A	/* Individual Threshold control for V2 onwards								*/
#define X2_THRES01_3		0x2B	/* Individual Threshold control for V2 onwards								*/
#define X2_THRES23_0		0x2C	/* Individual Threshold control for V2 onwards								*/
#define X2_THRES23_1		0x2D	/* Individual Threshold control for V2 onwards								*/
#define X2_THRES23_2		0x2E 	/* Individual Threshold control for V2 onwards								*/
#define X2_THRES23_3		0x2F	/* Individual Threshold control for V2 onwards								*/
/* Reuse Trigger 1 thresholds for "all" event lower window when confgure all event window */


#define X2_TRIGB_THRES0		0x30	/* Individual Trigger B Threshold control for V16 onwards					*/
#define X2_TRIGB_THRES1		0x31	/* Individual Trigger B Threshold control for V16 onwards					*/
#define X2_TRIGB_THRES2		0x32	/* Individual Trigger B Threshold control for V16 onwards					*/
#define X2_TRIGB_THRES3		0x33	/* Individual Trigger B Threshold control for V16 onwards					*/
#define X2_TRIGB_TIMEB0		0x34	/* Individual Trigger B TimesB for V16 onwards								*/
#define X2_TRIGB_TIMEB1		0x35	/* Individual Trigger B TimesB for V16 onwards								*/
#define X2_TRIGB_TIMEB2		0x36	/* Individual Trigger B TimesB for V16 onwards								*/
#define X2_TRIGB_TIMEB3		0x37	/* Individual Trigger B TimesB for V16 onwards								*/
#define X2_TRIGB_TIME0		0x38	/* Individual Trigger B Times for V16 onwards								*/
#define X2_TRIGB_TIME1		0x39	/* Individual Trigger B Times for V16 onwards								*/
#define X2_TRIGB_TIME2		0x3A	/* Individual Trigger B Times for V16 onwards								*/
#define X2_TRIGB_TIME3		0x3B	/* Individual Trigger B Times for V16 onwards								*/
#define X2_SERVO_CONT_B		0x3C	/* Servo Control B in Ch01, Rev 18 onwards									*/
#define X2_SCOPE_SEARCH     0x3D	/* Special scope search aid, when configured                                */
#define X2_SERVO_CONT_B_CH23 0x3E	/* Servo Control B in Ch23, Rev 18 onwards									*/
#define X2_SCOPE_SEARCH_CH23 0x3F	/* Special scope search aid, when configured                                */


#define XSPRESS2_NUM_REGS   X2_REGS_SIZE	/* Number of registers to keep a copy */
/* XSPRESS2 control register 1 (thresholds 0 and 1) bit maps and offsets */
#define X2V1TH01_THRESHOLD_0_START 0
#define X2V1TH01_THRESHOLD_0_MASK 0xffff
#define X2V1TH01_THRESHOLD_1_START 16
#define X2V1TH01_THRESHOLD_1_MASK 0xffff0000

/* XSPRESS2 control register 2 (thresholds 2 and 3) bit maps and offsets */
#define X2V1TH23_THRESHOLD_2_START 0
#define X2V1TH23_THRESHOLD_2_MASK 0xffff
#define X2V1TH23_THRESHOLD_3_START 16
#define X2V1TH23_THRESHOLD_3_MASK 0xffff0000
/* Similar for V2 onward code with individual threshold register for each channel and packed to make space eventr stretch */
#define X2V2TH01_THRESHOLD_0_START 0
#define X2V2TH01_THRESHOLD_0_MASK 0xfff
#define X2V2TH01_THRESHOLD_1_START 12
#define X2V2TH01_THRESHOLD_1_MASK 0x00fff000

#define X2V2TH01_GET_THRES0(x)    (((x)>>X2V2TH01_THRESHOLD_0_START)&X2V2TH01_THRESHOLD_0_MASK)
#define X2V2TH01_GET_THRES1(x)    (((x)>>X2V2TH01_THRESHOLD_1_START)&X2V2TH01_THRESHOLD_0_MASK)

#define X2V2TH01_SET_AVE_MODE(x)  (((x)&7)<<24)    
#define X2V2TH01_SET_ADVANCE(x)   (((x)&7)<<27)    
#define X2V2TH01_GET_AVE_MODE(x)  (((x)>>24)&7)    
#define X2V2TH01_GET_ADVANCE(x)   (((x)>>27)&7)    

/* XSPRESS2 control register 2 (thresholds 2 and 3) bit maps and offsets */
#define X2V2TH23_THRESHOLD_2_START 0
#define X2V2TH23_THRESHOLD_2_MASK 0xfff
#define X2V2TH23_THRESHOLD_3_START 12
#define X2V2TH23_THRESHOLD_3_MASK  0x00fff000

#define X2V2TH23_GET_THRES2(x)    (((x)>>X2V2TH23_THRESHOLD_2_START)&X2V2TH23_THRESHOLD_2_MASK)
#define X2V2TH23_GET_THRES3(x)    (((x)>>X2V2TH23_THRESHOLD_3_START)&X2V2TH23_THRESHOLD_2_MASK)

#define X2V2TH23_SET_STRETCH(x)   (((x)&0x3F)<<24)
#define X2V2TH23_GET_STRETCH(x)   (((x)>>24)&0x3F)
#define X2V2TH23_FIXED_EVENT_TIME (1<<30)
#define X2V2TH23_DISABLE          (1<<31)

#define X2_MAX_THRESHOLD_A			0xfff
#define X2_MAX_ADVANCE_A			0x7
#define X2_MAX_STRETCH_A			0x3f

#define X2TRIG_A_AVE1SEP1			0
#define X2TRIG_A_AVE1SEP2  	 		4
#define X2TRIG_A_AVE1SEP3			3
#define X2TRIG_A_AVE1SEP4			7
#define X2TRIG_A_AVE2SEP2			1
#define X2TRIG_A_AVE2SEP4			5
#define X2TRIG_A_AVE4SEP4			2

#define X2TRIGB_THRES_SET_ARM(x)	(((x)&0x3FF))
#define X2TRIGB_THRES_SET_END(x)	(((x)&0x3FF)<<10)
#define X2TRIGB_THRES_SET_REARM(x)	(((x)&0x3FF)<<20)
#define X2TRIGB_THRES_TWO_OVER		(1<<30)
#define X2TRIGB_THRES_ENABLE_CFD	(1<<31)
#define X2TRIGB_THRES_GET_ARM(x)	(((x)&0x3FF))
#define X2TRIGB_THRES_GET_END(x)	(((x)>>10)&0x3FF)
#define X2TRIGB_THRES_GET_REARM(x)	(((x)>>20)&0x3FF)

#define X2TRIGB_ENABLE_CFD     1		/* used as argument to setup-trigger_b function */
#define X2TRIGB_ENABLE_OVERTHR 2		/* used as argument to setup-trigger_b function */

#define X2TRIGB_TIME_SET_DIFF1(x)	(((x)&0x7f))		/* First differential delay. Value 0 creates delay of 3 */
#define X2TRIGB_TIME_SET_DIFF2(x)	(((x)&0x7f)<<7) 	/* Second differential delay. Value 0 creates delay of 3 */
#define X2TRIGB_TIME_SET_DATA(x)	(((x)&0x7f)<<14)	/* Data delay. Value 0 creates delay of 3 */
#define X2TRIGB_TIME_SET_EVENT(x)	(((x)&0x7f)<<21)	/* Event width. Value 1 creates width of 1 */
#define X2TRIGB_TIME_SET_AVEMODE(x) (((x)&3)<<28)		/* Average mode */
#define X2TRIGB_TIME_DISABLE_SPLIT	(1<<30)
#define X2TRIGB_TIME_ENABLE_OVERTHR (1<<31)
#define X2TRIGB_TIME_GET_DIFF1(x)	(((x)&0x7f))		/* First differential delay. Value 0 creates delay of 3 */
#define X2TRIGB_TIME_GET_DIFF2(x)	(((x)>>7)&0x7f) 	/* Second differential delay. Value 0 creates delay of 3 */
#define X2TRIGB_TIME_GET_DATA(x)	(((x)>>14)&0x7f)	/* Data delay. Value 0 creates delay of 3 */
#define X2TRIGB_TIME_GET_EVENT(x)	(((x)>>21)&0x7f)	/* Event width. Value 1 creates width of 1 */
#define X2TRIGB_TIME_GET_AVEMODE(x) (((x)>>28)&3)		/* Average mode */

#define X2TRIGB_SET_OVERTHR_DELAY(x)    (((x)&0x1F))
#define X2TRIGB_SET_OVERTHR_STRETCH(x)  (((x)&0x3F)<<8)
#define X2TRIGB_SET_OVERTHR_TRIM(x)     (((x)&0x1F)<<16)
#define X2TRIGB_TIMEB_SCALED_THRES_MODE	(1<<31)
#define X2TRIGB_TIMEB_SET_THRES_SCALE(x) (((x)&1)<<30)
#define X2TRIGB_GET_OVERTHR_DELAY(x)    (((x)&0x1F))
#define X2TRIGB_GET_OVERTHR_STRETCH(x)  (((x)>>8)&0x3F)
#define X2TRIGB_GET_OVERTHR_TRIM(x)     (((x)>>16)&0x1F)
#define X2TRIGB_TIMEB_GET_THRES_SCALE(x) (((x)>>30)&1)

#define X2TRIG_B_SCALED_THRES_OFF 0
#define X2TRIG_B_SCALED_THRES_DIV4 0x8
#define X2TRIG_B_SCALED_THRES_DIV2 0x9

#define X2TRIG_B_AVE1				0
#define X2TRIG_B_AVE2				1
#define X2TRIG_B_AVE4				2
#define X2TRIG_B_AVE8				3

#define X2DEFAULT_TRIGB_DELAY       (40+16) /* Trig B delay to use when using trigger A only */
#define X2TRIGB_MIN_DELAY			(18+8+16+1)
#define X2TRIGB_MAX_TRIM			31
#define X2TRIGB_MAX_OVERTHR_DELAY	31
#define X2TRIGB_MAX_OVERTHR_STRETCH	0x3F
#define X2TRIGB_MAX_THRES			0x3FF
#define X2TRIGB_MAX_SEP1			0x7F
#define X2TRIGB_MAX_SEP2			0x7F
#define X2TRIGB_MAX_DATA_DEL		0x7F
#define X2TRIGB_MAX_EVENT_TIMEL		0x7F

/* XSPRESS2 control registers 3,22,23,24 (windows for all 4 channels) bit maps and offsets */
#define X2WINDOW_LOW_START 	    	 0
#define X2WINDOW_LOW_MASK 	0x0000ffff
#define X2WINDOW_HIGH_START 	    16 
#define X2WINDOW_HIGH_MASK 	0xffff0000
#define X2WINDOW_ALL_EVENT_MASK  0xf000f000
#define X2WINDOW_PACK_ALL_EVENT(x)  (((x)&0x1e0)<<23|((x)&0x1E)<<11)

/* XSPRESS2 control register 4 (main control register bit maps */

#define X2CSR_SCOPESELECT(x)   (((x)&0x3)<< 8)
#define X2CSR_GET_SCOPESELECT(csr) (((csr)>>8)&0xF)
#define X2CSR_SCOPE0_CH01		0
#define X2CSR_SCOPE0_CH23		1

#define X2CSR_SCOPE1_CH01		0
#define X2CSR_SCOPE1_CH23		2

#define X2CSR_DATA_PATH_RUN			(0<<14)
#define X2CSR_DATA_PATH_SCOPE		(1<<14)
#define X2CSR_DATA_PATH_TEST_HIST	(2<<14)
#define X2CSR_DATA_PATH_TEST_SCOPE	(3<<14)
#define X2CSR_DATA_PATH_MASK		(3<<14)

#define X2CSR_ODD_BOARD				(1<<6)

#define X2F1CSR_USE_FEV_TP_BUSY		0x00002000
#define X2F1CSR_CLK_CH01_LOCKED 	0x08000000
#define X2F1CSR_CLK_CH23_LOCKED 	0x10000000
#define X2F1CSR_CLK_OUT_LOCKED 		0x20000000

#define X2CSR_ENABLE_DEBUG           (1<<12)

#define X2DATA_BURST				0x40000000	/* Mark data in test pattern as burst data */

#define X2F1CSR_FRAME_ZERO 			0x0800000
#define X2F1CSR_FRAMING 			0x1000000



#define X2SCOPE_MODE			0x0000001
#define X2SCOPE_BNUM(x)		(((x)&0xF)<<1)
/* Front end Scope select signal, copied to all 4 channels */
#define X2SCOPE_SEL1_LOW(x)	(((x)&0xF)<<8)
#define X2SCOPE_SEL1_HIGH(x)	(((x)&0xF)<<12)
#define X2SCOPE_SEL_INP		0
#define X2SCOPE_SEL_DEBUG_A	1
#define X2SCOPE_SEL_DEBUG_B	2
#define X2SCOPE_SEL_DEBUG_C	3
#define X2SCOPE_SEL_SERVO_GRAD	4
#define X2SCOPE_SEL_GLITCH	5
#define X2SCOPE_SEL_TRIGB_DIFF1	6
#define X2SCOPE_SEL_TRIGB_DIFF2	7
#define X2SCOPE_SEL_TRIGGERB	8
#define X2SCOPE_SEL_TRIGGERS	9
#define X2SCOPE_SEL_TRIGB_DIFF2TOP	10
#define X2SCOPE_SEL_SERVO_GRAD_EST	11
#define X2SCOPE_SEL_SERVO_GROSS_GRAD_EST	12

#define X2SCOPE_SEL_TEST_A	14
#define X2SCOPE_SEL_TEST_B	15
/* Second stage Scope select at output side of capture FIFO */
#define X2SCOPE_SEL2_LOW(x)	(((x)&1)<<6)
#define X2SCOPE_SEL2_HIGH(x)	(((x)&1)<<7)

/* Special Scope Servo mode so reset is replaced by the Ored event, reset glicth etc */
#define X2SCOPE_ALT_SCOPE_SERVO  (1<<21)
/* Special Scope Output mode to seacrh for specific top/bot counts */
#define X2SCOPE_ALT_SCOPE_OUT   (1<<22)
/* Interaction of scope mode with servo settling */
#define X2SCOPE_DELAY_TRIG      (1<<23)
#define X2SCOPE_RESTART_SERVO   (1<<24)
/* Top bit of scope control allow alternate digital bits to merge in with data */

#define X2SCOPE_OTHER_RESET     (2<<30)
#define X2SCOPE_GR_ACTIVE       (1<<30)

#define X2FORMAT_DISABLE_TFG		2	/* Write 1 here to disable TFG input and just use software controlled timing 	*/
#define X2FORMAT_DISABLE_SCALER		4	/* Disable scaler so top histogramming locations are not overwritten.		 	*/
#define X2FORMAT_DISABLE_HIST		8   /* Disable all histogram daAt and use all memory for many scalers.				*/

#define X2FORMAT_RES_MODE(x)	(((x)&0xF)<<4)
#define X2FORMAT_RES_THRES(x)	(((x)&0x3FF)<<8)
#define X2FORMATA_NBITS_ADC(x)	(((x)&0x3)<<24)
#define X2FORMATB_NBITS_ADC(x)	(((x)&0x3)<<25)
#define X2FORMATB_NBITS_ENG(x)	(((x)&0xF)<<21)	
#define X2FORMAT_AUX_MODE(x)	(((x)&0x7)<<28)
#define X2FORMAT_AUX_ADC            0
#define X2FORMAT_AUX_WIDTH          1
#define X2FORMAT_AUX_RST_START_ADC	2
#define X2FORMAT_AUX_NEB_RST_ADC	3
#define X2FORMAT_AUX_NEB_RST_RST_START_ADC	4
#define X2FORMAT_AUX_TIME_FROM_RST			5
#define X2FORMAT_AUX_NEB_RST_TIME_FROM_RST	6
#define X2FORMAT_AUX_NEB_RST_TIME_FROM_RSTX8 7


#define X2FORMAT_PILEUP_REJECT     (1<<27)

#define X2FORMAT_RES_MODE_MINDIV8	0		/* Normal Reslution grade mode of shorter of two runs / 8 	*/
#define X2FORMAT_RES_MODE_NONE		1		/* NO resolution grades 									*/
#define X2FORMAT_RES_MODE_THRES		2		/* 2 Res grades using threshold 						 	*/
#define X2FORMAT_RES_MODE_LOG		3		/* Approx logrithmic grade 									*/
#define X2FORMAT_RES_MODE_TOP		4		/* All 7 or 10 bits of top count							*/
#define X2FORMAT_RES_MODE_BOT		5		/* All 7 or 10 bits of Bottom Count							*/
#define X2FORMAT_RES_MODE_MIN		6		/* All 7 or 10 bits of Min of top and bottom				*/
#define X2FORMAT_RES_MODE_PILEUP	7
#define X2FORMAT_RES_MODE_LUT_SETUP	8		/* 12 bits of res grade to learn setup for LO/LUT mode 		*/
#define X2FORMAT_RES_MODE_LUT_THRES	9		/* 1 bit thrsholded using the LUT LSB						*/

#define X2FORMAT_GET_RES_MODE(x)	(((x)>>4)&0xF)
#define X2FORMAT_GET_RES_THRES(x)	(((x)>>8)&0x3FF)
#define X2FORMATA_GET_NBITS_ADC(x)	(((x)>>24)&0x3)
#define X2FORMATB_GET_NBITS_ADC(x)	(((x)>>25)&0x3)
#define X2FORMATA_GET_NBITS_ENG_LOST(x)	(((x)>>21)&0x7)
#define X2FORMATB_GET_NBITS_ENG_LOST(x)	(((x)>>21)&0xF)

#define XSPRESS2_MIN_BITS_ENG  1
#define XSPRESS2_MAX_BITS_ENG  12


#define X2FEC_DATA_INV			0x00010000    	/* Controls raw data stream inversion 							*/
#define X2FEC_FRAME0_INV  		0x00020000     	/* Controls frame 0 signal inversion							*/
#define X2FEC_FRAMING_INV       0x00040000     	/* Controls framing signal inversion							*/
#define X2FEC_USE_TP      		0x00080000     	/* Mux in data from test pat generator							*/
#define X2FEC_RESET_INV   		0x00100000     	/* Controls detector reset signal inversion						*/
#define X2FEC_TP_CONTINUOUS		0x00200000		/* Make Test pat generator run continuously 					*/
#define X2FECA_CHAN_MASK(x)		(((x)&0xF)<<22)	/* mask to turn off channels V1 Firmware 						*/
#define X2FEC_RUN_AVE_LEN(x)	(((x)&0x3)<<22)	/* Max running Average length 128, 256, 512 or 1024 V2 onwards  */
#define X2FEC_GET_RUN_AVE_LEN(x) (((x)>>22)&0x3)	/* Get Max Run Ave Length									*/

#define X2FEC_SEL_ENERGY(x)		(((x)&0x1F)<<26)
#define X2FEC_SERVO_DISABLE		0x80000000		/* Turn off Servo system 										*/
#define X2FECB_CHAN_MASK(x)		(((x)&0xF)<<0)	/* mask to turn off channels V16 on firmware					*/
#define X2FECB_WIN_COUNTS_RESETS 	(1<<4)		/* Switch Win event counter to count resets or alternate DTC	*/
#define X2FECB_RESET_COUNT_MODE(x)	(((x)&0x3)<<5)	/* Reset Counter Mode V16 on firmware						*/
#define X2FECB_RESET_COUNT_TICKS	0	/* Reset Counter counts ticks (normal)									*/
#define X2FECB_RESET_COUNT_EDGES	1	/* Reset Counter counts number of resets								*/
#define X2FECB_GET_RESET_COUNT_MODE(x)	(((x)>>5)&0x3)	/* Reset Counter Mode V16 on firmware					*/
#define X2FECB_ENB_INL_CORR			(1<<7)	       /* Enable use of Test Pat memory for INL correction     		*/
#define X2FECB_RESET_FROM_DIFF		(1<<8)	       /* Reset infered from differential of input          		*/
#define X2FECB_GR_ACT_MASKS_RESET	(1<<9)	       /* Global Reset Active input masks Reset ticks counter  		*/
#define X2FECB_GR_FROM_PLAYBACK		(1<<10)	       /* Global Reset Active From Other rest for use with Playback	*/

#define X2FEC_SERVO_DIV(x)		((x)&3)
#define X2FEC_SERVO_PWL_ENB         4
#define X2FEC_SERVO_MASK_GLITCH     8
#define X2FEC_SERVO_STRETCH(x)		((((x)&0x3F)<<4)|(((x)&64)<<24))
#define X2FEC_SERVO_PWL_TIME(x)     (((x)&3)<<10)
#define X2FEC_SERVO_PRE_GAP(x)		(((x)&0xF)<<12)
#define X2FEC_SERVO_DIVR16(x)  		(((x)&3)|(((x)&4)<<29))
#define X2FEC_SERVO_GET_STRETCH(x)	(((x)>>4)&0x3F)
#define X2FEC_SERVO_GET_PWL_TIME(x) (((x)>>10)&3)
#define X2FEC_SERVO_GET_PRE_GAP(x)	(((x)>>12)&0xF)
#define X2SERVO_LIMIT(x)         (((x)&0xFF)<<16)
#define X2SERVO_MAX_LIMIT		0xFF
#define X2SERVO_FROM_CAL		(1<<29)
#define X2SERVO_CAL_EXCLUDE(x)  (((x)&3)<<27)
#define X2SERVO_CLEAR_ON_UPDATE			(1<<26)
#define X2SERVO_ACCUM_ON_UPDATE			(1<<25)
#define X2SERVO_EXTEND_GLITCH           (1<<24)		/* This feature tried and removed at rev 20*/
#define X2SERVO_MASK_GLOBAL_GLITCH       (1<<24)

#define X2FEC_C2C_MASK				(7<<11)
#define X2FEC_C2C_DISABLE			(0<<11)
#define X2FEC_C2C_PRESERVE			(1<<11)
#define X2FEC_C2C_MASK_LONE			(2<<11)
#define X2FEC_C2C_MASK_ALL			(3<<11)
#define X2FEC_C2C_IGNORE_INP 		(4<<11)

#define X2SERVOB_GRAD_LIM(x)		(((x)&0xFFFF)<<0)
#define X2SERVOB_ENB_GLOB_GRAD		(1<<16)
#define X2SERVOB_GLOB_ADJ_SIZE(x)	(((x)&0x3)<<17)
#define X2SERVOB_GLOB_ADJ_POSN(x)	(((x)&0x7F)<<19)
#define X2SERVOB_GLOB_ADJ_SHIFT(x)	(((x)&0x7)<<26)

#define X2SERVOB_STRETCH_TIME		(1<<29)
#define X2SERVOB_GLITCH_AS_RESET	(1<<30)
#define X2SERVOB_USE_ADC_TOP4		(1<<31)

#define X2GLITCH_ENABLE					1
#define X2GLITCH_GLITCH_TIME(x)      (((x)&0x1ff)<<1)
#define X2GLITCH_GLITCH_THRES(x)     (((x)&0xff)<<10)
#define X2GLITCH_DIFF_TIME(x)        (((x)&0x1)<<18)
#define X2GLITCH_PAD_MODE(x)         (((x)&0x1)<<19)
#define X2GLITCH_PRE_TIME(x)         (((x)&0x1f)<<20)

#define X2GLITCH_FROM_GLOBAL_RESET_LEVEL (1<<25)
#define X2GLITCH_FROM_GLOBAL_RESET_EDGE	 (1<<26)
#define X2GLITCH_RETRIGGER       	 (1<<27)
#define X2GLITCH_MASK_GLITCH_EVENT   (1<<28)
#define X2GLITCH_COUNT_GLICTH_TIME   (1<<29)

#define X2GLITCH_EVDET(x)            (((x)&0x3)<<30)

#define X2GLITCH_GET_GLITCH_TIME(x)  (((x)>>1)&0x1ff)
#define X2GLITCH_GET_GLITCH_THRES(x) (((x)>>10)&0xff)

#define X2GLITCH_MAX_GLITCH_TIME 0x1FF
/* MAx glitch time was 0x3F in early releases, but discourangng such operation and not supported in newer firmware */
#define X2GLITCH_MAX_PRE_TIME    0x1F

#define X2GLITCHB_HOLDOFF(x)	 	((x)&0x1FF)
#define X2GLITCHB_SUMMODE(x)        (((x)&3)<<9)
#define X2GLITCHB_GET_SUMMODE(x)        (((x)>>9)&3)
#define X2GLITCHB_MASK_BOG_EVENT    	(1<<11)
#define X2GLITCHB_GLITCH_STRETCH(x)	 	(((x)&0x3F)<<12)
#define X2GLITCHB_GET_GLITCH_STRETCH(x) (((x)>>12)&0x3F)
#define X2GLITCHB_BGE_STRETCH(x)        (((x)&0x1F)<<18)
#define X2GLITCHB_GET_BGE_STRETCH(x)    (((x)>>18)&0x1F))
#define X2GLITCHB_DIFF_SEP(x)           (((x)&0xF)<<24)
#define X2GLITCHB_GET_DIFF_SEP(x)       (((x)>>24)&0xF)
#define X2GLITCHB_FORCE_EVENT           (1<<23)

#define X2GLITCHB_BGE_EXTEND            (1<<28)
#define X2GLITCHB_SHORT_PADDING         (1<<29)
#define X2GLITCHB_BGE_LOOKAHEAD         (1<<30)

#define X2GLITCHB_SUM1		0
#define X2GLITCHB_SUM2		1
#define X2GLITCHB_SUM4		2

#define X2GLITCHB_MAX_HOLDOFF 0x1FF
#define X2GLITCHB_MAX_STRETCH 0x3f
#define X2GLITCHB_MAX_BGE_STRETCH 0x1f
#define X2GLITCHB_MAX_DIFF_SEP    15

#define X2RESET_TIME(x)       (((x)&0x3ff)<<0)
#define X2RESET_OUTPUT_RESET   (1<<10)
#define X2RESET_GET_TIME(x)   (((x)>>0)&0x3ff)
#define X2RESET_DELAY(x)      (((x)&0xf)<<12)
#define X2RESET_GET_DELAY(x)  (((x)>>12)&0xf)
#define X2RESET_THRES(x)      (((x)&0x3FFF)<<16)
#define X2RESET_OVER_RANGE_EXTEND 0x40000000
#define X2RESET_ENABLE_TAIL_SUB 0x80000000

#define X2RESET_MAX_RESET_TIME 0x3FF

#define X2CAL_ENABLE           1
#define X2CAL_GAP(x)           (((x)&0x3FF)<<1)
#define X2CAL_MAX_GAP   0x3FF
#define X2CAL_AVOID           (1<<31)


#define X2FRAME_HIST_ENB	0x80000000
#define X2FRAME_FRAME		0x7FFFFFFF

/* extern int GDHist_nbits_adc[4];*/
/* extern int GDHist_nbits_grade[8];*/
extern int GDHist_x2_servo_time[4];

#define X2PLCSR_CHANSEL(x) 	(((x)&0xF)<<8)
#define X2PLCSR_BTO0 		 0x1
#define X2PLCSR_BTO1 		 0x2
#define X2PLCSR_BTO2 		 0x4
#define X2PLCSR_BTO3 		 0x8
#define X2PLCSR_RESET_POL 	0x1000

#define X2PLCSR_TEST_MODE 	0x2000
#define X2PLCSR_SLOW		0x4000


#define X2_FEV_RAM_TESTPAT	   0
#define X2_FEV_RAM_RESET_TAIL  1
#define X2_FEV_RAM_QUOTIENT    2
#define X2_FEV_RAM_ROI         3
#define X2_FEV_RAM_EVENT_TAIL  4
#define X2_FEV_RAM_PWL_SERVO   5
#define X2_FEV_RAM_PILEUP_TIME 6
#define X2_FEV_RAM_RES_GRADE   7
#define X2_FEV_RAM_SERVO_TAIL  8

#define X2_FEV_RAM_MAX		  8

#define X2_FEV_TP_SIZE		 4096
#define X2_RESET_TAIL_SIZE   1024
#define X2_QUOTIENT_SIZE     1024
#define X2_ROI_SIZE          4096
#define X2_EVENT_TAIL_SIZE   1024
#define X2_PWL_SERVO_SIZE     512
#define X2_PILEUP_TIME_SIZE  2048
#define X2_RES_GRADE_SIZE    4096
#define X2_SERVO_TAIL_SIZE   1024

#define X2_FEV_TP_RESET		0x8000

#define X2_PWL_SERVO_SIZE16 2048

extern int GDHist_fev_ram_size[X2_FEV_RAM_MAX+1];
extern int GDHist_fev_ram_width[X2_FEV_RAM_MAX+1];
extern char *GDHist_fev_ram_name[X2_FEV_RAM_MAX+1];
/* Top two bits of revision will code type of global reset functionality */
#define X2_FEV_REVISION(rev) ((rev)&0x3f)

#define FREDA_REGS_START 32
#define FREDA_REGS_LAST  127

#define FREADA_WIN_LEN		0x20
#define FREADA_RST_SUB_EN   0x21
#define FREDA_VH_VL(ch)		(0x40+((ch)&0x3F))

#define FREDA_XIIC_CR			34
#define FREDA_XIIC_SR			35
#define FREDA_XIIC_TX_FIFO		36
#define FREDA_XIIC_RC_FIFO		37
#define FREDA_XIIC_ADDR			38
#define FREDA_XIIC_TX_FIFO_OCY	39
#define FREDA_XIIC_RC_FIFO_OCY	40
#define FREDA_XIIC_TEN_ADDR		41
#define FREDA_XIIC_RC_FIFO_PIRQ	42
#define FREDA_XIIC_GPO			43

#define FREDA_DAC_VH			0
#define FREDA_DAC_VTHR			1
#define FREDA_DAC_VL			2
#define FREDA_DAC_VADC			3

#define FREDA_DAC_VH_MAX		(4095*25/33)
#define FREDA_DAC_VL_MIN		(4095*5/33)
#define FREDA_DAC_VTH_MAX		(4095*25/33)
#define FREDA_DAC_VADC_NORM		(4095*16/33)

#define FREDA_IOEXP_P0_RES10(res10)		(((res10)&1)<<0)
#define FREDA_IOEXP_P0_PWRDWN(pwrdwn)	(((pwrdwn)&1)<<1)
#define FREDA_IOEXP_P0_SYSRST(do_rst)   (((~(do_rst))&1)<<2)
#define FREDA_IOEXP_P0_PDD_EN(pdd_en)	(((pdd_en)&1)<<3)
#define FREDA_IOEXP_P0_IC(ic) ((((ic)&4)<<4-2) | (((ic)&2)<<5-1) | (((ic)&1)<<6-0))
#define FREDA_IOEXP_P0_MUX(mux) (((~(mux))&8)<<7-3)
/* note PWRDWN is active low */

#define FREDA_IOEXP_P0(res10,pwrdwn,rst,pdd_en,ic,mux)  (FREDA_IOEXP_P0_RES10(res10) | FREDA_IOEXP_P0_PWRDWN(pwrdwn) | FREDA_IOEXP_P0_SYSRST(rst) | FREDA_IOEXP_P0_PDD_EN(pdd_en) | FREDA_IOEXP_P0_IC(ic) | FREDA_IOEXP_P0_MUX(mux))
#define FREDA_IOEXP_P0_DEF(mux) FREDA_IOEXP_P0(0,1,0,1,FREDA_IOEXP_IC_DEFAULT,mux)

#define FREDA_IOEXP_P1_MUX(mux) ((((~(mux))&1)<<2) | (((~(mux))&2)>>1) | (((~(mux))&4)>>1))
#define FREDA_IOEXP_P1_IB(ib)   ( (((ib)&1)<<3) | (((ib)&2)<<4) | (((ib)&4)<<2) )
#define FREDA_IOEXP_P1_SSEL(ssel) ( (((ssel)&1)<<6) | (((ssel)&4)<<5) )
#define FREDA_IOEXP_P1(mux,ib,ssel) (FREDA_IOEXP_P1_MUX(mux) | FREDA_IOEXP_P1_IB(ib) | FREDA_IOEXP_P1_SSEL(ssel))
#define FREDA_IOEXP_P1_DEF(mux,ssel) FREDA_IOEXP_P1(mux,FREDA_IOEXP_IB_DEFAULT,ssel)

#define FREDA_IOEXP_P2_SSEL(ssel) ((ssel)&2)
#define FREDA_IOEXP_P2_POL(pol)	(((pol)&1)<<2)
/* #define FREDA_IOEXP_P2_GSEL(gsel) ( (((gsel)&1)<<3) | (((gsel)&2)<<1) | (((gsel)&4)<<1) |(((gsel)&8)>>2) ) */
/* Mod forces Gsel0 high so Gsel 0 is ignored */
#define FREDA_IOEXP_P2_GSEL(gsel) ( (1<<3) | (((gsel)&2)<<4) | (((gsel)&4)<<2) |(((gsel)&8)<<3) )
#define FREDA_IOEXP_P2(ssel,pol,gsel,hyst) (FREDA_IOEXP_P2_SSEL(ssel) | FREDA_IOEXP_P2_POL(pol) | FREDA_IOEXP_P2_GSEL(gsel) | (((hyst)&1) << 7) )
#define FREDA_IOEXP_IB_DEFAULT	6
#define FREDA_IOEXP_IC_DEFAULT	5
#define FREDA_IOEXP(p0,p1,p2) ((((p2)&255)<<16) | (((p1)&255)<<8) | ((p0)&255))

#define FREDA_IOEXP_DEF(pol,gsel,ssel,mux) FREDA_IOEXP(FREDA_IOEXP_P0_DEF(mux), FREDA_IOEXP_P1_DEF(mux,ssel), FREDA_IOEXP_P2(ssel,pol,gsel,1) )


#define FREDA_IOEXP_MUX_MASK FREDA_IOEXP(FREDA_IOEXP_P0(1,1,0,1,7,15), FREDA_IOEXP_P1(15,7,7), FREDA_IOEXP_P2(7,1,15,1))
