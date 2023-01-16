/*
 * xh_internal.h
 *
 *  Created on: Feb 26, 2013
 *      Author: gm
 */

#ifndef XH_INTERNAL_H_
#define XH_INTERNAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/mman.h>
#include "xh_regs.h"
#include "xh.h"
#include "aida_spi.h"


#define MAXXSTRIPSYSTEMS 2
#define XSTRIP_PATH_LEN 40

typedef struct _xstrip_head
{
	uint32_t pexp;
	uint64_t xchip3_cr;
	XHX3ChipMode opt_mode;
	enum {XHHeadInitNone=0, XHHeadInitPExp=1, XHHeadInitADC=2, XHHeadInitX3=4} init_done;
} XHHeadInfo;

typedef struct _xstrip
{
	int path;
	int valid;
	int debug;
	int num_x, num_y, num_t;
	int scope_mode, scope_x, scope_y;
	uint32_t revision;
	char name[XSTRIP_PATH_LEN+1];
	char path_name[XSTRIP_PATH_LEN+1];
	uint32_t *mmap;
	int first_adc, last_adc;
	int head_mask;
 	XHTimingFixed fixed_timing;
	XHTimingGroup *timing_groups;
	int current_group;
	int written_groups;
	uint64_t timing_clk_period;
	int pixels_per_adc;
	int interleaved;
	int xchip_num_op;
	int num_frames;
	uint32_t dma_control;
	char dead_pixel[XH_MAX_PIXELS];
	uint16_t offsets_dac[XH_NUM_CHAN];
	XHSensorType sensor_type;
	double vdd[2], vref[2], vrefc[2], vres1[2], vres2[2], vpupref[2], vclamp[2], vled[2];
	int xchip_type; // Should be 1..3 
	int xchip_perf;	// Set to 1 to enable full spped operation of Xchip3
	int bursts_per_frame;
	XHHeadInfo head[XH_NUM_HEAD_PCB];
	int head_serial;	// Head serial number to allow different Xchip* optimistations -- may move to eeprom.
} XStripSysDesc;

// this is the struct for the hv
typedef struct {

	double hvsetpoint;
	double lasthvmon;
	int lasthvmondirect;
	double last12Vmon;
	double last5Vmon;
	int last12Vmon_direct;
	int last5Vmon_direct;
	bool setpoint_unknown;
	int ke2410_sys;		//!< System number for keithely 2410, -1 to use internal PSU.
	double lastimon;
	int ke2410_op_stat;
} hv_info;

// this is the structure for the temp controller
typedef struct {
	int ocp;
	int scp;
	int otp;
	double t;
	int p;
	int i;
	int d;
	int gscale;
	double setpoint;
	int dacmsb;
	int daclsb;
	int gain;
	double t1cal_user;
	double t1cal_reading;
	int t1flag;
	double t2cal_user;
	double t2cal_reading;
	int t2flag;
	double fb;
	double pid_output;
	double pid_i;
	double pid_p;
	double toffset;
	int pid_y; // pwm control effort
	double pid_i_dbl; //integral action term
	double pid_y_dbl; //??
	int feedback; // adc readback (minus the toffset)
	double error; // in adc codes
	int omt;
	double adc_gain;
	int pt100_fault;
	double ave_t;
	int num_summed;
	double soft_overtemp_thres;
	int soft_overtemp;
	int num_overtemp;
} channel_params;

typedef struct
{
	channel_params chp[4];
	int pwm;
	int autoinc;
	int mode;
	int overtempprotect, shortcircuitprotect, opencircuitprotect;
	XH_HVState hv; 
	int headpwr;
//	int cn;
//	char sys_name[XSTRIP_PATH_LEN+1];
	int tty;
	pthread_t rx_thread;
	int rx_thread_ok;
	int quit;
	float *fptr;   // image_ave_mod->data
//	MOD_IMAGE *image_ave_mod;
//	mh_com *image_ave_head;
	int logcnt;
	//FILE *logallfp;
	FILE *logtfp;
	FILE *debugfp;
	int debug_con;
	unsigned long ullogtlen;
	//unsigned long ullogalllen;
	unsigned long uldebuglen;
	unsigned int ulogdebugflushcnt;
	unsigned int ulogtflushcnt;
	//unsigned int ulogallflushcnt;
	//char *logallname;//[200];
	char *logtname;//[200];
	char *debugname; // name of the debug file
	time_t starttime;
	int remaintime;
	int status;
	double maxt;
	int version;
	int nstatus;
	int npwm, nautoinc, nmode, ndone, notp, nscp, nocp, nsetpoint, nmaxt, npgain, nigain, ndgain, ntemp[4], nheadpwr, nhv, nadcgain;
	int maxtp;	// maximum temperature protection trip
	int logt_keep;	// Keep log files rather than rolling names
	int debug_keep;	// Keep debug files rather than rolling names
	int chan_data_valid;
	int num_ave;
	pthread_t hv_ramp_thread;
	enum {HVRamp_Off, HVRamping, HVRampJoin} hv_ramping_state;
} tc_info;

