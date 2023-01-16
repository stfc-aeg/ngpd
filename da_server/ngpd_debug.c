/*----------------------------------------------------------------------
 *	XSPRESS3 Device for DA with multi-client server.
 *	By William Helsby, Geoff Mant
 *----------------------------------------------------------------------
 */

#include "header.h"
#include "ngpd.h"

/*----- Private global variables -----*/
/*----- global variables -----*/
/*----- Private function prototypes -----*/

int ngpd_dma_print_data_cmd(int quals, int cn, int path, int card, int offset, int num_bytes);
double ngpd_read_adc_temp(int quals, int cn, int path, int card);
int ngpd_set_adc_temp(int quals, int cn, int path, int card, int temp);
double ngpd_read_preamp_temp(int quals, int cn, int path, int card);
int ngpd_set_preamp_temp(int quals, int cn, int path, int card, int temp);
double ngpd_read_fpga_temp(int quals, int cn, int path, int card);
double ngpd_read_adc_voltages(int quals, int cn, int path, int card);
void * ngpd_get_fpga_voltages(int quals, int cn, int path, int card);
void * ngpd_get_adc_voltages(int quals, int cn, int path, int card);
void * ngpd_get_adc_temp(int quals, int cn, int path, int card);
void * ngpd_get_preamp_temp(int quals, int cn, int path, int card);

#if 0
int ngpd_dma_reset(int quals, int cn, int path, int card);
int ngpd_dma_print_scope_data(int quals, int cn, int path, int card, int offset, int num_bytes);
int ngpd_dma_build_test_pat(int quals, int cn, int path, int card, int address, int num_bytes, int start_val);
int ngpd_dma_print_desc(int quals, int cn, int path, int card, int first_desc, int num_desc);
int ngpd_read_glob_reg(int quals, int cn, int path, int card, int offset);
int ngpd_read_mdio_reg(int quals, int cn, int path, int card, int port, int dev, int addr);
int ngpd_write_mdio_reg(int quals, int cn, int path, int card, int port, int dev, int addr, int data);
int ngpd_write_glob_reg(int quals, int cn, int path, int card, int offset, int value);
int ngpd_dma_read_status(int quals, int cn, int path, int card);
int ngpd_dma_read_status_block(int quals, int cn, int path, int card);
int ngpd_read_time_stat_a(int quals, int cn, int path, int card, int repeat);
int ngpd_dma_check_desc(int quals, int cn, int path, int card);
int ngpd_read_chan_reg(int quals, int cn, int path, int chan, int offset, int num);
int ngpd_mdio_set_conn(int quals, int cn, int path, int card, int val);
double ngpd_read_mid_plane_temp(int quals, int cn, int path);
int ngpd_set_fem_config(int quals, int cn, int path, int card, int new_card);
int ngpd_read_fem_config(int quals, int cn, int path, int card);
int ngpd_get_revision(int quals, int cn, int path);
int ngpd_get_features(int quals, int cn, int path, int card);
int ngpd_set_inter_packet_gap(int quals, int cn, int path, int card, int gap);
int ngpd_set_b2b_loopback(int quals, int cn, int path, int card);
int ngpd_show_b2b_status(int quals, int cn, int path, int card);
int ngpd_set_b2b_ecc(int quals, int cn, int path, int card, int disable);
int ngpd_init_xtk_map(int quals, int cn, int path, int chan, int b2b_stream);
int ngpd_print_lm82_regs(int quals, int cn, int path, int card);
int ngpd_read_adc_clk(int quals, int cn, int path, int card);
int ngpd_set_adc_clk_phase(int quals, int cn, int path, int card, int divide, double phase, double phase_del, double phase_radial);
int ngpd_measure_clock_frequency(int quals, int cn, int path, int card);
double ngpd_get_clock_period(int quals, int cn, int path, int card);
int ngpd_set_udp_buffer_size(int quals, int cn, int path, int card, int hist_ports, int scope_ports);
int ngpd_get_udp_buffer_size(int quals, int cn, int path, int card);
int ngpd_dma_read_ethernet_status(int quals, int cn, int path, int card);
int ngpd_dma_get_desc_status(int quals, int cn, int path, int card, int first, int num);
int ngpd_adc_mmcm_reset(int quals, int cn, int path, int card);
#endif

