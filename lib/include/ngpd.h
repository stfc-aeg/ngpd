/*
 * ngpd.h
 *
 *  Created on: 19 Mar 2018
 *      Author: wih73
 */
#ifndef NGPD_H_
#define NGPD_H_
 
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <pthread.h>
//#include <sched.h>

#ifdef __MINGW32__
#define U_INTX_T 1
typedef uint8_t  u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;
#endif
#include "datamod.h"
#include "ngzmp.h"
/**
	@defgroup NGPD_MACROS   Macros for Neturon Gamma Discriminator
*/

#define NGPD_OK (0)
#define NGPD_ERROR (-1)
#define NGPD_ERROR_OVERRUN (-2)

#define NGPD_MAX_PATH 10		//!< Maximum number of paths, may extend to handle multiple cards

#define NGPD_MAX_ERROR_MESSAGE 200
#define NGPD_MAX_MODNAME 80
#define NGPD_MODNAME_SPACE 82		//!< Space is 2 bytes longer to allow null and preserve at least 16 bit alignment

extern char ngpd_error_message[NGPD_MAX_ERROR_MESSAGE+2];

/**
@defgroup LIMITS_AND_SIZES Limit and sizes for buffers etc.
@ingroup NGPD_MACROS
*/
#define NGPD_MAX_NUM_CARDS	8
#define NGPD_MAX_CHANS		64		
#define NGPD_MAX_CHANS_PER_CARD		8
#define NGPD_MAX_CARD_INDEX 62		//!< Maximum card index across all systems

#define NGPD_MAX_BUFFER 	(128*1024)
#if 0
#define NGPD_DRAM_FIRST_ADDR		0
#define NGPD_SCOPE_NBYTES_PER_CARD (1024*1024*1024)		// 1 GByte per card of DRAM used for scope mode.
#define NGPD_SCOPE_NUM_T 	(512*1024*1024) 		// Just over 1 s at 500 MSa/s
#define NGPD_PLAYBACK_NBYTES_PER_CARD (1024*1024*1024)		// 1 GByte per card of DRAM used for playback.
#define NGPD_PLAYBACK_NUM_T 	(512*1024*1024) 			// Just over 1 s at 500 MSa/s
#else
#define NGPD_DRAM_FIRST_ADDR		32768
#define NGPD_SCOPE_NBYTES_PER_CARD (1022*1024*1024)		// 1 GByte per card of DRAM used for scope mode - 2 Mbytes discard at bottom .
#define NGPD_SCOPE_NUM_T 	(511*1024*1024) 		// Just over 1 s at 500 MSa/s
#define NGPD_PLAYBACK_NBYTES_PER_CARD (1022*1024*1024)		// 1 GByte per card of DRAM used for playback.
#define NGPD_PLAYBACK_NUM_T 	(511*1024*1024) 			// Just over 1 s at 500 MSa/s
#endif
#define NGPD_FILT_NUM_COEFF		51						//!< Number of coefficients to load into the filter see also 
#define NGPD_MAX_MOD_NAME		1026
/**@} */


/**
@defgroup ADDR_OFFSETS Address offsets
@ingroup NGPD_MACROS
@{
*/
#define NGPD_REGION_REGS 			0		//!< Control registers
#define NGPD_REGION_DPRAM 			1		//!< DP RAMS 
#define NGPD_GLOB_REVISION 			16
#define NGPD_GLOB_SCOPE_STATUS 		17
#define NGPD_GLOB_CYCLES_STATUS 	18

#define NGPD_GLOB_BASE	64
#define NGPD_GLOB_NUM	32
#define NGPD_CHAN_BASE	 0
#define NGPD_CHAN_NUM	32

#define NGPD_DATA_PATH				(NGPD_GLOB_BASE+0)

#define NGPD_UPLOAD_FIRST_ADDR		(NGPD_GLOB_BASE+4)
#define NGPD_UPLOAD_LAST_ADDR		(NGPD_GLOB_BASE+5)
#define NGPD_UPLOAD_WORD_COUNT		(NGPD_GLOB_BASE+6)
#define NGPD_UPLOAD_DATA			(NGPD_GLOB_BASE+7)
#define NGPD_PLAYBACK_FIRST_ADDR	(NGPD_GLOB_BASE+8)
#define NGPD_PLAYBACK_LAST_ADDR		(NGPD_GLOB_BASE+9)
#define NGPD_PLAYBACK_SKIP			(NGPD_GLOB_BASE+10)

#define NGPD_SCOPEOUT_FIRST_ADDR	(NGPD_GLOB_BASE+11)
#define NGPD_SCOPEOUT_LAST_ADDR		(NGPD_GLOB_BASE+12)
#define NGPD_SCOPEOUT_PAUSE_WORDS	(NGPD_GLOB_BASE+13)

#define NGPD_SCOPEOUT_SKIP			(NGPD_GLOB_BASE+14)

#define NGPD_SCOPE_FIRST_ADDR		(NGPD_GLOB_BASE+15)
#define NGPD_SCOPE_LAST_ADDR		(NGPD_GLOB_BASE+16)
#define NGPD_SCOPE_CONT				(NGPD_GLOB_BASE+17)

#define NGPD_SCOPE_CHAN_SEL			(NGPD_GLOB_BASE+18)
#define NGPD_SCOPE_SRC_SEL			(NGPD_GLOB_BASE+19)
#define NGPD_SCOPE_ALT_SEL			(NGPD_GLOB_BASE+20)

#define NGPD_ITFG_TIME				(NGPD_GLOB_BASE+24)
#define NGPD_ITFG_TRIG_MODE			(NGPD_GLOB_BASE+25)
#define NGPD_ITFG_CYCLES			(NGPD_GLOB_BASE+26)

