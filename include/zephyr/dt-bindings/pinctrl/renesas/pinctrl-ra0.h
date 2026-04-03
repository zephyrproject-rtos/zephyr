/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RA0_H__
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RA0_H__

/**
 * @file
 * @brief Renesas RA0 pinctrl pin configuration definitions.
 */

/**
 * @name Renesas RA0 pin configuration bit field positions and masks.
 * @{
 */

/** Position of the port field. */
#define RA_PORT_NUM_POS  0
/** Mask of the port field. */
#define RA_PORT_NUM_MASK 0xf
/** Position of the pin field. */
#define RA_PIN_NUM_POS   4
/** Mask of the pin field. */
#define RA_PIN_NUM_MASK  0xf
/** Position of the function field. */
#define RA_PSEL_POS      8
/** Mask for the function field. */
#define RA_PSEL_MASK     0x1f

/** @} */

/**
 * @name Renesas RA0 pinctrl pin functions.
 * @{
 */

/** JTAG/SWD SWDIO */
#define RA_PSEL_P1nPFS_SWDIO    0x1
/** TAU04 TI04A */
#define RA_PSEL_P1nPFS_TI04A    0x1
/** TAU04 TO04A */
#define RA_PSEL_P1nPFS_TO04A    0x1
/** TAU07 TI07A */
#define RA_PSEL_P1nPFS_TI07A    0x1
/** TAU07 TO07A */
#define RA_PSEL_P1nPFS_TO07A    0x1
/** TAU06 TI06A */
#define RA_PSEL_P1nPFS_TI06A    0x1
/** TAU06 TO06A */
#define RA_PSEL_P1nPFS_TO06A    0x1
/** TAU05 TI05A */
#define RA_PSEL_P1nPFS_TI05A    0x1
/** TAU05 TO05A */
#define RA_PSEL_P1nPFS_TO05A    0x1
/** TAU02 TI02D */
#define RA_PSEL_P1nPFS_TI02D    0x1
/** TAU02 TO02D */
#define RA_PSEL_P1nPFS_TO02D    0x1
/** TAU01 TI01D */
#define RA_PSEL_P1nPFS_TI01D    0x1
/** TAU01 TO01D */
#define RA_PSEL_P1nPFS_TO01D    0x1
/** TAU02 TI02A */
#define RA_PSEL_P1nPFS_TI02A    0x1
/** TAU02 TO02A */
#define RA_PSEL_P1nPFS_TO02A    0x1
/** TAU01 TI01A */
#define RA_PSEL_P1nPFS_TI01A    0x1
/** TAU01 TO01A */
#define RA_PSEL_P1nPFS_TO01A    0x1
/** TAU07 TI07B */
#define RA_PSEL_P1nPFS_TI07B    0x1
/** TAU07 TO07B */
#define RA_PSEL_P1nPFS_TO07B    0x1
/** TAU03 TI03A */
#define RA_PSEL_P1nPFS_TI03A    0x1
/** TAU03 TO03A */
#define RA_PSEL_P1nPFS_TO03A    0x1
/** TAU01 TI01B */
#define RA_PSEL_P1nPFS_TI01B    0x2
/** TAU01 TO01B */
#define RA_PSEL_P1nPFS_TO01B    0x2
/** TAU00 TI00C */
#define RA_PSEL_P1nPFS_TI00C    0x2
/** TAU00 TO00C */
#define RA_PSEL_P1nPFS_TO00C    0x2
/** SAU_SPI00 SSI00A */
#define RA_PSEL_P1nPFS_SSI00A   0x2
/** TAU00 TI00D */
#define RA_PSEL_P1nPFS_TI00D    0x2
/** TAU00 TO00D */
#define RA_PSEL_P1nPFS_TO00D    0x2
/** TAU03 TI03B */
#define RA_PSEL_P1nPFS_TI03B    0x2
/** TAU03 TO03B */
#define RA_PSEL_P1nPFS_TO03B    0x2
/** SAU_UART2 TXD2A */
#define RA_PSEL_P1nPFS_TXD2A    0x2
/** SAU_SPI20 SO20A */
#define RA_PSEL_P1nPFS_SO20A    0x2
/** SAU_UART2 RXD2A */
#define RA_PSEL_P1nPFS_RXD2A    0x2
/** SAU_SPI20 SI20A */
#define RA_PSEL_P1nPFS_SI20A    0x2
/** SAU_I2C20 SDA20A */
#define RA_PSEL_P1nPFS_SDA20A   0x2
/** SAU_SPI20 SCK20A */
#define RA_PSEL_P1nPFS_SCK20A   0x2
/** SAU_I2C20 SCL20A */
#define RA_PSEL_P1nPFS_SCL20A   0x2
/** SAU_UART0 RXD0A */
#define RA_PSEL_P1nPFS_RXD0A    0x3
/** SAU_SPI00 SI00A */
#define RA_PSEL_P1nPFS_SI00A    0x3
/** SAU_I2C00 SDA00A */
#define RA_PSEL_P1nPFS_SDA00A   0x3
/** SAU_UART0 TXD0A */
#define RA_PSEL_P1nPFS_TXD0A    0x3
/** SAU_SPI00 SO00A */
#define RA_PSEL_P1nPFS_SO00A    0x3
/** SAU_SPI00 SCK00A */
#define RA_PSEL_P1nPFS_SCK00A   0x3
/** SAU_I2C00 SCL00A */
#define RA_PSEL_P1nPFS_SCL00A   0x3
/** SAU_SPI10 SCK10A */
#define RA_PSEL_P1nPFS_SCK10A   0x3
/** SAU_I2C10 SCL10A */
#define RA_PSEL_P1nPFS_SCL10A   0x3
/** SAU_SPI10 SI10A */
#define RA_PSEL_P1nPFS_SI10A    0x3
/** SAU_I2C10 SDA10A */
#define RA_PSEL_P1nPFS_SDA10A   0x3
/** SAU_SPI10 SO10A */
#define RA_PSEL_P1nPFS_SO10A    0x3
/** IICA0 SDAA0C */
#define RA_PSEL_P1nPFS_SDAA0C   0x3
/** IICA0 SCLA0C */
#define RA_PSEL_P1nPFS_SCLA0C   0x3
/** SAU_SPI00 SSI00C */
#define RA_PSEL_P1nPFS_SSI00C   0x3
/** IICA0 SCLA0D */
#define RA_PSEL_P1nPFS_SCLA0D   0x4
/** IICA0 SDAA0D */
#define RA_PSEL_P1nPFS_SDAA0D   0x4
/** RTC RTCOUNTC */
#define RA_PSEL_P1nPFS_RTCOUNTC 0x4
/** UARTA1 RXDA1B */
#define RA_PSEL_P1nPFS_RXDA1B   0x4
/** UARTA1 TXDA1B */
#define RA_PSEL_P1nPFS_TXDA1B   0x4
/** UARTA0 TXDA0C */
#define RA_PSEL_P1nPFS_TXDA0C   0x4
/** UARTA0 RXDA0C */
#define RA_PSEL_P1nPFS_RXDA0C   0x4
/** UARTA0 RXDA0D */
#define RA_PSEL_P1nPFS_RXDA0D   0x5
/** UARTA0 TXDA0D */
#define RA_PSEL_P1nPFS_TXDA0D   0x5
/** CGC PCLBUZ0B */
#define RA_PSEL_P1nPFS_PCLBUZ0B 0x5
/** CGC PCLBUZ1B */
#define RA_PSEL_P1nPFS_PCLBUZ1B 0x5
/** IICA1 SCLA1G */
#define RA_PSEL_P1nPFS_SCLA1G   0x6
/** IICA1 SDAA1G */
#define RA_PSEL_P1nPFS_SDAA1G   0x6
/** IICA1 SCLA1B */
#define RA_PSEL_P1nPFS_SCLA1B   0x6
/** IICA1 SDAA1B */
#define RA_PSEL_P1nPFS_SDAA1B   0x6
/** UARTA1 RXDA1A */
#define RA_PSEL_P1nPFS_RXDA1A   0x7
/** UARTA1 TXDA1A */
#define RA_PSEL_P1nPFS_TXDA1A   0x7
/** CTSU TS11 */
#define RA_PSEL_P1nPFS_TS11     0xC
/** CTSU TS10 */
#define RA_PSEL_P1nPFS_TS10     0xC
/** CTSU TS9 */
#define RA_PSEL_P1nPFS_TS9      0xC
/** CTSU TS8 */
#define RA_PSEL_P1nPFS_TS8      0xC
/** CTSU TS7 */
#define RA_PSEL_P1nPFS_TS7      0xC
/** CTSU TS6 */
#define RA_PSEL_P1nPFS_TS6      0xC
/** CTSU TS5 */
#define RA_PSEL_P1nPFS_TS5      0xC
/** CTSU TS2 */
#define RA_PSEL_P1nPFS_TS2      0xC
/** CTSU TS3 */
#define RA_PSEL_P1nPFS_TS3      0xC
/** CTSU TS4 */
#define RA_PSEL_P1nPFS_TS4      0xC
/** CTSU TSCAP */
#define RA_PSEL_P1nPFS_TSCAP    0xC

