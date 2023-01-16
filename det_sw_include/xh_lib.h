/*
 * xh_lib.h
 *
 *  Created on: Feb 6, 2013
 *      Author: gm
 */

#ifndef XH_LIB_H_
#define XH_LIB_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {XH1024, XStrip1024, XHIR1024} XHSensorType;

typedef enum _XH_HVState
{
	XH_HVState_PosOff    = 0,
	XH_HVState_PosForced = 1,
	XH_HVState_PosAuto   = 2,
	XH_HVState_NegOff    = 3,
	XH_HVState_NegForced = 4,
	XH_HVState_NegAuto   = 5
} 	XH_HVState;


typedef enum _XHState {
	Idle=0, ///< The detector is not acquiringing data
	PausedAtGroup=1, ///< The detector has paused at the beginning of a group
	PausedAtFrame=2, ///< The detector has paused at the beginning of a frame
	PausedAtScan=3, ///< The detector has paused at the beginning of a scan
	Running=4, ///< The detector is acquiring data.
	BufferOverrun=5	///< Too many time frame (Ultra Only)
} XHState;

typedef struct _XHStatus {
	XHState state;
	int group_num; ///< The current group number being acquired, only valid when not {@link #Idle}
	int frame_num; ///< The current frame number, within a group, being acquired, only valid when not {@link #Idle}
	int scan_num; ///< The current scan number, within a frame, being acquired, only valid when not {@link #Idle}
	int cycle; ///< Not supported yet
	int completed_frames; ///< The number of frames completed, only valid when not {@link #Idle}
} XHStatus;

typedef enum _readoutModeType {
long32,
short16
} readoutModeType;

typedef enum _eepromDataType{
doubleData,
floatData,
uint32Data,
uint16Data,
uint8Data,
int32Data,
int16Data,
int8Data
} eepromDataType;

typedef enum _headVoltageType {
Vdd,
Vref,
Vrefc,
Vres1,
Vres2,
Vpupref,
Vclamp,
Vled,
Vtrip		// USed in ULtare only
} headVoltageType;

typedef enum _pexpType {
	setca = 0x1,
	sel8p = 0x2,
	calen = 0x4,
	selsi = 0x8,
	ledon = 0x10
} pexpType;

typedef enum _capacitanceType {
sel2p	= 0,		///> Dont add additional capacitance
sel3pAB = 0x1,		///> Add 3pF to AB
sel5pAB = 0x2,		///> Add 5pF to AB
sel10pAB = 0x4,		///> Add 10pF to AB
sel30pAB = 0x8,		///> Add 30pF to AB
sel3pCD = 0x10,		///> Add 3pF to CD
sel5pCD = 0x20,		///> Add 5pF to CD
sel10pCD = 0x40,	///> Add 10pF to CD
sel30pCD = 0x80		///> Add 30pF to CD
} capacitanceType;

typedef enum _triggerOutputType {
XHTrigOut_dc,				///> DC value from software polarity control only
XHTrigOut_wholeGroup,		///> Asserted for full duration of enabled groups
XHTrigOut_groupPreDel,		///> Pulse At start of Group before delay or trigger in
XHTrigOut_groupPostDel,		///> Pulse At start of Group after delay and trigger in
XHTrigOut_framePreDel,		///> Pulse At start of Frames before delay or trigger in
XHTrigOut_framePostDel,		///> Pulse At start of Frames after delay and trigger in
XHTrigOut_scanPreDel,		///> Pulse At start of Scans before delay or trigger in
XHTrigOut_scanPostDel,		///> Pulse At start of Scans after delay and trigger in
XHTrigOut_integration,		///> Assert During integration time
XHTrigOut_aux1,				///> Derive from aux 1 pulse
XHTrigOut_waitExtTrig,		///> Assert while waiting for external trigger
XHTrigOut_waitOrbitSync,	///> Assert while waiting for Orbit sync
XHTrigOut_toggle			///> Toggle output at 25 MHz for hardwar debug.
} XHTriggerOutputType;

typedef enum _triggerControlType {
XHTrigIn_noTrigger    = 0,			///>
XHTrigIn_groupTrigger = 0x1,		///>
XHTrigIn_frameTrigger = 0x2,		///>
XHTrigIn_scanTrigger  = 0x4,		///>
XHTrigIn_groupOrbit = 0x8,			///>
XHTrigIn_frameOrbit = 0x10,			///>
XHTrigIn_scanOrbit  = 0x20,			///>
XHTrigIn_fallingTrigger = 0x40		///>
} XHTriggerControlType;

