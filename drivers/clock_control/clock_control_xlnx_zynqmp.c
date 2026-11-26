/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xilinx ZynqMP clock controller, the CRL_APB and CRF_APB registers.
 */

#define DT_DRV_COMPAT xlnx_zynqmp_clk

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/xlnx-zynqmp-clk.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(clock_control_xlnx_zynqmp, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

/* Required by the named DEVICE_MMIO macros */
#define DEV_CFG(dev)  ((const struct zynqmp_clk_config *)(dev)->config)
#define DEV_DATA(dev) ((struct zynqmp_clk_data *)(dev)->data)

/* CRL_APB, the clocks of the low power domain (UG1087) */
#define CRL_APB_WPROT            0x1c
#define CRL_APB_IOPLL_CTRL       0x20
#define CRL_APB_RPLL_CTRL        0x30
#define CRL_APB_GEM_REF_CTRL(n)  (0x50 + (4 * (n)))
#define CRL_APB_LPD_LSBUS_CTRL   0xac
#define CRL_APB_GEM_TSU_REF_CTRL 0x100

/* CRF_APB, the clocks of the full power domain */
#define CRF_APB_DPLL_CTRL        0x2c
#define CRF_APB_DPLL_TO_LPD_CTRL 0x4c

#define WPROT_ACTIVE BIT(0)

#define PLL_CTRL_RESET    BIT(0)
#define PLL_CTRL_BYPASS   BIT(3)
#define PLL_CTRL_FBDIV    GENMASK(14, 8)
#define PLL_CTRL_DIV2     BIT(16)
#define PLL_CTRL_PRE_SRC  GENMASK(22, 20)
#define PLL_CTRL_POST_SRC GENMASK(26, 24)

#define PLL_TO_LPD_CTRL_DIVISOR0 GENMASK(13, 8)

/* Peripheral clocks: a PLL through one or two dividers and a gate */
#define CLK_CTRL_SRCSEL        GENMASK(2, 0)
#define CLK_CTRL_DIVISOR0      GENMASK(13, 8)
#define CLK_CTRL_DIVISOR1      GENMASK(21, 16)
#define CLK_CTRL_CLKACT        BIT(24)
#define GEM_REF_CTRL_CLKACT    BIT(25)
#define GEM_REF_CTRL_RX_CLKACT BIT(26)

#define CLK_DIV_MAX 63U

/* The input clocks, in the order of the PLL source select fields */
enum zynqmp_clk_input {
	ZYNQMP_CLK_PSS_REF_CLK,
	ZYNQMP_CLK_VIDEO_CLK,
	ZYNQMP_CLK_PSS_ALT_REF_CLK,
	ZYNQMP_CLK_AUX_REF_CLK,
	ZYNQMP_CLK_GT_CRX_REF_CLK,
	ZYNQMP_CLK_INPUT_COUNT,
};

/* The PLLs a SRCSEL field selects, NULL where reserved */
struct zynqmp_clk_sources {
	uint8_t pll[4];
};

/* Low power domain peripherals: IOPLL, reserved, RPLL, DPLL through the LPD divider */
static const struct zynqmp_clk_sources zynqmp_clk_periph_sources = {
	.pll = {IOPLL, UINT8_MAX, RPLL, DPLL_TO_LPD},
};

/* Low power domain bus: RPLL, reserved, IOPLL, DPLL through the LPD divider */
static const struct zynqmp_clk_sources zynqmp_clk_lsbus_sources = {
	.pll = {RPLL, UINT8_MAX, IOPLL, DPLL_TO_LPD},
};

struct zynqmp_clk_config {
	DEVICE_MMIO_NAMED_ROM(crl_apb);
	DEVICE_MMIO_NAMED_ROM(crf_apb);
	/* NULL for the inputs the devicetree does not give */
	const struct device *inputs[ZYNQMP_CLK_INPUT_COUNT];
};

struct zynqmp_clk_data {
	DEVICE_MMIO_NAMED_RAM(crl_apb);
	DEVICE_MMIO_NAMED_RAM(crf_apb);
	struct k_spinlock lock;
};

static int zynqmp_clk_input_rate(const struct device *dev, uint32_t src, uint32_t *rate)
{
	const struct zynqmp_clk_config *cfg = dev->config;
	const struct device *input;

	/* 0 to 3 select the PS reference clock, the others follow in order */
	input = cfg->inputs[(src < 4U) ? ZYNQMP_CLK_PSS_REF_CLK : (src - 3U)];
	if (input == NULL) {
		return -ENOTSUP;
	}

	return clock_control_get_rate(input, NULL, rate);
}

static int zynqmp_clk_pll_rate(const struct device *dev, mm_reg_t reg, uint32_t *rate)
{
	uint32_t ctrl = sys_read32(reg);
	uint32_t src_rate;
	int ret;

	if ((ctrl & PLL_CTRL_BYPASS) != 0U) {
		return zynqmp_clk_input_rate(dev, FIELD_GET(PLL_CTRL_POST_SRC, ctrl), rate);
	}

	if ((ctrl & PLL_CTRL_RESET) != 0U) {
		*rate = 0U;
		return 0;
	}

	ret = zynqmp_clk_input_rate(dev, FIELD_GET(PLL_CTRL_PRE_SRC, ctrl), &src_rate);
	if (ret < 0) {
		return ret;
	}

	*rate = src_rate * FIELD_GET(PLL_CTRL_FBDIV, ctrl);
	if ((ctrl & PLL_CTRL_DIV2) != 0U) {
		*rate /= 2U;
	}

	return 0;
}

static int zynqmp_clk_dpll_to_lpd_rate(const struct device *dev, uint32_t *rate)
{
	mm_reg_t crf_apb = DEVICE_MMIO_NAMED_GET(dev, crf_apb);
	uint32_t ctrl, dpll_rate;
	int ret;

	ret = zynqmp_clk_pll_rate(dev, crf_apb + CRF_APB_DPLL_CTRL, &dpll_rate);
	if (ret < 0) {
		return ret;
	}

	ctrl = sys_read32(crf_apb + CRF_APB_DPLL_TO_LPD_CTRL);
	*rate = dpll_rate / MAX(FIELD_GET(PLL_TO_LPD_CTRL_DIVISOR0, ctrl), 1U);

	return 0;
}

static int zynqmp_clk_pll_id_rate(const struct device *dev, uint32_t id, uint32_t *rate)
{
	mm_reg_t crl_apb = DEVICE_MMIO_NAMED_GET(dev, crl_apb);
	mm_reg_t crf_apb = DEVICE_MMIO_NAMED_GET(dev, crf_apb);

	switch (id) {
	case IOPLL:
		return zynqmp_clk_pll_rate(dev, crl_apb + CRL_APB_IOPLL_CTRL, rate);
	case RPLL:
		return zynqmp_clk_pll_rate(dev, crl_apb + CRL_APB_RPLL_CTRL, rate);
	case DPLL:
		return zynqmp_clk_pll_rate(dev, crf_apb + CRF_APB_DPLL_CTRL, rate);
	case DPLL_TO_LPD:
		return zynqmp_clk_dpll_to_lpd_rate(dev, rate);
	default:
		return -ENOTSUP;
	}
}

/* The rate of the PLL the SRCSEL field of a peripheral clock selects */
static int zynqmp_clk_src_rate(const struct device *dev, uint32_t ctrl,
			       const struct zynqmp_clk_sources *srcs, uint32_t *rate)
{
	uint32_t srcsel = FIELD_GET(CLK_CTRL_SRCSEL, ctrl);

	if ((srcsel >= ARRAY_SIZE(srcs->pll)) || (srcs->pll[srcsel] == UINT8_MAX)) {
		return -ENOTSUP;
	}

	return zynqmp_clk_pll_id_rate(dev, srcs->pll[srcsel], rate);
}

static int zynqmp_clk_periph_rate(const struct device *dev, uint32_t reg,
				  const struct zynqmp_clk_sources *srcs, bool two_divs,
				  uint32_t *rate)
{
	uint32_t ctrl = sys_read32(DEVICE_MMIO_NAMED_GET(dev, crl_apb) + reg);
	uint32_t divisor, src_rate;
	int ret;

	ret = zynqmp_clk_src_rate(dev, ctrl, srcs, &src_rate);
	if (ret < 0) {
		return ret;
	}

	divisor = MAX(FIELD_GET(CLK_CTRL_DIVISOR0, ctrl), 1U);
	if (two_divs) {
		divisor *= MAX(FIELD_GET(CLK_CTRL_DIVISOR1, ctrl), 1U);
	}

	*rate = src_rate / divisor;

	return 0;
}

/* Write a CRL_APB register, lifting the write protection for the write */
static void zynqmp_clk_crl_apb_write(const struct device *dev, uint32_t reg, uint32_t val)
{
	mm_reg_t crl_apb = DEVICE_MMIO_NAMED_GET(dev, crl_apb);
	uint32_t wprot = sys_read32(crl_apb + CRL_APB_WPROT);

	if ((wprot & WPROT_ACTIVE) != 0U) {
		sys_write32(wprot & ~WPROT_ACTIVE, crl_apb + CRL_APB_WPROT);
	}
	sys_write32(val, crl_apb + reg);
	if ((wprot & WPROT_ACTIVE) != 0U) {
		sys_write32(wprot, crl_apb + CRL_APB_WPROT);
	}
}

/* Set the two dividers of a peripheral clock to the pair closest to the target rate */
static int zynqmp_clk_periph_set_rate(const struct device *dev, uint32_t reg,
				      const struct zynqmp_clk_sources *srcs, uint32_t target)
{
	struct zynqmp_clk_data *data = dev->data;
	mm_reg_t crl_apb = DEVICE_MMIO_NAMED_GET(dev, crl_apb);
	uint32_t best_err = UINT32_MAX, best_div0 = 0U, best_div1 = 0U;
	uint32_t ctrl, src_rate;
	k_spinlock_key_t key;
	int ret;

	if (target == 0U) {
		return -EINVAL;
	}

	ctrl = sys_read32(crl_apb + reg);
	ret = zynqmp_clk_src_rate(dev, ctrl, srcs, &src_rate);
	if (ret < 0) {
		return ret;
	}

	for (uint32_t div0 = 1U; (div0 <= CLK_DIV_MAX) && (best_err > 0U); div0++) {
		for (uint32_t div1 = 1U; div1 <= CLK_DIV_MAX; div1++) {
			uint32_t rate = src_rate / (div0 * div1);
			uint32_t err = (rate > target) ? (rate - target) : (target - rate);

			if (err < best_err) {
				best_err = err;
				best_div0 = div0;
				best_div1 = div1;
			}
			if (err == 0U) {
				break;
			}
		}
	}

	LOG_DBG("dividers %u/%u for %u Hz from %u Hz", best_div0, best_div1, target, src_rate);

	key = k_spin_lock(&data->lock);

	ctrl = sys_read32(crl_apb + reg);
	ctrl &= ~(CLK_CTRL_DIVISOR0 | CLK_CTRL_DIVISOR1);
	ctrl |= FIELD_PREP(CLK_CTRL_DIVISOR0, best_div0) | FIELD_PREP(CLK_CTRL_DIVISOR1, best_div1);
	zynqmp_clk_crl_apb_write(dev, reg, ctrl);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int zynqmp_clk_gate(const struct device *dev, uint32_t reg, uint32_t mask, bool on)
{
	struct zynqmp_clk_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t val;

	key = k_spin_lock(&data->lock);

	val = sys_read32(DEVICE_MMIO_NAMED_GET(dev, crl_apb) + reg);
	if (on) {
		val |= mask;
	} else {
		val &= ~mask;
	}
	zynqmp_clk_crl_apb_write(dev, reg, val);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int zynqmp_clk_on_off(const struct device *dev, clock_control_subsys_t sys, bool on)
{
	uint32_t id = (uint32_t)(uintptr_t)sys;

	switch (id) {
	case IOPLL:
	case RPLL:
	case DPLL:
	case DPLL_TO_LPD:
	case GEM_TSU:
	case GEM0_REF ... GEM3_REF:
		/* Not gated, the PLLs run as the firmware set them up */
		return on ? 0 : -ENOTSUP;
	case LPD_LSBUS:
		return zynqmp_clk_gate(dev, CRL_APB_LPD_LSBUS_CTRL, CLK_CTRL_CLKACT, on);
	case GEM_TSU_REF:
		return zynqmp_clk_gate(dev, CRL_APB_GEM_TSU_REF_CTRL, CLK_CTRL_CLKACT, on);
	case GEM0_TX ... GEM3_TX:
		return zynqmp_clk_gate(dev, CRL_APB_GEM_REF_CTRL(id - GEM0_TX), GEM_REF_CTRL_CLKACT,
				       on);
	case GEM0_RX ... GEM3_RX:
		return zynqmp_clk_gate(dev, CRL_APB_GEM_REF_CTRL(id - GEM0_RX),
				       GEM_REF_CTRL_RX_CLKACT, on);
	default:
		return -ENOTSUP;
	}
}

static int zynqmp_clk_on(const struct device *dev, clock_control_subsys_t sys)
{
	return zynqmp_clk_on_off(dev, sys, true);
}

static int zynqmp_clk_off(const struct device *dev, clock_control_subsys_t sys)
{
	return zynqmp_clk_on_off(dev, sys, false);
}

static int zynqmp_clk_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	uint32_t id = (uint32_t)(uintptr_t)sys;

	switch (id) {
	case IOPLL:
	case RPLL:
	case DPLL:
	case DPLL_TO_LPD:
		return zynqmp_clk_pll_id_rate(dev, id, rate);
	case LPD_LSBUS:
		return zynqmp_clk_periph_rate(dev, CRL_APB_LPD_LSBUS_CTRL,
					      &zynqmp_clk_lsbus_sources, false, rate);
	case GEM_TSU_REF:
	case GEM_TSU:
		return zynqmp_clk_periph_rate(dev, CRL_APB_GEM_TSU_REF_CTRL,
					      &zynqmp_clk_periph_sources, true, rate);
	case GEM0_TX ... GEM3_TX:
		return zynqmp_clk_periph_rate(dev, CRL_APB_GEM_REF_CTRL(id - GEM0_TX),
					      &zynqmp_clk_periph_sources, true, rate);
	case GEM0_RX ... GEM3_RX:
		return zynqmp_clk_periph_rate(dev, CRL_APB_GEM_REF_CTRL(id - GEM0_RX),
					      &zynqmp_clk_periph_sources, true, rate);
	case GEM0_REF ... GEM3_REF:
		return zynqmp_clk_periph_rate(dev, CRL_APB_GEM_REF_CTRL(id - GEM0_REF),
					      &zynqmp_clk_periph_sources, true, rate);
	default:
		return -ENOTSUP;
	}
}

static int zynqmp_clk_set_rate(const struct device *dev, clock_control_subsys_t sys,
			       clock_control_subsys_rate_t rate)
{
	uint32_t id = (uint32_t)(uintptr_t)sys;
	uint32_t target = (uint32_t)(uintptr_t)rate;

	switch (id) {
	case GEM0_TX ... GEM3_TX:
		return zynqmp_clk_periph_set_rate(dev, CRL_APB_GEM_REF_CTRL(id - GEM0_TX),
						  &zynqmp_clk_periph_sources, target);
	case GEM0_REF ... GEM3_REF:
		return zynqmp_clk_periph_set_rate(dev, CRL_APB_GEM_REF_CTRL(id - GEM0_REF),
						  &zynqmp_clk_periph_sources, target);
	default:
		return -ENOTSUP;
	}
}

static int zynqmp_clk_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, crl_apb, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, crf_apb, K_MEM_CACHE_NONE);

	return 0;
}

