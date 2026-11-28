/*
 * Copyright (c) 2026 Zephyr SNMP Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_SENSOR_H_
#define APP_SENSOR_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize sensor simulation module.
 */
int app_sensor_init(void);

/**
 * @brief Get simulated temperature (INTEGER, in °C).
 */
int32_t app_sensor_get_temp(void);

/**
 * @brief Set temperature (allows SNMP SET testing on sensor if enabled).
 */
void app_sensor_set_temp(int32_t temp);

/**
 * @brief Get simulated relative humidity (GAUGE32, 0-100%).
 */
uint32_t app_sensor_get_humidity(void);

/**
 * @brief Get sensor node description string (OCTET STRING).
 */
const char *app_sensor_get_description(void);

/**
 * @brief Get sensor table row information by 1-based index.
 * @param index 1 to 3
 * @param name Output pointer for sensor name
 * @param value Output pointer for sensor value
 * @return 0 on success, -ENOENT if index out of range.
 */
int app_sensor_get_table_entry(uint32_t index, const char **name, int32_t *value);

/**
 * @brief Periodic update function to simulate sensor readings variation.
 */
void app_sensor_update_periodic(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_SENSOR_H_ */
