/*
 * Copyright 2024 NXP
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
#if defined(CONFIG_BT) || defined(CONFIG_IEEE802154)
	/* NBU interface Interrupt */
	IRQ_CONNECT(NBU_RX_IRQ_N, NBU_RX_IRQ_P, nbu_handler, 0, 0);
	irq_enable(NBU_RX_IRQ_N);

#if DT_INST_IRQ_HAS_NAME(0, wakeup_int)
	/* Wake up done interrupt */
	IRQ_CONNECT(NBU_WAKE_UP_IRQ_N, NBU_WAKE_UP_IRQ_P, nbu_wakeup_done_handler, 0, 0);
	irq_enable(NBU_WAKE_UP_IRQ_N);
#endif
#if (DT_INST_PROP(0, wakeup_source)) && CONFIG_PM
	NXP_ENABLE_WAKEUP_SIGNAL(NBU_RX_IRQ_N);
#endif

#endif
}
