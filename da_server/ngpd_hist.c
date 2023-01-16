/*----------------------------------------------------------------------
 *	Neutron Gamma Pulse Discriminator Device for DA with multi-client server.
 *	By William Helsby
 *----------------------------------------------------------------------
 */

#include "header.h"
#include <ngpd.h>


NGPDUnifType ngpd_unif_handle[MAX_OPEN_DEVICES];

int ngpd_hist_setup_cmd(int quals, int cn, int path)
{
	int max_height=65535;
	int max_tail_sum=65536*10;
	int max_fall_time=255;
	int nbins_height=1024;
	int nbins_tail_sum=1024;
	int nbins_fall_time=256;
	int nbins_tail_ratio=1024;
	double max_tail_ratio=0.5;
	double ratio_scale=1.0;
	NGPDGeneration generation;
	NGPDHistEnables enables=NGPDHistEnbAll;
	int rc;
	NGPDMeasure measure;

	generation = ngpd_get_generation(path);
	if (generation == NGPDGenError)
	{
		sv_printf(cn, " ! ERROR: Cannot read generation : %s\n", ngpd_get_error_message());
		return -1;
	}
	if (generation == NGPDGenZynqMP1)
	{
		sv_printf(cn, "! ERROR: Please use ngpd set-hist-ngzmp to use the firmware histogrammer on ZynqMP version\n");
		return -1;
	}

	if (quals & 1)
		nbins_height = Qualifier[0].IntArg;
	if (quals & 2)
		nbins_tail_sum = Qualifier[1].IntArg;
	if (quals & 4)
		nbins_fall_time = Qualifier[2].IntArg;
	if (quals & 8)
		nbins_tail_ratio = Qualifier[3].IntArg;
	if (quals & 0x10)
		max_tail_ratio = Qualifier[4].DoubleArg;
	if (quals & 0x20)
		enables |= NGPDHistSeparateNeutrons;
	if (quals & 0x40)
		enables |= NGPDHistDiscardPileup;

	if (ngpd_read_measure(path, 0, &measure) < 0)
	{
		sv_printf(cn,  "! ERROR: Reading pulse measurement %s\n", ngpd_get_error_message());
	}

	max_tail_sum = 65536/5*measure.tail_sum_num;
	ratio_scale = (double)nbins_tail_ratio/(max_tail_ratio*measure.tail_sum_num);
	sv_printf(cn, "# Note: max_tail_sum=%d, ratio_scale=%g\n", max_tail_sum, ratio_scale);
	rc = ngpd_hist_setup(path, max_height, max_tail_sum, max_fall_time, ratio_scale, nbins_height, nbins_tail_sum, nbins_fall_time, nbins_tail_ratio, enables);
	if (rc < 0)
	{
		sv_printf(cn,  "! ERROR: Setting histogramming %s\n", ngpd_get_error_message());
	}

	set_int_command(0, cn, "ngpd_nbins_height", nbins_height);
	set_int_command(0, cn, "ngpd_nbins_tail_sum", nbins_tail_sum);
	set_int_command(0, cn, "ngpd_nbins_fall_time", nbins_fall_time);
	set_int_command(0, cn, "ngpd_nbins_tail_ratio", nbins_tail_ratio);

	set_int_command(0, cn, "ngpd_separate_ngp", !!(enables & NGPDHistSeparateNeutrons));

	return rc;
}

