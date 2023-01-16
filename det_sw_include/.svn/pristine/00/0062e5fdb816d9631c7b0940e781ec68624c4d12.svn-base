#define NUM_ADCS 8
#define NUM_CLKS 2

#define CLK_SPI_BUS 32765
#define ADC_SPI_BUS 32766

#define XH_SPI_BUS_CLK 		32765
#define XH_SPI_BUS_ADC 		32766

#define XH_SPI_BUS_HEAD512  32763
#define XH_SPI_BUS_HEAD1024 32764

#define XH_SPI_BUS_OFFSET 	32761
#define XH_SPI_BUS_HV 		32762

#define XH_SPI_NUM_OFFSET	5
#define XH_SPI_NUM_HEAD		4
#define XH_SPI_NUM_HV		4

#define XH_SPI_BUS_FUNC_HEAD0	0
#define XH_SPI_BUS_FUNC_HEAD1	1
/* Please insert extr heads here if necessary */
#define XH_SPI_BUS_FUNC_OFFSETS	2
#define XH_SPI_BUS_FUNC_HV_POS	3
#define XH_SPI_BUS_FUNC_HV_NEG	4

#define XH_SPI_CHIP_HEAD_ADC	0
#define XH_SPI_CHIP_HEAD_ID		1
#define XH_SPI_CHIP_HEAD_PEXP	2
#define XH_SPI_CHIP_HEAD_DAC	3

// HV SPI chip selects (bus func)
#define XH_SPI_CHIP_HV_ADC_POS	0
#define XH_SPI_CHIP_HV_DAC_POS	1
#define XH_SPI_CHIP_HV_ADC_NEG	2
#define XH_SPI_CHIP_HV_DAC_NEG	3

// HV DAC channel connections
#define XH_HV_DAC_ILIMIT 1
#define XH_HV_DAC_HVSETPOINT 0
#define XH_HV_DAC_HVOFFSET 2

// HV ADC channel connections
#define XH_HV_ADC_IMONITOR 0
#define XH_HV_ADC_HVMONITOR 1
#define XH_HV_ADC_12VMONITOR 2
#define XH_HV_ADC_5V2MONITOR 3

int aida_spi_open_adcs();
int aida_spi_close_adcs();
int aida_spi_adc_write(int chip, int addr, int n, char * data);
int aida_spi_adc_read(int chip, int addr, int n, char * data);
int aida_spi_adc_write_adc_func(int chip, int addr, int chan_mask, int do_xfer, int n, char * data);
int aida_spi_adc_read_adc_func(int chip, int addr, int chan_mask, int n, char * data);
int aida_spi_adc_reset(int first, int last);
int aida_spi_open_clks();
int aida_spi_clk_write(int chip, u_int32_t data);

int xh_spi_write_dac(int bus_func, int chip, int chan, int value);
int xh_spi_adc_read(int bus_func, int chan);
int xh_spi_adc_init(int bus_func);

#define AD9252_ADDR_CHIP_PORT_CONFIG 0
#define AD9252_ADDR_CHIP_ID  		1
#define AD9252_ADDR_CHIP_GRADE  	2
#define AD9252_ADDR_DEV_INDEX2  	4
#define AD9252_ADDR_DEV_INDEX1  	5

#define AD9252_CHIP_ID  		0x9

#define AD9252_ADDR_DEV_UPDATE 		0xff

#define AD9252_ADDR_MODES			0x08
#define AD9252_ADDR_CLOCK			0x09
#define AD9252_ADDR_TEST_IO			0x0D
#define AD9252_ADDR_OUTPUT_MODE		0x14
#define AD9252_ADDR_OUTPUT_ADJUST	0x15
#define AD9252_ADDR_OUTPUT_PHASE	0x16
#define AD9252_ADDR_USER_PATT1_LSB	0x19
#define AD9252_ADDR_USER_PATT1_MSB	0x1a
#define AD9252_ADDR_USER_PATT2_LSB	0x1b
#define AD9252_ADDR_USER_PATT2_MSB	0x1c
#define AD9252_ADDR_SERIAL_CONTROL	0x21
#define AD9252_ADDR_SERIAL_CH_STAT	0x22

