typedef struct 
{
	volatile uint8_t pad[3], c; 
} SpreadChars;

typedef struct
{
	volatile uint8_t pad1[0x1c];
	SpreadChars cr[2];             	// Chars at 0x1F, 23
	SpreadChars manufacturer[3];	// Chars at 0x27, 2B, 2F
	SpreadChars board_id[4];     	// Chars at 0x33, 37, 3B, 3F
	SpreadChars revision[4];     	// Chars at 0x43, 47, 4B, 4F
	volatile uint8_t pad2[0x80-0x50];
} CSROM;

typedef struct _cc_regs
{
	volatile uint32_t status; 				// 0x40400
	volatile uint32_t control;				// 0x40404
	volatile uint32_t debouncea, debounceb;	// 0x40408
	volatile uint32_t clr_lost;				// 0x40410
	volatile uint32_t clr_addr;				// 0x40414
	volatile uint32_t clr_count;			// 0x40418	
	volatile uint32_t pad1;					// 0x4041c
	volatile uint32_t mon_clr_addr;			// 0x40420
	volatile uint32_t mon_clr_count;		// 0x40424
	volatile uint32_t controlb;		        // 0x40428
	volatile uint32_t controlc;		        // 0x4042C
	volatile uint8_t  pad2[0x800-0x430]; 
} CalChanRegs;

typedef struct _tf_regs
{
	volatile uint32_t status; 				// 0x40800
	volatile uint32_t control;				// 0x40804
	volatile uint32_t start;				// 0x40808  	Dataless start command
	volatile uint32_t contin;				// 0x4080c 	Dataless continue command
	volatile uint32_t clr_starts;			// 0x40810		Dataless clear starts counter command
	volatile uint32_t stop_eoc;				// 0x40814		Dataless StopEOC request command
	volatile uint32_t stop_eof;				// 0x40818 	Dataless StopEOF request command
	volatile uint32_t arm;					// 0x4081C 	Dataless arm command
	volatile uint32_t edge_locate;			// 0x40820		Dataless start edege location command		
	volatile uint32_t latch_tf_data;		// 0x40824
	volatile uint32_t pad1[2];				// 0x40828,2C
	volatile uint32_t clr_addr;				// 0x40430		Load Clear addr
	volatile uint32_t clr_count;			// 0x40434		Load Clear Count and start
	volatile uint32_t start_addr;			// 0x40838		Start Address for time frame patterns.
	volatile uint32_t pad2;					// 0x4083C
	volatile uint32_t cycles;				// 0x40840		Cycles register
	volatile uint32_t frame_inc;			// 0x40844
	volatile uint32_t port_cont;			// 0x40848		Port output control
	volatile uint32_t tfout_cont;			// 0x4084C		Frame outputs control
	volatile uint32_t start_cont;			// 0x40850		Start Control
	volatile uint32_t debouncea, debounceb;	// 0x40454, 0x58 debounce for external starts
	volatile uint32_t extra_start_cont;		// 0x4085C		Extra Start Control for I12 V2 
	volatile uint32_t debouncec, debounced;	// 0x40460, 0x64 debounce for external starts
	volatile uint32_t tfout_contb;			// 0x40868		Frame outputs control
	volatile uint32_t xspress2_cont_a;		// 0x4086C		Frame outputs control
	volatile uint32_t xspress2_cont_b;		// 0x40870		Frame outputs control

	uint8_t pad3[0x880-0x874];
	volatile uint32_t rd_cycles;			// 0x40880		Read Current cycle
	volatile uint32_t rd_frame;				// 0x40884		Read current frame
	volatile uint32_t rd_port;				// 0x40888		Read Port output
	volatile uint32_t rd_tf_addr;			// 0x4088C		Read time frame address
	volatile uint32_t rd_timer;				// 0x40890		Read Timer
	volatile uint32_t rd_starts;			// 0x40894		Read Start counter
	volatile uint32_t rd_circ_time;			// 0x40898		Read Circulation Timer
	volatile uint32_t rd_edge_pos;			// 0x4089C		Read Edge position control
	volatile uint32_t rd_ttl_tf;			// 0x408A0		Read TTL Timeframe Pins
	volatile uint32_t rd_s1_comb_tf;		// 0x408A4		Read Combined Time frame op pins.
	volatile uint32_t rd_s2_comb_tf;		// 0x408A8		Read Combined Time frame op pins.
	volatile uint32_t time_interval;		// 0x408AC		Read Time interval, internal time frame number
	volatile uint8_t  pad4[0xc00-0x8B0]; 
} TFRegs;

