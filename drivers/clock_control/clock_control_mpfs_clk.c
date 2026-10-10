/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mpfs_clk

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/mchp_mss_clock.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <limits.h>

LOG_MODULE_REGISTER(clock_control_mpfs_clk, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

/* -------------------------------------------------------------------------- */
/* Register offsets                                                           */
/* -------------------------------------------------------------------------- */
#define MPFS_TOP_SYSREG_CLOCK_CONFIG_CR_OFFSET 0x08u
#define MPFS_TOP_SYSREG_SUBBLK_CLOCK_CR_OFFSET 0x84u

/* -------------------------------------------------------------------------- */
/* SOFT_RESET fields                                                          */
/* -------------------------------------------------------------------------- */
/* AHB/APB clock divider*/
#define MPFS_TOP_SYSREG_CLOCK_CONFIG_CR_DIVIDER_AHB_MASK  GENMASK(5, 4)
#define MPFS_TOP_SYSREG_CLOCK_CONFIG_CR_DIVIDER_AHB_SHIFT 4U

/* AXI clock divider*/
#define MPFS_TOP_SYSREG_CLOCK_CONFIG_CR_DIVIDER_AXI_MASK  GENMASK(3, 2)
#define MPFS_TOP_SYSREG_CLOCK_CONFIG_CR_DIVIDER_AXI_SHIFT 2U

/* CPU clock divider*/
#define MPFS_TOP_SYSREG_CLOCK_CONFIG_CR_DIVIDER_CPU_MASK  GENMASK(1, 0)
#define MPFS_TOP_SYSREG_CLOCK_CONFIG_CR_DIVIDER_CPU_SHIFT 0U

#define MPFS_DIVIDER_FIELD_MASK  0x3U
#define MPFS_DIVIDER_ENC_INVALID UINT8_MAX

struct mpfs_clk_config {
	uint32_t clk_type;              /* clock identifier (MPFS_CLK_*) */
	uintptr_t top_sysreg_base;      /* base address of top SYSREG/CLKCFG registers */
	const struct device *clock_dev; /* pointer to the parent clock device */
	uint32_t frequency_max;         /* Maximum frequency for this PLL output */
	uint32_t frequency_min;         /* Minimum frequency for this PLL output */
};

struct mpfs_clk_data {
	struct k_spinlock lock; /* protects register accesses for this clock instance */
};

/* CPU/AXI encoding: 00->1, 01->2, 10->4, 11->8  */
static const uint8_t div_cpu_axi_tbl[4] = {1, 2, 4, 8};
/* AHB encoding: 00->(invalid), 01->2, 10->4, 11->8 */
static const uint8_t div_ahb_tbl[4] = {0, 2, 4, 8};

/**
 * @brief Report the current state of one peripheral gate clock.
 *
 * @param dev MPFS clock controller device.
 * @param sys Clock subsystem selector encoded as a gate identifier.
 *
 * @return Current gate status, or CLOCK_CONTROL_STATUS_UNKNOWN if @p sys is invalid.
 */
static enum clock_control_status mpfs_clock_get_status(const struct device *dev,
						       clock_control_subsys_t sys)
{
	const struct mpfs_clk_config *cfg = (const struct mpfs_clk_config *)dev->config;
	struct mpfs_clk_data *data = (struct mpfs_clk_data *)dev->data;
	k_spinlock_key_t key;
	uint32_t clk_id;
	uint32_t sub_ccr;

	if ((uint32_t)(uintptr_t)sys >= sizeof(uint32_t) * CHAR_BIT) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	clk_id = (uint32_t)(uintptr_t)sys;

	key = k_spin_lock(&data->lock);
	sub_ccr = sys_read32(cfg->top_sysreg_base + MPFS_TOP_SYSREG_SUBBLK_CLOCK_CR_OFFSET);
	k_spin_unlock(&data->lock, key);

	return ((sub_ccr & (1 << clk_id)) ? CLOCK_CONTROL_STATUS_ON : CLOCK_CONTROL_STATUS_OFF);
}

/**
 * @brief Enable one peripheral gate clock.
 *
 * @param dev MPFS clock controller device.
 * @param sys Clock subsystem selector encoded as a gate identifier.
 *
 * @retval 0 Gate enabled successfully.
 * @retval -EIO The gate identifier is out of range.
 */
static int mpfs_clock_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct mpfs_clk_config *cfg = (const struct mpfs_clk_config *)dev->config;
	struct mpfs_clk_data *data = (struct mpfs_clk_data *)dev->data;
	k_spinlock_key_t key;
	uint32_t clk_id;

	if ((uint32_t)(uintptr_t)sys >= sizeof(uint32_t) * CHAR_BIT) {
		return -EIO;
	}

	clk_id = (uint32_t)(uintptr_t)sys;

	key = k_spin_lock(&data->lock);
	sys_set_bit(cfg->top_sysreg_base + MPFS_TOP_SYSREG_SUBBLK_CLOCK_CR_OFFSET, clk_id);
	k_spin_unlock(&data->lock, key);

	return 0;
}

