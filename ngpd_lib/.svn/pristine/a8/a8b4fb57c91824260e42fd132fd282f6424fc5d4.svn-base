/* AMS system monitor support for ZynqMP
	For a zynmp there are monitors for both the PS and PL side of the FPGA.
	For the binary protocol to libngpd, suggest that this code works in integer mV and integer mili degC

*/


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>
#include <assert.h>
#include <stdbool.h>
#include "ngpd.h"
/*
 * events.h is suppose to be part of cross compile tool's linux includes
 * But as its under development, the file is in ../linux_include
 * once it is in the main, once should include
 * #include <linux/iio/events.h>
 * This file should be always in sync with <kernel>/include/linux/iio/events.h
 *
 */
/*#include <events.h> */

#include "zynqmp_xadc.h"


#define MULTIPLIER_12_BIT

#ifdef MULTIPLIER_12_BIT
	static const int multiplier = 1 << 12;
#elif MULTIPLIER_16_BIT
	static const int multiplier = 1 << 16;
#endif
	static const int mV_mul = 1000;
typedef struct 
{
	char node_name[MAX_NAME_SIZE];
	enum SysMonType type;
	struct XadcChan
	{
		int offset;
		double scale;
	} chan[AMSParamMax];
} SysMon;


static SysMon *sys_mon[ZYNQMP_NUM_XADC];


//function prototype
static int conv_temperature(struct XadcChan *chan, int input, enum EConvType conv_direction);
static int conv_voltage(struct XadcChan *chan, int input, enum EConvType conv_direction);
static int conv_voltage_ext_ch(struct XadcChan *chan, int input, enum EConvType conv_direction);
static int conv_from_files(struct XadcChan *chan, int input, enum EConvType conv_direction);

static int line_from_file(char* filename, char* linebuf);
static int line_to_file(char* filename, char* linebuf);

