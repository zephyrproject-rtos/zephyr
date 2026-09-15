/* Copyright 2023 The ChromiumOS Authors
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/intc_root.h>
#include <zephyr/irq.h>
#include <zephyr/sys/__assert.h>

#define DT_DRV_COMPAT mediatek_adsp_intc

struct intc_mtk_cfg {
	uint32_t xtensa_irq;
	uint32_t irq_mask;
	uint32_t sw_isr_off;
	volatile uint32_t *enable_reg;
	volatile uint32_t *status_reg;
};

bool intc_mtk_adsp_get_enable(const struct device *dev, int irq)
{
	const struct intc_mtk_cfg *cfg = dev->config;

	return (*cfg->enable_reg | (BIT(irq) & cfg->irq_mask)) != 0;
}

void intc_mtk_adsp_set_enable(const struct device *dev, int irq, bool val)
{
	const struct intc_mtk_cfg *cfg = dev->config;

	irq_enable(cfg->xtensa_irq);

	if ((BIT(irq) & cfg->irq_mask) != 0) {
		if (val) {
			*cfg->enable_reg |= BIT(irq);
		} else {
			*cfg->enable_reg &= ~BIT(irq);
		}
	}
}

static void intc_isr(const void *arg)
{
	const struct intc_mtk_cfg *cfg = ((struct device *)arg)->config;
	uint32_t irqs = *cfg->status_reg & cfg->irq_mask;

	while (irqs != 0) {
		uint32_t irq = find_msb_set(irqs) - 1;
		uint32_t off = cfg->sw_isr_off + irq;

		_sw_isr_table[off].isr(_sw_isr_table[off].arg);
		irqs &= ~BIT(irq);
	}
}

static void dev_init(const struct device *dev)
{
	const struct intc_mtk_cfg *cfg = dev->config;

	*cfg->enable_reg = 0;
	irq_enable(cfg->xtensa_irq);
}

#define DEV_INIT(N) \
	IRQ_CONNECT(DT_INST_IRQN(N), 0, intc_isr, DEVICE_DT_INST_GET(N), 0); \
	dev_init(DEVICE_DT_INST_GET(N));

static int intc_init(void)
{
	DT_INST_FOREACH_STATUS_OKAY(DEV_INIT);
	return 0;
}

SYS_INIT(intc_init, PRE_KERNEL_1, 0);

#define DEF_DEV(N)						\
static const struct intc_mtk_cfg dev_cfg##N = {			\
	.xtensa_irq = DT_INST_IRQN(N),				\
	.irq_mask   = DT_INST_PROP(N, mask),			\
	.sw_isr_off = (N + 1) * 32,				\
	.enable_reg = (void *)DT_INST_REG_ADDR(N),		\
	.status_reg = (void *)DT_INST_PROP(N, status_reg) };	\
DEVICE_DT_INST_DEFINE(N, NULL, NULL, NULL, &dev_cfg##N, PRE_KERNEL_1, 0, NULL);

DT_INST_FOREACH_STATUS_OKAY(DEF_DEV);

/* Sort of annoying: assumes there are exactly two controller devices
 * and that their instance IDs (i.e. the order in which they appear in
 * the .dts file) match their order in the _sw_isr_table[].  A better
 * scheme would be able to enumerate the tree at runtime.
 */
static const struct device *irq_dev(unsigned int *irq_inout)
{
#ifdef CONFIG_SOC_SERIES_MT8195
	/* Controller 0 is on Xtensa vector 1, controller 1 on vector 23. */
	if ((*irq_inout & 0xff) == 1) {
		*irq_inout >>= 8;
		return DEVICE_DT_GET(DT_INST(0, mediatek_adsp_intc));
	}
	__ASSERT_NO_MSG((*irq_inout & 0xff) == 23);
	*irq_inout = (*irq_inout >> 8) - 1;
	return DEVICE_DT_GET(DT_INST(1, mediatek_adsp_intc));
#elif defined(CONFIG_SOC_SERIES_MT8196)
	/* Two subcontrollers on core IRQs 1 and 2 */
	uint32_t lvl1 = *irq_inout & 0xff;

	*irq_inout = (*irq_inout >> 8) - 1;
	if (lvl1 == 1) {
		return DEVICE_DT_GET(DT_INST(0, mediatek_adsp_intc));
	}
	__ASSERT_NO_MSG(lvl1 == 2);
	return DEVICE_DT_GET(DT_INST(1, mediatek_adsp_intc));
#elif defined(CONFIG_SOC_SERIES_MT8365)
	/* Controller 0 is on Xtensa vector 1 */
	if ((*irq_inout & 0xff) == 1) {
		*irq_inout = (*irq_inout >> 8) - 1;
	}
	return DEVICE_DT_GET(DT_INST(0, mediatek_adsp_intc));
#else
	/* Only one on 818x */
	return DEVICE_DT_GET(DT_INST(0, mediatek_adsp_intc));
#endif
}

/*
 * The controllers own the multi-level interrupt routing of these SoCs, their
 * own lines through the controller and the core lines directly, so they
 * provide the root interrupt controller API.
 */
void intc_root_enable(unsigned int irq)
{
	/* First 32 IRQs are the Xtensa architectural vectors,  */
	if (irq < 32) {
		xtensa_irq_enable(irq);
#if defined(CONFIG_SOC_MT8188)
		if (irq < 25) {
			*(volatile uint32_t *)(DT_REG_ADDR(DT_NODELABEL(intc2))) |= BIT(irq);
		}
#elif defined(CONFIG_SOC_SERIES_MT8365)
		if (irq >= 2 && irq <= 5) {
			const struct device *dev = DEVICE_DT_GET(DT_INST(0, mediatek_adsp_intc));

			intc_mtk_adsp_set_enable(dev, irq - 2, true);
		}
#endif
	} else {
		const struct device *dev = irq_dev(&irq);

		intc_mtk_adsp_set_enable(dev, irq, true);
	}
}

void intc_root_disable(unsigned int irq)
{
	if (irq < 32) {
		xtensa_irq_disable(irq);
#if defined(CONFIG_SOC_MT8188)
		if (irq < 25) {
			*(volatile uint32_t *)(DT_REG_ADDR(DT_NODELABEL(intc2))) &= ~BIT(irq);
		}
#elif defined(CONFIG_SOC_SERIES_MT8365)
		if (irq >= 2 && irq <= 5) {
			const struct device *dev = DEVICE_DT_GET(DT_INST(0, mediatek_adsp_intc));

			intc_mtk_adsp_set_enable(dev, irq - 2, false);
		}
#endif
	} else {
		const struct device *dev = irq_dev(&irq);

		intc_mtk_adsp_set_enable(dev, irq, false);
	}
}

int intc_root_is_enabled(unsigned int irq)
{
	const struct device *dev;

	if (irq < 32) {
		return xtensa_irq_is_enabled(irq);
	}

	dev = irq_dev(&irq);

	return intc_mtk_adsp_get_enable(dev, irq);
}
