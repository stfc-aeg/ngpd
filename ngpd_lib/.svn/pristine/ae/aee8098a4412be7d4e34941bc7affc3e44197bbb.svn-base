#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include <stdlib.h>


#include "ngpd.h"
#include "zynqmp_protocol.h"
#include "zynqmp_api.h"


static NGZMPADCAddrData adc_regs[] = 
{
	{AD9694_SPI_CHAN_CHIP_CONF, 	3, 		NGZMPADC_DontRead},	// Power down links until link is configured
// AD9694_SPI_PAIR_PIN_CONT1		0x0040	// Can output Local Multi Frame Clock LMFC if requried for debug 
	{AD9694_SPI_PAIR_CLK_DIVIDER, 	0, 		NGZMPADC_Pair },	//  0 = > div 1, use 500 MHz sampling clock. Can consider 1 GHz clcok and dvide 2, an no DCS
	{AD9694_SPI_PAIR_CLK_PHASE,		0,		NGZMPADC_Pair },	// Default 0 => no delay

	{AD9694_SPI_PAIR_SYSREF_CONT_CLK, 0, 	NGZMPADC_Pair }, // Default Can can adjust capture of SYSREF.
	{AD9694_SPI_PAIR_CLOCK_DELAY,	0,		NGZMPADC_Pair },	// Would allow fine delay matching of sample time, defualt no delay
	{AD9694_SPI_GLOB_CLOCK_CONT3,	0x81,	NGZMPADC_DontRead },	// Sequence to enable Clock duty cycle stabilisor, since using 500 MHz sampling clock
	{AD9694_SPI_GLOB_CLOCK_CONT1,	0x09,	NGZMPADC_DontRead},
	{AD9694_SPI_GLOB_CLOCK_CONT2, 	0x09,	NGZMPADC_DontRead},
	{AD9694_SPI_GLOB_CLOCK_CONT1,	0x0B,	NGZMPADC_Glob},
	{AD9694_SPI_GLOB_CLOCK_CONT2, 	0x0B,	NGZMPADC_Glob},

	{AD9694_SPI_PAIR_SYSREF_CONT1,	0x0A, 	NGZMPADC_Pair },	// Continuous SYSREF captured on falling edge of clock.
	{AD9694_SPI_PAIR_CHIP_SYNC,		0, 		NGZMPADC_Pair },	// 0x01FF Default 0 uses SYSREF to sync multiple devices
	{AD9694_SPI_PAIR_SAMPLE_MODE,	0,		NGZMPADC_Pair },	// 0x0561	Change to offset binary mode.

	{AD9694_SPI_PAIR_CHIP_MODE,		0, 		NGZMPADC_Pair }, 	//	0x0200	Default 0 = Full bandwidth mode OK.
	{AD9694_SPI_PAIR_CHIP_DECIMATION, 0, 	NGZMPADC_Pair },	// 0x0201	Default 0 => full sample rate.
	{AD9694_SPI_PAIR_JESD_CONFIG,	0x49, 	NGZMPADC_Pair },	//	0x0570	From table 27 L=2., M=2, F=2, s=1, HD=0, N'=16 
	{AD9694_SPI_PAIR_JESD_JTX_K,	0x1F, 	NGZMPADC_Pair },	//	0x058D	K=32
	{AD9694_SPI_PAIR_JESD_JTX_CS_N,	0x0F, 	NGZMPADC_Pair },	//	0x058F	no control bits, and data padded to 16 bits
	{AD9694_SPI_CHAN_CHIP_CONF , 	0,		NGZMPADC_Chan },	// 2 		Power up links now configured
	
	/* Analogue input setup for DC coupled . datasheet page 25. */
	{AD9694_SPI_CHAN_ANA_INP,		4,		NGZMPADC_Chan },	//		0x1908	switch to DC coupled.
	{AD9694_SPI_PAIR_VREF_CONT,		0,		NGZMPADC_Pair },	// 0x18A6 Default 0 is internal reference.
	{AD9694_SPI_GLOB_DIODE_TEMP,	0,		NGZMPADC_Glob },  // 0x18E6 Turn off temperature diode export
	{AD9694_SPI_GLOB_VCM_CONT1,		0x04,	NGZMPADC_Glob },	// 0x18E0
	{AD9694_SPI_GLOB_VCM_CONT2,		0x1C,	NGZMPADC_Glob },	//	0x18E1
	{AD9694_SPI_GLOB_VCM_CONT3,		0x14,	NGZMPADC_Glob },	//  0x18E2
	{AD9694_SPI_GLOB_VCM_CONT,		0x4C,	NGZMPADC_Glob },	//	0x18E3	 Enabel VCM output with curretn buffer matching AD9694_SPI_CHAN_BUFFER_CONT1,2
	{AD9694_SPI_CHAN_ANA_FULLSCALE,	0,		NGZMPADC_Chan },  // 0x1910	Change TO 2.16 V
	{AD9694_SPI_CHAN_BUFFER_CONT1,	0x0C,	NGZMPADC_Chan },	// 0x1A4C  240 uA to use in First Nyquist region
	{AD9694_SPI_CHAN_BUFFER_CONT2,	0x0C,	NGZMPADC_Chan },	// 0x1A4D  240 uA to use in First Nyquist region
	{AD9694_SPI_GLOB_JESD_STARTUP,	0x4F,	NGZMPADC_DontRead | NGZMPADC_AddDelay },	//	0x1228  Reset the startup circuit
	{AD9694_SPI_GLOB_JESD_STARTUP,	0x0F,	NGZMPADC_Glob | NGZMPADC_AddDelay},	//	0x1228  Clear Reset on the startup circuit
	{AD9694_SPI_GLOB_JESD_PLL_CAL,	0x04,	NGZMPADC_DontRead | NGZMPADC_AddDelay}, // 0x1222   Start calibrtion of JESD PLL	
	{AD9694_SPI_GLOB_JESD_PLL_CAL,	0x00,	NGZMPADC_Glob | NGZMPADC_AddDelay }, // 0x1222   Restore normal operation
	{AD9694_SPI_GLOB_JESD_LOL_CLR,	0x08,	NGZMPADC_DontRead }, // 0x1262   Clear Loss of Lock signal
	{AD9694_SPI_GLOB_JESD_LOL_CLR,	0x00,	NGZMPADC_Glob  }, // 0x1262   
	{-1, 0,	NGZMPADC_DontRead }
};

