/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT hpmicro_hpm_pinctrl
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <zephyr/drivers/pinctrl/pinctrl_hpmicro_common.h>
#include <hpm_common.h>
#include <hpm_soc.h>

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinctrl), okay)
#define IOC_BASE_ADDRESS DT_REG_ADDR(DT_NODELABEL(pinctrl))
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinctrl_bioc), okay)
#define BIOC_BASE_ADDRESS DT_REG_ADDR(DT_NODELABEL(pinctrl_bioc))
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinctrl_pioc), okay)
#define PIOC_BASE_ADDRESS DT_REG_ADDR(DT_NODELABEL(pinctrl_pioc))
#endif

static int hpmicro_pin_configure(IOC_Type *ioc_base, const uint32_t pin_mux, const uint32_t pin_cfg)
{
	uint32_t ioc_pad = HPMICRO_PAD_NUM(pin_mux);
	uint32_t is_analog = HPMICRO_FUNC_ANALOG(pin_mux);
	uint32_t alt_select = HPMICRO_FUNC_ALT_SELECT(pin_mux);
	uint32_t loop_back = HPMICRO_FUNC_LOOPBACK(pin_cfg);
	uint32_t pad_ms = HPMICRO_PAD_CTL_MS(pin_cfg);
	uint32_t pad_od = HPMICRO_PAD_CTL_OD(pin_cfg);
	uint32_t pad_smt = HPMICRO_PAD_CTL_SMT(pin_cfg);
	uint32_t pad_ps = HPMICRO_PAD_CTL_PS(pin_cfg);
	uint32_t pad_pe = HPMICRO_PAD_CTL_PE(pin_cfg);
	uint32_t pad_ds = HPMICRO_PAD_CTL_DS(pin_cfg);

	ioc_base->PAD[ioc_pad].FUNC_CTL =
	IOC_PAD_FUNC_CTL_LOOP_BACK_SET(loop_back) |
	IOC_PAD_FUNC_CTL_ANALOG_SET(is_analog) |
	IOC_PAD_FUNC_CTL_ALT_SELECT_SET(alt_select);

	ioc_base->PAD[ioc_pad].PAD_CTL =
	IOC_PAD_PAD_CTL_MS_SET(pad_ms) |
	IOC_PAD_PAD_CTL_OD_SET(pad_od) |
	IOC_PAD_PAD_CTL_SMT_SET(pad_smt) |
	IOC_PAD_PAD_CTL_PS_SET(pad_ps) |
	IOC_PAD_PAD_CTL_PE_SET(pad_pe) |
	IOC_PAD_PAD_CTL_DS_SET(pad_ds);
	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);
	uint16_t i;
	uint32_t pin_mux, pin_cfg;
	int ret = -EINVAL;
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinctrl), okay)
	IOC_Type *ioc_base = (IOC_Type *)IOC_BASE_ADDRESS;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinctrl_bioc), okay)
	IOC_Type *bioc_base = (IOC_Type *)BIOC_BASE_ADDRESS;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinctrl_pioc), okay)
	IOC_Type *pioc_base = (IOC_Type *)PIOC_BASE_ADDRESS;
#endif
	/* configure all pins */
	for (i = 0; i < pin_cnt; i++) {
		pin_mux = pins[i].pinmux;
		pin_cfg = pins[i].pincfg;
		if (HPMICRO_FUNC_IOC_SELECT(pin_mux) == IOC_TYPE_IOC) {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinctrl), okay)
			ret = hpmicro_pin_configure(ioc_base, pin_mux, pin_cfg);
			if (ret < 0) {
				return ret;
			}
#endif
		} else if (HPMICRO_FUNC_IOC_SELECT(pin_mux) == IOC_TYPE_BIOC) {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinctrl_bioc), okay)
			ret = hpmicro_pin_configure(bioc_base, pin_mux, pin_cfg);
			if (ret < 0) {
				return ret;
			}
#endif
		} else if (HPMICRO_FUNC_IOC_SELECT(pin_mux) == IOC_TYPE_PIOC) {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pinctrl_pioc), okay)
			ret = hpmicro_pin_configure(pioc_base, pin_mux, pin_cfg);
			if (ret < 0) {
				return ret;
			}
#endif
		}
	}
	return ret;
}
