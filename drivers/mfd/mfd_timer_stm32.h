/*
 * Copyright (c) 2019 Max van Kessel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MFD_MFD_TIMER_STM32_H_
#define ZEPHYR_DRIVERS_MFD_MFD_TIMER_STM32_H_

#include <errno.h>
#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MFD timer data structure
 */
struct mfd_timer_stm32_data {
	TIM_TypeDef *tim;       /**< Base address */
	struct device *clock;   /**< Clock device */
};

typedef void (*mfd_timer_stm32_enable_t)(struct device *dev);

typedef void (*mfd_timer_stm32_disable_t)(struct device *dev);

typedef int (*mfd_timer_stm32_get_cycles_per_sec_t)(struct device *dev,
						    u64_t *cycles);
struct mfd_timer_stm32 {
	mfd_timer_stm32_enable_t enable;
	mfd_timer_stm32_disable_t disable;
	mfd_timer_stm32_get_cycles_per_sec_t get_cycles_per_sec;
};

/**
 * @brief Enable stm32 timer device
 * @param dev	The device to enable
 */
static inline void mfd_timer_stm32_enable(struct device *dev)
{
	const struct mfd_timer_stm32 *api = (const struct mfd_timer_stm32 *)
						dev->driver_api;
	return api->enable(dev);
}

/**
 * @brief Disable stm32 timer device
 * @param dev	The device to disable
 */
static inline void mfd_timer_stm32_disable(struct device *dev)
{
	const struct mfd_timer_stm32 *api = (const struct mfd_timer_stm32 *)
						dev->driver_api;
	return api->disable(dev);
}

/**
 * @brief Get the clock rate (cycles per second)
 * @param dev		Pointer to timer device structure
 * @param cycles	Pointer to the memory to store clock rate
 *			(cycles per second)
 * @retval 0 for success
 * @retval negative errno code
 */
static inline int mfd_timer_stm32_get_cycles_per_sec(struct device *dev,
						     u64_t *cycles)
{
	const struct mfd_timer_stm32 *api = (const struct mfd_timer_stm32 *)
						dev->driver_api;
	return api->get_cycles_per_sec(dev, cycles);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MFD_MFD_TIMER_STM32_H_ */
