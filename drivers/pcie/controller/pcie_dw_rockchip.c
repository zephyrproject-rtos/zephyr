/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rockchip_rk3588_pcie

#include <errno.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pcie/controller.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/reset.h>
#ifdef CONFIG_GIC_V3_ITS
#include <zephyr/drivers/interrupt_controller/gicv3_its.h>
#endif

#include "pcie_dw_common.h"

LOG_MODULE_REGISTER(pcie_dw_rockchip, CONFIG_PCIE_LOG_LEVEL);

#define DEV_CFG(dev)  ((const struct pcie_dw_rockchip_config *)(dev)->config)
#define DEV_DATA(dev) ((struct pcie_dw_rockchip_data *)(dev)->data)

/* RK3588 PCIe client APB registers. */
#define RK_PCIE_CLIENT_GENERAL_CON      0x000
#define RK_PCIE_CLIENT_INTR_MASK_LEGACY 0x01c
#define RK_PCIE_CLIENT_POWER_CON        0x02c
#define RK_PCIE_CLIENT_HOT_RESET_CTRL   0x180
#define RK_PCIE_CLIENT_LTSSM_STATUS     0x300
#define RK_PCIE_LTSSM_LINK_UP           (BIT(16) | BIT(17))

/*
 * The upper half of each RK3588 PCIe client register field is a write-enable
 * mask for the lower half.
 */
#define RK_PCIE_CLIENT_RC_MODE            0x00f00040U
#define RK_PCIE_CLIENT_LTSSM_DISABLE      0x000c0008U
#define RK_PCIE_CLIENT_LTSSM_ENABLE       0x000c000cU
/* CLKREQ# is not wired: pull it down and report the clock request as not ready. */
#define RK_PCIE_CLIENT_CLKREQ_ASSERT      0x30011000U
/* Enable the LTSSM control mode (LTSSM_ENABLE_ENHANCE). */
#define RK_PCIE_CLIENT_LTSSM_ENHANCE      0x00100010U
/* Unmask all four legacy INTx status bits (write-enable mask in the upper half). */
#define RK_PCIE_CLIENT_INTR_LEGACY_UNMASK 0xffff0000U

/*
 * RK3588 SoC register windows used for the pcie2x1l2 bring-up. PERST# is
 * GPIO3 PB0, muxed through the IOC and driven through the GPIO3 bank; the
 * ComboPHY0 and PIPE PHY GRF windows configure the PHY. These registers are
 * part of the RK3588 register map and are programmed directly by this driver.
 */
#define RK3588_IOC_GPIO3B_IOMUX_L     0x8068
#define RK3588_GPIO_SWPORT_DR_L       0x00
#define RK3588_GPIO_SWPORT_DDR_L      0x08
#define RK3588_GPIO3_PB0              BIT(8)
#define RK3588_GPIO3_PB0_WRITE_ENABLE BIT(24)

/* RK3588 ComboPHY0 PIPE PHY GRF and analog registers. */
#define RK3588_PIPE_PHY_GRF_CON0 0x00
#define RK3588_PIPE_PHY_GRF_CON1 0x04
#define RK3588_PIPE_PHY_GRF_CON2 0x08
#define RK3588_PIPE_PHY_GRF_CON3 0x0c

#define RK3588_COMBPHY_SSC           0x7c
#define RK3588_COMBPHY_GATE_TX_PCK   0x74
#define RK3588_COMBPHY_PLL_KVCO      0x80
#define RK3588_COMBPHY_RX_TRIM       0x6c
#define RK3588_COMBPHY_SU_TRIM0      0x28
#define RK3588_COMBPHY_SU_TRIM1      0x2c
#define RK3588_COMBPHY_SU_TRIM2      0x30
#define RK3588_COMBPHY_SU_TRIM3      0x34
#define RK3588_COMBPHY_FORCE_DET_OUT 0x64

