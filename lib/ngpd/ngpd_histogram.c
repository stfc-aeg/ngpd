#if NGPD_CONF_ADQ14
#include "ADQAPI.h"
#endif
#include <stdio.h>

#include <stdlib.h>
//#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>

#include "datamod.h"
#include "ngpd.h"


int ngpd_hist_setup(int path, int max_height, int max_tail_sum, int max_fall_time, double ratio_scale, int nbins_height, int nbins_tail_sum, int nbins_fall_time, int nbins_ratio, NGPDHistEnables enables)
{
	char mod_name[NGPD_MAX_MOD_NAME+2];
	int div;
	int num_ngp=1;
	char * suffix="_all";
	char *t_label="All";
	char *t_label3d="Channel";

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_hist_setup: Invalid path=%d", path);
		return -1;
	}
	
	if (enables & NGPDHistSeparateNeutrons)
	{
		if (enables & NGPDHistDiscardPileup)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_hist_setup: NGPDHistDiscardPileup cannot be used with NGPDHistSeparateNeutrons");
			return -1;
		}

		num_ngp=3;
		suffix = "_ngp";
		t_label = "Neutron/Gamma/Pileup";
		t_label3d="Chan:Neutron/Gamma/Pileup";
	}
	if ((enables & (NGPDHistEnbHeight | NGPDHistEnbHgtTailSum | NGPDHistEnbHgtFallTime | NGPDHistEnbHgtTailRatio)) &&  (max_height < 1 || nbins_height < 2))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_hist_setup: Requested histogram require max height and nbins height, but max_height=%d, nbins_height=%d", max_height, nbins_height);
		return -1;
	}

	if ((enables & (NGPDHistEnbTailSum | NGPDHistEnbHgtTailSum)) &&  (max_tail_sum < 1 || nbins_tail_sum < 2))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_hist_setup: Requested histogram require max tail sum and nbins tail sum, but max_tail_sum=%d, nbins_tail_sum=%d", max_tail_sum, nbins_tail_sum);
		return -1;
	}
	 
	if ((enables & (NGPDHistEnbFallTime |  NGPDHistEnbHgtFallTime)) &&  (max_fall_time < 1 || nbins_fall_time < 2))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_hist_setup: Requested histogram require max_fall_time and nbins_fall_time, but max_fall_time=%d, nbins_height=%d", max_fall_time, nbins_fall_time);
		return -1;
	}
	if ((enables & ( NGPDHistEnbTailRatio | NGPDHistEnbHgtTailRatio)) && (ratio_scale == 0 || nbins_ratio < 2))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_hist_setup: Requested histogram require tail ratio, but ratio_scale=%g, nbins_ratio=%d", ratio_scale, nbins_ratio);
		return -1;
	}
	if (NGPDPath[path].histogram.height_mod != NULL && (NGPDPath[path].histogram.height_mod->head.num_x != nbins_height || NGPDPath[path].histogram.height_mod->head.num_t != num_ngp || !(enables & NGPDHistEnbHeight)) )
	{
		munlink(NGPDPath[path].histogram.height_head);
		NGPDPath[path].histogram.height_mod = NULL;
	}
	if (NGPDPath[path].histogram.tail_sum_mod != NULL && (NGPDPath[path].histogram.tail_sum_mod->head.num_x != nbins_tail_sum || NGPDPath[path].histogram.tail_sum_mod->head.num_t != num_ngp || !(enables & NGPDHistEnbTailSum)))
	{
		munlink(NGPDPath[path].histogram.tail_sum_head);
		NGPDPath[path].histogram.tail_sum_mod = NULL;
	}
	if (NGPDPath[path].histogram.fall_time_mod != NULL && (NGPDPath[path].histogram.fall_time_mod->head.num_x != nbins_fall_time ||  NGPDPath[path].histogram.fall_time_mod->head.num_t != num_ngp ||!(enables & NGPDHistEnbFallTime)))
	{
		munlink(NGPDPath[path].histogram.fall_time_head);
		NGPDPath[path].histogram.fall_time_mod = NULL;
	}
	if (NGPDPath[path].histogram.tail_ratio_mod != NULL && (NGPDPath[path].histogram.tail_ratio_mod->head.num_x != nbins_ratio ||  NGPDPath[path].histogram.tail_ratio_mod->head.num_t != num_ngp || !(enables & NGPDHistEnbTailRatio)))
	{
		munlink(NGPDPath[path].histogram.tail_ratio_head);
		NGPDPath[path].histogram.tail_ratio_mod = NULL;
	}
	if (NGPDPath[path].histogram.hgt_tail_sum_mod != NULL && (NGPDPath[path].histogram.hgt_tail_sum_mod->head.num_x != nbins_height || NGPDPath[path].histogram.hgt_tail_sum_mod->head.num_y != nbins_tail_sum || NGPDPath[path].histogram.hgt_tail_sum_mod->head.num_t != NGPDPath[path].num_chan*num_ngp || !(enables & NGPDHistEnbHgtTailSum))) 
	{
		munlink(NGPDPath[path].histogram.hgt_tail_sum_head);
		NGPDPath[path].histogram.hgt_tail_sum_mod = NULL;
	}
	if (NGPDPath[path].histogram.hgt_fall_time_mod != NULL && (NGPDPath[path].histogram.hgt_fall_time_mod->head.num_x != nbins_height || NGPDPath[path].histogram.hgt_fall_time_mod->head.num_y != nbins_fall_time || NGPDPath[path].histogram.hgt_fall_time_mod->head.num_t != NGPDPath[path].num_chan*num_ngp  || !(enables & NGPDHistEnbHgtFallTime))) 
	{
		munlink(NGPDPath[path].histogram.hgt_fall_time_head);
		NGPDPath[path].histogram.hgt_fall_time_mod = NULL;
	}
	if (NGPDPath[path].histogram.hgt_tail_ratio_mod != NULL && (NGPDPath[path].histogram.hgt_tail_ratio_mod->head.num_x != nbins_height || NGPDPath[path].histogram.hgt_tail_ratio_mod->head.num_y != nbins_ratio || NGPDPath[path].histogram.hgt_tail_ratio_mod->head.num_t != NGPDPath[path].num_chan*num_ngp  || !(enables & NGPDHistEnbHgtTailRatio))) 
	{
		munlink(NGPDPath[path].histogram.hgt_tail_ratio_head);
		NGPDPath[path].histogram.hgt_tail_ratio_mod = NULL;
	}

	if (enables & (NGPDHistEnbHeight | NGPDHistEnbHgtTailSum |  NGPDHistEnbHgtFallTime))
	{
		div = (max_height+1)/nbins_height;
		if (div == 0) div = 1;
		NGPDPath[path].histogram.hgt_divide = div;
	}
	else
		NGPDPath[path].histogram.hgt_divide = 1;

	if (enables & (NGPDHistEnbTailSum | NGPDHistEnbHgtTailSum))
	{
		div = (max_tail_sum+1)/nbins_tail_sum;
		if (div == 0) div = 1;
		NGPDPath[path].histogram.tail_sum_divide = div;
	}
	else
		NGPDPath[path].histogram.tail_sum_divide = 1;

	if (enables & (NGPDHistEnbFallTime |  NGPDHistEnbHgtFallTime))
	{
		div = (max_fall_time+1)/nbins_fall_time;
		if (div == 0) div = 1;
		NGPDPath[path].histogram.fall_time_divide = div;
	}
	else
		NGPDPath[path].histogram.fall_time_divide = 1;

	NGPDPath[path].histogram.ratio_scale = ratio_scale;

	if (NGPDPath[path].histogram.height_mod == NULL && (enables & NGPDHistEnbHeight))
	{
		sprintf(mod_name, "ngpd%d_hist_height%d%s", NGPDPath[path].adq_num, nbins_height, suffix);
		NGPDPath[path].histogram.height_mod = id_mkmod3d (mod_name, nbins_height, NGPDPath[path].num_chan, num_ngp, "Height", "Channel", t_label, NULL, DATA_LONG, &(NGPDPath[path].histogram.height_head));
		if (NGPDPath[path].histogram.height_mod==NULL)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_hist_setup: Problem creating module %s", mod_name);
			return -1;
		}
	}
	if (enables & NGPDHistEnbHeight)
		sprintf(NGPDPath[path].histogram.height_mod->head.x_label, "Height/%d", NGPDPath[path].histogram.hgt_divide);

	if (NGPDPath[path].histogram.tail_sum_mod == NULL && (enables & NGPDHistEnbTailSum))
	{
		sprintf(mod_name, "ngpd%d_hist_tail_sum%d%s", NGPDPath[path].adq_num, nbins_tail_sum, suffix);
		NGPDPath[path].histogram.tail_sum_mod = id_mkmod3d (mod_name, nbins_tail_sum, NGPDPath[path].num_chan, num_ngp, "Tail Sum", "Channel", t_label, NULL, DATA_LONG, &(NGPDPath[path].histogram.tail_sum_head));
		if (NGPDPath[path].histogram.tail_sum_mod==NULL)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_hist_setup: Problem creating module %s", mod_name);
			return -1;
		}
	}
	if (enables & NGPDHistEnbTailSum)
		sprintf(NGPDPath[path].histogram.tail_sum_mod->head.x_label, "Tail Sum/%d", NGPDPath[path].histogram.tail_sum_divide);

	if (NGPDPath[path].histogram.fall_time_mod == NULL && (enables & NGPDHistEnbFallTime))
	{
		sprintf(mod_name, "ngpd%d_hist_fall_time%d%s", NGPDPath[path].adq_num, nbins_fall_time, suffix);
		NGPDPath[path].histogram.fall_time_mod = id_mkmod3d (mod_name, nbins_fall_time, NGPDPath[path].num_chan, num_ngp, "Fall Time", "Channel", t_label, NULL, DATA_LONG, &(NGPDPath[path].histogram.fall_time_head));
		if (NGPDPath[path].histogram.fall_time_mod==NULL)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_hist_setup: Problem creating module %s", mod_name);
			return -1;
		}
	}
	if (enables & NGPDHistEnbFallTime)
		sprintf(NGPDPath[path].histogram.fall_time_mod->head.x_label, "Fall Time/%d", NGPDPath[path].histogram.fall_time_divide);

	if (NGPDPath[path].histogram.tail_ratio_mod == NULL && (enables & NGPDHistEnbTailRatio))
	{
		sprintf(mod_name, "ngpd%d_hist_tail_ratio%d%s", NGPDPath[path].adq_num, nbins_ratio, suffix);
		NGPDPath[path].histogram.tail_ratio_mod = id_mkmod3d (mod_name, nbins_ratio, NGPDPath[path].num_chan, num_ngp, "TailRatio", "Channel", t_label, NULL, DATA_LONG, &(NGPDPath[path].histogram.tail_ratio_head));
		if (NGPDPath[path].histogram.tail_ratio_mod==NULL)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_hist_setup: Problem creating module %s", mod_name);
			return -1;
		}
	}
	if (enables & NGPDHistEnbTailRatio)
		sprintf(NGPDPath[path].histogram.tail_ratio_mod->head.x_label, "TailRatio * %g", NGPDPath[path].histogram.ratio_scale);


	if (NGPDPath[path].histogram.hgt_tail_sum_mod == NULL && (enables & NGPDHistEnbHgtTailSum))
	{
		sprintf(mod_name, "ngpd%d_hist_heightXtailsum%dx%d%s", NGPDPath[path].adq_num, nbins_height, nbins_tail_sum, suffix);
		NGPDPath[path].histogram.hgt_tail_sum_mod = id_mkmod3d (mod_name, nbins_height, nbins_tail_sum, NGPDPath[path].num_chan*num_ngp, "Height", "Tail Sum", t_label3d, NULL, DATA_LONG, &(NGPDPath[path].histogram.hgt_tail_sum_head));
		if (NGPDPath[path].histogram.hgt_tail_sum_mod==NULL)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_hist_setup: Problem creating module %s", mod_name);
			return -1;
		}
	}

	if (NGPDPath[path].histogram.hgt_fall_time_mod == NULL && (enables & NGPDHistEnbHgtFallTime))
	{
		sprintf(mod_name, "ngpd%d_hist_heightXfalltime%dx%d%s", NGPDPath[path].adq_num, nbins_height, nbins_fall_time, suffix);
		NGPDPath[path].histogram.hgt_fall_time_mod = id_mkmod3d (mod_name, nbins_height, nbins_fall_time, NGPDPath[path].num_chan*num_ngp, "Height", "Fall Time", t_label3d, NULL, DATA_LONG, &(NGPDPath[path].histogram.hgt_fall_time_head));
		if (NGPDPath[path].histogram.hgt_fall_time_mod==NULL)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_hist_setup: Problem creating module %s", mod_name);
			return -1;
		}
	}
	if (NGPDPath[path].histogram.hgt_tail_ratio_mod == NULL && (enables & NGPDHistEnbHgtTailRatio))
	{
		sprintf(mod_name, "ngpd%d_hist_heightXtailratio%dx%d%s", NGPDPath[path].adq_num, nbins_height, nbins_ratio, suffix);
		NGPDPath[path].histogram.hgt_tail_ratio_mod = id_mkmod3d (mod_name, nbins_height, nbins_ratio, NGPDPath[path].num_chan*num_ngp, "Height", "Tail Ratio", t_label3d, NULL, DATA_LONG, &(NGPDPath[path].histogram.hgt_tail_ratio_head));
		if (NGPDPath[path].histogram.hgt_tail_ratio_mod==NULL)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_hist_setup: Problem creating module %s", mod_name);
			return -1;
		}
	}

	if (enables & NGPDHistEnbHgtTailSum)
	{
		sprintf(NGPDPath[path].histogram.hgt_tail_sum_mod->head.x_label, "Height/%d", NGPDPath[path].histogram.hgt_divide);
		sprintf(NGPDPath[path].histogram.hgt_tail_sum_mod->head.y_label, "Tail Sum/%d", NGPDPath[path].histogram.tail_sum_divide);
	}

	if (enables & NGPDHistEnbHgtFallTime)
	{
		sprintf(NGPDPath[path].histogram.hgt_fall_time_mod->head.x_label, "Height/%d", NGPDPath[path].histogram.hgt_divide);
		sprintf(NGPDPath[path].histogram.hgt_fall_time_mod->head.y_label, "Fall Time/%d", NGPDPath[path].histogram.fall_time_divide);
	}
	if (enables & NGPDHistEnbHgtTailRatio)
	{
		sprintf(NGPDPath[path].histogram.hgt_tail_ratio_mod->head.x_label, "Height/%d", NGPDPath[path].histogram.hgt_divide);
		sprintf(NGPDPath[path].histogram.hgt_tail_ratio_mod->head.y_label, "TailRatio * %g", NGPDPath[path].histogram.ratio_scale);
	}
	NGPDPath[path].histogram.nbins_hgt = nbins_height ;
	NGPDPath[path].histogram.nbins_tail_sum = nbins_tail_sum;
	NGPDPath[path].histogram.nbins_fall_time = nbins_fall_time;
	NGPDPath[path].histogram.nbins_ratio = nbins_ratio;

	NGPDPath[path].histogram.enables = enables;
	printf("Height divide=%d, Tail sum divide=%d, Tail ratio scaling=%g, fall_time divide=%d\n", NGPDPath[path].histogram.hgt_divide, NGPDPath[path].histogram.tail_sum_divide, NGPDPath[path].histogram.ratio_scale, NGPDPath[path].histogram.fall_time_divide); 
	return 0;
}

