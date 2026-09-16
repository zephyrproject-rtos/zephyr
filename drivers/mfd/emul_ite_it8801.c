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
#include <zephyr/drivers/emul_stub_device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/mfd/emul_ite_it8801.h>
#include <zephyr/drivers/mfd/mfd_ite_it8801.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(emul_ite_it8801, CONFIG_MFD_LOG_LEVEL);

#define IT8801_EMUL_NUM_REGS 256

#define IS_I2C_MSG_WRITE(flags) ((flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE)
#define IS_I2C_MSG_READ(flags)  ((flags & I2C_MSG_RW_MASK) == I2C_MSG_READ)

struct it8801_emul_cfg {
	uint16_t addr;
};

struct it8801_emul_data {
	uint8_t regs[IT8801_EMUL_NUM_REGS];
	uint16_t read_fail_reg;
	uint16_t write_fail_reg;
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

void it8801_emul_reset(const struct emul *target)
{
	struct it8801_emul_data *data = target->data;

	memset(data->regs, 0, sizeof(data->regs));
	data->read_fail_reg = IT8801_EMUL_NO_FAIL_REG;
	data->write_fail_reg = IT8801_EMUL_NO_FAIL_REG;
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

void it8801_emul_set_read_fail_reg(const struct emul *target, uint16_t reg)
{
	struct it8801_emul_data *data = target->data;

	data->read_fail_reg = reg;
}

void it8801_emul_set_write_fail_reg(const struct emul *target, uint16_t reg)
{
	struct it8801_emul_data *data = target->data;

	data->write_fail_reg = reg;
}

/* Low bits of KSOMCR select which scan-out line is driven low. */
#define IT8801_EMUL_KSO_SEL_MASK GENMASK(4, 0)

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

/**
 * @brief Emulate an I2C transfer to the IT8801.
 *
 * Supports a subset of I2C access patterns used by the IT8801 driver:
 *   - Single-message write: [reg | data...]
 *   - Two-message read: [reg] then [data...]
 *
 * Failure injection may be configured via it8801_emul_set_read_fail_reg() and
 * it8801_emul_set_write_fail_reg() to force the emulator to reject accesses to
 * a specific register.
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

			if (data->write_fail_reg == offset) {
				return -EIO;
			}
			data->regs[offset] = msgs[0].buf[i];
		}
		return 0;
	}

	if (num_msgs == 2 && msgs[0].len == 1 && IS_I2C_MSG_READ(msgs[1].flags)) {
		for (size_t i = 0; i < msgs[1].len; i++) {
			uint8_t offset = reg + i;

			if (data->read_fail_reg == offset) {
				return -EIO;
			}
			msgs[1].buf[i] = data->regs[offset];
		}
		return 0;
	}

	LOG_ERR("Unsupported I2C transaction (num_msgs=%d)", num_msgs);
	return -EIO;
}

static const struct i2c_emul_api it8801_emul_bus_api = {
	.transfer = it8801_emul_transfer,
};

#define IT8801_EMUL_RESET_RULE_BEFORE(inst)                                                        \
	it8801_emul_reset(&EMUL_DT_NAME_GET(DT_DRV_INST(inst)));

static void it8801_emul_reset_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	DT_INST_FOREACH_STATUS_OKAY(IT8801_EMUL_RESET_RULE_BEFORE)
}
ZTEST_RULE(emul_ite_it8801_reset, it8801_emul_reset_before, NULL);

static int it8801_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	it8801_emul_reset(target);
	return 0;
}

#define IT8801_EMUL_DEFINE(n)                                                                      \
	static struct it8801_emul_data it8801_emul_data_##n;                                       \
	static const struct it8801_emul_cfg it8801_emul_cfg_##n = {                                \
		.addr = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, it8801_emul_init, &it8801_emul_data_##n, &it8801_emul_cfg_##n,      \
			    &it8801_emul_bus_api, NULL);

DT_INST_FOREACH_STATUS_OKAY(IT8801_EMUL_DEFINE)

/*
 * When the real IT8801 MFD driver is not compiled, downstream nodes that
 * reference the MFD parent via DEVICE_DT_GET() need a placeholder device
 * struct. Create stub devices for each MFD instance in that case so tests
 * can exercise sub-drivers (keyboard, PWM, GPIO) in isolation.
 */
#if !defined(CONFIG_MFD_ITE_IT8801)
DT_INST_FOREACH_STATUS_OKAY(EMUL_STUB_DEVICE)
#endif
