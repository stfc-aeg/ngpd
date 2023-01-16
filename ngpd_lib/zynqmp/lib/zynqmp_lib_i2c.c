#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/i2c-dev.h>
#include <pthread.h>

#include "smbus.h"

#include <string.h>
#include "ngpd.h"

#define NUM_I2C 6

extern char ngpd_error_message[NGPD_MAX_ERROR_MESSAGE + 2];

int ngzmp_i2c_path[NUM_I2C]= {-1, -1, -1, -1, -1};
int ngzmp_i2c_paths_open=0;
int pca9546_bus=-1;

pthread_mutex_t i2c_mutex[NUM_I2C];

int ngzmp_i2c_open()
{
	int bus_num;
	char dev_name[100];
	int rc = 0;
	
	if (ngzmp_i2c_paths_open)
		return 0;
	
	for (bus_num=0; bus_num<NUM_I2C; bus_num++)
	{
		sprintf(dev_name, "/dev/i2c-%d", bus_num);
		if ((ngzmp_i2c_path[bus_num]=open(dev_name, O_RDWR)) < 0)
		{
			perror(dev_name);
			rc = -1;
		}
		pthread_mutex_init(i2c_mutex+bus_num, NULL);
	}
	ngzmp_i2c_paths_open = 1;
	return rc;
}

int ngzmp_i2c_close()
{
	int i;
	if (ngzmp_i2c_paths_open==0)
		return 0;
	for (i=0; i<NUM_I2C; i++)
	{
		if (ngzmp_i2c_path[i] >= 0)
		{
			close(ngzmp_i2c_path[i]);
			ngzmp_i2c_path[i] = -1;
			pthread_mutex_destroy(i2c_mutex+i);
		}
	}

	ngzmp_i2c_paths_open = 0;
	return 0;
}

static int ngzmp_set_pca9546(int bus, int new_pca9546_bus)
{
	int rc;
	if (new_pca9546_bus != -1 && new_pca9546_bus != pca9546_bus)
	{
		u_int8_t cr = NGZMP_I2C_PCA9546_CR(new_pca9546_bus);
		if (ioctl(ngzmp_i2c_path[bus], I2C_SLAVE, NGZMP_I2C_ADDR_SFP_SWITCH) < 0)
	  	{
			perror("ERROR accessing SFP I2C bus PCA9546");
			pthread_mutex_unlock(i2c_mutex+bus);
			return -1;
	  	}
		rc = write(ngzmp_i2c_path[bus], &cr, 1);
		if (rc != 1)
		{
			perror ("ngzmp_set_pca9546: Writing to SFP I2C bus PCA9546 ");
		}
		pca9546_bus = new_pca9546_bus;
	}
	return 0;
}

/**
 * @ingroup internal
 * Write a series of values into the an I2C bus device registers for all or each specified card.
 * @n@n
 * The addresses are defined in {@link NGZMP_I2C_REGISTERS}
 *
 * @param path a handle to the top level of the ngpd system returned from {@link ngpg_config_ngzmp()}.
 * @param card matches the card number if this function is provided at the X86_64 end
 *        0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system, or -1 to copy to all cards.
 * @param bus	The I2C Bus number.
 * @param addr Is the I2C address on this bus
 * @param size the size of the array of values to write, skipping channel to channel.
 * @param value an array of 8 bit integer values to write.
 * @return 0 or a negative error code.
 */
int ngzmp_i2c_write_reg(int path, int card, int bus, int addr, int size, u_int8_t* value)
{
	int rc;
	int new_pca9546_bus=-1;

	if (ngzmp_i2c_open() < 0)
		return -1;

	if (bus >= NGZMP_I2C_BUS_SFP_A && bus <= NGZMP_I2C_BUS_SFP_PEXP)
	{
		new_pca9546_bus = bus - NGZMP_I2C_BUS_SFP_A;
		bus = NGZMP_I2C_BUS_SFP;
	}

	if (bus < 0 || bus >= NUM_I2C || ngzmp_i2c_path[bus] < 0)
	{
		printf("ngzmp_i2c_write_reg: bad I2C bus number %d\n", bus);
		return -1;
	}
	pthread_mutex_lock(i2c_mutex+bus);

	if (new_pca9546_bus != -1 && ngzmp_set_pca9546(bus, new_pca9546_bus) < 0)
		return -1;

	if (ioctl(ngzmp_i2c_path[bus], I2C_SLAVE, addr) < 0)
  	{
		perror("ngzmp_i2c_write_reg: ERROR accessing I2C bus");
		pthread_mutex_unlock(i2c_mutex+bus);
		return -1;
  	}
	rc = write(ngzmp_i2c_path[bus], value, size);
	if (rc != size)
	{
		perror ("ngzmp_i2c_write_reg: Writing to I2C bus");
	}
	pthread_mutex_unlock(i2c_mutex+bus);
	return rc;

}


