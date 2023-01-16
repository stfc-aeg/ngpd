/*----------------------------------------------------------------------
 *	Neutron Gamma Pulse Discriminator Device for DA with multi-client server.
 *	By William Helsby
 *----------------------------------------------------------------------
 */

#include "header.h"
#include <ngpd.h>



/*----- Private global variables -----*/
/*----- global variables -----*/
/*----- Private function prototypes -----*/

int ngpd_config_adq14_cmd(int quals, int cn, unsigned int adq_num);
int ngpd_config_ngzmp_cmd(int quals, int cn, int num_card);
int ngpd_get_chans_per_card_cmd(int quals, int cn, int path);
int ngpd_set_debug_cmd(int quals, int cn, int path, int level);
int ngpd_set_run_mode_cmd(int quals, int cn, int path);
int ngpd_set_chan_cont_cmd(int quals, int cn, int path, int chan);
int ngpd_setup_diff_trigger_cmd(int quals, int cn, int path, int chan, int thresh, int sep, int data_delay, int trig_delay, int delay_a, int width_a, int delay_b, int width_b);
int ngpd_read_scope_data_cmd(int quals, int cn, int path, int card);
int ngpd_start_cmd(int quals, int cn, int path, double col_time);
int ngpd_set_sync_mode(int quals, int cn, int path);
int ngpd_user_sync_mode(int quals, int cn, int path, int card);
int ngpd_setup_baseline_subtract(int quals, int cn, int path, int chan);
int ngpd_setup_measure(int quals, int cn, int path, int chan, int tail_delay, int tail_sum, double fall_frac);
int ngpd_hist_setup_cmd(int quals, int cn, int path);
int ngpd_save_diff_trigger(int quals, int cn, int path, char *fname);
int ngpd_setup_neutron_discrimination(int quals, int cn, int path, int chan, int min_height, int max_height);
int ngpd_load_tail_subtract_cmd(int quals, int cn, int path, int chan);
int ngpd_list_tail_subtract(int quals, int cn, int path, int chan);
int ngpd_set_ext_trigger_thres_cmd(int quals, int cn, int path, int card, double trig_thres);
int ngpd_write_dae_pulse_command(int quals, int cn, int path, int chan, int stretch);
int ngpd_hist_setup_ngzmp(int quals, int cn, int path);
int ngpd_wait_itfg(int quals, int cn, int path);

void * ngpd_get_diff_trigger(int quals, int cn, int path, int chan);
void * ngpd_get_baseline_subtract(int quals, int cn, int path, int chan);
void * ngpd_get_measure(int quals, int cn, int path, int chan);

//void to_bytes(char *ipName, unsigned char* b, int n);

/*----- Command tree under 'xspress3' -----*/
extern PARSE_TREE ngpd_debug_tree[];
extern PARSE_TREE ngpd_playback_tree[];
extern PARSE_TREE ngpd_scope_tree[];
extern PARSE_TREE ngpd_calib_tree[];
extern PARSE_TREE ngpd_trigger_b_tree[];
extern PARSE_TREE ngpd_trigger_c_tree[];
extern PARSE_TREE ngpd_histogram_tree[];
extern PARSE_TREE ngpd_resgrade_tree[];
extern PARSE_TREE ngpd_dtc_tree[];
extern PARSE_TREE ngpd_fan_tree[];
extern PARSE_TREE ngpd_itfg_tree[];
extern PARSE_TREE ngpd_adc_board_tree[];
extern PARSE_TREE ngpd_net_opt_tree[];
extern PARSE_TREE ngpd_cshare_tree[];
extern PARSE_TREE ngpd_filter_tree[];
extern PARSE_TREE ngpd_clock_tree[];

