/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_drw

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <r_drw_base.h>
#include <r_drw_cfg.h>

#define DRW_PRV_IRQCTL_DLISTIRQ_ENABLE (1U << 1)
#define DRW_PRV_IRQCTL_ENUMIRQ_CLEAR   (1U << 2)
#define DRW_PRV_IRQCTL_DLISTIRQ_CLEAR  (1U << 3)
#define DRW_PRV_IRQCTL_BUSIRQ_CLEAR    (1U << 5)
#define DRW_PRV_IRQCTL_ALLIRQ_DISABLE_AND_CLEAR                                                    \
	(DRW_PRV_IRQCTL_BUSIRQ_CLEAR | DRW_PRV_IRQCTL_DLISTIRQ_CLEAR | DRW_PRV_IRQCTL_ENUMIRQ_CLEAR)
#define DRW_PRV_IRQCTL_ALLIRQ_CLEAR_AND_DLISTIRQ_ENABLE                                            \
	(DRW_PRV_IRQCTL_BUSIRQ_CLEAR | DRW_PRV_IRQCTL_DLISTIRQ_CLEAR |                             \
	 DRW_PRV_IRQCTL_ENUMIRQ_CLEAR | DRW_PRV_IRQCTL_DLISTIRQ_ENABLE)
#define DRW_PRV_STATUS_DLISTIRQ_TRIGGERED (1U << 5)
#define VECTOR_NUMBER_DRW_INT             DT_IRQN(DT_NODELABEL(drw))

static struct k_sem d1_queryirq_sem;
K_HEAP_DEFINE(drw_heap_runtime, CONFIG_RENESAS_DAVE2D_RUNTIME_HEAP_SIZE);

d1_int_t d1_initirq_intern(d1_device_flex *handle)
{
	d1_int_t ret = 1;

	if (VECTOR_NUMBER_DRW_INT >= 0) {
		/* Clear all the D/AVE 2D IRQs and enable Display list IRQ. */
		R_DRW->IRQCTL = DRW_PRV_IRQCTL_ALLIRQ_CLEAR_AND_DLISTIRQ_ENABLE;
		R_FSP_IsrContextSet((IRQn_Type)VECTOR_NUMBER_DRW_INT, handle);
	}

	/* Initialize semaphore for use in d1_queryirq() */
	if (0 != k_sem_init(&d1_queryirq_sem, 0, 1)) {
		ret = 0;
	}

	return ret;
}

d1_int_t d1_shutdownirq_intern(d1_device_flex *handle)
{
	ARG_UNUSED(handle);

	/* Disable D/AVE 2D interrupt in NVIC. */
	irq_disable(VECTOR_NUMBER_DRW_INT);

	/* Clear all the D/AVE 2D IRQs and disable Display list IRQ. */
	R_DRW->IRQCTL = DRW_PRV_IRQCTL_ALLIRQ_DISABLE_AND_CLEAR;

	return 1;
}

d1_int_t d1_queryirq(d1_device *handle, d1_int_t irqmask, d1_int_t timeout)
{
	d1_int_t ret = 1;

	/* Wait for dlist processing to complete. */
	int err = k_sem_take(&d1_queryirq_sem, K_MSEC(timeout));

	/* If the wait timed out return 0. */
	if (err != 0) {
		ret = 0;
	}

	return ret;
}

void *d1_malloc(d1_uint_t size)
{
	return k_heap_alloc(&drw_heap_runtime, size, K_NO_WAIT);
}

void d1_free(void *ptr)
{
	k_heap_free(&drw_heap_runtime, ptr);
}

void drw_zephyr_irq_handler(const struct device *dev)
{
	uint32_t int_status;
	IRQn_Type irq = R_FSP_CurrentIrqGet();
	/* Read D/AVE 2D interrupt status */
	int_status = R_DRW->STATUS;
	/* Get current IRQ number */
	/* Clear all D/AVE 2D interrupts except for Display List IRQ enable */
	R_DRW->IRQCTL = DRW_PRV_IRQCTL_ALLIRQ_CLEAR_AND_DLISTIRQ_ENABLE;

	if (int_status & DRW_PRV_STATUS_DLISTIRQ_TRIGGERED) {
		d1_device_flex *p_context = (d1_device_flex *)R_FSP_IsrContextGet(irq);

		if (p_context->dlist_indirect_enable &&
		    (NULL != (uint32_t *)*(p_context->pp_dlist_indirect_start))) {
			R_DRW->DLISTSTART = *(p_context->pp_dlist_indirect_start);
			p_context->pp_dlist_indirect_start++;
		} else {
			k_sem_give(&d1_queryirq_sem);
		}
	}
	/* Clear IRQ status. */
	R_BSP_IrqStatusClear(irq);
}

#define DRW_INIT(inst)                                                                             \
	static int drw_renesas_ra_configure_func_##inst(void)                                      \
	{                                                                                          \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(inst, drw, irq)] = ELC_EVENT_DRW_INT;             \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, drw, irq),                                   \
			    DT_INST_IRQ_BY_NAME(inst, drw, priority), drw_zephyr_irq_handler,      \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, drw, irq));                                   \
		return 0;                                                                          \
	}                                                                                          \
	static int renesas_drw_init_##inst(const struct device *dev)                               \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		return drw_renesas_ra_configure_func_##inst();                                     \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(inst, renesas_drw_init_##inst, NULL, NULL, NULL, POST_KERNEL,        \
			      CONFIG_RENESAS_DRW_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(DRW_INIT)
