/*
 * Copyright (c) 2026 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

#ifdef CONFIG_SOC_SERIES_CC13X4_CC26X4_BOOTLOADER_ENABLE
#define SET_CCFG_BL_CONFIG_BOOTLOADER_ENABLE 0xC5
#else
#define SET_CCFG_BL_CONFIG_BOOTLOADER_ENABLE 0x00
#endif

#ifdef CONFIG_SOC_SERIES_CC13X4_CC26X4_BOOTLOADER_BACKDOOR_ENABLE
#define SET_CCFG_BL_CONFIG_BL_ENABLE     0xC5
#define SET_CCFG_BL_CONFIG_BL_PIN_NUMBER DT_PROP(DT_NODELABEL(backdoor), pin)
#define SET_CCFG_BL_CONFIG_BL_LEVEL      DT_PROP(DT_NODELABEL(backdoor), level)
#else
#define SET_CCFG_BL_CONFIG_BL_ENABLE 0x00
#endif

/* Use RCOSC_LF as the LF clock source. LF XOSC requires waiting for the
 * external 32.768 kHz crystal to stabilize (~300ms), which causes the PM
 * subsystem to hold DISALLOW_STANDBY and the oscillator ISR to be pending
 * during early boot. On a cold power reset (no debugger delay), this window
 * causes the device to hang. RCOSC_LF switches immediately and avoids this.
 */
#define SET_CCFG_MODE_CONF_SCLK_LF_OPTION 0x3 /* LF RCOSC */

#define SET_CCFG_MODE_CONF_XOSC_CAP_MOD        0x0
#define SET_CCFG_MODE_CONF_XOSC_CAPARRAY_DELTA CONFIG_SOC_SERIES_CC13X4_CC26X4_XOSC_CAPARRAY_DELTA

#ifdef CONFIG_SOC_SERIES_CC13X4_CC26X4_IDAU_CFG_ENABLE
#define SET_CCFG_TRUSTZONE_FLASH_CFG_NSADDR_BOUNDARY                                               \
	CONFIG_SOC_SERIES_CC13X4_CC26X4_TZ_FLASH_NS_BOUNDARY
#define SET_CCFG_TRUSTZONE_SRAM_CFG_NSADDR_BOUNDARY                                                \
	CONFIG_SOC_SERIES_CC13X4_CC26X4_TZ_SRAM_NS_BOUNDARY
#endif

#ifdef CONFIG_SOC_SERIES_CC13X4_CC26X4_SRAM_PARITY_DISABLE
#define SET_CCFG_BCFG_SRAM_PARITY_DIS 0x1
#else
#define SET_CCFG_BCFG_SRAM_PARITY_DIS 0x0
#endif

#include <startup_files/ccfg.c>
