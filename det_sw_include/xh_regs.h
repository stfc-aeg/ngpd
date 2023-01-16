
#ifndef XH_REGS_H
#define XH_REGS_H

#ifdef MODULE 
#else
#include <stdint.h>
#endif
#define XH_NUM_CHAN 64
#define XH_NUM_READOUT_CHAN 32
#define XH_NUM_ADC_CHIPS 8
#define XH_NUM_ADCS_PER_CHIP 8

#define XH_XCHIP_PIXELS_PER_CHAN 32
#define XH_MAX_PIXELS 1024

#define XH_SCOPE_NUM_ADC 8

#define XH_ADC_RANGE 16384

#define XH_REGS_SIZE 1024

#define XH_DMA_STATUS		0x34
#define XH_DMA_BASE_ADDR	0x35
#define XH_DMA_BURST_LEN	0x36
#define XH_DMA_BURSTNO_MAX	0x37
#define XH_DMA_BURSTNO_CNT	0x38

#define XH_DMA_CONTROL		0x39	/* Count LWord offsets */
#define XH_DMA_ADC_PWRDWN	0x3A
#define XH_DMA_ADC_CHIPSEL	0x3B
#define XH_TRIG_CONT		0x3C
#define XH_TRIG_ORBIT_DELAY	0x3D
#define XH_TRIG_NUM_GROUPS	0x3E
#define XH_TRIG_STATUS 		0x3F
#define XH_TRIG_GROUP_NUM 	0x40
#define XH_TRIG_FRAME_NUM 	0x41
#define XH_TRIG_SCAN_NUM 	0x42
#define XH_TRIG_OUT_CONT0 	0x43
#define XH_TRIG_OUT_CONT1 	0x44
#define XH_TRIG_OUT_CONT2 	0x45
#define XH_TRIG_OUT_CONT3 	0x46
#define XH_TRIG_LED_CONT 	0x47
#define XH_XDELAY 			0x48
#define XH_HV_SUPPLY		0x49


#define XH_DEBUG_STATUS 	0x4A
#define XH_DEBUG_BURSTNO_CNT 0x4B
#define XH_TIMING_FIXED		0x4C
#define XH_ORBIT_STATUS		0x4D
#define XH_INVERT_DATA		0x4E

#define XH_ADC_FIFO_RD_DATA	0x50

#define XH_REVISION 		0x5F

#define XH_DMA_STAT_DONE	0x0001
#define XH_DMA_STAT_ERROR	0x0002
#define XH_DMA_STAT_TIMEOUT	0x0004
#define XH_DMA_STAT_IDLE	0x0008

#define XH_DMA_CONT_TEST_COUNT  0x0002

#define XH_DMA_CONT_XSTRIP     4
#define XH_DMA_CONT_16BIT      8

/* ADC Calib control, currently overlap other functions but will be moved to somewhere sensible soon */
#define XH_ADC_CALIB_CONT		0
#define XH_ADC_CALIB_STAT		1
#define XH_ADC_CALIB_STAGE1_0	2
#define XH_ADC_CALIB_STAGE1_1	3
#define XH_ADC_CALIB_STAGE2_0	4
#define XH_ADC_CALIB_STAGE2_1	5
#define XH_ADC_CALIB_TAP_DEL	6
#define XH_ADC_CALIB_EYE_WID	0x18
#define XH_ADC_CALIB_BIT_SLIP   0x2A
#define XH_ADC_CALIB_TIMERS 	0x33

#define XH_ADC_CONT_STAGE1		1
#define XH_ADC_CONT_STAGE2		2

#define XH_ADC_STAT_STAGE1(adc)	(1 <<(adc))
#define XH_ADC_STAT_STAGE2(adc)	(1 <<(adc)+8)
#define XH_ADC_STAT_WE_ALIGN    (1<<16)

#define XH_MAX_BURST_LEN_BYTES 2048

#define XH_MAX_TIMING_GROUPS 1024
#define XH_TIMING_WORDS_PER_GROUP 16
#define XH_TIMING_BRAM_SIZE (XH_MAX_TIMING_GROUPS*16*4)

