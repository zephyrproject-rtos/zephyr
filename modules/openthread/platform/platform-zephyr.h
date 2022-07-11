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
#include <zephyr/net/net_pkt.h>

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
 * This function performs UART driver processing.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 *
 */
void platformUartProcess(otInstance *aInstance);

/**
 * Outer component calls this method to notify UART driver that it should
 * switch to panic mode and work in synchronous way.
 */
void platformUartPanic(void);

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


/**
 * Notify OpenThread task about new rx message.
 */
int notify_new_rx_frame(struct net_pkt *pkt);

/**
 * Notify OpenThread task about new tx message.
 */
int notify_new_tx_frame(struct net_pkt *pkt);

#endif  /* PLATFORM_POSIX_H_ */