#define NGPD_SCOPE_ALT_SEL			(NGPD_GLOB_BASE+20)
#define NGPD_CHAN_CONT				0
#define NGPD_CHAN_FILTER			1
#define NGPD_CHAN_DIFF_TRIG_A		2
#define NGPD_CHAN_DIFF_TRIG_B		3
#define NGPD_CHAN_DIFF_TRIG_C		3
#define NGPD_CHAN_BASESUB_A			6
#define NGPD_CHAN_BASESUB_B			7

#define NGPD_CHAN_ABS_TRIG_A		8

#define NGPD_CHAN_MEASURE_A			12
#define NGPD_CHAN_MEASURE_B			13
#define NGPD_CHAN_MEASURE_C			14
#define NGPD_CHAN_MEASURE_D			15
#define NGPD_CHAN_MEASURE_E			16
#define NGPD_CHAN_DAE_PULSE			17
/* DPRAM numbers */
#define NGPD_CHAN_DPADDR			0
#define NGPD_CHAN_MEASURE_TAIL_THRES_M 1
#define NGPD_CHAN_MEASURE_TAIL_THRES_C 2
#define NGPD_CHAN_MEASURE_TAIL_SUB_EVEN 3
#define NGPD_CHAN_MEASURE_TAIL_SUB_ODD 4


/* @} */

/** 
*defgroup NGPD_RUN_FLAGS
@ingroup NGPD_MACROS
@{
*/

typedef enum {
	NGPD_RUN_FLAGS_PLAYBACK				= (1<<0),
	NGPD_RUN_FLAGS_PLAYBACK_ALT			= (2<<0),
	NGPD_RUN_FLAGS_PLAYBACK_CONTINUOUS	= (1<<2),
	NGPD_RUN_FLAGS_SCOPEMODE			= (1<<4),
	NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS	= (1<<5),
	NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD	= (1<<6),
	NGPD_RUN_FLAGS_SCOPE_OUT_TEST		= (1<<15),
	NGPD_RUN_FLAGS_SCOPE_OUT_2_STREAM	= (1<<16),

	NGPD_RUN_FLAGS_DEFAULT				= (NGPD_RUN_FLAGS_SCOPE_OUT_2_STREAM),
	NGZMP_RUN_FLAGS_DEFAULT				= (0)
} NGPDRunFlags;
/* @} */

/** 
*defgroup NGPD_READ_SCOPE_OPTIONS
@ingroup NGPD_MACROS
@{
*/

#define NGPD_READ_SCOPE_NO_CHUNK			(1<<0)
#define NGPD_READ_SCOPE_FLOW_CONTROL		(1<<1)
#define NGPD_READ_SCOPE_RESET_EACH_RUN		(1<<2)
#define NGPD_READ_SCOPE_RESET_LONG_ABORT	(1<<3)
/* @} */

/** 
*defgroup NGPD_DATA_PATH_BITS
@ingroup NGPD_MACROS
@{
*/
#define NGPD_DP_DATA_SRC_INPUT(x)		((x)&3)		//!< Data Source
#define NGPD_DP_GET_DATA_SRC_INPUT(x)	((x)&3)		//!< Data Source
#define NGPD_DP_PLAYBACK_ENB	(1<<4)		//!< Enable playback DMA
#define NGPD_DP_PLAYBACK_CONT	(1<<5)		//!< Make playback loop continuously
#define NGPD_DP_DATA_SRC_OUTPUT(x)		(((x)&3)<<8)		//!< Data Source
#define NGPD_DP_GET_DATA_SRC_OUTPUT(x)	(((x)>>8)&3)		//!< Data Source

#define NGPD_DP_DATA_SRC_INP_ADC		0		//!< Normal run mode using dat from the ADC
#define NGPD_DP_DATA_SRC_INP_PLAYBACK	1		//!< Playback data from the DRAM DMA interface
#define NGPD_DP_DATA_SRC_INP_PLAYBACK_ALT	2

#define NGPD_DP_DATA_SRC_OUT_ADC		0		//!< Normal run mode using dat from the ADC
#define NGPD_DP_DATA_SRC_OUT_SCOPE_OUT	1		//!< Output scope mode data saved in DRAM
#define NGPD_DP_DATA_SRC_OUT_RUN		2		//!< Normal Run Mode.
#define NGPD_DP_DATA_SRC_OUT_TEST_PAT	3		//!< Test pattern generator but with timing generated from scope output DMA engine.

#define NGPD_DP_RUN_TRIG_FROM_RUN	(1<<29)		//!< Teledyne Trigger from rinsing edge of run.
#define NGPD_DP_RUN_SCOPE_OUTPUT	(1<<30)		//!< Overall Run Scope output
#define NGPD_DP_RUN					(1<<31)		//!< Overall Run bit



/* @} */

/** 
@defgroup NGPD_SCOPE_MODE
@ingroup NGPD_MACROS
@{
*/

#define NGPD_SCOPE_CONT_NUM_STREAMS(x)		((x)&3)
#define NGPD_SCOPE_CONT_GET_NUM_STREAMS(x)	((x)&3)
#define NGPD_SCOPE_CONT_ENABLE				(1<<31)

#define NGPD_SCOPE_NUM_STREAMS1				0
#define NGPD_SCOPE_NUM_STREAMS2				1
#define NGPD_SCOPE_NUM_STREAMS4				2
#define NGPD_SCOPE_NUM_STREAMS6				3

#define NGPD_SCOPE_CHAN_SEL_SET(s,x)	(((x)&0xF)<<4*(s))
#define NGPD_SCOPE_CHAN_SEL_GET(s,x)	(((x)>>4*(s))&0xF)


#define NGPD_SCOPE_SRC_SEL_SET(s,x)		(((x)&0xF)<<4*(s))
#define NGPD_SCOPE_SRC_SEL_GET(s,x) 	(((x)>>4*(s))&0xF)