/** TAU05 TI05B */
#define RA_PSEL_P2nPFS_TI05B    0x1
/** TAU05 TO05B */
#define RA_PSEL_P2nPFS_TO05B    0x1
/** TAU00 TO00B */
#define RA_PSEL_P2nPFS_TO00B    0x1
/** TAU00 TI00A */
#define RA_PSEL_P2nPFS_TI00B    0x1
/** TAU00 TO00A */
#define RA_PSEL_P2nPFS_TO00A    0x1
/** TAU00 TI00A */
#define RA_PSEL_P2nPFS_TI00A    0x1
/** SAU_SPI00 SSI00B */
#define RA_PSEL_P2nPFS_SSI00B   0x2
/** UARTA0 RXDA0A */
#define RA_PSEL_P2nPFS_RXDA0A   0x2
/** UARTA0 TXDA0A */
#define RA_PSEL_P2nPFS_TXDA0A   0x2
/** TAU03 TI03C */
#define RA_PSEL_P2nPFS_TI03C    0x2
/** TAU03 TO03C */
#define RA_PSEL_P2nPFS_TO03C    0x2
/** TAU02 TI02B */
#define RA_PSEL_P2nPFS_TI02B    0x2
/** TAU02 TO02B */
#define RA_PSEL_P2nPFS_TO02B    0x2
/** SAU_SPI11 SCK11B */
#define RA_PSEL_P2nPFS_SCK11B   0x3
/** SAU_I2C11 SCL11B */
#define RA_PSEL_P2nPFS_SCL11B   0x3
/** SAU_SPI01 SI01B */
#define RA_PSEL_P2nPFS_SI01B    0x3
/** SAU_I2C01 SDA01B */
#define RA_PSEL_P2nPFS_SDA01B   0x3
/** SAU_SPI01 SCK01B */
#define RA_PSEL_P2nPFS_SCK01B   0x3
/** SAU_I2C01 SCL01B */
#define RA_PSEL_P2nPFS_SCL01B   0x3
/** SAU_UART1 RXD1A */
#define RA_PSEL_P2nPFS_RXD1A    0x3
/** SAU_UART1 TXD1A */
#define RA_PSEL_P2nPFS_TXD1A    0x3
/** RTC RTCOUTB */
#define RA_PSEL_P2nPFS_RTCOUTB  0x4
/** IICA1 SCLA1A */
#define RA_PSEL_P2nPFS_SCLA1A   0x4
/** IICA1 SDAA1A */
#define RA_PSEL_P2nPFS_SDAA1A   0x4
/** SAU_SPI11 SI11A */
#define RA_PSEL_P2nPFS_SI11A    0x4
/** SAU_I2C11 SDA11A */
#define RA_PSEL_P2nPFS_SDA11A   0x4
/** SAU_SPI11 SO11A */
#define RA_PSEL_P2nPFS_SO11A    0x4
/** CGC PCLBUZ0A */
#define RA_PSEL_P2nPFS_PCLBUZ0A 0x5
/** IICA0 SCLA0B */
#define RA_PSEL_P2nPFS_SCLA0B   0x5
/** IICA0 SDAA0B */
#define RA_PSEL_P2nPFS_SDAA0B   0x5
/** UARTA0 RXDA0B */
#define RA_PSEL_P2nPFS_RXDA0B   0x6
/** UARTA0 TXDA0B */
#define RA_PSEL_P2nPFS_TXDA0B   0x6

