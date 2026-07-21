/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief MFD driver for the Infineon HPPASS CSG (Comparator Slope Generator).
 *
 * The CSG block contains five identical slices.  Each slice pairs a 10-bit,
 * 30 Msps DAC with a high-speed comparator.  This MFD owns:
 *
 *   - the combined CSG comparator interrupt handler.
 *   - the per-slice register access surface used by per-slice child drivers.
 *
 * Per-slice CMP_CFG and DAC_CFG init live in the comparator and DAC child
 * drivers respectively.  Per-slice DAC interrupts are handled by the DAC child
 * drivers.
 *
 * HPPASS is shared across multiple Infineon device families. The TRM
 * citations throughout this file refer to the PSOC Control C3 Mainline
 * documentation:
 * References:
 *   - <em>PSOC Control C3 Mainline Architecture TRM</em>, 002-39348
 *     (§27.2.3 CSG, §27.2.3.5 Post processing, §27.2.3.6 CSG interrupts)
 *   - <em>PSOC Control C3 Mainline Registers TRM</em>, 002-39445
 *     (HPPASS_CSG_* §20.1.53–20.1.63)
 */

#define DT_DRV_COMPAT infineon_hppass_csg

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <infineon_kconfig.h>
#include <infineon_hppass_csg.h>
#include <cy_device.h>
#include <cy_sysclk.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>

#include "mfd_infineon_hppass_regs.h"

LOG_MODULE_REGISTER(mfd_infineon_hppass_csg, CONFIG_MFD_LOG_LEVEL);

#define IFX_HPPASS_CSG_NUM_SLICES DT_INST_PROP(0, infineon_num_slices)

struct ifx_csg_data {
	ifx_hppass_csg_cmp_cb_t slice_cb[IFX_HPPASS_CSG_NUM_SLICES];
	void *slice_user[IFX_HPPASS_CSG_NUM_SLICES];
	ifx_hppass_csg_combined_cmp_cb_t combined_cb;
	void *combined_user;
};

struct ifx_csg_config {
	mem_addr_t base;
	const struct device *parent;
	struct ifx_cat1_clock clock;
	en_clk_dst_t clk_dst;
	uint8_t dac_observe_blank;
};

/*
 * Combined CSG comparator interrupt handler.  All five slice comparators
 * share a single hardware vector; the parent reads CMP_INTR_MASKED to
 * determine which slices fired and dispatches both the combined safety
 * callback and per-slice child callbacks.
 */
static void ifx_hppass_csg_cmp_isr(const struct device *dev)
{
	const struct ifx_csg_config *cfg = dev->config;
	struct ifx_csg_data *data = dev->data;
	mem_addr_t base = cfg->base;
	uint32_t masked;

	masked = sys_read32(base + IFX_HPPASS_CSG_CMP_INTR_MASKED) &
		 HPPASS_CSG_CMP_INTR_MASKED_CMP_Msk;

	/*
	 * Clear the observed interrupts up front so a re-trigger that occurs
	 * while a callback is running is captured by the next IRQ rather than
	 * lost to the next read of CMP_INTR_MASKED.
	 */
	sys_write32(masked, base + IFX_HPPASS_CSG_CMP_INTR);

	/* Combined callback runs first (lowest-latency safety/fault path, e.g. PWM trip). */
	if (data->combined_cb != NULL) {
		data->combined_cb(dev, (uint8_t)masked, data->combined_user);
	}

	for (uint8_t s = 0U; s < IFX_HPPASS_CSG_NUM_SLICES; s++) {
		if (((masked & BIT(s)) != 0U) && (data->slice_cb[s] != NULL)) {
			data->slice_cb[s](data->slice_user[s]);
		}
	}
} /* ifx_hppass_csg_cmp_isr() */

/*
 * Replace the per-slice comparator callback under irq_lock so the ISR
 * always observes a consistent cb/user_data pair.  Pass @p cb as NULL to
 * clear.  CMP_INTR_MASK ownership stays with the comparator child driver.
 */
int ifx_hppass_csg_register_cmp_cb(const struct device *dev, uint8_t slice,
				   ifx_hppass_csg_cmp_cb_t cb, void *user_data)
{
	struct ifx_csg_data *data;
	unsigned int key;

	if ((dev == NULL) || (slice >= IFX_HPPASS_CSG_NUM_SLICES)) {
		return -EINVAL;
	}

	data = dev->data;

	key = irq_lock();
	data->slice_cb[slice] = cb;
	data->slice_user[slice] = user_data;
	irq_unlock(key);

	return 0;
} /* ifx_hppass_csg_register_cmp_cb() */

/*
 * Replace the combined-comparator callback under irq_lock.  This
 * callback fires first in the ISR with a bitmask of every slice that
 * tripped, intended for the lowest-latency safety/fault path (e.g. PWM
 * trip on motor over-current).  Pass @p cb as NULL to clear.
 */
