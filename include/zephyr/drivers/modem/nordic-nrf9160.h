/*
 * Copyright (c) Sendrato B.V. 2023
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_NORDIC_NRF9160_H
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_NORDIC_NRF9160_H

#include <zephyr/types.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int mdm_nrf9160_reset(const struct device* dev);

#ifdef __cplusplus
}
#endif

#endif // ZEPHYR_INCLUDE_DRIVERS_MODEM_NORDIC_NRF9160_H
