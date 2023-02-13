/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USBC_TIMER_H_
#define ZEPHYR_SUBSYS_USBC_TIMER_H_

#include <zephyr/kernel.h>

/**
 * @brief USB-C Timer Object
 */
struct usbc_timer_t {
	/** kernel timer */
	struct k_timer timer;
	/** timeout value in ms */
	uint32_t timeout_ms;
	/** flags to track timer status */
	atomic_t flags;
};

/**
 * @brief Initialize a timer
 *
 * @param usbc_timer timer object
 * @param timeout_ms timer timeout in ms
 */
void usbc_timer_init(struct usbc_timer_t *usbc_timer, uint32_t timeout_ms);

/**
 * @brief Start a timer
 *
 * @param usbc_timer timer object
 */
void usbc_timer_start(struct usbc_timer_t *usbc_timer);

/**
 * @brief Check if a timer has expired
 *
 * @param usbc_timer timer object
 * @retval true if the timer has expired
 */
bool usbc_timer_expired(struct usbc_timer_t *usbc_timer);

/**
 * @brief Check if a timer has been started
 *
 * @param usbc_timer timer object
 * @retval true if the timer is running
 */
bool usbc_timer_running(struct usbc_timer_t *usbc_timer);

/**
 * @brief Stop a timer
 *
 * @param usbc_timer timer object
 */
void usbc_timer_stop(struct usbc_timer_t *usbc_timer);

#endif /* ZEPHYR_SUBSYS_USBC_TIMER_H_ */
