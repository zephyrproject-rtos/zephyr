/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <nrfx.h>

#include "approtect_setup.h"

void approtect_setup(void)
{
	uint32_t reset_behavior = NRF_TAMPC->PROTECT.RESETBEHAVIOR.CTRL;

	/* Set RESETBEHAVIOR such that a soft-reset clears the locked APPROTECT registers. */
	NRF_TAMPC->PROTECT.RESETBEHAVIOR.CTRL =
		((TAMPC_PROTECT_RESETBEHAVIOR_CTRL_WRITEPROTECTION_Clear
		  << TAMPC_PROTECT_RESETBEHAVIOR_CTRL_WRITEPROTECTION_Pos) |
		 (TAMPC_PROTECT_RESETBEHAVIOR_CTRL_KEY_KEY
		  << TAMPC_PROTECT_RESETBEHAVIOR_CTRL_KEY_Pos));
	NRF_TAMPC->PROTECT.RESETBEHAVIOR.CTRL = ((TAMPC_PROTECT_RESETBEHAVIOR_CTRL_VALUE_High
						  << TAMPC_PROTECT_RESETBEHAVIOR_CTRL_VALUE_Pos) |
						 (TAMPC_PROTECT_RESETBEHAVIOR_CTRL_KEY_KEY
						  << TAMPC_PROTECT_RESETBEHAVIOR_CTRL_KEY_Pos));

	/* Barriers to ensure register write lands before reset takes effect. */
	__DSB();
	__ISB();

	if (((NRF_TAMPC->PROTECT.DOMAIN[0].DBGEN.CTRL & TAMPC_PROTECT_DOMAIN_DBGEN_CTRL_LOCK_Msk) !=
	     0U) &&
	    ((reset_behavior & TAMPC_PROTECT_RESETBEHAVIOR_CTRL_VALUE_Msk) == 0U)) {
		__DSB();
		__ISB();
		NRF_CTRLAP->RESET = CTRLAPPERI_RESET_RESET_SoftReset << CTRLAPPERI_RESET_RESET_Pos;

		/* Reset will terminate execution here. */
		while (1) {
		}
		CODE_UNREACHABLE;
	}
}
