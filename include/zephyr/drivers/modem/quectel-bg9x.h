/*
 * Copyright (c) 2022 Sensorfy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_QUECTEL_BG9X_H
#define ZEPHYR_INCLUDE_DRIVERS_QUECTEL_BG9X_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/zephyr.h>
#include <zephyr/device.h>


/**
 * @brief Starts the modem in gnss operation mode.
 *
 * @return 0 on success. Otherwise <0 is returned.
 */
int mdm_bg9x_start_gnss(void);

/**
 * @brief Query gnss position form the modem.
 *
 * @return 0 on success. If no fix is acquired yet -EAGAIN is returned.
 *         Otherwise <0 is returned.
 */
int mdm_bg9x_query_gnss(struct bg9x_gnss_data *data);

/**
 * @brief Stops the gnss operation mode of the modem.
 *
 * @return 0 on success. Otherwise <0 is returned.
 */
int mdm_bg9x_stop_gnss(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_QUECTEL_BG9X_H */
