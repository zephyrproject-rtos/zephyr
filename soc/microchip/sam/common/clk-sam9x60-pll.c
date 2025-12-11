/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <pmc.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clk_pll, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define	PMC_PLL_CTRL0_DIV_MSK	GENMASK(7, 0)
#define	PMC_PLL_CTRL1_MUL_MSK	GENMASK(31, 24)
#define	PMC_PLL_CTRL1_FRACR_MSK	GENMASK(21, 0)

# define do_div(n, base) ({				\
	uint32_t __base = (base);			\
	uint32_t __rem;					\
	__rem = ((uint64_t)(n)) % __base;		\
	(n) = ((uint64_t)(n)) / __base;			\
	__rem;						\
})

#define DIV_ROUND_CLOSEST_ULL(x, divisor)(		\
{							\
	__typeof__(divisor) __d = divisor;		\
	unsigned long long _tmp = (x) + (__d) / 2;	\
	do_div(_tmp, __d);				\
	_tmp;						\
}							\
)

/* Calculate "x * n / d" without unnecessary overflow or loss of precision. */
#define mult_frac(x, n, d)				\
({							\
	__typeof__(x) x_ = (x);				\
	__typeof__(n) n_ = (n);				\
	__typeof__(d) d_ = (d);				\
							\
	__typeof__(x_) q = x_ / d_;			\
	__typeof__(x_) r = x_ % d_;			\
	q * n_ + r * n_ / d_;				\
})

#define PLL_MAX_ID		SOC_NUM_CLOCK_PLL_FRAC

struct sam9x60_pll_core {
	pmc_registers_t *pmc;
	struct k_spinlock *lock;
	const struct clk_pll_characteristics *characteristics;
	const struct clk_pll_layout *layout;
	struct device clk;
	const struct device *parent;
	uint8_t id;
};

struct sam9x60_frac {
	struct sam9x60_pll_core core;
	uint32_t frac;
	uint16_t mul;
};

struct sam9x60_div {
	struct sam9x60_pll_core core;
	uint8_t div;
};

#define to_sam9x60_pll_core(ptr)	CONTAINER_OF(ptr, struct sam9x60_pll_core, clk)
#define to_sam9x60_frac(ptr)		CONTAINER_OF(ptr, struct sam9x60_frac, core)
#define to_sam9x60_div(ptr)		CONTAINER_OF(ptr, struct sam9x60_div, core)

static struct sam9x60_frac clocks_frac[SOC_NUM_CLOCK_PLL_FRAC];
static uint32_t clocks_frac_idx;

static struct sam9x60_div clocks_div[SOC_NUM_CLOCK_PLL_DIV];
static uint32_t clocks_div_idx;

static inline bool sam9x60_pll_ready(pmc_registers_t *pmc, int id)
{
	unsigned int status;

	status = pmc->PMC_PLL_ISR0;

	return !!(status & BIT(id));
}

static int sam9x60_frac_pll_set(struct sam9x60_pll_core *core)
{
	struct sam9x60_frac *frac = to_sam9x60_frac(core);

	pmc_registers_t *pmc = core->pmc;
	uint32_t val, cfrac, cmul;
	k_spinlock_key_t key;

	key = k_spin_lock(core->lock);

	reg_update_bits(pmc->PMC_PLL_UPDT, PMC_PLL_UPDT_ID_Msk, core->id);
	val = pmc->PMC_PLL_CTRL1;
	cmul = (val & core->layout->mul_mask) >> core->layout->mul_shift;
	cfrac = (val & core->layout->frac_mask) >> core->layout->frac_shift;

	if (sam9x60_pll_ready(pmc, core->id) &&
	    (cmul == frac->mul && cfrac == frac->frac)) {
		goto unlock;
	}

	/* Recommended value for PMC_PLL_ACR */
	if (core->characteristics->upll) {
		val = AT91_PMC_PLL_ACR_DEFAULT_UPLL;
	} else {
		val = AT91_PMC_PLL_ACR_DEFAULT_PLLA;
	}
	pmc->PMC_PLL_ACR = val;

	pmc->PMC_PLL_CTRL1 = (frac->mul << core->layout->mul_shift) |
			     (frac->frac << core->layout->frac_shift);

#ifdef PMC_PLL_ACR_UTMIBG_Msk
	if (core->characteristics->upll) {
		/* Enable the UTMI internal bandgap */
		val |= PMC_PLL_ACR_UTMIBG_Msk;
		pmc->PMC_PLL_ACR = val;

		k_busy_wait(10);

		/* Enable the UTMI internal regulator */
		val |= PMC_PLL_ACR_UTMIVR_Msk;
		pmc->PMC_PLL_ACR = val;

		k_busy_wait(10);
	}
#endif

	reg_update_bits(pmc->PMC_PLL_UPDT,
			PMC_PLL_UPDT_UPDATE_Msk | PMC_PLL_UPDT_ID_Msk,
			PMC_PLL_UPDT_UPDATE_Msk | core->id);

	pmc->PMC_PLL_CTRL0 |= PMC_PLL_CTRL0_ENLOCK_Msk | PMC_PLL_CTRL0_ENPLL_Msk;

	reg_update_bits(pmc->PMC_PLL_UPDT,
			PMC_PLL_UPDT_UPDATE_Msk | PMC_PLL_UPDT_ID_Msk,
			PMC_PLL_UPDT_UPDATE_Msk | core->id);

	while (!sam9x60_pll_ready(pmc, core->id)) {
		k_busy_wait(1);
	}

unlock:
	k_spin_unlock(core->lock, key);

	return 0;
}

