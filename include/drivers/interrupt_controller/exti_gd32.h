/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for External interrupt/event controller in GD32 MCUs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_GD32_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_GD32_H_

#include <zephyr/types.h>

/* device name */
#define GD32_EXTI_NAME "gd32-exti"

/* external interrupt and event  */
typedef enum {
	GD32_EXTI_INTERRUPT = 0,                                        /*!< EXTI interrupt mode */
	GD32_EXTI_EVENT                                                 /*!< EXTI event mode */
} gd32_exti_mode_enum;

/* interrupt trigger mode */
typedef enum {
	GD32_EXTI_TRIG_RISING = 0,                                      /*!< EXTI rising edge trigger */
	GD32_EXTI_TRIG_FALLING,                                         /*!< EXTI falling edge trigger */
	GD32_EXTI_TRIG_BOTH,                                            /*!< EXTI rising edge and falling edge trigger */
	GD32_EXTI_TRIG_NONE                                             /*!< without rising edge or falling edge trigger */
} gd32_exti_trig_type_enum;


/* callback for exti interrupt */
typedef void (*gd32_exti_callback_t) (int line, void *user);

typedef int (*gd32_exti_set_callback_fn)(const struct device *, int line, gd32_exti_callback_t cb, void *arg);
typedef void (*gd32_exti_unset_callback_fn)(const struct device *, int line);
typedef void (*gd32_exti_enable_fn)(const struct device *dev, int line);
typedef void (*gd32_exti_disable_fn)(const struct device *dev, int line);
typedef void (*gd32_exti_trigger_fn)(const struct device *dev, int line, int trigger);

struct gd32_exti_driver_api {
	gd32_exti_enable_fn enable;
	gd32_exti_disable_fn disable;
	gd32_exti_trigger_fn trigger;
	gd32_exti_set_callback_fn set_callback;
	gd32_exti_unset_callback_fn unset_callback;
};

static inline void gd32_exti_enable(const struct device *dev, int line)
{
	const struct gd32_exti_driver_api *api =
		(const struct gd32_exti_driver_api *)dev->api;

	api->enable(dev, line);
}


static inline void gd32_exti_disable(const struct device *dev, int line)
{
	const struct gd32_exti_driver_api *api =
		(const struct gd32_exti_driver_api *)dev->api;

	api->disable(dev, line);
}

static inline void gd32_exti_trigger(const struct device *dev, int line, int trigger)
{
	const struct gd32_exti_driver_api *api =
		(const struct gd32_exti_driver_api *)dev->api;

	api->trigger(dev, line, trigger);
}

static inline int gd32_exti_set_callback(const struct device *dev, int line, gd32_exti_callback_t cb, void *arg)
{
	const struct gd32_exti_driver_api *api =
		(const struct gd32_exti_driver_api *)dev->api;

	return api->set_callback(dev, line, cb, arg);
}

/**
 * @brief Enable / disable GPIO port clock.
 *
 * @param dev GPIO port device pointer
 * @param on boolean for on/off clock request
 */
static inline void gd32_exti_unset_callback(const struct device *dev, int line)
{
	const struct gd32_exti_driver_api *api =
		(const struct gd32_exti_driver_api *)dev->api;

	api->unset_callback(dev, line);
}


#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_GD32_H_ */
