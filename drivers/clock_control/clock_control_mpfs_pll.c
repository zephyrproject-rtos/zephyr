/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mpfs_pll

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_mpfs_pll.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(clock_control_mpfs_pll, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define MPFS_PLL_OUT_MAX_POSTDIV 127U

struct mpfs_pll_config {
	uintptr_t pll_base;             /* base address of PLL registers */
	const struct device *clock_dev; /* pointer to the parent clock device */
};

struct mpfs_pll_data {
	struct k_spinlock *shared_lock; /* shared lock for this PLL instance */
};

struct mpfs_pll_out_config {
	uintptr_t pll_base;             /* base address of PLL registers */
	const struct device *clock_dev; /* pointer to the parent clock device */
	uint32_t pll_out_idx;           /* PLL output index (0..3) */
	uint32_t frequency_max;         /* Maximum frequency for this PLL output */
	uint32_t frequency_min;         /* Minimum frequency for this PLL output */
	uint32_t frequency_step;        /* Minimum frequency for this PLL output */
};

struct mpfs_pll_out_data {
	struct k_spinlock *shared_lock; /* shared lock for this PLL instance */
};

/******************************************************************************/
/*                                PLL CORE                                    */
/******************************************************************************/

/**
 * @brief Report the current status of the PLL core.
 *
 * @param dev MPFS clock controller device.
 * @param sys Clock subsystem selector encoded as a gate identifier.
 *
 * @return Current gate status, or CLOCK_CONTROL_STATUS_UNKNOWN if @p sys is invalid.
 */
static enum clock_control_status mpfs_clock_pll_get_status(const struct device *dev,
							   clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);
	const struct mpfs_pll_config *cfg = (const struct mpfs_pll_config *)dev->config;
	const struct mpfs_pll_data *data = (const struct mpfs_pll_data *)dev->data;
	k_spinlock_key_t key;
	uint32_t pll_ctrl;

	key = k_spin_lock(data->shared_lock);
	pll_ctrl = sys_read32(cfg->pll_base + MPFS_PLL_PLL_CTRL_OFFSET);
	k_spin_unlock(data->shared_lock, key);
	if (!(pll_ctrl & MPFS_PLL_PLL_CTRL_REG_POWERDOWN_B_MASK)) {
		return CLOCK_CONTROL_STATUS_OFF;
	}

	if (!(pll_ctrl & MPFS_PLL_PLL_CTRL_LOCK_B_MASK) &&
	    (pll_ctrl & MPFS_PLL_PLL_CTRL_LOCK_MASK)) {
		return CLOCK_CONTROL_STATUS_ON;
	} else if ((pll_ctrl & MPFS_PLL_PLL_CTRL_LOCK_B_MASK) &&
		   !(pll_ctrl & MPFS_PLL_PLL_CTRL_LOCK_MASK)) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

/**
 * @brief Power up the PLL core and wait for lock.
 *
 * @param dev MPFS clock controller device.
 * @param sys Clock subsystem selector encoded as a gate identifier.
 *
 * @retval 0 PLL core is enabled.
 */
static int mpfs_clock_pll_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);
	const struct mpfs_pll_config *cfg = (const struct mpfs_pll_config *)dev->config;
	const struct mpfs_pll_data *data = (const struct mpfs_pll_data *)dev->data;
	k_spinlock_key_t key;
	uint32_t pll_ctrl;

	key = k_spin_lock(data->shared_lock);
	pll_ctrl = sys_read32(cfg->pll_base + MPFS_PLL_PLL_CTRL_OFFSET);
	if (pll_ctrl & MPFS_PLL_PLL_CTRL_REG_POWERDOWN_B_MASK) {
		k_spin_unlock(data->shared_lock, key);
		goto check_lock;
	}

	pll_ctrl |= MPFS_PLL_PLL_CTRL_REG_POWERDOWN_B_MASK;
	sys_write32(pll_ctrl, cfg->pll_base + MPFS_PLL_PLL_CTRL_OFFSET);
	k_spin_unlock(data->shared_lock, key);

check_lock:
	/* Wait for LOCK to assert */
	while (1) {
		pll_ctrl = sys_read32(cfg->pll_base + MPFS_PLL_PLL_CTRL_OFFSET);
		if (!(pll_ctrl & MPFS_PLL_PLL_CTRL_LOCK_B_MASK) &&
		    (pll_ctrl & MPFS_PLL_PLL_CTRL_LOCK_MASK)) {
			break;
		} else if ((pll_ctrl & MPFS_PLL_PLL_CTRL_LOCK_B_MASK) &&
			   !(pll_ctrl & MPFS_PLL_PLL_CTRL_LOCK_MASK)) {
			break;
		}
		k_busy_wait(1);
	}

	return 0;
}

