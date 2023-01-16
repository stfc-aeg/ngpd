#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ngpd.h"
#include "ngpdtest.h"

#define NUM_WORDS_SHORT 64
#define NUM_WORDS_MEDIUM 2048
#define NUM_WORDS_LONG (2<<28)

int dma_tests(int path)
{
	if (test_dma & TEST_DMA_DISPLAY)
		dma_display(path);
	if (test_dma & TEST_DMA_UPLOAD_SHORT)
		dma_upload_short(path);
	if (test_dma & TEST_DMA_READ_SIZE)
		dma_read_size(path);
	if (test_dma & TEST_DMA_RUN_PLAYBACK)
		dma_run_playback(path);

	if (test_dma & TEST_DMA_READ_SCOPE)
		dma_test_read_scope(path, 0, 0);
	if (test_dma & TEST_DMA_READ_SCOPE_TP)
		dma_test_read_scope(path, 1, 0);
	if (test_dma & TEST_DMA_READ_SCOPE_TP_NO_CHUNK)
		dma_test_read_scope(path, 1, NGPD_READ_SCOPE_NO_CHUNK);
	if (test_dma & TEST_DMA_READ_SCOPE_TP_FLOW_CONT)
		dma_test_read_scope(path, 1, NGPD_READ_SCOPE_FLOW_CONTROL);

	if (test_dma & TEST_DMA_SCOPE_TP)
		dma_test_scope_tp(path, 1);

	return 0;
}