/**
 * @ingroup internal
 * Read a series of values from an I2C bus device registers for specified card.
 * @n@n
 * The addresses are defined in {@link NGZMP_I2C_REGISTERS}
 *
 * @param path a handle to the top level of the ngpd system returned from {@link ngpg_config_ngzmp()}.
 * @param card matches the card number if this function is provided at the X86_64 end
 *        0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 * @param bus	The I2C Bus number.
 * @param addr Is the I2C address on this bus
 * @param size the size of the array of values to write, skipping channel to channel.
 * @param value an array of 8 bit integer values to write.
 * @return {@link #XSP3_OK XSP3_OK} or a negative error code.
 */
int ngzmp_i2c_read_reg(int path, int card, int bus, int addr, int size, u_int8_t* value)
{
	int rc;
	int new_pca9546_bus=-1;

	if (ngzmp_i2c_open() < 0)
		return -1;

	if (bus >= NGZMP_I2C_BUS_SFP_A && bus <= NGZMP_I2C_BUS_SFP_PEXP)
	{
		new_pca9546_bus = bus - NGZMP_I2C_BUS_SFP_A;
		bus = NGZMP_I2C_BUS_SFP;
	}

	if (bus < 0 || bus >= NUM_I2C || ngzmp_i2c_path[bus] < 0)
	{
		printf("ngzmp_i2c_read_reg: bad I2C bus number %d\n", bus);
		return -1;
	}
	pthread_mutex_lock(i2c_mutex+bus);

	if (new_pca9546_bus != -1 && ngzmp_set_pca9546(bus, new_pca9546_bus) < 0)
		return -1;

	if (ioctl(ngzmp_i2c_path[bus], I2C_SLAVE, addr) < 0)
  	{
		perror("ngzmp_i2c_read_reg: ERROR accessing ADC I2C bus");
		pthread_mutex_unlock(i2c_mutex+bus);
		return -1;
  	}
	rc = read(ngzmp_i2c_path[bus], value, size);
	if (rc != size)
	{
		fprintf(stderr, "ngzmp_i2c_read_reg: bus=%d, ngzmp_i2c_path[bus]=%d, addr=%d\n", bus, ngzmp_i2c_path[bus], addr);
		perror ("Reading From I2C bus");
	}
	pthread_mutex_unlock(i2c_mutex+bus);
	return rc;

}


/**
 * @ingroup internal
 * Write a series of values into the ADC board I2C bus device registers for all or each specified card sending the register address as the command byte.
 * @n@n
 * The addresses are defined in {@link XSP3_I2C_REGISTERS}
 *
 * @param path a handle to the top level of the ngpd system returned from {@link ngpg_config_ngzmp()}.
 * @param card matches the card number if this function is provided at the X86_64 end
 *        0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *		  -1 to copy data top all cards (x86_64)
 * @param bus		The I2C Bus number.
 * @param addr 		The I2C slave address on this bus
 * @param reg_addr 	The register address within the device sent as the command
 * @param size 		The size of the array of values to write, skipping channel to channel.
 * @param value 	Pointer to an array of 8 bit integer values to write.
 * @return 0 or a negative error code.
 */