#define NGPD_SCOPE_ALTERNATE_SET(s,x)	(((x)&0xF)<<4*(s))
#define NGPD_SCOPE_ALTERNATE_GET(s,x) 	(((x)>>4*(s))&0xF)

#define NGPD_SCOPE_NUM_SRC0				12
#define NGPD_SCOPE_NUM_SRC1				16
#define NGPD_SCOPE_NUM_SRC2				12
#define NGPD_SCOPE_NUM_SRC3				16
#define NGPD_SCOPE_NUM_SRC4				11
#define NGPD_SCOPE_NUM_SRC5				16

#define NGPD_SCOPE_MAX_STREAMS 6

#define NGPD_SCOPE_SEL0TO5_TEST_PAT			0
#define NGPD_SCOPE_SEL0TO5_INPUT			1
#define NGPD_SCOPE_SEL0TO5_FILTER			2
#define NGPD_SCOPE_SEL0TO5_DIFF_TRIG_DIFF	3
#define NGPD_SCOPE_SEL0TO5_DIFF_TRIG_OUT	4
#define NGPD_SCOPE_SEL0TO5_BASELINE_SUB		5
#define NGPD_SCOPE_SEL0TO5_BSUB_INTERNAL	6
#define NGPD_SCOPE_SEL0TO5_ABS_TRIG			7
#define NGPD_SCOPE_SEL0TO5_MEASURE			8
#define NGPD_SCOPE_SEL0TO5_DATA_OUTPUT		9
#define NGPD_SCOPE_SEL0TO5_TAIL_SUB			10

#define NGPD_SCOPE_SEL5_DIG_3BIT		14
#define NGPD_SCOPE_SEL5_DIG_5BIT		15

typedef enum {
NGPD_SCOPE_STATUS_RUNNING			= (1<<0),
NGPD_SCOPE_STATUS_SCOPEOUT_RUNNING	= (1<<1),
NGPD_SCOPE_STATUS_SCOPEOUT_PAUSED	= (1<<2),
NGPD_SCOPE_STATUS_ITFG_RUNNING		= (1<<3),
NGPD_SCOPE_STATUS_ITFG_WAITING		= (1<<4),
NGPD_SCOPE_STATUS_ITFG_COUNTING		= (1<<5)
} NGPDScopeStatus;
#define NGPD_FILTER_STATUS_RELOAD_TREADY	(1<<16)
#define NGPD_FILTER_STATUS_TLAST_MISSING	(1<<17)
#define NGPD_FILTER_STATUS_TLAST_UNEXPECTED	(1<<18)
#define NGPD_FILTER_STATUS_MISSING_TREADY	(1<<19)		//!< Not from hardware, software uses this bit to mark when tready is not asserted, but should have been.


#define NGPD_SCOPE_SKIP_SET(x)			((x)&0x3F)
#define NGPD_SCOPE_SKIP_GET(x)			((x)&0x3F)
#define NGPD_SCOPE_SKIP_TWO_STREAM		(1<<31)
#define NGPD_SCOPE_SKIP_CONTINUE		(1<<30)
#define NGPD_SCOPE_SKIP_RESET_EACH_RUN	(1<<29)
#define NGPD_SCOPE_SKIP_LONG_ABORT		(1<<28)

#define NGPD_SCOPEOUT_SKIP_MAX		63



#define NGZMP_SCOPE_NUM_SRC_ANA			11
#define NGZMP_SCOPE_NUM_SRC_S3			16
#define NGZMP_SCOPE_NUM_SRC_S7			16
#define NGZMP_SCOPE_NUM_SRC_S8			15
#define NGZMP_SCOPE_NUM_SRC_S9			16

/* @} */

/** 
@defgroup NGPD_ITFG_MODE
@ingroup NGPD_MACROS
@{
*/
#define NGPD_ITFG_SET_TRIG_MODE(x)		((x)&7)
#define NGPD_ITFG_GET_TRIG_MODE(x)		((x)&7)


/** @} */
/** 
@defgroup NGPD_CHAN_REGS_MACROS
@ingroup NGPD_MACROS
@{
*/

#define NGPD_CC_USE_PB_START				(1<<0)			//!< Use Start trigger derived from playback data
#define NGPD_FILTER_COEF(x)					((x)&0x3FFFF)
#define NGPD_FILTER_FORCE_CONF				(1<<30)
#define NGPD_FILTER_TLAST					(1<<31)

#define NGPD_DIFF_TRIG_A_THRES(x)			((x)&0xFFFF)
#define NGPD_DIFF_TRIG_A_SEP(x)				(((x)&0xFF)<<16)
#define NGPD_DIFF_TRIG_A_DATA_DELAY(x)		(((x)&0xFF)<<24)
#define NGPD_DIFF_TRIG_B_TRIG_DELAY(x)		(((x)&0x3F)<<0)
#define NGPD_DIFF_TRIG_B_STRETCH_A(x)		(((x)&0xFF)<<8)
#define NGPD_DIFF_TRIG_B_DELAY_A(x)			(((x)&0x1F)<<24)
#define NGPD_DIFF_TRIG_C_STRETCH_B(x)		(((x)&0xFF)<<8)
#define NGPD_DIFF_TRIG_C_DELAY_B(x)			(((x)&0x1F)<<24)

#define NGPD_DIFF_TRIG_A_GET_THRES(x)		((x)&0xFFFF)
#define NGPD_DIFF_TRIG_A_GET_SEP(x)			(((x)>>16)&0xFF)
#define NGPD_DIFF_TRIG_A_GET_DATA_DELAY(x)	(((x)>>24)&0xFF)
#define NGPD_DIFF_TRIG_B_GET_TRIG_DELAY(x)	(((x)>>0)&0x3F)
#define NGPD_DIFF_TRIG_B_GET_STRETCH_A(x)	(((x)>>8)&0xFF)
#define NGPD_DIFF_TRIG_B_GET_DELAY_A(x)		(((x)>>24)&0x1F)
#define NGPD_DIFF_TRIG_C_GET_STRETCH_B(x)	(((x)>>8)&0xFF)
#define NGPD_DIFF_TRIG_C_GET_DELAY_B(x)		(((x)>>24)&0x1F)

