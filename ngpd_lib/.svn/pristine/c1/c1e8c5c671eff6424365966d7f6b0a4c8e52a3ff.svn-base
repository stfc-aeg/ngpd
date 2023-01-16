/* ngzmp_hardware_config.c
	This file will contain code to extract information on hardware limits that change with hardware build time configuration
	*/
#include <stdio.h>
#include "ngpd.h"
#include "ngzmp.h"

int ngzmp_bram_size_table[NGZMP_NUM_REGIONS]   = { 32,          256,             256,           1024,         1024       };
int ngzmp_bram_width_table[NGZMP_NUM_REGIONS]  = { 32,          32,              32,            18,           18         };
const char *ngzmp_bram_name[NGZMP_NUM_REGIONS] = { "Registers", "Tail Thres M", "Tail Thres C", "Tail Sub0", "Tail Sub1" };

extern char ngpd_error_message[];


int ngzmp_get_playback_nstreams(int path, int card)
{
	return 12;
}

int ngzmp_bram_size(int path, int chan, int region_num)
{

	if (region_num < 0 || region_num >= NGZMP_NUM_REGIONS)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_bram_size: Invalid region number %d", region_num);
		return NGPD_ERROR;
	}

	return ngzmp_bram_size_table[region_num];
}

int ngzmp_has_bram(int path, int chan, int region_num)
{
	if (region_num < 0 || region_num >= NGZMP_NUM_REGIONS)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_bram_size: Invalid region number %d", region_num);
		return 0;
	}
	return !!(ngzmp_bram_size_table[region_num]);
}