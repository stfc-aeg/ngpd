#if NGPD_CONF_ADQ14
#include "ADQAPI.h"
#else
#define ADQ_CLOCK_INT_INTREF 1
#define ADQ_SW_TRIGGER_MODE 1
#define ADQ_LEVEL_TRIGGER_ALL_CHANNELS 1

#endif
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#ifdef LINUX
#include <stdlib.h>
#endif
#include "ngpd.h"
#include "zynqmp_api.h"

/** Parameters for the data collection **/
const unsigned int nof_samples = 1000;
const unsigned int nof_records = 4;
const unsigned int clock_source = ADQ_CLOCK_INT_INTREF;
const unsigned int trigger_mode = ADQ_SW_TRIGGER_MODE;

/** Level Trigger specific options **/
const int level_trigger_level = 10;
const int level_trigger_edge = 1;
const unsigned int level_trigger_channels = ADQ_LEVEL_TRIGGER_ALL_CHANNELS;

/** Internal Trigger specific options **/
const int internal_trigger_period = 1000;
NGPDPathType NGPDPath[NGPD_MAX_PATH];
char ngpd_error_message[NGPD_MAX_ERROR_MESSAGE+2];

int ngpd_config_adq14(unsigned int adq_num, int debug)
{
  /** Declaration of variables for later user **/
	unsigned int success = 1;
	unsigned int nof_adq = 0;
	unsigned int nof_failed_adq;
	int adq_type;
	char *serial_number;
	char *product_name;
	int api_rev = 0;
	int* fw_rev;
	int path=0;
#if NGPD_CONF_ADQ14
  /** Create a control unit **/
	if (NGPDPath[0].valid == 0)
	{
  		NGPDPath[path].adq_cu = CreateADQControlUnit();
		NGPDPath[path].adq_num = adq_num;

		/** Enable Logging **/
		// Errors, Warnings and Info messages will be logged
  		ADQControlUnit_EnableErrorTrace(NGPDPath[path].adq_cu, LOG_LEVEL_INFO, ".");

		/* Enable Ethernet Scan
		* Ethernet communication. If activated, searches for ethernet connected
		* devices.
		* 0: Disabled
		* 1: Looks for ADQ14
		* 2: Looks for ADQ7
		*/
		ADQControlUnit_EnableEthernetScan(NGPDPath[path].adq_cu, 0);

		/** Find Devices **/
		// We will only connect to the first device in this example
		nof_adq = ADQControlUnit_FindDevices(NGPDPath[path].adq_cu);
		printf("Number of ADQ devices found: %u\n", nof_adq);
		nof_failed_adq = ADQControlUnit_GetFailedDeviceCount(NGPDPath[path].adq_cu);
		printf("Number of failed ADQ devices: %u\n", nof_failed_adq);

		if (nof_adq == 0)
		{
			printf("\nNo ADQ device found, aborting..\n");
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "No ADQ device found, aborting");
			return -1;
		}
		api_rev = ADQAPI_GetRevision();
		fw_rev = ADQ_GetRevision(NGPDPath[path].adq_cu, NGPDPath[path].adq_num);
		serial_number = ADQ_GetBoardSerialNumber(NGPDPath[path].adq_cu, NGPDPath[path].adq_num);
		product_name = ADQ_GetBoardProductName(NGPDPath[path].adq_cu, NGPDPath[path].adq_num);
		printf("\nAPI revision:        %d\n", api_rev);
		printf("Firmware revision pointer=%p\n", fw_rev);
		if (fw_rev != NULL)
			printf("Firmware revision:   %d\n", fw_rev[0]);
		printf("Board serial number: %s\n", serial_number);
		if (product_name != NULL)
			printf("Board product name:  %s\n", product_name);
		NGPDPath[path].valid = 1;
		NGPDPath[path].scope_mod = ngpd_scope_mod_create("ngpd_scope0", 1, NGPD_SCOPE_NBYTES_PER_CARD, &(NGPDPath[path].scope_head), NGPD_SCOPE_LAYOUT_4); 
		if (NGPDPath[path].scope_mod == NULL)
			return -1;
		NGPDPath[path].num_cards = 1;
		NGPDPath[path].num_chan = 1;
		NGPDPath[path].chans_per_card = 1;
		NGPDPath[path].generation = NGPDGenADQ14;
		NGPDPath[path].histogram.hgt_divide = 1;
		NGPDPath[path].histogram.tail_sum_divide = 1;
		NGPDPath[path].histogram.fall_time_divide = 1;
	}
	NGPDPath[path].debug = debug;
	NGPDPath[path].playback_first_addr = NGPD_DRAM_FIRST_ADDR ;
	NGPDPath[path].playback_last_addr = (1 << (30-6))-1 ;
	NGPDPath[path].playback_num_t = NGPD_PLAYBACK_NUM_T;
	NGPDPath[path].scope_first_addr = NGPD_DRAM_FIRST_ADDR; // Trying to increase this to 32768 and reduce numT etc to discard first 32768 * 512 bits as appears to get over written.
	NGPDPath[path].scope_last_addr = (1 << (30-6))-1;
	NGPDPath[path].run_flags = NGPD_RUN_FLAGS_DEFAULT;
	NGPDPath[path].scope_out_skip = 8;		// Default value to slow output rate of scope mode
	NGPDPath[path].playback_skip = 1;		// Default value to run playback at full speed
	NGPDPath[path].max_scope_streams = NGPD_SCOPE_MAX_STREAMS;
	NGPDPath[path].playback_num_streams = 1;
	ADQ_BypassUserLogic (NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1, 0);
	ADQ_BypassUserLogic (NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 2, 0);
	ADQ_SetSampleSkip(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1);

