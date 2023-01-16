#if NGPD_CONF_ADQ14
#include "ADQAPI.h"
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
#include "zynqmp_protocol.h"
#include "zynqmp_api.h"

#define DEFAULT_BASE_IP_ADDRESS "192.168.0.1"
#define DEFAULT_BASE_MAC_ADDRESS "02.00.00.00.00.00"

#define NGPD_MAX_IP_CHARS 16
int ngzmp_do_config(int ncards, char* baseIPaddress, int basePort, char* baseMACaddress, int num_chan, int debug, int card_index, int do_init,  NGPDDummy dummy_system);
int ngpd_config_tcp(char zynqmpHostName[][NGPD_MAX_IP_CHARS], int zynqmpPort, int ncards, int num_chan, int debug, NGPDDummy dummy_system);
static void to_bytes(char *ipName, unsigned char* b, int n) {
	char *end;
	char* iptr = ipName;
	int i;
	for (i=0; i<n; i++) {
		b[i] = (unsigned char) strtol(iptr, &end, 10);
		iptr = end + 1;
	}
}
 /**
 * @ingroup toplevel
 * Configure the complete xspress3 system with flag to specify whether the system in initialised. 
 * This configuration entry point is used by imgd to moditor xspress3 without overwriting the initialisation.
 * @param ncards the number of xspress3 cards that constitute the xspress3 system, between 1 and {@link #NGPD_MAX_CARDS  NGPD_MAX_CARDS}.
 * @param num_tf the maximum number of time frames when used as 4096 energy bins and no auxiliary data.
 *               If the memory is arranged differently (using RoI or Auxiliary data) then the actual number of tf that will fit can go up or down
 * @param baseIPaddress the IP address of the host server in dotted quad format (e.g. 192.168.0.1) from which all other address's are calculated or NULL to use the default
 * @param basePort the IP port number or -1 to use the default
 * @param baseMACaddress 	The base MAC address (e.g. 02.00.00.00.00) for card 0, from which all other card MAC address's are calculated or NULL to use the default
							Specify 00.00.00.00.00.00 to disable UDP connection ot this instance, to co-exist with other libxspress3 instance in da.server, EPICS etc.
 * @param num_chan 			The number of channels to be configured in the xspress3 system. between 1 and (ncards * {@link #NGPD_MAX_CHANS_PER_CARD}) or -1 to configure all available channels.
 
 * @param create_module 	A boolean flag to determine whether to create the scope mode data module. It usual to enable this, though some memory may be saved by not doing this.
 * @param modname 			The module name for the scope mode data module or NULL to use the default. Using the default is recommended.
 * @param debug 			An integer flag to turn debug messages off/on/verbose.
 * @param card_index 		Starting card number ie 0 = card1, 1 = card2 etc. -1 assumes card_index 0)
 * @param do_init			A boolean flag to specify whether the FPGA registers and BRAMS are initialised. Usually set, but cleared if a second progress (e.g. imgd) is to view data 
 *							without overwriting a setup done by da.server or EPICS.
 * @param xsp3m_readout		Override Xspress3 mini readout mode, Xsp3mRd_Auto to use default for detected firmware.
 * @return a handle to the top level of the xspress3 system or a negative error code.

 */

static int configNo;

NGPDSavedConfig ngpd_saved_config[NGPD_MAX_CONFIGS];