typedef struct _tfg2_regs
{
	CSROM csrom;
	uint8_t pad1[0x40000-0x80];
	volatile uint32_t clk_cont;
	volatile uint32_t ddr_cont;
	uint8_t pad1a[0x20-0x8];
	volatile uint32_t irq_stat;
	volatile uint32_t irq_cont;
	volatile uint32_t irq_clr;
	uint8_t pad2[0x40400-0x4002C];
	CalChanRegs cc;
	TFRegs tfg;
	CalChanRegs sp;
	volatile uint32_t s1_cont;
	volatile uint32_t s1_contb;
	volatile uint8_t pad3[0x41400-0x41008];
	volatile uint32_t s2_cont;
	volatile uint8_t pad4[0x41c00-0x41404];
	volatile uint32_t thres_dac;
	volatile uint8_t pad5[0x7ff63-0x41c04];
	volatile uint8_t f0_bar;
	volatile uint8_t pad6[0x7fff7-0x7ff64];
	volatile uint8_t bcr;
	volatile uint8_t pad7[0x7fffb-0x7fff8];
	volatile uint8_t bsr;
	volatile uint8_t pad8[0x7ffff-0x7fffc];
	volatile uint8_t bar;
} TFG2Regs;

#define TFG2CLKCONT_VCO_DIV_CONT(x)   ((x)&0x3ff)

#define TFG2CLKCONT_PLL_MUX_A(x)      (((x)&3)<<10)
#define TFG2CLKCONT_TTL_CLK_DRIVE     0x1000
#define TFG2CLKCONT_CLK_SELECT(x)   (((x)&3)<<13)	  
#define TFG2CLKCONT_GET_CLK_SELECT(x) (((x)>>13)&3)
#define TFG2CLKCONT_XTAL				0
#define TFG2CLKCONT_LVDS				1	
#define TFG2CLKCONT_PLL					2
#define TFG2CLKCONT_CLK_SELECT_MASK      0x6000
#define TFG2CLKCONT_PLL_SELECT_SR_CLK    0x8000
#define TFG2CLKCONT_GET_CLK_SELECT_IN(x) (((x)>>16)&3)
#define TFG2CLKCONT_LVDS_AVAILABLE		 0x40000
#define TFG2CLKCONT_PLL_AVAILABLE  		 0x80000
#define TFG2CLKCONT_EXTFB_LOCKED		0x100000
#define TFG2CLKCONT_INT_LOCKED			0x200000
#define TFG2CLKCONT_PLL_LOCKED			0x400000

#define TFG2S1_CLOCK_NONE               0x00000000
#define TFG2S1_CLOCK_CTF1               0x40000000
#define TFG2S1_CLOCK_CTF2               0x80000000
#define TFG2S1_CLOCK_MASK               0xC0000000

#define TFG2S2_CLOCK_NONE               0x00000000
#define TFG2S2_CLOCK_CTF3               0x40000000
#define TFG2S2_CLOCK_TTL                0x80000000
#define TFG2S2_CLOCK_MASK               0xC0000000

#define TFG2BANK_CC 0
#define TFG2BANK_TF 1
#define TFG2BANK_SP 2

