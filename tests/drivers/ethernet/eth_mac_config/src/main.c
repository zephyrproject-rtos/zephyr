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
#include <zephyr/nvmem.h>

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

ZTEST(ethernet_mac_config, test_eth_mac_nvmem_little_endian)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(eth_mac_nvmem_le));
	const struct vnd_ethernet_config *cfg = dev->config;
	const uint8_t stored[] = {0x78, 0x56, 0x34, 0x12, 0xcd, 0xab};
	const uint8_t expected[] = {0xab, 0xcd, 0x12, 0x34, 0x56, 0x78};
	uint8_t mac[NET_ETH_ADDR_LEN];
	int ret;

	ret = nvmem_cell_write(&cfg->mcfg.cell, stored, 0, sizeof(stored));
	zassert_ok(ret);

	ret = net_eth_mac_load(&cfg->mcfg, mac);
	zexpect_equal(ret, 0);
	zexpect_mem_equal(mac, expected, sizeof(expected));
	zexpect_equal(cfg->mcfg.cell.byte_order, NVMEM_CELL_BYTE_ORDER_LITTLE_ENDIAN);
}

ZTEST(ethernet_mac_config, test_eth_mac_nvmem_big_endian)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(eth_mac_nvmem_be));
	const struct vnd_ethernet_config *cfg = dev->config;
	const uint8_t stored[] = {0xab, 0xcd, 0x12, 0x34, 0x56, 0x78};
	uint8_t mac[NET_ETH_ADDR_LEN];
	int ret;

	ret = nvmem_cell_write(&cfg->mcfg.cell, stored, 0, sizeof(stored));
	zassert_ok(ret);

	ret = net_eth_mac_load(&cfg->mcfg, mac);
	zexpect_equal(ret, 0);
	zexpect_mem_equal(mac, stored, sizeof(stored));
	zexpect_equal(cfg->mcfg.cell.byte_order, NVMEM_CELL_BYTE_ORDER_BIG_ENDIAN);
}

ZTEST(ethernet_mac_config, test_eth_mac_nvmem_invalid_cell_size)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(eth_mac_nvmem_prefix));
	const struct vnd_ethernet_config *cfg = dev->config;
	struct net_eth_mac_config mcfg = cfg->mcfg;
	const enum nvmem_cell_byte_order byte_orders[] = {
		NVMEM_CELL_BYTE_ORDER_LITTLE_ENDIAN,
		NVMEM_CELL_BYTE_ORDER_BIG_ENDIAN,
	};
	const size_t invalid_sizes[] = {3U, NET_ETH_ADDR_LEN + 1U};
	uint8_t mac[NET_ETH_ADDR_LEN];

	zassert_ok(net_eth_mac_load(&mcfg, mac));

	for (size_t i = 0; i < ARRAY_SIZE(byte_orders); i++) {
		mcfg.cell.byte_order = byte_orders[i];
		for (size_t j = 0; j < ARRAY_SIZE(invalid_sizes); j++) {
			mcfg.cell.size = invalid_sizes[j];
			zexpect_equal(net_eth_mac_load(&mcfg, mac), -EINVAL,
				      "Byte order %d, cell size %zu", byte_orders[i],
				      invalid_sizes[j]);
		}
	}
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
