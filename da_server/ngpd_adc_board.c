/*----------------------------------------------------------------------
 *	NGPD Device for DA with multi-client server.
 * Functions or programmable gain and offset on ADC board.
 *	By William Helsby, Geoff Mant
 *----------------------------------------------------------------------
 */

#include "header.h"
#include <ngpd.h>


double ngpd_set_range_cmd(int quals, int cn, int path, int first, int num, double range);
int ngpd_set_offset_cmd(int quals, int cn, int path, int first, int num, int offset);
int ngzmp_adc_setup_cmd(int quals, int cn, int path, int card);
int ngzmp_adc_setup_test_cmd(int quals, int cn, int path, int card, int user_pat0, int user_pat1, int user_pat2, int user_pat3 );
int ngpd_list_jesd_regs(int quals, int cn, int path, int card);
int ngpd_jesd_reset_cmd(int quals, int cn, int path, int card, int link );
int ngpd_jesd_sysref_cmd(int quals, int cn, int path, int card, int link, int always, int delay, int required_resync);
int ngpd_adc_setup_output_cmd(int quals, int cn, int path, int card, int vout, int preemphasis);
int ngpd_adc_setup_sync_cmd(int quals, int cn, int path, int card, int mode, int invert);
int ngpd_adc_readback_regs_cmd(int quals, int cn, int path, int card);
int ngpd_adc_setup_jesd_test_cmd(int quals, int cn, int path, int card, int user_pat0, int user_pat1, int user_pat2, int user_pat3 );
int ngzmp_set_offset_cmd(int quals, int cn, int path, int first, int num, int offset);
int ngzmp_set_dga_cmd(int quals, int cn, int path, int first, int num, int gain);
int ngzmp_list_dga_offset(int quals, int cn, int path, int first, int num);
void * ngzmp_get_dga_cmd(int quals, int cn, int path, int chan, int num);
void *ngzmp_get_offset_cmd(int quals, int cn, int path, int chan, int num);
int ngzmp_write_offset_cmd(int quals, int cn, int path, int first, int num);