#define NGPD_DIFF_TRIG_MIN_SEP				2
#define NGPD_DIFF_TRIG_MAX_SEP				(2+63)

#define NGPD_DIFF_TRIG_MIN_DATA_DELAY		0
#define NGPD_DIFF_TRIG_MAX_DATA_DELAY		63

#define NGPD_DIFF_TRIG_MIN_TRIG_DELAY		0
#define NGPD_DIFF_TRIG_MAX_TRIG_DELAY		63

#define NGPD_NUM_STRETCHED_TRIG				2

#define NGPD_DIFF_TRIG_MIN_DELAY_AB			0
#define NGPD_DIFF_TRIG_MAX_DELAY_AB			31

#define NGPD_DIFF_TRIG_MIN_TRIG_STRETCH		1
#define NGPD_DIFF_TRIG_MAX_TRIG_STRETCH		0xFF

#define NGPD_DIFF_TRIG_MIN_THRES			0
#define NGPD_DIFF_TRIG_MAX_THRES			0xFFFF

#define NGPD_BASELINE_A_FIXED(x)			(((x)&0x3FFFF)<<0)
#define NGPD_BASELINE_A_USE_FIXED			(1<<31)
#define NGPD_BASELINE_A_DIV_CONT(x)			(((x)&0x7)<<24)
#define NGPD_BASELINE_B_ERR_LIM(x)			(((x)&0xFFFF)<<0)

#define NGPD_BASELINE_A_GET_FIXED(x)		(((x)>>0)&0x3FFFF)
#define NGPD_BASELINE_A_GET_DIV_CONT(x)		(((x)>>24)&0x7)
#define NGPD_BASELINE_B_GET_ERR_LIM(x)		(((x)>>0)&0xFFFF)


#define NGPD_ABS_TRIG_THRES(x)				(((x)&0xFFFF)<<0)

#define NGPD_MEAS_A_USE_ABS_TRIG			(1<<0)				//!< Use absolute level (above baseline) for measurement trigger rather than differential.
#define NGPD_MEAS_A_ADAPTIVE_TAIL_SUM		(1<<1)				//!< Enable adaptive tail sum where next trigger stops tail sum early.
#define NGPD_MEAS_A_IGNORE_FALL_TIME		(1<<2)				//!< Ignore faill time when performing neutron/gamma discrimination (usually leaving hgt and tail sum)
#define NGPD_MEAS_A_IGNORE_TAIL_SUM			(1<<3)				//!< Ignore tail sum when performing neutron/gamma discrimination (usually leaving hgt and fall time)
#define NGPD_MEAS_A_IGNORE_ENB_TAIL_SUB		(1<<4)				//!< Enable Pulse Tail subtraction 
#define NGPD_MEAS_A_IGNORE_TAIL_SUB_TEST	(1<<5)				//!< Enable Pulse Tail subtraction test mode, where subtraction starts imediately
#define NGPD_MEAS_A_IGNORE_TAIL_SUB_TEST_N	(1<<6)				//!< In tail subtratcion test mode assume all pulses are neutrons rather than gammas.

#define NGPD_MEAS_B_TAIL_FRAC(x)			(((x)&0x3FFFF)<<0)	//!< Level as fraction of 2^18 to end the fall time counting. 
#define NGPD_MEAS_C_SUM_DELAY(x)			(((x)&0xFF)<<0)		//!< Delay in samples from trigger to first sample sumed in tail sum
#define NGPD_MEAS_C_SUM_NUM(x)				(((x)&0xFF)<<8)		//!< Number of samples to be summed in tail sum.
#define NGPD_MEAS_D_MIN_HEIGHT(x)			(((x)&0xFFFF)<<0)	//!< Minimum Height to be consider a neutron, aligned to 16 bit ADC values.
#define NGPD_MEAS_D_MAX_HEIGHT(x)			(((x)&0xFFFF)<<16)	//!< Maxmimum Height to be consider a neutron
#define NGPD_MEAS_E_MIN_FALL_TIME(x)		(((x)&0xFF)<<0)		//!< Minimum Fall time to be consider a neutron in samples.
#define NGPD_MEAS_E_MAX_FALL_TIME(x)		(((x)&0xFF)<<8)		//!< Maxmimum Fall time to be consider a neutron in samples.
#define NGPD_MEAS_E_MIN_TAIL_COUNT(x)		(((x)&0xFF)<<24)		//!< Minimum number of samples in adpative tail sum to to be consider a neutron in samples.
#define NGPD_MEAS_F_TAIL_THRES(x)			(x)					//!< Tail Sum must be >= this to be considered  a neutron. Aligned to 16 bit ADC units.

#define NGPD_MEASURE_SUM_DELAY_MAX 			0xFF
#define NGPD_MEASURE_SUM_NUM_MAX 			0xFF
#define NGPD_MEASURE_HEIGHT_MAX 			0xFFFF
#define NGPD_MEASURE_FALL_TIME_MAX 			0xFF
#define NGPD_MEASURE_TAIL_COUNT_MAX			0xFF

#define NGPD_MEAS_B_GET_TAIL_FRAC(x)		(((x)>>0)&0x3FFFF)	//!< Level as fraction of 2^18 to end the fall time counting. 
#define NGPD_MEAS_C_GET_SUM_DELAY(x)		(((x)>>0)&0xFF)		//!< Delay in samples from trigger to first sample sumed in tail sum
#define NGPD_MEAS_C_GET_SUM_NUM(x)			(((x)>>8)&0xFF)		//!< Number of samples to be summed in tail sum.