#define AD9252_TEST_IO_OFF			0
#define AD9252_TEST_IO_MID			1
#define AD9252_TEST_IO_PFS			2
#define AD9252_TEST_IO_NFS			3
#define AD9252_TEST_IO_CHECKER		4
#define AD9252_TEST_IO_PN23			5
#define AD9252_TEST_IO_PN9			6 
#define AD9252_TEST_IO_WORD_TOGGLE	7
#define AD9252_TEST_IO_USER			0x48
#define AD9252_TEST_IO_BIT_TOGGLE	9
#define AD9252_TEST_IO_X1SYNC		0xA
#define AD9252_TEST_IO_1HIGH		0xB
#define AD9252_TEST_IO_MIXED		0xC

//#define AD9252_TEST_IO_OFF			0
//#define AD9252_TEST_IO_MID			1
//#define AD9252_TEST_IO_PFS			2
//#define AD9252_TEST_IO_NFS			3
//#define AD9252_TEST_IO_CHECKER		4
//#define AD9252_TEST_IO_PN23			5
//#define AD9252_TEST_IO_PN9			6
//#define AD9252_TEST_IO_WORD_TOGGLE	7
//#define AD9252_TEST_IO_USER			0x48
//#define AD9252_TEST_IO_BIT_TOGGLE	9
//#define AD9252_TEST_IO_X1SYNC		0xA
//#define AD9252_TEST_IO_1HIGH		0xB
//#define AD9252_TEST_IO_MIXED		0xC


// added to support testing of ad9257
#define AD9257_ADDR_CHIP_PORT_CONFIG 0x00
#define AD9257_ADDR_CHIP_ID  		0x01
#define AD9257_ADDR_CHIP_GRADE  	0x02
#define AD9257_ADDR_DEV_INDEX2  	0x04
#define AD9257_ADDR_DEV_INDEX1  	0x05

#define AD9257_CHIP_ID  			0x92
#define AD9257_CHIP_GRADE			0x30

#define AD9257_ADDR_DEV_UPDATE 		0xff
#define AD9257_ADDR_MODES			0x08 // default = chip run
#define AD9257_ADDR_CLOCK			0x09 // default = duty stabilise
#define AD9257_ADDR_CLOCK_DIVIDE	0x0b // default = divide by 1
#define AD9257_ADDR_ENHANCE_CNTRL	0x0c // default = disable chop
#define AD9257_ADDR_TEST_IO			0x0D // default = test mode off
#define AD9257_ADDR_OFFSET_ADJUST	0x10 // default = 0
#define AD9257_ADDR_OUTPUT_MODE		0x14 // default = 2'S COMP
#define AD9257_ADDR_OUTPUT_ADJUST	0x15 // default = 1x
#define AD9257_ADDR_OUTPUT_PHASE	0x16
#define AD9257_ADDR_VREF			0x18 // default = 2Vpp
#define AD9257_ADDR_USER_PATT1_LSB	0x19
#define AD9257_ADDR_USER_PATT1_MSB	0x1a
#define AD9257_ADDR_USER_PATT2_LSB	0x1b
#define AD9257_ADDR_USER_PATT2_MSB	0x1c
#define AD9257_ADDR_SERIAL_CONTROL	0x21
#define AD9257_ADDR_SERIAL_CH_STAT	0x22
#define AD9257_ADDR_RES_RATE_OVRIDE	0x100
#define AD9257_ADDR_USER_IO_CNTRL2	0x101
#define AD9257_ADDR_USER_IO_CNTRL3	0x102 //default = VCM powerdwn disabled
#define AD9257_ADDR_SYNC			0x109 // default = disabled

