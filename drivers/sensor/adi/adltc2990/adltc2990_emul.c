/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adltc2990

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "adltc2990.h"
#include "adltc2990_reg.h"
#include "adltc2990_emul.h"

LOG_MODULE_DECLARE(adltc2990, CONFIG_SENSOR_LOG_LEVEL);

#define ADLTC2990_NUM_REGS ADLTC2990_REG_VCC_LSB

struct adltc2990_emul_data {
	uint8_t reg[ADLTC2990_NUM_REGS];
};

struct adltc2990_emul_cfg {
};

void adltc2990_emul_set_reg(const struct emul *target, uint8_t reg_addr, const uint8_t *val)
{
	struct adltc2990_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr <= ADLTC2990_NUM_REGS);
	memcpy(data->reg + reg_addr, val, 1);
}

void adltc2990_emul_get_reg(const struct emul *target, uint8_t reg_addr, uint8_t *val)
{
	struct adltc2990_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr <= ADLTC2990_NUM_REGS);
	memcpy(val, data->reg + reg_addr, 1);
}

void adltc2990_emul_reset(const struct emul *target)
{
	struct adltc2990_emul_data *data = target->data;

	memset(data->reg, 0, ADLTC2990_NUM_REGS);
}

static int adltc2990_emul_handle_write(const struct emul *target, uint8_t regn, uint8_t value)
{
	struct adltc2990_emul_data *data = target->data;

	switch (regn) {
	case ADLTC2990_REG_CONTROL:
		data->reg[ADLTC2990_REG_CONTROL] = value;
		break;

	case ADLTC2990_REG_TRIGGER:
		data->reg[ADLTC2990_REG_TRIGGER] = value;
		break;

	default:
		break;
	}
	return 0;
}

static int adltc2990_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs,
				       int num_msgs, int addr)
{
	struct adltc2990_emul_data *data = target->data;

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	if (num_msgs < 1) {
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}
	if (FIELD_GET(I2C_MSG_READ, msgs->flags)) {
		LOG_ERR("Unexpected read");
		return -EIO;
	}
	if (msgs->len < 1) {
		LOG_ERR("Unexpected msg0 length %d", msgs->len);
		return -EIO;
	}

	uint8_t regn = msgs->buf[0];
	bool is_read = FIELD_GET(I2C_MSG_READ, msgs->flags) == 1;
	bool is_stop = FIELD_GET(I2C_MSG_STOP, msgs->flags) == 1;

	if (!is_stop && !is_read) {
		/* First message was a write with the register number, check next message */
		msgs++;
		is_read = FIELD_GET(I2C_MSG_READ, msgs->flags) == 1;
		is_stop = FIELD_GET(I2C_MSG_STOP, msgs->flags) == 1;
	}

	if (is_read) {
		for (int i = 0; i < msgs->len; ++i) {
			msgs->buf[i] = data->reg[regn + i];
		}
	} else {
		int rc = adltc2990_emul_handle_write(target, regn, msgs->buf[1]);

		if (rc != 0) {
			return rc;
		}
	}
	return 0;
};

static int adltc2990_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);
	adltc2990_emul_reset(target);

	return 0;
}

static const struct i2c_emul_api adltc2990_emul_api_i2c = {
	.transfer = adltc2990_emul_transfer_i2c,
};

#define ADLTC2990_EMUL(n)                                                                          \
	const struct adltc2990_emul_cfg adltc2990_emul_cfg_##n;                                    \
	struct adltc2990_emul_data adltc2990_emul_data_##n;                                        \
	EMUL_DT_INST_DEFINE(n, adltc2990_emul_init, &adltc2990_emul_data_##n,                      \
			    &adltc2990_emul_cfg_##n, &adltc2990_emul_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(ADLTC2990_EMUL)
