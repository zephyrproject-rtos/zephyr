/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/dsa.h>
#include <zephyr/net/ethernet.h>

int dsa_switch_read(struct net_if *iface, uint16_t reg_addr, uint8_t *value)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	return api->switch_read(dev, reg_addr, value);
}

int dsa_switch_write(struct net_if *iface, uint16_t reg_addr, uint8_t value)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

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
int dsa_switch_set_mac_table_entry(struct net_if *iface, const uint8_t *mac, uint8_t fw_port,
				   uint16_t tbl_entry_idx, uint16_t flags)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	return api->switch_set_mac_table_entry(dev, mac, fw_port, tbl_entry_idx, flags);
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
int dsa_switch_get_mac_table_entry(struct net_if *iface, uint8_t *buf, uint16_t tbl_entry_idx)
{
	const struct device *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->data;
	const struct dsa_api *api = (const struct dsa_api *)context->dapi;

	return api->switch_get_mac_table_entry(dev, buf, tbl_entry_idx);
}