#define NGPD_MEAS_D_GET_MIN_HEIGHT(x)		(((x)>>0)&0xFFFF)	//!< Minimum Height to be consider a neutron, aligned to 16 bit ADC values.
#define NGPD_MEAS_D_GET_MAX_HEIGHT(x)			(((x)>>16)&0xFFFF)	//!< Maxmimum Height to be consider a neutron
#define NGPD_MEAS_E_GET_MIN_FALL_TIME(x)	(((x)>>0)&0xFF)		//!< Minimum Fall time to be consider a neutron in samples.
#define NGPD_MEAS_E_GET_MAX_FALL_TIME(x)	(((x)>>8)&0xFF)		//!< Maxmimum Fall time to be consider a neutron in samples.
#define NGPD_MEAS_E_GET_MIN_TAIL_COUNT(x)	(((x)>>24)&0xFF)		//!< Minimum number of samples in adpative tail sum to to be consider a neutron in samples.
#define NGPD_MEAS_F_GET_TAIL_THRES(x)		(x)					//!< Tail Sum must be >= this to be considered  a neutron. Aligned to 16 bit ADC units.

#define NGPD_MEASURE_TAIL_THRES_SIZE		256					//!< Number of values to write to tail thres BRAM
#define NGPD_MEASURE_TAIL_SUB_SIZE			1024				//!< Number of values to write to each half of tail subtract BRAMs

#define NGPD_REGION_MAX_SIZE				1024				//!< Max region size for save/restore settings
/** @} */
/** 
@defgroup NGPD_EVENT_INFO_MACROS
@ingroup NGPD_MACROS
@{
*/
#define NGPD_EVENT_MAX_HEIGHT		0x1FFFF
#define NGPD_EVENT_MAX_TAIL_SUM		((1<<26)-1)
#define NGPD_EVENT_MAX_FALL_TIME	0xFF
#define NGPD_EVENT_SOF				0x40000000
#define NGPD_EVENT_EOF				0x80000000
#define NGPD_EVENT01_GET_TS(a, b)	(((a)&0x3FFFFFFF) | (((b)&0x1FFFFF)<<30))
#define NGPD_EVENT1_GET_COUNT(c)	(((c)>>21)&0xFF)
#define NGPD_EVENT1_SUM_ABORTED		(1<<29)
#define NGPD_EVENT2_GET_HGT(c)		((c)&0x3FFFF)
#define NGPD_EVENT2_GET_FALL(c)		(((c)>>18)&0xFF)
#define NGPD_EVENT2_FALL_VALID		(1<<26)
#define NGPD_EVENT2_TAIL_VALID		(1<<27)
#define NGPD_EVENT2_PILEUP			(1<<28)
#define NGPD_EVENT2_HGT_IN_WIN		(1<<29)
#define NGPD_EVENT3_FALL_IN_WIN		(1<<27)
#define NGPD_EVENT3_TAIL_GT_THRES	(1<<28)
#define NGPD_EVENT3_NEUTRON			(1<<29)

#define NGPD_EVENT3_GET_TAIL(d)		((d)&0x7FFFFFF)
#define NGPD_EVENT3_TAIL_SIGN		0x4000000

/** @} */

#define NGPD_ADQ14_TRANSFER_SIZE_BYTES		1024

#define NGPD_SCOPE_MODULE_MAGIC "ngpdscp"
#define NGPD_SCOPE_LAYOUT_1		0			//!< 1 streams 
#define NGPD_SCOPE_LAYOUT_2		1			//!< 2 streams
#define NGPD_SCOPE_LAYOUT_4		2			//!< 4 streams 
#define NGPD_SCOPE_LAYOUT_6		3			//!< 6 streams

#define NGZMP_SCOPE_LAYOUT_4		10			//!< 4 streams for NGZMP, DL559722
#define NGZMP_SCOPE_LAYOUT_8		11			//!< 8 streams 
#define NGZMP_SCOPE_LAYOUT_10		12			//!< 10 streams 

#define NGPD_SCOPE_CHUNK_NUM_T	65536			//!< for NGZMP using 1G ethernet scope mode is read in chunks and valid flag bits made.
typedef enum {NGPDGenError= -1, NGPDGenADQ14=0, NGPDGenZynqMP1} NGPDGeneration; 

typedef struct ngpd_scope_data_module
{
	struct ngpd_scope_module_head
	{
		char magic[8];
		int32_t layout;				//!< Module layout to differentiate between any future versions
		int32_t num_cards;			//!< Number of ADQ cards contained within the module. 	
		int32_t hw_version;			//!< hardware version in case we need to change interpretation of values.
		size_t nbytes_per_card;			//!< Number of bytes per card
		int num_t;					//!< Number of time points to use.
		struct
		{
			u_int32_t chan_sel[2], src_sel[2], alternate[2];	//!< Scope mode registers to be interpreted from ngpd.h
			size_t valid_offset;
		} card[NGPD_MAX_NUM_CARDS];
	} head;
	u_int16_t _data[1];			//!< Beginning of data, but use {@link ngpd_scope_mod_get_ptr()} for access
}	NGPDScopeModule;

typedef enum
{
		NGPDDataSrcADCFWD=0,
		NGPDDataSrcADCRev=1,
		NGPDDataSrcPlayback=4
} NGPDDataSrc;


typedef struct ngpd_chan_cont_t 
{
	int use_pb_start;
	int data_src;
	int inv_data;
} NGPDChanCont;

typedef struct  ngpd_diff_trig_t
{
	int thres, sep, data_delay, trig_delay;
	int delay_a, width_a;		// Delay and stretch for the slow/wide trigger A
	int delay_b, width_b;		// Delay and stretch for the slow/wide trigger B, used to mask accumulation in the baseline subtractor.

} NGPDDiffTrigger;

