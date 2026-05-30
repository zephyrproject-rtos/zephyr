/*
 * BCM2711 GENET v5 Ethernet driver — Zephyr / Raspberry Pi 4B
 *
 * Ported from Linux 5.4 drivers/net/ethernet/broadcom/genet/bcmgenet.c.
 *
 * Configuration: BCM2711 RPi 4B, external BCM54213PE PHY (MDIO addr 1),
 * RGMII-RXID, bare-metal boot (VPU network-boot, no U-Boot).
 *
 * Key platform quirk: the VideoCore firmware sets EXT_PWR_DOWN_DLL and
 * EXT_PWR_DOWN_BIAS in EXT_EXT_PWR_MGMT after using GENET for network-boot.
 * Those bits gate the UMAC register bus; any UMAC access before clearing them
 * returns SLVERR (EC=0x2F SError on Cortex-A72). genet_power_up() mirrors
 * Linux bcmgenet_power_up(GENET_POWER_PASSIVE) to clear them before touching
 * any UMAC, MDIO, or DMA register.
 *
 * DMA coherency: CONFIG_CACHE_MANAGEMENT=y is required. Cortex-A72 runs with
 * the D-cache enabled; without explicit flush/invalidate the DMA engine reads
 * stale zeros from physical memory while fresh data sits in L1/L2.
 *
 * Copyright (c) 2026 Khaled Abdalla <khaled.3bdulaziz@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_bcm2711_genet

#define LOG_MODULE_NAME eth_genet
#define LOG_LEVEL       CONFIG_ETHERNET_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/cache.h>
#include <zephyr/irq.h>
#include <zephyr/random/random.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <ethernet/eth_stats.h>

#include "eth_bcm_genet.h"

#define GENET_BUF_ALIGN          64U
#define GENET_PHY_POLL_INTERVAL  500U

/* ───── Static DMA buffers (identity-mapped → same VA == PA on RPi4) ───── */
static uint8_t genet_rx_buf[GENET_NUM_RX_DESC][GENET_RX_BUF_SIZE]
	__aligned(GENET_BUF_ALIGN);
static uint8_t genet_tx_buf[GENET_NUM_TX_DESC][GENET_TX_BUF_SIZE]
	__aligned(GENET_BUF_ALIGN);

struct genet_data {
	mm_reg_t              base;
	struct net_if        *iface;
	const struct device  *dev;
	uint8_t               mac[6];
	uint8_t               phy_addr;
	bool                  phy_rgmii_rxid;
	bool                  link_up;

	/* RX bookkeeping */
	uint16_t rx_cons;
	uint16_t rx_desc_pos;

	/* TX bookkeeping (clean: PROD=CONS=0 on bare-metal boot) */
	uint16_t tx_prod;
	uint32_t tx_write_ptr;
	uint32_t tx_ring_start;
	uint32_t tx_ring_end;

	struct k_work             rx_work;
	struct k_work_delayable   phy_poll_work;
};

struct genet_config {
	uintptr_t       phys_base;
	size_t          phys_size;
	void          (*irq0_config)(void);
	void          (*irq1_config)(void);
	uint8_t         phy_addr;
	bool            phy_rgmii_rxid;
	bool            has_local_mac;
	const uint8_t  *local_mac;
};

/* ── Power management ────────────────────────────────────────────────────────
 * Must run before any UMAC, MDIO, or DMA access.
 * Mirrors Linux bcmgenet_power_up(GENET_POWER_PASSIVE), GENET v5 external PHY.
 */
static int genet_power_up(mm_reg_t base)
{
	uint32_t reg = EXT_RD(base, EXT_EXT_PWR_MGMT);

	LOG_DBG("EXT_EXT_PWR_MGMT = 0x%08x", reg);

	/* Clear UMAC DLL/bias gates (VPU sets these after network-boot). */
	reg &= ~(EXT_PWR_DOWN_DLL | EXT_PWR_DOWN_BIAS | EXT_ENERGY_DET_MASK);
	/* GENET v5, external non-16nm PHY: clear sub-block power-down bits. */
	reg &= ~(EXT_PWR_DOWN_PHY_EN | EXT_PWR_DOWN_PHY_RD |
		 EXT_PWR_DOWN_PHY_SD | EXT_PWR_DOWN_PHY_RX |
		 EXT_PWR_DOWN_PHY_TX | EXT_IDDQ_GLBL_PWR);
	/* Pulse PHY-IF reset per Linux (1 ms asserted). */
	EXT_WR(base, reg | EXT_PHY_RESET, EXT_EXT_PWR_MGMT);
	k_sleep(K_MSEC(1));
	EXT_WR(base, reg, EXT_EXT_PWR_MGMT);
	k_busy_wait(60);   /* DLL lock: Linux udelay(60) */

	if (EXT_RD(base, EXT_EXT_PWR_MGMT) & (EXT_PWR_DOWN_DLL | EXT_PWR_DOWN_BIAS)) {
		LOG_ERR("UMAC power-up failed: DLL/BIAS still set");
		return -EIO;
	}
	return 0;
}

