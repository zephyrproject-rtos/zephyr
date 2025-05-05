/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Distributed Switch Architecture (DSA)
 */

#ifndef ZEPHYR_INCLUDE_NET_DSA_CORE_H_
#define ZEPHYR_INCLUDE_NET_DSA_CORE_H_

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/phy.h>

/**
 * @brief Distributed Switch Architecture (DSA)
 * @defgroup dsa_core Distributed Switch Architecture (DSA)
 * @since 4.2
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */

#if defined(CONFIG_DSA_PORT_MAX_COUNT)
#define DSA_PORT_MAX_COUNT CONFIG_DSA_PORT_MAX_COUNT
#else
#define DSA_PORT_MAX_COUNT 0
#endif

#if defined(CONFIG_DSA_TAG_SIZE)
#define DSA_TAG_SIZE CONFIG_DSA_TAG_SIZE
#else
#define DSA_TAG_SIZE 0
#endif

/** @endcond */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Macro for DSA port instance initialization.
 *
 * @param port	DSA port node identifier.
 * @param n	DSA instance number.
 * @param cfg	Pointer to dsa_port_config.
 */
#define DSA_PORT_INST_INIT(port, n, cfg)                                                           \
	NET_DEVICE_INIT_INSTANCE(CONCAT(dsa_, n, port), DEVICE_DT_NAME(port), DT_REG_ADDR(port),   \
				 dsa_port_initialize, NULL, &dsa_switch_context_##n, cfg,          \
				 CONFIG_ETH_INIT_PRIORITY, &dsa_eth_api, ETHERNET_L2,              \
				 NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);

/**
 * @brief Macro for DSA switch instance initialization.
 *
 * @param n	DSA instance number.
 * @param _dapi	Pointer to dsa_api.
 * @param data	Pointer to private data.
 * @param fn	DSA port instance init function.
 */
#define DSA_SWITCH_INST_INIT(n, _dapi, data, fn)                                                   \
	struct dsa_switch_context dsa_switch_context_##n = {                                       \
		.dapi = _dapi,                                                                     \
		.prv_data = data,                                                                  \
		.init_ports = 0,                                                                   \
		.num_ports = DT_INST_CHILD_NUM_STATUS_OKAY(n),                                     \
	};                                                                                         \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(n, fn, n);

/** DSA switch context data */
struct dsa_switch_context {
	/** Pointers to all DSA user network interfaces */
	struct net_if *iface_user[DSA_PORT_MAX_COUNT];

	/** Pointer to DSA conduit network interface */
	struct net_if *iface_conduit;

	/** DSA specific API callbacks */
	struct dsa_api *dapi;

	/** Instance specific data */
	void *prv_data;

	/** Number of ports in the DSA switch */
	uint8_t num_ports;

	/** Number of initialized ports in the DSA switch */
	uint8_t init_ports;
};

/**
 * Structure to provide DSA switch api callbacks - it is an augmented
 * struct ethernet_api.
 */
struct dsa_api {
	/** DSA helper callbacks */

	/** Handle receive packet on conduit port for untagging and redirection */
	struct net_if *(*recv)(struct net_if *iface, struct net_pkt *pkt);

	/** Transmit packet on the user port with tagging */
	struct net_pkt *(*xmit)(struct net_if *iface, struct net_pkt *pkt);

	/** Port init */
	int (*port_init)(const struct device *dev);

	/** Port link change */
	void (*port_phylink_change)(const struct device *dev, struct phy_link_state *state,
				    void *user_data);

	/** Port generates random mac address */
	void (*port_generate_random_mac)(uint8_t *mac_addr);

	/** Switch setup */
	int (*switch_setup)(const struct dsa_switch_context *dsa_switch_ctx);
};

/**
 * Structure of DSA port configuration.
 */
struct dsa_port_config {
	/** Port mac address */
	uint8_t mac_addr[6];
	/** Use random mac address or not */
	const bool use_random_mac_addr;
	/** Port index */
	const int port_idx;
	/** PHY device */
	const struct device *phy_dev;
	/** PHY mode */
	const char *phy_mode;
	/** Ethernet device connected to the port */
	const struct device *ethernet_connection;
	/** Instance specific config */
	void *prv_config;
};

/** @cond INTERNAL_HIDDEN */

enum dsa_port_type {
	NON_DSA_PORT,
	DSA_CONDUIT_PORT,
	DSA_USER_PORT,
	DSA_CPU_PORT,
	DSA_PORT,
};

/*
 * DSA port init
 *
 * Returns:
 *  - 0 if ok, < 0 if error
 */
int dsa_port_initialize(const struct device *dev);

/*
 * DSA transmit function
 *
 * param dev: Port device to transmit
 * param pkt: Network packet
 *
 * Returns:
 *  - 0 if ok, < 0 if error
 */
int dsa_xmit(const struct device *dev, struct net_pkt *pkt);

/*
 * DSA receive function
 *
 * param iface: Interface of conduit port
 * param pkt: Network packet
 *
 * Returns:
 *  - Interface to redirect
 */
struct net_if *dsa_recv(struct net_if *iface, struct net_pkt *pkt);

/*
 * DSA ethernet init function to handle flags
 *
 * param iface: Interface of port
 *
 * Returns:
 *  - 0 if ok, < 0 if error
 */
int dsa_eth_init(struct net_if *iface);

/* Ethernet APIs definition for switch ports */
extern const struct ethernet_api dsa_eth_api;

/** @endcond */

/**
 * @brief      Get network interface of a user port
 *
 * @param      iface     Conduit port
 * @param[in]  port_idx  Port index
 *
 * @return     network interface of the user if successful
 * @return     NULL if user port does not exist
 */
struct net_if *dsa_user_get_iface(struct net_if *iface, int port_idx);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_NET_DSA_CORE_H_ */
