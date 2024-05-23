/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

/* Driver config */
struct npcm_pinctrl_config {
	/* Device base address used for pinctrl driver */
	uintptr_t base_scfg;
	uintptr_t base_glue;
};

static const struct npcm_pinctrl_config npcm_pinctrl_cfg = {
	.base_scfg = NPCM_SCFG_REG_ADDR,
	.base_glue = NPCM_GLUE_REG_ADDR,
};

static void npcm_periph_pinmux_configure(const struct npcm_periph *alt, bool is_alternate)
{
	const uintptr_t scfg_base = npcm_pinctrl_cfg.base_scfg;
	uint8_t alt_mask = BIT(alt->bit);

	/*
	 * is_alternate == 0 means select GPIO, otherwise Alternate Func.
	 * inverted == 0:
	 *    Set devalt bit to select Alternate Func.
	 * inverted == 1:
	 *    Clear devalt bit to select Alternate Func.
	 */
	if (is_alternate != alt->inverted) {
		NPCM_DEVALT(scfg_base, alt->group) |=  alt_mask;
	} else {
		NPCM_DEVALT(scfg_base, alt->group) &= ~alt_mask;
	}
}

static void npcm_periph_pupd_configure(const struct npcm_periph *pupd,
	enum npcm_io_bias_type type)
{
	const uintptr_t scfg_base = npcm_pinctrl_cfg.base_scfg;

	if (type == NPCM_BIAS_TYPE_NONE) {
		NPCM_PUPD_EN(scfg_base, pupd->group) &= ~BIT(pupd->bit);
	} else {
		NPCM_PUPD_EN(scfg_base, pupd->group) |=  BIT(pupd->bit);
	}
}

static void npcm_periph_configure(const pinctrl_soc_pin_t *pin, uintptr_t reg)
{
	if (pin->cfg.periph.type == NPCM_PINCTRL_TYPE_PERIPH_PINMUX) {
		/* Configure peripheral device's pinmux functionality */
		npcm_periph_pinmux_configure(&pin->cfg.periph,
					     !pin->flags.pinmux_gpio);
	} else if (pin->cfg.periph.type == NPCM_PINCTRL_TYPE_PERIPH_PUPD) {
		/* Configure peripheral device's internal PU/PD */
		npcm_periph_pupd_configure(&pin->cfg.periph,
			pin->flags.io_bias_type);
	}
}

static void npcm_device_control_configure(const pinctrl_soc_pin_t *pin)
{
	const struct npcm_dev_ctl *ctrl = (const struct npcm_dev_ctl *)&pin->cfg.dev_ctl;
	const uintptr_t scfg_base = npcm_pinctrl_cfg.base_scfg;

	SET_FIELD(NPCM_DEV_CTL(scfg_base, ctrl->offest),
			      FIELD(ctrl->field_offset, ctrl->field_size),
			      ctrl->field_value);
}

/* Pinctrl API implementation */
int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);

	/* Configure all peripheral devices' properties here. */
	for (uint8_t i = 0; i < pin_cnt; i++) {
		if (pins[i].flags.type == NPCM_PINCTRL_TYPE_PERIPH) {
			/* Configure peripheral device's pinmux functionality */
			npcm_periph_configure(&pins[i], reg);
		} else if (pins[i].flags.type == NPCM_PINCTRL_TYPE_DEVICE_CTRL) {
			/* Configure device's io characteristics */
			npcm_device_control_configure(&pins[i]);
		} else {
			return -ENOTSUP;
		}
	}

	return 0;
}