int ngpd_hist_setup_ngzmp(int quals, int cn, int path)
{

	NGZMPHistConf hist_setup;
	int chan = -1;
	int rc;
	int num_chan;
	int nt;

	memset(&hist_setup, 0, sizeof(hist_setup));
	hist_setup.separate_ngp = 0;
	hist_setup.enb_hgt_sum = 1;
	hist_setup.enb_hgt_fall = 0;
	hist_setup.discard_pu = 0;
	hist_setup.nbits_height = 10;
	hist_setup.nbits_fall_time = 8;
	hist_setup.nbits_tail_sum = 10;
	
	num_chan = ngpd_get_num_chan(path);
	if (num_chan < 0 )
	{
		sv_printf(cn, "! ERROR: setup-hist cannot read number of channels : %s\n", ngpd_get_error_message());
		return num_chan;
	}

	if (quals & 1)
		hist_setup.nbits_height = Qualifier[0].IntArg;
	if (quals & 2)
		hist_setup.nbits_tail_sum = Qualifier[1].IntArg;
	if (quals & 4)
		hist_setup.nbits_fall_time = Qualifier[2].IntArg;

	hist_setup.shift_height = 16-hist_setup.nbits_height;
	hist_setup.shift_fall_time = 8-hist_setup.nbits_fall_time;
	hist_setup.shift_tail_sum = 19-hist_setup.nbits_tail_sum;

	if (quals & 8)
		hist_setup.shift_height = Qualifier[3].IntArg;
	if (quals & 0x10)
		hist_setup.shift_fall_time  = Qualifier[4].IntArg;
	if (quals & 0x20)
		hist_setup.shift_tail_sum = Qualifier[5].IntArg;

	if (quals & 0xC0)
	{
		hist_setup.enb_hgt_sum = !!(quals & 0x40);
		hist_setup.enb_hgt_fall = !! (quals & 0x80);
	}
	hist_setup.separate_ngp = !! (quals & 0x100);
	if (quals & 0x200)
	{
		if (hist_setup.separate_ngp)
			hist_setup.discard_pu = 1;
		else
		{
			sv_printf(cn, "! ERROR: Use discard pile up without separate-ngp\n");
			return -1;
		}
	}
	if (quals & 0x400)
		chan = Qualifier[10].IntArg;

	rc = ngzmp_hist_write_chan_config(path, chan, &hist_setup);
	if (rc < 0)
	{
		sv_printf(cn,  "! ERROR: Setting histogramming %s\n", ngpd_get_error_message());
	}

	set_int_command(0, cn, "ngpd_nbins_height", 1 << hist_setup.nbits_height);
	set_int_command(0, cn, "ngpd_nbins_tail_sum", 1 << hist_setup.nbits_tail_sum );
	set_int_command(0, cn, "ngpd_nbins_fall_time", 1 << hist_setup.nbits_fall_time);
	set_int_command(0, cn, "ngpd_nbins_tail_ratio", 0);

	set_int_command(0, cn, "ngpd_separate_ngp", hist_setup.separate_ngp);

	nt = num_chan;
	if (hist_setup.separate_ngp)
		nt *=3;
	if (hist_setup.enb_hgt_fall)
		ngpd_unif_resize(cn, path, NGPDHistType_HgtFallTime, 1<<hist_setup.nbits_height, 1<<hist_setup.nbits_fall_time, nt, hist_setup.separate_ngp);
	else
		ngpd_unif_resize(cn, path, NGPDHistType_HgtFallTime, 1, 1, 0, hist_setup.separate_ngp);

	if (hist_setup.enb_hgt_sum)
		ngpd_unif_resize(cn, path, NGPDHistType_HgtTailSum, 1<<hist_setup.nbits_height, 1<<hist_setup.nbits_tail_sum, nt, hist_setup.separate_ngp);
	else
		ngpd_unif_resize(cn, path, NGPDHistType_HgtTailSum, 1, 1, 0, hist_setup.separate_ngp);

	return rc;
}