#define XH_BRAM_WORD0(s1pw,nscan,s1fdel) (((s1pw)&0xFF) | ((nscan)&0x3FFFF)<<8 | ((s1fdel)&3)<<30)
#define XH_BRAM_WORD1(s2pw,nframe,s2fdel) (((s2pw)&0xFF) | ((nframe)&0x3FFFF)<<8 | ((s2fdel)&3)<<30)
#define XH_BRAM_WORD2(group_delay) (group_delay)
#define XH_BRAM_WORD3(frame_delay) (frame_delay)
#define XH_BRAM_WORD4(rst_pw) (rst_pw)
#define XH_BRAM_WORD5(s1_delay) (s1_delay)
#define XH_BRAM_WORD6(s2_delay) (s2_delay)
#define XH_BRAM_WORD7(shiftin_delay) (shiftin_delay)
#define XH_BRAM_WORD8(retrigger_delay) (retrigger_delay)
#define XH_BRAM_WORD9(aux_delay) (aux_delay)
#define XH_BRAM_WORDA(aux_pw,enb_stall2,stall2_cycle) (((aux_pw)&0xFFFF) | ((enb_stall2)&1)<<31 | ((stall2_cycle)&31)<<26)
#define XH_BRAM_WORDB(scan_delay,lemo_control) (((scan_delay)&0xFFFF) | ((lemo_control)&0xFFFF)<<16)
#define XH_BRAM_WORDC(rst_delay,xclk_half_period,pixel0_half_period,shift_down) (((rst_delay)&0xFF)<<0 | ((xclk_half_period)&0x3FF)<<8 | ((pixel0_half_period)&0x3FF)<<18 | ((shift_down)&7)<<29) 
#define XH_BRAM_WORDD(trig_control,rst_fdel_r,rst_fdel_f) ((trig_control) | ((rst_fdel_r)&3)<<30 | ((rst_fdel_f)&3)<<28)
#define XH_BRAM_WORDE(adc_settling_time,adc_settling_pixel0,num_adc_samples,xclk_fdel) (((adc_settling_time)&0x3FF)<<0 | ((adc_settling_pixel0)&0x3FF)<<10 | ((num_adc_samples)&0x3FF)<<20 | ((xclk_fdel)&3)<<30 )
#define XH_BRAM_WORDF(first,last,cycles,xclk_options)  (((first)&1)<< 31 | ((last)&1)<<30 | ((cycles)&0x3FFFF)<<0 | ((xclk_options)&0x3FF00000))

#define XH_TRIG_CONT_TRIG_MUX_SEL(x)	(((x)&0xF)<<0)
#define XH_TRIG_CONT_ENB_GROUP_TRIG		(1<<4)
#define XH_TRIG_CONT_ENB_FRAME_TRIG		(1<<5)
#define XH_TRIG_CONT_ENB_SCAN_TRIG		(1<<6)
#define XH_TRIG_CONT_TRIG_FALLING		(1<<7)
#define XH_TRIG_CONT_ORBIT_MUX_SEL(x)	(((x)&3)<<8)
#define XH_TRIG_CONT_ENB_GROUP_ORBIT	(1<<10)
#define XH_TRIG_CONT_ENB_FRAME_ORBIT	(1<<11)
#define XH_TRIG_CONT_ENB_SCAN_ORBIT		(1<<12)
#define XH_TRIG_CONT_GET_TRIG_MUX_SEL(x)	(((x)>>0)&0xF)

#define XH_TRIG_MAX_TRIG_MUX_SEL  9
#define XH_TRIG_MAX_ORBIT_MUX_SEL 3

#define XH_TRIG_SEL_ORBIT_DELAYED 8
#define XH_TRIG_SEL_SOFTWARE      9

#define XH_MAX_AUX_DELAY 0xFFFF

#define XH_RETRIGGER_LATENCY 7
/* Hardware produces 2 half periods of pixel0_half_period, then 63 of xclk_half_period = 65 total
	Software must calculate the retigger point to allow some gap before the next XClk either allowing 1 extra half period, or a fixed gap */

