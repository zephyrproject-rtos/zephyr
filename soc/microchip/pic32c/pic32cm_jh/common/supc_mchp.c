/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include "mchp_supc.h"
#include <soc.h>

int supc_mchp_vref_enable(void)
{
	SUPC_REGS->SUPC_VREF |= SUPC_VREF_VREFOE_Msk;
	return 0;
}

int supc_mchp_vref_disable(void)
{
	SUPC_REGS->SUPC_VREF &= ~SUPC_VREF_VREFOE_Msk;
	return 0;
}

int supc_mchp_vref_set_voltage(enum vref_sel vref_sel)
{
	SUPC_REGS->SUPC_VREF |=  SUPC_VREF_SEL(vref_sel);
	return 0;
}

enum vref_sel supc_mchp_vref_get_voltage(void)
{
	return ((SUPC_REGS->SUPC_VREF & SUPC_VREF_SEL_Msk)>>16);
}
