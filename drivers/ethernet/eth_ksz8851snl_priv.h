/* KSZ8851SNL Stand-alone Ethernet Controller with SPI
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_KSZ8851SNL_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_KSZ8851SNL_PRIV_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>

/* SPI command phase opcodes (upper two bits of the first command byte) */
#define KSZ8851_CMD_READ		0x00
#define KSZ8851_CMD_WRITE		0x40
#define KSZ8851_CMD_FIFO_READ		0x80
#define KSZ8851_CMD_FIFO_WRITE		0xC0

/* Byte enable bits used for 8/16/32 bit internal register access */
#define KSZ8851_BE0			0x1
#define KSZ8851_BE1			0x2
#define KSZ8851_BE2			0x4
#define KSZ8851_BE3			0x8
#define KSZ8851_BE_WORD_LOW		(KSZ8851_BE0 | KSZ8851_BE1)
#define KSZ8851_BE_WORD_HIGH		(KSZ8851_BE2 | KSZ8851_BE3)
#define KSZ8851_BE_DWORD		(KSZ8851_BE_WORD_LOW | KSZ8851_BE_WORD_HIGH)

/*
 * Register offsets (see KSZ8851SNL datasheet chapter 4, "Register
 * Descriptions").
 */
#define KSZ8851_REG_CCR			0x08 /* Chip Configuration Register */
#define KSZ8851_REG_MARL		0x10 /* MAC Address Low */
#define KSZ8851_REG_MARM		0x12 /* MAC Address Middle */
#define KSZ8851_REG_MARH		0x14 /* MAC Address High */
#define KSZ8851_REG_GRR			0x26 /* Global Reset Register */
#define KSZ8851_REG_TXCR		0x70 /* Transmit Control Register */
#define KSZ8851_REG_TXSR		0x72 /* Transmit Status Register */
#define KSZ8851_REG_RXCR1		0x74 /* Receive Control Register 1 */
#define KSZ8851_REG_RXCR2		0x76 /* Receive Control Register 2 */
#define KSZ8851_REG_TXMIR		0x78 /* TX Memory Information Register */
#define KSZ8851_REG_RXFHSR		0x7C /* Receive Frame Header Status Register */
#define KSZ8851_REG_RXFHBCR		0x7E /* Receive Frame Header Byte Count Register */
#define KSZ8851_REG_TXQCR		0x80 /* Transmit Queue Command Register */
#define KSZ8851_REG_RXQCR		0x82 /* Receive Queue Command Register */
#define KSZ8851_REG_TXFDPR		0x84 /* TX Frame Data Pointer Register */
#define KSZ8851_REG_RXFDPR		0x86 /* RX Frame Data Pointer Register */
#define KSZ8851_REG_RXDTTR		0x8C /* RX Duration Timer Threshold Register */
#define KSZ8851_REG_RXDBCTR		0x8E /* RX Data Byte Count Threshold Register */
#define KSZ8851_REG_IER			0x90 /* Interrupt Enable Register */
#define KSZ8851_REG_ISR			0x92 /* Interrupt Status Register */
#define KSZ8851_REG_MAHTR0		0xA0 /* MAC Address Hash Table Register 0..3 (0xA0-0xA6) */
#define KSZ8851_REG_RXFCTR		0x9C /* RX Frame Count & Threshold Register */
#define KSZ8851_REG_FCLWR		0xB0 /* Flow Control Low Watermark Register */
#define KSZ8851_REG_FCHWR		0xB2 /* Flow Control High Watermark Register */
#define KSZ8851_REG_CIDER		0xC0 /* Chip ID and Enable Register */
#define KSZ8851_REG_PHYRR		0xD8 /* PHY Reset Register */
#define KSZ8851_REG_P1MBCR		0xE4 /* PHY 1 MII-Register Basic Control Register */
#define KSZ8851_REG_P1MBSR		0xE6 /* PHY 1 MII-Register Basic Status Register */
#define KSZ8851_REG_P1CR		0xF6 /* PHY 1 Control Register */
#define KSZ8851_REG_P1SR		0xF8 /* PHY 1 Status Register */

/* Global Reset Register (0x26) */
#define GRR_QMU_MODULE_SOFT_RESET	BIT(1)
#define GRR_GLOBAL_SOFT_RESET		BIT(0)

/* Transmit Control Register (0x70) */
#define TXCR_TCGICMP			BIT(8)
#define TXCR_TCGTCP			BIT(6)
#define TXCR_TCGIP			BIT(5)
#define TXCR_FTXQ			BIT(4)
#define TXCR_TXFCE			BIT(3)
#define TXCR_TXPE			BIT(2)
#define TXCR_TXCE			BIT(1)
#define TXCR_TXE			BIT(0)

/* Receive Control Register 1 (0x74) */
#define RXCR1_FRXQ			BIT(15)
#define RXCR1_RXUDPFCC			BIT(14)
#define RXCR1_RXTCPFCC			BIT(13)
#define RXCR1_RXIPFCC			BIT(12)
#define RXCR1_RXPAFMA			BIT(11)
#define RXCR1_RXFCE			BIT(10)
#define RXCR1_RXEFE			BIT(9)
#define RXCR1_RXMAFMA			BIT(8)
#define RXCR1_RXBE			BIT(7)
#define RXCR1_RXME			BIT(6)
#define RXCR1_RXUE			BIT(5)
#define RXCR1_RXAE			BIT(4)
#define RXCR1_RXINVF			BIT(1)
#define RXCR1_RXE			BIT(0)

