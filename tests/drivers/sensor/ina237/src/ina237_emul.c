/*
 * Copyright (c) 2023 North River Systems Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for the TI INA237 I2C power monitor
 */
#define DT_DRV_COMPAT ti_ina237

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <ina237.h>
#include <ina237_emul.h>

LOG_MODULE_REGISTER(INA237_EMUL, CONFIG_SENSOR_LOG_LEVEL);

/* Register ID, size, and value table */
struct ina237_reg {
	uint8_t id;
	uint8_t bytes;
	uint32_t value;
};

/* Emulator configuration passed into driver instance */
struct ina237_emul_cfg {
	uint16_t addr;
};

struct ina237_emul_data {
	struct ina237_reg ina237_regs[INA237_REGISTER_COUNT];
};

static struct ina237_reg *get_register(struct ina237_emul_data *data, int reg)
{
	for (int i = 0; i < INA237_REGISTER_COUNT; i++) {
		if (data->ina237_regs[i].id == reg) {
			return &data->ina237_regs[i];
		}
	}
	return NULL;
}

int ina237_mock_set_register(void *data_ptr, int reg, uint32_t value)
{
	struct ina237_reg *reg_ptr = get_register(data_ptr, reg);

	if (reg_ptr == NULL) {
		return -EINVAL;
	}

	reg_ptr->value = value;
	return 0;
}

int ina237_mock_get_register(void *data_ptr, int reg, uint32_t *value_ptr)
{
	struct ina237_reg *reg_ptr = get_register(data_ptr, reg);

	if (reg_ptr == NULL || value_ptr == NULL) {
		return -EINVAL;
	}

	*value_ptr = reg_ptr->value;
	return 0;
}

static int ina237_emul_transfer_i2c(const struct emul *target, struct i2c_msg msgs[], int num_msgs,
				    int addr)
{
	struct ina237_emul_data *data = (struct ina237_emul_data *)target->data;

	/* The INA237 uses big-endian read 16, read 24, and write 16 transactions */
	if (!msgs || num_msgs < 1 || num_msgs > 2) {
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	if (msgs[0].flags & I2C_MSG_READ) {
		LOG_ERR("Expected write");
		return -EIO;
	}

	if (num_msgs == 1) {
		/* Write16 Transaction */
		if (msgs[0].len != 3) {
			LOG_ERR("Expected 3 bytes");
			return -EIO;
		}

		/* Write 2 bytes */
		uint8_t reg = msgs[0].buf[0];
		uint16_t val = sys_get_be16(&msgs[0].buf[1]);

		struct ina237_reg *reg_ptr = get_register(data, reg);

		if (!reg_ptr) {
			LOG_ERR("Invalid register: %02x", reg);
			return -EIO;
		}
		reg_ptr->value = val;
		LOG_DBG("Write reg %02x: %04x", reg, val);
	} else {
		/* Read 2 or 3 bytes */
		if ((msgs[1].flags & I2C_MSG_READ) == I2C_MSG_WRITE) {
			LOG_ERR("Expected read");
			return -EIO;
		}
		uint8_t reg = msgs[0].buf[0];

		struct ina237_reg *reg_ptr = get_register(data, reg);

		if (!reg_ptr) {
			LOG_ERR("Invalid register: %02x", reg);
			return -EIO;
		}

		if (msgs[1].len == 2) {
			sys_put_be16(reg_ptr->value, msgs[1].buf);
			LOG_DBG("Read16 reg %02x: %04x", reg, reg_ptr->value);
		} else if (msgs[1].len == 3) {
			sys_put_be24(reg_ptr->value, msgs[1].buf);
			LOG_DBG("Read24 reg %02x: %06x", reg, reg_ptr->value);
		} else {
			LOG_ERR("Invalid read length: %d", msgs[1].len);
			return -EIO;
		}
	}

	return 0;
}

static int ina237_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);

	return 0;
}

static const struct i2c_emul_api ina237_emul_api_i2c = {
	.transfer = ina237_emul_transfer_i2c,
};

#define INA237_EMUL(n)									\
	static const struct ina237_emul_cfg ina237_emul_cfg_##n = {			\
		.addr = DT_INST_REG_ADDR(n)						\
	};										\
	static struct ina237_emul_data ina237_emul_data_##n = {				\
		.ina237_regs = {							\
			{INA237_REG_CONFIG, 2, 0},					\
			{INA237_REG_ADC_CONFIG, 2, 0xFB68},				\
			{INA237_REG_CALIB, 2, 0x1000},					\
			{INA237_REG_SHUNT_VOLT, 2, 0},					\
			{INA237_REG_BUS_VOLT, 2, 0},					\
			{INA237_REG_DIETEMP, 2, 0},					\
			{INA237_REG_CURRENT, 2, 0},					\
			{INA237_REG_POWER, 3, 0},					\
			{INA237_REG_ALERT, 2, 0x0001},					\
			{INA237_REG_SOVL, 2, 0x7FFF},					\
			{INA237_REG_SUVL, 2, 0x8000},					\
			{INA237_REG_BOVL, 2, 0x7FFF},					\
			{INA237_REG_BUVL, 2, 0},					\
			{INA237_REG_TEMP_LIMIT, 2, 0x7FFF},				\
			{INA237_REG_PWR_LIMIT, 2, 0xFFFF},				\
			{INA237_REG_MANUFACTURER_ID, 2, INA2XX_MANUFACTURER_ID},	\
		}};									\
	EMUL_DT_INST_DEFINE(n, ina237_emul_init, &ina237_emul_data_##n, &ina237_emul_cfg_##n, \
			    &ina237_emul_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(INA237_EMUL)
