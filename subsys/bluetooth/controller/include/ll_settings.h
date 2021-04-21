/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CTLR_VERSION_SETTINGS)

uint16_t ll_settings_company_id(void);
uint16_t ll_settings_subversion_number(void);

#else

static inline uint16_t ll_settings_company_id(void)
{
	return CONFIG_BT_CTLR_COMPANY_ID;
}
static inline uint16_t ll_settings_subversion_number(void)
{
	return CONFIG_BT_CTLR_SUBVERSION_NUMBER;
}

#endif /* CONFIG_BT_CTLR_VERSION_SETTINGS */

bool ll_settings_smi_tx(void);
