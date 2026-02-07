/*
 * Copyright (c) 2026 Om Srivastava <srivastavaom97714@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rohm_bh1750

#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/sys/byteorder.h>
#include <errno.h>
#include <stdbool.h>
#include <zephyr/drivers/sensor/bh1750_emul.h>

void bh1750_emul_set_value(const struct device *dev, void *raw_data)
{
	const struct emul *emul = emul_get_binding(dev->name);

	__ASSERT(emul, "No emulator bound to device");

	struct bh1750_emul_data *data =
		(struct bh1750_emul_data *)emul->data;

	struct bh1750_emul_data *temp = raw_data;

	data->raw = temp->raw;
	data->mode = temp->mode;
	data->powered = temp->powered;
}

static int bh1750_emul_transfer(const struct emul *target,
				struct i2c_msg *msgs,
				int num_msgs,
				int addr)
{
	struct bh1750_emul_data *data =
		(struct bh1750_emul_data *)target->data;

	if (data->powered == false) {
		return -EIO;
	}

	/* Write: command */
	if (num_msgs == 1 && !(msgs->flags & I2C_MSG_READ)) {
		uint8_t cmd = msgs->buf[0];

		switch (cmd) {
		case 0x00: /* power down */
			data->powered = false;
			break;
		case 0x01: /* power on */
			data->powered = true;
			break;
		case 0x10: /* continuous high-res */
		case 0x20: /* one-time high-res */
			/* mode ignored for simplicity */
			break;
		default:
			return -EINVAL;
		}
		return 0;
	}

	/* Read: measurement */
	if (num_msgs == 1 && (msgs->flags & I2C_MSG_READ)) {
		if (!data->powered) {
			return -EIO;
		}
		sys_put_be16(data->raw, msgs->buf);
		return 0;
	}

	if (num_msgs == 2 &&
			!(msgs[0].flags & I2C_MSG_READ) &&
			(msgs[1].flags & I2C_MSG_READ) &&
			msgs[1].len == 2) {

		msgs[1].buf[0] = data->raw >> 8;
		msgs[1].buf[1] = data->raw & 0xff;
		return 0;
	}

	return -ENOTSUP;
}

const struct i2c_emul_api bh1750_emul_api = {
	.transfer = bh1750_emul_transfer,
};

static int emul_rohm_bh1750_init(const struct emul *target, const struct device *dev)
{
	struct bh1750_emul_data *data = target->data;

	ARG_UNUSED(dev);

	data->powered = true;
	data->raw = 0;

	return 0;
}

#define BH1750_EMUL_INIT(inst)                          \
	static struct bh1750_emul_data data_##inst = {  \
		.powered = false,                        \
		.mode = 0,                               \
		.raw = 0,                                \
	};                                                \
	static struct i2c_emul i2c_emul_##inst = {            \
		.api = &bh1750_emul_api,                  \
		.addr = DT_INST_REG_ADDR(inst),           \
	};   \
	static struct emul emul_##inst = {   \
		.bus.i2c = &i2c_emul_##inst,              \
	};                                          \
	EMUL_DT_INST_DEFINE(inst, emul_rohm_bh1750_init,    \
	 &data_##inst, &emul_##inst, &bh1750_emul_api, NULL);

DT_INST_FOREACH_STATUS_OKAY(BH1750_EMUL_INIT)