void ngpd_unif_close(int path, int handle)
{
	ngpd_unif_handle[handle].valid = 0;
}
#define PATH_TYPE_PATH	 	0xFFFF
#define PATH_TYPE_FALL_TIME 0x10000
static int ngpd_unif_clear_hist(int path,  unsigned x, unsigned y, unsigned t,
							 unsigned dx, unsigned dy, unsigned dt)
{
	int rc;
	path &= PATH_TYPE_PATH;
	rc = ngzmp_hist_clear_all_start(path, -1, 0, 0);
	if (rc < 0)
	{
		fprintf(stderr, "ERROR starting to clear memory : %s\n", ngpd_get_error_message());
		return -1;
	}
	rc = ngzmp_hist_clear_wait(path, -1);
	if (rc < 0)
	{
		fprintf(stderr, "ERROR waiting for clear memory : %s\n", ngpd_get_error_message());
		return -1;
	}
	return 0;
}
static int ngpd_unif_read_hist (int path, void *to, unsigned x, unsigned y, unsigned t, unsigned dx, unsigned dy, unsigned dt)
{
	int rc;
	int chan, ngp;
	NGZMPHistConf hist_setup;
	int i;
	u_int32_t offset;
	int fall_time = path & PATH_TYPE_FALL_TIME;
	int frame_size_tail_sum, frame_size_fall_time;
	int fall_time_marker;
	u_int32_t * data = (u_int32_t *)to;

	path &= PATH_TYPE_PATH;
	rc = ngzmp_hist_read_chan_config(path, 0, &hist_setup);
	if (rc < 0)
	{
		fprintf(stderr,  "! ERROR: Cannot retrieve histogram setup %s\n", ngpd_get_error_message());
		return -1;
	}
	if (hist_setup.enb_hgt_fall)
		frame_size_fall_time = 1 <<(hist_setup.nbits_height+hist_setup.nbits_fall_time);
	else 
		frame_size_fall_time = 0;

	if (hist_setup.enb_hgt_sum)
		frame_size_tail_sum =  1 <<(hist_setup.nbits_height+hist_setup.nbits_tail_sum);
	else
		frame_size_tail_sum = 0;
	if (hist_setup.enb_hgt_fall && hist_setup.enb_hgt_sum)
	{
		fall_time_marker = (frame_size_fall_time>frame_size_tail_sum)?frame_size_fall_time:frame_size_tail_sum;
		fall_time_marker *= 4;
	}
	else
		fall_time_marker = 0;
	for (i=0; i<dt; i++)
	{
		if (hist_setup.separate_ngp)
		{
			chan = (t+i)/3;
			ngp = (t+i)%3;
		}
		else
		{
			chan = t+i;
			ngp = 0;
		}
		offset = x + y << hist_setup.nbits_height;
		if (fall_time)
			offset += ngp*frame_size_fall_time+fall_time_marker;
		else
			offset += ngp*frame_size_tail_sum;
		
		rc = ngzmp_hist_read_chan(path,  chan, offset, dx*dy, data);
		if (rc < 0)
		{
			printf("ngpd_unif_read_hist: Error on read: %s\n", ngpd_get_error_message());
			return rc;
		}
		data += dx*dy;
	}
	return 0;
}

static int ngpd_unif_enable(int path)
{
	u_int32_t hist_gpio_cont=NGZMP_HIST_GPIO_ENABLE;
	int rc;

	path &= PATH_TYPE_PATH;
	rc = ngzmp_hist_write_reg(path, -1, NGZMP_HIST_GPIO_OFFSET, 1, &hist_gpio_cont);
	if (rc < 0)
		fprintf(stderr, "ERROR enabling histogramming: %s\n", ngpd_get_error_message());
	return 0;
}	

static int ngpd_unif_disable(int path)
{
	u_int32_t hist_gpio_cont=0;
	int rc;

	path &= PATH_TYPE_PATH;
	rc = ngzmp_hist_write_reg(path, -1, NGZMP_HIST_GPIO_OFFSET, 1, &hist_gpio_cont);
	if (rc < 0)
		fprintf(stderr, "ERROR disabling histogramming: %s\n", ngpd_get_error_message());
	return 0;
}	
	 
