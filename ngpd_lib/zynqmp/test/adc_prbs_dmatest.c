#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

#include "errors.h"
#include "ngpd.h"
#include "ngzmptest.h"

int prbs_pn9_dma_test(int path, int card, int num_pass);
int ngpd_list_jesd_regs(int path, int card);


int prbs_dma_tests(int path)
{
	int card =0;
	int rc;
	u_int8_t x;
	NGPDClockSetup cset;
	NGPDScopeOptions scope_options;
	
	cset.src = NGZMPClkSrcLMK61E2;
	cset.sysref = NGZMPSysRefContinuous;
	cset.monitor_op = NGZMPClockMonClk250;
	
	scope_options.nstreams=8;
	
	mprintf(1, "PRBS-DMA-TESTS\n");

	if ((rc=ngpd_clock_setup(path, -1, &cset)) <0 )
	{
		mprintf(MP_ERR, "ERROR setting clock : %s\n", ngpd_get_error_message());
		num_io_errors++;
		return -1;
	}
	if ((rc=ngpd_clock_check_locked(path, -1)) < 0)
	{
		mprintf(MP_ERR, "ERROR checking clock locked : %s\n", ngpd_get_error_message());
		num_io_errors++;
		return -1;
	}
	if ((rc=ngzmp_adc_setup_adc(path, card, NGZMPADC_Default)) < 0)
	{
		mprintf(MP_ERR, "ERROR setting up ADC : %s\n", ngpd_get_error_message());
		num_io_errors++;
		return -1;
	}
	ngzmp_adc_read_status(path, card);
	if ((rc=ngpd_set_scope_options(path, card, &scope_options)) < 0)
	{
		mprintf(MP_ERR, "ERROR setting scope options : %s\n", ngpd_get_error_message());
		num_io_errors++;
		return -1;
	}
	x = 1;  // Restore default 2's complement mode
	ngzmp_spi_write_adc(path, card, -1, 3, 3,  AD9694_SPI_PAIR_SAMPLE_MODE, 1,  &x);
	ngpd_list_jesd_regs(path, card);

	if (test_prbs & TEST_PRBS_PN9)
		prbs_pn9_dma_test(path, card, 1);
	return 0;
}