PARSE_TREE ngpd_tree[] = {
	PT_COMMAND ("config-adq14", "+ %d debug verbose = %d",
			    "Config to use Neutron Gamm Pulse Discriminator system based on ADQ14",
			    "ADQ Number (default 1)/Enable debug messages/Enable Verbose debugging messages",
			    ngpd_config_adq14_cmd),
	PT_COMMAND ("config-ngzmp", "%d debug verbose first-card %d base-ip %s port %d base-mac %d nchan %d no-init dummy dummy-adc dummy-preamp = %d",
			    "Config to use Neutron Gamma Pulse Discriminator system based on ZynqMP",
			    "Number of cards/Enable debug messages/Enable Verbose debugging messages/First Card index (default 0)/"
			    "base ip address (default 192.168.0.0)/Specify ZynqMP Port, default 30124)/Base MAC address for 10 G UDP versions/"
			    "Limit number of channels (default use all available)/Do not initialise hardware/Make a dummy system (no hardware)/Dummy system with FPGA on dev card/Real system but no pre-amp",
			    ngpd_config_ngzmp_cmd),
	PT_COMMAND ("get-chans-per-card", "%d = %d",
				"Get number of channels per card",
				"Path",
				ngpd_get_chans_per_card_cmd ),
	PT_COMMAND ("set-debug", "%d %d = %d",
			    "Override the debug level set in config ",
			    "Path/Debug level 0..2/",
			    ngpd_set_debug_cmd),
	PT_COMMAND ("set-run-mode", "%d playback playback-oneshot scope-mode scope-output-1s = %d",
			    "Setup overall run mode options",
			    "Path/Enable Playback Continuous mode/Enabel Playback one-shot mode/Enable Scope Mode/Force scope mode output to use single stream only",
			    ngpd_set_run_mode_cmd),
	PT_COMMAND ("set-chan-cont", "%d %d adc-fwd adc-rev pb inv-data = %d",
				"Set data source etc in channel  control register (ZynqMP version)",
				"Path/Channel (-1 for all)/Data from ADC with forward channel numbering (default/Data fro, ADC with channel numbering reversed/Data from playback/"
				"Invert (1's complement) the data",
				ngpd_set_chan_cont_cmd),
	PT_COMMAND ("setup-diff-trigger", "%d %d %d %d %d %d %d %d %d %d = %d",
				"Setup differential Trigger",
				"Path/Channel number (-1 for all)/Arm threshold (diff1)/diff1 separation (samples)/Data Delay (samples)/Trigger Delay (samples)/"
				"Stretched trigger A delay (2*samples)/Stretched Trigger A Width (Clock cycles = 2* samples)/"
				"Stretched trigger B delay for basesub (2*samples)/Stretched Trigger B for basesub Width (Clock cycles = 2* samples)/",
				ngpd_setup_diff_trigger_cmd),
	PT_COMMAND ("setup-base-sub", "%d %d fixed %d div-64 div-128 div-256 div-512 div-1k div-2k div-4k div-8k error-lim %d = %d",
				"Setup baseline subtractor",
				"Path/Channel number (-1 for all)/Use a Fixed basline subtract value/Feedback error divided by 64/Feedback error divided by 128/Feedback error divided by 256/Feedback error divided by 512/"
				"Feedback error divided by 1024/Feedback error divided by 2048/Feedback error divided by 4096/Feedback error divided by 8192/Limit feedback error to this +-value/",
				ngpd_setup_baseline_subtract),
	PT_COMMAND ("setup-measure", "%d %d %d %d %g tail-subtract tail-sub-test test-neutron verbose = %d",
				"Setup pulse measurement",
				"Path/Channel number (-1 for all)/Tail sum delay (0..255 samples)/Number of samples to sum in tail sum (1..255)/Fall time end fraction (0.0...1.0)/"
				"Enable pulse tail subtraction/Enable test mode for tail subtraction/Assume event in neutron in tail subtraction/Display debugging messages",
				ngpd_setup_measure),
	PT_COMMAND("load-tail-subtract", "%d %d verbose " UNIFIED_WRITE_QUALIFIERS " = %d",
				"Load Pulse tail subtraction BRAM from unified write get system",
				"Path/Channel number/Calculate and print data total for debugging " UNIFIED_WRITE_QUALIFIER_HELP,    
				ngpd_load_tail_subtract_cmd),
	PT_COMMAND("list-tail-subtract", "%d %d = %d",
				"List Pulse tail subtraction BRAM",
				"Path/Channel number",
				ngpd_list_tail_subtract),
	PT_COMMAND ("setup-neutron-disc", "%d %d %d %d adaptive ignore-fall ignore-sum min-fall %d max-fall %d min-count %d tail-thres-c %d tail-thres-m %g tail-thres-file %s = %d",
				"Setup Neutron/Gamm discrination on the basis of the measurements output",
				"Path/Channel (-1 for all)/Minimum Pulse Height of neutrons/maximum pulse height of neutrons/Use adpative tail summing which stops when next pulse arrives/"
				"Ignore fall time for neutron discrimination/Ignore tail sum for neturon discrimination/Minimum fall time in samples of a neutron/Maximum fall time in samples of a neutron/"
				"Minimum number of tail sum samples to signal a neutron in adaptive mode/Specify a fixed minimum tail sum for neutron (usually not adaptive)/Specify a fixed minimum tail sum Energy dependence for neutron (usually not adaptive)/"
				"Specify a file of 256 pairs of double M value and C in ADC values for Tail sum threshold for 0...255 samples used in the tail sum (adaptive mode)",
				ngpd_setup_neutron_discrimination ),
	PT_COMMAND ("save-diff-trigger", "%d %s = %d",
				"Save setting of differential trigger to a script file",
				"Path/Filename for script file",
				ngpd_save_diff_trigger ),
	PT_COMMAND ("get-diff-trigger", "%d %d = [8%d]",
				"Get setting of differential trigger ",
				"Path/Channel Number",
				ngpd_get_diff_trigger ),
	PT_COMMAND ("get-base-sub", "%d %d = {}",
				"Get baseline subtractor setup",
				"Path/Channel number",
				ngpd_get_baseline_subtract),
	PT_COMMAND ("get-measure", "%d %d = {}",
				"Get tail measurement setup",
				"Path/Channel number",
				ngpd_get_measure),

	PT_COMMAND ("start", "%d %g read wait-scope pause no-set no-clear ext-trig-cycles ncycles %d card %d = %d",
				"Configure and start the system",
				"Path Number/Collection Time (s) or negative to start running without histogramming/Wait fro scope DMA and Read scope data back on completion/Wait for scope DMA to finish/Start system but don't enable counting in software timing mode/"
				"Do not update scope mux setting from scope module/Do not clear any histogram modules/Enable external trigger each cycle/Specify number of cycles/Run a single card rather than all/",
				ngpd_start_cmd),
	PT_COMMAND ("wait-itfg",  "%d once quiet ignore-pause card %d = %d",
				"Wait for ITFG to stop running or reach pause (in ZynqMP generation)",
				"Path/Poll once and return/Do not print status/Do not exit when paused onlt when not running/Specify card (default 0)",
				ngpd_wait_itfg ),
	PT_COMMAND ("read-scope-data", "%d %d no-chunk pausing reset-each-run long-abort = %d",
				"Read scope data from Card via PCIe DMA",
				"path/card/Enable mode where tries to read all data in 1 chunk with no flow control/Enable mode to read in 1 chunk but with pausing (default is multiple smaller chunks and works best)/"
				"Reset DMA each block/Hold Read Abort when idle/",
				ngpd_read_scope_data_cmd),
	PT_COMMAND ("setup-hist", "%d nbins-height %d nbins-tail-sum %d nbins-fall-time %d nbins-tail-ratio %d max-ratio %g separate-ngp discard-pileup = %d",
				"Setup diagnostic histogramming of pulse information",
				"Path/Number of bins in Height histograms (default 1024)/Number of bins in Tail Sum histograms (default 1024)/Number of bins in Fall Time histograms (default 256)/Number of bins in Tail Ratio histograms (default 1024)/"
				"Maximum value of tail sum ratio to fit MCA/Form separate histograms for Neutron, Gammas and Pileup/Discard Pileup events when collecting neutrons and gammas together",
				ngpd_hist_setup_cmd ),
	PT_COMMAND ("setup-hist-ngzmp", "%d nbits-height %d nbits-tail-sum %d nbits-fall-time %d shift-height %d shift-fall-time %d shift-tail-sum %d enb-hgt-sum enb-hgt-fall separate-ngp discard-pileup chan %d = %d",
				"Setup firmware diagnostic histogramming of pulse information in ZynqMP version",
				"Path/Number of bits in Height histograms (default 10 bit => 1024 bins)/Number of bits in Tail Sum histograms (default 10 => 1024 bins)/Number of bits in Fall Time histograms (default 8 => 256 bins)/"
				"Override right shift of height (default allow max hgt of 65535)/Override right shift of tail sum (default allows max of 8*65536)/Override right shift of Fall Time (default allows max of 255)/"
				"Form separate histograms for Neutron, Gammas and Pileup/Discard Pileup events when collecting neutrons and gammas together/"
				"Setup single channel (default to set all channels the same to simplify data layout)/",
				ngpd_hist_setup_ngzmp ),
	PT_COMMAND ("open-hist","%d fall-time tail-sum = %d",
				"Open unified read handle to ZynqMP firmware histograms",
				"Path Number/Height x FllTime/Height x TailSum",
				ngpd_open_histogram_cmd),
#if 0
	PT_COMMAND ("open-scalers","%d dtc = %d",
				"Open Scalers, return unified read handle",
				"Path Number/apply deadtime correction",
				ngpd_open_scaler_cmd),
	PT_COMMAND("save-settings", " %d %s = %d",
		   "Save all hardware settings",
		   "Path/Directory name for save files",
		   ngpd_save_settings_cmd),
	PT_COMMAND("restore-settings", " %d %s force clocks = %d",
		   "Restore all hardware settings",
		   "Path/Directory name for save files/Load even if major revision mismatch/Also restore clock setup",
		   ngpd_restore_settings_cmd),
#endif
	PT_SUBTREE("playback", "Commands for playback functionality", ngpd_playback_tree),
	PT_SUBTREE("filter", "Commands to load or generate the filter coefficients", ngpd_filter_tree),
	PT_SUBTREE("calib", "Commands for calibration functionality", ngpd_calib_tree),
	PT_SUBTREE("scope", "Commands for scope mode functionality", ngpd_scope_tree),
	PT_SUBTREE("debug", "Commands for debug and dma functionality", ngpd_debug_tree),
	PT_SUBTREE("clock", "Commands for Clock generators (on ZynqMP version)", ngpd_clock_tree),
/*
	PT_SUBTREE("trigger-b", "Commands for trigger-B functionality", ngpd_trigger_b_tree),
	PT_SUBTREE("trigger-c", "Commands for trigger-C functionality", ngpd_trigger_c_tree),
	PT_SUBTREE("fan-cont", "Commands for fan speed controller functionality", ngpd_fan_tree),
	PT_SUBTREE("itfg", "Commands for Internal Time frame generator functionality", ngpd_itfg_tree),
	*/
/*	PT_COMMAND("check-progress", "%d busy once = %d",
			"Check progress of experiment time frames",
			"Path/Also monitor UDP thread busy flags/Just read once, otherwise polls until ^C",
			ngpd_check_progress ),
*/
#if 0
	PT_COMMAND("set-sync-mode", "%d none lemo-radial lemo-daisy idc mid-plane lemo-t-daisy global-reset %d = %d",
			"Inform system how board to board synchronisation is wired",
			"Path/None (software only)/LEMO Cables radially from Box 0/LEMO cables from box 0 including daisy-chaining/IDC header in XSPRESS3 and XSPRESS3Mini/Radial Midplane LVDS in XSPRESS4 only/"
			"LEMO daisy chain using LEMOO Tees, X3M terminate last card/"
			"Specify that Global reset is output from this card",
				ngpd_set_sync_mode),
#endif
	PT_SUBTREE("adc-board", "Commands to control gain/offset on XSPRESS3-Mini and XSPRESS4", ngpd_adc_board_tree),
	PT_COMMAND("set-ext-trig-thres", " %d %d %g = %d",
		   "Set External Trigger threshold",
		   "Path/Card/Trigger Threshold (V)",
			ngpd_set_ext_trigger_thres_cmd ),
	PT_COMMAND ("set-dae-pulse", "%d %d %d = %d",
			"Set pulse stretchtime for DAE output on GPIO connector",
			"Path/Channel/Pulse Stretch time in 4 ns cycles",
			ngpd_write_dae_pulse_command ),
	PT_END
};