#define AD9257_TEST_IO_OFF			0x0
#define AD9257_TEST_IO_MID			0x1
#define AD9257_TEST_IO_PFS			0x2
#define AD9257_TEST_IO_NFS			0x3
#define AD9257_TEST_IO_CHECKER		0x4
#define AD9257_TEST_IO_PN23			0x5
#define AD9257_TEST_IO_PN9			0x6
#define AD9257_TEST_IO_WORD_TOGGLE	0x7
#define AD9257_TEST_IO_USER			0x8
#define AD9257_TEST_IO_BIT_TOGGLE	0x9
#define AD9257_TEST_IO_X1SYNC		0xA
#define AD9257_TEST_IO_1HIGH		0xB
#define AD9257_TEST_IO_MIXED		0xC


//ad9257 defaults
// for our use of the chip the powerup defaults for the follwoing registers are ok
//MODES
//CLOCK
//CLOCK_DIVIDE
//ENHANCE_CNTRL
//OUTPUT_ADJUST
//SYNC


// port expander stuff
#define MCP23S18_WRITE (0x40)
#define MCP23S18_READ (0x41)
#define MCP23S18_XH_CONFIG (0x26)
	// bank = 0  -- interleaved channel a/b addresses
	// seqop = 1 -- sequential addressing operation disabled
	// odr = 1 -- open drain output on int pin (dont care really but will make sure output is never driven)
	// intpol = 1 -- set int pin polarity to active high
#define MCP23S18_XH_IODIR (0x00)	
	// define lower 5 bits of port as output, top 3 bits as input
	// WIH 27/11/2017: For Canberra release of heasd board need top 3 bits as output. Does not harm for earlier head boards
#define MCP23S18_ADDR_IOCON (0x0a)
#define MCP23S18_ADDR_IODIR (0x00)
#define MCP23S18_ADDR_GPIOA (0x12)
#define MCP23S18_ADDR_OLATA (0x14)
int xh_spi_pexp_write_reg(int head, int reg, int value);
int xh_spi_pexp_init(int head);
int xh_spi_pexp_read(int head, int reg);

#define XH_PEXP_SETCA0	1
#define XH_PEXP_SEL8P	2
#define XH_PEXP_CALEN	4
#define XH_PEXP_SELSI	8
#define XH_PEXP_LED_OFF	16
#define XSTRIP_PEXP_LED_ON	16

#define XH_PEXP_SETCA1	32
#define XH_PEXP_SETCA2	64
#define XH_PEXP_SETCA3	128

#define XH_PEXP_SETCA_ALL	(XH_PEXP_SETCA0 | XH_PEXP_SETCA1 | XH_PEXP_SETCA2 | XH_PEXP_SETCA3)

/* Setup ADC, Power mode 3  (normal), No sequence, 0 to Vref, straight binary */

#define XH_ADC_CONFIG_HIGH (0x83)
#define XH_ADC_CONFIG_LOW (0x30)
#define ADC_DUMMY_WRITE (0x00)


#define XH_NUM_HEAD_PCB 2

#define XH_NUM_HEAD_DAC 8

#define XH_HEAD_DAC_VDD		0
#define XH_HEAD_DAC_VREF	1
#define XH_HEAD_DAC_VREFC	2
#define XH_HEAD_DAC_VRES1	3
#define XH_HEAD_DAC_VRES2	4
#define XH_HEAD_DAC_VPUPREF	5
#define XH_HEAD_DAC_VCLAMP	6
#define XH_HEAD_DAC_LED		7
	
#define XH_HEAD_ADC_VDD		0
#define XH_HEAD_ADC_VREF	1
#define XH_HEAD_ADC_VREFC	2


#define XH_HEAD_ADC_SCALE (2.5/1.5)
/* Amplifier input transistor current */
#define XCHIP3_IBIAS_IN(x)		((u_int64_t)((x)&0xF)<<38)
#define XCHIP3_IBIAS_IN_DEFAULT 	8
#define XCHIP3_VCASC(x)			((u_int64_t)((x)&0xFF)<<30)
#define XCHIP3_VCASC_DEFAULT 	0x60
#define XCHIP3_IBIAS_SF(x)		((u_int64_t)((x)&0xF)<<26)
#define XCHIP3_IBIAS_SF_DEFAULT	8
#define XCHIP3_BIAS_RC(x)		((u_int64_t)((x)&0xFF)<<18)	/* Compensation control voltage*/
// #define XCHIP3_BIAS_RC_DEFAULT	0x5C
#define XCHIP3_BIAS_RC_DEFAULT	140

