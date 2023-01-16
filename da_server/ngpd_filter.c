/*----------------------------------------------------------------------
 *	Neutron Gamma Pulse Discriminator Device for DA with multi-client server.
 *	By William Helsby
 *----------------------------------------------------------------------
 */

#include "header.h"
#include <ngpd.h>



/*----- Private global variables -----*/
/*----- global variables -----*/
/*----- Private function prototypes -----*/

int ngpd_filter_generate(int quals, int cn, int path, int chan);
char * ngpd_filter_get(int quals, int cn, int path, int chan);


PARSE_TREE ngpd_filter_tree[] = {
	PT_COMMAND ("generate", "%d %d ave %d exp %g gaussian %g trap-top %d trap-bot %d = %d",
			    "Generate various filter functions",
			    "Path/Channel/Simple running average or 1..n points/exp(-|t div T|) where t is in samples/Gaussian with sigma in samples/"
				"Trapezoidal filter with bot top and bottom width specified",
			    ngpd_filter_generate),
	PT_COMMAND ("get", "%d %d = %s",
				"Enquire how filter was last set up",
				"Path/Channel",
			    ngpd_filter_get),
	PT_END
};


int ngpd_filter_generate(int quals, int cn, int path, int chan)
{
	int rc;
	int i;
	int num_ave;
	double Tsamples, sigma;
	int trap_top, trap_bot;
	
	switch (quals)
	{
	case 1:
		num_ave = Qualifier[0].IntArg;
		if (num_ave < 1 || num_ave >= 2*NGPD_FILT_NUM_COEFF)
		{
			sv_printf(cn, "! ERROR: Please specify num_ave in range 1...%d not %d\n",  2*NGPD_FILT_NUM_COEFF-1, num_ave);
			return -1;
		}
		rc = ngpd_filter_load_rect( path, chan, num_ave);
		break;

	case 2:
		Tsamples = Qualifier[1].DoubleArg;
		if (Tsamples < 1 || Tsamples >= 2*NGPD_FILT_NUM_COEFF)
		{
			sv_printf(cn, "! ERROR: Please specify Tsamples in range 1...%d not %g\n",  2*NGPD_FILT_NUM_COEFF-1, Tsamples);
			return -1;
		}
		rc = ngpd_filter_load_exp(path, chan, Tsamples);
		break;

	case 4:
		sigma = Qualifier[2].DoubleArg;
		if (sigma < 1 || sigma >= 2*NGPD_FILT_NUM_COEFF)
		{
			sv_printf(cn, "! ERROR: Please specify sigma (in samples) in range 1...%d not %g\n",  2*NGPD_FILT_NUM_COEFF-1, sigma);
			return -1;
		}
		rc = ngpd_filter_load_gaus(path, chan, sigma);
		break;
		
	case 8:
	case 16:
		sv_printf(cn, "! ERROR: Please specify both trapezoidal filter top and bootm width\n");
		return -1;
		
	case 0x18:
		trap_top = Qualifier[3].IntArg;
		trap_bot = Qualifier[4].IntArg;
		rc = ngpd_filter_load_trapeziod(path, chan, trap_top, trap_bot);
		break;

	default:
		sv_printf(cn, "! Please specify one and only one filter type\n");
		return -1;
	}

	if (rc < 0)
		sv_printf(cn, "! ERROR generating filter: %s\n", ngpd_get_error_message());
	return rc;
}

char * ngpd_filter_get(int quals, int cn, int path, int chan)
{
	char filter_desc[100];
	NGPDFilter filter;
	
	if (ngpd_get_filter_type(path, chan, &filter) < 0)
	{
		sv_printf(cn, "! ERROR: Cannot get filter type : %s\n", ngpd_get_error_message());
		return NULL;
	}
	switch (filter.type)
	{
	case NGPDFilter_Unknown:
		strcpy(filter_desc, "Unknown");
		break;
		
	case NGPDFilter_Rectangle:
		sprintf(filter_desc,"ave %d", filter.iarg1);
		break;
		
	case NGPDFilter_Gaussian:
		sprintf(filter_desc,"gaussian %e", filter.darg);
		break;
	
	case NGPDFilter_Exponential:
		sprintf(filter_desc,"exp %e", filter.darg);
		break;
	
	case NGPDFilter_Trapezoidal:
		sprintf(filter_desc,"trap-top %d trap-bot %d", filter.iarg1, filter.iarg2);
		break;
	
	default:
		sprintf(filter_desc,"Unknown type %d", filter.type);
		break;
	}
	return strdup (filter_desc);
}
