/*
 * read_hist_test.c.c
 *
 *  Created on: 10 Sept 2012
 *      Author: wih
 * Load a test pattern into the xspress3 histogram data modules and then read back using the various read interfaces and check the results
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xspress3.h"
#include "xspress3test.h"
#include "errors.h"

int read_hist_test3d(int path, int inp_nbins_eng, int inp_nbins_adc);
int read_hist_test4d(int path, int inp_nbins_eng, int inp_nbins_adc);
int read_hist_test_chan(int path);

int read_hist_test(int path)
{
	int rc;
	int errors = 0;		
	int io_errors=0;
	int aux2_cont;
	int nbits_eng, nbits_adc;
	int i, j;
	int num_chan;

	mprintf(1, "READ_HIST\n");
	num_chan = xsp3_get_num_chan(path);

	if (test_read_hist & (TEST_READ_HIST_3D | TEST_READ_HIST_4D))
	{
		for (i=0; i<3; i++)
		{
			nbits_eng = 12-2*i;
			for (j=0; j<3; j++)
			{
				if (j ==0)
					aux2_cont =  XSP3_FORMAT_NBITS_AUX0;
				else if (j == 1)
					aux2_cont =  XSP3_FORMAT_NBITS_AUX4;
				else
					aux2_cont = XSP3_FORMAT_NBITS_AUX10;

				rc = xsp3_format_run(path, -1, XSP3_FORMAT_AUX1_MODE_NONE, 0, aux2_cont, 0, XSP3_FORMAT_AUX2_ADC, nbits_eng);
				if (rc < 0)
				{
					mprintf(MP_ERR, "Error setting memory format: %s\n", xsp3_get_error_message());
					io_errors++;
				}
				nbits_adc = xsp3_nbits_adc(path, aux2_cont);
				mprintf(1, "Building test pattern for nbits_eng=%d and nbits_adc=%d\n", nbits_eng, nbits_adc);
				if ((rc=xsp3_histogram_write_test_pat(path, Xsp3TP_IncPlusPeaks)) < 0)
				{
					mprintf(MP_ERR, "Error writing test pat: %s\n", xsp3_get_error_message());
					io_errors++;
				}
				if (test_read_hist & TEST_READ_HIST_3D)
					errors += read_hist_test3d(path, 1<<nbits_eng, 1<<nbits_adc);
				if (test_read_hist & TEST_READ_HIST_4D)
					errors += read_hist_test4d(path, 1<<nbits_eng, 1<<nbits_adc);
			}
		}
	}
	if (test_read_hist & TEST_READ_HIST_CHAN)
	{
		for (i=0; i<num_chan; i++)
		{
			nbits_eng = 12-i;
			aux2_cont = i % 3;
			rc = xsp3_format_run(path, i, XSP3_FORMAT_AUX1_MODE_NONE, 0, aux2_cont, 0, XSP3_FORMAT_AUX2_ADC, nbits_eng);
			if (rc < 0)
			{
				mprintf(MP_ERR, "Error setting memory format: %s\n", xsp3_get_error_message());
				io_errors++;
			}
		}
		if ((rc=xsp3_histogram_write_test_pat(path, Xsp3TP_IncPlusPeaks)) < 0)
		{
			mprintf(MP_ERR, "Error writing test pat: %s\n", xsp3_get_error_message());
			io_errors++;
		}
		errors += read_hist_test_chan(path);
	}

	return 0;
}
#define MAX_BUF_POINTS (1024*1024)

int read_hist_test3d(int path, int inp_nbins_eng, int inp_nbins_adc)
{
	int errors = 0;		
	int io_errors=0;
	u_int32_t *buf, *ptr, expected;
	int eng, n_eng, aux=0, n_aux=0, t=0, n_t=0;
	int first_aux_chan, last_aux_chan, n_aux_chan;
	int i, j, k, l, p, q, r, s;
	int n_aux_cases;
	int nbins_eng, nbins_aux1, nbins_aux2;
	int num_chan;
	int chan=0, n_chan=1;
	int nbins_tf;
	int frame_size;
	int max_tf_read;
	int x, y, z;
	int total_aux;
	int rc;

	mprintf(1, "Testing Read hist 3D\n");
	xsp3_get_format(path, 0, &nbins_eng, &nbins_aux1, &nbins_aux2, &nbins_tf);
	num_chan = xsp3_get_num_chan(path);

	if (nbins_eng != inp_nbins_eng || nbins_aux1 != 1 || nbins_aux2 != inp_nbins_adc)
	{
		mprintf(MP_ERR, "ERROR reading format: Read Nbins eng, aux1, aux2=(%d, %d, %d) read (%d, %d, %d)\n", inp_nbins_eng, 1, inp_nbins_adc, nbins_eng, nbins_aux1, nbins_aux2);
		errors++;
	}
		
	if ((buf=(u_int32_t *)malloc(sizeof(u_int32_t)*MAX_BUF_POINTS)) == NULL)
	{
		fprintf(stderr, "Insufficient memory\n");
		exit(1);
	}
	total_aux = nbins_aux1*nbins_aux2;

	for (i=0;i<6; i++)
	{
		switch (i)
		{
		case 0: eng = 0; break;
		case 1: eng = 1; break;
		case 2: eng = nbins_eng/8-2; break;
		case 3: eng = nbins_eng-8; break;
		case 4: eng = nbins_eng/4-2; break;
		case 5: eng = nbins_eng/4-1; break;
		default: printf("Programming error  i = %d\n", i);
		exit(1);
		}
		for (j=0; j<6; j++)
		{
			switch (j)
			{
			case 0: n_eng = 1; break;
			case 1: n_eng = 2; break;
			case 2: n_eng = nbins_eng/8-1; break;
			case 3: n_eng = nbins_eng-8; break;
			case 4: n_eng = nbins_eng/4-1; break;
			case 5: n_eng = nbins_eng/4; break;
			default: printf("Programming error  j = %d\n", j);
			exit(1);
			}
			if (eng+n_eng > nbins_eng/4) /* The first /1/4 of the spectra has binary tets code, the rest has dummy spectra */
				continue;
			mprintf(3, ".. Energy from %d for %d\n", eng, n_eng);
 			for (k=0;k<4; k++)
			{
				switch (k)
				{
				case 0: chan = 0; break;
				case 1: chan = 1; break;
				case 2: chan = num_chan-2; break;
				case 3: chan = num_chan-1; break;
				}
				for (l=0; l<4; l++)
				{
					switch (l)
					{
					case 0: n_chan=1; break;
					case 1: n_chan=2; break;
					case 2: n_chan=num_chan-1; break;
					case 3: n_chan= num_chan; break;
					}
					if (chan+n_chan > num_chan)
						continue;
					if (nbins_aux1*nbins_aux2 == 1)
						n_aux_cases = 1;
					else
						n_aux_cases = 4;
					for (p=0;p<n_aux_cases; p++)
					{
						switch (p)
						{
						case 0:	aux = 0; break;
						case 1:	aux = 1; break;
						case 2:	aux = nbins_aux2/2; break;
						case 3:	aux = nbins_aux2-1; break;
						}
						for (q=0; q<n_aux_cases; q++)
						{
							switch (q)
							{
							case 0: n_aux = 1; break;
							case 1: n_aux = nbins_aux2/2; break;
							case 2: n_aux = nbins_aux2-1; break;
							case 3: n_aux = nbins_aux2; break;
							}
							/* Now 3D and 4D case vary */
							first_aux_chan = chan*nbins_aux1*nbins_aux2 + aux;
							last_aux_chan  = (chan+n_chan-1)*nbins_aux1*nbins_aux2 + aux+n_aux;
							if (last_aux_chan >= num_chan * nbins_aux1*nbins_aux2)
								continue;
							n_aux_chan = 1 + last_aux_chan- first_aux_chan;
							frame_size = n_eng * n_aux_chan;
							if (frame_size > MAX_BUF_POINTS)
								continue;
							mprintf(3, ".... Aux and chan from %d for %d\n", first_aux_chan, n_aux_chan);
							max_tf_read = MAX_BUF_POINTS/frame_size;
							if (max_tf_read > nbins_tf)
								max_tf_read = nbins_tf;
							for (r=0; r<5; r++)
							{
								switch (r)
								{
								case 0: t = 0; break;
								case 1: t = 1; break;
								case 2: t = nbins_tf/2+1; break;
								case 3: t = nbins_tf-2; break;
								case 4: t = nbins_tf-1; break;
								}
								for (s=0; s<((max_tf_read>=6)?4:2); s++)
								{
									switch (r)
									{
									case 0: n_t = 1; break;
									case 1: n_t = 2; break;
									case 2: n_t = max_tf_read/2; break;
									case 3: n_t = max_tf_read; break;
									}
									if (t + n_t > max_tf_read)
										continue;
									mprintf(3, "...... TF from %d for %d => (%d, %d, %d) for (%d, %d, %d)\n", t, n_t, eng, first_aux_chan, t, n_eng, n_aux_chan, n_t);
									rc = xsp3_histogram_read3d (path, buf,	eng, first_aux_chan, t, n_eng, n_aux_chan, n_t);
									if (rc < 0)
									{
										mprintf(MP_ERR, "ERROR reading data: %s\n", xsp3_get_error_message());
										errors++;
									}
									ptr = buf;
									for (z=t; z<t+n_t; z++)
									{
										for (y=first_aux_chan; y< first_aux_chan+n_aux_chan; y++)
										{
											for (x=eng; x<eng+n_eng; x++)
											{
												int c, a;
												if (total_aux == 1)
												{
													a = 0;
													c = y;
												}
												else
												{
													a = y % total_aux;
													c = y / total_aux;
												}
												expected =  c + 16*(x+a)+0x10000*z;
												if (*ptr != expected)
												{
													errors++;
													if (errors < max_errors)
														mprintf(MP_ERR, "ERROR at (%d, %d, %d) Read 0x%08X, expected %08X\n", x, y, z, *ptr, expected);
													else if (errors == max_errors)
														mprintf(MP_ERR, "Too many errors, counting\n");
												}
												ptr++;
											}
										}
									}
									if (errors+io_errors > 0)
										mprintf(MP_ERREX, "Found %d data errors and %d function call errors testing xsp3_histogram_read3d\n", errors, io_errors);
								}
							}
						}
					}
				}
			}
		}
	}
	if (errors+io_errors > 0)
		mprintf(MP_ERREX, "Found %d data errors and %d function call errors testing xsp3_histogram_read3d\n", errors, io_errors);
		 
	num_io_errors += io_errors;
	num_errors += errors;

	free(buf);
	return errors+io_errors;
}


