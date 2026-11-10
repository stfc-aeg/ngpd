#if NGPD_CONF_ADQ14
#include "ADQAPI.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
// #include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#ifdef LINUX
#include <stdlib.h>
#endif


#include "ngpd.h"
#include "zynqmp_protocol.h"
#include "zynqmp_api.h"


/**
 * @ingroup fullcontrol
 * Write a series of values into the DGA gain registers.
 * @n@n
 * The addresses is the channel number.
 *
 * @param path a handle to the top level of the xspress3 system returned from {@link xsp3_config()}.
 * @param chan is the channel number of the first channel to write to.
 *        If chan==-1 the first array element is copied to all channels.
 * @param region number  identifying Offset DAC, Gain Switch or CPLD registers
 * @param size the size of the array of values to write, skipping channel to channel.
 * @param data an array of 16 bit integer values to write.
 * @return {@link #XSP3_OK XSP3_OK} or a negative error code.
 */
int ngzmp_spi_write_dga(int path, int chan, int num,  u_int16_t* data)
{
	int rc;
	int card, this_path, this_card, chan_of_card, this_num;
	int offset;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_write_dga: Invalid path=%d", path);
		return -1;
	}

	if (chan < 0)
	{
		for (card=0; card<NGPDPath[path].num_cards; card++)
		{
			if (NGPDPath[path].type == NGPDComposite)
			{
				this_path = NGPDPath[path].sub_path[card];
			}
			else
				this_path = path;

			if (NGPDPath[path].dummy_system & NGPDDummy_ADC)
			{
				for (chan_of_card=0; chan_of_card<NGZMP_CHANS_PER_CARD; chan_of_card++)
					NGPDPath[this_path].dummy_chan_dga[chan_of_card] = *data;
			}
			else
			{
				offset = -1;
				rc = zynqmp_access_write(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_SPI_DGA, ZYNQMP_WIDTH_WORD, offset, 1, 1, (u_int8_t *)data);
				if (rc < 0)
				{
					snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_write_dga: %s", zynqmp_get_error_message());
					return rc;
				}
			}
		}
		return 0;
	}
	else if (chan < 0 || chan >= NGPDPath[path].num_chan || num < 1 || chan+num > NGPDPath[path].num_chan)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_write_dga: ERROR chan %d for %d out of range 0...%d\n", chan, num, NGPDPath[path].num_chan);
		return -1;
	}
	while (num > 0)
	{
		this_path=path;
		if ((rc=ngpd_resolve_path_chan_card(path, chan, &this_path, &chan_of_card, &this_card)) < 0)
			return rc;
		if (NGPDPath[path].type == NGPDComposite)
			this_path = NGPDPath[path].sub_path[this_card];

		if (chan_of_card+num > NGPDPath[this_path].num_chan)
			this_num = NGPDPath[this_path].num_chan-chan_of_card;
		else
			this_num = num;
		offset = chan_of_card;
		if (NGPDPath[path].dummy_system & NGPDDummy_ADC)
		{
			int i;
			for (i=0; i<this_num; i++)
				NGPDPath[this_path].dummy_chan_dga[chan_of_card+i] = *data++; 
		}
		else
		{
			rc = zynqmp_access_write(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_SPI_DGA, ZYNQMP_WIDTH_WORD, offset, this_num, 1, (u_int8_t *)data);
			if (rc < 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_write_dga: %s", zynqmp_get_error_message());
				return rc;
			}
		}
		num -= this_num;
		chan += this_num;
		data += this_num;
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
 * @param data an array of 16 bit integer values to read.
 * @return {@link #XSP3_OK XSP3_OK} or a negative error code.
 */
int ngzmp_spi_read_dga(int path, int chan, int num,  u_int16_t* data)
{
	int rc;
	int this_path, this_card, chan_of_card, this_num;
	int offset;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_read_dga: Invalid path=%d", path);
		return -1;
	}

	if (chan < 0 || chan >= NGPDPath[path].num_chan || num < 1 || chan+num > NGPDPath[path].num_chan)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_read_dga: ERROR chan %d for %d out of range 0...%d\n", chan, num, NGPDPath[path].num_chan);
		return -1;
	}
	while (num > 0)
	{
		this_path=path;
		if ((rc=ngpd_resolve_path_chan_card(path, chan, &this_path, &chan_of_card, &this_card)) < 0)
			return rc;
		if (NGPDPath[path].type == NGPDComposite)
			this_path = NGPDPath[path].sub_path[this_card];

		if (chan_of_card+num > NGPDPath[this_path].num_chan)
			this_num = NGPDPath[this_path].num_chan-chan_of_card;
		else
			this_num = num;
		offset = chan_of_card;
		if (NGPDPath[path].dummy_system & NGPDDummy_ADC)
		{
			int i;
			for (i=0; i<this_num; i++)
				*data++ = NGPDPath[this_path].dummy_chan_dga[chan_of_card+i];
		}
		else
		{
			rc = zynqmp_access_read(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_SPI_DGA, ZYNQMP_WIDTH_WORD, offset, this_num, 1, (u_int8_t *)data);
			if (rc < 0)
			{
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_read_dga: %s", zynqmp_get_error_message());
				return rc;
			}
		}
		num -= this_num;
		chan += this_num;
		data += this_num;
	}
	return 0;
}


