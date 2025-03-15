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

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/phy.h>
#include <zephyr/drivers/pinctrl.h>

/**
 * @brief DSA definitions and helpers
 * @defgroup DSA Distributed Switch Architecture definitions and helpers
 * @since 2.5
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */

#define NET_DSA_PORT_MAX_COUNT 8
#define DSA_STATUS_PERIOD_MS   K_MSEC(1000)

#ifdef CONFIG_DSA_TAG_SIZE
#define DSA_TAG_SIZE CONFIG_DSA_TAG_SIZE
#else
#define DSA_TAG_SIZE 0
#endif

#define DSA_PORT_INIT_INSTANCE(port, dsa)                                                          \
	COND_CODE_1(DT_NUM_PINCTRL_STATES(port),                                                   \
				(PINCTRL_DT_DEFINE(port);), (EMPTY))                               \
	const struct dsa_port_config dsa_##dsa##_port_##port##_config = {                          \
		.mac_addr = DT_PROP_OR(port, local_mac_address, {0}),                              \
		.port_idx = DT_REG_ADDR(port),                                                     \
		.phy_dev = (COND_CODE_1(DT_NODE_HAS_PROP(port, phy_handle),                        \
					DEVICE_DT_GET(DT_PHANDLE(port, phy_handle)),               \
					NULL)),                                                    \
		.phy_mode = DT_PROP_OR(port, phy_connection_type, ""),                             \
		.ethernet_connection = (COND_CODE_1(DT_NODE_HAS_PROP(port, ethernet),              \
					(DEVICE_DT_GET(DT_PHANDLE(port, ethernet))), NULL)),       \
		.pincfg = COND_CODE_1(DT_NUM_PINCTRL_STATES(port),                                 \
				(PINCTRL_DT_DEV_CONFIG_GET(port)), NULL),                          \
	};                                                                                         \
	NET_DEVICE_INIT_INSTANCE(                                                                  \
		CONCAT(dsa_, dsa, _port_, port),                                                   \
		"swp" STRINGIFY(DT_REG_ADDR(port)), DT_REG_ADDR(port), dsa_port_initialize, NULL,  \
				&dsa_context_##dsa, &dsa_##dsa##_port_##port##_config,             \
				CONFIG_ETH_INIT_PRIORITY, &dsa_eth_api, ETHERNET_L2,               \
				NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);

#define DSA_INIT_INSTANCE(n, _dapi, data)                                                          \
	struct dsa_context dsa_context_##n = {                                                     \
		.dapi = _dapi,                                                                     \
		.prv_data = (void *)data,                                                          \
		.init_ports = 0,                                                                   \
		.num_ports = DT_INST_CHILD_NUM_STATUS_OKAY(n),                                     \
	};                                                                                         \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(n, DSA_PORT_INIT_INSTANCE, n);

