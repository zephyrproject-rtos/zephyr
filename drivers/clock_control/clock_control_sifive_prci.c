/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SiFive FU540 Power Reset Clock Interrupt block (PRCI), the clock controller.
 */

#define DT_DRV_COMPAT sifive_fu540_c000_prci

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/sifive-fu540-prci.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(clock_control_sifive_prci, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

/* Registers (FU540-C000 manual, chapter 7) */
#define PRCI_COREPLLCFG0   0x04
#define PRCI_COREPLLCFG1   0x08
#define PRCI_DDRPLLCFG0    0x0c
#define PRCI_DDRPLLCFG1    0x10
#define PRCI_GEMGXLPLLCFG0 0x1c
#define PRCI_GEMGXLPLLCFG1 0x20
#define PRCI_CORECLKSEL    0x24

/*
 * The PLLs are Analog Bits wide-range PLLs with internal feedback:
 * f_out = f_ref / (DIVR + 1) * 2 * (DIVF + 1) / 2^DIVQ
 */
#define PLLCFG0_DIVR   GENMASK(5, 0)
#define PLLCFG0_DIVF   GENMASK(14, 6)
#define PLLCFG0_DIVQ   GENMASK(17, 15)
#define PLLCFG0_BYPASS BIT(24)

/* Output clock enable */
#define PLLCFG1_CKE BIT(31)

/* The core clock is the reference clock instead of the core PLL */
#define CORECLKSEL_HFCLK BIT(0)

struct sifive_prci_pll {
	uint8_t cfg0;
	uint8_t cfg1;
};

static const struct sifive_prci_pll sifive_prci_plls[] = {
	[FU540_PRCI_CLK_COREPLL] = {.cfg0 = PRCI_COREPLLCFG0, .cfg1 = PRCI_COREPLLCFG1},
	[FU540_PRCI_CLK_DDRPLL] = {.cfg0 = PRCI_DDRPLLCFG0, .cfg1 = PRCI_DDRPLLCFG1},
	[FU540_PRCI_CLK_GEMGXLPLL] = {.cfg0 = PRCI_GEMGXLPLLCFG0, .cfg1 = PRCI_GEMGXLPLLCFG1},
};

struct sifive_prci_config {
	DEVICE_MMIO_ROM;
	const struct device *hfclk;
};

struct sifive_prci_data {
	DEVICE_MMIO_RAM;
	struct k_spinlock lock;
};

static int sifive_prci_pll_rate(const struct device *dev, const struct sifive_prci_pll *pll,
				uint32_t *rate)
{
	const struct sifive_prci_config *cfg = dev->config;
	uint32_t cfg0 = sys_read32(DEVICE_MMIO_GET(dev) + pll->cfg0);
	uint32_t hfclk_rate;
	uint64_t vco;
	int ret;

	ret = clock_control_get_rate(cfg->hfclk, NULL, &hfclk_rate);
	if (ret < 0) {
		return ret;
	}

	if ((cfg0 & PLLCFG0_BYPASS) != 0U) {
		*rate = hfclk_rate;
		return 0;
	}

	vco = (uint64_t)hfclk_rate * 2U * (FIELD_GET(PLLCFG0_DIVF, cfg0) + 1U) /
	      (FIELD_GET(PLLCFG0_DIVR, cfg0) + 1U);
	*rate = (uint32_t)(vco >> FIELD_GET(PLLCFG0_DIVQ, cfg0));

	return 0;
}

/* The TileLink clock is half the core clock, the core PLL or the reference clock */
static int sifive_prci_tlclk_rate(const struct device *dev, uint32_t *rate)
{
	const struct sifive_prci_config *cfg = dev->config;
	uint32_t src_rate;
	int ret;

	if ((sys_read32(DEVICE_MMIO_GET(dev) + PRCI_CORECLKSEL) & CORECLKSEL_HFCLK) != 0U) {
		ret = clock_control_get_rate(cfg->hfclk, NULL, &src_rate);
	} else {
		ret = sifive_prci_pll_rate(dev, &sifive_prci_plls[FU540_PRCI_CLK_COREPLL],
					   &src_rate);
	}
	if (ret < 0) {
		return ret;
	}

	*rate = src_rate / 2U;

	return 0;
}

static int sifive_prci_on_off(const struct device *dev, clock_control_subsys_t sys, bool on)
{
	struct sifive_prci_data *data = dev->data;
	uint32_t id = (uint32_t)(uintptr_t)sys;
	k_spinlock_key_t key;
	mm_reg_t reg;
	uint32_t val;

	if (id == FU540_PRCI_CLK_TLCLK) {
		/* Not gated, the system runs from it */
		return on ? 0 : -ENOTSUP;
	}

	if (id >= ARRAY_SIZE(sifive_prci_plls)) {
		return -ENOTSUP;
	}

	reg = DEVICE_MMIO_GET(dev) + sifive_prci_plls[id].cfg1;

	key = k_spin_lock(&data->lock);

	val = sys_read32(reg);
	if (on) {
		val |= PLLCFG1_CKE;
	} else {
		val &= ~PLLCFG1_CKE;
	}
	sys_write32(val, reg);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int sifive_prci_on(const struct device *dev, clock_control_subsys_t sys)
{
	return sifive_prci_on_off(dev, sys, true);
}

static int sifive_prci_off(const struct device *dev, clock_control_subsys_t sys)
{
	return sifive_prci_on_off(dev, sys, false);
}

static int sifive_prci_get_rate(const struct device *dev, clock_control_subsys_t sys,
				uint32_t *rate)
{
	uint32_t id = (uint32_t)(uintptr_t)sys;

	if (id == FU540_PRCI_CLK_TLCLK) {
		return sifive_prci_tlclk_rate(dev, rate);
	}

	if (id >= ARRAY_SIZE(sifive_prci_plls)) {
		return -ENOTSUP;
	}

	return sifive_prci_pll_rate(dev, &sifive_prci_plls[id], rate);
}

static int sifive_prci_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	return 0;
}

static DEVICE_API(clock_control, sifive_prci_api) = {
	.on = sifive_prci_on,
	.off = sifive_prci_off,
	.get_rate = sifive_prci_get_rate,
};

#define SIFIVE_PRCI_DEVICE(n)                                                                      \
	BUILD_ASSERT(DT_INST_CLOCKS_HAS_NAME(n, hfclk), "the hfclk clock is required");            \
                                                                                                   \
	static const struct sifive_prci_config sifive_prci_config_##n = {                          \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.hfclk = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, hfclk)),                     \
	};                                                                                         \
                                                                                                   \
	static struct sifive_prci_data sifive_prci_data_##n;                                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, sifive_prci_init, NULL, &sifive_prci_data_##n,                    \
			      &sifive_prci_config_##n, PRE_KERNEL_1,                               \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &sifive_prci_api);

DT_INST_FOREACH_STATUS_OKAY(SIFIVE_PRCI_DEVICE)
