/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_COMMON_STM32_STM32_USB_COMMON_H_
#define ZEPHYR_DRIVERS_USB_COMMON_STM32_STM32_USB_COMMON_H_

#include <zephyr/device.h>
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

/* STM32 USB PHY pseudo-device */
struct stm32_usb_phy {
	/**
	 * Apply PHY configuration and enable PHY clock.
	 *
	 * @pre The USB controller clock is enabled.
	 *
	 * @param phy PHY pseudo-device
	 * @returns 0 on success, negative error code otherwise.
	 */
	int (*enable)(const struct stm32_usb_phy *phy);

	/**
	 * Disable PHY clock.
	 *
	 * @pre The USB controller clock is enabled.
	 *
	 * @param phy PHY pseudo-device
	 * @returns 0 on success, negative error code otherwise.
	 */
	int (*disable)(const struct stm32_usb_phy *phy);

	/**
	 * PHY-specific configuration
	 *
	 * This field is reserved for PHY pseudo-device drivers;
	 * USB controller drivers must not examine this field.
	 *
	 * If possible, the configuration data is encoded inside
	 * a pointer-sized integer `cfg`; otherwise, a pointer to
	 * an out-of-band configuration block is saved in `pcfg`.
	 */
	union {
		uintptr_t cfg;
		const void *pcfg;
	};
};

/*
 * Returns the name of the PHY pseudo-device for `usb_node`.
 *
 * Implementation notes:
 * Use unique DT device name suffixed with "__stm32_phy".
 */
#define USB_STM32_PHY_PSEUDODEV_NAME(usb_node)					\
	CONCAT(DEVICE_DT_NAME_GET(usb_node), __stm32_phy)

/*
 * Evaluates to 1 if there is a corresponding PHY pseudo-device
 * for `usb_node`, 0 otherwise.
 *
 * Implementation notes:
 * We don't create a PHY pseudo-device for embedded FS PHY since
 * there is nothing to configure, except on a few series which
 * have quirky hardware (STM32F4, STM32H7, ...)
 */
#define USB_STM32_HAS_PHY_PSEUDODEV(usb_node) UTIL_OR(UTIL_OR(			\
	IS_ENABLED(CONFIG_SOC_SERIES_STM32H7X),					\
	IS_ENABLED(CONFIG_SOC_SERIES_STM32F4X)),				\
	UTIL_NOT(USB_STM32_NODE_PHY_IS_EMBEDDED_FS(usb_node)))

/*
 * If PHY pseudo-device exists for `usb_node`'s PHY,
 * returns pointer to it; otherwise, returns NULL.
 */
#define USB_STM32_PHY_PSEUDODEV_GET_OR_NULL(usb_node)				\
	COND_CODE_1(USB_STM32_HAS_PHY_PSEUDODEV(usb_node),			\
		(&USB_STM32_PHY_PSEUDODEV_NAME(usb_node)), (NULL))

/* Forward declare all PHY pseudo-devices */
#define _STM32_USB_PHY_PSEUDODEV_DECLARE(usb_node)				\
	extern const struct stm32_usb_phy USB_STM32_PHY_PSEUDODEV_NAME(usb_node);
#define _STM32_USB_DECLARE_ALL_PHYS_OF_COMPAT(compat)				\
	DT_FOREACH_STATUS_OKAY(compat, _STM32_USB_PHY_PSEUDODEV_DECLARE)
FOR_EACH(_STM32_USB_DECLARE_ALL_PHYS_OF_COMPAT, (), STM32_USB_COMPATIBLES)
/* End of PHY pseudo-devices declaration */

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