/**
 * @brief Power down the PLL core.
 *
 * @param dev MPFS clock controller device.
 * @param sys Clock subsystem selector encoded as a gate identifier.
 *
 * @retval 0 PLL core is disabled.
 */
static int mpfs_clock_pll_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);
	const struct mpfs_pll_config *cfg = (const struct mpfs_pll_config *)dev->config;
	const struct mpfs_pll_data *data = (const struct mpfs_pll_data *)dev->data;
	k_spinlock_key_t key;
	uint32_t pll_ctrl;

	key = k_spin_lock(data->shared_lock);
	pll_ctrl = sys_read32(cfg->pll_base + MPFS_PLL_PLL_CTRL_OFFSET);
	if (!(pll_ctrl & MPFS_PLL_PLL_CTRL_REG_POWERDOWN_B_MASK)) {
		k_spin_unlock(data->shared_lock, key);
		return 0;
	}

	pll_ctrl &= ~(MPFS_PLL_PLL_CTRL_REG_POWERDOWN_B_MASK);
	sys_write32(pll_ctrl, cfg->pll_base + MPFS_PLL_PLL_CTRL_OFFSET);
	k_spin_unlock(data->shared_lock, key);

	return 0;
}

/**
 * @brief Query the rate of the PLL core output.
 *
 * @param dev MPFS clock controller device.
 * @param sys Clock subsystem selector.
 * @param rate Output pointer for the resolved clock frequency in hertz.
 *
 * @retval 0 Rate written to @p rate.
 * @retval -EIO Invalid divider state encountered while computing rate.
 */
static int mpfs_clock_pll_get_rate(const struct device *dev, clock_control_subsys_t sys,
				   uint32_t *rate)
{
	ARG_UNUSED(sys);
	const struct mpfs_pll_config *cfg = (const struct mpfs_pll_config *)dev->config;
	const struct mpfs_pll_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t parent_clk_rate, ref_fb, sscg_2, intin, refdiv, pllctrl, pll_fracn, fracin = 0U;
	uint64_t feedback_div_scaled;
	uint64_t calc_rate;
	int ret;

	ret = clock_control_get_rate(cfg->clock_dev, 0, &parent_clk_rate);
	if (ret != 0) {
		return ret;
	}

	key = k_spin_lock(data->shared_lock);

	/* get rfdiv */
	pllctrl = sys_read32(cfg->pll_base + MPFS_PLL_PLL_CTRL_OFFSET);
	if ((pllctrl & MPFS_PLL_PLL_CTRL_REG_RFDIV_EN_MASK) == 0U) {
		LOG_INF("PLL REFDIV is disabled (RFDIV_EN=0), refdiv treated as 1");
		refdiv = 1U;
	} else {
		ref_fb = sys_read32(cfg->pll_base + MPFS_PLL_PLL_REF_FB_OFFSET);
		refdiv = FIELD_GET(MPFS_PLL_PLL_REF_FB_RFDIV_MASK, ref_fb);
	}

	/* get intin */
	sscg_2 = sys_read32(cfg->pll_base + MPFS_PLL_SSCG_REG_2_OFFSET);
	intin = FIELD_GET(MPFS_PLL_SSCG_REG_2_INTIN_MASK, sscg_2);

	if (intin == 0 || refdiv == 0) {
		k_spin_unlock(data->shared_lock, key);
		return -EAGAIN;
	}

	/* get fracin */
	pll_fracn = sys_read32(cfg->pll_base + MPFS_PLL_PLL_FRACN_OFFSET);
	if ((pll_fracn & MPFS_PLL_PLL_FRACN_FRACN_EN_MASK) != 0U) {
		uint32_t sscg_0 = sys_read32(cfg->pll_base + MPFS_PLL_SSCG_REG_0_OFFSET);

		fracin = FIELD_GET(MPFS_PLL_SSCG_REG_0_FRACIN_MASK, sscg_0);
	}

	k_spin_unlock(data->shared_lock, key);

	feedback_div_scaled = ((uint64_t)intin << MPFS_PLL_FRACIN_Q24_SHIFT) + fracin;

	/*
	 * VCO = parent * (INTIN + FRACIN / 2^24) / REFDIV
	 * OUT = VCO / 4
	 */
	calc_rate = (uint64_t)parent_clk_rate * feedback_div_scaled;
	calc_rate /=
		(uint64_t)refdiv * MPFS_PLL_FIXED_DIV * (UINT64_C(1) << MPFS_PLL_FRACIN_Q24_SHIFT);

	*rate = (uint32_t)calc_rate;

	return 0;
}

