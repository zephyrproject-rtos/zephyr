/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Radio initialization for Silicon Labs Series 1 products
 */

#include <zephyr/kernel.h>

#include <soc_radio.h>

#define RADIO_IRQN(name)     DT_IRQ_BY_NAME(DT_NODELABEL(radio), name, irq)
#define RADIO_IRQ_PRIO(name) DT_IRQ_BY_NAME(DT_NODELABEL(radio), name, priority)

void rail_isr_installer(void)
{
	IRQ_CONNECT(RADIO_IRQN(agc), RADIO_IRQ_PRIO(agc), AGC_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(bufc), RADIO_IRQ_PRIO(bufc), BUFC_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(frc_pri), RADIO_IRQ_PRIO(frc_pri), FRC_PRI_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(frc), RADIO_IRQ_PRIO(frc), FRC_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(modem), RADIO_IRQ_PRIO(modem), MODEM_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(prortc), RADIO_IRQ_PRIO(prortc), PRORTC_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(protimer), RADIO_IRQ_PRIO(protimer), PROTIMER_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(rac_rsm), RADIO_IRQ_PRIO(rac_rsm), RAC_RSM_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(rac_seq), RADIO_IRQ_PRIO(rac_seq), RAC_SEQ_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(synth), RADIO_IRQ_PRIO(synth), SYNTH_IRQHandler, NULL, 0);
	IRQ_CONNECT(RADIO_IRQN(rfsense), RADIO_IRQ_PRIO(rfsense), RFSENSE_IRQHandler, NULL, 0);
}

void soc_radio_init(void)
{
	rail_isr_installer();
}