/* Interrupt Status/Control and Clear */
#define TFG2IRQ_BIT_START_RUN	0x0		// Start of Run (of interest with Ext Start)
#define TFG2IRQ_BIT_END_RUN		0x1		// End of Run
#define TFG2IRQ_BIT_PAUSE		0x2		// Entering pause state
#define TFG2IRQ_BIT_CONTINUE	0x3		// Continuing from pause
#define TFG2IRQ_BIT_TF_CLEAR	0x4
#define TFG2IRQ_BIT_CC_CLEAR	0x5
#define TFG2IRQ_BIT_SP_CLEAR	0x6
#define TFG2IRQ_BIT_USER_FRAME	0x7
#define TFG2IRQ_BIT_END_CYCLE	0x8		// End of Cycle interrupt.

#define TFG2IRQ_NUMIRQS           9
#define TFG2IRQ_START_RUN	(1<<TFG2IRQ_BIT_START_RUN)	// Start of Run (of interest with Ext Start)
#define TFG2IRQ_END_RUN		(1<<TFG2IRQ_BIT_END_RUN)		// End of Run
#define TFG2IRQ_PAUSE		(1<<TFG2IRQ_BIT_PAUSE)		// Entering pause state
#define TFG2IRQ_CONTINUE	(1<<TFG2IRQ_BIT_CONTINUE)	// Continuing from pause
#define TFG2IRQ_TF_CLEAR	(1<<TFG2IRQ_BIT_TF_CLEAR)
#define TFG2IRQ_CC_CLEAR	(1<<TFG2IRQ_BIT_CC_CLEAR)
#define TFG2IRQ_SP_CLEAR	(1<<TFG2IRQ_BIT_SP_CLEAR)
#define TFG2IRQ_USER_FRAME	(1<<TFG2IRQ_BIT_USER_FRAME)
#define TFG2IRQ_END_CYCLE	(1<<TFG2IRQ_BIT_END_CYCLE)	// End of Cycle interrupt.

#define TFG2IRQ_ALLIRQ		0x1FF		// All IRQS

#define TFG2IRQCONT_ENB		0x80000000	// Global VME interrupt enable
#define TFG2IRQCONT_LEVEL(l)	(((l)&7)<<24)
#define TFG2IRQCONT_VECTOR(l)	(((l)&0xFF)<<16)
#define TFG2IRQ_VME_STAT	0x80000000	// Global VME interrupt status

typedef struct _tfmem
{
	uint16_t port;
	uint16_t control;
	uint16_t count;
} TFG2Mem;
/* TFG2 memory word is 48 bits per frame.
  This is stored in a struct of 3 uint16_t words.
  The LSWord is the rate.
  The middle word is contro and the #defined below help build it.
  The MSWord is used for use and detcontrol port info.
*/
#define TFG2MBIT_RATE      (16-16)
#define TFG2MBIT_PAUSE     (19-16)
#define TFG2MBIT_EOC       (20-16)
#define TFG2MBIT_LIVE      (21-16)
#define TFG2MBIT_TFINC     (22-16)
#define TFG2MBIT_CLRA      (22-16)
#define TFG2MBIT_INCA      (TFG2MBIT_CLRA+1)
#define TFG2MBIT_CLRB      (TFG2MBIT_CLRA+2)
#define TFG2MBIT_INCB      (TFG2MBIT_CLRA+3)
#define TFG2MBIT_PSPOL     (26-16)
#define TFG2MBIT_PSARM     (27-16)
#define TFG2NMBITS_PSARM   4  // Arm = 30..27
#define TFG2MBIT_DC0       (31-16)
#define TFG2MBIT_PORT      (32-32)
#define TFG2MBIT_USER_PORT (40-32)
                       // Note 47..40 are User Port 7..0, 39..31 are fine port or det control.
#define TFG2NMBITS_DETCON 6
/* Port layout
 top 8 bits are user port.
 Next 3 bits allocated for fine timing.
 Next 6 are detector control.
*/

#define TFG2MEM_CONTROL(rate,pause,eoc,live,tfinc,pspol,psarm,dc0) (((dc0)&1)<<TFG2MBIT_DC0 | \
   ((psarm)&15)<<TFG2MBIT_PSARM | ((pspol)&1)<<TFG2MBIT_PSPOL | ((tfinc)&15)<<TFG2MBIT_TFINC | \
   ((live)&1)<<TFG2MBIT_LIVE | ((eoc)&1)<<TFG2MBIT_EOC | ((pause)&1)<<TFG2MBIT_PAUSE | ((rate)&7)<<TFG2MBIT_RATE)

