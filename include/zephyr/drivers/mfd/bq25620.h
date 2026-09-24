/*
 * Copyright (c) 2026 Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_BQ25620_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_BQ25620_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bq25620.h
 * @brief TI BQ25620 MFD interface
 * @defgroup mfd_interface_bq25620 MFD BQ25620 Interface
 * @ingroup mfd_interfaces
 * @{
 */

/**
 * @name BQ25620 events
 *
 * The events correspond to the flag bits of the Charger_Flag_0,
 * Charger_Flag_1 and FAULT_Flag_0 registers.
 * @{
 */
/** I2C watchdog timer expired. */
#define MFD_BQ25620_EVENT_WD         BIT(0)
/** Safety timer expired. */
#define MFD_BQ25620_EVENT_SAFETY_TMR BIT(1)
/** Input voltage or OTG voltage regulation entered. */
#define MFD_BQ25620_EVENT_VINDPM     BIT(2)
/** Input current, ILIM or OTG current regulation entered. */
#define MFD_BQ25620_EVENT_IINDPM     BIT(3)
/** Minimal system voltage regulation entered or exited. */
#define MFD_BQ25620_EVENT_VSYS       BIT(4)
/** Thermal regulation entered. */
#define MFD_BQ25620_EVENT_TREG       BIT(5)
/** ADC conversion done (one-shot mode only). */
#define MFD_BQ25620_EVENT_ADC_DONE   BIT(6)
/** VBUS status changed. */
#define MFD_BQ25620_EVENT_VBUS       BIT(8)
/** Charge status changed. */
#define MFD_BQ25620_EVENT_CHG        BIT(11)
/** TS temperature zone changed. */
#define MFD_BQ25620_EVENT_TS         BIT(16)
/** Thermal shutdown entered. */
#define MFD_BQ25620_EVENT_TSHUT      BIT(19)
/** OTG fault. */
#define MFD_BQ25620_EVENT_OTG_FAULT  BIT(20)
/** System over voltage or short circuit fault. */
#define MFD_BQ25620_EVENT_SYS_FAULT  BIT(21)
/** Battery over current or over voltage fault. */
#define MFD_BQ25620_EVENT_BAT_FAULT  BIT(22)
/** VBUS over voltage or sleep fault. */
#define MFD_BQ25620_EVENT_VBUS_FAULT BIT(23)
/** @} */

struct mfd_bq25620_callback;

/**
 * @brief Event handler of a BQ25620 function driver.
 *
 * Called from the system work queue.
 *
 * @param dev BQ25620 MFD device.
 * @param cb Callback that has been registered.
 * @param events Pending events that are enabled in @p cb.
 */
typedef void (*mfd_bq25620_callback_handler_t)(const struct device *dev,
					       struct mfd_bq25620_callback *cb, uint32_t events);

/**
 * @brief BQ25620 event callback.
 *
 * Embed this structure in the data of the function driver and use
 * CONTAINER_OF() in the handler to get back to the data.
 */
struct mfd_bq25620_callback {
	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	/** @endcond */
	/** Event handler. */
	mfd_bq25620_callback_handler_t handler;
	/** Events to be handled, see @ref MFD_BQ25620_EVENT_WD and following. */
	uint32_t events;
};

/**
 * @brief Register an event callback.
 *
 * The events of the callback are unmasked in the device.
 *
 * @param dev BQ25620 MFD device.
 * @param cb Callback to register.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If the device has no interrupt pin configured.
 * @retval -errno Negative errno code on I2C failure.
 */
int mfd_bq25620_add_callback(const struct device *dev, struct mfd_bq25620_callback *cb);

/**
 * @brief Remove an event callback.
 *
 * Events that are no longer used by any callback are masked in the device.
 *
 * @param dev BQ25620 MFD device.
 * @param cb Callback to remove.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If the callback is not registered.
 * @retval -ENOTSUP If the device has no interrupt pin configured.
 * @retval -errno Negative errno code on I2C failure.
 */
int mfd_bq25620_remove_callback(const struct device *dev, struct mfd_bq25620_callback *cb);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_BQ25620_H_ */
