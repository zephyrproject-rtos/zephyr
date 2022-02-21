/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

/* Driver config */
struct npcx_pinctrl_config {
	/* scfg device base address */
	uintptr_t base_scfg;
};

static const struct npcx_pinctrl_config npcx_pinctrl_cfg = {
	.base_scfg = DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), scfg),
};

/* Pin-control local functions for peripheral devices */
static bool npcx_periph_pinmux_has_lock(int group)
{
#if defined(CONFIG_SOC_SERIES_NPCX7)
	if (group == 0x00 || (group >= 0x02 && group <= 0x04) || group == 0x06 ||
		group == 0x0b || group == 0x0f) {
		return true;
	}
#elif defined(CONFIG_SOC_SERIES_NPCX9)
	if (group == 0x00 || (group >= 0x02 && group <= 0x06) || group == 0x0b ||
		group == 0x0d || (group >= 0x0f && group <= 0x12)) {
		return true;
	}
#endif
	return false;
}

static void npcx_periph_pinmux_configure(const struct npcx_periph *alt, bool is_alternate,
	bool is_locked)
{
	const uintptr_t scfg_base = npcx_pinctrl_cfg.base_scfg;
	uint8_t alt_mask = BIT(alt->bit);

	/*
	 * is_alternate == 0 means select GPIO, otherwise Alternate Func.
	 * inverted == 0:
	 *    Set devalt bit to select Alternate Func.
	 * inverted == 1:
	 *    Clear devalt bit to select Alternate Func.
	 */
	if (is_alternate != alt->inverted) {
		NPCX_DEVALT(scfg_base, alt->group) |=  alt_mask;
	} else {
		NPCX_DEVALT(scfg_base, alt->group) &= ~alt_mask;
	}

	if (is_locked && npcx_periph_pinmux_has_lock(alt->group)) {
		NPCX_DEVALT_LK(scfg_base, alt->group) |= alt_mask;
	}
}

static void npcx_periph_pupd_configure(const struct npcx_periph *pupd,
	enum npcx_io_bias_type type)
{
	const uintptr_t scfg_base = npcx_pinctrl_cfg.base_scfg;

	if (type == NPCX_BIAS_TYPE_NONE) {
		NPCX_PUPD_EN(scfg_base, pupd->group) &= ~BIT(pupd->bit);
	} else {
		NPCX_PUPD_EN(scfg_base, pupd->group) |=  BIT(pupd->bit);
	}
}

static void npcx_periph_pwm_drive_mode_configure(uintptr_t reg, bool is_od)
{
	struct pwm_reg *const inst = (struct pwm_reg *)(reg);

	if (is_od) {
		inst->PWMCTLEX |= BIT(NPCX_PWMCTLEX_OD_OUT);
	} else {
		inst->PWMCTLEX &= ~BIT(NPCX_PWMCTLEX_OD_OUT);
	}
}

static void npcx_periph_configure(const pinctrl_soc_pin_t *pin, uintptr_t reg)
{
	if (pin->cfg.periph.type == NPCX_PINCTRL_TYPE_PERIPH_PINMUX) {
		/* Configure peripheral device's pinmux functionality */
		npcx_periph_pinmux_configure(&pin->cfg.periph,
					     !pin->flags.pinmux_gpio,
					     pin->flags.pinmux_lock);
	} else if (pin->cfg.periph.type == NPCX_PINCTRL_TYPE_PERIPH_PUPD) {
		/* Configure peripheral device's internal PU/PD */
		npcx_periph_pupd_configure(&pin->cfg.periph,
			pin->flags.io_bias_type);
	} else if (pin->cfg.periph.type == NPCX_PINCTRL_TYPE_PERIPH_DRIVE) {
		/* Configure peripheral device's drive mode. (Only PWM pads support it) */
		npcx_periph_pwm_drive_mode_configure(reg,
			pin->flags.io_drive_type == NPCX_DRIVE_TYPE_OPEN_DRAIN);
	}
}

/* Pinctrl API implementation */
int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);

	/* Configure all peripheral devices' properties here. */
	for (uint8_t i = 0; i < pin_cnt; i++) {
		npcx_periph_configure(&pins[i], reg);
	}

	return 0;
}