/* Pulse SYS_RBUF_FLUSH_CTRL bit 1 — the "Reset MAC" trigger used by Linux
 * bcmgenet_umac_reset(). In the SYS block, always accessible regardless of
 * UMAC power state.
 */
static void genet_sys_reset(mm_reg_t base)
{
	uint32_t reg = SYS_RD(base, SYS_RBUF_FLUSH_CTRL);

	SYS_WR(base, reg |  SYS_RBUF_FLUSH_RESET, SYS_RBUF_FLUSH_CTRL);
	k_busy_wait(10);
	SYS_WR(base, reg & ~SYS_RBUF_FLUSH_RESET, SYS_RBUF_FLUSH_CTRL);
	k_busy_wait(10);
	SYS_WR(base, 0, SYS_RBUF_FLUSH_CTRL);
	k_busy_wait(10);
}

/* ─────────────────────────────────────────────────────────────────────────
 * UMAC + RBUF basic setup (Linux init_umac, set_hw_addr)
 * ─────────────────────────────────────────────────────────────────────────
 */
static void genet_umac_init(mm_reg_t base)
{
	/* Software-reset the UMAC. The local-loop bit is set per Linux. */
	UMAC_WR(base, 0,                              UMAC_CMD);
	UMAC_WR(base, CMD_SW_RESET | CMD_LCL_LOOP_EN, UMAC_CMD);
	k_busy_wait(2);
	UMAC_WR(base, 0,                              UMAC_CMD);

	/* MIB counter reset */
	UMAC_WR(base, MIB_RESET_RX | MIB_RESET_TX | MIB_RESET_RUNT, UMAC_MIB_CTRL);
	UMAC_WR(base, 0,                                            UMAC_MIB_CTRL);

	UMAC_WR(base, GENET_MAX_MTU,  UMAC_MAX_FRAME_LEN);
	RBUF_WR(base, RBUF_ALIGN_2B,  RBUF_CTRL);
	RBUF_WR(base, 1,              RBUF_TBUF_SIZE_CTRL);

	/* TX FIFO flush */
	UMAC_WR(base, 1, UMAC_TX_FLUSH);
	k_busy_wait(10);
	UMAC_WR(base, 0, UMAC_TX_FLUSH);
}

static void genet_set_mac(mm_reg_t base, const uint8_t *mac)
{
	UMAC_WR(base,
		((uint32_t)mac[0] << 24) | ((uint32_t)mac[1] << 16) |
		((uint32_t)mac[2] << 8)  |  (uint32_t)mac[3],
		UMAC_MAC0);
	UMAC_WR(base, ((uint32_t)mac[4] << 8) | (uint32_t)mac[5], UMAC_MAC1);
}

static void genet_set_speed(mm_reg_t base, uint32_t mbps)
{
	uint32_t field = (mbps == 1000) ? CMD_SPEED_1000 :
			 (mbps == 100)  ? CMD_SPEED_100  : CMD_SPEED_10;
	uint32_t reg;

	/* Set RGMII_LINK once link is confirmed (Linux bcmgenet_mac_config) */
	reg  = EXT_RD(base, EXT_RGMII_OOB_CTRL) | RGMII_LINK;
	EXT_WR(base, reg, EXT_RGMII_OOB_CTRL);

	reg  = (UMAC_RD(base, UMAC_CMD) & ~CMD_SPEED_MASK) | field;
	UMAC_WR(base, reg, UMAC_CMD);

	LOG_INF("speed %u Mbps: UMAC_CMD=0x%08x RGMII=0x%08x",
		mbps, UMAC_RD(base, UMAC_CMD), EXT_RD(base, EXT_RGMII_OOB_CTRL));
}

/* ─────────────────────────────────────────────────────────────────────────
 * EXT block: external-PHY power and RGMII OOB configuration
 * Linux bcmgenet_phy_power_set + bcmgenet_mii_setup combined.
 * ─────────────────────────────────────────────────────────────────────────
 */
static void genet_ext_init(mm_reg_t base, bool rxid)
{
	uint32_t reg;

	/* PHY 25 MHz reference clock — must be on before any other PHY action */
	reg  = EXT_RD(base, EXT_GPHY_CTRL);
	reg &= ~EXT_CK25_DIS;
	EXT_WR(base, reg, EXT_GPHY_CTRL);
	k_sleep(K_MSEC(1));

	/* Power up + assert PHY hardware reset */
	reg &= ~(EXT_CFG_IDDQ_BIAS | EXT_CFG_PWR_DOWN | EXT_CFG_IDDQ_GLOBAL_PWR);
	reg |=   EXT_GPHY_RESET;
	EXT_WR(base, reg, EXT_GPHY_CTRL);
	k_sleep(K_MSEC(1));

	/* Release reset; PHY PLL needs ~60us */
	reg &= ~EXT_GPHY_RESET;
	EXT_WR(base, reg, EXT_GPHY_CTRL);
	k_busy_wait(60);

	/* RGMII OOB control: enable RGMII, leave RGMII_LINK off until link up */
	reg  = EXT_RD(base, EXT_RGMII_OOB_CTRL);
	reg |= RGMII_MODE_EN;
	reg &= ~OOB_DISABLE;
	if (rxid) {
		reg |= ID_MODE_DIS;
	} else {
		reg &= ~ID_MODE_DIS;
	}
	EXT_WR(base, reg, EXT_RGMII_OOB_CTRL);
}

