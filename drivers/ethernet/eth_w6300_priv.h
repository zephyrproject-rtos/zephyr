/* W6300 Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2026 WIZnet Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#ifndef _W6300_
#define _W6300_

/*
 * W6300 Address Encoding:
 *   A 32-bit value encodes both block select and register offset:
 *     Bits [7:0]   = Block select byte
 *     Bits [23:8]  = 16-bit register offset within block
 *
 * SPI Frame (VDM mode):
 *   Byte 0: Opcode = Block[4:0] | RW[5] | Mode[7:6]
 *   Byte 1: Address high (offset bits 15:8)
 *   Byte 2: Address low  (offset bits 7:0)
 *   Byte 3+: Data
 */

/* Block Select */
#define W6300_CREG_BLOCK     0x00
#define W6300_SREG_BLOCK(n)  (1 + 4 * (n))
#define W6300_TXBUF_BLOCK(n) (2 + 4 * (n))
#define W6300_RXBUF_BLOCK(n) (3 + 4 * (n))

/* Address construction macro */
#define W6300_ADDR(offset, block) (((uint32_t)(offset) << 8) | (block))

/* SPI Control flags (bit 5 of opcode byte) */
#define W6300_SPI_READ  0x00
#define W6300_SPI_WRITE 0x20

/*
 * W6300 Common Registers (Block 0x00)
 */
#define W6300_CIDR0 W6300_ADDR(0x0000, W6300_CREG_BLOCK)
#define W6300_CIDR1 W6300_ADDR(0x0001, W6300_CREG_BLOCK)
#define W6300_CIDR2 W6300_ADDR(0x0004, W6300_CREG_BLOCK)
#define W6300_SYSR  W6300_ADDR(0x2000, W6300_CREG_BLOCK)
#define W6300_SYCR0 W6300_ADDR(0x2004, W6300_CREG_BLOCK)
#define W6300_SYCR1 W6300_ADDR(0x2005, W6300_CREG_BLOCK)

/* Interrupt registers */
#define W6300_IR    W6300_ADDR(0x2100, W6300_CREG_BLOCK)
#define W6300_SIR   W6300_ADDR(0x2101, W6300_CREG_BLOCK)
#define W6300_SIMR  W6300_ADDR(0x2114, W6300_CREG_BLOCK)
#define W6300_IRCLR W6300_ADDR(0x2108, W6300_CREG_BLOCK)

/* Network info registers */
#define W6300_SHAR W6300_ADDR(0x4120, W6300_CREG_BLOCK)
#define W6300_GAR  W6300_ADDR(0x4130, W6300_CREG_BLOCK)
#define W6300_SUBR W6300_ADDR(0x4134, W6300_CREG_BLOCK)
#define W6300_SIPR W6300_ADDR(0x4138, W6300_CREG_BLOCK)

/* Retry registers */
#define W6300_RTR W6300_ADDR(0x4200, W6300_CREG_BLOCK)
#define W6300_RCR W6300_ADDR(0x4204, W6300_CREG_BLOCK)

/* PHY registers */
#define W6300_PHYSR  W6300_ADDR(0x3000, W6300_CREG_BLOCK)
#define W6300_PHYCR0 W6300_ADDR(0x301C, W6300_CREG_BLOCK)
#define W6300_PHYCR1 W6300_ADDR(0x301D, W6300_CREG_BLOCK)

/* Lock registers */
#define W6300_CHPLCKR W6300_ADDR(0x41F4, W6300_CREG_BLOCK)
#define W6300_NETLCKR W6300_ADDR(0x41F5, W6300_CREG_BLOCK)
#define W6300_PHYLCKR W6300_ADDR(0x41F6, W6300_CREG_BLOCK)

/* SYSR bits */
#define SYSR_CHPL BIT(7)
#define SYSR_NETL BIT(6)
#define SYSR_PHYL BIT(5)

/* SYCR0 values */
#define SYCR0_RST    0x00   /* Software reset */
#define SYCR0_NORMAL BIT(7) /* 1 = normal operation */

/* SYCR1 bits */
#define SYCR1_IEN BIT(7) /* Global interrupt enable */

/* PHYSR bits */
#define PHYSR_LNK BIT(0) /* Link: 1=Up, 0=Down */
#define PHYSR_SPD BIT(1) /* Speed: 1=10M, 0=100M */
#define PHYSR_DPX BIT(2) /* Duplex: 1=Half, 0=Full */

/* Lock/Unlock values */
#define CHPLCKR_UNLOCK 0xCE
#define CHPLCKR_LOCK   0xFF
#define NETLCKR_UNLOCK 0x3A
#define NETLCKR_LOCK   0xC5
#define PHYLCKR_UNLOCK 0x53
#define PHYLCKR_LOCK   0xFF

/*
 * W6300 Socket Registers (parametric by socket number)
 */
