
// #include <stdint.h>
#ifndef _NGZMP_H
#define _NGZMP_H

#include "ngzmp_dma_protocol.h"
#include "axi_hist.h"

/** @defgroup NGZMP_SCOPE_MODE		Macros to describe scope mode in thw ZynMP version
@ingroup NGPD_SCOPE_MODE
*/

#define NGZMP_CHANS_PER_CARD			8
#define NGZMP_MAX_CARDS					8
#define NGZMP_MAX_CHAN					(NGZMP_CHANS_PER_CARD*NGZMP_MAX_CARDS)
#define NGZMP_TOTAL_HIST_SIZE_BYTES		(512*1024*1024)
#define NGZMP_CHAN_HIST_SIZE_BYTES		(NGZMP_TOTAL_HIST_SIZE_BYTES/(NGZMP_CHANS_PER_CARD))
#define NGZMP_SCOPE_MAX_STREAMS			10
#define NGZMP_PB_MAX_STREAMS			8
#define NGZMP_NUM_PREAMP_OFFSET_MAPS	2		//!< Number of preamp mappings, 

#define NGZMP_ADDR_SLICE_REGION			18		// Byte address start bit of region
#define NGZMP_ADDR_SLICE_CHAN			22		// Byte address start bit of Channel
#define NGZMP_ADDR_BIT_GLOB_REGS		26
#define NGZMP_FAN_LOG_POINTS 10000
#define NGZMP_FAN_MODE_OFF				0
#define NGZMP_FAN_MODE_MONITOR_LOOP 		1
#define NGZMP_FAN_MODE_MONITOR_ONESHOT 	2
#define NGZMP_FAN_MODE_CONTROL 			3

#define NGZMP_FAN_CONT_NUM_AVERAGE 10
#define NGZMP_FAN_CONT_FRAC_PRECISION 10

#define NGZMP_REGION_REGS			0
#define NGZMP_REGION_TAIL_THRES_M	1
#define NGZMP_REGION_TAIL_THRES_C	2
#define NGZMP_REGION_TAIL_SUB0		3
#define NGZMP_REGION_TAIL_SUB1		4

#define NGZMP_NUM_REGIONS			5

#define NGZMP_CHAN_CONT				0				//!< Tracks NGPD_CHAN_CONT, but used within the driver.
#define NGZMP_CHAN_FILTER			1
#define NGZMP_FILT_NUM_COEFF		51				//!< Number of coefficients to load into the fillter see also  NGPD_FILT_NUM_COEFF	
#define NGZMP_CHAN_HIST_CONT_HGT	20				//!< First of 2 hist control registers, this one has flags and Nbits Height and height shift 
#define NGZMP_CHAN_HIST_CONT_TAIL	21				//!< Hist control of Nbits tail Sum and time

#define NGZMP_DATA_PATH				0
#define NGZMP_PLAYBACK				1
#define NGZMP_CLK_SELECT			2
#define NGZMP_LEMO_CONT				3

#define NGZMP_SCOPE_NUMWORDS		16
#define NGZMP_SCOPE_CONT			17

#define NGZMP_SCOPE_CHAN_SEL0		18
#define NGZMP_SCOPE_CHAN_SEL1		19
#define NGZMP_SCOPE_SRC_SEL0		20
#define NGZMP_SCOPE_SRC_SEL1		21
#define NGZMP_SCOPE_ALT_SEL0		22
#define NGZMP_SCOPE_ALT_SEL1		23

#define NGZMP_ITFG_TIME				24
#define NGZMP_ITFG_TRIG_MODE		25
#define NGZMP_ITFG_CYCLES			26
#define NGZMP_GLOB_NUM				32			//!< maximum number of global registers for dummy system, register copy

#define NGZMP_GLOB_READ_BASE		(256)
#define NGZMP_REVISION				(NGZMP_GLOB_READ_BASE+0)
#define NGZMP_SCOPE_STATUS			(NGZMP_GLOB_READ_BASE+1)
#define NGZMP_CYCLES_STATUS			(NGZMP_GLOB_READ_BASE+2)
#define NGZMP_FILTER_STATUS			(NGZMP_GLOB_READ_BASE+3)
#define NGZMP_TIMER_STATUS			(NGZMP_GLOB_READ_BASE+4)

#define NGZMP_FILTER_STATUS_RELOAD_TREADY(chan)		(1<<(4*(chan)))
#define NGZMP_FILTER_STATUS_TLAST_MISSING(chan)		(2<<(4*(chan)))
#define NGZMP_FILTER_STATUS_TLAST_UNEXPECTED(chan)	(4<<(4*(chan)))
#define NGZMP_FILTER_STATUS_MISSING_TREADY(chan)	(8<<(4*(chan)))		//!< Not from hardware, software uses this bit to mark when tready is not asserted, but should have been.

#define NGZMP_FILTER_COEF(x)			((x)&0x3FFFF)
#define NGZMP_FILTER_FORCE_CONF			(1<<30)
#define NGZMP_FILTER_TLAST				(1<<31)

#define NGZMP_PLAYBACK_ENABLE			(1<<4)				//!< Provissiona enable bit, not used yet
#define NGZMP_PLAYBACK_RST_TS			(1<<5)				//!< Reset time stamp each loop of playback data
#define NGZMP_PLAYBACK_NUM_STREAMS(x)	(((x)&3)<<8)		//!< Set 2 bit code for number of playbackstream 1, 2, 4, 8
#define NGZMP_PLAYBACK_GET_NUM_STREAMS(x)	(((x)>>8)&3)	//!< Get 2 bit code for number of playbackstream 1, 2, 4, 8

#define NGZMP_CLK_SELECT_LMK04828			(1<<0)			//!< Select clock from LMK04828
#define NGZMP_CLK_SELECT_TEST_CLK			(1<<1)			//!< Select input to LMK04828 from TEST CLK input (SMC)

#define NGZMP_LEMO0_SRC_SET(x)			(((x)&3)<<0)
#define NGZMP_LEMO1_SRC_SET(x)			(((x)&3)<<2)
#define NGZMP_LEMO0_SRC_GET(x)			(((x)>>0)&3)
#define NGZMP_LEMO1_SRC_GET(x)			(((x)>>2)&3)

#define NGZMP_LEMO0_SRC_CLK					0			//!< Select Clk250MHz divide fro LEMO 0

#define NGZMP_SCOPE_CONT_NUM_STREAMS(x)		(((x)&3))		//!< Set 2 bit code from number of scope stream 0, 1,2 => 4, 8, 10.
#define NGZMP_SCOPE_CONT_GET_NUM_STREAMS(x)	(((x))&3)		//!< Get 2 bit code from number of scope stream 0, 1,2 => 4, 8, 10.

#define NGZMP_SCOPE_CHAN_SEL_SET(s,x)	(((x)&0xF)<<4*((s)&7))
#define NGZMP_SCOPE_CHAN_SEL_GET(s,x0,x1) (((s)>=8)?(((x1)>>4*((s)-8))&0xF):(((x0)>>4*((s)))&0xF))


