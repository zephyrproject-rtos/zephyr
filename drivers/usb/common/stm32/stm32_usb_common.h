/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_COMMON_STM32_STM32_USB_COMMON_H_
#define ZEPHYR_DRIVERS_USB_COMMON_STM32_STM32_USB_COMMON_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

/* Compatible of all STM32 USB controllers */
#define STM32_USB_COMPATIBLES								\
	st_stm32_usb, st_stm32_otgfs, st_stm32_otghs

/* Shorthand to obtain PHY node for an instance */
#define USB_STM32_PHY(usb_node)			DT_PROP_BY_IDX(usb_node, phys, 0)

/* Evaluates to 1 if `usb_node` is High-Speed capable, 0 otherwise. */
#define USB_STM32_NODE_IS_HS_CAPABLE(usb_node)	DT_NODE_HAS_COMPAT(usb_node, st_stm32_otghs)

/* Evaluates to 1 if PHY of `usb_node` is an ULPI PHY, 0 otherwise. */
#define USB_STM32_NODE_PHY_IS_ULPI(usb_node)						\
	UTIL_AND(USB_STM32_NODE_IS_HS_CAPABLE(usb_node),				\
		DT_NODE_HAS_COMPAT(USB_STM32_PHY(usb_node), usb_ulpi_phy))

/*
 * Evaluates to 1 if PHY of `usb_node` is an embedded HS PHY, 0 otherwise.
 *
 * Implementation notes:
 * All embedded HS PHYs have specific compatibles (with ST vendor).
 */
#define USB_STM32_NODE_PHY_IS_EMBEDDED_HS(usb_node)					\
	UTIL_OR(DT_NODE_HAS_COMPAT(USB_STM32_PHY(usb_node), st_stm32_usbphyc),		\
		DT_NODE_HAS_COMPAT(USB_STM32_PHY(usb_node), st_stm32u5_otghs_phy))

/*
 * Evaluates to 1 if PHY of `usb_node` is an embedded FS PHY, 0 otherwise.
 *
 * Implementation notes:
 * USB PHYs are either ULPI, embedded HS or embedded FS.
 */
#define USB_STM32_NODE_PHY_IS_EMBEDDED_FS(usb_node)					\
	UTIL_AND(UTIL_NOT(USB_STM32_NODE_PHY_IS_ULPI(usb_node)),			\
		 UTIL_NOT(USB_STM32_NODE_PHY_IS_EMBEDDED_HS(usb_node)))

/**
 * @brief Configures the Power Controller as necessary
 * for proper operation of the USB controllers
 *
 * @returns 0 on success, negative error code otherwise.
 */
int stm32_usb_pwr_enable(void);

/**
 * @brief Configures the Power Controller to disable
 * USB-related regulators/etc if no controller is
 * still active (refcounted).
 *
 * @returns 0 on success, negative error code otherwise.
 */
int stm32_usb_pwr_disable(void);

#endif /* ZEPHYR_DRIVERS_USB_COMMON_STM32_STM32_USB_COMMON_H_ */