/* ─────────────────────────────────────────────────────────────────────────
 * MDIO (UMAC offset 0x614)
 * ─────────────────────────────────────────────────────────────────────────
 */
static int genet_mdio_wait(mm_reg_t base)
{
	for (int i = 0; i < 1000; i++) {
		if (!(UMAC_RD(base, UMAC_MDIO_CMD) & MDIO_START_BUSY)) {
			return 0;
		}
		k_busy_wait(10);
	}
	return -ETIMEDOUT;
}

static int genet_mdio_read(mm_reg_t base, uint8_t phy, uint8_t reg, uint16_t *val)
{
	uint32_t cmd = MDIO_RD | ((uint32_t)phy << MDIO_PMD_SHIFT) |
		       ((uint32_t)reg << MDIO_REG_SHIFT);
	int ret;

	UMAC_WR(base, cmd,                    UMAC_MDIO_CMD);
	UMAC_WR(base, cmd | MDIO_START_BUSY,  UMAC_MDIO_CMD);
	ret = genet_mdio_wait(base);
	if (ret) {
		return ret;
	}
	cmd = UMAC_RD(base, UMAC_MDIO_CMD);
	if (cmd & MDIO_READ_FAIL) {
		return -EIO;
	}
	*val = (uint16_t)(cmd & MDIO_DATA_MASK);
	return 0;
}

static int genet_mdio_write(mm_reg_t base, uint8_t phy, uint8_t reg, uint16_t val)
{
	uint32_t cmd = MDIO_WR | ((uint32_t)phy << MDIO_PMD_SHIFT) |
		       ((uint32_t)reg << MDIO_REG_SHIFT) | val;

	UMAC_WR(base, cmd,                    UMAC_MDIO_CMD);
	UMAC_WR(base, cmd | MDIO_START_BUSY,  UMAC_MDIO_CMD);
	return genet_mdio_wait(base);
}

/* ─────────────────────────────────────────────────────────────────────────
 * BCM54213PE shadow-register helpers — RGMII-RXID clock-delay configuration.
 * Mirrors bcm54xx_config_clock_delay() from U-Boot / Linux broadcom PHY drv.
 * ─────────────────────────────────────────────────────────────────────────
 */
#define BCM54XX_SHD_REG            0x1C
#define BCM54XX_SHD_VAL(n)         ((uint16_t)((n) << 10))
#define BCM54XX_SHD_WRITE          0x8000U
#define BCM54810_SHD_CLK_CTL       0x03
#define BCM54810_GTXCLK_EN         0x0200U

#define BCM54XX_AUX_CTL            0x18
#define BCM54XX_AUXCTL_MISC        0x7
#define BCM54XX_AUXCTL_READ_SEL(n) ((uint16_t)(0x7 | ((n) << 12)))
#define BCM54XX_AUXCTL_WREN        0x8000U
#define BCM54XX_AUXCTL_RGMII_SKEW  0x0100U

static void genet_phy_bcm54213_config(mm_reg_t base, uint8_t phy, bool rxid)
{
	uint16_t val, readback;

	/* AUX CTL shadow 7 — RX clock skew */
	genet_mdio_write(base, phy, BCM54XX_AUX_CTL,
			 BCM54XX_AUXCTL_READ_SEL(BCM54XX_AUXCTL_MISC));
	genet_mdio_read(base, phy, BCM54XX_AUX_CTL, &val);
	val |= BCM54XX_AUXCTL_WREN;
	if (rxid) {
		val |= BCM54XX_AUXCTL_RGMII_SKEW;
	} else {
		val &= ~BCM54XX_AUXCTL_RGMII_SKEW;
	}
	genet_mdio_write(base, phy, BCM54XX_AUX_CTL,
			 (uint16_t)(BCM54XX_AUXCTL_MISC | val));

	/* CLK CTL shadow 0x03 — TX clock delay */
	genet_mdio_write(base, phy, BCM54XX_SHD_REG,
			 BCM54XX_SHD_VAL(BCM54810_SHD_CLK_CTL));
	genet_mdio_read(base, phy, BCM54XX_SHD_REG, &val);
	val &= 0x03FFU;
	if (rxid) {
		val &= ~BCM54810_GTXCLK_EN;
	} else {
		val |=  BCM54810_GTXCLK_EN;
	}
	genet_mdio_write(base, phy, BCM54XX_SHD_REG,
			 (uint16_t)(BCM54XX_SHD_WRITE |
				    BCM54XX_SHD_VAL(BCM54810_SHD_CLK_CTL) | val));

	/* Read-back for confirmation */
	genet_mdio_write(base, phy, BCM54XX_SHD_REG,
			 BCM54XX_SHD_VAL(BCM54810_SHD_CLK_CTL));
	genet_mdio_read(base, phy, BCM54XX_SHD_REG, &readback);
	readback &= 0x03FFU;

	LOG_INF("PHY BCM54213: CLK_CTL=0x%03x (GTXCLK=%u SKEW=%u rxid=%d)",
		readback,
		!!(readback & BCM54810_GTXCLK_EN),
		!!(val & BCM54XX_AUXCTL_RGMII_SKEW),
		rxid);
}

