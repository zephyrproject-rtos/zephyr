/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Register-file emulator of the TPS65214 and TPS65215 PMICs. On TPS65214 it
 * models REG_LOCK: writes to protected registers are dropped unless 0x5A was
 * written to offset 0x03 (SLVSHK7A section 7.3.15). It also models the
 * interrupt flags, INT_SOURCE and nINT.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/ztest_assert.h>

#include "emul_tps6521x.h"

#define PMIC_NODE   DT_NODELABEL(pmic)
#define IS_TPS65214 DT_NODE_HAS_COMPAT(PMIC_NODE, ti_tps65214)
#define HAS_NINT    DT_NODE_HAS_PROP(PMIC_NODE, int_gpios)

#define REG_GENERAL_CONFIG    0x1eU
#define REG_MFP_2_CONFIG      0x20U
#define REG_MASK_CONFIG       0x25U
#define REG_INT_SOURCE        0x2bU
#define REG_INT_FIRST         0x2cU
#define REG_INT_BUCK_3        0x2eU
#define REG_INT_TIMEOUT_RV_SD 0x32U
#define REG_INT_PB            0x33U
#define REG_INT_LAST          0x35U

#define INT_SOURCE_PB      BIT(7)
#define INT_SOURCE_BITS    7U
#define PB_FALLING_EDGE    BIT(0)
#define PB_RISING_EDGE     BIT(1)
#define PB_STATUS          BIT(2)
#define MASK_INT_FOR_PB    BIT(7)
#define MASK_EFFECT        BIT(5)
#define GENERAL_UV_BIT     BIT(5)
#define GENERAL_GPIO_OUT   BIT(1)
#define MFP_2_PIN_IS_PB    (IS_TPS65214 ? 0x00 : 0x10)
#define MFP_2_PIN_IS_EN    (IS_TPS65214 ? 0x10 : 0x00)

static const uint8_t w1c_tps65214[] = {0x00, 0x3f, 0x0f, 0xff, 0xee, 0x2f, 0xb7, 0x0b, 0xe0, 0xe1};
static const uint8_t w1c_tps65215[] = {0x07, 0x07, 0x0f, 0xff, 0xff, 0x2f, 0xaf, 0x03, 0xe0, 0xe1};

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
	uint8_t sticky[REG_INT_LAST - REG_INT_FIRST + 1];
	int raise_on_read_reg;
	uint8_t raise_on_read_bits;
	struct emul_tps6521x_stats stats;
};

static struct emul_tps6521x_data emul_data;

#if HAS_NINT
static const struct gpio_dt_spec nint = GPIO_DT_SPEC_GET(PMIC_NODE, int_gpios);
#endif

static bool is_int_reg(uint8_t reg)
{
	return reg >= REG_INT_FIRST && reg <= REG_INT_LAST;
}

static uint8_t w1c_mask(uint8_t reg)
{
	return IS_TPS65214 ? w1c_tps65214[reg - REG_INT_FIRST] : w1c_tps65215[reg - REG_INT_FIRST];
}

static uint8_t int_source(const struct emul_tps6521x_data *data)
{
	uint8_t source = 0;

	if ((data->regs[REG_INT_PB] & w1c_mask(REG_INT_PB)) != 0U) {
		source |= INT_SOURCE_PB;
	}
	for (uint8_t bit = 0; bit < INT_SOURCE_BITS; bit++) {
		uint8_t reg = REG_INT_TIMEOUT_RV_SD - bit;

		if ((data->regs[reg] & w1c_mask(reg)) != 0U) {
			source |= BIT(bit);
		}
	}

	return source;
}

static bool nint_asserted(const struct emul_tps6521x_data *data)
{
	uint8_t source = int_source(data);

	if ((data->regs[REG_MASK_CONFIG] & MASK_INT_FOR_PB) != 0U) {
		source &= ~INT_SOURCE_PB;
	}

	return source != 0;
}

