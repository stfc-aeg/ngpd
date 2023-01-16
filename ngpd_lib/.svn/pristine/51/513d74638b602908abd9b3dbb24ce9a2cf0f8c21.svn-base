#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "xspress3.h"
#include "xspress3test.h"
#include "errors.h"

int spi_test_spartan(int path);
int spi_write_clks(int path);

int spi_tests(int path)
{
	int errors = 0;
	mprintf(1, "SPI TESTS\n");

	if (test_spi & TEST_SPI_SPARTAN)
		errors += spi_test_spartan(path);

	if (test_spi & TEST_SPI_WRITE_CLKS)
		spi_write_clks(path);

	return errors;
}


int spi_test_spartan(int path)
{
	int errors=0;
	int io_errors=0;
	int i;
	u_int32_t x, y;

	mprintf(1, "Testing SPI Write/Read to Spartan\n");

	for (i=1;i < 256; i++)
	{
		x = 0x050301*i;
		if (xsp3_write_spi_reg(path, 0, XSP3_SPI_SS_SPARTAN, 1, &x) != 0)
		{
			mprintf(MP_ERR, "xsp3_write_spi_reg: failed: '%s'\n", xsp3_get_error_message());
			io_errors++;
		}

		if (xsp3_read_spi_reg(path, 0, XSP3_SPI_SS_SPARTAN, 1, &y) != 0)
		{
			mprintf(MP_ERR, "xsp3_read_spi_reg: failed: '%s'\n", xsp3_get_error_message());
			io_errors++;
		}

		if ( (x & XSP3_SPI_S3_READ_WRITE_MASK) != (y & XSP3_SPI_S3_READ_WRITE_MASK)) 
		{
			errors++;
			if (errors < max_errors)
				mprintf(MP_ERR, "ERROR on SPI Write: Wrote %08X, Read %08X\n", x, y);
			else if (errors == max_errors)
				mprintf(MP_ERR, "Too many errors, countign\n");
		}
	}
	
	mprintf((errors || io_errors)?MP_ERREX:1, "Finished Spartan 3 SPI test with %d errors and %d io errors\n", errors, io_errors);

	num_io_errors += io_errors;
	num_errors += errors;
	return errors;
}

int spi_write_clks(int path)
{
	u_int32_t x=0;
	printf("Write clock forever, stop with ^C\n");

	do
	{
		if (test_spi & TEST_SPI_WRITE_CLK0)
			xsp3_write_spi_reg(path, 0, XSP3_SPI_SS_CLK0, 1, &x);
		x++;

		if (test_spi & TEST_SPI_WRITE_CLK1)
			xsp3_write_spi_reg(path, 0, XSP3_SPI_SS_CLK1, 1, &x);
		x++;
		if (test_spi & TEST_SPI_WRITE_CLK2)
			xsp3_write_spi_reg(path, 0, XSP3_SPI_SS_CLK2, 1, &x);
		x++;
		if (test_spi & TEST_SPI_WRITE_CLK3)
			xsp3_write_spi_reg(path, 0, XSP3_SPI_SS_CLK3, 1, &x);
		x++;
	} while (1);
	return 0;
}