static int genet_phy_init(mm_reg_t base, uint8_t phy, bool rxid)
{
	uint16_t v;
	int ret = genet_mdio_write(base, phy, MII_BMCR, BMCR_RESET);

	if (ret) {
		return ret;
	}
	for (int i = 0; i < 500; i++) {
		ret = genet_mdio_read(base, phy, MII_BMCR, &v);
		if (ret) {
			return ret;
		}
		if (!(v & BMCR_RESET)) {
			break;
		}
		k_sleep(K_MSEC(1));
	}

	genet_phy_bcm54213_config(base, phy, rxid);

	v = ADVERTISE_CSMA |
	    ADVERTISE_10HALF  | ADVERTISE_10FULL |
	    ADVERTISE_100HALF | ADVERTISE_100FULL |
	    ADVERTISE_PAUSE;
	genet_mdio_write(base, phy, MII_ADVERTISE, v);
	genet_mdio_write(base, phy, MII_CTRL1000,  ADVERTISE_1000FULL);

	return genet_mdio_write(base, phy, MII_BMCR,
				BMCR_ANENABLE | BMCR_ANRESTART);
}

static int genet_phy_wait_link(mm_reg_t base, uint8_t phy, uint32_t *speed)
{
	uint16_t bmsr, stat1000, lpa;

	LOG_INF("waiting for PHY link...");
	for (int i = 0; i < 5000; i++) {
		genet_mdio_read(base, phy, MII_BMSR, &bmsr);
		genet_mdio_read(base, phy, MII_BMSR, &bmsr);   /* latch-low */
		if ((bmsr & BMSR_LSTATUS) && (bmsr & BMSR_ANEGCOMPLETE)) {
			break;
		}
		k_sleep(K_MSEC(1));
		if (i == 4999) {
			LOG_WRN("PHY link timeout");
			return -ETIMEDOUT;
		}
	}

	genet_mdio_read(base, phy, MII_STAT1000, &stat1000);
	genet_mdio_read(base, phy, MII_LPA,      &lpa);

	if (stat1000 & LPA_1000FULL) {
		*speed = 1000;
	} else if (lpa & (ADVERTISE_100FULL | ADVERTISE_100HALF)) {
		*speed = 100;
	} else {
		*speed = 10;
	}
	LOG_INF("PHY link up: %u Mbps (BMSR=0x%04x LPA=0x%04x STAT1000=0x%04x)",
		*speed, bmsr, lpa, stat1000);
	return 0;
}

/* ─────────────────────────────────────────────────────────────────────────
 * DMA — bring rings into known state, then enable
 * ─────────────────────────────────────────────────────────────────────────
 */
static void genet_dma_disable(mm_reg_t base)
{
	TDMA_CTRL_WR(base, DMA_CTRL, TDMA_CTRL_RD(base, DMA_CTRL) & ~DMA_EN);
	for (uint32_t i = 0; i < DMA_TIMEOUT_USEC; i++) {
		if (TDMA_CTRL_RD(base, DMA_STATUS) & DMA_DISABLED) {
			break;
		}
		k_busy_wait(1);
	}

	RDMA_CTRL_WR(base, DMA_CTRL, RDMA_CTRL_RD(base, DMA_CTRL) & ~DMA_EN);
	for (uint32_t i = 0; i < DMA_TIMEOUT_USEC; i++) {
		if (RDMA_CTRL_RD(base, DMA_STATUS) & DMA_DISABLED) {
			break;
		}
		k_busy_wait(1);
	}

	TDMA_CTRL_WR(base, DMA_RING_CFG, 0);
	TDMA_CTRL_WR(base, DMA_CTRL,     0);
	RDMA_CTRL_WR(base, DMA_RING_CFG, 0);
	RDMA_CTRL_WR(base, DMA_CTRL,     0);

	INTRL2_0_WR(base, 0xFFFFFFFF, INTRL2_CPU_MASK_SET);
	INTRL2_1_WR(base, 0xFFFFFFFF, INTRL2_CPU_MASK_SET);
	INTRL2_0_WR(base, 0xFFFFFFFF, INTRL2_CPU_CLEAR);
	INTRL2_1_WR(base, 0xFFFFFFFF, INTRL2_CPU_CLEAR);
}