pthread_t gEvent_handler;
static bool gExitNow = false;
static enum XADC_Init_Type gInit_state = ZYNQMP_XADC_NOT_INITIALIZED;
/*
dev                              in_voltage0_vcc_pspll0_scale     in_voltage15_psmgtravcc_scale    in_voltage20_vccvrefp_scale      in_voltage26_vccams_scale        in_voltage7_vccpsintlp_scale
events                           in_voltage10_vccpsddr_raw        in_voltage16_psmgtravtt_raw      in_voltage21_vccvrefn_raw        in_voltage2_vccint_raw           in_voltage8_vccpsintfp_raw
in_temp0_ps_temp_offset          in_voltage10_vccpsddr_scale      in_voltage16_psmgtravtt_scale    in_voltage21_vccvrefn_scale      in_voltage2_vccint_scale         in_voltage8_vccpsintfp_scale
                                 in_voltage11_vccpsio3_raw        in_voltage17_vccams_raw          in_voltage22_vccbram_raw         in_voltage3_vccbram_raw          in_voltage9_vccpsaux_raw
in_temp0_ps_temp_scale           in_voltage11_vccpsio3_scale      in_voltage17_vccams_scale        in_voltage22_vccbram_scale       in_voltage3_vccbram_scale        in_voltage9_vccpsaux_scale
in_temp1_remote_temp_offset      in_voltage12_vccpsio0_raw        in_voltage18_vccint_raw          in_voltage23_vccplintlp_raw      in_voltage4_vccaux_raw           name
in_temp1_remote_temp_raw         in_voltage12_vccpsio0_scale      in_voltage18_vccint_scale        in_voltage23_vccplintlp_scale    in_voltage4_vccaux_scale         of_node
in_temp1_remote_temp_scale       in_voltage13_vccpsio1_raw        in_voltage19_vccaux_raw          in_voltage24_vccplintfp_raw      in_voltage5_vcc_psddrpll_raw     power
in_temp2_pl_temp_offset          in_voltage13_vccpsio1_scale      in_voltage19_vccaux_scale        in_voltage24_vccplintfp_scale    in_voltage5_vcc_psddrpll_scale   sampling_frequency
in_temp2_pl_temp_raw             in_voltage14_vccpsio2_raw        in_voltage1_vcc_psbatt_raw       in_voltage25_vccplaux_raw        in_voltage6_vccpsintfpddr_raw    subsystem
in_temp2_pl_temp_scale           in_voltage14_vccpsio2_scale      in_voltage1_vcc_psbatt_scale     in_voltage25_vccplaux_scale      in_voltage6_vccpsintfpddr_scale  uevent
in_voltage0_vcc_pspll0_raw       in_voltage15_psmgtravcc_raw      in_voltage20_vccvrefp_raw        in_voltage26_vccams_raw          in_voltage7_vccpsintlp_raw


With ams_pl disabled 
dev                              in_voltage0_vcc_pspll0_scale     in_voltage14_vccpsio2_raw        in_voltage1_vcc_psbatt_scale     in_voltage6_vccpsintfpddr_raw    of_node
events                           in_voltage10_vccpsddr_raw        in_voltage14_vccpsio2_scale      in_voltage2_vccint_raw           in_voltage6_vccpsintfpddr_scale  power
in_temp0_ps_temp_offset          in_voltage10_vccpsddr_scale      in_voltage15_psmgtravcc_raw      in_voltage2_vccint_scale         in_voltage7_vccpsintlp_raw       sampling_frequency
in_temp0_ps_temp_raw             in_voltage11_vccpsio3_raw        in_voltage15_psmgtravcc_scale    in_voltage3_vccbram_raw          in_voltage7_vccpsintlp_scale     subsystem
in_temp0_ps_temp_scale           in_voltage11_vccpsio3_scale      in_voltage16_psmgtravtt_raw      in_voltage3_vccbram_scale        in_voltage8_vccpsintfp_raw       uevent
in_temp1_remote_temp_offset      in_voltage12_vccpsio0_raw        in_voltage16_psmgtravtt_scale    in_voltage4_vccaux_raw           in_voltage8_vccpsintfp_scale
in_temp1_remote_temp_raw         in_voltage12_vccpsio0_scale      in_voltage17_vccams_raw          in_voltage4_vccaux_scale         in_voltage9_vccpsaux_raw
in_temp1_remote_temp_scale       in_voltage13_vccpsio1_raw        in_voltage17_vccams_scale        in_voltage5_vcc_psddrpll_raw     in_voltage9_vccpsaux_scale
in_voltage0_vcc_pspll0_raw       in_voltage13_vccpsio1_scale      in_voltage1_vcc_psbatt_raw       in_voltage5_vcc_psddrpll_scale   name
*/

struct XadcParameter
{
	const char name[MAX_NAME_SIZE];
	int (* const conv_fn)(struct XadcChan *chan, int , enum EConvType);
};

