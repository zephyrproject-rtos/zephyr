/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PCIE_HOST_PCIE_PRIV_H_
#define ZEPHYR_DRIVERS_PCIE_HOST_PCIE_PRIV_H_

#include <zephyr/drivers/pcie/pcie.h>

static inline struct pcie_endpoint z_pcie_legacy_endpoint(pcie_bdf_t bdf)
{
	return (struct pcie_endpoint){
#ifdef CONFIG_PCIE_CONTROLLER
		.ctrl = DEVICE_DT_GET(DT_CHOSEN(zephyr_pcie_controller)),
#endif
		.bdf = bdf,
	};
}

static inline int z_pcie_conf_read(const struct pcie_endpoint *ep, unsigned int reg,
				   uint32_t *value)
{
#ifdef CONFIG_PCIE_CONTROLLER
	return pcie_endpoint_conf_read(ep, reg, value);
#else
	*value = pcie_conf_read(ep->bdf, reg);
	return 0;
#endif
}

int z_pcie_get_bar(const struct pcie_endpoint *ep, unsigned int bar_index,
		   struct pcie_bar *bar, bool io);

#endif /* ZEPHYR_DRIVERS_PCIE_HOST_PCIE_PRIV_H_ */