static void genet_init_rx_ring(mm_reg_t base, struct genet_data *d)
{
	const uint32_t ring = GENET_RX_RING;
	const uint32_t n    = GENET_NUM_RX_DESC;
	const uint32_t s    = GENET_RX_DESC_START;

	RDMA_RING_WR(base, ring, RING_HW_PTR_LO,        0);
	RDMA_RING_WR(base, ring, RING_HW_PTR_HI,        0);
	RDMA_RING_WR(base, ring, RING_SW_PTR_LO,        0);
	RDMA_RING_WR(base, ring, RING_SW_PTR_HI,        0);
	RDMA_RING_WR(base, ring, RING_HW_INDEX,         0);
	RDMA_RING_WR(base, ring, RING_SW_INDEX,         0);
	RDMA_RING_WR(base, ring, RING_START_ADDR_LO,    DESC_START_ADDR(s));
	RDMA_RING_WR(base, ring, RING_START_ADDR_HI,    0);
	RDMA_RING_WR(base, ring, RING_END_ADDR_LO,      DESC_END_ADDR(s, n));
	RDMA_RING_WR(base, ring, RING_END_ADDR_HI,      0);
	RDMA_RING_WR(base, ring, RING_BUF_SIZE,
		     RING_BUF_SIZE_VAL(n, GENET_RX_BUF_SIZE));
	RDMA_RING_WR(base, ring, RING_MBUF_DONE_THRESH, 1);
	RDMA_RING_WR(base, ring, RING_XON_XOFF,         RDMA_XON_XOFF_VAL);

	for (uint32_t i = 0; i < n; i++) {
		uintptr_t pa = (uintptr_t)genet_rx_buf[i];

		sys_cache_data_invd_range(genet_rx_buf[i],
			ROUND_UP(GENET_RX_BUF_SIZE, GENET_BUF_ALIGN));
		RDESC_WR(base, s + i, DMA_DESC_ADDRESS_LO, GENET_DMA_ADDR(pa));
		RDESC_WR(base, s + i, DMA_DESC_ADDRESS_HI, 0);
		RDESC_WR(base, s + i, DMA_DESC_LENGTH_STATUS,
			 ((uint32_t)GENET_RX_BUF_SIZE << DMA_BUFLENGTH_SHIFT) | DMA_OWN);
	}

	d->rx_cons     = 0;
	d->rx_desc_pos = 0;
}

static void genet_init_tx_ring(mm_reg_t base, struct genet_data *d)
{
	const uint32_t ring = GENET_TX_RING;
	const uint32_t end  = GENET_TOTAL_DESC * GENET_WORDS_PER_BD - 1U;

	TDMA_RING_WR(base, ring, RING_START_ADDR_LO,    0);
	TDMA_RING_WR(base, ring, RING_START_ADDR_HI,    0);
	TDMA_RING_WR(base, ring, RING_END_ADDR_LO,      end);
	TDMA_RING_WR(base, ring, RING_END_ADDR_HI,      0);
	TDMA_RING_WR(base, ring, RING_HW_PTR_LO,        0);
	TDMA_RING_WR(base, ring, RING_HW_PTR_HI,        0);
	TDMA_RING_WR(base, ring, RING_SW_PTR_LO,        0);
	TDMA_RING_WR(base, ring, RING_SW_PTR_HI,        0);
	TDMA_RING_WR(base, ring, RING_HW_INDEX,         0);   /* CONS=0 */
	TDMA_RING_WR(base, ring, RING_SW_INDEX,         0);   /* PROD=0 */
	TDMA_RING_WR(base, ring, RING_MBUF_DONE_THRESH, 1);
	TDMA_RING_WR(base, ring, RING_XON_XOFF,         0);
	TDMA_RING_WR(base, ring, RING_BUF_SIZE,
		     RING_BUF_SIZE_VAL(GENET_TOTAL_DESC, GENET_TX_BUF_SIZE));

	d->tx_prod       = 0;
	d->tx_write_ptr  = 0;
	d->tx_ring_start = 0;
	d->tx_ring_end   = end;
}

static void genet_dma_enable(mm_reg_t base)
{
	RDMA_CTRL_WR(base, DMA_SCB_BURST_SIZE, DMA_SCB_BURST_VAL);
	TDMA_CTRL_WR(base, DMA_SCB_BURST_SIZE, DMA_SCB_BURST_VAL);
	TDMA_CTRL_WR(base, DMA_ARB_CTRL,       DMA_ARBITER_SP);

	RDMA_CTRL_WR(base, DMA_RING_CFG, DMA_RING_CFG_EN(GENET_RX_RING));
	RDMA_CTRL_WR(base, DMA_CTRL,     DMA_EN | DMA_RING_EN(GENET_RX_RING));

	TDMA_CTRL_WR(base, DMA_RING_CFG, DMA_RING_CFG_EN(GENET_TX_RING));
	TDMA_CTRL_WR(base, DMA_CTRL,     DMA_EN | DMA_RING_EN(GENET_TX_RING));
}

/* ─────────────────────────────────────────────────────────────────────────
 * TX
 * ─────────────────────────────────────────────────────────────────────────
 */
