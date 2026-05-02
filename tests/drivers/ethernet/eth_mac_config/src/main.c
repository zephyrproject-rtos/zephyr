/*
 * Copyright (c) 2025 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>

#include "eth_test_priv.h"

ZTEST(ethernet_mac_config, test_eth_mac_local)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(eth_mac_local));
	const struct vnd_ethernet_config *cfg = dev->config;
	struct vnd_ethernet_data *data = dev->data;
	const uint8_t addr[NET_ETH_ADDR_LEN] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};

	zexpect_equal(data->mac_addr_load_result, 0);
	zexpect_equal(cfg->mcfg.type, NET_ETH_MAC_STATIC);
	zexpect_equal(cfg->mcfg.addr_len, NET_ETH_ADDR_LEN);
	zexpect_mem_equal(cfg->mcfg.addr, addr, NET_ETH_ADDR_LEN);
	zexpect_mem_equal(data->mac_addr, addr, NET_ETH_ADDR_LEN);
}

ZTEST(ethernet_mac_config, test_eth_mac_random)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(eth_mac_random));
	const struct vnd_ethernet_config *cfg = dev->config;
	struct vnd_ethernet_data *data = dev->data;

	zexpect_equal(data->mac_addr_load_result, 0);
	zexpect_equal(cfg->mcfg.type, NET_ETH_MAC_RANDOM);
	zexpect_equal(cfg->mcfg.addr_len, 0);
	zexpect_equal(data->mac_addr[0] & 0x02, 0x02, "Missing LAA bit");
	zexpect_equal(data->mac_addr[0] & 0x01, 0x00, "Erroneous I/G bit");
}

ZTEST(ethernet_mac_config, test_eth_mac_random_prefix)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(eth_mac_random_prefix));
	const struct vnd_ethernet_config *cfg = dev->config;
	struct vnd_ethernet_data *data = dev->data;
	uint8_t prefix[] = {0x00, 0x22, 0x44};

	zexpect_equal(data->mac_addr_load_result, 0);
	zexpect_equal(cfg->mcfg.type, NET_ETH_MAC_RANDOM);
	zexpect_equal(cfg->mcfg.addr_len, 3);
	zexpect_mem_equal(cfg->mcfg.addr, prefix, sizeof(prefix));

	/* NOTE: Prefix with LAA bit set */
	prefix[0] |= 0x02;
	zexpect_mem_equal(data->mac_addr, prefix, sizeof(prefix));
}

ZTEST(ethernet_mac_config, test_eth_mac_nvmem)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(eth_mac_nvmem));
	const struct vnd_ethernet_config *cfg = dev->config;
	struct vnd_ethernet_data *data = dev->data;

	zexpect_equal(data->mac_addr_load_result, 0);
	zexpect_equal(cfg->mcfg.type, NET_ETH_MAC_NVMEM);
	zexpect_equal(cfg->mcfg.addr_len, 0);

	zexpect_equal_ptr(cfg->mcfg.cell.dev, DEVICE_DT_GET(DT_NODELABEL(eeprom0)));
	zexpect_equal(cfg->mcfg.cell.offset, DT_REG_ADDR(DT_NODELABEL(cell0)));
	zexpect_equal(cfg->mcfg.cell.size, DT_REG_SIZE(DT_NODELABEL(cell0)));
}

ZTEST(ethernet_mac_config, test_eth_mac_nvmem_prefix)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(eth_mac_nvmem_prefix));
	const struct vnd_ethernet_config *cfg = dev->config;
	struct vnd_ethernet_data *data = dev->data;
	uint8_t prefix[] = {0x00, 0x33, 0x66};

	zexpect_equal(data->mac_addr_load_result, 0);
	zexpect_equal(cfg->mcfg.type, NET_ETH_MAC_NVMEM);
	zexpect_equal(cfg->mcfg.addr_len, 3);
	zexpect_mem_equal(cfg->mcfg.addr, prefix, sizeof(prefix));

	/* NOTE: No LAA bit set in this case */
	zexpect_mem_equal(data->mac_addr, prefix, sizeof(prefix));

	zexpect_equal_ptr(cfg->mcfg.cell.dev, DEVICE_DT_GET(DT_NODELABEL(eeprom0)));
	zexpect_equal(cfg->mcfg.cell.offset, DT_REG_ADDR(DT_NODELABEL(cell6)));
	zexpect_equal(cfg->mcfg.cell.size, DT_REG_SIZE(DT_NODELABEL(cell6)));
}

ZTEST(ethernet_mac_config, test_eth_mac_none)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(eth_mac_none));
	const struct vnd_ethernet_config *cfg = dev->config;
	struct vnd_ethernet_data *data = dev->data;

	zexpect_equal(data->mac_addr_load_result, -ENODATA);
	zexpect_equal(cfg->mcfg.type, NET_ETH_MAC_DEFAULT);
	zexpect_equal(cfg->mcfg.addr_len, 0);
}

ZTEST_SUITE(ethernet_mac_config, NULL, NULL, NULL, NULL, NULL);