// this is the structure for the eeprom
typedef struct {
	unsigned char sn_month; 			//0
	unsigned char sn_yr;				//1
	unsigned short int sn_id;			//2
	unsigned char pcb_ver;				//4
	unsigned char pcb_mod;				//5
	unsigned char xtype;				//6
	unsigned char perf;					//7
	unsigned char loc;					//8
	unsigned char heater_val;			//9
	unsigned char heater_conn;			//10
	unsigned char reserved0;
	unsigned char reserved1;
	unsigned char reserved2;
	unsigned char reserved3;
	unsigned char reserved4;
	float vdd_m;
	float vdd_c;
	float vref_m;
	float vref_c;
	float vrefc_m;
	float vrefc_c;
	float vres1_m;
	float vres1_c;
	float vres2_m;
	float vres2_c;
	float vpupref_m;
	float vpupref_c;
	float vclamp_m;
	float vclamp_c;
	float vled_m;
	float vled_c;
	float vddmon_m;
	float vddmon_c;
	float vrefmon_m;
	float vrefmon_c;
	float vrefcmon_m;
	float vrefcmon_c;
	float pos_hv_m;
	float pos_hv_c;
	float pos_hvmon_m;
	float pos_hvmon_c;
	float pos_v5mon_m;
	float pos_v5mon_c;
	float pos_v12mon_m;
	float pos_v12mon_c;
	float neg_hv_m;
	float neg_hv_c;
	float neg_hvmon_m;
	float neg_hvmon_c;
	float neg_v5mon_m;
	float neg_v5mon_c;
	float neg_v12mon_m;
	float neg_v12mon_c;
	unsigned char pt100_c [4];

} head_eeprom_data;

// TC autoinc delay ~ 4sec, 48 hrs of logging = 24hrs*60mins*(60/4)*4chans =
// Y=8 channels = ch0,1,2,3
#if 1
#define TC_LOG_X_NORMAL (5*60*15*3)
#define TC_LOG_Y_NORMAL 5
#define TC_LOG_X_DETAILS (5*60*15)
#define TC_LOG_Y_DETAILS 15
#else
/* Short versions for test wrap round */
#define TC_LOG_X_NORMAL (5*15*3)
#define TC_LOG_Y_NORMAL 5
#define TC_LOG_X_DETAILS (5*15)
#define TC_LOG_Y_DETAILS 15
#endif

#define TC_SLEEP_DEFAULT_SECS (int)2  		// default delay to wait for tc to respond
#define TC_SLEEP_STATUS_SECS (int)3			//
#define TC_SLEEP_HEADPWR_UP_SECS (int)3		// if we have powered up the head add an extra second for the head power to stabilise
#define TC_SLEEP_RESET_SECS (int)11			// resetting the TC takes a long time

#define TC_LOGFILE_SIZE 5000000 // threshold above which a new log file is generated
//#define TC_LOGFILE_SIZE 5000 // threshold above which a new log file is generated
//#define TC_LOGFILE_TIME 100 // threshold above which a new log file is generated
#define TC_LOGFILE_FLUSH_SIZE  300// number of chars above which wwe will issue a flush file.
//#define TC_LOGFILE_FLUSH_TIME  5// number of seconds to have elapsed before we will issue a flush file.
#define TC_LOGFILE_FILES_TO_KEEP 10 // Nnumber of logfiles to keep on the system

#define HV_STEP 1.0   // 1V
#define HV_WAIT_US 500000   // 500us delay

//m and c for HV control dacs
#define HV_NEG_DACV_M -0.002129
#define HV_NEG_DACV_C 5.30478
#define HV_POS_DACV_M 0.002081
#define HV_POS_DACV_C 1.642262
// m and c for the adc readback of hv supplies
#define HV_NEG_VMON_M  	-0.05884
#define HV_NEG_VMON_C  	-4.231
#define HV_NEG_12V_M  	(12.066/3412.0)
#define HV_NEG_12V_C	0.0
#define HV_NEG_5V2_M  	(5.2515/3547.0)
#define HV_NEG_5V2_C  	0.0
#define HV_POS_VMON_M  	0.0341
#define HV_POS_VMON_C  	-5.68
#define HV_POS_12V_M  	(12.066/3412.0)
#define HV_POS_12V_C	0.0
#define HV_POS_5V2_M  	(5.2515/3547.0)
#define HV_POS_5V2_C  	0.0



#define XSTRIP_NOISE_MEAN  		0
#define XSTRIP_NOISE_STDEV 		1
#define XSTRIP_NOISE_MIN   		2
#define XSTRIP_NOISE_MAX   		3
#define XSTRIP_NOISE_CODE_SPREAD 4
#define XSTRIP_NOISE_FW10  		5
#define XSTRIP_NOISE_FW100  		6
#define XSTRIP_NOISE_FW1000  	7
#define XSTRIP_NOISE_FW10000 	8

#define XSTRIP_NOISE_NUM_ROWS 	9
#define XSTRIP_AVE_MOD_FLOAT 2
typedef double XHFloatAve;


#endif /* XH_INTERNAL_H_ */
