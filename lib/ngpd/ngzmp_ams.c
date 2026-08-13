#if NGPD_CONF_ADQ14
#include "ADQAPI.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
//#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#ifdef LINUX
#include <stdlib.h>
#endif


#include "ngpd.h"
#include "zynqmp_protocol.h"
#include "zynqmp_api.h"

static int ngzmp_dummy_read_xadc(int path, int card, int first, int num, int32_t *data);

/**
 * @ingroup fullcontrol
 * Read temperatures and voltages from the ZynqMP system monitors.
 * The data point numbering concatenates the values read from the AMS and the XADC.  See {@link NGZMP_SysMon_Param}
 *
 * @param path a handle to the top level of the xspress3 system returned from {@link xsp3_config()}.
 * @param card is the card number int t he system 0... ngzmp_get_num_cards()-1
 * @param first 	First data value to read. 
 * @param num		Number of data values to read from one card.
 * @param data 		Pointer to buffer to return data as 32  bit ints, measureing voltages in mV and temperatures in mDegree
 * @return 			0 on success or a negative error code.
 */
int ngzmp_read_xadc(int path, int card, int first, int num, int32_t *data)
{
	int rc;
	int this_path;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_read_xadc: Invalid path=%d", path);
		return -1;
	}

	if (card <0 || card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_read_xadc: Invalid card=%d", card);
		return -1;
	}
	if (NGPDPath[path].type == NGPDComposite)
	{
		this_path = NGPDPath[path].sub_path[card];
	}
	else
		this_path = path;
	if (NGPDPath[path].dummy_system & NGPDDummy_FPGA)
	{
		return ngzmp_dummy_read_xadc(path, card, first, num, data);
	}

	rc = zynqmp_access_read(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_XADC, ZYNQMP_WIDTH_LONG, first, num, 1, (u_int8_t *)data);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_read_xadc: %s", zynqmp_get_error_message());
		return rc;
	}
	return 0;
}

static int ngzmp_dummy_read_xadc(int path, int card, int first, int num, int32_t *data)
{
	int i;
	static int rdnum;
	
	for (i=0;i<num; i++)
	{
		switch(i+first)
		{
		case NGZMP_AMS_PSTempLPD:	*data++ = (50+5*card)*1000+ 10*rdnum; break;
		case NGZMP_AMS_PSTempFPD:	*data++ = (53+5*card)*1000+ 10*rdnum; break;
		case NGZMP_AMS_PSVccIntLP: 	*data++ =   850+10*card+rdnum; break;
		case NGZMP_AMS_PSVccIntFP: 	*data++ =   852+10*card+rdnum; break;
		case NGZMP_AMS_PSVccAux: 	*data++ =  1800+10*card+rdnum; break;
		case NGZMP_AMS_PSVccDDR: 	*data++ =  1200+10*card+rdnum; break;
		case NGZMP_AMS_PSVccIO0: 	*data++ =  1800+10*card+rdnum; break;
		case NGZMP_AMS_PSVccIO1: 	*data++ =  1801+10*card+rdnum; break;
		case NGZMP_AMS_PSVccIO2: 	*data++ =  1802+10*card+rdnum; break;
		case NGZMP_AMS_PSVccIO3: 	*data++ =  1803+10*card+rdnum; break;
		case NGZMP_AMS_PSAVccMGTR: 	*data++ =   900+10*card+rdnum; break;
		case NGZMP_AMS_PSAVttMGTR: 	*data++ =  1200+10*card+rdnum; break;
		case NGZMP_AMS_PSVccAMS: 	*data++ =  1800+10*card+rdnum; break;
		case NGZMP_AMS_PSVccPLL0: 	*data++ =  1200+10*card+rdnum; break;
		case NGZMP_AMS_PSVccBatt: 	*data++ =  1500+10*card+rdnum; break;
		case NGZMP_AMS_PLVccInt: 	*data++ =   852+10*card+rdnum; break;
		case NGZMP_AMS_PLVccBRAM: 	*data++ =  852+10*card+rdnum; break;
		case NGZMP_AMS_PLVccAux:	*data++ =  1800+10*card+rdnum; break;
		case NGZMP_AMS_PSVccDDRPLL:	*data++ =  1800+10*card+rdnum; break;
		case NGZMP_AMS_PSVccIntFPDDR:*data++ =  852+10*card+rdnum; break;
		case NGZMP_XADC_VccInt:		*data++ =   852+10*card+rdnum; break;
  		case NGZMP_XADC_VccAux:		*data++ =  1800+10*card+rdnum; break;
		case NGZMP_XADC_VccBRam:	*data++ =   850+10*card+rdnum; break;
		case NGZMP_XADC_VccPInt:	*data++ =   852+10*card+rdnum; break;
		case NGZMP_XADC_VccPAux:	*data++ =  1800+10*card+rdnum; break;
		case NGZMP_XADC_VccoDdr:	*data++ =   900+10*card+rdnum; break;
		case NGZMP_XADC_Temp:		*data++ = (50+5*card)*1000+ 10*rdnum; break;
		default : *data++ = 0;
		}
	}
	rdnum = (rdnum+1) % 10;
	return 0;
}
