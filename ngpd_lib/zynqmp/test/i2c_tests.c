/*
 * i2c_tests.c
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

int test_i2c_preamp_dac(int path);
int test_ad7410_regs(int path, int card, int bus, int num_chips, char * name);
int test_ad7410_regs_preamp(int path, int card);
int test_ad7410_regs_adc(int path, int card);
int test_i2c_psu_pexp(int path, int card);
int test_i2c_sfp_pexp(int path, int card);
int test_i2c_rd_lmk61e2(int path, int card);
int test_i2c_rd_si5324(int path, int card);
int test_i2c_enclustra(int path, int card);
int test_i2c_ucd90160(int path, int card);
int test_i2c_ucd90160_vout(int path, int card);

int i2c_tests(int path)
{
	int card;
	mprintf(1, "TEST I2C BUSSES\n");

	if (test_i2c & TEST_I2C_OFFSET_DAC)
	{
		test_i2c_preamp_dac(path);
	}
	for (card=0; card<num_cards; card++)
	{
		if (test_i2c & TEST_I2C_PREAMP_ADT7410)
			test_ad7410_regs_preamp(path, card);
		if (test_i2c & TEST_I2C_ADC_ADT7410)
			test_ad7410_regs_adc(path, card);
		if (test_i2c & TEST_I2C_PSU_PEXP)
			test_i2c_psu_pexp(path, card);
		if (test_i2c & TEST_I2C_SFP_PEXP)
			test_i2c_sfp_pexp(path, card);
		if (test_i2c & TEST_I2C_CLK_LMK61E2)
			test_i2c_rd_lmk61e2(path, card);
		if (test_i2c & TEST_I2C_ESSCLK_SI5324)
			test_i2c_rd_si5324(path, card);
		if (test_i2c & TEST_I2C_ENCLUSTRA)
			test_i2c_enclustra(path, card);
		if (test_i2c & TEST_I2C_UCD90160)
		{
			test_i2c_ucd90160(path, card);
			test_i2c_ucd90160_vout(path, card);
		}
	}
	return 0;
}

int test_i2c_preamp_dac(int path)
{
	int rc, i;
	u_int16_t write_buff[NGZMP_MAX_CHAN], read_buff[NGZMP_MAX_CHAN];
	int errors = 0;		
	int io_errors=0;
	int chan;
	int start, num;
	int card;
	
	mprintf(1, "I2C-OFFSET: Writing and reading offset DACs of pre-amp card\n");

	for (i=0;i<64; i++)
	{
		if (num_cards == 1)
			start = (i & 7);
		else
		{
			card = (( i >> 1) & 3) % num_cards;
			start = (i&1)+ 8 * card;
		}
		if ( !(i & 32))
		{
			num = num_chan - start;
			num -= (i>>3) & 3;
			if (num <= 0)
				num = 1;
		}
		else
		{
			num = (i>>3) & 3;
			num++;
			if (start+num> num_chan)
				num = num_chan-start;
		}
		for (chan=0; chan<num_chan; chan++) 
			write_buff[chan] = 0xAAAA;
		if (manual_test)
		{
			printf("About to write 0xAAAA to all channels\n");
			getchar();
		}
		if ((rc=ngpd_i2c_write_preamp_offset(path, 0, num_chan, write_buff)) < 0)
		{
#if defined(__x86_64__)
			mprintf(MP_ERR, "Error on writing to all channel offset DACs : %d : %s\n", rc, ngpd_get_error_message());
#else
			mprintf(MP_ERR, "Error on writing to all channel offset DACs : %d\n", rc);
#endif
			io_errors++;
		}

		for (chan=start; chan<start+num; chan++) 
			write_buff[chan] = (0x400*i+0x0301*chan);
		
		if (manual_test)
		{
			printf("About to write to chan %d, for %d. Press return to continue\n", start, num);
			getchar();
		}
		if ((rc=ngpd_i2c_write_preamp_offset(path, start, num, write_buff+start)) < 0)
		{
#if defined(__x86_64__)
			mprintf(MP_ERR, "Error on writing offset DACs to channel=%d for %d channels  : %d : %s\n", start, num, rc, ngpd_get_error_message());
#else
			mprintf(MP_ERR, "Error on writing offset DACs to channel=%d for %d channels  : %d\n", start, num, rc);
#endif
			io_errors++;
		}

		if (manual_test)
		{
			printf("About to read from chan %d, press return to continue\n", start);
			getchar();
		}
		rc = ngpd_i2c_read_preamp_offset(path, start, num, read_buff+start);
		if (rc < 0)
		{
#if defined(__x86_64__)
			mprintf(MP_ERR, "Error on Reading I2C Register at from channel %d for %d: %s : %d\n", start, num,  rc, ngpd_get_error_message());
#else
			mprintf(MP_ERR, "Error on Reading I2C Register at from channel %d for %d: %d\n", start, num,  rc);
#endif
			io_errors++;
		}
		for (chan=start; chan < start+num; chan++)
		{
			if (read_buff[chan] != write_buff[chan])
			{
				errors++;
				if (errors < max_errors)
					mprintf(MP_ERR, "ERROR: Chan  %d, Wrote %04X, read %04X\n", chan, write_buff[chan], read_buff[chan]);
				else if (errors == max_errors)
					mprintf(MP_ERR, "Too Many Errors, counting\n");
			}
		}
	}
	write_buff[0] =  0;
	if ((rc=ngpd_i2c_write_preamp_offset(path, 0, -1, write_buff)) < 0)
	{
#if defined(__x86_64__)
		mprintf(MP_ERR, "Error on writing offset DACs All 0  : %d : %s\n",rc, ngpd_get_error_message());
#else
		mprintf(MP_ERR, "Error on writing offset DACs All 0  : %d\n", rc);
#endif
		io_errors++;
	}
	rc = ngpd_i2c_read_preamp_offset(path, 0, num_chan, read_buff);
	if (rc < 0)
	{
		mprintf(MP_ERR, "Error on Reading I2C Register at channel all channel: %d\n",  rc);
		io_errors++;
	}
	for (chan=0; chan < num_chan; chan++)
	{
		if (read_buff[chan] != write_buff[0])
		{
			errors++;
			if (errors < max_errors)
				mprintf(MP_ERR, "ERROR: Chan  %d, Wrote %04X, read %04X\n", chan, write_buff[0], read_buff[chan]);
			else if (errors == max_errors)
				mprintf(MP_ERR, "Too Many Errors, counting\n");
		}
	}
	
	if (errors == 0 && io_errors == 0)
		printf("Passed preamp offset DAC  read/write tests\n");
	else
		mprintf(MP_ERR, "Preamp offset DAC read/write tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}


int test_ad7410_regs_preamp(int path, int card)
{
	return test_ad7410_regs(path, card, NGZMP_I2C_BUS_PREAMP, NGZMP_I2C_NUM_ADT7410_PREAMP, "preamp");
}
int test_ad7410_regs_adc(int path, int card)
{
	return test_ad7410_regs(path, card, NGZMP_I2C_BUS_PMBUS, NGZMP_I2C_NUM_ADT7410_ADC, "ADC");
}

int test_ad7410_regs(int path, int card, int bus, int num_chips, char * name)
{
	int rc, i;
	u_int8_t x[2], y[2];
	int errors = 0;		
	int io_errors=0;
	int chip;
	u_int16_t raw;
	u_int8_t id;
	float temp[4];
	int status[4];
	
	for (chip=0; chip<num_chips; chip++)
	{
		if (manual_test) {printf("Press return to read Id register from ADT7410 chip=%d on %s\n", chip, name); getchar();}
		rc = ngzmp_i2c_read_reg_addr(path, card, bus, NGZMP_I2C_ADDR_ADT7410(chip), ADT7410_ID, 1, &id);
		if (rc < 0)
		{
#if defined(__x86_64__)
			mprintf(MP_ERR, "Error on Reading I2C ID Register at ADT7410 %s %d : %d %s\n", name, chip,  rc, ngpd_get_error_message());
#else
			mprintf(MP_ERR, "Error on Reading I2C ID Register at ADT7410 %s %d : %d\n", name, chip,  rc);
#endif
			io_errors++;
		}
		if ((id & 0xC0) == 0xC0)
			printf("ADT7410 ID on %s chip %d = %02X\n", name, chip, id);
		else
		{
			errors++;
			mprintf(MP_ERR, "ADT7410 ID on %s chip %d = %02X, expected 0xCX\n", name, chip, id);
		}
	}
	if (manual_test) {printf("Press return to read temperature from all %d chips on %s\n", num_chips, name); getchar();}
	rc = ngpd_i2c_read_temp(path, card, temp, status, bus, num_chips, name);
	if (rc < 0)
	{
#if defined(__x86_64__)
		mprintf(MP_ERR, "Error on Reading temperature at ADT7410 %s %d : %d %s\n", name, chip,  rc, ngpd_get_error_message());
#else
		mprintf(MP_ERR, "Error on Reading temperature at ADT7410 %s %d : %d\n", name, chip,  rc);
#endif
		io_errors++;
	}
	for (chip=0; chip<num_chips; chip++)
	{
		printf("ADT7410 on %s chip %d : temperature= %f, status = %d\n", name, chip, temp[chip], status[chip]);
	}
	for (i=0; i<256; i++)
	{
		for (chip=0; chip<num_chips; chip++)
		{
			raw = 0x0301*i + 0x0A0A*chip;
			x[0] = raw>>8;
			x[1] = raw&0xFF;
			
			if (manual_test)
			{
				printf("About to write 0x%02X, %02X to ADT7410 %s chip %d Tlow register. Press return to continue\n", x[0], x[1], name, chip);
				getchar();
			}

			rc = ngzmp_i2c_write_reg_addr(path, card, bus, NGZMP_I2C_ADDR_ADT7410(chip), ADT7410_TLOW_MSB, 2, x);
			if (rc < 0)
			{
#if defined(__x86_64__)
				mprintf(MP_ERR, "Error on writing I2C Tlow Register at ADT7410 %s %d : %d : %s\n",  name, chip, rc, ngpd_get_error_message());
#else
				mprintf(MP_ERR, "Error on writing I2C Tlow Register at ADT7410 %s %d : %d \n",  name, chip, rc);
#endif
				io_errors++;
			}
		}
		for (chip=0; chip<num_chips; chip++)
		{
			raw = 0x0301*i + 0x0A0A*chip;
			x[0] = raw>>8;
			x[1] = raw&0xFF;
			if (manual_test)
			{
				printf("About to read from ADR7410, press return to continue\n");
				getchar();
			}
			rc = ngzmp_i2c_read_reg_addr(path, card, bus, NGZMP_I2C_ADDR_ADT7410(chip), ADT7410_TLOW_MSB, 2, y);
			if (rc < 0)
			{
#if defined(__x86_64__)
				mprintf(MP_ERR, "Error on Reading I2C Register at ADT7410 %s %d : %d %s\n", name, chip,  rc, ngpd_get_error_message());
#else
				mprintf(MP_ERR, "Error on Reading I2C Register at ADT7410 %s %d : %d\n", name, chip,  rc);
#endif
				io_errors++;
			}
			if (x[0] != y[0] || x[1] != y[1])
			{
				errors++;
				if (errors < max_errors)
					mprintf(MP_ERR, "ERROR: ADT7410 %s %d Tlow Wrote %02X, %02X, read %02X, %02X\n", name, chip, x[0], x[1], y[0], y[1] );
				else if (errors == max_errors)
					mprintf(MP_ERR, "Too Many Errors, counting\n");
			}
		}
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed ADT7410 on %s chip %d i2C Register read/write tests\n", name, chip);
	else
		mprintf(MP_ERR, "Failed ADT7410 on %s I2C Register read/write tests with %d IO Errors and %d Data Errors\n", name, io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}

/* test MCP23008 port expanded used for PSU status on the ADC board by write/read to 3 registers from GPINTEN */
#define NUM_PEXP_REGS 3
int test_i2c_pexp(int path, int card, int bus, int addr, char *name)
{
	int rc, i, j;
	u_int8_t write_buff[NUM_PEXP_REGS], read_buff[NUM_PEXP_REGS];
	int errors = 0;		
	int io_errors=0;
	
	mprintf(1, "I2C-%s-PEXP: Writing and reading ADC card PSu status port expander\n", name);

//	for (i=0; i<256; i++)
	for (i=0; i<1; i++)
	{
		if (i == 255)
		{
			for (j=0;j<NUM_PEXP_REGS; j++)
				write_buff[j] = 0; // Restore default.
		}
		else
		{
			for (j=0;j<NUM_PEXP_REGS; j++)
				write_buff[j] = i+0x31*j;
		}
		if (manual_test) { printf("Press return to write to bus=%d, slave_addr=%02X, reg_addr=%02X data=(%02X,%02X,%02X)\n", bus, addr, MCP23008_GPINTEN, write_buff[0], write_buff[1], write_buff[2] ); getchar();}
		rc = ngzmp_i2c_write_reg_addr(path, card, bus, addr, MCP23008_GPINTEN, NUM_PEXP_REGS, write_buff);
		if (rc < 0)
		{
#if defined(__x86_64__)
			mprintf(MP_ERR, "Error on writing to MCP23008 %s port expander : %d : %s\n", name, rc, ngpd_get_error_message());
#else
			mprintf(MP_ERR, "Error on writing to MCP23008 %s  port expander  : %d\n", name, rc);
#endif
			io_errors++;
		}
		if (manual_test) { printf("Press return to read form bus=%d, slave_addr=%02X, reg_addr=%02X\n", bus, addr, MCP23008_GPINTEN); getchar();}
		rc = ngzmp_i2c_read_reg_addr(path, card, bus, addr, MCP23008_GPINTEN, NUM_PEXP_REGS, read_buff);
		if (rc < 0)
		{
#if defined(__x86_64__)
			mprintf(MP_ERR, "Error on reading to MCP23008 %s port expander : %d : %s\n", name, rc, ngpd_get_error_message());
#else
			mprintf(MP_ERR, "Error on reading to MCP23008 %s port expander  : %d\n", name, rc);
#endif
			io_errors++;
		}
		for (i=0; i < NUM_PEXP_REGS; i++)
		{
			if (read_buff[i] != write_buff[i])
			{
				errors++;
				if (errors < max_errors)
					mprintf(MP_ERR, "ERROR: %s PEXP Address %d, Wrote %04X, read %04X\n", name, MCP23008_GPINTEN+i, write_buff[i], read_buff[i]);
				else if (errors == max_errors)
					mprintf(MP_ERR, "Too Many Errors, counting\n");
			}
		}
	}	
	if (errors == 0 && io_errors == 0)
		printf("Passed %s port expander read/write tests\n", name);
	else
		mprintf(MP_ERR, "FAILED %s port expander tests with %d IO Errors and %d Data Errors\n", name, io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}

int test_i2c_psu_pexp(int path, int card)
{
	return test_i2c_pexp(path, card, NGZMP_I2C_BUS_PMBUS, NGZMP_I2C_ADDR_PSU_PEXP, "PSU");
}
int test_i2c_sfp_pexp(int path, int card)
{
	return test_i2c_pexp(path, card, NGZMP_I2C_BUS_SFP_PEXP, NGZMP_I2C_ADDR_SFP_PEXP, "SFP");
}

int test_i2c_rd_lmk61e2(int path, int card)
{
	int io_errors=0, errors=0;
	int rc;
	u_int8_t rbuf[4];
	
	mprintf(1, "I2C-LMK61E2: Reads of LMK61E2 fixed registers\n");
	rc = ngzmp_i2c_read_reg_addr(path, card, NGZMP_I2C_BUS_CLK, NGZMP_I2C_ADDR_CLK_LMK61E2, 0, 3, (u_int8_t *)rbuf);
	if (rc < 0)
	{
#if defined(__x86_64__)
		mprintf(MP_ERR, "Error on reading 3 bytes from LMK61E2 : %d : %s\n", rc, ngpd_get_error_message());
#else
		mprintf(MP_ERR, "Error on reading 3 bytes from LMK61E2   : %d\n", rc);
#endif
		io_errors++;
	}
	if (rbuf[0] == 0x10 && rbuf[1] == 0x0B && rbuf[2] == 0x33)
		mprintf(MP_PRINT, "Correct: Read back %02X, %02X, %02X\n", rbuf[0], rbuf[1], rbuf[2]);
	else
	{
		errors++;
		mprintf(MP_ERR, "ERROR reading LMK61e2: Read (%02X, %02X, %02X) Expected (10, 0B, 33)\n", rbuf[0], rbuf[1], rbuf[2]);
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed LMK61E2 tests\n");
	else
		mprintf(MP_ERR, "FAILED LMK61E2 tests with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}
	
int test_i2c_rd_si5324(int path, int card)
{
	int io_errors=0, errors=0;
	int rc;
	u_int8_t rbuf[4];
	
	mprintf(1, "I2C-SI5324: Reads of SI5324 fixed registers\n");
	rc = ngzmp_i2c_read_reg_addr(path, card, NGZMP_I2C_BUS_ESS_CLK, NGZMP_I2C_ADDR_ESSCLK_SI5324, 134, 2, (u_int8_t *)rbuf);
	if (rc < 0)
	{
#if defined(__x86_64__)
		mprintf(MP_ERR, "Error on reading 2 bytes from SI5324 : %d : %s\n", rc, ngpd_get_error_message());
#else
		mprintf(MP_ERR, "Error on reading 2 bytes from SI5324   : %d\n", rc);
#endif
		io_errors++;
	}
	if (rbuf[0] == 0x01 && rbuf[1] == 0x82)
		mprintf(MP_PRINT, "Correct: Read back %02X, %02X\n", rbuf[0], rbuf[1]);
	else
	{
		errors++;
		mprintf(MP_ERR, "ERROR reading SI5324: Read (%02X, %02X) Expected (01, 82)\n", rbuf[0], rbuf[1]);
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed SI5324 tests\n");
	else
		mprintf(MP_ERR, "FAILED SI5324 tests with %d IO Errors and %d Data Errors\n", io_errors, errors);
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}
	
int test_i2c_enclustra(int path, int card)
{
	int io_errors=0;
	int rc;
	u_int8_t rbuf[4];
	int i;
	char *name;
	int addr;
	
	mprintf(1, "I2C-ENCLUSTRA: Reads of Enclustar EEPROM\n");
	for (i=0; i<2; i++)
	{
		name = i?"DS28CN01U":"ATSHA204A";
		addr = i?NGZMP_I2C_ADDR_ENC_DS28CN01U:NGZMP_I2C_ADDR_ENC_ATSHA204A;

		rc = ngzmp_i2c_read_reg_addr(path, card, NGZMP_I2C_BUS_ENCLUSTRA, addr, 0, 2, (u_int8_t *)rbuf);
		if (rc < 0)
		{
	#if defined(__x86_64__)
			mprintf(MP_ERR, "Error on reading 2 bytes from %s : %d : %s\n", name, rc, ngpd_get_error_message());
	#else
			mprintf(MP_ERR, "Error on reading 2 bytes from %s   : %d\n", name, rc);
	#endif
			io_errors++;
		}
		else
		{
			mprintf(MP_PRINT, "Succesfully read 2 bytes from %s : Read back %02X, %02X\n", name, rbuf[0], rbuf[1]);
		}
	}
	return 0;
}
	
int test_i2c_ucd90160(int path, int card)
{
#if defined(__aarch64__)
	int io_errors=0;
	int rc;
	u_int8_t rbuf[4], vout_mode;
	int i, chip, raw, v;
	int8_t exp;
	
	mprintf(1, "I2C-UCD9160: Reads of UCD90160 PSU monitor\n");
	
	for (chip=0; chip<2; chip++)
	{
		for (i=0; i<16; i++)
		{
			rc = ngzmp_i2c_read_reg_addr_page(path, card, NGZMP_I2C_BUS_PMBUS, i, NGZMP_I2C_ADDR_UDC90160+chip, NGZMP_I2C_UDC90160_RW_VOUT_MODE, 1, &vout_mode);
			if (rc < 0)	
			{
				io_errors++;
				mprintf(MP_ERR, "ERROR reading UCD90160\n");
			}
			rc = ngzmp_i2c_read_reg_addr_page(path, card, NGZMP_I2C_BUS_PMBUS, i, NGZMP_I2C_ADDR_UDC90160+chip, NGZMP_I2C_UDC90160_READ_VOUT, 2, rbuf);
			if (rc < 0)	
			{
				io_errors++;
				mprintf(MP_ERR, "ERROR reading UCD90160\n");
			}
			raw = rbuf[1]<<8 | rbuf[0];
			v = raw *1000;
			exp = vout_mode & 0x1F; 
			exp |= ( vout_mode & 0x10) << 1 | ( vout_mode & 0x10) << 2 | ( vout_mode & 0x10) << 3;
			if (exp < 0)
				v >>= (-exp);
			else
				v <<= exp;
			printf("Chip %d, Rail %2d : Mode= 0x%02X, exp=%d, raw=%d, V=%d mV\n", chip, i+1, vout_mode, exp, raw, v);
		}
	}
	if (io_errors > 0)
		mprintf(MP_ERREX, "Found %d io errros reading UCD91060\n", io_errors);
	num_errors += io_errors;
#endif
	return 0;
}

int test_i2c_ucd90160_vout(int path, int card)
{
	int io_errors=0;
	int rc;
	int i, chip;
	int32_t	v[NGZMP_UCD90160_NUM_RAILS];
	
	mprintf(1, "I2C-UCD9160-VOUT: Reads of UCD90160 PSU monitor Vout API\n");
	
	for (chip=0; chip<NGZMP_UCD90160_NUM_CHIPS; chip++)
	{
		rc = ngzmp_i2c_read_ucd90160_vout(path, card, chip, 0, NGZMP_UCD90160_NUM_RAILS, v);
		if (rc < 0)	
		{
			io_errors++;
			mprintf(MP_ERR, "ERROR reading UCD90160\n");
		}
		for (i=0; i<NGZMP_UCD90160_NUM_RAILS; i++)
		{
			
#if defined (__aarch64__)
			printf("Chip %d, Rail %2d : V=%d mV = %g V\n", chip, i+1, v[i]/10, v[i]/10000.0);
#else
			printf("Chip %d, Rail %2d : %18s : V=%d mV = %g V\n", chip, i+1, ngpd_ucd90160_signals[chip][i], v[i]/10, v[i]/10000.0);
#endif
		}
	}
	if (io_errors > 0)
		mprintf(MP_ERREX, "Found %d io errros reading UCD91060\n", io_errors);
	num_errors += io_errors;
	return 0;
}

