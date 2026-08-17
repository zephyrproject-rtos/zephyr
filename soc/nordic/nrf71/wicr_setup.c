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

/* When out-of-tree patch is included, it defines the LMAC and UMAC patch
 * addresses and takes precedence over the devicetree values.
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
	bool write_enabled = false;
	int err = 0;

	for (size_t i = 0; i < ARRAY_SIZE(wicr_words); i++) {
		volatile uint32_t *reg = (uint32_t *)(WICR_BASE + wicr_words[i].offset);

		/* Skip unchanged values to avoid MRAM wear. */
		if (*reg == wicr_words[i].value) {
			continue;
		}

		if (!write_enabled) {
			nrfx_mramc_confignvr_perm_set(true, MRAM_CONFIGNVR_WICR_PAGE);
			write_enabled = true;
		}

		*reg = wicr_words[i].value;

		if (*reg != wicr_words[i].value) {
			err = -EIO;
			break;
		}
	}

	if (write_enabled) {
		nrfx_mramc_confignvr_perm_set(false, MRAM_CONFIGNVR_WICR_PAGE);
	}

	return err;
}
