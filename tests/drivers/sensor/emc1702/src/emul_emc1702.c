/*
 * Copyright (c) 2026 Microchip Technology Inc. and its subsidiaries
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "emc1702.h"
#include "emul_emc1702.h"

#define DT_DRV_COMPAT microchip_emc1702

LOG_MODULE_DECLARE(emc1702, CONFIG_SENSOR_LOG_LEVEL);

/* Total number of registers. */
#define NUM_REGS 256

struct emc1702_emul_data {
	uint64_t reg[NUM_REGS];
};

struct emc1702_emul_cfg {
	uint16_t addr;
};

void emc1702_emul_set_reg_8(const struct emul *target, uint8_t reg_addr, uint16_t value)
{
	struct emc1702_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr < NUM_REGS);

	LOG_DBG("Setting emulated EMC1702 8-bit register %u: Value 0x%04X", reg_addr, value);

	data->reg[reg_addr] = (uint64_t)value;
}

void emc1702_emul_reset(const struct emul *target)
{
	struct emc1702_emul_data *data = target->data;

	LOG_DBG("Resetting EMC1702 emulator registers");

	memset(&data->reg, 0, sizeof(data->reg));
	data->reg[EMC1702_REG_MANUFACTURER_ID] = EMC1702_MANUFACTURER_ID;
	data->reg[EMC1702_REG_PRODUCT_ID] = EMC1702_PRODUCT_ID;
	data->reg[EMC1702_REG_REVISION] = 0x82;
}

static int emc1702_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	LOG_DBG("Initializing EMC1702 emulator");

	emc1702_emul_reset(target);

	return 0;
}

static int emc1702_emul_i2c_write(const struct emul *target, struct i2c_msg msgs[], int num_msgs,
				  int addr)
{
	uint8_t reg_addr;
	uint8_t write_value;
	struct emc1702_emul_data *data = (struct emc1702_emul_data *)target->data;

	if (msgs[0].len != 2) {
		LOG_ERR("Write messages should contain 2 bytes, has %d bytes", msgs[0].len);
		return -EIO;
	}

	reg_addr = msgs[0].buf[0];
	write_value = msgs[0].buf[1];

	if (reg_addr >= NUM_REGS) {
		LOG_ERR("Invalid register address for write: %02X", reg_addr);
		return -EIO;
	}

	data->reg[reg_addr] = write_value;
	LOG_DBG("Write 8 bits to register %u: value 0x%04X via emulated I2C", reg_addr,
		write_value);

	return 0;
}

static int emc1702_emul_i2c_read(const struct emul *target, struct i2c_msg msgs[], int num_msgs,
				 int addr)
{
	uint32_t read_len;
	uint8_t reg_addr;
	struct emc1702_emul_data *data = (struct emc1702_emul_data *)target->data;

	if (!(msgs[1].flags & I2C_MSG_READ)) {
		LOG_ERR("The second I2C message should be of type read");
		return -EIO;
	}
	if (msgs[0].len != 1) {
		LOG_ERR("First message for read should have 1 byte for register address, but has "
			"%d bytes",
			msgs[0].len);
		return -EIO;
	}

	read_len = msgs[1].len;
	reg_addr = msgs[0].buf[0];
	if (reg_addr >= NUM_REGS) {
		LOG_ERR("Invalid register address for read: %u", reg_addr);
		return -EIO;
	}

	if (read_len == 1) {
		msgs[1].buf[0] = (uint8_t)data->reg[reg_addr];
		LOG_DBG("Read 8 bits from register %u: 0x%04X via emulated I2C", reg_addr,
			msgs[1].buf[0]);
		return 0;
	}

	LOG_ERR("Invalid read size for register %u: %u bytes", reg_addr, read_len);
	return -EIO;
}

static int emc1702_emul_transfer_i2c(const struct emul *target, struct i2c_msg msgs[], int num_msgs,
				     int addr)
{
	if (!msgs || num_msgs < 1 || num_msgs > 2) {
		LOG_ERR("Invalid number of I2C messages: %d", num_msgs);
		return -EIO;
	}

	if (msgs[0].flags & I2C_MSG_READ) {
		LOG_ERR("The first I2C message should be write");
		return -EIO;
	}

	if (num_msgs == 1) {
		return emc1702_emul_i2c_write(target, msgs, num_msgs, addr);
	}

	return emc1702_emul_i2c_read(target, msgs, num_msgs, addr);
}

static const struct i2c_emul_api emc1702_emul_api_i2c = {
	.transfer = emc1702_emul_transfer_i2c,
};

#define EMC1702_EMUL(n)                                                                            \
	static const struct emc1702_emul_cfg emc1702_emul_cfg_##n = {.addr = DT_INST_REG_ADDR(n)}; \
	static struct emc1702_emul_data emc1702_emul_data_##n = {0};                               \
	EMUL_DT_INST_DEFINE(n, emc1702_emul_init, &emc1702_emul_data_##n, &emc1702_emul_cfg_##n,   \
			    &emc1702_emul_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(EMC1702_EMUL)