#define NGZMP_SCOPE_SRC_SEL_SET(s,x)	(((x)&0xF)<<4*((s)&7))
#define NGZMP_SCOPE_SRC_SEL_GET(s,x0,x1) (((s)>=8)?(((x1)>>4*((s)-8))&0xF):(((x0)>>4*((s)))&0xF))

#define NGZMP_SCOPE_ALTERNATE_SET(s,x)		(((x)&0xF)<<4*((s)&7))
#define NGZMP_SCOPE_ALTERNATE_GET(s,x0,x1) (((s)>=8)?(((x1)>>4*((s)-8))&0xF):(((x0)>>4*((s)))&0xF))

#define NGZMP_SCOPE_STAT_SCOPE_RUNNING		(1<<0)			//!< Scope Mode state machine is running, now DMA will take a short time to finish after this falls.		
#define NGZMP_SCOPE_STAT_SCOPE_OVERUN		(1<<1)			//!< Scope Mode overrun, FIFO filled DMA could not keep up.

#define NGZMP_SCOPE_STAT_PB_READY			(1<<8)			//!< Playback DMA has filled FIFO, ready to start.
#define NGZMP_SCOPE_STAT_PB_UNDERUN			(1<<9)			//!< Playback under run. DMA could not keep so FIFO went empty.

#define NGZMP_CC_SRC_USE_PB_START			(1<<0)
#define NGZMP_CC_SRC_SET(x)				(((x)&0x7)<<1)		//!< Set data source bits of channel control register
#define NGZMP_CC_SRC_GET(x)				(((x)>>1)&0x7)		//!< Get data source bits of channel control register
#define NGZMP_CC_SRC_ADC_FWD				0				//!< Normal data source from ADC		
#define NGZMP_CC_SRC_ADC_REV				1				//!< Normal data source from ADC		
#define NGZMP_CC_SRC_PB						4				//!< Data from Playback data

#define NGZMP_CC_INV_DATA					(1<<7)			//!< Invert (1's complement) the ADC data.

/** @defgroup NGZMP_SCOPE_SEL_MACROS		Macros to select data source in scope mode
@ingroup NGZMP_SCOPE_MODE
@{
*/
#define NGZMP_SCOPE_SEL_ANA_TEST_PAT		0		//!< Select test pattern generator
#define NGZMP_SCOPE_SEL_ANA_INPUT			1		//!< Select analogue input`
#define NGZMP_SCOPE_SEL_ANA_FILTER			2		//!< Select output of FIR filter
#define NGZMP_SCOPE_SEL_ANA_DIFF_TRIG_DIFF	3		//!< Select differential from differential base filter
#define NGZMP_SCOPE_SEL_ANA_DIFF_TRIG_OUT	4		//!< Select output of differential filter.
#define NGZMP_SCOPE_SEL_ANA_BASELINE_SUB	5		//!< Select output of baseline subtraction block.
#define NGZMP_SCOPE_SEL_ANA_BSUB_INTERNAL	6		//!< Select internals of baseline subtraction block.
#define NGZMP_SCOPE_SEL_ANA_ABS_TRIG		7		//!< Select absolute leve trigger (for future expansion)
#define NGZMP_SCOPE_SEL_ANA_MEASURE			8		//!< Select internals of the pulse measurement block	
#define NGZMP_SCOPE_SEL_ANA_DATA_OUTPUT		9		//!< Select the output data
#define NGZMP_SCOPE_SEL_ANA_TAIL_SUB		10		//!< Select tail subtraction signals

#define NGZMP_SCOPE_ALT_DIFF_OUT_DIG4		2

#define NGZMP_SCOPE_ALT_MEAS_RAW1			0
#define NGZMP_SCOPE_ALT_MEAS_RAW2			1
#define NGZMP_SCOPE_ALT_MEAS_CORRECTED		5

#define NGZMP_SCOPE_SEL37_DIG_5BIT			15		//!< Use for streams 3 and 7 to select 5 bit digital data from 0..2 and 4..6 respectively in 4 or 8 stream modes

#define NGZMP_SCOPE_SEL8_DIG_4BIT			14		//!< Use for stream 8 to select 4 bit digital data from streams 0..3 in 10 stream mode
#define NGZMP_SCOPE_SEL8_DIG_5BIT			15		//!< Future expansion

#define NGZMP_SCOPE_SEL9_DIG_4X14X2BIT		13		//!< Use for stream 9 in 10 stream mode to select 1 dig bit from stream 0..3, 2 dig bits from 4..7 + 4 from stream 8
#define NGZMP_SCOPE_SEL9_DIG_4BIT			14		//!< Use for stream 9 to select 4 bit digital data from streams 4..7 in 10 stream mode
#define NGZMP_SCOPE_SEL9_DIG_5BIT			15		//!< Future expansion
/**
@}
@defgroup NGZMP_SCOPE_NUM_SRC		Macros to define number of data sources defined for scope mode
@ingroup NGZMP_SCOPE_MODE
@{
*/
#define NGZMP_SCOPE_NUM_SRC_ANA				11
#define NGZMP_SCOPE_NUM_SRC3				16
#define NGZMP_SCOPE_NUM_SRC7				16
#define NGZMP_SCOPE_NUM_SRC8				15
#define NGZMP_SCOPE_NUM_SRC9				15
/**
@}
*/


#define NGZMP_SPI_NUM_DGA	8
#define NGZMP_SPI_NUM_ADC	2
#define NGZMP_SPI_NUM_CLK	1

#define NGZMP_I2C_BUS_ENCLUSTRA	0				//!< /dev/i2c-0 is the i2c bus to the devices on the Enclustar Mercury XU7
#define NGZMP_I2C_BUS_PMBUS		1				//!< /dev/i2c-1 is the i2c bus to the power management bus for the PSU controllers and temperature measurement.
#define NGZMP_I2C_BUS_CLK		2				//!< /dev/i2c-2 is the i2c bus to the clk devices, LMK61E2.
#define NGZMP_I2C_BUS_ESS_CLK	3				//!< /dev/i2c-3 is the i2c bus to the ESS mode clock devices.
#define NGZMP_I2C_BUS_PREAMP	4				//!< /dev/i2c-4 is the i2c bus to the pre-amp
#define NGZMP_I2C_BUS_SFP		5				//!< /dev/i2c-5 is the i2c bus to the SFP+ modules

#define NGZMP_I2C_BUS_SFP_A			10				//!< Sub bus under /dev/i2c-5 accessed via the PCA9546A to SFP+ A
#define NGZMP_I2C_BUS_SFP_B			11				//!< Sub bus under /dev/i2c-5 accessed via the PCA9546A to SFP+ B
#define NGZMP_I2C_BUS_MGT_REFCLK	12				//!< Sub bus under /dev/i2c-5 accessed via the PCA9546A to MGT RefClk SI570
#define NGZMP_I2C_BUS_SFP_PEXP 	 	13				//!< Sub bus under /dev/i2c-5 accessed via the PCA9546A to parallel expander to read SFP+ status pins.

