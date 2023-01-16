#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>



#include "ngpd.h"
#include "ngzmp_lib_driver.h"
#include "zynqmp_protocol.h"
#include "zynqmp_api.h"
#include "lmk0482x.h"

#define MAX_REGS 160


static NGZMPSPIAddrData ngzmp_clock_regs[MAX_REGS] = {
/* R0	*/ {0x0000, 0x80},
/* R2	*/ {0x0002, 0x00},
/* R256	*/ {0x0100, 0x05},
/* R257	*/ {0x0101, 0x55},
/* R258	*/ {0x0102, 0x55},
/* R259	*/ {0x0103, LMK0482X_DCLKoutX_MUX_DIV_DCC_HS},
/* R260	*/ {0x0104, LMK0482X_DCLKoutY_MUX | LMK0482X_SDCLKoutY_DDLY(10-1)},
/* R261	*/ {0x0105, 0x00},
/* R262	*/ {0x0106, 0xB0},
/* R263	*/ {0x0107, 0x13},
/* R264	*/ {0x0108, 0x05},
/* R265	*/ {0x0109, 0x55},
/* R266	*/ {0x010A, 0x55},
/* R267	*/ {0x010B, LMK0482X_DCLKoutX_MUX_DIV_DCC_HS},
/* R268	*/ {0x010C, LMK0482X_DCLKoutY_MUX | LMK0482X_SDCLKoutY_DDLY(10-1)},
/* R269	*/ {0x010D, 0x00},
/* R270	*/ {0x010E, 0xF0},
/* R271	*/ {0x010F, 0x13},
/* R272	*/ {0x0110, 0x0A},
/* R273	*/ {0x0111, 0x55},
/* R274	*/ {0x0112, 0x55},
/* R275	*/ {0x0113, LMK0482X_DCLKoutX_MUX_DIV_DCC_HS},
/* R276	*/ {0x0114, LMK0482X_DCLKoutY_MUX | LMK0482X_SDCLKoutY_DDLY(10-1)},
/* R277	*/ {0x0115, 0x00},
/* R278	*/ {0x0116, 0xF0},
/* R279	*/ {0x0117, 0x11},
/* R280	*/ {0x0118, 0x0A},
/* R281	*/ {0x0119, 0x55},
/* R282	*/ {0x011A, 0x55},
/* R283	*/ {0x011B, LMK0482X_DCLKoutX_MUX_DIV_DCC_HS},
/* R284	*/ {0x011C, LMK0482X_DCLKoutY_MUX | LMK0482X_SDCLKoutY_DDLY(10-1)},
/* R285	*/ {0x011D, 0x00},
/* R286	*/ {0x011E, 0xF0},
/* R287	*/ {0x011F, 0x11},
/* R288	*/ {0x0120, 0x0A},
/* R289	*/ {0x0121, 0x55},
/* R290	*/ {0x0122, 0x55},
/* R291	*/ {0x0123, LMK0482X_DCLKoutX_MUX_DIV_DCC_HS},
/* R292	*/ {0x0124, LMK0482X_DCLKoutY_MUX | LMK0482X_SDCLKoutY_DDLY(10-1)},
/* R293	*/ {0x0125, 0x00},
/* R294	*/ {0x0126, 0xF1},
/* R295	*/ {0x0127, 0x05},
/* R296	*/ {0x0128, 0x0A},
/* R297	*/ {0x0129, 0x55},
/* R298	*/ {0x012A, 0x55},
/* R299	*/ {0x012B, LMK0482X_DCLKoutX_MUX_DIV_DCC_HS},
/* R300	*/ {0x012C, LMK0482X_DCLKoutY_MUX | LMK0482X_SDCLKoutY_DDLY(10-1)},
/* R301	*/ {0x012D, 0x00},
/* R302	*/ {0x012E, 0xF1},
/* R303	*/ {0x012F, 0x05},
/* R304	*/ {0x0130, 0x05},
/* R305	*/ {0x0131, 0x55},
/* R306	*/ {0x0132, 0x55},
/* R307	*/ {0x0133, LMK0482X_DCLKoutX_MUX_DIV_DCC_HS},  // 1 = >DDC_Divider, usefull for divide 5 for 500 MHz, 0 => divider, OK for Divide 10 for 250 MHz
/* R308	*/ {0x0134, LMK0482X_DCLKoutY_MUX | LMK0482X_SDCLKoutY_DDLY(10-1)},	// LSB controls Half Step of SYSREF
/* R309	*/ {0x0135, 0x00},
/* R310	*/ {0x0136, 0xB0},
/* R311	*/ {0x0137, 0x55},  //0x45 to make SYSREF monitor HSDS, 0x55 make make both LVPECL16
/* R312	*/ {0x0138, 0x00},
/* R313	*/ {0x0139, 0x00},
/* R314	*/ {0x013A, 0x00},
/* R315	*/ {0x013B, 0xA0},
/* R316	*/ {0x013C, 0x00},
/* R317	*/ {0x013D, 0x08}, // SYSREF_DDLY miniumu sysref delay
/* R318	*/ {0x013E, 0x03},
/* R319	*/ {0x013F, 0x0D},
/* R320	*/ {0x0140, 0x00}, // LMK0482X_140_POWERDOWN
/* R321	*/ {0x0141, 0x00},
/* R322	*/ {0x0142, 0x00},
/* R323	*/ {0x0143, LMK0482X_SYNC_ENABLE | LMK0482X_SYNC_SYSREF_CLR | LMK0482X_SYNC_MODE_SYNC_PIN},
/* R324	*/ {0x0144, 0x00}, // Clear Sync disable. LMK0482X_144_SYNC_DISABLE
/* R325	*/ {0x0145, 0x7F},
/* R326	*/ {0x0146, 0x00},
/* R327	*/ {0x0147, 0x1B},
/* R328	*/ {0x0148, 0x02},
/* R329	*/ {0x0149, 0x02},
/* R330	*/ {0x014A, 0x02},
/* R331	*/ {0x014B, 0x16},
/* R332	*/ {0x014C, 0x00},
/* R333	*/ {0x014D, 0x00},
/* R334	*/ {0x014E, 0xC0},
/* R335	*/ {0x014F, 0x7F},
/* R336	*/ {0x0150, 0x03},
/* R337	*/ {0x0151, 0x02},
/* R338	*/ {0x0152, 0x00},
/* R339	*/ {0x0153, 0x00},
/* R340	*/ {0x0154, 0x78},
/* R341	*/ {0x0155, 0x00},
/* R342	*/ {0x0156, 0x01},
/* R343	*/ {0x0157, 0x00},
/* R344	*/ {0x0158, 0x01},
/* R345	*/ {0x0159, 0x00},
/* R346	*/ {0x015A, 0x01},
/* R347	*/ {0x015B, 0xD4},
/* R348	*/ {0x015C, 0x20},
/* R349	*/ {0x015D, 0x00},
/* R350	*/ {0x015E, 0x00},
/* R351	*/ {0x015F, 0x0B},
/* R352	*/ {0x0160, 0x00},
/* R353	*/ {0x0161, 0x02},
/* R354	*/ {0x0162, 0x24},
/* R355	*/ {0x0163, 0x00},
/* R356	*/ {0x0164, 0x00},
/* R357	*/ {0x0165, 0x10},
/* R369	*/ {0x0171, 0xAA},
/* R370	*/ {0x0172, 0x02},
/* R380	*/ {0x017C, 0x15},
/* R381	*/ {0x017D, 0x33},
/* R358	*/ {0x0166, 0x00},
/* R359	*/ {0x0167, 0x00},
/* R360	*/ {0x0168, 0x10},
/* R361	*/ {0x0169, 0x51},
/* R362	*/ {0x016A, 0x20},
/* R363	*/ {0x016B, 0x00},
/* R364	*/ {0x016C, 0x00},
/* R365	*/ {0x016D, 0x00},
/* R366	*/ {0x016E, 0x13},
/* R371	*/ {0x0173, 0x00},
/* R386	*/ {0x0182, 0x00},
/* R387	*/ {0x0183, 0x00},
/* R388	*/ {0x0184, 0x00},
/* R389	*/ {0x0185, 0x00},
/* R392	*/ {0x0188, 0x00},
/* R393	*/ {0x0189, 0x00},
/* R394	*/ {0x018A, 0x00},
/* R395	*/ {0x018B, 0x00},
/* R8189	{0x1FFD, 0x00}, */
/* R8190	{0x1FFE, 0x00}, */
/* R8191	0x1FFF, 0x53}, */
/* Terminator*/ {-1, 0}
};