int ngpd_config_adq14_cmd(int quals, int cn, unsigned int adq_num)
{
	int path = -1;
	int debug;
	debug = (quals & 1) ? 1 : 0;
	if (quals & 2)
		debug = 2;
	if (TotalArgumentCount == 0)
		adq_num = 1;
	path = ngpd_config_adq14(adq_num, debug);
	if (path < 0) {
		sv_printf(cn, "! ERROR %s\n", ngpd_get_error_message());
	}
	return path;
}
int ngpd_config_ngzmp_cmd(int quals, int cn, int ncards)
{
	int path = -1;
	int debug;
	char *baseIPaddress=NULL;
	int basePort=-1;
	char *baseMACaddress=NULL;
	int num_chan = -1;
	int card_index=0;
	int do_init=1;
	int dummy_system=NGPDDummy_None;	// Real system

	debug = (quals & 1) ? 1 : 0;
	if (quals & 2)
		debug = 2;
	if (quals & 4)
		card_index = Qualifier[2].IntArg;
	if (quals & 8)
		baseIPaddress = Qualifier[3].StringArg;
	if (quals & 0x10)
		basePort = Qualifier[4].IntArg;
	if (quals & 0x20)
		baseMACaddress=Qualifier[5].StringArg;
	if (quals & 0x40)
		num_chan = Qualifier[6].IntArg;
	if (quals & 0x80)
		do_init=0;
	if (quals & 0x100)
		dummy_system = NGPDDummy_All;
	else if (quals & 0x200)
		dummy_system = NGPDDummy_ADC;
	else if (quals & 0x400)
		dummy_system = NGPDDummy_Preamp;
	
	path = ngpd_config_ngzmp(ncards,  baseIPaddress, basePort, baseMACaddress, num_chan, debug, card_index, do_init, dummy_system);
	if (path < 0) {
		sv_printf(cn, "! ERROR %s\n", ngpd_get_error_message());
	}
	return path;
}
int ngpd_get_chans_per_card_cmd(int quals, int cn, int path)
{
	int nc = ngpd_get_chans_per_card(path);
	if (nc < 0)
		sv_printf(cn, "! ERROR: Cannot get number of channels per card : %s\n", ngpd_get_error_message());
	return nc;
}

