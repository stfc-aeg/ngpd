#if NGPD_CONF_ADQ14
#include "ADQAPI.h"
#endif
#include <stdint.h>
//#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
// #include "smbus.h"

#include <string.h>
#include "ngpd.h"

#include "zynqmp_protocol.h"
#include "zynqmp_api.h"

const char *ngpd_ucd90160_signals[NGZMP_UCD90160_NUM_CHIPS][NGZMP_UCD90160_NUM_RAILS] = {
{ "AVDD3V3_INT=3V9", "AVDD1V_INT=1V3", "AVDD1V8_INT=2V4", "AVDD2V5_INT=3V4", "DVDD1V_INT=1V3", "VDD3V3_CLK_INT=3V9", "VDD1V2_DIG", "VDD3V3_DAE", "VDD1V8_SPI", "AVDD3V3<0>", "AVDD3V3<1>", "AVDD1V<0>", "AVDD1V<1>", "VIN_12V", NULL, NULL },
{ "AVDD2V5<0>", "AVDD2V5<1>", "DVDD1V<0>", "DVDD1V<1>", "AVDD1V8_PLL<0>", "AVDD1V8_PLL<1>", "AVDD1V8<0>", "AVDD1V8<1>", "VIN_12V", "VDD3V3_CLK", "VDD5V_I2C", "VMON_INT/(5V0*2/5)", "VMON_VTT", "VMON_PSINT/1V2", "VMON_0V9", "VMON_1V2/BAT"}
};

const char *ngpd_ucd90160_signals_python[NGZMP_UCD90160_NUM_CHIPS][NGZMP_UCD90160_NUM_RAILS] = {
{ "AVDD3V3_INT_3V9", "AVDD1V_INT_1V3", "AVDD1V8_INT_2V4", "AVDD2V5_INT_3V4", "DVDD1V_INT_1V3", "VDD3V3_CLK_INT_3V9", "VDD1V2_DIG", "VDD3V3_DAE", "VDD1V8_SPI", "AVDD3V3_0", "AVDD3V3_1", "AVDD1V_0", "AVDD1V_1", "VIN_12VA", NULL, NULL },
{ "AVDD2V5_0", "AVDD2V5_1", "DVDD1V_0", "DVDD1V_1", "AVDD1V8_PLL_0", "AVDD1V8_PLL_1", "AVDD1V8_0", "AVDD1V8_1", "VIN_12VB", "VDD3V3_CLK", "VDD5V_I2C", "VMON_INT_OR_2V", "VMON_VTT", "VMON_PSINT_OR_1V2", "VMON_0V9", "VMON_1V2_OR_BAT"}
};


static int preamp_offset_map[NGZMP_NUM_PREAMP_OFFSET_MAPS][NGZMP_CHANS_PER_CARD] = 
{
	{4, 5, 7, 6, 1, 0, 3, 2},		// Normal forward map for DL559720 pre-amp board
	{2, 3, 0, 1, 6, 7, 5, 4}		// For DL5550720 used with mirrored input connectiion. 
									// Consider adding extra rows for other pre-amp builds.
};
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
 * @param data an array of 8 bit integer values to write.
 * @return 0 or a negative error code.
 */
