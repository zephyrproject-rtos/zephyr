/*
 * BCM2711 GENET v5 Ethernet — register definitions.
 *
 * Layout and constants ported from Linux 5.4
 *   drivers/net/ethernet/broadcom/genet/bcmgenet.h
 *
 *   Copyright (c) 2026 Khaled Abdalla <khaled.3bdulaziz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ETH_BCM_GENET_H
#define ETH_BCM_GENET_H

#include <zephyr/sys/sys_io.h>

/* ───── Top-level GENET block offsets ───────────────────────────────────── */
#define GENET_SYS_OFF           0x0000
#define GENET_GR_BRIDGE_OFF     0x0040
#define GENET_EXT_OFF           0x0080
#define GENET_INTRL2_0_OFF      0x0200
#define GENET_INTRL2_1_OFF      0x0240
#define GENET_RBUF_OFF          0x0300
#define GENET_UMAC_OFF          0x0800
#define GENET_HFB_OFF           0x8000
#define GENET_HFB_REG_OFF       0xFC00
/* DMA offsets are GENET-version dependent; v4/v5 (BCM2711): */
#define GENET_RDMA_OFF          0x2000
#define GENET_TDMA_OFF          0x4000

/* ───── SYS block (0x0000) ──────────────────────────────────────────────── */
#define SYS_REV_CTRL            0x00
#define SYS_PORT_CTRL           0x04
#define  PORT_MODE_EXT_GPHY     3
#define SYS_RBUF_FLUSH_CTRL     0x08
#define  SYS_RBUF_FLUSH_RESET   (1U << 1)
#define SYS_TBUF_FLUSH_CTRL     0x0C

/* ───── EXT block (0x0080) ──────────────────────────────────────────────── */
/* EXT_EXT_PWR_MGMT gates UMAC DLL/bias — must be cleared before any UMAC reg */
#define EXT_EXT_PWR_MGMT        0x00
#define  EXT_PWR_DOWN_BIAS      (1U << 0)
#define  EXT_PWR_DOWN_DLL       (1U << 1)
#define  EXT_PWR_DOWN_PHY       (1U << 2)
#define  EXT_PWR_DN_EN_LD       (1U << 3)
#define  EXT_ENERGY_DET         (1U << 4)
#define  EXT_IDDQ_FROM_PHY      (1U << 5)
#define  EXT_IDDQ_GLBL_PWR      (1U << 7)
#define  EXT_PHY_RESET          (1U << 8)
#define  EXT_ENERGY_DET_MASK    (1U << 12)
#define  EXT_PWR_DOWN_PHY_TX    (1U << 16)
#define  EXT_PWR_DOWN_PHY_RX    (1U << 17)
#define  EXT_PWR_DOWN_PHY_SD    (1U << 18)
#define  EXT_PWR_DOWN_PHY_RD    (1U << 19)
#define  EXT_PWR_DOWN_PHY_EN    (1U << 20)

#define EXT_RGMII_OOB_CTRL      0x0C
#define  RGMII_MODE_EN_V123     (1U << 0)
#define  RGMII_LINK             (1U << 4)
#define  OOB_DISABLE            (1U << 5)
#define  RGMII_MODE_EN          (1U << 6)
#define  ID_MODE_DIS            (1U << 16)

#define EXT_GPHY_CTRL           0x1C
#define  EXT_CFG_IDDQ_BIAS      (1U << 0)
#define  EXT_CFG_PWR_DOWN       (1U << 1)
#define  EXT_CFG_IDDQ_GLOBAL_PWR (1U << 3)
#define  EXT_CK25_DIS           (1U << 4)
#define  EXT_GPHY_RESET         (1U << 5)

/* ───── INTRL2 (interrupt registers, two banks at 0x0200 and 0x0240) ────── */
#define INTRL2_CPU_STAT         0x00
#define INTRL2_CPU_SET          0x04
#define INTRL2_CPU_CLEAR        0x08
#define INTRL2_CPU_MASK_STATUS  0x0C
#define INTRL2_CPU_MASK_SET     0x10
#define INTRL2_CPU_MASK_CLEAR   0x14

