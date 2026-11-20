/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PCIE_HOST_PCIE_PRIV_H_
#define ZEPHYR_DRIVERS_PCIE_HOST_PCIE_PRIV_H_

#include <zephyr/drivers/pcie/pcie.h>

static inline const struct device *z_pcie_resolve_ctrl(const struct device *ctrl)
{
#ifdef CONFIG_PCIE_CONTROLLER
	if (ctrl != NULL) {
		return ctrl;
	}

	/*
	 * Default to the devicetree chosen controller when no explicit
	 * controller pointer is provided.
	 */
	return DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_pcie_controller));
#else
	ARG_UNUSED(ctrl);

	return NULL;
#endif
}

#endif /* ZEPHYR_DRIVERS_PCIE_HOST_PCIE_PRIV_H_ */
