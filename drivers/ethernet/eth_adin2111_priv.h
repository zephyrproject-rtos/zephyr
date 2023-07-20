/*
 * Copyright (c) 2023 PHOENIX CONTACT Electronics GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ETH_ADIN2111_PRIV_H__
#define ETH_ADIN2111_PRIV_H__

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/net_if.h>
#include <ethernet/eth_stats.h>

/* SPI frequency maximum, based on clock cycle time */
#define ADIN2111_SPI_MAX_FREQUENCY		25000000U

#define ADIN2111_PHYID				0x01U
/* PHY Identification Register Reset Value */
#define ADIN2111_PHYID_RST_VAL			0x0283BCA1U
#define ADIN1110_PHYID_RST_VAL			0x0283BC91U

/* Reset Control and Status Register */
#define ADIN2111_RESET				0x03U
/* MACPHY software reset */
#define ADIN2111_RESET_SWRESET			BIT(0)

/* Configuration Register 0 */
#define ADIN2111_CONFIG0			0x04U
/* Configuration Synchronization */
#define ADIN2111_CONFIG0_SYNC			BIT(15)
/* Transmit Frame Check Sequence Validation Enable */
#define ADIN2111_CONFIG0_TXFCSVE		BIT(14)
/* Transmit Cut Through Enable */
#define ADIN2111_CONFIG0_TXCTE			BIT(9)
/* Receive Cut Through Enable. Must be 0 for Generic SPI */
#define ADIN2111_CONFIG0_RXCTE			BIT(8)

/* Configuration Register 2 */
#define ADIN2111_CONFIG2			0x06U
/* Forward Frames from Port 2 Not Matching a MAC Address to Port 1 */
#define ADIN2111_CONFIG2_P2_FWD_UNK2P1		BIT(14)
/* Forward Frames from Port 1 Not Matching a MAC Address to Port 2 */
#define ADIN2111_CONFIG2_P1_FWD_UNK2P2		BIT(13)
/* Enable Cut Through from Port to Port */
#define ADIN2111_CONFIG2_PORT_CUT_THRU_EN	BIT(11)
/* Enable CRC Append */
#define ADIN2111_CONFIG2_CRC_APPEND		BIT(5)

/* Status Register 0 */
#define ADIN2111_STATUS0			0x08U
/* PHY Interrupt for Port 1 */
#define ADIN2111_STATUS0_PHYINT			BIT(7)
/**
 * Reset Complete.
 * The bit is set when the MACPHY reset is complete
 * and ready for configuration.
 */
#define ADIN2111_STATUS0_RESETC			BIT(6)
/* Value to completely clear status register 0 */
#define ADIN2111_STATUS0_CLEAR			0x1F7FU

/* Status Register 1 */
#define ADIN2111_STATUS1			0x09U
/* PHY Interrupt for Port 2 */
#define ADIN2111_STATUS1_PHYINT			BIT(19)
/* Port 2 RX FIFO Contains Data */
#define ADIN2111_STATUS1_P2_RX_RDY		BIT(17)
/* Indicates that a CRC error was detected */
#define ADIN2111_STATUS1_SPI_ERR		BIT(10)
/* Port 1 RX FIFO Contains Data */
#define ADIN2111_STATUS1_P1_RX_RDY		BIT(4)
/* Value to completely clear status register 1 */
#define ADIN2111_STATUS1_CLEAR			0xFFF01F08U


/* Interrupt Mask Register 0 */
#define ADIN2111_IMASK0				0x0CU
/* Physical Layer Interrupt Mask */
#define ADIN2111_IMASK0_PHYINTM			BIT(7)

/* Interrupt Mask Register 1 */
#define ADIN2111_IMASK1				0x0DU
/* Mask Bit for P2_PHYINT */
#define ADIN2111_IMASK1_P2_PHYINT_MASK		BIT(19)
/*!< Mask Bit for P2_RX_RDY. Generic SPI only.*/
#define ADIN2111_IMASK1_P2_RX_RDY_MASK		BIT(17)
/*!< Mask Bit for SPI_ERR. Generic SPI only. */
#define ADIN2111_IMASK1_SPI_ERR_MASK		BIT(10)
/*!< Mask Bit for P1_RX_RDY. Generic SPI only.*/
#define ADIN2111_IMASK1_P1_RX_RDY_MASK		BIT(4)
/*!< Mask Bit for TX_FRM_DONE. Generic SPI only.*/
#define ADIN2111_IMASK1_TX_RDY_MASK		BIT(4)

/* MAC Tx Frame Size Register */
#define ADIN2111_TX_FSIZE			0x30U
/* Tx FIFO Space Register */
#define ADIN2111_TX_SPACE			0x32U