/** Write burst of 8 bit data to consequitive addresses in one or both ADCs. 

* @param path	NGPD path.
* @param card	Card number in system. card <0 => write same data to all cards.
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
	int rc;
	int offset;	
	int this_path = path;
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_write_adc: Invalid path=%d", path);
		return -1;
	}

	if (NGPDPath[path].dummy_system & NGPDDummy_ADC)
		return 0;

	if (card < 0)
	{
		for (card=0; card<NGPDPath[path].num_cards; card++)
		{
			if ((rc=ngzmp_spi_write_adc(path, card, adc, pair_sel, chan_sel, addr, n, tx_data)) < 0)
				return rc;
		}
		return 0;
	}
	if (card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_write_adc: card=%d is out of range 0..%d", path, NGPDPath[path].num_cards-1);
		return -1;
	}
	if (NGPDPath[path].type == NGPDComposite)
		this_path = NGPDPath[path].sub_path[card];
	
	offset = NGZMP_SPI_ADC_ADDR_SET(addr) | NGZMP_SPI_ADC_ADC_SET(adc) | NGZMP_SPI_ADC_PAIR_SET(pair_sel) | NGZMP_SPI_ADC_CHAN_SET(chan_sel);
	rc = zynqmp_access_write(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_SPI_ADC, ZYNQMP_WIDTH_BYTE, offset, n, 1, tx_data);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_write_adc: %s", zynqmp_get_error_message());
		return rc;
	}
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

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_write_adc: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_ADC)
		return 0;
	while (addr_data->addr >= 0)
	{
		rc = ngzmp_spi_write_adc(path, card, adc, pair_sel, chan_sel, addr_data->addr, 1, &addr_data->data);
		if (rc < 0)
			return rc;
		if (addr_data->flags & NGZMPADC_AddDelay)
		{
#ifdef _MSC_VER
			Sleep(5);
#else
			delay.tv_sec = 0;
			delay.tv_nsec=5000000;	// 5 ms delay
			while (nanosleep(&delay, &delay) <0 && errno == EINTR);
#endif
		}
		addr_data++;
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
	int rc;
	int offset;	
	int this_path = path;
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_read_adc: Invalid path=%d", path);
		return -1;
	}

	if (card < 0 || card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_read_adc: card=%d is out of range 0..%d", path, NGPDPath[path].num_cards-1);
		return -1;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_ADC)
		return 0;

	if (NGPDPath[path].type == NGPDComposite)
		this_path = NGPDPath[path].sub_path[card];
	
	offset = NGZMP_SPI_ADC_ADDR_SET(addr) | NGZMP_SPI_ADC_ADC_SET(adc) | NGZMP_SPI_ADC_PAIR_SET(pair_sel) | NGZMP_SPI_ADC_CHAN_SET(chan_sel);
	rc = zynqmp_access_read(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_SPI_ADC, ZYNQMP_WIDTH_BYTE, offset, n, 1, rx_data);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_read_adc: %s", zynqmp_get_error_message());
		return rc;
	}
	return 0;
}


int ngzmp_spi_write_clk(int path, int card, int addr, int n, u_int8_t *data)
{
	int rc;
	int offset;	
	int this_path = path;
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_write_clk: Invalid path=%d", path);
		return -1;
	}

	if (NGPDPath[path].dummy_system & NGPDDummy_ADC)
		return 0;

	if (card < 0)
	{
		for (card=0; card<NGPDPath[path].num_cards; card++)
		{
			if ((rc=ngzmp_spi_write_clk(path, card, addr, n, data)) < 0)
				return rc;
		}
		return 0;
	}
	if (card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_write_clk: card=%d is out of range 0..%d", path, NGPDPath[path].num_cards-1);
		return -1;
	}
	if (NGPDPath[path].type == NGPDComposite)
		this_path = NGPDPath[path].sub_path[card];
	
	offset = addr;
	rc = zynqmp_access_write(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_SPI_CLK, ZYNQMP_WIDTH_BYTE, offset, n, 1, data);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_write_clk: %s", zynqmp_get_error_message());
		return rc;
	}
	return 0;
}

int ngzmp_spi_read_clk(int path, int card, int addr, int n, u_int8_t *data)
{
	int rc;
	int offset;	
	int this_path = path;
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_read_clk: Invalid path=%d", path);
		return -1;
	}

	if (NGPDPath[path].dummy_system & NGPDDummy_ADC)
		return 0;

	if (card < 0  || card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_read_clk: card=%d is out of range 0..%d", path, NGPDPath[path].num_cards-1);
		return -1;
	}
	if (NGPDPath[path].type == NGPDComposite)
		this_path = NGPDPath[path].sub_path[card];
	
	offset = addr;
	rc = zynqmp_access_read(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_SPI_CLK, ZYNQMP_WIDTH_BYTE, offset, n, 1, data);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_spi_read_clk: %s", zynqmp_get_error_message());
		return rc;
	}
	return 0;
}
