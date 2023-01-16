#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <time.h>
#include <errno.h>

#include <string.h>
#include "ngpd.h"


int ngzmp_spi_path_dga[NGZMP_SPI_NUM_DGA];
int ngzmp_spi_path_adc[NGZMP_SPI_NUM_ADC];
int ngzmp_spi_path_clk[NGZMP_SPI_NUM_CLK];


int ngzmp_spi_dga_open=0;
int ngzmp_spi_adc_open=0;
int ngzmp_spi_clk_open=0;

int ngzmp_adc_cur_pair[NGZMP_SPI_NUM_ADC];
int ngzmp_adc_cur_chan[NGZMP_SPI_NUM_ADC][2];

int ngzmp_spi_open_dga()
{
	int i, j;
	char dev_name[40];
	u_int8_t mode, lsb_first, req_mode;
	u_int32_t max_speed;
	int num_paths=NGZMP_SPI_NUM_DGA;

	if (ngzmp_spi_dga_open)
		return 0;

	for (i=0; i<num_paths; i++)
	{
		sprintf(dev_name, "/dev/spidev%d.%d", 1, i);
		printf("Trying to open device %d name='%s'\n", i, dev_name);
		if ((ngzmp_spi_path_dga[i]=open(dev_name, O_RDWR)) < 0)
		{
			perror(dev_name);
			for (j=0; j<i; j++)
			{
				if (ngzmp_spi_path_dga[j] >= 0)
					close(ngzmp_spi_path_dga[j]);
			}
			return -1;
		}
		else 
			printf(" .... Path=%d\n", ngzmp_spi_path_dga[i]);
		if (ioctl(ngzmp_spi_path_dga[i], SPI_IOC_RD_MODE, &mode) < 0)
			perror ("ioctl: SPI_IOC_RD_MODE");
		if (ioctl(ngzmp_spi_path_dga[i], SPI_IOC_RD_LSB_FIRST, &lsb_first) < 0)
			perror ("ioctl: SPI_IOC_RD_MODE");
		if (ioctl(ngzmp_spi_path_dga[i], SPI_IOC_RD_MAX_SPEED_HZ, &max_speed) < 0)
			perror ("ioctl: SPI_IOC_RD_MAX_SPEED_HZ");
		printf("Initial settings: Device %s read: mode=%d, lsb_first=%d, max_speed=%d\n", dev_name, mode, lsb_first, max_speed);
		req_mode = SPI_MODE_0;
		mode =0;
		lsb_first =0;
		max_speed = 892857;
		if (ioctl(ngzmp_spi_path_dga[i], SPI_IOC_WR_MODE, &req_mode) < 0)
			perror ("ioctl: SPI_IOC_WR_MODE");
		if (ioctl(ngzmp_spi_path_dga[i], SPI_IOC_WR_LSB_FIRST, &lsb_first) < 0)
			perror ("ioctl: SPI_IOC_WR_MODE");
		if (ioctl(ngzmp_spi_path_dga[i], SPI_IOC_WR_MAX_SPEED_HZ, &max_speed) < 0)
			perror ("ioctl: SPI_IOC_WE_MAX_SPEED_HZ");

		mode = 0;
		if (ioctl(ngzmp_spi_path_dga[i], SPI_IOC_RD_MODE, &mode) < 0)
			perror ("ioctl: SPI_IOC_RD_MODE");
		if (ioctl(ngzmp_spi_path_dga[i], SPI_IOC_RD_LSB_FIRST, &lsb_first) < 0)
			perror ("ioctl: SPI_IOC_RD_MODE");
		if (ioctl(ngzmp_spi_path_dga[i], SPI_IOC_RD_MAX_SPEED_HZ, &max_speed) < 0)
			perror ("ioctl: SPI_IOC_RD_MAX_SPEED_HZ");
		if (mode != req_mode)
			fprintf(stderr, "Cannot set SPI mode 0 on device %s, read mode=%d\n", dev_name, mode);
		if (lsb_first != 0)
			fprintf(stderr, "Cannot set SPI MSB first on device %s, read lsb_first=%d\n", dev_name, lsb_first);
		if (max_speed > 20000000)
			fprintf(stderr, "Cannot set SPI max speed on device %s, read max speed=%d\n", dev_name, max_speed);
		else
			printf("SPI device %s, max_speed = %d\n", dev_name, max_speed);
	}
	ngzmp_spi_dga_open = 1;
	return 0;
}