#define TFG2TFINC_CLRA      1
#define TFG2TFINC_INCA      2
#define TFG2TFINC_CLRB      4
#define TFG2TFINC_INCB      8

#define TFG2MGET_COUNT(tfmem)   ((tfmem)[0].count)
#define TFG2MGET_RATE(tfmem)    ((tfmem)[0].control & 7)
#define TFG2MGET_PAUSE(tfmem)   (((tfmem)[0].control>>TFG2MBIT_PAUSE) & 1)
#define TFG2MGET_EOC(tfmem)     (((tfmem)[0].control>>TFG2MBIT_EOC) & 1)
#define TFG2MGET_LIVE(tfmem)    (((tfmem)[0].control>>TFG2MBIT_LIVE) & 1)
#define TFG2MGET_TFINC(tfmem)   (((tfmem)[0].control>>TFG2MBIT_TFINC) & 15)
#define TFG2MGET_CLRA(tfmem)    (((tfmem)[0].control>>TFG2MBIT_CLRA) & 1)
#define TFG2MGET_INCA(tfmem)    (((tfmem)[0].control>>TFG2MBIT_INCA) & 1)
#define TFG2MGET_CLRB(tfmem)    (((tfmem)[0].control>>TFG2MBIT_CLRB) & 1)
#define TFG2MGET_INCB(tfmem)    (((tfmem)[0].control>>TFG2MBIT_INCB) & 1)
#define TFG2MGET_PSPOL(tfmem)   (((tfmem)[0].control>>TFG2MBIT_PSPOL) & 1)
#define TFG2MGET_PSARM(tfmem)   (((tfmem)[0].control>>TFG2MBIT_PSARM) & 15)
#define TFG2MGET_PORT(tfmem)    (((tfmem)[0].control>>TFG2MBIT_DC0) & 1 | (tfmem)[0].port<<1)
#define TFG2MGET_DETCON(tfmem)    (((tfmem)[0].control>>TFG2MBIT_DC0) & 1 | ((tfmem)[0].port&0x1F)<<1)
#define TFG2MGET_USER_PORT(tfmem)   (((tfmem)[0].port>>TFG2MBIT_USER_PORT) & 0xFF)
#define TFG2MGET_USER_IRQ(tfmem)    (((tfmem)[0].control>>TFG2MBIT_DC0) & 1)
                       // Note 47..40 are User Port 7..0, 39..31 are fine port or det control.
#define TFG2NMBITS_DETCON 6

/* Memory sizes
 Memories are 64 Megx16*2 = 64Meg x 32bit 
 With 32 off 32 bit locations, the Cal chan num bits tf is 2 meg, 21 bit
 each TF takes 48 bits s othe maximum number of different time intervals = 64 Meg*2/3 
 .. Needs 64 Meg.== 26 bits.
 But Could generate as many bits of TF as you like .. upto 32 bits.
 Limit Nbits TF to 26 bits
*/
#define TFG2RATE_10NS	0
#define TFG2RATE_100NS	1
#define TFG2RATE_1US	2
#define TFG2RATE_10US	3
#define TFG2RATE_100US	4
#define TFG2RATE_1MS	5
#define TFG2RATE_10MS	6
#define TFG2RATE_100MS	7


#define TFG2_NBITS_TF       26
#define TFG2_CC_NBITS_TF    21
#define TFG2_TF_MASK		((1<<TFG2_NBITS_TF)-1)
/* TFG Global control register */
#define TFG2TFCONT_ENBMEM     1		// Enable memory, take out of setup mode
#define TFG2TFCONT_DISSHORT   2		// Disable short time frame optimisation to easy/force testing