PARSE_TREE ngpd_adc_board_tree[] = {
	PT_COMMAND ("set-input-range", "%d %d %d %g = %g",
			    "Set ADC board gain control switches to achive specified range",
			    "Path/First Channel/Number of channels/Required total input range (mili-Volts 100 mV-10000 mV)",
			    ngpd_set_range_cmd),
	PT_COMMAND ("set-offset", "%d %d %d %d = %d",
			    "Set values of ADC board offset DACs",
			    "Path/First Channel/Number of channels (-1 for all)/Target Signed ADC Value for 0.0 V input (-32768 to +32767)",
			    ngpd_set_offset_cmd),
	PT_COMMAND ("ngzmp-setup-adc", "%d %d = %d",
				"Setup JESD204 link on ADC",
				"Path/Card",
				ngzmp_adc_setup_cmd),
	PT_COMMAND ("setup-test", "%d %d + %d %d %d %d off midscale pos-full neg-full checkerboard prbs-long prbs-short toggle user ramp chan %d = %d",
				"Setup ADC test pattern output",
				"Path/card/User Patern 0 (for user mode)/User Pattern 1/User pattern2/User Pattern3/"
				"Test pattern off, ADC data (default)/Mid scale 0x1FFF/Positive full (0x3FFF)/Negative full scale (0)/Checker board 01010101010101/"
				"Long PRBS X^23+X^18+1/Short PRBS X^9+X^5+1/Toggle all 1's All 0s/User code (see User0...3)/Ramp/Chan number (0..7 default all channels)",
				ngzmp_adc_setup_test_cmd ),
	PT_COMMAND ("list-jesd-regs", "%d %d = %d",
				"List JESD registers",
				"Path/card",
				ngpd_list_jesd_regs ),
	PT_COMMAND ("jesd-reset", "%d %d %d = %d",
				"Reset JESD link(s)",
				"Path/Card (-1 for all)/Link (-1 for all)",
				ngpd_jesd_reset_cmd ),
	PT_COMMAND ("jesd-sysref", "%d %d %d + %d %d %d =%d",
				"Setup SYSREF handling of JESD receiver",
				"Path/Card (-1 for all)/Link (-1 for all)/Always re-align LMFC on SYSREF/Delay from SYSREF to LMFC/Require SYSREF to re-align link",
				ngpd_jesd_sysref_cmd ),
	PT_COMMAND ("set-jesd-outputs", "%d %d %d %d adc %d pair-sel %d = %d",
			"Setup the output drive of  the JESD outputs",
			"Path/Card (-1 for all)/Vout 0=> 1.0 * DRVDD, 1 => 0.85 * DRVDD/preamphasis 0...4/"
			"Select single ADC 0 or 1 (default -1 => both)/Bitmask to select which pairs are selected (default 3 selects both)/",
			ngpd_adc_setup_output_cmd ),
	PT_COMMAND ("set-adc-sync", "%d %d %d %d adc %d pair-sel %d = %d",
			"Setup the SYNCIN behaviour ADC",
			"Path/Card (-1 for all)/Mode (0, 2 or 3 see AD9694 reg 0x0572)/invert sync (0 or 1)/"
			"Select single ADC 0 or 1 (default -1 => both)/Bitmask to select which pairs are selected (default 3 selects both)/",
			ngpd_adc_setup_sync_cmd ),
	PT_COMMAND ("readback-adc", "%d %d = %d",
				"readback all ADC SPI registers and compared to expecetd values",
				"Path/Card (-1 for all)/",
				ngpd_adc_readback_regs_cmd ),
	PT_COMMAND("setup-jesd-test", "%d %d + %d %d %d %d off checkerboard toggle prbs31 prbs23 prbs15 prbs9 prbs7 ramp cont single phy input scrambler adc %d pair-sel %d = %d",
			"Setup test mode in JESD transport layer block",
			"Path/Card/User test pattern 0/User test pattern 1/User test pattern 2/User test pattern 3/"
			"Disable Test mode/Checkerboard 0x5555 0xAAAA/Toggle 0xFFFF 0000/PRBS x^31+x^28+1/PRBS x^23+x^18+1/PRBS x^15+x^14+1/PRBS x^9+x^5+1/PRBS x^7+x^6+1/Ramp to 65535/Repeat use pattern/Single user pattern",
			ngpd_adc_setup_jesd_test_cmd ),
	PT_COMMAND ("ngzmp-set-offset", "%d %d %d %d = %d",
				"Set offset DACs directly in NGZMP version",
				"Path/First channel/Number of channels/Offset value 0..65535",
				ngzmp_set_offset_cmd),
	PT_COMMAND ("ngzmp-write-offset", "%d %d %d " UNIFIED_WRITE_QUALIFIERS " = %d",
				"Write offset DACs from file in NGZMP version",
				"Path/First channel/Number of channels/" UNIFIED_WRITE_QUALIFIER_HELP,
				ngzmp_write_offset_cmd),
	PT_COMMAND ("ngzmp-set-dga", "%d %d %d %d = %d",
				"Set Digitally contrlled gain amplifier gain directly in NGZMP version",
				"Path/First channel/Number of channels/Gain setting 0..35",
				ngzmp_set_dga_cmd),
	PT_COMMAND ("ngzmp-list-dga", "%d %d %d = %d",
				"List DGA and offset values",
				"Path/First Channel/Number of channels (-1 for all)",
				ngzmp_list_dga_offset ),
	PT_COMMAND ("ngzmp-get-offset", "%d %d %d = [%n%d]",
				"Get offset DACs directly in NGZMP version for specifed channel",
				"Path/First Channel/Number of Channels",
				ngzmp_get_offset_cmd),
	PT_COMMAND ("ngzmp-get-dga", "%d %d %d = [%n%d]",
				"Get Digitally contrlled gain amplifier gain directly in NGZMP version",
				"Path/First Channel/Number of channels",
				ngzmp_get_dga_cmd),
	PT_END
};


