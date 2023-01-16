/* setup functions for ngpd calibration modes to abstract any future differences in configuring scope mode */

#include "header.h"
#include "peak_fit.h"
#include "ngpd.h"

int ngpd_calib_setup_trig_or_measure(int cn, int path,  int card, int use_measure, int do_upper, int *first_stream, int *last_stream,  int *trig_offset, int *wide_a_offset, int *wide_b_offset, int *neutron_offset, int sync_boxes, int *need_upper)
{
	NGPDGeneration generation = ngpd_get_generation(path);
	int stream;
	u_int32_t scope_src;
	u_int32_t scope_chan;
	u_int32_t scope_alt;
	int rc;
	NGPDScopeOptions scope_options;
	NGPDScopeModule * scope_mod;

	scope_mod = ngpd_scope_get_mod(path);
	if (scope_mod == NULL)
	{
		sv_printf(cn, "! ERROR: Cannot determine scope module pointer for path %d : %s\n", path, ngpd_get_error_message());
		return -1;
	}

	if (generation == NGPDGenADQ14)
	{
		int chans_per_card = ngpd_get_chans_per_card(path);

		if (chans_per_card == 1)
			scope_options.nstreams = 2;
		else
			scope_options.nstreams = 3;

#if 0
		if (sync_boxes)
			scope_options.flags = NGPDScopeOpt_Synchronised;	// Turn on delayed start mode, to try to sync boxes.
		else
			scope_options.flags = 0; 	// Turn off delayed start mode, not required.
#endif
		rc = ngpd_set_scope_options(path, -1, &scope_options);
		if (rc < 0)
		{
			sv_printf(cn, "! Error setting scope options nstreams= %d: %s\n", scope_options.nstreams, ngpd_get_error_message());
			return -1;
		}

		for (stream=0; stream<chans_per_card; stream++)
		{
			scope_src = use_measure?NGPD_SCOPE_SEL0TO5_MEASURE:NGPD_SCOPE_SEL0TO5_DIFF_TRIG_OUT;

			if ((rc=ngpd_scope_setup_stream(path, card, stream, stream, scope_src, 0)) < 0)
			{
				sv_printf(cn, "! Error setting scope stream %d: %s\n", stream, ngpd_get_error_message());
				return -1;
			}
		}
		if ((rc=ngpd_scope_setup_stream(path, card, scope_options.nstreams-1, 0, NGPD_SCOPE_SEL5_DIG_5BIT, 0)) < 0)
		{
			sv_printf(cn, "! Error setting scope stream %d: %s\n", 5, ngpd_get_error_message());
			return -1;
		}
		*first_stream = 0;
		*last_stream = chans_per_card-1;
		if (use_measure)
		{
			*trig_offset = 0;
			*wide_a_offset = -1;
			*wide_b_offset = -1;
			*neutron_offset = -1;
		}
		else
		{
			*trig_offset = 2;
			*wide_a_offset = 3;
			*wide_b_offset = 4;
			*neutron_offset = -1;
		}
		if (need_upper != NULL)
			*need_upper = 0;
	}
	else if (generation == NGPDGenZynqMP1)
	{
		int chans_per_card = ngpd_get_chans_per_card(path);
		int dma;

		if (need_upper != NULL)
		{
			if (chans_per_card > 6)
				*need_upper = 1;
			else
				*need_upper = 0;
		}
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
			sv_printf(cn, "! Error setting scope options nstreams= %d: %s\n", scope_options.nstreams, ngpd_get_error_message());
			return -1;
		}
		scope_src = use_measure?NGZMP_SCOPE_SEL_ANA_MEASURE:NGZMP_SCOPE_SEL_ANA_DIFF_TRIG_OUT;
		for (dma=0; dma<2; dma++)
		{
			for (stream=0; stream<3; stream++)
			{
				int chan = stream+dma*3+do_upper*4;
				chan %= chans_per_card;
				if ((rc=ngpd_scope_setup_stream(path, card, stream+dma*4, chan, scope_src, 0)) < 0)
				{
					sv_printf(cn, "! Error setting scope stream %d: %s\n", stream+dma*4, ngpd_get_error_message());
					return -1;
				}
			}
			if ((rc=ngpd_scope_setup_stream(path, card, 3+dma*4, 0, NGZMP_SCOPE_SEL37_DIG_5BIT, 0)) < 0)
			{
				sv_printf(cn, "! Error setting scope stream %d: %s\n",  3+dma*4, ngpd_get_error_message());
				return -1;
			}
		}
		*first_stream = 0;
		*last_stream = 6;
		if (use_measure)
		{
			*trig_offset = 0;
			*wide_a_offset = -1;
			*wide_b_offset = -1;
			*neutron_offset = -1;
		}
		else
		{
			*trig_offset = 2;
			*wide_a_offset = 3;
			*wide_b_offset = 4;
			*neutron_offset = -1;
		}
	}
	else
	{
		sv_printf(cn, "! ERROR: Unknown NGPD generation %d\n", generation);
		return -1;
	}
	return ngpd_scope_resize_read_handle(cn, path);
}

