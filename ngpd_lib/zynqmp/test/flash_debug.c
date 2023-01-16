#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "xspress3.h"
#include "xspress4_lib_driver.h"
#include "xsp4_driver.h"

int main(int argc, char **argv)
{
	int path, xsp_path;
	struct spi_ioc_transfer xfer;
	int i,j;
	char tx_data[7], rx_data[7];

	xsp_path = xsp3_config(1, 1, NULL, -1, NULL, -1, 1, NULL, 1, 0);
//	path = open ("/dev/flash", O_RDWR);
	path = open ("/dev/spidev32765.0", O_RDWR);
	if (path < 0)
	{
		fprintf(stderr, "Cannot open path, errno=%d\n", errno);
		exit(errno);
	}

	memset(&xfer, 0, sizeof (struct spi_ioc_transfer));
	for (i=0; i<7; i++)
		tx_data[i] = 0;
	
	for (i=1; i<argc; i++)
	{
		if (argv[i][0] == '-')
		{
			if (strcmp(argv[i], "-reset") == 0)
			{
				u_int32_t reg;
				xsp4_read(xsp_path, XSP4_ADDR_SPACE_LOCAL, XSP4_ZYNQ_LOCAL_CONF_CONT, 1, &reg);
				reg |= XSP4_ZYNC_CONF_CONT_FLASH_RST;
				printf("Writing 0x%02X to assert Flash Reset\n", reg);
				xsp4_write(xsp_path, XSP4_ADDR_SPACE_LOCAL, XSP4_ZYNQ_LOCAL_CONF_CONT, 1, &reg);
				sleep(1);
				reg &= ~XSP4_ZYNC_CONF_CONT_FLASH_RST;
				printf("Writing 0x%02X to negate Flash Reset\n", reg);
				xsp4_write(xsp_path, XSP4_ADDR_SPACE_LOCAL, XSP4_ZYNQ_LOCAL_CONF_CONT, 1, &reg);
				sleep(1);
			}
			if (strcmp(argv[i], "-assert") == 0)
			{
				u_int32_t reg;
				xsp4_read(xsp_path, XSP4_ADDR_SPACE_LOCAL, XSP4_ZYNQ_LOCAL_CONF_CONT, 1, &reg);
				reg |= XSP4_ZYNC_CONF_CONT_FLASH_RST;
				printf("Writing 0x%02X to assert Flash Reset\n", reg);
				xsp4_write(xsp_path, XSP4_ADDR_SPACE_LOCAL, XSP4_ZYNQ_LOCAL_CONF_CONT, 1, &reg);
				sleep(1);
			}
			if (strcmp(argv[i], "-negate") == 0)
			{
				u_int32_t reg;
				xsp4_read(xsp_path, XSP4_ADDR_SPACE_LOCAL, XSP4_ZYNQ_LOCAL_CONF_CONT, 1, &reg);
				reg &= ~XSP4_ZYNC_CONF_CONT_FLASH_RST;
				printf("Writing 0x%02X to negate Flash Reset\n", reg);
				xsp4_write(xsp_path, XSP4_ADDR_SPACE_LOCAL, XSP4_ZYNQ_LOCAL_CONF_CONT, 1, &reg);
				sleep(1);
			}
		}
		else
		{
			memset(rx_data, 0, 7);
			tx_data[0] = strtol(argv[i], NULL, 16);
			xfer.tx_buf = (u_int64_t)tx_data;
			xfer.rx_buf = (u_int64_t)rx_data;
			xfer.len = 7;
			xfer.bits_per_word = 8;
			if (ioctl(path, SPI_IOC_MESSAGE(1), &xfer) < 0)
			{
				perror("main : ioctl");
			}
			printf("Command 0x%02X :", tx_data[0]);
			for (j=0; j<7; j++)
				printf(" %02X", rx_data[j]);
			printf("\n");
		}
	}
	return 0;
}