int ngzmp_adc_setup_adc(int path, int card, NGZMPADCFlags flags)
{
	int rc;
	u_int8_t reg;
	int adc = -1; /* Both */
	int pair_sel=3; /* Both */
	int chan_sel=3; /* Both */
	struct timespec delay;
	char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
	int link;
	u_int32_t reg32;
	
	reg = 0x81;	// Reset.
	if ((rc=ngzmp_spi_write_adc(path, card, adc, pair_sel, chan_sel, AD9694_SPI_GLOB_CONF_A, 1, &reg)) < 0 ) goto exit_error;
	delay.tv_sec = 0;
	delay.tv_nsec=5000000;	// 5 ms delay
	
	while (nanosleep(&delay, &delay) <0 && errno == EINTR);
	rc = ngzmp_spi_write_adc_multiple(path, card, adc, pair_sel, chan_sel,  adc_regs);
	if (rc < 0)
		goto exit_error;
	delay.tv_sec = 0;
	delay.tv_nsec=5000000;	// 5 ms delay
	/* delay to make sure PLL lock has occured */
	while (nanosleep(&delay, &delay) <0 && errno == EINTR);
	
	for (link=0; link<4; link++)
	{
		reg32 = 1;
		rc = ngpd_write_jesd_regs(path, card, link, JESD_RESET, 1, &reg32);
		if (rc < 0)
			goto exit_error;
		reg32 = 0;
	}
	return 0;
	
exit_error:
	strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
	old_msg[NGPD_MAX_ERROR_MESSAGE] = 0;
	snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_adc_read_status:   : %s", old_msg);
	return rc;
}


int ngzmp_adc_read_status(int path, int card)
{
	int rc;
	int adc, pair;
	u_int8_t clock_status, pll_status;
	char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
	

	for (adc=0;adc<2; adc++)
	{
		for (pair=1; pair<4; pair<<= 1)
		{
			rc = ngzmp_spi_read_adc(path, card, adc, pair, 0, AD9694_SPI_PAIR_CLOCK_STATUS, 1, &clock_status);
			if (rc < 0)
				goto exit_error;
			rc = ngzmp_spi_read_adc(path, card, adc, pair, 0, AD9694_SPI_PAIR_JESD_PLL_STATUS, 1, &pll_status);
			if (rc < 0)
				goto exit_error;
			printf("Card %d: ADC=%d, pair=%d : Clock Status=0x%02X, PLL_Status=0x%02X\n", card, adc, pair, clock_status, pll_status);
		}
	}
	return 0;

exit_error:
	strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
	old_msg[NGPD_MAX_ERROR_MESSAGE] = 0;
	snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_adc_read_status:   : %s", old_msg);
	return rc;
}


int ngzmp_adc_setup_test(int path, int card, int adc, int pair_sel, int chan_sel, int test_mode, u_int16_t *user_pat)
{
	int rc;
	u_int8_t regs[9];
	int n=1;
	char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
	
	regs[0] = test_mode;
	if (test_mode == AD9694_TEST_MODE_USER && user_pat != NULL)
	{
		int i;
		for (i=0;i<4; i++)
		{
			regs[2*i+1] = user_pat[i] & 0xFF;
			regs[2*i+2] = user_pat[i] >> 8;
		}
		n = 9;
	}
	if ((rc=ngzmp_spi_write_adc(path, card, adc, pair_sel, chan_sel, AD9694_SPI_CHAN_TEST_MODE, n, regs)) < 0 )
	{
		strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
		old_msg[NGPD_MAX_ERROR_MESSAGE] = 0;
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_adc_setup_test:   : %s", old_msg);
	}
	return rc;
}