/**
 * @ingroup debug
 * Configure a the LMK04828 from file generated using TICS Pro clock design tool.
 *
 * @param path 		A handle to the top level of the NGPD system returned from {@link ngpd_config_ngzmp()}.
 * @param card		Card is the number of the card in the NGPD system,
 *        			0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *      	  		where this value has been passed to {@link ngpd_config_ngzmp()} at NGPD system configuration.
 *					If card <0 then the data is copied to all cards.
 * @param fname		File name for the values to write to the device.
 * @return 			Return 0 on success or negative error message
 */
int ngpd_spi_write_lmk04828_from_file(int path, int card, char *fname)
{
	FILE *fp;
	NGZMPSPIAddrData reg[MAX_REGS];
	int i, j, l, num;
	char line_buf[102];


	if ((fp=fopen(fname, "r")) == NULL)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_spi_write_lmk04828_from_file: Cannot open file %s, errno=%d", fname, errno);
		return -1;
	}

	num=0;
	while (fgets(line_buf, 100, fp) != NULL)
	{
		if (sscanf(line_buf,"R%d %i", &i, &j) != 2)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_spi_write_lmk04828_from_file: Unexpected format at line %d,  '%s'", num+1, line_buf);
			fclose(fp);
			return -1;
		}
		l = strlen(line_buf);
		if (l>0 && line_buf[l-1] == '\n')
			line_buf[l-1] = 0;
		if (l>1 && line_buf[l-2] == '\r')
			line_buf[l-2] = 0;
		reg[num].addr = j >> 8;
		reg[num].data = j & 0xFF;
		printf("'%s' => len=%d, Addr %03X, Data %02X\n", line_buf, l, reg[num].addr, reg[num].data);
		if (i != reg[num].addr)
			printf("Mismatch  R%d != %d\n", i, reg[num].addr);
		num++;
		if (num == MAX_REGS)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_spi_write_lmk04828_from_file: Too many lines of data, maximum %d", MAX_REGS);
			fclose(fp);
			return -1;
		}

	}
	fclose(fp);

	return ngpd_spi_write_lmk04828(path, card, num, reg);
}
/**
 * @ingroup debug
 * Write registers of the LMK04828 clock fanout chip on a NGPD ZynqMP version.
 *
 * @param path 		A handle to the top level of the NGPD system returned from {@link ngpd_config_ngzmp()}.
 * @param card		Card is the number of the card in the NGPD system,
 *        			0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *      	  		where this value has been passed to {@link ngpd_config_ngzmp()} at NGPD system configuration.
 *					If card <0 then the data is copied to all cards.
 * @param num		Number of address/value pair to write.
 * @param reg		Pointer to array of address/value pair to write.
 * @return 			Returns 0 on success or negative error message
 */