int ngzmp_i2c_write_reg_addr(int path, int card, int bus, int addr, int reg_addr, int size, u_int8_t* value)
{
	int rc;
	int new_pca9546_bus=-1;

	if (ngzmp_i2c_open() < 0)
		return -1;

	if (bus >= NGZMP_I2C_BUS_SFP_A && bus <= NGZMP_I2C_BUS_SFP_PEXP)
	{
		new_pca9546_bus = bus - NGZMP_I2C_BUS_SFP_A;
		bus = NGZMP_I2C_BUS_SFP;
	}

	if (bus < 0 || bus >= NUM_I2C || ngzmp_i2c_path[bus] < 0)
	{
		printf("ngzmp_i2c_write_reg_addr: bad I2C bus number %d\n", bus);
		return -1;
	}
	pthread_mutex_lock(i2c_mutex+bus);

	if (new_pca9546_bus != -1 && ngzmp_set_pca9546(bus, new_pca9546_bus) < 0)
		return -1;

	if (ioctl(ngzmp_i2c_path[bus], I2C_SLAVE, addr) < 0)
  	{
		perror("ngzmp_i2c_write_reg_addr : ERROR accessing ADC I2C bus");
		pthread_mutex_unlock(i2c_mutex+bus);
		return -1;
  	}
	rc = i2c_smbus_write_i2c_block_data(ngzmp_i2c_path[bus], reg_addr, size, value); 
	if (rc < 0)
	{
		printf("Error writing to I2C bus %d, path=%d, reg_addr=0x%02X\n", bus, ngzmp_i2c_path[bus], reg_addr);
		perror ("ngzmp_i2c_write_reg_addr: Writing to I2C bus");
		pthread_mutex_unlock(i2c_mutex+bus);
		return -1;
	}
	pthread_mutex_unlock(i2c_mutex+bus);
	return size;

}


/**
 * @ingroup internal
 * Read a series of values from the ADC board I2C bus device registers for specified card sending the register address as the command byte.
 * @n@n
 * The addresses are defined in {@link NGZMP_I2C_REGISTERS}
 *
 * @param path 		The handle to the top level of the ngpd system returned from {@link ngpg_config_ngzmp()}.
 * @param card 		Matches the card number if this function is provided at the X86_64 end
 *        			0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 * @param bus		The I2C Bus number.
 * @param addr 		The I2C slave address on this bus
 * @param reg_addr 	The register address within the device sent as the command
 * @param size 		The size of the array of values to write, skipping channel to channel.
 * @param value 	Pointer to an array of 8 bit integer values to receive data.
 * @return 0 or a negative error code.
 */