/* TFG Status register */
#define TFG2TFSTAT_RUNNING     1
#define TFG2TFSTAT_PAUSED      2
#define TFG2TFSTAT_ARMED       4
#define TFG2TFSTAT_EXTARMED    8
#define TFG2TFSTAT_CLEARING  0x8000 
#define TFG2TFSTAT_GET_EXTTRIG(s)   ((s)>>16 & 0x7fff)
#define TFG2TFSTAT_SET_EXTTRIG(tn)   (1<<((tn)+15))

/* TFG Start Control */
#define TFG2TFSTART_START_SEL(x)    ((x)&0xF)		// select external trigger 1.. 15 (0 => none)
#define TFG2TFSTART_START_MASK      0xF
#define TFG2TFSTART_START_FALLING   0x10 			// 0=> rising edge 1=> falling edge
#define TFG2TFSTART_AUTO_REARM		0x20			//  Rearm after each external start.
#define TFG2TFSTART_PAUSE_SEL(x)    (((x)&0xF)<<6) 	// select external Pause request 1.. 15 (0 => none)
#define TFG2TFSTART_PAUSE_MASK       (0xF<<6) 		
#define TFG2TFSTART_PAUSE_FALLING	0x400         	// 0=> rising edge 1=> falling edge
#define TFG2TFSTART_PAUSE_DEAD      0x800         	// Software or External Pause only in dead frames.
#define TFG2TFSTART_PAUSE_LEVEL     0x1000         	// Enable the option to make Pause signals level sensitive. Then Memory bit 39 is also used to enable this mode in a given frame
#define TFG2TFSTART_ENB_VTHRES      0x10000         // Enable Variable threshold instead of CTF1 ext start
#define TFG2TFSTART_SRCLK_DEL(x)    (((x)&0xFFF)<<20)

#define TFG2EXTSTART_BEAM_CIRC      1
#define TFG2EXTSTART_ADC0           2
#define TFG2EXTSTART_ADC1           3
#define TFG2EXTSTART_ADC2           4
#define TFG2EXTSTART_ADC3           5
#define TFG2EXTSTART_ADC4           6
#define TFG2EXTSTART_ADC5           7
#define TFG2EXTSTART_TTL0           8
#define TFG2EXTSTART_TTL1           9
#define TFG2EXTSTART_TTL2           10
#define TFG2EXTSTART_TTL3           11
#define TFG2EXTSTART_LVDS           12
#define TFG2EXTSTART_VTHRES         13	// Variable threshold  shared with Combined LVDS cable 1
#define TFG2EXTSTART_CTF1         	13	// Variable threshold  shared with Combined LVDS cable 1
#define TFG2EXTSTART_CTF2         	14
#define TFG2EXTSTART_CTF3         	15

#define TFG2ESTART_SET_ADC_ALT(adc,alt)  (((alt)&7)<<(3*(adc)))
#define TFG2ESTART_SET_CTF2_ALT(alt)     (((alt)&7)<<18)
#define TFG2ESTART_SET_CTF3_ALT(alt)     (((alt)&7)<<21)
#define TFG2ESTART_MASK    7
/* TFG TF output control */
#define TFG2TFOUT_SET_MASKWIDTH(x) ((x)&3)
#define TFG2TFOUT_USEEXTVETO       0x0008 
#define TFG2TFOUT_EXTHIGHTOVETO    0x0010
#define TFG2TFOUT_TTL1_DRIVE       0x0020 // High For weak = series term.
#define TFG2TFOUT_TTL2_DRIVE       0x0040
#define TFG2TFOUT_TTL3_DRIVE       0x0080
#define TFG2TFOUT_VETO2_INV        0x0100
#define TFG2TFOUT_XFER2_INV        0x0200
#define TFG2TFOUT_VETO3_INV        0x0400
#define TFG2TFOUT_XFER3_INV        0x0800
#define TFG2TFOUT_ENB_TFOUT3       0x1000	// Version 2 TIME FRAEM OUT 3 enable
#define TFG2TFOUT_ALTCCXFERINDEP   0x2000   // TFOutCont(13);
#define TFG2TFOUT_ALTCALCHANTF     0x4000   // <= TFOutCont(14); -- Make alternate time frame for Cal Chans, using A for normal ops and B for CalChan.
#define TFG2TFOUT_ENBREADBACK      0x8000  // Loopback to enable Readback from Combframe S2 as input
#define TFG2TFOUT_ENBTTLREADBACK   0x10000 // Enable loopback TTL readback
#define TFG2TFOUT_SET_TTLREADBACK(x) (((x)&7)<<17)
#define TFG2TFOUT_TTLRB_1OR2TO3_SS    0 // 1 or 2  readback via 3, series term to series term
#define TFG2TFOUT_TTLRB_1OR2TO3_SD    1 // 1 or 2  readback via 3, series term to direct
#define TFG2TFOUT_TTLRB_1OR2TO3_DS    2 // 1 or 2  readback via 3, direct to series term
#define TFG2TFOUT_TTLRB_1OR2TO3_DD    3 // 1 or 2  readback via 3, direct to direct
#define TFG2TFOUT_TTLRB_3TO2_SS       4 // 3 readback via 2, 1 disabled series term to series term
#define TFG2TFOUT_TTLRB_3TO2_SD       5 // 3 readback via 2, 1 disabled series term to direct
#define TFG2TFOUT_TTLRB_3TO2_DS       6 // 3 readback via 2, 1 disabled direct to series term
#define TFG2TFOUT_TTLRB_3TO2_DD       7 // 3 readback via 2, 1 disabled direct to direct

