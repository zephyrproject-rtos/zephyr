/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq40zxx

#include <drivers/i2c.h>
#include <init.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <string.h>
#include <sys/byteorder.h>

#include "bq40zxx.h"

#define BQ40ZXX_SUBCLASS_DELAY 5 /* subclass 64 & 82 needs 5ms delay */

#define MAC_READ_OVERHEAD  2
#define MAC_READ_BLOCK_SZ(SZ)    (SZ + 3)
#define MAC_WRITE_BLOCK_SZ(SZ)   (SZ + 1)

#define READ_BLOCK_SZ(SZ)    (SZ + 0)
#define WRITE_BLOCK_SZ(SZ)   (SZ + 1)

#define U1    1
#define U2    2
#define U4    4

#define I1    1
#define I2    2
#define I4    4

#define FP    4

/*******************************************************************
 * ********************** SMBUS block ******************************
 * *****************************************************************/
static int smbus_block_read(struct bq40zxx_data *bq40zxx, uint8_t reg_addr,
                    uint8_t* rd_buf, size_t rd_sz)
{
    //LOG_INF("SBR: %d, 0x%x", rd_sz, reg_addr);
    int status = i2c_burst_read(bq40zxx->i2c, DT_INST_REG_ADDR(0),
            reg_addr, rd_buf, rd_sz);
	if (status < 0) {
		LOG_DBG("Unable to read register");
		return -EIO;
	}
    return status;
}

static int smbus_block_write(struct bq40zxx_data *bq40zxx, uint8_t reg_addr,
                    uint8_t* wr_buf, size_t wr_sz)
{
    //LOG_INF("SBW: %d, 0x%x", wr_sz, reg_addr);
    wr_buf[0] = wr_sz - 1;

	int status = i2c_burst_write(bq40zxx->i2c, DT_INST_REG_ADDR(0),
                    reg_addr, wr_buf, wr_sz);
	if (status < 0) {
		LOG_DBG("Failed to write into block access register: %d", status);
		return -EIO;
	}
    return status;
}

/*******************************************************************
 * ********************** BQ40ZXX helpers **************************
 * *****************************************************************/
static int bq40zxx_control_reg_write(struct bq40zxx_data *bq40zxx,
				     uint16_t subcommand)
{
	int status = 0;
    uint8_t buf[MAC_WRITE_BLOCK_SZ(sizeof(subcommand))];
    buf[MAC_WRITE_BLOCK_SZ(0)] = (subcommand & 0x00FF);
    buf[MAC_WRITE_BLOCK_SZ(1)] = ((subcommand >> 8) & 0x00FF);

	status = smbus_block_write(bq40zxx,
            BQ40ZXX_COMMAND_MANUFACTURER_BLOCK_ACCESS,
			buf, sizeof(buf));
	if (status < 0) {
		LOG_DBG("Failed to write into block access register");
		return -EIO;
	}

	k_msleep(2);

	return 0;
}

static int bq40zxx_read_data_block(struct bq40zxx_data *bq40zxx, uint8_t offset,
				   uint8_t *data, uint8_t bytes)
{
	uint8_t rd_buf;
	int status = 0;

	rd_buf = BQ40ZXX_EXTENDED_BLOCKDATA_START + offset;

	status = i2c_burst_read(bq40zxx->i2c, DT_INST_REG_ADDR(0), rd_buf,
				data, bytes);
	if (status < 0) {
		LOG_DBG("Failed to read block");
		return -EIO;
	}

	k_msleep(BQ40ZXX_SUBCLASS_DELAY);

	return 0;
}

static int bq40zxx_write_data_block(struct bq40zxx_data *bq40zxx, uint8_t offset,
				   uint8_t *data, uint8_t bytes)
{
	uint8_t rd_buf;
	int status = 0;

	rd_buf = BQ40ZXX_EXTENDED_BLOCKDATA_START + offset;

	status = i2c_burst_write(bq40zxx->i2c, DT_INST_REG_ADDR(0), rd_buf,
				data, bytes);
	if (status < 0) {
		LOG_DBG("Failed to read block");
		return -EIO;
	}

	k_msleep(BQ40ZXX_SUBCLASS_DELAY);

	return 0;
}

/*******************************************************************
 * ********************** BQ40ZXX MAC functions ********************
 * *****************************************************************/
