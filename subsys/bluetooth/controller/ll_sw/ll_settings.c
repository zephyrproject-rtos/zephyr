/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/settings/settings.h>

#include "ll_settings.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_ctlr_ll_settings, LOG_LEVEL_DBG);

#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_VERSION_SETTINGS)

static uint16_t company_id = CONFIG_BT_CTLR_COMPANY_ID;
static uint16_t subversion = CONFIG_BT_CTLR_SUBVERSION_NUMBER;

uint16_t ll_settings_company_id(void)
{
	return company_id;
}
uint16_t ll_settings_subversion_number(void)
{
	return subversion;
}

#endif /* CONFIG_BT_CTLR_VERSION_SETTINGS */

#if defined(CONFIG_BT_CTLR_SMI_TX_SETTING)

static uint8_t smi_tx;

bool ll_settings_smi_tx(void)
{
	return smi_tx;
}

#endif /* CONFIG_BT_CTLR_SMI_TX_SETTING */

static int ctlr_set(const char *name, size_t len_rd,
		    settings_read_cb read_cb, void *store)
{
	ssize_t len;
	int nlen;
	const char *next;

	nlen = settings_name_next(name, &next);

#if defined(CONFIG_BT_CTLR_VERSION_SETTINGS)
	if (!strncmp(name, "company", nlen)) {
		len = read_cb(store, &company_id, sizeof(company_id));
		if (len < 0) {
			LOG_ERR("Failed to read Company Id from storage"
			       " (err %zd)", len);
		} else {
			LOG_DBG("Company Id set to %04x", company_id);
		}
		return 0;
	}
	if (!strncmp(name, "subver", nlen)) {
		len = read_cb(store, &subversion, sizeof(subversion));
		if (len < 0) {
			LOG_ERR("Failed to read Subversion from storage"
			       " (err %zd)", len);
		} else {
			LOG_DBG("Subversion set to %04x", subversion);
		}
		return 0;
	}
#endif /* CONFIG_BT_CTLR_VERSION_SETTINGS */

#if defined(CONFIG_BT_CTLR_SMI_TX_SETTING)
	if (!strncmp(name, "smi_tx", nlen)) {
		len = read_cb(store, &smi_tx, sizeof(smi_tx));
		if (len < 0) {
			LOG_ERR("Failed to read SMI TX flag from storage"
			       " (err %zd)", len);
		} else {
			LOG_DBG("SMI TX flag set to %04x", smi_tx);
		}
		return 0;
	}
#endif /* CONFIG_BT_CTLR_SMI_TX_SETTING */

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt_ctlr, "bt/ctlr", NULL, ctlr_set, NULL, NULL);