int ngpd_set_run_mode_cmd(int quals, int cn, int path)
{
	int run_flags = NGPD_RUN_FLAGS_DEFAULT;
	int rc;

	switch (quals & 3)
	{
	case 0:	break;		// No playback
	case 1: run_flags |= NGPD_RUN_FLAGS_PLAYBACK | NGPD_RUN_FLAGS_PLAYBACK_CONTINUOUS; break;
	case 2: run_flags |= NGPD_RUN_FLAGS_PLAYBACK ; break;
	default:
		sv_printf(cn, "! ERROR: Please specify either playback or playback-oneshot, not both\n");
		return -1;
	}
	
	if (quals & 4) 
		run_flags |= NGPD_RUN_FLAGS_SCOPEMODE;

	if (quals & 8)
		run_flags &= ~NGPD_RUN_FLAGS_SCOPE_OUT_2_STREAM;

	rc = ngpd_set_run_flags(path, run_flags);

	if (rc < 0)
	{
		sv_printf(cn, "! ERROR: Cannot set run flags : %s\n", ngpd_get_error_message());
	}
	return rc;
}

int ngpd_set_chan_cont_cmd(int quals, int cn, int path, int chan)
{
	int src=NGZMP_CC_SRC_ADC_FWD;
	u_int32_t chan_cont=0;
	int rc;

	switch(quals & 7)
	{
	case 0:
	case 1:
		src=NGZMP_CC_SRC_ADC_FWD;
		break;
		
	case 2:
		src=NGZMP_CC_SRC_ADC_REV;
		break;
		
	case 4:	
		src = NGZMP_CC_SRC_PB;
		break;
		
	default:
		sv_printf(cn, "! ERROR: Please sepcify at most one data source\n");
		return -1;
	}
	
	chan_cont = NGZMP_CC_SRC_SET(src);
	if (quals & 0x8)
		chan_cont |= NGZMP_CC_INV_DATA;
	
	rc = ngpd_write_chan_cont(path, chan, chan_cont);
	if (rc < 0)
		sv_printf(cn, "! ERROR: Caoont wtite chan cont : %s\n", ngpd_get_error_message());
	return rc;
}

int ngpd_read_scope_data_cmd(int quals, int cn, int path, int card) {
	int rc = 0;
	int read_options = 0;
	if (quals & 1)
		read_options |= NGPD_READ_SCOPE_NO_CHUNK;
	if (quals & 2)
		read_options |= NGPD_READ_SCOPE_FLOW_CONTROL;
	if (quals & 4)
		read_options |= NGPD_SCOPE_SKIP_RESET_EACH_RUN;
	if (quals & 8)
		read_options |= NGPD_SCOPE_SKIP_LONG_ABORT;

	rc = ngpd_dma_read_scope(path, card, 0, 0, read_options);
	if (rc < 0) {
		sv_printf(cn,"! ERROR %s\n", ngpd_get_error_message());
	}
	return rc;
}

int ngpd_setup_diff_trigger_cmd(int quals, int cn, int path, int chan, int thresh, int sep, int data_delay, int trig_delay, int delay_a, int width_a, int delay_b, int width_b)
{
	int rc = 0;
	NGPDDiffTrigger trig;
	int num_chan;

	trig.thres      = thresh;
	trig.sep        = sep;
	trig.data_delay = data_delay;
	trig.trig_delay = trig_delay;
	trig.width_a  = width_a;
	trig.delay_a   	= delay_a;
	trig.width_b  = width_b;
	trig.delay_b   	= delay_b;

	if ((num_chan=ngpd_get_num_chan(path)) < 0)
	{
		sv_printf(cn, "! ERROR: ngpd_setup_diff_trigger_cmd:  determine number of channels for path %d: %s\n", path, ngpd_get_error_message());
		return -1;
	}
	if (chan > 0 && chan > num_chan-1)
	{
		sv_printf(cn, "! ERROR: ngpd_setup_diff_trigger_cmd: Channel %d is out of range 0..%d\n", chan, num_chan-1);
		return -1;
	}

	rc = ngpd_write_diff_trigger(path, chan, &trig);
	if (rc != 0) {
		sv_printf(cn,"! ERROR ngpd_setup_diff_trigger_cmd: %s\n", ngpd_get_error_message());
	}
	return rc;
}


