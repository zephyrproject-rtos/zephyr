/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief DHCPv6 client
 */

#ifndef ZEPHYR_INCLUDE_NET_DHCPV6_H_
#define ZEPHYR_INCLUDE_NET_DHCPV6_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DHCPv6
 * @defgroup dhcpv6 DHCPv6
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/** Current state of DHCPv6 client address/prefix negotiation. */
enum net_dhcpv6_state {
	NET_DHCPV6_DISABLED,
	NET_DHCPV6_INIT,
	NET_DHCPV6_SOLICITING,
	NET_DHCPV6_REQUESTING,
	NET_DHCPV6_CONFIRMING,
	NET_DHCPV6_RENEWING,
	NET_DHCPV6_REBINDING,
	NET_DHCPV6_INFO_REQUESTING,
	NET_DHCPV6_BOUND,
} __packed;

#define DHCPV6_TID_SIZE 3

#ifndef CONFIG_NET_DHCPV6_DUID_MAX_LEN
#define CONFIG_NET_DHCPV6_DUID_MAX_LEN 22
#endif

struct net_dhcpv6_duid_raw {
	uint16_t type;
	uint8_t buf[CONFIG_NET_DHCPV6_DUID_MAX_LEN];
} __packed;

struct net_dhcpv6_duid_storage {
	struct net_dhcpv6_duid_raw duid;
	uint8_t length;
};

struct net_if;

/** @endcond */

/** @brief DHCPv6 client configuration parameters. */
struct net_dhcpv6_params {
	bool request_addr : 1; /**< Request IPv6 address. */
	bool request_prefix : 1; /**< Request IPv6 prefix. */
};

/**
 *  @brief Start DHCPv6 client on an iface
 *
 *  @details Start DHCPv6 client on a given interface. DHCPv6 client will start
 *  negotiation for IPv6 address and/or prefix, depending on the configuration.
 *  Once the negotiation is complete, IPv6 address/prefix details will be added
 *  to the interface.
 *
 *  @param iface A valid pointer to a network interface
 *  @param params DHCPv6 client configuration parameters.
 */
void net_dhcpv6_start(struct net_if *iface, struct net_dhcpv6_params *params);

/**
 *  @brief Stop DHCPv6 client on an iface
 *
 *  @details Stop DHCPv6 client on a given interface. DHCPv6 client
 *  will remove all configuration obtained from a DHCP server from the
 *  interface and stop any further negotiation with the server.
 *
 *  @param iface A valid pointer to a network interface
 */
void net_dhcpv6_stop(struct net_if *iface);

/**
 *  @brief Restart DHCPv6 client on an iface
 *
 *  @details Restart DHCPv6 client on a given interface. DHCPv6 client
 *  will restart the state machine without any of the initial delays.
 *
 *  @param iface A valid pointer to a network interface
 */
void net_dhcpv6_restart(struct net_if *iface);

/** @cond INTERNAL_HIDDEN */

/**
 *  @brief DHCPv6 state name
 *
 *  @internal
 */
const char *net_dhcpv6_state_name(enum net_dhcpv6_state state);

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_DHCPV6_H_ */
