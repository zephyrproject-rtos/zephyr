/*
 * Copyright 2023, Alvaro Garcia <maxpowel@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for max17048 fuel gauge
 */


#define DT_DRV_COMPAT maxim_max17048


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(maxim_max17048);

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sys/byteorder.h>

#include "max17048.h"

/** Static configuration for the emulator */
struct max17048_emul_cfg {
	/** I2C address of emulator */
	uint16_t addr;
};

static int emul_max17048_reg_write(const struct emul *target, int reg, int val)
{

	return -EIO;
}

static int emul_max17048_reg_read(const struct emul *target, int reg, int *val)
{

	switch (reg) {
	case REGISTER_VERSION:
	*val = 0x1000;
		break;
	case REGISTER_CRATE:
	*val = 0x4000;
	break;
	case REGISTER_SOC:
	*val = 0x3525;
		break;
	case REGISTER_VCELL:
	*val = 0x4387;
		break;
	default:
		LOG_ERR("Unknown register 0x%x read", reg);
		return -EIO;
	}
	LOG_INF("read 0x%x = 0x%x", reg, *val);

	return 0;
}

static int max17048_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs,
				       int num_msgs, int addr)
{
	/* Largely copied from emul_bmi160.c */
	struct max17048_emul_data *data;
	unsigned int val;
	int reg;
	int rc;

	data = target->data;

	__ASSERT_NO_MSG(msgs && num_msgs);

	i2c_dump_msgs_rw("emul", msgs, num_msgs, addr, false);
	switch (num_msgs) {
	case 2:
		if (msgs->flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected read");
			return -EIO;
		}
		if (msgs->len != 1) {
			LOG_ERR("Unexpected msg0 length %d", msgs->len);
			return -EIO;
		}
		reg = msgs->buf[0];

		/* Now process the 'read' part of the message */
		msgs++;
		if (msgs->flags & I2C_MSG_READ) {
			switch (msgs->len - 1) {
			case 1:
				rc = emul_max17048_reg_read(target, reg, &val);
				if (rc) {
					/* Return before writing bad value to message buffer */
					return rc;
				}

				/* SBS uses SMBus, which sends data in little-endian format. */
				sys_put_le16(val, msgs->buf);
				break;
			default:
				LOG_ERR("Unexpected msg1 length %d", msgs->len);
				return -EIO;
			}
		} else {
			/* We write a word (2 bytes by the SBS spec) */
			if (msgs->len != 2) {
				LOG_ERR("Unexpected msg1 length %d", msgs->len);
			}
			uint16_t value = sys_get_le16(msgs->buf);

			rc = emul_max17048_reg_write(target, reg, value);
		}
		break;
	default:
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	return rc;
}

static const struct i2c_emul_api max17048_emul_api_i2c = {
	.transfer = max17048_emul_transfer_i2c,
};

/**
 * Set up a new emulator (I2C)
 *
 * @param emul Emulation information
 * @param parent Device to emulate
 * @return 0 indicating success (always)
 */
static int emul_max17048_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);

	return 0;
}

/*
 * Main instantiation macro.
 */
#define MAX17048_EMUL(n)                                                                          \
	static const struct max17048_emul_cfg max17048_emul_cfg_##n = {                          \
		.addr = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, emul_max17048_init, NULL,                  \
			    &max17048_emul_cfg_##n, &max17048_emul_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(MAX17048_EMUL)