static int sam9x60_clk_frac_pll_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct sam9x60_pll_core *core = to_sam9x60_pll_core(dev);

	return sam9x60_frac_pll_set(core);
}

static int sam9x60_clk_frac_pll_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct sam9x60_pll_core *core = to_sam9x60_pll_core(dev);
	pmc_registers_t *pmc = core->pmc;
	k_spinlock_key_t key;

	key = k_spin_lock(core->lock);

	reg_update_bits(pmc->PMC_PLL_UPDT, PMC_PLL_UPDT_ID_Msk, core->id);

	reg_update_bits(pmc->PMC_PLL_CTRL0, PMC_PLL_CTRL0_ENPLL_Msk, 0);

#ifdef PMC_PLL_ACR_UTMIBG_Msk
	if (core->characteristics->upll) {
		reg_update_bits(pmc->PMC_PLL_ACR,
				PMC_PLL_ACR_UTMIBG_Msk | PMC_PLL_ACR_UTMIVR_Msk, 0);
	}
#endif

	reg_update_bits(pmc->PMC_PLL_UPDT,
			PMC_PLL_UPDT_UPDATE_Msk | PMC_PLL_UPDT_ID_Msk,
			PMC_PLL_UPDT_UPDATE_Msk | core->id);

	k_spin_unlock(core->lock, key);

	return 0;
}