int ngpd_spi_write_lmk04828(int path, int card, int num, NGZMPSPIAddrData *reg)
{
	int i;
	int rc;

	for (i=0; i<num; i++)
	{
		rc = ngzmp_spi_write_clk(path, card, reg[i].addr, 1, &(reg[i].data));
		if (rc < 0)
		{
			char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
			strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
			old_msg[NGPD_MAX_ERROR_MESSAGE] = 0;
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_spi_write_lmk04828: %s", old_msg);
			return rc;
		}
	}
	return 0;
}

int ngpd_clock_setup_lmk04828(int path, int card, NGPDClockSetup *cset)
{
	int i;
	NGZMPSPIAddrData clock_regs[MAX_REGS];
		
	for (i=0; i< MAX_REGS; i++)
	{
		if (ngzmp_clock_regs[i].addr == -1)
			break;
	}
	if (i+8 >= MAX_REGS)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_clock_setup_lmk04828: ngzmp_clock_regs is too long for MAX_REGS=%d, please increase MAX_REGS", MAX_REGS);
		return -1;
	}
	for (i=0; ngzmp_clock_regs[i].addr != -1; i++)
	{
		clock_regs[i].addr = ngzmp_clock_regs[i].addr;
		if (ngzmp_clock_regs[i].addr == LMK0482X_147_CLKin_CONT)
		{
			clock_regs[i].addr = LMK0482X_147_CLKin_CONT;
			if (cset->src == NGZMPClkSrcBkPlane)
				clock_regs[i].data = LMK0482X_CLKin_SEL_MODE(2) | LMK0482X_CLKin1_OUT_MUX(2) | LMK0482X_CLKin0_OUT_MUX(3);
			else
				clock_regs[i].data = LMK0482X_CLKin_SEL_MODE(1) | LMK0482X_CLKin1_OUT_MUX(2) | LMK0482X_CLKin0_OUT_MUX(3);
		}
		else if (ngzmp_clock_regs[i].addr == LMK0482X_130_DCLKout12_DIV)
		{
			
			if (cset->monitor_op & NGZMPClockMonClk250)
				clock_regs[i].data = 10;  // 2500/10 = 250 MHz
			else
				clock_regs[i].data = 5;		// Divide 2500/5 = 500 MHz
		}	
		else
			clock_regs[i].data = ngzmp_clock_regs[i].data;
	}
	clock_regs[i].addr = LMK0482X_143_SYNC_MODE;
	clock_regs[i++].data = LMK0482X_SYNC_SYSREF_CLR | LMK0482X_SYNC_ENABLE | LMK0482X_SYNC_POL | LMK0482X_SYNC_MODE_SYNC_PIN;
	clock_regs[i].addr = LMK0482X_143_SYNC_MODE;
	clock_regs[i++].data = LMK0482X_SYNC_SYSREF_CLR | LMK0482X_SYNC_ENABLE | LMK0482X_SYNC_MODE_SYNC_PIN;
	clock_regs[i].addr = LMK0482X_144_SYNC_DISABLE;
	clock_regs[i++].data = 0xFF;
	clock_regs[i].addr = LMK0482X_143_SYNC_MODE;
	clock_regs[i++].data = LMK0482X_SYNC_ENABLE;

	if (cset->sysref == NGZMPSysRefContinuous) 
	{
		clock_regs[i].addr = LMK0482X_139_SYSREF_MUX;
		clock_regs[i].data = LMK0482X_SYSREF_MUX_CONTINUOUS;
		printf("Setting Continuous SYSREF addr=%d=0x%03X, data=%02X\n", clock_regs[i].addr, clock_regs[i].addr, clock_regs[i].data);
		i++;
//		clock_regs[i].addr = LMK0482X_143_SYNC_MODE;

	}
	else if (cset->sysref == NGZMPSysRefBurstSPI)
	{
		clock_regs[i].addr = LMK0482X_139_SYSREF_MUX;
		clock_regs[i].data = LMK0482X_SYSREF_MUX_PULSER;
		printf("Setting Burst SYSREF addr=%d=0x%03X, data=%02X\n", clock_regs[i].addr, clock_regs[i].addr, clock_regs[i].data);
		i++;
		clock_regs[i].addr = LMK0482X_143_SYNC_MODE;
		clock_regs[i].data = LMK0482X_SYNC_ENABLE | LMK0482X_SYNC_MODE_PULSER_SPI;
		printf("Setting Burst SYSREF SYNBC MODE = addr=%d=0x%03X, data=%02X\n", clock_regs[i].addr, clock_regs[i].addr, clock_regs[i].data);
		i++;
	}
	else
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_clock_setup_lmk04828: Unknown sysref mode %d", cset->sysref);
		return -1;
	}
	clock_regs[i].addr = -1;
	clock_regs[i].data = 0;
	
	return ngpd_spi_write_lmk04828(path, card, i, clock_regs);
}