/** JTAG/SWD SWCLK */
#define RA_PSEL_P3nPFS_SWCLK  0x1
/** TAU06 TI06B */
#define RA_PSEL_P3nPFS_TI06B  0x1
/** TAU06 TO06B */
#define RA_PSEL_P3nPFS_TO06B  0x1
/** TAU05 TI05C */
#define RA_PSEL_P3nPFS_TI05C  0x1
/** TAU05 TO05C */
#define RA_PSEL_P3nPFS_TO05C  0x1
/** TAU04 TI04B */
#define RA_PSEL_P3nPFS_TI04B  0x2
/** TAU04 TO04B */
#define RA_PSEL_P3nPFS_TO04B  0x2
/** SAU_I2C21 SDA21A */
#define RA_PSEL_P3nPFS_SDA21A 0x2
/** SAU_I2C21 SCL21A */
#define RA_PSEL_P3nPFS_SCL21A 0x2
/** IICA1 SCLA1C */
#define RA_PSEL_P3nPFS_SCLA1C 0x3
/** IICA1 SDAA1C */
#define RA_PSEL_P3nPFS_SDAA1C 0x3
/** UARTA1 RXDA1C */
#define RA_PSEL_P3nPFS_RXDA1C 0x4
/** UARTA1 TXDA1C */
#define RA_PSEL_P3nPFS_TXDA1C 0x4
/** CTSU TS1 */
#define RA_PSEL_P3nPFS_TS1    0xC
/** CTSU TS0 */
#define RA_PSEL_P3nPFS_TS0    0xC