static void update_nint(struct emul_tps6521x_data *data)
{
#if HAS_NINT
	int ret = gpio_emul_input_set(nint.port, nint.pin, nint_asserted(data) ? 0 : 1);

	zassert_true(ret == 0 || ret == -EINVAL, "nINT emulation failed (%d)", ret);
#else
	ARG_UNUSED(data);
#endif
}

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

	if (is_int_reg(reg)) {
		uint8_t clear = val & w1c_mask(reg) & ~data->sticky[reg - REG_INT_FIRST];

		data->stats.int_writes++;
		data->regs[reg] &= ~clear;
		update_nint(data);
		return 0;
	}

	if (reg == REG_INT_SOURCE) {
		return 0;
	}

	data->regs[reg] = val;
	if (reg == REG_MASK_CONFIG) {
		update_nint(data);
	}

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
		if (IS_TPS65214 && r == REG_LOCK) {
			msgs[1].buf[i] = 0;
		} else if (r == REG_INT_SOURCE) {
			msgs[1].buf[i] = int_source(data);
		} else {
			msgs[1].buf[i] = data->regs[r];
		}

		if (r == data->raise_on_read_reg) {
			data->raise_on_read_reg = -1;
			data->regs[r] |= data->raise_on_read_bits;
			update_nint(data);
		}
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

void emul_tps6521x_raise(const struct emul *target, uint8_t reg, uint8_t bits)
{
	struct emul_tps6521x_data *data = target->data;

	data->regs[reg] |= bits;
	update_nint(data);
}

void emul_tps6521x_stick(const struct emul *target, uint8_t reg, uint8_t bits)
{
	struct emul_tps6521x_data *data = target->data;

	data->sticky[reg - REG_INT_FIRST] = bits;
}

void emul_tps6521x_pb_hold(const struct emul *target, bool pressed)
{
	struct emul_tps6521x_data *data = target->data;

	if (pressed) {
		data->regs[REG_INT_PB] &= ~PB_STATUS;
	} else {
		data->regs[REG_INT_PB] |= PB_STATUS;
	}
}

void emul_tps6521x_raise_on_read(const struct emul *target, uint8_t reg, uint8_t bits)
{
	struct emul_tps6521x_data *data = target->data;

	data->raise_on_read_bits = bits;
	data->raise_on_read_reg = reg;
}

void emul_tps6521x_int_reset(const struct emul *target)
{
	struct emul_tps6521x_data *data = target->data;

	memset(data->sticky, 0, sizeof(data->sticky));
	data->raise_on_read_reg = -1;
	for (uint8_t reg = REG_INT_FIRST; reg <= REG_INT_LAST; reg++) {
		data->regs[reg] &= ~w1c_mask(reg);
	}
	update_nint(data);
}

bool emul_tps6521x_nint_asserted(const struct emul *target)
{
	return nint_asserted(target->data);
}

static int emul_tps6521x_init(const struct emul *target, const struct device *parent)
{
	struct emul_tps6521x_data *data = target->data;

	ARG_UNUSED(parent);

	memset(data->regs, 0, sizeof(data->regs));
	memset(data->sticky, 0, sizeof(data->sticky));
	data->locked = true;
	data->fail_reg = -1;
	data->raise_on_read_reg = -1;

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

	data->regs[REG_GENERAL_CONFIG] = GENERAL_UV_BIT | GENERAL_GPIO_OUT;
#ifdef EMUL_TPS6521X_PIN_IS_EN
	data->regs[REG_MFP_2_CONFIG] = MFP_2_PIN_IS_EN;
#else
	data->regs[REG_MFP_2_CONFIG] = MFP_2_PIN_IS_PB;
#endif
	data->regs[REG_MASK_CONFIG] = MASK_INT_FOR_PB | MASK_EFFECT;
	data->regs[REG_INT_PB] = PB_STATUS;
#ifdef EMUL_TPS6521X_BOOT_FLAGS
	data->regs[REG_INT_PB] |= PB_FALLING_EDGE | PB_RISING_EDGE;
	data->regs[REG_INT_BUCK_3] = BIT(0);
#endif

	return 0;
}

static const struct i2c_emul_api emul_tps6521x_api = {
	.transfer = emul_tps6521x_transfer,
};

EMUL_DT_DEFINE(PMIC_NODE, emul_tps6521x_init, &emul_data, NULL, &emul_tps6521x_api, NULL);
