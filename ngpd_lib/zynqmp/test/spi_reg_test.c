/*
 * spi_reg_test.c
 *
 *  Created on: 29/6/2021
 *      Author: wih73
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "errors.h"
#include "ngpd.h"
#include "ngzmp.h"
#include "ngzmptest.h"

int test_spi_regs_dga_single(int path);
int test_spi_regs_dga_multiple(int path);
int test_spi_regs_adc_pair_index(int path, int card);
int test_spi_regs_clk_dclk0(int path, int card);
int test_spi_regs_clk_devid(int path, int card);

int spi_regs_test(int path)
{
	int card;
	mprintf(1, "TEST SPI REGISTERS\n");

	if (test_spi & TEST_SPI_DGA)
	{
		test_spi_regs_dga_single(path);
		test_spi_regs_dga_multiple(path);
	}
	for (card=0; card<num_cards; card++)
	{
		if (test_spi & TEST_SPI_ADC_GLOB)
			test_spi_regs_adc_pair_index(path, card);
		if (test_spi & TEST_SPI_CLK)
		{
			test_spi_regs_clk_devid(path, card);
			test_spi_regs_clk_dclk0(path, card);
		}
	}
	return 0;
}

int test_spi_regs_dga_single(int path)
{
	int rc, i;
	u_int16_t x, y;
	int errors = 0;		
	int io_errors=0;
	int chan;

	for (i=0;i<32; i++)
	{
		for (chan=0; chan<num_chan; chan++) 
		{
			x = (chan+19*i)&0xFF;
			if (manual_test)
			{
				printf("About to write 0x%02X to chan %d, press return to continue\n", x, chan);
				getchar();
			}
			rc = ngzmp_spi_write_dga(path, chan, 1, &x);
			if (rc != 0)
			{
#if defined(__x86_64__)
				mprintf(MP_ERR, "Error on writing SPI Register at channel %d : %d : %s\n", chan,  rc, ngpd_get_error_message());
#else
				mprintf(MP_ERR, "Error on writing SPI Register at channel %d : %d\n", chan,  rc);
#endif
				io_errors++;
			}
		}
		for (chan=0; chan<num_chan; chan++) 
		{
			x = (chan+19*i)&0xFF;
			if (manual_test)
			{
				printf("About to read from chan %d, press return to continue\n", chan);
				getchar();
			}
			rc = ngzmp_spi_read_dga(path, chan, 1, &y);
			if (rc != 0)
			{
				mprintf(MP_ERR, "Error on Reading SPI Register at channel %d : %d\n", chan,  rc);
				io_errors++;
			}
			if (x != y)
			{
				errors++;
				if (errors < max_errors)
					mprintf(MP_ERR, "ERROR: Chan  %d, Wrote %02X, read %02X\n", chan, x, y);
				else if (errors == max_errors)
					mprintf(MP_ERR, "Too Many Errors, counting\n");
				
			}
		}
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed DGA SPI Register single read/write tests\n");
	else
		mprintf(MP_ERR, "DGA SPI Register single read/write tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}

int test_spi_regs_dga_multiple(int path)
{
	int rc, i;
	u_int16_t x[NGZMP_MAX_CHAN], y[NGZMP_MAX_CHAN];
	int errors = 0;		
	int io_errors=0;
	int chan;

	for (i=0;i<32; i++)
	{
		if (i < 16)
		{
			for (chan=0; chan<num_chan; chan++) 
				x[chan] = (chan+21*i)&0xFF;
			if (manual_test)
			{
				printf("About to write array to all channels, press return to continue\n");
				getchar();
			}
			rc = ngzmp_spi_write_dga(path, 0, num_chan, x);
			if (rc != 0)
			{
				mprintf(MP_ERR, "Error on writing SPI Register at channel %d : %d\n", chan,  rc);
				io_errors++;
			}
		}
		else
		{
			x[0] = (23*i)&0xFF;
			for (chan=1; chan<num_chan; chan++) 
				x[chan] = 0; // Should not be used in broadcast
			if (manual_test)
			{
				printf("About to write one value=%02X to all channels, press return to continue\n", x[0]);
				getchar();
			}
			rc = ngzmp_spi_write_dga(path, -1, 0, x);
			if (rc != 0)
			{
				mprintf(MP_ERR, "Error on writing SPI Register at channel %d : %d\n", chan,  rc);
				io_errors++;
			}
			for (chan=1; chan<num_chan; chan++) 
				x[chan] = x[0]; 
		}
		rc = ngzmp_spi_read_dga(path, 0, num_chan, y);
		if (rc != 0)
		{
			mprintf(MP_ERR, "Error on Reading SPI Register at channel %d : %d\n", chan,  rc);
			io_errors++;
		}
		for (chan=0; chan<num_chan; chan++) 
		{
			if (x[chan] != y[chan])
			{
				errors++;
				if (errors < max_errors)
					mprintf(MP_ERR, "ERROR: Chan  %d, Wrote %02X, read %02X\n", chan, x[chan], y[chan]);
				else if (errors == max_errors)
					mprintf(MP_ERR, "Too Many Errors, counting\n");
				
			}
		}
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed DGA SPI Register block read/write tests\n");
	else
		mprintf(MP_ERR, "DGA SPI Register block read/write tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}


int test_spi_regs_adc_pair_index(int path, int card)
{
	int rc, i;
	u_int8_t x, y;
	int errors = 0;		
	int io_errors=0;
	int adc;

	for (i=0;i<4; i++)
	{
		for (adc=0; adc<NGZMP_SPI_NUM_ADC; adc++) 
		{
			x = (i+adc*3)&0x3;
			if (manual_test)
			{
				printf("About to write 0x%02X to ADC %d, address=%04X. Press return to continue\n", x, adc, AD9694_SPI_GLOB_MAP_PAIR_INDEX);
				getchar();
			}
			rc = ngzmp_spi_write_adc(path, card, adc, 0, 0, AD9694_SPI_GLOB_MAP_PAIR_INDEX, 1, &x);
			if (rc != 0)
			{
				mprintf(MP_ERR, "Error on writing SPI Register at ADC %d : %d\n", adc,  rc);
				io_errors++;
			}
		}
		for (adc=0; adc<NGZMP_SPI_NUM_ADC; adc++) 
		{
			x = (i+adc*3)&0x3;
			if (manual_test)
			{
				printf("About to read from ADC %d, press return to continue\n", adc);
				getchar();
			}
			rc = ngzmp_spi_read_adc(path, card, adc, 0, 0, AD9694_SPI_GLOB_MAP_PAIR_INDEX, 1, &y);
			if (rc != 0)
			{
				mprintf(MP_ERR, "Error on Reading SPI Register at ADC %d : %d\n", adc,  rc);
				io_errors++;
			}
			if (x != y)
			{
				errors++;
				if (errors < max_errors)
					mprintf(MP_ERR, "ERROR: ADC %d, glob map Pair index, Wrote %02X, read %02X\n", adc, x, y);
				else if (errors == max_errors)
					mprintf(MP_ERR, "Too Many Errors, counting\n");
				
			}
		}
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed ADC SPI Global Map Index Register read/write tests\n");
	else
		mprintf(MP_ERR, "ADC SPI Global Map Index Register read/write tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}

int test_spi_regs_clk_devid(int path, int card)
{
	u_int8_t x[4], cd[2];
	int errors = 0;		
	int io_errors=0;
	int rc;
	u_int8_t do_reset=0x80;
	
	rc = ngzmp_spi_write_clk(path, card, 0, 1, &do_reset);
	if (rc != 0)
	{
#if defined(__x86_64__)
		mprintf(MP_ERR, "Error on Writing Reset to SPI Register at LMK04828 : %d\n", rc, ngpd_get_error_message());
#else
		mprintf(MP_ERR, "Error on Writing Reset to SPI Register at LMK04828 : %d\n", rc);
#endif
		io_errors++;
	}
	
	rc = ngzmp_spi_read_clk(path, card, 0x3, 4, x);
	if (rc != 0)
	{
#if defined(__x86_64__)
		mprintf(MP_ERR, "Error on Reading Fixed SPI Register at LMK04828 : %d\n", rc, ngpd_get_error_message());
#else
		mprintf(MP_ERR, "Error on Reading Fixed SPI Register at LMK04828 : %d\n", rc);
#endif
		io_errors++;
	}

	rc = ngzmp_spi_read_clk(path, card, 0xc, 2, cd);
	if (rc != 0)
	{
#if defined(__x86_64__)
		mprintf(MP_ERR, "Error on Reading Fixed SPI Register at LMK04828 : %d\n", rc, ngpd_get_error_message());
#else
		mprintf(MP_ERR, "Error on Reading Fixed SPI Register at LMK04828 : %d\n", rc);
#endif
		io_errors++;
	}
	if (x[0] != 6 || /* x[1] != 208 || */ x[2] != 91 || x[3] != 32)
	{
		mprintf(MP_ERR, "ERROR reading fixed register 3..6 . Read (%d %d, %d, %d), expected (6, 208, 91, 32)\n", x[0], x[1], x[2], x[3]);
		errors++;
	}
	else
		mprintf(MP_PRINT, "Correct: fixed register 3..6 . Read (%d %d, %d, %d)\n", x[0], x[1], x[2], x[3]);
	if (cd[0] != 81 || cd[1] != 4)
	{
		mprintf(MP_ERR, "ERROR reading Vendor ID reg 0xC and 0xD: Read (%d, %d) expected (81, 4)\n", cd[0], cd[1]);
		errors++;
	}
	else
		mprintf(MP_ERR, "Correct: Vendor ID reg 0xC and 0xD: Read (%d, %d)\n", cd[0], cd[1]);
	if (errors == 0 && io_errors == 0)
		printf("Passed LMK04828 Fixed SPI Register read tests\n");
	else
		mprintf(MP_ERR, "LMK04828 Fixed SPI Register read tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}

int test_spi_regs_clk_dclk0(int path, int card)
{
	int rc, i, j;
	u_int8_t x[2], y[2];
	int errors = 0;		
	int io_errors=0;

	for (i=0;i<16; i++)
	{
		for (j=0; j<2; j++)
			x[j] = (i*0x31+0x12*j)&0xFF;
		x[0] &= 0x7F;		/* MSB of register 0x100 is shown as always 0 in the datasheet, so don't expect to write/read 1 */
		if (manual_test)
		{
			printf("About to write 0x%02X, %02X to LMK04828 address=%03X. Press return to continue\n", x[0], x[1], 0x101);
			getchar();
		}
		rc = ngzmp_spi_write_clk(path, card, 0x100, 2, x);
		if (rc != 0)
		{
#if defined(__x86_64__)
			mprintf(MP_ERR, "Error on writing SPI Register at LMK04828 : %d : %s\n",  rc, ngpd_get_error_message());
#else
			mprintf(MP_ERR, "Error on writing SPI Register at LMK04828 : %d\n",  rc);
#endif
			io_errors++;
		}
		if (manual_test)
		{
			printf("About to read from LMK04828, press return to continue\n");
			getchar();
		}
		rc = ngzmp_spi_read_clk(path, card, 0x100, 2, y);
		if (rc != 0)
		{
#if defined(__x86_64__)
			mprintf(MP_ERR, "Error on Reading SPI Register at LMK04828 : %d\n", rc, ngpd_get_error_message());
#else
			mprintf(MP_ERR, "Error on Reading SPI Register at LMK04828 : %d\n", rc);
#endif
			io_errors++;
		}
		if (x[0] != y[0] || x[1] != y[1])
		{
			errors++;
			if (errors < max_errors)
				mprintf(MP_ERR, "ERROR: LMK04828 address 0x%03X,%03X, Wrote %02X, %02X, read %02X, %02X\n", 0x101, 0x102, x[0], x[1], y[0], y[1] );
			else if (errors == max_errors)
				mprintf(MP_ERR, "Too Many Errors, counting\n");
		}
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed LMK04828 SPI Register read/write tests\n");
	else
		mprintf(MP_ERR, "LMK04828 SPI  Register read/write tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}