int ngzmp_i2c_write_reg(int path, int card, int bus, int addr, int size, u_int8_t* data)
{
	int rc;
	int this_path = path;
	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_write_reg: Invalid path=%d", path);
		return -1;
	}
	if (card < 0)
	{
		for (card=0; card<NGPDPath[path].num_cards; card++)
		{
			if ((rc=ngzmp_i2c_write_reg(path, card, bus, addr, size, data)) < 0)
				return rc;
		}
		return 0;
	}
	if (card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_write_reg: card=%d is out of range 0..%d", card, NGPDPath[path].num_cards-1);
		return -1;
	}
	if (NGPDPath[path].type == NGPDComposite)
		this_path = NGPDPath[path].sub_path[card];

	if (!((bus >= NGZMP_I2C_BUS_SFP_A && bus <= NGZMP_I2C_BUS_SFP_PEXP) || (bus >= 0 && bus <= NGZMP_I2C_BUS_SFP)))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_write_reg: bus=%d is out of range 0..%d or %d..%d", bus, NGZMP_I2C_BUS_SFP, NGZMP_I2C_BUS_SFP_A, NGZMP_I2C_BUS_SFP_PEXP);
		return -1;
	}
	if (addr < 0 || addr > 255)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_write_reg: slave addr=%d is out of range 0..255", addr);
		return -1;
	}
	addr |= bus << 8;
	rc = zynqmp_access_write(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_I2C, ZYNQMP_WIDTH_BYTE, addr, size, 0, (u_int8_t *)data);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_write_reg: %s", zynqmp_get_error_message());
		return rc;
	}
	return 0;
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
 * @param data an array of 8 bit integer values to write.
 * @return {@link #XSP3_OK XSP3_OK} or a negative error code.
 */
int ngzmp_i2c_read_reg(int path, int card, int bus, int addr, int size, u_int8_t* data)
{
	int rc;
	int this_path = path;
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg: Invalid path=%d", path);
		return -1;
	}
	if (card < 0 || card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg: card=%d is out of range 0..%d", card, NGPDPath[path].num_cards-1);
		return -1;
	}
	if (NGPDPath[path].type == NGPDComposite)
		this_path = NGPDPath[path].sub_path[card];

	if (!((bus >= NGZMP_I2C_BUS_SFP_A && bus <= NGZMP_I2C_BUS_SFP_PEXP) || (bus >= 0 && bus <= NGZMP_I2C_BUS_SFP)))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg: bus=%d is out of range 0..%d or %d..%d", bus, NGZMP_I2C_BUS_SFP, NGZMP_I2C_BUS_SFP_A, NGZMP_I2C_BUS_SFP_PEXP);
		return -1;
	}
	if (addr < 0 || addr > 255)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg: slave addr=%d is out of range 0..255", addr);
		return -1;
	}
	addr |= bus << 8;
	rc = zynqmp_access_read(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_I2C, ZYNQMP_WIDTH_BYTE, addr, size, 0, (u_int8_t *)data);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg: %s", zynqmp_get_error_message());
		return rc;
	}
	return 0;
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
 * @param data 	Pointer to an array of 8 bit integer values to write.
 * @return 0 or a negative error code.
 */
int ngzmp_i2c_write_reg_addr(int path, int card, int bus, int addr, int reg_addr, int size, u_int8_t* data)
{
	int rc;
	int this_path = path;
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_write_reg_addr: Invalid path=%d", path);
		return -1;
	}
	if (card < 0)
	{
		for (card=0; card<NGPDPath[path].num_cards; card++)
		{
			if ((rc=ngzmp_i2c_write_reg_addr(path, card, bus, addr, reg_addr, size, data)) < 0)
				return rc;
		}
		return 0;
	}
	if (card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_write_reg_addr: card=%d is out of range 0..%d", card, NGPDPath[path].num_cards-1);
		return -1;
	}
	if (NGPDPath[path].type == NGPDComposite)
		this_path = NGPDPath[path].sub_path[card];

	if (!((bus >= NGZMP_I2C_BUS_SFP_A && bus <= NGZMP_I2C_BUS_SFP_PEXP) || (bus >= 0 && bus <= NGZMP_I2C_BUS_SFP)))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_write_reg_addr: bus=%d is out of range 0..%d or %d..%d", bus, NGZMP_I2C_BUS_SFP, NGZMP_I2C_BUS_SFP_A, NGZMP_I2C_BUS_SFP_PEXP);
		return -1;
	}
	if (addr < 0 || addr > 255)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_write_reg_addr: slave addr=%d is out of range 0..255", addr);
		return -1;
	}
	if (reg_addr < 0 || reg_addr > 255)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_write_reg_addr: register addr=%d is out of range 0..255", reg_addr);
		return -1;
	}
	addr |= bus << 8 | reg_addr << 16 | ZYNQMP_I2C_USE_REG_ADDR;
	rc = zynqmp_access_write(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_I2C, ZYNQMP_WIDTH_BYTE, addr, size, 0, (u_int8_t *)data);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_write_reg_addr: %s", zynqmp_get_error_message());
		return rc;
	}
	return 0;
}


/**
 * @ingroup internal
 * Read a series of values from the ADC board I2C bus device registers for all or each specified card sending the register address as the command byte.
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
 * @param data 	Pointer to an array of 8 bit integer values to receive data.
 * @return 0 or a negative error code.
 */