int ngpd_calib_setup_trig_or_measure_get_chan(int cn, int path, int use_measure, int do_upper, int stream)
{
	NGPDGeneration generation = ngpd_get_generation(path);
	int nstreams;
	int chans_per_card = ngpd_get_chans_per_card(path);

	if (generation == NGPDGenADQ14)
	{
		if (chans_per_card == 1)
			nstreams = 2;
		else
			nstreams = 3;

		if (stream < 0 || stream >= nstreams-1)
			return -1;
		return stream;
	}
	else if (generation == NGPDGenZynqMP1)
	{
		if (stream <0 || stream == 3 || stream >= 5)
			return -1;
		if (stream <= 2)
			return stream+do_upper*4;
		if (stream == 4)
			return 3+do_upper*4;
	}
	return -1;
}

int ngpd_calib_setup_all_chan(int cn, int path,  int card, int scope_src, int *first_stream, int *last_stream,  int *trig_offset, int *wide_a_offset, int *wide_b_offset, int *neutron_offset, int sync_boxes)
{
	NGPDGeneration generation = ngpd_get_generation(path);
	int stream;
	u_int32_t scope_chan;
	u_int32_t scope_alt;
	int rc;
	NGPDScopeOptions scope_options;
	NGPDScopeModule * scope_mod;

	scope_mod = ngpd_scope_get_mod(path);
	if (scope_mod == NULL)
	{
		sv_printf(cn, "! ERROR: Cannot determine scope module pointer for path %d : %s\n", path, ngpd_get_error_message());
		return -1;
	}

	if (generation == NGPDGenADQ14)
	{
		int chans_per_card = ngpd_get_chans_per_card(path);

		if (chans_per_card == 1)
			scope_options.nstreams = 2;
		else
			scope_options.nstreams = 4;

#if 0
		if (sync_boxes)
			scope_options.flags = NGPDScopeOpt_Synchronised;	// Turn on delayed start mode, to try to sync boxes.
		else
			scope_options.flags = 0; 	// Turn off delayed start mode, not required.
#endif
		rc = ngpd_set_scope_options(path, -1, &scope_options);
		if (rc < 0)
		{
			sv_printf(cn, "! Error setting scope options nstreams= %d: %s\n", scope_options.nstreams, ngpd_get_error_message());
			return -1;
		}

		for (stream=0; stream<chans_per_card; stream++)
		{
			if ((rc=ngpd_scope_setup_stream(path, card, stream, stream, scope_src, 0)) < 0)
			{
				sv_printf(cn, "! Error setting scope stream %d: %s\n", stream, ngpd_get_error_message());
				return -1;
			}
		}
		if ((rc=ngpd_scope_setup_stream(path, card, scope_options.nstreams-1, 0, NGPD_SCOPE_SEL5_DIG_5BIT, 0)) < 0)
		{
			sv_printf(cn, "! Error setting scope stream %d: %s\n", 5, ngpd_get_error_message());
			return -1;
		}
		*first_stream = 0;
		*last_stream = chans_per_card-1;

		if (scope_src == NGPD_SCOPE_SEL0TO5_MEASURE)
		{
			*trig_offset = 0;
			*wide_a_offset = -1;
			*wide_b_offset = -1;
			*neutron_offset = -1;
		}
		else if (scope_src == NGPD_SCOPE_SEL0TO5_DIFF_TRIG_OUT)
		{
			*trig_offset = 2;
			*wide_a_offset = 3;
			*wide_b_offset = 4;
			*neutron_offset = -1;
		}
		else
		{
			*trig_offset = -1;
			*wide_a_offset = -1;
			*wide_b_offset = -1;
			*neutron_offset = -1;
		}
	}
	else if (generation == NGPDGenZynqMP1)
	{
		int chans_per_card = ngpd_get_chans_per_card(path);

		scope_options.nstreams = 10;

#if 0
		if (sync_boxes)
			scope_options.flags = NGPDScopeOpt_Synchronised;	// Turn on delayed start mode, to try to sync boxes.
		else
			scope_options.flags = 0; 	// Turn off delayed start mode, not required.
#endif
		rc = ngpd_set_scope_options(path, -1, &scope_options);
		if (rc < 0)
		{
			sv_printf(cn, "! Error setting scope options nstreams= %d: %s\n", scope_options.nstreams, ngpd_get_error_message());
			return -1;
		}
		for (stream=0; stream<8; stream++)
		{
			int chan = stream;
			chan %= chans_per_card;
			if ((rc=ngpd_scope_setup_stream(path, card, stream, chan, scope_src, 0)) < 0)
			{
				sv_printf(cn, "! Error setting scope stream %d: %s\n", stream, ngpd_get_error_message());
				return -1;
			}
		}
		if ((rc=ngpd_scope_setup_stream(path, card, 8, 0, NGZMP_SCOPE_SEL8_DIG_4BIT, 0)) < 0)
		{
			sv_printf(cn, "! Error setting scope stream %d: %s\n", 8, ngpd_get_error_message());
			return -1;
		}
		if ((rc=ngpd_scope_setup_stream(path, card, 9, 0, NGZMP_SCOPE_SEL9_DIG_4BIT, 0)) < 0)
		{
			sv_printf(cn, "! Error setting scope stream %d: %s\n", 9, ngpd_get_error_message());
			return -1;
		}
		*first_stream = 0;
		*last_stream = 7;
		if (scope_src == NGZMP_SCOPE_SEL_ANA_MEASURE)
		{
			*trig_offset = 0;
			*wide_a_offset = -1;
			*wide_b_offset = -1;
			*neutron_offset = -1;
		}
		else if (scope_src == NGZMP_SCOPE_SEL_ANA_DIFF_TRIG_OUT)
		{
			*trig_offset = 2;
			*wide_a_offset = 3;
			*wide_b_offset = -1;
			*neutron_offset = -1;
		}
		else
		{
			*trig_offset = -1;
			*wide_a_offset = -1;
			*wide_b_offset = -1;
			*neutron_offset = -1;
		}
	}
	else
	{
		sv_printf(cn, "! ERROR: Unknown NGPD generation %d\n", generation);
		return -1;
	}
	return ngpd_scope_resize_read_handle(cn, path);
}

