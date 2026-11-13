/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for the ITE IT8801 I2C multi-function device.
 */

#define DT_DRV_COMPAT ite_it8801_mfd

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/mfd/emul_ite_it8801.h>
#include <zephyr/drivers/mfd/mfd_ite_it8801.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(emul_ite_it8801, CONFIG_MFD_LOG_LEVEL);

#define IT8801_EMUL_NUM_REGS 256

#define IS_I2C_MSG_WRITE(flags) ((flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE)
#define IS_I2C_MSG_READ(flags)  ((flags & I2C_MSG_RW_MASK) == I2C_MSG_READ)

/* Low bits of KSOMCR select which scan-out line is driven low. */
#define IT8801_EMUL_KSO_SEL_MASK GENMASK(4, 0)

/* Keyboard sub-device register addresses, taken from the kbd child node. */
struct it8801_emul_kbd_regs {
	uint8_t ksomcr;
	uint8_t ksidr;
	uint8_t ksieer;
	uint8_t ksiier;
};

struct it8801_emul_cfg {
	uint16_t addr;
	/* SMB_INT# alert line, driven through the GPIO emulator */
	struct gpio_dt_spec irq_gpio;
	bool has_kbd;
	struct it8801_emul_kbd_regs kbd;
};

struct it8801_emul_data {
	uint8_t regs[IT8801_EMUL_NUM_REGS];
	/* Failure injection: register and number of accesses left to fail */
	uint8_t read_fail_reg;
	int read_fail_count;
	uint8_t write_fail_reg;
	int write_fail_count;
	/* Pressed keys: one bit per KSI line, indexed by KSO line */
	uint8_t key_matrix[IT8801_EMUL_KSO_COUNT];
	/* KSI line levels seen at the last update, used for edge detection */
	uint8_t ksi_level;
};

