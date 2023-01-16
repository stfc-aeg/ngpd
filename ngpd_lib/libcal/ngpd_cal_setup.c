/* 	
	ngpd_cal_setup.c
	ngpd calibration library
	Created: wih73   28/10/2021
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "datamod.h"
#include "ngpd.h"
#include "ngpd_cal.h"

extern char ngpd_cal_error_message[NGPD_MAX_ERROR_MESSAGE+2];


char * ngpd_cal_get_error_message()
{
	return ngpd_cal_error_message;
}

int ngpd_cal_setup_all_ana(int path,  int card, int sync_boxes, int src)
{
	NGPDGeneration generation = ngpd_get_generation(path);
	int stream;
	int rc;
	NGPDScopeOptions scope_options;
	NGPDScopeModule * scope_mod;

	scope_mod = ngpd_scope_get_mod(path);
	if (scope_mod == NULL)
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_ana: Cannot determine scope module pointer for path %d : %s", path, ngpd_get_error_message());
		return -1;
	}

	if (generation == NGPDGenADQ14)
	{
		int chans_per_card = ngpd_get_chans_per_card(path);

		scope_options.nstreams = chans_per_card;

#if 0
		if (sync_boxes)
			scope_options.flags = NGPDScopeOpt_Synchronised;	// Turn on delayed start mode, to try to sync boxes.
		else
			scope_options.flags = 0; 	// Turn off delayed start mode, not required.
#endif
		rc = ngpd_set_scope_options(path, -1, &scope_options);
		if (rc < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_ana: Error setting scope options nstreams= %d: %s\n", scope_options.nstreams, ngpd_get_error_message());
			return -1;
		}

		for (stream=0; stream<chans_per_card; stream++)
		{
			if ((rc=ngpd_scope_setup_stream(path, card, stream, stream, src, 0)) < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_ana: Error setting scope stream %d: %s\n", stream, ngpd_get_error_message());
				return -1;
			}
		}
	}
	else if (generation == NGPDGenZynqMP1)
	{
		int chans_per_card = ngpd_get_chans_per_card(path);

		scope_options.nstreams = 8;

#if 0
		if (sync_boxes)
			scope_options.flags = NGPDScopeOpt_Synchronised;	// Turn on delayed start mode, to try to sync boxes.
		else
			scope_options.flags = 0; 	// Turn off delayed start mode, not required.
#endif
		rc = ngpd_set_scope_options(path, -1, &scope_options);
		if (rc < 0)
		{
			snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_ana: Error setting scope options nstreams= %d: %s\n", scope_options.nstreams, ngpd_get_error_message());
			return -1;
		}
		for (stream=0; stream<8; stream++)
		{
			int chan = stream;
			chan %= chans_per_card;
			if ((rc=ngpd_scope_setup_stream(path, card, stream, chan, src, 0)) < 0)
			{
				snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_ana: Error setting scope stream %d: %s", stream, ngpd_get_error_message());
				return -1;
			}
		}
	}
	else
	{
		snprintf(ngpd_cal_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_cal_setup_all_ana: Error Unknown NGPD generation %d\n", generation);
		return -1;
	}
	return 0;
}