#define NGZMP_I2C_BUS_MAX NGZMP_I2C_BUS_SFP_PEXP	//! Set to highest supported bus number 
#define ZYNQMP_I2C_USE_REG_ADDR	0x40000000		//!< Used to mark adress filed in ZynqMP client/server protocol to use the reg_addr field.

#define NGZMP_I2C_ADDR_ENC_DS28CN01U 	0x5C	//!< I2C Slave address for the DS28CN01U EEPROM for encluistar use.
#define NGZMP_I2C_ADDR_ENC_ATSHA204A 	0x64	//!< I2C Slave address for the ATSHA204A EEPROM for encluistar use.

#define NGZMP_I2C_ADDR_SFP_SWITCH 		0x70	//!< I2C Slave address for the PCA9546A switch on the I2C bus to the SFP+ modules
#define NGZMP_I2C_ADDR_REFCLK_SI570 	0x5D	//!< I2C Slave address for the SI570 on the MGT RefClk I2C bus
#define NGZMP_I2C_ADDR_SFP_PEXP 		0x20	//!< I2C Slave address for the MCP23008 parallel expander to read ststus from the SFP+moduels, accessed via NGZMP_I2C_BUS_SFP_PEXP

#define NGZMP_I2C_ADDR_CLK_LMK61E2 		0x58	//!< I2C Slave address for the LMK61E2 on the Clk I2C bus

#define NGZMP_I2C_ADDR_ESSCLK_SI5324 	0x68	//!< I2C Slave address for the SI5324 on the ESS MGT Clk I2C bus
#define NGZMP_I2C_ADDR_ESSCLK_SI570 	0x5D	//!< I2C Slave address for the SI570 on the ESS MGT Clk I2C bus
#define NGZMP_I2C_ADDR_ESSCLK_EEPROM 	0x50	//!< I2C Slave address for the EEPROM on the ESS MGT Clk I2C bus


#define NGZMP_I2C_NUM_ADT7410_ADC		4			//!< Number of ADT7410 temperature monitor chips on PCB.
#define NGZMP_I2C_NUM_ADT7410_PREAMP	2			//!< Number of ADT7410 temperature monitor chips on PCB.
#define NGZMP_I2C_ADDR_ADT7410(a)	(0x48+((a)&3))	//!< I2C Slave address for ADT7410 temperature monitor chips on PCB.

#define NGZMP_I2C_ADDR_PSU_PEXP			0x20		//!< I2C Slave address for MCP23008 port expander used to monitor PSU status
#define NGZMP_I2C_ADDR_SFP_PEXP			0x20		//!< I2C Slave address for MCP23008 port expander used to control/monitor SFP+ status
#define NGZMP_I2C_ADDR_OFFSET_DAC		0x0C		//!< I2C Slave address for AD5675 offset DAC on pre-amp.

#define NGZMP_I2C_ADDR_UDC90160			0x34		//!< I2C Slave addres for teh first UCD90160 PSU, monitor, the other is one higher.
#define NGZMP_I2C_UDC90160_PAGE			0x00		//!< I2C command to read or write SMBUS Page, rail-1
#define NGZMP_I2C_UDC90160_RW_VOUT_MODE 0x20		//!< I2C command to read or write VOUT mode, which controls the Exponent used in the data.
#define NGZMP_I2C_UDC90160_READ_VOUT	0x8B		//!< I2C command to read Vout from UCD90160

#define NGZMP_UCD90160_NUM_RAILS		16			//!< maximum number of rails supported by UCD90160
#define NGZMP_UCD90160_NUM_CHIPS		2			//!< maximum number of UCD90160 chips on board
#define NGZMP_I2C_PCA9546_CR(sub_bus)	((1<<(sub_bus)))	//!< Enable single downstream bus/channel 0..3


#define ADT7410_TEMP_MSB				0
#define ADT7410_TEMP_LSB				1
#define ADT7410_STATUS					2
#define ADT7410_CONFIG					3
#define ADT7410_THIGH_MSB				4
#define ADT7410_THIGH_LSB				5
#define ADT7410_TLOW_MSB				6
#define ADT7410_TLOW_LSB				7
#define ADT7410_TCRIT_MSB				8
#define ADT7410_TCRIT_LSB				9
#define ADT7410_THYST					0xA
#define ADT7410_ID						0xB
#define ADT7410_RESET					0x2F

#define ADT7410_STATUS_TLOW				(1<<4)
#define ADT7410_STATUS_THIGH			(1<<5)
#define ADT7410_STATUS_TCRIT			(1<<6)

#define MCP23008_IODIR					0x00
#define MCP23008_IPOL					0x01
#define MCP23008_GPINTEN				0x02
#define MCP23008_DEFVAL					0x03
#define MCP23008_INTCON					0x04
#define MCP23008_IOCON					0x05
#define MCP23008_GPPU					0x06
#define MCP23008_INTF					0x07
#define MCP23008_INTCAP					0x08
#define MCP23008_GPIO					0x09
#define MCP23008_OLAT					0x0A

#define AD5675_CMD_WRITE(chan)			(0x10|((chan)&7))	//!< AD5675 DAC write to Input register command. LDAC is low so will update.
#define AD5675_CMD_UPDATE				0x20				//!< AD5675 DAC update outputs from input register.
#define AD5675_CMD_WRITE_UPDATE(chan)	(0x30|((chan)&7))	//!< AD5675 DAC write to Input register and update DAC output command
#define AD5675_CMD_POWER_DOWN			0x40				//!< AD5675 DAC 
#define AD5675_CMD_LDAC_MASK			0x40				//!< AD5675 DAC 
#define AD5675_CMD_RESET				0x60				//!< AD5675 DAC 
#define AD5675_CMD_GAIN					0x70				//!< AD5675 DAC 
#define AD5675_CMD_RESET				0x60				//!< AD5675 DAC 
#define AD5675_CMD_READ(chan)			(0x90|((chan)&7))	//!< AD5675 DAC setup address to prepare fro read command
#define AD5675_CMD_WRITE_ALL			0xA0				//!< AD5675 DAC write data to all input registers ?
#define AD5675_CMD_WRITE_ALL_UPDATE		0xB0				//!< AD5675 DAC write data to all input registers ?

/* Hist GPIO control register */
#define NGZMP_HIST_GPIO_OFFSET			0x400		//!< Word address offset in Hist register sapce to the GPIO register

#define NGZMP_HIST_GPIO_ENABLE			(1<<0)		//!< Hist Enable
#define NGZMP_HIST_GPIO_DISABLE_UPPER	(1<<1)		//!< Disable hist stream 15..8 when present.   
#define NGZMP_HIST_GPIO_SEL_TEST		(1<<2)		//!< Select test pattern data from HistTest DMA stream.
typedef enum {
	NGZMPHistEnbDisable		= 0,
	NGZMPHistEnbRun			= 1,
	NGZMPHistEnbTestFull	= 5,
	NGZMPHistEnbTestLower	= 7
} NGZMPHistEnable;