struct XadcParameter gXadcData[XADCParamMax] = {
	[XADCParamVccInt]  = { "in_voltage0_vccint", 	conv_voltage},
	[XADCParamVccAux]  = { "in_voltage1_vccaux", 	conv_voltage},
	[XADCParamVccBRam] = { "in_voltage2_vccbram", 	conv_voltage},
	[XADCParamVccPInt] = { "in_voltage3_vccpint", 	conv_voltage},	// PS7 Processor VccInt
	[XADCParamVccPAux] = { "in_voltage4_vccpaux", 	conv_voltage}, 	// PS7 Processor Vccaux
	[XADCParamVccoDdr] = { "in_voltage5_vccoddr",	conv_voltage}, 	// PS7 Vccoddr
	[XADCParamTemp]    = { "in_temp0", 				conv_temperature},
	[XADCParamVAux0]   = { "in_voltage8",			conv_voltage_ext_ch},
};	
struct XadcParameter gAMSData[AMSParamMax] = {
	[AMSParamPSTempLPD]		= { "in_temp0_ps_temp", 		conv_from_files },
	[AMSParamPSTempFPD]   	= { "in_temp1_remote_temp", 	conv_from_files},
	[AMSParamPSVccIntLP]   	= { "in_voltage7_vccpsintlp",	conv_from_files},
	[AMSParamPSVccIntFP]   	= { "in_voltage8_vccpsintfp",	conv_from_files},
	[AMSParamPSVccAux]   	= { "in_voltage9_vccpsaux",		conv_from_files},
	[AMSParamPSVccDDR]   	= { "in_voltage10_vccpsddr",	conv_from_files},
	[AMSParamPSVccIO0]   	= { "in_voltage12_vccpsio0",	conv_from_files},
	[AMSParamPSVccIO1]   	= { "in_voltage13_vccpsio1",	conv_from_files},
	[AMSParamPSVccIO2]   	= { "in_voltage14_vccpsio2",	conv_from_files},
	[AMSParamPSVccIO3]   	= { "in_voltage11_vccpsio3",	conv_from_files},
	[AMSParamPSAVccMGTR]   	= { "in_voltage15_psmgtravcc",	conv_from_files},
	[AMSParamPSAVttMGTR]   	= { "in_voltage16_psmgtravtt",	conv_from_files},
	[AMSParamPSVccAMS]   	= { "in_voltage17_vccams",		conv_from_files},
	[AMSParamPSVccPLL0]   	= { "in_voltage0_vcc_pspll0",	conv_from_files},
	[AMSParamPSVccBatt]   	= { "in_voltage1_vcc_psbatt",	conv_from_files},
	[AMSParamPLVccInt]  	= { "in_voltage2_vccint",		conv_from_files},
	[AMSParamPLVccBRAM]    	= { "in_voltage3_vccbram",		conv_from_files},
	[AMSParamPLVccAux]      = { "in_voltage4_vccaux",		conv_from_files},
	[AMSParamPSVccDDRPLL]   = { "in_voltage5_vcc_psddrpll",	conv_from_files},
	[AMSParamPSVccIntFPDDR] = { "in_voltage6_vccpsintfpddr",conv_from_files} 
};


#if 0
struct XadcThreshold gXadcAlarm[EAlarmMAX] = {
		[EAlarmVccInt_TH] = { EAlarmVccInt_TH, 	"in_voltage0_vccint_thresh",  	0, 	0, conv_voltage, 		false,(struct Xadc_callback){0,0}, NULL},
		[EAlarmVccAux_TH] = { EAlarmVccAux_TH, 	"in_voltage4_vccpaux_thresh", 	0, 	0, conv_voltage, 		false,(struct Xadc_callback){0,0}, NULL},	// todo: workaround
		[EAlarmVccBRam_TH]= { EAlarmVccBRam_TH, "in_voltage2_vccbram_thresh", 	0, 	0, conv_voltage,		false,(struct Xadc_callback){0,0}, NULL},
		[EAlarmTemp_TH]   = { EAlarmTemp_TH, 	"in_temp0_thresh", 				0, 	0, conv_temperature,	false,(struct Xadc_callback){0,0}, NULL},
};
#endif


int zynqmp_read_xadc_param(int xadc_num, int pnum, int *value)
{
	char filename[MAX_PATH_SIZE];
	char read_value[MAX_VALUE_SIZE];
	struct XadcParameter *param;
	
	if (xadc_num < 0 || xadc_num >= ZYNQMP_NUM_XADC)
	{
		printf("zynqmp_read_xadc_param: xadc_num=%d out of range 0..%d\n", xadc_num, ZYNQMP_NUM_XADC-1);
		return -1;
	}
	if (sys_mon[xadc_num] == NULL)
	{
		printf("zynqmp_read_xadc_param: xadc_num=%d system was not found at init\n", xadc_num);
		return -1;
	}
	switch(sys_mon[xadc_num]->type)
	{
	case SysMonXADC:
		if (pnum < 0 || pnum >= XADCParamMax)
		{
			printf("zynqmp_read_xadc_param: Expected parameter number to be in range 0..%d-1, not %d\n", XADCParamMax, pnum);
			return -1;
		}
		param = gXadcData+pnum;
		break;
		
	case SysMonAMS:
		if (pnum < 0 || pnum >= AMSParamMax)
		{
			printf("zynqmp_read_xadc_param: Expected parameter number to be in range 0..%d-1, not %d\n", AMSParamMax, pnum);
			return -1;
		}
		param = gAMSData+pnum;
		break;

	default:
		printf("Unknown or unitialiased xadc_num=%d, sys_mon_type=%d\n", xadc_num, sys_mon[xadc_num]->type);
		return -1;
	}
	memset(filename, 0, sizeof(filename) );

	sprintf(filename, "%s/%s/%s_raw", SYS_PATH_IIO, sys_mon[xadc_num]->node_name, param->name );

	if (line_from_file(filename, read_value) == 0)
	{
		*value = param->conv_fn(sys_mon[xadc_num]->chan+pnum, strtol(read_value, NULL, 0), EConvType_Raw_to_Scale);
	}
	else
	{
		printf("\n***Error: reading file %s\n",filename);
		return -1;
	}

	return 0;
}