#define XH_XCLK_HALF_PERIODS  66
#define XH_XCLK_PERIODS  33
#define XH_XCLK_GAP			1		
/* Number of frames that can be stored in burst mode, 16/32 bit mode */
#define XH_FRAME_FIFO_SIZE32	64
#define XH_FRAME_FIFO_SIZE16	128
#define XH_XCLK_DISABLE_LAST	(1<<29)
#define XH_XCLK_ENABLE_PRE		(1<<28)
#define XH_XCLK_ENABLE_AUX1		(1<<27)
#define XH_XCLK_ENABLE_AUX_DEL	(1<<26)
#define XH_XCLK_ENABLE_STALL	(1<<25)
#define XH_XCLK_STALL_CYCLE(x)  (((x)&0x1f)<<20)

#define XH_S1_PW_MIN 5
#define XH_S1_PW_MAX 255
#define XH_S2_PW_MIN 5
#define XH_S2_PW_MAX 255
#define XH_RST_MIN 4
#define XH_S1_PW_MAX 255
#define XH_XCLK_HP_MIN 1
#define XH_XCLK_HP_MAX 1024
#define XH_XCLK_STALL_MAX 1024
#define XH_CYCLES_MAX 0x40000


#define XH_HIDE_GET_MODE(x) ((x)&0xF)
#define XH_HIDE_MODE_NONE       		0
#define XH_HIDE_MODE_MUXRESET   		1
#define XH_HIDE_MODE_RST_S1S2F  		2
#define XH_HIDE_MODE_ALL_EDGES  		3
#define XH_HIDE_MODE_NO_OVERLAP 		4
#define XH_HIDE_MODE_DELAY_MUXRESET 	5
#define XH_HIDE_MODE_DEL_MR_ALIGN   	6
#define XH_HIDE_MODE_34_XCLK_PRE    	7
#define XH_HIDE_MODE_33_XCLK_PRE    	8
#define XH_HIDE_MODE_33_XCLK_PRE_ALIGN  9
#define XH_HIDE_MODE_FULLY_OVERLAPPED   10
#define XH_HIDE_MODE_RST_S1S2F_STALL	11
#define XH_HIDE_MODE_DEL_MR_STALL   	12
#define XH_HIDE_MODE_DEL_MR_NO_OVERLAP  13
#define XH_HIDE_MODE_DEL_MR_STALL_SHORT 14
#define XH_HIDE_MODE_MAX  XH_HIDE_MODE_DEL_MR_STALL_SHORT

#define XH_HIDE_LONG_S2			0x10
#define XH_HIDE_RST_FIRST		0x20
#define XH_HIDE_NO_ADJUST		0x40

typedef struct
{
uint32_t status, group_num, frame_num, scan_num, burst_num, group_num_after;
} XHTrigStatus;

#define XH_TRIG_NUM_EXT_OUT 8
#define XH_TRIG_OUT_INVERT	1
#define XH_TRIG_OUT_SET_MODE(mode)  (((mode)&0xF)<<1)
#define XH_TRIG_OUT_MODE_DC				0
#define XH_TRIG_OUT_MODE_WHOLE_GROUP	1
#define XH_TRIG_OUT_MODE_GROUP_PRE_DEL	2	
#define XH_TRIG_OUT_MODE_GROUP_POST_DEL	3
#define XH_TRIG_OUT_MODE_FRAME_PRE_DEL	4
#define XH_TRIG_OUT_MODE_FRAME_POST_DEL	5
#define XH_TRIG_OUT_MODE_SCAN_PRE_DEL	6
#define XH_TRIG_OUT_MODE_SCAN_POST_DEL	7
#define XH_TRIG_OUT_MODE_INTEGRATION	8
#define XH_TRIG_OUT_MODE_AUX1			9
#define XH_TRIG_OUT_MODE_WAIT_TRIG	   10
#define XH_TRIG_OUT_MODE_WAIT_ORBIT	   11
#define XH_TRIG_OUT_TOGGLE	   			12

#define XH_TRIG_OUT_GET_MODE(x)  (((x)>>1)&0xF)
  
#define XH_TRIG_OUT_MAX_PW	0xFF
#define XH_TRIG_OUT_SET_PW(pw)			(((pw)&0xFF)<<8)

#define XH_LED_CONT_PAUSED(t)		(((t)&0x3FF)<<0)
#define XH_LED_CONT_FRAMING(t)		(((t)&0x3FF)<<10)
#define XH_LED_CONT_INTEGRATION(t)	(((t)&0x3FF)<<20)
#define XH_LED_CONT_DEFAULT (XH_LED_CONT_PAUSED(76) | XH_LED_CONT_FRAMING(76) | XH_LED_CONT_INTEGRATION(76))