int ngpd_hist_clear(int path)
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_hist_clear: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].histogram.height_mod != NULL)
		id_clear_mod(NGPDPath[path].histogram.height_mod);
	if (NGPDPath[path].histogram.tail_sum_mod != NULL)
		id_clear_mod(NGPDPath[path].histogram.tail_sum_mod);
	if (NGPDPath[path].histogram.fall_time_mod != NULL)
		id_clear_mod(NGPDPath[path].histogram.fall_time_mod);
	if (NGPDPath[path].histogram.tail_ratio_mod != NULL)
		id_clear_mod(NGPDPath[path].histogram.tail_ratio_mod);

	if (NGPDPath[path].histogram.hgt_tail_sum_mod != NULL)
		id_clear_mod(NGPDPath[path].histogram.hgt_tail_sum_mod);
	if (NGPDPath[path].histogram.hgt_fall_time_mod != NULL)
		id_clear_mod(NGPDPath[path].histogram.hgt_fall_time_mod);
	if (NGPDPath[path].histogram.hgt_tail_ratio_mod != NULL)
		id_clear_mod(NGPDPath[path].histogram.hgt_tail_ratio_mod);
	return 0;
}

int ngpd_hist_process(int path, unsigned int buffers_filled)
{
	u_int32_t *src;
	unsigned int i, j;
	unsigned int samples_in_buffer;
	int rc;
	NGPDEventInfo event;
	int height, tail_sum, fall, ratio;
	int num_missing_sof=0;
	int num_malformed=0;
	int num_well_formed = 0;
	int num_pileup = 0;
	int separate_ngp, ngp=0;
	int ignore_tail_sum, ignore_fall_time;
	int chan_mult=1;
	int min_tail_count;
	int discard_pileup;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_hist_process: Invalid path=%d", path);
		return -1;
	}