int zynqmp_write_xadc_threshold(struct XadcThreshold *param, int xadc_num, int thres_low, int thres_high)
{
	char filename[MAX_PATH_SIZE];
	char write_value[MAX_VALUE_SIZE];
	int value;
	
	if (xadc_num < 0 || xadc_num >= ZYNQMP_NUM_XADC)
	{
		printf("zynqmp_write_xadc_threshold: xadc_num=%d out of range 0..%d\n", xadc_num, ZYNQMP_NUM_XADC-1);
		return -1;
	}
	if (sys_mon[xadc_num] == NULL)
	{
		printf("zynqmp_write_xadc_threshold: xadc_num=%d system was not found at init\n", xadc_num);
		return -1;
	}

	memset(filename, 0, sizeof(filename));
	memset(write_value, 0, sizeof(write_value));

	// set the high threshold
	sprintf(filename, "%s/%s/events/%s_rising_value", SYS_PATH_IIO, sys_mon[xadc_num]->node_name, param->name );
	value = param->conv_fn(thres_high, EConvType_Scale_to_Raw);
	sprintf(write_value, "%d", value);
	if (line_to_file(filename, write_value) != 0) {
		perror("Error writing high threshold");
	}

	if (param->id != EAlarmTemp_TH) 
	{
		// set the low threshold
		sprintf(filename, "%s/%s/events/%s_falling_value", SYS_PATH_IIO, sys_mon[xadc_num]->node_name, param->name );
		value = param->conv_fn(thres_low, EConvType_Scale_to_Raw);
		sprintf(write_value, "%d", value);
		if (line_to_file(filename, write_value) != 0)
		{
			perror("Error writing low threshold");
		}

	} 
	else
	{
		// set the Hysteresis for temperature threshold
		sprintf(filename, "%s/%s/events/%s_rising_hysteresis", SYS_PATH_IIO, sys_mon[xadc_num]->node_name, param->name );
		value = param->conv_fn(thres_high=thres_low, EConvType_Scale_to_Raw);
		sprintf(write_value, "%d", value);
		if (line_to_file(filename, write_value) != 0)
		{
			perror("Error writing Hysteresis value");
		}
	}

	if (param->id != EAlarmTemp_TH)
	{
		// Enable the alarm threshold low
		sprintf(filename, "%s/%s/events/%s_falling_en", SYS_PATH_IIO, sys_mon[xadc_num]->node_name, param->name );
		if (line_to_file(filename, "1") != 0) {
			perror("Error Enable low threshold");
		}
	}

	// Enable the alarm threshold high
	sprintf(filename, "%s/%s/events/%s_rising_en", SYS_PATH_IIO, sys_mon[xadc_num]->node_name, param->name );
	if (line_to_file(filename, "1") != 0)
	{
		perror("Error Enabling high threshold");
	}

	return 0;
}