typedef enum _xchip_response_process {
XHXChipProcess_Uninterleave 	= 0x1, 
XHXChipProcess_MaskDead 		= 0x2
} XHXChipProcess;
 

typedef enum _clockModeType {
	internal,			///> internal clock
	ESRF5468MHz,		///> ESRF 5.468 MHz clcok
	ESRF1136MHz,		///> ESRF Clock settings for RF div 31 = 11.3 MHz
	Ext50MHz,			///> test mode with 50 MHz external source
	Ext12_5MHz,			///> test mode with 12.5 MHz external source
	DLS6406MHz,          ///< DLS 6.405820513 MHz Clock input, = RF/78
	DLS1601MHz,			///< DLS 1.601455128 MHz Clock input = RF/312
	DLS533kHz			///< DLS 533 kHz 1 per turn RF/936
} clockModeType;

typedef enum _tcParamType {
	pGain,			///> PID P term (0->1023)
	iGain,			///> PID I term (0->1023)
	dGain,			///> PID D term (0->1023)
	gScale,			///> gain scaling
	offset,			///> Sensor offset adjust (0->255)
	gain,			///> Sensor gain
	setPoint,		///> temperature set point

// get params
	pwm,			///> pwm status (1=enabled, 0=disabled)
	timeLeft,		///> Remaining log time in s
	mode,			///> mode
	overTemp,		///> Over temperature protection (0=disabled, 1=enabled)
	hV,				///> hv master enable (0=+ve off, 1=+ve on, 2=+ve on bias shutdown protection 3=-ve off, 4=-ve on, 5=-ve on bias shutdown protection)
	headPower,		///> Head power master enable (0=off, 1=on, 2=thermal shutdown protection)
	tcStatus,		///> TC status byte indicates status of headpwr, mode, pwm status & hv status
// get temp
	temp,			///> Current temperature
// get status
	ocs,			///> open circuit status
	scs,			///> short circuit status
	tcs,			///> over temp status
//
	reset,
	loadDefaults,
	autoinc,
	logTC,
	logTemp,
	//logall,
	//logTC_with_autoinc,
	//logTemp_with_autoinc,
	//logall_with_autoinc,
	additionalNotifications,
	interruptPrescale,
	no_debug,
	debug_overwrite,
	debug_append,
	ShortCircuitProtect,	//!<	Enable PT100 short circuit detection and protection
	OpenCircuitProtect,		//!<	Enable PT100 open circuit detection and protection
	writeEEPROM,
	debugConnection,		//!<   Enable/Disable debug message to da.server connection.
	bangBangHyst,
	readbackAll,			//!< Force output of most setable parameters
	ADCGain,
	logtKeep,				//! Keep logt files using current date ana time, rathe than rolling name.1 ... name.10
	numAve
} tcParamType;

typedef enum 
{
	XHINLMapAuto,
	XHINLMapDirect,
	XHINLMapXH,
	XHINLMapXStrip
} XHINLCorrMapping;

typedef enum
{
	XHX3ChipMode_Default = 0,
	XHX3ChipMode_HighSpeed = 1,
	XHX3ChipMode_MinPower = 2,
	XHX3ChipMode_LowPower2 = 3,
	XHX3ChipMode_LowPower4 = 4
} XHX3ChipMode;

typedef enum { XHHvADCSel_HV_V, XHHvADCSel_HV_VSet, XHHvADCSel_HV_I, XHHvADCSel_12V, XHHvADCSel_5V2 }
XHHvADCSel ;
#define XHT_CORRECT_ROUNDING 1
#define XHT_ALLOW_OVERFLOW  	2		//!< This group is allowed to overflow 16/32bit limit.
#define XHT_CYCLE_FIRST 	(1<<31)
#define XHT_CYCLE_LAST 		(1<<30)

#define XH_T_RSTR_S1R_UNUSED 10000