typedef struct  ngpd_base_sub_t
{
	int use_fixed;
	int fixed;
	int error_limit;
	int div_cont;
} NGPDBaseSubtract;

typedef struct ngpg_test_pattern_t
{
	enum {NGPDTP_Ramp, NGPDTP_Delta, NGPDTP_Square} type;
	int i_baseline, i_height, i_period;
	int num_t;
} NGPDTestPattern;

typedef struct ngpd_measure_t
{
	int use_abs_trig;
	int adaptive_tail_sum;
	int ignore_fall_time;
	int ignore_tail_sum;
	int enable_tail_subtract;
	int tail_subtract_test;
	int tail_subtract_test_neutron;
	int tail_sum_delay, tail_sum_num;
	double fall_time_frac;
	int min_height, max_height;
	int min_fall_time, max_fall_time;
	int min_tail_count;
	int tail_thres_m[NGPD_MEASURE_TAIL_THRES_SIZE];
	int tail_thres_c[NGPD_MEASURE_TAIL_THRES_SIZE];
} NGPDMeasure ;
 
typedef enum 
{
	NGPDHistEnbHeight=1, 
	NGPDHistEnbTailSum=2,
	NGPDHistEnbFallTime=4,
	NGPDHistEnbTailRatio=8,
	NGPDHistEnbHgtTailSum=0x10,
	NGPDHistEnbHgtFallTime=0x20,
	NGPDHistEnbHgtTailRatio=0x40,

	NGPDHistEnbAll=0x7F,
	NGPDHistSeparateNeutrons=0x10000,
	NGPDHistDiscardPileup=0x20000
} NGPDHistEnables;

typedef enum 
{
	NGPDHistIgnoreFallTime=1, 
	NGPDHistIgnoreTailSum=2
} NGPDHistIgnore;

typedef struct ngpd_histogram_t
{
	int nbins_hgt, nbins_tail_sum, nbins_fall_time, nbins_ratio;
	int hgt_divide, tail_sum_divide, fall_time_divide;
	double ratio_scale;
	MOD_IMAGE3D *height_mod, *tail_sum_mod, *fall_time_mod, *tail_ratio_mod;
	MOD_IMAGE3D *hgt_tail_sum_mod, * hgt_fall_time_mod, *hgt_tail_ratio_mod;
	mh_com *height_head, *tail_sum_head, *fall_time_head, *tail_ratio_head;
	mh_com *hgt_tail_sum_head, * hgt_fall_time_head, *hgt_tail_ratio_head;
	NGPDHistEnables enables;
	NGPDHistIgnore ignores;
	int min_tail_count;
} NGPDHistogram;

typedef struct ngpd_event_t
{
	u_int64_t timestamp;
	int height;
	int tail_sum;
	int fall_time;
	int tail_count;
	int height_valid, tail_valid, fall_valid;
	int hgt_in_win, fall_in_win, tail_gt_thres, neutron;
	int sum_aborted, pileup;
} NGPDEventInfo;

typedef enum
{	NGPDScopeOpt_TBD 		= 1,		//!< To Be determined
} NGPDScopeOptionFlags;

typedef struct
{	NGPDScopeOptionFlags flags;					//!< Scope options flags {@link NGPDScopeOptionFlags};
	int nstreams;								//!< Number of scope streams to use where programmable (xspress3V7 onwards)
} NGPDScopeOptions;

typedef enum  {NGPDPreampMirrorNone=0, NGPDPreampMirrorEven=1, NGPDPreampMirrorOdd=2, NGPDPreampMirrorBoth=3} NGPDPreampMirror;

typedef struct {
	enum {NGZMPClkSrcFPGA, NGZMPClkSrcLMK61E2, NGZMPClkSrcTestClk, NGZMPClkSrcBkPlane } src;
	enum {NGZMPSysRefContinuous, NGZMPSysRefBurstSPI}  sysref;
	enum {NGZMPClockMonDefault=0, NGZMPClockMonClk250=1 } monitor_op;
} NGPDClockSetup;
typedef enum 
{
	NGPDDummy_None=0,
	NGPDDummy_FPGA=1,
	NGPDDummy_ADC=2,
	NGPDDummy_Preamp=4,
	NGPDDummy_All=0xFF
} NGPDDummy;

typedef enum 
{
	NGPDFilter_Unknown=0,
	NGPDFilter_Rectangle,
	NGPDFilter_Gaussian,
	NGPDFilter_Exponential,
	NGPDFilter_Trapezoidal,
	NGPDFilter_Custom
} NGPDFilterType;

typedef struct 
{
	NGPDFilterType type;
	int iarg1, iarg2;
	double darg;
} NGPDFilter;

typedef struct 
{
	int valid;
	enum {NGPDSingle, NGPDComposite} type;
	NGPDGeneration generation;
	void * adq_cu;
	void *zynqmp; // ZynqMPHandle *
	int starting_card;
	int sub_path[NGPD_MAX_NUM_CARDS+1];
	int adq_num;
	int debug;
	int num_cards;
	int num_chan;
	int max_num_chan;
	int chans_per_card;
	int chan_of_system;
	u_int32_t revision;
	u_int32_t playback_first_addr;
	u_int32_t playback_last_addr;
	u_int32_t scope_first_addr;
	u_int32_t scope_last_addr;
	u_int32_t scope_out_skip;
	u_int32_t playback_skip;
	int playback_num_streams;
	size_t playback_num_bytes;
	int playback_num_t;
	int run_flags;
	NGPDScopeModule *scope_mod;
	mh_com * scope_head;
	NGPDHistogram histogram;
	NGPDDummy dummy_system;
	int max_scope_streams;
	size_t nbytes_scope_per_card;
	int mem_layout;
	AXIHistConfig hist_config;
	int hist_config_read;
	NGPDPreampMirror preamp_mirror_offset;
	NGPDClockSetup clock_setup;
	NGPDFilter filter[NGZMP_CHANS_PER_CARD];
	u_int32_t *dummy_chan_reg[NGZMP_CHANS_PER_CARD];		//!< Used to hold copy of channel registers for dummy systems, 1 array per channel.
	u_int32_t *dummy_glob_reg;	
	u_int16_t *dummy_chan_offset;
	u_int8_t  *dummy_chan_dga;
	
	//!< Used to hold copy of global registers for dummy systems, 1 array per dummy card.
} NGPDPathType;