int ngzmp_i2c_read_reg_addr(int path, int card, int bus, int addr, int reg_addr, int size, u_int8_t* data)
{
	int rc;
	int this_path = path;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg_addr: Invalid path=%d", path);
		return -1;
	}
	if (card < 0 || card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg_addr: card=%d is out of range 0..%d", card, NGPDPath[path].num_cards-1);
		return -1;
	}
	if (NGPDPath[path].type == NGPDComposite)
		this_path = NGPDPath[path].sub_path[card];

	if (!((bus >= NGZMP_I2C_BUS_SFP_A && bus <= NGZMP_I2C_BUS_SFP_PEXP) || (bus >= 0 && bus <= NGZMP_I2C_BUS_SFP)))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg_addr: bus=%d is out of range 0..%d or %d..%d", bus, NGZMP_I2C_BUS_SFP, NGZMP_I2C_BUS_SFP_A, NGZMP_I2C_BUS_SFP_PEXP);
		return -1;
	}
	if (addr < 0 || addr > 255)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg_addr: slave addr=%d is out of range 0..255", addr);
		return -1;
	}
	if (reg_addr < 0 || reg_addr > 255)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg_addr: register addr=%d is out of range 0..255", reg_addr);
		return -1;
	}
	addr |= bus << 8 | reg_addr << 16 | ZYNQMP_I2C_USE_REG_ADDR;
	rc = zynqmp_access_read(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_I2C, ZYNQMP_WIDTH_BYTE, addr, size, 0, (u_int8_t *)data);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg_addr: %s", zynqmp_get_error_message());
		return rc;
	}
	return 0;
}

/**
 * @ingroup internal
 * Read a series of values from the ADC board I2C bus device registers for all or each specified card sending the register address as the command byte.
 * This version allows the command/address byte to be padded from 1 to 4 bytes to suit the AD9675
 * @n@n
 * The addresses are defined in {@link NGZMP_I2C_REGISTERS}
 *
 * @param path 		The handle to the top level of the ngpd system returned from {@link ngpg_config_ngzmp()}.
 * @param card 		Matches the card number if this function is provided at the X86_64 end
 *        			0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 * @param bus		The I2C Bus number.
 * @param addr 		The I2C slave address on this bus
 * @param reg_addr 	The register address within the device sent as the command
 * @param reg_addr_size	Number of bytes 1..4 to be sent to the I2C device as command/address.
 * @param size 		The size of the array of values to write, skipping channel to channel.
 * @param data 	Pointer to an array of 8 bit integer values to receive data.
 * @return 0 or a negative error code.
 */
int ngzmp_i2c_read_reg_addr_padded(int path, int card, int bus, int addr, int reg_addr, int reg_addr_size, int size, u_int8_t* data)
{
	int rc;
	int this_path = path;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg_addr_padded: Invalid path=%d", path);
		return -1;
	}
	if (card < 0 || card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg_addr_padded: card=%d is out of range 0..%d", card, NGPDPath[path].num_cards-1);
		return -1;
	}
	if (NGPDPath[path].type == NGPDComposite)
		this_path = NGPDPath[path].sub_path[card];

	if (!((bus >= NGZMP_I2C_BUS_SFP_A && bus <= NGZMP_I2C_BUS_SFP_PEXP) || (bus >= 0 && bus <= NGZMP_I2C_BUS_SFP)))
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg_addr_padded: bus=%d is out of range 0..%d or %d..%d", bus, NGZMP_I2C_BUS_SFP, NGZMP_I2C_BUS_SFP_A, NGZMP_I2C_BUS_SFP_PEXP);
		return -1;
	}
	if (addr < 0 || addr > 255)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg_addr_padded: slave addr=%d is out of range 0..255", addr);
		return -1;
	}
	if (reg_addr < 0 || reg_addr > 255)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg_addr_padded: register addr=%d is out of range 0..255", reg_addr);
		return -1;
	}
	if (reg_addr_size < 1 || reg_addr_size > 4)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg_addr_padded: register addr size =%d is out of range 1..4", reg_addr_size);
		return -1;
	}
	addr |= bus << 8 | reg_addr << 16 | reg_addr_size<< 24 | ZYNQMP_I2C_USE_REG_ADDR;
	rc = zynqmp_access_read(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_I2C, ZYNQMP_WIDTH_BYTE, addr, size, 0, (u_int8_t *)data);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_reg_addr: %s", zynqmp_get_error_message());
		return rc;
	}
	return 0;
}