int ifx_hppass_csg_register_combined_cmp_cb(const struct device *dev,
					    ifx_hppass_csg_combined_cmp_cb_t cb,
					    void *user_data)
{
	struct ifx_csg_data *data;
	unsigned int key;

	if (dev == NULL) {
		return -EINVAL;
	}

	data = dev->data;

	key = irq_lock();
	data->combined_cb = cb;
	data->combined_user = user_data;
	irq_unlock(key);

	return 0;
} /* ifx_hppass_csg_register_combined_cmp_cb() */

/*
 * Forward a CSG slice's DAC output onto the SAR ADC AROUTE MUX0
 * observability path.  Slice argument is 0-based; -1 disables the route
 * (VDAC_OUT_SEL = 0 = Hi-Z).  Hardware encoding is 1-based: 1=DAC0..5=DAC4.
 */
int mfd_infineon_hppass_csg_route_dac_to_adc(const struct device *dev, int slice)
{
	const struct ifx_csg_config *cfg;
	uint32_t sel;
	uint32_t reg;

	if (dev == NULL) {
		return -EINVAL;
	}
	if (slice < -1 || slice >= (int)IFX_HPPASS_CSG_NUM_SLICES) {
		return -EINVAL;
	}

	cfg = dev->config;
	sel = (slice < 0) ? 0U : (uint32_t)(slice + 1);

	reg = sys_read32(cfg->base + IFX_HPPASS_CSG_CTRL) & ~HPPASS_CSG_CSG_CTRL_VDAC_OUT_SEL_Msk;
	reg |= FIELD_PREP(HPPASS_CSG_CSG_CTRL_VDAC_OUT_SEL_Msk, sel);
	sys_write32(reg, cfg->base + IFX_HPPASS_CSG_CTRL);

	return 0;
} /* mfd_infineon_hppass_csg_route_dac_to_adc() */

/*
 * Configure clock divider, mask all comparator interrupts, clear any stale
 * CMP_INTR latch bits, and connect the combined CSG comparator IRQ.
 */
static int ifx_hppass_csg_init(const struct device *dev)
{
	const struct ifx_csg_config *cfg = dev->config;
	mem_addr_t base = cfg->base;
	cy_rslt_t result;

	if (!device_is_ready(cfg->parent)) {
		LOG_ERR("HPPASS parent not ready");
		return -ENODEV;
	}

	result = ifx_cat1_utils_peri_pclk_assign_divider(cfg->clk_dst, &cfg->clock);
	if (result != CY_RSLT_SUCCESS) {
		LOG_ERR("CSG peri-pclk assign failed (0x%08x)", result);
		return -EIO;
	}

	/* All comparator interrupts masked until a child unmasks its slice. */
	sys_write32(0U, base + IFX_HPPASS_CSG_CMP_INTR_MASK);

	/* Clear any stale pending CMP interrupts. */
	sys_write32(HPPASS_CSG_CMP_INTR_CMP_Msk, base + IFX_HPPASS_CSG_CMP_INTR);

	/*
	 * VDAC_OUT_BLANK.BLANK_CNT (Regs TRM §20.1.62): static board-level
	 * setting applied once from the `infineon,dac-observe-blank-cycles`
	 * DT property.  Only needed for designs that exercise DAC observability.
	 */
	sys_write32(FIELD_PREP(HPPASS_CSG_VDAC_OUT_BLANK_BLANK_CNT_Msk,
			       (uint32_t)cfg->dac_observe_blank),
		    base + IFX_HPPASS_CSG_VDAC_OUT_BLANK);

	return 0;
} /* ifx_hppass_csg_init() */

#define IFX_CSG_INIT(n)                                                                            \
	BUILD_ASSERT(DT_INST_PROP_OR(n, infineon_dac_observe_blank_cycles, 0) <=                   \
			(HPPASS_CSG_VDAC_OUT_BLANK_BLANK_CNT_Msk >>                                \
			 HPPASS_CSG_VDAC_OUT_BLANK_BLANK_CNT_Pos),                                 \
		     "infineon,dac-observe-blank-cycles must fit in BLANK_CNT (0..31)");           \
	static const struct ifx_csg_config ifx_csg_config_##n = {                                  \
		.base = (mem_addr_t)DT_INST_REG_ADDR(n),                                           \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(n)),                                        \
		.clock = {                                                                         \
			.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                 \
				DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),         \
				DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                     \
			.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                    \
		},                                                                                 \
		.clk_dst = (en_clk_dst_t)DT_INST_PROP(n, clk_dst),                                 \
		.dac_observe_blank = (uint8_t)DT_INST_PROP(n, infineon_dac_observe_blank_cycles),  \
	};                                                                                         \
                                                                                                   \
	static struct ifx_csg_data ifx_csg_data_##n;                                               \
                                                                                                   \
	static int ifx_hppass_csg_init_##n(const struct device *dev)                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ifx_hppass_csg_cmp_isr,     \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
		return ifx_hppass_csg_init(dev);                                                   \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ifx_hppass_csg_init_##n, NULL, &ifx_csg_data_##n,                 \
			      &ifx_csg_config_##n, POST_KERNEL,                                    \
			      CONFIG_MFD_INFINEON_HPPASS_CSG_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(IFX_CSG_INIT)