#define BUFF_WORDS (1024*1024)
#if 0
prbs_dma_test(int test_type)
{
	u_int32_t *buff, *ptr;
	int i, j, k;
	int errors = 0;
	int burst_len_words=2048/4;
	int num_bursts;
	int burst;
	int pass, num_pass;
	u_int32_t status;
	struct timespec sl_time;
	int adc;
	char test_io = AD9252_TEST_IO_PN23;		
	u_int32_t expected[8], actual;
	int block;
	int next_burst_to_read;
	int errors_by_chan[XH_NUM_CHAN];
	int buff_num_times;
	int bursts_to_read;
	int mismatch;
	int num_missing[8];
	int errors_at_same_time[8][8];
	u_int32_t error_bits[XH_NUM_CHAN];

	sl_time.tv_sec  = 0;
	sl_time.tv_nsec = 1000000;

	switch (test_type)
	{
	case 0:
		mprintf(1, "Print data PSRB DMA Tests \n");
		num_bursts = 4;
		num_pass = 1;
		break;

	case 1:
		num_bursts = BUFF_SIZE/burst_len_words;
		mprintf(1, "Testing Medium sized blocks = %d bursts\n", num_bursts);
		num_pass = 1;
		break;

	case 2:
		num_bursts = mem_size_words/burst_len_words;
		mprintf(1, "Testing DMA all mem = %d bursts\n", num_bursts);
		num_pass = 1;
		break;
	}
	if ((buff = (u_int32_t *)malloc(BUFF_SIZE*sizeof(u_int32_t))) == NULL)
	{
		mprintf(MP_ERREX, "Insufficient memory\n");
		exit(1);
	}
	for (i=0; i<XH_NUM_CHAN; i++)
	{
		error_bits[i] =0;
		errors_by_chan[i] = 0;
	}
	for (i=0;i<8; i++)
		num_missing[i] = 0;
	for (i=0;i<8; i++)
	{
		for (j=0; j<8; j++)
			errors_at_same_time[i][j] = 0;
	}

	for (pass = 0; pass < num_pass; pass++)
	{
		mprintf(1, "Testing %d bursts, pass %d\n", num_bursts, pass);
		for (adc=first_adc; adc <= last_adc; adc++)
		{
			mprintf(2, ".... Testing ADC %d\n", adc);
			test_io = AD9252_TEST_IO_PN9;
			aida_spi_adc_write_adc_func(adc, AD9252_ADDR_TEST_IO, 0xFF, 1, 1, &test_io);

			xh_wr_dma_adc_chip_sel(path, adc<<1 | ((adc<<1)+1) << 4 );			
			xh_dma_start_scope(path, 0, num_bursts, NULL);

		/* try to Prove need for cache invalidate */


			printf("Waiting for DMA\n");
			i = 0;
			while((status=xh_rd_debug_status(path)) == 0 && i++ < 100)
				nanosleep(&sl_time, NULL);
			if (status != (XH_DMA_STAT_DONE |  XH_DMA_STAT_IDLE))
			{
				errors++;
				mprintf(MP_ERR, "ERROR in status at end of busrt read 0x%04X, expected 0x%04X\n", status, (XH_DMA_STAT_DONE |  XH_DMA_STAT_IDLE));
			}
			xh_wr_dma_control(path, 0);
			
			next_burst_to_read= 0;
			for (burst=0; burst<num_bursts; burst+=bursts_to_read)
			{
				bursts_to_read = num_bursts-burst;
				/* Note that data size double when unpacking D16 data into D32 boundaries. */
				/* read out into array Time 0..n-1, Chan A..H
					A0	A1	A2 ..... Abuff_num_times-1
					B0	B1	B2 ..... Bbuff_num_times-1
					C0	C1	C2 ..... Cbuff_num_times-1
					...................................
					H0	H1	H2	.... HAbuff_num_times-1
				*/

				if (bursts_to_read*burst_len_words > BUFF_SIZE/2)
					bursts_to_read = (BUFF_SIZE/burst_len_words)/2;
				mprintf(3, ".. Reading burst %d, num=%d\n", burst, bursts_to_read);
				xh_read_scope(path, burst*burst_len_words/4, bursts_to_read*burst_len_words/4, 0, 8, buff);

				buff_num_times = bursts_to_read*burst_len_words/4;

				ptr = buff;
				for (i=0; i<buff_num_times*8; i++)
				{
					*ptr &= 0x3FFF;
					ptr ++;
				}
				ptr = buff;
				for (i=0; i<buff_num_times; i++)
				{
					if (burst==0 && i == 0)
					{	
						for (j=0; j<8; j++)
						{
							if (ptr[j] == 0)
							{
								if (errors < max_errors)
								{
									mprintf(MP_ERR, "ERROR at start of transfer, channels %d reads %d. PRBS cannot give 0\n", ptr[j]);
								}
								else if (errors == max_errors)
									mprintf(MP_ERR, "Too many errors. Counting\n");
								errors++;
							}
						}
						mismatch = 0;
						for (j=1; j<8; j++)
							if (ptr[0] !=  ptr[buff_num_times * j])
							{
								errors_by_chan[adc*8+j]++;
								mismatch++;
							}
						if (mismatch)
						{
							if (errors < max_errors)
							{
								mprintf(MP_ERR, "ERROR at start of transfer, channels out of step\n");
								for (j=0; j<8; j++)
									mprintf(MP_ERR, (j==7)?"\t%03X\n":"\t%03X", ptr[buff_num_times*j]);
							}
							else if (errors == max_errors)
								mprintf(MP_ERR, "Too many errors. Counting\n");
							errors++;
						}
						for (j=0; j<8; j++)
							expected[j] = ptr[buff_num_times * j];
					}
					else
					{
						mismatch = 0;
						for (j=0; j<8; j++)
						{
							if ((actual = ptr[buff_num_times*j+i]) != expected[j])
								mismatch++;
						}
						if (mismatch)
						{
							for (j=0; j<8; j++)
							{
								if ((actual = ptr[buff_num_times*j+i]) != expected[j])
								{
									if (errors < max_errors)
									{
										mprintf(MP_ERR, "ERROR at offset Time %d, Chan%d. Read 0x%04X, Expected 0x%04X\n", burst*burst_len_words/4+i, j, actual, expected[j]);
									}
									else if (errors == max_errors)
										mprintf(MP_ERR, "Too many errors. Counting\n");
									errors++;
									errors_by_chan[adc*8+j]++;
									if (mismatch != 8)
										error_bits[adc*8+j] |= actual ^ expected[j];
								}
							}
							errors_at_same_time[adc][mismatch-1]++;
						}
						else if (test_type == 0)
						{
							for (j=0; j<8; j++)
							{
								if (j==0)
									printf("%d\t%03X", burst*burst_len_words/4+i, actual);
								else if (j==7)
									printf("\t%03X\n", actual);
								else
									printf("\t%03X", actual);
							}
						}
					}

					if (mismatch == 8) /* Consider than we lost one sync which will loose two points in ram, so rolling forward pattern twice now gets a match */
					{
						u_int32_t keep[8];

						for (j=0; j<8; j++)
							keep[j] = expected[j];

						roll_prbs(expected);
						roll_prbs(expected);
						mismatch= 0;
						for (j=0; j<8; j++)
						{
							if ((actual = ptr[buff_num_times*j+i]) != expected[j])
								mismatch++;
						}
						if (mismatch < 2)
						{
							num_missing[adc]++;
							if (errors < max_errors)
								mprintf(MP_ERR, "ERROR: *** Missing all 8 samples at time %d \n", burst*burst_len_words/4+i);
						}
						else
						{
							for (j=0; j<8; j++)
								expected[j] = keep[j];
						}
					}
					roll_prbs(expected);
				}
			}
		}
	}
	if (errors > 0)
	{
		mprintf(MP_ERR, "Finished testing DMA with %d errors\n", errors);
		for (adc=first_adc; adc<=last_adc; adc++)
		{
			mprintf(MP_ERR, "ADC%d:Num Errors", adc);
			for (i=0; i<8; i++)
				mprintf(MP_ERR, " %6d", errors_by_chan[adc*8+i]);
			mprintf(MP_ERR,", missing=%d\n", num_missing[adc]);

			mprintf(MP_ERR, "ADC%d:Error Bits", adc);
			for (i=0; i<8; i++)
				mprintf(MP_ERR, " 0x%04X", error_bits[adc*8+i]);

			mprintf(MP_ERR, "\nCoincident Errs", adc);
			for (i=0; i<8; i++)
				mprintf(MP_ERR, " %6d", errors_at_same_time[adc][i]);

			mprintf(adc==last_adc?MP_ERREX:MP_ERR,"\n\n");
		}
	}

	num_errors += errors;
	free(buff);
}

