/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hal/cpu.h"

/* nRF51 and nRF52 Series IRQ mapping*/
#if defined(CONFIG_SOC_SERIES_NRF51) || defined(CONFIG_SOC_COMPATIBLE_NRF52X)

#define HAL_SWI_RADIO_IRQ  SWI4_IRQn
#define HAL_SWI_WORKER_IRQ RTC0_IRQn

#if !defined(CONFIG_BT_CTLR_LOW_LAT) && \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
#define HAL_SWI_JOB_IRQ    HAL_SWI_WORKER_IRQ
#else
#define HAL_SWI_JOB_IRQ    SWI5_IRQn
#endif

#define HAL_RTC_IRQn       RTC0_IRQn

/* nRF53 Series IRQ mapping */
#elif defined(CONFIG_SOC_COMPATIBLE_NRF53X)

/* nRF53 Series Engineering D and Revision 1 IRQ mapping */
#if defined(CONFIG_SOC_COMPATIBLE_NRF5340_CPUNET)

#define HAL_SWI_RADIO_IRQ  SWI2_IRQn
#define HAL_SWI_WORKER_IRQ RTC0_IRQn

#if !defined(CONFIG_BT_CTLR_LOW_LAT) && \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
#define HAL_SWI_JOB_IRQ    HAL_SWI_WORKER_IRQ
#else
#define HAL_SWI_JOB_IRQ    SWI3_IRQn
#endif

#define HAL_RTC_IRQn       RTC0_IRQn

#elif /* !CONFIG_SOC_COMPATIBLE_NRF5340_CPUNET */
#error Unknown NRF5340 CPU.
#endif /* !CONFIG_SOC_COMPATIBLE_NRF5340_CPUNET */

/* nRF54L Series IRQ mapping */
#elif defined(CONFIG_SOC_COMPATIBLE_NRF54LX)

#if defined(CONFIG_RISCV)
/* FLPR (VPR RISC-V) core uses VPRCLIC software interrupts */
#define HAL_SWI_RADIO_IRQ  EGU10_IRQn

#if defined(CONFIG_BT_CTLR_NRF_GRTC)
#define HAL_SWI_WORKER_IRQ GRTC_1_IRQn
#define HAL_RTC_IRQn       GRTC_1_IRQn
#else /* !CONFIG_BT_CTLR_NRF_GRTC */
#error "nRF54L FLPR requires CONFIG_BT_CTLR_NRF_GRTC"
#endif /* !CONFIG_BT_CTLR_NRF_GRTC */

#if !defined(CONFIG_BT_CTLR_LOW_LAT) && \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
#define HAL_SWI_JOB_IRQ    HAL_SWI_WORKER_IRQ
#else
#define HAL_SWI_JOB_IRQ    EGU20_IRQn
#endif

#else /* !CONFIG_RISCV - nRF54L Application core (ARM) */
#define HAL_SWI_RADIO_IRQ  SWI02_IRQn

#if defined(CONFIG_BT_CTLR_NRF_GRTC)
#define HAL_SWI_WORKER_IRQ GRTC_1_IRQn
#define HAL_RTC_IRQn       GRTC_1_IRQn
#else /* !CONFIG_BT_CTLR_NRF_GRTC */
#define HAL_SWI_WORKER_IRQ RTC10_IRQn
#define HAL_RTC_IRQn       RTC10_IRQn
#endif /* !CONFIG_BT_CTLR_NRF_GRTC */

#if !defined(CONFIG_BT_CTLR_LOW_LAT) && \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
#define HAL_SWI_JOB_IRQ    HAL_SWI_WORKER_IRQ
#else
#define HAL_SWI_JOB_IRQ    SWI03_IRQn
#endif
#endif /* !CONFIG_RISCV */

#endif

static inline void hal_swi_init(void)
{
	/* No platform-specific initialization required. */
}

static inline void hal_swi_lll_pend(void)
{
	cpu_irq_pending_set(HAL_SWI_RADIO_IRQ);
}

static inline void hal_swi_worker_pend(void)
{
	cpu_irq_pending_set(HAL_SWI_WORKER_IRQ);
}

static inline void hal_swi_job_pend(void)
{
	cpu_irq_pending_set(HAL_SWI_JOB_IRQ);
}