#define NGZMP_HCH_SEPARATE_NGP			(1<<0)		//!< Separate neutron, gamma and pileup in diagnostic histogram
#define NGZMP_HCH_ENB_HGT_SUM			(1<<1)		//!< Enable height/Tail sum histogram
#define NGZMP_HCH_ENB_HGT_FALL			(1<<2)		//!< Enable height/Fall Time histogram
#define NGZMP_HCH_DISCARD_PU			(1<<3)		//!< Discard pileup events (particuarly used when not separating ngp)

#define NGZMP_HCH_NBITS_HGT_SET(x)		(((x)&0x1F)<<16)	//!< Set Number of bits of Height to be used in diagnostic histograms 0...18
#define NGZMP_HCH_NBITS_HGT_GET(x)		(((x)>>16)&0x1F)	//!< Get Number of bits of Height to be used in diagnostic histograms 0...18
#define NGZMP_HCH_HGT_SHIFT_SET(x)		(((x)&0x1F)<<24)	//!< Set Number of LS bits of Height to discard
#define NGZMP_HCH_HGT_SHIFT_GET(x)		(((x)>>24)&0x1F)	//!< Get Number of LS bits of Height to discard

#define NGZMP_HCT_NBITS_FALL_SET(x)		(((x)&0xF)<<0)		//!< Set Number of bits of Fall time to use
#define NGZMP_HCT_NBITS_FALL_GET(x)		(((x)>>0)&0xF)		//!< Get Number of bits of Fall time to use
#define NGZMP_HCT_FALL_SHIFT_SET(x)		(((x)&0xF)<<8)		//!< Set Number of LS bits of Fall time to discard
#define NGZMP_HCT_FALL_SHIFT_GET(x)		(((x)>>8)&0xF)		//!< Get Number of LS bits of Fall time to discard

#define NGZMP_HCT_NBITS_TSUM_SET(x)		(((x)&0x1F)<<16)	//!< Set Number of bits of Tail Sum to use
#define NGZMP_HCT_NBITS_TSUM_GET(x)		(((x)>>16)&0x1F)	//!< Get Number of bits of Tail Sum to use
#define NGZMP_HCT_TSUM_SHIFT_SET(x)		(((x)&0x1F)<<24)	//!< Set Number of LS bits of Tail Sum to discard
#define NGZMP_HCT_TSUM_SHIFT_GET(x)		(((x)>>24)&0x1F)	//!< Get Number of LS bits of Tail Sum to discard

typedef struct
{
	int separate_ngp;
	int enb_hgt_sum;
	int enb_hgt_fall;
	int discard_pu;
	int nbits_height;
	int shift_height;
	int nbits_fall_time;
	int shift_fall_time;
	int nbits_tail_sum;
	int shift_tail_sum;
} NGZMPHistConf;



// Target bus for commands
enum zynqmp_protocol_bus_type
{
	ZYNQMP_BUS_LOCAL   	= 0,	//!< AXI memory-mapped peripherals local to the Zynq in both XSpress4 and xspress3-mini
	ZYNQMP_BUS_REGS   	= 1,	//!< AXI memory-mapped peripherals  THESE MATCH THE BUS TYPE IN THE DRIVER
	ZYNQMP_BUS_JESD     = 2,	//!< RDMA register space 
	ZYNQMP_BUS_DMA 		= 3,	//!< Xilinx AXI peripherals, DMA controllers
	ZYNQMP_BUS_PS_LOW 	= 4,	//!< Remote DRAM on Virtex
    ZYNQMP_BUS_PS_HIGH  = 5,    //!< DRAM allocated to Playback and Scope
   	ZYNQMP_BUS_PL_RAM   = 6,    //!< DRAM allocated to Histogram and playback
    ZYNQMP_BUS_HIST		= 7,    //!< AXI Interface to histogrammer control registers

	ZYNQMP_BUS_DIRECT	= 15,	//!< Direct memory write

	ZYNQMP_BUS_EEPROM   = 16,
	ZYNQMP_BUS_I2C      = 17,
	ZYNQMP_BUS_SPI_DGA	= 20,	//!< SPI bus for DGA, address is simply the channel number
	ZYNQMP_BUS_SPI_ADC	= 21,	//!< SPI bus for ADCS Address bit 16 select which ADC 14..0 is the ADI SPI protocol address
	ZYNQMP_BUS_SPI_CLK	= 22,	//!< SPI bus for Clock chips. Address gives the address bits in the SPI word
	ZYNQMP_BUS_FAN_CONT  = 30,	//!< Access to the fan control internal memory.
	ZYNQMP_BUS_FEM_CONFIG = 31,	//!< Access to the FEM Config
	ZYNQMP_BUS_DMA_STATUS = 32,	//!< Access to the DMA status block
	ZYNQMP_BUS_XADC		  = 33,	//!< Access to the XADC system monitor.
	ZYNQMP_BUS_UCD90160   = 34,	//!< Access to read of voltage interface to UCD90160 via I2C bus.
	ZYNQMP_BUS_CONFIG	  = 35, //!< Area to store config (particularly filter type) which will be preserved if the x86_64 program is restarted.
	ZYNQMP_BUS_DMA_BUFF  = 100
};

#define ZYNQMP_CONFIG_BUFFER_SIZE  1024

typedef struct _ngzmp_fan_log {
	int16_t temp; 	// Signed int in 0.5 deg C units
	int16_t zynq_temp; 	// Signed int in 0.5 deg C units
	int16_t speed; // 0 = off, 4000 = full speed
} NGZMPFanLog;

typedef struct _ngzmp_fan_cont
{
	int32_t mode;
	int32_t start_run;
	int32_t cur_temp[4];
	int32_t cur_max;
	int32_t target, p_const, i_const;
	int32_t cur_point;
	NGZMPFanLog log[NGZMP_FAN_LOG_POINTS];
} NGZMPFanControl;

typedef struct {
	u_int8_t addr, data;
} NGZMPI2CAddrData;
typedef struct {
	int addr;
	u_int8_t data;
} NGZMPSPIAddrData;
typedef struct {
	int addr;
	u_int8_t data;
	enum {NGZMPADC_DontRead, NGZMPADC_Glob, NGZMPADC_Pair, NGZMPADC_Chan, NGZMPADC_AddDelay=0x10} flags;
} NGZMPADCAddrData;

// These must track  enum XADC_Param and enum AMS_Param in zynqmp_xadc.h.
// In the NGZMP case there is a single ZynMP presenting the AMS followed by XAC, but other implementations may vary
enum NGZMP_SysMon_Param
{
	NGZMP_AMS_PSTempLPD,
	NGZMP_AMS_PSTempFPD,
	NGZMP_AMS_PSVccIntLP,
	NGZMP_AMS_PSVccIntFP,
	NGZMP_AMS_PSVccAux,
	NGZMP_AMS_PSVccDDR,
	NGZMP_AMS_PSVccIO0,
	NGZMP_AMS_PSVccIO1,
	NGZMP_AMS_PSVccIO2,
	NGZMP_AMS_PSVccIO3,
	NGZMP_AMS_PSAVccMGTR,
	NGZMP_AMS_PSAVttMGTR,
	NGZMP_AMS_PSVccAMS,
	NGZMP_AMS_PSVccPLL0,
	NGZMP_AMS_PSVccBatt,
	NGZMP_AMS_PLVccInt,
	NGZMP_AMS_PLVccBRAM,
	NGZMP_AMS_PLVccAux,
	NGZMP_AMS_PSVccDDRPLL,
	NGZMP_AMS_PSVccIntFPDDR,
	NGZMP_XADC_VccInt,
  	NGZMP_XADC_VccAux,
	NGZMP_XADC_VccBRam,
	NGZMP_XADC_VccPInt,	// PS8 Int
	NGZMP_XADC_VccPAux,	// PS8 Aux
	NGZMP_XADC_VccoDdr,	// PS8 Vccoddr
	NGZMP_XADC_Temp,

