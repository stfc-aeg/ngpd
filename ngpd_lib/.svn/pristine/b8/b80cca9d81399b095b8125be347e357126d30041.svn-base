/**
 * ----------------------------------------------------------------------------
 * ZYNMP  embedded platform
 *
 * William Helsby (william.Helsby@stfc.ac.uk)
 * Technology, STFC Daresbury Lab.
 *
 * ----------------------------------------------------------------------------
 *
 * OVERVIEW:
 *
 * Provides Binary TCP sockets base command protocol between the server on Arm Linux on ZynqMP and a client, typically x86_64 linux.
 * remote register read / write functionality and full support for I2C
 * devices (M24C08 8k EEPROM, LM82 monitoring chip) using a simple socket interface.
 *
 * Provides support for FPM (Frontend Personality Modules), providing application specific
 * functionality through special commands.
 * 
 * ----------------------------------------------------------------------------
 *
 * Version 1.0 - not for distribution
 *
 * ----------------------------------------------------------------------------
 *
 * CHANGELOG:
 *
 * 1.0		23-Feb-2021 
 * 	- Derrive from FEM-2 code use on Zynq.
 *
 * ----------------------------------------------------------------------------
 *
 * TO DO LIST: (in order of descending importance)
 *
 * ----------------------------------------------------------------------------
 */



// ----------------------------------------------------------------------------
// Includes
// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include "args.h"

// Protocol command processor
#include "fem.h"
#include "femConfig.h"
#include "commandProcessor.h"
#include "ngzmp.h"
#include "zynqmp_xadc.h"
// FEM Personality module template
//#include "personality.h"

int force_default=0;		// Set to 1 to build embedded software to use fall-back settings
int dev_board=0;

// ----------------------------------------------------------------------------
// Functions
// ----------------------------------------------------------------------------

int zynqmp_path;
char *progname;
int num_chan=0;
NGZMPFanControl fan_cont ;

void *fanSpeedControl(void * unused);

int main(int argc, char* argv[])
{
//	int i, rc;
//	u_int32_t rev,egs;
//	pthread_t  fan_speed_thread;

	progname = argv[0];

	do_args(argc, argv);

	printf("ZynqMP hardware server\n");

	zynqmp_path = ngzmp_config_details(1, 0, NULL, 0, NULL, 0, 0, NULL, 0, 0, 1);
	if (zynqmp_path < 0)
	{
		fprintf(stderr, "Cannot open file to device drivr. errno=%d\n", errno);
		exit(errno);
	}

	zynqmp_xadc_init(ZYNQMP_XADC_INIT_FULL);
	if (ngpd_i2c_write_adc_tcrit(zynqmp_path, -1, -1, 50) < 0)
	{
		printf("ERROR: Cannot set over temperature protection on ADC board\n");
	}
	if (ngpd_i2c_write_preamp_tcrit(zynqmp_path, -1, -1, 50) < 0)
	{
		printf("ERROR: Cannot set over temperature protection on pre-amp  board\n");
	}
	
#if 0
	printf("Chan\tRevision\tRegs Present\n");
	for (i=0;i<15; i++)
	{
		rc = xsp3_read_reg(zynqmp_path, i, XSP3_REGION_REGS, XSP3_REVISION, 1, &rev);
		if (rc != XSP3_OK) {
			printf("Error on calling xsp3_read_reg(): %d\n", rc);
		}
		rc = xsp3_read_reg(zynqmp_path, i, XSP3_REGION_REGS, XSP3_REG_PRESENT, 1, &regs);
		if (rc != XSP3_OK) {
			printf("Error on calling xsp3_read_reg(): %d\n", rc);
		}
		printf("%d\t0x%08X\t0x%08X\n", i, rev, regs);
		if (regs != 0)
			num_chan++;
	}
	if (xsp3_get_generation(zynqmp_path, 0) == XspressGen3Mini)
	{
		u_int32_t adc_cont = XSP3M_ADC_CONT_PSU_ENB;	//!< PSU Enable	Enable PSU to ADC board.
		xsp4_write_glob_reg(zynqmp_path, 0, XSP4_GLOB_ADC_CONT, 1, &adc_cont);
		pthread_create(&fan_speed_thread, NULL, fanSpeedControl, NULL);
	}
	else
	{
		u_int32_t adc_cont = XSP4_ADC_CONT_PSU_ENB;	//!< PSU Enable	Enable PSU to ADC board.
		xsp4_write_glob_reg(zynqmp_path, 0, XSP4_GLOB_ADC_CONT, 1, &adc_cont);
		xsp4_write_spi_par_exp(zynqmp_path, 0, XSP4_SPI_MID_PLANE_CHIP_TERM, XSP4_PCA9502_ADDR_DATA, 0x0);		// Turn off Output and Input termination.
		xsp4_write_spi_par_exp(zynqmp_path, 0, XSP4_SPI_MID_PLANE_CHIP_TERM, XSP4_PCA9502_ADDR_IODIR, 0xFF);
	}
	sleep(2);	// Allow PSUs to sequence on before starting writing the DACS.
	for (i=0; i<num_chan;i++)
	{
		u_int16_t dac=32768;	// Default DAC value is mid range
		printf("Chan %d: Writing DAC=%d\n", i, dac);
		xsp3_write_chan_spi_reg(zynqmp_path, i, XSP4_SPI_ADDR_REGION_DAC, 1, &dac);
	}
#endif
	command_processor();

    return 0;
}