int ngzmp_i2c_read_reg_addr(int path, int card, int bus, int addr, int reg_addr, int size, u_int8_t* value)
{
	int rc;
	int new_pca9546_bus=-1;

	if (ngzmp_i2c_open() < 0)
		return -1;

	if (bus >= NGZMP_I2C_BUS_SFP_A && bus <= NGZMP_I2C_BUS_SFP_PEXP)
	{
		new_pca9546_bus = bus - NGZMP_I2C_BUS_SFP_A;
		bus = NGZMP_I2C_BUS_SFP;
	}

	if (bus < 0 || bus >= NUM_I2C || ngzmp_i2c_path[bus] < 0)
	{
		printf("ngzmp_i2c_read_reg_addr: bad I2C bus number %d\n", bus);
		return -1;
	}
	pthread_mutex_lock(i2c_mutex+bus);

	if (new_pca9546_bus != -1 && ngzmp_set_pca9546(bus, new_pca9546_bus) < 0)
		return -1;

/*	printf("Press return to Call iotcl I2C_SLAVE, path=%d, slave=0x%02X\n", ngzmp_i2c_path[bus], addr);
	getchar(); */
	if (ioctl(ngzmp_i2c_path[bus], I2C_SLAVE, addr) < 0)
  	{
		perror("ngzmp_i2c_read_reg_addr: ERROR accessing ADC I2C bus");
		pthread_mutex_unlock(i2c_mutex+bus);
		return -1;
  	}
/*	printf("Press return to Call i2c_smbus_read_i2c_block_data, path=%d, reg_addr=0x%02X, size=%d\n", ngzmp_i2c_path[bus], reg_addr, size);
	getchar(); */
	rc = i2c_smbus_read_i2c_block_data(ngzmp_i2c_path[bus], reg_addr, size, value);
	if (rc != size)
	{
		fprintf(stderr, "ngzmp_i2c_read_reg_addr: bus=%d, ngzmp_i2c_path[%d]=%d, addr=%d, reg_addr=%d => rc=%d\n", bus, bus, ngzmp_i2c_path[bus], addr, reg_addr, rc);
		perror ("Reading From I2C bus");
	}
	pthread_mutex_unlock(i2c_mutex+bus);
	return rc;
}
int ngpd_i2c_read_adc_temp(int path, int card, float *temp, int *status) 
{
	return ngpd_i2c_read_temp(path, card, temp, status, NGZMP_I2C_BUS_PMBUS, NGZMP_I2C_NUM_ADT7410_ADC, "adc");
}
int ngpd_i2c_read_preamp_temp(int path, int card, float *temp, int *status) 
{
	return ngpd_i2c_read_temp(path, card, temp, status, NGZMP_I2C_BUS_PREAMP, NGZMP_I2C_NUM_ADT7410_PREAMP, "preamp");
}
int ngpd_i2c_read_temp(int path, int card, float *temp, int *status, int bus, int num_adt7410, char *name) 
{
	int rc;
	int i;
	u_int8_t buf[2];
	int16_t raw_temp;
	
	for (i=0; i<num_adt7410; i++)
	{
		u_int32_t addr;
		addr = NGZMP_I2C_ADDR_ADT7410(i);

		rc = ngzmp_i2c_read_reg_addr(path, card, bus, addr, ADT7410_TEMP_MSB, 2, buf);
		if (rc == 2)
		{
			raw_temp = (buf[0] << 8) | (buf[1] & 0xF8);
			*temp++ = raw_temp / 128.0;
			*status ++ = buf[1] & 0x7;
		}
		else
		{
			*temp ++ = -100.0;
			*status++ = 0;
		}
	}
	return rc;
}
int ngpd_i2c_write_adc_tcrit(int path, int card, int chip, int tcrit) 
{
	return ngpd_i2c_write_tcrit(path, card, chip, tcrit, NGZMP_I2C_BUS_PMBUS, NGZMP_I2C_NUM_ADT7410_ADC, "adc"); 
}
int ngpd_i2c_write_preamp_tcrit(int path, int card, int chip, int tcrit) 
{
	return ngpd_i2c_write_tcrit(path, card, chip, tcrit, NGZMP_I2C_BUS_PREAMP, NGZMP_I2C_NUM_ADT7410_PREAMP, "preamp"); 
}
int ngpd_i2c_write_tcrit(int path, int card, int chip, int tcrit, int bus, int num_adt7410, char * name) 
{
	int rc;
	int i;
	u_int8_t buf[2];
	int first, last;
	
	tcrit *=  128;
	buf[0] = tcrit >> 8;
	buf[1] = tcrit & 0xFF;

	if (chip < 0)
	{
		first = 0;
		last = num_adt7410-1;
	}
	else if (chip >= num_adt7410)
	{
		printf("ngpd_i2c_write_tcrit: Chip=%d out or range 0..%d\n", chip, num_adt7410-1);
		return -1;
	}
	else
	{
		first = last = chip;
	}

	for (i=first; i<=last; i++)
	{
		u_int32_t addr;
		u_int8_t csr=0x10;		// Set comparator mode fro ADT 7410
		addr = NGZMP_I2C_ADDR_ADT7410(i);
	
		rc = ngzmp_i2c_write_reg_addr(path, card, bus, addr, ADT7410_CONFIG, 1, &csr);
		rc = ngzmp_i2c_write_reg_addr(path, card, bus, addr, ADT7410_TCRIT_MSB, 2, buf);
		if (rc < 0)
			break;
	}
	return rc;
}

/**
 * @ingroup fullcontrol
 * Write offset DAC on the preamp card.
 *
 * @param path 		A handle to the top level of the NGPD system returned from {@link ngpd_config_ngzmp()}.
 * @param chan		Channel number within the system of first data value.
 * @param num		Number of channels to write. num =-1 copies the first data point to all channels. 
 * @param data		Pointer to u_int16_t data values to write
 * @return 			Returns 0 on success or negative error message
 */