double ngpd_set_range_cmd(int quals, int cn, int path, int first, int num, double range)
{
	int chan, num_chan = ngpd_get_num_chan(path);
	double actual;
	if (num_chan< 0)
	{
		sv_printf(cn, "! ERROR: Cannot read number of channels : %s\n", ngpd_get_error_message());
		return -1.0;
	}
	if (first < 0 || first >= num_chan)
	{
		sv_printf(cn, "! ERROR: Channel %d for %d does not fit in %d channel system\n", first, num, num_chan);
		return -1.0;
	}
	if (num < 0)
		num = num_chan-first;
	else if (num+first > num_chan)
	{
		sv_printf(cn, "! ERROR: Channel %d for %d does not fit in %d channel system\n", first, num, num_chan);
		return -1;
	}
	for (chan=first; chan<first+num; chan++)
	{
		if ((actual=ngpd_adc_set_range(path, chan, range)) < 0)
		{
			sv_printf(cn, "! ERROR: Error setting range: %s\n", ngpd_get_error_message());
			return -1;
		}
		else
			sv_printf(cn, "# Note: Chan %d Requested %g, actual range %g\n", chan, range, actual);
	}

	return actual;
}

int ngpd_set_offset_cmd(int quals, int cn, int path, int first, int num, int offset)
{
	int chan, num_chan = ngpd_get_num_chan(path);

	if (num_chan< 0)
	{
		sv_printf(cn, "! ERROR: Cannot read number of channels : %s\n", ngpd_get_error_message());
		return -1;
	}
	if (first < 0 || first >= num_chan)
	{
		sv_printf(cn, "! ERROR: Channel %d for %d does not fit in %d channel system\n", first, num, num_chan);
		return -1;
	}
	if (num < 0)
		num = num_chan-first;
	else if (num+first > num_chan)
	{
		sv_printf(cn, "! ERROR: Channel %d for %d does not fit in %d channel system\n", first, num, num_chan);
		return -1;
	}
	for (chan=first; chan<first+num; chan++)
	{
		if (ngpd_adc_set_offset(path, chan, offset) < 0)
		{
			sv_printf(cn, "! ERROR: Error setting offset: %s\n", ngpd_get_error_message());
			return -1;
		}
	}
	return 0;
}

int ngzmp_adc_setup_cmd(int quals, int cn, int path, int card)
{
	int rc;
	NGZMPADCFlags flags=NGZMPADC_Default;
	
	rc = ngzmp_adc_setup_adc(path, card, flags);
	if (rc < 0)
	{
		sv_printf(cn, "! ERROR: Error setting ADC: %s\n", ngpd_get_error_message());
		return rc;
	}
	
	rc = ngzmp_adc_read_status(path, card);
	if (rc < 0)
	{
		sv_printf(cn, "! ERROR: Error setting ADC: %s\n", ngpd_get_error_message());
		return rc;
	}
	return 0;
}