int ngpd_open_histogram_cmd(int quals, int cn, int path)
{
	NGPDHistType type = NGPDHistType_HgtTailSum;
	int chan = 0;
	int rc;
	NGZMPHistConf hist_setup;
	int nx, ny, nt;
	int num_chan;
	char * title;
	int handle;
	int path_and_type = path;
	switch (quals & 3)
	{
	case 1:
		type = NGPDHistType_HgtFallTime;
		break;
	case 2:
		type = NGPDHistType_HgtTailSum;
		break;
	default:
		sv_printf(cn, "! ERROR: Please specify one and only one of tail-sum or fall-time\n");
		return -1;
	}
	if ((num_chan = ngpd_get_num_chan(path)) < 0)
	{
		sv_printf(cn, "! ERROR: Cannot get number of channels : %s\n", ngpd_get_error_message());
		return num_chan;
	}
	rc = ngzmp_hist_read_chan_config(path, chan, &hist_setup);
	if (rc < 0)
	{
		sv_printf(cn,  "! ERROR: Cannot retrieve histogram setup %s\n", ngpd_get_error_message());
		return -1;
	}
	nx = 1<<hist_setup.nbits_height;

	if (type == NGPDHistType_HgtFallTime)
	{
		if (hist_setup.enb_hgt_fall == 0)
		{
			sv_printf(cn, "! ERROR: Please enable histogram height x FallTime using ngpd setup-hist-ngzmp first\n");
			return -1;
		}
		ny = 1 << hist_setup.nbits_fall_time;
		if (hist_setup.separate_ngp)
			title = "NGPD ZynqMP Height x FallTime, separating neutron/gamma/Pileup";
		else
			title = "NGPD ZynqMP Height x FallTime, combining neutron/gamma/Pileup";
		path_and_type |= PATH_TYPE_FALL_TIME;
	}
	else if (type == NGPDHistType_HgtTailSum)
	{
		if (hist_setup.enb_hgt_sum == 0)
		{
			sv_printf(cn, "! ERROR: Please enable histogram height x TailSum using ngpd setup-hist-ngzmp first\n");
			return -1;
		}
		ny = 1 << hist_setup.nbits_tail_sum;
		if (hist_setup.separate_ngp)
			title = "NGPD ZynqMP Height x TailSum, separating neutron/gamma/Pileup";
		else
			title = "NGPD ZynqMP Height x TailSum, combining neutron/gamma/Pileup";
	}
	else
		return -1;
	nt = num_chan;
	if (hist_setup.separate_ngp)
		nt *= 3;

	handle = unified_open (cn, path_and_type, nx, ny, nt, UNIF_DATA_LONG, title,
				ngpd_unif_read_hist, ngpd_unif_close, NULL, NULL, NULL,
				ngpd_unif_clear_hist,
				ngpd_unif_enable,
				ngpd_unif_disable);
	if (handle < 0)
	{
		sv_printf (cn, "! Unified open command failed.\n");
		return -1;
	}
	ngpd_unif_handle[handle].valid = 1;
	ngpd_unif_handle[handle].ngpd_path = path;
	if (type == NGPDHistType_HgtTailSum)
		ngpd_unif_handle[handle].type = NGPDHistType_HgtTailSum;
	else
		ngpd_unif_handle[handle].type = NGPDHistType_HgtFallTime;
	ngpd_unif_handle[handle].separate_ngp = hist_setup.separate_ngp;

	return handle;
}

int ngpd_unif_resize(int cn, int path, NGPDHistType type, int nx, int ny, int nt, int separate_ngp)
{
	int handle;
	char *title= "Unknown";

	if (type == NGPDHistType_HgtFallTime)
	{
		if (separate_ngp)
			title = "NGPD ZynqMP Height x FallTime, separating neutron/gamma/Pileup";
		else
			title = "NGPD ZynqMP Height x FallTime, combining neutron/gamma/Pileup";
	}
	else if (type == NGPDHistType_HgtTailSum)
	{
		if (separate_ngp)
			title = "NGPD ZynqMP Height x TailSum, separating neutron/gamma/Pileup";
		else
			title = "NGPD ZynqMP Height x TailSum, combining neutron/gamma/Pileup";
	}
	else if (type == NGPDHistType_Scope)
		title = "NGPD System - Scope Mode";
	
	sv_printf(cn, "# NGPD resize type=%d, (%d, %d, %d) title=%s\n", type, nx, ny, nt, title);

	for (handle=0; handle<MAX_OPEN_DEVICES; handle++)
	{
		if (ngpd_unif_handle[handle].valid && ngpd_unif_handle[handle].ngpd_path == path && ngpd_unif_handle[handle].type == type)
		{
			if (nt == 0)
			{
				unified_set_nx(cn, handle, 1);
				unified_set_ny(cn, handle, 1);
				unified_set_nt(cn, handle, 0);
			}
			else
			{
				if (nx >= 0) unified_set_nx(cn, handle, nx);
				if (ny >= 0) unified_set_ny(cn, handle, ny);
				if (nt >= 0) unified_set_nt(cn, handle, nt);
			}
			unif_set_description(handle, title);
		}
	}
	return 0;
}
