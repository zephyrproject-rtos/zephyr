/*
 * Copyright (c) 2024 Navimatix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_COUNTER_RPI_PICO_PIT_H_
#define ZEPHYR_DRIVERS_COUNTER_RPI_PICO_PIT_H_

#include <stdint.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rpi_pico_pit_callback {
	sys_snode_t node;

	/** Actual callback function being called when relevant. */
	counter_top_callback_t callback;

	/** User data of the callback function */
	void *top_user_data;

	/** Slice number of the pit channel the callback belongs to */
	uint32_t slice;
};

/**
 * @brief Get whether the PIT channel has a pending interrupt
 *
 * @param dev Device pointer to the RPI Pico PIT Controller
 * @param channel Channel Id of the RPI Pico Pit Channel for which the interrupt status is checked
 *
 * @return 1 if interrupt is pending, 0 otherwise.
 */
uint32_t counter_rpi_pico_pit_get_pending_int(const struct device *dev, uint32_t channel);

/**
 * @brief Allow RPI Pico PIT channels to register callbacks with their controller
 *
 * @param dev Device pointer to the RPI Pico PIT Controller
 * @param callback A pointer to the callback to be inserted or removed from the list
 * @param set A boolean indicating insertion or removal of the callback
 *
 * @return 0 on success, negative errno otherwise.
 */
int counter_rpi_pico_pit_manage_callback(const struct device *dev,
					 struct rpi_pico_pit_callback *callback, bool set);

/**
 * @brief Get clock frequency from the controller
 *
 * @param dev Device pointer to the RPI Pico PIT Controller
 * @param frequency A pointer to the location where the frequency is to be written
 *
 * @return 0 on success, negative errno otherwise.
 */
int counter_rpi_pico_pit_get_base_frequency(const struct device *dev, uint32_t *frequency);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_COUNTER_RPI_PICO_PIT_H_ */