int ngzmp_spi_close_dga()
{
	int i;
	int num_paths=NGZMP_SPI_NUM_DGA;

	if (ngzmp_spi_dga_open == 0)
		return 0;
	for (i=0; i<num_paths; i++)
	{
		if (ngzmp_spi_path_dga[i] >= 0)
			close(ngzmp_spi_path_dga[i]);
		ngzmp_spi_path_dga[i] = -1;
	}
	ngzmp_spi_dga_open = 0;

	return 0;
}

/**
 * @ingroup internal
 * Write a series of values into the DGA gain registers.
 * @n@n
 * The addresses is the channel number.
 *
 * @param path a handle to the top level of the xspress3 system returned from {@link xsp3_config()}.
 * @param chan is the channel number of the first channel to write to.
 *        If chan==-1 the first array element is copied to all channels.
 * @param region number  identifying Offset DAC, Gain Switch or CPLD registers
 * @param size the size of the array of values to write, skipping channel to channel.
 * @param value an array of 16 bit integer values to write.
 * @return {@link #XSP3_OK XSP3_OK} or a negative error code.
 */
int ngzmp_spi_write_dga(int path, int chan, int num,  u_int16_t* value)
{
	int rc, i;
	struct spi_ioc_transfer xfer;
	u_int16_t tx_buff, rx_buff;

	if (ngzmp_spi_open_dga() < 0)
		return -1;

	if (chan < 0)
	{
		for (chan=0; chan<NGZMP_CHANS_PER_CARD; chan++)
		{
			rc = ngzmp_spi_write_dga(path, chan, 1, value);
			if (rc < 0)
				return rc;
		}
		return 0;
	}
	else if ( chan >= NGZMP_CHANS_PER_CARD || num < 1 || chan+num > NGZMP_CHANS_PER_CARD)
	{
		fprintf(stderr, "ngzmp_spi_write_dga: ERROR chan %d for %d out of range\n", chan, num);
		return -1;
	}
	for (i=0;i<num; i++)
	{
		memset(&xfer, 0, sizeof(struct spi_ioc_transfer));
		tx_buff = ((value[i]>>8)&0xFF) |  ((value[i]<<8)&0xFF00);
		xfer.tx_buf = (u_int64_t)&tx_buff;
		xfer.rx_buf = (u_int64_t)&rx_buff;
		xfer.len = 2;
		xfer.bits_per_word = 8;
		xfer.cs_change = 0;

		if ((rc=ioctl(ngzmp_spi_path_dga[chan+i], SPI_IOC_MESSAGE(1), xfer)) < 0)
		{
			perror("ngzmp_spi_write_dga : ioctl");
			return -1;
		}
	}
	return 0;
}

/**
 * @ingroup internal
 * Read a series of values from the DGA registers, 1 register per channel.
 * @n@n
 * The adress is the channle number 0..7
 *
 * @param path a handle to the top level of the xspress3 system returned from {@link xsp3_config()}.
 * @param chan is the channel number of the first channel to read from.
 * @param region number  identifying Offset DAC, Gain Switch or CPLD registers
 * @param num the size of the array of values to write, skipping channel to channel.
 * @param value an array of 16 bit integer values to read.
 * @return {@link #XSP3_OK XSP3_OK} or a negative error code.
 */