#define TFG2TFOUT_GET_TTLREADBACK(x) (((x)>>17)&7)

#define TFG2TFOUT_SET_S1_MUXCONT(x)    (((x)&63)<<20)
#define TFG2TFOUT_SET_S2_MUXCONT(x)    (((x)&63)<<26)

#define TFG2TFOUT_GET_S1_MUXCONT(x)    (((x)>>20)&0x3F)
#define TFG2TFOUT_GET_S2_MUXCONT(x)    (((x)>>26)&0x3F)
#define TFG2CTFMUX_FIXED               0x20

#define TFG2CTFMUX_EXTENDED_VETO		4	/* for use with extned veto mode on VETO, FZERO and XFER LEMOs */

#define TFG2TFOUTB_SET_EXTENDED(sect,ext)  (((ext)&7)<<((sect)*3))
#define TFG2TFOUTB_GET_EXTENDED(sect,x)  (((x)>>((sect)*3))&7)
#define TFG2TFOUTB_SET_INV_BIT(bit)         (1<<((bit)+15))
#define TFG2TFOUTB_GET_INV_BIT(bit,x)         (((x)>>((bit)+15))&1)


#define TFG2TFOUTB_EXTENDED_NORMAL 0
#define TFG2TFOUTB_EXTENDED_DC     1
#define TFG2TFOUTB_EXTENDED_MEM    2
#define TFG2TFOUTB_EXTENDED_MASK   7

/* TFG Port Control */

#define TFG2PCONT_SET_INVERT(x)   ((x)&0xFF)
#define TFG2PCONT_INVERT_MASK     0xFF 
#define TFG2PCONT_SET_DRIVE(x)    (((x)&0xFF)<<8)
#define TFG2PCONT_GET_INVERT(x)   ((x)&0xFF)
#define TFG2PCONT_GET_DRIVE(x)    (((x)&0xFF00)>>8)

/* TFG PortOut readback format */
#define TFG2POUT_SET_USER(x)   (((x)&0xFF)<<24)
#define TFG2POUT_SET_PORT(x)   (((x)&0x1FFFF)<<0)
#define TFG2POUT_GET_USER(x)   (((x)&0xFF000000)>>24)

/* TFG FrameOut readback format */
#define TFG2FOUT_SET_TF(x)   (((x)&TFG2_TF_MASK)<<0)
#define TFG2FOUT_GET_TF(x)   (((x)&TFG2_TF_MASK)>>0)
#define TFG2FOUT_LIVE         0x80000000
#define TFG2FOUT_XFER         0x40000000
#define TFG2FOUT_FRAME0       0x20000000