#define RK3588_PCIE_DEFAULT_PERST_MS        200U
#define RK3588_PCIE_MAX_PERST_MS            1000U
#define RK3588_PCIE_DEFAULT_LINK_TIMEOUT_MS 2000U
#define RK3588_PCIE_MAX_LINK_TIMEOUT_MS     60000U
#define RK3588_PCIE_LINK_POLL_US            20000U
#define RK3588_PCIE_LINK_STABLE_US          50000U
#define RK3588_PCIE2X1L2_APB_PHYS           0xfe190000U

enum rk3588_pcie_reset {
	RK3588_PCIE_RESET_POWER,
	RK3588_PCIE_RESET_PIPE,
	RK3588_PCIE_RESET_PHY,
	RK3588_PCIE_RESET_PHY_APB,
	RK3588_PCIE_RESET_COUNT,
};

struct rk3588_pcie_clock {
	const struct device *dev;
	uint16_t id;
};

struct pcie_dw_rockchip_config {
	struct pcie_dw_config dw;
	DEVICE_MMIO_NAMED_ROM(pcie_apb);
	const struct rk3588_pcie_clock *clocks;
	size_t clock_count;
	struct rk3588_pcie_clock top_clock;
	struct rk3588_pcie_clock gpio_clock;
	struct rk3588_pcie_clock ref_clock;
	struct reset_dt_spec resets[RK3588_PCIE_RESET_COUNT];
	DEVICE_MMIO_NAMED_ROM(pipe_phy_grf);
	DEVICE_MMIO_NAMED_ROM(combphy);
	DEVICE_MMIO_NAMED_ROM(gpio);
	DEVICE_MMIO_NAMED_ROM(ioc);
	uint32_t perst_inactive_ms;
	uint32_t link_timeout_ms;
	bool firmware_initialized;
};

struct pcie_dw_rockchip_data {
	struct pcie_dw_data dw;
	DEVICE_MMIO_NAMED_RAM(pcie_apb);
	DEVICE_MMIO_NAMED_RAM(pipe_phy_grf);
	DEVICE_MMIO_NAMED_RAM(combphy);
	DEVICE_MMIO_NAMED_RAM(gpio);
	DEVICE_MMIO_NAMED_RAM(ioc);
};

static void rk3588_hiword_update(mm_reg_t base, uint32_t offset, uint32_t mask, uint32_t value)
{
	sys_write32((mask << 16) | (value & mask), base + offset);
}

static int rk3588_clock_enable(const struct rk3588_pcie_clock *clk)
{
	if (!device_is_ready(clk->dev)) {
		return -ENODEV;
	}
	return clock_control_on(clk->dev, (clock_control_subsys_t)(uintptr_t)clk->id);
}

static int rk3588_enable_clocks(const struct pcie_dw_rockchip_config *config)
{
	int ret;

	for (size_t i = 0; i < config->clock_count; i++) {
		ret = rk3588_clock_enable(&config->clocks[i]);
		if (ret != 0) {
			return ret;
		}
	}
	/* The ComboPHY needs a 100-MHz PIPE reference clock. */
	return clock_control_set_rate(config->ref_clock.dev,
				      (clock_control_subsys_t)(uintptr_t)config->ref_clock.id,
				      (clock_control_subsys_rate_t)(uintptr_t)MHZ(100));
}

static void rk3588_perst_hold(const struct device *dev)
{
	mm_reg_t ioc = DEVICE_MMIO_NAMED_GET(dev, ioc);
	mm_reg_t gpio = DEVICE_MMIO_NAMED_GET(dev, gpio);

	/* Select GPIO function for GPIO3 PB0 and drive PERST# low without a high glitch. */
	rk3588_hiword_update(ioc, RK3588_IOC_GPIO3B_IOMUX_L, GENMASK(3, 0), 0U);
	sys_write32(RK3588_GPIO3_PB0_WRITE_ENABLE, gpio + RK3588_GPIO_SWPORT_DR_L);
	sys_write32(RK3588_GPIO3_PB0_WRITE_ENABLE | RK3588_GPIO3_PB0,
		    gpio + RK3588_GPIO_SWPORT_DDR_L);
}