/** IICA1 SCLA1D */
#define RA_PSEL_P4nPFS_SCLA1D   0x1
/** IICA1 SDAA1D */
#define RA_PSEL_P4nPFS_SDAA1D   0x1
/** SAU_SPI11 SCK11A */
#define RA_PSEL_P4nPFS_SCK11A   0x1
/** SAU_I2C11 SCL11A */
#define RA_PSEL_P4nPFS_SCL11A   0x1
/** SAU_SPI11 SCK11C */
#define RA_PSEL_P4nPFS_SCK11C   0x1
/** SAU_I2C11 SCL11C */
#define RA_PSEL_P4nPFS_SCL11C   0x1
/** RTC RTCOUTA */
#define RA_PSEL_P4nPFS_RTCOUTA  0x2
/** TAU04 TI04C */
#define RA_PSEL_P4nPFS_TI04C    0x2
/** TAU04 TO04C */
#define RA_PSEL_P4nPFS_TO04C    0x2
/** TAU03 TI03E */
#define RA_PSEL_P4nPFS_TI03E    0x2
/** TAU03 TO03E */
#define RA_PSEL_P4nPFS_TO03E    0x2
/** CGC PCLBUZ0C */
#define RA_PSEL_P4nPFS_PCLBUZ0C 0x3
/** IICA1 SDAA1F */
#define RA_PSEL_P4nPFS_SDAA1F   0x4
/** IICA1 SCLA1F */
#define RA_PSEL_P4nPFS_SCLA1F   0x4

/** TAU03 TI03D */
#define RA_PSEL_P5nPFS_TI03D 0x1
/** TAU03 TO03D */
#define RA_PSEL_P5nPFS_TO03D 0x1
/** CTSU TS12 */
#define RA_PSEL_P5nPFS_TS12  0xC

/** IICA0 SDAA0A */
#define RA_PSEL_P9nPFS_SDAA0A 0x1
/** IICA0 SCLA0A */
#define RA_PSEL_P9nPFS_SCLA0A 0x1
/** SAU_SPI01 SO01B */
#define RA_PSEL_P9nPFS_SO01B  0x3

/** @} */

/**
 * @brief Utility macro to build Renesas RA0 psels property entry.
 *
 * @param psel Pin function configuration (see RA_PSEL_{name} macros).
 * @param port_num Port (0 or 15).
 * @param pin_num Pin (0..31).
 */
#define RA_PSEL(psel, port_num, pin_num)                                                           \
	(psel << RA_PSEL_POS | port_num << RA_PORT_NUM_POS | pin_num << RA_PIN_NUM_POS)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RA0_H__ */