static int read_offset_scale(int xadc_num)
{
	char filename[MAX_PATH_SIZE];
	char read_value[MAX_VALUE_SIZE];
	struct XadcParameter *param;
	int max_pnum;
	int pnum;
	
	if (xadc_num < 0 || xadc_num >= ZYNQMP_NUM_XADC)
	{
		printf("read_offset_scale: xadc_num=%d out of range 0..%d\n", xadc_num, ZYNQMP_NUM_XADC-1);
		return -1;
	}
	if (sys_mon[xadc_num] == NULL)
	{
		printf("read_offset_scale: xadc_num=%d system was not found at init\n", xadc_num);
		return -1;
	}
	switch(sys_mon[xadc_num]->type)
	{
	case SysMonXADC:
		max_pnum = XADCParamMax;
		param = gXadcData;
		break;
		
	case SysMonAMS:
		max_pnum = AMSParamMax;
		param = gAMSData;
		break;

	default:
		printf("Unknown or unitialiased xadc_num=%d, sys_mon_type=%d\n", xadc_num, sys_mon[xadc_num]->type);
		return -1;
	}
	for (pnum=0; pnum<max_pnum; pnum++)
	{
		sprintf(filename, "%s/%s/%s_offset", SYS_PATH_IIO, sys_mon[xadc_num]->node_name, param->name );

		if (line_from_file(filename, read_value) == 0)
		{
			sys_mon[xadc_num]->chan[pnum].offset = strtol(read_value, NULL, 0);
		}
		else
			sys_mon[xadc_num]->chan[pnum].offset = 0;

		sprintf(filename, "%s/%s/%s_scale", SYS_PATH_IIO, sys_mon[xadc_num]->node_name, param->name );

		if (line_from_file(filename, read_value) == 0)
		{
			sys_mon[xadc_num]->chan[pnum].scale = strtod(read_value, NULL);
		}
		else
		{
			printf("\n***Error: reading file %s\n",filename);
			return -1;
		}
		param++;
	}
	return 0;
}
	

static int zynqmp_find_iio_nodes(const char ** deviceName)
{
	struct dirent **namelist;
	char file[MAX_PATH_SIZE];
	char name[MAX_NAME_SIZE];
	char of_name[MAX_NAME_SIZE];
	int i,n;
	int dev_num;
	enum SysMonType mon_type;
	int found_device;
	int num_found = 0;
	
	n = scandir(SYS_PATH_IIO, &namelist, 0, alphasort);
	if (n <= 0)
	{
		printf("Found no IIO devices in %s\n", SYS_PATH_IIO);
		return -1;
	}
	for (i=0; i < n; i++)
	{
		sprintf(file, "%s/%s/name", SYS_PATH_IIO, namelist[i]->d_name);
		printf("Trying %s\n", file);
		if ((line_from_file(file, name) == 0) && (strcmp(name, "ams") == 0 || strcmp(name, "xadc") == 0))
		{
			if (strcmp(name, "ams") == 0 )
				mon_type = SysMonAMS;
			else
				mon_type = SysMonXADC;
			
			sprintf(file, "%s/%s/of_node/name", SYS_PATH_IIO, namelist[i]->d_name);
			if (line_from_file(file, of_name) != 0)
			{
				printf("Cannot read of_name from file %s\n", file);
			}
			else
			{
				found_device = 0;
				for (dev_num=0; dev_num <  ZYNQMP_NUM_XADC  && deviceName[dev_num] != NULL; dev_num++)
				{
					if (strcmp(of_name, deviceName[dev_num]) == 0)
					{
						sys_mon[dev_num] = (SysMon *)malloc(sizeof(SysMon));
						if (sys_mon[dev_num] == NULL)
						{
							printf("zynqmp_find_iio_nodes: Out of memory\n");
							return -1;
						}
						memset(sys_mon[dev_num], 0, sizeof(SysMon));
						sys_mon[dev_num]->type = mon_type;
						strcpy(sys_mon[dev_num]->node_name, namelist[i]->d_name);
						read_offset_scale(dev_num);
						found_device = 1;
						num_found++;
					}
				}
				if (found_device == 0)
				{
					printf("Device %s of_name=%s is not present in device name list, ignoring\n", namelist[i]->d_name, of_name);
				}
			}
		}
		free(namelist[i]);
	}
	free(namelist);
	 
	return num_found;
}