/*	ADQ_StopStreaming(NGPDPath[path].adq_cu, NGPDPath[path].adq_num);	
	ADQ_SetStreamStatus(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 0); 
	ADQ_SetTriggerMode(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1);	// SWTrig
	ADQ_SetTriggerEdge(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1, 1);   // SWTrig Rising

	ADQ_SetStreamConfig(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 1, 1);		// Disable the DRAM FIFO while Playback is being used.
	ADQ_SetTransferBuffers(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, 64, NGPD_MAX_BUFFER);
*/

	return path;
#else
	printf("This version compiled without ADQ14 support. Build with NGPD_CONF_ADQ14 set\n");
	return -1;
#endif
}

void ngpd_close(int path)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_debug: Invalid path=%d", path);
		return ;
	}
	if (NGPDPath[path].generation == NGPDGenZynqMP1)
	{
		if (NGPDPath[path].type == NGPDComposite)
		{
			int card;
			for (card=0; card<NGPDPath[path].num_cards; card++)
			{
				int sub_path=NGPDPath[path].sub_path[card];
				zynqmp_close(NGPDPath[sub_path].zynqmp);
				NGPDPath[sub_path].valid=0;
			}
			NGPDPath[path].valid=0;
		}
		else
		{
			zynqmp_close(NGPDPath[path].zynqmp);
			NGPDPath[path].valid=0;
		}
	}
}



char *ngpd_get_error_message()
{
	return ngpd_error_message;
}


int ngpd_set_debug(int path, int debug)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_debug: Invalid path=%d", path);
		return -1;
	}

	NGPDPath[path].debug = debug;
	return 0;
}

int ngpd_get_num_cards(int path)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_get_num_cards: Invalid path=%d", path);
		return -1;
	}

	return NGPDPath[path].num_cards;
}

	
int ngpd_get_num_chan(int path)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_get_num_chan: Invalid path=%d", path);
		return -1;
	}

	return NGPDPath[path].num_chan;
}

int ngpd_get_chans_per_card(int path)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_get_chans_per_card: Invalid path=%d", path);
		return -1;
	}

	return NGPDPath[path].chans_per_card;
}

int ngpd_get_chans_on_card(int path, int card, int *first, int *num)
{
	int this_path;
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_get_chans_on_card: Invalid path=%d", path);
		return -1;
	}
	if (card <0 || card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_get_chans_on_card: Invalid card=%d, expected 0...%d", card, NGPDPath[path].num_cards);
		return -1;
	}
	if (NGPDPath[path].type == NGPDComposite)
	{
		this_path = NGPDPath[path].sub_path[card];
	}
	else
		this_path = path;
	
	if (first != NULL)
		*first = NGPDPath[this_path].chan_of_system;
	if (num != NULL)
		*num = NGPDPath[this_path].chans_per_card;
	return 0;
}

NGPDGeneration ngpd_get_generation(int path)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_get_generation: Invalid path=%d\n", path);
		return -1;
	}

	return NGPDPath[path].generation;
}

int ngpd_get_max_scope_streams(int path)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_get_max_scope_streams: Invalid path=%d\n", path);
		return -1;
	}

	return NGPDPath[path].max_scope_streams;
}

