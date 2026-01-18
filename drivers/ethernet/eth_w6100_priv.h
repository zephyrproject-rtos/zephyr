/*W6100 Stand-alone Ethernet Controller with SPI by
 * Author: Sayed Naser Moravej <seyednasermoravej@gmail.com>
 * based on
 *W5500 Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2020 Linumiz
 * Author: Parthiban Nallathambi <parthiban@linumiz.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#ifndef _W6100_
#define _W6100_

/*
 * W5500 common registers
 */
#define W6100_COMMON_REGS	0x0000

#define W6100_SYSR			0x2000
#define W6100_SYCR0			0x2004 /*SYCR0 (System Config Register 0) replacement for  Mode Register in w5500 */
#define W6100_GW			0x4130
#define W6100_NET4MR		0x4000
#define W6100_CHPLCKR		0x41f4
//////////////////////maybe MR_RST must be changed
#define SYCR0_RST			0x00 /* S/W reset MR_RST in w5500*/
#define CHPLCKR_UNLOCK		0xce
///////////////////////////////////////
//////////ping ipv4
#define W6100_NETLCKR		0x41f5
#define NET_UNLOCK			0x3a
#define NET4MR_PB			0x00 /* Ping block */
////////////////////////////////////////
#define MR_AI				0x02 /* Address Auto-Increment */
#define MR_IND				0x01 /* Indirect mode */
#define W6100_SHAR			0x4120 /* Source MAC address */
#define W6100_IR			0x2100 /* Interrupt Register */


//////////////it seems that is not used
#define W6100_COMMON_REGS_LEN	0x0040
////////////////////////////
//#define W5500_PHYCFGR		0x002E /* PHY Configuration register */
#define W6100_PHYSR			0x3000 /* PHYSR (PHY Status Register), replacement for W5500_PHYCFGR*/

#define W6100_Sn_MR			0x0000 /* Sn Mode Register */
#define W6100_Sn_CR			0x0010 /* Sn Command Register */
#define W6100_Sn_IR			0x0020 /* Sn Interrupt Register */
#define W6100_Sn_SR			0x0030 /* Sn Status Register */
#define W6100_Sn_TX_FSR		0x0204 /* Sn Transmit free memory size */
#define W6100_Sn_TX_RD		0x0208 /* Sn Transmit memory read pointer */
#define W6100_Sn_TX_WR		0x020c /* Sn Transmit memory write pointer */
#define W6100_Sn_RX_RSR		0x0224 /* Sn Receive free memory size */
#define W6100_Sn_RX_RD		0x0228 /* Sn Receive memory read pointer */

#define W6100_S0_REGS		0x10000
#define W6100_Sn_IMR		0x0024

#define W6100_S0_MR			(W6100_S0_REGS + W6100_Sn_MR)
#define S0_MR_MACRAW		0x07 /* MAC RAW mode */
#define S0_MR_MF			0x40 /* MAC Filter for W6100 */
#define W6100_S0_CR			(W6100_S0_REGS + W6100_Sn_CR)
#define S0_CR_OPEN		0x01 /* OPEN command */
#define S0_CR_CLOSE		0x10 /* CLOSE command */
#define S0_CR_SEND		0x20 /* SEND command */
#define S0_CR_RECV		0x40 /* RECV command */
#define W6100_S0_IR		(W6100_S0_REGS + W6100_Sn_IR)
#define W6100_SLIR		0x2102 
#define W6100_SLIRCLR	0x2128
#define S0_IR_SENDOK		0x10 /* complete sending */
#define S0_IR_RECV		0x04 /* receiving data */
#define W6100_S0_SR		(W6100_S0_REGS + W6100_Sn_SR)
#define S0_SR_MACRAW		0x42 /* mac raw mode */
#define W6100_S0_TX_FSR		(W6100_S0_REGS + W6100_Sn_TX_FSR)
#define W6100_S0_TX_RD		(W6100_S0_REGS + W6100_Sn_TX_RD)
#define W6100_S0_TX_WR		(W6100_S0_REGS + W6100_Sn_TX_WR)
#define W6100_S0_RX_RSR		(W6100_S0_REGS + W6100_Sn_RX_RSR)
#define W6100_S0_RX_RD		(W6100_S0_REGS + W6100_Sn_RX_RD)
#define W6100_S0_IMR		(W6100_S0_REGS + W6100_Sn_IMR)k

#define W6100_S0_MR_MF		7 /* MAC Filter for W6100 */
// #define W6100_S0_MR_PM		
#define W6100_Sn_REGS_LEN	0x0040
#define W6100_SIMR		0x2114 /* Socket Interrupt Mask Register */
#define IR_S0			0x01
#define RTR_DEFAULT		2000
#define W6100_RTR		0x4200 /* Retry Time-value Register */


#define W6100_Sn_RXMEM_SIZE(n)	\
		(0x10220 + (n) * 0x40000) /* Sn RX Memory Size */
#define W6100_Sn_TXMEM_SIZE(n)	\
		(0x10200 + (n) * 0x40000) /* Sn TX Memory Size */

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

#endif /*_W6100_*/
