/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright 2025 NXP
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

#if defined(CONFIG_OPENTHREAD_DIAG)
/**
 * Set channel on radio driver.
 *
 * @param[in]  aChannel  The channel that the radio driver should use for operation.
 *
 */
void platformRadioChannelSet(uint8_t aChannel);
#endif /* CONFIG_OPENTHREAD_DIAG */

#if defined(CONFIG_OPENTHREAD_PLATFORM_CARRIER_FUNCTIONS)
/**
 * Start/stop continuous carrier wave transmission.
 */
otError platformRadioTransmitCarrier(otInstance *aInstance, bool aEnable);
#endif /* CONFIG_OPENTHREAD_PLATFORM_CARRIER_FUNCTIONS */

#if defined(CONFIG_OPENTHREAD_PLATFORM_CARRIER_FUNCTIONS)
/**
 * Start/stop modulated carrier wave transmission.
 */
otError platformRadioTransmitModulatedCarrier(otInstance *aInstance, bool aEnable,
					      const uint8_t *aData);
#endif /* CONFIG_OPENTHREAD_PLATFORM_CARRIER_FUNCTIONS */

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

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER)
otError infra_if_init(otInstance *instance, struct net_if *ail_iface);
otError infra_if_start_icmp6_listener(void);
void infra_if_stop_icmp6_listener(void);
otError udp_plat_init(otInstance *ot_instance, struct net_if *ail_iface, struct net_if *ot_iface);
void udp_plat_deinit(void);
otError mdns_plat_socket_init(otInstance *ot_instance, uint32_t ail_iface_idx);
void mdns_plat_monitor_interface(struct net_if *ail_iface);
otError border_agent_init(otInstance *instance);
void border_agent_deinit(void);
otError trel_plat_init(otInstance *instance, struct net_if *ail_iface_ptr);
otError dhcpv6_pd_client_init(otInstance *ot_instance, uint32_t ail_iface_index);
otError dns_upstream_resolver_init(otInstance *ot_instance);
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER */

#endif /* PLATFORM_ZEPHYR_H_ */