roll_prbs(u_int32_t * expected)
{
	int j, k;
	for (j=0; j<8; j++)
	{
		for (k=0; k<14; k++)
			expected[j] = (expected[j]<<1 | (expected[j] >> 8 & 1 ^ expected[j] >> 4 & 1)) & 0x3fff;
	}
}
#endif
/* Lookup table version */
#define NPTS_PRBS 511
int prbs_pn9_dma_test(int path, int card, int num_pass)
{
	u_int16_t *buff[2], *ptr;
	int i, j, k, chan;
	unsigned int errors = 0;
	int pass;
	int adc;
	int  test_mode = AD9694_TEST_MODE_PRBS_SHORT;
	u_int32_t expected, actual;
	int mismatch[2], mismatch_between;
	int num_missing[NGZMP_CHANS_PER_CARD];
	int errors_by_chan[NGZMP_CHANS_PER_CARD];
	int errors_at_same_time[NGZMP_CHANS_PER_CARD][NGZMP_CHANS_PER_CARD];
	u_int32_t error_bits[NGZMP_CHANS_PER_CARD];
	int rpos[NGZMP_CHANS_PER_CARD];
	int16_t prbs[NPTS_PRBS];
	int x;
	int rc;
	int dma, num_dma=2;
	ZYNQMP_DMA_StatusBlock statusBlock;
	u_int64_t remaining;
	int chunk_bytes;
	u_int64_t chunk_start;
	int t, nt;
//FILE *ofp = fopen("prbs.xtx", "w");
	x = 0x125B;
//	x = 0x92;
	for (i=0; i<NPTS_PRBS; i++)
	{
		prbs[i] = x;
		// printf("%d\t%04X\n", i, x);
		for (k=0; k<14; k++)
			x = (x<<1 | (x >> 8 & 1 ^ x >> 4 & 1)) & 0x3fff;
	}
//	fclose(ofp);
	if ((rc=ngzmp_adc_setup_test(path, card, -1, 3, 3, test_mode, NULL)) < 0)
	{
		mprintf(MP_ERR, "Cannot setup ADC test pattern output: %s\n", ngpd_get_error_message());
		num_io_errors++;
		return -1;
	}

	mprintf(1, "Testing DMA with %d passes", num_pass);
	for (dma=0; dma<num_dma; dma++)
	{
		if ((buff[dma] = (u_int16_t *)malloc(BUFF_WORDS*sizeof(u_int16_t))) == NULL)
		{
			mprintf(MP_ERREX, "Insufficient memory\n");
			exit(1);
		}
	}
	for (i=0; i<NGZMP_CHANS_PER_CARD; i++)
	{
		error_bits[i] =0;
		errors_by_chan[i] = 0;
	}
	for (i=0;i<NGZMP_CHANS_PER_CARD; i++)
		num_missing[i] = 0;
	for (i=0;i<NGZMP_CHANS_PER_CARD; i++)
	{
		for (j=0; j<NGZMP_CHANS_PER_CARD; j++)
			errors_at_same_time[i][j] = 0;
	}

	for (i=0; i<NGZMP_CHANS_PER_CARD; i++)
	{
		if ((rc=ngpd_scope_setup_stream(path, card, i, i, NGZMP_SCOPE_SEL_ANA_INPUT, 0)) < 0)
		{
			mprintf(MP_ERR, "Cannot setup scope mode: %s\n", ngpd_get_error_message());
			num_io_errors++;
			return -1;
		}
	}
	
	for (pass = 0; pass < num_pass; pass++)
	{
		mprintf(1, "Testing pass %d\n", pass);

		if ((rc=ngzmp_system_start_count_enb(path, card, 0, 8, NGPD_RUN_FLAGS_SCOPEMODE | NGPD_RUN_FLAGS_SCOPE_REGS_TO_MOD, 0)) < 0)
		{
			mprintf(MP_ERR, "Cannot run scope mode: %s\n", ngpd_get_error_message());
			num_io_errors++;
			return -1;
		}
		if ((rc=ngpd_dma_wait_scope(path, card, NULL)) < 0)
		{
			mprintf(MP_ERR, "Error waiting for scope mode: %s\n", ngpd_get_error_message());
			num_io_errors++;
			return -1;
		}
		sleep(1);
		rc = ngzmp_get_dma_status_block(path, card, &statusBlock);
		if (rc != 0)
		{
			mprintf(MP_ERR, "check_scope_read_tp_data: Error Reading Status Block %s\n", ngpd_get_error_message());
			num_io_errors++;
			return -1;
		}
		remaining = statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0].transfer_size;
		mprintf(1, ".... checking data from %d DMA, transfer size=%lld\n", num_dma, remaining);
		chunk_start = 0;
		t = 0;
		for (j=0;j<NGZMP_CHANS_PER_CARD; j++)
			rpos[j] = -1;
		while (remaining > 0)
		{
			if (remaining > BUFF_WORDS*sizeof(u_int16_t))
				chunk_bytes = BUFF_WORDS*sizeof(u_int16_t);
			else
				chunk_bytes = remaining;
			nt = chunk_bytes/(4*sizeof(u_int16_t));		
			for (dma=0; dma<num_dma; dma++)
			{
				int num_lwords = chunk_bytes/sizeof(u_int32_t);
				int bytes_offset = chunk_start;
				if (dma == 2)
				{
					num_lwords /=2;
					bytes_offset /=2;
				}
			mprintf(3, ".... Reading DMA=%d, chunk byte start=%lld=0x%08llX, transfer_start=%lld=0x%llX, size=%lld=0x%08llX, nt=%d\n", 
				dma, bytes_offset, bytes_offset, statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0+dma].transfer_start, statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0+dma].transfer_start,
				 num_lwords*sizeof(u_int32_t), num_lwords*sizeof(u_int32_t), nt); 
				if ((rc=zynqmp_read_dma_buff(path, card, NGZMP_DMA_STREAM_BNUM_SCOPE0, (bytes_offset+statusBlock.stream_def[NGZMP_DMA_STREAM_BNUM_SCOPE0+dma].transfer_start)/sizeof(u_int32_t), num_lwords, (u_int32_t *)buff[dma])) < 0)
				{
					mprintf(MP_ERR, "check_scope_read_tp_data: Error Reading Scope DMA buffer %s\n", ngpd_get_error_message());
					num_io_errors++;
					return -1;
				}
			}
			if (t == 0)
			{
				mismatch[0] = mismatch[1] = 0;
				mismatch_between = 0;
				for (dma=0; dma<num_dma; dma++)
				{
					for (i=1; i<4; i++)
					{
						if (buff[dma][0] != buff[dma][i])
						{
							errors_by_chan[i+4*dma]++;
							mismatch[dma]++;
						}
					}
					for (i=0; i<4; i++)
					{
						if (buff[0][i] != buff[1][i])
							mismatch_between++;
					}
				}
				if (mismatch[0] || mismatch[1] || mismatch_between)
				{
					if (errors < max_errors)
					{
						mprintf(MP_ERR, "ERROR at start of transfer, channels out of step\n");
						for (dma=0; dma<num_dma; dma++)
						{
							for (j=0; j<4; j++)
								mprintf(MP_ERR, (j==3&&dma==num_dma-1)?"\t%03X\n":"\t%03X", buff[dma][j]);
						}
					}
					else if (errors == max_errors)
						mprintf(MP_ERR, "Too many errors. Counting\n");
					errors++;
				}
			}
				
			for (i=0; i<nt; i++)
			{
				mismatch[0] = mismatch[1] = 0;
				mismatch_between = 0;
				chan = 0;
				for (dma=0; dma<num_dma; dma++)
				{
					ptr= buff[dma]+4*i;
					for (j=0; j<4; j++, chan++)
					{
						actual = ptr[j] >> 2;
						if (actual == 0)
						{
							if (errors < max_errors)
							{
								mprintf(MP_ERR, "ERROR at start of transfer, channels %d reads %d. PRBS cannot give 0\n", chan, actual);
							}
							else if (errors == max_errors)
								mprintf(MP_ERR, "Too many errors. Counting\n");
							errors++;
							errors_by_chan[chan]++;
							rpos[chan] = -1;
						}
						else if (rpos[chan] == -1)
						{
							for (k=0; k<NPTS_PRBS; k++)
							{
								if (actual == prbs[k])
									break;
							}
							if (k == NPTS_PRBS)
							{
								if (errors < max_errors)
									mprintf(MP_ERR, "ERROR Step %d Chan %d 0x%04X is not valid pattern\n", t, j+4*dma, actual);
								else if (errors == max_errors)
									mprintf(MP_ERR, "Too many errors. Counting\n");
								errors++;
								errors_by_chan[chan]++;
							}
							else
								rpos[chan] = k;
						}
						else
						{
							k = rpos[chan] = (rpos[chan]+1) % NPTS_PRBS;
							expected = prbs[k];
							if (actual != expected)
							{
								mismatch[dma]++;
								if (errors < max_errors)
								{
									mprintf(MP_ERR, "ERROR at offset Time %d, Chan %d. Read 0x%04X, Expected 0x%04X\n", t, chan, actual, expected);
								}
								else if (errors == max_errors)
									mprintf(MP_ERR, "Too many errors. Counting\n");
								errors++;
								errors_by_chan[chan]++;
							}
						}
					} /* End for j */
					if (mismatch[dma] > 0)
					{
						chan = 4*dma;
						if (mismatch[dma] != 4)
						{
							for (j=0; j<4 ; j++)
							{
								actual = ptr[j]>>2;
								expected = prbs[rpos[chan]];
								error_bits[chan++] |= actual ^ expected;
							}
							errors_at_same_time[dma][mismatch[dma]-1]++;
						}
						else
						{
							/* Mismatch == 4 so consider we may have lost 1 word, possible one LMFC??? */
							/*
								mismatch = 0;
								for (j=0; j<NGZMP_CHANS_PER_CARD ; j++)
								{
									actual = ptr[buff_num_times * j+i] >> 2;
									k = (rwp[j]+2) % 511;
									expected = prbs[k];
									if (actual != expected)
										mismatch++;
								}
								if (mismatch < 2)
								{
									num_missing[adc]++;
									if (errors < max_errors)
										mprintf(MP_ERR, "ERROR: *** Missing all NGZMP_CHANS_PER_CARD samples at time %d \n", burst*burst_len_words/4+i);
									for (j=0; j<NGZMP_CHANS_PER_CARD; j++)
										rwp[j] = (rwp[j]+2) % 511;
								}
							*/
						}
					}
					else if (reporting >= 4 && t < 1000)
					{
						for (j=0; j<4; j++)
						{
							actual = ptr[j] >> 2;
							if (j==0&&dma==0)
								printf("%d\t%03X", t, actual);
							else if (j==3&&dma==1)
								printf("\t%03X\n", actual);
							else
								printf("\t%03X", actual);
						}
					}
				}
				t++;
			}
			chunk_start += chunk_bytes;
			remaining -= chunk_bytes;
		}
	}
	if (errors > 0)
	{
		mprintf(MP_ERR, "Finished testing ADC pRBS with %d errors\n", errors);
		for (i=0; i<NGZMP_CHANS_PER_CARD; i++)
			mprintf(MP_ERR, "Chan %d: ERRORS = %6d, ERROR Bits= 0x%04X\n", i, errors_by_chan[i], error_bits[i]);

/*		mprintf(MP_ERR, "\nCoincident Errs", adc);
			for (i=0; i<NGZMP_CHANS_PER_CARD; i++)
				mprintf(MP_ERR, " %6d", errors_at_same_time[adc][i]);

			mprintf(adc==last_adc?MP_ERREX:MP_ERR,"\n\n");
*/
	}
	if (errors > 100)
	{
		printf("Forcing stop to investigate large number of errors\n");
		exit(0);
	}
	num_errors += errors;
	for (dma=0; dma<num_dma; dma++)
		free(buff[dma]);
	return 0;
}

