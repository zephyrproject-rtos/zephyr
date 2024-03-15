/*
 * Copyright 2023 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for SBS 1.1 compliant smart battery charger.
 */

#define DT_DRV_COMPAT sbs_sbs_charger

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "sbs_charger.h"

LOG_MODULE_REGISTER(sbs_sbs_charger);

/** Static configuration for the emulator */
struct sbs_charger_emul_cfg {
	/** I2C address of emulator */
	uint16_t addr;
};

/** Run-time data used by the emulator */
struct sbs_charger_emul_data {
	uint16_t reg_charger_mode;
};

static int emul_sbs_charger_reg_write(const struct emul *target, int reg, int val)
{
	struct sbs_charger_emul_data *data = target->data;

	LOG_INF("write %x = %x", reg, val);
	switch (reg) {
	case SBS_CHARGER_REG_CHARGER_MODE:
		data->reg_charger_mode = val;
		break;
	default:
		LOG_ERR("Unknown write %x", reg);
		return -EIO;
	}

	return 0;
}

static int emul_sbs_charger_reg_read(const struct emul *target, int reg, int *val)
{
	switch (reg) {
	case SBS_CHARGER_REG_SPEC_INFO:
	case SBS_CHARGER_REG_CHARGER_MODE:
	case SBS_CHARGER_REG_STATUS:
	case SBS_CHARGER_REG_ALARM_WARNING:
		/* Arbitrary stub value. */
		*val = 1;
		break;
	default:
		LOG_ERR("Unknown register 0x%x read", reg);
		return -EIO;
	}
	LOG_INF("read 0x%x = 0x%x", reg, *val);

	return 0;
}

static int sbs_charger_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs,
					 int num_msgs, int addr)
{
	/* Largely copied from emul_sbs_gauge.c */
	struct sbs_charger_emul_data *data;
	unsigned int val;
	int reg;
	int rc;

	data = target->data;

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);
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
				rc = emul_sbs_charger_reg_read(target, reg, &val);
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

			rc = emul_sbs_charger_reg_write(target, reg, value);
		}
		break;
	default:
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	return rc;
}

static const struct i2c_emul_api sbs_charger_emul_api_i2c = {
	.transfer = sbs_charger_emul_transfer_i2c,
};

static int emul_sbs_sbs_charger_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);

	return 0;
}

/*
 * Main instantiation macro. SBS Charger Emulator only implemented for I2C
 */
#define SBS_CHARGER_EMUL(n)                                                                        \
	static struct sbs_charger_emul_data sbs_charger_emul_data_##n;                             \
                                                                                                   \
	static const struct sbs_charger_emul_cfg sbs_charger_emul_cfg_##n = {                      \
		.addr = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, emul_sbs_sbs_charger_init, &sbs_charger_emul_data_##n,              \
			    &sbs_charger_emul_cfg_##n, &sbs_charger_emul_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(SBS_CHARGER_EMUL)
