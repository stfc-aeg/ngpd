/*
 *	NGPD (ZynqMP version) clock setup.
 *	By William Helsby
 *----------------------------------------------------------------------
 */

#include "header.h"
#include <ngpd.h>

int ngpd_lmk61e2_write_from_file(int quals, int cn, int path, int card, char *fname);
int ngpd_lmk61e2_write(int quals, int cn, int path, int card, int addr, int value);
int ngpd_lmk61e2_list(int quals, int cn, int path, int card, int addr, int num);
int ngpd_lmk04828_write_from_file(int quals, int cn, int path, int card, char *fname);
int ngpd_clock_setup_cmd(int quals, int cn, int path, int card);


PARSE_TREE ngpd_clock_tree[] = {
	PT_COMMAND ("lmk61e2-from-file", "%d %d %s save = %d",
			    "Write LMK61E2 settings from file written by TI tool",
			    "Path/Card (-1 for all)/File name/Save to EEPROM",
			    ngpd_lmk61e2_write_from_file),
	PT_COMMAND ("write-reg", "%d %d %d %d = %d",
			    "Write LMK61E2 register from file",
			    "Path/Card (-1 1for all)/Register Address/Value",
			    ngpd_lmk61e2_write),
	PT_COMMAND ("list-regs", "%d %d %d %d = %d",
			    "List values from LMK61E2 registers",
			    "Path/Card/First register address/Number of Registers/",
			    ngpd_lmk61e2_list),
	PT_COMMAND ("lmk04828-from-file", "%d %d %s = %d",
			    "Write LMK61E2 settings from file written by TI tool",
			    "Path/Card (-1 for all)/File name",
			    ngpd_lmk04828_write_from_file),
	PT_COMMAND ("setup", "%d %d fpga lmk61e2 test-clk bkplane sysref-cont monitor-250 = %d", 
				"Setup system clock source and generator",
				"Path/Card (-1 to apply to all cards/Us internal FPGA clock (default)/Use LMK61E2 on ADC board (normal for single board)/Use test clk SMC input/Use backplane clock/Force continuous SYSREF/"
				"Monitor 250 MHZ clcok rather than 500 MHz",
				ngpd_clock_setup_cmd ),
	PT_END
};

int ngpd_lmk61e2_write_from_file(int quals, int cn, int path, int card, char *fname)
{
	int rc;
	int to_eeprom = !!(quals & 1);
	if ((rc=ngpd_i2c_write_lmk61e2_from_file(path, card, fname)) < 0)
	{
		sv_printf(cn, "ERROR: Setting LMK61E2 from file: %s\n", ngpd_get_error_message());
		return rc;
	}
	if (to_eeprom)
	{
		if ((rc=ngpd_i2c_save_lmk61e2_to_eeprom(path, card)) < 0)
		{
			sv_printf(cn, "ERROR: Saving LMK61E2 to EEPROM file: %s\n", ngpd_get_error_message());
			return rc;
		}
	}
	return 0;
}

int ngpd_lmk61e2_write(int quals, int cn, int path, int card, int addr, int value)
{
	NGZMPI2CAddrData reg;
	int rc;

	reg.addr = addr;
	reg.data = value;

	if ((rc=ngpd_i2c_write_lmk61e2(path, card, 1, &reg)) < 0)
	{
		sv_printf(cn, "ERROR: Writing LMK61E2 register: %s\n", ngpd_get_error_message());
		return rc;
	}
	return 0;
}

int ngpd_lmk61e2_list(int quals, int cn, int path, int card, int addr, int num)
{
	int rc;
	u_int8_t data[num];
	int i;

	if ((rc=ngpd_i2c_read_lmk61e2(path, card, addr, num, data)) < 0)
	{
		sv_printf(cn, "ERROR: Reading LMK61E2 register: %s\n", ngpd_get_error_message());
		return rc;
	}
	for (i=0; i<num; i++)
		sv_printf(cn, "# Reg %d=0x%02X : 0x%02X\n", addr+i, addr+i, data[i]);
	return 0;
}

int ngpd_lmk04828_write_from_file(int quals, int cn, int path, int card, char *fname)
{
	int rc;
	
	if ((rc=ngpd_spi_write_lmk04828_from_file(path, card, fname)) < 0)
	{
		sv_printf(cn, "ERROR: Setting LMK04828 from file: %s\n", ngpd_get_error_message());
		return rc;
	}
	return 0;
}

int ngpd_clock_setup_cmd(int quals, int cn, int path, int card)
{
	NGPDClockSetup cset;
	int rc;
	
	switch (quals & 0xF)
	{
		case 0:
		case 1:	cset.src = NGZMPClkSrcFPGA; break;
		case 2: cset.src = NGZMPClkSrcLMK61E2; break;
		case 4: cset.src = NGZMPClkSrcTestClk; break;
		case 8: cset.src = NGZMPClkSrcBkPlane; break;
		default:
			sv_printf(cn, "! ERROR: Please specify at most 1 clock src qualifier\n");
			return -1;
	}
	cset.sysref = NGZMPSysRefBurstSPI;
	if (quals & 0x10)
		cset.sysref = NGZMPSysRefContinuous;
	cset.monitor_op = NGZMPClockMonDefault;
	if (quals & 0x20)
		cset.monitor_op = NGZMPClockMonClk250;

	if ((rc=ngpd_clock_setup(path, card, &cset)) < 0)
	{
		sv_printf(cn, "! ERROR setting clocks : %s\n", ngpd_get_error_message());
	}
	return rc;
}

