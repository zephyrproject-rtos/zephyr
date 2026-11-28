/*
 * Copyright (c) 2022-2025 HPMicro
 * Copyright (c) 2026 Cody Gu <gujiaqi@iscas.ac.cn>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pinctrl/pinctrl_hpmicro_common.h>
#include <zephyr/sys/util.h>

#include <hpm_soc.h>

static int hpmicro_pin_configure(IOC_Type *ioc, uint32_t pinmux, uint32_t pincfg)
{
	uint32_t pad = HPMICRO_PAD_NUM(pinmux);

	if (ioc == NULL) {
		return -ENODEV;
	}

	if (pad >= ARRAY_SIZE(ioc->PAD)) {
		return -EINVAL;
	}

	ioc->PAD[pad].FUNC_CTL = IOC_PAD_FUNC_CTL_LOOP_BACK_SET(HPMICRO_FUNC_LOOPBACK(pincfg)) |
				 IOC_PAD_FUNC_CTL_ANALOG_SET(HPMICRO_FUNC_ANALOG(pinmux)) |
				 IOC_PAD_FUNC_CTL_ALT_SELECT_SET(HPMICRO_FUNC_ALT_SELECT(pinmux));

	ioc->PAD[pad].PAD_CTL = IOC_PAD_PAD_CTL_OD_SET(HPMICRO_PAD_CTL_OD(pincfg)) |
				IOC_PAD_PAD_CTL_PS_SET(HPMICRO_PAD_CTL_PS(pincfg)) |
				IOC_PAD_PAD_CTL_PE_SET(HPMICRO_PAD_CTL_PE(pincfg)) |
				IOC_PAD_PAD_CTL_DS_SET(HPMICRO_PAD_CTL_DS(pincfg)) |
				IOC_PAD_PAD_CTL_HYS_SET(HPMICRO_PAD_CTL_HYS(pincfg)) |
				IOC_PAD_PAD_CTL_SPD_SET(HPMICRO_PAD_CTL_SPD(pincfg)) |
				IOC_PAD_PAD_CTL_PRS_SET(HPMICRO_PAD_CTL_PRS(pincfg)) |
				IOC_PAD_PAD_CTL_SR_SET(HPMICRO_PAD_CTL_SR(pincfg)) |
				IOC_PAD_PAD_CTL_KE_SET(!HPMICRO_PAD_CTL_KE_DISABLE(pincfg));

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		IOC_Type *ioc;
		int ret;

		switch (HPMICRO_FUNC_IOC_SELECT(pins[i].pinmux)) {
		case IOC_TYPE_IOC:
			ioc = (IOC_Type *)DT_REG_ADDR(DT_NODELABEL(pinctrl));
			break;
		case IOC_TYPE_PIOC:
			ioc = (IOC_Type *)DT_REG_ADDR(DT_NODELABEL(pinctrl_pioc));
			break;
		default:
			return -EINVAL;
		}

		ret = hpmicro_pin_configure(ioc, pins[i].pinmux, pins[i].pincfg);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