	NGZMP_ParamMax
};
extern const char *ngzmp_sys_mon_name[NGZMP_ParamMax];

typedef enum {NGZMPADC_Default=0} NGZMPADCFlags;

#define NGZMP_SPI_ADC_ADDR_SET(x)	((x)&0x7FFF)
#define NGZMP_SPI_ADC_ADDR_GET(x)	((x)&0x7FFF)
#define NGZMP_SPI_ADC_ADC_SET(x)	(((x)&3)<<16)
#define NGZMP_SPI_ADC_ADC_GET(x)	(((x)>>16)&3)
#define NGZMP_SPI_ADC_PAIR_SET(x)	(((x)&3)<<24)
#define NGZMP_SPI_ADC_PAIR_GET(x)	(((x)>>24)&3)
#define NGZMP_SPI_ADC_CHAN_SET(x)	(((x)&3)<<28)
#define NGZMP_SPI_ADC_CHAN_GET(x)	(((x)>>28)&3)

int ngzmp_config_details(int ncards, int num_tf, char* baseIPaddress, int basePort, char* baseMACaddress, int num_chan, int create_module, char* modname, int debug, int card_index, int do_init);
int ngzmp_dma_config_memory(int path, int card, int layout, int just_read, int force);

int ngzmp_hist_read_reg(int path, int card, int offset, int size, u_int32_t *value);
int ngzmp_hist_write_reg(int path, int card, int offset, int size, u_int32_t *value);
int ngzmp_read_chan_reg(int path, int chan, int region, int offset, int size, u_int32_t *value);
int ngzmp_write_chan_reg(int path, int chan, int region, int offset, int size, u_int32_t *value);
int ngzmp_read_glob_reg(int path, int card,  int offset, int size, u_int32_t *value);
int ngzmp_write_glob_reg(int path, int card, int offset, int size, u_int32_t *value);
int ngzmp_write_dma_buff(int path, int card, int stream, int offset, int size, u_int32_t *data);
int ngzmp_read_dma_buff(int path, int card, int stream, int offset, int size, u_int32_t *data);

int ngzmp_hist_read_dma(int path, int card, u_int32_t offset, u_int32_t size, u_int32_t *data, u_int32_t flags);
int ngzmp_hist_read_stream_dma(int path, int card, int stream, u_int32_t offset, u_int32_t size, u_int32_t *data, int use_dma_reset);
int ngzmp_hist_read_stream(int path, int card, int stream, u_int32_t offset, u_int32_t size, u_int32_t *data);

int ngzmp_hist_read_discard(int path, int card, u_int32_t offset, u_int32_t size);

int ngzmp_dma_reset(int path, int card, u_int32_t function_mask);
int ngzmp_dma_build_desc(int path, int card, u_int32_t stream, ZYNQMP_DMA_MsgBuildDesc *msg, int32_t *num_desc);
int ngzmp_dma_build_debug_desc(int path, int card, u_int32_t stream, ZYNQMP_DMA_MsgBuildDebugDesc *msg);
int ngzmp_dma_start(int path, int card, u_int32_t stream_mask, ZYNQMP_DMA_MsgStart *msg);
int ngzmp_dma_build_test_pat(int path, int card, u_int32_t stream, ZYNQMP_DMA_MsgTestPat *msg);
int ngzmp_dma_print_data(int path, int card, u_int32_t stream, ZYNQMP_DMA_MsgPrint *msg);
int ngzmp_dma_print_scope_data(int path, int card, ZYNQMP_DMA_MsgPrint *msg);
int ngzmp_dma_print_desc(int path, int card, u_int32_t stream, ZYNQMP_DMA_MsgPrintDesc *msg);
int ngzmp_get_dma_status_block(int path, int card, ZYNQMP_DMA_StatusBlock *statusBlock);
int	ngzmp_dma_check_desc(int path, int card, u_int32_t stream, ZYNQMP_DMA_MsgCheckDesc *msg, int32_t *good_desc, u_int32_t *completed_desc, u_int32_t *last_frame, u_int32_t *status);
int ngzmp_dma_read_status(int path, int card, u_int32_t stream_mask, int32_t *dma_status);

int ngzmp_dma_wait_scope(int path, int card, u_int32_t *statusP);


int ngzmp_hist_read_config(int path, int card, AXIHistConfig * config);
int ngzmp_hist_clear_start(int path, int card, int start_word, int num_words, int all_streams);
int ngzmp_hist_clear_wait(int path, int card);
int ngzmp_hist_clear_start_tpgen(int path, int card, int start_word, int num_words, int all_streams);
int ngzmp_hist_clear_all_start(int path, int card, int start_word, int num_words);
int ngzmp_hist_clear_stream_start(int path, int card, int stream, int start_word, int num_words);

int ngzmp_hist_write_chan_config(int path, int chan, NGZMPHistConf *hist_conf);
int ngzmp_hist_read_chan_config(int path, int chan, NGZMPHistConf *hist_conf);

int ngzmp_system_start_count_enb(int path, int card, int num_pb_streams, int num_streams, int run_flags, int pb_num_t);

int ngzmp_spi_write_dga(int path, int chan, int num,  u_int16_t* value);
int ngzmp_spi_read_dga(int path, int chan, int num,  u_int16_t* value);

int ngzmp_spi_write_adc_glob(int path, int card, int adc, int addr, int n, u_int8_t * tx_data);
int ngzmp_spi_read_adc_glob(int path, int card, int adc, int addr, int n, u_int8_t *rx_data);
int ngzmp_spi_write_adc(int path, int card, int adc, int pair_sel, int chan_sel,  int addr, int n, u_int8_t *tx_data);
int ngzmp_spi_read_adc(int path, int card, int adc, int pair_sel, int chan_sel,  int addr, int n, u_int8_t *rx_data);
int ngzmp_spi_write_adc_multiple(int path, int card, int adc, int pair_sel, int chan_sel,  NGZMPADCAddrData *addr_data);

int ngzmp_spi_write_clk(int path, int card, int addr, int n, u_int8_t *data);
int ngzmp_spi_read_clk(int path, int card, int addr, int n, u_int8_t *data);
int ngzmp_hist_read_row(int path, int card, int row_len, int num_streams, int row, u_int32_t offset, u_int32_t size, u_int32_t *data);
int ngzmp_hist_setup(int path, int chan, NGZMPHistConf hist_conf);
int ngzmp_hist_read_chan(int path, int chan, u_int32_t offset, u_int32_t size, u_int32_t *data);