int ngpd_config_ngzmp(int ncards,  char* baseIPaddress, int basePort, char* baseMACaddress, int num_chan, int debug, int card_index, int do_init,  NGPDDummy dummy_system) {
	int i, path=-1, rc;
	int use_existing_path = 0;

	printf("debug level %d, dummy_system=%d\n", debug, dummy_system);
	if (dummy_system & NGPDDummy_FPGA)
		dummy_system = NGPDDummy_All;
	else if (dummy_system & NGPDDummy_ADC)
		dummy_system |= NGPDDummy_Preamp;
	
	if (ncards <= 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config_ngzmp: invalid number of ncards=%d", ncards);
		return NGPD_ERROR;
	}
	if (baseIPaddress != NULL && strlen(baseIPaddress) == 0)
		baseIPaddress = NULL;	// Allow user API to pass "" for interface to Pyton ?
	if (baseMACaddress != NULL && strlen(baseMACaddress) == 0)
		baseMACaddress = NULL;
	// Has exactly the same configuration been used.
	for (i = 0; i < configNo; i++) {
		if (ngpd_saved_config[i].type == ConfigZynqMP1 && ngpd_saved_config[i].ncards == ncards && 
				((baseIPaddress==NULL && ngpd_saved_config[i].baseIPaddress[0]==0) || (baseIPaddress != NULL && strcmp(ngpd_saved_config[i].baseIPaddress, baseIPaddress) == 0)) &&
				ngpd_saved_config[i].basePort == basePort  &&
				((baseMACaddress==NULL && ngpd_saved_config[i].baseMACaddress[0]==0) || (baseMACaddress != NULL && strcmp(ngpd_saved_config[i].baseMACaddress, baseMACaddress) == 0)) &&
				ngpd_saved_config[i].nchan == num_chan &&
				ngpd_saved_config[i].card_index == card_index && dummy_system == ngpd_saved_config[i].dummy_system)
		{
			path = ngpd_saved_config[i].path;
			NGPDPath[path].debug = debug;
			break;			
		}
	}
	if (path == -1)
	{
		/* Not a matching configuration */
		/* Search through any existing configs */
		for (i = 0; i < configNo; i++) {
			if (card_index+ncards-1 < ngpd_saved_config[i].card_index) {
				if (debug > 1)
					printf("OK: card index+ncards-1 %d is less than saved card index %d\n", card_index+ncards-1, ngpd_saved_config[i].card_index);
			
			} else if (card_index > ngpd_saved_config[i].card_index+ngpd_saved_config[i].ncards-1) {
				if (debug > 1)
					printf("OK: card index %d is greater than saved card index+saved ncards-1 %d\n", card_index, ngpd_saved_config[i].card_index+ngpd_saved_config[i].ncards-1);
			} else {
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config_ngzmp: card index=%d and ncards=%d conflicts with saved card index=%d, ncards=%d", 
								card_index, ncards, ngpd_saved_config[i].card_index, ngpd_saved_config[i].ncards-1);
				return NGPD_ERROR;
			}
		}
		if (configNo >= NGPD_MAX_CONFIGS) 
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config_ngzmp: Reached maximum number of configs");
			return NGPD_ERROR;
		}		
		path = ngzmp_do_config(ncards,  baseIPaddress, basePort, baseMACaddress, num_chan, debug, card_index, do_init, dummy_system);
		if (path >= 0) {
			ngpd_saved_config[configNo].type = ConfigZynqMP1;
			ngpd_saved_config[configNo].ncards = ncards;
			ngpd_saved_config[configNo].dummy_system = dummy_system;
			if (baseIPaddress == NULL)
				ngpd_saved_config[configNo].baseIPaddress[0] = 0;
			else
			{
				strncpy(ngpd_saved_config[configNo].baseIPaddress, baseIPaddress, NGPD_MAX_IP_ADDR);
				ngpd_saved_config[configNo].baseIPaddress[NGPD_MAX_IP_ADDR] = 0;
			}
			ngpd_saved_config[configNo].basePort = basePort;
			if (baseMACaddress == NULL)
				ngpd_saved_config[configNo].baseMACaddress[0] = 0;
			else
			{
				strncpy(ngpd_saved_config[configNo].baseMACaddress, baseMACaddress, NGPD_MAX_MAC_ADDR);
				ngpd_saved_config[configNo].baseMACaddress[NGPD_MAX_MAC_ADDR]= 0;
			}
			ngpd_saved_config[configNo].nchan = num_chan;
			ngpd_saved_config[configNo].card_index = card_index;
			ngpd_saved_config[configNo].path = path;
			configNo++;
		}
	}
	else
		use_existing_path = 1;

	if (path >= 0)
	{
		/* Default configuration */

		if ((NGPDPath[path].dummy_system & NGPDDummy_FPGA) == 0)
		{
			if (do_init)
			{
				NGZMPHistConf hist_setup;
#if 0
				printf("Initialising hardware registers\n");			 
				if ((rc=ngpd_register_init(path, -1)) < 0 )
				{
					char err_copy[NGPD_MAX_ERROR_MESSAGE + 2];
					strcpy(err_copy, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config: %s", err_copy);
					return rc;
				}
				if ((rc=ngpd_glob_register_init(path, -1)) < 0 )
				{
					char err_copy[NGPD_MAX_ERROR_MESSAGE + 2];
					strcpy(err_copy, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config: %s", err_copy);
					return rc;
				}
				printf("Initialising hardware BRAM\n");			 
				if ((rc=ngpd_bram_init(path, -1, -1, 1.0)) < 0)
				{
					char err_copy[NGPD_MAX_ERROR_MESSAGE + 2];
					strcpy(err_copy, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config: %s", err_copy);
					return rc;
				}
#endif
				if (use_existing_path)
				{
					int mem_layout;
					/* Do this (again if first time config) to be sure in a reconfig that the memory is in a default state */
					rc = ngzmp_dma_config_memory(path, -1, mem_layout, !do_init, 1);		// If !do_init, call this to read the current DMA buffer sizes for scope module.
					if (rc < 0)
					{
						char err_copy[NGPD_MAX_ERROR_MESSAGE + 2];
						strcpy(err_copy, ngpd_get_error_message());
						snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_dma_config_memory: %s : returns %d", err_copy, rc);
						return rc;
					}	
				}
				NGPDPath[path].playback_num_streams = 1;
				NGPDPath[path].playback_num_t = NGPDPath[path].playback_num_bytes/sizeof(u_int16_t);
				memset(&hist_setup, 0, sizeof(hist_setup));

				hist_setup.separate_ngp = 0;
				hist_setup.enb_hgt_sum = 0;
				hist_setup.enb_hgt_fall = 0;
				hist_setup.discard_pu = 0;
				hist_setup.nbits_height = 10;
				hist_setup.nbits_fall_time = 8;
				hist_setup.nbits_tail_sum = 10;
				if ((rc=ngzmp_hist_write_chan_config(path, -1, &hist_setup)) < 0)
				{
					char err_copy[NGPD_MAX_ERROR_MESSAGE + 2];
					strcpy(err_copy, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config: %s", err_copy);
					return rc;
				}
			}
			else
			{
				u_int32_t pb_cont;
				rc = ngpd_read_glob_regs(path, 0, NGZMP_PLAYBACK, 1, &pb_cont);
				if (rc < 0)
				{
					char err_copy[NGPD_MAX_ERROR_MESSAGE + 2];
					strcpy(err_copy, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config_ngzmp: Error reading playback control register: %s", err_copy);
					ngpd_error_message[NGPD_MAX_ERROR_MESSAGE] = 0;
					return rc;
				}
				switch (NGZMP_PLAYBACK_GET_NUM_STREAMS(pb_cont))
				{
				case 0:	NGPDPath[path].playback_num_streams = 1; break;
				case 1:	NGPDPath[path].playback_num_streams = 2; break;
				case 2:	NGPDPath[path].playback_num_streams = 4; break;
				case 3:	NGPDPath[path].playback_num_streams = 8; break;
				}
				NGPDPath[path].playback_num_t = NGPDPath[path].playback_num_bytes/(sizeof(u_int16_t)*NGPDPath[path].playback_num_streams);
			}
		}
#if 0
				if (debug >= 1)
					printf("Setting default format_run\n");
				rc = ngpd_format_run(path, -1, NGPD_FORMAT_AUX1_MODE_NONE, 0, 0, 0, 0, 12);
				if (rc < 0) {
					char err_copy[NGPD_MAX_ERROR_MESSAGE + 2];
					strcpy(err_copy, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config: %s", err_copy);
					return rc;
				}
				printf("config with init: After ngpd_format_run, mem_layout = %08X, numtf_hist=%d, scaler_num_tf=%d, scaler_num_tf_fw=%d\n", NGPDPath[path].mem_layout, NGPDPath[path].hist_frames_num_tf, 
						NGPDPath[path].scaler_num_tf, NGPDPath[path].scaler_num_tf_fw);
			}
			else
			{
			  	ngpd_read_format(path, -1, 0, NULL, NULL, NULL, NULL, NULL); // Force read of format information in to internal storage.
				printf("config no init: After ngpd_format_run, mem_layout = %08X, numtf_hist=%d, scaler_num_tf=%d, scaler_num_tf_fw=%d\n", NGPDPath[path].mem_layout, NGPDPath[path].hist_frames_num_tf, 
						NGPDPath[path].scaler_num_tf, NGPDPath[path].scaler_num_tf_fw);
			}
			if (NGPDPath[path].features.soft_scalers)
			{
			}
			else
			{
				/* This updates NGPDPath[path]. but does not change the firmware settings, so is done even !do_init */
				if (debug >= 1)
					printf("Configuring Firmware scalars\n");
				rc = ngpd_config_scaler(path);
				if (rc != 0) {
					char err_copy[NGPD_MAX_ERROR_MESSAGE + 2];
					strcpy(err_copy, ngpd_get_error_message());
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config: %s", err_copy);
					return rc;
				}
			}
		}
#endif

		if (debug > 1)
			printf("Returning path %d\n", path);
	}
	return path;
}

/**
 * @ingroup internal
 * Configure the complete  ngzmp system. 
 * This should be called only once. It is used internally by ngpd_config_ngzmp.
 * The number of card and number of time frame parameters must be specified. The debug and create_module
 * parameters are boolean and should be specified as 0 or 1. All other parameters can be used as default by specifying either NULL or -1.
 *
 * @verbatim
 * Using the default baseIPaddress, for the host server, of 192.168.0.1 the following values are calculated for the complete xspress3 system.
 *
 * xspress3 1Gb tcp IP address 		card 1		192.168.0.2
 * 									card 2		192.168.0.3
 * 									card 3		192.168.0.4
 * 									...
 *
 * host 10Gb udp IP address 		card 1		192.168.0.65
 * 									card 2		192.168.0.69
 * 									card 3		192.168.0.73
 * 									...
 *
 * xspress3 10Gb udp IP address 	card 1		192.168.0.66
 * 									card 2		192.168.0.70
 * 									card 3		192.168.0.74
 * 									...
 *
 * Using the default MAC address of 02.00.00.00.00.00 the following values are calculated for the complete xspress3 system.
 *
 * xspress3 MAC address 			card 1		02.00.00.00.00.00
 * 									card 2		02.00.00.00.00.02
 * 									card 3		02.00.00.00.00.04
 * 									...
 *
 * For all xspress3 cards the default basePort for the tcp connection is 30123 and for the udp connection 30124.
 *
 * @endverbatim
 * @param ncards 			The number of xspress3 cards that constitute the xspress3 system, between 1 and {@link #NGPD_MAX_CARDS  NGPD_MAX_CARDS}.
 * @param num_tf 			The number of time frames
 * @param baseIPaddress 	The IP address of the host server in dotted quad format (e.g. 192.168.0.1) from which all other address's are calculated or NULL to use the default
 * @param basePort 			The IP port number or -1 to use the default
 * @param baseMACaddress 	The base MAC address (e.g. 02.00.00.00.00) for card 0, from which all other card MAC address's are calculated or NULL to use the default
							Specify 00.00.00.00.00.00 to disable UDP connection to this instance, to co-exist with other libxspress3 instance in da.server, EPICS etc.
 * @param num_chan 			The number of channels to be configured in the xspress3 system. Between 1 and (ncards * {@link #NGPD_MAX_CHANS_PER_CARD}) or -1 to configure all available channels.
 * @param debug 			An integer flag to turn debug messages off/on/verbose.
 * @param card_index 		Starting card number ie 0 = card1, 1 = card2 etc. -1 assumes card_index 0)
 * @param do_init			A boolean flag to specify whether the FPGA registers and BRAMS are initialised. Usually set, but cleared if a second progress (e.g. imgd) is to view data 
 *							without overwriting a setup done by da.server or EPICS.
 * @param xsp3m_readout		Override Xspress3 mini readout mode, Xsp3mRd_Auto to use default for detected firmware.
 * @param dummy_system		Create a dummy system, currently only to support offline DTC, but may be extended for test
 * @return a handle to the top level of the xspress3 system or a negative error code.
 */
int ngzmp_do_config(int ncards, char* baseIPaddress, int basePort, char* baseMACaddress, int num_chan,
		int debug, int card_index, int do_init,  NGPDDummy dummy_system) {
	int i;
	int rc = 0;
	char host10GigeIPaddress[NGZMP_MAX_CARDS][NGPD_MAX_IP_CHARS];
	char zynqmpIPaddress[NGZMP_MAX_CARDS][NGPD_MAX_IP_CHARS];
	char zynqmp10GigeIPaddress[NGZMP_MAX_CARDS][NGPD_MAX_IP_CHARS];
	char zynqmpMACaddress[NGZMP_MAX_CARDS][18];
	int path = -1;
	unsigned char b[4], m[6], last;
	int tcpPort = (basePort <= 0) ? ZYNQMP_CMD_PORT : basePort;
	int udpPort = tcpPort + 1;
	int starting_card = (card_index == -1) ? 0 : card_index;
	char defaultScopeModname[100];
	int do_udp;
	int mem_layout=0;
	int layout;

	to_bytes((baseIPaddress == NULL) ? DEFAULT_BASE_IP_ADDRESS : baseIPaddress, b, 4);
	to_bytes((baseMACaddress == NULL) ? DEFAULT_BASE_MAC_ADDRESS : baseMACaddress, m, 6);

	if (m[0] == 0 && m[1] == 0 && m[2] == 0 && m[3] == 0 && m[4] == 0 && m[5] == 0)
		do_udp = 0;
	else
		do_udp = 1;

	last = b[3];
	if (starting_card+ncards-1 > NGPD_MAX_CARD_INDEX) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_do_config: starting card %d + ncards %d is out of range 0 to %d", starting_card, ncards, NGPD_MAX_CARD_INDEX);
		return NGPD_ERROR;
	}
	if (ncards > NGZMP_MAX_CARDS)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_do_config: ncards %d > NGZMP_MAX_CARDS %d", ncards, NGZMP_MAX_CARDS);
		return NGPD_ERROR;
	}
		
	for (i = 0; i < ncards; i++) {
		b[3] = last + i + starting_card + 1;
		sprintf(zynqmpIPaddress[i], "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
		b[3] = last + (i+starting_card) * 4 + 64;
		sprintf(host10GigeIPaddress[i], "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
		b[3] = last + (i+starting_card) * 4 + 64 + 1;
		sprintf(zynqmp10GigeIPaddress[i], "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
		m[5] = (i+starting_card) * 2;
		sprintf(zynqmpMACaddress[i], "%x.%x.%x.%x.%x.%x", m[0], m[1], m[2], m[3], m[4], m[5]);

		if (debug) {
			printf("xspress3_config: Card %d\n", i);
			printf("xspress3_config: host10GigeIPaddress %s\n", host10GigeIPaddress[i]);
			printf("xspress3_config: zynqmpIPaddress %s\n", zynqmpIPaddress[i]);
			printf("xspress3_config: zynqmp10GigeIPaddress %s\n", zynqmp10GigeIPaddress[i]);
			printf("xspress3_config: zynqmpMACaddress %s\n", zynqmpMACaddress[i]);
		}
	}
	sprintf(defaultScopeModname, "ngpd_scope%d", starting_card);
	if (debug) {
		printf("xspress3_config: 1GigePort %d\n", tcpPort);
		printf("xspress3_config: 10GigePort %d\n", udpPort);
		printf("xspress3_config: scope module name %s\n", defaultScopeModname);
	}
	path = ngpd_config_tcp(zynqmpIPaddress, tcpPort, ncards, num_chan, debug, dummy_system);
	if (path < 0) {
		char err_copy[NGPD_MAX_ERROR_MESSAGE + 2];
		strcpy(err_copy, ngpd_get_error_message());
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config: %s", err_copy);
		return path;
	}
	NGPDPath[path].starting_card = starting_card;
	if (ncards > 1)
	{
		for (i=0;i<ncards; i++)
		{
			int sub_path = NGPDPath[path].sub_path[i];
			if (sub_path >=0 && sub_path < NGPD_MAX_PATH && NGPDPath[sub_path].valid)
				NGPDPath[sub_path].starting_card = starting_card;
		}
	}

/*	if ((rc=ngpd_features_unpack(path, -1)) < 0)
	{
		char err_copy[NGPD_MAX_ERROR_MESSAGE + 2];
		strcpy(err_copy, ngpd_get_error_message());
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_do_config: %s", err_copy);
		return rc;
	}
*/
	if ((NGPDPath[path].dummy_system & NGPDDummy_FPGA) == 0)
	{
		if (NGPDPath[path].debug)
			printf("Calling ngzmp_do_config: calling ngpd_dma_config_memory_int(mem_layout=0x%08X)\n", mem_layout);
		rc = ngzmp_dma_config_memory(path, -1, mem_layout, !do_init, 1);		// If !do_init, call this to read the current DMA buffer sizes for scope module.
		if (rc < 0)
		{
			char err_copy[NGPD_MAX_ERROR_MESSAGE + 2];
			strcpy(err_copy, ngpd_get_error_message());
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_do_config: %s : returns %d", err_copy, rc);
			return rc;
		}	
	}
	else
	{
		NGPDPath[path].nbytes_scope_per_card = 1024*1024*1024;	// Some value to make a scope mode module which is reasonable
		if (ncards > 1)
		{
			for (i=0;i<ncards; i++)
			{
				int sub_path = NGPDPath[path].sub_path[i];
				if (sub_path >=0 && sub_path < NGPD_MAX_PATH && NGPDPath[sub_path].valid)
					NGPDPath[sub_path].nbytes_scope_per_card = NGPDPath[path].nbytes_scope_per_card;
			}
		}
	}
	if (debug > 1)
		printf("Creating scope module name  '%s'\n", defaultScopeModname );

	NGPDPath[path].scope_mod = ngpd_scope_mod_create(defaultScopeModname, NGPDPath[path].num_cards, NGPDPath[path].nbytes_scope_per_card, &(NGPDPath[path].scope_head), NGZMP_SCOPE_LAYOUT_4);
	if (NGPDPath[path].scope_mod != NULL)
	{
		char err_copy[NGPD_MAX_ERROR_MESSAGE + 2];
		strcpy(err_copy, ngpd_get_error_message());
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config: %s", err_copy);
		return rc;
	}

	return path;
}

/**
 * @ingroup internal
 * Configure the xspress3 system and create a 1Gb TCP socket link to each card in the system.
 *
 * @param zynqmpHostName an array of strings representing each of the xspress3 system card's IP address in dotted quad format (e.g. 192.123.456.789).
 * @param zynqmpPort the IP port number.
 * @param ncards the number of xspress3 cards that constitute the xspress3 system, between 1 and {@link #NGPD_MAX_CARDS  NGPD_MAX_CARDS}.
 * @param num_chan the number of channels to be configured in the xspress3 system. between 1 and (ncards * {@link #NGPD_MAX_CHANS_PER_CARD}).
 * @param debug a integer flag to turn debug messages off/on/verbose.
 * @return a handle to the top level of the xspress3 system or a negative error code.
 */
int ngpd_config_tcp(char zynqmpHostName[][NGPD_MAX_IP_CHARS], int zynqmpPort, int ncards, int num_chan, int debug, NGPDDummy dummy_system) {
	ZynqMPHandle *zynqmpHandle = NULL;
	int rc = 0;
	u_int32_t revision;
	int path = 0;
	int i, newPath;
	static int topLevel = 1;

	if (ncards > NGPD_MAX_NUM_CARDS) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config: Too many cards specified for system");
		return NGPD_ERROR;
	}
	// Find the next available path
	while (NGPDPath[path].valid && path < NGPD_MAX_PATH) {
		if (debug > 1)
			printf("path %d isValid %d\n", path, NGPDPath[path].valid);
		path++;
	}
	if (path == NGPD_MAX_PATH) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config_tcp: Too many paths open");
		return NGPD_ERROR;
	}
	if (debug==1)
		printf("ngpd_config: Configuring %s on path %d name=%s\n", (topLevel == 1 ? "node" : "leaf"), path, zynqmpHostName[0]);
	else if (debug>1)
	{
		printf("ngpd_config: Configuring %s on path %d\n", (topLevel == 1 ? "node" : "leaf"), path);

		printf(" This path zynqmpHostName=%p\n", zynqmpHostName[0]);
		printf(" This path zynqmpHostName=%s\n", zynqmpHostName[0]);
		printf(" Ncards=%d\n", ncards);
	}

	memset(&NGPDPath[path], 0, sizeof(NGPDPathType));
	NGPDPath[path].num_cards = ncards;
	NGPDPath[path].type = (topLevel == 1 && ncards > 1) ? NGPDComposite : NGPDSingle;
	NGPDPath[path].debug = debug;
	NGPDPath[path].valid = 1;
	NGPDPath[path].scope_mod = NULL;
	NGPDPath[path].run_flags = NGZMP_RUN_FLAGS_DEFAULT;
	NGPDPath[path].chan_of_system = 0;
	NGPDPath[path].dummy_system = dummy_system;
#if 0
	for (i=0; i<NGPD_MAX_CHANS_PER_CARD; i++)
	{
		/* Put sensible (non-zero) value into the histogram structure as the UDP threads will start before the first call to ngpd_format_run */
		NGPDPath[path].histogram[i].nbins_eng = 1;
		NGPDPath[path].histogram[i].nbins_aux1 = 1; 
		NGPDPath[path].histogram[i].nbins_aux2 = 1;
		NGPDPath[path].histogram[i].num_sub_frames = 1;
		NGPDPath[path].histogram[i].ts_divide = 1;	
		NGPDPath[path].histogram[i].list_mode_fd = -1;	
	}
#endif
	for (i=0;i<NGPD_MAX_NUM_CARDS; i++)
		NGPDPath[path].sub_path[i] = -1;

	if (ncards > 1) {
		topLevel = 0;
		for (i = 0; i < ncards; i++) {
			if (debug > 1)
			{
				printf("Calling ngpd_config_tcp for card %d of %d \n", i, ncards);
				printf("Top level zynqmpHostName=%p\n", zynqmpHostName);
				printf("This path zynqmpHostName=%p\n", zynqmpHostName[i]);
				printf(".... zynqmpHostname=%s\n", zynqmpHostName[i]);
			}

			newPath = ngpd_config_tcp(zynqmpHostName[i], zynqmpPort, 1, num_chan, debug, dummy_system); // Be very careful of how zynqmpHostName is dereferenced
			if (newPath < 0) {
				topLevel = 1;
				ngpd_close(path);
				return newPath;
			}
			NGPDPath[path].sub_path[i] = newPath;
		}
		NGPDPath[path].revision = NGPDPath[NGPDPath[path].sub_path[0]].revision;
		NGPDPath[path].chans_per_card = NGPDPath[NGPDPath[path].sub_path[0]].chans_per_card;
		rc = ngpd_set_num_chan(path, num_chan);
		if (rc != 0) {
			if (debug)
				printf("Opening sub-path failed\n");
			ngpd_close(path);
			topLevel = 1;
			return rc;
		}
		//return the top level path
		topLevel = 1;
		return path;
	}
	if (debug>1)
		printf("ngpd_config: Configuring host %s on port %d\n", zynqmpHostName[0], zynqmpPort);
	NGPDPath[path].valid = 0;
	if ((dummy_system & NGPDDummy_FPGA) == 0)
	{
		zynqmpHandle = zynqmp_initialise((const char*) zynqmpHostName[0], zynqmpPort);
		if (zynqmpHandle == NULL) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config: Cannot initialise Fem %s on port %d", zynqmpHostName[0],
					zynqmpPort);
			if (topLevel != 0) {
				ngpd_close(path);
			}
			return NGPD_ERROR;
		}
		NGPDPath[path].zynqmp = zynqmpHandle;
		NGPDPath[path].generation = NGPDGenZynqMP1;
		NGPDPath[path].valid = 1;
		rc = ngpd_read_glob_regs(path, 0, NGZMP_REVISION, 1,  &revision);
		if (rc != NGPD_OK) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config: Cannot read revision register %d", rc);
			if (topLevel != 0) {
				ngpd_close(path);
			}
			return rc;
		}
	}
	else
	{
		NGPDPath[path].generation = NGPDGenZynqMP1;
		NGPDPath[path].valid = 1;
		revision =  1;
		printf("Building dummy system from type=%d, revision=0x%08X\n", dummy_system, revision);
		for (i=0; i< NGZMP_CHANS_PER_CARD; i++)
		{
			if ((NGPDPath[path].dummy_chan_reg[i]=(u_int32_t *)malloc(sizeof(u_int32_t)*NGPD_CHAN_NUM)) == NULL)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config: Cannot read malloc space for dummy channel registers");
				return -1;
			}
		}
		if ((NGPDPath[path].dummy_glob_reg = (u_int32_t *)malloc(sizeof(u_int32_t)*NGZMP_GLOB_NUM)) == NULL)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config: Cannot read malloc space for dummy global registers");
			return -1;
		}
	}
	if (dummy_system & NGPDDummy_ADC)
	{
		if ((NGPDPath[path].dummy_chan_dga=(u_int8_t *)malloc(sizeof(u_int8_t)*NGZMP_CHANS_PER_CARD)) == NULL)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config: Cannot read malloc space for dummy channel DGA");
			return -1;
		}
		memset(NGPDPath[path].dummy_chan_dga, 31, NGZMP_CHANS_PER_CARD);
	}
	if (dummy_system & NGPDDummy_Preamp)
	{
		if ((NGPDPath[path].dummy_chan_offset=(u_int16_t *)malloc(sizeof(u_int16_t)*NGZMP_CHANS_PER_CARD)) == NULL)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_config: Cannot read malloc space for dummy channel offsets");
			return -1;
		}
		memset(NGPDPath[path].dummy_chan_offset, 0, sizeof(u_int16_t)*NGZMP_CHANS_PER_CARD);
	}
	NGPDPath[path].revision = revision;

	NGPDPath[path].chans_per_card = NGZMP_CHANS_PER_CARD;
	NGPDPath[path].max_num_chan = NGZMP_CHANS_PER_CARD;
	NGPDPath[path].num_chan = NGZMP_CHANS_PER_CARD;
	NGPDPath[path].max_scope_streams = NGZMP_SCOPE_MAX_STREAMS;

	if (topLevel) {
		rc = ngpd_set_num_chan(path, num_chan);
		if (rc != 0) {
			ngpd_close(path);
			return rc;
		}
	}

	return path;
}