/******************************************************************************/
/*                                  PLL OUT                                   */
/******************************************************************************/

/**
 * @brief Get the status of the PLL out clock.
 *
 * @param dev MPFS clock controller device.
 * @param sys Clock subsystem selector encoded as a gate identifier.
 *
 * @retval CLOCK_CONTROL_STATUS_ON The PLL out is enabled.
 * @retval CLOCK_CONTROL_STATUS_OFF The PLL out is disabled.
 */
static enum clock_control_status mpfs_clock_pllout_get_status(const struct device *dev,
							      clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);
	const struct mpfs_pll_out_config *cfg = (const struct mpfs_pll_out_config *)dev->config;
	const struct mpfs_pll_out_data *data = (const struct mpfs_pll_out_data *)dev->data;
	k_spinlock_key_t key;
	uint32_t pll_ctrl;

	key = k_spin_lock(data->shared_lock);
	pll_ctrl = sys_read32(cfg->pll_base + MPFS_PLL_PLL_CTRL_OFFSET);
	k_spin_unlock(data->shared_lock, key);
	if (pll_ctrl & BIT(cfg->pll_out_idx + MPFS_PLL_PLL_CTRL_REG_DIVQ_EN_SHIFT)) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

/**
 * @brief Enable one PLL output gate.
 *
 * @param dev MPFS clock controller device.
 * @param sys Clock subsystem selector encoded as a gate identifier.
 *
 * @retval 0 PLL output gate enabled successfully.
 */
static int mpfs_clock_pllout_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);
	const struct mpfs_pll_out_config *cfg = (const struct mpfs_pll_out_config *)dev->config;
	const struct mpfs_pll_out_data *data = (const struct mpfs_pll_out_data *)dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(data->shared_lock);
	sys_set_bit(cfg->pll_base + MPFS_PLL_PLL_CTRL_OFFSET,
		    cfg->pll_out_idx + MPFS_PLL_PLL_CTRL_REG_DIVQ_EN_SHIFT);
	k_spin_unlock(data->shared_lock, key);

	return 0;
}

/**
 * @brief Disable one PLL output gate clock (glitchless).
 *
 * @param dev MPFS clock controller device.
 * @param sys Clock subsystem selector encoded as a gate identifier.
 *
 * @retval 0 PLL output gate disabled successfully.
 */
static int mpfs_clock_pllout_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);
	const struct mpfs_pll_out_config *cfg = (const struct mpfs_pll_out_config *)dev->config;
	const struct mpfs_pll_out_data *data = (const struct mpfs_pll_out_data *)dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(data->shared_lock);
	sys_clear_bit(cfg->pll_base + MPFS_PLL_PLL_CTRL_OFFSET,
		      cfg->pll_out_idx + MPFS_PLL_PLL_CTRL_REG_DIVQ_EN_SHIFT);
	k_spin_unlock(data->shared_lock, key);

	return 0;
}

/**
 * @brief Query the rate of one PLL output.
 *
 * @param dev MPFS clock controller device.
 * @param sys Clock subsystem selector.
 * @param rate Output pointer for the resolved clock frequency in hertz.
 *
 * @retval 0 Rate written to @p rate.
 * @retval -EAGAIN The selected PLL output is disabled.
 * @retval -ENOSYS Invalid PLL output index.
 */