static int bq40zxx_get_device_type(struct bq40zxx_data *bq40zxx,
        uint16_t *val)
{
	int status;
    uint8_t rd_buf[MAC_READ_BLOCK_SZ(2)];

	status = bq40zxx_control_reg_write(bq40zxx,
            BQ40ZXX_CONTROL_DEVICE_TYPE);
	if (status < 0) {
		LOG_DBG("Unable to write control register");
		return -EIO;
	}

    status = smbus_block_read(bq40zxx,
            BQ40ZXX_COMMAND_MANUFACTURER_BLOCK_ACCESS,
            rd_buf, sizeof(rd_buf));
	if (status < 0) {
		LOG_DBG("Unable to read register");
		return -EIO;
	}

    LOG_INF("0x%x, 0x%x, 0x%x, 0x%x, 0x%x", rd_buf[0], rd_buf[1],
            rd_buf[2], rd_buf[3], rd_buf[4]);
    *val = (rd_buf[4] << 8) | rd_buf[3];

	return 0;
}

static int bq40zxx_command_block_read(struct bq40zxx_data *bq40zxx,
        uint16_t mac_sub_command, uint32_t* val, size_t val_sz)
{
	int status;
    uint8_t rd_buf[MAC_READ_BLOCK_SZ(val_sz)];

	status = bq40zxx_control_reg_write(bq40zxx,
            mac_sub_command);
	if (status < 0) {
		LOG_DBG("Unable to write control register");
		return -EIO;
	}

    status = smbus_block_read(bq40zxx,
            BQ40ZXX_COMMAND_MANUFACTURER_BLOCK_ACCESS,
            rd_buf, sizeof(rd_buf));
	if (status < 0) {
		LOG_DBG("Unable to read register");
		return -EIO;
	}

    LOG_DBG("0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0%x", rd_buf[0], rd_buf[1],
            rd_buf[2], rd_buf[3], rd_buf[4], rd_buf[5], rd_buf[6]);

    if ((rd_buf[0] - MAC_READ_OVERHEAD) == 4) {
        *((uint32_t* )val) = (rd_buf[6] << 24) | (rd_buf[5] << 16) |
            (rd_buf[4] << 8) | rd_buf[3];
    }
    else if ((rd_buf[0] - MAC_READ_OVERHEAD) == 3) {
        *((uint32_t* )val) = (rd_buf[5] << 16) |
            (rd_buf[4] << 8) | rd_buf[3];
    }
    else if ((rd_buf[0] - MAC_READ_OVERHEAD) == 2) {
        *((uint32_t* )val) = (rd_buf[4] << 8) | rd_buf[3];
    }
    else if ((rd_buf[0] - MAC_READ_OVERHEAD) == 1) {
        *((uint32_t* )val) = rd_buf[3];
    }

	return 0;

}

static int bq40zxx_print_fw_ver(struct bq40zxx_data *bq40zxx)
{
	int status;
    uint8_t buf[32];

	status = bq40zxx_control_reg_write(bq40zxx,
            BQ40ZXX_CONTROL_FW_VERSION);
	if (status < 0) {
		LOG_DBG("Unable to write control register");
		return -EIO;
	}

    status = smbus_block_read(bq40zxx,
            BQ40ZXX_COMMAND_MANUFACTURER_BLOCK_ACCESS,
            buf, 14);
	if (status < 0) {
		LOG_DBG("Failed to read from block access register");
		return -EIO;
	}

    //LOG_DBG("0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x", buf[0],
    //        buf[1], buf[9], buf[10], buf[11], buf[12]);
    LOG_DBG("FW Ver:%04X, Build:%04X\n", (buf[2] << 8) | buf[3],
            (buf[4] << 8) | buf[5]);
    LOG_DBG("Ztrack Ver:%04X\n", (buf[7] << 8) | buf[8]);

    return 0;
}

/*******************************************************************
 * ********************** BQ40ZXX data read/write ******************
 * *****************************************************************/