typedef struct
{ 
	/* First half is requested by user */
 	uint32_t group_delay, frame_delay;
 	int nscans, nframes;
	int int_time;
	int lemo_control;
	int fixed_rst_s1;
	int fixed_rstr_s1r;				// Fixed time from Reset risign to S1 rising
	int req_scan_period;
	XHTriggerControlType trig_flags;		// Requested trigger setup, used to build trig_control
	int trig_mux_sel, orbit_mux_sel;
	/* This is calculated */
	int trig_control;
	int reset_s1_overlap;
	int scan_period;
 	int rst_delay, rst_pw;
 	int s1_delay, s1_pw;
 	int s2_delay, s2_pw;
 	int shiftin_delay;
 	int retrigger_delay;
	int aux_delay, aux_pw;
	int scan_delay;
 	int xclk_half_period;
 	int pixel0_half_period;
	int adc_settling_time, adc_settling_pixel0, num_adc_samples;
	int long_s12;
	int frame_time;
	int flags;
	int warnings;
	int shift_down;
	int cycles;	/* see also flags */
	unsigned char s1fine_delay, s2fine_delay, rst_fine_delay_r, rst_fine_delay_f, xclk_fine_delay;
	int xclk_options;
	int xclk_period;
	int xclk_stall;		// < disables stall, 0..31 inserts stall in specified Xclk cycle to all setting around S1
	int xclk_stall2;	// <0 disables stall2, 0..31 inserts stall in specified Xclk cycle to all setting around S1
 } XHTimingGroup;


typedef struct 
{
	int s1_pw, s2_pw;
	int rst_min_non_overlap, rst_min_overlapped; /* reset width if it is or is not overlapped with S1 */
	int s2_pw_min;		/* IF S2 Pulse width needs to be less than this, consider delaying S1 and reset to make space */
	int tcs, ts2r;
	int trs_r2f; /* Rising edge of reset to falling edge of s1 in ideal non-overlapped case */
	int trs_r2r;  /* Less ideal where Reset and S1 overlap, but still allow some settling after rising edge of reset to rising edge of s1 */  
	int ts2readout;
	int ts2muxrst;	/* time from S2 to Mux reset Asserted when using Mux Reset delayed modes */
	int s1first;
	int beam_gap; /* gap before integration period (S1 rising) where there is no flux, so can allow settling time, 0 for multibunch patterns */
//	int xclk_half_period;
	int min_xclk_half_period;
//	int min_pixel0_half_period;
	int max_xclk_half_period;
	int pixel0_stretch;
	int min_adc_settle;
	int adc_post_gap;
	int hide_xchip_glitches;
	int extra_readout_gap;
	int trst_s1_fixed;	/* If this is specifeid, force time from Reset Rising to S1 falling tot be fixed to this value */
	int muxrst_to_s1, muxrst_to_s2, reset_to_1st_xclk;
	int muxrst_pw;			//!< Minimum mux reset pulse width
	int s2_to_1st_xclk;		//!< Calculated time from S2 to 1 XClk in delayed mux reset modes.
	int muxrst_to_rstr;		// Gap from mux reset assert to reset Rising (going false)
	int muxrst_to_rstf;		// Gap from mux reset assert to reset Falling (going true)
	int rstf_to_muxrst;		// Gap from reset Falling (going true) to mux reset assert
	int muxrst_neg_to_next;	 // Gap from mux reset negated to start ofd next scan (S1 or reset 
	int s1_to_muxrst;		// Gap from S1 Rising to mux reset assert 
	int ts2rst_early;
	int shift_in_to_xclk;	// Delay from Shift in rising to first XClk rising 
//	int xclk_to_shift_in;	// Delay from Last Xclk rising to Shift In Rising ... State machine cannot make this too early
	int s1_to_readout;		//!< Time from S1 rising to readout (ShiftIn) of previous scan in fully overlapped mode.
	int fixed_xclk_hi_hp;	//!< If >0, use a fixed time for the high phase of all Xclk cycles.
	int xclk_r_to_s1;		//!< Minimum time from Previous XClk rising to S1 rising in stall Xclk timing modes
	int xclk_f_to_s1;		//!< Minimum time from Previous XClk falling to S1 rising in stall Xclk timing modes
	int s1_to_xclk;			//!< Minimum time from S1 rising to next XClk rising in stall Xclk timing modes.
	int samples_to_s1;		//!< Time from last used ADC sample to S1 or Reset rising in mode=XH_HIDE_MODE_DEL_MR_STALL_SHORT
	int samples_to_s2;		//!< Time from last used ADC sample to S2. Shpuld be tested in all modes, should be related to align_offset and allow for post_gap.
	int s1_to_samples;		//!< Time from S1 to first  used ADC sample2. Only supported in mode=XH_HIDE_MODE_DEL_MR_STALL_SHORT so farm, using stall2
	int xclk_stall_time;	//!< width of gap (stall) in XClk to allow settling of Vdd for S1 .. calculated from pervious numbers
	int xclk_stall2_time;	//!< width of gap (stall2) in XClk to allow settling of Vdd for S1 .. calculated from pervious numbers
	int align_offset;		//!< Position of S1 relative to XClk rising when using Align edges mode (0 == aligned)
	int bunch_sync_cycles;	//!< Time in cycles of beam circulation if working synced to orbit.
	int min_xclk_to_si;		//!< When using aux1 to make Mux reset assert xclk, leave gap to shift in.
	double tracking_fraction;	//<! Fraction of integration time used allowed S1 to track ramp, which loses dynamic range.
	int t_fixed_rstr_s1r;	//!< Used fixed time from Reset Rising to S1 rising. Similar to trst_s1_fixed, but handles rounding of S1 pulse width differently.
	int s2f_to_s1r;			//! < Minimum time from S2 falling (start of sampling) to S1 rising (final sampling)
	int s1r_to_s2f;			//! < Minimum time from S1 rising (final sampling) to S2 falling (start of sampling)
	unsigned char s1fine_delay, s2fine_delay, rst_fine_delay_r, rst_fine_delay_f, xclk_fine_delay;

} XHTimingFixed;