/* TFG TTL Readback format */
#define TFG2TTL_TF_SET_TF123(tf)   (((tf)&0x3FF)<<0)
#define TFG2TTL_TF_SET_TF2(tf)     (((tf)&0x7C00)<<0) 
#define TFG2TTL_TF_SET_TF3(tf)     (((tf)&0x7FFF)<<16)
#define TFG2TTL_TF_VETO2           0x8000
#define TFG2TTL_TF_XFER3           0x80000000
#define TFG2TTL_IGNORE_TF3         0x0000FFFF

/* Combined Time frame format */
#define TFG2COMB_TF_LIVE		   1
#define TFG2COMB_TF_XFER		   2
#define TFG2COMB_TF_SHIFT		   2

/* S1 Control register */
#define TFG2S1_CONT_TTL_VETO0_INV   0x00001
#define TFG2S1_CONT_TTL_VETO1_INV   0x00002
#define TFG2S1_CONT_TTL_VETO2_INV   0x00004
#define TFG2S1_CONT_TTL_FZERO0_INV  0x00008
#define TFG2S1_CONT_TTL_FZERO1_INV  0x00010
#define TFG2S1_CONT_TTL_XFER0_INV   0x00020
#define TFG2S1_CONT_TTL_XFER1_INV   0x00040

#define TFG2S1_CONT_TTL_VETO0_DRIVE 0x00100
#define TFG2S1_CONT_TTL_VETO1_DRIVE 0x00200
#define TFG2S1_CONT_TTL_VETO2_DRIVE 0x00400

#define TFG2S1_CONT_TTL_FZERO0_DRIVE 0x00800
#define TFG2S1_CONT_TTL_FZERO1_DRIVE 0x01000

#define TFG2S1_CONT_TTL_XFER0_DRIVE	0x02000
#define TFG2S1_CONT_TTL_XFER1_DRIVE	0x04000

#define TFG2S1_CONT_LVDS_VETO_INV   0x10000
#define TFG2S1_CONT_LVDS_F0_INV     0x20000
#define TFG2S1_CONT_LVDS_XFER_INV   0x40000

#define TFG2S1_CONT_LEDS_ON_LEMOS   0x08000000

#define TFG2S1_CONT_B_EXTENDED(m,c)  (((m)&7)<<(c)*3)

#define TFG2S1_CONT_B_EXTENDED_VETO(m,c)  (((m)&7)<<(c)*3)
#define TFG2S1_CONT_B_EXTENDED_FZERO(m,c) (((m)&7)<<((c)+3)*3)
#define TFG2S1_CONT_B_EXTENDED_XFER(m,c)  (((m)&7)<<((c)+5)*3)

#define TFG2S1_CONT_B_GET_EXTENDED_VETO(x,c)  (((x)>>(c)*3)&7)
#define TFG2S1_CONT_B_GET_EXTENDED_FZERO(x,c) (((x)>>((c)+3)*3)&7)
#define TFG2S1_CONT_B_GET_EXTENDED_XFER(x,c)  (((x)>>((c)+5)*3)&7)
#define TFG2S1_CONT_B_EXTENDED_NORMAL		0
#define TFG2S1_CONT_B_EXTENDED_DC			1
#define TFG2S1_CONT_B_EXTENDED_PORT			2

#define TFG2S1_CONT_B_HOTWAXS1  (1<<30)
#define TFG2S1_CONT_B_HOTWAXS2  (1<<31)

#define TFG2S2_CONT_EXTSTART		1
#define TFG2S2_CONT_ENB_EXTSTART	2

#define TFG2EP_EDGE_POS(x)   		(((x)>>28)&3)
#define TFG2EP_RUNNING       		0x80000000


#define TFG2CCSTAT_ACCUMULATING 	0x0001
#define TFG2CCSTAT_CLEARING 		0x0002
#define TFG2CCSTAT_INPUTREADY 		0x0004
#define TFG2CCSTAT_LOSTSYNC 		0x0008
#define TFG2CCSTAT_INP_MASK         0xFF000000
#define TFG2CCSTAT_INP_SHIFT        24