typedef struct
{
	double col_time;
	enum {NGPDTrigSW, NGPDTrigExtTrigCycles} trig_mode;
	int cycles;
} NGPDITFGSetup;

typedef struct
{
	u_int32_t flags;
	u_int32_t cycles;
	u_int32_t timer_us;
} NGPDITFGStatus;
extern NGPDPathType NGPDPath[];
char *ngpd_get_error_message();
int ngpd_config_adq14(unsigned int adq_num, int debug);
int ngpd_set_run_flags(int path, int run_flags);
int ngpd_get_run_flags(int path);
int ngpd_set_debug(int path, int debug);
int ngpd_get_num_cards(int path);
int ngpd_get_num_chan(int path);
int ngpd_get_chans_per_card(int path);
NGPDGeneration ngpd_get_generation(int path);

int ngpd_dma_set_playback_skip(int path, int playback_skip);
int ngpd_dma_get_playback_skip(int path);
int ngpd_dma_set_scope_out_skip(int path, int scope_out_skip);
int ngpd_dma_get_scope_out_skip(int path);
int ngpd_read_chan_regs(int path, int chan, int region, int offset, int num_regs, u_int32_t *data);
int ngpd_read_glob_regs(int path, int card, int offset, int num_regs, u_int32_t *data);
int ngpd_write_chan_regs(int path, int chan, int region, int offset, int num_regs, u_int32_t *data);
int ngpd_write_glob_regs(int path, int card, int offset, int num_regs, u_int32_t *data);
int ngpd_rmw_glob_regs(int path, int card,  int offset, u_int32_t and_mask, u_int32_t or_mask, u_int32_t *ret_value);

int ngpd_dma_upload(int path, int card, int offset, int num_words, u_int32_t *data);
int ngpd_dma_system_start(int path, int card, u_int32_t pb_start, u_int32_t pb_num512, NGPDITFGSetup *itfg_setup);
int ngpd_dma_wait_scope(int path, int card, u_int32_t *statusP);

int ngpd_dma_read_playback(int path, int offset, int num_words, u_int32_t *data, unsigned int *bytes_receivedP);
int ngpd_dma_read_scope(int path, int card, u_int32_t scope_start, u_int32_t scope_num512, int options);
int ngpd_scope_setup_stream(int path, int card, int stream, int chan_sel, int src_sel, int alt);
int ngpd_playback_generate(int path, int card, NGPDTestPattern *test_pat);
int ngpd_playback_load(int path, int card, char *filename, int max_num_t);

int ngpd_filter_load_integer(int path, int chan, int num_words, u_int32_t *data, u_int32_t * status);
int ngpd_filter_load_rect(int path, int chan, int num_ave);
int ngpd_filter_load_exp(int path, int chan, double Tsamples);
int ngpd_filter_load_gaus(int path, int chan, double sigma);
int ngpd_filter_load_trapezoid(int path, int chan, int wid_top, int wid_bot);
int ngpd_write_diff_trigger(int path, int chan, NGPDDiffTrigger * trig);
int ngpd_read_diff_trigger(int path, int chan, NGPDDiffTrigger * trig);
int ngpd_write_baseline_subtract(int path, int chan, NGPDBaseSubtract * bsub);
int ngpd_read_baseline_subtract(int path, int chan, NGPDBaseSubtract * bsub);
int ngpd_write_measure(int path, int chan, NGPDMeasure * measure);
int ngpd_read_measure(int path, int chan, NGPDMeasure * measure);
int ngpd_write_tail_subtract(int path, int chan, int32_t *data);
int ngpd_read_tail_subtract(int path, int chan, int32_t *data);

int ngpd_hist_setup(int path, int max_height, int max_tail_sum, int max_fall_time, double ratio_scale, int nbins_height, int nbins_tail_sum, int nbins_fall_time, int nbins_ratio, NGPDHistEnables enables);
int ngpd_hist_clear(int path);
int ngpd_hist_process(int path, unsigned int filled_buffers);

double ngpd_adc_set_range(int path, int chan, double vrange);
int ngpd_adc_set_offset(int path, int chan, int codes);
int ngpd_dma_set_ext_trigger_thres(int path, int card, double trig_thres);
int ngpd_write_dae_pulse(int path, int chan, int stretch);

NGPDScopeModule *ngpd_scope_get_mod(int path);

int ngpd_config_ngzmp(int ncards,  char* baseIPaddress, int basePort, char* baseMACaddress, int num_chan, int debug, int card_index, int do_init,  NGPDDummy dummy_system);
int ngpd_resolve_path_chan_card(int path, int chan, int *thisPath, int *chanIdx, int *cardP);
int ngpd_set_num_chan(int path, int num_chan);
int ngpd_has_bram(int path, int chan, int region_num);
int ngpd_bram_size(int path, int chan, int region_num);
int ngpd_get_playback_nstreams(int path, int card);
void ngpd_close(int path);
int ngpd_scope_mark_all(int path, int card, int value);
int ngpd_get_max_scope_streams(int path);
int ngpd_set_playback_streams(int path, int num_streams, int num_t);
int ngpd_get_playback_streams(int path);
int ngpd_write_chan_cont_reg(int path, int chan, u_int32_t chan_cont_reg);
int ngpd_write_chan_cont(int path, int chan, NGPDChanCont * chan_cont);
int ngpd_read_chan_cont(int path, int chan, NGPDChanCont * chan_cont);
int ngpd_read_scope_status(int path, int card, u_int32_t *scope_status);
int ngpd_read_itfg_status(int path, int card, NGPDITFGStatus *itfg_status);

