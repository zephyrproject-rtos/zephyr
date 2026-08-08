/* W5100S Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/phy.h>

#ifndef _W5100S_
#define _W5100S_

/*
 * W5100S common registers
 */
#define W5100S_MR      0x0000 /* Mode Register */
#define MR_RST         0x80   /* S/W reset */
#define W5100S_SHAR    0x0009 /* Source MAC address */
#define W5100S_IMR     0x0016 /* Interrupt Mask Register */
#define W5100S_PHYSR0  0x003C /* PHY Status Register 0 */
#define W5100S_NETLCKR 0x0071 /* Network Lock Register */
#define NETLCKR_UNLOCK 0x3A
#define W5100S_VERR    0x0080 /* Chip Version Register */
#define W5100S_VERSION 0x51

/* PHYSR0 bit positions (note: polarity differs from W5500 PHYCFGR) */
#define W5100S_PHYSR0_LNK_BIT  0 /* Link status: 1 = up */
#define W5100S_PHYSR0_FSPD_BIT 1 /* Flag speed: 1 = 10Mbps, 0 = 100Mbps */
#define W5100S_PHYSR0_FDPX_BIT 2 /* Flag duplex: 1 = half, 0 = full */

/*
 * W5100S socket 0 registers (flat address map, base 0x0400)
 */
#define W5100S_S0_REGS 0x0400

#define W5100S_S0_MR            (W5100S_S0_REGS + 0x0000) /* Sn Mode Register */
#define S0_MR_MACRAW            0x04                      /* MAC RAW mode */
#define W5100S_S0_MR_MF         6                         /* MAC Filter bit */
#define W5100S_S0_CR            (W5100S_S0_REGS + 0x0001) /* Sn Command Register */
#define W5100S_S0_IR            (W5100S_S0_REGS + 0x0002) /* Sn Interrupt Register */
#define W5100S_SOCKET_COUNT     4
#define W5100S_Sn_RXBUF_SIZE(n) (W5100S_S0_REGS + (n) * 0x0100 + 0x001E)
#define W5100S_Sn_TXBUF_SIZE(n) (W5100S_S0_REGS + (n) * 0x0100 + 0x001F)
#define W5100S_S0_TX_WR         (W5100S_S0_REGS + 0x0024) /* Sn TX write pointer */
#define W5100S_S0_RX_RSR        (W5100S_S0_REGS + 0x0026) /* Sn RX received size */
#define W5100S_S0_RX_RD         (W5100S_S0_REGS + 0x0028) /* Sn RX read pointer */

/*
 * W5100S TX/RX memory: 8 KB each, socket 0 uses the whole block.
 */
#define W5100S_S0_TX_MEM_START 0x4000
#define W5100S_TX_MEM_SIZE     0x2000
#define W5100S_S0_RX_MEM_START 0x6000
#define W5100S_RX_MEM_SIZE     0x2000

#endif /*_W5100S_*/