typedef enum {XH_XChipImpulse_Normal, XH_XChipImpulse_TestCard64, XH_XChipImpulse_TestCard4} XH_XChipImpulse;

int xh_check_tc_power();
int xh_check_tc_running(int sys);
int sv_printf(int cn, char* format, ...);

int xh_set_headmask(int sys, int mask);
int xh_get_sys(int cn, char *sys_name);
void xh_set_cn(int cn);
int xh_get_revision(int sys);
int xh_get_max_heads();
int xh_config(const char *sys_name, const char *path_name, int head_mask, int num_pixels, XHSensorType sensor_type, bool xchip3, int debug, bool full_perf, int head_serial, int ke2410_sys);
int xh_read_normal(int sys, void *to, unsigned x, unsigned y, unsigned t, unsigned dx, unsigned dy, unsigned dt);
int xh_read_un_interleave(int sys, void *to, unsigned x, unsigned y, unsigned t, unsigned dx, unsigned dy, unsigned dt);
int xh_read_un_intl1024(int sys, void *to, unsigned x, unsigned y, unsigned t, unsigned dx, unsigned dy, unsigned dt);
int xh_read_normal16(int sys, void *to, unsigned x, unsigned y, unsigned t, unsigned dx, unsigned dy, unsigned dt);
int xh_read_un_interleave16(int sys, void *to, unsigned x, unsigned y, unsigned t, unsigned dx, unsigned dy, unsigned dt);
int xh_read_un_intl1024_16(int sys, void *to, unsigned x, unsigned y, unsigned t, unsigned dx, unsigned dy, unsigned dt);
int xh_read(int sys, void *to, unsigned x, unsigned y, unsigned t, unsigned dx, unsigned dy, unsigned dt, int un_interleave);

int xh_read_ave_adc(int sys, void *data, unsigned x, unsigned y, unsigned t, unsigned dx, unsigned dy, unsigned dt);
int xh_read_ave_odd_even(int sys, void *data, unsigned x, unsigned y, unsigned t, unsigned dx, unsigned dy, unsigned dt);
int xh_read_ave_adc16(int sys, void *data, unsigned x, unsigned y, unsigned t, unsigned dx, unsigned dy, unsigned dt);
int xh_read_ave_odd_even16(int sys, void *data, unsigned x, unsigned y, unsigned t, unsigned dx, unsigned dy,
		unsigned dt);
int xh_read_ave(int sys, void *data, unsigned x, unsigned y, unsigned t, unsigned dx, unsigned dy, unsigned dt,
		int mode, int mode16bit);
int xh_read_scopedata(int sys, void *to, unsigned x, unsigned y, unsigned t, unsigned dx, unsigned dy, unsigned dt);
int xh_read_mmap(int sys, int start, int num);
int xh_mem_generate(int sys, int seed);
int xh_sync_clocks(int sys);
int xh_set_xdelay(int sys, int value);
int xh_get_xdelay(int sys);
int xh_enable_counter(int sys, bool mode);
int xh_enable_pixel_ordering(int sys, bool mode);
int xh_set_readout_mode(int sys, readoutModeType mode, bool preserve);
int xh_mark_dead_pixels(int sys, int first, int num, bool reset);
// offsets
int xh_offsets_write(int sys, int first, int num, int direct, int* data);
int xh_offsets_set(int sys, int first, int num, int direct, int value);
int xh_offsets_read(int sys, int first, int num, int direct, int* data);

