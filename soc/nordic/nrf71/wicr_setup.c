/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/autoconf.h>
#include <zephyr/devicetree.h>

#include <nrfx_mramc.h>
#include "wicr_setup.h"

#define WICR_NODE DT_NODELABEL(wicr)
#define WICR_BASE DT_REG_ADDR(WICR_NODE)
#define MRAM_CONFIGNVR_WICR_PAGE 0

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

/* A Wi-Fi ROM patch build defines the LMAC and UMAC patch addresses, taken
 * from the patch manifest and reserved in MRAM by the build system, and those
 * take precedence. Without a patch the devicetree values are used, which point
 * at the entry the ROM uses when no patch is applied.
 */
#ifdef NRF71_WIFI_LMAC_PATCH_ADDR
#define WICR_LMAC_PATCH_ADDR NRF71_WIFI_LMAC_PATCH_ADDR
#else
#define WICR_LMAC_PATCH_ADDR \
	DT_REG_ADDR(DT_PHANDLE(WICR_NODE, firmware_lmacrompatchaddr))
#endif /* NRF71_WIFI_LMAC_PATCH_ADDR */

#ifdef NRF71_WIFI_UMAC_PATCH_ADDR
#define WICR_UMAC_PATCH_ADDR NRF71_WIFI_UMAC_PATCH_ADDR
#else
#define WICR_UMAC_PATCH_ADDR \
	DT_REG_ADDR(DT_PHANDLE(WICR_NODE, firmware_umacrompatchaddr))
#endif /* NRF71_WIFI_UMAC_PATCH_ADDR */

struct wicr_word {
	uint16_t offset;
	uint32_t value;
};

static const struct wicr_word wicr_words[] = {
	{0x000, DT_REG_ADDR(DT_PHANDLE(WICR_NODE, firmware_lmacinitpc))},
	{0x004, DT_REG_ADDR(DT_PHANDLE(WICR_NODE, firmware_umacinitpc))},
	{0x008, WICR_LMAC_PATCH_ADDR},
	{0x00C, WICR_UMAC_PATCH_ADDR},
	{0x080, DT_REG_ADDR(DT_PHANDLE(WICR_NODE, ipcconfig_commandmbox))},
	{0x084, DT_REG_SIZE(DT_PHANDLE(WICR_NODE, ipcconfig_commandmbox))},
	{0x088, DT_REG_ADDR(DT_PHANDLE(WICR_NODE, ipcconfig_eventmbox))},
	{0x08C, DT_REG_SIZE(DT_PHANDLE(WICR_NODE, ipcconfig_eventmbox))},
	{0x090, DT_REG_ADDR(DT_PHANDLE(WICR_NODE, ipcconfig_sparembox))},
	{0x094, DT_REG_SIZE(DT_PHANDLE(WICR_NODE, ipcconfig_sparembox))},
};

int wicr_setup(void)
{
	int err = 0;

	while (!nrfx_mramc_ready_check()) {
		/* Wait until MRAMC is ready for the next operation. */
	}

	/* The page permissions gate reads as well as writes, so the page has to
	 * be unlocked before the comparison below is meaningful. Reading it
	 * while locked yields 0xFFFFFFFF for every word, so no word ever
	 * matches and the whole block is rewritten on every boot.
	 */
	nrfx_mramc_confignvr_perm_set(true, MRAM_CONFIGNVR_WICR_PAGE);

	for (size_t i = 0; i < ARRAY_SIZE(wicr_words); i++) {
		volatile uint32_t *reg = (uint32_t *)(WICR_BASE + wicr_words[i].offset);

		/* Skip unchanged values to avoid MRAM wear. */
		if (*reg == wicr_words[i].value) {
			continue;
		}

		*reg = wicr_words[i].value;

		while (!nrfx_mramc_ready_check()) {
			/* Let the write commit before reading it back. */
		}

		if (*reg != wicr_words[i].value) {
			err = -EIO;
			break;
		}
	}

	while (!nrfx_mramc_ready_check()) {
		/* Do not lock the page with a write still in flight. */
	}

	nrfx_mramc_confignvr_perm_set(false, MRAM_CONFIGNVR_WICR_PAGE);

	return err;
}
