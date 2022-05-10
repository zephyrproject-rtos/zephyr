/*
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for SBS 1.1 compliant smart battery fuel gauge.
 * This emulator is *NOT* fully featured yet, only enough to get the sbs_gauge driver tests passing.
 */

#define DT_DRV_COMPAT sbs_sbs_gauge

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(sbs_sbs_gauge);

#include <sbs_gauge.h>
#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/sys/byteorder.h>

/** Run-time data used by the emulator */
struct sbs_gauge_emul_data {
	/** I2C emulator detail */
	struct i2c_emul emul_i2c;
	/** sbs_gauge device being emulated */
	const struct device *dev;
	uint8_t pmu_status;
	/** Current register to read (address) */
	uint32_t cur_reg;
};

/** Static configuration for the emulator */
struct sbs_gauge_emul_cfg {
	/** Label of the I2C bus this emulator connects to */
	const char *bus_label;
	/** Chip registers */
	uint8_t *reg;
	union {
		/** I2C address of emulator */
		uint16_t addr;
	};
};

static void reg_write(const struct emul *emu, int regn, int val)
{
	/* Unimplemented reg_write handler */
	struct sbs_gauge_emul_data *data = emu->data;
	const struct sbs_gauge_emul_cfg *cfg = emu->cfg;

	ARG_UNUSED(data);

	LOG_INF("write %x = %x", regn, val);
	cfg->reg[regn] = val;
	switch (regn) {
	default:
		LOG_INF("Unknown write %x", regn);
	}
}

static int reg_read(const struct emul *emu, int regn)
{
	struct sbs_gauge_emul_data *data = emu->data;
	const struct sbs_gauge_emul_cfg *cfg = emu->cfg;
	int val;

	ARG_UNUSED(data);

	LOG_INF("read %x =", regn);
	val = cfg->reg[regn];
	switch (regn) {
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
		val = 1;
		break;
	default:
		LOG_INF("Unknown read %x", regn);
	}
	LOG_INF("       = %x", val);

	return val;
}

static int sbs_gauge_emul_transfer_i2c(struct i2c_emul *emul, struct i2c_msg *msgs, int num_msgs,
				       int addr)
{
	struct sbs_gauge_emul_data *data;
	unsigned int val;

	data = CONTAINER_OF(emul, struct sbs_gauge_emul_data, emul_i2c);

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
		data->cur_reg = msgs->buf[0];

		/* Now process the 'read' part of the message */
		msgs++;
		if (msgs->flags & I2C_MSG_READ) {
			switch (msgs->len - 1) {
			case 1:
				val = reg_read(emul->parent, data->cur_reg);
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
			reg_write(emul->parent, data->cur_reg, msgs->buf[0]);
		}
		break;
	default:
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	return 0;
}

/* Device instantiation below */

static struct i2c_emul_api sbs_gauge_emul_api_i2c = {
	.transfer = sbs_gauge_emul_transfer_i2c,
};

static void emul_sbs_gauge_init(const struct emul *emu, const struct device *parent)
{
	const struct sbs_gauge_emul_cfg *cfg = emu->cfg;
	struct sbs_gauge_emul_data *data = emu->data;
	uint8_t *reg = cfg->reg;

	ARG_UNUSED(reg);

	data->dev = parent;
	data->pmu_status = 0;
}

/**
 * Set up a new SBS_GAUGE emulator (I2C)
 *
 * This should be called for each SBS_GAUGE device that needs to be emulated. It
 * registers it with the i2c emulation controller.
 *
 * @param emul Emulation information
 * @param parent Device to emulate (must use sbs_gauge driver)
 * @return 0 indicating success (always)
 */
static int emul_sbs_sbs_gauge_init_i2c(const struct emul *emu, const struct device *parent)
{
	const struct sbs_gauge_emul_cfg *cfg = emu->cfg;
	struct sbs_gauge_emul_data *data = emu->data;

	emul_sbs_gauge_init(emu, parent);
	data->emul_i2c.api = &sbs_gauge_emul_api_i2c;
	data->emul_i2c.addr = cfg->addr;
	data->emul_i2c.parent = emu;

	int rc = i2c_emul_register(parent, emu->dev_label, &data->emul_i2c);

	return rc;
}

#define SBS_GAUGE_EMUL_DATA(n)                                                                     \
	static uint8_t sbs_gauge_emul_reg_##n[2];                                                  \
	static struct sbs_gauge_emul_data sbs_gauge_emul_data_##n;

#define SBS_GAUGE_EMUL_DEFINE(n, type)                                                             \
	EMUL_DEFINE(emul_sbs_sbs_gauge_init_##type, DT_DRV_INST(n), &sbs_gauge_emul_cfg_##n,       \
		    &sbs_gauge_emul_data_##n)

#define SBS_GAUGE_EMUL_I2C(n)                                                                      \
	SBS_GAUGE_EMUL_DATA(n)                                                                     \
	static const struct sbs_gauge_emul_cfg sbs_gauge_emul_cfg_##n = {                          \
		.bus_label = DT_INST_BUS_LABEL(n),                                                 \
		.reg = sbs_gauge_emul_reg_##n,                                                     \
		.addr = DT_INST_REG_ADDR(n)                                                        \
	};                                                                                         \
	SBS_GAUGE_EMUL_DEFINE(n, i2c)

/*
 * Main instantiation macro. SBS Gauge Emulator only implemented for I2C
 */
#define SBS_GAUGE_EMUL(n) SBS_GAUGE_EMUL_I2C(n)

DT_INST_FOREACH_STATUS_OKAY(SBS_GAUGE_EMUL)