int ngpd_i2c_write_preamp_offset(int path, int chan, int num, u_int16_t *data)
{
	int rc;
	int copy_data=0;
	u_int8_t buf[2];
	int chan_of_card, card;
	int c;
	
	if (chan == 0 && num < 0)
	{
		copy_data = 1;
		num = NGZMP_CHANS_PER_CARD;
	}
	else if (chan < 0 || chan >= NGZMP_CHANS_PER_CARD || num <1 || chan+num > NGZMP_CHANS_PER_CARD)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_preamp_offset: Channel %d and num %d is out of range 0...%d", chan, num, NGZMP_CHANS_PER_CARD-1);
		return -1;
	}
	for (c=0; c<num; c++)
	{
		buf[0] = *data >> 8;
		buf[1] = *data & 0xFF;
		if (!copy_data)
			data++;
		card = 0;
		chan_of_card = chan+c;
		rc = ngzmp_i2c_write_reg_addr(path, card, NGZMP_I2C_BUS_PREAMP, NGZMP_I2C_ADDR_OFFSET_DAC, AD5675_CMD_WRITE_UPDATE(chan_of_card), 2, buf);
		if (rc < 0)
		{
			return rc;
		}
	}
	return rc;
}

/**
 * @ingroup fullcontrol
 * Read offset DAC on the preamp card.
 *
 * @param path 		A handle to the top level of the NGPD system returned from {@link ngpd_config_ngzmp()}.
 * @param chan		Channel number within the system of irst data value.
 * @param num		Number of channels to read. 
 * @param data		Pointer to u_int16_t to return data values read.
 * @return 			Returns 0 on success or negative error message
 */

int ngpd_i2c_read_preamp_offset(int path, int chan, int num, u_int16_t *data)
{
	int rc;
	u_int8_t buf[2];
	int chan_of_card, card=0;
	int c;
	
	if (chan < 0 || chan >= NGZMP_CHANS_PER_CARD || num <1 || chan+num >NGZMP_CHANS_PER_CARD)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_read_preamp_offset: Channel %d and num %d is out of range 0...%d", chan, num, NGZMP_CHANS_PER_CARD-1);
		return -1;
	}
	for (c=0; c<num; c++)
	{
		card = 0;
		chan_of_card = chan+c;
		rc = ngzmp_i2c_read_reg_addr_padded(path, card, NGZMP_I2C_BUS_PREAMP, NGZMP_I2C_ADDR_OFFSET_DAC, AD5675_CMD_READ(chan_of_card), 3, 2, buf);
/*		rc = ngzmp_i2c_read_reg_addr(path, card, NGZMP_I2C_BUS_PREAMP, NGZMP_I2C_ADDR_OFFSET_DAC, AD5675_CMD_READ(chan_of_card), 2, buf); */
		if (rc < 0)
		{
			char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
			strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
			old_msg[NGPD_MAX_ERROR_MESSAGE] = 0;
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_read_preamp_offset: %s", old_msg);
			return rc;
		}
		*data ++ = buf[0] << 8 | buf[1];
	}
	return num;
}

/**
 * @ingroup internal
 * Read a series of values from the ADC board I2C bus device registers for all or each specified card sending the register address as the command byte.
 * Special case where the address/command is longer than 1 byte aimed at AD5675.
 * @n@n
 * The addresses are defined in {@link NGZMP_I2C_REGISTERS}
 *
 * @param path 		The handle to the top level of the ngpd system returned from {@link ngpg_config_ngzmp()}.
 * @param card 		Matches the card number if this function is provided at the X86_64 end
 *        			0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 * @param bus		The I2C Bus number.
 * @param addr 		The I2C slave address on this bus
 * @param reg_addr 	The register address within the device sent as the command
 * @param size 		The size of the array of values to write, skipping channel to channel.
 * @param value 	Pointer to an array of 8 bit integer values to receive data.
 * @return 0 or a negative error code.
 */