int ngpd_start_cmd(int quals, int cn, int path, double col_time)
{
	int rc;
	int do_read = quals &1;
	int wait_scope = !!(quals & 0x2);
	int count_enb = ! (quals & 4);
	int scope_setup = ! (quals & 8);
	int do_clear   = !(quals & 0x10);
	int save_flags;
	int run_flags;
	int card = -1;
	NGPDITFGSetup itfg_setup;
	u_int32_t scope_status;

	itfg_setup.col_time = col_time;
	itfg_setup.trig_mode = 0;

	if (quals & 0x20)
		itfg_setup.trig_mode = NGPDTrigExtTrigCycles;

	if (quals & 0x40)
		itfg_setup.cycles = Qualifier[5].IntArg;
	else
	 	itfg_setup.cycles = 1;
	if (quals & 0x80)
		card = Qualifier[7].IntArg;

	save_flags = ngpd_get_run_flags(path);
	run_flags = save_flags;
	if (scope_setup)
	{
		run_flags |= NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS;
		run_flags &= ~NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD;
	}
	else
	{
		run_flags |= NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD;
		run_flags &= ~NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS;
	}
	if (do_read || wait_scope)
		run_flags |= NGPD_RUN_FLAGS_SCOPEMODE;

	ngpd_set_run_flags(path, run_flags);
	if (do_clear)
		ngpd_hist_clear(path);

	rc = ngpd_dma_system_start(path, card, 0, 0, &itfg_setup);
	if (rc < 0)
	{
		sv_printf(cn, "! ERROR starting system: %s\n", ngpd_get_error_message());
		return rc;
	}

	if (do_read || wait_scope)
	{
		rc = ngpd_dma_wait_scope(path, card, &scope_status);
		if (rc < 0) {
			sv_printf(cn,"! ERROR waiting for scope DMAs %s\n", ngpd_get_error_message());
		}
		if (scope_status != 0)
		{
			sv_printf(cn, "! ERROR: Scope status appears wrong at end of run = %08X\n", scope_status);
			return -1;
		}
	}	
	if (do_read)
	{
		rc = ngpd_dma_read_scope(path, card, 0, 0, 0);
		if (rc < 0) {
			sv_printf(cn,"! ERROR reading scope data %s\n", ngpd_get_error_message());
		}
	}
	ngpd_set_run_flags(path, save_flags);
	set_double_command(0, cn, "ngpd_col_time", col_time);
	return rc;
}


/*
int ngpd_save_settings(int quals, int cn, int path, char *dir_name)
{
	int rc = 0;
	rc = xsp3_save_settings(path, dir_name);
	if (rc != 0) {
		sv_printf(cn,"! ERROR %s\n", ngpd_get_error_message());
	}
	return rc;
}
int ngpd_restore_settings(int quals, int cn, int path, char *dir_name)
{
	int rc = 0;
	int force= quals & 1;
	int do_clock = !!(quals & 2);

	if (do_clock)
		rc = xsp3_restore_settings_and_clock(path, dir_name, force);
	else
		rc = xsp3_restore_settings(path, dir_name, force);

	if (rc != 0) {
		sv_printf(cn,"! ERROR %s\n", ngpd_get_error_message());
	}
	ngpd_format_update_paths(cn, path);
	return rc;
}
*/


/*
int ngpd_check_progress(int quals, int cn, int path)
{
	int completed, prev_frame;
	int busy, prev_busy;
	int show_busy = quals&1;
	int once= !!(quals&2);
	Xsp3ErrFlag flags;
	prev_frame=-1;
	prev_busy=0;

	do 
	{
		completed = xsp3_scaler_check_progress_details(path, &flags, 0, NULL);
		if (completed < 0)
		{
			sv_printf(cn, "! ERROR: Cannot read progress: %s\n", ngpd_get_error_message());
			return -1;
		}
		busy = xsp3_histogram_is_any_busy(path);
		if (completed != prev_frame || show_busy && prev_busy != busy)
		{
			sv_printf(cn, "# Completed=%d, Flags=0x%08X, busy=%d\n", completed, flags, busy);
		}
		prev_busy = busy;
		prev_frame = completed;
	} while (sv_quit == 0 && once == 0 );

	return completed;
}
*/		 

int ngpd_set_debug_cmd(int quals, int cn, int path, int level)
{
	return ngpd_set_debug(path, level);
}

/*
int ngpd_set_sync_mode(int quals, int cn, int path)
{
	int rc;
	int mode=0;
	int enb_gr=0, gr_card=0;

	switch (quals & 0x3F)
	{
	case 0x01: mode = XSP3_SYNC_NONE; break;
	case 0x02: mode = XSP3_SYNC_LEMO_RADIAL; break;
	case 0x04: mode = XSP3_SYNC_LEMO_DAISYCHAIN; break;
	case 0x08: mode = XSP3_SYNC_IDC; break;
	case 0x10: mode = XSP3_SYNC_MIDPLANE; break;
	case 0x20: mode = XSP3_SYNC_LEMO_DAISYCHAIN_TEE; break;
	default:
		sv_printf(cn, "! ERROR: Please specify exactly one sync mode\n");
		return -1;
	}

	if (quals & 0x40)
	{
		enb_gr = 1;
		gr_card = Qualifier[6].IntArg;
	}

	if ((rc=xsp3_set_sync_mode(path, mode, enb_gr, gr_card)) <0 )
		sv_printf(cn, "! ERROR: Setting sync mode: %s\n", ngpd_get_error_message());
	return rc;
}
*/

int ngpd_setup_baseline_subtract(int quals, int cn, int path, int chan)
{
	NGPDBaseSubtract bsub;
	int rc;
		
	memset(&bsub, 0, sizeof(bsub));
	if (quals & 1)
	{
		bsub.fixed = Qualifier[0].IntArg;
		bsub.use_fixed = 1;
	}
	else
	{
		switch (quals & 0x1FE)
		{
		case 2: bsub.div_cont = 0; break;
		case 4: bsub.div_cont = 1; break;
		case 8: bsub.div_cont = 2; break;
		case 0x10: bsub.div_cont = 3; break;
		case 0x20: bsub.div_cont = 4; break;
		case 0x40: bsub.div_cont = 5; break;
		case 0x80: bsub.div_cont = 6; break;
		case 0x100: bsub.div_cont =7; break;
		default:
			sv_printf(cn, "! ERROR: If not using a fixed basline subtraction please specify one and only 1 divide ratio for feedback\n");
			return -1;
		}
	}
	if (quals & 0x200)
		bsub.error_limit = Qualifier[9].IntArg;

	rc = ngpd_write_baseline_subtract(path, chan, &bsub);

	if (rc < 0)
	{
		sv_printf(cn,  "! ERROR: Setting baseline subtractor %s\n", ngpd_get_error_message());
	}
	return rc;
}