int dma_upload_short(int path)
{
	int rc, i;
	u_int32_t wr_buff[NUM_WORDS_SHORT];
	int errors = 0;		
	int io_errors=0;


	printf("Testing DMA of short burst of data via uploader\n");

	for (i=0; i<NUM_WORDS_SHORT; i++) 
	{ 
		wr_buff[i] = 0x01050301*i;
	}

	rc = ngpd_dma_upload(path, 0, NUM_WORDS_SHORT, wr_buff);

	if (rc != 0)
	{
		printf("Error running dma_upload: %s\n", ngpd_get_error_message());
		io_errors++;
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed Upload DMA short bursts test\n");
	else
	{
		printf("Uploader DMA tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
		error_tests++;
	}
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}
int dma_upload_medium(int path)
{
	int rc, i;
	u_int32_t wr_buff[NUM_WORDS_MEDIUM];
	int errors = 0;		
	int io_errors=0;


	printf("Testing DMA of %d word burst of data via uploader\n", NUM_WORDS_MEDIUM);

	for (i=0; i<NUM_WORDS_MEDIUM; ) 
	{ 
		wr_buff[i] = i | 0xA0000000;
		i++;
		wr_buff[i] = i | 0x50000000;
		i++;
	}

	rc = ngpd_dma_upload(path, 0, NUM_WORDS_MEDIUM, wr_buff);

	if (rc != 0)
	{
		printf("Error running dma_upload: %s\n", ngpd_get_error_message());
		io_errors++;
	}
	if (errors == 0 && io_errors == 0)
		printf("Passed Upload DMA short bursts test\n");
	else
	{
		printf("Uploader DMA tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
		error_tests++;
	}
	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}

int dma_upload_full(int path)
{
	int rc, i;
	u_int32_t *wr_buff, *rd_buff;
	int errors = 0;		
	int io_errors=0;

	printf("Testing DMA of full buffer burst of data via uploader\n");

	wr_buff = (u_int32_t *)malloc(NUM_WORDS_LONG*sizeof(u_int32_t));
	rd_buff = (u_int32_t *)malloc(NUM_WORDS_LONG*sizeof(u_int32_t));
	if (wr_buff == NULL || rd_buff == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}
	
	for (i=0; i<NUM_WORDS_LONG; ) 
	{ 
		wr_buff[i] = i | 0xA0000000;
		i++;
		wr_buff[i] = i | 0x50000000;
		i++;
	}

	rc = ngpd_dma_upload(path, 0, NUM_WORDS_LONG, wr_buff);

	if (rc != 0)
	{
		printf("Error running dma_upload: %s\n", ngpd_get_error_message());
		io_errors++;
	}



	if (errors == 0 && io_errors == 0)
		printf("Passed Upload DMA short bursts test\n");
	else
	{
		printf("Uploader DMA tests failed with %d IO Errors and %d Data Errors\n", io_errors, errors);
		error_tests++;
	}
	num_io_errors += io_errors;
	num_errors += errors;
	free(rd_buff);
	free(wr_buff);
	return errors;
}
#define NUM_WORDS_READ (32*32)
int dma_display(int path)
{
	int i, j, k, rc;
	u_int32_t rd_buff[NUM_WORDS_READ];
	unsigned int first, bytes_received;

	for (i=0; i<20; i++)
	{
		if (i==0)
			first = 0;
		else
			first = NUM_WORDS_READ << (i-1);

		printf("Calling ngpd_dma_read_playback(%d, %d, %d...)\n", path, first, NUM_WORDS_READ);
		rc = ngpd_dma_read_playback(path, first, NUM_WORDS_READ, rd_buff, &bytes_received);
		if (rc != 0)
			printf("Error calling ngpd_dma_read_playback: %s\n", ngpd_get_error_message());
		printf("Start offset=%X, bytes received=%d\n", first, bytes_received);
		for (j=0; j<64; j++)
		{
			printf("%08X:", first+j/2);
			for (k=0;k<16; k++)
				printf(" %08X", rd_buff[j*16+k]);
			printf("\n");
		}
	}
	return 0;
}

int dma_read_size(int path)
{
	int rc;
//	int j;
	u_int32_t *rd_buff;
	unsigned int first, bytes_received;
	size_t rd_size;
	rd_buff = (u_int32_t *)malloc((size_t)2*1024*1024*1024);
	
	printf("Testing reading different block sizes\n");

	for (rd_size=32*32; rd_size<=512*1024*1024; rd_size <<= 1)
	{
		first=0;
		printf("Calling ngpd_dma_read_playback(%d, %d, %zu...)\n", path, first, rd_size);
		rc = ngpd_dma_read_playback(path, first, rd_size, rd_buff, &bytes_received);
		if (rc != 0)
			printf("Error calling ngpd_dma_read_playback: message=%s, rc=%d\n", ngpd_get_error_message(), rc);
		printf("Size=%zx, bytes received=%d\n", rd_size, bytes_received);

/* 		for (j=0; j<64; j++)
		{
			printf("%08X:", first+j/2);
			for (k=0;k<16; k++)
				printf(" %08X", rd_buff[j*16+k]);
			printf("\n");
		}
*/
	}
	return 0;
}

int dma_run_playback(int path)
{
	u_int32_t pb_skip=8;
	int card=-1;

	dma_upload_medium(path);

	ngpd_write_glob_regs(path, NGPD_PLAYBACK_SKIP, 1, &pb_skip);
	
	ngpd_set_run_flags(path, NGPD_RUN_FLAGS_PLAYBACK | NGPD_RUN_FLAGS_PLAYBACK_CONTINUOUS | NGPD_RUN_FLAGS_SCOPEMODE);

	ngpd_dma_system_start(path, card, 0, NUM_WORDS_MEDIUM/16, NULL);
	return 0;
}

int dma_test_read_scope(int path, int use_tp, int read_options)
{
	int rc;
	int run_flags = 0;
	int io_errors=0;
	NGPDScopeModule * scope_mod = ngpd_scope_get_mod(path);
	int card =-1;
	if (scope_mod == NULL)
	{
		printf("Cannot gte pointer to scope module ; %s\n", ngpd_get_error_message());
		return -1;
	}
	ngpd_scope_set_streams(scope_mod, 4);
	fill_scope_mod(scope_mod);

	if (test_2stream)
		run_flags |= NGPD_RUN_FLAGS_SCOPE_OUT_2_STREAM;
	if (use_tp)
		run_flags |= NGPD_RUN_FLAGS_SCOPE_OUT_TEST;

	ngpd_set_run_flags(path, run_flags);
	if (scope_out_skip != 0)
	{
		ngpd_dma_set_scope_out_skip(path, scope_out_skip);
	}
	rc = ngpd_dma_read_scope(path, card, 0, 0, read_options); // 
	if (rc < 0)
	{
		printf("ngpd_dma_read_scope returns error: %s\n", ngpd_get_error_message());
		io_errors++;
	}

	if (use_tp)
		check_scope_read_tp_data(scope_mod);
	num_io_errors += io_errors;
	return 0;
}


int fill_scope_mod(NGPDScopeModule *mod)
{
	int t, s, c, ns, inc;
	u_int16_t *p;
	u_int16_t x=0;
	ns = ngpd_scope_mod_get_nstreams(mod);
	inc = ngpd_scope_mod_get_inc(mod);
	for (c=0; c<mod->head.num_cards; c++)
	{
		for (s=0; s<ns; s++)
		{
			p = ngpd_scope_mod_get_ptr(mod, c, s);
			if (p == NULL)
			{
				printf("Cannot get pointer into scope module: %s\n", ngpd_get_error_message());
				num_errors++;
				return -1;
			}
			for (t=0; t<mod->head.num_t; t++)
			{
				*p = x;
				x = x ^ 0xFFFF;
				p+= inc;
			}
		}
	}
	return 0;
}

int check_scope_read_tp_data(NGPDScopeModule *mod)
{
	int t, s, c, ns, inc;
	u_int16_t *p, expected;
	int errors=0;
	ns = ngpd_scope_mod_get_nstreams(mod);
	inc = ngpd_scope_mod_get_inc(mod);
	printf("Scope inc=%d\n", inc);
	if (ns != 4)
	{
		printf("Expected number of stream = 4 to check test pattern data\n");
		num_errors++;
		return -1;
	}
	for (c=0; c<mod->head.num_cards; c++)
	{
		p = ngpd_scope_mod_get_ptr(mod, c, 0);
		if (p == NULL)
		{
			printf("Cannot get pointer into scope module: %s\n", ngpd_get_error_message());
			num_errors++;
			return -1;
		}
		for (t=0; t<mod->head.num_t; t++)
		{
			for (s=0; s<4; s++)
			{
				if (test_2stream)
				{
					switch (s)
					{
					case 0: expected = t & 0xFFFF; break;
					case 1: expected = t >> 16; break;
					case 2: expected = (t << 1) & 0xFFFF; break;
					case 3: expected = t >> 15; break;
					}
				}
				else
				{
					switch (s)
					{
					case 0: expected = 2*t & 0xFFFF; break;
					case 1: expected = t >> 15; break;
					case 2: expected = (2*t+1) & 0xFFFF; break;
					case 3: expected = (2*t+1) >> 16; break;
					}
				}
				if (p[s] != expected)
				{
					errors++;
					if (errors < max_errors)
						printf("ERROR: t=%6d=0x%06X=(%d, %d), stream %d: Read %d, expected %d\n", t, t, t % 16384, t/16384, s, p[s], expected);
					else if (errors == max_errors)
						printf("Too many errors, counting\n");
				}
			}
			p+= inc;
		}
	}
	if (errors == 0)
		printf("Checked scope mode buffer OK\n");
	else
	{
		printf("Found %d errors checking scope mode buffer against test patterns\n", errors);
		error_tests++;
	}
	num_errors += errors;
	return 0;
}


int check_scope_tp_data(NGPDScopeModule *mod)
{
	int t, s, c, ns, inc;
	u_int16_t *p, expected;
	int errors=0;
	ns = ngpd_scope_mod_get_nstreams(mod);
	inc = ngpd_scope_mod_get_inc(mod);
	printf("Scope inc=%d\n", inc);

	for (c=0; c<mod->head.num_cards; c++)
	{
		p = ngpd_scope_mod_get_ptr(mod, c, 0);
		if (p == NULL)
		{
			printf("Cannot get pointer into scope module: %s\n", ngpd_get_error_message());
			num_errors++;
			return -1;
		}
		for (t=0; t<mod->head.num_t; t++)
		{
			for (s=0; s<ns; s++)
			{
				if (s== 5)
					expected = 0;
				else if (1 || s < 3)
					expected = (t * (s+1)) & 0x7FFF;
				else
					expected = (-t * (s+1)) & 0x7FFF;

				if (p[s] != expected)
				{
					errors++;
					if (errors < max_errors)
						printf("ERROR: t=%6d=0x%06X=(%d, %d), stream %d: Read %d, expected %d\n", t, t, t % 16384, t/16384, s, p[s], expected);
					else if (errors == max_errors)
						printf("Too many errors, counting\n");
				}
			}
			p+= inc;
		}
	}
	if (errors == 0)
		printf("Checked scope mode buffer with %d stream OK\n", ns);
	else
	{
		printf("Found %d errors checking scope mode buffer with %d streams against test patterns\n", errors, ns);
		error_tests++;
	}
	num_errors += errors;
	return 0;
}



int dma_test_scope_tp(int path, int use_tp)
{
	int rc;
	int num_code, nstreams_code, nstreams;
	int i;
	NGPDScopeModule * scope_mod = ngpd_scope_get_mod(path);
	int run_flags;
	int card = -1;
	if (scope_mod == NULL)
	{
		printf("Cannot get pointer to scope module ; %s\n", ngpd_get_error_message());
		return -1;
	}
	if (use_tp)
		num_code = 4;
	else
		num_code = 1;
	for (nstreams_code=0; nstreams_code<num_code; nstreams_code++)
	{
		switch (nstreams_code)
		{
		case 0: nstreams = 1; break;
		case 1: nstreams = 2; break;
		case 2: nstreams = 4; break;
		case 3: nstreams = 6; break;
		}
		printf("Testing scope mode with nstreams=%d\n", nstreams);
		ngpd_scope_set_streams(scope_mod, nstreams);
		for (i=0; i<nstreams; i++)
			ngpd_scope_setup_stream(path, 0, i, 0, 0, 0);

		fill_scope_mod(scope_mod);
		run_flags = NGPD_RUN_FLAGS_SCOPEMODE | NGPD_RUN_FLAGS_SCOPE_MOD_TO_REGS;

		if (test_2stream)
			run_flags |= NGPD_RUN_FLAGS_SCOPE_OUT_2_STREAM;
		if (!use_tp)
			run_flags |= NGPD_RUN_FLAGS_PLAYBACK;
		ngpd_set_run_flags(path, run_flags);

		if (manual_test)
		{
			printf("Press return to run test\n");
			getchar();
		}
		rc = ngpd_dma_system_start(path, card, 0, 0, NULL);
		if (rc != 0)
			printf("ngpd_dma_system_start retuned error : %s\n", ngpd_get_error_message());
		
		ngpd_dma_wait_scope(path, -1, NULL);
//		sleep (2);
//		rc = ngpd_dma_read_scope(path, card, 0, 0, NGPD_READ_SCOPE_NO_CHUNK);
		rc = ngpd_dma_read_scope(path, card, 0, 0, 0);
		if (rc < 0)
			printf("ngpd_dma_read_scope returns error: %s\n", ngpd_get_error_message());
		if (use_tp)
			check_scope_tp_data(scope_mod);
		if (manual_test)
		{
			printf("Collected data with %d streams, press return\n", nstreams);
			getchar();
		}
	}
	return 0;
}
