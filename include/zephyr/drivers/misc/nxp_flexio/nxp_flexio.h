/*
 * Copyright (c) 2024, STRIM, ALC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_NXP_FLEXIO_NXP_FLEXIO_H_
#define ZEPHYR_DRIVERS_MISC_NXP_FLEXIO_NXP_FLEXIO_H_

#include <zephyr/device.h>

/**
 * @struct nxp_flexio_child_res
 * @brief Structure containing information about the required
 * resources for a FlexIO child.
 */
struct nxp_flexio_child_res {
	uint8_t *shifter_index;
	uint8_t shifter_count;
	uint8_t *timer_index;
	uint8_t timer_count;
};

/**
 * @typedef nxp_flexio_child_isr_t
 * @brief Callback API to inform API user that FlexIO triggered interrupt
 *
 * This callback is called from IRQ context.
 */
typedef int (*nxp_flexio_child_isr_t)(void *user_data);

/**
 * @struct nxp_flexio_child
 * @brief Structure containing the required child data for FlexIO
 */
struct nxp_flexio_child {
	nxp_flexio_child_isr_t isr;
	void *user_data;
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

#endif /* ZEPHYR_DRIVERS_MISC_NXP_FLEXIO_NXP_FLEXIO_H_ */