/**
 * @ingroup internal
 * Set the number of channels to be used in system. 
 * This is generally called from {@link ngpd_config}. Trying to change this afterwards may leave the wrong number of histogram threads running, so is not recommended.
 * If the requested number is -1 all available channels will be used, else it is assumed that each card uses its full allocation of
 * channels, the cards are contiguous so that any non-full remainder is in the last card. There is no current provision
 * for a sparse allocation of channels across cards. 
 * A later API may address this.
 *
 * @param path a handle to the top level of the xspress3 system returned from {@link ngpd_config()}.
 * @param num_chan the number of channels requested or -1 for all available channels
 * @return {@link #NGPD_OK NGPD_OK} or a negative error code.
 */
int ngpd_set_num_chan(int path, int num_chan) {

	int i, remaining;
	int thisPath, max_chans = 0;

	if (path < 0 || path >= NGPD_MAX_PATH) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_set_num_chan: invalid path %d", path);
		return NGPD_ERROR;
	}
	// Assign maximum number of channels and range check requested value
	if (NGPDPath[path].type == NGPDComposite) {
		for (i = 0; i < NGPDPath[path].num_cards && NGPDPath[path].sub_path[i] >= 0; i++) 
		{
			thisPath = NGPDPath[path].sub_path[i];
			max_chans += NGPDPath[thisPath].chans_per_card;
		}
		NGPDPath[path].max_num_chan = max_chans;
	} else {
		NGPDPath[path].max_num_chan = NGPDPath[path].chans_per_card;
	}
	if (NGPDPath[path].debug)
		printf("ngpd_set_num_chan: Max number of channels on path %d is set to %d\n", path, NGPDPath[path].max_num_chan);

	if (num_chan != -1 && num_chan > NGPDPath[path].max_num_chan) {
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE,
				"ngpd_set_num_chan: number requested (%d) exceeds maximum possible (%d)", num_chan,
				NGPDPath[path].max_num_chan);
		return NGPD_ERROR;
	}
	if (num_chan == -1) 
	{
		NGPDPath[path].num_chan = NGPDPath[path].max_num_chan;
	} 
	else 
	{
		if (NGPDPath[path].type == NGPDComposite)
		{
			remaining = num_chan;
			for (i = 0; i < NGPDPath[path].num_cards && NGPDPath[path].sub_path[i] >= 0; i++)
			{
				thisPath = NGPDPath[path].sub_path[i];
				// Loop over sub paths and set number requested. We assume that these are done in full, contiguous units
				// of NGPD units so that any non-full remainder is in the last channel. There is no current provision
				// for a sparse allocation of these across boards
				if (remaining >= NGPDPath[thisPath].chans_per_card) 
				{
					remaining -= NGPDPath[thisPath].chans_per_card;
					NGPDPath[thisPath].num_chan = NGPDPath[path].chans_per_card;		// Changed from [path] on review 12/10/2016 NOT tested
				} 
				else 
				{
					if (remaining == 0) 
					{
						snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE,
								"ngpd_set_num_chan: too many cards specified(%d) for the requested number of channels (%d)\n",
								NGPDPath[path].num_cards, num_chan);
						return NGPD_ERROR;
					}
					NGPDPath[thisPath].num_chan = remaining;
					remaining = 0;
					if (NGPDPath[path].debug)
						printf("ngpd_set_num_chan: Remaining number of channels on path %d is set as %d\n", thisPath,
								NGPDPath[thisPath].num_chan);
				}
			}
		}
		NGPDPath[path].num_chan = num_chan;
	}
	if (NGPDPath[path].debug)
		printf("ngpd_set_num_chan: Number of channels on path %d is set as %d\n", path, NGPDPath[path].num_chan);
	return NGPD_OK;
}