int ngzmp_i2c_write_reg(int path, int card, int bus, int addr, int size, u_int8_t* value);
int ngzmp_i2c_read_reg(int path, int card, int bus, int addr, int size, u_int8_t* value);
int ngzmp_i2c_write_reg_addr(int path, int card, int bus, int addr, int reg_addr, int size, u_int8_t* value);
int ngzmp_i2c_read_reg_addr(int path, int card, int bus, int addr, int reg_addr, int size, u_int8_t* value);
int ngzmp_i2c_read_reg_addr_padded(int path, int card, int bus, int addr, int reg_addr, int reg_addr_size, int size, u_int8_t* value);

int ngpd_i2c_read_adc_temp(int path, int card, float *temp, int *status); 
int ngpd_i2c_read_preamp_temp(int path, int card, float *temp, int *status);
int ngpd_i2c_read_temp(int path, int card, float *temp, int *status, int bus, int num_adt7410, char *name);
int ngpd_i2c_write_adc_tcrit(int path, int card, int chip, int tcrit);
int ngpd_i2c_write_preamp_tcrit(int path, int card, int chip, int tcrit);
int ngpd_i2c_write_tcrit(int path, int card, int chip, int tcrit, int bus, int num_adt7410, char * name);

int ngpd_i2c_write_lmk61e2_from_file(int path, int card, char *fname);
int ngpd_i2c_write_lmk61e2(int path, int card, int num, NGZMPI2CAddrData *reg);
int ngpd_i2c_save_lmk61e2_to_eeprom(int path, int card);
int ngpd_i2c_read_lmk61e2(int path, int card, int reg_addr, int num, u_int8_t *reg);
int ngpd_i2c_write_preamp_offset(int path, int chan, int num, u_int16_t *data);
int ngpd_i2c_read_preamp_offset(int path, int chan, int num, u_int16_t *data);

int ngzmp_i2c_write_reg_addr_page(int path, int card, int bus, int page, int addr, int command, int size, u_int8_t* value);
int ngzmp_i2c_read_reg_addr_page(int path, int card, int bus, int page, int addr, int command, int size, u_int8_t* value);
int ngzmp_i2c_read_ucd90160_vout(int path, int card, int chip, int page, int num,  int32_t* vout);

int ngpd_spi_write_lmk04828_from_file(int path, int card, char *fname);
int ngpd_spi_write_lmk04828(int path, int card, int num, NGZMPSPIAddrData *reg);
int ngpd_clock_send_sysref(int path, int card, int num_pulse_code);

int ngpd_clock_check_locked(int path, int card);
int ngpd_clock_wait_locked(int path, int card);

int ngzmp_adc_setup_adc(int path, int card, NGZMPADCFlags flags);
int ngzmp_adc_read_status(int path, int card);
int ngzmp_adc_setup_test(int path, int card, int adc, int pair_sel, int chan_sel, int test_mode, u_int16_t *user_pat);
int ngzmp_adc_setup_jesd_test(int path, int card, int adc, int pair_sel, int test_mode, int injection, u_int16_t *user_pat);
int ngzmp_adc_setup_output(int path, int card, int adc, int pair_sel, int vout, int preemphasis);
int ngzmp_adc_setup_sync(int path, int card, int adc, int pair_sel, int mode, int invert);
int ngzmp_adc_readback_regs(int path, int card);

int ngpd_read_jesd_regs(int path, int card, int link, int offset, int num_regs, u_int32_t *data);
int ngpd_write_jesd_regs(int path, int card, int link, int offset, int num_regs, u_int32_t *data);
int ngzmp_jesd_reset(int path, int card, int link);
int ngzmp_jesd_sysref(int path, int card, int link, int always, int delay, int required_resync);
int ngzmp_read_xadc(int path, int card, int first, int num, int32_t *data);
int ngzmp_hist_enable(int path, int card, NGZMPHistEnable enable);
int ngzmp_scope_read(int path, int card, int t, int dt);
int ngzmp_system_stop(int path, int card);

extern char ngpd_error_message[];
extern int ngzmp_bram_size_table[NGZMP_NUM_REGIONS];
extern int ngzmp_bram_width_table[NGZMP_NUM_REGIONS];
extern const char *ngzmp_bram_name[NGZMP_NUM_REGIONS];
extern const char *ngzmp_dataset_name[NGZMP_NUM_REGIONS];


extern const char *ngzmp_scope_alt_names_ana[16][16];
extern const char *ngzmp_scope_alt_names_s3and7[16][16];
extern const char *ngzmp_scope_alt_names_s8[16][16];
extern const char *ngzmp_scope_alt_names_s9[16][16];
extern const char *ngzmp_scope_name_ana[NGZMP_SCOPE_NUM_SRC_ANA];
extern const char *ngzmp_scope_name_s3[NGZMP_SCOPE_NUM_SRC3];
extern const char *ngzmp_scope_name_s7[NGZMP_SCOPE_NUM_SRC7];
extern const char *ngzmp_scope_name_s8[NGZMP_SCOPE_NUM_SRC8];
extern const char *ngzmp_scope_name_s9[NGZMP_SCOPE_NUM_SRC9];
extern const int ngzmp_scale_to_16bit_ana[16][16];
extern const char *ngpd_ucd90160_signals[NGZMP_UCD90160_NUM_CHIPS][NGZMP_UCD90160_NUM_RAILS];
extern const char *ngpd_ucd90160_signals_python[NGZMP_UCD90160_NUM_CHIPS][NGZMP_UCD90160_NUM_RAILS];

#define AD9694_SPI_GLOB_CONF_A			0
#define AD9694_SPI_GLOB_CONF_B			1
#define AD9694_SPI_CHAN_CHIP_CONF		2
#define AD9694_SPI_PAIR_CHIP_TYPE		3
#define AD9694_SPI_PAIR_CHIP_ID			4
#define AD9694_SPI_PAIR_MAP_CHAN_INDEX	8
#define AD9694_SPI_GLOB_MAP_PAIR_INDEX	9
#define AD9694_SPI_PAIR_SCRATCH_PAD		0x000A
#define AD9694_SPI_PAIR_SPI_REVISION	0x000B
#define AD9694_SPI_PAIR_VENDOR_LSB		0x000C
#define AD9694_SPI_PAIR_VENDOR_MSB		0x000D
#define AD9694_SPI_CHAN_POWER_DOWN		0x003F

#define AD9694_SPI_PAIR_PIN_CONT1		0x0040	// Can output Local Multi Frame Clock LMFC if requried for debug 
#define AD9694_SPI_PAIR_CLK_DIVIDER		0x0108	// 0 = > div 1
#define AD9694_SPI_PAIR_CLK_PHASE		0x0109	// Default 0 => no delay

