#if NGPD_CONF_ADQ14
#include "ADQAPI.h"
#endif
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef LINUX
#include <stdlib.h>
#endif


#include "ngpd.h"

double ngpd_adc_set_range(int path, int chan, double vrange)
{
	float actual=999;
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_adc_set_range: Invalid path=%d", path);
		return -1.0;
	}
	if (NGPDPath[path].generation != NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_adc_set_range: Called on non ADQ14 path");
		return -1.0;
	}
		
#if NGPD_CONF_ADQ14
	if (ADQ_SetInputRange(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, chan+1, (float)vrange, &actual) == 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_adc_set_range: ADQ_SetInputRange failed, actual=%g ", actual);
		return -1.0;
	}
	return actual;
#else
	printf("This version compiled without ADQ14 support. Build with NGPD_CONF_ADQ14 set\n");
	return -1;
#endif
	
}

int ngpd_adc_set_offset(int path, int chan, int codes)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_adc_set_offset: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation != NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_adc_set_offset: Called on non ADQ14 path");
		return -1;
	}
	
#if NGPD_CONF_ADQ14
	if (ADQ_SetAdjustableBias(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, chan+1, codes) == 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_adc_set_offset: ADQ_SetAdjustableBias failed");
		return -1.0;
	}
	return 0;
#else
	printf("This version compiled without ADQ14 support. Build with NGPD_CONF_ADQ14 set\n");
	return -1;
#endif
}

int ngpd_adc_get_range(int path, int chan, float *vrange)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_adc_get_range: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation != NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_adc_get_range: Called on non ADQ14 path");
		return -1;
	}

#if NGPD_CONF_ADQ14
	if (ADQ_GetInputRange(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, chan+1, vrange) == 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_adc_get_range: ADQ_GetInputRange failed");
		return -1;
	}
	return 0;
#else
	printf("This version compiled without ADQ14 support. Build with NGPD_CONF_ADQ14 set\n");
	return -1;
#endif
	
}

int ngpd_adc_get_offset(int path, int chan, int *codes)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_adc_get_offset: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation != NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_adc_get_offset: Called on non ADQ14 path");
		return -1;
	}
	
#if NGPD_CONF_ADQ14
	if (ADQ_GetAdjustableBias(NGPDPath[path].adq_cu, NGPDPath[path].adq_num, chan+1, codes) == 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_adc_get_offset: ADQ_GetAdjustableBias failed");
		return -1;
	}
	return 0;
#else
	printf("This version compiled without ADQ14 support. Build with NGPD_CONF_ADQ14 set\n");
	return -1;
#endif
}