int read_hist_test4d(int path, int inp_nbins_eng, int inp_nbins_adc)
{
	int errors = 0;		
	int io_errors=0;
	u_int32_t *buf, *ptr, expected;
	unsigned eng, n_eng, aux=0, n_aux=0, t=0, n_t=0;
	int i, j, k, l, p, q, r, s;
	int n_aux_cases;
	int nbins_eng, nbins_aux1, nbins_aux2;
	int num_chan;
	unsigned chan=0, n_chan=1;
	int nbins_tf;
	int frame_size;
	int max_tf_read;
	int e, a, c, z;
	int total_aux;
	int rc;

	mprintf(1, "Testing Read hist 4D\n");
	xsp3_get_format(path, 0, &nbins_eng, &nbins_aux1, &nbins_aux2, &nbins_tf);
	num_chan = xsp3_get_num_chan(path);

	if (nbins_eng != inp_nbins_eng || nbins_aux1 != 1 || nbins_aux2 != inp_nbins_adc)
	{
		mprintf(MP_ERR, "ERROR reading format: Read Nbins eng, aux1, aux2=(%d, %d, %d) read (%d, %d, %d)\n", inp_nbins_eng, 1, inp_nbins_adc, nbins_eng, nbins_aux1, nbins_aux2);
		errors++;
	}
		
	if ((buf=(u_int32_t *)malloc(sizeof(u_int32_t)*MAX_BUF_POINTS)) == NULL)
	{
		fprintf(stderr, "Insufficient memory\n");
		exit(1);
	}
	total_aux = nbins_aux1*nbins_aux2;

	for (i=0;i<6; i++)
	{
		switch (i)
		{
		case 0: eng = 0; break;
		case 1: eng = 1; break;
		case 2: eng = nbins_eng/8-2; break;
		case 3: eng = nbins_eng-8; break;
		case 4: eng = nbins_eng/4-2; break;
		case 5: eng = nbins_eng/4-1; break;
		default: printf("Programming error  i = %d\n", i);
		exit(1);
		}
		for (j=0; j<6; j++)
		{
			switch (j)
			{
			case 0: n_eng = 1; break;
			case 1: n_eng = 2; break;
			case 2: n_eng = nbins_eng/8-1; break;
			case 3: n_eng = nbins_eng-8; break;
			case 4: n_eng = nbins_eng/4-1; break;
			case 5: n_eng = nbins_eng/4; break;
			default: printf("Programming error  j = %d\n", j);
			exit(1);
			}
			if (eng+n_eng > nbins_eng/4) /* The first /1/4 of the spectra has binary tets code, the rest has dummy spectra */
				continue;
			mprintf(3, ".. Energy from %d for %d\n", eng, n_eng);

			if (nbins_aux1*nbins_aux2 == 1)
				n_aux_cases = 1;
			else
				n_aux_cases = 4;

			for (k=0;k<n_aux_cases; k++)
			{
				switch (k)
				{
				case 0:	aux = 0; break;
				case 1:	aux = 1; break;
				case 2:	aux = nbins_aux2/2; break;
				case 3:	aux = nbins_aux2-1; break;
				}
				for (l=0; l<n_aux_cases; l++)
				{
					switch (l)
					{
					case 0: n_aux = 1; break;
					case 1: n_aux = nbins_aux2/2; break;
					case 2: n_aux = nbins_aux2-1; break;
					case 3: n_aux = nbins_aux2; break;
					}
					if (aux+n_aux > total_aux)
						continue;
					mprintf(4, ".... Aux from %d for %d\n", aux, n_aux);
		 			for (p=0;p<4; p++)
					{
						switch (p)
						{
						case 0: chan = 0; break;
						case 1: chan = 1; break;
						case 2: chan = num_chan-2; break;
						case 3: chan = num_chan-1; break;
						}
						for (q=0; q<4; q++)
						{
							switch (q)
							{
							case 0: n_chan=1; break;
							case 1: n_chan=2; break;
							case 2: n_chan=num_chan-1; break;
							case 3: n_chan= num_chan; break;
							}
							if (chan+n_chan > num_chan)
								continue;
							frame_size = n_eng*n_aux*n_chan;
							if (frame_size > MAX_BUF_POINTS)
								continue;
							mprintf(5, ".... Chan from %d for %d\n", chan, n_chan);
							max_tf_read = MAX_BUF_POINTS/frame_size;
							if (max_tf_read > nbins_tf)
								max_tf_read = nbins_tf;
							for (r=0; r<5; r++)
							{
								switch (r)
								{
								case 0: t = 0; break;
								case 1: t = 1; break;
								case 2: t = nbins_tf/2+1; break;
								case 3: t = nbins_tf-2; break;
								case 4: t = nbins_tf-1; break;
								}
								for (s=0; s<((max_tf_read>=6)?4:2); s++)
								{
									switch (r)
									{
									case 0: n_t = 1; break;
									case 1: n_t = 2; break;
									case 2: n_t = max_tf_read/2; break;
									case 3: n_t = max_tf_read; break;
									}
									if (t + n_t > max_tf_read)
										continue;
									mprintf(6, "...... TF from %d for %d => (%d, %d, %d, %d) for (%d, %d, %d, %d)\n", t, n_t, eng, aux, chan, t, n_eng, n_aux, n_chan, n_t);
									rc = xsp3_histogram_read4d(path, buf, eng, aux, chan, t, n_eng, n_aux, n_chan, (double) n_t);
									if (rc < 0)
									{
										mprintf(MP_ERR, "ERROR reading data: %s\n", xsp3_get_error_message());
										errors++;
									}
									ptr = buf;
									for (z=t; z<t+n_t; z++)
									{
										for (c=chan; c<chan+n_chan; c++)
										{
											for (a=aux; a<aux+n_aux; a++)
											{
												for (e=eng; e<eng+n_eng; e++)
												{
													expected =  c + 16*(e+a)+0x10000*z;
													if (*ptr != expected)
													{
														errors++;
														if (errors < max_errors)
															mprintf(MP_ERR, "ERROR at (%d, %d, %d, %d) Read 0x%08X, expected %08X\n", e, a, c, z, *ptr, expected);
														else if (errors == max_errors)
															mprintf(MP_ERR, "Too many errors, counting\n");
													}
													ptr++;
												}
											}
										}
									}
									if (errors+io_errors > 0)
										mprintf(MP_ERREX, "Found %d data errors and %d function call errors testing xsp3_histogram_read4d\n", errors, io_errors);
								}
							}
						}
					}
				}
			}
		}
	}
	if (errors+io_errors > 0)
		mprintf(MP_ERREX, "Found %d data errors and %d function call errors testing xsp3_histogram_read4d\n", errors, io_errors);
		 
	num_io_errors += io_errors;
	num_errors += errors;

	free(buf);
	return errors+io_errors;
}