#if 1
int ngpd_calib_setup_inp(int cn, int path,  int card, int sync_boxes)
{
	int rc;
	rc = ngpd_cal_setup_all_ana(path, card, sync_boxes, NGZMP_SCOPE_SEL_ANA_INPUT);
	if (rc < 0)
	{
		sv_printf(cn, "! ERROR: %s\n", ngpd_cal_get_error_message());
		return rc;
	}

	return ngpd_scope_resize_read_handle(cn, path);
}
#else
int ngpd_calib_setup_inp(int cn, int path,  int card, int sync_boxes)
{
	NGPDGeneration generation = ngpd_get_generation(path);
	int stream;
	u_int32_t scope_chan;
	u_int32_t scope_alt;
	int rc;
	NGPDScopeOptions scope_options;
	NGPDScopeModule * scope_mod;

	scope_mod = ngpd_scope_get_mod(path);
	if (scope_mod == NULL)
	{
		sv_printf(cn, "! ERROR: Cannot determine scope module pointer for path %d : %s\n", path, ngpd_get_error_message());
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
			sv_printf(cn, "! Error setting scope options nstreams= %d: %s\n", scope_options.nstreams, ngpd_get_error_message());
			return -1;
		}

		for (stream=0; stream<chans_per_card; stream++)
		{
			if ((rc=ngpd_scope_setup_stream(path, card, stream, stream, NGZMP_SCOPE_SEL_ANA_INPUT, 0)) < 0)
			{
				sv_printf(cn, "! Error setting scope stream %d: %s\n", stream, ngpd_get_error_message());
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
			sv_printf(cn, "! Error setting scope options nstreams= %d: %s\n", scope_options.nstreams, ngpd_get_error_message());
			return -1;
		}
		for (stream=0; stream<8; stream++)
		{
			int chan = stream;
			chan %= chans_per_card;
			if ((rc=ngpd_scope_setup_stream(path, card, stream, chan, NGZMP_SCOPE_SEL_ANA_INPUT, 0)) < 0)
			{
				sv_printf(cn, "! Error setting scope stream %d: %s\n", stream, ngpd_get_error_message());
				return -1;
			}
		}
	}
	else
	{
		sv_printf(cn, "! ERROR: Unknown NGPD generation %d\n", generation);
		return -1;
	}
	return ngpd_scope_resize_read_handle(cn, path);
}
#endif