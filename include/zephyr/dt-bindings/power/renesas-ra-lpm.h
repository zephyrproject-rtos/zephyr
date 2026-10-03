/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RA LPM Standby Wake Source definitions
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_POWER_RENESAS_RA_LPM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_POWER_RENESAS_RA_LPM_H_

#if defined(CONFIG_SOC_SERIES_RA8D2) || defined(CONFIG_SOC_SERIES_RA8M2) ||                        \
	defined(CONFIG_SOC_SERIES_RA8P1) || defined(CONFIG_SOC_SERIES_RA8T2)
#include "renesas-ra8x2-lpm.h"
#else

/**
 * @name Default wake sources (WUPEN1:WUPEN0 bit positions)
 * @{
 */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ0     0  /**< IRQ0. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ1     1  /**< IRQ1. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ2     2  /**< IRQ2. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ3     3  /**< IRQ3. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ4     4  /**< IRQ4. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ5     5  /**< IRQ5. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ6     6  /**< IRQ6. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ7     7  /**< IRQ7. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ8     8  /**< IRQ8. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ9     9  /**< IRQ9. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ10    10 /**< IRQ10. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ11    11 /**< IRQ11. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ12    12 /**< IRQ12. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ13    13 /**< IRQ13. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ14    14 /**< IRQ14. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ15    15 /**< IRQ15. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IWDT     16 /**< Independent Watchdog interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_KEY      17 /**< Key interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_LVD1     18 /**< Low Voltage Detection 1 interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_LVD2     19 /**< Low Voltage Detection 2 interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_VBATT    20 /**< VBATT Monitor interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_VRTC     21 /**< LVDVRTC interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_EXLVD    22 /**< LVDEXLVD interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_ACMPHS0  22 /**< Analog Comparator High-speed 0. */
#define RA_LPM_STANDBY_WAKE_SOURCE_ACMPLP0  23 /**< Analog Comparator Low-speed 0. */
#define RA_LPM_STANDBY_WAKE_SOURCE_RTCALM1  23 /**< RTC Alarm interrupt 1. */
#define RA_LPM_STANDBY_WAKE_SOURCE_RTCALM   24 /**< RTC Alarm interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_RTCPRD   25 /**< RTC Period interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_USBHS    26 /**< USB High-speed interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_USBFS    27 /**< USB Full-speed interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGTW0UD  27 /**< AGTW0 Underflow interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGTW1UD  28 /**< AGTW1 Underflow interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGTW1CA  29 /**< AGTW1 Compare Match A interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGTW1CB  30 /**< AGTW1 Compare Match B interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT1UD   28 /**< AGT1 Underflow interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT1CA   29 /**< AGT1 Compare Match A interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT1CB   30 /**< AGT1 Compare Match B interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IIC0     31 /**< I2C 0 interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT0UD   32 /**< AGT0 Underflow interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT3UD   32 /**< AGT3 Underflow interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT1UD_S 33 /**< AGT1 Underflow (board-specific). */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT3CA   33 /**< AGT3 Compare Match A interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT2UD   34 /**< AGT2 Underflow interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT3CB   34 /**< AGT3 Compare Match B interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT3UD_S 35 /**< AGT3 Underflow (board-specific). */
#define RA_LPM_STANDBY_WAKE_SOURCE_COMPHS0  35 /**< Comparator-HS0 Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT4UD   36 /**< AGT4 Underflow interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT5UD   37 /**< AGT5 Underflow interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT6UD   38 /**< AGT6 Underflow interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT7UD   39 /**< AGT7 Underflow interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_SOSTD    40 /**< SOSTD interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_ULP0U    40 /**< ULPT0 Underflow Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_ULP0A    41 /**< ULPT0 Compare Match A Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_ULP0B    42 /**< ULPT0 Compare Match B Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_I3C0     43 /**< I3C0 address match interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_ULP1U    44 /**< ULPT1 Underflow Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_ULP1A    45 /**< ULPT1 Compare Match A Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_ULP1B    46 /**< ULPT1 Compare Match B Interrupt. */
/** @} */

/**
 * @name Extended wake sources (WUPEN2 bit positions, separate register)
 * @{
 */
#define RA_LPM_STANDBY_WAKE_SOURCE_INTUR0   0  /**< UARTA0 INTUR Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_INTURE0  1  /**< UARTA0 INTURE Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_INTUR1   2  /**< UARTA1 INTUR Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_INTURE1  3  /**< UARTA1 INTURE Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_USBCCS   4  /**< USBCC Status Change Interrupt. */
/** @} */

#endif

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_POWER_RENESAS_RA_LPM_H_ */
