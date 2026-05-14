/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RA8x2 LPM Standby Wake Source definitions
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_POWER_RENESAS_RA8X2_LPM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_POWER_RENESAS_RA8X2_LPM_H_

/**
 * @name RA8x2 wake sources (WUPEN1:WUPEN0 bit positions)
 * @{
 */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ0    0  /**< IRQ0. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ1    1  /**< IRQ1. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ2    2  /**< IRQ2. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ3    3  /**< IRQ3. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ4    4  /**< IRQ4. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ5    5  /**< IRQ5. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ6    6  /**< IRQ6. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ7    7  /**< IRQ7. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ8    8  /**< IRQ8. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ9    9  /**< IRQ9. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ10   10 /**< IRQ10. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ11   11 /**< IRQ11. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ12   12 /**< IRQ12. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ13   13 /**< IRQ13. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ14   14 /**< IRQ14. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ15   15 /**< IRQ15. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IWDT    16 /**< Independent Watchdog interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_LVD1    18 /**< Low Voltage Detection 1 interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_LVD2    19 /**< Low Voltage Detection 2 interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_VBATT   20 /**< VBATT Monitor interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_RTCALM  24 /**< RTC Alarm interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_RTCPRD  25 /**< RTC Period interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_USBHS   26 /**< USB High-speed interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_USBFS   27 /**< USB Full-speed interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT1UD  28 /**< AGT1 Underflow interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT1CA  29 /**< AGT1 Compare Match A interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_AGT1CB  30 /**< AGT1 Compare Match B interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IIC0    31 /**< I2C 0 interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_COMPHS0 35 /**< Comparator-HS0 Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_SOSTD   39 /**< SOSTD interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_ULP0U   40 /**< ULPT0 Underflow Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_ULP0A   41 /**< ULPT0 Compare Match A Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_ULP0B   42 /**< ULPT0 Compare Match B Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_I3C0    43 /**< I3C0 address match interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_ULP1U   44 /**< ULPT1 Underflow Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_ULP1A   45 /**< ULPT1 Compare Match A Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_ULP1B   46 /**< ULPT1 Compare Match B Interrupt. */
#define RA_LPM_STANDBY_WAKE_SOURCE_PDM     47 /**< PDM Sound Detection. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ16   48 /**< IRQ16. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ17   49 /**< IRQ17. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ18   50 /**< IRQ18. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ19   51 /**< IRQ19. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ20   52 /**< IRQ20. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ21   53 /**< IRQ21. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ22   54 /**< IRQ22. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ23   55 /**< IRQ23. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ24   56 /**< IRQ24. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ25   57 /**< IRQ25. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ26   58 /**< IRQ26. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ27   59 /**< IRQ27. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ28   60 /**< IRQ28. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ29   61 /**< IRQ29. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ30   62 /**< IRQ30. */
#define RA_LPM_STANDBY_WAKE_SOURCE_IRQ31   63 /**< IRQ31. */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_POWER_RENESAS_RA8X2_LPM_H_ */
