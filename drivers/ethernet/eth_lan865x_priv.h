/*
 * Copyright (c) 2023 DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ETH_LAN865X_PRIV_H__
#define ETH_LAN865X_PRIV_H__

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/net_if.h>
#include <ethernet/eth_stats.h>
#include "oa_tc6.h"

#define LAN865X_SPI_MAX_FREQUENCY		25000000U
#define LAN865X_HW_BOOT_DELAY_MS                7
#define LAN8650_DEVID                           0x8650
#define LAN8651_DEVID                           0x8651
#define LAN865X_REV_MASK                        GENMASK(3, 0)
#define LAN865X_RESET_TIMEOUT                   10

/* Memory Map Sector (MMS) 1 (0x1) */
#define LAN865x_MAC_NCR		        MMS_REG(0x1, 0x000)
#define LAN865x_MAC_NCR_TXEN		BIT(3)
#define LAN865x_MAC_NCR_RXEN		BIT(2)
#define LAN865x_MAC_NCFGR		MMS_REG(0x1, 0x001)
#define LAN865x_MAC_NCFGR_CAF		BIT(4)
#define LAN865x_MAC_SAB1		MMS_REG(0x1, 0x022)
#define LAN865x_MAC_SAB2		MMS_REG(0x1, 0x024)
#define LAN865x_MAC_SAT2		MMS_REG(0x1, 0x025)

#define LAN865x_MAC_TXRX_ON             1
#define LAN865x_MAC_TXRX_OFF            0

/* Memory Map Sector (MMS) 10 (0xA) */
#define LAN865x_DEVID MMS_REG(0xA, 0x094)

struct lan865x_config_plca {
	bool enable : 1; /* 1 - PLCA enable, 0 - CSMA/CD enable */
	uint8_t node_id  /* PLCA node id range: 0 to 254 */;
	uint8_t node_count;  /* PLCA node count range: 1 to 255 */
	uint8_t burst_count;  /* PLCA burst count range: 0x0 to 0xFF */
	uint8_t burst_timer;  /* PLCA burst timer */
	uint8_t to_timer;  /* PLCA TO value */
};

struct lan865x_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec interrupt;
	struct gpio_dt_spec reset;
	int32_t timeout;

	/* PLCA */
	struct lan865x_config_plca *plca;

	/* MAC */
	bool tx_cut_through_mode; /* 1 - tx cut through, 0 - Store and forward */
	bool rx_cut_through_mode; /* 1 - rx cut through, 0 - Store and forward */
};

struct lan865x_data {
	struct net_if *iface;
	struct gpio_callback gpio_int_callback;
	struct k_sem tx_rx_sem;
	struct k_sem int_sem;
	struct oa_tc6 *tc6;
	uint16_t chip_id;
	uint8_t silicon_rev;
	uint8_t mac_address[6];
	bool iface_initialized;
	bool reset;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ETH_LAN865X_IRQ_THREAD_STACK_SIZE);
	struct k_thread thread;
	k_tid_t tid_int;
};

#endif /* ETH_LAN865X_PRIV_H__ */
