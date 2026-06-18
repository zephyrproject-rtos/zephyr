/*
 * Copyright (C) 2026, Savoir-faire Linux
 * Author: Paolo Wattebled <paolo.wattebled@savoirfairelinux.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx8mp_media_blk_ctrl

#define LOG_LEVEL CONFIG_POWER_DOMAIN_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_imx8mp_media_blk_ctrl);

#include <stdint.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/* MEDIA_BLK_CTRL register offsets (i.MX8MP Reference Manual) */
#define MBC_SFT_RSTN           0x00U /* Software Reset — active-low: 0=assert, 1=release */
#define MBC_CLK_EN             0x04U /* Clock Enable  — active-high: 1=enable */
#define MBC_MIPI_RESET_DIV     0x08U /* MIPI PHY resets, bits 16/17/29/30 active-low */
#define MBC_LCDIF_ARCACHE_CTRL 0x4CU /* LCDIF AXI cache control + hurry priority */

/*
 * LCDIF_ARCACHE_CTRL (0x4C): hurry priority for LCDIF1 AXI read initiator.
 * Bits 12:10 = LCDIF1 hurry level (i.MX8MP RM §13.2.2, §13.2.3).
 * Urges the NoC to raise bus priority for pending LCDIF1 reads. Effective when
 * the NIC-301 honours the LCDIF urgency input ("noc" reg present, extctrl=1).
 */
#define MBC_LCDIF1_HURRY_MASK GENMASK(12, 10)

/* Bus sub-domain: bit 8 of SFT_RSTN and CLK_EN */
#define MBC_BUS_DOMAIN_MASK BIT(8)

/*
 * NIC-301 Media NOC QoS — LCDIF1 AXI initiator offsets from the "noc" reg.
 *
 * The NIC-301 GPV at 0x32700000 is the AXI interconnect arbiter for the media
 * subsystem. Its defaults are normally programmed by the boot firmware (Arm
 * TF-A imx8mp gpc.c / Linux imx8m-blk-ctrl.c). When the M-core brings up the
 * display before — or independently of — that firmware path, the GPV stays at
 * its power-on default (lowest priority); LCDIF1 then starves under heavy DDR
 * load (e.g. OS userspace startup), seen as split-screen tearing. This driver
 * programs the LCDIF1 slots itself so the priority matches the BSP value.
 *
 * Each initiator slot has three QoS registers at offsets +0x08, +0x0C, +0x18:
 *   priority — 0x80HHLLLL: bit31=urgency_en, HH=high_prio, LL=low_prio (0-7)
 *   mode     — 0=fixed priority (regulator mode would additionally require the
 *              bandwidth/saturation registers at +0x10/+0x14, not used here)
 *   extctrl  — 1=enable urgency override from the AXI master ARURGENT/AWURGENT
 *
 * 0x80000202 / mode 0 / extctrl 1 is the canonical i.MX8MP LCDIF setting written
 * by both Arm TF-A (plat/imx/imx8m/imx8mp/gpc.c) and Linux imx8m-blk-ctrl.c.
 * Boards with tighter display-under-load margins may raise the priority through DT.
 */
#define MBC_NOC_LCDIF1_RD_OFF 0x0980U
#define MBC_NOC_LCDIF1_WR_OFF 0x0A00U
#define MBC_NOC_QOS_PRIO_OFF  0x0008U
#define MBC_NOC_QOS_MODE_OFF  0x000CU
#define MBC_NOC_QOS_EXT_OFF   0x0018U
#define MBC_NOC_LCDIF_MODE    0U
#define MBC_NOC_LCDIF_EXTCTRL 1U

/*
 * ~5 µs propagation delay after enabling clocks, before deasserting reset.
 * At PRE_KERNEL_1 the kernel timer is not yet running, so k_busy_wait() is not
 * usable; spin on a fixed arch_nop() loop instead. Each iteration costs at least
 * one CPU cycle, so 8000 iterations are >= 8000 cycles: >= 5 µs even at an
 * implausibly high 1.6 GHz, and >= 10 µs on the i.MX8MP M7 (<= 800 MHz). Loop
 * and call overhead only lengthen the delay, so the 5 µs minimum always holds.
 */
#define MBC_RESET_DELAY_NOPS 8000U

static inline void mbc_reset_delay(void)
{
	for (uint32_t i = MBC_RESET_DELAY_NOPS; i > 0U; i--) {
		arch_nop();
	}
}

static struct k_spinlock mbc_lock;

/* Serialize RMW of the parent registers shared by all sub-domains. */
static inline void mbc_rmw(mm_reg_t base, uint32_t off, uint32_t mask, uint32_t val)
{
	k_spinlock_key_t key = k_spin_lock(&mbc_lock);
	uint32_t r = sys_read32(base + off);

	r = (r & ~mask) | (val & mask);
	sys_write32(r, base + off);
	k_spin_unlock(&mbc_lock, key);
}