/* INTRL2_0 bits */
#define UMAC_IRQ_LINK_UP        (1U << 4)
#define UMAC_IRQ_LINK_DOWN      (1U << 5)
#define UMAC_IRQ_RXDMA_MBDONE   (1U << 13)
#define UMAC_IRQ_TXDMA_MBDONE   (1U << 16)
#define UMAC_IRQ_MDIO_DONE      (1U << 23)
#define UMAC_IRQ_MDIO_ERROR     (1U << 24)

/* ───── RBUF block (0x0300) ─────────────────────────────────────────────── */
#define RBUF_CTRL               0x00
#define  RBUF_64B_EN            (1U << 0)
#define  RBUF_ALIGN_2B          (1U << 1)
#define  RBUF_BAD_DIS           (1U << 2)
#define RBUF_CHK_CTRL           0x14
#define RBUF_TBUF_SIZE_CTRL     0xB4

/* ───── UMAC block (0x0800) ─────────────────────────────────────────────── */
#define UMAC_CMD                0x008
#define  CMD_TX_EN              (1U << 0)
#define  CMD_RX_EN              (1U << 1)
#define  CMD_SPEED_10           (0U << 2)
#define  CMD_SPEED_100          (1U << 2)
#define  CMD_SPEED_1000         (2U << 2)
#define  CMD_SPEED_MASK         (3U << 2)
#define  CMD_SW_RESET           (1U << 13)
#define  CMD_LCL_LOOP_EN        (1U << 15)
#define UMAC_MAC0               0x00C
#define UMAC_MAC1               0x010
#define UMAC_MAX_FRAME_LEN      0x014
#define UMAC_TX_FLUSH           0x334
#define UMAC_MIB_CTRL           0x580
#define  MIB_RESET_RX           (1U << 0)
#define  MIB_RESET_RUNT         (1U << 1)
#define  MIB_RESET_TX           (1U << 2)
#define UMAC_MDIO_CMD           0x614
#define  MDIO_START_BUSY        (1U << 29)
#define  MDIO_READ_FAIL         (1U << 28)
#define  MDIO_RD                (2U << 26)
#define  MDIO_WR                (1U << 26)
#define  MDIO_PMD_SHIFT         21
#define  MDIO_REG_SHIFT         16
#define  MDIO_DATA_MASK         0xFFFF

#define GENET_MAX_MTU           1536U

/* ───── DMA descriptor (12 bytes for v4/v5) ─────────────────────────────── */
#define DMA_DESC_LENGTH_STATUS  0x00
#define DMA_DESC_ADDRESS_LO     0x04
#define DMA_DESC_ADDRESS_HI     0x08
#define GENET_DESC_SIZE         12U

#define DMA_BUFLENGTH_MASK      0x0FFF
#define DMA_BUFLENGTH_SHIFT     16
#define DMA_OWN                 0x8000
#define DMA_EOP                 0x4000
#define DMA_SOP                 0x2000
#define DMA_WRAP                0x1000
#define DMA_TX_QTAG_SHIFT       7
#define DMA_TX_APPEND_CRC       0x0040

#define DMA_RX_OV               0x0001
#define DMA_RX_CRC_ERROR        0x0002
#define DMA_RX_RXER             0x0004
#define DMA_RX_NO               0x0008
#define DMA_RX_LG               0x0010
#define DMA_RX_ERROR_MASK       (DMA_RX_OV | DMA_RX_CRC_ERROR | \
				 DMA_RX_RXER | DMA_RX_LG)

#define DMA_P_INDEX_MASK        0xFFFF
#define DMA_C_INDEX_MASK        0xFFFF

/* ───── DMA layout (v4/v5) ──────────────────────────────────────────────── */
#define GENET_TOTAL_DESC        256U
#define GENET_WORDS_PER_BD      3U
#define DMA_RING_SIZE           0x40U
#define DESC_INDEX              16U                       /* default ring */
#define NUM_DMA_RINGS           (DESC_INDEX + 1U)