static DEVICE_API(clock_control, zynqmp_clk_api) = {
	.on = zynqmp_clk_on,
	.off = zynqmp_clk_off,
	.get_rate = zynqmp_clk_get_rate,
	.set_rate = zynqmp_clk_set_rate,
};

#define ZYNQMP_CLK_INPUT(n, name)                                                                  \
	COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(n, name),                                              \
		    (DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, name))), (NULL))

#define ZYNQMP_CLK_DEVICE(n)                                                                       \
	BUILD_ASSERT(DT_INST_CLOCKS_HAS_NAME(n, pss_ref_clk),                                      \
		     "the pss_ref_clk clock is required");                                         \
                                                                                                   \
	static const struct zynqmp_clk_config zynqmp_clk_config_##n = {                            \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(crl_apb, DT_DRV_INST(n)),                       \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(crf_apb, DT_DRV_INST(n)),                       \
		.inputs = {                                                                        \
			[ZYNQMP_CLK_PSS_REF_CLK] = ZYNQMP_CLK_INPUT(n, pss_ref_clk),               \
			[ZYNQMP_CLK_VIDEO_CLK] = ZYNQMP_CLK_INPUT(n, video_clk),                   \
			[ZYNQMP_CLK_PSS_ALT_REF_CLK] = ZYNQMP_CLK_INPUT(n, pss_alt_ref_clk),       \
			[ZYNQMP_CLK_AUX_REF_CLK] = ZYNQMP_CLK_INPUT(n, aux_ref_clk),               \
			[ZYNQMP_CLK_GT_CRX_REF_CLK] = ZYNQMP_CLK_INPUT(n, gt_crx_ref_clk),         \
		},                                                                                 \
	};                                                                                         \
                                                                                                   \
	static struct zynqmp_clk_data zynqmp_clk_data_##n;                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, zynqmp_clk_init, NULL, &zynqmp_clk_data_##n,                      \
			      &zynqmp_clk_config_##n, PRE_KERNEL_1,                                \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &zynqmp_clk_api);

DT_INST_FOREACH_STATUS_OKAY(ZYNQMP_CLK_DEVICE)
