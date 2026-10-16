/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/posix/posix_soc_if.h>

#define HAL_BSR_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_bsim_2g4_radio)

#define HAL_RADIO_IRQn     DT_IRQ_BY_NAME(HAL_BSR_NODE, radio, irq)
#define HAL_CNTR_IRQn      DT_IRQ_BY_NAME(HAL_BSR_NODE, cntr, irq)
#define HAL_SWI_RADIO_IRQ  DT_IRQ_BY_NAME(HAL_BSR_NODE, swi_lll, irq)
#define HAL_SWI_WORKER_IRQ HAL_CNTR_IRQn

#if !defined(CONFIG_BT_CTLR_LOW_LAT) && \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
#define HAL_SWI_JOB_IRQ    HAL_SWI_WORKER_IRQ
#else /* CONFIG_BT_CTLR_LOW_LAT || ULL_HIGH_PRIO != ULL_LOW_PRIO */
#define HAL_SWI_JOB_IRQ    DT_IRQ_BY_NAME(HAL_BSR_NODE, swi_ull_low, irq)
#endif /* CONFIG_BT_CTLR_LOW_LAT || ULL_HIGH_PRIO != ULL_LOW_PRIO */

static inline void hal_swi_init(void)
{
}

static inline void hal_swi_lll_pend(void)
{
	posix_sw_set_pending_IRQ(HAL_SWI_RADIO_IRQ);
}

static inline void hal_swi_worker_pend(void)
{
	posix_sw_set_pending_IRQ(HAL_SWI_WORKER_IRQ);
}

static inline void hal_swi_job_pend(void)
{
	posix_sw_set_pending_IRQ(HAL_SWI_JOB_IRQ);
}
