/*
 * clear_hist_test.c.c
 *
 *  Created on: 12 Sept 2012
 *      Author: wih
 * Load a test pattern into the xspress3 histogram data modules, clear various parts and then check what had been cleared
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xspress3.h"
#include "xspress3test.h"
#include "errors.h"

int clear_hist_test_all_tf(int path);
int clear_hist_test_partial(int path);

int clear_hist_test(int path)
{
	mprintf(1, "CLEAR_HIST\n");

	if (test_clear_hist & TEST_CLEAR_HIST_ALL)
		clear_hist_test_all_tf(path);
	if (test_clear_hist & TEST_CLEAR_HIST_PART)
		clear_hist_test_partial(path);

	return 0;
}
#define MAX_BUF_POINTS (1024*1024)

int clear_hist_test_all_tf(int path)
{
	int errors = 0;		
	int io_errors=0;
	u_int32_t *buf, *ptr, expected;
	int num_chan;
	int nbits_eng = 12;
	int nbins_eng, nbins_aux1, nbins_aux2, nbins_tf;
	int rc;
	int pass;
	int chan=0, n_chan=1, tf=0, n_tf=0;
	int frame_size;
	int max_tf_read;
	int e, a, z;
	int cleared_chan, cleared_n_chan;
	int i;
	mprintf(1, "Clear All\n");

	num_chan = xsp3_get_num_chan(path);

	rc = xsp3_format_run(path, -1, XSP3_FORMAT_AUX1_MODE_NONE, 0, XSP3_FORMAT_NBITS_AUX4, 0, XSP3_FORMAT_AUX2_ADC, nbits_eng);


	xsp3_get_format(path, 0, &nbins_eng, &nbins_aux1, &nbins_aux2, &nbins_tf);

		
	if ((buf=(u_int32_t *)malloc(sizeof(u_int32_t)*MAX_BUF_POINTS)) == NULL)
	{
		fprintf(stderr, "Insufficient memory\n");
		exit(1);
	}

	for (pass=0; pass<10; pass++)
	{
		switch (pass)
		{
		case 0:
			chan=0;
			n_chan = num_chan;
			tf=0;
			n_tf = nbins_tf;
			break;

		case 1:
			chan= -1;
			n_chan = -1;
			tf=0;
			n_tf = nbins_tf;
			break;

		case 2:
			chan= -1;
			n_chan = -1;
			tf=-1;
			n_tf = -1;
			break;

		case 3:
			chan=0;
			n_chan = num_chan;
			tf=-1;
			n_tf = -1;
			break;

		case 4:
			chan  = 0;
			n_chan = 1;
			tf=-1;
			n_tf = -1;
			break;

		case 5:
			chan  = 0;
			n_chan = 2;
			tf=-1;
			n_tf = -1;
			break;

		case 6:
			chan  = 1;
			n_chan = 1;
			tf=-1;
			n_tf = -1;
			break;

		case 7:
			chan  = 1;
			n_chan = 2;
			tf=-1;
			n_tf = -1;
			break;

		case 8:
			chan  = 1;
			n_chan = num_chan-1;
			tf=-1;
			n_tf = -1;
			break;

		case 9:
			chan  = 2;
			n_chan = num_chan-2;
			tf=-1;
			n_tf = -1;
			break;

		}
		if (chan < 0 || n_chan < 0)
		{
			cleared_chan = 0;
			cleared_n_chan = num_chan;
		}
		else
		{
			cleared_chan = chan;
			cleared_n_chan = n_chan;
		}
				
		mprintf(2, ".... Clearing all using xsp3_histogram_clear(path=%d, first_chan=%d, num_chan=%d, first_frame=%d, num_frames=%d)\n", path, chan, n_chan, tf, n_tf);
		if ((rc=xsp3_histogram_write_test_pat(path, Xsp3TP_Inc)) < 0)
		{
			mprintf(MP_ERR, "Error writing test pat: %s\n", xsp3_get_error_message());
			io_errors++;
		}
		if ((rc=xsp3_histogram_clear(path, chan, n_chan, tf, n_tf)) < 0)
		{
			mprintf(MP_ERR, "Error clearing memory: %s\n", xsp3_get_error_message());
			io_errors++;
		}
		frame_size = nbins_eng*nbins_aux1*nbins_aux2;
		max_tf_read = MAX_BUF_POINTS/frame_size;
		if (max_tf_read == 0)
		{
			printf("Coding error. Need larger buffer as max_tf_read=%d\n", max_tf_read);
			exit(1);
		}
		for (chan=0; chan < num_chan; chan++)
		{
			int cleared = chan >= cleared_chan && chan < cleared_chan+cleared_n_chan;
			tf = 0;
			n_tf = nbins_tf;
			i=0;
			while (n_tf > 0)
			{
				int frames_to_read = (n_tf < max_tf_read)?n_tf:max_tf_read;

				rc = xsp3_histogram_read_chan (path, buf, chan, 0, 0, tf, nbins_eng, nbins_aux1*nbins_aux2, frames_to_read);
				if (rc < 0)
				{
					mprintf(MP_ERR, "ERROR reading data: %s\n", xsp3_get_error_message());
					errors++;
				}
				ptr = buf;
				for (z=tf; z<tf+frames_to_read; z++)
				{
					for (a=0; a<nbins_aux1*nbins_aux2; a++)
					{
						for (e=0; e<nbins_eng; e++)
						{
							if (cleared)
								expected =  0;
							else
								expected =  chan + 16*i;
							i++;
							if (*ptr != expected)
							{
								errors++;
								if (errors < max_errors)
									mprintf(MP_ERR, "ERROR at chan=%d, (%d, %d, %d) Read 0x%08X, expected %08X\n", chan, e, a, z, *ptr, expected);
								else if (errors == max_errors)
									mprintf(MP_ERR, "Too many errors, counting\n");
							}
							ptr++;
						}
					}
				}
				n_tf -= frames_to_read;
				tf += frames_to_read;
			}
		}
		if (errors+io_errors > 0)
			mprintf(MP_ERREX, "Found %d data errors and %d function call errors testing xsp3_histogram_clear\n", errors, io_errors);
	}
	if (errors+io_errors > 0)
		mprintf(MP_ERREX, "Found %d data errors and %d function call errors testing xsp3_histogram_clear\n", errors, io_errors);
		 
	num_io_errors += io_errors;
	num_errors += errors;

	free(buf);
	return errors+io_errors;
}

int clear_hist_test_partial(int path)
{
	int errors = 0;		
	int io_errors=0;
	u_int32_t *buf, *ptr, expected;
	int num_chan;
	int nbits_eng = 12;
	int nbins_eng, nbins_aux1, nbins_aux2, nbins_tf;
	int rc;
	int pass;
	int chan=0, n_chan=1, tf=0, n_tf=1;
	int frame_size;
	int max_tf_read;
	int e, a, z;
	int cleared_chan, cleared_n_chan;
	int i;
	int t, n_t;
	mprintf(1, "Clear All\n");

	num_chan = xsp3_get_num_chan(path);

	rc = xsp3_format_run(path, -1, XSP3_FORMAT_AUX1_MODE_NONE, 0, XSP3_FORMAT_NBITS_AUX4, 0, XSP3_FORMAT_AUX2_ADC, nbits_eng);


	xsp3_get_format(path, 0, &nbins_eng, &nbins_aux1, &nbins_aux2, &nbins_tf);

		
	if ((buf=(u_int32_t *)malloc(sizeof(u_int32_t)*MAX_BUF_POINTS)) == NULL)
	{
		fprintf(stderr, "Insufficient memory\n");
		exit(1);
	}

	for (pass=0; pass<18; pass++)
	{
		switch (pass%6)
		{
		case 0:
			chan=0;
			n_chan = num_chan;
			break;

		case 1:
			chan= -1;
			n_chan = -1;
			break;

		case 2:
			chan  = 1;
			n_chan = 1;
			break;

		case 3:
			chan  = 1;
			n_chan = 2;
			break;

		case 4:
			chan  = 0;
			n_chan = 1;
			break;

		case 5:
			chan  = 2;
			n_chan = num_chan-2;
			break;

		}
		if (chan < 0 || n_chan < 0)
		{
			cleared_chan = 0;
			cleared_n_chan = num_chan;
		}
		else
		{
			cleared_chan = chan;
			cleared_n_chan = n_chan;
		}

		switch (pass/6)
		{
		case 0:
			tf = 3;
			n_tf = 1;
			break;

		case 1:
			tf = 5;
			n_tf = 7;
			break;

		case 2:
			tf = 11;
			n_tf = nbins_tf - 11;
			break;
		}
		mprintf(2, ".... Clearing all using xsp3_histogram_clear(path=%d, first_chan=%d, num_chan=%d, first_frame=%d, num_frames=%d)\n", path, chan, n_chan, tf, n_tf);
		if ((rc=xsp3_histogram_write_test_pat(path, Xsp3TP_Inc)) < 0)
		{
			mprintf(MP_ERR, "Error writing test pat: %s\n", xsp3_get_error_message());
			io_errors++;
		}
		if ((rc=xsp3_histogram_clear(path, chan, n_chan, tf, n_tf)) < 0)
		{
			mprintf(MP_ERR, "Error clearing memory: %s\n", xsp3_get_error_message());
			io_errors++;
		}
		frame_size = nbins_eng*nbins_aux1*nbins_aux2;
		max_tf_read = MAX_BUF_POINTS/frame_size;
		if (max_tf_read == 0)
		{
			printf("Coding error. Need larger buffer as max_tf_read=%d\n", max_tf_read);
			exit(1);
		}
		for (chan=0; chan < num_chan; chan++)
		{
			t = 0;
			n_t = nbins_tf;
			i=0;
			while (n_t > 0)
			{
				int frames_to_read = (n_t < max_tf_read)?n_t:max_tf_read;

				rc = xsp3_histogram_read_chan (path, buf, chan, 0, 0, t, nbins_eng, nbins_aux1*nbins_aux2, frames_to_read);
				if (rc < 0)
				{
					mprintf(MP_ERR, "ERROR reading data: %s\n", xsp3_get_error_message());
					errors++;
				}
				ptr = buf;
				for (z=t; z<t+frames_to_read; z++)
				{
					int cleared = chan >= cleared_chan && chan < cleared_chan+cleared_n_chan && z >= tf && z < tf+n_tf;
					for (a=0; a<nbins_aux1*nbins_aux2; a++)
					{
						for (e=0; e<nbins_eng; e++)
						{
							if (cleared)
								expected =  0;
							else
								expected =  chan + 16*i;
							i++;
							if (*ptr != expected)
							{
								errors++;
								if (errors < max_errors)
									mprintf(MP_ERR, "ERROR at chan=%d, (%d, %d, %d) Read 0x%08X, expected %08X\n", chan, e, a, z, *ptr, expected);
								else if (errors == max_errors)
									mprintf(MP_ERR, "Too many errors, counting\n");
							}
							ptr++;
						}
					}
				}
				n_t -= frames_to_read;
				t += frames_to_read;
			}
		}
		if (errors+io_errors > 0)
			mprintf(MP_ERREX, "Found %d data errors and %d function call errors testing xsp3_histogram_clear\n", errors, io_errors);
	}
	if (errors+io_errors > 0)
		mprintf(MP_ERREX, "Found %d data errors and %d function call errors testing xsp3_histogram_clear\n", errors, io_errors);
		 
	num_io_errors += io_errors;
	num_errors += errors;

	free(buf);
	return errors+io_errors;
}