PARSE_TREE ngpd_debug_tree[] = {
	PT_COMMAND ("print-data", "%d %d + %d %d direct playback scope0 scope1 scope1 hist-test hist-read dram_to_teng teng_to_dram one-col decimal = %d",
				"print data ",
				"path/card/offset/num bytes/direct/playback/scope0/scope1/scope2/histogram test data/histogram read buffer/DRAM to 10G/10G to DRAM/single column/decimal format",
				ngpd_dma_print_data_cmd),
	PT_COMMAND("read-preamp-temp", "%d %d = %g",
				"Read temperature of preamp board in 2 places and return highest",
				"Path/card (-1 for all card)",
				ngpd_read_preamp_temp),
	PT_COMMAND("set-preamp-temp", "%d %d %d chip %d = %d",
				"Set over temperature threshold for p board",
				"Path/card (-1 for all card)/Threshold temperature in degrees C/Specify chip 0 or 1 (default is to set both)",
				ngpd_set_preamp_temp),
	PT_COMMAND("read-adc-temp", "%d %d = %g",
				"Read temperature of ADC board in 4 places and return highest",
				"Path/card (-1 for all card)",
				ngpd_read_adc_temp),
	PT_COMMAND("set-adc-temp", "%d %d %d chip %d = %d",
				"Set over temperature threshold for ADC board",
				"Path/card (-1 for all card)/Threshold temperature in degrees C/Specify chip 0...3 (default is to set all 4)",
				ngpd_set_adc_temp),
	PT_COMMAND("read-adc-voltage", "%d %d file %s = %d",
				"Read supply voltage monitors from ADC boards",
				"Path/card (-1 for all card)/Write data to file rather than screen",
				ngpd_read_adc_voltages),
	PT_COMMAND("read-fpga-temp", "%d %d file %s = %g",
				"Read temperature of FPGA board and FPGA and Voltages where supported",
				"Path/card (-1 for all card)/Write data to file rather than screen",
				ngpd_read_fpga_temp),
	PT_COMMAND("get-fpga-voltage", "%d %d = {}",
				"Get FPGA voltages as name/value pairs (Python dictionary)",
				"Path/Card",
				ngpd_get_fpga_voltages ),
	PT_COMMAND("get-adc-voltage", "%d %d = {}",
				"Get ADC board voltages as name/value pairs (Python dictionary)",
				"Path/Card",
				ngpd_get_adc_voltages ),
	PT_COMMAND("get-adc-temp", "%d %d = {}",
				"Read temperature of ADC board in 4 places and return as dictionary",
				"Path/card",
				ngpd_get_adc_temp),
	PT_COMMAND("get-preamp-temp", "%d %d = {}",
				"Read temperature of preamp board in 2 places and return as dictionary",
				"Path/card",
				ngpd_get_preamp_temp),
#if 0
	PT_COMMAND ("reset", "%d %d playback scope0 scope1 scalers hist-to-dram dram-to-teng teng-to-dram = %d",
				"reset a dma stream default is all streams",
				"path/card/playback/scope0/scope1/scalers/hist-to-dram/dram-to-teng/teng-to-dram",
				ngpd_dma_reset),
	PT_COMMAND ("build-test-pat", "%d %d + %d %d %d direct playback scope0 scope1 scalers dram_to_teng teng_to_dram inc32 inc8 slide interleave = %d",
				"build a test pattern default pattern is inc32",
				"path/card/address/num_val/start_val/direct/playback/scope0/scope1/scalers/dram_to_teng/teng_to_dram/32bit incrementing count/8bit incrementing count/32bit sliding pattern/"
				"2 interleaved 16 bit patterns",
				ngpd_dma_build_test_pat),
	PT_COMMAND ("print-scope", "%d %d + %d %d decimal = %d",
				"print data ",
				"path/card/offset/num bytes/decimal format",
				ngpd_dma_print_scope_data),
	PT_COMMAND ("print-desc", "%d %d + %d %d playback scope0 scope1 scalers dram-to-teng teng-to-dram  hist-list hist-frames = %d",
				"Print DMA Descriptors ",
				"path/card/First Descriptor/Num Descriptor/playback/scope0/scope1/scalers/dram_to_teng/teng_to_dram/hist-list/hist-frames",
				ngpd_dma_print_desc),
	PT_COMMAND ("read-status", "%d %d playback scope0 scope1 scalers dram-to-teng teng-to-dram hist-list hist-frames = %d",
				"Read Status from DMA stream, print and return ",
				"path/card/playback/scope0/scope1/scalers/DRAM to 10GEthernet/10GEthernet to DRAM_to_dram/hist-list/hist-frames",
				ngpd_dma_read_status),
	PT_COMMAND ("read-status-block", "%d %d playback scope0 scope1 scalers dram-to-teng teng-to-dram hist-list hist-frames  = %d",
				"read and Print DMA status block ",
				"path/card/playback/scope0/scope1/scalers/DRAM to 10GEthernet/10GEthernet to DRAM_to_dram/Histogram event List (Xspress3mini only)/Histogram complete Frames (Xspress3Mini Only) ",
				ngpd_dma_read_status_block),
	PT_COMMAND ("read-glob-reg", "%d %d %d bus-clock = %d",
				"Read and Print a Global Control register ",
				"path/card/Long word Offset/Or Offset with flag to mark this as a bus clock register access (Xspress3m and Xspress4)",
				ngpd_read_glob_reg),
	PT_COMMAND ("write-glob-reg", "%d %d %d %d bus-clock  = %d",
				"Write and Print a Global Control register ",
				"path/card/Long word Offset/Value/Or Offset with flag to mark this as a bus clock register access (Xspress3m and Xspress4)",
				ngpd_write_glob_reg),
	PT_COMMAND ("read-chan-reg", "%d %d %d %d region %d = %d",
				"Read and Print a Channel Control register(s) ",
				"path/Channel (-1 for all)/Long word Offset/Number of regs/Specify region (default registers)",
				ngpd_read_chan_reg),
#if 0
	PT_COMMAND ("start-dma", "%d %d + %d %d direct playback scope0 scope1 scalers dram_to_teng teng_to_dram = %d",
				"start dma ",
				"path/card/offset/num bytes/direct/playback/scope0/scope1/scalers/dram_to_teng/teng_to_dram/single column/decimal format",
				ngpd_dma_start),
#endif
	PT_COMMAND( "mdio-set-conn", "%d %d %d = %d",
				"Set MDIO connection to on Hitech Global FMC", 
				"Path/Card/Value 0 => via FMC, 1=> Via Aux connector",
				ngpd_mdio_set_conn ),
	PT_COMMAND( "read-mdio", "%d %d %d %d %d = %d",
				"Read MDIO register on Hitech Global FMC via Aux Cable", 
				"Path/Card/Port/Device/Addr",
				ngpd_read_mdio_reg ),
	PT_COMMAND( "write-mdio", "%d %d %d %d %d %d = %d",
				"Read MDIO register on Hitech Global FMC via Aux Cable", 
				"Path/Card/Port/Device/Addr/Data",
				ngpd_write_mdio_reg ),
	PT_COMMAND( "read-time-stat-a", "%d %d + %d  = %d",
				"Read Global Timing status register A", 
				"Path/Card/Repeat Count (default 1)",
				ngpd_read_time_stat_a),
	PT_COMMAND( "check-desc", "%d %d playback scope0 scope1 scalers dram-to-teng teng-to-dram hist-list hist-frames circ = %d",
				"Check DMA descriptor", 
				"Path/Card/playback/scope0/scope1/scalers/dram_to_teng/teng_to_dram/hist-list/hist-frames/Use circular buffer version (xspress3mini only)",
				ngpd_dma_check_desc),
	PT_COMMAND( "get-desc-status", "%d %d +%d %d playback scope0 scope1 scalers dram-to-teng teng-to-dram hist-list hist-frames = %d",
				"Get DMA descriptor status words", 
				"Path/Card/First Descriptor (default 0)/Num descriptor (default all)/playback/scope0/scope1/scalers/dram_to_teng/teng_to_dram/hist-list/hist-frames",
				ngpd_dma_get_desc_status ),
	PT_COMMAND ("set-trigger-regs-b", "%d %d %d %d %d %d = %d",
		        "Setup channel triggers B registers",
		        "path/channel/thres/timea/timeb/fast",
		    	ngpd_set_trigger_regs_b),
	PT_COMMAND ("set-trigger-regs-c", "%d %d %d %d = %d",
		        "Setup channel triggers",
		        "path/channel/time OTD for servo/Trig C thresh if available",
		    	ngpd_set_trigger_regs_c),
	PT_COMMAND ("set-reset-reg", "%d %d %d %d %d = %d",
		    	"Setup channel resets",
		    	"path/channel/reseta/resetb/resetc",
		    	ngpd_set_reset_regs),
	PT_COMMAND ("set-glitch-reg", "%d %d %d %d = %d",
				"Setup channel glitchs",
				"path/channel/glitcha/glitchb",
				ngpd_set_glitch_regs),
	PT_COMMAND ("set-servo-regs", "%d %d %d %d %d = %d",
				"Setup channel servos",
				"path/channel/servoa/servob/servoc",
				ngpd_set_servo_regs),
	PT_COMMAND("set-ppc-debuglevel", "%d %d ppc1 ppc2 quiet sparse normal verbose = %d",
				"Set debug level for ppc1 and/or ppc2",
				"Path/card (-1 for all card)/PPC1/PPC2/no messages/very few messages/normal messages/verbose messages",
				ngpd_set_ppc_debuglevel),
	PT_COMMAND("read-mid-plane-temp", "%d = %g",
				"Read temperature of Mid-plane  in 3 places and return highest",
				"Path",
				ngpd_read_mid_plane_temp),
	PT_COMMAND("read-lm82-regs", "%d %d = %d",
				"Read and print register of LM82 temperature monitor of FPGA board and FPGA (XSpress3 only)",
				"Path/card (-1 for all card)",
				ngpd_print_lm82_regs),
	PT_COMMAND("read-fem-config", "%d %d = %d",
				"Read and print FEM Config",
				"Path/card/",
				ngpd_read_fem_config),
	PT_COMMAND("set-fem-config", "%d %d %d force t_crit %d = %d",
				"Modify FEM Config to set new board number for card and write to EEPROM",
				"Path/card/New Card Number (0..7)/Override range checking on temperatures/Change LM82 T_crit FPGA shutdown temperature",
				ngpd_set_fem_config),
	PT_COMMAND("get-revision", "%d = %d",
				"Get revison number",
				"Path",
				ngpd_get_revision),
	PT_COMMAND("get-features", "%d %d = %d",
				"Display Firmware Compiled features",
				"Path/Card",
				ngpd_get_features),
	PT_COMMAND("set-int-pack-gap", "%d %d %d = %d",
				"Set UDP inter-packet gap in cycles of 6.4 ns",
				"Path/Card/Gap",
				ngpd_set_inter_packet_gap),
	PT_COMMAND("setup-b2b-loopback", "%d %d l0 l1 l2 l3 l4 l5 near-pcs near-pma far-pma far-pcs over-ride = %d",
		"Setup loopback modes in B2b aurora cores",
		"Path/Card or -1 for all/Link 0/Link 1/Link 2/Link 3/Link 4/Link 5/Near End PCS/Near End PMA/Far End PMA/Far End PCS/Over ride bit", 
		ngpd_set_b2b_loopback ),
	PT_COMMAND("show-b2b-status", "%d %d = %d",
			"Print Board to Board Aurora Status",
			"Path/Card (-1 for all)",
			ngpd_show_b2b_status ),
	PT_COMMAND("set-b2b-ecc", "%d %d %d = %d",
			"Enable or Disable Board to Board Aurora Error Detection and Correction",
			"Path/Card (-1 for all)/Disable (1 to disable, 0 to enable ECC)",
			ngpd_set_b2b_ecc ),
	PT_COMMAND("init-xtk-map", "%d %d %d enable = %d",
			"Initialise Xtk mapping RAM to enable/disable certain B2B streams",
			"Path/Channel (-1 for all)/B2b create (0=> same board 1..6 neighbours 0..5)/Enable from this board",
			ngpd_init_xtk_map ),
	PT_COMMAND("read-adc-clk", "%d %d = %d",
			"Read and display the DRP registers in the ADC Clk MMCM",
			"Path/Card (-1 for all)/",
			ngpd_read_adc_clk ),
	PT_COMMAND("setup-adc-mmcm", "%d %d %d %g %g %g = %d",
			"Setup ADC MMCM divde and phase",
			"Path/Card (-1 for all)/Divide from VCO clock to ADC CLK (typically 14)/"
			"Phase of main ADC CLk (degrees)/Phase of delayed ADC CLk (degrees)/Phase of Radial signal receive ADC CLk (degrees)/",
			ngpd_set_adc_clk_phase ),
	PT_COMMAND ("reset-adc-mmcm", " %d %d reload = %d",
			"Reset the ADC MMCM",
			"Path/Card/Force Reload rather than just reset",
			ngpd_adc_mmcm_reset ),
	PT_COMMAND("measure-freq", "%d %d all = %d",
		"Measure current ADC Clock frequency (against FPGA clock), store period into data structure and return frequency",
		"Path/Card (-1 for all)/Also measure frequencies for the UDP and Brd2Brd clocking",
		ngpd_measure_clock_frequency ),
	PT_COMMAND("get-clock-period", "%d %d = %g",
		"Return the clock period stored in the XSPRESS system data structures",
		"Path/Card (-1 for all)",
		ngpd_get_clock_period ),
	PT_COMMAND("set-udp-buffers", "%d %d %d %d = %d",
		"Set Buffer size of histogram and scope ports",
		"Path/Card (-1 for all)/Histogram UDP buffer size in bytes (-1 to use default)/Scope UDP buffer size in bytes (-1 to use default)",
		ngpd_set_udp_buffer_size),
	PT_COMMAND("get-udp-buffers", "%d %d = %d",
		"Get buffer sizes for histogram and scoep mode ports",
		"Path/Card (-1 for print all)",
		ngpd_get_udp_buffer_size ),
	PT_COMMAND ("ethernet-status", "%d %d reset-counts = %d",
		"Read and display the Ethernet status registers/Reset XGMII counts after read",
		"Path/Card (-1 for all)",
		ngpd_dma_read_ethernet_status ),
	PT_COMMAND ("toggle-enb", "%d even period %d offset %d debug = %d",
				"Run tests where discriminator enables are toggled tp simulate turning beam onn and off",
				"Path/Turn off in even frames (default odd)/Specify period of toggling (default 2)/Specify frame to turn beam off in (default 1)/Print debug messages", 
		ngpd_toggle_tests ),
#endif
	PT_END

};

double ngpd_read_adc_temp(int quals, int cn, int path, int card)
{
	int rc;
	double max_temp;
	float temp[4];
	int status[4];
	int first, last, num_cards;
	int i;

	num_cards = ngpd_get_num_cards(path); 
	if (num_cards <= 0)
	{
		sv_printf(cn, "! ERROR: ngpd_get_num_cards returns error : %s\n", ngpd_get_error_message());
		return num_cards;
	}

	if (card < 0)
	{
		first = 0;
		last = num_cards-1;
	}
	else
	{
		if (card >= num_cards)
		{
			sv_printf(cn, " ! ERROR: card %d is out of range 0..%d\n", card, num_cards-1);
			return -1;
		}
		first = card;
		last  = card;
	}
	max_temp = -200;
	for (card=first; card <= last; card++)
	{
		sv_printf(cn, "# Card=%d", card);
		rc = ngpd_i2c_read_adc_temp(path, card, temp, status);
		if (rc < 0)
		{
			sv_printf(cn, "ERROR reading temperatures: %s\n", ngpd_get_error_message());
			return rc;
		}
		for (i=0;i<NGZMP_I2C_NUM_ADT7410_ADC;i++)
		{
			if (temp[i] > max_temp)
				max_temp = temp[i];
		}

		for (i=0;i<NGZMP_I2C_NUM_ADT7410_ADC;i++)
			sv_printf(cn, "\t%g", temp[i]);
		for (i=0;i<NGZMP_I2C_NUM_ADT7410_ADC;i++)
			sv_printf(cn, "\t%d", status[i]);
		sv_printf(cn, "\n");
	}
	return max_temp;
}
#define CHUNK_NUM_CARDS 10