int read_hist_test_chan(int path)
{
	int errors = 0;		
	int io_errors=0;
	u_int32_t *buf, *ptr, expected;
	int eng, n_eng, aux=0, n_aux=0, t=0, n_t=0;
	int i, j, k, l, r, s;
	int n_aux_cases;
	int nbins_eng, nbins_aux1, nbins_aux2;
	int num_chan;
	int chan;
	int nbins_tf;
	int frame_size;
	int max_tf_read;
	int e, a, z;
	int total_aux;
	int rc;

	mprintf(1, "Testing Read hist by channel\n");
	num_chan = xsp3_get_num_chan(path);
	
	if ((buf=(u_int32_t *)malloc(sizeof(u_int32_t)*MAX_BUF_POINTS)) == NULL)
	{
		fprintf(stderr, "Insufficient memory\n");
		exit(1);
	}

	for (chan=0; chan<num_chan; chan++)
	{
		xsp3_get_format(path, chan, &nbins_eng, &nbins_aux1, &nbins_aux2, &nbins_tf);
		mprintf(3, ".. Chan %d nbins_eng=%d, nbins_aux1=%d, nbins_aux2=%d, nbins_tf=%d\n", chan, nbins_eng, nbins_aux1, nbins_aux2, nbins_tf);
		total_aux = nbins_aux1*nbins_aux2;

		for (i=0;i<6; i++)
		{
			switch (i)
			{
			case 0: eng = 0; break;
			case 1: eng = 1; break;
			case 2: eng = nbins_eng/8-2; break;
			case 3: eng = nbins_eng-8; break;
			case 4: eng = nbins_eng/4-2; break;
			case 5: eng = nbins_eng/4-1; break;
			default: printf("Programming error  i = %d\n", i);
			exit(1);
			}
			for (j=0; j<6; j++)
			{
				switch (j)
				{
				case 0: n_eng = 1; break;
				case 1: n_eng = 2; break;
				case 2: n_eng = nbins_eng/8-1; break;
				case 3: n_eng = nbins_eng-8; break;
				case 4: n_eng = nbins_eng/4-1; break;
				case 5: n_eng = nbins_eng/4; break;
				default: printf("Programming error  j = %d\n", j);
				exit(1);
				}
				if (eng+n_eng > nbins_eng/4) /* The first /1/4 of the spectra has binary tets code, the rest has dummy spectra */
					continue;
				mprintf(4, ".... Energy from %d for %d\n", eng, n_eng);

				if (nbins_aux1*nbins_aux2 == 1)
					n_aux_cases = 1;
				else
					n_aux_cases = 4;

				for (k=0;k<n_aux_cases; k++)
				{
					switch (k)
					{
					case 0:	aux = 0; break;
					case 1:	aux = 1; break;
					case 2:	aux = nbins_aux2/2; break;
					case 3:	aux = nbins_aux2-1; break;
					}
					for (l=0; l<n_aux_cases; l++)
					{
						switch (l)
						{
						case 0: n_aux = 1; break;
						case 1: n_aux = nbins_aux2/2; break;
						case 2: n_aux = nbins_aux2-1; break;
						case 3: n_aux = nbins_aux2; break;
						}
						if (aux+n_aux > total_aux)
							continue;
						mprintf(5, "...... Aux from %d for %d\n", aux, n_aux);
						frame_size = n_eng*n_aux;
						if (frame_size > MAX_BUF_POINTS)
							continue;
						max_tf_read = MAX_BUF_POINTS/frame_size;
						if (max_tf_read > nbins_tf)
							max_tf_read = nbins_tf;
						for (r=0; r<5; r++)
						{
							switch (r)
							{
							case 0: t = 0; break;
							case 1: t = 1; break;
							case 2: t = nbins_tf/2+1; break;
							case 3: t = nbins_tf-2; break;
							case 4: t = nbins_tf-1; break;
							}
							for (s=0; s<((max_tf_read>=6)?4:2); s++)
							{
								switch (r)
								{
								case 0: n_t = 1; break;
								case 1: n_t = 2; break;
								case 2: n_t = max_tf_read/2; break;
								case 3: n_t = max_tf_read; break;
								}
								if (t + n_t > max_tf_read)
									continue;
								mprintf(6, "........ TF from %d for %d => Chan =%d, from (%d, %d, %d) for (%d, %d, %d)\n", t, n_t, chan, eng, aux, t, n_eng, n_aux, n_t);
								rc = xsp3_histogram_read_chan (path, buf, chan, eng, aux, t, n_eng, n_aux, n_t);
								if (rc < 0)
								{
									mprintf(MP_ERR, "ERROR reading data: %s\n", xsp3_get_error_message());
									errors++;
								}
								ptr = buf;
								for (z=t; z<t+n_t; z++)
								{
									for (a=aux; a<aux+n_aux; a++)
									{
										for (e=eng; e<eng+n_eng; e++)
										{
											expected =  chan + 16*(e+a)+0x10000*z;
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
								if (errors+io_errors > 0)
									mprintf(MP_ERREX, "Found %d data errors and %d function call errors testing xsp3_histogram_read4d\n", errors, io_errors);
							}
						}
					}
				}
			}
		}
	}
	if (errors+io_errors > 0)
		mprintf(MP_ERREX, "Found %d data errors and %d function call errors testing xsp3_histogram_read4d\n", errors, io_errors);
		 
	num_io_errors += io_errors;
	num_errors += errors;

	free(buf);
	return errors+io_errors;
}