int ngzmp_spi_read_dga(int path, int chan, int num,  u_int16_t* value)
{
	int i;
	int rc;
	struct spi_ioc_transfer xfer[2];
	u_int16_t tx_buff[2], rx_buff[2];

	if (ngzmp_spi_open_dga() < 0)
		return -1;

	if (chan < 0 || chan >= NGZMP_CHANS_PER_CARD || num < 1 || chan+num > NGZMP_CHANS_PER_CARD)
	{
		fprintf(stderr, "ngzmp_spi_read_dga: ERROR chan %d for %d out of range\n", chan, num);
		return -1;
	}

	for (i=0; i<num; i++)
	{
		memset(xfer, 0, 2*sizeof(struct spi_ioc_transfer));
		tx_buff[0] = 0x1;	// Will byte swap to 0x100 as written out MSB first, by arm is little endian
		tx_buff[1] = 0;
		xfer[0].tx_buf = (u_int64_t)tx_buff;
		xfer[0].rx_buf = (u_int64_t)rx_buff;
		xfer[0].len = 2;
		xfer[0].bits_per_word = 8;
		xfer[0].cs_change = 1;

		xfer[1].tx_buf = (u_int64_t)(tx_buff+1);
		xfer[1].rx_buf = (u_int64_t)(rx_buff+1);
		xfer[1].len = 2;
		xfer[1].bits_per_word = 8;

		if ((rc=ioctl(ngzmp_spi_path_dga[chan+i], SPI_IOC_MESSAGE(2), xfer)) < 0)
		{
			perror("ngzmp_spi_read_dga : ioctl");
			return -1;
		}
//		value[i] = ((rx_buff[1]>>8)&0xFF) | ((rx_buff[1]<<8)&0xFF00); 
		value[i] = ((rx_buff[1]>>8)&0xFF); // Only want the LS Byte, otherwise get the Read/nWrite bit back set.
	}
	return 0;
}


int ngzmp_spi_open_adc()
{
	int i, j;
	char dev_name[40];
	u_int8_t mode, lsb_first, req_mode;
	u_int32_t max_speed;
	int num_paths=NGZMP_SPI_NUM_ADC;

	if (ngzmp_spi_adc_open)
		return 0;

	for (i=0; i<num_paths; i++)
	{
		sprintf(dev_name, "/dev/spidev%d.%d", 2, i);
		printf("Trying to open device %d name='%s'\n", i, dev_name);
		if ((ngzmp_spi_path_adc[i]=open(dev_name, O_RDWR)) < 0)
		{
			perror(dev_name);
			for (j=0; j<i; j++)
			{
				if (ngzmp_spi_path_adc[j] >= 0)
					close(ngzmp_spi_path_adc[j]);
			}
			return -1;
		}
		else 
			printf(" .... Path=%d\n", ngzmp_spi_path_adc[i]);
		if (ioctl(ngzmp_spi_path_adc[i], SPI_IOC_RD_MODE, &mode) < 0)
			perror ("ioctl: SPI_IOC_RD_MODE");
		if (ioctl(ngzmp_spi_path_adc[i], SPI_IOC_RD_LSB_FIRST, &lsb_first) < 0)
			perror ("ioctl: SPI_IOC_RD_MODE");
		if (ioctl(ngzmp_spi_path_adc[i], SPI_IOC_RD_MAX_SPEED_HZ, &max_speed) < 0)
			perror ("ioctl: SPI_IOC_RD_MAX_SPEED_HZ");
		printf("Initial settings: Device %s read: mode=%d, lsb_first=%d, max_speed=%d\n", dev_name, mode, lsb_first, max_speed);
		req_mode = SPI_MODE_0;
		mode =0;
		lsb_first =0;
		max_speed = 1600000;
		if (ioctl(ngzmp_spi_path_adc[i], SPI_IOC_WR_MODE, &req_mode) < 0)
			perror ("ioctl: SPI_IOC_WR_MODE");
		if (ioctl(ngzmp_spi_path_adc[i], SPI_IOC_WR_LSB_FIRST, &lsb_first) < 0)
			perror ("ioctl: SPI_IOC_WR_MODE");
		if (ioctl(ngzmp_spi_path_adc[i], SPI_IOC_WR_MAX_SPEED_HZ, &max_speed) < 0)
			perror ("ioctl: SPI_IOC_WE_MAX_SPEED_HZ");

		mode = 0;
		if (ioctl(ngzmp_spi_path_adc[i], SPI_IOC_RD_MODE, &mode) < 0)
			perror ("ioctl: SPI_IOC_RD_MODE");
		if (ioctl(ngzmp_spi_path_adc[i], SPI_IOC_RD_LSB_FIRST, &lsb_first) < 0)
			perror ("ioctl: SPI_IOC_RD_MODE");
		if (ioctl(ngzmp_spi_path_adc[i], SPI_IOC_RD_MAX_SPEED_HZ, &max_speed) < 0)
			perror ("ioctl: SPI_IOC_RD_MAX_SPEED_HZ");
		if (mode != req_mode)
			fprintf(stderr, "Cannot set SPI mode 0 on device %s, read mode=%d\n", dev_name, mode);
		if (lsb_first != 0)
			fprintf(stderr, "Cannot set SPI MSB first on device %s, read lsb_first=%d\n", dev_name, lsb_first);
		if (max_speed > 20000000)
			fprintf(stderr, "Cannot set SPI max speed on device %s, read max speed=%d\n", dev_name, max_speed);
		else
			printf("SPI device %s, max_speed = %d\n", dev_name, max_speed);
	}
	ngzmp_spi_adc_open = 1;
	return 0;
}

