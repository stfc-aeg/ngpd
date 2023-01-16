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
#include "datamod.h"

#define MAXXSTRIPSYSTEMS 2
#define XSTRIP_PATH_LEN 40

typedef struct _xstrip_head
{
	uint32_t pexp;
	uint64_t xchip3_cr;
	int opt_speed;
	enum {XHHeadInitNone=0, XHHeadInitPExp=1, XHHeadInitADC=2, XHHeadInitX3=4} init_done;
} XHHeadInfo;

typedef enum  {
	ULTRA_HEAD_SILICON = 0,
	ULTRA_HEAD_INGAAS  = 1,
	ULTRA_HEAD_MCT     = 2,
	ULTRA_HEAD_MCT_512 = 3,
	ULTRA_HEAD_GAAS_POS = 4,
	ULTRA_HEAD_GAAS_NEG = 5 } HeadType;


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
} channel_params;

typedef struct
{
	channel_params chp[4];
	int pwm;
	int autoinc;
	int mode;
	int overtempprotect;
	int hv; // 0 =XH1024=-ve HV, 1=XStrip1024=+ve hv
	int headpwr;
	pthread_t rx_thread;
	int rx_thread_ok;
	int quit;
	float *fptr;   // image_ave_mod->data
	int logcnt;
	//FILE *logallfp;
	FILE *logtfp;
	unsigned long ullogtlen;
	unsigned int ulogtflushcnt;
	char *logtname;//[200];
	time_t starttime;
	int remaintime;
	int status;
	double maxt;
	MOD_IMAGE *image_ave_mod;
	mh_com *image_ave_head;
} tc_info;

// this is the structure for the eeprom
typedef struct {
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
} HeadCalData;


typedef struct _xstrip
{
	int valid;
	int debug;
	int num_x, num_y, num_t;
	int num_t_by_1;		// Num_t in default num_y==1 configuration */
	int scope_mode;
	u_int32_t dma_control;
	u_int32_t *mmap;
	uint32_t revision;
	char name[XSTRIP_PATH_LEN+1];
	char host_address[XSTRIP_PATH_LEN+1];
	char head_address[XSTRIP_PATH_LEN+1];
	int head_port;
	int sock_tcp;
	int sock_udp;
	MOD_IMAGE *image_mod;
	mh_com *image_head;
	u_int32_t *image_data;
	MOD_IMAGE *frame_num_mod;
	mh_com *frame_num_head;
	u_int32_t *frame_num_data;
	char dead_pixel[ULTRA_NUMPIXELS];

 	XHTimingFixed fixed_timing;
	XHTimingGroup *timing_groups;
	int current_group;
	int written_groups;
	uint64_t timing_clk_period;
	int pixels_per_adc;
	int interleaved;
	int xchip_num_op;
	int num_frames;
	HeadType head_type;
	int xchip_type; // Should be 1..3 
	int xchip_perf;
	int bursts_per_frame;
	XHHeadInfo head[XH_NUM_HEAD_PCB];
	int head_serial;	// Head serial number to allow different Xchip* optimistations -- may move to eeprom.
	pthread_mutex_t mutex;
	pthread_mutex_t cmd_mutex;
	pthread_t udp_thread;
	volatile int acq_cycle, acq_group, acq_frame, acq_scan, op_frame;
	volatile XHState collect;
	pthread_mutex_t fpga_regs_mutex;
	volatile int rst_delay, rst_pw, s1_delay, s1_pw, s2_delay, s2_pw, shiftin_delay, xclk_half_period, adc_settling_time;
	tc_info tc;
	pthread_mutex_t mutextc;
	HeadCalData head_cal_data;

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

} hv_info;


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
#define TC_LOG_X 5*60*15
#define TC_LOG_Y 4
#define TC_SLEEP_DEFAULT_SECS (int)2  		// default delay to wairt for tc to respond
#define TC_SLEEP_STATUS_SECS (int)3			//
#define TC_SLEEP_HEADPWR_UP_SECS (int)3		// if we have powered up the head add an extra second for the head power to stabilise
#define TC_SLEEP_RESET_SECS (int)11			// resetting the TC takes a long time

#define TC_LOGFILE_SIZE 5000000 // threshold above which a new log file is generated
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

extern XStripSysDesc xstrip_sys[MAXXSTRIPSYSTEMS];

#endif /* XH_INTERNAL_H_ */
