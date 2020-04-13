/** @file
 @brief DSA definitions and handlers

 */

/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *               Lukasz Majewski <lukma@denx.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_DSA_H_
#define ZEPHYR_INCLUDE_NET_DSA_H_

#include <device.h>
#include <net/net_if.h>

/**
 * @brief LLDP definitions and helpers
 * @defgroup lldp Link Layer Discovery Protocol definitions and helpers
 * @ingroup networking
 * @{
 */

#define NET_DSA_PORT_MAX_COUNT 8

#define DSA_NET_DEVICE_INIT(dev_name, drv_name, init_fn, pm_control_fn,	\
			    data, cfg_info, prio, api, mtu, port_num)	  \
	DEVICE_DEFINE(dev_name, drv_name, init_fn, pm_control_fn, data,	\
		      cfg_info, POST_KERNEL, prio, api);		\
	NET_L2_DATA_INIT(dev_name, 0, NET_L2_GET_CTX_TYPE(ETHERNET_L2)); \
	NET_IF_INIT(dev_name, 0, ETHERNET_L2, mtu, port_num)

#define DSA_STATUS_TICK_MS K_MSEC(1000)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DSA function to adjust packet
 *
 * This is a function for adjusting packets passed from slave DSA interface to
 * master (e.g. implement tail tagging).
 *
 * @param dev Device
 * @param pkt Network packet
 *
 * Returns:
 *  - Pointer to (modified) net_pkt structure
 */
struct net_pkt *dsa_xmit_pkt(struct device *dev, struct net_pkt *pkt);

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
int dsa_tx(struct device *dev, struct net_pkt *pkt);


/**
 * @brief DSA function to get proper interface
 *
 * This is a generic function for assigning proper slave interface after
 * receiving the packet on master.
 *
 * @param iface Network interface
 * @param pkt Network packet
 *
 * Returns:
 *  - Pointer to struct net_if
 */
struct net_if *dsa_get_iface(struct net_if *iface, struct net_pkt *pkt);

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
typedef enum net_verdict (*net_dsa_recv_cb_t)(struct net_if *iface,
					      struct net_pkt *pkt);

/**
 * @brief Register DSA Rx callback functions
 *
 * @param iface Network interface
 * @param cbrx Receive callback function
 *
 * @return 0 if ok, < 0 if error
 */
int dsa_register_recv_callback(struct net_if *iface, net_dsa_recv_cb_t cb);

/**
 * @brief Parse DSA packet
 *
 * @param iface Network interface (master)
 * @param pkt Network packet
 *
 * @return Return NET_DROP if packet was invalid, rejected or we want the stack
 *         to free it. otherwise NET_OK
 */
enum net_verdict net_dsa_recv(struct net_if *iface, struct net_pkt *pkt);

/**
 * @brief Set DSA interface to packet
 *
 * @param iface Network interface (master)
 * @param pkt Network packet
 *
 * @return Return the slave network interface
 */
struct net_if *dsa_recv_set_iface(struct net_if *iface, struct net_pkt **pkt);

/**
 * @brief Write value to DSA register
 */
int dsa_write_reg(u16_t reg_addr, u8_t value);

/**
 * @brief Read value from DSA register
 */
int dsa_read_reg(u16_t reg_addr, u8_t *value);

/**
 * @brief Enable DSA port
 *
 * @param iface Network interface (master)
 * @param port Port number to be enabled
 *
 * @return 0 if ok, < 0 if error
 */
int dsa_enable_port(struct net_if *iface, u8_t port);

/**
 * @brief Structure to provide dsa context
 */

struct dsa_context {
	u8_t num_slave_ports;
	struct k_delayed_work dsa_work;
	struct net_if *iface_slave[NET_DSA_PORT_MAX_COUNT];
	struct net_if *iface_master;
	bool link_up[NET_DSA_PORT_MAX_COUNT];

	int (*sw_read) (u16_t reg_addr, u8_t *value);
	int (*sw_write) (u16_t reg_addr, u8_t value);

	u8_t mac_addr[NET_DSA_PORT_MAX_COUNT][6];
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_NET_DSA_H_ */
