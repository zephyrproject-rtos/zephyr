/*
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for SBS 1.1 compliant smart battery fuel gauge.
 */

#define DT_DRV_COMPAT sbs_sbs_gauge

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sbs_sbs_gauge);

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sys/byteorder.h>

#include <sbs_gauge.h>

/** Run-time data used by the emulator */
struct sbs_gauge_emul_data {
	/* Stub */
};

/** Static configuration for the emulator */
struct sbs_gauge_emul_cfg {
	/** I2C address of emulator */
	uint16_t addr;
};

static void reg_write(const struct emul *target, int reg, int val)
{
	ARG_UNUSED(target);

	LOG_INF("write %x = %x", reg, val);
	switch (reg) {
	default:
		LOG_INF("Unknown write %x", reg);
	}
}

static int reg_read(const struct emul *target, int reg)
{
	int val;

	ARG_UNUSED(target);

	switch (reg) {
	case SBS_GAUGE_CMD_VOLTAGE:
	case SBS_GAUGE_CMD_AVG_CURRENT:
	case SBS_GAUGE_CMD_TEMP:
	case SBS_GAUGE_CMD_ASOC:
	case SBS_GAUGE_CMD_FULL_CAPACITY:
	case SBS_GAUGE_CMD_REM_CAPACITY:
	case SBS_GAUGE_CMD_NOM_CAPACITY:
	case SBS_GAUGE_CMD_AVG_TIME2EMPTY:
	case SBS_GAUGE_CMD_AVG_TIME2FULL:
	case SBS_GAUGE_CMD_CYCLE_COUNT:
	case SBS_GAUGE_CMD_DESIGN_VOLTAGE:
		/* Arbitrary stub value. */
		val = 1;
		break;
	default:
		LOG_ERR("Unknown register 0x%x read", reg);
		return -EIO;
	}
	LOG_INF("read 0x%x = 0x%x", reg, val);

	return val;
}

static int sbs_gauge_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs,
				       int num_msgs, int addr)
{
	/* Largely copied from emul_bmi160.c */
	struct sbs_gauge_emul_data *data;
	unsigned int val;
	int reg;

	data = target->data;

	__ASSERT_NO_MSG(msgs && num_msgs);

	i2c_dump_msgs("emul", msgs, num_msgs, addr);
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
				val = reg_read(target, reg);
				msgs->buf[0] = val;
				break;
			default:
				LOG_ERR("Unexpected msg1 length %d", msgs->len);
				return -EIO;
			}
		} else {
			if (msgs->len != 1) {
				LOG_ERR("Unexpected msg1 length %d", msgs->len);
			}
			reg_write(target, reg, msgs->buf[0]);
		}
		break;
	default:
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	return 0;
}

static const struct i2c_emul_api sbs_gauge_emul_api_i2c = {
	.transfer = sbs_gauge_emul_transfer_i2c,
};

/**
 * Set up a new SBS_GAUGE emulator (I2C)
 *
 * @param emul Emulation information
 * @param parent Device to emulate (must use sbs_gauge driver)
 * @return 0 indicating success (always)
 */
static int emul_sbs_sbs_gauge_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);

	return 0;
}

/*
 * Main instantiation macro. SBS Gauge Emulator only implemented for I2C
 */
#define SBS_GAUGE_EMUL(n)                                                                          \
	static struct sbs_gauge_emul_data sbs_gauge_emul_data_##n;                                 \
	static const struct sbs_gauge_emul_cfg sbs_gauge_emul_cfg_##n = {                          \
		.addr = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, emul_sbs_sbs_gauge_init, &sbs_gauge_emul_data_##n,                  \
			    &sbs_gauge_emul_cfg_##n, &sbs_gauge_emul_api_i2c)

DT_INST_FOREACH_STATUS_OKAY(SBS_GAUGE_EMUL)
