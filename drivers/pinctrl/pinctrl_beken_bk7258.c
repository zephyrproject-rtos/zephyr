/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT beken_bk7258_pinctrl

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#define BK7258_PIN_COUNT 56

#define PAD_BASE  DT_INST_REG_ADDR_BY_NAME(0, pad)
#define FUNC_BASE DT_INST_REG_ADDR_BY_NAME(0, func)

/* One configuration word per pad */
#define PAD_INPUT_EN     BIT(2)
#define PAD_OUTPUT_DIS   BIT(3)
#define PAD_PULL_UP      BIT(4)
#define PAD_PULL_EN      BIT(5)
#define PAD_SECOND_FUNC  BIT(6)

/* Function select: eight pads per word, four bits each */
#define FUNC_PINS_PER_REG 8
#define FUNC_FIELD_WIDTH  4

static struct k_spinlock bk7258_pinctrl_lock;

static int pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	uintptr_t pad = PAD_BASE + (pin->pin * 4U);
	uintptr_t func = FUNC_BASE + ((pin->pin / FUNC_PINS_PER_REG) * 4U);
	uint32_t shift = (pin->pin % FUNC_PINS_PER_REG) * FUNC_FIELD_WIDTH;
	uint32_t func_mask = GENMASK(FUNC_FIELD_WIDTH - 1U, 0) << shift;
	uint32_t pad_mask = PAD_INPUT_EN | PAD_OUTPUT_DIS | PAD_PULL_UP | PAD_PULL_EN |
			    PAD_SECOND_FUNC;
	uint32_t pad_val = PAD_OUTPUT_DIS | PAD_SECOND_FUNC;
	k_spinlock_key_t key;
	uint32_t val;

	if ((pin->pin >= BK7258_PIN_COUNT) || (pin->pull_up && pin->pull_down)) {
		return -EINVAL;
	}

	if (pin->pull_up) {
		pad_val |= PAD_PULL_EN | PAD_PULL_UP;
	} else if (pin->pull_down) {
		pad_val |= PAD_PULL_EN;
	}

	key = k_spin_lock(&bk7258_pinctrl_lock);

	/*
	 * Select the function before handing the pad to it, so that it is
	 * never driven by whichever function the field held before. The
	 * GPIO input and output paths are switched off, as the vendor SDK
	 * does: the peripheral controls the pad's direction itself.
	 */
	val = sys_read32(func);
	val = (val & ~func_mask) | ((uint32_t)pin->func << shift);
	sys_write32(val, func);

	val = sys_read32(pad);
	val = (val & ~pad_mask) | pad_val;
	sys_write32(val, pad);

	k_spin_unlock(&bk7258_pinctrl_lock, key);

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		int err = pinctrl_configure_pin(&pins[i]);

		if (err < 0) {
			return err;
		}
	}

	return 0;
}