int xh_gains_write(int sys, int first, int num, int direct, int* data);
int xh_gains_set(int sys, int first, int num, int direct, int value);
int xh_gains_read(int sys, int first, int num, int direct, int* data);

// hv
int xh_hv_init(int sys, int ke2410_sys);
int xh_hv_set_default_cal_adc(int sys);
int xh_hv_set_default_cal_dac(int sys);
int xh_hv_get_cal_adc(int sys, int head);
int xh_hv_get_cal_dac(int sys, int head);
int xh_hv_cal(int sys, int sign, int update, double dacm, double dacc, double adchvm, double adchvc, double adc12vm,
		double adc12vc, double adc5vm, double adc5vc);
int xh_hv_dac_set(int sys, double value, int sign, bool direct, bool noslew);
int xh_hv_adc_get(int sys, int direct, int sign, XHHvADCSel which, double *adc_rb);
int xh_hv_enable(int sys, int enable, int overtemp, int forced);
char* xh_hv_summary(int sys, char* buff);
int xh_hv_update_log(int sys, double vset, double vmon, double imon, int op_stat);

// head
int xh_head_init(int sys, int head);
int xh_head_set_cal_pt100(int sys, int head, int update, unsigned char ch0, unsigned char ch1, unsigned char ch2,
			unsigned char ch3);
int xh_head_get_cal_pt100(int sys, int head);
int xh_head_update_eeprom(int sys, int head, int include_sn);
int xh_head_default_cal_adc(int sys);
int xh_head_set_cal_adc(int sys, int head, int update, double vdd_m, double vdd_c, double vref_m, double vref_c, double vrefc_m, double vrefc_c);
int xh_head_get_cal_dac(int sys, int head);
int xh_head_read_eeprom(int sys, int head, int update);
int xh_head_write_eeprom(int sys, int head, int loc, double dval, char *unlock, eepromDataType dataType);
int xh_head_set_cal(int sys, double value);
int xh_head_set_led(int sys, int head, bool mode);
int xh_head_get_led(int sys, int head);
int xh_head_dac_set(int sys, int head, double inval, bool direct, headVoltageType vType);
int xh_head_default_cal_dac(int sys, int is_xstrip);
int xh_head_set_cal_dac(int sys, int head, int update, double vddm, double vddc, double vrefm, double vrefc,
		double vrefcm, double vrefcc, double vres1m, double vres1c, double vres2m, double vres2c, double vpuprefm,
		double vpuprefc, double vclampm, double vclampc, double vledm, double vledc);
int xh_head_pexp_set(int sys, int head, pexpType pexp_val);
int xh_head_set_cal_en(int sys, int head, bool mode);
int xh_head_get_cal_en(int sys, int head);
int xh_head_adc_list(int sys, int head);
int xh_head_adc_get(int sys, int head, headVoltageType vType, double* adcValue, int raw);
int xh_head_get_cal_adc(int sys, int head);
int xh_head_get_cal_adc(int sys, int head);
char* xh_head_summary(int sys);
int xh_head_xchips_defaults(int sys, int head, int ibias_in, int vcasc, int ibias_sf, int bias_rc, int reset_ab,
		int reset_cd, int comp_ab, int comp_cd, bool clamp_p, bool clamp_n, XHX3ChipMode opt_mode);
int xh_head_xchips_set_caps(int sys, int head, int cap_ab, int cap_cd, int no_defaults);
int xh_head_xchips_get_caps(int sys, int *values, int *alternate);
int xh_head_xchips_caps(int sys, int head, capacitanceType caps);
int xh_head_send_xchip3_cr(int sys, int head, uint64_t cr, int chip_mask);
int xh_check_head(int sys, int head);
int xh_head_default_reset_rate(int cap_code);
int xh_head_set_pcb_info(int sys, int head, int update, int sn_month, int sn_yr, int sn_id,	int pcb_ver, int pcb_mod, int xtype, int perf, int loc, int heater_val, int heater_conn );
char * xh_head_xchip3_describe(int sys, int head);