int ngzmp_i2c_read_reg_addr_padded(int path, int card, int bus, int addr, int reg_addr, int reg_addr_size, int size, u_int8_t* value)
{
	int rc;
	int new_pca9546_bus=-1;
	struct i2c_rdwr_ioctl_data queue;
	struct i2c_msg msgs[2];
	u_int8_t padded_addr[4];
	int i;
	
	if (ngzmp_i2c_open() < 0)
		return -1;

	if (bus >= NGZMP_I2C_BUS_SFP_A && bus <= NGZMP_I2C_BUS_SFP_PEXP)
	{
		new_pca9546_bus = bus - NGZMP_I2C_BUS_SFP_A;
		bus = NGZMP_I2C_BUS_SFP;
	}

	if (bus < 0 || bus >= NUM_I2C || ngzmp_i2c_path[bus] < 0)
	{
		printf("ngzmp_i2c_read_reg_addr: bad I2C bus number %d\n", bus);
		return -1;
	}
	if (reg_addr_size > 4)
	{
		fprintf(stderr, "ngzmp_i2c_read_reg_addr_padded: padded regsietr address/command size=%d > 4\n", reg_addr_size);
		return -1;
	}
	for (i=0; i < 4; i++)
		padded_addr[i] = (reg_addr >> (8*i)) & 0xFF;
	
	queue.nmsgs = 2;
	queue. msgs = msgs;
	
	msgs[0].addr = addr;
	msgs[0].len = reg_addr_size;
	msgs[0].flags = 0;
	msgs[0].buf = padded_addr;

	msgs[1].addr = addr;
	msgs[1].len = size;
	msgs[1].flags = I2C_M_RD;
	msgs[1].buf = value;
	pthread_mutex_lock(i2c_mutex+bus);

	if (new_pca9546_bus != -1 && ngzmp_set_pca9546(bus, new_pca9546_bus) < 0)
		return -1;

/*	printf("Press return to Call iotcl I2C_SLAVE, path=%d, slave=0x%02X\n", ngzmp_i2c_path[bus], addr);
	getchar(); */
	if ((rc=ioctl(ngzmp_i2c_path[bus], I2C_RDWR, &queue)) < 0)
  	{
		fprintf(stderr, "ngzmp_i2c_read_reg_addr: bus=%d, ngzmp_i2c_path[%d]=%d, addr=%d, reg_addr=%d => rc=%d\n", bus, bus, ngzmp_i2c_path[bus], addr, reg_addr, rc);
		perror("ngzmp_i2c_read_reg_addr_padded: ERROR reading ADC I2C bus");
		pthread_mutex_unlock(i2c_mutex+bus);
		return -1;
  	}
	pthread_mutex_unlock(i2c_mutex+bus);
	return size;
}


/**
 * @ingroup internal
 * Write a series of values into the ADC board SM bus device registers on the specified page for all or each specified card sending the register address as the command byte.
 * @n@n
 * The addresses are defined in {@link XSP3_I2C_REGISTERS}
 *
 * @param path a handle to the top level of the ngpd system returned from {@link ngpg_config_ngzmp()}.
 * @param card matches the card number if this function is provided at the X86_64 end
 *        0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *		  -1 to copy data top all cards (x86_64)
 * @param bus		The I2C Bus number.
 * @param page		The SMbus page number (rail 1...n)
 * @param addr 		The I2C slave address on this bus
 * @param command 	The register address within the device sent as the command
 * @param size 		The size of the array of values to write.
 * @param value 	Pointer to an array of 8 bit integer values to write.
 * @return 0 or a negative error code.
 */
int ngzmp_i2c_write_reg_addr_page(int path, int card, int bus, int page, int addr, int command, int size, u_int8_t* value)
{
	int rc;
	int new_pca9546_bus=-1;
	u_int8_t page8 = (u_int8_t)page;

	if (ngzmp_i2c_open() < 0)
		return -1;

	if (bus >= NGZMP_I2C_BUS_SFP_A && bus <= NGZMP_I2C_BUS_SFP_PEXP)
	{
		new_pca9546_bus = bus - NGZMP_I2C_BUS_SFP_A;
		bus = NGZMP_I2C_BUS_SFP;
	}

	if (bus < 0 || bus >= NUM_I2C || ngzmp_i2c_path[bus] < 0)
	{
		printf("ngzmp_i2c_write_reg_addr: bad I2C bus number %d\n", bus);
		return -1;
	}
	pthread_mutex_lock(i2c_mutex+bus);

	if (new_pca9546_bus != -1 && ngzmp_set_pca9546(bus, new_pca9546_bus) < 0)
		return -1;

	if (ioctl(ngzmp_i2c_path[bus], I2C_SLAVE, addr) < 0)
  	{
		perror("ngzmp_i2c_write_reg_addr : ERROR accessing ADC I2C bus");
		pthread_mutex_unlock(i2c_mutex+bus);
		return -1;
  	}
	rc = i2c_smbus_write_i2c_block_data(ngzmp_i2c_path[bus], 0, 1, &page8); 
	if (rc < 0)
	{
		perror ("ngzmp_i2c_write_reg_addr_page: Writing page to I2C bus");
		pthread_mutex_unlock(i2c_mutex+bus);
		return -1;
	}

	rc = i2c_smbus_write_i2c_block_data(ngzmp_i2c_path[bus], command, size, value); 
	if (rc < 0)
	{
		perror ("ngzmp_i2c_write_reg_addr_page: Writing to I2C bus");
		pthread_mutex_unlock(i2c_mutex+bus);
		return -1;
	}
	pthread_mutex_unlock(i2c_mutex+bus);
	return 0;

}


