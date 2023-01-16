/* ngzmp_hardware_config.c
	This file will contain code to extract information on hardware limits that change with hardware build time configuration
	*/
#include <stdio.h>
#include "ngpd.h"
#include "ngzmp.h"

int ngzmp_bram_size_table[NGZMP_NUM_REGIONS]   = { 32,          256,             256,           1024,         1024       };
int ngzmp_bram_width_table[NGZMP_NUM_REGIONS]  = { 32,          32,              32,            18,           18         };
const char *ngzmp_bram_name[NGZMP_NUM_REGIONS] = { "Registers", "Tail Thres M", "Tail Thres C", "Tail Sub0", "Tail Sub1" };


const char *ngzmp_sys_mon_name[NGZMP_ParamMax] = {"AMS_PSTempLPD", "AMS_PSTempFPD", "AMS_PSVccIntLP", "AMS_PSVccIntFP", "AMS_PSVccAux", "AMS_PSVccDDR", "AMS_PSVccIO0", "AMS_PSVccIO1", "AMS_PSVccIO2", "AMS_PSVccIO3",
	"AMS_PSAVccMGTR", "AMS_PSAVttMGTR", "AMS_PSVccAMS", "AMS_PSVccPLL0", "AMS_PSVccBatt", "AMS_PLVccInt", "AMS_PLVccBRAM", "AMS_PLVccAux", "AMS_PSVccDDRPLL", "AMS_PSVccIntFPDDR", 
	"XADC_VccInt", "XADC_VccAux", "XADC_VccBRam", "XADC_VccPSInt", "XADC_VccPSAux", "XADC_VccoDdr", "XADC_Temp" };

extern char ngpd_error_message[];


int ngpd_get_playback_nstreams(int path, int card)
{
	return 12;
}

int ngpd_bram_size(int path, int chan, int region_num)
{

	if (region_num < 0 || region_num >= NGZMP_NUM_REGIONS)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_bram_size: Invalid region number %d", region_num);
		return NGPD_ERROR;
	}

	return ngzmp_bram_size_table[region_num];
}

int ngpd_has_bram(int path, int chan, int region_num)
{
	if (region_num < 0 || region_num >= NGZMP_NUM_REGIONS)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_bram_size: Invalid region number %d", region_num);
		return 0;
	}
	return !!(ngzmp_bram_size_table[region_num]);
}