int ngpd_setup_measure(int quals, int cn, int path, int chan, int tail_delay, int tail_sum, double fall_frac)
{
	NGPDMeasure measure;
	int rc;
	int first_chan, last_chan, num_chan;

	num_chan = ngpd_get_num_chan(path);
	if (num_chan < 0)
	{
		sv_printf(cn, "! ERROR determining number of channels.: %s\n", ngpd_get_error_message());
		return -1;
	}
	if (chan < 0)
	{
		first_chan = 0;
		last_chan = num_chan -1;
	}
	else if (chan >= num_chan)
	{
		sv_printf(cn, "! ERROR: Expected num_chan in range 0..%d or -1 for all channels, not %d\n", num_chan-1, chan);
		return -1;
	}
	else
	{
		first_chan = chan;
		last_chan = chan;
	}
	for (chan=first_chan; chan<=last_chan; chan++)
	{
		memset(&measure, 0, sizeof(measure));
		rc = ngpd_read_measure(path, chan, &measure);

		if (rc < 0)
		{
			sv_printf(cn,  "! ERROR: Reading pulse measurement %s\n", ngpd_get_error_message());
		}

		measure.tail_sum_delay = tail_delay;
		measure.tail_sum_num = tail_sum;
		measure.fall_time_frac = fall_frac;
		measure.enable_tail_subtract = !!(quals & 1);
		measure.tail_subtract_test = !!(quals & 2);
		measure.tail_subtract_test_neutron = !!(quals & 4);
		measure.min_tail_count = 1;
		if (quals & 8)
		{
			sv_printf(cn, "# Min_height=%d, max=%d\n", measure.min_height, measure.max_height);
			sv_printf(cn, "# Min_fall_time=%d, max=%d, min_tail_count=%d\n", measure.min_fall_time, measure.max_fall_time, measure.min_tail_count) ;
		}

		rc = ngpd_write_measure(path, chan, &measure);

		if (rc < 0)
		{
			sv_printf(cn,  "! ERROR: Setting pulse measurement %s\n", ngpd_get_error_message());
		}
	}
	return rc;
}
int ngpd_setup_neutron_discrimination(int quals, int cn, int path, int chan, int min_height, int max_height)
{
	NGPDMeasure measure;
	int rc;
	int first_chan, last_chan, num_chan;
	int adaptive, ignore_fall, ignore_tail_sum;
	int min_fall_time=0, max_fall_time=0;
	int min_tail_count = 1;
	int tail_thres_m[NGPD_MEASURE_TAIL_THRES_SIZE];
	int tail_thres_c[NGPD_MEASURE_TAIL_THRES_SIZE];
	int i;
	FILE *fp;

	memset(tail_thres_m, 0, NGPD_MEASURE_TAIL_THRES_SIZE*sizeof(int));
	memset(tail_thres_c, 0, NGPD_MEASURE_TAIL_THRES_SIZE*sizeof(int));

	num_chan = ngpd_get_num_chan(path);
	if (num_chan < 0)
	{
		sv_printf(cn, "! ERROR determining number of channels.: %s\n", ngpd_get_error_message());
		return -1;
	}
	if (chan < 0)
	{
		first_chan = 0;
		last_chan = num_chan -1;
	}
	else if (chan >= num_chan)
	{
		sv_printf(cn, "! ERROR: Expected num_chan in range 0..%d or -1 for all channels, not %d\n", num_chan-1, chan);
		return -1;
	}
	else
	{
		first_chan = chan;
		last_chan = chan;
	}

	adaptive = !!(quals & 1);
	ignore_fall = !!(quals & 2);
	ignore_tail_sum = !!(quals & 4);

	switch (quals & 0x18)
	{
	case 0:
		min_fall_time = 0;
		max_fall_time = NGPD_MEASURE_FALL_TIME_MAX;
		break;

	case    8:
	case 0x10: 
		sv_printf(cn, "! To use fall time please specify both min and max fall time\n");
		return -1;

	case 0x18:
		if (ignore_fall)
		{
			sv_printf(cn, "! ERROR: Do not use ignore fall if specifying min and max fall time\n");
			return -1;
		}
		min_fall_time = Qualifier[3].IntArg;
		max_fall_time = Qualifier[4].IntArg;
		break;
	}
	if (quals & 0x20)
	{
		if (!adaptive)
		{
			sv_printf(cn, "ERROR: Please only specify min tail count if using adaptive tail sum\n");
			return -1;
		}
		min_tail_count = Qualifier[5].IntArg;
	}
	if (quals & 0x40)
	{
		if (ignore_tail_sum)
		{
			sv_printf(cn, "ERROR: Please do not use ignore tail sum if specifiy tail sum threshold\n");
			return -1;
		}
		for (i=0; i< NGPD_MEASURE_TAIL_THRES_SIZE; i++)
			tail_thres_c[i] = Qualifier[6].IntArg;
	}
	if (quals & 0x80)
	{
		double m;
		m = Qualifier[7].DoubleArg;
		if (m < 0)
		{
			sv_printf(cn, "! ERROR: M value must be >=0, not %g\n", m);
			return -1;
		}
		if (ignore_tail_sum)
		{
			sv_printf(cn, "ERROR: Please do not use ignore tail sum if specifiy tail sum threshold\n");
			return -1;
		}
		for (i=0; i< NGPD_MEASURE_TAIL_THRES_SIZE; i++)
			tail_thres_m[i] = m*0x800000;
	}
	if (quals & 0x100)
	{	
		char line_buf[80];
		if ((fp=fopen(Qualifier[7].StringArg, "r")) == NULL)
		{
			sv_printf(cn, "! ERROR: Cannot open input file '%s\n", Qualifier[8].StringArg);
			return -1;
		}
		for (i=0; i<NGPD_MEASURE_TAIL_THRES_SIZE; i++)
		{
			line_buf[79]=0;
			double m;
			if (fgets(line_buf, 78, fp) == NULL)
			{
				sv_printf(cn, "! ERROR: premature end of file at line %d\n", i);
				fclose(fp);
				return -1;
			}

			if (sscanf(line_buf, "%lg %d", &m, &(tail_thres_c[i])) != 2)
			{
				sv_printf(cn, "! ERROR: Cannot read value M and C values for point %d of 0..%d from file=%s:line=%s\n", i, NGPD_MEASURE_TAIL_THRES_SIZE-1, Qualifier[8].StringArg, line_buf);
				fclose(fp);
				return -1;
			}
			tail_thres_m[i] = 0x800000*m;
			if (m < 0)
			{
				sv_printf(cn, "! ERROR: Line %d: M value must be >=0, not %g\n",i, m);
				fclose(fp);
				return -1;
			}
		}
		fclose(fp);
	}
	for (chan=first_chan; chan<=last_chan; chan++)
	{
		memset(&measure, 0, sizeof(measure));
		rc = ngpd_read_measure(path, chan, &measure);
		if (rc < 0)
		{
			sv_printf(cn,  "! ERROR: Reading pulse measurement %s\n", ngpd_get_error_message());
		}

		measure.adaptive_tail_sum = adaptive;
		measure.ignore_fall_time = ignore_fall;
		measure.ignore_tail_sum = ignore_tail_sum;
		measure.min_height = min_height;
		measure.max_height = max_height;
		measure.min_fall_time = min_fall_time;
		measure.max_fall_time = max_fall_time;
		measure.min_tail_count = min_tail_count;

		for (i=0; i<NGPD_MEASURE_TAIL_THRES_SIZE; i++)
		{
			measure.tail_thres_m[i] = tail_thres_m[i];
			measure.tail_thres_c[i] = tail_thres_c[i];
		}
		rc = ngpd_write_measure(path, chan, &measure);

		if (rc < 0)
		{
			sv_printf(cn,  "! ERROR: Setting pulse measurement %s\n", ngpd_get_error_message());
		}
	}
	return rc;
}