static int bq40zxx_command_reg_read(struct bq40zxx_data *bq40zxx, uint8_t command,
        uint8_t *val, size_t val_sz)
{
	int status;
    uint8_t rd_buf[READ_BLOCK_SZ(val_sz)];

    //memset(rd_buf, 0, READ_BLOCK_SZ(val_sz));
    status = smbus_block_read(bq40zxx, command,
                rd_buf, sizeof(rd_buf));
	if (status < 0) {
		LOG_DBG("Failed to read from command register");
		return -EIO;
	}

    if (val_sz == 1) {
        //LOG_DBG("0x%x", rd_buf[0]);
        *val = rd_buf[0];
    }
    if(val_sz == 2) {
        //LOG_DBG("0x%x, 0x%x", rd_buf[0], rd_buf[1]);
        *((uint16_t* )val) = (rd_buf[1] << 8) | rd_buf[0];
    }
    return 0; 
}

static int bq40zxx_command_reg_write(struct bq40zxx_data *bq40zxx, uint8_t command,
				     uint8_t data)
{
	uint8_t rd_buf, reg_addr;
	int status = 0;

	reg_addr = command;
	rd_buf = data;

	status = i2c_reg_write_byte(bq40zxx->i2c, DT_INST_REG_ADDR(0),
            reg_addr, rd_buf);
	if (status < 0) {
		LOG_DBG("Failed to write into control register");
		return -EIO;
	}

	return 0;
}

/**
 * @brief sensor value get
 *
 * @return -ENOTSUP for unsupported channels
 */