#if 0
static void do_event_action(enum iio_event_type ev_type, enum iio_chan_type ch_type, enum iio_event_direction ev_dir, int channel_number)
{

	enum XADC_Alarm alarm_id = EAlarmMAX;
	bool isActive = false;
	bool isEventValid = false;

	switch(ev_type)
	{
	case IIO_EV_TYPE_THRESH:
		isActive = true;
		isEventValid = true;
		break;
	case IIO_EV_TYPE_THRESH_NOT_ACTIVE:
		isActive = false;
		isEventValid = true;
		break;
	default:
		//not a valid event to handle do nothing.
		perror("Unknown event Type !!\n");
		isEventValid = false;
		return;
	}

	// sanity check for the direction:
	switch(ev_dir)
	{
	case IIO_EV_DIR_EITHER:
	case IIO_EV_DIR_RISING:
	case IIO_EV_DIR_FALLING:
		// do Nothing; as this is just sanity check.
		break;
	default:
		//not a valid event to handle do nothing.
		perror("Unknown event direction !!\n");
		isEventValid = false;
		return;
	}

	switch(ch_type)
	{
	case IIO_VOLTAGE:
		switch(channel_number)
		{
		case 0:
			alarm_id = EAlarmVccInt_TH;
			break;
		case 4:
			alarm_id = EAlarmVccAux_TH;
			break;
		case 2:
			alarm_id = EAlarmVccBRam_TH;
			break;
		case 8:
		default:
			printf("Voltage Threshold for this channel (%d) not supported!\n",channel_number);
			isEventValid = false;
			return;
		}
		break;
	case IIO_TEMP:
		if(channel_number == 0)
		{
			alarm_id = EAlarmTemp_TH;
		}
		else
		{
			printf("Temperature Threshold for this channel (%d) not supported!\n",channel_number);
			isEventValid = false;
			return;
		}
		break;
	default:
		//not a valid event to handle do nothing.
		perror("Unknown event!\n");
		isEventValid = false;
		return;
	}

	assert(isEventValid); //just sanity check... will never reach here for invalid event
	assert(alarm_id >= 0 && alarm_id < EAlarmMAX);

	pthread_mutex_lock(gXadcAlarm[alarm_id].lock);

	gXadcAlarm[alarm_id].isActive = isActive;

	//todo: Important... currently the function being called is in mutex lock.
	//      it shouldn't fail.
	//todo: callback can be called out side mutex.

	if (gXadcAlarm[alarm_id].callback.func)
	{
		gXadcAlarm[alarm_id].callback.func(gXadcAlarm[alarm_id].callback.arg);
	}

	pthread_mutex_unlock(gXadcAlarm[alarm_id].lock);
}

static void * thread_event_handler(void* args)
{
	int fd, event_fd;
	char device_file[MAX_PATH_SIZE];
	struct iio_event_data event;
	enum iio_event_type ev_type;
	enum iio_chan_type ch_type;
	enum iio_event_direction ev_dir;
	int channel_number;

	int ret = 0;

	if (sprintf(device_file, "/dev/%s", zynqmp_xadc_node) < 0)
	{
		perror("Error:Out of Memory");
		ret = 1;
		goto ERR_OUT;
	}

	if ((fd = open(device_file,O_RDONLY)) == -1)
	{
		perror("Failed to open device file");
		ret = 2;
		goto ERR_OUT;
	}

	//get the event_fd

	if (ioctl(fd, IIO_GET_EVENT_FD_IOCTL, &event_fd) == -1)
	{
		perror("IOCTL failed on device file");
		ret = 3;
		goto ERR_OUT;
	}

	close (fd);

	if(event_fd == -1)
	{
		perror("Couldn't obtain event file descriptor\n");
		ret = 3;
		goto ERR_OUT;
	}

	while (gExitNow == false)
	{
		ret=read(event_fd, &event, sizeof(event));
		if(ret == -1)
		{
			if(errno == EAGAIN)
			{
				printf("Nothing available\n");
				continue;
			}
			else
			{
				perror("Failed to read event from device");
				ret = -errno;
				break;
			}
		}
		// Parsing event
		ev_type = IIO_EVENT_CODE_EXTRACT_TYPE(event.id);
		ch_type = IIO_EVENT_CODE_EXTRACT_CHAN_TYPE(event.id);
		ev_dir = IIO_EVENT_CODE_EXTRACT_DIR(event.id);
		channel_number = IIO_EVENT_CODE_EXTRACT_CHAN(event.id);

		do_event_action(ev_type,ch_type,ev_dir,channel_number);
	}

	close (event_fd);

	return NULL;

ERR_OUT:
	exit(ret);
}

