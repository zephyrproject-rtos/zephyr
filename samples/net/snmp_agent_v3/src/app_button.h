/*
 * Copyright (c) 2026 Zephyr SNMP Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_BUTTON_H_
#define APP_BUTTON_H_

#include <stdint.h>

struct snmp_agent;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize user button hardware (sw0 alias) with GPIO interrupt and workqueue.
 *
 * @param agent Agent through which a button press dispatches its SNMP notification.
 *              The caller owns the storage; it must outlive the module.
 * @return 0 on success, negative errno on failure.
 */
int app_button_init(struct snmp_agent *agent);

/**
 * @brief Get total user button press count.
 */
uint32_t app_button_get_counter(void);

/**
 * @brief Explicitly trigger the buttonPressed SNMP Trap/Notification.
 *        Can be called by button ISR or from shell / test code.
 * @return 0 on success, negative errno on failure.
 */
int app_button_trigger_notification(void);

/**
 * @brief Simulate button press (increments counter and triggers notification).
 * @return 0 on success, negative errno on failure.
 */
int app_button_simulate_press(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_BUTTON_H_ */