#define XCHIP3_SEL3P_AB		(1<<17)				/* Adds 3pF feedback to blocks AB 	*/
#define XCHIP3_SEL5P_AB		(1<<15)				/* Adds 5pF feedback to blocks AB 	*/
#define XCHIP3_SEL10P_AB	(1<<13)				/* Adds 10pF feedback to blocks AB 	*/
#define XCHIP3_SEL30P_AB	(1<<11)				/* Adds 30pF feedback to blocks AB 	*/
#define XCHIP3_SEL3P_CD		(1<<16)				/* Adds 3pF feedback to blocks CD 	*/
#define XCHIP3_SEL5P_CD		(1<<14)				/* Adds 5pF feedback to blocks CD 	*/
#define XCHIP3_SEL10P_CD	(1<<12)				/* Adds 10pF feedback to blocks CD 	*/
#define XCHIP3_SEL30P_CD	(1<<10)				/* Adds 30pF feedback to blocks CD 	*/



#define XCHIP3_CLAMP_NEG	(1<<8)				/* Clamp enable for negative charge mode */
#define XCHIP3_CLAMP_POS	(1<<9)				/* Clamp disable for positive charge mode */
#define XCHIP3_RESET_RES_AB(x) (((x)&7)<<5)		/*	Enable bits for reset resistance on blocks AB */
#define XCHIP3_RESET_RES_AB_DEFAULT 7
#define XCHIP3_RESET_RES_CD(x)	(((x)&7)<<2)	/*	Enable bits for reset resistance on blocks CD */
#define XCHIP3_RESET_RES_CD_DEFAULT 7
#define XCHIP3_COMP_AB		(1<<0)				/* Adds compensation capacitance to blocks AB */
#define XCHIP3_COMP_CD		(1<<1)				/* Adds compensation capacitance to blocks CD */
#define XCHIP3_DEFAULT_TOP 	(XCHIP3_IBIAS_IN(XCHIP3_IBIAS_IN_DEFAULT) | XCHIP3_VCASC(XCHIP3_VCASC_DEFAULT) | XCHIP3_IBIAS_SF(XCHIP3_IBIAS_SF_DEFAULT) | XCHIP3_BIAS_RC(XCHIP3_BIAS_RC_DEFAULT))
#define XCHIP3_DEFAULT_BOT	(XCHIP3_RESET_RES_AB(XCHIP3_RESET_RES_AB_DEFAULT) |	XCHIP3_RESET_RES_CD(XCHIP3_RESET_RES_CD_DEFAULT) | XCHIP3_COMP_AB | XCHIP3_COMP_CD | XCHIP3_CLAMP_POS)
#define XCHIP3_DEFAULT (XCHIP3_DEFAULT_TOP | XCHIP3_DEFAULT_BOT)

#define XH_HEAD_EEPROM_WREN 0x6
#define XH_HEAD_EEPROM_WRDI 0x4
#define XH_HEAD_EEPROM_RD_STATUS 0x5
#define XH_HEAD_EEPROM_WRITE_STATUS 0x1
#define XH_HEAD_EEPROM_READ 0x3
#define XH_HEAD_EEPROM_WRITE 0x2
#define XH_HEAD_EEPROM_SIZE 512


int xh_spi_eeprom_read_status(int head);
int xh_spi_eeprom_write_status(int head, int value);

int xh_spi_eeprom_write_enable(int head);
int xh_spi_eeprom_write_disable(int head);

int xh_spi_eeprom_write(int head, int reg, int value);
int xh_spi_eeprom_read(int head, int reg);