static void deinit_mutexes(int max_index)	// currently only for alarm; todo: modify it to handle for the parameters lock as well.
{
	if(max_index > 0)
	{
		while (max_index--)
		{
			assert(gXadcAlarm[max_index].lock != NULL);
			pthread_mutex_destroy(gXadcAlarm[max_index].lock);
			free(gXadcAlarm[max_index].lock);
			gXadcAlarm[max_index].lock = NULL;
		}
	}
	else
	{
		assert(0);
	}
}

static int init_mutexes(void)	// currently only for alarm modify it to handle for the parameters lock as well.
{
	int i;
	int ret = 0;
	for(i=0; i< EAlarmMAX; i++)
	{
		assert(gXadcAlarm[i].lock == NULL);
		gXadcAlarm[i].lock = malloc(sizeof(pthread_mutex_t));
		if (pthread_mutex_init(gXadcAlarm[i].lock, NULL) != 0)
		{
			perror("mutex init failed\n");
			ret = -1;
			/*
			 * unroll the mutext initializaton
			 */
			free(gXadcAlarm[i].lock);
			deinit_mutexes(i);
			break;
		}
	}
	return ret;
}
#endif

int zynqmp_xadc_init(enum XADC_Init_Type init_type)
{
	int ret = 0;
	gExitNow = false;	//just reassuring
	static int zynqmp_num_xadc;
	const char *device_names[] = {"ams", "xadc", NULL};
	if (gInit_state != ZYNQMP_XADC_NOT_INITIALIZED)
		return zynqmp_num_xadc;

	if (init_type < ZYNQMP_XADC_INIT_READ_ONLY || init_type > ZYNQMP_XADC_INIT_FULL)
	{
		printf("zynqmp_xadc_init:Invalid Init type %d\n" ,init_type);
		return -1;
	}

	if ((ret=zynqmp_find_iio_nodes(device_names)) < 1)
	{
		printf("zynqmp_xadc_init:No devices found\n");
		ret = -1;
		goto EXIT;
	}
	zynqmp_num_xadc = ret;
#if 0
	if(init_type == ZYNQMP_XADC_INIT_FULL)
	{
		if(init_mutexes() != 0)
		{
			ret = -1;
			goto EXIT;
		}

		if (pthread_create(&gEvent_handler, NULL, thread_event_handler, NULL) != 0)
		{
			perror("Couldn't spwan the event handler thread\n");
			ret = -1;
			deinit_mutexes(EAlarmMAX);
			goto EXIT;
		}
	}
#endif
	gInit_state = init_type;

EXIT:
	return ret;
}


int zynqmp_xadc_deinit(void)
{
	assert(gInit_state != ZYNQMP_XADC_NOT_INITIALIZED);

	gExitNow = true;
#if 0
	if (gInit_state == ZYNQMP_XADC_INIT_FULL)
	{
		pthread_join(gEvent_handler, NULL);
		deinit_mutexes(EAlarmMAX);
	}
#endif
	gInit_state = ZYNQMP_XADC_NOT_INITIALIZED;
	return 0;
}

#if 0
int xadc_set_threshold(enum XADC_Alarm alarm,float threshold_low, float threshold_high, struct Xadc_callback *cb_ptr)
{
	int ret =0;
	assert(gInit_state == ZYNQMP_XADC_INIT_FULL);
	assert((alarm >= 0) && (alarm < EAlarmMAX));

	pthread_mutex_lock(gXadcAlarm[alarm].lock);
	gXadcAlarm[alarm].thres_low =threshold_low;
	gXadcAlarm[alarm].thres_high =threshold_high;

	gXadcAlarm[alarm].callback.func = (cb_ptr)? cb_ptr->func : NULL;
	gXadcAlarm[alarm].callback.arg = (cb_ptr)? cb_ptr->arg : NULL;

	write_xadc_threshold(gXadcAlarm + alarm);
	pthread_mutex_unlock(gXadcAlarm[alarm].lock);
	return ret;
}