double ngpd_read_adc_voltages(int quals, int cn, int path, int card)
{
	int rc;
	int first, last, num_cards;
	int i, chip, rail;
	int32_t v[NGZMP_MAX_CARDS][NGZMP_UCD90160_NUM_CHIPS][NGZMP_UCD90160_NUM_RAILS];
	NGPDGeneration generation;
	int chunk_num_cards;
	char *fname = NULL;
	FILE *ofp=NULL;
	
	if (quals & 1)
		fname = Qualifier[0].StringArg;
	
	num_cards = ngpd_get_num_cards(path); 
	if (num_cards <= 0)
	{
		sv_printf(cn, "! ERROR: ngpd_get_num_cards returns error : %s\n", ngpd_get_error_message());
		return num_cards;
	}
	generation = ngpd_get_generation(path);
	if (generation == NGPDGenADQ14)
	{
		sv_printf(cn, "ngpd debug read-adc-voltage not supportd on ADQ14 (yet?)\n");
		return -1;
	}
	if (card < 0)
	{
		first = 0;
		last = num_cards-1;
	}
	else
	{
		if (card >= num_cards)
		{
			sv_printf(cn, " ! ERROR: card %d is out of range 0..%d\n", card, num_cards-1);
			return -1;
		}
		first = card;
		last  = card;
	}
	if (fname != NULL)
	{
		if ((ofp=fopen(fname, "w")) == NULL)
		{
			sv_printf(cn, "! ERROR: Cannot open output file '%s', errno=%d\n", fname, errno);
			return -1;
		}
	}
			
	while (first <= last)
	{
		if (fname == NULL && 1+last-first > CHUNK_NUM_CARDS)
			chunk_num_cards = CHUNK_NUM_CARDS;
		else
			chunk_num_cards = 1+last-first;
		for (i=0; i<chunk_num_cards; i++)
		{
			card = first+i;
			for (chip=0; chip<NGZMP_UCD90160_NUM_CHIPS; chip++)
			{
				rc = ngzmp_i2c_read_ucd90160_vout(path, card, chip, 0, NGZMP_UCD90160_NUM_RAILS, v[i][chip]);
				if (rc < 0)
				{
					sv_printf(cn, "ERROR reading UCD90160  data: %s\n", ngpd_get_error_message());
					if (ofp != NULL)
						fclose (ofp);
					return rc;
				}
			}
		}
		if (ofp != NULL)
		{
			fprintf(ofp, "Card\t\t");
			for (i=0; i<chunk_num_cards; i++)
				fprintf(ofp, "\t%d", i+first);
			for (chip=0; chip<NGZMP_UCD90160_NUM_CHIPS; chip++)
			{
				for (rail=0; rail<NGZMP_UCD90160_NUM_RAILS; rail++)
				{
					if (ngpd_ucd90160_signals[chip][rail] != NULL)
					{
						fprintf(ofp, "\n%d\t%d\t%s", chip, rail+1, ngpd_ucd90160_signals[chip][rail]);
						for (i=0; i<chunk_num_cards; i++)
							fprintf(ofp, "\t%7.3f", v[i][chip][rail]*0.0001);
					}
				}
			}
			fprintf(ofp, "\n");
		}
		else		
		{
			sv_printf(cn, "#                           ");
			for (i=0; i<chunk_num_cards; i++)
				sv_printf(cn, "  Card%d", i+first);
		
			for (chip=0; chip<NGZMP_UCD90160_NUM_CHIPS; chip++)
			{
				for (rail=0; rail<NGZMP_UCD90160_NUM_RAILS; rail++)
				{
					if (ngpd_ucd90160_signals[chip][rail] != NULL)
					{
						sv_printf(cn, "\n# C%d R%02d %18s ", chip, rail+1, ngpd_ucd90160_signals[chip][rail]);
						for (i=0; i<chunk_num_cards; i++)
							sv_printf(cn, " %6.3f", v[i][chip][rail]*0.0001);
					}
				}
			}
			sv_printf(cn, "\n");
		}
		first += chunk_num_cards;
	}
	if (ofp != NULL)
		fclose(ofp);

	return 0;
}


int ngpd_set_adc_temp(int quals, int cn, int path, int card, int temp)
{
	int rc;
	int chip = -1;
	if (quals & 1)
		chip = Qualifier[0].IntArg;
	
	rc = ngpd_i2c_write_adc_tcrit(path, card, chip, temp);
	if (rc < 0)
	{
		sv_printf(cn, "ERROR writing temperature: %s\n", ngpd_get_error_message());
		return rc;
	}

	return rc;
}

double ngpd_read_fpga_temp(int quals, int cn, int path, int card)
{
	int rc;
	double max_temp, temp;
	int first, last, num_cards;
	int i;
	int pnum;
	u_int32_t ams_data[NGZMP_MAX_CARDS][NGZMP_ParamMax];
	NGPDGeneration generation;
	int chunk_num_cards;
	char *fname=NULL;
	FILE *ofp=NULL;
	
	num_cards = ngpd_get_num_cards(path); 
	if (num_cards <= 0)
	{
		sv_printf(cn, "! ERROR: ngpd_get_num_cards returns error : %s\n", ngpd_get_error_message());
		return num_cards;
	}
	generation = ngpd_get_generation(path);
	if (generation == NGPDGenADQ14)
	{
		sv_printf(cn, "ngpd debug read-fpga-temp not supportd on ADQ14 (yet?)\n");
		return -1;
	}
	if (quals & 1)
	{
		fname = Qualifier[0].StringArg;
		if ((ofp=fopen(fname, "w")) == NULL)
		{
			sv_printf(cn, "! ERROR: Cannot open output file '%s, errno=%d\n", fname, errno);
			return -1;
		}
	}
	
	if (card < 0)
	{
		first = 0;
		last = num_cards-1;
	}
	else
	{
		if (card >= num_cards)
		{
			sv_printf(cn, " ! ERROR: card %d is out of range 0..%d\n", card, num_cards-1);
			return -1;
		}
		first = card;
		last  = card;
	}
	max_temp = -200;
	while (first <= last)
	{
		if (ofp==NULL && 1+last-first > CHUNK_NUM_CARDS)
			chunk_num_cards = CHUNK_NUM_CARDS;
		else
			chunk_num_cards = 1+last-first;
		for (i=0; i<chunk_num_cards; i++)
		{
			card = first+i;
			rc = ngzmp_read_xadc(path, card, 0, NGZMP_ParamMax, ams_data[i]);
			if (rc < 0)
			{
				sv_printf(cn, "ERROR reading system monitor data: %s\n", ngpd_get_error_message());
				if (ofp != NULL)
					fclose (ofp);
				return rc;
			}
			if ((temp=0.001*ams_data[i][NGZMP_AMS_PSTempLPD]) > max_temp)
				max_temp = temp;
			if ((temp=0.001*ams_data[i][NGZMP_AMS_PSTempFPD]) > max_temp)
				max_temp = temp;
			if ((temp=0.001*ams_data[i][NGZMP_XADC_Temp]) > max_temp)
				max_temp = temp;	
		}
		if (ofp == NULL)
		{
			sv_printf(cn, "#                      ");
			for (i=0; i<chunk_num_cards; i++)
				sv_printf(cn, "  Card%d", i+first);
			for (pnum=0; pnum<NGZMP_ParamMax; pnum++)
			{
				sv_printf(cn, "\n# %20s ", ngzmp_sys_mon_name[pnum]);
				for (i=0; i<chunk_num_cards; i++)
					sv_printf(cn, " %6.3f", ams_data[i][pnum]*0.001);
			}
			sv_printf(cn, "\n");
		}
		else
		{
			fprintf(ofp, "Card");
			for (i=0; i<chunk_num_cards; i++)
				fprintf(ofp, "\t%d", i+first);
			for (pnum=0; pnum<NGZMP_ParamMax; pnum++)
			{
				fprintf(ofp, "\n%s", ngzmp_sys_mon_name[pnum]);
				for (i=0; i<chunk_num_cards; i++)
					fprintf(ofp, "\t%6.3f", ams_data[i][pnum]*0.001);
			}
			fprintf(ofp, "\n");
		}
		first += chunk_num_cards;
	}
	if (ofp != NULL)
		fclose (ofp);

	return max_temp;
}

double ngpd_read_preamp_temp(int quals, int cn, int path, int card)
{
	int rc;
	double max_temp;
	float temp[4];
	int status[4];
	int first, last, num_cards;
	int i;

	num_cards = ngpd_get_num_cards(path); 
	if (num_cards <= 0)
	{
		sv_printf(cn, "! ERROR: ngpd_get_num_cards returns error : %s\n", ngpd_get_error_message());
		return num_cards;
	}

	if (card < 0)
	{
		first = 0;
		last = num_cards-1;
	}
	else
	{
		if (card >= num_cards)
		{
			sv_printf(cn, " ! ERROR: card %d is out of range 0..%d\n", card, num_cards-1);
			return -1;
		}
		first = card;
		last  = card;
	}
	max_temp = -200;
	for (card=first; card <= last; card++)
	{
		sv_printf(cn, "# Card=%d", card);
		rc = ngpd_i2c_read_preamp_temp(path, card, temp, status);
		if (rc < 0)
		{
			sv_printf(cn, "ERROR reading temperatures: %s\n", ngpd_get_error_message());
			return rc;
		}
		for (i=0;i<NGZMP_I2C_NUM_ADT7410_PREAMP;i++)
		{
			if (temp[i] > max_temp)
				max_temp = temp[i];
		}

		for (i=0;i<NGZMP_I2C_NUM_ADT7410_PREAMP;i++)
			sv_printf(cn, "\t%g", temp[i]);
		for (i=0;i<NGZMP_I2C_NUM_ADT7410_PREAMP;i++)
			sv_printf(cn, "\t%d", status[i]);
		sv_printf(cn, "\n");
	}
	return max_temp;
}

int ngpd_set_preamp_temp(int quals, int cn, int path, int card, int temp)
{
	int rc;
	int chip = -1;
	if (quals & 1)
		chip = Qualifier[0].IntArg;
	
	rc = ngpd_i2c_write_preamp_tcrit(path, card, chip, temp);
	if (rc < 0)
	{
		sv_printf(cn, "ERROR writing temperature: %s\n", ngpd_get_error_message());
		return rc;
	}

	return rc;
}



#if 0
int ngpd_dma_reset(int quals, int cn, int path, int card) {
	int rc = 0;
	u_int32_t stream_mask = 0;

	if (quals == 0) {
		stream_mask = XSP3_DMA_STREAM_MASK_PLAYBACK
					| XSP3_DMA_STREAM_MASK_SCOPE0
					| XSP3_DMA_STREAM_MASK_SCOPE1
					| XSP3_DMA_STREAM_MASK_SCALERS
					| XSP3_DMA_STREAM_MASK_HIST_TO_DRAM
					| XSP3_DMA_STREAM_MASK_DRAM_TO_10G
					| XSP3_DMA_STREAM_MASK_10G_TO_DRAM;
	} else {
		if (quals & 0x1)
			stream_mask |= XSP3_DMA_STREAM_MASK_PLAYBACK;
		if (quals & 0x2)
			stream_mask |= XSP3_DMA_STREAM_MASK_SCOPE0;
		if (quals & 0x4)
			stream_mask |= XSP3_DMA_STREAM_MASK_SCOPE1;
		if (quals & 0x8)
			stream_mask |= XSP3_DMA_STREAM_MASK_SCALERS;
		if (quals & 0x10)
			stream_mask |= XSP3_DMA_STREAM_MASK_DRAM_TO_10G;
		if (quals & 0x20)
			stream_mask |= XSP3_DMA_STREAM_MASK_10G_TO_DRAM;
	}
	rc = ngpd_dma_reset(path, card, stream_mask);
	if (rc != 0) {
		sv_printf(cn,"! ERROR %s\n", ngpd_get_error_message());
	}
	return rc;
}