int ngzmp_spi_close_adc()
{
	int i;
	int num_paths=NGZMP_SPI_NUM_ADC;

	if (ngzmp_spi_adc_open == 0)
		return 0;
	for (i=0; i<num_paths; i++)
	{
		if (ngzmp_spi_path_adc[i] >= 0)
			close(ngzmp_spi_path_adc[i]);
		ngzmp_spi_path_adc[i] = -1;
	}
	ngzmp_spi_adc_open = 0;

	return 0;
}


int ngzmp_spi_open_clk()
{
	int i, j;
	char dev_name[40];
	u_int8_t mode, lsb_first, req_mode;
	u_int32_t max_speed;
	int num_paths=NGZMP_SPI_NUM_CLK;

	if (ngzmp_spi_clk_open)
		return 0;

	for (i=0; i<num_paths; i++)
	{
		sprintf(dev_name, "/dev/spidev%d.%d", 3, i);
		printf("Trying to open device %d name='%s'\n", i, dev_name);
		if ((ngzmp_spi_path_clk[i]=open(dev_name, O_RDWR)) < 0)
		{
			perror(dev_name);
			for (j=0; j<i; j++)
			{
				if (ngzmp_spi_path_clk[j] >= 0)
					close(ngzmp_spi_path_clk[j]);
			}
			return -1;
		}
		else 
			printf(" .... Path=%d\n", ngzmp_spi_path_clk[i]);
		if (ioctl(ngzmp_spi_path_clk[i], SPI_IOC_RD_MODE, &mode) < 0)
			perror ("ioctl: SPI_IOC_RD_MODE");
		if (ioctl(ngzmp_spi_path_clk[i], SPI_IOC_RD_LSB_FIRST, &lsb_first) < 0)
			perror ("ioctl: SPI_IOC_RD_MODE");
		if (ioctl(ngzmp_spi_path_clk[i], SPI_IOC_RD_MAX_SPEED_HZ, &max_speed) < 0)
			perror ("ioctl: SPI_IOC_RD_MAX_SPEED_HZ");
		printf("Initial settings: Device %s read: mode=%d, lsb_first=%d, max_speed=%d\n", dev_name, mode, lsb_first, max_speed);
		req_mode = SPI_MODE_0;
		mode =0;
		lsb_first =0;
		max_speed = 1600000;
		if (ioctl(ngzmp_spi_path_clk[i], SPI_IOC_WR_MODE, &req_mode) < 0)
			perror ("ioctl: SPI_IOC_WR_MODE");
		if (ioctl(ngzmp_spi_path_clk[i], SPI_IOC_WR_LSB_FIRST, &lsb_first) < 0)
			perror ("ioctl: SPI_IOC_WR_MODE");
		if (ioctl(ngzmp_spi_path_clk[i], SPI_IOC_WR_MAX_SPEED_HZ, &max_speed) < 0)
			perror ("ioctl: SPI_IOC_WE_MAX_SPEED_HZ");

		mode = 0;
		if (ioctl(ngzmp_spi_path_clk[i], SPI_IOC_RD_MODE, &mode) < 0)
			perror ("ioctl: SPI_IOC_RD_MODE");
		if (ioctl(ngzmp_spi_path_clk[i], SPI_IOC_RD_LSB_FIRST, &lsb_first) < 0)
			perror ("ioctl: SPI_IOC_RD_MODE");
		if (ioctl(ngzmp_spi_path_clk[i], SPI_IOC_RD_MAX_SPEED_HZ, &max_speed) < 0)
			perror ("ioctl: SPI_IOC_RD_MAX_SPEED_HZ");
		if (mode != req_mode)
			fprintf(stderr, "Cannot set SPI mode 0 on device %s, read mode=%d\n", dev_name, mode);
		if (lsb_first != 0)
			fprintf(stderr, "Cannot set SPI MSB first on device %s, read lsb_first=%d\n", dev_name, lsb_first);
		if (max_speed > 20000000)
			fprintf(stderr, "Cannot set SPI max speed on device %s, read max speed=%d\n", dev_name, max_speed);
		else
			printf("SPI device %s, max_speed = %d\n", dev_name, max_speed);
	}
	ngzmp_spi_clk_open = 1;
	return 0;
}