#define DMA_DESC_ARRAY_SIZE     (GENET_TOTAL_DESC * GENET_WORDS_PER_BD * 4U)
#define DMA_RINGS_SIZE          (DMA_RING_SIZE * NUM_DMA_RINGS)

#define RDMA_RING_BASE(n)       (GENET_RDMA_OFF + DMA_DESC_ARRAY_SIZE + \
				 (n) * DMA_RING_SIZE)
#define TDMA_RING_BASE(n)       (GENET_TDMA_OFF + DMA_DESC_ARRAY_SIZE + \
				 (n) * DMA_RING_SIZE)
#define RDMA_CTRL_BASE          (GENET_RDMA_OFF + DMA_DESC_ARRAY_SIZE + DMA_RINGS_SIZE)
#define TDMA_CTRL_BASE          (GENET_TDMA_OFF + DMA_DESC_ARRAY_SIZE + DMA_RINGS_SIZE)

/* Per-ring control offsets (identical layout for RDMA/TDMA, semantics differ) */
#define RING_HW_PTR_LO          0x00
#define RING_HW_PTR_HI          0x04
#define RING_HW_INDEX           0x08   /* RDMA: prod / TDMA: cons */
#define RING_SW_INDEX           0x0C   /* RDMA: cons / TDMA: prod */
#define RING_BUF_SIZE           0x10
#define RING_START_ADDR_LO      0x14
#define RING_START_ADDR_HI      0x18
#define RING_END_ADDR_LO        0x1C
#define RING_END_ADDR_HI        0x20
#define RING_MBUF_DONE_THRESH   0x24
#define RING_XON_XOFF           0x28
#define RING_SW_PTR_LO          0x2C
#define RING_SW_PTR_HI          0x30

#define RING_BUF_SIZE_VAL(n, sz) (((uint32_t)(n) << 16) | (uint32_t)(sz))

#define DMA_FC_THRESH_HI        (GENET_TOTAL_DESC >> 4)
#define DMA_FC_THRESH_LO        5
#define RDMA_XON_XOFF_VAL       (DMA_FC_THRESH_HI | (DMA_FC_THRESH_LO << 16))

#define DESC_START_ADDR(idx)    ((idx) * GENET_WORDS_PER_BD)
#define DESC_END_ADDR(idx, cnt) (((idx) + (cnt)) * GENET_WORDS_PER_BD - 1U)

/* Global DMA control registers (relative to RDMA_CTRL_BASE / TDMA_CTRL_BASE) */
#define DMA_RING_CFG            0x00
#define DMA_CTRL                0x04
#define  DMA_EN                 (1U << 0)
#define  DMA_RING_BUF_EN_SHIFT  1
#define  DMA_RING_EN(r)         (1U << ((r) + DMA_RING_BUF_EN_SHIFT))
#define  DMA_RING_CFG_EN(r)     (1U << (r))
#define DMA_STATUS              0x08
#define  DMA_DISABLED           (1U << 0)
#define DMA_SCB_BURST_SIZE      0x0C
#define  DMA_SCB_BURST_VAL      8
#define DMA_ARB_CTRL            0x2C
#define  DMA_ARBITER_SP         0x02
#define DMA_TIMEOUT_USEC        5000U

/* ───── MII (standard PHY registers) ────────────────────────────────────── */
#define MII_BMCR                0x00
#define  BMCR_RESET             (1U << 15)
#define  BMCR_ANENABLE          (1U << 12)
#define  BMCR_ANRESTART         (1U << 9)
#define MII_BMSR                0x01
#define  BMSR_LSTATUS           (1U << 2)
#define  BMSR_ANEGCOMPLETE      (1U << 5)
#define MII_ADVERTISE           0x04
#define  ADVERTISE_CSMA         0x0001
#define  ADVERTISE_10HALF       (1U << 5)
#define  ADVERTISE_10FULL       (1U << 6)
#define  ADVERTISE_100HALF      (1U << 7)
#define  ADVERTISE_100FULL      (1U << 8)
#define  ADVERTISE_PAUSE        (1U << 10)
#define MII_LPA                 0x05
#define MII_CTRL1000            0x09
#define  ADVERTISE_1000FULL     (1U << 9)
#define MII_STAT1000            0x0A
#define  LPA_1000FULL           (1U << 11)