int ngpd_dma_build_test_pat(int quals, int cn, int path, int card, int address, int num_bytes, int start_val) {

	int rc = 0;
	u_int32_t stream = 0;
	XSP3_DMA_MsgTestPat msg;
	u_int32_t options = 0;

	switch (quals & 0x7F) {
	case 0x1:
		stream = XSP3_DMA_STREAM_BNUM_DIRECT;
		break;
	case 0x2:
		stream = XSP3_DMA_STREAM_BNUM_PLAYBACK;
		break;
	case 0x4:
		stream = XSP3_DMA_STREAM_BNUM_SCOPE0;
		break;
	case 0x8:
		stream = XSP3_DMA_STREAM_BNUM_SCOPE1;
		break;
	case 0x10:
		stream = XSP3_DMA_STREAM_BNUM_SCALERS;
		break;
	case 0x20:
		stream = XSP3_DMA_STREAM_BNUM_DRAM_TO_10G;
		break;
	case 0x40:
		stream = XSP3_DMA_STREAM_BNUM_10G_TO_DRAM;
		break;
	default:
		stream = XSP3_DMA_STREAM_BNUM_DIRECT;
		break;
	}
	switch (quals & 0x780) {
	case 0x80:
		options = XSP3_DMA_MSG_TP_INC32;
		break;
	case 0x100:
		options = XSP3_DMA_MSG_TP_INC8;
		break;
	case 0x200:
		options = XSP3_DMA_MSG_TP_SLIDE;
		if (start_val == 0) start_val = 1;
		break;
	case 0x400:
		options = XSP3_DMA_MSG_TP_PLAYBACK;
		break;
	default:
		options = XSP3_DMA_MSG_TP_INC32;
		break;
	}
	msg.address = address;
	msg.num_bytes = num_bytes;
	msg.start_val = start_val;
	msg.options = options;

	rc =  ngpd_dma_build_test_pat(path, card, stream, &msg);
	if (rc != 0) {
		sv_printf(cn,"! ERROR %s\n", ngpd_get_error_message());
	}
	return rc;
}
#endif

int ngpd_dma_print_data_cmd(int quals, int cn, int path, int card, int offset, int num_bytes)
{
	int rc = 0;
	u_int32_t stream;
	ZYNQMP_DMA_MsgPrint msg;
	u_int32_t options = 0;

	switch (quals & 0x1FF) {
	case 0x1:
		stream = ZYNQMP_DMA_STREAM_BNUM_DIRECT;
		break;
	case 0x2:
		stream = NGZMP_DMA_STREAM_BNUM_PLAYBACK0;
		break;
	case 0x4:
		stream = NGZMP_DMA_STREAM_BNUM_SCOPE0;
		break;
	case 0x8:
		stream = NGZMP_DMA_STREAM_BNUM_SCOPE1;
		break;
	case 0x10:
		stream = NGZMP_DMA_STREAM_BNUM_SCOPE2;
		break;
	case 0x20:
		stream = NGZMP_DMA_STREAM_BNUM_HIST_TEST;
		break;
	case 0x40:
		stream = NGZMP_DMA_STREAM_BNUM_HIST_READ;
		break;
	case 0x80:
		stream = ZYNQMP_DMA_STREAM_BNUM_DRAM_TO_10G;
		break;
	case 0x100:
		stream = ZYNQMP_DMA_STREAM_BNUM_10G_TO_DRAM;
		break;

	default:
		sv_printf(cn, "! ERROR: Please specify one and only one stream\n");
		return -1;
	}
	if (quals & 0x200) {
		options |= ZYNQMP_DMA_MSG_PRINT_1COL;
	}
	if (quals & 0x400) {
		options |= ZYNQMP_DMA_MSG_PRINT_DEC;
	}

	msg.offset = offset;
	msg.num_bytes = num_bytes;
	msg.options = options;

	rc = ngzmp_dma_print_data(path, card, stream, &msg);
	if (rc != 0) {
		sv_printf(cn,"! ERROR %s\n", ngpd_get_error_message());
	}
	return rc;
}
#if 0
int ngpd_dma_print_scope_data(int quals, int cn, int path, int card, int offset, int num_bytes) {
	int rc = 0;
	u_int32_t stream;
	XSP3_DMA_MsgPrint msg;
	u_int32_t options = 0;

	if (quals & 0x1) {
		options |= ZYNQMP_DMA_MSG_PRINT_DEC;
	}

	msg.offset = offset;
	msg.num_bytes = num_bytes;
	msg.options = options;

	rc = ngpd_dma_print_scope_data(path, card, &msg);
	if (rc != 0) {
		sv_printf(cn,"! ERROR %s\n", ngpd_get_error_message());
	}
	return rc;
}

int ngpd_dma_print_desc(int quals, int cn, int path, int card, int first_desc, int num_desc)
{
	int rc = 0;
	u_int32_t stream;
	XSP3_DMA_MsgPrintDesc msg;
	u_int32_t options = 0;

	switch (quals & 0xFF) {
	case 0x1:
		stream = XSP3_DMA_STREAM_BNUM_PLAYBACK;
		break;
	case 0x2:
		stream = XSP3_DMA_STREAM_BNUM_SCOPE0;
		break;
	case 0x4:
		stream = XSP3_DMA_STREAM_BNUM_SCOPE1;
		break;
	case 0x8:
		stream = XSP3_DMA_STREAM_BNUM_SCALERS;
		break;
	case 0x10:
		stream = XSP3_DMA_STREAM_BNUM_DRAM_TO_10G;
		break;
	case 0x20:
		stream = XSP3_DMA_STREAM_BNUM_10G_TO_DRAM;
		break;
	case 0x40:
		stream = XSP3M_DMA_STREAM_BNUM_HIST_LIST;
		break;
	case 0x80:
		stream = XSP3M_DMA_STREAM_BNUM_HIST_FRAMES;
		break;
	default:
		sv_printf(cn, "! Please specify 1 and only 1 DMA stream\n");
		return -1;
	}

	msg.first_desc = first_desc;
	msg.num_desc   = num_desc;
	msg.options = options;

	rc = ngpd_dma_print_desc(path, card, stream, &msg);
	if (rc != 0) {
		sv_printf(cn,"! ERROR %s\n", ngpd_get_error_message());
	}
	return rc;
}
int ngpd_read_glob_reg(int quals, int cn, int path, int card, int offset)
{
	u_int32_t reg;
	int bus_clock=quals & 1;
	int rc;

	if (ngpd_get_generation(path, card) == XspressGen3)
		rc = ngpd_read_glob_reg(path, card, offset, 1, &reg);
	else
	{
		if (bus_clock)
			offset |= XSP4_GLOB_REG_BUS_CLK_FLAG;
		rc = xsp4_read_glob_reg(path, card, offset, 1, &reg);
	}
	sv_printf(cn, "# rc=%d, Reg=0x%08X=%d\n", rc, reg, reg);
	return (int)reg;
}

int ngpd_write_glob_reg(int quals, int cn, int path, int card, int offset, int value)
{
	u_int32_t reg=value;
	int bus_clock=quals & 1;
	int rc;
	
	if (ngpd_get_generation(path, card) == XspressGen3)
		rc = ngpd_write_glob_reg(path, card, offset, 1, &reg);
	else
	{
		if (bus_clock)
			offset |= XSP4_GLOB_REG_BUS_CLK_FLAG;
		rc = xsp4_write_glob_reg(path, card, offset, 1, &reg);
	}
	sv_printf(cn, "# rc=%d, Reg=0x%08X=%d\n", rc, reg, reg);
	return (int)reg;
}
int ngpd_read_mdio_reg(int quals, int cn, int path, int card, int port, int dev, int addr)
{
	u_int32_t data;
	int rc;
	rc = ngpd_mdio_read(path, card, port, dev, addr, &data);
	sv_printf(cn, "# Phy %d %d.%04X=%04X\n", port, dev, addr, data & 0xFFFF);
	return rc;
} 

int ngpd_write_mdio_reg(int quals, int cn, int path, int card, int port, int dev, int addr, int data)
{
	int rc;
	rc = ngpd_mdio_write(path, card, port, dev, addr, data);
//	sv_printf(cn, "# Phy %d %d.%04X=%04X\n", port, dev, addr, (data>>1) & 0xFFFF);
	return rc;
} 

int ngpd_dma_read_status(int quals, int cn, int path, int card)
{
	int rc = 0;
	u_int32_t stream;
	XSP3_DMA_MsgPrintDesc msg;
	u_int32_t options = 0;

	switch (quals & 0xFF) {
	case 0x1:
		stream = XSP3_DMA_STREAM_BNUM_PLAYBACK;
		break;
	case 0x2:
		stream = XSP3_DMA_STREAM_BNUM_SCOPE0;
		break;
	case 0x4:
		stream = XSP3_DMA_STREAM_BNUM_SCOPE1;
		break;
	case 0x8:
		stream = XSP3_DMA_STREAM_BNUM_SCALERS;
		break;
	case 0x10:
		stream = XSP3_DMA_STREAM_BNUM_DRAM_TO_10G;
		break;
	case 0x20:
		stream = XSP3_DMA_STREAM_BNUM_10G_TO_DRAM;
		break;
	case 0x40:
		stream = XSP3M_DMA_STREAM_BNUM_HIST_LIST;
		break;
	case 0x80:
		stream = XSP3M_DMA_STREAM_BNUM_HIST_FRAMES;
		break;
	default:
		sv_printf(cn, "! Please specify 1 and only 1 DMA stream\n");
		return -1;
	}


	rc = ngpd_dma_read_status(path, card, 1<<stream);
	if (rc < 0) {
		sv_printf(cn,"! ERROR %s\n", ngpd_get_error_message());
	}
	sv_printf(cn, "# Status = %08X\n", rc);
	return rc;
}
int ngpd_dma_read_status_block(int quals, int cn, int path, int card)
{
	int rc = 0;
	u_int32_t stream;
	XSP3_DMA_StatusBlock statusBlock;

	switch (quals & 0xFF) {
	case 0x1:
		stream = XSP3_DMA_STREAM_BNUM_PLAYBACK;
		break;
	case 0x2:
		stream = XSP3_DMA_STREAM_BNUM_SCOPE0;
		break;
	case 0x4:
		stream = XSP3_DMA_STREAM_BNUM_SCOPE1;
		break;
	case 0x8:
		stream = XSP3_DMA_STREAM_BNUM_SCALERS;
		break;
	case 0x10:
		stream = XSP3_DMA_STREAM_BNUM_DRAM_TO_10G;
		break;
	case 0x20:
		stream = XSP3_DMA_STREAM_BNUM_10G_TO_DRAM;
		break;
	case 0x40:
		stream = XSP3M_DMA_STREAM_BNUM_HIST_LIST;
		break;
	case 0x80:
		stream = XSP3M_DMA_STREAM_BNUM_HIST_FRAMES;
		break;
	default:
		sv_printf(cn, "! Please specify 1 and only 1 DMA stream\n");
		return -1;
	}


	rc = ngpd_get_dma_status_block(path, card, &statusBlock);
	if (rc < 0) {
		sv_printf(cn,"! ERROR %s\n", ngpd_get_error_message());
	}
	sv_printf(cn, "Stream %d statusblock:\n", stream);
	sv_printf(cn, "state          : 0x%x       = %d\n", statusBlock.stream_def[stream].state, statusBlock.stream_def[stream].state);
	sv_printf(cn, "base address   : 0x%08x = %d\n", statusBlock.stream_def[stream].base_addr, statusBlock.stream_def[stream].base_addr);
	sv_printf(cn, "is_tx          : 0x%x       = %d\n", statusBlock.stream_def[stream].is_tx, statusBlock.stream_def[stream].is_tx);
	sv_printf(cn, "desc           : 0x%08x = %d\n", statusBlock.stream_def[stream].desc, statusBlock.stream_def[stream].desc);
	sv_printf(cn, "num_desc       : 0x%08x = %d\n", statusBlock.stream_def[stream].num_desc, statusBlock.stream_def[stream].num_desc);
	sv_printf(cn, "data_start     : 0x%08x = %d\n", statusBlock.stream_def[stream].data_start, statusBlock.stream_def[stream].data_start);
	sv_printf(cn, "data_size      : 0x%08x = %d\n", statusBlock.stream_def[stream].data_size, statusBlock.stream_def[stream].data_size);
	sv_printf(cn, "max_block_bytes: 0x%08x = %d\n", statusBlock.stream_def[stream].max_block_bytes, statusBlock.stream_def[stream].max_block_bytes);
	sv_printf(cn, "defined_desc   : 0x%08x = %d\n", statusBlock.stream_def[stream].defined_desc, statusBlock.stream_def[stream].defined_desc);
	return rc;
}