int ngpd_i2c_read_adc_temp(int path, int card, float *temp, int *status) 
{
	int i;	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_read_adc_temp: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_ADC)
	{
		for (i=0; i<NGZMP_I2C_NUM_ADT7410_ADC; i++)
		{
			*temp++ = 40.0+card+0.1*i;
			if (status != NULL)
				*status++ = (i&3)<<5;
		}
		return 0;
	}
	return ngpd_i2c_read_temp(path, card, temp, status, NGZMP_I2C_BUS_PMBUS, NGZMP_I2C_NUM_ADT7410_ADC, "adc");
}
int ngpd_i2c_read_preamp_temp(int path, int card, float *temp, int *status) 
{
	int i;	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_read_preamp_temp: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_Preamp)
	{
		for (i=0; i<NGZMP_I2C_NUM_ADT7410_PREAMP; i++)
		{
			*temp++ = 30.0+card+0.1*i;
			if (status != NULL)
				*status++ = i<<6;
		}
		return 0;
	}
	return ngpd_i2c_read_temp(path, card, temp, status, NGZMP_I2C_BUS_PREAMP, NGZMP_I2C_NUM_ADT7410_PREAMP, "preamp");
}
int ngpd_i2c_read_temp(int path, int card, float *temp, int *status, int bus, int num_adt7410, char *name) 
{
	int rc;
	int i;
	u_int8_t buf[2];
	int16_t raw_temp;
	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_read_%s_temp: Invalid path=%d", name, path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_read_%s_temp: Not supported on ADQ14", name);
		return -1;
	}

	for (i=0; i<num_adt7410; i++)
	{
		u_int32_t addr;
		addr = NGZMP_I2C_ADDR_ADT7410(i);

		rc = ngzmp_i2c_read_reg_addr(path, card, bus, addr, ADT7410_TEMP_MSB, 2, buf);
		if (rc < 0)
			return -1;
		raw_temp = (buf[0] << 8) | (buf[1] & 0xF8);
		*temp++ = raw_temp / 128.0;
		if (status != NULL)
			*status++ = buf[1] & 7;
	}
	return rc;
}
int ngpd_i2c_write_adc_tcrit(int path, int card, int chip, int tcrit) 
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_adc_tcrit: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_ADC)
	{
		printf("Dummy: ngpd_i2c_write_adc_tcrit : card=%d, chip=%d, tcrit=%d\n", card, chip, tcrit);
		return 0;
	}
	return ngpd_i2c_write_tcrit(path, card, chip, tcrit, NGZMP_I2C_BUS_PMBUS, NGZMP_I2C_NUM_ADT7410_ADC, "adc"); 
}
int ngpd_i2c_write_preamp_tcrit(int path, int card, int chip, int tcrit) 
{
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_preamp_tcrit: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_Preamp)
	{
		printf("Dummy: ngpd_i2c_write_preamp_tcrit : card=%d, chip=%d, tcrit=%d\n",  card, chip, tcrit);
		return 0;
	}
	return ngpd_i2c_write_tcrit(path, card, chip, tcrit, NGZMP_I2C_BUS_PREAMP, NGZMP_I2C_NUM_ADT7410_PREAMP, "preamp"); 
}
int ngpd_i2c_write_tcrit(int path, int card, int chip, int tcrit, int bus, int num_adt7410, char * name) 
{
	int rc;
	int i;
	u_int8_t buf[2];
	int first, last;
	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_%s_tcrit: Invalid path=%d", name, path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_%s_tcrit: Not supported on ADQ14", name);
		return -1;
	}
	if (chip < 0)
	{
		first = 0;
		last = num_adt7410-1;
	}
	else if (chip >= num_adt7410)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_%s_tcrit: Chip=%d out or range 0..%d", name, chip, num_adt7410-1);
		return -1;
	}
	else
	{
		first = last = chip;
	}
		
	tcrit *=  128;
	buf[0] = tcrit >> 8;
	buf[1] = tcrit & 0xFF;
	for (i=first; i<=last; i++)
	{
		u_int32_t addr;
		u_int8_t csr=0x10;
		addr = NGZMP_I2C_ADDR_ADT7410(i);
		rc = ngzmp_i2c_write_reg_addr(path, card, bus, addr, ADT7410_CONFIG, 1, &csr);
		if (rc < 0)
			break;

		rc = ngzmp_i2c_write_reg_addr(path, card, bus, addr, ADT7410_TCRIT_MSB, 2, buf);
		if (rc < 0)
			break;
	}
	return rc;
}

#define MAX_REGS 60

/**
 * @ingroup debug
 * Configure a the LMK61E2 clock generator chip on an ZynqMP ngpd.
 * This is done once during commissioning of the board and saved to EEPROM. The user code should not access this.
 *
 * @param path 		A handle to the top level of the NGPD system returned from {@link ngpd_config_ngzmp()}.
 * @param card		Card is the number of the card in the NGPD system,
 *        			0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *      	  		where this value has been passed to {@link ngpd_config_ngzmp()} at NGPD system configuration.
 *					If card <0 then the data is copied to all cards.
 * @param fname		File name for the values to write to the device.
 * @return 			Return 0 on success or negative error message
 */

