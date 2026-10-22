/*
 * Copyright (c) 2022 Vaishnav Achath
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_CC13X2_CC26X2_BOOST_MODE
#define CCFG_FORCE_VDDR_HH 1
#endif

#ifdef CONFIG_CC13X2_CC26X2_BOOTLOADER_ENABLE
#define SET_CCFG_BL_CONFIG_BOOTLOADER_ENABLE 0xC5
#else
#define SET_CCFG_BL_CONFIG_BOOTLOADER_ENABLE 0x00
#endif /* CONFIG_CC13X2_CC26X2_BOOTLOADER_ENABLE */

#ifdef CONFIG_CC13X2_CC26X2_BOOTLOADER_BACKDOOR_ENABLE
#define SET_CCFG_BL_CONFIG_BL_ENABLE 0xC5
#define SET_CCFG_BL_CONFIG_BL_PIN_NUMBER CONFIG_CC13X2_CC26X2_BOOTLOADER_BACKDOOR_PIN
#define SET_CCFG_BL_CONFIG_BL_LEVEL CONFIG_CC13X2_CC26X2_BOOTLOADER_BACKDOOR_LEVEL
#else
#define SET_CCFG_BL_CONFIG_BL_ENABLE 0x00
#endif /* CONFIG_CC13X2_CC26X2_BOOTLOADER_BACKDOOR_ENABLE */

#if defined(CONFIG_SOC_CC1352P7) || defined(CONFIG_SOC_CC2652P7)
/* Workaround required to be able to establish links between P and R devices,
 * see SimpleLink(TM) cc13xx_cc26xx SDK 6.20+ Release Notes, Known Issues:
 * https://software-dl.ti.com/simplelink/esd/simplelink_cc13xx_cc26xx_sdk/6.20.00.29/exports/release_notes_simplelink_cc13xx_cc26xx_sdk_6_20_00_29.html#known-issues
 */
#define SET_CCFG_MODE_CONF_XOSC_CAP_MOD        0x0
#define SET_CCFG_MODE_CONF_XOSC_CAPARRAY_DELTA CONFIG_CC13X2_CC26X2_XOSC_CAPARRAY_DELTA
#endif

/* TI recommends setting CCFG values and then including the TI provided ccfg.c */
#include <startup_files/ccfg.c>
