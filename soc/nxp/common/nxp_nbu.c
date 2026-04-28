/*
 * Copyright 2024, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Interrupt Narrow Band Unit. This is meant for a RCP radio device.
 */

/* -------------------------------------------------------------------------- */
/*                                  Includes                                  */
/* -------------------------------------------------------------------------- */
#include <zephyr/irq.h>
#include <soc.h>

/* -------------------------------------------------------------------------- */
/*                                  Definitions                               */
/* -------------------------------------------------------------------------- */
#define DT_DRV_COMPAT nxp_nbu

#define NBU_RX_IRQ_N DT_IRQ_BY_NAME(DT_DRV_INST(0), nbu_rx_int, irq)
#define NBU_RX_IRQ_P DT_IRQ_BY_NAME(DT_DRV_INST(0), nbu_rx_int, priority)
#if DT_INST_IRQ_HAS_NAME(0, wakeup_int)
#define NBU_WAKE_UP_IRQ_N DT_IRQ_BY_NAME(DT_DRV_INST(0), wakeup_int, irq)
#define NBU_WAKE_UP_IRQ_P DT_IRQ_BY_NAME(DT_DRV_INST(0), wakeup_int, priority)
#endif

/* Check if XTAL properties are set and get values */
#if DT_INST_NODE_HAS_PROP(0, rfmc_xo_cdac)
#define NBU_RFMC_XO_CDAC_VALUE DT_INST_PROP(0, rfmc_xo_cdac)
#endif

#if DT_INST_NODE_HAS_PROP(0, rfmc_xo_isel)
#define NBU_RFMC_XO_ISEL_VALUE DT_INST_PROP(0, rfmc_xo_isel)
#endif

/* -------------------------------------------------------------------------- */
/*                             Private prototypes                             */
/* -------------------------------------------------------------------------- */
extern int32_t nbu_handler(void);
extern int32_t nbu_wakeup_done_handler(void);

/* -------------------------------------------------------------------------- */
/*                             Public function                                */
/* -------------------------------------------------------------------------- */

void nxp_nbu_init(void)
{
#if defined(NBU_RFMC_XO_CDAC_VALUE) || defined(NBU_RFMC_XO_ISEL_VALUE)
	uint32_t rfmc_xo;

	rfmc_xo = RFMC->XO_TEST;

#if defined(NBU_RFMC_XO_CDAC_VALUE)
	/* Apply a Trim value/ Current selection for the crystal oscillator */
	rfmc_xo &= ~(RFMC_XO_TEST_CDAC_MASK);
	rfmc_xo |= RFMC_XO_TEST_CDAC(NBU_RFMC_XO_CDAC_VALUE);
#endif

#if defined(NBU_RFMC_XO_ISEL_VALUE)
	rfmc_xo &= ~(RFMC_XO_TEST_ISEL_MASK);
	rfmc_xo |= RFMC_XO_TEST_ISEL(NBU_RFMC_XO_ISEL_VALUE);
#endif

	RFMC->XO_TEST = rfmc_xo;
#endif
	/* NBU interface Interrupt */
	IRQ_CONNECT(NBU_RX_IRQ_N, NBU_RX_IRQ_P, nbu_handler, 0, 0);

#if DT_INST_IRQ_HAS_NAME(0, wakeup_int)
	/* Wake up done interrupt */
	IRQ_CONNECT(NBU_WAKE_UP_IRQ_N, NBU_WAKE_UP_IRQ_P, nbu_wakeup_done_handler, 0, 0);
#endif
#if (DT_INST_PROP(0, wakeup_source)) && CONFIG_PM
	NXP_ENABLE_WAKEUP_SIGNAL(NBU_RX_IRQ_N);
#endif
}