#define W6300_Sn_MR(n)     W6300_ADDR(0x0000, W6300_SREG_BLOCK(n))
#define W6300_Sn_CR(n)     W6300_ADDR(0x0010, W6300_SREG_BLOCK(n))
#define W6300_Sn_IR(n)     W6300_ADDR(0x0020, W6300_SREG_BLOCK(n))
#define W6300_Sn_IMR(n)    W6300_ADDR(0x0024, W6300_SREG_BLOCK(n))
#define W6300_Sn_IRCLR(n)  W6300_ADDR(0x0028, W6300_SREG_BLOCK(n))
#define W6300_Sn_SR(n)     W6300_ADDR(0x0030, W6300_SREG_BLOCK(n))
#define W6300_Sn_TX_BSR(n) W6300_ADDR(0x0200, W6300_SREG_BLOCK(n))
#define W6300_Sn_TX_FSR(n) W6300_ADDR(0x0204, W6300_SREG_BLOCK(n))
#define W6300_Sn_TX_RD(n)  W6300_ADDR(0x0208, W6300_SREG_BLOCK(n))
#define W6300_Sn_TX_WR(n)  W6300_ADDR(0x020C, W6300_SREG_BLOCK(n))
#define W6300_Sn_RX_BSR(n) W6300_ADDR(0x0220, W6300_SREG_BLOCK(n))
#define W6300_Sn_RX_RSR(n) W6300_ADDR(0x0224, W6300_SREG_BLOCK(n))
#define W6300_Sn_RX_RD(n)  W6300_ADDR(0x0228, W6300_SREG_BLOCK(n))
#define W6300_Sn_RX_WR(n)  W6300_ADDR(0x022C, W6300_SREG_BLOCK(n))

/* Convenience macros for Socket 0 */
#define W6300_S0_MR     W6300_Sn_MR(0)
#define W6300_S0_CR     W6300_Sn_CR(0)
#define W6300_S0_IR     W6300_Sn_IR(0)
#define W6300_S0_IMR    W6300_Sn_IMR(0)
#define W6300_S0_IRCLR  W6300_Sn_IRCLR(0)
#define W6300_S0_SR     W6300_Sn_SR(0)
#define W6300_S0_TX_BSR W6300_Sn_TX_BSR(0)
#define W6300_S0_TX_FSR W6300_Sn_TX_FSR(0)
#define W6300_S0_TX_RD  W6300_Sn_TX_RD(0)
#define W6300_S0_TX_WR  W6300_Sn_TX_WR(0)
#define W6300_S0_RX_BSR W6300_Sn_RX_BSR(0)
#define W6300_S0_RX_RSR W6300_Sn_RX_RSR(0)
#define W6300_S0_RX_RD  W6300_Sn_RX_RD(0)
#define W6300_S0_RX_WR  W6300_Sn_RX_WR(0)

/* Socket Mode Register values */
#define S0_MR_MACRAW 0x07 /* MACRAW mode (W6300) */
#define S0_MR_MF     0x80 /* MAC Filter enable (bit 7) */

/* Socket Command Register values */
#define S0_CR_OPEN  0x01
#define S0_CR_CLOSE 0x10
#define S0_CR_SEND  0x20
#define S0_CR_RECV  0x40

/* Socket Interrupt Register bits */
#define S0_IR_SENDOK  0x10
#define S0_IR_RECV    0x04
#define S0_IR_DISCON  0x02
#define S0_IR_TIMEOUT 0x08
#define S0_IR_CON     0x01

/* Socket Status Register values */
#define S0_SR_MACRAW 0x42

/* TX/RX buffer block for Socket 0 */
#define W6300_S0_TX_MEM_BLOCK W6300_TXBUF_BLOCK(0)
#define W6300_S0_RX_MEM_BLOCK W6300_RXBUF_BLOCK(0)

/* Default buffer sizes (16KB each for Socket 0) */
#define W6300_TX_MEM_SIZE 0x4000
#define W6300_RX_MEM_SIZE 0x4000

/* RTR default value */
#define RTR_DEFAULT 2000

/* Socket Interrupt Mask - Socket 0 */
#define IR_S0 0x01

/* MAC Filter bit position in S0_MR */
#define W6300_S0_MR_MF 7

struct w6300_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec interrupt;
	struct gpio_dt_spec reset;
	struct net_eth_mac_config mac_cfg;
	int32_t timeout;
};

struct w6300_runtime {
	struct net_if *iface;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ETH_W6300_RX_THREAD_STACK_SIZE);
	struct k_thread thread;
	uint8_t mac_addr[6];
	struct gpio_callback gpio_cb;
	struct k_sem tx_sem;
	struct k_sem int_sem;
	/*
	 * Serializes the whole TX path.  The driver uses a shared staging buffer
	 * and a single hardware TX write pointer, so concurrent TCP/ICMP sends
	 * must not interleave.
	 */
	struct k_mutex tx_lock;
	/*
	 * Serializes Socket 0 Command Register (S0_CR) writes between the
	 * net TX thread (w6300_tx) and the RX/IRQ thread (w6300_thread).
	 * Without this, SEND and RECV commands can overwrite each other
	 * on the 33 MHz PIO QSPI bus where k_yield() allows both threads
	 * to issue SPI transactions concurrently.
	 */
	struct k_mutex cmd_lock;
	bool link_up;
	uint8_t buf[NET_ETH_MAX_FRAME_SIZE];
};

#endif /* _W6300_ */