/* Receive Control Register 2 (0x76) */
#define RXCR2_SRDBL2			BIT(7)
#define RXCR2_SRDBL1			BIT(6)
#define RXCR2_SRDBL0			BIT(5)
/* Single frame data burst length: RXQ reads are not limited to fixed bursts */
#define RXCR2_SRDBL_FRAME		(RXCR2_SRDBL2 | RXCR2_SRDBL1 | RXCR2_SRDBL0)
#define RXCR2_IUFFP			BIT(4)
#define RXCR2_RXIUFCEZ			BIT(3)
#define RXCR2_UDPLFE			BIT(2)
#define RXCR2_RXICMPFCC			BIT(1)
#define RXCR2_RXSAF			BIT(0)

#define TXMIR_TXMA_MASK			0x1FFF

/* Receive Frame Header Status Register (0x7C) */
#define RXFHSR_RXFV			BIT(15)
#define RXFHSR_RXICMPFCS		BIT(13)
#define RXFHSR_RXIPFCS			BIT(12)
#define RXFHSR_RXTCPFCS			BIT(11)
#define RXFHSR_RXUDPFCS			BIT(10)
#define RXFHSR_RXBF			BIT(7)
#define RXFHSR_RXMF			BIT(6)
#define RXFHSR_RXUF			BIT(5)
#define RXFHSR_RXMR			BIT(4)
#define RXFHSR_RXFT			BIT(3)
#define RXFHSR_RXFTL			BIT(2)
#define RXFHSR_RXRF			BIT(1)
#define RXFHSR_RXCE			BIT(0)
/* Bits that indicate the received frame should be discarded */
#define RXFHSR_RX_ERRORS \
	(RXFHSR_RXMR | RXFHSR_RXFTL | RXFHSR_RXRF | RXFHSR_RXCE)

#define RXFHBCR_RXBC_MASK		0x0FFF

/* Transmit Queue Command Register (0x80) */
#define TXQCR_AETFE			BIT(2)
#define TXQCR_TXQMAM			BIT(1)
#define TXQCR_METFE			BIT(0)

/* Receive Queue Command Register (0x82) */
#define RXQCR_RXDTTS			BIT(12)
#define RXQCR_RXDBCTS			BIT(11)
#define RXQCR_RXFCTS			BIT(10)
#define RXQCR_RXIPHTOE			BIT(9)
#define RXQCR_RXDTTE			BIT(7)
#define RXQCR_RXDBCTE			BIT(6)
#define RXQCR_RXFCTE			BIT(5)
#define RXQCR_ADRFE			BIT(4)
#define RXQCR_SDA			BIT(3)
#define RXQCR_RRXEF			BIT(0)

#define TXFDPR_TXFPAI			BIT(14)
#define RXFDPR_RXFPAI			BIT(14)
/* Frame data pointer low bits, wrapped back to 0 at the start of a frame */
#define RXFDPR_RXFP_MASK		0x07FF

/* Interrupt Enable / Interrupt Status Register bits (0x90 / 0x92) */
#define IER_LCIE			BIT(15) /* Link Change */
#define IER_TXIE			BIT(14) /* Transmit Done */
#define IER_RXIE			BIT(13) /* Receive Done */
#define IER_RXOIE			BIT(11) /* Receive Overrun */
#define IER_TXPSIE			BIT(9)  /* Transmit Process Stopped */
#define IER_RXPSIE			BIT(8)  /* Receive Process Stopped */
#define IER_TXSAIE			BIT(6)  /* Transmit Space Available */
#define IER_RXWFDIE			BIT(5)  /* Receive Wakeup Frame Detect */
#define IER_RXMPDIE			BIT(4)  /* Receive Magic Packet Detect */
#define IER_LDIE			BIT(3)  /* Linkup Detect */
#define IER_EDIE			BIT(2)  /* Energy Detect */
#define IER_SPIBEIE			BIT(1)  /* SPI Bus Error */
#define ISR_CLEAR_ALL			0xFFFF

#define KSZ8851_IRQ_MASK \
	(IER_LCIE | IER_RXIE | IER_RXOIE)

/* Chip ID and Enable Register (0xC0) */
#define CIDER_CHIP_ID_MASK		0xFFF0
#define CIDER_FAMILY_ID			0x8870

/* PHY Reset Register (0xD8) */
#define PHYRR_PHY_RESET			BIT(0)

/* PHY 1 Status Register (0xF8) */
#define P1SR_HP_MDIX			BIT(15)
#define P1SR_POLARITY_REVERSE		BIT(13)
#define P1SR_OPERATION_SPEED		BIT(10)
#define P1SR_OPERATION_DUPLEX		BIT(9)
#define P1SR_MDIX_STATUS		BIT(7)
#define P1SR_AN_DONE			BIT(6)
#define P1SR_LINK_GOOD			BIT(5)

struct ksz8851snl_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec interrupt;
	struct gpio_dt_spec reset;
	struct net_eth_mac_config mac_cfg;
};

struct ksz8851snl_runtime {
	struct net_if *iface;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ETH_KSZ8851SNL_RX_THREAD_STACK_SIZE);
	struct k_thread thread;

	struct gpio_callback gpio_cb;
	struct k_sem int_sem;
	struct k_mutex lock;
	struct phy_link_state state;
	/* Lower bound of the free TX queue memory, refreshed from TXMIR when short */
	uint16_t tx_space;
};

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_KSZ8851SNL_PRIV_H_ */
