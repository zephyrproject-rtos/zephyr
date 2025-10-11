/*
 * Copyright (c) 2024, STRIM, ALC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for NXP FlexIO driver
 * @ingroup nxp_flexio_interface
 */

#ifndef ZEPHYR_DRIVERS_MISC_NXP_FLEXIO_NXP_FLEXIO_H_
#define ZEPHYR_DRIVERS_MISC_NXP_FLEXIO_NXP_FLEXIO_H_

/**
 * @brief Interfaces for NXP FlexIO.
 * @defgroup nxp_flexio_interface NXP FlexIO
 * @ingroup misc_interfaces
 *
 * @{
 */

#include <zephyr/device.h>

/**
 * @brief Structure containing information about the required resources for a FlexIO child.
 */
struct nxp_flexio_child_res {
	/** Output array where assigned shifter indices are stored.
	 *
	 * Must point to an array with at least @ref shifter_count entries. Values are 0-based
	 * hardware indices valid for the bound FlexIO.
	 */
	uint8_t *shifter_index;
	/** Number of shifter indices required by the child. */
	uint8_t shifter_count;
	/** Output array where assigned timer indices are stored.
	 *
	 * Must point to an array with at least @ref timer_count entries.  Values are 0-based
	 * hardware indices valid for the bound FlexIO.
	 */
	uint8_t *timer_index;
	/** Number of timer indices required by the child. */
	uint8_t timer_count;
};

/**
 * @typedef nxp_flexio_child_isr_t
 * @brief Callback API to inform API user that FlexIO triggered interrupt
 *
 * The controller calls this from IRQ context whenever one of the child's mapped shifters or timers
 * has a pending and enabled interrupt.
 *
 * @param user_data Opaque pointer provided at attachment time.
 */
typedef int (*nxp_flexio_child_isr_t)(void *user_data);

/**
 * @struct nxp_flexio_child
 * @brief Structure containing the required child data for FlexIO
 */
struct nxp_flexio_child {
	/** ISR called from the FlexIO controller's IRQ handler.
	 * May be @c NULL if the child does not require IRQ notifications.
	 */
	nxp_flexio_child_isr_t isr;
	/** Opaque pointer passed to @ref isr function when it is invoked. */
	void *user_data;
	/** Resource requirements and output indices filled by nxp_flexio_child_attach(). */
	struct nxp_flexio_child_res res;
};

/**
 * @brief Enable FlexIO IRQ
 * @param dev Pointer to the device structure for the FlexIO driver instance
 */
void nxp_flexio_irq_enable(const struct device *dev);

/**
 * @brief Disable FlexIO IRQ
 * @param dev Pointer to the device structure for the FlexIO driver instance
 */
void nxp_flexio_irq_disable(const struct device *dev);

/**
 * @brief Lock FlexIO mutex.
 * @param dev Pointer to the device structure for the FlexIO driver instance
 */
void nxp_flexio_lock(const struct device *dev);

/**
 * @brief Unlock FlexIO mutex.
 * @param dev Pointer to the device structure for the FlexIO driver instance
 */
void nxp_flexio_unlock(const struct device *dev);

/**
 * @brief Obtain the clock rate of sub-system used by the FlexIO
 * @param dev Pointer to the device structure for the FlexIO driver instance
 * @param[out] rate Subsystem clock rate
 * @retval 0 on successful rate reading.
 * @retval -EAGAIN if rate cannot be read. Some drivers do not support returning the rate when the
 *         clock is off.
 * @retval -ENOTSUP if reading the clock rate is not supported for the given sub-system.
 * @retval -ENOSYS if the interface is not implemented.
 */
int nxp_flexio_get_rate(const struct device *dev, uint32_t *rate);

/**
 * @brief Attach flexio child to flexio controller
 * @param dev Pointer to the device structure for the FlexIO driver instance
 * @param child Pointer to flexio child
 * @retval 0 if successful
 * @retval -ENOBUFS if there are not enough available resources
 */
int nxp_flexio_child_attach(const struct device *dev,
	const struct nxp_flexio_child *child);

/**
 * @}
 */

#endif /* ZEPHYR_DRIVERS_MISC_NXP_FLEXIO_NXP_FLEXIO_H_ */
