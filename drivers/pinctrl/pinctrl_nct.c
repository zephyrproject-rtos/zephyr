/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

/* Driver config */
struct nct_pinctrl_config {
	/* Device base address used for pinctrl driver */
	uintptr_t base_scfg;
	uintptr_t base_glue;
};

static const struct nct_pinctrl_config nct_pinctrl_cfg = {
	.base_scfg = NCT_SCFG_REG_ADDR,
	.base_glue = NCT_GLUE_REG_ADDR,
};

static void nct_periph_pinmux_configure(const struct nct_periph *alt, bool is_alternate)
{
	const uintptr_t scfg_base = nct_pinctrl_cfg.base_scfg;
	uint8_t alt_mask = BIT(alt->bit);

	/*
	 * is_alternate == 0 means select GPIO, otherwise Alternate Func.
	 * inverted == 0:
	 *    Set devalt bit to select Alternate Func.
	 * inverted == 1:
	 *    Clear devalt bit to select Alternate Func.
	 */
	if (is_alternate != alt->inverted) {
		NCT_DEVALT(scfg_base, alt->group) |=  alt_mask;
	} else {
		NCT_DEVALT(scfg_base, alt->group) &= ~alt_mask;
	}
}

static void nct_periph_pupd_configure(const struct nct_periph *pupd,
	enum nct_io_bias_type type)
{
	const uintptr_t scfg_base = nct_pinctrl_cfg.base_scfg;

	if (type == NCT_BIAS_TYPE_NONE) {
		NCT_PUPD_EN(scfg_base, pupd->group) &= ~BIT(pupd->bit);
	} else {
		NCT_PUPD_EN(scfg_base, pupd->group) |=  BIT(pupd->bit);
	}
}

static void nct_periph_configure(const pinctrl_soc_pin_t *pin, uintptr_t reg)
{
	if (pin->cfg.periph.type == NCT_PINCTRL_TYPE_PERIPH_PINMUX) {
		/* Configure peripheral device's pinmux functionality */
		nct_periph_pinmux_configure(&pin->cfg.periph,
					     !pin->flags.pinmux_gpio);
	} else if (pin->cfg.periph.type == NCT_PINCTRL_TYPE_PERIPH_PUPD) {
		/* Configure peripheral device's internal PU/PD */
		nct_periph_pupd_configure(&pin->cfg.periph,
			pin->flags.io_bias_type);
	}
}

static void nct_device_control_configure(const pinctrl_soc_pin_t *pin)
{
	const struct nct_dev_ctl *ctrl = (const struct nct_dev_ctl *)&pin->cfg.dev_ctl;
	const uintptr_t scfg_base = nct_pinctrl_cfg.base_scfg;

	SET_FIELD(NCT_DEV_CTL(scfg_base, ctrl->offest),
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
		if (pins[i].flags.type == NCT_PINCTRL_TYPE_PERIPH) {
			/* Configure peripheral device's pinmux functionality */
			nct_periph_configure(&pins[i], reg);
		} else if (pins[i].flags.type == NCT_PINCTRL_TYPE_DEVICE_CTRL) {
			/* Configure device's io characteristics */
			nct_device_control_configure(&pins[i]);
		} else {
			return -ENOTSUP;
		}
	}

	return 0;
}