static void rk3588_perst_release(const struct device *dev)
{
	mm_reg_t gpio = DEVICE_MMIO_NAMED_GET(dev, gpio);

	sys_write32(RK3588_GPIO3_PB0_WRITE_ENABLE | RK3588_GPIO3_PB0,
		    gpio + RK3588_GPIO_SWPORT_DR_L);
}

static void rk3588_combphy_update(mm_reg_t combphy, uint32_t offset, uint32_t mask, uint32_t value)
{
	uint32_t reg = sys_read32(combphy + offset);

	reg = (reg & ~mask) | (value & mask);
	sys_write32(reg, combphy + offset);
}

static void rk3588_combphy_init(const struct device *dev)
{
	mm_reg_t pipe_phy_grf = DEVICE_MMIO_NAMED_GET(dev, pipe_phy_grf);
	mm_reg_t combphy = DEVICE_MMIO_NAMED_GET(dev, combphy);

	/* Select PCIe mode, internal 100-MHz reference clock, and PCIe terminations. */
	rk3588_hiword_update(pipe_phy_grf, RK3588_PIPE_PHY_GRF_CON0, GENMASK(15, 0), 0x1000U);
	rk3588_hiword_update(pipe_phy_grf, RK3588_PIPE_PHY_GRF_CON1, GENMASK(15, 0), 0U);
	rk3588_hiword_update(pipe_phy_grf, RK3588_PIPE_PHY_GRF_CON2, GENMASK(15, 0), 0x0101U);
	rk3588_hiword_update(pipe_phy_grf, RK3588_PIPE_PHY_GRF_CON3, GENMASK(15, 0), 0x0200U);
	rk3588_hiword_update(pipe_phy_grf, RK3588_PIPE_PHY_GRF_CON1, GENMASK(14, 13), 2U << 13);

	/* RK3588 PCIe tuning values for a 100-MHz reference clock. */
	rk3588_combphy_update(combphy, RK3588_COMBPHY_SSC, GENMASK(5, 4), BIT(4));
	sys_write32(0xc0U, combphy + RK3588_COMBPHY_GATE_TX_PCK);
	rk3588_combphy_update(combphy, RK3588_COMBPHY_PLL_KVCO, GENMASK(4, 2), BIT(4));
	sys_write32(0x4cU, combphy + RK3588_COMBPHY_RX_TRIM);
	sys_write32(0x90U, combphy + RK3588_COMBPHY_SU_TRIM0);
	sys_write32(0x43U, combphy + RK3588_COMBPHY_SU_TRIM1);
	sys_write32(0x88U, combphy + RK3588_COMBPHY_SU_TRIM2);
	sys_write32(0x56U, combphy + RK3588_COMBPHY_SU_TRIM3);
	rk3588_combphy_update(combphy, RK3588_COMBPHY_FORCE_DET_OUT, BIT(5), BIT(5));
}

static void rk3588_map_hardware(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, pipe_phy_grf, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, combphy, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, gpio, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, ioc, K_MEM_CACHE_NONE);
}

static int rk3588_prepare_hardware(const struct device *dev)
{
	const struct pcie_dw_rockchip_config *config = DEV_CFG(dev);
	int ret;

	/* GPIO3 must be clocked before asserting the endpoint reset. */
	ret = rk3588_clock_enable(&config->top_clock);
	if (ret != 0) {
		return ret;
	}
	ret = rk3588_clock_enable(&config->gpio_clock);
	if (ret != 0) {
		return ret;
	}
	rk3588_perst_hold(dev);

	for (size_t i = 0; i < ARRAY_SIZE(config->resets); i++) {
		if (!device_is_ready(config->resets[i].dev)) {
			return -ENODEV;
		}
		ret = reset_line_assert_dt(&config->resets[i]);
		if (ret != 0) {
			return ret;
		}
	}
	k_busy_wait(10);

	ret = rk3588_enable_clocks(config);
	if (ret != 0) {
		return ret;
	}

	/* The ComboPHY register file is accessed through its APB clock domain. */
	ret = reset_line_deassert_dt(&config->resets[RK3588_PCIE_RESET_PHY_APB]);
	if (ret != 0) {
		return ret;
	}
	rk3588_combphy_init(dev);
	ret = reset_line_deassert_dt(&config->resets[RK3588_PCIE_RESET_PHY]);
	if (ret != 0) {
		return ret;
	}
	k_busy_wait(10);

	/* Release controller resets only after the PHY configuration is active. */
	ret = reset_line_deassert_dt(&config->resets[RK3588_PCIE_RESET_PIPE]);
	if (ret != 0) {
		return ret;
	}
	return reset_line_deassert_dt(&config->resets[RK3588_PCIE_RESET_POWER]);
}