int ngpd_read_time_stat_a(int quals, int cn, int path, int card, int repeat)
{
	int i;
	u_int32_t t;
	int rc;
	if (OptionalArgumentCount == 0)
		repeat = 1;

	for (i=0; i<repeat; i++)
	{
		if ((rc = ngpd_get_glob_time_statA(path, card, &t)) < 0)
		{
			sv_printf(cn, "! ERROR reading timing status A:%s\n", ngpd_get_error_message());
			return rc;
		}
		sv_printf(cn, "# Time status A=%08X\n", t);
	}
	return 0;
}

int ngpd_dma_check_desc(int quals, int cn, int path, int card)
{
	int rc = 0;
	u_int32_t stream;
	XSP3_DMA_MsgCheckDesc msg;
	u_int32_t completed=0, last=0, status=0;
	int circ = !!(quals & 0x100);

	switch (quals & 0xFF) {
	case 0x1:
		stream = XSP3_DMA_STREAM_BNUM_PLAYBACK;
		break;
	case 0x2:
		stream = XSP3_DMA_STREAM_BNUM_SCOPE0;
		break;
	case 0x4:
		stream = XSP3_DMA_STREAM_BNUM_SCOPE1;
		break;
	case 0x8:
		stream = XSP3_DMA_STREAM_BNUM_SCALERS;
		break;
	case 0x10:
		stream = XSP3_DMA_STREAM_BNUM_DRAM_TO_10G;
		break;
	case 0x20:
		stream = XSP3_DMA_STREAM_BNUM_10G_TO_DRAM;
		break;
	case 0x40:
		stream = XSP3M_DMA_STREAM_BNUM_HIST_LIST;
		break;
	case 0x80:
		stream = XSP3M_DMA_STREAM_BNUM_HIST_FRAMES;
		break;
	default:
		sv_printf(cn, "! Please specify 1 and only 1 DMA stream\n");
		return -1;
	}
	
	msg.first_desc = 0;
	msg.num_desc   = 0;
	msg.options    = 0;

	if (circ)
	{
		if (ngpd_get_generation(path, card) == XspressGen3)
		{
			sv_printf(cn, "! Circular option not supported on XSPRESS3\n");
			return -1;
		}
		int64 good64;
		u_int64_t comp64,  last64, desc_frame;
		u_int32_t desc_status;
		good64 = xsp3m_dma_check_desc(path, card, stream, &comp64, &last64, &status, &desc_status, &desc_frame);
		if (good64 < 0) 
		if (rc < 0)
		{
			sv_printf(cn,"! ERROR %s\n", ngpd_get_error_message());
			return rc;
		}
		sv_printf(cn, "# Good Descriptors=%ld, completed=%lu, last frame=%lu, dma status=0x%08X, desc_status=0x%08X, descriptor frame=%lu\n", good64, comp64, last64, status, desc_status, desc_frame);
	}
	else
	{
		rc = ngpd_dma_check_desc(path, card, stream, &msg, &completed, &last, &status);
		if (rc < 0) {
			sv_printf(cn,"! ERROR %s\n", ngpd_get_error_message());
		}
		if (ngpd_get_generation(path, card) != XspressGen3)
		{
			sv_printf(cn, "# Good descriptors = %d, total compeleted=%u, last_frame=%u, last_status=0x%08X\n", rc, completed, last, status);
		}
	}
	return rc;
}
int ngpd_read_chan_reg(int quals, int cn, int path, int chan, int offset, int num)
{
	int i, rc;
	int num_chan;
	num_chan = ngpd_get_num_chan(path);
	int first_chan, last_chan;
	int region=XSP3_REGION_REGS;
	u_int32_t buf[XSP3_MAX_NUM_READ_CHAN_REG];

	if (num_chan <= 0)
	{
		sv_printf(cn, "! ERROR: Cannot read num_chan from path %d, %s\n", path, ngpd_get_error_message());
		return -1;
	}
	if (chan < 0)
	{
		first_chan = 0; 
		last_chan  = num_chan-1;
	}
	else
	{
		if (chan >= num_chan)
		{
			sv_printf(cn, "! ERROR: Chan should be 0..%d, not %d\n", num_chan-1, chan);
			return -1;
		}
		first_chan = last_chan = chan;
	}
	sv_printf(cn, "# Num_chan=%d, first=%d, last=%d\n", num_chan, first_chan, last_chan);
	if (num <= 0)
		num = 1;

	if (quals & 1)
	{
		region = Qualifier[0].IntArg;
		if (region < 0 || region > XSP3_REGION_RAM_MAX)
		{
			sv_printf(cn, "! ERROR: Expected region in range 0...%d, not %d\n", XSP3_REGION_RAM_MAX, region);
			return -1;
		}
		if (num >XSP3_MAX_NUM_READ_CHAN_REG)
		{
			sv_printf(cn, "! ERROR: number of reads limited to %d, not %d\n", XSP3_MAX_NUM_READ_CHAN_REG, num);
			return -1;
		}
	}
	else if (offset <0 || offset >= XSP3_MAX_NUM_READ_CHAN_REG || offset+num >XSP3_MAX_NUM_READ_CHAN_REG)
	{
		sv_printf(cn, "ERROR in address offset range first=%d, num=%d, max=%d\n", offset, num, XSP3_MAX_NUM_READ_CHAN_REG-1) ;
		return -1;
	}
	sv_printf(cn, "# Chan ");
	for (i=0; i<num; i++)
		sv_printf(cn, " %8d", i+offset);
	sv_printf(cn, "\n");

	for (chan=first_chan; chan<=last_chan; chan++)
	{
		rc = ngpd_read_reg(path, chan, region, offset, num, buf);
		if (rc < 0)
		{
			sv_printf(cn, "! ERROR: Cannot read registers from offset=%d, num=%d, path %d, %s\n", offset, num, path, ngpd_get_error_message());
			return -1;
		}
		sv_printf(cn, "#   %d ", chan);
		for (i=0; i<num; i++)
			sv_printf(cn, " %08X", buf[i]);
		sv_printf(cn, "\n");
	}

	return 0;
}

int ngpd_mdio_set_conn(int quals, int cn, int path, int card, int val)
{
	int rc;

	rc = ngpd_mdio_set_connection(path, card, val);
	if (rc < 0)
		sv_printf(cn, "! ERROR: Cannot set mdio connection type: %s\n", ngpd_get_error_message());
	return rc;
}


double ngpd_read_mid_plane_temp(int quals, int cn, int path)
{
	int rc;
	double max_temp;
	float temp[3];
	int i;

	max_temp = -200;
	rc = ngpd_i2c_read_mid_plane_temp(path, temp);
	if (rc < 0)
	{
		sv_printf(cn, "ERROR reading temperatures: %s\n", ngpd_get_error_message());
		return rc;
	}
	for (i=0;i<3;i++)
	{
		if (temp[i] > max_temp)
			max_temp = temp[i];
	}

	sv_printf(cn, "# Mid-plane Temperatures :");
	for (i=0;i<3;i++)
		sv_printf(cn, "\t%g", temp[i]);
	sv_printf(cn, "\n");
	return max_temp;
}



int ngpd_read_fem_config(int quals, int cn, int path, int card)
{
	struct fem_config	femConfig;
	int rc;

	rc = ngpd_read_fem_config(path, card, 0, sizeof(struct fem_config), (u_int8_t *)&femConfig);
	if (rc < 0)
	{
		sv_printf(cn, "ERROR reading FEM Config: %s\n", ngpd_get_error_message());
		return rc;
	}
	sv_printf(cn, "# Header   = %04X\n", femConfig.header);
	sv_printf(cn, "# MAC Addr = %02X:%02X:%02X:%02X:%02X:%02X\n", femConfig.net_mac[0], femConfig.net_mac[1], femConfig.net_mac[2], femConfig.net_mac[3], femConfig.net_mac[4], femConfig.net_mac[5]);
	sv_printf(cn, "# IP  Addr = %d.%d.%d.%d\n", femConfig.net_ip[0], femConfig.net_ip[1], femConfig.net_ip[2], femConfig.net_ip[3]);
	sv_printf(cn, "# Net Mask = %d.%d.%d.%d\n", femConfig.net_nm[0], femConfig.net_nm[1], femConfig.net_nm[2], femConfig.net_nm[3]);
	sv_printf(cn, "# Gateway  = %d.%d.%d.%d\n", femConfig.net_gw[0], femConfig.net_gw[1], femConfig.net_gw[2], femConfig.net_gw[3]);
	sv_printf(cn, "# LM82 Temperature high set point (unused)       = %d\n", femConfig.temp_high_setpoint);
	sv_printf(cn, "# LM82 Temperature crytical set point (shutdown) = %d\n", femConfig.temp_crit_setpoint);
	return 0;
}

