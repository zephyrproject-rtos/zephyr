/* W6100 Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2024 Parth Sanepara
 * Author: Parth Sanepara <parthsanepara@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#ifndef ETH_W6100_PRIV_H_
#define ETH_W6100_PRIV_H_

/*
 * W6100 common registers
 */
#define W6100_COMMON_REGS	0x0000
#define W6100_MR		0x0000 /* Mode Register */
#define W6100_GW		0x0001
#define MR_RST			0x80 /* S/W reset */
#define MR_PB			0x10 /* Ping block */
#define MR_AI			0x02 /* Address Auto-Increment */
#define MR_IND			0x01 /* Indirect mode */
#define W6100_SHAR		0x0009 /* Source MAC address */
#define W6100_IR		0x0015 /* Interrupt Register */
#define W6100_COMMON_REGS_LEN	0x0040
#define W6100_PHYCFGR		0x002E /* PHY Configuration register */

#define W6100_Sn_MR		0x0000 /* Sn Mode Register */
#define W6100_Sn_CR		0x0001 /* Sn Command Register */
#define W6100_Sn_IR		0x0002 /* Sn Interrupt Register */
#define W6100_Sn_SR		0x0003 /* Sn Status Register */
#define W6100_Sn_TX_FSR		0x0020 /* Sn Transmit free memory size */
#define W6100_Sn_TX_RD		0x0022 /* Sn Transmit memory read pointer */
#define W6100_Sn_TX_WR		0x0024 /* Sn Transmit memory write pointer */
#define W6100_Sn_RX_RSR		0x0026 /* Sn Receive free memory size */
#define W6100_Sn_RX_RD		0x0028 /* Sn Receive memory read pointer */

#define W6100_S0_REGS		0x10000

#define W6100_S0_MR		(W6100_S0_REGS + W6100_Sn_MR)
#define S0_MR_MACRAW		0x04 /* MAC RAW mode */
#define S0_MR_MF		0x40 /* MAC Filter for W6100 */
#define W6100_S0_CR		(W6100_S0_REGS + W6100_Sn_CR)
#define S0_CR_OPEN		0x01 /* OPEN command */
#define S0_CR_CLOSE		0x10 /* CLOSE command */
#define S0_CR_SEND		0x20 /* SEND command */
#define S0_CR_RECV		0x40 /* RECV command */
#define W6100_S0_IR		(W6100_S0_REGS + W6100_Sn_IR)
#define S0_IR_SENDOK		0x10 /* complete sending */
#define S0_IR_RECV		0x04 /* receiving data */
#define W6100_S0_SR		(W6100_S0_REGS + W6100_Sn_SR)
#define S0_SR_MACRAW		0x42 /* mac raw mode */
#define W6100_S0_TX_FSR		(W6100_S0_REGS + W6100_Sn_TX_FSR)
#define W6100_S0_TX_RD		(W6100_S0_REGS + W6100_Sn_TX_RD)
#define W6100_S0_TX_WR		(W6100_S0_REGS + W6100_Sn_TX_WR)
#define W6100_S0_RX_RSR		(W6100_S0_REGS + W6100_Sn_RX_RSR)
#define W6100_S0_RX_RD		(W6100_S0_REGS + W6100_Sn_RX_RD)
#define W6100_S0_IMR		(W6100_S0_REGS + W6100_Sn_IMR)

#define W6100_S0_MR_MF		7 /* MAC Filter for W6100 */
#define W6100_Sn_REGS_LEN	0x0040
#define W6100_SIMR		0x0018 /* Socket Interrupt Mask Register */
#define IR_S0			0x01
#define RTR_DEFAULT		2000
#define W6100_RTR		0x0019 /* Retry Time-value Register */


#define W6100_Sn_RXMEM_SIZE(n)	\
		(0x1001e + (n) * 0x40000) /* Sn RX Memory Size */
#define W6100_Sn_TXMEM_SIZE(n)	\
		(0x1001f + (n) * 0x40000) /* Sn TX Memory Size */

#define W6100_Sn_TX_MEM_START	0x20000
#define W6100_TX_MEM_SIZE	0x04000
#define W6100_Sn_RX_MEM_START	0x30000
#define W6100_RX_MEM_SIZE	0x04000

/* Delay for PHY write/read operations (25.6 us) */
#define W6100_PHY_ACCESS_DELAY		26U
struct w6100_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec interrupt;
	struct gpio_dt_spec reset;
	int32_t timeout;
};

struct w6100_runtime {
	struct net_if *iface;

	K_KERNEL_STACK_MEMBER(thread_stack,
			      CONFIG_ETH_W6100_RX_THREAD_STACK_SIZE);
	struct k_thread thread;
	uint8_t mac_addr[6];
	struct gpio_callback gpio_cb;
	struct k_sem tx_sem;
	struct k_sem int_sem;
	bool link_up;
	uint8_t buf[NET_ETH_MAX_FRAME_SIZE];
};

#endif /*ETH_W6100_PRIV_H_*/
