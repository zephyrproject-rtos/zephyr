/*
 * Copyright (c) 2024-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Private definitions for the ADIN1140 MAC-PHY ethernet driver.
 */

#ifndef ETH_ADIN1140_PRIV_H__
#define ETH_ADIN1140_PRIV_H__

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

#include "oa_tc6.h"

/* Constants */
#define ADIN1140_SPI_MAX_FREQUENCY          MHZ(25)
#define ADIN1140_PHYID_RST_VAL              0x0283BE00
#define ADIN1140_RESET_CYCLE_TIME_US        2U
#define ADIN1140_WAKE_PIN_ASSERT_TIME_MS    100U
#define ADIN1140_RST_COMPLETE               0x3
#define ADIN1140_IMASK0_RST_VAL             0x00001FBF
#define ADIN1140_IMASK1_RST_VAL             0x43FA1F5A
#define ADIN1140_A0_CFG_FIELDS_1_RST_VAL    0x4
#define ADIN1140_CONFIG0_TXCTHRESH_CREDIT_8 0x2
#define ADIN1140_OA_SPI_BUF_SIZE            (255U * 68U) /* Max 255 68-byte chunks */
#define ADIN1140_MAX_RX_FRAME_SIZE          (NET_ETH_MAX_FRAME_SIZE + 4) /* + FCS */

/* ADIN1140 Registers (non-OA) */
#define ADIN1140_MAC_RST_STATUS            MMS_REG(0x1, 0x3B)
#define ADIN1140_A0_CFG_FIELDS_1           MMS_REG(0xA, 0xB703)
#define ADIN1140_MAC_ADDR_FILT_UPR         MMS_REG(0x1, 0x50)
#define ADIN1140_MAC_ADDR_FILT_LWR         MMS_REG(0x1, 0x51)
#define ADIN1140_MAC_ADDR_MASK_UPR         MMS_REG(0x1, 0x70)
#define ADIN1140_MAC_ADDR_MASK_LWR         MMS_REG(0x1, 0x71)
#define ADIN1140_MAC_RST_STATUS_MASK       GENMASK(1, 0)
#define ADIN1140_STATUS1_RX_RDY            BIT(4)
#define ADIN1140_A0_CFG_FIELDS_1_CFG_VALID BIT(7)
#define ADIN1140_MAC_ADDR_FILT_APPLY2PORT1 BIT(30)
#define ADIN1140_MAC_ADDR_FILT_TO_HOST     BIT(16)

/* OA TC6 Registers */
#define OA_CONFIG2                         MMS_REG(0x0, 0x006)
#define OA_IMASK0_PHYINTM                  BIT(7)
#define OA_CONFIG0_TXCTHRESH               10U
#define OA_CONFIG0_TXCTE                   BIT(9)
#define OA_CONFIG0_RXCTE                   BIT(8)
#define OA_CONFIG0_FTSE                    BIT(7)
#define OA_CONFIG0_FTSS                    BIT(6)
#define OA_CONFIG0_TXFCSVE                 BIT(14)
#define OA_CONFIG2_LO_PRIO_FIFO_CRC_APPEND BIT(17)
#define OA_CONFIG2_TX_RDY_ON_EMPTY         BIT(8)
#define OA_CONFIG2_HOST_CRC_APPEND         BIT(5)
#define OA_CONFIG2_FWD_UNK2HOST            BIT(2)
#define OA_STATUS0_LOFE                    BIT(4)
#define OA_STATUS0_RXBOE                   BIT(3)

/* MAC address filter table slots (16 total).
 * Slot addresses increment in 2.
 * Slot 0 is not available in U3/U4 silicon.
 */
#define ADIN1140_MAC_FILT_TABLE_MULTICAST_SLOT 2U
#define ADIN1140_MAC_FILT_TABLE_BROADCAST_SLOT 4U
#define ADIN1140_MAC_FILT_TABLE_UNICAST_SLOT   6U
#define ADIN1140_MAC_FILT_TABLE_BASE_SLOT      8U
#define ADIN1140_MAC_FILT_TABLE_SLOT_SIZE      2U
#define ADIN1140_MAC_FILT_TABLE_MAX_SLOT       30U

/** ADIN1140 device configuration (from devicetree). */
struct adin1140_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec interrupt;
	struct gpio_dt_spec wake;
	const struct device *mdio;
	const struct device *phy;
	struct net_eth_mac_config mac_cfg;
};

/** ADIN1140 device runtime data. */
struct adin1140_data {
	struct net_if *iface;
	struct gpio_callback gpio_irq_callback;
	struct k_mutex lock;
	struct k_sem irq_sem;
	struct oa_tc6 *tc6;
	uint8_t mac_address[NET_ETH_ADDR_LEN];
	bool tx_cut_through_en: 1;
	bool rx_cut_through_en: 1;

	/* Frame/packet data buffers */
	uint8_t oa_tx_buf[ADIN1140_OA_SPI_BUF_SIZE];
	uint8_t oa_rx_buf[ADIN1140_OA_SPI_BUF_SIZE];
	uint8_t *pkt_buf;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_ADIN1140_RX_THREAD_STACK_SIZE);
	struct k_thread rx_thread;
};

#endif /* ETH_ADIN1140_PRIV_H__ */