static int genet_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct genet_data *d   = dev->data;
	mm_reg_t           b   = d->base;
	size_t             len = net_pkt_get_len(pkt);
	uint32_t           abs_desc, desc_idx, status;
	uint16_t           target_cons;

	if (len > GENET_TX_BUF_SIZE) {
		return -EMSGSIZE;
	}

	abs_desc = d->tx_write_ptr / GENET_WORDS_PER_BD;
	desc_idx = abs_desc % GENET_NUM_TX_DESC;

	net_pkt_cursor_init(pkt);
	if (net_pkt_read(pkt, genet_tx_buf[desc_idx], len)) {
		return -EIO;
	}

	sys_cache_data_flush_range(genet_tx_buf[desc_idx],
				   ROUND_UP(len, GENET_BUF_ALIGN));
	barrier_dmem_fence_full();

	status = ((uint32_t)len << DMA_BUFLENGTH_SHIFT) |
		 (0x3F << DMA_TX_QTAG_SHIFT) |
		 DMA_SOP | DMA_EOP | DMA_TX_APPEND_CRC;

	TDESC_WR(b, abs_desc, DMA_DESC_ADDRESS_LO,
		 GENET_DMA_ADDR(genet_tx_buf[desc_idx]));
	TDESC_WR(b, abs_desc, DMA_DESC_ADDRESS_HI,    0);
	TDESC_WR(b, abs_desc, DMA_DESC_LENGTH_STATUS, status);

	d->tx_write_ptr += GENET_WORDS_PER_BD;
	if (d->tx_write_ptr > d->tx_ring_end) {
		d->tx_write_ptr = d->tx_ring_start;
	}

	d->tx_prod   = (d->tx_prod + 1) & DMA_P_INDEX_MASK;
	target_cons  = d->tx_prod;
	TDMA_RING_WR(b, GENET_TX_RING, RING_SW_INDEX, d->tx_prod);

	for (int i = 0; i < 50000; i++) {
		uint16_t cons = (uint16_t)(TDMA_RING_RD(b, GENET_TX_RING,
							RING_HW_INDEX) &
					   DMA_C_INDEX_MASK);
		if (cons == target_cons) {
			return 0;
		}
		k_busy_wait(10);
	}
	LOG_WRN("TX timeout prod=%u", d->tx_prod);
	return 0;
}

/* ─────────────────────────────────────────────────────────────────────────
 * RX
 * ─────────────────────────────────────────────────────────────────────────
 */
static void genet_rx_work(struct k_work *work)
{
	struct genet_data *d = CONTAINER_OF(work, struct genet_data, rx_work);
	mm_reg_t           b = d->base;
	uint16_t prod = (uint16_t)(RDMA_RING_RD(b, GENET_RX_RING, RING_HW_INDEX)
				   & DMA_P_INDEX_MASK);
	uint16_t pending = (prod - d->rx_cons) & DMA_P_INDEX_MASK;

	while (pending--) {
		uint32_t idx = d->rx_desc_pos % GENET_NUM_RX_DESC;
		uint32_t st;
		uint16_t plen;
		struct net_pkt *pkt;

		barrier_dmem_fence_full();
		sys_cache_data_invd_range(genet_rx_buf[idx],
			ROUND_UP(GENET_RX_BUF_SIZE, GENET_BUF_ALIGN));

		st = RDESC_RD(b, GENET_RX_DESC_START + idx, DMA_DESC_LENGTH_STATUS);
		if (st & DMA_RX_ERROR_MASK) {
			goto next;
		}
		if (!(st & DMA_EOP) || !(st & DMA_SOP)) {
			goto next;
		}
		plen = (uint16_t)((st >> DMA_BUFLENGTH_SHIFT) & DMA_BUFLENGTH_MASK);
		if (plen <= GENET_RX_DATA_OFFSET) {
			goto next;
		}
		plen -= GENET_RX_DATA_OFFSET;

		pkt = net_pkt_rx_alloc_with_buffer(d->iface, plen, AF_UNSPEC, 0,
						   K_NO_WAIT);
		if (!pkt) {
			goto next;
		}
		if (net_pkt_write(pkt, genet_rx_buf[idx] + GENET_RX_DATA_OFFSET,
				  plen)) {
			net_pkt_unref(pkt);
			goto next;
		}
		net_pkt_cursor_init(pkt);
		if (net_recv_data(d->iface, pkt) < 0) {
			net_pkt_unref(pkt);
		}
next:
		RDESC_WR(b, GENET_RX_DESC_START + idx, DMA_DESC_LENGTH_STATUS,
			 ((uint32_t)GENET_RX_BUF_SIZE << DMA_BUFLENGTH_SHIFT) | DMA_OWN);
		d->rx_desc_pos++;
		d->rx_cons = (d->rx_cons + 1) & DMA_P_INDEX_MASK;
	}

	RDMA_RING_WR(b, GENET_RX_RING, RING_SW_INDEX, d->rx_cons);
	INTRL2_0_WR(b, UMAC_IRQ_RXDMA_MBDONE, INTRL2_CPU_MASK_CLEAR);
}