int ngpd_i2c_write_lmk61e2_from_file(int path, int card, char *fname)
{
	FILE *fp;
	NGZMPI2CAddrData reg[MAX_REGS];
	int i, j, l, num;
	char line_buf[102];

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_lmk61e2_from_file: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_lmk61e2_from_file: Not supported on ADQ14");
		return -1;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_ADC)
	{
		printf("Dummy: ngpd_i2c_write_lmk61e2_from_file : card=%d, file=%s\n", card, fname);
		return 0;
	}

	if ((fp=fopen(fname, "r")) == NULL)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_lmk61e2_from_file: Cannot open file %s, errno=%d", fname, errno);
		return -1;
	}

	num=0;
	while (fgets(line_buf, 100, fp) != NULL)
	{
		if (sscanf(line_buf,"R%d %i", &i, &j) != 2)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_lmk61e2_from_file: Unexpected format at line %d,  '%s'", num+1, line_buf);
			fclose(fp);
			return -1;
		}
		l = strlen(line_buf);
		if (l>0 && line_buf[l-1] == '\n')
			line_buf[l-1] = 0;
		if (l>1 && line_buf[l-2] == '\r')
			line_buf[l-2] = 0;
		reg[num].addr = j >> 8;
		reg[num].data = j & 0xFF;
		printf("'%s' => len=%d, Addr %02X, Data %02X\n", line_buf, l, reg[num].addr, reg[num].data);
		num++;
		if (num == MAX_REGS)
		{
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_lmk61e2_from_file: Too many lines of data, maximum %d", MAX_REGS);
			fclose(fp);
			return -1;
		}

	}
	fclose(fp);

	return ngpd_i2c_write_lmk61e2(path, card, num, reg);
}

/**
 * @ingroup debug
 * Write registers of the LMK61E2 clock generator chip on a NGPD ZynqMP version.
 * This is can be used for debugging, but the correct configuration is usually done once during commissioning of the board and saved to EEPROM. The user code should not access this.
 *
 * @param path 		A handle to the top level of the NGPD system returned from {@link ngpd_config_ngzmp()}.
 * @param card		Card is the number of the card in the NGPD system,
 *        			0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *      	  		where this value has been passed to {@link ngpd_config_ngzmp()} at NGPD system configuration.
 *					If card <0 then the data is copied to all cards.
 * @param num		Number of address/value pair to write.
 * @param reg		Pointer to array of address/value pair to write.
 * @return 			Returns 0 on success or negative error message
 */
int ngpd_i2c_write_lmk61e2(int path, int card, int num, NGZMPI2CAddrData *reg)
{
	int i;
	int rc;
	int bus = NGZMP_I2C_BUS_CLK;
	int addr = NGZMP_I2C_ADDR_CLK_LMK61E2;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_lmk61e2: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_lmk61e2: Not supported on ADQ14");
		return -1;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_ADC)
	{
		printf("Dummy: ngpd_i2c_write_lmk61e2 : card=%d\n", card);
		return 0;
	}

	if (card < 0)
	{
		for (card=0; card<NGPDPath[path].num_cards; card++)
		{
			rc = ngpd_i2c_write_lmk61e2(path, card, num, reg);
			if (rc < 0)
				break;
		}
		return rc;
	}

	for (i=0; i<num; i++)
	{
		rc = ngzmp_i2c_write_reg_addr(path, card, bus, addr, reg[i].addr, 1, &(reg[i].data));
		if (rc < 0)
		{
			char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
			strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
			old_msg[NGPD_MAX_ERROR_MESSAGE] = 0;
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_lmk61e2: %s", old_msg);
			return rc;
		}
	}
	return 0;
}

/**
 * @ingroup debug
 * Save current register setup of the LMK61E2 clock generator chip on an NGPD zynqMP version
 * This is done once during commissioning of the board. The user code should not access this.
 *
 * @param path 		A handle to the top level of the NGPD system returned from {@link ngpd_config_ngzmp()}.
 * @param card		Card is the number of the card in the NGPD system,
 *        			0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *      	  		where this value has been passed to {@link ngpd_config_ngzmp()} at NGPD system configuration.
 *					If card <0 then the data is copied to all cards.
 * @return {@link #XSP3_OK XSP3_OK}
 */
