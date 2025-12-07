/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/intc.h>
#include <zephyr/drivers/intc/intl.h>
#include <hal/nrf_vpr_clic.h>

#define DT_DRV_COMPAT nordic_nrf_clic

static int driver_configure_irq(const struct device *dev, uint16_t irq, uint32_t flags)
{
	if (flags > CLIC_CLIC_CLICINT_PRIORITY_PRIOLEVEL3) {
		return -EINVAL;
	}

	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, irq, false);
	nrf_vpr_clic_int_pending_clear(NRF_VPRCLIC, irq);
	nrf_vpr_clic_int_priority_set(NRF_VPRCLIC, irq, NRF_VPR_CLIC_INT_TO_PRIO(flags));
	return 0;
}

static int driver_enable_irq(const struct device *dev, uint16_t irq)
{
	ARG_UNUSED(dev);

	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, irq, true);
	return 0;
}

static int driver_disable_irq(const struct device *dev, uint16_t irq)
{
	int ret;

	ARG_UNUSED(dev);

	ret = nrf_vpr_clic_int_enable_check(NRF_VPRCLIC, irq);
	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, irq, false);
	return ret;
}

static int driver_trigger_irq(const struct device *dev, uint16_t irq)
{
	ARG_UNUSED(dev);

	nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, irq);
	return 0;
}

static int driver_clear_irq(const struct device *dev, uint16_t irq)
{
	int ret;

	ARG_UNUSED(dev);

	ret = nrf_vpr_clic_int_pending_check(NRF_VPRCLIC, irq);
	nrf_vpr_clic_int_pending_clear(NRF_VPRCLIC, irq);
	return ret;
}

static DEVICE_API(intc, driver_api) = {
	.configure_irq = driver_configure_irq,
	.enable_irq = driver_enable_irq,
	.disable_irq = driver_disable_irq,
	.trigger_irq = driver_trigger_irq,
	.clear_irq = driver_clear_irq,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1, 0, &driver_api);

#define DRIVER_VECTOR_TABLE_ENTRY_DEFINE(inst, intln) \
	_isr_wrapper

#define DRIVER_VECTOR_TABLE_ENTRIES_DEFINE(inst) \
	INTC_DT_INST_FOREACH_INTL_SEP(inst, DRIVER_VECTOR_TABLE_ENTRY_DEFINE, (,))

intc_vector_t __irq_vector_table __used _irq_vector_table[] = {
	DRIVER_VECTOR_TABLE_ENTRIES_DEFINE(0)
};

const sys_irq_intl_handler_t _intl_handlers[] = {
	INTC_DT_INST_FOREACH_INTL_SEP(0, INTC_DT_INST_INTL_HANDLER_SYMBOL, (,))
};