/**
 * @ingroup fullcontrol
 * Determine which card and consequently the handle, a particular channel is configured on and the channel number within that card,
 * given the top level system handle and the channel number. Also returns the card number
 *
 * @param path a handle to the top level of the xspress3 system returned from {@link ngpd_config()}.
 * @param chan is the number of the channel in the xspress3 system, 0 to ({@link ngpd_get_num_chan()} - 1).
 * @param thisPath the handle to the particular card for the given channel.
 * @param chanIdx the channel within the card specified by the 'thisPath' handle.
 * @param cardP 	Pointer to return the card number within the system of this card. (NULL to omit)
 * @return {@link #NGPD_OK NGPD_OK} or a negative error code.
 */
int ngpd_resolve_path_chan_card(int path, int chan, int *thisPath, int *chanIdx, int *cardP)
{
	// Resolve path and channel indices
	// NOTE: only if FEMS have same nos of channels per board
	// Now even if different, but calibration code may not work.
	int pathIdx;
	int sub_path;
	
	if (chan < 0 || chan >= NGPDPath[path].num_chan)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_resolve_path: chan=%d out of range 0..%d", chan, NGPDPath[path].num_chan - 1);
		return NGPD_ERROR;
	}

	if (NGPDPath[path].type == NGPDComposite)
	{
		pathIdx = 0;
		do
		{
			sub_path = NGPDPath[path].sub_path[pathIdx];
			if (chan < NGPDPath[sub_path].num_chan) // allows less than full channel usage on leaf cards
			{
				*thisPath = sub_path;
				*chanIdx = chan;
				if (cardP != NULL)
					*cardP = pathIdx;
				return 0;
			}
			chan -= NGPDPath[sub_path].num_chan;	// allows less than full channel usage on leaf cards
			pathIdx++;
		} while (pathIdx < NGPDPath[path].num_cards);
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_resolve_path: Ran out of cards: Coding error");

	} else {
		*thisPath = path;
		*chanIdx = chan;
		if (cardP != NULL)
			*cardP = 0;
		if (chan > NGPDPath[path].max_num_chan) {
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_resolve_path: illegal channel (%d) on single device path",
					chan);
			return NGPD_ERROR;
		}
	}
	return NGPD_OK;
}