/**
 * @ingroup internal
 * Read a series of values from the ADC board SM bus device registers for specified page and specified card sending the register address as the command byte.
 * @n@n
 * The addresses are defined in {@link NGZMP_I2C_REGISTERS}
 *
 * @param path 		The handle to the top level of the ngpd system returned from {@link ngpg_config_ngzmp()}.
 * @param card 		Matches the card number if this function is provided at the X86_64 end
 *        			0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 * @param bus		The I2C Bus number.
 * @param page		The SMbus page number 0...n-1
 * @param addr 		The I2C slave address on this bus
 * @param command 	The register address within the device sent as the command
 * @param size 		The size of the array of values to write, skipping channel to channel.
 * @param value 	Pointer to an array of 8 bit integer values to receive data.
 * @return 0 or a negative error code.
 */
int ngzmp_i2c_read_reg_addr_page(int path, int card, int bus, int page, int addr, int command, int size, u_int8_t* value)
{
	int rc;
	int new_pca9546_bus=-1;
	u_int8_t page8 = (u_int8_t)page;

	if (ngzmp_i2c_open() < 0)
		return -1;

	if (bus >= NGZMP_I2C_BUS_SFP_A && bus <= NGZMP_I2C_BUS_SFP_PEXP)
	{
		new_pca9546_bus = bus - NGZMP_I2C_BUS_SFP_A;
		bus = NGZMP_I2C_BUS_SFP;
	}

	if (bus < 0 || bus >= NUM_I2C || ngzmp_i2c_path[bus] < 0)
	{
		printf("ngzmp_i2c_read_reg_addr: bad I2C bus number %d\n", bus);
		return -1;
	}
	pthread_mutex_lock(i2c_mutex+bus);

	if (new_pca9546_bus != -1 && ngzmp_set_pca9546(bus, new_pca9546_bus) < 0)
		return -1;

	if (ioctl(ngzmp_i2c_path[bus], I2C_SLAVE, addr) < 0)
  	{
		perror("ngzmp_i2c_read_reg_addr: ERROR accessing ADC I2C bus");
		pthread_mutex_unlock(i2c_mutex+bus);
		return -1;
  	}
	rc = i2c_smbus_write_i2c_block_data(ngzmp_i2c_path[bus], 0, 1, &page8); 
	if (rc < 0)
	{
		perror ("ngzmp_i2c_read_reg_addr_page: Writing page to I2C bus");
		pthread_mutex_unlock(i2c_mutex+bus);
		return -1;
	}
	rc = i2c_smbus_read_i2c_block_data(ngzmp_i2c_path[bus], command, size, value);
	if (rc != size)
	{
		fprintf(stderr, "ngzmp_i2c_read_reg_addr_page: bus=%d, ngzmp_i2c_path[%d]=%d, addr=%d, reg_addr=%d => rc=%d\n", bus, bus, ngzmp_i2c_path[bus], addr, command, rc);
		perror ("Reading From I2C bus");
		return -1;
	}
	pthread_mutex_unlock(i2c_mutex+bus);
	return 0;
}

/**
 * @ingroup internal
 * Read Vout from the UCD90160 applying current (default) scaling set in the UCD90160.
 * This code wraps the set of page, read of VOut mode and read of VOut in the i2c bus mutex to ensure the data and scaling are all from the same rail.
 *
 * @param path 		The handle to the top level of the ngpd system returned from {@link ngpg_config_ngzmp()}.
 * @param card 		Matches the card number if this function is provided at the X86_64 end
 *        			0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 * @param chip		Chip number 0 to 1
 * @param page		The SMbus page number 0...n-1
 * @param num		Number of rails to read in one chip
 * @param value 	Pointer to a 32 bit integer array to return vout *10,000 V (0.1 of mV units)
 * @return 0 or a negative error code.
 */
