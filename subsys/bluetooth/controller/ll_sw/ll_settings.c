/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <string.h>

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include "ll_settings.h"

#define LOG_MODULE_NAME bt_ctlr_ll_settings
#include "common/log.h"
#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_VERSION_SETTINGS)

static u16_t company_id = CONFIG_BT_CTLR_COMPANY_ID;
static u16_t subversion = CONFIG_BT_CTLR_SUBVERSION_NUMBER;

u16_t ll_settings_company_id(void)
{
	return company_id;
}
u16_t ll_settings_subversion_number(void)
{
	return subversion;
}

#endif /* CONFIG_BT_CTLR_VERSION_SETTINGS */

static int ctlr_set(const char *name, size_t len_rd,
		    settings_read_cb read_cb, void *store)
{
	int len, nlen;
	const char *next;

	nlen = settings_name_next(name, &next);

#if defined(CONFIG_BT_CTLR_VERSION_SETTINGS)
	if (!strncmp(name, "company", nlen)) {
		len = read_cb(store, &company_id, sizeof(company_id));
		if (len < 0) {
			BT_ERR("Failed to read Company Id from storage"
			       " (err %d)", len);
		} else {
			BT_DBG("Company Id set to %04x", company_id);
		}
		return 0;
	}
	if (!strncmp(name, "subver", nlen)) {
		len = read_cb(store, &subversion, sizeof(subversion));
		if (len < 0) {
			BT_ERR("Failed to read Subversion from storage"
			       " (err %d)", len);
		} else {
			BT_DBG("Subversion set to %04x", subversion);
		}
		return 0;
	}
#endif /* CONFIG_BT_CTLR_VERSION_SETTINGS */

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt_ctlr, "bt/ctlr", NULL, ctlr_set, NULL, NULL);
