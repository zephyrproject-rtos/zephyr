/*
 * Copyright (c) 2024 David Ullmann
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Build-only test: verifies that the public lora.h API compiles correctly
 * for all supported modulation types (LoRa, FSK).  No runtime assertions
 * are made; the file is intentionally empty at run time.
 */

#include <zephyr/drivers/lora.h>

/*
 * Compile-time check: LORA_MOD_LORA must be 0 so that zero-initialised
 * lora_modem_config structs default to LoRa mode (backward compatibility).
 */
BUILD_ASSERT(LORA_MOD_LORA == 0,
	"LORA_MOD_LORA must be 0 so zero-initialised structs default to LoRa");

/*
 * Verify that all FSK public types are accessible from <zephyr/drivers/lora.h>
 * alone, without including any driver-private header.
 */
static void fsk_api_compile_check(void)
{
	struct lora_modem_config cfg = {0};

	/* LoRa path - existing code must still compile unchanged */
	cfg.frequency   = 868100000U;
	cfg.bandwidth   = BW_125_KHZ;
	cfg.datarate    = SF_10;
	cfg.coding_rate = CR_4_5;
	cfg.preamble_len = 8U;
	cfg.tx_power    = 14;
	cfg.tx          = true;

	/* FSK path - new fields added in this PR */
	cfg.modulation       = LORA_MOD_FSK;
	cfg.fsk.bitrate      = 50000U;
	cfg.fsk.fdev         = 25000U;
	cfg.fsk.shaping      = LORA_FSK_SHAPING_GAUSS_BT_0_5;
	cfg.fsk.bandwidth    = FSK_BW_117_KHZ;
	cfg.fsk.preamble_len = 5U;
	cfg.fsk.variable_len = true;
	cfg.fsk.crc_on       = true;
	cfg.fsk.whitening    = true;
	cfg.fsk.sync_word_len = 0U;

	(void)cfg;
}

int main(void)
{
	fsk_api_compile_check();
	return 0;
}