static void rk3588_setup_client(const struct device *dev)
{
	mm_reg_t apb = DEVICE_MMIO_NAMED_GET(dev, pcie_apb);

	/* Hold the LTSSM in reset while the client registers are configured. */
	sys_write32(RK_PCIE_CLIENT_LTSSM_DISABLE, apb + RK_PCIE_CLIENT_GENERAL_CON);
	sys_write32(RK_PCIE_CLIENT_LTSSM_ENHANCE, apb + RK_PCIE_CLIENT_HOT_RESET_CTRL);
	/* CLKREQ# is not wired: keep it pulled down and not ready. */
	sys_write32(RK_PCIE_CLIENT_CLKREQ_ASSERT, apb + RK_PCIE_CLIENT_POWER_CON);
	sys_write32(RK_PCIE_CLIENT_RC_MODE, apb + RK_PCIE_CLIENT_GENERAL_CON);
	/*
	 * Unmask the legacy INTx status lines. Interrupt-map routing of the
	 * combined legacy interrupt is not implemented; endpoints use MSI.
	 */
	sys_write32(RK_PCIE_CLIENT_INTR_LEGACY_UNMASK, apb + RK_PCIE_CLIENT_INTR_MASK_LEGACY);
}

static bool rk3588_link_up(const struct device *dev)
{
	mm_reg_t apb = DEVICE_MMIO_NAMED_GET(dev, pcie_apb);

	return (sys_read32(apb + RK_PCIE_CLIENT_LTSSM_STATUS) & RK_PCIE_LTSSM_LINK_UP) ==
	       RK_PCIE_LTSSM_LINK_UP;
}

static int rk3588_start_link(const struct device *dev)
{
	const struct pcie_dw_rockchip_config *config = DEV_CFG(dev);
	mm_reg_t apb = DEVICE_MMIO_NAMED_GET(dev, pcie_apb);
	uint64_t elapsed_us = 0U;
	uint64_t timeout_us = (uint64_t)config->link_timeout_ms * 1000U;

	sys_write32(RK_PCIE_CLIENT_LTSSM_ENABLE, apb + RK_PCIE_CLIENT_GENERAL_CON);

	if (!config->firmware_initialized) {
		k_busy_wait(config->perst_inactive_ms * 1000U);
		rk3588_perst_release(dev);
	}

	while (elapsed_us <= timeout_us) {
		if (rk3588_link_up(dev)) {
			if (!config->firmware_initialized) {
				k_busy_wait(RK3588_PCIE_LINK_STABLE_US);
				if (!rk3588_link_up(dev)) {
					elapsed_us += RK3588_PCIE_LINK_STABLE_US;
					continue;
				}
			}
			LOG_INF("PCIe link up, LTSSM status %#x",
				sys_read32(apb + RK_PCIE_CLIENT_LTSSM_STATUS));
			return 0;
		}

		k_busy_wait(RK3588_PCIE_LINK_POLL_US);
		elapsed_us += RK3588_PCIE_LINK_POLL_US;
	}

	LOG_ERR("PCIe link timeout, LTSSM status %#x",
		sys_read32(apb + RK_PCIE_CLIENT_LTSSM_STATUS));
	return -ETIMEDOUT;
}

