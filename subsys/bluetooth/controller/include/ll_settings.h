/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CTLR_VERSION_SETTINGS)
#define LL_VERSION_SETTINGS
#else /* !CONFIG_BT_CTLR_VERSION_SETTINGS */
#define LL_VERSION_SETTINGS static __attribute__((always_inline)) inline
#endif /* !CONFIG_BT_CTLR_VERSION_SETTINGS */

/* Version Interfaces */
LL_VERSION_SETTINGS uint16_t ll_settings_company_id(void);
LL_VERSION_SETTINGS uint16_t ll_settings_subversion_number(void);

/* Stable Modulation Index Interfaces */
bool ll_settings_smi_tx(void);

/* Static inline functions */
#if !defined(CONFIG_BT_CTLR_VERSION_SETTINGS)
LL_VERSION_SETTINGS uint16_t ll_settings_company_id(void)
{
	return CONFIG_BT_CTLR_COMPANY_ID;
}
LL_VERSION_SETTINGS uint16_t ll_settings_subversion_number(void)
{
	return CONFIG_BT_CTLR_SUBVERSION_NUMBER;
}
#endif /* !CONFIG_BT_CTLR_VERSION_SETTINGS */
