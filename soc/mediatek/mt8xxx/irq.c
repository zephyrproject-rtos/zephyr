/* Copyright 2023 The ChromiumOS Authors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>

bool intc_mtk_adsp_get_enable(const struct device *dev, int irq);
void intc_mtk_adsp_set_enable(const struct device *dev, int irq, bool val);

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
#else
	/* Only one on 818x */
	return DEVICE_DT_GET(DT_INST(0, mediatek_adsp_intc));
#endif
}

void z_soc_irq_enable(unsigned int irq)
{
	/* First 32 IRQs are the Xtensa architectural vectors,  */
	if (irq < 32) {
		xtensa_irq_enable(irq);
	} else {
		const struct device *dev = irq_dev(&irq);

		intc_mtk_adsp_set_enable(dev, irq, true);
	}
}

void z_soc_irq_disable(unsigned int irq)
{
	if (irq < 32) {
		xtensa_irq_disable(irq);
	} else {
		const struct device *dev = irq_dev(&irq);

		intc_mtk_adsp_set_enable(dev, irq, false);
	}
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	if (irq < 32) {
		return xtensa_irq_is_enabled(irq);
	}
	return intc_mtk_adsp_get_enable(irq_dev(&irq), irq);
}