/**
 * @brief Disable one peripheral gate clock.
 *
 * @param dev MPFS clock controller device.
 * @param sys Clock subsystem selector encoded as a gate identifier.
 *
 * @retval 0 Gate disabled successfully.
 * @retval -EIO The gate identifier is out of range.
 */
static int mpfs_clock_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct mpfs_clk_config *cfg = (const struct mpfs_clk_config *)dev->config;
	struct mpfs_clk_data *data = (struct mpfs_clk_data *)dev->data;
	k_spinlock_key_t key;
	uint32_t clk_id;

	if ((uint32_t)(uintptr_t)sys >= sizeof(uint32_t) * CHAR_BIT) {
		return -EIO;
	}

	clk_id = (uint32_t)(uintptr_t)sys;

	key = k_spin_lock(&data->lock);
	sys_clear_bit(cfg->top_sysreg_base + MPFS_TOP_SYSREG_SUBBLK_CLOCK_CR_OFFSET, clk_id);
	k_spin_unlock(&data->lock, key);

	return 0;
}

/**
 * @brief Query the rate of a supported MPFS clock.
 *
 * @param dev MPFS clock controller device.
 * @param sys Clock subsystem selector.
 * @param rate Output pointer for the resolved clock frequency in hertz.
 *
 * @retval 0 Rate written to @p rate.
 * @retval -ENOTSUP The requested clock ID is not rate-queryable by this driver.
 */
static int mpfs_clock_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(sys);
	const struct mpfs_clk_config *cfg = (const struct mpfs_clk_config *)dev->config;
	struct mpfs_clk_data *data = (struct mpfs_clk_data *)dev->data;
	k_spinlock_key_t key;
	uint32_t parent_clk_rate, ccr, encoding, divider, clk_type;
	int ret;

	ret = clock_control_get_rate(cfg->clock_dev, 0, &parent_clk_rate);
	if (ret != 0) {
		return ret;
	}

	clk_type = cfg->clk_type;

	key = k_spin_lock(&data->lock);
	ccr = sys_read32(cfg->top_sysreg_base + MPFS_TOP_SYSREG_CLOCK_CONFIG_CR_OFFSET);
	k_spin_unlock(&data->lock, key);

	switch (clk_type) {
	case MPFS_CLK_CPU_CLK:
		encoding = FIELD_GET(MPFS_TOP_SYSREG_CLOCK_CONFIG_CR_DIVIDER_CPU_MASK, ccr);
		divider = div_cpu_axi_tbl[encoding];
		if (divider == 0U) {
			return -EAGAIN;
		}
		*rate = parent_clk_rate / divider;
		return 0;

	case MPFS_CLK_AXI_CLK:
		encoding = FIELD_GET(MPFS_TOP_SYSREG_CLOCK_CONFIG_CR_DIVIDER_AXI_MASK, ccr);
		divider = div_cpu_axi_tbl[encoding];
		if (divider == 0U) {
			return -EAGAIN;
		}
		*rate = parent_clk_rate / divider;
		return 0;

	case MPFS_CLK_AHB_APB_CLK:
		encoding = FIELD_GET(MPFS_TOP_SYSREG_CLOCK_CONFIG_CR_DIVIDER_AHB_MASK, ccr);
		divider = div_ahb_tbl[encoding]; /* 0 means invalid/reserved */
		if (divider == 0U) {
			return -EAGAIN;
		}
		*rate = parent_clk_rate / divider;
		return 0;

	default:
		return -ENOTSUP;
	}
}

/* Optional: divider-only rate setting for CPU/AXI/AHB in REG_CLOCK_CONFIG_CR.
 * PLL remains read-only regardless of this option.
 * For simplicity, we accept exact target dividers (1,2,4,8 for CPU/AXI; 2,4,8 for AHB).
 */
/**
 * @brief Update a 2-bit divider encoding in a register image.
 *
 * @param val Pointer to the register image to update.
 * @param shift Bit position of the 2-bit field.
 * @param enc Encoded divider value.
 *
 * @retval 0 Field updated.
 * @retval -EINVAL The encoding is outside the supported 2-bit range.
 */
static int set_divider_field(uint32_t *val, uint32_t shift, uint8_t enc)
{
	if (enc > MPFS_DIVIDER_FIELD_MASK) {
		return -EINVAL;
	}

	*val &= ~(MPFS_DIVIDER_FIELD_MASK << shift);
	*val |= ((uint32_t)enc << shift);
	return 0;
}

static uint8_t mpfs_find_divider_encoding(const uint8_t *divider_table, size_t table_len,
					  uint32_t parent_rate, uint32_t target_rate)
{
	for (uint8_t i = 0; i < table_len; ++i) {
		if (divider_table[i] == 0U) {
			continue;
		}

		if ((parent_rate / divider_table[i]) == target_rate) {
			return i;
		}
	}

	return MPFS_DIVIDER_ENC_INVALID;
}

static int mpfs_write_divider_encoding(const struct mpfs_clk_config *cfg,
				       struct mpfs_clk_data *data, uint32_t shift, uint8_t enc)
{
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t c = sys_read32(cfg->top_sysreg_base + MPFS_TOP_SYSREG_CLOCK_CONFIG_CR_OFFSET);
	int ret = set_divider_field(&c, shift, enc);