int ngzmp_spi_close_clk()
{
	int i;
	int num_paths=NGZMP_SPI_NUM_CLK;

	if (ngzmp_spi_clk_open == 0)
		return 0;
	for (i=0; i<num_paths; i++)
	{
		if (ngzmp_spi_path_clk[i] >= 0)
			close(ngzmp_spi_path_clk[i]);
		ngzmp_spi_path_clk[i] = -1;
	}
	ngzmp_spi_clk_open = 0;

	return 0;
}


/** write burst of 8 bit data to consequitive addresses in one ADC  

* @param path	NGPD path.
* @param card	Card number in system.
* @param adc	ADC number on board, 0 or 1.
* @param addr	Address of register within ADC	
* @param n		Number of bytes to write
* @param tx_data	Pointer to array of 8 bit ints to write into ADC.
*/

int ngzmp_spi_write_adc_glob(int path, int card, int adc, int addr, int n, u_int8_t * tx_data)
{
	struct spi_ioc_transfer xfer[2];
	u_int8_t tx_header[2];
	int first_adc, last_adc;

	if (ngzmp_spi_open_adc() < 0)
		return -1;

	memset(xfer, 0, sizeof (xfer));

	if (adc < 0)
	{
		first_adc = 0; 
		last_adc = 1;
	}
	else if (adc >= NGZMP_SPI_NUM_ADC)
	{
		fprintf(stderr, "ngzmp_spi_write_adc_glob: ERROR ADC %d  out of range 0...1\n", adc);
		return -1;
	}
	else
	{
		first_adc = adc;
		last_adc = adc;
	}
	tx_header[0] = (addr >>8) & 0x7F;
	tx_header[1] = (addr) & 0xFF;
	xfer[0].tx_buf = (u_int64_t)&tx_header;
	xfer[1].tx_buf = (u_int64_t)tx_data;

	xfer[0].len = 2;
	xfer[1].len = n;
	xfer[0].bits_per_word = 8;
	xfer[1].bits_per_word = 8;

	for (adc=first_adc; adc<=last_adc; adc++)
	{
		if (ioctl(ngzmp_spi_path_adc[adc], SPI_IOC_MESSAGE(2), xfer) < 0)
		{
			perror("ngzmp_spi_write_adc_glob : ioctl");
			return -1;
		}
	}
	return 0;
}

