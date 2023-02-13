/*
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for SBS 1.1 compliant smart battery fuel gauge.
 */

#ifdef CONFIG_FUEL_GAUGE
#define DT_DRV_COMPAT sbs_sbs_gauge_new_api
#else
#define DT_DRV_COMPAT sbs_sbs_gauge
#endif /* CONFIG_FUEL_GAUGE */

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
	uint16_t mfr_acc;
};

/** Static configuration for the emulator */
struct sbs_gauge_emul_cfg {
	/** I2C address of emulator */
	uint16_t addr;
};

static int emul_sbs_gauge_reg_write(const struct emul *target, int reg, int val)
{
	struct sbs_gauge_emul_data *data = target->data;

	LOG_INF("write %x = %x", reg, val);
	switch (reg) {
	case SBS_GAUGE_CMD_MANUFACTURER_ACCESS:
		data->mfr_acc = val;
		break;
	default:
		LOG_INF("Unknown write %x", reg);
		return -EIO;
	}

	return 0;
}

static int emul_sbs_gauge_reg_read(const struct emul *target, int reg, int *val)
{
	struct sbs_gauge_emul_data *data = target->data;

	switch (reg) {
	case SBS_GAUGE_CMD_MANUFACTURER_ACCESS:
		*val = data->mfr_acc;
		break;
	case SBS_GAUGE_CMD_VOLTAGE:
	case SBS_GAUGE_CMD_AVG_CURRENT:
	case SBS_GAUGE_CMD_TEMP:
	case SBS_GAUGE_CMD_ASOC:
	case SBS_GAUGE_CMD_FULL_CAPACITY:
	case SBS_GAUGE_CMD_REM_CAPACITY:
	case SBS_GAUGE_CMD_NOM_CAPACITY:
	case SBS_GAUGE_CMD_AVG_TIME2EMPTY:
	case SBS_GAUGE_CMD_AVG_TIME2FULL:
	case SBS_GAUGE_CMD_RUNTIME2EMPTY:
	case SBS_GAUGE_CMD_CYCLE_COUNT:
	case SBS_GAUGE_CMD_DESIGN_VOLTAGE:
	case SBS_GAUGE_CMD_CURRENT:
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

static int sbs_gauge_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs,
				       int num_msgs, int addr)
{
	/* Largely copied from emul_bmi160.c */
	struct sbs_gauge_emul_data *data;
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
				rc = emul_sbs_gauge_reg_read(target, reg, &val);
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
			uint16_t *value = (uint16_t *)msgs->buf;

			rc = emul_sbs_gauge_reg_write(target, reg, *value);
		}
		break;
	default:
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	return rc;
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
			    &sbs_gauge_emul_cfg_##n, &sbs_gauge_emul_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(SBS_GAUGE_EMUL)