int ngzmp_adc_setup_test_cmd(int quals, int cn, int path, int card, int user_pat0, int user_pat1, int user_pat2, int user_pat3 )
{
	u_int16_t user_pat[4] ;
	int test_mode = AD9694_TEST_MODE_OFF;
	int chan;
	int adc=-1;
	int pair_sel=3;
	int chan_sel=3;
	int rc;
	
	switch ( quals & 0x3FF)
	{
	case     0:
	case     1: test_mode = AD9694_TEST_MODE_OFF; break;
	case     2: test_mode = AD9694_TEST_MODE_MIDSCALE; break;
	case     4: test_mode = AD9694_TEST_MODE_POS_FULL; break;
	case     8: test_mode = AD9694_TEST_MODE_NEG_FULL; break;
	case 0x010: test_mode = AD9694_TEST_MODE_CHECKERBOARD; break;
	case 0x020: test_mode = AD9694_TEST_MODE_PRBS_LONG; break;
	case 0x040: test_mode = AD9694_TEST_MODE_PRBS_SHORT; break;
	case 0x080: test_mode = AD9694_TEST_MODE_TOGGLE; break;
	case 0x100: test_mode = AD9694_TEST_MODE_USER; break;
	case 0x200: test_mode = AD9694_TEST_MODE_RAMP; break;
	default:
		sv_printf(cn, "! ERROR: Please specify at most one test mode (or none to turn off)\n");
		return -1;
	}
	if (quals & 0x400)
	{
		chan = Qualifier[10].IntArg;
		adc = (chan/4) % 2;
		chan_sel = 1 << (chan & 1);
		chan = (chan >> 1) & 1;
		pair_sel = 1 << chan;
	}
	user_pat[0] = user_pat0;
	user_pat[1] = user_pat1;
	user_pat[2] = user_pat2;
	user_pat[3] = user_pat3;
	
	rc = ngzmp_adc_setup_test(path, card, adc, pair_sel, chan_sel, test_mode, user_pat);
	if (rc < 0)
	{
		sv_printf(cn, "! ERROR: Error setting test mode: %s\n", ngpd_get_error_message());
		return rc;
	}
	return 0;
}
#if JESD204C
int ngpd_list_jesd_regs(int quals, int cn, int path, int card)
{
	int first_card, last_card, num_cards;
	u_int32_t ip[2], reset, cfg_8b10b, rx_status[3];
	int link;
	int rc;
	num_cards = ngpd_get_num_cards(path);
	if (num_cards < 0)
	{
		sv_printf(cn, "! ERROR getting number of cards : %s\n", ngpd_get_error_message());
		return num_cards;
	}
	
	if (card < 0)
	{
		first_card = 0;
		last_card = num_cards -1;
	}
	else if (card >= num_cards)
	{
		sv_printf(cn, "! ERROR: Card %d is out of range 0...%d\n", card, num_cards);
		return -1;
	}
	else
	{
		first_card = last_card = card;
	}
	
	for (card=first_card; card <= last_card; card++)
	{
		for (link=0; link<4; link++)
		{
			if ((rc=ngpd_read_jesd_regs(path, card, link, JESD_IP_VERSION, 2, ip)) < 0)
			{
				sv_printf(cn, "! ERROR reading JESD_IP_VERSION=0x%03X : %s\n", JESD_IP_VERSION, ngpd_get_error_message());
				return -1;
			}
			if ((rc=ngpd_read_jesd_regs(path, card, link, JESD_RESET, 1, &reset)) < 0)
			{
				sv_printf(cn, "! ERROR reading JESD_RESET=0x%03X: %s\n", JESD_RESET, ngpd_get_error_message());
				return -1;
			}
			
			if ((rc=ngpd_read_jesd_regs(path, card, link, JESD_CTRL_8B10B_CFG, 1, &cfg_8b10b)) < 0)
			{
				sv_printf(cn, "! ERROR reading JESD_CTRL_8B10B_CFG=%08X: %s\n", JESD_CTRL_8B10B_CFG, ngpd_get_error_message());
				return -1;
			}
			
			if ((rc=ngpd_read_jesd_regs(path, card, link, JESD_STAT_RX_ERR, 3, rx_status)) < 0)
			{
				sv_printf(cn, "! ERROR reading JESD_STAT_RX_ERR=0x%03X: %s\n", JESD_STAT_RX_ERR, ngpd_get_error_message());
				return -1;
			}
			sv_printf(cn, "# : Card %d : link %d : IP_VERSION = %08X, IP_CONFIG=%08X, RESET=%08X, CFG_8B10B=%08X, RX_ERR = %08X, RS_DEBUG=%08X, RX_STATUS=%08X\n", card, link, ip[0], ip[1], reset, cfg_8b10b, rx_status[0], rx_status[1], rx_status[2]);
		}		
	}
	return 0;
}
#else
int ngpd_list_jesd_regs(int quals, int cn, int path, int card)
{
	int first_card, last_card, num_cards;
	u_int32_t ip[1], reset, cfg[4], rx_err_status, rx_status[2];
	int link;
	int rc;
	num_cards = ngpd_get_num_cards(path);
	if (num_cards < 0)
	{
		sv_printf(cn, "! ERROR getting number of cards : %s\n", ngpd_get_error_message());
		return num_cards;
	}
	
	if (card < 0)
	{
		first_card = 0;
		last_card = num_cards -1;
	}
	else if (card >= num_cards)
	{
		sv_printf(cn, "! ERROR: Card %d is out of range 0...%d\n", card, num_cards);
		return -1;
	}
	else
	{
		first_card = last_card = card;
	}
	
	for (card=first_card; card <= last_card; card++)
	{
		for (link=0; link<4; link++)
		{
			if ((rc=ngpd_read_jesd_regs(path, card, link, JESD_IP_VERSION, 1, ip)) < 0)
			{
				sv_printf(cn, "! ERROR reading JESD_IP_VERSION=0x%03X : %s\n", JESD_IP_VERSION, ngpd_get_error_message());
				return -1;
			}
			if ((rc=ngpd_read_jesd_regs(path, card, link, JESD_RESET, 1, &reset)) < 0)
			{
				sv_printf(cn, "! ERROR reading JESD_RESET=0x%03X: %s\n", JESD_RESET, ngpd_get_error_message());
				return -1;
			}
			
			if ((rc=ngpd_read_jesd_regs(path, card, link, JESD_STAT_RX_ERR, 1, &rx_err_status)) < 0)
			{
				sv_printf(cn, "! ERROR reading JESD_STAT_RX_ERR=%08X: %s\n", JESD_STAT_RX_ERR, ngpd_get_error_message());
				return -1;
			}
			
			if ((rc=ngpd_read_jesd_regs(path, card, link, JESD_CFG_OCTETS, 4, cfg)) < 0)
			{
				sv_printf(cn, "! ERROR reading JESD_CFG_OCTETS=0x%03X: %s\n", JESD_CFG_OCTETS, ngpd_get_error_message());
				return -1;
			}

			if ((rc=ngpd_read_jesd_regs(path, card, link, JESD_SYNC_STATUS, 2, rx_status)) < 0)
			{
				sv_printf(cn, "! ERROR reading JESD_CFG_OCTETS=0x%03X: %s\n", JESD_SYNC_STATUS, ngpd_get_error_message());
				return -1;
			}

			sv_printf(cn, "# Card %02d : link %d : IP_VERSION = %08X, Octets=0x%08X, Frames=%08X, Lanes=%08X, Subclass=%08X\n", card, link, ip[0], cfg[0], cfg[1], cfg[2], cfg[3]);
			sv_printf(cn, "#           RESET=%08X, RX_ERR = %08X, Sync STATUS=%08X, RX_DEBUG=%08X\n",  reset, rx_err_status, rx_status[0], rx_status[1]);
		}		
	}
	return 0;
}
#endif