int ngpd_set_fem_config(int quals, int cn, int path, int card, int new_card)
{
	struct fem_config	femConfig;
	int rc;
	int force= quals & 1;


	rc = ngpd_read_fem_config(path, card, 0, sizeof(struct fem_config), (u_int8_t *)&femConfig);
	if (rc < 0)
	{
		sv_printf(cn, "ERROR reading FEM Config: %s\n", ngpd_get_error_message());
		return rc;
	}

	if (card < 0 || card > XSP3_MAX_CARD_INDEX)
		sv_printf(cn, "! ERROR: Expected new card to be in range 0 to %d, not %d\n", XSP3_MAX_CARD_INDEX, new_card);
	femConfig.net_mac[5] = new_card;
	femConfig.net_ip[3] = new_card+2;
	if (quals & 2)
	{
		int t_crit = Qualifier[1].IntArg;
		if (t_crit < -127 || t_crit> 127)
		{
			sv_printf(cn, "! ERROR: T_crit must be in range -127 to 127 and usually about 75 C, not %d\n", t_crit);
			return -1;
		}
		if (t_crit < 40 || t_crit > 100)
		{
			if (!force)
			{
				sv_printf(cn, "! ERROR: T_crit should be in range 40 to 100 and usually about 75 C not %d. Use 'force' to override\n", t_crit);
				return -1;
			}
			else
				sv_printf(cn, "# WARNING: T_crit should be in range 40 to 100 and usually about 75 C not %d.\n", t_crit);
		}
		femConfig.temp_crit_setpoint = t_crit;
	}
	sv_printf(cn, "# Header   = %04X\n", femConfig.header);
	sv_printf(cn, "# MAC Addr = %02X:%02X:%02X:%02X:%02X:%02X\n", femConfig.net_mac[0], femConfig.net_mac[1], femConfig.net_mac[2], femConfig.net_mac[3], femConfig.net_mac[4], femConfig.net_mac[5]);
	sv_printf(cn, "# IP  Addr = %d.%d.%d.%d\n", femConfig.net_ip[0], femConfig.net_ip[1], femConfig.net_ip[2], femConfig.net_ip[3]);
	sv_printf(cn, "# Net Mask = %d.%d.%d.%d\n", femConfig.net_nm[0], femConfig.net_nm[1], femConfig.net_nm[2], femConfig.net_nm[3]);
	sv_printf(cn, "# Gateway  = %d.%d.%d.%d\n", femConfig.net_gw[0], femConfig.net_gw[1], femConfig.net_gw[2], femConfig.net_gw[3]);
	sv_printf(cn, "# LM82 Temperature high set point (unused)       = %d\n", femConfig.temp_high_setpoint);
	sv_printf(cn, "# LM82 Temperature crytical set point (shutdown) = %d\n", femConfig.temp_crit_setpoint);

	rc = ngpd_write_fem_config(path, card, 0, sizeof(struct fem_config), (u_int8_t *)&femConfig);
	if (rc < 0)
	{
		sv_printf(cn, "ERROR writing FEM Config: %s\n", ngpd_get_error_message());
		return rc;
	}

	return 0;
}

int ngpd_get_revision(int quals, int cn, int path)
{
	int revision;
	int det, major, minor;
	if ((revision=ngpd_get_revision(path)) < 0)
	{
		sv_printf(cn, "! ERROR reading reviosn: %s\n", ngpd_get_error_message());
		return revision;
	}

	det = XSP3_REVISION_GET_DETECTOR(revision);
	major = XSP3_REVISION_GET_MAJOR(revision);
	minor = XSP3_REVISION_GET_MINOR(revision);

	sv_printf(cn, "# Revision = 0x%08X => Det=%d, Major=%d, Minor=%d\n", revision, det, major, minor);
	return revision;
}

int ngpd_get_features(int quals, int cn, int path, int card)
{
	Xspress3_features features;

	int rc;

	if ((rc=ngpd_get_features(path, card, &features)) < 0)
	{
		sv_printf(cn, "! ERROR: Cannot read features: %s\n", ngpd_get_error_message());
		return rc;
	}
	sv_printf(cn, "# Test data Source = %d => %s and %s \n", features.test_data_source , ngpd_feature_test_data_source_a[features.test_data_source & 3], ngpd_feature_test_data_source_b[features.test_data_source>>2]);
	sv_printf(cn, "# Real data Source = %d => %s\n", features.real_data_source , ngpd_feature_real_data_source[features.real_data_source]);
	sv_printf(cn, "# Detector Type    = %d => %s\n", features.data_mux , ngpd_feature_data_mux3[features.data_mux >> 3]);
	sv_printf(cn, "# Data Mux         = %d => %s\n", features.data_mux , ngpd_feature_data_mux20_pr[features.data_mux & 7]);
	sv_printf(cn, "# INL Correction   = %d => %s and %s\n", features.inl_corr , ngpd_feature_inl_corr_a[features.inl_corr& 3], ngpd_feature_inl_corr_b[features.inl_corr>>2]);
	sv_printf(cn, "# Reset Detector   = %d => %s\n", features.reset_detector , ngpd_feature_reset_detector[features.reset_detector]);
	sv_printf(cn, "# Reset Correction = %d => %s\n", features.reset_corr , ngpd_feature_reset_corr[features.reset_corr]);
	sv_printf(cn, "# Glitch Detector  = %d => %s %s\n", features.glitch_detect , ngpd_feature_glitch_detect02[features.glitch_detect & 7], ngpd_feature_glitch_detect3[features.glitch_detect/8]);
	sv_printf(cn, "# Glitch Pad       = %d => %s\n", features.glitch_pad , ngpd_feature_glitch_pad[features.glitch_pad]);
	sv_printf(cn, "# Trigger B        = %d => %s, %s\n", features.trigger_b , ngpd_feature_trigger_b_l[features.trigger_b&3], ngpd_feature_trigger_b_m[features.trigger_b>>2]);
	sv_printf(cn, "# Trigger C        = %d => %s\n", features.trigger_c , ngpd_feature_trigger_c[features.trigger_c]);
	sv_printf(cn, "# Trigger Extra    = %d => %s, %s, %s, %s\n", features.trigger_extra, ngpd_feature_trigger_extra0[features.trigger_extra&1], ngpd_feature_trigger_extra1[(features.trigger_extra>>1)&1],
																						 ngpd_feature_trigger_extra2[(features.trigger_extra>>2)&1], ngpd_feature_trigger_extra3[(features.trigger_extra>>3)&1]);
	sv_printf(cn, "# Calibrator       = %d => %s\n", features.calibrator , ngpd_feature_calibrator[features.calibrator]);
	sv_printf(cn, "# Neighbour Events = %d => %s, %s\n", features.neighbour_events , ngpd_feature_neighbour_events0[features.neighbour_events&1], ngpd_feature_neighbour_events32[features.neighbour_events>>2]);
	sv_printf(cn, "# Servo Base       = %d => %s\n", features.servo_base , ngpd_feature_servo_base[features.servo_base]);
//	sv_printf(cn, "# Servo Details    = %d => %s\n", features.servo_details , ngpd_feature_[features.]);
	sv_printf(cn, "# Run Ave          = %d => %s, %s\n", features.run_ave , ngpd_feature_run_ave1[(features.run_ave>>1)&1], ngpd_feature_run_ave3[features.run_ave>>3]);
	sv_printf(cn, "# Lead Tail Correct= %d => %s and %s\n", features.lead_tail_corr, ngpd_feature_lead_tail0[features.lead_tail_corr & 1], ngpd_feature_lead_tail12[(features.lead_tail_corr >> 1)& 3]);
	sv_printf(cn, "# Output Format    = %d => %s\n", features.output_format , ngpd_feature_output_format[features.output_format]);
	sv_printf(cn, "# Format Details A = %d => %s and %s\n", features.format_details_a , ngpd_feature_format_details_a01[features.format_details_a&3], ngpd_feature_format_details_a23[features.format_details_a>>2]);
//	sv_printf(cn, "# Format Details B = %d => %s\n", features.format_details_b , ngpd_feature_[features.]);
	sv_printf(cn, "# Global Reset     = %d => %s\n", features.global_reset , ngpd_feature_global_reset[features.global_reset]);
//	sv_printf(cn, "# Timing Source    = %d => %s\n", features.timing_source , ngpd_feature_[features.]);
	sv_printf(cn, "# Timing Generator = %d => %s\n", features.timing_generator , ngpd_feature_timing_generator[features.timing_generator]);
	sv_printf(cn, "# Scope Mode       = %d => %s and %s\n", features.scope_mode , ngpd_feature_scope_mode210[features.scope_mode&7], ngpd_feature_scope_mode3[features.scope_mode>>3] );

	sv_printf(cn, "# Farm Mode    = %d\n", features.farm_mode);
	sv_printf(cn, "# Soft Scalers = %d\n", features.soft_scalers);

	return 0;
}

int ngpd_set_inter_packet_gap(int quals, int cn, int path, int card, int gap)
{
	u_int32_t reg13 = (u_int32_t) gap;
	int rc;
	if ((rc = ngpd_write_rdma_reg(path, card, 13, 1, &reg13)) != 0) 
	{
		sv_printf(cn, "! ERROR setting interpacket gap: %s\n", ngpd_get_error_message());
	}
	return rc;
}

int ngpd_set_b2b_loopback(int quals, int cn, int path, int card)
{
	int b2b_mask=0;
	int loopback_type=0;
	u_int32_t loop_cont;
	int i, rc;

	b2b_mask = quals & 0x3F;

	switch ((quals>>6)&0xF)
	{
	case 0:	loopback_type = XSP4_AURORA_CONT_LOOPBACK_NONE; break;
	case 1: loopback_type = XSP4_AURORA_CONT_LOOPBACK_NEAR_PCS; break;
	case 2: loopback_type = XSP4_AURORA_CONT_LOOPBACK_NEAR_PMA; break;
	case 4: loopback_type = XSP4_AURORA_CONT_LOOPBACK_FAR_PMA; break;
	case 8: loopback_type = XSP4_AURORA_CONT_LOOPBACK_FAR_PCS; break;
	default:
		sv_printf(cn, "! ERROR: Please specify at most 1 loop back mode\n");
		return -1;
	}

	loop_cont = 0;
	for (i=0; i<6; i++)
	{
		if ((b2b_mask & 1 << i))
		{
			loop_cont |= XSP4_AURORA_CONT_SET_LOOPBACK(i,loopback_type);
			if (quals & (1<<10))
				loop_cont |= XSP4_AURORA_CONT_SET_RXCDROVRDEN(i, 1);
		}
	}
	if ((rc=xsp4_set_aurora_cont(path, card, loop_cont))<0)
	{
		sv_printf(cn, "! ERROR writing loopback control register '%s'\n", ngpd_get_error_message());
		return -1;
	}

	if ((rc=xsp4_set_aurora_cont(path, card, loop_cont|XSP4_AURORA_CONT_RESET))<0)
	{
		sv_printf(cn, "! ERROR writing loopback control register '%s'\n", ngpd_get_error_message());
		return rc;
	}
	sleep (2);
	return 0;
}

int ngpd_set_b2b_ecc(int quals, int cn, int path, int card, int disable)
{
	u_int32_t aurora_cont=0;
	int rc;

	if (disable)
		aurora_cont = XSP4_AURORA_CONT_DISABLE_ECC;

	if ((rc=xsp4_set_aurora_cont(path, card, aurora_cont))<0)
	{
		sv_printf(cn, "! ERROR writing aurora_cont control register '%s'\n", ngpd_get_error_message());
		return rc;
	}
	return 0;
}