static inline void mbc_reset_assert(mm_reg_t base, uint32_t mask)
{
	mbc_rmw(base, MBC_SFT_RSTN, mask, 0U);
}

static inline void mbc_reset_deassert(mm_reg_t base, uint32_t mask)
{
	mbc_rmw(base, MBC_SFT_RSTN, mask, mask);
}

static inline void mbc_clk_enable(mm_reg_t base, uint32_t mask)
{
	mbc_rmw(base, MBC_CLK_EN, mask, mask);
}

static inline void mbc_clk_disable(mm_reg_t base, uint32_t mask)
{
	mbc_rmw(base, MBC_CLK_EN, mask, 0U);
}

static inline void mbc_phy_reset_deassert(mm_reg_t base, uint32_t mask)
{
	mbc_rmw(base, MBC_MIPI_RESET_DIV, mask, mask);
}

static inline void mbc_phy_reset_assert(mm_reg_t base, uint32_t mask)
{
	mbc_rmw(base, MBC_MIPI_RESET_DIV, mask, 0U);
}

/* Per-sub-domain config (built from DT at compile time) */
struct mbc_domain_config {
	mm_reg_t base;         /**< Parent MMIO base address */
	uint32_t rst_mask;     /**< SFT_RSTN bitmask (nxp,rst-mask) */
	uint32_t clk_mask;     /**< CLK_EN bitmask (nxp,clk-mask) */
	uint32_t phy_rst_mask; /**< MIPI_RESET_DIV bitmask, or 0 if absent */
};

/* Parent config (bus bring-up + QoS) */
struct mbc_parent_config {
	mm_reg_t base;
	mm_reg_t noc_base;       /**< NIC-301 GPV base address */
	uint32_t lcdif_priority; /**< nxp,lcdif-noc-priority (default 0x80000202) */
	uint32_t lcdif_rd_hurry; /**< nxp,lcdif-rd-hurry (default 7) */
};

/* Power-on sequence: assert reset (+ PHY), enable clocks, settle, deassert. */
static int mbc_domain_power_on(const struct mbc_domain_config *cfg)
{
	mbc_reset_assert(cfg->base, cfg->rst_mask);
	if (cfg->phy_rst_mask != 0U) {
		mbc_phy_reset_assert(cfg->base, cfg->phy_rst_mask);
	}

	mbc_clk_enable(cfg->base, cfg->clk_mask);
	mbc_reset_delay();
	mbc_reset_deassert(cfg->base, cfg->rst_mask);

	if (cfg->phy_rst_mask != 0U) {
		mbc_phy_reset_deassert(cfg->base, cfg->phy_rst_mask);
	}

	return 0;
}

static int mbc_domain_resume(const struct device *dev)
{
	int ret = mbc_domain_power_on(dev->config);

#ifdef CONFIG_PM_DEVICE_POWER_DOMAIN
	if (ret == 0) {
		pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_ON, NULL);
	}
#endif

	return ret;
}

#ifdef CONFIG_PM_DEVICE
/* Reverse of mbc_domain_power_on(). */
static int mbc_domain_power_off(const struct mbc_domain_config *cfg)
{
	if (cfg->phy_rst_mask != 0U) {
		mbc_phy_reset_assert(cfg->base, cfg->phy_rst_mask);
	}

	mbc_reset_assert(cfg->base, cfg->rst_mask);
	mbc_clk_disable(cfg->base, cfg->clk_mask);

	return 0;
}

static int mbc_domain_suspend(const struct device *dev)
{
#ifdef CONFIG_PM_DEVICE_POWER_DOMAIN
	pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_OFF, NULL);
#endif

	return mbc_domain_power_off(dev->config);
}

static int mbc_domain_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return mbc_domain_resume(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		return mbc_domain_suspend(dev);
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;
	default:
		return -ENOTSUP;
	}
}
#else
static int mbc_domain_pm_action(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return mbc_domain_resume(dev);
	case PM_DEVICE_ACTION_SUSPEND:
	default:
		return -ENOTSUP;
	}
}
#endif

static int mbc_domain_init(const struct device *dev)
{
	return pm_device_driver_init(dev, mbc_domain_pm_action);
}