int ngpd_save_diff_trigger(int quals, int cn, int path, char *fname)
{
	int num_chan, chan;
	NGPDDiffTrigger trig;
	int rc;
	FILE *fp;

	num_chan= ngpd_get_num_chan(path);
	if (num_chan < 0)
	{
		sv_printf(cn, "! ERROR: ngpd_save_diff_trigger : %s\n", ngpd_get_error_message());
		return num_chan;
	}
	
	if ((fp=fopen(fname, "w")) == NULL)
	{
		sv_printf(cn, "! ERROR: ngpd_save_diff_trigger: Cannot open output file %s, errno=%d\n", fname, errno);
		return -1;
	}
	for (chan=0; chan<num_chan; chan++)
	{
		if (ngpd_read_diff_trigger(path, chan, &trig) < 0)
		{
			sv_printf(cn, "! ERROR reading diff trigger setup on chan %d: %s\n", chan, ngpd_get_error_message());
			fclose(fp);
			return -1;
		}
		fprintf(fp, "ngpd setup-diff-trigger %d %d %d %d %d %d %d %d %d %d\n", path, chan, trig.thres, trig.sep, trig.data_delay, trig.trig_delay, trig.delay_a, trig.width_a, trig.delay_b, trig.width_b);
	}
	fclose(fp);
	return 0;
}

void * ngpd_get_diff_trigger(int quals, int cn, int path, int chan)
{
	NGPDDiffTrigger trig, *ptr;
	
	if (ngpd_read_diff_trigger(path, chan, &trig) < 0)
	{
		sv_printf(cn, "! ERROR reading diff trigger setup on chan %d: %s\n", chan, ngpd_get_error_message());
		return NULL;
	}
	ptr=malloc(sizeof(NGPDDiffTrigger));
	if (ptr == NULL)
	{
		sv_printf(cn, "! ERROR: Out of memory\n");
		return NULL;
	}
	*ptr = trig;
	return (void *)ptr;
}

void * ngpd_get_baseline_subtract(int quals, int cn, int path, int chan)
{
	NGPDBaseSubtract bsub;
	DictReturnType *dict;
	int i=0;
	char * div_names[9] = {"div-64", "div-128", "div-256", "div-512", "div-1k", "div-2k", "div-4k", "div-8k" };
	dict= malloc(sizeof(DictReturnType)*6);
	
	if (ngpd_read_baseline_subtract(path, chan, &bsub) < 0)
	{
		sv_printf(cn, "! ERROR: Cannot read baseline subtraction setup : %s\n", ngpd_get_error_message());
		return NULL;
	}
	if (bsub.use_fixed)
	{
		dict[0].type = 'd';
		dict[0].label = strdup("fixed");
		dict[0].IntVal = bsub.fixed;
		dict[1].type = 0;
	}
	else
	{
		dict[0].type = 's';
		dict[0].label = strdup("divCont");
		dict[0].StringVal = strdup(div_names[bsub.div_cont]);
		i = 1;
		if (bsub.error_limit > 0)
		{
			dict[i].type = 'd';
			dict[i].label = strdup("errorLimit");
			dict[i].IntVal = bsub.error_limit;
			i++;
		}
		dict[i].type = 0;
	}
	
	return (void *)dict;
}
void *ngpd_get_measure(int quals, int cn, int path, int chan)
{
	NGPDMeasure measure;
	DictReturnType *dict;
	int i=0, rc;

	dict= malloc(sizeof(DictReturnType)*10);
	memset(&measure, 0, sizeof(measure));

	rc = ngpd_read_measure(path, chan, &measure);
	if (rc < 0)
	{
		sv_printf(cn, "! ERROR : Cannot read measurement setup: %s\n", ngpd_get_error_message());
		return NULL;
	}
	i=0;
	dict[i].type = 'd'; 
	dict[i].label = strdup("tailSumDelay");
	dict[i++].IntVal = measure.tail_sum_delay;
	dict[i].type = 'd'; 
	dict[i].label = strdup("tailSumNum");
	dict[i++].IntVal = measure.tail_sum_num;
	dict[i].type = 'g'; 
	dict[i].label = strdup("fallTimeFrac");
	dict[i++].DoubleVal = measure.fall_time_frac;
	if (measure.enable_tail_subtract)
	{
		dict[i].type = 'q'; 
		dict[i].label = strdup("tailSubtract");
		dict[i++].StringVal = strdup("True");
	}
	if (measure.tail_subtract_test)
	{
		dict[i].type = 'q'; 
		dict[i].label = strdup("tailSubTest");
		dict[i++].StringVal = strdup("True");
	}
	if (measure.tail_subtract_test_neutron)
	{
		dict[i].type = 'q'; 
		dict[i].label = strdup("testNeutron");
		dict[i++].StringVal = strdup("True");
	}
	dict[i].type = 0;
	return (void *) dict;
}