int ngpd_show_b2b_status(int quals, int cn, int path, int card)
{
	int num_cards = ngpd_get_num_cards(path);
	int first, last;	
	u_int16_t status[6];
	int rc, i;

	if (num_cards < 0)
	{
		sv_printf(cn, "! ERROR reading number of cards :'%s'\n", ngpd_get_error_message());
		return num_cards;
	}
	if (card < 0)
	{
		first = 0;
		last = num_cards-1;
	}
	else
	{
		first = last= card;
	}

	for (card=first; card<=last; card++)
	{
		if ((rc=xsp4_get_aurora_status(path, card, status)) < 0)
		{
			sv_printf(cn, "! ERROR reading aurora status :'%s'\n", ngpd_get_error_message());
			return rc;
		}
		for (i=0; i<6; i++)
		{
			sv_printf(cn, "# Card %d, Link %d: %-26s : Status=%02X => ", card, i, xsp4_aurora_names[i], status[i]);
			if (status[i] & XSP4_AUROA_STAT_HARD_ERR) sv_printf(cn, " Hard error"); 
			if (status[i] & XSP4_AUROA_STAT_SOFT_ERR) sv_printf(cn, " Soft Error"); 
			if (status[i] & XSP4_AUROA_STAT_CHANNEL_UP) sv_printf(cn, " Channel up"); 
			if (status[i] & XSP4_AUROA_STAT_LANE_UP) sv_printf(cn, " Lane up"); 
			if (status[i] & XSP4_AUROA_STAT_LINK_RESET) sv_printf(cn, " Link reset"); 
			if (status[i] & XSP4_AUROA_STAT_GT_PLL_LOCK) sv_printf(cn, " CPLL lock"); 
			sv_printf(cn, "\n");
		}
	}
	return 0;
}

int ngpd_init_xtk_map(int quals, int cn, int path, int chan, int b2b_stream)
{
	int rc;
	if ((rc=ngpd_bram_init_xtk(path, chan, b2b_stream, quals & 1)) < 0)
	{
		sv_printf(cn, "! ERROR setting xtk map BRAM '%s'\n", ngpd_get_error_message());
		return rc;
	}
	return 0;
}
	
int ngpd_print_lm82_regs(int quals, int cn, int path, int card)
{
	return ngpd_i2c_print_lm82_regs(path, card);
}

int ngpd_read_adc_clk(int quals, int cn, int path, int card)
{
	int rc;
	int first, last, num_cards;
	u_int32_t status_regs[2];
	u_int32_t mmcm_regs[23];
	u_int32_t drp_regs[23];
	int i;

	num_cards = ngpd_get_num_cards(path);
	if (num_cards < 0)
	{
		sv_printf(cn, "! ERROR reading number of cards: %s\n", ngpd_get_error_message());
		return -1;
	}

	if (card < 0)
	{
		first = 0;
		last= num_cards-1;
	}
	else
	{
		first = card;
		last = card;
	}
	for (card=first; card <=last; card++)
	{		
		if ((rc=ngpd_read_dma_or_clk_reg(path, card, XSP3_DMAS_ADC_CLK_BASE+XSP3_V7_MMCM_SR, 2, status_regs)) < 0)
		{
			sv_printf(cn, "! ERROR reading From MMCM registers : %s\n", ngpd_get_error_message());
			return -1;
		}
		if ((rc=ngpd_read_dma_or_clk_reg(path, card, XSP3_DMAS_ADC_CLK_BASE+XSP3_V7_MMCM_CCR0_DIVIDE, 23, mmcm_regs)) < 0)
		{
			sv_printf(cn, "! ERROR reading From MMCM registers : %s\n", ngpd_get_error_message());
			return -1;
		}
		if ((rc=ngpd_read_dma_or_clk_reg(path, card, XSP3_DMAS_ADC_CLK_BASE+XSP3_V7_MMCM_DRP_BASE, 23, drp_regs)) < 0)
		{
			sv_printf(cn, "! ERROR reading From MMCM registers : %s\n", ngpd_get_error_message());
			return -1;
		}
		sv_printf(cn, "# Card %d : Status   = %d = 0x%08X\n", card, status_regs[0], status_regs[0]);
		sv_printf(cn, "# Card %d : Clk Mon  = %d = 0x%08X\n", card, status_regs[1], status_regs[1]);
		for (i=0;i<23; i++)
			sv_printf(cn, "# Card %d : CCR %2d = %6d = 0x%08X, DRP %02d = %6d = 0x%08X\n", card, i, mmcm_regs[i], mmcm_regs[i], i, drp_regs[i], drp_regs[i]);

	}
	return 0;
}

int ngpd_set_adc_clk_phase(int quals, int cn, int path, int card, int divide, double phase, double phase_del, double phase_radial)
{
	int rc;

	if (phase < -360 || phase > 360)
	{
		sv_printf(cn, " ! ERROR: Expected phase in range -360 to 360, not %g\n", phase);
		return -1;
	}
	if (phase_del < -360 || phase_del > 360)
	{
		sv_printf(cn, " ! ERROR: Expected phase in range -360 to 360, not %g\n", phase_del);
		return -1;
	}
	if (phase_radial < -360 || phase_radial > 360)
	{
		sv_printf(cn, " ! ERROR: Expected phase in range -360 to 360, not %g\n", phase_radial);
		return -1;
	}

	if (phase_del <= phase) 
		sv_printf(cn, "# WARNING: phase of delayed ADC clock %g <= phase of main ADC Clk, so questionable Hold in FPGA\n", phase_del, phase);
	
	if (phase_radial <= phase) 
		sv_printf(cn, "# WARNING: phase of ADC clock for radial %g <= phase of main ADC Clk, so questionable Hold in FPGA\n", phase_del, phase);

	if ((rc = ngpd_adc_mmcm_setup(path, card, divide, phase, phase_del, phase_radial)) < 0)
	{
		sv_printf(cn, "! ERROR: Cannot setup ADC MMCM : '%s'\n", ngpd_get_error_message());
		return -1;
	}
	return 0;
}

int ngpd_measure_clock_frequency(int quals, int cn, int path, int card)
{
	int rc;
	int all = quals & 1;
	int first, last, num_cards;

	if (all)
	{
		int first, last, num_cards;
		u_int32_t raw[XSP4_NUM_FREQ_MEASURERS*XSP3_MAX_CARDS], *rp;
		int freq[XSP4_NUM_FREQ_MEASURERS*XSP3_MAX_CARDS], *fp;
		int i;

		num_cards = ngpd_get_num_cards(path);
		if (num_cards < 0)
		{
			sv_printf(cn, "! ERROR: %s\n", ngpd_get_error_message());
			return num_cards;
		}
		if (card < 0)
		{
			first = 0;
			last = num_cards-1;
		}
		else if (card >= num_cards)
		{
			sv_printf(cn, "! ERROR: Expected card in range 0..%d for single card or -1 to read all\n", num_cards);
			return -1;
		}
		else
		{
			first = card;
			last  = card;
		}
		rc = ngpd_measure_clock_frequency_all(path, card, XSP4_NUM_FREQ_MEASURERS*(1+last-first), raw, freq );
		if (rc < 0)
		{
			sv_printf(cn, "! ERROR: %s\n", ngpd_get_error_message());
			return rc;
		}
		sv_printf(cn,"# Card      ADC (MHz)  10G UDP    MGT113     MGT114\n");
		rp = raw;
		fp = freq;
		for (card=first; card<=last; card++)
		{
			sv_printf(cn, "#    %d   ", card);
			for (i=0;  i<XSP4_NUM_FREQ_MEASURERS; i++)
			{
				if (!(*rp & XSP4_ADC_CLOCK_FREQ_TIMER_READY))
					sv_printf(cn, " BusClkErr ");
				else if (!(*rp & XSP4_ADC_CLOCK_FREQ_ADC_VALID))
					sv_printf(cn, " Not Ready ");
				else
					sv_printf(cn, " %10.4F", *fp/1000000.0);
				rp++;
				fp++;
			}
			sv_printf(cn, "\n");
		}
		return rc;
	}

	rc = ngpd_measure_clock_frequency(path, card);
	
	if (rc < 0)
	{
		sv_printf(cn, "! ERROR: %s\n", ngpd_get_error_message());
		return rc;
	}
	
	return rc;
}
double ngpd_get_clock_period(int quals, int cn, int path, int card)
{
	int first, last, num_cards;
	double period=-1.0;
	if ((num_cards = ngpd_get_num_cards(path)) < 0)
	{
		sv_printf(cn, "! ERROR: Cannot read number of cards: %s\n", ngpd_get_error_message());
		return num_cards;
	}
	if (card < 0)
	{
		first = 0; 
		last  = num_cards-1;
	}
	else
	{
		first = card;
		last  = card;
	}
	for (card=first; card<=last; card++)
	{
		period = ngpd_get_clock_period(path, card);
		sv_printf(cn, "# Card %d: Period = %g s = %g ns\n", card, period, period*1E9);
	}
	return period;
}

int ngpd_set_udp_buffer_size(int quals, int cn, int path, int card, int hist_ports, int scope_ports)
{
	int rc;
	rc = ngpd_set_udp_buffer_size(path, card, hist_ports, scope_ports);
	if (rc < 0)
		sv_printf(cn, "! ERROR: Error setting buffer sizes: %s\n", ngpd_get_error_message());
	return rc;
}

int ngpd_get_udp_buffer_size(int quals, int cn, int path, int card)
{
	int first, last, num_cards;
	int scope_size, hist_size[XSP3_MAX_CHANS_PER_CARD];
	int rc, i;
	
	num_cards = ngpd_get_num_cards(path);
	if (num_cards < 0)
	{
		sv_printf(cn, "! ERROR : getting number of cards: %s\n", ngpd_get_error_message());
		return num_cards;
	}
		
	if (card < 0)
	{
		first = 0;
		last = num_cards-1;
	}
	else
	{
		first = card;
		last = card;
	}
	for (card=first; card <= last; card++)
	{
		rc = ngpd_get_udp_buffer_size(path, card, hist_size, &scope_size);
		if (rc < 0)
			sv_printf(cn, "! ERROR: Error setting buffer sizes: %s\n", ngpd_get_error_message());
	
		sv_printf(cn, "# Card %d: Scope=%d, Hist ", card, scope_size);
		for (i=0; i<rc; i++)
			sv_printf(cn, " C%d=%d", i, hist_size[i]);
		sv_printf(cn, "\n");
	}
	return hist_size[0];
}

