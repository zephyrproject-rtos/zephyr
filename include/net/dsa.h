/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *               Lukasz Majewski <lukma@denx.de>
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief DSA definitions and handlers
 */

#ifndef ZEPHYR_INCLUDE_NET_DSA_H_
#define ZEPHYR_INCLUDE_NET_DSA_H_

#include <device.h>
#include <net/net_if.h>

/**
 * @brief DSA definitions and helpers
 * @defgroup DSA - Distributed Switch Architecture definitions and helpers
 * @ingroup networking
 * @{
 */

#define NET_DSA_PORT_MAX_COUNT 8
#define DSA_STATUS_PERIOD_MS K_MSEC(1000)

/*
 * Size of the DSA TAG:
 * - KSZ8794 - 1 byte
 */
#if defined(CONFIG_DSA_KSZ8794) && defined(CONFIG_DSA_KSZ8794_TAIL_TAGGING)
#define DSA_TAG_SIZE 1
#else
#define DSA_TAG_SIZE 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DSA generic transmit function
 *
 * This is a generic function for passing packets from slave DSA interface to
 * master.
 *
 * @param dev Device
 * @param pkt Network packet
 *
 * Returns:
 *  - 0 if ok (packet sent via master iface), < 0 if error
 */
int dsa_tx(const struct device *dev, struct net_pkt *pkt);

/**
 * @brief DSA (MGMT) Receive packet callback
 *
 * Callback gets called upon receiving packet. It is responsible for
 * freeing packet or indicating to the stack that it needs to free packet
 * by returning correct net_verdict.
 *
 * Returns:
 *  - NET_DROP, if packet was invalid, rejected or we want the stack to free it.
 *    In this case the core stack will free the packet.
 *  - NET_OK, if the packet was accepted, in this case the ownership of the
 *    net_pkt goes to callback and core network stack will forget it.
 */
typedef enum net_verdict (*dsa_net_recv_cb_t)(struct net_if *iface,
					      struct net_pkt *pkt);

/**
 * @brief Register DSA Rx callback functions
 *
 * @param iface Network interface
 * @param cb Receive callback function
 *
 * @return 0 if ok, < 0 if error
 */
int dsa_register_recv_callback(struct net_if *iface, dsa_net_recv_cb_t cb);

/**
 * @brief Set DSA interface to packet
 *
 * @param iface Network interface (master)
 * @param pkt Network packet
 *
 * @return Return the slave network interface
 */
struct net_if *dsa_net_recv(struct net_if *iface, struct net_pkt **pkt);

/**
 * @brief DSA helper function to get context
 *
 * This is the helper function to provide DSA context pointer.
 * (master and LAN interfaces shall have pointer to struct dsa_context
 * available)
 *
 * @param iface Network interface (master)
 *
 * Returns:
 *  - 0 if ok (packet sent via master iface), < 0 if error
 */
struct dsa_context *dsa_get_context(struct net_if *iface);

/**
 * @brief Pointer to master interface send function
 */
typedef int (*dsa_send_t)(const struct device *dev, struct net_pkt *pkt);

/**
 * @brief DSA helper function to register transmit function for master
 *
 * @param iface Network interface (master)
 * @param fn Pointer to master interface send method
 *
 * Returns:
 *  - 0 if ok, < 0 if error
 */
int dsa_register_master_tx(struct net_if *iface, dsa_send_t fn);

/**
 * @brief DSA helper function to check if port is master
 *
 * @param iface Network interface (master)
 *
 * Returns:
 *  - true if ok, false otherwise
 */
bool dsa_is_port_master(struct net_if *iface);

/** DSA context data */
struct dsa_context {
	/** Pointers to all DSA slave network interfaces */
	struct net_if *iface_slave[NET_DSA_PORT_MAX_COUNT];

	/** Pointer to DSA master network interface */
	struct net_if *iface_master;

	/** DSA specific API callbacks - filled in the switch IC driver */
	struct dsa_api *dapi;

	/** DSA related work (e.g. monitor if network interface is up) */
	struct k_delayed_work dsa_work;

	/** The switch_id, which equals to the reg property number from
	 * DTS is used to distinct between many connected switches.
	 */
	uint8_t switch_id;

	/** Number of slave ports in the DSA switch */
	uint8_t num_slave_ports;

	/** Status of each port */
	bool link_up[NET_DSA_PORT_MAX_COUNT];
};

/**
 * @brief Structure to provide DSA switch api callbacks - it is an augmented
 * struct ethernet_api.
 */
struct dsa_api {
	/** Function to get proper LAN{123} interface */
	struct net_if *(*dsa_get_iface)(struct net_if *iface,
					struct net_pkt *pkt);
	/*
	 * Callbacks required for DSA switch initialization and configuration.
	 *
	 * Each switch instance (e.g. two KSZ8794 ICs) would have its own struct
	 * dsa_context.
	 */
	/** Read value from DSA register */
	int (*switch_read)(int switch_id, uint16_t reg_addr, uint8_t *value);
	/** Write value to DSA register */
	int (*switch_write)(int switch_id, uint16_t reg_addr, uint8_t value);

	/** Enable single port in the DSA switch */
	int (*switch_enable_port)(int switch_id, uint8_t port);

	/** Program (set) mac table entry in the DSA switch */
	int (*switch_set_mac_table_entry)(int switch_id,
					  const uint8_t *mac, uint8_t fw_port,
					  uint16_t tbl_entry_idx,
					  uint16_t flags);

	/** Read mac table entry from the DSA switch */
	int (*switch_get_mac_table_entry)(int switch_id, uint8_t *buf,
					  uint16_t tbl_entry_idx);

	/*
	 * DSA helper callbacks
	 */
	struct net_pkt *(*dsa_xmit_pkt)(struct net_if *iface,
					struct net_pkt *pkt);
};

/**
 * @brief Structure to provide mac address for each LAN interface
 */

struct dsa_slave_config {
	/** MAC address for each LAN{123.,} ports */
	uint8_t mac_addr[6];
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_NET_DSA_H_ */
