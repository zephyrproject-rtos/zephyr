/*
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control driver for Alif Semiconductor SoCs
 *
 * This driver provides clock control for Alif Semiconductor SoC families.
 */

#define DT_DRV_COMPAT alif_clockctrl

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/dt-bindings/clock/alif-clocks-common.h>

LOG_MODULE_REGISTER(alif_clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DEV_CFG(dev)  ((const struct alif_clock_control_config *)(dev)->config)
#define DEV_DATA(dev) ((struct alif_clock_control_data *)(dev)->data)

/**
 * @brief Alif clock controller data structure
 */
struct alif_clock_control_data {
	DEVICE_MMIO_NAMED_RAM(cgu);
	DEVICE_MMIO_NAMED_RAM(clkctl_per_mst);
	DEVICE_MMIO_NAMED_RAM(clkctl_per_slv);
	DEVICE_MMIO_NAMED_RAM(aon);
	DEVICE_MMIO_NAMED_RAM(vbat);
	DEVICE_MMIO_NAMED_RAM(m55he_cfg);
	/* m55hp_cfg is optional, see m55hp_cfg_phys in the config struct */
	mm_reg_t m55hp_cfg;
};

/**
 * @brief Alif clock controller configuration structure
 */
struct alif_clock_control_config {
	DEVICE_MMIO_NAMED_ROM(cgu);
	DEVICE_MMIO_NAMED_ROM(clkctl_per_mst);
	DEVICE_MMIO_NAMED_ROM(clkctl_per_slv);
	DEVICE_MMIO_NAMED_ROM(aon);
	DEVICE_MMIO_NAMED_ROM(vbat);
	DEVICE_MMIO_NAMED_ROM(m55he_cfg);
	/*
	 * m55hp_cfg is optional (absent on Balletto family and E1C series SoCs).
	 * The DEVICE_MMIO_NAMED_* helpers cannot express an optional region, so
	 * its physical address is stored here (0 if absent) and mapped manually
	 * with device_map() in the init function.
	 */
	uintptr_t m55hp_cfg_phys;
};

/*
 * Fixed-clock frequencies from device tree
 */
#define ALIF_CLOCK_SYST_ACLK_FREQ    DT_PROP(DT_NODELABEL(syst_aclk), clock_frequency)
#define ALIF_CLOCK_SYST_HCLK_FREQ    DT_PROP(DT_NODELABEL(syst_hclk), clock_frequency)
#define ALIF_CLOCK_SYST_PCLK_FREQ    DT_PROP(DT_NODELABEL(syst_pclk), clock_frequency)

/*
 * Clock Configuration Field Extraction Macros
 *
 * These macros extract specific fields from the 32-bit clock ID encoded
 * using ALIF_CLK_CFG() in the device tree. The encoding layout is:
 *
 *   Bit   31:    Reserved for future use
 *   Bits  30-26: Input clock source identifier (0-31)
 *   Bits  25-21: Clock source bit position (0-31)
 *   Bits  20-19: Clock source mask width (0-3, 0=no source select)
 *   Bits  18-17: Clock source value (0-3)
 *   Bit   16:    Enable control available flag (1=yes, 0=always-on)
 *   Bits  15-11: Enable bit position (0-31)
 *   Bits  10-3:  Register offset within module (0-255)
 *   Bits  2-0:   Clock module ID (0-6)
 */

/**
 * @brief Extract clock module ID from encoded clock ID
 * @param id Clock ID from device tree (encoded with ALIF_CLK_CFG)
 * @return Module ID (ALIF_CGU_MODULE, ALIF_CLKCTL_PER_MST_MODULE, etc)
 */
#define ALIF_CLOCK_CFG_MODULE(id) \
	(((id) >> ALIF_CLOCK_MODULE_SHIFT) & ALIF_CLOCK_MODULE_MASK)

/**
 * @brief Extract register offset from encoded clock ID
 * @param id Clock ID from device tree (encoded with ALIF_CLK_CFG)
 * @return Register offset within the clock module (0-255 bytes)
 */
#define ALIF_CLOCK_CFG_REG(id) \
	(((id) >> ALIF_CLOCK_REG_SHIFT) & ALIF_CLOCK_REG_MASK)

/**
 * @brief Extract enable bit position from encoded clock ID
 * @param id Clock ID from device tree (encoded with ALIF_CLK_CFG)
 * @return Bit position for clock enable control (0-31)
 */
#define ALIF_CLOCK_CFG_ENABLE(id) \
	(((id) >> ALIF_CLOCK_EN_BIT_POS_SHIFT) & ALIF_CLOCK_EN_BIT_POS_MASK)

/**
 * @brief Check if clock has enable control from encoded clock ID
 * @param id Clock ID from device tree (encoded with ALIF_CLK_CFG)
 * @return 1 if clock has enable/disable control, 0 if always-on
 */
#define ALIF_CLOCK_CFG_EN_MASK(id) \
	(((id) >> ALIF_CLOCK_EN_MASK_SHIFT) & 0x1U)

/**
 * @brief Extract clock source value from encoded clock ID
 * @param id Clock ID from device tree (encoded with ALIF_CLK_CFG)
 * @return Clock source selection value (0-3)
 */
#define ALIF_CLOCK_CFG_SRC_VAL(id) \
	(((id) >> ALIF_CLOCK_SRC_VAL_SHIFT) & ALIF_CLOCK_SRC_VAL_MASK)

/**
 * @brief Extract clock source field width from encoded clock ID
 * @param id Clock ID from device tree (encoded with ALIF_CLK_CFG)
 * @return Field width for source selection (0=no source select, 1-3=field width in bits)
 */
#define ALIF_CLOCK_CFG_SRC_FIELD_WIDTH(id) \
	(((id) >> ALIF_CLOCK_SRC_FIELD_WIDTH_SHIFT) & ALIF_CLOCK_SRC_FIELD_WIDTH_MASK)

/**
 * @brief Extract clock source field position from encoded clock ID
 * @param id Clock ID from device tree (encoded with ALIF_CLK_CFG)
 * @return Bit position for clock source selection field (0-31)
 */
#define ALIF_CLOCK_CFG_SRC_FIELD_POS(id) \
	(((id) >> ALIF_CLOCK_SRC_FIELD_POS_SHIFT) & ALIF_CLOCK_SRC_FIELD_POS_MASK)

/**
 * @brief Extract parent clock source from encoded clock ID
 * @param id Clock ID from device tree (encoded with ALIF_CLK_CFG)
 * @return Parent clock source identifier (0-31, see ALIF_PARENT_CLK_*)
 */
#define ALIF_CLOCK_CFG_PARENT_CLK(id) \
	(((id) >> ALIF_CLOCK_PARENT_CLK_SHIFT) & ALIF_CLOCK_PARENT_CLK_MASK)

/**
 * @brief Get the register address for a clock ID
 * @param dev Clock control device
 * @param clk_id Encoded clock ID from device tree
 * @return Register address for the clock control
 */
static mem_addr_t alif_get_clk_reg_addr(const struct device *dev, uint32_t clk_id)
{
	struct alif_clock_control_data *data = DEV_DATA(dev);
	mem_addr_t base;
	uint32_t reg_offset = ALIF_CLOCK_CFG_REG(clk_id);

	switch (ALIF_CLOCK_CFG_MODULE(clk_id)) {
	case ALIF_CGU_MODULE:
		base = DEVICE_MMIO_NAMED_GET(dev, cgu);
		break;
	case ALIF_CLKCTL_PER_MST_MODULE:
		base = DEVICE_MMIO_NAMED_GET(dev, clkctl_per_mst);
		break;
	case ALIF_CLKCTL_PER_SLV_MODULE:
		base = DEVICE_MMIO_NAMED_GET(dev, clkctl_per_slv);
		break;
	case ALIF_AON_MODULE:
		base = DEVICE_MMIO_NAMED_GET(dev, aon);
		break;
	case ALIF_VBAT_MODULE:
		base = DEVICE_MMIO_NAMED_GET(dev, vbat);
		break;
	case ALIF_M55HE_CFG_MODULE:
		base = DEVICE_MMIO_NAMED_GET(dev, m55he_cfg);
		break;
	case ALIF_M55HP_CFG_MODULE:
		/* Optional region, mapped manually in init (see m55hp_cfg_phys) */
		if (data->m55hp_cfg == 0) {
			__ASSERT(false, "M55HP_CFG module not available on this SoC");
			return 0;
		}
		base = data->m55hp_cfg;
		break;
	default:
		__ASSERT(false, "Invalid clock module: %u", ALIF_CLOCK_CFG_MODULE(clk_id));
		return 0;
	}

	return base + reg_offset;
}

/**
 * @brief Get clock frequency for a given clock ID
 *
 * Returns the frequency of the clock represented by the given clock ID.
 *
 * @param clock_id Encoded clock ID from device tree
 * @return Clock frequency in Hz, or 0 if unknown
 */
static uint32_t alif_get_clock_freq(uint32_t clock_id)
{
	uint32_t parent_clk = ALIF_CLOCK_CFG_PARENT_CLK(clock_id);

	switch (parent_clk) {
	case ALIF_PARENT_CLK_SYST_ACLK:
		return ALIF_CLOCK_SYST_ACLK_FREQ;
	case ALIF_PARENT_CLK_SYST_HCLK:
		return ALIF_CLOCK_SYST_HCLK_FREQ;
	case ALIF_PARENT_CLK_SYST_PCLK:
		return ALIF_CLOCK_SYST_PCLK_FREQ;
	default:
		__ASSERT(false, "Invalid parent clock: %u", parent_clk);
		return 0;
	}
}

/**
 * @brief Enable a peripheral clock
 *
 * Enables the specified peripheral clock. For clocks with source selection,
 * this also configures the clock source based on the device tree encoding.
 * Always-on clocks return success immediately.
 *
 * @param dev Clock control device (unused)
 * @param sub_system Encoded clock ID from device tree
 * @return 0 on success, -ENOTSUP if clock is not controllable
 */
static int alif_clock_control_on(const struct device *dev,
			clock_control_subsys_t sub_system)
{
	uint32_t clk_id = (uint32_t)sub_system;
	mem_addr_t reg_addr;
	uint32_t reg_value;
	uint32_t enable_mask;

	if (!ALIF_CLOCK_CFG_EN_MASK(clk_id)) {
		/* Clock is always-on, already enabled */
		return 0;
	}

	reg_addr = alif_get_clk_reg_addr(dev, clk_id);
	enable_mask = 1U << ALIF_CLOCK_CFG_ENABLE(clk_id);

	reg_value = sys_read32(reg_addr);

	/*
	 * Set the default clock source if mux is available.
	 */
	if (ALIF_CLOCK_CFG_SRC_FIELD_WIDTH(clk_id)) {
		uint32_t src_bit = ALIF_CLOCK_CFG_SRC_FIELD_POS(clk_id);
		uint32_t src_mask = ALIF_CLOCK_CFG_SRC_FIELD_WIDTH(clk_id);
		uint32_t src_value = ALIF_CLOCK_CFG_SRC_VAL(clk_id);

		/* Clear and set source bits */
		reg_value &= ~(src_mask << src_bit);
		reg_value |= (src_value << src_bit);
	}

	/* Enable the clock */
	reg_value |= enable_mask;

	sys_write32(reg_value, reg_addr);

	return 0;
}

/**
 * @brief Disable a peripheral clock
 *
 * Disables the specified peripheral clock. Always-on clocks cannot be
 * disabled and will return an error.
 *
 * @param dev Clock control device (unused)
 * @param sub_system Encoded clock ID from device tree
 * @return 0 on success, -ENOTSUP if clock is always-on
 */
static int alif_clock_control_off(const struct device *dev,
			clock_control_subsys_t sub_system)
{
	uint32_t clk_id = (uint32_t)sub_system;
	mem_addr_t reg_addr;

	if (!ALIF_CLOCK_CFG_EN_MASK(clk_id)) {
		LOG_ERR("Clock is always-on and cannot be disabled");
		return -ENOTSUP;
	}

	reg_addr = alif_get_clk_reg_addr(dev, clk_id);
	sys_clear_bit(reg_addr, ALIF_CLOCK_CFG_ENABLE(clk_id));

	return 0;
}

/**
 * @brief Get the clock rate for a peripheral clock
 *
 * Returns the input clock frequency for the specified peripheral.
 *
 * @param dev Clock control device (unused)
 * @param sub_system Encoded clock ID from device tree
 * @param rate Pointer to store the clock rate in Hz
 * @return 0 on success
 */
static int alif_clock_control_get_rate(const struct device *dev,
				clock_control_subsys_t sub_system,
				uint32_t *rate)
{
	uint32_t clk_id = (uint32_t)sub_system;

	ARG_UNUSED(dev);

	*rate = alif_get_clock_freq(clk_id);

	return 0;
}

/**
 * @brief Get the status of a peripheral clock
 *
 * Returns whether the clock is currently enabled or disabled.
 * Always-on clocks always return ON status.
 *
 * @param dev Clock control device (unused)
 * @param sub_system Encoded clock ID from device tree
 * @return CLOCK_CONTROL_STATUS_ON if enabled, CLOCK_CONTROL_STATUS_OFF if disabled
 */
static enum clock_control_status
	alif_clock_control_get_status(const struct device *dev,
				clock_control_subsys_t sub_system)
{
	uint32_t clk_id = (uint32_t)sub_system;
	mem_addr_t reg_addr;

	if (!ALIF_CLOCK_CFG_EN_MASK(clk_id)) {
		/* Clock is always-on */
		return CLOCK_CONTROL_STATUS_ON;
	}

	reg_addr = alif_get_clk_reg_addr(dev, clk_id);

	if (sys_test_bit(reg_addr, ALIF_CLOCK_CFG_ENABLE(clk_id)) != 0) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

static DEVICE_API(clock_control, alif_clock_control_driver_api) = {
	.on = alif_clock_control_on,
	.off = alif_clock_control_off,
	.get_rate = alif_clock_control_get_rate,
	.get_status = alif_clock_control_get_status,
};

/**
 * @brief Map clock controller MMIO regions into the virtual address space
 *
 * Map the clock module register regions using the device MMIO API during
 * driver initialization. On non-MMU targets this is a no-op that records
 * the physical address as the linear address.
 *
 * @param dev Clock control device
 * @return 0 on success
 */
static int alif_clock_control_init(const struct device *dev)
{
	struct alif_clock_control_data *data = DEV_DATA(dev);
	const struct alif_clock_control_config *config = DEV_CFG(dev);

	DEVICE_MMIO_NAMED_MAP(dev, cgu, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, clkctl_per_mst, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, clkctl_per_slv, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, aon, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, vbat, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, m55he_cfg, K_MEM_CACHE_NONE);

	/* Optional m55hp_cfg: map manually only when present (phys != 0) */
	if (config->m55hp_cfg_phys != 0) {
		device_map(&data->m55hp_cfg, config->m55hp_cfg_phys,
			   DT_REG_SIZE_BY_NAME_OR(DT_NODELABEL(clockctrl), m55hp_cfg, 0),
			   K_MEM_CACHE_NONE);
	}

	return 0;
}

/* Clock controller configuration from device tree */
static const struct alif_clock_control_config alif_clock_config = {
	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(cgu, DT_NODELABEL(clockctrl)),
	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(clkctl_per_mst, DT_NODELABEL(clockctrl)),
	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(clkctl_per_slv, DT_NODELABEL(clockctrl)),
	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(aon, DT_NODELABEL(clockctrl)),
	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(vbat, DT_NODELABEL(clockctrl)),
	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(m55he_cfg, DT_NODELABEL(clockctrl)),
	/* Optional region: 0 when m55hp_cfg is absent from the DT node */
	.m55hp_cfg_phys = DT_REG_ADDR_BY_NAME_OR(DT_NODELABEL(clockctrl), m55hp_cfg, 0),
};

static struct alif_clock_control_data alif_clock_data;

DEVICE_DT_DEFINE(DT_NODELABEL(clockctrl),
		 alif_clock_control_init,
		 NULL,
		 &alif_clock_data,
		 &alif_clock_config,
		 PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &alif_clock_control_driver_api);