/* ─────────────────────────────────────────────────────────────────────────
 * PHY polling (external PHY: link-up/down interrupts don't fire)
 * ─────────────────────────────────────────────────────────────────────────
 */
static void genet_phy_poll(struct k_work *work)
{
	struct k_work_delayable *dw = k_work_delayable_from_work(work);
	struct genet_data *d = CONTAINER_OF(dw, struct genet_data, phy_poll_work);
	uint16_t bmsr;

	(void)genet_mdio_read(d->base, d->phy_addr, MII_BMSR, &bmsr);
	if (genet_mdio_read(d->base, d->phy_addr, MII_BMSR, &bmsr) == 0) {
		bool up = !!(bmsr & BMSR_LSTATUS);

		if (up != d->link_up) {
			d->link_up = up;
			LOG_INF("PHY link %s", up ? "UP" : "DOWN");
			if (d->iface) {
				if (up) {
					uint32_t mbps;

					if (genet_phy_wait_link(d->base,
								d->phy_addr,
								&mbps) == 0) {
						genet_set_speed(d->base, mbps);
					}
					net_eth_carrier_on(d->iface);
				} else {
					net_eth_carrier_off(d->iface);
				}
			}
		}
	}
	k_work_schedule(dw, K_MSEC(GENET_PHY_POLL_INTERVAL));
}

/* ─────────────────────────────────────────────────────────────────────────
 * IRQ handlers
 * ─────────────────────────────────────────────────────────────────────────
 */
static void genet_irq0_handler(const struct device *dev)
{
	struct genet_data *d = dev->data;
	mm_reg_t           b = d->base;
	uint32_t stat;

	barrier_dmem_fence_full();
	stat = INTRL2_0_RD(b, INTRL2_CPU_STAT);
	INTRL2_0_WR(b, stat, INTRL2_CPU_CLEAR);

	if (stat & UMAC_IRQ_RXDMA_MBDONE) {
		INTRL2_0_WR(b, UMAC_IRQ_RXDMA_MBDONE, INTRL2_CPU_MASK_SET);
		k_work_submit(&d->rx_work);
	}
	/* TX completion polled in genet_tx; LINK_UP/DOWN don't fire on
	 * external PHY (handled by genet_phy_poll).
	 */
}

static void genet_irq1_handler(const struct device *dev)
{
	mm_reg_t b = ((struct genet_data *)dev->data)->base;
	uint32_t stat = INTRL2_1_RD(b, INTRL2_CPU_STAT);

	INTRL2_1_WR(b, stat, INTRL2_CPU_CLEAR);
}

/* ─────────────────────────────────────────────────────────────────────────
 * Net device API
 * ─────────────────────────────────────────────────────────────────────────
 */
static void genet_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct genet_data   *d   = dev->data;

	d->iface = iface;
	net_if_set_link_addr(iface, d->mac, sizeof(d->mac), NET_LINK_ETHERNET);
	ethernet_init(iface);

	if (d->link_up) {
		net_eth_carrier_on(iface);
	} else {
		net_eth_carrier_off(iface);
	}
	LOG_INF("MAC: %02x:%02x:%02x:%02x:%02x:%02x",
		d->mac[0], d->mac[1], d->mac[2],
		d->mac[3], d->mac[4], d->mac[5]);
}

static int genet_start(const struct device *dev)
{
	struct genet_data *d = dev->data;
	mm_reg_t b = d->base;

	UMAC_WR(b, UMAC_RD(b, UMAC_CMD) | CMD_TX_EN | CMD_RX_EN, UMAC_CMD);
	INTRL2_0_WR(b, UMAC_IRQ_RXDMA_MBDONE | UMAC_IRQ_TXDMA_MBDONE,
		    INTRL2_CPU_MASK_CLEAR);
	return 0;
}

static int genet_stop(const struct device *dev)
{
	struct genet_data *d = dev->data;
	mm_reg_t b = d->base;

	UMAC_WR(b, 0, UMAC_CMD);
	INTRL2_0_WR(b, 0xFFFFFFFF, INTRL2_CPU_MASK_SET);
	return 0;
}

static enum ethernet_hw_caps genet_caps(const struct device *dev)
{
	ARG_UNUSED(dev);
	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE | ETHERNET_LINK_1000BASE;
}

static const struct ethernet_api genet_api = {
	.iface_api.init   = genet_iface_init,
	.start            = genet_start,
	.stop             = genet_stop,
	.get_capabilities = genet_caps,
	.send             = genet_tx,
};

/* ─────────────────────────────────────────────────────────────────────────
 * Driver init — runs at CONFIG_ETH_INIT_PRIORITY
 * ─────────────────────────────────────────────────────────────────────────
 */