/** Read burst of 8 bit data from consequitive addresses in one ADC. Use to access  the global registers and wrapped for accesss to the pair and channel registers  

* @param path	NGPD path.
* @param card	Card number in system.
* @param adc	ADC number on board, 0 or 1.
* @param addr	Address of register within ADC	
* @param n		Number of bytes to write
* @param rx_data	Pointer to array of 8 bit ints for received data.
*/

int ngzmp_spi_read_adc_glob(int path, int card, int adc, int addr, int n, u_int8_t *rx_data)
{
	struct spi_ioc_transfer xfer[2];
	u_int8_t tx_header[2];

	if (ngzmp_spi_open_adc() < 0)
		return -1;

	memset(xfer, 0, sizeof (xfer));

	if (adc < 0 || adc >= NGZMP_SPI_NUM_ADC)
	{
		fprintf(stderr, "ngzmp_spi_read_adc_glob: ERROR ADC %d  out of range 0...1\n", adc);
		return -1;
	}
	tx_header[0] = ((addr >>8) & 0x7F) | 0x80;
	tx_header[1] = (addr) & 0xFF;
	xfer[0].tx_buf = (u_int64_t)&tx_header;
	xfer[1].rx_buf = (u_int64_t)rx_data;

	xfer[0].len = 2;
	xfer[1].len = n;
	xfer[0].bits_per_word = 8;
	xfer[1].bits_per_word = 8;

	if (ioctl(ngzmp_spi_path_adc[adc], SPI_IOC_MESSAGE(2), xfer) < 0)
	{
		perror("ngzmp_spi_read_adc_glob : ioctl");
		return -1;
	}
	return 0;
}


static int adc_pair_select[NGZMP_SPI_NUM_ADC];
static int adc_chan_select[NGZMP_SPI_NUM_ADC][2];


/** Write burst of 8 bit data to consequitive addresses in one or both ADCs. 

* @param path	NGPD path.
* @param card	Card number in system.
* @param adc	ADC number on board, 0 or 1 or -1 for both
* @param pair_sel	Bit mask to select which pair(s) of ADCs receive the operation. bit 0 => Pair AB, Bit 1=> Pair CD.
* @param chan_sel	Bit mask to select which individual ADC channel within pair receive the operation. bit 0 => Chan A/C, Bit 1=> chan C/D.
* @param addr	Address of register within ADC	
* @param n		Number of bytes to write
* @param tx_data	Pointer to array of 8 bit ints of data.
* @return		Return 0 on success or -1 on error.
*/