int ngpd_dma_read_ethernet_status(int quals, int cn, int path, int card)
{
	int first, last, num_cards;
	int rc;
	int reset_counts=quals & 1;

	if ((num_cards=ngpd_get_num_cards(path)) < 0)
	{
		sv_printf(cn, "! ERROR reading number of cards : %s\n", ngpd_get_error_message());
		return num_cards;
	}

	if (card < 0)
	{
		first = 0;
		last= num_cards-1;
	}
	else
	{
		first = card;
		last= card;
	}

	for (card=first; card <= last; card++)
	{
		u_int32_t mon_reg[6];
		if ((rc=ngpd_read_rdma_reg(path, card, 0x18, 6, mon_reg)) < 0)
		{
			sv_printf(cn, " ! ERROR reading rdma monitor registers : %s\n", ngpd_get_error_message());
			return rc;
		}
		sv_printf(cn, "# Card %d: mon_reg_0=0x%08X, TX XGMII SOF=%d, TX XGMII EOF=%d\n", card, mon_reg[0], mon_reg[4], mon_reg[5]);
		sv_printf(cn, "# ....... ResetDone      = %d\n", !!(mon_reg[0] & 0x200));
		sv_printf(cn, "# ....... SignalDetect   = %d\n", !!(mon_reg[0] & 0x100));

/*		sv_printf(cn, "# ....... Auto Neg Link Up  = %d\n", !!(mon_reg[0] & 0x20 ));  Don;t exist as this is not 10GBase-kr
		sv_printf(cn, "# ....... Auto Neg Enable   = %d\n", !!(mon_reg[0] & 0x10));
		sv_printf(cn, "# ....... PMD Signal Detect (training done) = %d\n", !!(mon_reg[0] &));
*/
		sv_printf(cn, "# ....... PCS Block Lock = %d\n", !!(mon_reg[0] & 1));
		if (reset_counts)
		{
			u_int32_t reg15;
			if ((rc=ngpd_read_rdma_reg(path, card, 0xF, 1, &reg15)) < 0)
			{
				sv_printf(cn, " ! ERROR reading rdma register 15 : %s\n", ngpd_get_error_message());
				return rc;
			}
			reg15 |= 0x80000000;
			if ((rc=ngpd_write_rdma_reg(path, card, 0xF, 1, &reg15)) < 0)
			{
				sv_printf(cn, " ! ERROR writing rdma register 15 : %s\n", ngpd_get_error_message());
				return rc;
			}
			reg15 &= ~0x80000000;
			if ((rc=ngpd_write_rdma_reg(path, card, 0xF, 1, &reg15)) < 0)
			{
				sv_printf(cn, " ! ERROR writing rdma register 15 : %s\n", ngpd_get_error_message());
				return rc;
			}
		}
	}
	return 0;
}

int ngpd_dma_get_desc_status(int quals, int cn, int path, int card, int first, int num)
{
	int rc = 0;
	u_int32_t stream;
	XSP3_DMA_StatusBlock statusBlock;
	XSP3_DMA_MsgGetDescStatus msg;
	int i;
	u_int32_t status;
	int total =0;

	switch (quals & 0x3F) {
	case 0x1:
		stream = XSP3_DMA_STREAM_BNUM_PLAYBACK;
		break;
	case 0x2:
		stream = XSP3_DMA_STREAM_BNUM_SCOPE0;
		break;
	case 0x4:
		stream = XSP3_DMA_STREAM_BNUM_SCOPE1;
		break;
	case 0x8:
		stream = XSP3_DMA_STREAM_BNUM_SCALERS;
		break;
	case 0x10:
		stream = XSP3_DMA_STREAM_BNUM_DRAM_TO_10G;
		break;
	case 0x20:
		stream = XSP3_DMA_STREAM_BNUM_10G_TO_DRAM;
		break;
	default:
		sv_printf(cn, "! Please specify 1 and only 1 DMA stream\n");
		return -1;
	}
	
	if (ngpd_get_generation(path, card) == XspressGen3)
	{
		sv_printf(cn, "! ERROR: Not suported on XSPRESS3\n");
		return -1;
	}
	rc = ngpd_get_dma_status_block(path, card, &statusBlock);
	if (rc < 0) {
		sv_printf(cn,"! ERROR %s\n", ngpd_get_error_message());
	}
	if (num <= 0)
		num = statusBlock.stream_def[stream].num_desc-first;

	for (i=0; i<num; i++)
	{
		msg.first_desc = first+i;
		rc = ngpd_dma_get_desc_status(path, card, stream, &msg, &status);
		if (rc < 0)
		{
			sv_printf(cn, "# ERROR reading status: %s : %d\n", ngpd_get_error_message(), rc);
			return rc;
		}
		total += status & 0x7FFFFF;
		sv_printf(cn, "# %d : Status=%08X, bytes=%d, total=%d\n", i, status, status&0x7FFFFF, total);
	}
	return 0;
}

int ngpd_adc_mmcm_reset(int quals, int cn, int path, int card)
{
	int rc;
	int reload= quals & 1;

	if (reload == 0)
	{
		if ((rc=ngpd_adc_mmcm_reset(path, card)) < 0)
		{
			sv_printf(cn, "! ERROR reseting ADC MMCM '%s' : %d\n", ngpd_get_error_message(), rc);
		}
	}
	else
	{
		if ((rc=ngpd_adc_mmcm_reload(path, card)) < 0)
		{
			sv_printf(cn, "! ERROR reloading ADC MMCM '%s' : %d\n", ngpd_get_error_message(), rc);
		}
	}
	return rc;
}
#endif


void * ngpd_get_fpga_voltages(int quals, int cn, int path, int card)
{
	int rc;
	int pnum;
	u_int32_t ams_data[NGZMP_ParamMax];
	NGPDGeneration generation;
	DictReturnType *dict;
	
	generation = ngpd_get_generation(path);
	if (generation == NGPDGenError)
	{
		sv_printf(cn, "! ERROR: ngpd_get_generation returns error : %s\n", ngpd_get_error_message());
		return NULL;
	}
	if (generation == NGPDGenADQ14)
	{
		sv_printf(cn, "ngpd debug get-fpga-voltages not supportd on ADQ14 (yet?)\n");
		return NULL;
	}

	rc = ngzmp_read_xadc(path, card, 0, NGZMP_ParamMax, ams_data);
	if (rc < 0)
	{
		sv_printf(cn, "ERROR reading system monitor data: %s\n", ngpd_get_error_message());
		return NULL;
	}
	if ((dict=(DictReturnType *)malloc(sizeof(DictReturnType)*(NGZMP_ParamMax+1))) == NULL)
	{
		sv_printf(cn, "! ERROR : Out of memory\n");
		return NULL;
	}
	
	for (pnum=0; pnum<NGZMP_ParamMax; pnum++)
	{
		dict[pnum].type ='g';
		dict[pnum].label = strdup(ngzmp_sys_mon_name[pnum]);
		dict[pnum].DoubleVal = ams_data[pnum]*0.001;
	}
	dict[pnum].type = 0;

	return (void *) dict;
}

void * ngpd_get_adc_voltages(int quals, int cn, int path, int card)
{
	int rc;
	int i, rail;
	int32_t v[NGZMP_UCD90160_NUM_CHIPS][NGZMP_UCD90160_NUM_RAILS];
	NGPDGeneration generation;
	DictReturnType *dict;
	int chip;
	
	generation = ngpd_get_generation(path);
	if (generation == NGPDGenError)
	{
		sv_printf(cn, "! ERROR: ngpd_get_generation returns error : %s\n", ngpd_get_error_message());
		return NULL;
	}
	if (generation == NGPDGenADQ14)
	{
		sv_printf(cn, "ngpd debug get-fpga-voltages not supportd on ADQ14 (yet?)\n");
		return NULL;
	}
	for (chip=0; chip<NGZMP_UCD90160_NUM_CHIPS; chip++)
	{
		rc = ngzmp_i2c_read_ucd90160_vout(path, card, chip, 0, NGZMP_UCD90160_NUM_RAILS, v[chip]);
		if (rc < 0)
		{
			sv_printf(cn, "ERROR reading UCD90160 monitor data: %s\n", ngpd_get_error_message());
			return NULL;
		}
	}
	if ((dict=(DictReturnType *)malloc(sizeof(DictReturnType)*(NGZMP_UCD90160_NUM_RAILS*NGZMP_UCD90160_NUM_CHIPS+1))) == NULL)
	{
		sv_printf(cn, "! ERROR : Out of memory\n");
		return NULL;
	}

	i = 0;
	for (chip=0; chip<NGZMP_UCD90160_NUM_CHIPS; chip++)
	{
		for (rail=0; rail<NGZMP_UCD90160_NUM_RAILS; rail++)
		{
			if (ngpd_ucd90160_signals[chip][rail] != NULL)
			{
				dict[i].type ='g';
				dict[i].label = strdup(ngpd_ucd90160_signals_python[chip][rail]);
				dict[i].DoubleVal = v[chip][rail]*0.0001;
				i++;
			}
		}
	}
	dict[i].type = 0;

	return (void *) dict;
}

void * ngpd_get_preamp_temp(int quals, int cn, int path, int card)
{
	int rc;
	float temp[4];
	int status[4];
	int i, j;
	DictReturnType *dict;
	char label[20];
	
	rc = ngpd_i2c_read_preamp_temp(path, card, temp, status);
	if (rc < 0)
	{
		sv_printf(cn, "ERROR reading temperatures: %s\n", ngpd_get_error_message());
		return NULL;
	}
	if ((dict=(DictReturnType *)malloc(sizeof(DictReturnType)*(NGZMP_I2C_NUM_ADT7410_PREAMP*4+1))) == NULL)
	{
		sv_printf(cn, "! ERROR : Out of memory\n");
		return NULL;
	}
	j=0;
	for (i=0;i<NGZMP_I2C_NUM_ADT7410_PREAMP;i++)
	{
		dict[j].type ='g';
		sprintf(label, "temp%d", i);
		dict[j].label = strdup(label);
		dict[j].DoubleVal = temp[i];
		j++;
		if (status[i] & ADT7410_STATUS_THIGH)
		{
			dict[j].type ='q';
			sprintf(label, "Thigh%d", i);
			dict[j].label = strdup(label);
			dict[j].StringVal = strdup("True");
			j++;
		}
		if (status[i] & ADT7410_STATUS_TCRIT)
		{
			dict[j].type ='q';
			sprintf(label, "Tcrit%d", i);
			dict[j].label = strdup(label);
			dict[j].StringVal = strdup("True");
			j++;
		}
	}
	dict[j].type = 0;
	return (void *)dict;
}

void * ngpd_get_adc_temp(int quals, int cn, int path, int card)
{
	int rc;
	float temp[4];
	int status[4];
	int i, j;
	DictReturnType *dict;
	char label[20];
	
	rc = ngpd_i2c_read_adc_temp(path, card, temp, status);
	if (rc < 0)
	{
		sv_printf(cn, "ERROR reading temperatures: %s\n", ngpd_get_error_message());
		return NULL;
	}
	if ((dict=(DictReturnType *)malloc(sizeof(DictReturnType)*(NGZMP_I2C_NUM_ADT7410_ADC*4+1))) == NULL)
	{
		sv_printf(cn, "! ERROR : Out of memory\n");
		return NULL;
	}
	j=0;
	for (i=0;i<NGZMP_I2C_NUM_ADT7410_ADC;i++)
	{
		dict[j].type ='g';
		sprintf(label, "temp%d", i);
		dict[j].label = strdup(label);
		dict[j].DoubleVal = temp[i];
		j++;
		if (status[i] & ADT7410_STATUS_THIGH)
		{
			dict[j].type ='q';
			sprintf(label, "Thigh%d", i);
			dict[j].label = strdup(label);
			dict[j].StringVal = strdup("True");
			j++;
		}
		if (status[i] & ADT7410_STATUS_TCRIT)
		{
			dict[j].type ='q';
			sprintf(label, "Tcrit%d", i);
			dict[j].label = strdup(label);
			dict[j].StringVal = strdup("True");
			j++;
		}
	}
	dict[j].type = 0;
	return (void *)dict;
}