// timing
int xh_default_timing(int sys, int xchip_type);
int xh_default_timing_xchip1(int sys);
int xh_default_timing_xchip2(int sys);
int xh_default_timing_xchip3(int sys);

int xh_fixed_timing(int sys, int s1_pw, int s2_pw, int s2_pw_min, int rst_min, int rst_min_no, int xclk_hp_min,
		int xclk_hp_max, int pxl0_str, int s1first, int beam_gap, int min_adc_settle, int adc_post_gap,
		int hide_xchip_glitches, int extra_readout_gap, int t2_readout, int ts2_reset, int trst_s1_fixed,
		int shift_in_to_xclk, bool reload);
int xh_timing_get_fixed(int sys, XHTimingFixed *fixed);
int xh_timing_set_fixed(int sys, XHTimingFixed *fixed, int relaxed);

int xh_timing_group_init(int sys, XHTimingGroup *group, int nframes, int nscans, int int_time);
int xh_single_group(int sys, int group_num, int nframes, int nscans, int int_time, bool islast, int group_delay,
		int frame_delay, int req_scan_period, int aux_delay, int aux_pw, int lemo_control,
		XHTriggerControlType trig_control, int long_s12, bool adjust_fr_delay, int frame_time, int cycles, int shift_down, bool end_block, int s1fine_delay,
		int s2fine_delay, int xclk_fine_delay, int rst_fine_delay_r, int rst_fine_delay_f, int trig_mux_sel,
		int orbit_mux_sel);
int xh_timing_set_group(int sys, int group_num, XHTimingGroup *group, bool islast, bool allow_excess);
int xh_timing_get_group(int sys, int group_num, XHTimingGroup *group);
int xh_timing_put_group(int sys, int group_num, XHTimingGroup *group, bool islast, bool allow_excess);

int _xh_timing_set_group(int sys, int group_num, XHTimingGroup *group, bool islast, bool allow_excess, int check_next);

int xh_timing_get_group(int sys, int group_num, XHTimingGroup *group);
int xh_timing_put_group(int sys, int group_num, XHTimingGroup *group, bool islast, bool allow_excess);

int _xh_timing_set_group(int sys, int group_num, XHTimingGroup *group, bool islast, bool allow_excess, int check_next);

int xh_timing_modify_group(int sys, int group_num, int fixed_reset, bool islast, bool allow_excess);
int xh_timing_write(int sys, int allow_excess);
int xh_timing_print(int sys, FILE * ofp, int do_fixed);
int xh_timing_bram_list(int sys, int ngroups);
int xh_bram_write(int sys, int offset, uint32_t value);
int xh_timing_trig_outputs(int sys, int op_num, XHTriggerOutputType trigout, bool invert, int pw);
int xh_timing_setup_leds(int sys, int pause_time, int frame_time, int int_time, bool led_default, bool flash_on_wait_orbit);
int xh_timing_continue(int sys);
int xh_timing_start(int sys, int start_frame);
int xh_timing_stop(int sys);
int xh_timing_wait(int sys, bool ignore_pause, bool verbose);
int xh_timing_read_status(int sys, XHStatus *status, bool verbose);
int xh_timing_orbit_setup(int sys, int delay, bool use_falling_edge);
int xh_timing_read(int sys, void *to, unsigned x, unsigned y, unsigned t, unsigned dx, unsigned dy, unsigned dt);
int xh_timing_corr_pre_post_scal(int sys, int t, double *pre, double *post);
int xh_timing_find_s1_gap(int gnum,  XHTimingGroup *group, XHTimingFixed *fixed, int xclk, int next_s1_rising);


char* xh_timing_summary(int sys);

// clocks
int xh_clock_setup(int sys, clockModeType clock_mode, int gain, int extra_div, int r3, int r4,
		int caps, bool stage1_only, bool nocheck, int over_ride_r, int over_ride_n, int over_ride_div, int less_div);