int ngpd_i2c_save_lmk61e2_to_eeprom(int path, int card)
{
	NGZMPI2CAddrData reg[6] ={{0x31, 0x50}, //R49 bit 6 => Copy regs to SRAM
									{0x38, 0xBE},	// Write enable EEPROM
									{0x31, 0x53},	// Trigger transfer
									{0x38, 0x00},	// Write protect EEPROM
									{0x31, 0x50},
									{0x31, 0x10} };

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_save_lmk61e2_to_eeprom: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_save_lmk61e2_to_eeprom: Not supported on ADQ14");
		return -1;
	}

	return ngpd_i2c_write_lmk61e2(path, card, 6, reg);
}
/**
 * @ingroup debug
 * Read registers from the LMK61E2 clock generator chip on NGPD ZynqMP version.
 * This is for debugging purposes.
 *
 * @param path 		A handle to the top level of the NGPD system returned from {@link ngpd_config_ngzmp()}.
 * @param card		Card is the number of the card in the NGPD system,
 *        			0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *      	  		where this value has been passed to {@link ngpd_config_ngzmp()} at NGPD system configuration.
 * @param reg_addr	Register address within the device of the first register to read.
 * @param num		Number sequential registers to read
 * @param reg		Pointer to array to return data.
 * @return 			Returns 0 on success or negative error message
 */

int ngpd_i2c_read_lmk61e2(int path, int card, int reg_addr, int num, u_int8_t *reg)
{
	int bus = NGZMP_I2C_BUS_CLK;
	int addr = NGZMP_I2C_ADDR_CLK_LMK61E2;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_read_lmk61e2: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_read_lmk61e2: Not supported on ADQ14");
		return -1;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_ADC)
	{
		printf("Dummy: ngpd_i2c_read_lmk61e2 : card=%d", card);
		return 0;
	}
	return ngzmp_i2c_read_reg_addr(path, card, bus, addr, reg_addr, num, reg);
}

/**
 * @ingroup debug
 * Read power status from UCD90160 and MCP232008 on NGPD ZynqMP version.
 * This is for debugging purposes.
 *
 * @param path 		A handle to the top level of the NGPD system returned from {@link ngpd_config_ngzmp()}.
 * @param card		Card is the number of the card in the NGPD system,
 *        			0 for a single card system and up to ({@link ngpd_get_num_cards()} - 1) for a multi-card system,
 *      	  		where this value has been passed to {@link ngpd_config_ngzmp()} at NGPD system configuration.
 * @param ucd90160	Pointer to integer array to get ststus from UDC90160
 * @param mcp23008	Pointer to integer to return data from MCP23008
 * @return 			Returns 0 on success or negative error message
 */