static void it8801_emul_load_defaults(struct it8801_emul_data *data)
{
	/*
	 * Pre-populate the vendor ID registers so that the real MFD driver
	 * passes its initialization identity check when running against this
	 * emulator. Tests may override these by writing to the emulator.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(it8801_id_verify); i++) {
		data->regs[it8801_id_verify[i].reg] = it8801_id_verify[i].chip_id;
	}
}

/* Active-low KSI line levels for the scan-out lines currently driven. */
static uint8_t it8801_emul_ksi_level(const struct emul *target)
{
	const struct it8801_emul_cfg *cfg = target->cfg;
	struct it8801_emul_data *data = target->data;
	uint8_t ksomcr = data->regs[cfg->kbd.ksomcr];
	uint8_t kso = ksomcr & IT8801_EMUL_KSO_SEL_MASK;
	uint8_t pressed = 0;

	if ((ksomcr & IT8801_REG_MASK_KSOSDIC) != 0) {
		/* Scan-out disabled: no key can pull a KSI line low. */
		pressed = 0;
	} else if ((ksomcr & IT8801_REG_MASK_AKSOSC) != 0) {
		for (size_t i = 0; i < ARRAY_SIZE(data->key_matrix); i++) {
			pressed |= data->key_matrix[i];
		}
	} else if (kso < ARRAY_SIZE(data->key_matrix)) {
		pressed = data->key_matrix[kso];
	}

	return ~pressed;
}

/* Drive SMB_INT# low while a keyboard edge event is pending. */
static void it8801_emul_update_irq(const struct emul *target)
{
	const struct it8801_emul_cfg *cfg = target->cfg;
	struct it8801_emul_data *data = target->data;
	bool pending = data->regs[cfg->kbd.ksieer] != 0;

	gpio_emul_input_set(cfg->irq_gpio.port, cfg->irq_gpio.pin, pending ? 0 : 1);
}

/*
 * Re-evaluate the KSI lines after a key or scan-out change. A falling edge
 * on a line enabled in KSIIER latches into KSIEER and raises the alert.
 */
static void it8801_emul_update_ksi(const struct emul *target)
{
	const struct it8801_emul_cfg *cfg = target->cfg;
	struct it8801_emul_data *data = target->data;
	uint8_t level;
	uint8_t falling;

	if (!cfg->has_kbd) {
		return;
	}

	level = it8801_emul_ksi_level(target);
	falling = data->ksi_level & ~level & data->regs[cfg->kbd.ksiier];
	data->ksi_level = level;

	if ((data->regs[IT8801_REG_GIECR] & IT8801_REG_MASK_GKSIIE) == 0) {
		falling = 0;
	}

	if (falling != 0) {
		data->regs[cfg->kbd.ksieer] |= falling;
		it8801_emul_update_irq(target);
	}
}

void it8801_emul_reset(const struct emul *target)
{
	struct it8801_emul_data *data = target->data;

	memset(data->regs, 0, sizeof(data->regs));
	memset(data->key_matrix, 0, sizeof(data->key_matrix));
	data->ksi_level = 0xFF;
	data->read_fail_count = 0;
	data->write_fail_count = 0;
	it8801_emul_load_defaults(data);
}

int it8801_emul_get_reg(const struct emul *target, uint8_t reg, uint8_t *val)
{
	struct it8801_emul_data *data = target->data;

	if (val == NULL) {
		return -EINVAL;
	}

	*val = data->regs[reg];
	return 0;
}

int it8801_emul_set_reg(const struct emul *target, uint8_t reg, uint8_t val)
{
	struct it8801_emul_data *data = target->data;

	data->regs[reg] = val;
	return 0;
}

void it8801_emul_set_read_fail(const struct emul *target, uint8_t reg, int count)
{
	struct it8801_emul_data *data = target->data;

	data->read_fail_reg = reg;
	data->read_fail_count = count;
}

void it8801_emul_set_write_fail(const struct emul *target, uint8_t reg, int count)
{
	struct it8801_emul_data *data = target->data;

	data->write_fail_reg = reg;
	data->write_fail_count = count;
}

/* Check whether an access to @p reg should fail, consuming one failure. */
static bool it8801_emul_access_fails(uint8_t fail_reg, int *fail_count, uint8_t reg)
{
	if (*fail_count == 0 || reg != fail_reg) {
		return false;
	}

	if (*fail_count > 0) {
		(*fail_count)--;
	}

	return true;
}

int it8801_emul_get_driven_column(const struct emul *target, uint8_t ksomcr_reg)
{
	struct it8801_emul_data *data = target->data;
	uint8_t ksomcr = data->regs[ksomcr_reg];

	/* Scan-out disabled: all outputs high, no column driven. */
	if ((ksomcr & IT8801_REG_MASK_KSOSDIC) != 0) {
		return IT8801_EMUL_COLUMN_NONE;
	}

	/* Assert all scan-out: every column driven low. */
	if ((ksomcr & IT8801_REG_MASK_AKSOSC) != 0) {
		return IT8801_EMUL_COLUMN_ALL;
	}

	return ksomcr & IT8801_EMUL_KSO_SEL_MASK;
}

int it8801_emul_set_key(const struct emul *target, uint8_t kso, uint8_t ksi, bool pressed)
{
	const struct it8801_emul_cfg *cfg = target->cfg;
	struct it8801_emul_data *data = target->data;

	if (!cfg->has_kbd || kso >= IT8801_EMUL_KSO_COUNT || ksi >= IT8801_EMUL_KSI_COUNT) {
		return -EINVAL;
	}

	WRITE_BIT(data->key_matrix[kso], ksi, pressed);
	it8801_emul_update_ksi(target);

	return 0;
}

static uint8_t it8801_emul_read_reg(const struct emul *target, uint8_t reg)
{
	const struct it8801_emul_cfg *cfg = target->cfg;
	struct it8801_emul_data *data = target->data;

	if (cfg->has_kbd && reg == cfg->kbd.ksidr) {
		return it8801_emul_ksi_level(target);
	}

	return data->regs[reg];
}

static void it8801_emul_write_reg(const struct emul *target, uint8_t reg, uint8_t val)
{
	const struct it8801_emul_cfg *cfg = target->cfg;
	struct it8801_emul_data *data = target->data;

	if (cfg->has_kbd && reg == cfg->kbd.ksieer) {
		/* Edge event bits are write-1-to-clear. */
		data->regs[reg] &= ~val;
		it8801_emul_update_irq(target);
		return;
	}

	data->regs[reg] = val;

	if (cfg->has_kbd && reg == cfg->kbd.ksomcr) {
		it8801_emul_update_ksi(target);
	}
}

/**
 * @brief Emulate an I2C transfer to the IT8801.
 *
 * Supports a subset of I2C access patterns used by the IT8801 driver:
 *   - Single-message write: [reg | data...]
 *   - Two-message read: [reg] then [data...]
 *
 * Failure injection may be configured via it8801_emul_set_read_fail() and
 * it8801_emul_set_write_fail() to force the emulator to reject accesses to a
 * specific register. A failed read returns 0xFF data, as on an idle-high bus.
 */
static int it8801_emul_transfer(const struct emul *target, struct i2c_msg *msgs,
				int num_msgs, int addr)
{
	const struct it8801_emul_cfg *cfg = target->cfg;
	struct it8801_emul_data *data = target->data;
	uint8_t reg;

	if (cfg->addr != addr) {
		LOG_ERR("Address mismatch, expected 0x%02x got 0x%02x", cfg->addr, addr);
		return -EIO;
	}

	i2c_dump_msgs(target->dev, msgs, num_msgs, addr);

	if (num_msgs < 1 || msgs[0].len < 1 || !IS_I2C_MSG_WRITE(msgs[0].flags)) {
		LOG_ERR("First message must be a write of the register address");
		return -EIO;
	}
	reg = msgs[0].buf[0];

	if (num_msgs == 1) {
		if (msgs[0].len < 2) {
			LOG_ERR("Write requires register address and data byte");
			return -EIO;
		}

		for (size_t i = 1; i < msgs[0].len; i++) {
			uint8_t offset = reg + (i - 1);

			if (it8801_emul_access_fails(data->write_fail_reg,
						     &data->write_fail_count, offset)) {
				return -EIO;
			}
			it8801_emul_write_reg(target, offset, msgs[0].buf[i]);
		}
		return 0;
	}

	if (num_msgs == 2 && msgs[0].len == 1 && IS_I2C_MSG_READ(msgs[1].flags)) {
		for (size_t i = 0; i < msgs[1].len; i++) {
			uint8_t offset = reg + i;

			if (it8801_emul_access_fails(data->read_fail_reg,
						     &data->read_fail_count, offset)) {
				memset(msgs[1].buf, 0xFF, msgs[1].len);
				return -EIO;
			}
			msgs[1].buf[i] = it8801_emul_read_reg(target, offset);
		}
		return 0;
	}

	LOG_ERR("Unsupported I2C transaction (num_msgs=%d)", num_msgs);
	return -EIO;
}

static const struct i2c_emul_api it8801_emul_bus_api = {
	.transfer = it8801_emul_transfer,
};

static int it8801_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	it8801_emul_reset(target);
	return 0;
}

