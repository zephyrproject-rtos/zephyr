/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Register-file emulator of the TPS65214 and TPS65215 PMICs. On TPS65214 it
 * models REG_LOCK: writes to protected registers are dropped unless 0x5A was
 * written to offset 0x03 (SLVSHK7A section 7.3.15).
 */

#include <errno.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>

#include "emul_tps6521x.h"

#define PMIC_NODE   DT_NODELABEL(pmic)
#define IS_TPS65214 DT_NODE_HAS_COMPAT(PMIC_NODE, ti_tps65214)

#define REG_TI_DEV_ID    0x00U
#define REG_ENABLE_CTRL  0x02U
#define REG_LOCK         0x03U
#define REG_LDO2_VOUT_14 0x06U
#define REG_BUCK2_VOUT   0x09U
#define REG_MFP_CTRL     0x29U
#define REG_USER_NVM_CMD 0x34U

#define REG_UNLOCK_CMD 0x5AU

/* TI_DEV_ID as read from a TPS6521401 on a TMDS62LEVM, and TPS6521501-Q1 per SLVUCW5 */
#define DEV_ID_TPS65214 0x14U
#define DEV_ID_TPS65215 0x95U

/* BUCK2_VOUT bandwidth and UV threshold bits, set to check they are kept */
#define BUCK2_VOUT_CONFIG_BITS 0xC0U
#define LDO2_LSW_CONFIG_14     BIT(7)

struct emul_tps6521x_data {
	uint8_t regs[EMUL_TPS6521X_NUM_REGS];
	bool locked;
	int fail_reg;
	struct emul_tps6521x_stats stats;
};

static struct emul_tps6521x_data emul_data;

static bool reg_protected(uint8_t reg)
{
	return IS_TPS65214 && reg != REG_LOCK && reg != REG_MFP_CTRL && reg != REG_USER_NVM_CMD;
}

static int reg_write(struct emul_tps6521x_data *data, uint8_t reg, uint8_t val)
{
	if (reg >= EMUL_TPS6521X_NUM_REGS) {
		return -EIO;
	}
	if (reg == data->fail_reg) {
		return -EIO;
	}

	data->stats.writes++;
	if (reg == REG_LOCK) {
		data->stats.reg3_writes++;
		if (IS_TPS65214) {
			data->locked = val != REG_UNLOCK_CMD;
			return 0;
		}
	}

	if (reg_protected(reg) && data->locked) {
		data->stats.blocked_writes++;
		return 0;
	}

	data->regs[reg] = val;

	return 0;
}

static int emul_tps6521x_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				  int addr)
{
	struct emul_tps6521x_data *data = target->data;
	uint8_t reg;

	ARG_UNUSED(addr);

	if (num_msgs < 1 || (msgs[0].flags & I2C_MSG_READ) || msgs[0].len < 1) {
		return -EIO;
	}

	reg = msgs[0].buf[0];

	if (num_msgs == 1) {
		for (uint32_t i = 1; i < msgs[0].len; i++) {
			int ret = reg_write(data, reg + i - 1, msgs[0].buf[i]);

			if (ret < 0) {
				return ret;
			}
		}
		return 0;
	}

	if (num_msgs != 2 || !(msgs[1].flags & I2C_MSG_READ)) {
		return -EIO;
	}

	for (uint32_t i = 0; i < msgs[1].len; i++) {
		uint8_t r = reg + i;

		if (r >= EMUL_TPS6521X_NUM_REGS) {
			return -EIO;
		}
		msgs[1].buf[i] = (IS_TPS65214 && r == REG_LOCK) ? 0 : data->regs[r];
	}

	return 0;
}

uint8_t emul_tps6521x_get(const struct emul *target, uint8_t reg)
{
	struct emul_tps6521x_data *data = target->data;

	return data->regs[reg];
}

void emul_tps6521x_set(const struct emul *target, uint8_t reg, uint8_t val)
{
	struct emul_tps6521x_data *data = target->data;

	data->regs[reg] = val;
}

bool emul_tps6521x_locked(const struct emul *target)
{
	struct emul_tps6521x_data *data = target->data;

	return data->locked;
}

void emul_tps6521x_stats(const struct emul *target, struct emul_tps6521x_stats *stats)
{
	struct emul_tps6521x_data *data = target->data;

	*stats = data->stats;
}

void emul_tps6521x_fail_writes_to(const struct emul *target, int reg)
{
	struct emul_tps6521x_data *data = target->data;

	data->fail_reg = reg;
}

static int emul_tps6521x_init(const struct emul *target, const struct device *parent)
{
	struct emul_tps6521x_data *data = target->data;

	ARG_UNUSED(parent);

	memset(data->regs, 0, sizeof(data->regs));
	data->locked = true;
	data->fail_reg = -1;

	/* the other variant's ID, for the id_mismatch scenarios */
	if (IS_ENABLED(CONFIG_TPS6521X_TEST_ID_MISMATCH)) {
		data->regs[REG_TI_DEV_ID] = IS_TPS65214 ? DEV_ID_TPS65215 : DEV_ID_TPS65214;
	} else {
		data->regs[REG_TI_DEV_ID] = IS_TPS65214 ? DEV_ID_TPS65214 : DEV_ID_TPS65215;
	}
	/* all rails off, so that always-on and boot-on are visible */
	data->regs[REG_ENABLE_CTRL] = 0x00U;
	data->regs[REG_BUCK2_VOUT] = BUCK2_VOUT_CONFIG_BITS;
	if (IS_TPS65214) {
		data->regs[REG_LDO2_VOUT_14] = LDO2_LSW_CONFIG_14;
	}

	return 0;
}

static const struct i2c_emul_api emul_tps6521x_api = {
	.transfer = emul_tps6521x_transfer,
};

EMUL_DT_DEFINE(PMIC_NODE, emul_tps6521x_init, &emul_data, NULL, &emul_tps6521x_api, NULL);