int ngzmp_spi_write_adc(int path, int card, int adc, int pair_sel, int chan_sel,  int addr, int n, u_int8_t *tx_data)
{
	int pair;
	int rc;
	u_int8_t data;
	int first_adc, last_adc;

	if (adc < 0)
	{
		first_adc = 0;
		last_adc = 1; 
	}
	else if (adc >= NGZMP_SPI_NUM_ADC)
	{
		fprintf(stderr, "ngzmp_spi_write_adc: ERROR ADC %d  out of range 0...1\n", adc);
		return -1;
	}
	else
	{
		first_adc = adc;
		last_adc = adc;
	}
	pair_sel &= 3;
	chan_sel &= 3;
	for (adc=first_adc; adc<=last_adc; adc++)
	{
		if (pair_sel)
		{
			if (pair_sel != adc_pair_select[adc])
			{
				data = (u_int8_t)pair_sel;
				rc = ngzmp_spi_write_adc_glob(path, card, adc, AD9694_SPI_GLOB_MAP_PAIR_INDEX, 1, &data);
				if (rc < 0)
					return rc;
				adc_pair_select[adc] = pair_sel;
			}
			if (chan_sel)
			{
				for (pair=0; pair<2; pair++)
				{
					if (pair_sel & 1 << pair)
					{
						if (adc_chan_select[adc][pair] != chan_sel)
						{
							data = (u_int8_t)chan_sel;
							rc = ngzmp_spi_write_adc_glob(path, card, adc, AD9694_SPI_PAIR_MAP_CHAN_INDEX, 1, &data);
							if (rc < 0)
								return rc;
							adc_chan_select[adc][pair] = chan_sel;
							break;	// Once written once it will update for both pairs if both enabled
						}
					}
				}
			}
		}
		rc= ngzmp_spi_write_adc_glob(path, card, adc, addr, n, tx_data);
		if (rc < 0)
			return -1;
	}
	return 0;
}

/** Read burst of 8 bit data from consequitive addresses in one ADC. Use to access  the global registers and wrapped for accesss to the pair and channel registers  

* @param path	NGPD path.
* @param card	Card number in system.
* @param adc	ADC number on board, 0 or 1.
* @param pair_sel	Bit mask to select which pair(s) of ADCs receive the operation. bit 0 => Pair AB, Bit 1=> Pair CD.
* @param chan_sel	Bit mask to select which individual ADC channel within pair receive the operation. bit 0 => Chan A/C, Bit 1=> chan C/D.
* @param addr	Address of register within ADC	
* @param n		Number of bytes to write
* @param rx_data	Pointer to array of 8 bit ints for received data.
* @return		Return 0 on success or -1 on error.
*/
int ngzmp_spi_read_adc(int path, int card, int adc, int pair_sel, int chan_sel,  int addr, int n, u_int8_t *rx_data)
{
	int pair;
	int rc;
	u_int8_t data;

	pair_sel &= 3;
	chan_sel &= 3;
	if (adc < 0 || adc >= NGZMP_SPI_NUM_ADC)
	{
		fprintf(stderr, "ngzmp_spi_read_adc: ERROR ADC %d  out of range 0...1\n", adc);
		return -1;
	}
	if (pair_sel)
	{
		if (pair_sel != adc_pair_select[adc])
		{
			data = (u_int8_t)pair_sel;
			rc = ngzmp_spi_write_adc_glob(path, card, adc, AD9694_SPI_GLOB_MAP_PAIR_INDEX, 1, &data);
			if (rc < 0)
				return rc;
			adc_pair_select[adc] = pair_sel;
		}
		if (chan_sel)
		{
			for (pair=0; pair<2; pair++)
			{
				if (pair_sel & 1 << pair)
				{
					if (adc_chan_select[adc][pair] != chan_sel)
					{
						data = (u_int8_t)chan_sel;
						rc = ngzmp_spi_write_adc_glob(path, card, adc, AD9694_SPI_PAIR_MAP_CHAN_INDEX, 1, &data);
						if (rc < 0)
							return rc;
						adc_chan_select[adc][pair] = chan_sel;
						break;	// Once written once it will update for both pairs if both enabled
					}
				}
			}
		}
	}
	return ngzmp_spi_read_adc_glob(path, card, adc, addr, n, rx_data);
}