static enum clock_control_status sam9x60_clk_frac_pll_get_status(const struct device *dev,
								 clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct sam9x60_pll_core *core = to_sam9x60_pll_core(dev);

	if (sam9x60_pll_ready(core->pmc, core->id)) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

static long sam9x60_frac_pll_compute_mul_frac(struct sam9x60_pll_core *core,
					      unsigned long rate,
					      unsigned long parent_rate,
					      bool update)
{
	struct sam9x60_frac *frac = to_sam9x60_frac(core);
	unsigned long tmprate, remainder;
	unsigned long nmul = 0;
	unsigned long nfrac = 0;

	if (rate < core->characteristics->core_output[0].min ||
	    rate > core->characteristics->core_output[0].max) {
		return -ERANGE;
	}

	/*
	 * Calculate the multiplier associated with the current
	 * divider that provide the closest rate to the requested one.
	 */
	nmul = mult_frac(rate, 1, parent_rate);
	tmprate = mult_frac(parent_rate, nmul, 1);
	remainder = rate - tmprate;

	if (remainder) {
		nfrac = DIV_ROUND_CLOSEST_ULL((uint64_t)remainder * (1 << 22),
					      parent_rate);

		tmprate += DIV_ROUND_CLOSEST_ULL((uint64_t)nfrac * parent_rate,
						 (1 << 22));
	}

	/* Check if resulted rate is a valid.  */
	if (tmprate < core->characteristics->core_output[0].min ||
	    tmprate > core->characteristics->core_output[0].max) {
		return -ERANGE;
	}

	if (update) {
		frac->mul = nmul - 1;
		frac->frac = nfrac;
	}

	return tmprate;
}

static int sam9x60_clk_frac_pll_get_rate(const struct device *dev,
					 clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(sys);

	struct sam9x60_pll_core *core = to_sam9x60_pll_core(dev);
	struct sam9x60_frac *frac = to_sam9x60_frac(core);
	int retval = 0;

	retval = clock_control_get_rate(frac->core.parent, NULL, rate);
	if (retval) {
		LOG_ERR("get parent clock rate failed.");
		*rate = 0;
	} else {
		*rate = *rate * (frac->mul + 1) +
			DIV_ROUND_CLOSEST_ULL((uint64_t)*rate * frac->frac, (1<<22));
	}
	LOG_DBG("clk %s: rate %d", dev->name, *rate);

	return retval;
}

static DEVICE_API(clock_control, frac_pll_api) = {
	.on = sam9x60_clk_frac_pll_on,
	.off = sam9x60_clk_frac_pll_off,
	.get_rate = sam9x60_clk_frac_pll_get_rate,
	.get_status = sam9x60_clk_frac_pll_get_status,
};

int sam9x60_clk_register_frac_pll(pmc_registers_t *const pmc, struct k_spinlock *lock,
				  const char *name,
				  const struct device *parent, uint8_t id,
				  const struct clk_pll_characteristics *characteristics,
				  const struct clk_pll_layout *layout, struct device **clk)
{
	struct sam9x60_frac *frac;
	uint32_t parent_rate;
	k_spinlock_key_t key;
	unsigned int val;
	int ret = 0;

	if (id > PLL_MAX_ID || !lock || !parent) {
		return -EINVAL;
	}

	frac = &clocks_frac[clocks_frac_idx++];
	if (clocks_frac_idx > ARRAY_SIZE(clocks_frac)) {
		LOG_ERR("Array for PLL frac clock not enough");
		return -ENOMEM;
	}

	*clk = &frac->core.clk;
	(*clk)->name = name;
	(*clk)->api = &frac_pll_api;
	frac->core.parent = parent;
	frac->core.id = id;
	frac->core.characteristics = characteristics;
	frac->core.layout = layout;
	frac->core.pmc = pmc;
	frac->core.lock = lock;

	key = k_spin_lock(frac->core.lock);
	if (sam9x60_pll_ready(pmc, id)) {
		reg_update_bits(pmc->PMC_PLL_UPDT, PMC_PLL_UPDT_ID_Msk, id);
		val = pmc->PMC_PLL_CTRL1;
		frac->mul = FIELD_GET(PMC_PLL_CTRL1_MUL_MSK, val);
		frac->frac = FIELD_GET(PMC_PLL_CTRL1_FRACR_MSK, val);
	} else {
		/*
		 * This means the PLL is not setup by bootloaders. In this
		 * case we need to set the minimum rate for it. Otherwise
		 * a clock child of this PLL may be enabled before setting
		 * its rate leading to enabling this PLL with unsupported
		 * rate. This will lead to PLL not being locked at all.
		 */
		ret = clock_control_get_rate(parent, NULL, &parent_rate);
		if (ret) {
			LOG_ERR("get parent clock rate failed.");
			goto free;
		}

		long tmp = sam9x60_frac_pll_compute_mul_frac(&frac->core,
							     characteristics->core_output[0].min,
							     parent_rate, true);
		if (tmp < 0) {
			ret = -ENOTSUP;
			goto free;
		}
	}
	k_spin_unlock(frac->core.lock, key);

	return ret;

free:
	k_spin_unlock(frac->core.lock, key);
	k_free(frac);
	return ret;
}

/* This function should be called with spinlock acquired. */
static void sam9x60_div_pll_set_div(struct sam9x60_pll_core *core, uint32_t div, bool enable)
{
	pmc_registers_t *pmc = core->pmc;
	uint32_t ena_msk = enable ? core->layout->endiv_mask : 0;
	uint32_t ena_val = enable ? (1 << core->layout->endiv_shift) : 0;

	reg_update_bits(pmc->PMC_PLL_CTRL0,
			core->layout->div_mask | ena_msk,
			(div << core->layout->div_shift) | ena_val);

	reg_update_bits(pmc->PMC_PLL_UPDT,
			PMC_PLL_UPDT_UPDATE_Msk | PMC_PLL_UPDT_ID_Msk,
			PMC_PLL_UPDT_UPDATE_Msk | core->id);

	while (!sam9x60_pll_ready(pmc, core->id)) {
		k_busy_wait(1);
	}
}

static int sam9x60_div_pll_set(struct sam9x60_pll_core *core)
{
	struct sam9x60_div *div = to_sam9x60_div(core);
	pmc_registers_t *pmc = core->pmc;
	k_spinlock_key_t key;
	uint32_t val, cdiv;

	key = k_spin_lock(core->lock);
	reg_update_bits(pmc->PMC_PLL_UPDT, PMC_PLL_UPDT_ID_Msk, core->id);
	val = pmc->PMC_PLL_CTRL0;
	cdiv = (val & core->layout->div_mask) >> core->layout->div_shift;

	/* Stop if enabled an nothing changed. */
	if (!!(val & core->layout->endiv_mask) && cdiv == div->div) {
		goto unlock;
	}

	sam9x60_div_pll_set_div(core, div->div, 1);

unlock:
	k_spin_unlock(core->lock, key);

	return 0;
}

static int sam9x60_clk_div_pll_on(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct sam9x60_pll_core *core = to_sam9x60_pll_core(dev);

	return sam9x60_div_pll_set(core);
}

static int sam9x60_clk_div_pll_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct sam9x60_pll_core *core = to_sam9x60_pll_core(dev);
	pmc_registers_t *pmc = core->pmc;
	k_spinlock_key_t key;

	key = k_spin_lock(core->lock);

	reg_update_bits(pmc->PMC_PLL_UPDT, PMC_PLL_UPDT_ID_Msk, core->id);

	reg_update_bits(pmc->PMC_PLL_CTRL0, core->layout->endiv_mask, 0);

	reg_update_bits(pmc->PMC_PLL_UPDT,
			PMC_PLL_UPDT_UPDATE_Msk | PMC_PLL_UPDT_ID_Msk,
			PMC_PLL_UPDT_UPDATE_Msk | core->id);

	k_spin_unlock(core->lock, key);

	return 0;
}