int ngpd_load_tail_subtract_cmd(int quals, int cn, int path, int chan)
{
	int32_t *ptr=NULL;
	int num_chan;
	int size;
	int rc;
	int i, total;
	int verbose = quals & 1;

	if ((num_chan = ngpd_get_num_chan(path)) < 0)
	{
		sv_printf(cn, "! ERROR: Cannot get number of channels: %s\n", ngpd_get_error_message());
		return num_chan;
	}

	size = 2*NGPD_MEASURE_TAIL_SUB_SIZE;


	if (chan < 0 || chan >= num_chan)
	{
		sv_printf(cn, "! ERROR: Channel %d is out of range 0..%d\n", chan, num_chan-1);
		return -1;
	}

	if (unified_write_get(quals, cn, 1, size, UNIF_DATA_LONG, (int **)&ptr) < 0)
	{
		sv_printf(cn, "! Unified write system returned error\n");
		return ERROR;
	}
	if (verbose)
	{
		total=0;
		for (i=0; i<size; i++)
			total += ptr[i];
		sv_printf(cn, "# Note data total =%d\n", total);
	}

	rc = ngpd_write_tail_subtract(path, chan, ptr);
	if (rc < 0)
		sv_printf(cn, "! ERROR writing pulse tail subtract BRAMs : %s\n", ngpd_get_error_message());
	free (ptr);
	return rc;
}

int ngpd_list_tail_subtract(int quals, int cn, int path, int chan)
{
	int size = 2*NGPD_MEASURE_TAIL_SUB_SIZE;
	int32_t ptr[size];
	int num_chan;
	int rc;
	int i, total;
	int supress0=!!(quals & 1);

	if ((num_chan = ngpd_get_num_chan(path)) < 0)
	{
		sv_printf(cn, "! ERROR: Cannot get number of channels: %s\n", ngpd_get_error_message());
		return num_chan;
	}

	if (chan < 0 || chan >= num_chan)
	{
		sv_printf(cn, "! ERROR: Channel %d is out of range 0..%d\n", chan, num_chan-1);
		return -1;
	}
	rc = ngpd_read_tail_subtract(path, chan, ptr);
	if (rc < 0)
		sv_printf(cn, "! ERROR writing pulse tail subtract BRAMs : %s\n", ngpd_get_error_message());

	total=0;
	for (i=0; i<size; i++)
	{
		if (ptr[i] & 0x20000)
			ptr[i] -= 0x40000;
		if (ptr[i] == 0 && supress0)
		{
		}
		else
		{
			sv_printf(cn, "# %d\t%d\n", i, ptr[i]);
		}
		total += ptr[i];
	}
	sv_printf(cn, "# Note data total =%d\n", total);
	return rc;
}


int ngpd_set_ext_trigger_thres_cmd(int quals, int cn, int path, int card, double trig_thres)
{
	if (ngpd_dma_set_ext_trigger_thres(path, card, trig_thres) != 0)
	{
		sv_printf(cn, "! ERROR Setting external trigger threshold\n");
		return -1;
	}
	return 0;
}
int ngpd_write_dae_pulse_command(int quals, int cn, int path, int chan, int stretch)
{
	if (ngpd_write_dae_pulse(path, chan, stretch) < 0)
	{
		sv_printf(cn, "! ERROR: Setting DAE Pulse Output : %s\n", ngpd_get_error_message());
		return -1;
	}
	return 0;
}

int ngpd_wait_itfg(int quals, int cn, int path)
{
	NGPDGeneration generation;
	int rc;
	int once = quals & 1;
	int quiet = !!(quals & 2);
	int ignore_pause = !!(quals & 4);
	int card = 0;
	u_int32_t prev_status=0xFFFFFFFF, prev_cycles=0xFFFFFFFF;
	u_int32_t status_regs[2];
			
	generation = ngpd_get_generation(path);
	if (generation != NGPDGenZynqMP1)
	{
		sv_printf(cn, "! ERROR: Wait ITFG only supported on ZYnqMP generation, not %d\n", generation);
		return -1;
	}

	do 
	{
		if ((rc=ngpd_read_glob_regs(path, card, NGZMP_SCOPE_STATUS, 2, status_regs)) < 0)
		{
			sv_printf(cn, "! ERROR reading statsus registers : %s\n", ngpd_get_error_message());
			return rc;
		}
		if (!quiet && (status_regs[0] != prev_status || status_regs[1] != prev_cycles))
		{
			char *state_string;
			if (!(status_regs[0] & NGPD_SCOPE_STATUS_ITFG_RUNNING))
				state_string = "Idle";
			else
			{
				if (status_regs[0] & NGPD_SCOPE_STATUS_ITFG_WAITING)
					state_string = "Paused";
				else if (status_regs[0] & NGPD_SCOPE_STATUS_ITFG_COUNTING)
					state_string = "Counting";
				else
					state_string = "Not Counting";
			}
			sv_printf(cn, "# Status=0x%08X, cycles=%u, %s\n", status_regs[0], status_regs[1], state_string);
		}
		if (!ignore_pause && (status_regs[0] & NGPD_SCOPE_STATUS_ITFG_WAITING))
			break;
		prev_status = status_regs[0];
		prev_cycles = status_regs[1];
	} while (!once && (status_regs[0] & NGPD_SCOPE_STATUS_ITFG_RUNNING));
	return 0;
}

