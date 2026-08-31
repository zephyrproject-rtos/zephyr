/*
 * Copyright (c) 2025 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SMTC_MODEM_HAL_EXT_H
#define SMTC_MODEM_HAL_EXT_H

#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialization of the hal implementation.
 *
 * This must be called before smtc_modem_init
 *
 * @param[in] transceiver The device pointer of the transceiver instance that will be used.
 */
void smtc_modem_hal_init(const struct device *transceiver);

/**
 * @brief Tell the stack owner that the modem engine has work to do.
 *
 * The HAL calls this on every modem interrupt. Code that queues a task from
 * another thread must call it too, because the engine is otherwise asleep
 * until the wake-up the last engine run asked for.
 *
 * The default implementation does nothing.
 */
void lbm_engine_notify(void);

/**
 * @brief Report the battery level the modem answers a DevStatusReq with.
 *
 * @return 0 on external power, 1 to 254 for a battery level, 255 when the
 *         level is unknown.
 *
 * The default implementation returns 255.
 */
uint8_t lbm_battery_level(void);

#ifdef __cplusplus
}
#endif

#endif /* SMTC_MODEM_HAL_EXT_H */