int ngpd_clock_wait_locked(int path, int card)
{
	u_int8_t status[2];
	int i;
	int all_locked=1;
	int nchar=0;
	int rc;
	
	for (i=0;i<100; i++)
	{
		all_locked = 1;
		rc = ngzmp_spi_read_clk(path, card, LMK0482X_182_PLL1_STATUS, 2, status);
		if (rc < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_clock_setup: reading LMK04828 status ");
			return rc;
		}
		if (!(status[0] & LMK0482X_RB_PLLX_LD) || !(status[1] & LMK0482X_RB_PLLX_LD))
		{
			all_locked = 0;
			if (i == 99)
			{
				if (nchar == 0)
					nchar =+ snprintf(ngpd_error_message+nchar, NGPD_MAX_ERROR_MESSAGE-nchar, "ngpd_clock_wait_locked: Not locked: (Card, PLL1_LD, PLL2_LD)=(%d, %d, %d)", 
										card, !!(status[0] & LMK0482X_RB_PLLX_LD), !!(status[1] & LMK0482X_RB_PLLX_LD) );
				else
					nchar =+ snprintf(ngpd_error_message+nchar, NGPD_MAX_ERROR_MESSAGE-nchar, ", (%d, %d, %d)", card, !!(status[0] & LMK0482X_RB_PLLX_LD), !!(status[1] & LMK0482X_RB_PLLX_LD) );
			}
		}
		if (all_locked)
			break;
	}
	if (!all_locked)
		return -1;
	
	status[0] = LMK0482X_CLR_PLLX_LD_LOST;
	status[1] = LMK0482X_CLR_PLLX_LD_LOST;
	
	rc = ngzmp_spi_write_clk(path, card, LMK0482X_182_PLL1_STATUS, 2, status);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_clock_setup: Writing LMK04828 staus to clear LOST ");
		return rc;
	}
	return 0;
}
/* Read status from LMK04828 to check that clock isstill locked and that lock was not lost since last poll */
int ngpd_clock_check_locked(int path, int card)
{
	u_int8_t status[2];
	int nchar=0;
	int all_locked = 1;
	int rc;
	

	rc = ngzmp_spi_read_clk(path, card, LMK0482X_182_PLL1_STATUS, 2, status);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_clock_setup: reading LMK04828 status");
		return rc;
	}
	if (!(status[0] & LMK0482X_RB_PLLX_LD) || !(status[1] & LMK0482X_RB_PLLX_LD) || (status[0] & LMK0482X_RB_PLLX_LD_LOST) || (status[1] & LMK0482X_RB_PLLX_LD_LOST) )
	{
		all_locked = 0;
		if (nchar == 0)
			nchar =+ snprintf(ngpd_error_message+nchar, NGPD_MAX_ERROR_MESSAGE-nchar, "ngpd_clock_check_locked: Not locked: (Card, PLL1_LD, PLL2_LD, PLL1_LD_LOST, PLL2_LD_LOST)=(%d, %d, %d, %d, %d)", 
								card, !!(status[0] & LMK0482X_RB_PLLX_LD), !!(status[1] & LMK0482X_RB_PLLX_LD), !!(status[0] & LMK0482X_RB_PLLX_LD_LOST), !!(status[1] & LMK0482X_RB_PLLX_LD_LOST) );
		else
			nchar =+ snprintf(ngpd_error_message+nchar, NGPD_MAX_ERROR_MESSAGE-nchar, ", (%d, %d, %d, %d, %d)", card,
						!!(status[0] & LMK0482X_RB_PLLX_LD), !!(status[1] & LMK0482X_RB_PLLX_LD), !!(status[0] & LMK0482X_RB_PLLX_LD_LOST), !!(status[1] & LMK0482X_RB_PLLX_LD_LOST) );
		status[0] = LMK0482X_CLR_PLLX_LD_LOST;
		status[1] = LMK0482X_CLR_PLLX_LD_LOST;
		rc = ngzmp_spi_write_clk(path, card, LMK0482X_182_PLL1_STATUS, 2, status);
		if (rc < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_clock_check_locked: Writing LMK04828 staus to clear LOST ");
			return rc;
		}
	}
	if (all_locked)
		return 0;
	else
		return -1;
}

int ngpd_clock_setup(int path, int card, NGPDClockSetup *cset)
{
	int rc;
	u_int32_t clock_select;
	
	clock_select = 0;		// Force back to internal FPGA clock
	if (cset->src == NGZMPClkSrcTestClk)
		clock_select = NGZMP_CLK_SELECT_TEST_CLK;
	/* Switch FPGA to internal clock and setup input to LMK04828 */
	rc = ngpd_write_glob_regs(path, card, NGZMP_CLK_SELECT, 1, &clock_select);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_clock_setup: Writing clock control register");
		return rc;
	}

	if (cset->src == NGZMPClkSrcFPGA)
		return 0;
	/* Program LMK04828, which can use LMK61e2, Test Clock or BkPlane Clock */
	if ((rc=ngpd_clock_setup_lmk04828(path, card, cset)) < 0)
		return rc;
	
	clock_select |= NGZMP_CLK_SELECT_LMK04828;
	rc = ngpd_write_glob_regs(path, card, NGZMP_CLK_SELECT, 1, &clock_select);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_clock_setup: Writing clock control register");
		return rc;
	}
	if (cset->src == NGZMPClkSrcFPGA)
		return 0;
	
	return ngpd_clock_wait_locked(path, card);
}