int ngpd_jesd_reset_cmd(int quals, int cn, int path, int card, int link )
{
	int rc;
	rc = ngzmp_jesd_reset(path, card, link );
	if (rc < 0)
	{
		sv_printf(cn, "! ERROR reseting JESD links: %s\n", ngpd_get_error_message());
		return rc;
	}
	return 0;
}

int ngpd_jesd_sysref_cmd(int quals, int cn, int path, int card, int link, int always, int delay, int required_resync)
{
	int rc;
	if (TotalArgumentCount == 2)
	{
		link = -1;
		always = JESD_SYSREF_ALWAYS_DEFAULT;
		delay = JESD_SYSREF_DELAY_DEFAULT;
		required_resync = JESD_SYSREF_RESYNC_DEFAULT;
	}
	else if (TotalArgumentCount == 3)
	{
		link = -1;
		always = JESD_SYSREF_ALWAYS_DEFAULT;
		delay = JESD_SYSREF_DELAY_DEFAULT;
		required_resync = JESD_SYSREF_RESYNC_DEFAULT;
	}
	else if (TotalArgumentCount != 6)
	{
		sv_printf(cn, "! ERROR: Please omit all setting to use default or specify all 6 arguments, not %d\n", TotalArgumentCount);
		return -1;
	}
	
	rc = ngzmp_jesd_sysref(path, card, link, always, delay, required_resync);
	if (rc < 0)
	{
		sv_printf(cn, "! ERROR setting JESD SYSREF behaviour: %s\n", ngpd_get_error_message());
		return rc;
	}
	return 0;
}
	