#define IT8801_EMUL_KBD_REGS(node_id)                                                              \
	IF_ENABLED(DT_NODE_HAS_COMPAT(node_id, ite_it8801_kbd),                                    \
		   (.has_kbd = true,                                                               \
		    .kbd = {                                                                       \
			    .ksomcr = DT_REG_ADDR_BY_IDX(node_id, 0),                              \
			    .ksidr = DT_REG_ADDR_BY_IDX(node_id, 1),                               \
			    .ksieer = DT_REG_ADDR_BY_IDX(node_id, 2),                              \
			    .ksiier = DT_REG_ADDR_BY_IDX(node_id, 3),                              \
		    },))

#define IT8801_EMUL_DEFINE(n)                                                                      \
	static struct it8801_emul_data it8801_emul_data_##n;                                       \
	static const struct it8801_emul_cfg it8801_emul_cfg_##n = {                                \
		.addr = DT_INST_REG_ADDR(n),                                                       \
		.irq_gpio = GPIO_DT_SPEC_INST_GET(n, irq_gpios),                                   \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(n, IT8801_EMUL_KBD_REGS)                         \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, it8801_emul_init, &it8801_emul_data_##n, &it8801_emul_cfg_##n,      \
			    &it8801_emul_bus_api, NULL);

DT_INST_FOREACH_STATUS_OKAY(IT8801_EMUL_DEFINE)