static int pcie_dw_rockchip_init(const struct device *dev)
{
	const struct pcie_dw_rockchip_config *config = DEV_CFG(dev);
	int ret;

	ret = pcie_dw_map(dev);
	if (ret != 0) {
		return ret;
	}

	DEVICE_MMIO_NAMED_MAP(dev, pcie_apb, K_MEM_CACHE_NONE);

	if (!config->firmware_initialized) {
		rk3588_map_hardware(dev);
		ret = rk3588_prepare_hardware(dev);
		if (ret != 0) {
			return ret;
		}
	}

	rk3588_setup_client(dev);

	ret = pcie_dw_setup_root(dev);
	if (ret != 0) {
		return ret;
	}

	ret = pcie_dw_setup_outbound_ranges(dev);
	if (ret != 0) {
		return ret;
	}

	ret = rk3588_start_link(dev);
	if (ret != 0) {
		return ret;
	}

	pcie_generic_ctrl_enumerate(dev, PCIE_BDF(config->dw.root_bus, 0, 0));
	return 0;
}

#if CONFIG_PCIE_MSI
static uint8_t pcie_dw_rockchip_msi_device_setup(const struct device *dev, unsigned int priority,
						 msi_vector_t *vectors, uint8_t n_vector)
{
#ifdef CONFIG_GIC_V3_ITS
	const struct pcie_dw_rockchip_config *config = dev->config;
	const struct device *its = config->dw.common->msi_parent;
	unsigned int device_id;
	unsigned int ites;
	pcie_bdf_t bdf;
	int ret;
	int i;

	if (n_vector == 0U) {
		return 0U;
	}

	bdf = vectors[0].bdf;

	/*
	 * Vectors for multiple BDFs are not supported: each BDF needs its own
	 * ITS DeviceID table, which cannot be sized correctly here because more
	 * vectors may be allocated later.
	 */
	for (i = 1; i < n_vector; i++) {
		if (vectors[i].bdf != bdf) {
			LOG_ERR("multiple BDFs in a single MSI vector allocation");
			return 0U;
		}
	}

	if (!device_is_ready(its)) {
		LOG_ERR("MSI parent %s is not ready", its->name);
		return 0U;
	}

	/*
	 * The ITS MAPD command encodes the event table capacity as a power of
	 * two. MSI-X allows other vector counts, so size the device table for
	 * the next power of two while still mapping n_vector interrupts.
	 */
	ites = 2U;
	while (ites < n_vector) {
		ites <<= 1U;
	}

	device_id = PCI_BDF_TO_DEVID(bdf);
	ret = its_setup_deviceid(its, device_id, ites);
	if (ret != 0) {
		LOG_ERR("failed to set up ITS DeviceID %#x (%d)", device_id, ret);
		return 0U;
	}

	for (i = 0; i < n_vector; i++) {
		vectors[i].arch.irq = its_alloc_intid(its);
		vectors[i].arch.address = its_get_msi_addr(its);
		vectors[i].arch.eventid = i;
		vectors[i].arch.priority = priority;

		ret = its_map_intid(its, device_id, vectors[i].arch.eventid, vectors[i].arch.irq);
		if (ret != 0) {
			LOG_ERR("failed to map MSI event %d for DeviceID %#x (%d)", i, device_id,
				ret);
			break;
		}
	}

	return i;
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(priority);
	ARG_UNUSED(vectors);
	ARG_UNUSED(n_vector);

	return 0U;
#endif
}
#endif

static DEVICE_API(pcie_ctrl, pcie_dw_rockchip_api) = {
	.conf_read = pcie_dw_conf_read,
	.conf_write = pcie_dw_conf_write,
	.region_allocate = pcie_dw_region_allocate,
	.region_get_allocate_base = pcie_dw_region_get_allocate_base,
	.region_translate = pcie_dw_region_translate,
#if CONFIG_PCIE_MSI
	.msi_device_setup = pcie_dw_rockchip_msi_device_setup,
#endif
};

