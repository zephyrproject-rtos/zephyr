/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pca9555

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "pca9555_emul.h"

#define PCA9555_REGISTER_COUNT 8

struct pca9555_emul_data {
	uint8_t registers[PCA9555_REGISTER_COUNT];
};

static int pca9555_emul_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				 int addr)
{
	struct pca9555_emul_data *data = target->data;
	uint8_t reg;

	ARG_UNUSED(addr);

	if (num_msgs != 2 || msgs[0].len != 1 || (msgs[0].flags & I2C_MSG_READ) != 0) {
		return -EIO;
	}

	reg = msgs[0].buf[0];
	if (reg >= PCA9555_REGISTER_COUNT || msgs[1].len > PCA9555_REGISTER_COUNT - reg) {
		return -EIO;
	}

	if ((msgs[1].flags & I2C_MSG_READ) != 0) {
		memcpy(msgs[1].buf, &data->registers[reg], msgs[1].len);
	} else {
		memcpy(&data->registers[reg], msgs[1].buf, msgs[1].len);
	}

	return 0;
}

uint16_t pca9555_emul_get_word(const struct emul *target, uint8_t reg)
{
	struct pca9555_emul_data *data = target->data;

	return sys_get_le16(&data->registers[reg]);
}

void pca9555_emul_set_word(const struct emul *target, uint8_t reg, uint16_t value)
{
	struct pca9555_emul_data *data = target->data;

	sys_put_le16(value, &data->registers[reg]);
}

static int pca9555_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);

	return 0;
}

static const struct i2c_emul_api pca9555_emul_api = {
	.transfer = pca9555_emul_transfer,
};

#define PCA9555_EMUL(inst)                                                                         \
	static struct pca9555_emul_data pca9555_emul_data_##inst = {                               \
		.registers =                                                                       \
			{                                                                          \
				[0x00] = 0x5a,                                                     \
				[0x01] = 0xa5,                                                     \
				[0x02] = 0x34,                                                     \
				[0x03] = 0x12,                                                     \
				[0x04] = (PCA9555_EMUL_INITIAL_POLARITY & 0xff),                   \
				[0x05] = (PCA9555_EMUL_INITIAL_POLARITY >> 8),                     \
				[0x06] = 0x0f,                                                     \
				[0x07] = 0xf0,                                                     \
			},                                                                         \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(inst, pca9555_emul_init, &pca9555_emul_data_##inst, NULL,              \
			    &pca9555_emul_api, NULL)

DT_INST_FOREACH_STATUS_OKAY(PCA9555_EMUL)