	if (ret == 0) {
		sys_write32(c, cfg->top_sysreg_base + MPFS_TOP_SYSREG_CLOCK_CONFIG_CR_OFFSET);
	}

	k_spin_unlock(&data->lock, key);
	return ret;
}

/**
 * @brief Change a supported derived clock rate by updating divider fields.
 *
 * Only CPU, AXI, and AHB divider-backed clocks are writable, and only exact
 * target rates representable by the hardware divider encodings are accepted.
 * The PLL itself remains read-only.
 *
 * @param dev MPFS clock controller device.
 * @param sys Clock subsystem selector.
 * @param target_rate_hz Requested target rate encoded as a subsystem-rate value.
 *
 * @retval 0 Divider updated successfully.
 * @retval -EIO The selected parent PLL output rate could not be determined.
 * @retval -EINVAL The requested rate cannot be encoded for the selected clock.
 * @retval -ENOTSUP The selected clock is not writable.
 */
static int mpfs_clock_set_rate(const struct device *dev, clock_control_subsys_t sys,
			       clock_control_subsys_rate_t target_rate_hz)
{
	ARG_UNUSED(sys);
	const struct mpfs_clk_config *cfg = (const struct mpfs_clk_config *)dev->config;
	struct mpfs_clk_data *data = (struct mpfs_clk_data *)dev->data;
	uint32_t target = POINTER_TO_UINT(target_rate_hz);
	uint32_t parent_rate;
	uint8_t enc;
	int ret;

	if (target == 0U) {
		return -EINVAL;
	}

	ret = clock_control_get_rate(cfg->clock_dev, 0, &parent_rate);
	if (ret != 0) {
		return ret;
	}

	switch (cfg->clk_type) {
	case MPFS_CLK_CPU_CLK:
		enc = mpfs_find_divider_encoding(div_cpu_axi_tbl, ARRAY_SIZE(div_cpu_axi_tbl),
						 parent_rate, target);
		if (enc == MPFS_DIVIDER_ENC_INVALID) {
			return -EINVAL;
		}
		return mpfs_write_divider_encoding(
			cfg, data, MPFS_TOP_SYSREG_CLOCK_CONFIG_CR_DIVIDER_CPU_SHIFT, enc);

	case MPFS_CLK_AXI_CLK:
		enc = mpfs_find_divider_encoding(div_cpu_axi_tbl, ARRAY_SIZE(div_cpu_axi_tbl),
						 parent_rate, target);
		if (enc == MPFS_DIVIDER_ENC_INVALID) {
			return -EINVAL;
		}
		return mpfs_write_divider_encoding(
			cfg, data, MPFS_TOP_SYSREG_CLOCK_CONFIG_CR_DIVIDER_AXI_SHIFT, enc);

	case MPFS_CLK_AHB_APB_CLK:
		enc = mpfs_find_divider_encoding(div_ahb_tbl, ARRAY_SIZE(div_ahb_tbl), parent_rate,
						 target);
		if (enc == MPFS_DIVIDER_ENC_INVALID) {
			return -EINVAL;
		}
		return mpfs_write_divider_encoding(
			cfg, data, MPFS_TOP_SYSREG_CLOCK_CONFIG_CR_DIVIDER_AHB_SHIFT, enc);

	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Initialize the MPFS clock controller driver instance.
 *
 * The PLL is configured earlier by platform firmware, so initialization is
 * currently limited to successful driver registration.
 *
 * @param dev MPFS clock controller device.
 *
 * @retval 0 Always returns success.
 */
static int mpfs_clk_init(const struct device *dev)
{
	/* PLL is read-only; no programming here.
	 * If needed, you can sanity-check RTCREF to be near 1 MHz and warn if not.
	 */
	ARG_UNUSED(dev);
	return 0;
}

static DEVICE_API(clock_control, mpfs_clk_api) = {
	.on = mpfs_clock_on,
	.off = mpfs_clock_off,
	.get_status = mpfs_clock_get_status,
	.get_rate = mpfs_clock_get_rate,
	.set_rate = mpfs_clock_set_rate,
};

#define MPFS_CLK_INIT(inst)                                                                        \
	static struct mpfs_clk_data mpfs_clk_data_##inst;                                          \
	static const struct mpfs_clk_config mpfs_clk_cfg_##inst = {                                \
		.clk_type = DT_PROP(DT_DRV_INST(inst), clk_type),                                  \
		.top_sysreg_base = DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(inst), microchip_sysreg)),   \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.frequency_max = DT_PROP(DT_DRV_INST(inst), frequency_max),                        \
		.frequency_min = DT_PROP(DT_DRV_INST(inst), frequency_min),                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, mpfs_clk_init, NULL, &mpfs_clk_data_##inst,                    \
			      &mpfs_clk_cfg_##inst, PRE_KERNEL_1,                                  \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &mpfs_clk_api);

DT_INST_FOREACH_STATUS_OKAY(MPFS_CLK_INIT)