#if defined(CONFIG_PCIE_MSI) && defined(CONFIG_GIC_V3_ITS)
#define PCIE_DW_ROCKCHIP_MSI_PARENT(inst)                                                          \
	.msi_parent = DEVICE_DT_GET(DT_INST_PHANDLE(inst, msi_parent)),
#else
#define PCIE_DW_ROCKCHIP_MSI_PARENT(inst)
#endif

#define PCIE_DW_RK_CLOCK(inst, name)                                                               \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(inst, name)),                     \
		.id = DT_INST_CLOCKS_CELL_BY_NAME(inst, name, clkid),                              \
	}

#define PCIE_DW_RK_CLOCK_ENTRY(node_id, prop, idx)                                                 \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_IDX(node_id, idx)),                         \
		.id = DT_CLOCKS_CELL_BY_IDX(node_id, idx, clkid),                                  \
	},

#define PCIE_DW_RK_RESET(inst, name)                                                               \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_INST_RESET_CTLR_BY_NAME(inst, name)),                      \
		.id = DT_INST_RESET_CELL_BY_NAME(inst, name, id),                                  \
	}

#define PCIE_DW_ROCKCHIP_INIT(inst)                                                                \
	BUILD_ASSERT(offsetof(struct pcie_dw_rockchip_data, dw) == 0,                              \
		     "pcie_dw_data must be the first member of the driver data");                  \
	BUILD_ASSERT(offsetof(struct pcie_dw_rockchip_config, dw) == 0,                            \
		     "pcie_dw_config must be the first member of the driver config");              \
	BUILD_ASSERT((DT_INST_REG_ADDR_BY_NAME(inst, pcie_apb) == RK3588_PCIE2X1L2_APB_PHYS) ||    \
			     DT_INST_PROP(inst, rockchip_firmware_initialized),                    \
		     "direct initialization only supports RK3588 pcie2x1l2");                      \
	BUILD_ASSERT(DT_INST_REG_SIZE_BY_NAME(inst, pcie_apb) >                                    \
			     (RK_PCIE_CLIENT_LTSSM_STATUS + sizeof(uint32_t)),                     \
		     "pcie_apb register window is too small");                                     \
	BUILD_ASSERT(DT_INST_REG_SIZE_BY_NAME(inst, pipe_phy_grf) >                                \
			     (RK3588_PIPE_PHY_GRF_CON3 + sizeof(uint32_t)),                        \
		     "pipe_phy_grf register window is too small");                                 \
	BUILD_ASSERT(DT_INST_REG_SIZE_BY_NAME(inst, combphy) >                                     \
			     (RK3588_COMBPHY_PLL_KVCO + sizeof(uint32_t)),                         \
		     "combphy register window is too small");                                      \
	BUILD_ASSERT(DT_INST_REG_SIZE_BY_NAME(inst, gpio) >                                        \
			     (RK3588_GPIO_SWPORT_DDR_L + sizeof(uint32_t)),                        \
		     "gpio register window is too small");                                         \
	BUILD_ASSERT(DT_INST_REG_SIZE_BY_NAME(inst, ioc) >                                         \
			     (RK3588_IOC_GPIO3B_IOMUX_L + sizeof(uint32_t)),                       \
		     "ioc register window is too small");                                          \
	BUILD_ASSERT((uint32_t)DT_INST_PROP_OR(inst, rockchip_perst_inactive_ms,                   \
					       RK3588_PCIE_DEFAULT_PERST_MS) <=                    \
			     RK3588_PCIE_MAX_PERST_MS,                                             \
		     "rockchip,perst-inactive-ms is too large");                                   \
	BUILD_ASSERT((uint32_t)DT_INST_PROP_OR(inst, rockchip_link_timeout_ms,                     \
					       RK3588_PCIE_DEFAULT_LINK_TIMEOUT_MS) <=             \
			     RK3588_PCIE_MAX_LINK_TIMEOUT_MS,                                      \
		     "rockchip,link-timeout-ms is too large");                                     \
	static struct pcie_dw_rockchip_data pcie_dw_rockchip_data_##inst;                          \
	static const struct rk3588_pcie_clock pcie_dw_clocks_##inst[] = {                          \
		DT_INST_FOREACH_PROP_ELEM(inst, clocks, PCIE_DW_RK_CLOCK_ENTRY)};                  \
	static const struct pcie_ctrl_config pcie_dw_ctrl_config_##inst = {                        \
		PCIE_DW_ROCKCHIP_MSI_PARENT(inst).cfg_addr =                                       \
			DT_INST_REG_ADDR_BY_NAME(inst, config),                                    \
		.cfg_size = DT_INST_REG_SIZE_BY_NAME(inst, config),                                \
		.ranges_count = DT_NUM_RANGES(DT_DRV_INST(inst)),                                  \
		.ranges = {DT_FOREACH_RANGE(DT_DRV_INST(inst), PCIE_RANGE_FORMAT)},                \
	};                                                                                         \
	static const struct pcie_dw_rockchip_config pcie_dw_rockchip_config_##inst = {             \
		.dw =                                                                              \
			{                                                                          \
				.common = &pcie_dw_ctrl_config_##inst,                             \
				DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(pcie_dbi, DT_DRV_INST(inst)),   \
				DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(config, DT_DRV_INST(inst)),     \
				.dbi_size = DT_INST_REG_SIZE_BY_NAME(inst, pcie_dbi),              \
				.root_bus = DT_INST_PROP_BY_IDX(inst, bus_range, 0),               \
				.last_bus = DT_INST_PROP_BY_IDX(inst, bus_range, 1),               \
				.num_lanes = DT_INST_PROP(inst, num_lanes),                        \
				.max_link_speed = DT_INST_PROP(inst, max_link_speed),              \
			},                                                                         \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(pcie_apb, DT_DRV_INST(inst)),                   \
		.clocks = pcie_dw_clocks_##inst,                                                   \
		.clock_count = ARRAY_SIZE(pcie_dw_clocks_##inst),                                  \
		.top_clock = PCIE_DW_RK_CLOCK(inst, top),                                          \
		.gpio_clock = PCIE_DW_RK_CLOCK(inst, gpio),                                        \
		.ref_clock = PCIE_DW_RK_CLOCK(inst, ref),                                          \
		.resets =                                                                          \
			{                                                                          \
				[RK3588_PCIE_RESET_POWER] = PCIE_DW_RK_RESET(inst, pwr),           \
				[RK3588_PCIE_RESET_PIPE] = PCIE_DW_RK_RESET(inst, pipe),           \
				[RK3588_PCIE_RESET_PHY] = PCIE_DW_RK_RESET(inst, phy),             \
				[RK3588_PCIE_RESET_PHY_APB] = PCIE_DW_RK_RESET(inst, phy_apb),     \
			},                                                                         \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(pipe_phy_grf, DT_DRV_INST(inst)),               \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(combphy, DT_DRV_INST(inst)),                    \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(gpio, DT_DRV_INST(inst)),                       \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(ioc, DT_DRV_INST(inst)),                        \
		.perst_inactive_ms = DT_INST_PROP_OR(inst, rockchip_perst_inactive_ms,             \
						     RK3588_PCIE_DEFAULT_PERST_MS),                \
		.link_timeout_ms = DT_INST_PROP_OR(inst, rockchip_link_timeout_ms,                 \
						   RK3588_PCIE_DEFAULT_LINK_TIMEOUT_MS),           \
		.firmware_initialized = DT_INST_PROP(inst, rockchip_firmware_initialized),         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, pcie_dw_rockchip_init, NULL, &pcie_dw_rockchip_data_##inst,    \
			      &pcie_dw_rockchip_config_##inst, PRE_KERNEL_1,                       \
			      CONFIG_PCIE_INIT_PRIORITY, &pcie_dw_rockchip_api);

DT_INST_FOREACH_STATUS_OKAY(PCIE_DW_ROCKCHIP_INIT)
