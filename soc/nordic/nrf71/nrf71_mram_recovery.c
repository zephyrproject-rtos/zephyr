/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrfx.h>

#include <nrf71_mram_recovery.h>

void nrf71_mram_recovery_snapshot_request(void)
{
	uint32_t reg = NRF_REGULATORS_NS->MRAMRECOVERY;

	reg &= ~(REGULATORS_MRAMRECOVERY_TAKESNAPSHOT_Msk |
		 REGULATORS_MRAMRECOVERY_KEY_Msk);
	reg |= (REGULATORS_MRAMRECOVERY_TAKESNAPSHOT_takesnapshot
		<< REGULATORS_MRAMRECOVERY_TAKESNAPSHOT_Pos) |
	       (REGULATORS_MRAMRECOVERY_KEY_Key << REGULATORS_MRAMRECOVERY_KEY_Pos);
	NRF_REGULATORS_NS->MRAMRECOVERY = reg;
}

enum nrf71_mram_recovery_snapshot_status nrf71_mram_recovery_snapshot_status_get(void)
{
	return (enum nrf71_mram_recovery_snapshot_status)(
		(NRF_REGULATORS_NS->MRAMRECOVERY &
		 REGULATORS_MRAMRECOVERY_TAKESNAPSHOT_Msk) >>
		REGULATORS_MRAMRECOVERY_TAKESNAPSHOT_Pos);
}

void nrf71_mram_recovery_request(void)
{
	uint32_t reg = NRF_REGULATORS_NS->MRAMRECOVERY;

	reg &= ~REGULATORS_MRAMRECOVERY_KEY_Msk;
	reg |= REGULATORS_MRAMRECOVERY_RECOVERMRAM_Msk |
	       (REGULATORS_MRAMRECOVERY_KEY_Key << REGULATORS_MRAMRECOVERY_KEY_Pos);
	NRF_REGULATORS_NS->MRAMRECOVERY = reg;
}

uint8_t nrf71_mram_recovery_boot_count_get(void)
{
	return (uint8_t)((NRF_REGULATORS_NS->MRAMRECOVERY &
			  REGULATORS_MRAMRECOVERY_BOOTCOUNT_Msk) >>
			 REGULATORS_MRAMRECOVERY_BOOTCOUNT_Pos);
}

void nrf71_mram_recovery_boot_count_clear(void)
{
	uint32_t reg = NRF_REGULATORS_NS->MRAMRECOVERY;

	reg &= ~(REGULATORS_MRAMRECOVERY_BOOTCOUNT_Msk | REGULATORS_MRAMRECOVERY_KEY_Msk);
	reg |= REGULATORS_MRAMRECOVERY_KEY_Key << REGULATORS_MRAMRECOVERY_KEY_Pos;
	NRF_REGULATORS_NS->MRAMRECOVERY = reg;
}