#define XH_LED_CONT_INCLUDE_ORBIT	(1<<31)

#define XH_TRIG_CONT_START 1
#define XH_TRIG_CONT_CONTINUE 2
#define XH_TRIG_XCHIP3_SETUP_MODE (1<<31)
#define XH_TRIG_XCHIP3_SETUP_DATA (1<<30)


#define XH_TRIG_STAT_IDLE			1
#define XH_TRIG_STAT_S_ORBIT		2
#define XH_TRIG_STAT_F_ORBIT		4
#define XH_TRIG_STAT_G_ORBIT		8
#define XH_TRIG_STAT_S_EXTTRIG	 0x10
#define XH_TRIG_STAT_F_EXTTRIG	 0x20
#define XH_TRIG_STAT_G_EXTTRIG	 0x40

#define XH_TRIG_STAT_EXT_TRIG_MASK 	0x7F000000
#define XH_TRIG_STAT_ORBIT_TRIG_REG	0x80000000

#define XH_TRIG_STAT_MASK 0x7F

#define XH_ORBIT_DELAY_SET(x)  (((x)&0x3FFFFFF)<<0)
#define XH_ORBIT_FALLING       (1<<31)
#define XH_ORBIT_LOCATE_EDGE   (1<<30)
#define XH_ORBIT_EDGE_SEL_AUTO	0
#define XH_ORBIT_EDGE_SEL_FIXED(x) ((((x)&3)<<28) | (1<<27))

#define XH_ORBIT_DELAY_MAX 0x3FFFFFF

#define XH_ORBIT_STAT_GET_C0(x)  ((x)&0x7F)
#define XH_ORBIT_STAT_GET_C1(x)  (((x)>>7)&0x7F)
#define XH_ORBIT_STAT_GET_C2(x)  (((x)>>14)&0x7F)
#define XH_ORBIT_STAT_GET_C3(x)  (((x)>>21)&0x7F)
#define XH_ORBIT_STAT_GET_SEL(x) (((x)>>28)&0x3)
#define XH_ORBIT_STAT_GET_RUN(x) (((x)>>31)&1)

#define XH_XDELAY_DEFAULT	37
#define XH_XDELAY_MAX  		63

#define XH_ADCPD_POWER_DOWN(adcs) ((adcs)&0xFF)
#define XH_ADCPD_CLK_SYNC		(1<<8)
#define XH_ADCPD_CLK_GOE		(1<<9)
#define XH_ADCPD_CLK_LOCKED		(1<<10)
#define XH_ADCPD_CLK_MUX_SEL	(1<<11)
#define XH_ADCPD_HEAD_POWER		(1<<16)
#define XH_ADCPD_CLK_LDD1		(1<<30)
#define XH_ADCPD_CLK_LDD2		(1<<31)

#define XH_TIMING_FIXED_DELAY_READOUT (1<<31)	// Trigger shiftin from Retrig signal to make delayed readout.
#define XH_TIMING_FIXED_USE_FIXED_HP  (1<<30)	// Used Fixed XClk half period from this register
#define XH_TIMING_FIXED_FIXED_HP(hp) (((hp)&0x1F)<<10)	// Fixed XClk Half periof (-1) value of 0 => 1 cycle delay.
#define XH_TIMING_FIXED_STALL2_TIME(st) (((st)&0x1F)<<15)	//!< Length of gap2 in Xclk if stall enabled

#define XH_TIMING_FIXED_STALL_TIME(st) (((st)&0x3FF)<<20)	//!< Length of gaps in Xclk if stall enabled
#define XH_TIMING_MAX_FIXED_HIGH	31			//!< Max length of fixed high


#define XH_INL_BRAM_NUM 32			// Number of INL correction BRAMs, currently 1 per ADC channel
#define XH_INL_BRAM_WORDS	4096	// Currently top 12 bits of ADC address BRAM, which is 9 bits wide on 32 bit boundaries.

#define XH_REVISION_MAJOR(x) (((x)>>16)&0xFFFF)
#define XH_REVISION_MINOR(x) (((x)>>0)&0xFFFF)
#endif