int ngpd_read_jesd_regs(int path, int card, int link, int offset, int num_regs, u_int32_t *data)
{
	int rc;
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_jesd_regs: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_jesd_regs: not supporteed on ADC14");
		return -1;
	}
	else if (NGPDPath[path].dummy_system)
	{
		int i;
		for (i=0;i<num_regs; i++)
			*data++ = i;
		return 0;
	}
	else
	{
		int this_path;
		if (card < 0 || card >= NGPDPath[path].num_cards)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_jesd_regs: Card=%d out of range 0...%d", card, NGPDPath[path].num_cards-1);
			return -1;
		}
		if (NGPDPath[path].type == NGPDComposite)
		{
			this_path = NGPDPath[path].sub_path[card];
			if (this_path <0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_jesd_regs: Card=%d resolved illegal sub_path=%d", card, this_path);
				return -1;
			}
		}
		else
		{
			this_path = path;
		}
		offset = (link&3) << 10  | offset;
		rc = zynqmp_access_read(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_JESD, ZYNQMP_WIDTH_LONG, offset, num_regs, 1, (u_int8_t *)data);
		if (rc < 0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_jesd_regs: error reading registers: %s", zynqmp_get_error_message());
			return rc;
		}			
		return 0;
	}
}
int ngpd_write_jesd_regs(int path, int card, int link, int offset, int num_regs, u_int32_t *data)
{
	int rc;
	int this_path;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_jesd_regs: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_jesd_regs: not supporteed on ADC14");
		return -1;
	}
	if (NGPDPath[path].dummy_system)
		return 0;
	
	if (card < 0)
	{
		for (card=0; card< NGPDPath[path].num_cards; card++)
		{
			if ((rc=ngpd_write_jesd_regs(path, card, link, offset, num_regs, data)) <0 )
				return rc;
		}
		return 0;
	}
	if (card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_jesd_regs: Card=%d out of range 0...%d", card, NGPDPath[path].num_cards-1);
		return -1;
	}
	if (NGPDPath[path].type == NGPDComposite)
	{
		this_path = NGPDPath[path].sub_path[card];
		if (this_path <0)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_write_jesd_regs: Card=%d resolved illegal sub_path=%d", card, this_path);
			return -1;
		}
	}
	else
	{
		this_path = path;
	}
	offset = (link&3) << 10  | offset;
	rc = zynqmp_access_write(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_JESD, ZYNQMP_WIDTH_LONG, offset, num_regs, 1, (u_int8_t *)data);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_read_jesd_regs: error reading registers: %s", zynqmp_get_error_message());
		return rc;
	}			
	return 0;
}
