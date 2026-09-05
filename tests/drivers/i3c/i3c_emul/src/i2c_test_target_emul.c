/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tiny legacy-i2c-on-i3c peripheral emulator. Uses the standard
 * struct i2c_emul_api shape that any in-tree i2c sensor emul (BMI160,
 * AKM09918C, ...) implements. This proves an existing i2c emul sensor
 * can be reparented from a zephyr,i2c-emul-controller to a
 * zephyr,i3c-emul without any sensor-side changes.
 */

#define DT_DRV_COMPAT zephyr_i2c_on_i3c_emul_test_target

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>

#include <string.h>

#include "i2c_test_target_emul.h"

#define I2C_TEST_TARGET_REG_SIZE 16U

struct i2c_test_target_data {
	uint8_t regs[I2C_TEST_TARGET_REG_SIZE];
	uint8_t cursor;
};

static int i2c_test_target_transfer(const struct emul *target, struct i2c_msg *msgs,
				    int num_msgs, int addr)
{
	struct i2c_test_target_data *data = target->data;

	ARG_UNUSED(addr);

	for (int i = 0; i < num_msgs; i++) {
		struct i2c_msg *m = &msgs[i];

		if ((m->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			for (uint32_t j = 0; j < m->len; j++) {
				m->buf[j] = data->regs[data->cursor];
				data->cursor = (data->cursor + 1U) % I2C_TEST_TARGET_REG_SIZE;
			}
		} else {
			if (m->len == 0U) {
				continue;
			}
			data->cursor = m->buf[0] % I2C_TEST_TARGET_REG_SIZE;
			for (uint32_t j = 1; j < m->len; j++) {
				data->regs[data->cursor] = m->buf[j];
				data->cursor = (data->cursor + 1U) % I2C_TEST_TARGET_REG_SIZE;
			}
		}
	}

	return 0;
}

static const struct i2c_emul_api i2c_test_target_api = {
	.transfer = i2c_test_target_transfer,
};

uint8_t i2c_test_target_get_reg(const struct emul *target, uint8_t idx)
{
	struct i2c_test_target_data *data = target->data;

	return data->regs[idx % I2C_TEST_TARGET_REG_SIZE];
}

static int i2c_test_target_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);
	return 0;
}

#define I2C_TEST_TARGET_INIT(n)                                                                    \
	static struct i2c_test_target_data i2c_test_target_data_##n;                               \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &i2c_test_target_data_##n, NULL, POST_KERNEL,         \
			      CONFIG_APPLICATION_INIT_PRIORITY, NULL);                             \
	EMUL_DT_INST_DEFINE(n, i2c_test_target_init, &i2c_test_target_data_##n, NULL,              \
			    &i2c_test_target_api, NULL);

DT_INST_FOREACH_STATUS_OKAY(I2C_TEST_TARGET_INIT)
