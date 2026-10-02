/*
 * Copyright (c) 2026 Aleksandr Senin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Raw Ethernet transport for the MCUmgr SMP protocol.
 * @ingroup mcumgr_transport_ethernet
 */

#ifndef ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_SMP_ETHERNET_H_
#define ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_SMP_ETHERNET_H_

#include <stdint.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>

/**
 * @brief This allows to use the MCUmgr SMP protocol over raw Ethernet frames.
 * @defgroup mcumgr_transport_ethernet Raw Ethernet transport
 * @ingroup mcumgr_transport
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Address of an SMP over raw Ethernet peer.
 */
struct smp_ethernet_addr {
	/** MAC address of the peer, in transmission order */
	uint8_t mac[NET_ETH_ADDR_LEN];
	/** Index of the network interface to communicate through */
	int iface_index;
};

/**
 * @brief	Enables the raw Ethernet SMP MCUmgr transport, which will process SMP
 *		requests received in Ethernet frames with the configured EtherType.
 *
 * @note	API is not thread safe.
 *
 * @return	0 on success
 * @return	-errno code on failure.
 */
int smp_ethernet_open(void);

/**
 * @brief	Disables the raw Ethernet SMP MCUmgr transport, which will drop SMP
 *		requests received in Ethernet frames.
 *
 * @note	API is not thread safe.
 *
 * @return	0 on success
 * @return	-errno code on failure.
 */
int smp_ethernet_close(void);

/**
 * @brief	Set the destination address for smp_client_object
 *
 * @note	addr should be valid as long as obj is valid.
 *
 * @return	0 on success
 * @return	-errno code on failure.
 */
int smp_client_ethernet_set_dst(struct smp_client_object *obj,
				const struct smp_ethernet_addr *addr);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_SMP_ETHERNET_H_ */