int ngzmp_i2c_read_ucd90160_vout(int path, int card, int chip, int page, int num, int32_t* vout)
{
	int rc;
	int new_pca9546_bus=-1;
	u_int8_t page8 = (u_int8_t)page, rbuf[2];
	int bus = NGZMP_I2C_BUS_PMBUS;
	int8_t vout_mode, exp;
	int raw, v;
	int i;
	
	if (ngzmp_i2c_open() < 0)
		return -1;

	if (bus >= NGZMP_I2C_BUS_SFP_A && bus <= NGZMP_I2C_BUS_SFP_PEXP)
	{
		new_pca9546_bus = bus - NGZMP_I2C_BUS_SFP_A;
		bus = NGZMP_I2C_BUS_SFP;
	}

	if (bus < 0 || bus >= NUM_I2C || ngzmp_i2c_path[bus] < 0)
	{
		printf("ngzmp_i2c_read_ucd90160_vout: bad I2C bus number %d\n", bus);
		return -1;
	}
	if (chip < 0 || chip >= NGZMP_UCD90160_NUM_CHIPS)
	{
		printf("ngzmp_i2c_read_ucd90160_vout: bad chip number %d\n", chip);
		return -1;
	}
	if (page <0 || page >= NGZMP_UCD90160_NUM_RAILS || num < 1 || page + num > NGZMP_UCD90160_NUM_RAILS)
	{
		printf("ngzmp_i2c_read_ucd90160_vout: bad page number %d or num pages %d\n", page, num);
		return -1;
	}
	pthread_mutex_lock(i2c_mutex+bus);

	if (new_pca9546_bus != -1 && ngzmp_set_pca9546(bus, new_pca9546_bus) < 0)
		return -1;

	if (ioctl(ngzmp_i2c_path[bus], I2C_SLAVE, NGZMP_I2C_ADDR_UDC90160+chip) < 0)
  	{
		perror("ngzmp_i2c_read_ucd90160_vout: ERROR accessing ADC I2C bus");
		pthread_mutex_unlock(i2c_mutex+bus);
		return -1;
  	}
	for (i=0; i<num; i++)
	{
		rc = i2c_smbus_write_i2c_block_data(ngzmp_i2c_path[bus], NGZMP_I2C_UDC90160_PAGE, 1, &page8); 
		if (rc < 0)
		{
			perror ("ngzmp_i2c_read_ucd90160_vout: Writing page to I2C bus");
			pthread_mutex_unlock(i2c_mutex+bus);
			return -1;
		}
		rc = i2c_smbus_read_i2c_block_data(ngzmp_i2c_path[bus], NGZMP_I2C_UDC90160_RW_VOUT_MODE, 1, &vout_mode);
		if (rc != 1)
		{
			fprintf(stderr, "ngzmp_i2c_read_ucd90160_vout: bus=%d, ngzmp_i2c_path[%d]=%d, addr=%d, reg_addr=%d => rc=%d\n", bus, bus, ngzmp_i2c_path[bus], NGZMP_I2C_ADDR_UDC90160+chip, NGZMP_I2C_UDC90160_RW_VOUT_MODE, rc);
			perror ("Reading vout_mode From I2C bus");
			return -1;
		}
			
		rc = i2c_smbus_read_i2c_block_data(ngzmp_i2c_path[bus], NGZMP_I2C_UDC90160_READ_VOUT, 2, rbuf);
		if (rc != 2)
		{
			fprintf(stderr, "ngzmp_i2c_read_ucd90160_vout: bus=%d, ngzmp_i2c_path[%d]=%d, addr=%d, reg_addr=%d => rc=%d\n", bus, bus, ngzmp_i2c_path[bus], NGZMP_I2C_ADDR_UDC90160+chip, NGZMP_I2C_UDC90160_READ_VOUT, rc);
			perror ("Reading Vout value from I2C bus");
			return -1;
		}
		pthread_mutex_unlock(i2c_mutex+bus);
		raw = rbuf[1]<<8 | rbuf[0];
		v = raw *10000;
		exp = vout_mode & 0x1F; 
		exp |= ( vout_mode & 0x10) << 1 | ( vout_mode & 0x10) << 2 | ( vout_mode & 0x10) << 3;
		if (exp < 0)
			v >>= (-exp);
		else
			v <<= exp;
		*vout++ = v;
		page8++;
	}
	return 0;
}