int ngpd_adc_setup_output_cmd(int quals, int cn, int path, int card, int vout, int preemphasis)
{
	int adc=-1;
	int pair_sel=3;
	int rc;
	
	if (quals & 1)
		adc = Qualifier[0].IntArg;
	if (quals & 2)
		pair_sel = Qualifier[1].IntArg;
	
	if ((rc=ngzmp_adc_setup_output(path, card, adc, pair_sel, vout, preemphasis)) < 0)
	{
		sv_printf(cn, "! ERROR setting ADC JESD output drive: %s\n", ngpd_get_error_message());
		return rc;
	}
	return 0;
}

int ngpd_adc_setup_sync_cmd(int quals, int cn, int path, int card, int mode, int invert)
{
	int adc=-1;
	int pair_sel=3;
	int rc;
	
	if (quals & 1)
		adc = Qualifier[0].IntArg;
	if (quals & 2)
		pair_sel = Qualifier[1].IntArg;
	
	if ((rc=ngzmp_adc_setup_sync(path, card, adc, pair_sel, mode, invert)) < 0)
	{
		sv_printf(cn, "! ERROR setting ADC JESD Sync mode: %s\n", ngpd_get_error_message());
		return rc;
	}
	return 0;
}

int ngpd_adc_readback_regs_cmd(int quals, int cn, int path, int card)
{
	int rc;
	rc=ngzmp_adc_readback_regs(path, card);
	if (rc < 0)
		sv_printf(cn, "! ERROR reading back ADC SPI registers: %s\n", ngpd_get_error_message());
	else if (rc > 0)
		sv_printf(cn, "! ERROR: %d data mismatches found reading back ADC SPI registers\n", rc);
	return rc;
}
int ngpd_adc_setup_jesd_test_cmd(int quals, int cn, int path, int card, int user_pat0, int user_pat1, int user_pat2, int user_pat3 )
{
	int test_mode = AD9694_JESD_TEST_MODE_OFF;
	int injection = AD9694_JESD_TEST_INJECT_PHY;
	u_int16_t user_pat[4];
	int adc = -1;
	int pair_sel = 3;
	int rc;
	
	switch (quals &0x7FF)
	{
	case 0:
	case 0x001:	test_mode = AD9694_JESD_TEST_MODE_OFF; break;
	case 0x002: test_mode = AD9694_JESD_TEST_MODE_CHECKERBOARD; break;
	case 0x004: test_mode = AD9694_JESD_TEST_MODE_TOGGLE; break;
	case 0x008: test_mode = AD9694_JESD_TEST_MODE_PRBS31; break;
	case 0x010: test_mode = AD9694_JESD_TEST_MODE_PRBS23; break;
	case 0x020: test_mode = AD9694_JESD_TEST_MODE_PRBS15; break;
	case 0x040: test_mode = AD9694_JESD_TEST_MODE_PRBS9; break;
	case 0x080: test_mode = AD9694_JESD_TEST_MODE_PRBS7; break;
	case 0x100: test_mode = AD9694_JESD_TEST_MODE_RAMP; break;
	case 0x200: test_mode = AD9694_JESD_TEST_MODE_CONT; break;
	case 0x400: test_mode = AD9694_JESD_TEST_MODE_SINGLE; break;
	default:
		sv_printf(cn, "! Please specify at most one test mode\n");
		return -1;
	}
	
	switch (quals & 0x3800)
	{
	case 0:
	case 0x0800:  injection = AD9694_JESD_TEST_INJECT_PHY; break;
	case 0x1000:  injection = AD9694_JESD_TEST_INJECT_INPUT; break; 
	case 0x2000:  injection = AD9694_JESD_TEST_INJECT_SCRAMBLER; break; 
	default:
		sv_printf(cn, "! Please specify at most one injection point\n");
		return -1;
	}
	if (quals & 0x4000)
		adc = Qualifier[14].IntArg;
	if (quals & 0x8000)
		pair_sel = Qualifier[15].IntArg;
	
	user_pat[0] = user_pat0;
	user_pat[1] = user_pat1;
	user_pat[2] = user_pat2;
	user_pat[3] = user_pat3;
	
	rc = ngzmp_adc_setup_jesd_test(path, card, adc, pair_sel, test_mode, injection, user_pat);

	if (rc < 0)
		sv_printf(cn, "! ERROR setting JESD test mode: %s\n", ngpd_get_error_message());
	return rc;
}