static int mpfs_clock_pllout_get_rate(const struct device *dev, clock_control_subsys_t sys,
				      uint32_t *rate)
{
	ARG_UNUSED(sys);
	const struct mpfs_pll_out_config *cfg = (const struct mpfs_pll_out_config *)dev->config;
	const struct mpfs_pll_out_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t parent_clk_rate, pll_ctrl, post01, post23, clksel, outdiv2, outdiv3, divq_x_en,
		postdiv;
	int ret;

	key = k_spin_lock(data->shared_lock);
	pll_ctrl = sys_read32(cfg->pll_base + MPFS_PLL_PLL_CTRL_OFFSET);

	divq_x_en = FIELD_GET(MPFS_PLL_PLL_CTRL_REG_DIVQ_EN_MASK, pll_ctrl);
	if ((divq_x_en & BIT(cfg->pll_out_idx)) == 0U) {
		k_spin_unlock(data->shared_lock, key);
		LOG_WRN("PLL output %u is disabled (DIVQx_EN=0x%x)", cfg->pll_out_idx, divq_x_en);
		return -EAGAIN;
	}

	post01 = sys_read32(cfg->pll_base + MPFS_PLL_PLL_DIV_0_1_OFFSET);
	post23 = sys_read32(cfg->pll_base + MPFS_PLL_PLL_DIV_2_3_OFFSET);

	switch (cfg->pll_out_idx) {
	case 0:
		postdiv = FIELD_GET(MPFS_PLL_PLL_DIV_0_1_POST0DIV_MASK, post01);
		break;
	case 1:
		postdiv = FIELD_GET(MPFS_PLL_PLL_DIV_0_1_POST1DIV_MASK, post01);
		break;
	case 2:
		postdiv = FIELD_GET(MPFS_PLL_PLL_DIV_2_3_POST2DIV_MASK, post23);
		break;
	case 3:
		clksel = FIELD_GET(MPFS_PLL_PLL_DIV_2_3_CKPOST3_SEL_MASK, post23);
		if (clksel == 1) {
			outdiv2 = FIELD_GET(MPFS_PLL_PLL_DIV_2_3_POST2DIV_MASK, post23);
			outdiv3 = FIELD_GET(MPFS_PLL_PLL_DIV_2_3_POST3DIV_MASK, post23);
			postdiv = outdiv2 * outdiv3;
			break;
		}
		postdiv = FIELD_GET(MPFS_PLL_PLL_DIV_2_3_POST3DIV_MASK, post23);
		break;
	default:
		k_spin_unlock(data->shared_lock, key);
		return -ENOSYS;
	}

	k_spin_unlock(data->shared_lock, key);

	ret = clock_control_get_rate(cfg->clock_dev, 0, &parent_clk_rate);
	if (ret != 0) {
		return ret;
	}

	*rate = parent_clk_rate / postdiv;

	return 0;
}

static int mpfs_pllout_write_postdiv(const struct device *dev, uint32_t postdiv)
{
	const struct mpfs_pll_out_config *cfg = (const struct mpfs_pll_out_config *)dev->config;
	const struct mpfs_pll_out_data *data = (const struct mpfs_pll_out_data *)dev->data;
	k_spinlock_key_t key;
	uint32_t reg;

	key = k_spin_lock(data->shared_lock);

	switch (cfg->pll_out_idx) {
	case 0:
		reg = sys_read32(cfg->pll_base + MPFS_PLL_PLL_DIV_0_1_OFFSET);
		reg &= ~MPFS_PLL_PLL_DIV_0_1_POST0DIV_MASK;
		reg |= FIELD_PREP(MPFS_PLL_PLL_DIV_0_1_POST0DIV_MASK, postdiv);
		sys_write32(reg, cfg->pll_base + MPFS_PLL_PLL_DIV_0_1_OFFSET);
		k_spin_unlock(data->shared_lock, key);
		return 0;

	case 1:
		reg = sys_read32(cfg->pll_base + MPFS_PLL_PLL_DIV_0_1_OFFSET);
		reg &= ~MPFS_PLL_PLL_DIV_0_1_POST1DIV_MASK;
		reg |= FIELD_PREP(MPFS_PLL_PLL_DIV_0_1_POST1DIV_MASK, postdiv);
		sys_write32(reg, cfg->pll_base + MPFS_PLL_PLL_DIV_0_1_OFFSET);
		k_spin_unlock(data->shared_lock, key);
		return 0;

	case 2:
		reg = sys_read32(cfg->pll_base + MPFS_PLL_PLL_DIV_2_3_OFFSET);
		reg &= ~MPFS_PLL_PLL_DIV_2_3_POST2DIV_MASK;
		reg |= FIELD_PREP(MPFS_PLL_PLL_DIV_2_3_POST2DIV_MASK, postdiv);
		sys_write32(reg, cfg->pll_base + MPFS_PLL_PLL_DIV_2_3_OFFSET);
		k_spin_unlock(data->shared_lock, key);
		return 0;

	case 3:
		reg = sys_read32(cfg->pll_base + MPFS_PLL_PLL_DIV_2_3_OFFSET);
		reg &= ~MPFS_PLL_PLL_DIV_2_3_CKPOST3_SEL_MASK;
		reg &= ~MPFS_PLL_PLL_DIV_2_3_POST3DIV_MASK;
		reg |= FIELD_PREP(MPFS_PLL_PLL_DIV_2_3_POST3DIV_MASK, postdiv);
		sys_write32(reg, cfg->pll_base + MPFS_PLL_PLL_DIV_2_3_OFFSET);
		k_spin_unlock(data->shared_lock, key);
		return 0;

	default:
		k_spin_unlock(data->shared_lock, key);
		return -ENOSYS;
	}
}