int ngpd_list_jesd_regs(int path, int card)
{
	u_int32_t ip[1], reset, cfg[4], rx_err_status, rx_status[2];
	int link;
	int rc;
	for (link=0; link<4; link++)
	{
		if ((rc=ngpd_read_jesd_regs(path, card, link, JESD_IP_VERSION, 1, ip)) < 0)
		{
			mprintf(MP_ERR, "! ERROR reading JESD_IP_VERSION=0x%03X : %s\n", JESD_IP_VERSION, ngpd_get_error_message());
			return -1;
		}
		if ((rc=ngpd_read_jesd_regs(path, card, link, JESD_RESET, 1, &reset)) < 0)
		{
			mprintf(MP_ERR, "! ERROR reading JESD_RESET=0x%03X: %s\n", JESD_RESET, ngpd_get_error_message());
			return -1;
		}
		
		if ((rc=ngpd_read_jesd_regs(path, card, link, JESD_STAT_RX_ERR, 1, &rx_err_status)) < 0)
		{
			mprintf(MP_ERR, "! ERROR reading JESD_STAT_RX_ERR=%08X: %s\n", JESD_STAT_RX_ERR, ngpd_get_error_message());
			return -1;
		}
		
		if ((rc=ngpd_read_jesd_regs(path, card, link, JESD_CFG_OCTETS, 4, cfg)) < 0)
		{
			mprintf(MP_ERR, "! ERROR reading JESD_CFG_OCTETS=0x%03X: %s\n", JESD_CFG_OCTETS, ngpd_get_error_message());
			return -1;
		}

		if ((rc=ngpd_read_jesd_regs(path, card, link, JESD_SYNC_STATUS, 2, rx_status)) < 0)
		{
			mprintf(MP_ERR, "! ERROR reading JESD_CFG_OCTETS=0x%03X: %s\n", JESD_SYNC_STATUS, ngpd_get_error_message());
			return -1;
		}

		printf("# Card %02d : link %d : IP_VERSION = %08X, Octets=0x%08X, Frames=%08X, Lanes=%08X, Subclass=%08X\n", card, link, ip[0], cfg[0], cfg[1], cfg[2], cfg[3]);
		printf("#           RESET=%08X, RX_ERR = %08X, Sync STATUS=%08X, RX_DEBUG=%08X\n",  reset, rx_err_status, rx_status[0], rx_status[1]);
	}		
	return 0;
}