int ngpd_i2c_read_psu_status(int path, int card, int *udc90160, int *mcp23008)
{
	int rc;
	u_int8_t d8;

	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_read_psu_status: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_read_psu_status: Not supported on ADQ14");
		return -1;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_ADC)
	{
		printf("Dummy: ngpd_i2c_read_psu_status : card=%d", card);
		return 0;
	}
	rc = ngzmp_i2c_read_reg_addr(path, card, NGZMP_I2C_BUS_PMBUS, NGZMP_I2C_ADDR_PSU_PEXP, MCP23008_GPIO, 1, &d8);
	*mcp23008 = d8;
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
	int this_path, chan_of_card, card;
	int c;
	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_preamp_offset: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_preamp_offset: Not supported on ADQ14");
		return -1;
	}
	if (chan == 0 && num < 0)
	{
		copy_data = 1;
		num = NGPDPath[path].num_chan;
	}
	else if (chan < 0 || chan >= NGPDPath[path].num_chan || num <1 || chan+num > NGPDPath[path].num_chan)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_preamp_offset: Channel %d and num %d is out of range 0...%d", chan, num, NGPDPath[path].num_chan-1);
		return -1;
	}
	for (c=0; c<num; c++)
	{
		if ((rc = ngpd_resolve_path_chan_card(path, chan+c, &this_path, &chan_of_card, &card)) < 0)
		{
			char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
			strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
			old_msg[NGPD_MAX_ERROR_MESSAGE] = 0;
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_preamp_offset: %s", old_msg);
			return rc;
		}
		if ( ((card &1) == 0 && (NGPDPath[path].preamp_mirror_offset & NGPDPreampMirrorEven)) || ((card &1) == 1 && (NGPDPath[path].preamp_mirror_offset & NGPDPreampMirrorOdd)))
			chan_of_card = preamp_offset_map[1][chan_of_card];
		else
			chan_of_card = preamp_offset_map[0][chan_of_card];
		if (NGPDPath[path].dummy_system & NGPDDummy_Preamp)
		{
			NGPDPath[this_path].dummy_chan_offset[chan_of_card] = *data;
			if (!copy_data)
				data++;
		}
		else
		{
			buf[0] = *data >> 8;
			buf[1] = *data & 0xFF;
			if (!copy_data)
				data++;
			rc = ngzmp_i2c_write_reg_addr(path, card, NGZMP_I2C_BUS_PREAMP, NGZMP_I2C_ADDR_OFFSET_DAC, AD5675_CMD_WRITE_UPDATE(chan_of_card), 2, buf);
			if (rc < 0)
			{
				char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
				strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
				old_msg[NGPD_MAX_ERROR_MESSAGE] = 0;
				snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_write_preamp_offset: %s", old_msg);
				return rc;
			}
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
	int rc=0;
	u_int8_t buf[2];
	int this_path, chan_of_card, card;
	int c;
	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_read_preamp_offset: Invalid path=%d", path);
		return -1;
	}
	if (NGPDPath[path].generation == NGPDGenADQ14)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_read_preamp_offset: Not supported on ADQ14");
		return -1;
	}
	if (chan < 0 || chan >= NGPDPath[path].num_chan || num <1 || chan+num > NGPDPath[path].num_chan)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_read_preamp_offset: Channel %d and num %d is out of range 0...%d", chan, num, NGPDPath[path].num_chan-1);
		return -1;
	}
	for (c=0; c<num; c++)
	{
		if ((rc = ngpd_resolve_path_chan_card(path, chan+c, &this_path, &chan_of_card, &card)) < 0)
		{
			char old_msg[NGPD_MAX_ERROR_MESSAGE+2];
			strncpy(old_msg, ngpd_error_message, NGPD_MAX_ERROR_MESSAGE);
			old_msg[NGPD_MAX_ERROR_MESSAGE] = 0;
			snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngpd_i2c_read_preamp_offset: %s", old_msg);
			return rc;
		}
		if ( ((card &1) == 0 && (NGPDPath[path].preamp_mirror_offset & NGPDPreampMirrorEven)) || ((card &1) == 1 && (NGPDPath[path].preamp_mirror_offset & NGPDPreampMirrorOdd)))
			chan_of_card = preamp_offset_map[1][chan_of_card];
		else
			chan_of_card = preamp_offset_map[0][chan_of_card];

		if (NGPDPath[path].dummy_system & NGPDDummy_Preamp)
		{
			*data++ = NGPDPath[this_path].dummy_chan_offset[chan_of_card];
		}
		else
		{
			rc = ngzmp_i2c_read_reg_addr_padded(path, card, NGZMP_I2C_BUS_PREAMP, NGZMP_I2C_ADDR_OFFSET_DAC, AD5675_CMD_READ(chan_of_card), 3, 2, buf);
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
	}
	return rc;
}

static int ngzmp_dummy_read_ucd90160_vout(int path, int card, int chip, int page, int num,  int32_t* vout);
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
 * @parm num		Number of pages (rails) to read within one chip
 * @param value 	Pointer to a 32 bit integer to return vout *10,000 V (0.1 of mV units)
 * @return 0 or a negative error code.
 */
int ngzmp_i2c_read_ucd90160_vout(int path, int card, int chip, int page, int num,  int32_t* vout)
{
	int rc;
	int this_path ;
	int addr;
	
	if (path <0 || path >= NGPD_MAX_PATH || !NGPDPath[path].valid)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_ucd90160_vout: Invalid path=%d", path);
		return -1;
	}
	if (card < 0 || card >= NGPDPath[path].num_cards)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_ucd90160_vout: card=%d is out of range 0..%d", card, NGPDPath[path].num_cards-1);
		return -1;
	}
	if (NGPDPath[path].type == NGPDComposite)
		this_path = NGPDPath[path].sub_path[card];
	else
		this_path = path;
	
	if (chip <0 || chip >= NGZMP_UCD90160_NUM_CHIPS)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_ucd90160_vout: chip=%d is out of range 0..%d", chip, NGZMP_UCD90160_NUM_CHIPS-1);
		return -1;
	}
	if (page <0 || page >= NGZMP_UCD90160_NUM_RAILS || num < 1 || page + num > NGZMP_UCD90160_NUM_RAILS)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_ucd90160_vout: page=%d or num=%d is out of range 0..%d", page, num, NGZMP_UCD90160_NUM_RAILS-1);
		printf("ngzmp_i2c_read_ucd90160_vout: bad page number %d or num pages %d\n", page, num);
		return -1;
	}
	if (NGPDPath[path].dummy_system & NGPDDummy_ADC)
		return ngzmp_dummy_read_ucd90160_vout(path, card, chip, page, num, vout);
	addr = chip <<8 | page;
	rc = zynqmp_access_read(NGPDPath[this_path].zynqmp, ZYNQMP_BUS_UCD90160, ZYNQMP_WIDTH_LONG, addr, num, 1, (u_int8_t *)vout);
	if (rc < 0)
	{
		snprintf(ngpd_error_message, NGPD_MAX_ERROR_MESSAGE, "ngzmp_i2c_read_ucd90160_vout: %s", zynqmp_get_error_message());
		return rc;
	}
	return 0;
}