/* Program the LCDIF1 read (0x980) and write (0xA00) NIC-301 slots; see MBC_NOC_* above. */
static void mbc_set_noc_qos(const struct mbc_parent_config *cfg)
{
	mm_reg_t noc_base = cfg->noc_base;
	mm_reg_t rd = noc_base + MBC_NOC_LCDIF1_RD_OFF;
	mm_reg_t wr = noc_base + MBC_NOC_LCDIF1_WR_OFF;

	sys_write32(cfg->lcdif_priority, rd + MBC_NOC_QOS_PRIO_OFF);
	sys_write32(MBC_NOC_LCDIF_MODE, rd + MBC_NOC_QOS_MODE_OFF);
	sys_write32(MBC_NOC_LCDIF_EXTCTRL, rd + MBC_NOC_QOS_EXT_OFF);

	sys_write32(cfg->lcdif_priority, wr + MBC_NOC_QOS_PRIO_OFF);
	sys_write32(MBC_NOC_LCDIF_MODE, wr + MBC_NOC_QOS_MODE_OFF);
	sys_write32(MBC_NOC_LCDIF_EXTCTRL, wr + MBC_NOC_QOS_EXT_OFF);
}

/* Bring up the always-on bus sub-domain, then program NoC QoS + LCDIF1 hurry. */
static int mbc_parent_init(const struct device *dev)
{
	const struct mbc_parent_config *cfg = dev->config;
	mm_reg_t base = cfg->base;

	mbc_reset_assert(base, MBC_BUS_DOMAIN_MASK);
	mbc_clk_enable(base, MBC_BUS_DOMAIN_MASK);
	mbc_reset_delay();
	mbc_reset_deassert(base, MBC_BUS_DOMAIN_MASK);

	mbc_set_noc_qos(cfg);

	mbc_rmw(base, MBC_LCDIF_ARCACHE_CTRL, MBC_LCDIF1_HURRY_MASK, cfg->lcdif_rd_hurry << 10U);

	LOG_DBG("parent init: bus up, noc set, lcdif-rd-hurry=%u", cfg->lcdif_rd_hurry);

	return 0;
}

#define MBC_DOMAIN_HAS_PHY_RST(node_id) DT_NODE_HAS_PROP(node_id, nxp_mipi_phy_rst_mask)

#define MBC_INST_NOC_BASE(inst) DT_INST_REG_ADDR_BY_NAME(inst, noc)

#define MBC_INST_VALIDATE(inst)                                                                    \
	BUILD_ASSERT(DT_INST_PROP(inst, nxp_lcdif_rd_hurry) >= 0 &&                                \
			     DT_INST_PROP(inst, nxp_lcdif_rd_hurry) <= 7,                          \
		     "nxp,lcdif-rd-hurry must be in range 0..7")

/* Instantiate one child sub-domain device from a DT child node. */
#define MBC_DOMAIN_DEFINE(child_node_id)                                                           \
	static const struct mbc_domain_config mbc_domain_cfg_##child_node_id = {                   \
		.base = DT_REG_ADDR(DT_PARENT(child_node_id)),                                     \
		.rst_mask = DT_PROP(child_node_id, nxp_rst_mask),                                  \
		.clk_mask = DT_PROP(child_node_id, nxp_clk_mask),                                  \
		.phy_rst_mask = COND_CODE_1(MBC_DOMAIN_HAS_PHY_RST(child_node_id),            \
			(DT_PROP(child_node_id, nxp_mipi_phy_rst_mask)), (0U)),       \
	};                                                                                         \
	PM_DEVICE_DT_DEFINE(child_node_id, mbc_domain_pm_action);                                  \
	DEVICE_DT_DEFINE(child_node_id, mbc_domain_init, PM_DEVICE_DT_GET(child_node_id), NULL,    \
			 &mbc_domain_cfg_##child_node_id, PRE_KERNEL_1,                            \
			 CONFIG_POWER_DOMAIN_NXP_IMX8MP_MEDIA_BLK_CTRL_INIT_PRIORITY, NULL);

/* Instantiate parent device + all child sub-domains for one DT instance. */
#define MBC_PARENT_DEFINE(inst)                                                                    \
	MBC_INST_VALIDATE(inst);                                                                   \
	static const struct mbc_parent_config mbc_parent_cfg_##inst = {                            \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.noc_base = MBC_INST_NOC_BASE(inst),                                               \
		.lcdif_priority = DT_INST_PROP(inst, nxp_lcdif_noc_priority),                      \
		.lcdif_rd_hurry = DT_INST_PROP(inst, nxp_lcdif_rd_hurry),                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(                                                                     \
		inst, mbc_parent_init, NULL, NULL, &mbc_parent_cfg_##inst, PRE_KERNEL_1,           \
		CONFIG_POWER_DOMAIN_NXP_IMX8MP_MEDIA_BLK_CTRL_PARENT_INIT_PRIORITY, NULL);         \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, MBC_DOMAIN_DEFINE)

DT_INST_FOREACH_STATUS_OKAY(MBC_PARENT_DEFINE)
