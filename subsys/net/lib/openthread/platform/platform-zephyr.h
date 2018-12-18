/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief
 *   This file includes the Zephyr platform-specific initializers.
 */

#ifndef PLATFORM_ZEPHYR_H_
#define PLATFORM_ZEPHYR_H_

#include <stdint.h>

#include <openthread/instance.h>

/**
 * This function initializes the alarm service used by OpenThread.
 *
 */
void platformAlarmInit(void);

/**
 * This function performs alarm driver processing.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 *
 */
void platformAlarmProcess(otInstance *aInstance);

/**
 * This function initializes the radio service used by OpenThread.
 *
 */
void platformRadioInit(void);

/**
 * This function performs radio driver processing.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 *
 */
void platformRadioProcess(otInstance *aInstance);

/**
 * Get current channel from radio driver.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 *
 * @return Current channel radio driver operates on.
 *
 */
uint16_t platformRadioChannelGet(otInstance *aInstance);

/**
 * This function initializes the random number service used by OpenThread.
 *
 */
void platformRandomInit(void);


/**
 *  Initialize platform Shell driver.
 */
void platformShellInit(otInstance *aInstance);

#endif  /* PLATFORM_POSIX_H_ */