/* ───── DMA-buffer address translation ──────────────────────────────────── */
/* BCM2711 GENET is on the ARM AXI bus — identity mapping, no offset needed. */
#define GENET_DMA_ADDR(p)       ((uint32_t)(uintptr_t)(p))

/* ───── Driver-side buffer configuration ────────────────────────────────── */
#define GENET_RX_DATA_OFFSET    2U    /* RBUF_ALIGN_2B prepends 2 bytes */
#define GENET_NUM_RX_DESC       16U
#define GENET_NUM_TX_DESC       16U
#define GENET_RX_BUF_SIZE       2048U
#define GENET_TX_BUF_SIZE       2048U
#define GENET_RX_RING           DESC_INDEX
#define GENET_TX_RING           DESC_INDEX
#define GENET_RX_DESC_START     0U
#define GENET_TX_DESC_START     0U

/* ───── Accessor macros (operate on the device_map'd virtual base) ──────── */
#define GENET_RD(b, off)        sys_read32((b) + (off))
#define GENET_WR(b, val, off)   sys_write32((val), (b) + (off))

#define SYS_RD(b, r)            GENET_RD((b), GENET_SYS_OFF + (r))
#define SYS_WR(b, v, r)         GENET_WR((b), (v), GENET_SYS_OFF + (r))

#define EXT_RD(b, r)            GENET_RD((b), GENET_EXT_OFF + (r))
#define EXT_WR(b, v, r)         GENET_WR((b), (v), GENET_EXT_OFF + (r))

#define INTRL2_0_RD(b, r)       GENET_RD((b), GENET_INTRL2_0_OFF + (r))
#define INTRL2_0_WR(b, v, r)    GENET_WR((b), (v), GENET_INTRL2_0_OFF + (r))
#define INTRL2_1_RD(b, r)       GENET_RD((b), GENET_INTRL2_1_OFF + (r))
#define INTRL2_1_WR(b, v, r)    GENET_WR((b), (v), GENET_INTRL2_1_OFF + (r))

#define RBUF_RD(b, r)           GENET_RD((b), GENET_RBUF_OFF + (r))
#define RBUF_WR(b, v, r)        GENET_WR((b), (v), GENET_RBUF_OFF + (r))

#define UMAC_RD(b, r)           GENET_RD((b), GENET_UMAC_OFF + (r))
#define UMAC_WR(b, v, r)        GENET_WR((b), (v), GENET_UMAC_OFF + (r))

#define RDESC_WR(b, idx, woff, v) \
	sys_write32((v), (b) + GENET_RDMA_OFF + (idx) * GENET_DESC_SIZE + (woff))
#define RDESC_RD(b, idx, woff) \
	sys_read32((b) + GENET_RDMA_OFF + (idx) * GENET_DESC_SIZE + (woff))
#define TDESC_WR(b, idx, woff, v) \
	sys_write32((v), (b) + GENET_TDMA_OFF + (idx) * GENET_DESC_SIZE + (woff))

#define RDMA_RING_RD(b, r, off) sys_read32((b) + RDMA_RING_BASE(r) + (off))
#define RDMA_RING_WR(b, r, off, v) sys_write32((v), (b) + RDMA_RING_BASE(r) + (off))
#define TDMA_RING_RD(b, r, off) sys_read32((b) + TDMA_RING_BASE(r) + (off))
#define TDMA_RING_WR(b, r, off, v) sys_write32((v), (b) + TDMA_RING_BASE(r) + (off))

#define RDMA_CTRL_RD(b, r)      sys_read32((b) + RDMA_CTRL_BASE + (r))
#define RDMA_CTRL_WR(b, r, v)   sys_write32((v), (b) + RDMA_CTRL_BASE + (r))
#define TDMA_CTRL_RD(b, r)      sys_read32((b) + TDMA_CTRL_BASE + (r))
#define TDMA_CTRL_WR(b, r, v)   sys_write32((v), (b) + TDMA_CTRL_BASE + (r))

#endif /* ETH_BCM_GENET_H */