#if NGPD_CONF_ADQ14
	separate_ngp = !!(NGPDPath[path].histogram.enables & NGPDHistSeparateNeutrons);
	discard_pileup = !!(NGPDPath[path].histogram.enables & NGPDHistDiscardPileup);

	ignore_tail_sum = !!(NGPDPath[path].histogram.ignores & NGPDHistIgnoreTailSum);
	ignore_fall_time = !!(NGPDPath[path].histogram.ignores & NGPDHistIgnoreFallTime);
	min_tail_count = NGPDPath[path].histogram.min_tail_count;
	if (separate_ngp)
		chan_mult = 3;

	for (i=0; i<buffers_filled; i++)
	{
		rc = ADQ_CollectDataNextPage(NGPDPath[path].adq_cu, NGPDPath[path].adq_num);
		samples_in_buffer = ADQ_GetSamplesPerPage(NGPDPath[path].adq_cu, NGPDPath[path].adq_num);
//		printf("samples_in_buffer = %d\n", samples_in_buffer);
		if (ADQ_GetStreamOverflow(NGPDPath[path].adq_cu, NGPDPath[path].adq_num))
		{
			printf("WARNING: Streaming Overflow samples_in_buffer=%u, buffers_filled=%u\n", samples_in_buffer, buffers_filled);
		}		
		if (rc)
		{
			src = (u_int32_t *)ADQ_GetPtrStream(NGPDPath[path].adq_cu, NGPDPath[path].adq_num);
			for (j=0; j<samples_in_buffer/2-3;)
			{
				if (src[j] & NGPD_EVENT_SOF)
				{
					if ((src[j+1] & (NGPD_EVENT_SOF | NGPD_EVENT_EOF)) || (src[j+2] & (NGPD_EVENT_SOF | NGPD_EVENT_EOF)) || (src[j+3] & (NGPD_EVENT_SOF | NGPD_EVENT_EOF)) != NGPD_EVENT_EOF)
					{
//						printf("Extra SOF/EOF in packet %08X, %08X, %08X, %08X\n", src[j], src[j+1], src[j+2], src[j+3]);
						num_malformed++;
						j++;
					}
					else
					{
						u_int32_t *p;
						int chan = 0;
						num_well_formed++;
						event.timestamp = NGPD_EVENT01_GET_TS(src[j], src[j+1]);
						event.tail_count = NGPD_EVENT1_GET_COUNT(src[j+1]);
						event.height = NGPD_EVENT2_GET_HGT(src[j+2]);
	 					event.tail_sum = NGPD_EVENT3_GET_TAIL(src[j+3]);
						if (src[j+3] & NGPD_EVENT3_TAIL_SIGN)
							event.tail_sum -= 0x8000000;
						event.fall_time = NGPD_EVENT2_GET_FALL(src[j+2]);
						event.height_valid = 1;
						event.tail_valid = !!(src[j+2] &  NGPD_EVENT2_TAIL_VALID);
						event.fall_valid = !!(src[j+2] &  NGPD_EVENT2_FALL_VALID);
						event.hgt_in_win = !!(src[j+2] & NGPD_EVENT2_HGT_IN_WIN);
						event.fall_in_win= !!(src[j+3] & NGPD_EVENT3_FALL_IN_WIN);
						event.tail_gt_thres = !!(src[j+3] & NGPD_EVENT3_TAIL_GT_THRES);
						event.neutron	 = !!(src[j+3] & NGPD_EVENT3_NEUTRON);
						event.sum_aborted= !!(src[j+1] & NGPD_EVENT1_SUM_ABORTED);
						event.pileup     = !!(src[j+2] & NGPD_EVENT2_PILEUP);
						if (event.pileup)
							num_pileup++;

						if (event.pileup && discard_pileup)
						{
						}
						else
						{
							if (separate_ngp)
							{
								ngp = 1;
								if (event.neutron)
									ngp = 0;
								else if (event.pileup)
									ngp = 2;
								else if (!ignore_tail_sum)
								{
									if (!event.fall_valid || event.sum_aborted || event.tail_count < min_tail_count)
										ngp = 2;
								}
								else if (!ignore_fall_time)
								{
									if (!event.fall_valid)
										ngp = 2;
								}
							}
							// printf("Event HGT=%d, fall=%d, tail=%d\n", event.height, event.fall_time, event.tail_sum);
							height = event.height/NGPDPath[path].histogram.hgt_divide;
							if (height < 0 )
								height = 0;
							else if (height >= NGPDPath[path].histogram.nbins_hgt)
								height = NGPDPath[path].histogram.nbins_hgt-1;

							tail_sum = event.tail_sum/NGPDPath[path].histogram.tail_sum_divide;
							if (tail_sum < 0 )
								tail_sum = 0;
							else if (tail_sum >= NGPDPath[path].histogram.nbins_tail_sum)
								tail_sum = NGPDPath[path].histogram.nbins_tail_sum-1;

							fall = event.fall_time/NGPDPath[path].histogram.fall_time_divide;
							if (fall < 0 )
								fall = 0;
							else if (fall >= NGPDPath[path].histogram.nbins_fall_time)
								fall = NGPDPath[path].histogram.nbins_fall_time-1;

							ratio = (double)(event.tail_sum)/event.height*NGPDPath[path].histogram.ratio_scale;
							if (ratio < 0 )
								ratio = 0;
							else if (ratio >= NGPDPath[path].histogram.nbins_ratio)
								ratio = NGPDPath[path].histogram.nbins_ratio-1;

							if (NGPDPath[path].histogram.height_mod != NULL)
							{
								p = id_get_ptr(NGPDPath[path].histogram.height_mod, height, chan, ngp);
								(*p)++;
							}
							if (NGPDPath[path].histogram.tail_sum_mod != NULL && event.tail_valid)
							{
								p = id_get_ptr(NGPDPath[path].histogram.tail_sum_mod, tail_sum, chan, ngp);
								(*p)++;
							}
							if (NGPDPath[path].histogram.fall_time_mod != NULL && event.fall_valid)
							{
								p = id_get_ptr(NGPDPath[path].histogram.fall_time_mod, fall, chan, ngp);
								(*p)++;
							}
							if (NGPDPath[path].histogram.tail_ratio_mod != NULL && event.tail_valid)
							{
								p = id_get_ptr(NGPDPath[path].histogram.tail_ratio_mod, ratio, chan, ngp);
								(*p)++;
							}
							if (NGPDPath[path].histogram.hgt_tail_sum_mod != NULL && event.tail_valid)
							{
								p = id_get_ptr(NGPDPath[path].histogram.hgt_tail_sum_mod, height, tail_sum, chan*chan_mult+ngp);
								(*p)++;
							}
							if (NGPDPath[path].histogram.hgt_fall_time_mod != NULL && event.fall_valid)
							{
								p = id_get_ptr(NGPDPath[path].histogram.hgt_fall_time_mod, height, fall, chan*chan_mult+ngp);
								(*p)++;
							}
							if (NGPDPath[path].histogram.hgt_tail_ratio_mod != NULL && event.tail_valid)
							{
								p = id_get_ptr(NGPDPath[path].histogram.hgt_tail_ratio_mod, height, ratio, chan*chan_mult+ngp);
								(*p)++;
							}
						} /* End else  not pileup */
						j += 4;
					}
				}
				else
				{
					num_missing_sof++;
//					printf("Missing SOF @ j=%u data=0x%08X, %08X, %08X, %08X\n", j, src[j], src[j+1], src[j+2], src[j+3]);
					j++;
				}
			}
		}
		else
		{
			printf("Error from ADQ_CollectDataNextPage\n");
		}
	}
	if (num_missing_sof || num_malformed)
		printf("num missing SOF=%d, num with extra SOF/EOF=%d, num well formed=%d, num_pileup=%d\n", num_missing_sof, num_malformed, num_well_formed, num_pileup);
	return 0;
#else
	printf("This version compiled without ADQ14 support. Build with NGPD_CONF_ADQ14 set\n");
	return -1;
#endif
}