int ngzmp_set_offset_cmd(int quals, int cn, int path, int first, int num, int offset)
{
	int chan, num_chan = ngpd_get_num_chan(path);
	u_int16_t buf[NGZMP_MAX_CHAN];
	int i;
	
	if (num_chan< 0)
	{
		sv_printf(cn, "! ERROR: Cannot read number of channels : %s\n", ngpd_get_error_message());
		return -1;
	}
	if (first < 0 || first >= num_chan)
	{
		sv_printf(cn, "! ERROR: Channel %d for %d does not fit in %d channel system\n", first, num, num_chan);
		return -1;
	}
	if (num < 0)
		num = num_chan-first;
	else if (num+first > num_chan)
	{
		sv_printf(cn, "! ERROR: Channel %d for %d does not fit in %d channel system\n", first, num, num_chan);
		return -1;
	}
	for (i=0; i<num; i++)
		buf[i] = offset;
	if (ngpd_i2c_write_preamp_offset(path, first, num, buf) < 0)
	{
		sv_printf(cn, "! ERROR: Error setting offset: %s\n", ngpd_get_error_message());
		return -1;
	}

	return 0;
}
int ngzmp_set_dga_cmd(int quals, int cn, int path, int first, int num, int gain)
{
	int chan, num_chan = ngpd_get_num_chan(path);
	u_int16_t buf[NGZMP_MAX_CHAN];
	int i;
	
	if (num_chan< 0)
	{
		sv_printf(cn, "! ERROR: Cannot read number of channels : %s\n", ngpd_get_error_message());
		return -1;
	}
	if (first < 0 || first >= num_chan)
	{
		sv_printf(cn, "! ERROR: Channel %d for %d does not fit in %d channel system\n", first, num, num_chan);
		return -1;
	}
	if (num < 0)
		num = num_chan-first;
	else if (num+first > num_chan)
	{
		sv_printf(cn, "! ERROR: Channel %d for %d does not fit in %d channel system\n", first, num, num_chan);
		return -1;
	}
	for (i=0; i<num; i++)
		buf[i] = gain&0x3F;
	if (ngzmp_spi_write_dga(path, first, num, buf) < 0)
	{
		sv_printf(cn, "! ERROR: Error setting gain: %s\n", ngpd_get_error_message());
		return -1;
	}

	return 0;
}
void * ngzmp_get_dga_cmd(int quals, int cn, int path, int chan, int num)
{
	u_int16_t dga[num];
	int *base, i;

	if ((base=(int *)malloc(sizeof(int)*(num+1))) == NULL)
	{
		sv_printf(cn, "! ERROR: Out of memory\n");
		return NULL;
	}
	base[0] = num;
	if (ngzmp_spi_read_dga(path, chan, num, dga) < 0)
	{
		sv_printf(cn, "! ERROR: Error reading gain: %s\n", ngpd_get_error_message());
		free(base);
		return NULL;
	}
	for (i=0; i<num; i++)
		base[i+1] = dga[i];
	return (void *)base;
}
	