int ngpd_get_chans_on_card(int path, int card, int *first, int *num);
int ngpd_set_scope_options(int path, int card, NGPDScopeOptions *options);
int ngpd_scope_update_mod(int path, int card, int t, int dt);
int ngpd_get_filter_type(int path, int chan, NGPDFilter *filter);
int ngpd_read_config_buffer(int path, int card, int offset, int num_bytes, u_int8_t *data);
int ngpd_write_config_buffer(int path, int card, int offset, int num_bytes, u_int8_t *data);
int ngpd_filter_read(int path, int chan, int num_words, u_int32_t *data);
int ngpd_save_settings(int path, char *file_name);
int ngpd_restore_settings(int path, char *file_name);
int ngzmp_save_settings(int path, char *file_name);
int ngzmp_restore_settings(int path, char *file_name);

int ngpd_clock_setup(int path, int card, NGPDClockSetup *cset);
int ngpd_scope_read_to_buff(int path, int card, int stream, int t, int dt, uint16_t *ptr);

int ngpd_adc_get_range(int path, int chan, float *vrange);
int ngpd_adc_get_offset(int path, int chan, int *codes);
int ngpd_read_revision(int path, int card, u_int32_t *rev);

#define CHECKADQ(f) if(!(f)){snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, #f " returns error"); goto error;}


typedef enum {
	NGPDScopeStream_Error=-1, //!< Error interpreting arguments
	NGPDScopeStream_NotPresent=0,  //!< The stream is not present in the current configuration of the data module.
	NGPDScopeStream_Unsigned=1,    //!< The data is to be interpreted as 16 bit unsigned "analogue" data.
	NGPDScopeStream_Signed=2,      //!< The data is to be interpreted as 16 bit signed "analogue" data.
	NGPDScopeStream_AllDigital=3,  //!< The data is digital data extracted from across all channels as should be displayed as up to 16 digital traces.
	NGPDScopeStream_AuxDigital=4   //!< The data is digital data which is associated with other streams and is often displayed with those streams rather than in its own right.
	} NGPDScopeStream;


NGPDScopeModule *ngpd_scope_mod_create(char *name, int num_cards, size_t nbytes_per_card, mh_com ** mod_head, int layout);
u_int16_t * ngpd_scope_mod_get_ptr(NGPDScopeModule *mod, int card, int stream);
int ngpd_scope_mod_get_inc(NGPDScopeModule *mod, int stream) ;
int ngpd_scope_mod_get_nstreams(NGPDScopeModule *mod) ;
const char *ngpd_scope_stream_name(NGPDScopeModule *mod, int card, int stream);
int ngpd_scope_stream_flags(NGPDScopeModule *mod, int card, int stream);
NGPDScopeStream ngpd_scope_stream_details_path(int path, int card, int stream, int *num_digP, int *bit_posnP, int *dig_strP, int *num_extraP, int *extra_posnP, int *extra_strP, int *scope_incP, int *dig_incP);
NGPDScopeStream ngpd_scope_stream_details_num(NGPDScopeModule *mod, int card, int stream, int *num_digP, int *bit_posnP, int *dig_strP, int *num_extraP, int *extra_posnP, int *extra_strP, int *scope_incP, int *dig_incP);
NGPDScopeStream ngpd_scope_stream_details(NGPDScopeModule *mod, int card, int stream, int *num_digP, int *bit_posnP, u_int16_t **dig_ptrP, int *num_extraP, int *extra_posnP, u_int16_t **extra_ptrP, int *scope_incP, int *dig_incP);

int ngpd_scope_set_streams(NGPDScopeModule *mod, int num_streams); 
int ngpd_scope_calc_num_t(NGPDScopeModule *mod);
NGPDScopeStream ngpd_scope_stream_details_path(int path, int card, int stream, int *num_digP, int *bit_posnP, int *dig_strP, int *num_extraP, int *extra_posnP, int *extra_strP, int *scope_incP, int *dig_incP);

/* #define NGPD_SCOPE_NUM_SRC0TO2 16*/

extern const char *ngpd_scope_name_s0[];
extern const char *ngpd_scope_name_s1[];
extern const char *ngpd_scope_name_s2[];
extern const char *ngpd_scope_name_s3[];
extern const char *ngpd_scope_name_s4[];
extern const char *ngpd_scope_name_s5[];
extern const char *ngpd_scope_alt_names_s0to4[16][16];
extern const char *ngpd_scope_alt_names_s5[16][16];
extern const int ngpd_scale_to_16bit_s0to4[16][16];
#define NGPD_MAX_IP_ADDR  16
#define NGPD_MAX_MAC_ADDR 18
#define NGPD_MAX_CONFIGS 10

typedef struct _ngpd_saved_config{
	enum { ConfigADQ14, ConfigZynqMP1 } type;
	int ncards;
	char baseIPaddress[NGPD_MAX_IP_ADDR+2];
	int basePort;
	char baseMACaddress[NGPD_MAX_MAC_ADDR+2];
	int nchan;
	char modname[NGPD_MAX_MODNAME+2];
	int card_index;
	int path;
	NGPDDummy dummy_system;
} NGPDSavedConfig;

extern NGPDSavedConfig ngpd_saved_config[NGPD_MAX_CONFIGS];

#endif /* NGPD_H_ */