/**
 * @brief Change a supported derived clock rate by updating pll out divider fields.
 *
 * @param dev MPFS clock controller device.
 * @param sys Clock subsystem selector.
 * @param target_rate_hz Requested target rate encoded as a subsystem-rate value.
 *
 * @retval 0 Divider updated successfully.
 * @retval -EIO The selected parent PLL output rate could not be determined.
 * @retval -EINVAL The requested rate cannot be encoded for the selected clock.
 */
static int mpfs_clock_pllout_set_rate(const struct device *dev, clock_control_subsys_t sys,
				      clock_control_subsys_rate_t target_rate_hz)
{
	ARG_UNUSED(sys);
	const struct mpfs_pll_out_config *cfg = (const struct mpfs_pll_out_config *)dev->config;
	uint32_t parent_clk_rate, target_rate, postdiv;
	int ret;

	target_rate = (uint32_t)(uintptr_t)target_rate_hz;

	ret = clock_control_get_rate(cfg->clock_dev, 0, &parent_clk_rate);
	if (ret != 0) {
		return ret;
	}

	if (parent_clk_rate == 0) {
		return -EIO;
	}

	if (target_rate == 0U) {
		return -EINVAL;
	}

	if ((target_rate > cfg->frequency_max) || (target_rate < cfg->frequency_min) ||
	    ((target_rate % cfg->frequency_step) != 0U)) {
		return -EINVAL;
	}

	if ((parent_clk_rate % target_rate) != 0U) {
		LOG_ERR("parent_clk_rate %u is not divisible by target_rate %u", parent_clk_rate,
			target_rate);
		return -EINVAL;
	}

	postdiv = parent_clk_rate / target_rate;

	if (postdiv == 0U) {
		return -EINVAL;
	}

	if (postdiv > MPFS_PLL_OUT_MAX_POSTDIV) {
		return -EINVAL;
	}

	ret = mpfs_pllout_write_postdiv(dev, postdiv);
	return ret;
}

/**
 * @brief Initialize the MPFS PLL and PLL output clock drivers.
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

static DEVICE_API(clock_control, mpfs_pll_api) = {
	.on = mpfs_clock_pll_on,
	.off = mpfs_clock_pll_off,
	.get_status = mpfs_clock_pll_get_status,
	.get_rate = mpfs_clock_pll_get_rate,
};

static DEVICE_API(clock_control, mpfs_pllout_api) = {
	.on = mpfs_clock_pllout_on,
	.off = mpfs_clock_pllout_off,
	.get_status = mpfs_clock_pllout_get_status,
	.get_rate = mpfs_clock_pllout_get_rate,
	.set_rate = mpfs_clock_pllout_set_rate,
};

#define MPFS_PLL_OUT_INIT(node, inst)                                                              \
	static struct mpfs_pll_out_data mpfs_pll_out_data_##node = {                               \
		.shared_lock = &mpfs_pll_lock_##inst,                                              \
	};                                                                                         \
	static const struct mpfs_pll_out_config mpfs_pll_out_config_##node = {                     \
		.pll_base = DT_REG_ADDR(DT_PARENT(node)),                                          \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(node)),                                  \
		.pll_out_idx = DT_REG_ADDR(node),                                                  \
		.frequency_max = DT_PROP(node, frequency_max),                                     \
		.frequency_min = DT_PROP(node, frequency_min),                                     \
		.frequency_step = DT_PROP(node, frequency_step),                                   \
	};                                                                                         \
	DEVICE_DT_DEFINE(node, mpfs_clk_init, NULL, &mpfs_pll_out_data_##node,                     \
			 &mpfs_pll_out_config_##node, PRE_KERNEL_1,                                \
			 CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &mpfs_pllout_api);

#define MPFS_PLL_INIT(inst)                                                                        \
	static struct k_spinlock mpfs_pll_lock_##inst;                                             \
	static struct mpfs_pll_data mpfs_pll_data_##inst = {                                       \
		.shared_lock = &mpfs_pll_lock_##inst,                                              \
	};                                                                                         \
	DT_INST_FOREACH_CHILD_VARGS(inst, MPFS_PLL_OUT_INIT, inst);                                \
	static const struct mpfs_pll_config mpfs_pll_config_##inst = {                             \
		.pll_base = DT_REG_ADDR(DT_DRV_INST(inst)),                                        \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, mpfs_clk_init, NULL, &mpfs_pll_data_##inst,                    \
			      &mpfs_pll_config_##inst, PRE_KERNEL_1,                               \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &mpfs_pll_api);

DT_INST_FOREACH_STATUS_OKAY(MPFS_PLL_INIT)