#define AD9694_SPI_PAIR_SYSREF_CONT_CLK	0x010A	// Can can adjust capture of SYSREF.
#define AD9694_SPI_PAIR_CLOCK_DELAY		0x0110	// Would allow fine delay matching of sample time, defualt no delay
#define AD9694_SPI_CHAN_SUPER_FINE_DELAY 0x0111
#define AD9694_SPI_CHAN_FINE_DELAY		0x0112
#define AD9694_SPI_PAIR_CLOCK_DETECTION	0x011A	// default required clock > 300 MSa/s ... OK.
#define AD9694_SPI_PAIR_CLOCK_STATUS	0x011B	// CAn be used to confirm clock input detected.
#define AD9694_SPI_GLOB_CLOCK_CONT1		0x011C	// May want to enable DCS 1
#define AD9694_SPI_GLOB_CLOCK_CONT2		0x011E	// May want to enable DCS 2 and 1 to same value.
#define AD9694_SPI_GLOB_CLOCK_CONT3		0x011F	// May want to enable DCS 2 and 1 to same value.

#define AD9694_SPI__
#define AD9694_SPI_PAIR_SYSREF_CONT1	0x0120	// Will need to setup sysref input
#define AD9694_SPI_PAIR_SYSREF_CONT2	0x0121	// Setup count fro n-shot sysref if using burst.
#define AD9694_SPI_PAIR_SYSREF_CONT4	0x0123	// Allows delay.
#define AD9694_SPI_PAIR_SYSREF_STATUS1	0x0128	// Read back of SYSREF setup/hold time.
#define AD9694_SPI_PAIR_SYSREF_STATUS2	0x0129	// Read back of SYSREF phase. Presumably only if dividing input clock
#define AD9694_SPI_PAIR_SYSREF_STATUS3	0x012A	// Read back count of number of SYSREF events counted.
#define AD9694_SPI_PAIR_CHIP_SYNC		0x01FF	// Default 0 uses SYSREF to sync multiple devices

#define AD9694_SPI_PAIR_CHIP_MODE		0x0200	// Default 0 = Full bandwidth mode OK.
#define AD9694_SPI_PAIR_CHIP_DECIMATION	0x0201	// Default 0 => full sample rate.
#define AD9694_SPI_CHAN_OFFSET			0x0228	// Can adjust offset +128 to -128 codes.
#define AD9694_SPI_CHAN_FAST_DET_CONT 	0x0245
#define AD9694_SPI_CHAN_FDET_UPP_THRES_L 0x0247
#define AD9694_SPI_CHAN_FDET_UPP_THRES_H 0x0248
#define AD9694_SPI_CHAN_FDET_LOW_THRES_L 0x0249
#define AD9694_SPI_CHAN_FDET_LOW_THRES_H 0x024A
#define AD9694_SPI_CHAN_FDET_DWELL_L 	0x024B
#define AD9694_SPI_CHAN_FDET_DWELL_H 	0x024C

#define AD9694_SPI_PAIR_MONITOR_SYNC	0x026F	// MAy need to write to get synchronisation

#define AD9694_SPI_CHAN_SIG_MON_CONT	0x0270
#define AD9694_SPI_CHAN_SIG_MON_PERIOD0	0x0271
#define AD9694_SPI_CHAN_SIG_MON_PERIOD1	0x0272
#define AD9694_SPI_CHAN_SIG_MON_PERIOD2	0x0273
#define AD9694_SPI_CHAN_SIG_MON_STAT_CONT 0x0274
#define AD9694_SPI_CHAN_SIG_MON_STATUS0	0x0275
#define AD9694_SPI_CHAN_SIG_MON_STATUS1	0x0276
#define AD9694_SPI_CHAN_SIG_MON_STATUS2	0x0277

#define AD9694_SPI_CHAN_SIG_MON_FRM_COUNT	0x0278
#define AD9694_SPI_CHAN_SIG_MON_FRM_CONT	0x0279

#define AD9694_SPI_PAIR_DDC_SYNC_CONT		0x0300
#define AD9694_SPI_PAIR_DDC_TEST_ENABLE	0x0347
#define AD9694_SPI_CHAN_TEST_MODE		0x0550
#define AD9694_SPI_PAIR_TEST_USER1_L	0x0551
#define AD9694_SPI_PAIR_TEST_USER1_H	0x0552
#define AD9694_SPI_PAIR_TEST_USER2_L	0x0553
#define AD9694_SPI_PAIR_TEST_USER2_H	0x0554
#define AD9694_SPI_PAIR_TEST_USER3_L	0x0555
#define AD9694_SPI_PAIR_TEST_USER3_H	0x0556
#define AD9694_SPI_PAIR_TEST_USER4_L	0x0557
#define AD9694_SPI_PAIR_TEST_USER4_H	0x0558
#define AD9694_SPI_PAIR_OUTPUT_MODE0	0x0559
#define AD9694_SPI_PAIR_OUTPUT_MODE1	0x055A

