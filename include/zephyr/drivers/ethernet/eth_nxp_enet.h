/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ETH_NXP_ENET_H__
#define ZEPHYR_INCLUDE_DRIVERS_ETH_NXP_ENET_H__

/*
 * This header is for NXP ENET driver development
 * and has definitions for internal implementations
 * not to be used by application
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Reasons for callback to a driver:
 *
 * Module reset: The ENET module was reset, perhaps because of power management
 * actions, and subdriver should reinitialize part of the module.
 * Interrupt: An interrupt of a type relevant to the subdriver occurred.
 * Interrupt enable: The driver's relevant interrupt was enabled in NVIC
 */
enum nxp_enet_callback_reason {
	nxp_enet_module_reset,
	nxp_enet_interrupt,
	nxp_enet_interrupt_enabled,
};

struct nxp_enet_ptp_data_for_mac {
	struct k_mutex *ptp_mutex;
};

union nxp_enet_ptp_data {
	struct nxp_enet_ptp_data_for_mac for_mac;
};

/* Calback for mdio device called from mac driver */
void nxp_enet_mdio_callback(const struct device *mdio_dev,
		enum nxp_enet_callback_reason event);

void nxp_enet_ptp_clock_callback(const struct device *dev,
		enum nxp_enet_callback_reason event,
		union nxp_enet_ptp_data *ptp_data);


#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_DRIVERS_ETH_NXP_ENET_H__ */