#define TFG2SPSTAT_CLEARING 0x0002

#define TFG2CC_CONT_COUNT_ENB   	0x0001
#define TFG2CC_CONT_ACC_ENB			0x0002
#define TFG2CC_CONT_TEST			0x0004

#define TFG2CC_CONT_SET_CCMODE(mode) (((mode)&7)<<4)
#define TFG2CC_CONT_GET_CCMODE(x)    (((x)>>4)&7)
#define TFG2CC_CONT_IGNORE_VETO(chan) (1<<(((chan)&7)+8))

#define TFG2CC_CONT_SET_EDGEMODE(chan,mode) (((mode)&3)<<((chan)*2+16))
#define TFG2CC_CONT_GET_EDGEMODE(x,chan) (((x)>>((chan)*2+16))&3)

#define TFG2CC_EDGEMODE_LEVEL    	0		// Count while input is high
#define TFG2CC_EDGEMODE_EDGE     	1		// Normal Rising edge mode
#define TFG2CC_EDGEMODE_INV_LEVEL 	2		// Count while input is low
#define TFG2CC_EDGEMODE_DEBOUNCE 	3		// Count Rising edges, debounced

#define TFG2CC_CONT_B_SET_EXTENDED(chan,mode) (((mode)&0xF)<<((chan)*4))
#define TFG2CC_CONT_B_GET_EXTENDED(x,chan) (((x)>>((chan)*4))&0xF)

#define TFG2CC_EXTENDED_NORMAL    0
#define TFG2CC_EXTENDED_MEM_VETO  1
#define TFG2CC_EXTENDED_EXTRA_VETO 2
#define TFG2CC_EXTENDED_MASK      0xF
#define TFG2CC_EXTENDED_ALT_INP(x)  (((x)&3)<<2)
#define TFG2CC_CONT_C_SET_EXTRA_VETOA(x) (((x)&0x1F))
#define TFG2CC_CONT_C_GET_EXTRA_VETOA(x) (((x)&0x1F))
#define TFG2CC_CONT_C_SET_EXTRA_VETOB(x) (((x)&0x1F)<<8)
#define TFG2CC_CONT_C_GET_EXTRA_VETOB(x) (((x)>>8)&0x1F)
#define TFG2CC_CONT_C_EXTRA_VETO_MASK 0x1F
#define TFG2CC_CONT_C_INV_VETOA (1<<7)
#define TFG2CC_CONT_C_INV_VETOB (1<<15)

#define TFG2CCMODE_NORMAL   0
#define TFG2CCMODE_SCALER64 1
#define TFG2CCMODE_6ADCS    2
#define TFG2CCMODE_SHORT    3
#define TFG2CCMODE_8SCALERS 5
#define TFG2CCMODE_8ADCS    6

#define TFG2CCMODE_MAX      6
#define TFG2CC_NUM_DIGITAL  10		// Maximum number of Digital Calibration Channels

#define TFG2_X2_ENABLE 		1
#define TFG2_X2A_SET_SYNC_MODE(x) (((x)&3)<<1)
#define TFG2_X2A_SET_RST_WIDTH(x) (((x)&0x7f)<<4)
#define TFG2_X2A_SET_HOLD_OFF(x)  (((x)&0x3ff)<<12)
#define TFG2_X2A_SET_ACT_DELAY(x) (((x)&0x7F)<<22)
#define TFG2_X2B_SET_ACT_WIDTH(x) ((x)&0x3FF) 
#define TFG2_X2B_SET_CIRC_OFF(x)  (((x)&0x7FF)<<12)

#define TFG2_X2_ASYNC 	0
#define TFG2_X2_SYNC_GAP  1

#define TFG2_X2A_MAX_RST_WIDTH	0x7f
#define TFG2_X2A_MAX_HOLD_OFF  	0x3ff
#define TFG2_X2A_MAX_ACT_DELAY 	0x7F
#define TFG2_X2B_MAX_ACT_WIDTH 	0x3FF
#define TFG2_X2B_MAX_CIRC_OFF  	0x7FF