/* 8 bit data is a combined with 16 bit of address to give 24 bit transactions */
int ngzmp_spi_write_clk(int path, int card, int addr, int n, u_int8_t *data)
{
	struct spi_ioc_transfer xfer[n];
	u_int8_t tx_buff[3*n];
	int i;

	if (ngzmp_spi_open_clk() < 0)
		return -1;

	memset(xfer, 0, sizeof (struct spi_ioc_transfer)*n);

	for (i=0; i<n; i++)
	{
		tx_buff[3*i] = (addr>>8)&0x1F;
		tx_buff[3*i+1] = (addr)&0xFF;
		tx_buff[3*i+2] = data[i];
		xfer[i].tx_buf = (u_int64_t)(tx_buff+3*i);
		xfer[i].len = 3;
		xfer[i].bits_per_word = 8;
		if (i == n-1)
			xfer[i].cs_change = 0;		// Seems to be differnt on the last transfer
		else
			xfer[i].cs_change = 1;		// Try to force CS to negate between transfers to the same device

/*  __u32     speed_hz;

  __u16     delay_usecs;
  __u32     pad;

*/ 
		addr++;
	}
	if (ioctl(ngzmp_spi_path_clk[0], SPI_IOC_MESSAGE(n), xfer) < 0)
	{
		perror("ngzmp_spi_write_clk : ioctl");
		return -1;
	}
	return 0;
} 

int ngzmp_spi_read_clk(int path, int card, int addr, int n, u_int8_t *data)
{
	struct spi_ioc_transfer xfer[n];
	u_int8_t tx_buff[3*n], rx_buff[3*n];
	int i;

	if (ngzmp_spi_open_clk() < 0)
		return -1;

	memset(xfer, 0, sizeof (struct spi_ioc_transfer)*n);

	for (i=0; i<n; i++)
	{
		tx_buff[3*i] = ((addr>>8)&0x1F) | 0x80;
		tx_buff[3*i+1] = (addr)&0xFF;
		tx_buff[3*i+2] = 0;
		xfer[i].tx_buf = (u_int64_t)(tx_buff+3*i);
		xfer[i].rx_buf = (u_int64_t)(rx_buff+3*i);
		xfer[i].len = 3;
		xfer[i].bits_per_word = 8;
		if (i == n-1)
			xfer[i].cs_change = 0;		// Seems to be differnt on the last transfer
		else
			xfer[i].cs_change = 1;		// Try to force CS to negate between transfers to the same device

/*  __u32     speed_hz;

  __u16     delay_usecs;
  __u32     pad;

*/ 
		addr++;
	}
	if (ioctl(ngzmp_spi_path_clk[0], SPI_IOC_MESSAGE(n), xfer) < 0)
	{
		perror("ngzmp_spi_read_clk : ioctl");
		return -1;
	}
	for (i=0; i<n; i++)
		data[i] = rx_buff[3*i+2];
	return 0;
} 

/** Write array of addrss data values pairs to one or both ADCs. 

* @param path	NGPD path.
* @param card	Card number in system. card <0 => write same data to all cards.
* @param adc	ADC number on board, 0 or 1 or -1 for both
* @param pair_sel	Bit mask to select which pair(s) of ADCs receive the operation. bit 0 => Pair AB, Bit 1=> Pair CD.
* @param chan_sel	Bit mask to select which individual ADC channel within pair receive the operation. bit 0 => Chan A/C, Bit 1=> chan C/D.
* @param addr_data	Pointer to array of structure type Address of register within ADC, terminated with entry with addr < 0 
* @return		Return 0 on success or -1 on error.
*/

int ngzmp_spi_write_adc_multiple(int path, int card, int adc, int pair_sel, int chan_sel,  NGZMPADCAddrData *addr_data)
{
	int rc;
	struct timespec delay;

	while (addr_data->addr >= 0)
	{				
		rc = ngzmp_spi_write_adc(path, card, adc, pair_sel, chan_sel, addr_data->addr, 1, &addr_data->data);
		if (rc < 0)
			return rc;

		if (addr_data->flags & NGZMPADC_AddDelay)
		{
			delay.tv_sec = 0;
			delay.tv_nsec=5000000;	// 5 ms delay
			while (nanosleep(&delay, &delay) <0 && errno == EINTR);
		}
		addr_data++;
	}
	return 0;
}