// tc
//int xh_tc_open(int sys, const char* path_name, FILE *debugfp, float *modp);
int xh_tc_open(int sys, const char* path_name, tcParamType tc_param, char* debugfname, int verbose, int debug_keep) ;
int xh_tc_open_tty(int sys, const char* path_name);
int xh_tc_close(int sys);
int xh_tc_set_setpoint(int sys, int channel, double value, int raw);
int xh_tc_set_toffset(int sys, int channel, double value);
int xh_tc_set_maxt(int sys, double value);
int xh_tc_set(int sys, int channel, tcParamType tc_param, int value);
int xh_tc_set_param(int sys, tcParamType tc_param, int value, int force);
//int xh_tc_set_param_file(int sys, char* file_name, FILE* fp);
//int xh_tc_get_param_file(int sys, char** file_name, FILE** fp);
int xh_tc_set_param_file(int sys, tcParamType tc_param, char* fname);
int xh_tc_get_param_file(int sys, tcParamType tc_param, FILE** fp);
int xh_tc_get_pid(int sys, int channel, tcParamType tc_param, int* value);
int xh_tc_get_param(int sys, int channel, tcParamType tc_param, int *value);
int xh_tc_get_setpoint(int sys, int channel, double* value);
int xh_tc_get(int sys, tcParamType tc_param, int* value);
int xh_tc_get_temp(int sys, int channel, double* value);
int xh_tc_get_protection_status(int sys, int channel, tcParamType tc_param, int* value);
int xh_tc_print(int sys, int verbose);
int xh_tc_wait_log(int sys, int n_steps, int print);

// scope
int xh_scope_xchip (int sys, int scope_size, int num_x, int num_pass, int chan, int max_scans, int num_phase, double tolerance,
		int op_row, bool do_ave, bool odd_even);
int xh_scope_set_adc(int sys, int adc);
int xh_scope_set_xchip(int sys, int sel0, int sel1);

int xh_data_timing_scan(int sys, int int_time, int xclk_hp, int first_delay, int num_delay, double vsig, int no_collect, int quarter_cycle);
int xh_timing_orbit_calibate(int sys, bool use_falling_edge, int verbose, bool fixed, int fixed_val);
int xh_timing_check_data_size(int sys, int max_data_value);
int xh_timing_corr_pre_post_scal(int sys, int t, double *pre, double *post);

int xh_inl_corr_write(int sys, int first, int num, XHINLCorrMapping chan_mapping, int16_t * data);
int xh_inl_corr_set(int sys, int first, int num, XHINLCorrMapping chan_mapping, int value);

int xh_measure_xchip_response_setup(int sys, int num_pts, int num_scans, int orbit_mux_sel, int num_quarters, int num_stability, int fixed_rstr_s1r, int npts_rst, int cycles, int scan_period, int frame_delay, int npts_hold_reset, int rst_fixed_int, int first_rstr_s1r);
int xh_measure_xchip_response_process(int sys, XHXChipProcess flags, int pulse_sep, double *weights, int row, int num_rows, int frac_a_time, int frac_b_time, float settle_fraction, int late_first_row, int min_signal, int lhs_bk_first0, int lhs_bk_last0, int lhs_bk_first1, int lhs_bk_last1, int rhs_bk_first0, int rhs_bk_last0, int rhs_bk_first1, int rhs_bk_last1, XH_XChipImpulse xh_impulse);
int xh_measure_xchip_lin_arb_response_setup(int sys, int num_hgt, int num_pts, int num_quarters, int num_stability, int fixed_rst_s1, int cycles, int scan_period);
int xh_measure_xchip_lin_response_process(int sys, int row, int num_rows, int frac_time, float settle_fraction, int late_first_row, int min_signal, XH_XChipImpulse xh_impulse);
int xh_measure_xchip_lin_beam_setup(int sys, int num_blocks, int num_frac, int first_fine, int num_fine, int nscans_fine, int first_late, int num_late, int nscans_late, int ncycles, int fixed_rst_s1, int scan_period, int orbit_mux_sel, int scan_delay);

int xh_set_invert_data(int sys, int value);
int xh_get_invert_data(int sys);
char * xh_tc_summary(int sys);
int xh_tc2_set_param(int sys, tcParamType tc_param, int value, int force);
int xh_tc2_set_chan(int sys, int channel, tcParamType tc_param, int value);
int xh_tc2_set_adc_gain(int sys, int channel, double value);
int xh_set_num_y(int sys, int num_y);
int xh_timing_orbit_read(int sys, int *delay, bool *use_falling_edge);
int xh_tc_get_temp_quick(int sys, double *value);
int xh_tc_get_power_hv(int sys, int *headpwr, int *hv);
double xh_hv_get_setpoint(int sys);

#ifdef __cplusplus
}
#endif

#endif /* XH_LIB_H_ */