void * ngzmp_get_offset_cmd(int quals, int cn, int path, int chan, int num)
{
	u_int16_t offset[num];
	int *base, i;

	if (ngpd_i2c_read_preamp_offset(path, chan, num, offset) < 0)
	{
		sv_printf(cn, "! ERROR: Error reading offset: %s\n", ngpd_get_error_message());
		return NULL;
	}
	if ((base=(int *)malloc(sizeof(int)*(num+1))) == NULL)
	{
		sv_printf(cn, "! ERROR: Out of memory\n");
		return NULL;
	}
	base[0] = num;
	for (i=0; i<num; i++)
		base[i+1] = offset[i];
	return (void *)base;
}

int ngzmp_list_dga_offset(int quals, int cn, int path, int first, int num)
{
	int chan, num_chan;
	u_int16_t dga[NGZMP_MAX_CHAN];
	u_int16_t offset[NGZMP_MAX_CHAN];
	int i;
	
	num_chan = ngpd_get_num_chan(path);
	if (num_chan< 0)
	{
		sv_printf(cn, "! ERROR: Cannot read number of channels : %s\n", ngpd_get_error_message());
		return -1;
	}
	if (first < 0 || first >= num_chan)
	{
		sv_printf(cn, "! ERROR: Channel %d for %d does not fit in %d channel system\n", first, num, num_chan);
		return -1;
	}
	if (num < 0)
		num = num_chan-first;
	else if (num+first > num_chan)
	{
		sv_printf(cn, "! ERROR: Channel %d for %d does not fit in %d channel system\n", first, num, num_chan);
		return -1;
	}
	if (ngzmp_spi_read_dga(path, first, num, dga) < 0)
	{
		sv_printf(cn, "! ERROR: Error reading gain: %s\n", ngpd_get_error_message());
		return -1;
	}
	if (ngpd_i2c_read_preamp_offset(path, first, num, offset) < 0)
	{
		sv_printf(cn, "! ERROR: Error reading offset: %s\n", ngpd_get_error_message());
		return -1;
	}
	for (i=0;i<num; i++)
		sv_printf(cn, "# Chan=%02d: DGA=%d, Offset=%d\n", first+i, dga[i], offset[i]);
	
	return 0;
}

int ngzmp_write_offset_cmd(int quals, int cn, int path, int first, int num)
{
	int chan, num_chan = ngpd_get_num_chan(path);
	u_int16_t buf[NGZMP_MAX_CHAN];
	int *buf32=NULL;
	int i;
	
	if (num_chan< 0)
	{
		sv_printf(cn, "! ERROR: Cannot read number of channels : %s\n", ngpd_get_error_message());
		return -1;
	}
	if (first < 0 || first >= num_chan)
	{
		sv_printf(cn, "! ERROR: Channel %d for %d does not fit in %d channel system\n", first, num, num_chan);
		return -1;
	}
	if (num < 0)
		num = num_chan-first;
	else if (num+first > num_chan)
	{
		sv_printf(cn, "! ERROR: Channel %d for %d does not fit in %d channel system\n", first, num, num_chan);
		return -1;
	}

	if (unified_write_get(quals, cn, 0, num, UNIF_DATA_LONG, &buf32) < 0)
	{
		sv_printf(cn, "! Unified write system returned error\n");
		return ERROR;
	}

	for (i=0; i<num; i++)
		buf[i] = buf32[i];
	if (ngpd_i2c_write_preamp_offset(path, first, num, buf) < 0)
	{
		sv_printf(cn, "! ERROR: Error setting offset: %s\n", ngpd_get_error_message());
		free (buf32);
		return -1;
	}

	free (buf32);
	return 0;
}
