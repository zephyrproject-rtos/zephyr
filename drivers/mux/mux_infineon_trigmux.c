/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_trigmux

#include <errno.h>

#include <infineon_kconfig.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mux.h>
#include <zephyr/dt-bindings/mux/infineon-trigmux.h>
#include <zephyr/sys/util.h>

#include <cy_trigmux.h>

#define IFX_TRIGMUX_OUT_1TO1 BIT(12)
#define IFX_TRIGMUX_FLAGS    (IFX_TRIGMUX_EDGE | IFX_TRIGMUX_INV)

static int ifx_trigmux_disconnect(const struct device *dev, const struct mux_control *control)
{
	uint32_t out = control->cells[0];
	cy_en_trigmux_status_t status;

	ARG_UNUSED(dev);

	if ((out & CY_TR_OUT_CTL_MASK) == 0U) {
		return -EINVAL;
	}

	if ((out & IFX_TRIGMUX_OUT_1TO1) != 0U) {
		status = Cy_TrigMux_Deselect(out);
	} else {
		uint32_t in = out & ~(CY_TR_OUT_CTL_MASK | PERI_TR_CMD_TR_SEL_Msk);

		status = Cy_TrigMux_Connect(in, out, false, TRIGGER_TYPE_LEVEL);
	}

	return (status == CY_TRIGMUX_SUCCESS) ? 0 : -EINVAL;
}

static int ifx_trigmux_set(const struct device *dev, const struct mux_control *control,
			   uint32_t state)
{
	uint32_t out = control->cells[0];
	uint32_t flags = control->cells[1];
	bool invert = (flags & IFX_TRIGMUX_INV) != 0U;
	en_trig_type_t type =
		((flags & IFX_TRIGMUX_EDGE) != 0U) ? TRIGGER_TYPE_EDGE : TRIGGER_TYPE_LEVEL;
	cy_en_trigmux_status_t status;

	if (((out & CY_TR_OUT_CTL_MASK) == 0U) || ((flags & ~IFX_TRIGMUX_FLAGS) != 0U)) {
		return -EINVAL;
	}

	if (state == IFX_TRIGMUX_OFF) {
		return ifx_trigmux_disconnect(dev, control);
	}

	if ((out & IFX_TRIGMUX_OUT_1TO1) != 0U) {
		if (state != IFX_TRIGMUX_ON) {
			return -EINVAL;
		}

		status = Cy_TrigMux_Select(out, invert, type);
	} else {
		status = Cy_TrigMux_Connect(state, out, invert, type);
	}

	return (status == CY_TRIGMUX_SUCCESS) ? 0 : -EINVAL;
}

static DEVICE_API(mux_control, ifx_trigmux_api) = {
	.set = ifx_trigmux_set,
	.disconnect = ifx_trigmux_disconnect,
};

#define IFX_TRIGMUX_INIT(n)                                                                        \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, PRE_KERNEL_1, CONFIG_MUX_INIT_PRIORITY,   \
			      &ifx_trigmux_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_TRIGMUX_INIT)
