/* W6300 Stand-alone Ethernet Controller with SPI/QSPI
 *
 * Copyright (c) 2025 WIZnet Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ETH_W6300_PRIV_H_
#define ETH_W6300_PRIV_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/util.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mspi)
#include <zephyr/drivers/mspi.h>
#endif

/*
 * Instruction byte: MOD[7:6] selects the data line width used for the address
 * and data phases, RWB[5] selects read or write, BSB[4:0] selects the block.
 */
#define W6300_SPI_MOD_SINGLE 0x00
#define W6300_SPI_MOD_QUAD 0x02
#define W6300_SPI_RWB_READ 0x00
#define W6300_SPI_RWB_WRITE 0x01

#define W6300_BSB_COMMON 0x00
#define W6300_BSB_SOCK(n) (1U + (4U * (n)))
#define W6300_BSB_TX(n) (2U + (4U * (n)))
#define W6300_BSB_RX(n) (3U + (4U * (n)))

/* Common register offsets (W6300 datasheet, Section 4.1). */
#define W6300_CIDR0 0x0000
#define W6300_CIDR1 0x0001
#define W6300_CIDR2 0x0004
#define W6300_SYSR 0x2000
#define W6300_SYCR0 0x2004
#define W6300_SYCR1 0x2005
#define W6300_IR 0x2100
#define W6300_IMR 0x2104
#define W6300_IRCLR 0x2108
#define W6300_SIMR 0x2114
#define W6300_PHYSR 0x3000
#define W6300_SHAR 0x4120

/* SYCR0/1 bits. */
#define W6300_SYCR0_RST BIT(7)
#define W6300_SYCR1_IEN BIT(7)
#define W6300_SYCR1_CLKSEL BIT(0)

/* Socket register offsets (W6300 datasheet, Section 4.2). */
#define W6300_Sn_MR 0x0000
#define W6300_Sn_PSR 0x0004
#define W6300_Sn_CR 0x0010
#define W6300_Sn_IR 0x0020
#define W6300_Sn_IMR 0x0024
#define W6300_Sn_IRCLR 0x0028
#define W6300_Sn_SR 0x0030
#define W6300_Sn_PORTR 0x0114
#define W6300_Sn_DHAR 0x0118
#define W6300_Sn_DIPR 0x0120
#define W6300_Sn_DPORTR 0x0140
#define W6300_Sn_TX_BSR 0x0200
#define W6300_Sn_TX_FSR 0x0204
#define W6300_Sn_TX_RD 0x0208
#define W6300_Sn_TX_WR 0x020C
#define W6300_Sn_RX_BSR 0x0220
#define W6300_Sn_RX_RSR 0x0224
#define W6300_Sn_RX_RD 0x0228
#define W6300_Sn_RX_WR 0x022C

/* Socket mode values. */
#define W6300_Sn_MR_MF BIT(7)
#define W6300_Sn_MR_MACRAW 0x07

/* Socket command values. */
#define W6300_Sn_CR_OPEN 0x01
#define W6300_Sn_CR_CLOSE 0x10
#define W6300_Sn_CR_SEND 0x20
#define W6300_Sn_CR_RECV 0x40

/* Socket interrupt bits. */
#define W6300_Sn_IR_SENDOK 0x10
#define W6300_Sn_IR_RECV 0x04

/* PHY status bits. */
#define W6300_PHYSR_DPX BIT(2)
#define W6300_PHYSR_SPD BIT(1)
#define W6300_PHYSR_LNK BIT(0)

#define W6300_PKT_INFO_LEN 2
#define W6300_ETH_MIN_FRAME_LEN 14
/*
 * The driver runs socket 0 in MACRAW mode and does not use the other hardware
 * sockets, so all 16 KB of TX/RX buffer memory is assigned to socket 0.
 */
#define W6300_NUM_SOCKETS 8
#define W6300_SOCK0_BSR_KB 16
#define W6300_BSR_TO_BYTES(val) ((uint16_t)(val) << 10)
/*
 * Returning the read pointer to the chip costs a Sn_RX_RD write, a RECV
 * command and the command completion poll -- more bus traffic than reading a
 * small frame.  Frames are consumed in batches and the pointer is handed back
 * once per batch.  The caps bound how much of the chip's 16 KB receive buffer
 * stays unreclaimed while a batch is in flight.
 */
#define W6300_RX_BATCH_MAX 8
#define W6300_RX_BATCH_BYTES 8192

#define W6300_CMD_TIMEOUT_MS 100
#define W6300_CMD_POLL_US 10U
#define W6300_TX_SEM_TIMEOUT_MS 10

/*
 * The W6300 speaks the same register protocol over a plain SPI bus and over
 * an MSPI bus in quad (1-4-4) mode, so the register accessors are abstracted
 * behind this per-instance operation table.  Which one an instance gets is
 * decided by the bus its devicetree node hangs off.
 */
struct w6300_bus_io {
	int (*init)(const struct device *dev);
	int (*read)(const struct device *dev, uint8_t bsb, uint16_t addr,
		    uint8_t *data, size_t len);
	int (*write)(const struct device *dev, uint8_t bsb, uint16_t addr,
		     uint8_t *data, size_t len);
};

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mspi)
struct w6300_mspi_bus {
	const struct device *ctlr;
	struct mspi_dev_id dev_id;
	struct mspi_dev_cfg dev_cfg;
};
#endif

union w6300_bus {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mspi)
	struct w6300_mspi_bus mspi;
#endif
};

struct w6300_config {
	const struct w6300_bus_io *bus_io;
	union w6300_bus bus;
	struct gpio_dt_spec interrupt;
	struct gpio_dt_spec reset;
	struct net_eth_mac_config mac_cfg;
	const struct device *phy_dev;
};

struct w6300_runtime {
	struct net_if *iface;

	K_KERNEL_STACK_MEMBER(thread_stack,
			      CONFIG_ETH_W6300_RX_THREAD_STACK_SIZE);
	struct k_thread thread;
	uint8_t mac_addr[NET_ETH_ADDR_LEN];
	struct gpio_callback gpio_cb;
	struct k_mutex cmd_lock;
	struct k_sem tx_sem;
	struct k_sem int_sem;
	struct phy_link_state state;
	uint16_t tx_buf_size;
	uint16_t rx_buf_size;
	uint8_t buf[NET_ETH_MAX_FRAME_SIZE];
};

#endif /* ETH_W6300_PRIV_H_ */