static int bq40zxx_channel_get(struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct bq40zxx_data *bq40zxx = dev->data;
	float int_temp;

	switch (chan) {
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		val->val1 = ((bq40zxx->voltage / 1000));
		val->val2 = ((bq40zxx->voltage % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_AVG_CURRENT:
		val->val1 = ((bq40zxx->avg_current / 1000));
		val->val2 = ((bq40zxx->avg_current % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_TIME_TO_EMPTY:
		val->val1 = ((bq40zxx->time_to_empty / 1000));
		val->val2 = ((bq40zxx->time_to_empty % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_TIME_TO_FULL:
		val->val1 = ((bq40zxx->time_to_full / 1000));
		val->val2 = ((bq40zxx->time_to_full % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_TEMP:
		int_temp = (bq40zxx->internal_temperature * 0.1);
		int_temp = int_temp - 273.15;
		val->val1 = (int)int_temp;
		val->val2 = (int_temp - (int)int_temp) * 1000000;
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
		val->val1 = bq40zxx->state_of_charge;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_HEALTH:
		val->val1 = bq40zxx->state_of_health;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY:
		val->val1 = (bq40zxx->full_charge_capacity / 1000);
		val->val2 = ((bq40zxx->full_charge_capacity % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY:
		val->val1 = (bq40zxx->remaining_charge_capacity / 1000);
		val->val2 =
			((bq40zxx->remaining_charge_capacity % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_CYCLE_COUNT:
		val->val1 = bq40zxx->cycle_count;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_DESIGN_CAPACITY:
		val->val1 = (bq40zxx->design_capacity / 1000);
		val->val2 = ((bq40zxx->design_capacity % 1000) * 1000U);
		break;

	case SENSOR_CHAN_GAUGE_SAFETY_ALERT:
		val->val1 = bq40zxx->safety_alert;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_SAFETY_STATUS:
		val->val1 = bq40zxx->safety_status;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_PF_ALERT:
		val->val1 = bq40zxx->pf_alert;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_PF_STATUS:
		val->val1 = bq40zxx->pf_status;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_GAUGING_STATUS:
		val->val1 = bq40zxx->gauging_status;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_CHARGING_STATUS:
		val->val1 = bq40zxx->ch_status;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_OPERATING_STATUS:
		val->val1 = bq40zxx->op_status;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_MANUFACTURING_STATUS:
		val->val1 = bq40zxx->mfg_status;
		val->val2 = 0;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int bq40zxx_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct bq40zxx_data *bq40zxx = dev->data;
	int status = 0;

	switch (chan) {
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		status = bq40zxx_command_reg_read(bq40zxx,
                BQ40ZXX_COMMAND_VOLTAGE, &bq40zxx->voltage, U2);
		if (status < 0) {
			LOG_DBG("Failed to read voltage");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_AVG_CURRENT:
		status = bq40zxx_command_reg_read(bq40zxx,
						  BQ40ZXX_COMMAND_AVG_CURRENT,
						  &bq40zxx->avg_current, I2);
		if (status < 0) {
			LOG_DBG("Failed to read average current ");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_TEMP:
		status = bq40zxx_command_reg_read(
			bq40zxx, BQ40ZXX_COMMAND_TEMP,
			&bq40zxx->internal_temperature, U2);
		if (status < 0) {
			LOG_DBG("Failed to read internal temperature");
			return -EIO;
		}
		break;

    case SENSOR_CHAN_GAUGE_FLAGS:
		status = bq40zxx_command_reg_read(bq40zxx,
						  BQ40ZXX_COMMAND_FLAGS,
						  &bq40zxx->flags, U2);
		if (status < 0) {
			LOG_DBG("Failed to read flags");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_TIME_TO_EMPTY:
		status = bq40zxx_command_reg_read(bq40zxx,
						  BQ40ZXX_COMMAND_TIME_TO_EMPTY,
						  &bq40zxx->time_to_empty, U2);
		if (status < 0) {
			LOG_DBG("Failed to read time to empty");
			return -EIO;
		}
		break;

    case SENSOR_CHAN_GAUGE_TIME_TO_FULL:
		status = bq40zxx_command_reg_read(bq40zxx,
						  BQ40ZXX_COMMAND_TIME_TO_FULL,
						  &bq40zxx->time_to_full, U2);
		if (status < 0) {
			LOG_DBG("Failed to read time to full");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
		status = bq40zxx_command_reg_read(bq40zxx, 
                          BQ40ZXX_COMMAND_SOC,
						  &bq40zxx->state_of_charge, U1);
		if (status < 0) {
			LOG_DBG("Failed to read state of charge");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_CYCLE_COUNT:
		status = bq40zxx_command_reg_read(bq40zxx, 
                          BQ40ZXX_COMMAND_CYCLE_COUNT,
						  &bq40zxx->cycle_count, U2);
		if (status < 0) {
			LOG_DBG("Failed to read cycle count");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY:
		status = bq40zxx_command_reg_read(
			bq40zxx, BQ40ZXX_COMMAND_FULL_CAPACITY,
			&bq40zxx->full_charge_capacity, U2);
		if (status < 0) {
			LOG_DBG("Failed to read full charge capacity");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY:
		status = bq40zxx_command_reg_read(
			bq40zxx, BQ40ZXX_COMMAND_REM_CAPACITY,
			&bq40zxx->remaining_charge_capacity, U2);
		if (status < 0) {
			LOG_DBG("Failed to read remaining charge capacity");
			return -EIO;
		}
		break;

	case SENSOR_CHAN_GAUGE_STATE_OF_HEALTH:
		status = bq40zxx_command_reg_read(bq40zxx, BQ40ZXX_COMMAND_SOH,
						  &bq40zxx->state_of_health, U2);

		bq40zxx->state_of_health = bq40zxx->state_of_health;

		if (status < 0) {
			LOG_DBG("Failed to read state of health");
			return -EIO;
		}
		break;

    case SENSOR_CHAN_GAUGE_DESIGN_CAPACITY:
		status = bq40zxx_command_reg_read(bq40zxx,
                          BQ40ZXX_COMMAND_DESIGN_CAPACITY,
						  &bq40zxx->design_capacity, U2);
		if (status < 0) {
			LOG_DBG("Failed to read design capacity");
			return -EIO;
		}
		break;

    case SENSOR_CHAN_GAUGE_SAFETY_ALERT:
		status = bq40zxx_command_block_read(bq40zxx,
                          BQ40ZXX_COMMAND_SAFETY_ALERT,
						  &bq40zxx->safety_alert, U4);
		if (status < 0) {
			LOG_DBG("Failed to read safety alert");
			return -EIO;
		}
		break;

    case SENSOR_CHAN_GAUGE_SAFETY_STATUS:
		status = bq40zxx_command_block_read(bq40zxx,
                          BQ40ZXX_COMMAND_SAFETY_STATUS,
						  &bq40zxx->safety_status, U4);
		if (status < 0) {
			LOG_DBG("Failed to read safety status");
			return -EIO;
		}
		break;

    case SENSOR_CHAN_GAUGE_PF_ALERT:
		status = bq40zxx_command_block_read(bq40zxx,
                          BQ40ZXX_COMMAND_PF_ALERT,
						  &bq40zxx->pf_alert, U4);
		if (status < 0) {
			LOG_DBG("Failed to read pf alert");
			return -EIO;
		}
		break;

    case SENSOR_CHAN_GAUGE_PF_STATUS:
		status = bq40zxx_command_block_read(bq40zxx,
                          BQ40ZXX_COMMAND_PF_STATUS,
						  &bq40zxx->pf_status, U4);
		if (status < 0) {
			LOG_DBG("Failed to read pf status");
			return -EIO;
		}
		break;

    case SENSOR_CHAN_GAUGE_OPERATING_STATUS:
		status = bq40zxx_command_block_read(bq40zxx,
                          BQ40ZXX_COMMAND_OPERATION_STATUS,
						  &bq40zxx->op_status, U4);
		if (status < 0) {
			LOG_DBG("Failed to read op status");
			return -EIO;
		}
		break;

    case SENSOR_CHAN_GAUGE_GAUGING_STATUS:
		status = bq40zxx_command_block_read(bq40zxx,
                          BQ40ZXX_COMMAND_GAUGING_STATUS,
						  &bq40zxx->gauging_status, U4);
		if (status < 0) {
			LOG_DBG("Failed to read gauging status");
			return -EIO;
		}
		break;

    case SENSOR_CHAN_GAUGE_CHARGING_STATUS:
		status = bq40zxx_command_block_read(bq40zxx,
                          BQ40ZXX_COMMAND_CHARGING_STATUS,
						  &bq40zxx->ch_status, U4);
		if (status < 0) {
			LOG_DBG("Failed to read charging status");
			return -EIO;
		}
		break;

    case SENSOR_CHAN_GAUGE_MANUFACTURING_STATUS:
		status = bq40zxx_command_block_read(bq40zxx,
                          BQ40ZXX_COMMAND_MANUFACTURING_STATUS,
						  &bq40zxx->mfg_status, U4);
		if (status < 0) {
			LOG_DBG("Failed to read mfg status");
			return -EIO;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief initialise the fuel gauge
 *
 * @return 0 for success
 */
static int bq40zxx_gauge_init(struct device *dev)
{
	struct bq40zxx_data *bq40zxx = dev->data;
	const struct bq40zxx_config *const config = dev->config;
	int status = 0;
	uint8_t tmp_checksum = 0, checksum_old = 0, checksum_new = 0;
	uint16_t flags = 0, designenergy_mwh = 0, taperrate = 0, id = 0;
	uint8_t designcap_msb, designcap_lsb, designenergy_msb, designenergy_lsb,
		terminatevolt_msb, terminatevolt_lsb, taperrate_msb,
		taperrate_lsb;
	uint8_t block[32];

	designenergy_mwh = (uint16_t)3.7 * config->design_capacity;
	taperrate =
		(uint16_t)config->design_capacity / (0.1 * config->taper_current);

	bq40zxx->i2c = device_get_binding(config->bus_name);
	if (bq40zxx == NULL) {
		LOG_DBG("Could not get pointer to %s device.",
			config->bus_name);
		return -EINVAL;
	}

	status = bq40zxx_get_device_type(bq40zxx, &id);
	if (status < 0) {
		LOG_DBG("Unable to get device ID");
		//return -EIO;
	}

	if (id != BQ40ZXX_DEVICE_ID) {
		LOG_DBG("Invalid Device 0x%x", id);
		//return -EINVAL;
	}

    LOG_INF("BQ40z50 id: %d", id);

	/** Print firmware version **/
    status = bq40zxx_print_fw_ver(bq40zxx);
	if (status < 0) {
		LOG_DBG("Unable to print firmware version");
        /* FIXME Hack */
		//return -EIO;
	}

#if 0
	/** Unseal the battery control register **/
	status = bq40zxx_control_reg_write(bq40zxx, BQ40ZXX_UNSEAL_KEY_1);
	if (status < 0) {
		LOG_DBG("Unable to unseal the battery");
		return -EIO;
	}

	status = bq40zxx_control_reg_write(bq40zxx, BQ40ZXX_UNSEAL_KEY_2);
	if (status < 0) {
		LOG_DBG("Unable to unseal the battery");
		return -EIO;
	}
#endif

#if 0
	/** Step to place the Gauge into CONFIG UPDATE Mode **/
	do {
		status = bq40zxx_command_reg_read(
			bq40zxx, BQ40ZXX_COMMAND_FLAGS, &flags);
		if (status < 0) {
			LOG_DBG("Unable to read flags");
			return -EIO;
		}

		if (!(flags & 0x0010)) {
			k_msleep(BQ40ZXX_SUBCLASS_DELAY * 10);
		}

	} while (!(flags & 0x0010));

	status = bq40zxx_command_reg_write(bq40zxx,
					   BQ40ZXX_EXTENDED_DATA_CONTROL, 0x00);
	if (status < 0) {
		LOG_DBG("Failed to enable block data memory");
		return -EIO;
	}

	/* Access State subclass */
	status = bq40zxx_command_reg_write(bq40zxx, BQ40ZXX_EXTENDED_DATA_CLASS,
					   0x52);
	if (status < 0) {
		LOG_DBG("Failed to update state subclass");
		return -EIO;
	}

	/* Write the block offset */
	status = bq40zxx_command_reg_write(bq40zxx, BQ40ZXX_EXTENDED_DATA_BLOCK,
					   0x00);
	if (status < 0) {
		LOG_DBG("Failed to update block offset");
		return -EIO;
	}

	for (uint8_t i = 0; i < 32; i++) {
		block[i] = 0;
	}

	status = bq40zxx_read_data_block(bq40zxx, 0x00, block, 32);
	if (status < 0) {
		LOG_DBG("Unable to read block data");
		return -EIO;
	}

	tmp_checksum = 0;
	for (uint8_t i = 0; i < 32; i++) {
		tmp_checksum += block[i];
	}
	tmp_checksum = 255 - tmp_checksum;

	/* Read the block checksum */
	status = i2c_reg_read_byte(bq40zxx->i2c, DT_INST_REG_ADDR(0),
				   BQ40ZXX_EXTENDED_CHECKSUM, &checksum_old);
	if (status < 0) {
		LOG_DBG("Unable to read block checksum");
		return -EIO;
	}

	designcap_msb = config->design_capacity >> 8;
	designcap_lsb = config->design_capacity & 0x00FF;
	designenergy_msb = designenergy_mwh >> 8;
	designenergy_lsb = designenergy_mwh & 0x00FF;
	terminatevolt_msb = config->terminate_voltage >> 8;
	terminatevolt_lsb = config->terminate_voltage & 0x00FF;
	taperrate_msb = taperrate >> 8;
	taperrate_lsb = taperrate & 0x00FF;

	status = i2c_reg_write_byte(bq40zxx->i2c, DT_INST_REG_ADDR(0),
				    BQ40ZXX_EXTENDED_BLOCKDATA_DESIGN_CAP_HIGH,
				    designcap_msb);
	if (status < 0) {
		LOG_DBG("Failed to write designCAP MSB");
		return -EIO;
	}

	status = i2c_reg_write_byte(bq40zxx->i2c, DT_INST_REG_ADDR(0),
				    BQ40ZXX_EXTENDED_BLOCKDATA_DESIGN_CAP_LOW,
				    designcap_lsb);
	if (status < 0) {
		LOG_DBG("Failed to erite designCAP LSB");
		return -EIO;
	}

	status = i2c_reg_write_byte(bq40zxx->i2c, DT_INST_REG_ADDR(0),
				    BQ40ZXX_EXTENDED_BLOCKDATA_DESIGN_ENR_HIGH,
				    designenergy_msb);
	if (status < 0) {
		LOG_DBG("Failed to write designEnergy MSB");
		return -EIO;
	}

	status = i2c_reg_write_byte(bq40zxx->i2c, DT_INST_REG_ADDR(0),
				    BQ40ZXX_EXTENDED_BLOCKDATA_DESIGN_ENR_LOW,
				    designenergy_lsb);
	if (status < 0) {
		LOG_DBG("Failed to erite designEnergy LSB");
		return -EIO;
	}

	status = i2c_reg_write_byte(
		bq40zxx->i2c, DT_INST_REG_ADDR(0),
		BQ40ZXX_EXTENDED_BLOCKDATA_TERMINATE_VOLT_HIGH,
		terminatevolt_msb);
	if (status < 0) {
		LOG_DBG("Failed to write terminateVolt MSB");
		return -EIO;
	}

	status = i2c_reg_write_byte(
		bq40zxx->i2c, DT_INST_REG_ADDR(0),
		BQ40ZXX_EXTENDED_BLOCKDATA_TERMINATE_VOLT_LOW,
		terminatevolt_lsb);
	if (status < 0) {
		LOG_DBG("Failed to write terminateVolt LSB");
		return -EIO;
	}

	status = i2c_reg_write_byte(bq40zxx->i2c, DT_INST_REG_ADDR(0),
				    BQ40ZXX_EXTENDED_BLOCKDATA_TAPERRATE_HIGH,
				    taperrate_msb);
	if (status < 0) {
		LOG_DBG("Failed to write taperRate MSB");
		return -EIO;
	}

	status = i2c_reg_write_byte(bq40zxx->i2c, DT_INST_REG_ADDR(0),
				    BQ40ZXX_EXTENDED_BLOCKDATA_TAPERRATE_LOW,
				    taperrate_lsb);
	if (status < 0) {
		LOG_DBG("Failed to erite taperRate LSB");
		return -EIO;
	}

	for (uint8_t i = 0; i < 32; i++) {
		block[i] = 0;
	}

	status = bq40zxx_read_data_block(bq40zxx, 0x00, block, 32);
	if (status < 0) {
		LOG_DBG("Unable to read block data");
		return -EIO;
	}

	checksum_new = 0;
	for (uint8_t i = 0; i < 32; i++) {
		checksum_new += block[i];
	}
	checksum_new = 255 - checksum_new;

	status = bq40zxx_command_reg_write(bq40zxx, BQ40ZXX_EXTENDED_CHECKSUM,
					   checksum_new);
	if (status < 0) {
		LOG_DBG("Failed to update new checksum");
		return -EIO;
	}

	tmp_checksum = 0;
	status = i2c_reg_read_byte(bq40zxx->i2c, DT_INST_REG_ADDR(0),
				   BQ40ZXX_EXTENDED_CHECKSUM, &tmp_checksum);
	if (status < 0) {
		LOG_DBG("Failed to read checksum");
		return -EIO;
	}

	status = bq40zxx_control_reg_write(bq40zxx, BQ40ZXX_CONTROL_BAT_INSERT);
	if (status < 0) {
		LOG_DBG("Unable to configure BAT Detect");
		return -EIO;
	}

	status = bq40zxx_control_reg_write(bq40zxx, BQ40ZXX_CONTROL_SOFT_RESET);
	if (status < 0) {
		LOG_DBG("Failed to soft reset the gauge");
		return -EIO;
	}

	flags = 0;
	/* Poll Flags   */
	do {
		status = bq40zxx_command_reg_read(
			bq40zxx, BQ40ZXX_COMMAND_FLAGS, &flags);
		if (status < 0) {
			LOG_DBG("Unable to read flags");
			return -EIO;
		}

		if (!(flags & 0x0010)) {
			k_msleep(BQ40ZXX_SUBCLASS_DELAY * 10);
		}
	} while ((flags & 0x0010));
#endif

#if 0
	/* Seal the gauge */
	status = bq40zxx_control_reg_write(bq40zxx, BQ40ZXX_CONTROL_SEALED);
	if (status < 0) {
		LOG_DBG("Failed to seal the gauge");
		return -EIO;
	}
#endif

	return 0;
}

static const struct sensor_driver_api bq40zxx_battery_driver_api = {
	.sample_fetch = bq40zxx_sample_fetch,
	.channel_get = bq40zxx_channel_get,
};

#define BQ40ZXX_INIT(index)                                                    \
	static struct bq40zxx_data bq40zxx_driver_##index;                     \
									       \
	static const struct bq40zxx_config bq40zxx_config_##index = {          \
		.bus_name = DT_INST_BUS_LABEL(index),                          \
		.design_voltage = DT_INST_PROP(index, design_voltage),         \
		.design_capacity = DT_INST_PROP(index, design_capacity),       \
		.taper_current = DT_INST_PROP(index, taper_current),           \
		.terminate_voltage = DT_INST_PROP(index, terminate_voltage),   \
	};                                                                     \
                        									       \
	DEVICE_DT_INST_DEFINE(index, &bq40zxx_gauge_init,		       \
			    PM_DEVICE_DT_INST_GET(index),		       \
			    &bq40zxx_driver_##index,                           \
			    &bq40zxx_config_##index, POST_KERNEL,              \
			    CONFIG_SENSOR_INIT_PRIORITY,                       \
			    &bq40zxx_battery_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BQ40ZXX_INIT)
