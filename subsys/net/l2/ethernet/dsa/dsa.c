/** @file
 * @brief DSA related functions
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dsa, CONFIG_NET_DSA_LOG_LEVEL);

#include <errno.h>
#include <stdlib.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/dsa.h>

/*
 * Store, in the ethernet_context for master interface, the original
 * eth_tx() function, which will send packet with tag appended.
 */
int dsa_register_master_tx(struct net_if *iface, dsa_send_t fn)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	ctx->dsa_send = fn;
	return 0;
}

#ifdef CONFIG_NET_L2_ETHERNET
bool dsa_is_port_master(struct net_if *iface)
{
	/* First check if iface points to ETH interface */
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		/* Check its capabilities */
		if (net_eth_get_hw_capabilities(iface) &
		    ETHERNET_DSA_CONDUIT_PORT) {
			return true;
		}
	}

	return false;
}
#else
bool dsa_is_port_master(struct net_if *iface)
{
	return false;
}
#endif

/*
 * RECEIVE HANDLING CODE - ingress (ETH -> DSA slave ports)
 */

static int dsa_check_iface(struct net_if *iface)
{
	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return -ENOENT;
	}

	if (!((net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_CONDUIT_PORT) ||
	    (net_eth_get_hw_capabilities(iface) & ETHERNET_DSA_USER_PORT))) {
		return -ESRCH;
	}

	return 0;
}


int dsa_register_recv_callback(struct net_if *iface, dsa_net_recv_cb_t cb)
{
	struct ethernet_context *ctx;
	int ret;

	ret = dsa_check_iface(iface);
	if (ret < 0) {
		return ret;
	}

	if (cb) {
		ctx = net_if_l2_data(iface);
		ctx->dsa_recv_cb = cb;
	}

	return 0;
}

struct net_if *dsa_net_recv(struct net_if *iface, struct net_pkt **pkt)
{
	const struct ethernet_context *c;
	const struct dsa_context *ctx;
	struct net_if *iface_sw;
	int ret;

	if (*pkt == NULL || iface == NULL) {
		return NULL;
	}

	c = net_if_l2_data(iface);
	ctx = c->dsa_ctx;

	if (ctx == NULL || ctx->dapi == NULL) {
		return iface;
	}

	if (ctx->dapi->dsa_get_iface == NULL) {
		NET_ERR("DSA: No callback to set LAN interfaces!");
		return iface;
	}

	iface_sw = ctx->dapi->dsa_get_iface(iface, *pkt);

	ret = dsa_check_iface(iface_sw);
	if (ret < 0) {
		return iface_sw;
	}

	/*
	 * Optional code to change the destination interface with some
	 * custom callback (to e.g. filter/switch packets based on MAC).
	 *
	 * The callback shall be only present (and used) for lan1..3, but
	 * not for the master interface, which shall support all other
	 * protocols - i.e. UDP. ICMP, TCP.
	 */
	c = net_if_l2_data(iface_sw);
	if (c->dsa_recv_cb) {
		if (c->dsa_recv_cb(iface_sw, *pkt)) {
			return iface_sw;
		}
	}

	return iface;
}

/*
 * TRANSMISSION HANDLING CODE egress (DSA slave ports -> ETH)
 */
int dsa_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct net_if *iface_master, *iface;
	struct ethernet_context *ctx;
	struct dsa_context *context;

	iface = net_if_lookup_by_dev(dev);
	if (dsa_is_port_master(iface)) {
		/*
		 * The master interface's ethernet_context structure holds
		 * pointer to its original eth_tx().
		 * The wrapper (i.e. dsa_tx()) is needed to modify packet -
		 * it appends tag to it.
		 */
		ctx = net_if_l2_data(iface);
		context = ctx->dsa_ctx;
		return ctx->dsa_send(dev,
				     context->dapi->dsa_xmit_pkt(iface, pkt));
	}

	context = dev->data;
	iface_master = context->iface_master;

	if (iface_master == NULL) {
		NET_ERR("DSA: No master interface!");
		return -ENODEV;
	}

	/*
	 * Here packets are send via lan{123} interfaces in user program.
	 * Those structs' ethernet_api only have .send callback set to point
	 * to this wrapper function.
	 *
	 * Hence, it is necessary to get this callback from master's ethernet
	 * context structure..
	 */

	/* Adjust packet for DSA routing and send it via master interface */
	ctx = net_if_l2_data(iface_master);
	return ctx->dsa_send(net_if_get_device(iface_master),
			     context->dapi->dsa_xmit_pkt(iface, pkt));
}

struct net_if *dsa_get_slave_port(struct net_if *iface, int slave_num)
{
	struct ethernet_context *eth_ctx;
	struct dsa_context *dsa_ctx;

	eth_ctx = net_if_l2_data(iface);
	if (eth_ctx == NULL) {
		LOG_ERR("Iface %p context not available!", iface);
		return NULL;
	}

	dsa_ctx = eth_ctx->dsa_ctx;

	if (slave_num < 0 || slave_num >= dsa_ctx->num_slave_ports) {
		return NULL;
	}

	return dsa_ctx->iface_slave[slave_num];
}

int dsa_switch_read(struct net_if *iface, uint16_t reg_addr, uint8_t *value)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api =
		(const struct dsa_api *)context->dapi;

	return api->switch_read(dev, reg_addr, value);
}

int dsa_switch_write(struct net_if *iface, uint16_t reg_addr, uint8_t value)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api =
		(const struct dsa_api *)context->dapi;

	return api->switch_write(dev, reg_addr, value);
}

/**
 * @brief      Write static MAC table entry
 *
 * @param      iface          DSA interface
 * @param[in]  mac            MAC address
 * @param[in]  fw_port        The firmware port
 * @param[in]  tbl_entry_idx  Table entry index
 * @param[in]  flags          Flags
 *
 * @return     0 if successful, negative if error
 */
int dsa_switch_set_mac_table_entry(struct net_if *iface,
					const uint8_t *mac,
					uint8_t fw_port,
					uint16_t tbl_entry_idx,
					uint16_t flags)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api =
		(const struct dsa_api *)context->dapi;

	return api->switch_set_mac_table_entry(dev, mac, fw_port,
							tbl_entry_idx, flags);
}

/**
 * @brief      Read static MAC table entry
 *
 * @param      iface          DSA interface
 * @param      buf            Buffer to receive MAC address
 * @param[in]  tbl_entry_idx  Table entry index
 *
 * @return     0 if successful, negative if error
 */
int dsa_switch_get_mac_table_entry(struct net_if *iface,
					uint8_t *buf,
					uint16_t tbl_entry_idx)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api =
		(const struct dsa_api *)context->dapi;

	return api->switch_get_mac_table_entry(dev, buf, tbl_entry_idx);
}