static int ngzmp_dummy_read_ucd90160_vout(int path, int card, int chip, int page, int num,  int32_t* vout)
{
	int i;
	static int rdnum;

	if (chip == 0)
	{
		for (i=0; i<num; i++)
		{
			switch (page+i)
			{
			case 0: *vout++ = 39000+card*10+rdnum; break; 	// AVDD3V3_INT=3V9
			case 1: *vout++ = 13000+card*10+rdnum; break;	// AVDD1V_INT=1V3
			case 2: *vout++ = 24000+card*10+rdnum; break;	// AVDD1V8_INT=2V4
			case 3: *vout++ = 34000+card*10+rdnum; break;	// AVDD2V5_INT=3V4
			case 4: *vout++ = 13000+card*10+rdnum; break;	// DVDD1V_INT=1V3
			case 5: *vout++ = 39000+card*10+rdnum; break;	// VDD3V3_CLK_INT=3V9
			case 6: *vout++ = 12000+card*10+rdnum; break;	// VDD1V2_DIG
			case 7: *vout++ = 33000+card*10+rdnum; break;	// VDD3V3_DAE
			case 8: *vout++ = 18000+card*10+rdnum; break;	// VDD1V8_SPI
			case 9: *vout++ = 33000+card*10+rdnum; break;	// AVDD3V3<0>
			case 10: *vout++ = 33000+card*10+rdnum; break;	// AVDD3V3<1>
			case 11: *vout++ = 9750+card*10+rdnum; break;	// AVDD1V<0>
			case 12: *vout++ = 9750+card*10+rdnum; break;	// AVDD1V<1>
			case 13: *vout++ = 120000+card*10+rdnum; break;	// VIN_12V
			case 14: *vout++ = 0; break;	// 
			case 15: *vout++ = 0; break;	// 
			}
		}
	}
	else
	{
		for (i=0; i<num; i++)
		{
			switch (page+i)
			{
			case 0: *vout++ = 25000+card*10+rdnum; break;	// AVDD2V5<0>
			case 1: *vout++ = 25000+card*10+rdnum; break;	// AVDD2V5<1>
			case 2: *vout++ =  9750+card*10+rdnum; break;	// DVDD1V<0>
			case 3: *vout++ =  9750+card*10+rdnum; break;	// DVDD1V<1>
			case 4: *vout++ = 18000+card*10+rdnum; break;	// AVDD1V8_PLL<0>
			case 5: *vout++ = 18000+card*10+rdnum; break;	// AVDD1V8_PLL<1>
			case 6: *vout++ = 18000+card*10+rdnum; break;	// AVDD1V8<0>
			case 7: *vout++ = 18000+card*10+rdnum; break;	// AVDD1V8<1>
			case 8: *vout++ = 120000+card*10+rdnum; break;	// VIN_12V
			case 9: *vout++ = 33000+card*10+rdnum; break;	// VDD3V3_CLK
			case 10: *vout++ = 50000+card*10+rdnum; break;	// VDD5V_I2C
			case 11: *vout++ =  8500+card*10+rdnum; break;	// VMON_INT/(5V0*2/5)
			case 12: *vout++ =  9000+card*10+rdnum; break;	// VMON_VTT
			case 13: *vout++ =  8500+card*10+rdnum; break;	// VMON_PSINT/1V2
			case 14: *vout++ =  9000+card*10+rdnum; break;	// VMON_0V9
			case 15: *vout++ = 12000+card*10+rdnum; break;	// VMON_1V2/BAT
			}
		}
	}
	rdnum =(rdnum+1) % 10;
	return 0;
}
	
