/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The counter is the simulation time in microseconds, which is also the time
 * the radio model takes the times of its operations in.
 */

#include <stdbool.h>
#include <stdint.h>

#include "bs_types.h"
#include "nsi_hw_scheduler.h"
#include "nsi_hws_models_if.h"
#include "NHW_common_types.h"
#include "NHW_config.h"
#include "irq_ctrl.h"

#include "nbr_cntr_if.h"

/* The interrupt goes to the CPU that the nRF RADIO would interrupt */
static const struct nhw_irq_mapping irq_map[] = NHW_RADIO_INT_MAP;

static unsigned int cntr_irq;
static bs_time_t timer_cntr = TIME_NEVER;
static bool cntr_cmp_evt;

static void cntr_timer_triggered(void)
{
	timer_cntr = TIME_NEVER;
	nsi_hws_find_next_event();

	cntr_cmp_evt = true;
	hw_irq_ctrl_raise_im(irq_map[0].cntl_inst, cntr_irq);
}

NSI_HW_EVENT(timer_cntr, cntr_timer_triggered, 50);

void nbr_cntr_init(unsigned int irq)
{
	cntr_irq = irq;
	timer_cntr = TIME_NEVER;
	cntr_cmp_evt = false;
	nsi_hws_find_next_event();
}

uint64_t nbr_cntr_get(void)
{
	return nsi_hws_get_time();
}

void nbr_cntr_cmp_set(uint64_t value)
{
	timer_cntr = (value > nsi_hws_get_time()) ? value : TIME_NEVER;
	nsi_hws_find_next_event();
}

void nbr_cntr_cmp_disable(void)
{
	timer_cntr = TIME_NEVER;
	nsi_hws_find_next_event();
}

bool nbr_cntr_cmp_evt_get_clear(void)
{
	bool evt = cntr_cmp_evt;

	cntr_cmp_evt = false;

	return evt;
}