static int genet_init(const struct device *dev)
{
	struct genet_data        *d = dev->data;
	const struct genet_config *c = dev->config;
	uint32_t mbps;
	mm_reg_t b;
	int ret;

	d->dev            = dev;
	d->phy_addr       = c->phy_addr;
	d->phy_rgmii_rxid = c->phy_rgmii_rxid;
	k_work_init(&d->rx_work, genet_rx_work);
	k_work_init_delayable(&d->phy_poll_work, genet_phy_poll);

	device_map(&d->base, c->phys_base, c->phys_size, K_MEM_CACHE_NONE);
	b = d->base;

	LOG_DBG("GENET v5 phys=0x%08lx virt=0x%08lx",
		(unsigned long)c->phys_base, (unsigned long)b);

	/* Power up UMAC before touching it (clears VPU power-down state). */
	ret = genet_power_up(b);
	if (ret) {
		return ret;
	}

	/* MAC-wide reset via SYS block; clears any state from VPU's GENET use. */
	genet_sys_reset(b);

	genet_dma_disable(b);
	genet_umac_init(b);

	if (c->has_local_mac) {
		memcpy(d->mac, c->local_mac, 6);
	} else {
		sys_rand_get(d->mac, 6);
		d->mac[0] = (d->mac[0] & ~0x01) | 0x02;
	}
	genet_set_mac(b, d->mac);
	SYS_WR(b, PORT_MODE_EXT_GPHY, SYS_PORT_CTRL);

	genet_ext_init(b, c->phy_rgmii_rxid);

	ret = genet_phy_init(b, c->phy_addr, c->phy_rgmii_rxid);
	if (ret) {
		LOG_ERR("PHY init failed: %d", ret);
		return ret;
	}

	genet_init_rx_ring(b, d);
	genet_init_tx_ring(b, d);
	genet_dma_enable(b);

	INTRL2_0_WR(b, 0xFFFFFFFF, INTRL2_CPU_MASK_SET);
	INTRL2_0_WR(b, 0xFFFFFFFF, INTRL2_CPU_CLEAR);
	INTRL2_0_WR(b, UMAC_IRQ_RXDMA_MBDONE | UMAC_IRQ_TXDMA_MBDONE,
		    INTRL2_CPU_MASK_CLEAR);
	INTRL2_1_WR(b, 0xFFFFFFFF, INTRL2_CPU_MASK_SET);
	INTRL2_1_WR(b, 0xFFFFFFFF, INTRL2_CPU_CLEAR);
	c->irq0_config();
	c->irq1_config();

	UMAC_WR(b, CMD_TX_EN | CMD_RX_EN, UMAC_CMD);

	ret = genet_phy_wait_link(b, c->phy_addr, &mbps);
	if (ret == 0) {
		genet_set_speed(b, mbps);
		d->link_up = true;
	} else {
		LOG_WRN("no PHY link at boot");
	}

	k_work_schedule(&d->phy_poll_work, K_MSEC(GENET_PHY_POLL_INTERVAL));
	LOG_INF("GENET ready");
	return 0;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Device-tree instantiation
 * ─────────────────────────────────────────────────────────────────────────
 */
#define GENET_DEFINE(n)                                                         \
static const uint8_t genet_mac_##n[] =                                          \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, local_mac_address),                \
		    (DT_INST_PROP(n, local_mac_address)),                       \
		    ({0, 0, 0, 0, 0, 0}));                                      \
static void genet_irq0_config_##n(void)                                         \
{                                                                               \
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(n, 0),                                  \
		    DT_INST_IRQ_BY_IDX(n, 0, priority),                         \
		    genet_irq0_handler, DEVICE_DT_INST_GET(n), 0);              \
	irq_enable(DT_INST_IRQN_BY_IDX(n, 0));                                  \
}                                                                               \
static void genet_irq1_config_##n(void)                                         \
{                                                                               \
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(n, 1),                                  \
		    DT_INST_IRQ_BY_IDX(n, 1, priority),                         \
		    genet_irq1_handler, DEVICE_DT_INST_GET(n), 0);              \
	irq_enable(DT_INST_IRQN_BY_IDX(n, 1));                                  \
}                                                                               \
static struct genet_data genet_data_##n;                                        \
static const struct genet_config genet_config_##n = {                           \
	.phys_base      = DT_INST_REG_ADDR(n),                                  \
	.phys_size      = DT_INST_REG_SIZE(n),                                  \
	.irq0_config    = genet_irq0_config_##n,                                \
	.irq1_config    = genet_irq1_config_##n,                                \
	.phy_addr       = DT_INST_PROP(n, phy_addr),                            \
	.phy_rgmii_rxid = (DT_INST_PROP_OR(n, rx_internal_delay_ps, 0) > 0),    \
	.has_local_mac  = DT_INST_NODE_HAS_PROP(n, local_mac_address),          \
	.local_mac      = genet_mac_##n,                                        \
};                                                                              \
ETH_NET_DEVICE_DT_INST_DEFINE(n, genet_init, NULL,                              \
			      &genet_data_##n, &genet_config_##n,               \
			      CONFIG_ETH_INIT_PRIORITY,                         \
			      &genet_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(GENET_DEFINE)