static enum clock_control_status sam9x60_clk_div_pll_get_status(const struct device *dev,
								clock_control_subsys_t sys)
{
	ARG_UNUSED(sys);

	struct sam9x60_pll_core *core = to_sam9x60_pll_core(dev);
	pmc_registers_t *pmc = core->pmc;
	k_spinlock_key_t key;
	uint32_t val;

	key = k_spin_lock(core->lock);

	reg_update_bits(pmc->PMC_PLL_UPDT, PMC_PLL_UPDT_ID_Msk, core->id);
	val = pmc->PMC_PLL_CTRL0;

	k_spin_unlock(core->lock, key);

	if (!!(val & core->layout->endiv_mask)) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

static int sam9x60_clk_div_pll_get_rate(const struct device *dev,
					clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(sys);

	struct sam9x60_pll_core *core = to_sam9x60_pll_core(dev);
	struct sam9x60_div *div = to_sam9x60_div(core);
	int retval = 0;

	retval = clock_control_get_rate(div->core.parent, NULL, rate);
	if (retval) {
		LOG_ERR("get parent clock rate failed.");
		*rate = 0;
	} else {
		*rate = DIV_ROUND_CLOSEST_ULL(*rate, div->div + 1);
	}
	LOG_DBG("clk %s: rate %d", dev->name, *rate);

	return retval;
}

static DEVICE_API(clock_control, div_pll_api) = {
	.on = sam9x60_clk_div_pll_on,
	.off = sam9x60_clk_div_pll_off,
	.get_rate = sam9x60_clk_div_pll_get_rate,
	.get_status = sam9x60_clk_div_pll_get_status,
};

int sam9x60_clk_register_div_pll(pmc_registers_t *const pmc, struct k_spinlock *lock,
				 const char *name,
				 const struct device *parent, uint8_t id,
				 const struct clk_pll_characteristics *characteristics,
				 const struct clk_pll_layout *layout,
				 struct device **clk)
{
	struct sam9x60_div *div;
	k_spinlock_key_t key;
	uint32_t val;

	/* We only support one changeable PLL. */
	if (id > PLL_MAX_ID || !lock || !parent) {
		return -EINVAL;
	}

	div = &clocks_div[clocks_div_idx++];
	if (clocks_div_idx > ARRAY_SIZE(clocks_div)) {
		LOG_ERR("Array for PLL div clock not enough");
		return -ENOMEM;
	}

	*clk = &div->core.clk;
	(*clk)->name = name;
	(*clk)->api = &div_pll_api;
	div->core.parent = parent;
	div->core.id = id;
	div->core.characteristics = characteristics;
	div->core.layout = layout;
	div->core.pmc = pmc;
	div->core.lock = lock;

	key = k_spin_lock(div->core.lock);

	reg_update_bits(pmc->PMC_PLL_UPDT, PMC_PLL_UPDT_ID_Msk, id);
	val = pmc->PMC_PLL_CTRL0;
	div->div = FIELD_GET(PMC_PLL_CTRL0_DIV_MSK, val);

	k_spin_unlock(div->core.lock, key);

	return 0;
}
