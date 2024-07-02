/** @file
 * @brief Routines for network subsystem initialization.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_CONFIG_H_
#define ZEPHYR_INCLUDE_NET_NET_CONFIG_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ieee802154_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network configuration library
 * @defgroup net_config Network Configuration Library
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */

#if defined(CONFIG_NET_CONFIG_USE_YAML)
#include "net_init_config.inc"
#else
#if !defined(NET_CONFIG_IFACE_COUNT)
#define NET_CONFIG_IFACE_COUNT 0
#endif
#if !defined(NET_CONFIG_DHCPV4_SERVER_INSTANCE_COUNT)
#define NET_CONFIG_DHCPV4_SERVER_INSTANCE_COUNT 0
#endif
#endif

struct net_init_config_ipv6_prefix {
	struct in6_addr address;
	uint32_t lifetime;
	uint8_t len;
};

struct net_init_config_dhcpv6 {
	bool do_request_addr : 1;
	bool do_request_prefix : 1;
	bool is_enabled : 1;
};

/* Compliance checker complains about this valid macro so use a define */
#define __z_net_config_check_dhcpv6 \
	IF_ENABLED(CONFIG_NET_DHCPV6, (struct net_init_config_dhcpv6 dhcpv6;))

struct net_init_config_ipv6 {
	struct in6_addr address[NET_IF_MAX_IPV6_ADDR];
	struct in6_addr mcast_address[NET_IF_MAX_IPV6_MADDR];
	struct net_init_config_ipv6_prefix prefix[NET_IF_MAX_IPV6_PREFIX];
	uint8_t hop_limit;
	uint8_t mcast_hop_limit;

	__z_net_config_check_dhcpv6
	bool is_enabled : 1;
};

struct net_init_config_dhcpv4_server {
	struct in_addr base_addr;
	bool is_enabled : 1;
};

#define __z_net_config_check_dhcpv4_server				 \
	IF_ENABLED(CONFIG_NET_DHCPV4_SERVER,				 \
		   (struct net_init_config_dhcpv4_server dhcpv4_server;))

struct net_init_config_addr_ipv4 {
	/** IPv4 address */
	struct in_addr ipv4;
	/** Netmask */
	uint8_t netmask_len;
};

struct net_init_config_ipv4 {
	struct net_init_config_addr_ipv4 address[NET_IF_MAX_IPV4_ADDR];
	struct in_addr mcast_address[NET_IF_MAX_IPV4_MADDR];
	struct in_addr gateway;

	__z_net_config_check_dhcpv4_server
	uint8_t ttl;
	uint8_t mcast_ttl;
	bool is_dhcpv4 : 1;
	bool is_ipv4_autoconf : 1;
	bool is_enabled : 1;
};

struct net_init_config_vlan {
	uint16_t tag;
	bool is_enabled : 1;
};

#define __z_net_config_check_iface_options				 \
	IF_ENABLED(CONFIG_NET_IPV6, (struct net_init_config_ipv6 ipv6;)) \
	IF_ENABLED(CONFIG_NET_IPV4, (struct net_init_config_ipv4 ipv4;)) \
	IF_ENABLED(CONFIG_NET_VLAN, (struct net_init_config_vlan vlan;))

struct net_init_config_iface {
	const char *name;
	const char *new_name;
	const char *dev;
	enum net_if_flag flags;

	__z_net_config_check_iface_options
	int bind_to;
	bool is_default : 1;
};

#define __z_net_config_check_ieee802154_security		    \
	IF_ENABLED(CONFIG_NET_L2_IEEE802154_SECURITY,		    \
		   (struct ieee802154_security_params sec_params;))

struct net_init_config_ieee802154 {
	int bind_to;
	uint16_t channel;
	uint16_t pan_id;
	int16_t tx_power;

	__z_net_config_check_ieee802154_security
};

struct net_init_config_sntp {
	const char *server;
	int timeout;
	int bind_to;
};

#define __z_net_config_check_other_options				\
	IF_ENABLED(CONFIG_NET_L2_IEEE802154,				\
		   (struct net_init_config_ieee802154 ieee802154;))	\
	IF_ENABLED(CONFIG_SNTP, (struct net_init_config_sntp sntp;))

struct net_init_config {
	struct net_init_config_iface iface[NET_CONFIG_IFACE_COUNT];

	__z_net_config_check_other_options
	bool is_ieee802154 : 1;
	bool is_sntp : 1;
};

const struct net_init_config *net_config_get_init_config(void);
/** @endcond */

/* Flags that tell what kind of functionality is needed by the client. */
/**
 * @brief Application needs routers to be set so that connectivity to remote
 * network is possible. For IPv6 networks, this means that the device should
 * receive IPv6 router advertisement message before continuing.
 */
#define NET_CONFIG_NEED_ROUTER 0x00000001

/**
 * @brief Application needs IPv6 subsystem configured and initialized.
 * Typically this means that the device has IPv6 address set.
 */
#define NET_CONFIG_NEED_IPV6   0x00000002

/**
 * @brief Application needs IPv4 subsystem configured and initialized.
 * Typically this means that the device has IPv4 address set.
 */
#define NET_CONFIG_NEED_IPV4   0x00000004

/**
 * @brief Initialize this network application.
 *
 * @details This will call net_config_init_by_iface() with NULL network
 *          interface.
 *
 * @param app_info String describing this application.
 * @param flags Flags related to services needed by the client.
 * @param timeout How long to wait the network setup before continuing
 * the startup.
 *
 * @return 0 if ok, <0 if error.
 */
int net_config_init(const char *app_info, uint32_t flags, int32_t timeout);

/**
 * @brief Initialize this network application using a specific network
 * interface.
 *
 * @details If network interface is set to NULL, then the default one
 *          is used in the configuration.
 *
 * @param iface Initialize networking using this network interface.
 * @param app_info String describing this application.
 * @param flags Flags related to services needed by the client.
 * @param timeout How long to wait the network setup before continuing
 * the startup.
 *
 * @return 0 if ok, <0 if error.
 */
int net_config_init_by_iface(struct net_if *iface, const char *app_info,
			     uint32_t flags, int32_t timeout);

/**
 * @brief Initialize this network application.
 *
 * @details If CONFIG_NET_CONFIG_AUTO_INIT is set, then this function is called
 *          automatically when the device boots. If that is not desired, unset
 *          the config option and call the function manually when the
 *          application starts.
 *
 * @param dev Network device to use. The function will figure out what
 *        network interface to use based on the device. If the device is NULL,
 *        then default network interface is used by the function.
 * @param app_info String describing this application.
 *
 * @return 0 if ok, <0 if error.
 */
int net_config_init_app(const struct device *dev, const char *app_info);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_NET_CONFIG_H_ */
