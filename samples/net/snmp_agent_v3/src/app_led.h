/*
 * Copyright (c) 2026 Zephyr SNMP Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_LED_H_
#define APP_LED_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the user LED hardware (led0 alias).
 * @return 0 on success, negative errno on failure.
 */
int app_led_init(void);

/**
 * @brief Get the current state of the LED.
 * @return 1 if ON, 0 if OFF, negative errno on error.
 */
int app_led_get_state(void);

/**
 * @brief Set the LED state.
 * @param state 0 to turn OFF, 1 to turn ON.
 * @return 0 on success, -EINVAL if state is invalid, negative errno on hardware error.
 */
int app_led_set_state(int state);

#ifdef __cplusplus
}
#endif

#endif /* APP_LED_H_ */