/* MAC Address Rule and DA Filter Upper 16 Bits Registers */
#define ADIN2111_ADDR_FILT_UPR			0x50U
#define ADIN2111_ADDR_APPLY2PORT2		BIT(31)
#define ADIN2111_ADDR_APPLY2PORT1		BIT(30)
#define ADIN2111_ADDR_TO_OTHER_PORT		BIT(17)
#define ADIN2111_ADDR_TO_HOST			BIT(16)

/* MAC Address DA Filter Lower 32 Bits Registers */
#define ADIN2111_ADDR_FILT_LWR			0x51U
/* Upper 16 Bits of the MAC Address Mask */
#define ADIN2111_ADDR_MSK_UPR			0x70U
/* Lower 32 Bits of the MAC Address Mask */
#define ADIN2111_ADDR_MSK_LWR			0x71U

/* P1 MAC Rx Frame Size Register */
#define ADIN2111_P1_RX_FSIZE			0x90U
/* P1 MAC Receive Register */
#define ADIN2111_P1_RX				0x91U

/* P2 MAC Rx Frame Size Register */
#define ADIN2111_P2_RX_FSIZE			0xC0U
/* P2 MAC Receive Register */
#define ADIN2111_P2_RX				0xC1U

/* SPI header size in bytes */
#define ADIN2111_SPI_HEADER_SIZE		2U
/* SPI header size for write transaction */
#define ADIN2111_WRITE_HEADER_SIZE		ADIN2111_SPI_HEADER_SIZE
/* SPI header size for read transaction (1 for TA) */
#define ADIN2111_READ_HEADER_SIZE		(ADIN2111_SPI_HEADER_SIZE + 1U)

/* SPI register write buffer size without CRC */
#define ADIN2111_REG_WRITE_BUF_SIZE		(ADIN2111_WRITE_HEADER_SIZE + sizeof(uint32_t))
/* SPI register write buffer with appended CRC size (1 for header, 1 for register) */
#define ADIN2111_REG_WRITE_BUF_SIZE_CRC		(ADIN2111_REG_WRITE_BUF_SIZE + 2U)

/* SPI register read buffer size with TA without CRC */
#define ADIN2111_REG_READ_BUF_SIZE		(ADIN2111_READ_HEADER_SIZE + sizeof(uint32_t))
/* SPI register read buffer with TA and appended CRC size (1 header, 1 for register) */
#define ADIN2111_REG_READ_BUF_SIZE_CRC		(ADIN2111_REG_READ_BUF_SIZE + 2U)

/* SPI read fifo cmd buffer size with TA without CRC */
#define ADIN2111_FIFO_READ_CMD_BUF_SIZE		(ADIN2111_READ_HEADER_SIZE)
/* SPI read fifo cmd buffer with TA and appended CRC size */
#define ADIN2111_FIFO_READ_CMD_BUF_SIZE_CRC	(ADIN2111_FIFO_READ_CMD_BUF_SIZE + 1U)

/* SPI Header for writing control transaction in half duplex mode */
#define ADIN2111_WRITE_TXN_CTRL			0xA000U
/* SPI Header for writing control transaction with MAC TX register (!) in half duplex mode */
#define ADIN2111_TXN_CTRL_TX_REG		0xA031U
/* SPI Header for reading control transaction in half duplex mode */
#define ADIN2111_READ_TXN_CTRL			0x8000U

/* Frame header size in bytes */
#define ADIN2111_FRAME_HEADER_SIZE		2U
#define ADIN2111_INTERNAL_HEADER_SIZE		2U
/* Number of buffer bytes in TxFIFO to provide frame margin upon writes */
#define ADIN2111_TX_FIFO_BUFFER_MARGIN		4U

enum adin2111_chips_id {
	ADIN2111_MAC = 0,
	ADIN1110_MAC,
};

struct adin2111_config {
	enum adin2111_chips_id id;
	struct spi_dt_spec spi;
	struct gpio_dt_spec interrupt;
	struct gpio_dt_spec reset;
};

struct adin2111_data {
	/* Port 0: PHY 1, Port 1: PHY 2 */
	const struct device *port[2];
	struct gpio_callback gpio_int_callback;
	struct k_sem offload_sem;
	struct k_mutex lock;
	uint32_t imask0;
	uint32_t imask1;
	uint16_t ifaces_left_to_init;
	uint8_t *buf;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_ADIN2111_IRQ_THREAD_STACK_SIZE);
	struct k_thread rx_thread;
};

struct adin2111_port_data {
	struct net_if *iface;
	uint8_t mac_addr[6];
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
};

struct adin2111_port_config {
	const struct device *adin;
	const struct device *phy;
	const uint16_t port_idx;
	const uint16_t phy_addr;
};

#endif /* ETH_ADIN2111_PRIV_H__ */