bool xadc_get_alarm_status(enum XADC_Alarm alarm)
{
	assert(gInit_state == ZYNQMP_XADC_INIT_FULL);
	assert((alarm >= 0) && (alarm < EAlarmMAX));
	return gXadcAlarm[alarm].isActive;
}
#endif
//utility functions
static int conv_voltage(struct XadcChan *chan, int input, enum EConvType conv_direction)
{
	int result=0;

	switch(conv_direction)
	{
	case EConvType_Raw_to_Scale:
		result = ((input * 3 * mV_mul)/multiplier);
		break;
	case EConvType_Scale_to_Raw:
		result = (input*multiplier)/(3 * mV_mul);
		break;
	default:
		printf("Convertion type incorrect... Doing no conversion\n");
		//  intentional no break;
	case EConvType_None:
			result = input;
			break;
	}

	return result;
}

static int conv_voltage_ext_ch(struct XadcChan *chan, int input, enum EConvType conv_direction)
{
	int result=0;

	switch(conv_direction)
	{
	case EConvType_Raw_to_Scale:
		result = ((input * mV_mul)/multiplier);
		break;
	case EConvType_Scale_to_Raw:
		result = (input*multiplier)/mV_mul;
		break;
	default:
		printf("Convertion type incorrect... Doing no conversion\n");
		//  intentional no break;
	case EConvType_None:
			result = input;
			break;
	}

	return result;
}

static int conv_temperature(struct XadcChan *chan, int input, enum EConvType conv_direction)
{
	int result=0;

	switch(conv_direction)
	{
	case EConvType_Raw_to_Scale:
		result = ((input * 503975)/multiplier) - 273150;
		break;
	case EConvType_Scale_to_Raw:
		result = ((input + 273150)*multiplier)/503975;
		break;
	default:
		printf("Conversion type incorrect... Doing no conversion\n");
		//  intentional no break;
	case EConvType_None:
			result = input;
			break;
	}

	return result;
}

static int conv_from_files(struct XadcChan *chan, int input, enum EConvType conv_direction)
{
	int result=0;

	switch(conv_direction)
	{
	case EConvType_Raw_to_Scale:
		result = (input + chan->offset)* chan->scale;
		break;
	case EConvType_Scale_to_Raw:
		result = input/chan->scale-chan->offset;
		break;
	default:
		printf("Conversion type incorrect... Doing no conversion\n");
		//  intentional no break;
	case EConvType_None:
			result = input;
			break;
	}

	return result;
}


static int line_from_file(char* filename, char* linebuf)
{
    char* s;
    int i;
    FILE* fp = fopen(filename, "r");
    if (!fp) return -1;
    s = fgets(linebuf, MAX_VALUE_SIZE, fp);
    fclose(fp);
    if (!s) return -1;

    for (i=0; (*s)&&(i<MAX_VALUE_SIZE); i++) 
    {
        if (*s == '\n')
        	*s = 0;
        s++;
    }
    return 0;
}

static int line_to_file(char* filename, char* linebuf)
{
    int fd;
    fd = open(filename, O_WRONLY);
    if (fd < 0)
    	return -1;
    write(fd, linebuf, strlen(linebuf));
    close(fd);
    return 0;
}

int ngzmp_read_xadc(int path, int card, int first, int num, int32_t *data)
{
	enum XADC_Param pnum=0;
	int xadc_num=0;
	int i;
	int rc, error=0;
	int max_num=AMSParamMax+XADCParamMax-1;		// Do not allow rad of XADCParamVAux0 (yet?)

	if (first < 0 || num < 1 || first+num > max_num)
	{
		printf("zynqmp_read_xadc: Error, expected first=%d and num=%d to access range 0..%d\n", first, num, max_num-1);
		return -1;
	}

	for (i=0; i<num;i++)
	{
		if (i+first < AMSParamMax)
		{
			pnum = i+first;
			xadc_num = 0;
		}
		else
		{
			pnum = i+first - AMSParamMax;
			xadc_num = 1;
		}
		if ((rc=zynqmp_read_xadc_param(xadc_num, pnum, (int *)(data+i))) < 0)
		{
			printf("zynqmp_read_xadc: Error reading pnum=%d, xadc_num=%d, rc=%d\n", pnum, xadc_num, rc);
			error = rc;
		}
	}
		
	return error;
}
