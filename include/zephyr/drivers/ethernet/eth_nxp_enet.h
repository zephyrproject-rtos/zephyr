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
	NXP_ENET_MODULE_RESET,
	NXP_ENET_INTERRUPT,
	NXP_ENET_INTERRUPT_ENABLED,
};

enum nxp_enet_driver {
	NXP_ENET_MAC,
	NXP_ENET_MDIO,
	NXP_ENET_PTP_CLOCK,
};

extern void nxp_enet_mdio_callback(const struct device *mdio_dev,
		enum nxp_enet_callback_reason event,
		void *data);

#ifdef CONFIG_PTP_CLOCK_NXP_ENET
extern void nxp_enet_ptp_clock_callback(const struct device *dev,
		enum nxp_enet_callback_reason event,
		void *data);
#else
#define nxp_enet_ptp_clock_callback(...)
#endif

/*
 * Internal implementation, inter-driver communication function
 *
 * dev: target device to call back
 * dev_type: which driver to call back
 * event: reason/cause of callback
 * data: opaque data, will be interpreted based on reason and target driver
 */
extern void nxp_enet_driver_cb(const struct device *dev,
				enum nxp_enet_driver dev_type,
				enum nxp_enet_callback_reason event,
				void *data);

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_DRIVERS_ETH_NXP_ENET_H__ */