#define AD9694_SPI_PAIR_SAMPLE_MODE		0x0561	// Defaulty is Twos complement binary, non-inveted. can change here if necessary
#define AD9694_SPI_PAIR_CHAN_SELECT		0x0564	// Can swap channels.
#define AD9694_SPI_PAIR_JESD_PLL_CONT	0x056E	// default 0 supports 6.75 to 13.5 Gbps
#define AD9694_SPI_PAIR_JESD_PLL_STATUS	0x056F	// Can check that the JESD PLL is in lock.
#define AD9694_SPI_PAIR_JESD_CONFIG		0x0570	// Will need to setup
#define AD9694_SPI_PAIR_JESD_LINK_CONT1 0x0571	// Can modify lane initialisation
#define AD9694_SPI_PAIR_JESD_LINK_CONT2	0x0572	// Can change SYNC polarity and behaviour.
#define AD9694_SPI_PAIR_JESD_LINK_CONT3	0x0573	// Enables test modes
#define AD9694_SPI_PAIR_JESD_LINK_CONT4	0x0574	// Can change ILAS timing.
#define AD9694_SPI_PAIR_JESD_LINK_JTX_OFFSET 0x0578	// Can adjust timing.
#define AD9694_SPI_PAIR_JESD_JTX_DID	0x0580	// Device ID
#define AD9694_SPI_PAIR_JESD_JTX_BID	0x0581	// Bank ID
#define AD9694_SPI_PAIR_JESD_JTX_LID0	0x0583	// Lane ID 0
#define AD9694_SPI_PAIR_JESD_JTX_LID1	0x0585	// Lane ID 0
#define AD9694_SPI_PAIR_JESD_JTX_SCR_L	0x058B	// Scrambler and Lanes per link.
#define AD9694_SPI_PAIR_JESD_JTX_F		0x058C	// Read only
#define AD9694_SPI_PAIR_JESD_JTX_K		0x058D	// planned K=32, write 0x1F
#define AD9694_SPI_PAIR_JESD_JTX_M		0x058E	// readback of M
#define AD9694_SPI_PAIR_JESD_JTX_CS_N	0x058F	// select which control bits, default 0 will do.
#define AD9694_SPI_PAIR_JESD_JTX_NP		0x0590	// default subclas 1, 16 bit data will do.
#define AD9694_SPI_PAIR_JESD_JTX_S		0x0591	// read only
#define AD9694_SPI_PAIR_JESD_JTX_HD_CF	0x0592	// High density fomrat read only
#define AD9694_SPI_PAIR_JESD_JTX_CSUM0	0x05A0	// Lane 0 checksum, read only	
#define AD9694_SPI_PAIR_JESD_JTX_CSUM1	0x05A1	// Lane 0 checksum, read only	
#define AD9694_SPI_PAIR_JESD_POWER_DOWN	0x05B0
#define AD9694_SPI_PAIR_JESD_JTX_LANE_ASS1 0x05B2	// Can change Lane assignment .. accept default.
#define AD9694_SPI_PAIR_JESD_JTX_LANE_ASS2 0x05B3	// Can change Lane assignment .. accept default.
#define AD9694_SPI_PAIR_JESD_DRIVE0		0x05C0	// Can turn up drive if necessary.
#define AD9694_SPI_PAIR_JESD_DRIVE1		0x05C1	// Can turn up drive if necessary.
#define AD9694_SPI_PAIR_JESD_PREEMPH0	0x05C4	// Can apply pre-emphasis
#define AD9694_SPI_PAIR_JESD_PREEMPH1	0x05C6	// Can apply pre-emphasis
#define AD9694_SPI_GLOB_JESD_LARGE_DITHER	0x0922	// default is enabled?
#define AD9694_SPI_GLOB_JESD_PLL_CAL	0x1222
#define AD9694_SPI_GLOB_JESD_STARTUP	0x1228
#define AD9694_SPI_GLOB_JESD_LOL_CLR	0x1262
#define AD9694_SPI_GLOB_DC_CALIB_CONT	0x0701
#define AD9694_SPI_GLOB_DC_CALIB_CONT2	0x073B
#define AD9694_SPI_PAIR_VREF_CONT		0x18A6		// Default 0 is internal reference.
#define AD9694_SPI_GLOB_VCM_CONT1		0x18E0
#define AD9694_SPI_GLOB_VCM_CONT2		0x18E1
#define AD9694_SPI_GLOB_VCM_CONT3		0x18E2
#define AD9694_SPI_GLOB_VCM_CONT		0x18E3
#define AD9694_SPI_GLOB_DIODE_TEMP		0x18E6
#define AD9694_SPI_CHAN_ANA_INP			0x1908	// Will want to switch to DC coupled.
#define AD9694_SPI_CHAN_ANA_FULLSCALE	0x1910	// Default 1.8V, may/will want to adjust.
#define AD9694_SPI_CHAN_BUFFER_CONT1	0x1A4C
#define AD9694_SPI_CHAN_BUFFER_CONT2	0x1A4D


#define AD9694_TEST_MODE_OFF			0		//!< Test mode off, output normal ADC data
#define AD9694_TEST_MODE_MIDSCALE		1		//!< Test mode output midscale 0x1FFF
#define AD9694_TEST_MODE_POS_FULL		2		//!< Test mode output full scale 0x3FFF
#define AD9694_TEST_MODE_NEG_FULL		3		//!< Test mode output 0 
#define AD9694_TEST_MODE_CHECKERBOARD	4		//!< Test mode output 10101010101010 
#define AD9694_TEST_MODE_PRBS_LONG		5		//!< Test mode output PRBS X^23+X^18+1
#define AD9694_TEST_MODE_PRBS_SHORT		6		//!< Test mode output PRBS X^9+X^5+1
#define AD9694_TEST_MODE_TOGGLE			7		//!< Test mode output toggle all 0 all 1
#define AD9694_TEST_MODE_USER			8		//!< Test mode output user pattern 
#define AD9694_TEST_MODE_RAMP			15		//!< Test mode output ramp.

#define AD9694_JESD_TEST_MODE_OFF			0		//!< Test mode off, output normal ADC data
#define AD9694_JESD_TEST_MODE_CHECKERBOARD	1		//!< Test mode output 10101010101010 
#define AD9694_JESD_TEST_MODE_TOGGLE		2		//!< Test mode output toggle all 0 all 1
#define AD9694_JESD_TEST_MODE_PRBS31		3		//!< Test mode output PRBS X^31+X^28+1
#define AD9694_JESD_TEST_MODE_PRBS23		4		//!< Test mode output PRBS X^23+X^18+1
#define AD9694_JESD_TEST_MODE_PRBS15		5		//!< Test mode output PRBS X^15+X^14+1
#define AD9694_JESD_TEST_MODE_PRBS9			6		//!< Test mode output PRBS X^9+X^5+1
#define AD9694_JESD_TEST_MODE_PRBS7			7		//!< Test mode output PRBS X^7+X^6+1
#define AD9694_JESD_TEST_MODE_RAMP			8		//!< Test mode output RAMP
#define AD9694_JESD_TEST_MODE_CONT			14		//!< Test mode output Continuous repeat of user test
#define AD9694_JESD_TEST_MODE_SINGLE		15		//!< Test mode output single repeat of user test

#define AD9694_JESD_TEST_INJECT_INPUT		0		//!< Inject at the N' bit Input to JESD block
#define AD9694_JESD_TEST_INJECT_PHY			1		//!< Inject at the input to the PHY, after 8b/10 for PHY tetsing.
#define AD9694_JESD_TEST_INJECT_SCRAMBLER	2		//!< Inject at 8 bit data the input to the scrambler.

#define AD9694_SPI_GLOB_CONF_A_DEFAULT 0
#define AD9694_SPI_GLOB_CONF_B_DEFAULT 0

#define JESD_IP_VERSION				(0)
#define JESD_RESET					(0x4>>2)
#define JESD_SYSREF					(0x010>>2)

#define JESD_STAT_RX_ERR			(0x01C>>2)
#define JESD_CFG_OCTETS				(0x020>>2)
#define JESD_CFG_FRAMES				(0x024>>2)
#define JESD_CFG_LANES				(0x028>>2)
#define JESD_CFG_SUBCLASS			(0x02C>>2)

#define JESD_ERROR_REPORTING		(0x034>>2)

#define JESD_SYNC_STATUS			(0x038>>2)
#define JESD_STAT_RX_DEBUG			(0x03C>>2)

#define JESD_SYSREF_REQUIRE_RESYNC	(1<<16)			//!< Require SYSREF event to allow link to 
#define JESD_SYSREF_DELAY(x)		(((x)&0xF)<<8)	//!< Delay from System to LMFC
#define JESD_SYSREF_ALWAYS			(1<<0)			//!< Core re-aligns LMFC an all SYSREF events.

#define JESD_SYSREF_ALWAYS_DEFAULT		1
#define JESD_SYSREF_DELAY_DEFAULT		0
#define JESD_SYSREF_RESYNC_DEFAULT		1

#define JESD204C_IP_VERSION				(0)
#define JESD204C_IP_CONFIG				(4>>2)
#define JESD204C_RESET					(0x20>>2)
#define JESD204C_CTRL_8B10B_CFG			(0x03C>>2)

#define JESD204C_STAT_RX_ERR			(0x058>>2)
#define JESD204C_STAT_RX_DEBUG			(0x05C>>2)
#define JESD204C_STAT_STATUS			(0x060>>2)


#endif
