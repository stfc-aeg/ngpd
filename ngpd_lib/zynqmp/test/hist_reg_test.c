/*
 * hist_reg_test.c
 *
 *  Created on: Jan 2021
 *      Author: wih
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "args.h"
#include "errors.h"

#include "ngzmp.h"
#include "ngzmptest.h"
#include "zynqmp_lib_driver.h"
#include "ngzmp_lib_driver.h"


int test_hist_read_config(int path, int card)
{
	AXIHistConfig hist_conf;
	int rc;
	int io_errors = 0;

	rc = ngzmp_hist_read_config(path, card, &hist_conf);

	if (rc != 0)
	{
		io_errors++;
		printf("Error reading AXI Histogrammer config\n");
	}

	printf("NBitsData Hist   = %d\n", hist_conf.NBitsDataHist );
	printf("NBitsData AXI    = %d\n", hist_conf.NBitsDataAXI );
	printf("Num Streams      = %d\n",	hist_conf.NumStreams );
	printf("NBits Addr Top   = %d\n", hist_conf.NBitsAddrTopFixed );
	printf("NBits Stream     = %d\n", hist_conf.NBitsStream );
	printf("NBits Sub Stream = %d\n", hist_conf.NBitsSubStream );
	printf("NBits Bank/Grp   = %d\n", hist_conf.NBitsBankAndGroup );
	printf("NBits Addr Mem   = %d\n", hist_conf.NBitsAddrMem );
	printf("NBits Addr In    = %d\n", hist_conf.NBitsAddrIn );
	printf("Stream at Bottom = %d\n", hist_conf.StreamAtBottom );
	printf("Hist TPG Present = %d\n", hist_conf.HistTPGPresent );
	printf("Hist Cache       = %d  => %s\n", hist_conf.Cache, axi_hist_cache_names[hist_conf.Cache]);
	printf("Read/Write Delays= %d  => %s\n", hist_conf.ReadWaitWrite, axi_hist_read_write_names[hist_conf.ReadWaitWrite]);

	if (report_file != NULL)
	{
		fprintf(report_file, "NBitsData Hist   = %d\n", hist_conf.NBitsDataHist );
		fprintf(report_file, "NBitsData AXI    = %d\n", hist_conf.NBitsDataAXI );
		fprintf(report_file, "Num Streams      = %d\n",	hist_conf.NumStreams );
		fprintf(report_file, "NBits Addr Top   = %d\n", hist_conf.NBitsAddrTopFixed );
		fprintf(report_file, "NBits Stream     = %d\n", hist_conf.NBitsStream );
		fprintf(report_file, "NBits Sub Stream = %d\n", hist_conf.NBitsSubStream );
		fprintf(report_file, "NBits Bank/Grp   = %d\n", hist_conf.NBitsBankAndGroup );
		fprintf(report_file, "NBits Addr Mem   = %d\n", hist_conf.NBitsAddrMem );
		fprintf(report_file, "NBits Addr In    = %d\n", hist_conf.NBitsAddrIn );
		fprintf(report_file, "Stream at Bottom = %d\n", hist_conf.StreamAtBottom );
		fprintf(report_file, "Hist TPG Present = %d\n", hist_conf.HistTPGPresent );
		fprintf(report_file, "Hist Cache       = %d  => %s\n", hist_conf.Cache, axi_hist_cache_names[hist_conf.Cache]);
		fprintf(report_file, "Read/Write Delays= %d  => %s\n", hist_conf.ReadWaitWrite, axi_hist_read_write_names[hist_conf.ReadWaitWrite]);
	}
	
	num_io_errors += io_errors;
	return 0;
}

int test_hist_reg_wr_rd(int path, int card)
{
	int rc, i, j;
	int pass;
	u_int32_t wr_data[6];
	u_int32_t rd_data[6];
	int io_errors = 0;
	int errors = 0;
	int num_words, start_word, n;
	printf("HIST-REGS: Write & read back long values from histogram block control registers\n");


	if (manual_test) { printf("Press return to run simple tests\n"); getchar(); }
	for (pass=0; pass<10; pass++)
	{
		num_words = 6;
		for (j=0;j<num_words; j++)
			wr_data[j] = (3*pass+j)*0x07050301;
		rc = ngzmp_hist_write_reg(path, card, AXI_HIST_CLEAR_CONT, num_words, wr_data);
		if (rc < 0)
		{
			io_errors ++;
			if (io_errors < max_errors)
				printf("Error writing Histogram cobtrol register, rc=%d\n", rc);
			else if (io_errors == max_errors)
				printf ("Too many errors. Counting\n");
		}
		rc = ngzmp_hist_read_reg(path, card, AXI_HIST_CLEAR_CONT, num_words, rd_data);
		if (rc < 0)
		{
			io_errors ++;
			if (io_errors < max_errors)
				printf("Error reading Histogram cobtrol register, rc=%d\n", rc);
			else if (io_errors == max_errors)
				printf ("Too many errors. Counting\n");
		}
		for (j=0; j<num_words;j++)
		{
			if (rd_data[j] != wr_data[j])
			{
				errors++;
				if (errors < max_errors)
					printf("ERROR at offset %d : Wrote 0x%08X, Read 0x%08X\n", j+AXI_HIST_CLEAR_CONT, wr_data[j], rd_data[j]);
				else if (errors == max_errors)
					printf("Too many errors, counting\n");
			}
			else if (reporting > 4)
				printf("Correct: Offset %d : data=0x%08X\n", j+AXI_HIST_CLEAR_CONT, wr_data[j]);
		}
	}
	printf(".. Fininshed simple tests with %d errors\n", errors);
	if (manual_test) { printf("Press return to continue\n"); getchar(); }
	for (pass=0; pass<100; pass++)
	{
		num_words = (pass % 6)+1;
		start_word = (pass/6) % 5;
		for (j=0;j<6; j++)
			wr_data[j] = (pass+j)*0x07050301;
		if (manual_test) { printf("Press return to run with num_words=%d, start_word=%d\n", num_words, start_word); getchar(); }
		for (i=0;i<6; )
		{
			if (i < start_word)
				n = 1 ;
			else
			{
				n= num_words;
				if (i+n > 6)
					n = 6-i;
			}
			if (reporting > 4)
				printf("write %d,%d\n", AXI_HIST_CLEAR_CONT+i, n);
			rc = ngzmp_hist_write_reg(path, card, AXI_HIST_CLEAR_CONT+i, n, wr_data+i);
			if (rc < 0)
			{
				io_errors ++;
				if (io_errors < max_errors)
					printf("Error writing Histogram control register, rc=%d\n", rc);
				else if (io_errors == max_errors)
					printf ("Too many errors. Counting\n");
			}
			i+=n;
		}
		for (i=0;i<6; i++)
		{
			if (i < start_word)
				n = 1 ;
			else
			{
				n= num_words;
				if (i+n > 6)
					n = 6-i;
			}
			rc = ngzmp_hist_read_reg(path, card, AXI_HIST_CLEAR_CONT+i, n, rd_data+i);
			if (rc < 0)
			{
				io_errors ++;
				if (io_errors < max_errors)
					printf("Error reading Histogram control register, rc=%d\n", rc);
				else if (io_errors == max_errors)
					printf ("Too many errors. Counting\n");
			}
		}
		for (j=0; j<6;j++)
		{
			if (rd_data[j] != wr_data[j])
			{
				errors++;
				if (errors < max_errors)
					printf("ERROR at offset %d : Wrote 0x%08X, Read 0x%08X\n", j+AXI_HIST_CLEAR_CONT, wr_data[j], rd_data[j]);
				else if (errors == max_errors)
					printf("Too many errors, counting\n");
			}
			else if (reporting > 4)
				printf("Correct: Offset %d : data=0x%08X\n", j+AXI_HIST_CLEAR_CONT, wr_data[j]);
		}
	}
	if (errors != 0 || io_errors != 0)
	{
		printf("Found %d errors amd %d IO error testing Write/Read to Histogram Registers\n", errors, io_errors);
		num_errors += errors;
		num_io_errors += io_errors;
	}
	else
		printf("Finished Hist register write/read test with no errors\n");

	return 0;
}