#ifdef __cplusplus
extern "C" {
#endif

/** DSA context data */
struct dsa_context {
	/** Pointers to all DSA slave network interfaces */
	struct net_if *iface_slave[NET_DSA_PORT_MAX_COUNT];

	/** Pointer to DSA master network interface */
	struct net_if *iface_master;

	/** DSA specific API callbacks - filled in the switch IC driver */
	struct dsa_api *dapi;

#if defined(CONFIG_NET_DSA_LEGACY)
	/** DSA related work (e.g. monitor if network interface is up) */
	struct k_work_delayable dsa_work;

	/** Status of each port */
	bool link_up[NET_DSA_PORT_MAX_COUNT];

	/** Number of slave ports in the DSA switch */
	uint8_t num_slave_ports;
#endif
	/** Number of ports in the DSA switch */
	uint8_t num_ports;

	/** Number of initialized ports in the DSA switch */
	uint8_t init_ports;

	/** Instance specific data */
	void *prv_data;
};

/**
 * @brief Structure to provide DSA switch api callbacks - it is an augmented
 * struct ethernet_api.
 */
struct dsa_api {
	/*
	 * Callbacks required for DSA switch initialization and configuration.
	 *
	 * Each switch instance (e.g. two KSZ8794 ICs) would have its own struct
	 * dsa_context.
	 */
	/** Read value from DSA register */
	int (*switch_read)(const struct device *dev, uint16_t reg_addr, uint8_t *value);
	/** Write value to DSA register */
	int (*switch_write)(const struct device *dev, uint16_t reg_addr, uint8_t value);

	/** Program (set) mac table entry in the DSA switch */
	int (*switch_set_mac_table_entry)(const struct device *dev, const uint8_t *mac,
					  uint8_t fw_port, uint16_t tbl_entry_idx, uint16_t flags);

	/** Read mac table entry from the DSA switch */
	int (*switch_get_mac_table_entry)(const struct device *dev, uint8_t *buf,
					  uint16_t tbl_entry_idx);

	/*
	 * DSA helper callbacks
	 */
#if defined(CONFIG_NET_DSA_LEGACY)
	/** Function to get proper LAN{123} interface */
	struct net_if *(*dsa_get_iface)(struct net_if *iface, struct net_pkt *pkt);

	struct net_pkt *(*dsa_xmit_pkt)(struct net_if *iface, struct net_pkt *pkt);
#endif
	/** Handle receive packet for untagging and redirection */
	struct net_if *(*recv)(struct net_if *iface, struct net_pkt **pkt);

	/** transmit packet with tagging */
	struct net_pkt *(*xmit)(struct net_if *iface, struct net_pkt *pkt);

	/** port init */
	int (*port_init)(const struct device *dev);

	/** port link change */
	void (*port_phylink_change)(const struct device *dev, struct phy_link_state *state,
				    void *user_data);

	/** switch setup */
	int (*switch_setup)(const struct dsa_context *dsa_ctx);
};

enum dsa_port_type {
	NON_DSA_PORT,
	DSA_MASTER_PORT,
	DSA_SLAVE_PORT,
	DSA_CPU_PORT,
	DSA_PORT,
};

struct dsa_port_config {
	uint8_t mac_addr[6];
	const int port_idx;
	const struct device *phy_dev;
	const char *phy_mode;
	const struct device *ethernet_connection;
	const struct pinctrl_dev_config *pincfg;
};

/**
 * @brief DSA port init
 *
 * @param dev Device
 *
 * Returns:
 *  - 0 if ok, < 0 if error
 */
int dsa_port_initialize(const struct device *dev);

/**
 * @brief DSA transmit function
 *
 * @param dev Device
 * @param pkt Network packet
 *
 * Returns:
 *  - 0 if ok, < 0 if error
 */
int dsa_xmit(const struct device *dev, struct net_pkt *pkt);

/**
 * @brief DSA receive function
 *
 * @param iface Interface
 * @param pkt Network packet
 *
 * Returns:
 *  - Interface to redirect
 */
struct net_if *dsa_recv(struct net_if *iface, struct net_pkt **pkt);

/**
 * @brief Ethernet APIs definition for switch ports
 */
extern const struct ethernet_api dsa_eth_api;

/**
 * @endcond
 */

#if defined(CONFIG_NET_DSA_LEGACY)
/**
 * @brief Structure to provide mac address for each LAN interface
 */

struct dsa_slave_config {
	/** MAC address for each LAN{123.,} ports */
	uint8_t mac_addr[6];
};

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
 * @brief Set DSA interface to packet
 *
 * @param iface Network interface (master)
 * @param pkt Network packet
 *
 * @return Return the slave network interface
 */
struct net_if *dsa_net_recv(struct net_if *iface, struct net_pkt **pkt);

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
#endif

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
typedef enum net_verdict (*dsa_net_recv_cb_t)(struct net_if *iface, struct net_pkt *pkt);

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
 * @brief DSA helper function to check if port is master
 *
 * @param iface Network interface (master)
 *
 * Returns:
 *  - true if ok, false otherwise
 */
bool dsa_port_is_master(struct net_if *iface);

/**
 * @brief      Get network interface of a slave port
 *
 * @param      iface      Master port
 * @param[in]  slave_num  Slave port number
 *
 * @return     network interface of the slave if successful
 * @return     NULL if slave port does not exist
 */
struct net_if *dsa_slave_get_iface(struct net_if *iface, int slave_num);

/**
 * @brief      Read from DSA switch register
 *
 * @param      iface     The interface
 * @param[in]  reg_addr  The register address
 * @param      value     The value
 *
 * @return     0 if successful, negative if error
 */
int dsa_switch_read(struct net_if *iface, uint16_t reg_addr, uint8_t *value);

/**
 * @brief      Write to DSA switch
 *
 * @param      iface     The interface
 * @param[in]  reg_addr  The register address
 * @param[in]  value     The value
 *
 * @return     { description_of_the_return_value }
 */
int dsa_switch_write(struct net_if *iface, uint16_t reg_addr, uint8_t value);

/**
 * @brief      Write static MAC table entry
 *
 * @param      iface          Master DSA interface
 * @param[in]  mac            MAC address
 * @param[in]  fw_port        The firmware port
 * @param[in]  tbl_entry_idx  Table entry index
 * @param[in]  flags          Flags
 *
 * @return     0 if successful, negative if error
 */
int dsa_switch_set_mac_table_entry(struct net_if *iface, const uint8_t *mac, uint8_t fw_port,
				   uint16_t tbl_entry_idx, uint16_t flags);

/**
 * @brief      Read static MAC table entry
 *
 * @param      iface          Master DSA interface
 * @param      buf            Buffer to receive MAC address
 * @param[in]  tbl_entry_idx  Table entry index
 *
 * @return     0 if successful, negative if error
 */
int dsa_switch_get_mac_table_entry(struct net_if *iface, uint8_t *buf, uint16_t tbl_entry_idx);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_NET_DSA_H_ */
