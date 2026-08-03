/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT omron_d7s

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "d7s.h"

LOG_MODULE_DECLARE(D7S, CONFIG_SENSOR_LOG_LEVEL);

#define D7S_EMUL_BASIC_COUNT  6
#define D7S_EMUL_RECORD_COUNT 5

struct d7s_emul_data {
	uint8_t basic[D7S_EMUL_BASIC_COUNT];
	uint8_t record[D7S_EMUL_RECORD_COUNT][D7S_RECORD_SIZE];
};

static uint8_t *d7s_emul_reg(const struct emul *target, uint16_t addr)
{
	struct d7s_emul_data *data = target->data;
	uint16_t page = addr >> 8;
	uint8_t offset = addr & 0xFFU;

	if (page == 0x10U && offset < D7S_EMUL_BASIC_COUNT) {
		return &data->basic[offset];
	}

	if (page >= 0x30U && page < 0x30U + D7S_EMUL_RECORD_COUNT && offset < D7S_RECORD_SIZE) {
		return &data->record[page - 0x30U][offset];
	}

	return NULL;
}

static int d7s_emul_read(const struct emul *target, uint16_t addr, uint8_t *buf, size_t len)
{
	struct d7s_emul_data *data = target->data;

	for (size_t i = 0; i < len; i++) {
		uint8_t *reg = d7s_emul_reg(target, addr + i);

		if (reg == NULL) {
			LOG_ERR("emul: read from unmapped register 0x%04x",
				(unsigned int)(addr + i));
			return -EIO;
		}

		buf[i] = *reg;

		/* The EVENT flags are cleared by the read that reports them. */
		if (addr + i == D7S_REG_EVENT) {
			data->basic[D7S_REG_EVENT & 0xFFU] = 0;
		}
	}

	return 0;
}

static int d7s_emul_write(const struct emul *target, uint16_t addr, const uint8_t *buf, size_t len)
{
	struct d7s_emul_data *data = target->data;

	for (size_t i = 0; i < len; i++) {
		uint8_t *reg = d7s_emul_reg(target, addr + i);

		if (reg == NULL) {
			LOG_ERR("emul: write to unmapped register 0x%04x",
				(unsigned int)(addr + i));
			return -EIO;
		}

		switch (addr + i) {
		case D7S_REG_MODE:
			*reg = buf[i] & D7S_MODE_MASK;
			/*
			 * The sensor runs the requested mode and returns to
			 * Normal Mode on its own, which the model collapses
			 * into an immediate transition.
			 */
			data->basic[D7S_REG_STATE & 0xFFU] = D7S_STATE_NORMAL_STANDBY;
			data->basic[D7S_REG_MODE & 0xFFU] = D7S_MODE_NORMAL;
			break;
		case D7S_REG_CLEAR_COMMAND:
			if ((buf[i] & BIT(0)) != 0) {
				memset(data->record, 0, sizeof(data->record));
			}
			*reg = 0;
			break;
		case D7S_REG_STATE:
		case D7S_REG_AXIS_STATE:
		case D7S_REG_EVENT:
			return -EIO;
		default:
			*reg = buf[i];
			break;
		}
	}

	return 0;
}

static int d7s_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				 int addr)
{
	uint16_t reg;

	ARG_UNUSED(addr);

	if (num_msgs < 1 || (msgs[0].flags & I2C_MSG_READ) != 0 || msgs[0].len < 2) {
		return -EIO;
	}

	reg = sys_get_be16(msgs[0].buf);

	if (num_msgs == 1) {
		return d7s_emul_write(target, reg, &msgs[0].buf[2], msgs[0].len - 2);
	}

	if (num_msgs == 2 && (msgs[1].flags & I2C_MSG_READ) != 0) {
		return d7s_emul_read(target, reg, msgs[1].buf, msgs[1].len);
	}

	return -EIO;
}

void d7s_emul_set_record(const struct emul *target, uint8_t index, uint16_t si, uint16_t pga,
			 int16_t temp)
{
	struct d7s_emul_data *data = target->data;

	__ASSERT_NO_MSG(index < D7S_EMUL_RECORD_COUNT);

	sys_put_be16(si, &data->record[index][D7S_RECORD_SI]);
	sys_put_be16(pga, &data->record[index][D7S_RECORD_PGA]);
	sys_put_be16((uint16_t)temp, &data->record[index][D7S_RECORD_T_AVE]);
}

void d7s_emul_set_event(const struct emul *target, uint8_t flags)
{
	struct d7s_emul_data *data = target->data;

	data->basic[D7S_REG_EVENT & 0xFFU] = flags & D7S_EVENT_MASK;
}

void d7s_emul_set_state(const struct emul *target, uint8_t state)
{
	struct d7s_emul_data *data = target->data;

	data->basic[D7S_REG_STATE & 0xFFU] = state & D7S_STATE_MASK;
}

uint8_t d7s_emul_get_ctrl(const struct emul *target)
{
	struct d7s_emul_data *data = target->data;

	return data->basic[D7S_REG_CTRL & 0xFFU];
}

static int d7s_emul_init(const struct emul *target, const struct device *parent)
{
	struct d7s_emul_data *data = target->data;

	ARG_UNUSED(parent);

	memset(data, 0, sizeof(*data));
	data->basic[D7S_REG_STATE & 0xFFU] = D7S_STATE_NORMAL_STANDBY;
	data->basic[D7S_REG_AXIS_STATE & 0xFFU] = D7S_AXIS_MODE_XY;
	data->basic[D7S_REG_MODE & 0xFFU] = D7S_MODE_NORMAL;
	data->basic[D7S_REG_CTRL & 0xFFU] =
		FIELD_PREP(D7S_CTRL_AXIS_MASK, D7S_AXIS_MODE_SWITCH_AT_INSTALL);

	return 0;
}

static const struct i2c_emul_api d7s_emul_api_i2c = {
	.transfer = d7s_emul_transfer_i2c,
};

#define D7S_EMUL_DEFINE(inst)                                                                      \
	static struct d7s_emul_data d7s_emul_data_##inst;                                          \
                                                                                                   \
	EMUL_DT_INST_DEFINE(inst, d7s_emul_init, &d7s_emul_data_##inst, NULL, &d7s_emul_api_i2c,   \
			    NULL)

DT_INST_FOREACH_STATUS_OKAY(D7S_EMUL_DEFINE)
