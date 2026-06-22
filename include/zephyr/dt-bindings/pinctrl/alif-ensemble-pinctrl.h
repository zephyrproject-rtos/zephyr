/*
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ALIF_ENSEMBLE_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ALIF_ENSEMBLE_PINCTRL_H_

/**
 * @file
 * @brief Pinctrl definitions for Alif Ensemble SoC family
 *
 * This header provides pin multiplexing definitions for the Alif Ensemble family.
 * Each pin can be configured for up to 8 different functions (GPIO + 7 alternate functions).
 *
 * The pinmux encoding uses an 11-bit value:
 * - Bits [0:2]   - Alternate function (0 = GPIO, 1-7 = AF1-AF7)
 * - Bits [3:5]   - Pin number within port (0-7)
 * - Bits [6:10]  - Port number (0-17)
 *
 * Pin naming convention: PIN_P{port}_{pin}__{function}
 * Example: PIN_P0_0__UART0_RX_A means Port 0, Pin 0, UART0 RX function (A group)
 */

/**
 * @name Pinmux encoding bit field definitions
 * @{
 */

/** Bit position for alternate function field */
#define ALIF_PINMUX_FUNC_POS    0U
/** Bit mask for alternate function field (3 bits) */
#define ALIF_PINMUX_FUNC_MASK   0x7U
/** Bit position for pin number field */
#define ALIF_PINMUX_PIN_POS     3U
/** Bit mask for pin number field (3 bits) */
#define ALIF_PINMUX_PIN_MASK    0x7U
/** Bit position for port number field */
#define ALIF_PINMUX_PORT_POS    6U
/** Bit mask for port number field (5 bits) */
#define ALIF_PINMUX_PORT_MASK   0x1FU

/** @} */

/**
 * @brief Encode pinmux configuration
 *
 * @param port Port number (0-17)
 * @param pin Pin number within port (0-7)
 * @param func Alternate function (0 = GPIO, 1-7 = AF1-AF7)
 * @return Encoded pinmux value for use in devicetree
 */
#define ALIF_PINMUX(port, pin, func)					\
	((((port) & ALIF_PINMUX_PORT_MASK) << ALIF_PINMUX_PORT_POS) |	\
	(((pin) & ALIF_PINMUX_PIN_MASK) << ALIF_PINMUX_PIN_POS) |	\
	(((func) & ALIF_PINMUX_FUNC_MASK) << ALIF_PINMUX_FUNC_POS))

/**
 * @name Port 0 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 0.
 * @{
 */

/** Pin P0.0 alternate functions */
#define PIN_P0_0__GPIO                  ALIF_PINMUX(0, 0, 0)  /**< Port 0 Pin 0: GPIO */
#define PIN_P0_0__OSPI0_D0_A            ALIF_PINMUX(0, 0, 1)  /**< Port 0 Pin 0: OSPI0_D0_A */
#define PIN_P0_0__UART0_RX_A            ALIF_PINMUX(0, 0, 2)  /**< Port 0 Pin 0: UART0_RX_A */
#define PIN_P0_0__I3C_SDA_A             ALIF_PINMUX(0, 0, 3)  /**< Port 0 Pin 0: I3C_SDA_A */
#define PIN_P0_0__UT0_T0_A              ALIF_PINMUX(0, 0, 4)  /**< Port 0 Pin 0: UT0_T0_A */
#define PIN_P0_0__LPCAM_HSYNC_B         ALIF_PINMUX(0, 0, 5)  /**< Port 0 Pin 0: LPCAM_HSYNC_B */
#define PIN_P0_0__CAM_HSYNC_A           ALIF_PINMUX(0, 0, 6)  /**< Port 0 Pin 0: CAM_HSYNC_A */
#define PIN_P0_0__ANA_S0                ALIF_PINMUX(0, 0, 7)  /**< Port 0 Pin 0: ANA_S0 */

/** Pin P0.1 alternate functions */
#define PIN_P0_1__GPIO                  ALIF_PINMUX(0, 1, 0)  /**< Port 0 Pin 1: GPIO */
#define PIN_P0_1__OSPI0_D1_A            ALIF_PINMUX(0, 1, 1)  /**< Port 0 Pin 1: OSPI0_D1_A */
#define PIN_P0_1__UART0_TX_A            ALIF_PINMUX(0, 1, 2)  /**< Port 0 Pin 1: UART0_TX_A */
#define PIN_P0_1__I3C_SCL_A             ALIF_PINMUX(0, 1, 3)  /**< Port 0 Pin 1: I3C_SCL_A */
#define PIN_P0_1__UT0_T1_A              ALIF_PINMUX(0, 1, 4)  /**< Port 0 Pin 1: UT0_T1_A */
#define PIN_P0_1__LPCAM_VSYNC_B         ALIF_PINMUX(0, 1, 5)  /**< Port 0 Pin 1: LPCAM_VSYNC_B */
#define PIN_P0_1__CAM_VSYNC_A           ALIF_PINMUX(0, 1, 6)  /**< Port 0 Pin 1: CAM_VSYNC_A */
#define PIN_P0_1__ANA_S1                ALIF_PINMUX(0, 1, 7)  /**< Port 0 Pin 1: ANA_S1 */

/** Pin P0.2 alternate functions */
#define PIN_P0_2__GPIO                  ALIF_PINMUX(0, 2, 0)  /**< Port 0 Pin 2: GPIO */
#define PIN_P0_2__OSPI0_D2_A            ALIF_PINMUX(0, 2, 1)  /**< Port 0 Pin 2: OSPI0_D2_A */
#define PIN_P0_2__UART0_CTS_A           ALIF_PINMUX(0, 2, 2)  /**< Port 0 Pin 2: UART0_CTS_A */
#define PIN_P0_2__I2C0_SDA_A            ALIF_PINMUX(0, 2, 3)  /**< Port 0 Pin 2: I2C0_SDA_A */
#define PIN_P0_2__UT1_T0_A              ALIF_PINMUX(0, 2, 4)  /**< Port 0 Pin 2: UT1_T0_A */
#define PIN_P0_2__LPCAM_PCLK_B          ALIF_PINMUX(0, 2, 5)  /**< Port 0 Pin 2: LPCAM_PCLK_B */
#define PIN_P0_2__CAM_PCLK_A            ALIF_PINMUX(0, 2, 6)  /**< Port 0 Pin 2: CAM_PCLK_A */
#define PIN_P0_2__ANA_S2                ALIF_PINMUX(0, 2, 7)  /**< Port 0 Pin 2: ANA_S2 */

/** Pin P0.3 alternate functions */
#define PIN_P0_3__GPIO                  ALIF_PINMUX(0, 3, 0)  /**< Port 0 Pin 3: GPIO */
#define PIN_P0_3__OSPI0_D3_A            ALIF_PINMUX(0, 3, 1)  /**< Port 0 Pin 3: OSPI0_D3_A */
#define PIN_P0_3__UART0_RTS_A           ALIF_PINMUX(0, 3, 2)  /**< Port 0 Pin 3: UART0_RTS_A */
#define PIN_P0_3__I2C0_SCL_A            ALIF_PINMUX(0, 3, 3)  /**< Port 0 Pin 3: I2C0_SCL_A */
#define PIN_P0_3__UT1_T1_A              ALIF_PINMUX(0, 3, 4)  /**< Port 0 Pin 3: UT1_T1_A */
#define PIN_P0_3__LPCAM_XVCLK_B         ALIF_PINMUX(0, 3, 5)  /**< Port 0 Pin 3: LPCAM_XVCLK_B */
#define PIN_P0_3__CAM_XVCLK_A           ALIF_PINMUX(0, 3, 6)  /**< Port 0 Pin 3: CAM_XVCLK_A */
#define PIN_P0_3__ANA_S3                ALIF_PINMUX(0, 3, 7)  /**< Port 0 Pin 3: ANA_S3 */

/** Pin P0.4 alternate functions */
#define PIN_P0_4__GPIO                  ALIF_PINMUX(0, 4, 0)  /**< Port 0 Pin 4: GPIO */
#define PIN_P0_4__OSPI0_D4_A            ALIF_PINMUX(0, 4, 1)  /**< Port 0 Pin 4: OSPI0_D4_A */
#define PIN_P0_4__UART1_RX_A            ALIF_PINMUX(0, 4, 2)  /**< Port 0 Pin 4: UART1_RX_A */
#define PIN_P0_4__PDM_D0_A              ALIF_PINMUX(0, 4, 3)  /**< Port 0 Pin 4: PDM_D0_A */
#define PIN_P0_4__I2C1_SDA_A            ALIF_PINMUX(0, 4, 4)  /**< Port 0 Pin 4: I2C1_SDA_A */
#define PIN_P0_4__UT2_T0_A              ALIF_PINMUX(0, 4, 5)  /**< Port 0 Pin 4: UT2_T0_A */
#define PIN_P0_4__CAN_RXD_B             ALIF_PINMUX(0, 4, 6)  /**< Port 0 Pin 4: CAN_RXD_B */
#define PIN_P0_4__ANA_S4                ALIF_PINMUX(0, 4, 7)  /**< Port 0 Pin 4: ANA_S4 */

/** Pin P0.5 alternate functions */
#define PIN_P0_5__GPIO                  ALIF_PINMUX(0, 5, 0)  /**< Port 0 Pin 5: GPIO */
#define PIN_P0_5__OSPI0_D5_A            ALIF_PINMUX(0, 5, 1)  /**< Port 0 Pin 5: OSPI0_D5_A */
#define PIN_P0_5__UART1_TX_A            ALIF_PINMUX(0, 5, 2)  /**< Port 0 Pin 5: UART1_TX_A */
#define PIN_P0_5__PDM_C0_A              ALIF_PINMUX(0, 5, 3)  /**< Port 0 Pin 5: PDM_C0_A */
#define PIN_P0_5__I2C1_SCL_A            ALIF_PINMUX(0, 5, 4)  /**< Port 0 Pin 5: I2C1_SCL_A */
#define PIN_P0_5__UT2_T1_A              ALIF_PINMUX(0, 5, 5)  /**< Port 0 Pin 5: UT2_T1_A */
#define PIN_P0_5__CAN_TXD_B             ALIF_PINMUX(0, 5, 6)  /**< Port 0 Pin 5: CAN_TXD_B */
#define PIN_P0_5__ANA_S5                ALIF_PINMUX(0, 5, 7)  /**< Port 0 Pin 5: ANA_S5 */

/** Pin P0.6 alternate functions */
#define PIN_P0_6__GPIO                  ALIF_PINMUX(0, 6, 0)  /**< Port 0 Pin 6: GPIO */
#define PIN_P0_6__OSPI0_D6_A            ALIF_PINMUX(0, 6, 1)  /**< Port 0 Pin 6: OSPI0_D6_A */
#define PIN_P0_6__UART1_CTS_A           ALIF_PINMUX(0, 6, 2)  /**< Port 0 Pin 6: UART1_CTS_A */
#define PIN_P0_6__PDM_D1_A              ALIF_PINMUX(0, 6, 3)  /**< Port 0 Pin 6: PDM_D1_A */
#define PIN_P0_6__I2C2_SCL_A            ALIF_PINMUX(0, 6, 4)  /**< Port 0 Pin 6: I2C2_SCL_A */
#define PIN_P0_6__UT3_T0_A              ALIF_PINMUX(0, 6, 5)  /**< Port 0 Pin 6: UT3_T0_A */
#define PIN_P0_6__CAN_STBY_B            ALIF_PINMUX(0, 6, 6)  /**< Port 0 Pin 6: CAN_STBY_B */
#define PIN_P0_6__ANA_S6                ALIF_PINMUX(0, 6, 7)  /**< Port 0 Pin 6: ANA_S6 */

/** Pin P0.7 alternate functions */
#define PIN_P0_7__GPIO                  ALIF_PINMUX(0, 7, 0)  /**< Port 0 Pin 7: GPIO */
#define PIN_P0_7__OSPI0_D7_A            ALIF_PINMUX(0, 7, 1)  /**< Port 0 Pin 7: OSPI0_D7_A */
#define PIN_P0_7__UART1_RTS_A           ALIF_PINMUX(0, 7, 2)  /**< Port 0 Pin 7: UART1_RTS_A */
#define PIN_P0_7__PDM_C1_A              ALIF_PINMUX(0, 7, 3)  /**< Port 0 Pin 7: PDM_C1_A */
#define PIN_P0_7__I2C2_SDA_A            ALIF_PINMUX(0, 7, 4)  /**< Port 0 Pin 7: I2C2_SDA_A */
#define PIN_P0_7__UT3_T1_A              ALIF_PINMUX(0, 7, 5)  /**< Port 0 Pin 7: UT3_T1_A */
#define PIN_P0_7__CDC_DE_B              ALIF_PINMUX(0, 7, 6)  /**< Port 0 Pin 7: CDC_DE_B */
#define PIN_P0_7__ANA_S7                ALIF_PINMUX(0, 7, 7)  /**< Port 0 Pin 7: ANA_S7 */

/** @} */

/**
 * @name Port 1 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 1.
 * @{
 */

/** Pin P1.0 alternate functions */
#define PIN_P1_0__GPIO                  ALIF_PINMUX(1, 0, 0)  /**< Port 1 Pin 0: GPIO */
#define PIN_P1_0__UART2_RX_A            ALIF_PINMUX(1, 0, 1)  /**< Port 1 Pin 0: UART2_RX_A */
#define PIN_P1_0__SPI0_MISO_A           ALIF_PINMUX(1, 0, 2)  /**< Port 1 Pin 0: SPI0_MISO_A */
#define PIN_P1_0__I2C3_SDA_A            ALIF_PINMUX(1, 0, 3)  /**< Port 1 Pin 0: I2C3_SDA_A */
#define PIN_P1_0__UT4_T0_A              ALIF_PINMUX(1, 0, 4)  /**< Port 1 Pin 0: UT4_T0_A */
#define PIN_P1_0__LPCAM_HSYNC_C         ALIF_PINMUX(1, 0, 5)  /**< Port 1 Pin 0: LPCAM_HSYNC_C */
#define PIN_P1_0__ETH_RXD0_C            ALIF_PINMUX(1, 0, 6)  /**< Port 1 Pin 0: ETH_RXD0_C */
#define PIN_P1_0__ANA_S8                ALIF_PINMUX(1, 0, 7)  /**< Port 1 Pin 0: ANA_S8 */

/** Pin P1.1 alternate functions */
#define PIN_P1_1__GPIO                  ALIF_PINMUX(1, 1, 0)  /**< Port 1 Pin 1: GPIO */
#define PIN_P1_1__UART2_TX_A            ALIF_PINMUX(1, 1, 1)  /**< Port 1 Pin 1: UART2_TX_A */
#define PIN_P1_1__SPI0_MOSI_A           ALIF_PINMUX(1, 1, 2)  /**< Port 1 Pin 1: SPI0_MOSI_A */
#define PIN_P1_1__I2C3_SCL_A            ALIF_PINMUX(1, 1, 3)  /**< Port 1 Pin 1: I2C3_SCL_A */
#define PIN_P1_1__UT4_T1_A              ALIF_PINMUX(1, 1, 4)  /**< Port 1 Pin 1: UT4_T1_A */
#define PIN_P1_1__LPCAM_VSYNC_C         ALIF_PINMUX(1, 1, 5)  /**< Port 1 Pin 1: LPCAM_VSYNC_C */
#define PIN_P1_1__ETH_RXD1_C            ALIF_PINMUX(1, 1, 6)  /**< Port 1 Pin 1: ETH_RXD1_C */
#define PIN_P1_1__ANA_S9                ALIF_PINMUX(1, 1, 7)  /**< Port 1 Pin 1: ANA_S9 */

/** Pin P1.2 alternate functions */
#define PIN_P1_2__GPIO                  ALIF_PINMUX(1, 2, 0)  /**< Port 1 Pin 2: GPIO */
#define PIN_P1_2__UART3_RX_A            ALIF_PINMUX(1, 2, 1)  /**< Port 1 Pin 2: UART3_RX_A */
#define PIN_P1_2__SPI0_SCLK_A           ALIF_PINMUX(1, 2, 2)  /**< Port 1 Pin 2: SPI0_SCLK_A */
#define PIN_P1_2__I3C_SDA_B             ALIF_PINMUX(1, 2, 3)  /**< Port 1 Pin 2: I3C_SDA_B */
#define PIN_P1_2__UT5_T0_A              ALIF_PINMUX(1, 2, 4)  /**< Port 1 Pin 2: UT5_T0_A */
#define PIN_P1_2__LPCAM_PCLK_C          ALIF_PINMUX(1, 2, 5)  /**< Port 1 Pin 2: LPCAM_PCLK_C */
#define PIN_P1_2__ETH_RST_C             ALIF_PINMUX(1, 2, 6)  /**< Port 1 Pin 2: ETH_RST_C */
#define PIN_P1_2__ANA_S10               ALIF_PINMUX(1, 2, 7)  /**< Port 1 Pin 2: ANA_S10 */

/** Pin P1.3 alternate functions */
#define PIN_P1_3__GPIO                  ALIF_PINMUX(1, 3, 0)  /**< Port 1 Pin 3: GPIO */
#define PIN_P1_3__UART3_TX_A            ALIF_PINMUX(1, 3, 1)  /**< Port 1 Pin 3: UART3_TX_A */
#define PIN_P1_3__SPI0_SS0_A            ALIF_PINMUX(1, 3, 2)  /**< Port 1 Pin 3: SPI0_SS0_A */
#define PIN_P1_3__I3C_SCL_B             ALIF_PINMUX(1, 3, 3)  /**< Port 1 Pin 3: I3C_SCL_B */
#define PIN_P1_3__UT5_T1_A              ALIF_PINMUX(1, 3, 4)  /**< Port 1 Pin 3: UT5_T1_A */
#define PIN_P1_3__LPCAM_XVCLK_C         ALIF_PINMUX(1, 3, 5)  /**< Port 1 Pin 3: LPCAM_XVCLK_C */
#define PIN_P1_3__ETH_TXD0_C            ALIF_PINMUX(1, 3, 6)  /**< Port 1 Pin 3: ETH_TXD0_C */
#define PIN_P1_3__ANA_S11               ALIF_PINMUX(1, 3, 7)  /**< Port 1 Pin 3: ANA_S11 */

/** Pin P1.4 alternate functions */
#define PIN_P1_4__GPIO                  ALIF_PINMUX(1, 4, 0)  /**< Port 1 Pin 4: GPIO */
#define PIN_P1_4__OSPI0_SS0_A           ALIF_PINMUX(1, 4, 1)  /**< Port 1 Pin 4: OSPI0_SS0_A */
#define PIN_P1_4__UART0_RX_B            ALIF_PINMUX(1, 4, 2)  /**< Port 1 Pin 4: UART0_RX_B */
#define PIN_P1_4__SPI0_SS1_A            ALIF_PINMUX(1, 4, 3)  /**< Port 1 Pin 4: SPI0_SS1_A */
#define PIN_P1_4__UT6_T0_A              ALIF_PINMUX(1, 4, 4)  /**< Port 1 Pin 4: UT6_T0_A */
#define PIN_P1_4__LPCAM_D0_C            ALIF_PINMUX(1, 4, 5)  /**< Port 1 Pin 4: LPCAM_D0_C */
#define PIN_P1_4__ETH_TXD1_C            ALIF_PINMUX(1, 4, 6)  /**< Port 1 Pin 4: ETH_TXD1_C */
#define PIN_P1_4__ANA_S12               ALIF_PINMUX(1, 4, 7)  /**< Port 1 Pin 4: ANA_S12 */

/** Pin P1.5 alternate functions */
#define PIN_P1_5__GPIO                  ALIF_PINMUX(1, 5, 0)  /**< Port 1 Pin 5: GPIO */
#define PIN_P1_5__OSPI0_SS1_A           ALIF_PINMUX(1, 5, 1)  /**< Port 1 Pin 5: OSPI0_SS1_A */
#define PIN_P1_5__UART0_TX_B            ALIF_PINMUX(1, 5, 2)  /**< Port 1 Pin 5: UART0_TX_B */
#define PIN_P1_5__SPI0_SS2_A            ALIF_PINMUX(1, 5, 3)  /**< Port 1 Pin 5: SPI0_SS2_A */
#define PIN_P1_5__UT6_T1_A              ALIF_PINMUX(1, 5, 4)  /**< Port 1 Pin 5: UT6_T1_A */
#define PIN_P1_5__LPCAM_D1_C            ALIF_PINMUX(1, 5, 5)  /**< Port 1 Pin 5: LPCAM_D1_C */
#define PIN_P1_5__ETH_TXEN_C            ALIF_PINMUX(1, 5, 6)  /**< Port 1 Pin 5: ETH_TXEN_C */
#define PIN_P1_5__ANA_S13               ALIF_PINMUX(1, 5, 7)  /**< Port 1 Pin 5: ANA_S13 */

/** Pin P1.6 alternate functions */
#define PIN_P1_6__GPIO                  ALIF_PINMUX(1, 6, 0)  /**< Port 1 Pin 6: GPIO */
#define PIN_P1_6__OSPI0_RXDS_B          ALIF_PINMUX(1, 6, 1)  /**< Port 1 Pin 6: OSPI0_RXDS_B */
#define PIN_P1_6__UART1_RX_B            ALIF_PINMUX(1, 6, 2)  /**< Port 1 Pin 6: UART1_RX_B */
#define PIN_P1_6__I2S0_SDI_A            ALIF_PINMUX(1, 6, 3)  /**< Port 1 Pin 6: I2S0_SDI_A */
#define PIN_P1_6__UT7_T0_A              ALIF_PINMUX(1, 6, 4)  /**< Port 1 Pin 6: UT7_T0_A */
#define PIN_P1_6__LPCAM_D2_C            ALIF_PINMUX(1, 6, 5)  /**< Port 1 Pin 6: LPCAM_D2_C */
#define PIN_P1_6__ETH_IRQ_C             ALIF_PINMUX(1, 6, 6)  /**< Port 1 Pin 6: ETH_IRQ_C */
#define PIN_P1_6__ANA_S14               ALIF_PINMUX(1, 6, 7)  /**< Port 1 Pin 6: ANA_S14 */

/** Pin P1.7 alternate functions */
#define PIN_P1_7__GPIO                  ALIF_PINMUX(1, 7, 0)  /**< Port 1 Pin 7: GPIO */
#define PIN_P1_7__OSPI0_SCLK_A          ALIF_PINMUX(1, 7, 1)  /**< Port 1 Pin 7: OSPI0_SCLK_A */
#define PIN_P1_7__UART1_TX_B            ALIF_PINMUX(1, 7, 2)  /**< Port 1 Pin 7: UART1_TX_B */
#define PIN_P1_7__I2S0_SDO_A            ALIF_PINMUX(1, 7, 3)  /**< Port 1 Pin 7: I2S0_SDO_A */
#define PIN_P1_7__UT7_T1_A              ALIF_PINMUX(1, 7, 4)  /**< Port 1 Pin 7: UT7_T1_A */
#define PIN_P1_7__LPCAM_D3_C            ALIF_PINMUX(1, 7, 5)  /**< Port 1 Pin 7: LPCAM_D3_C */
#define PIN_P1_7__ETH_REFCLK_C          ALIF_PINMUX(1, 7, 6)  /**< Port 1 Pin 7: ETH_REFCLK_C */
#define PIN_P1_7__ANA_S15               ALIF_PINMUX(1, 7, 7)  /**< Port 1 Pin 7: ANA_S15 */

/** @} */

/**
 * @name Port 2 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 2.
 * @{
 */

/** Pin P2.0 alternate functions */
#define PIN_P2_0__GPIO                  ALIF_PINMUX(2, 0, 0)  /**< Port 2 Pin 0: GPIO */
#define PIN_P2_0__OSPI0_D0_B            ALIF_PINMUX(2, 0, 1)  /**< Port 2 Pin 0: OSPI0_D0_B */
#define PIN_P2_0__UART2_RX_B            ALIF_PINMUX(2, 0, 2)  /**< Port 2 Pin 0: UART2_RX_B */
#define PIN_P2_0__LPPDM_DO_A            ALIF_PINMUX(2, 0, 3)  /**< Port 2 Pin 0: LPPDM_DO_A */
#define PIN_P2_0__UT8_T0_A              ALIF_PINMUX(2, 0, 4)  /**< Port 2 Pin 0: UT8_T0_A */
#define PIN_P2_0__LPCAM_D4_C            ALIF_PINMUX(2, 0, 5)  /**< Port 2 Pin 0: LPCAM_D4_C */
#define PIN_P2_0__ETH_MDIO_C            ALIF_PINMUX(2, 0, 6)  /**< Port 2 Pin 0: ETH_MDIO_C */
#define PIN_P2_0__ANA_S16               ALIF_PINMUX(2, 0, 7)  /**< Port 2 Pin 0: ANA_S16 */

/** Pin P2.1 alternate functions */
#define PIN_P2_1__GPIO                  ALIF_PINMUX(2, 1, 0)  /**< Port 2 Pin 1: GPIO */
#define PIN_P2_1__OSPI0_D1_B            ALIF_PINMUX(2, 1, 1)  /**< Port 2 Pin 1: OSPI0_D1_B */
#define PIN_P2_1__UART2_TX_B            ALIF_PINMUX(2, 1, 2)  /**< Port 2 Pin 1: UART2_TX_B */
#define PIN_P2_1__LPPDM_CO_A            ALIF_PINMUX(2, 1, 3)  /**< Port 2 Pin 1: LPPDM_CO_A */
#define PIN_P2_1__UT8_T1_A              ALIF_PINMUX(2, 1, 4)  /**< Port 2 Pin 1: UT8_T1_A */
#define PIN_P2_1__LPCAM_D5_C            ALIF_PINMUX(2, 1, 5)  /**< Port 2 Pin 1: LPCAM_D5_C */
#define PIN_P2_1__ETH_MDC_C             ALIF_PINMUX(2, 1, 6)  /**< Port 2 Pin 1: ETH_MDC_C */
#define PIN_P2_1__ANA_S17               ALIF_PINMUX(2, 1, 7)  /**< Port 2 Pin 1: ANA_S17 */

/** Pin P2.2 alternate functions */
#define PIN_P2_2__GPIO                  ALIF_PINMUX(2, 2, 0)  /**< Port 2 Pin 2: GPIO */
#define PIN_P2_2__OSPI0_D2_B            ALIF_PINMUX(2, 2, 1)  /**< Port 2 Pin 2: OSPI0_D2_B */
#define PIN_P2_2__UART3_RX_B            ALIF_PINMUX(2, 2, 2)  /**< Port 2 Pin 2: UART3_RX_B */
#define PIN_P2_2__LPPDM_D1_A            ALIF_PINMUX(2, 2, 3)  /**< Port 2 Pin 2: LPPDM_D1_A */
#define PIN_P2_2__UT9_T0_A              ALIF_PINMUX(2, 2, 4)  /**< Port 2 Pin 2: UT9_T0_A */
#define PIN_P2_2__LPCAM_D6_C            ALIF_PINMUX(2, 2, 5)  /**< Port 2 Pin 2: LPCAM_D6_C */
#define PIN_P2_2__ETH_CRS_DV_C          ALIF_PINMUX(2, 2, 6)  /**< Port 2 Pin 2: ETH_CRS_DV_C */
#define PIN_P2_2__ANA_S18               ALIF_PINMUX(2, 2, 7)  /**< Port 2 Pin 2: ANA_S18 */

/** Pin P2.3 alternate functions */
#define PIN_P2_3__GPIO                  ALIF_PINMUX(2, 3, 0)  /**< Port 2 Pin 3: GPIO */
#define PIN_P2_3__OSPI0_D3_B            ALIF_PINMUX(2, 3, 1)  /**< Port 2 Pin 3: OSPI0_D3_B */
#define PIN_P2_3__UART3_TX_B            ALIF_PINMUX(2, 3, 2)  /**< Port 2 Pin 3: UART3_TX_B */
#define PIN_P2_3__LPPDM_C1_A            ALIF_PINMUX(2, 3, 3)  /**< Port 2 Pin 3: LPPDM_C1_A */
#define PIN_P2_3__UT9_T1_A              ALIF_PINMUX(2, 3, 4)  /**< Port 2 Pin 3: UT9_T1_A */
#define PIN_P2_3__LPCAM_D7_C            ALIF_PINMUX(2, 3, 5)  /**< Port 2 Pin 3: LPCAM_D7_C */
#define PIN_P2_3__CDC_PCLK_B            ALIF_PINMUX(2, 3, 6)  /**< Port 2 Pin 3: CDC_PCLK_B */
#define PIN_P2_3__ANA_S19               ALIF_PINMUX(2, 3, 7)  /**< Port 2 Pin 3: ANA_S19 */

/** Pin P2.4 alternate functions */
#define PIN_P2_4__GPIO                  ALIF_PINMUX(2, 4, 0)  /**< Port 2 Pin 4: GPIO */
#define PIN_P2_4__OSPI0_D4_B            ALIF_PINMUX(2, 4, 1)  /**< Port 2 Pin 4: OSPI0_D4_B */
#define PIN_P2_4__LPI2S_SDI_A           ALIF_PINMUX(2, 4, 2)  /**< Port 2 Pin 4: LPI2S_SDI_A */
#define PIN_P2_4__SPI1_MISO_A           ALIF_PINMUX(2, 4, 3)  /**< Port 2 Pin 4: SPI1_MISO_A */
#define PIN_P2_4__UT10_T0_A             ALIF_PINMUX(2, 4, 4)  /**< Port 2 Pin 4: UT10_T0_A */
#define PIN_P2_4__LPCAM_D0_B            ALIF_PINMUX(2, 4, 5)  /**< Port 2 Pin 4: LPCAM_D0_B */
#define PIN_P2_4__CAM_D0_A              ALIF_PINMUX(2, 4, 6)  /**< Port 2 Pin 4: CAM_D0_A */
#define PIN_P2_4__ANA_S20               ALIF_PINMUX(2, 4, 7)  /**< Port 2 Pin 4: ANA_S20 */

/** Pin P2.5 alternate functions */
#define PIN_P2_5__GPIO                  ALIF_PINMUX(2, 5, 0)  /**< Port 2 Pin 5: GPIO */
#define PIN_P2_5__OSPI0_D5_B            ALIF_PINMUX(2, 5, 1)  /**< Port 2 Pin 5: OSPI0_D5_B */
#define PIN_P2_5__LPI2S_SDO_A           ALIF_PINMUX(2, 5, 2)  /**< Port 2 Pin 5: LPI2S_SDO_A */
#define PIN_P2_5__SPI1_MOSI_A           ALIF_PINMUX(2, 5, 3)  /**< Port 2 Pin 5: SPI1_MOSI_A */
#define PIN_P2_5__UT10_T1_A             ALIF_PINMUX(2, 5, 4)  /**< Port 2 Pin 5: UT10_T1_A */
#define PIN_P2_5__LPCAM_D1_B            ALIF_PINMUX(2, 5, 5)  /**< Port 2 Pin 5: LPCAM_D1_B */
#define PIN_P2_5__CAM_D1_A              ALIF_PINMUX(2, 5, 6)  /**< Port 2 Pin 5: CAM_D1_A */
#define PIN_P2_5__ANA_S21               ALIF_PINMUX(2, 5, 7)  /**< Port 2 Pin 5: ANA_S21 */

/** Pin P2.6 alternate functions */
#define PIN_P2_6__GPIO                  ALIF_PINMUX(2, 6, 0)  /**< Port 2 Pin 6: GPIO */
#define PIN_P2_6__OSPI0_D6_B            ALIF_PINMUX(2, 6, 1)  /**< Port 2 Pin 6: OSPI0_D6_B */
#define PIN_P2_6__LPI2S_SCLK_A          ALIF_PINMUX(2, 6, 2)  /**< Port 2 Pin 6: LPI2S_SCLK_A */
#define PIN_P2_6__SPI1_SCLK_A           ALIF_PINMUX(2, 6, 3)  /**< Port 2 Pin 6: SPI1_SCLK_A */
#define PIN_P2_6__UT11_T0_A             ALIF_PINMUX(2, 6, 4)  /**< Port 2 Pin 6: UT11_T0_A */
#define PIN_P2_6__LPCAM_D2_B            ALIF_PINMUX(2, 6, 5)  /**< Port 2 Pin 6: LPCAM_D2_B */
#define PIN_P2_6__CAM_D2_A              ALIF_PINMUX(2, 6, 6)  /**< Port 2 Pin 6: CAM_D2_A */
#define PIN_P2_6__ANA_S22               ALIF_PINMUX(2, 6, 7)  /**< Port 2 Pin 6: ANA_S22 */

/** Pin P2.7 alternate functions */
#define PIN_P2_7__GPIO                  ALIF_PINMUX(2, 7, 0)  /**< Port 2 Pin 7: GPIO */
#define PIN_P2_7__OSPI0_D7_B            ALIF_PINMUX(2, 7, 1)  /**< Port 2 Pin 7: OSPI0_D7_B */
#define PIN_P2_7__LPI2S_WS_A            ALIF_PINMUX(2, 7, 2)  /**< Port 2 Pin 7: LPI2S_WS_A */
#define PIN_P2_7__SPI1_SS0_A            ALIF_PINMUX(2, 7, 3)  /**< Port 2 Pin 7: SPI1_SS0_A */
#define PIN_P2_7__UT11_T1_A             ALIF_PINMUX(2, 7, 4)  /**< Port 2 Pin 7: UT11_T1_A */
#define PIN_P2_7__LPCAM_D3_B            ALIF_PINMUX(2, 7, 5)  /**< Port 2 Pin 7: LPCAM_D3_B */
#define PIN_P2_7__CAM_D3_A              ALIF_PINMUX(2, 7, 6)  /**< Port 2 Pin 7: CAM_D3_A */
#define PIN_P2_7__ANA_S23               ALIF_PINMUX(2, 7, 7)  /**< Port 2 Pin 7: ANA_S23 */

/** @} */

/**
 * @name Port 3 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 3.
 * @{
 */

/** Pin P3.0 alternate functions */
#define PIN_P3_0__GPIO                  ALIF_PINMUX(3, 0, 0)  /**< Port 3 Pin 0: GPIO */
#define PIN_P3_0__OSPI0_SCLK_B          ALIF_PINMUX(3, 0, 1)  /**< Port 3 Pin 0: OSPI0_SCLK_B */
#define PIN_P3_0__UART4_RX_A            ALIF_PINMUX(3, 0, 2)  /**< Port 3 Pin 0: UART4_RX_A */
#define PIN_P3_0__PDM_D0_B              ALIF_PINMUX(3, 0, 3)  /**< Port 3 Pin 0: PDM_D0_B */
#define PIN_P3_0__I2S0_SCLK_A           ALIF_PINMUX(3, 0, 4)  /**< Port 3 Pin 0: I2S0_SCLK_A */
#define PIN_P3_0__QEC0_X_A              ALIF_PINMUX(3, 0, 5)  /**< Port 3 Pin 0: QEC0_X_A */
#define PIN_P3_0__LPCAM_D4_B            ALIF_PINMUX(3, 0, 6)  /**< Port 3 Pin 0: LPCAM_D4_B */
#define PIN_P3_0__CAM_D4_A              ALIF_PINMUX(3, 0, 7)  /**< Port 3 Pin 0: CAM_D4_A */

/** Pin P3.1 alternate functions */
#define PIN_P3_1__GPIO                  ALIF_PINMUX(3, 1, 0)  /**< Port 3 Pin 1: GPIO */
#define PIN_P3_1__OSPI0_SCLKN_B         ALIF_PINMUX(3, 1, 1)  /**< Port 3 Pin 1: OSPI0_SCLKN_B */
#define PIN_P3_1__UART4_TX_A            ALIF_PINMUX(3, 1, 2)  /**< Port 3 Pin 1: UART4_TX_A */
#define PIN_P3_1__PDM_C0_B              ALIF_PINMUX(3, 1, 3)  /**< Port 3 Pin 1: PDM_C0_B */
#define PIN_P3_1__I2S0_WS_A             ALIF_PINMUX(3, 1, 4)  /**< Port 3 Pin 1: I2S0_WS_A */
#define PIN_P3_1__QEC0_Y_A              ALIF_PINMUX(3, 1, 5)  /**< Port 3 Pin 1: QEC0_Y_A */
#define PIN_P3_1__LPCAM_D5_B            ALIF_PINMUX(3, 1, 6)  /**< Port 3 Pin 1: LPCAM_D5_B */
#define PIN_P3_1__CAM_D5_A              ALIF_PINMUX(3, 1, 7)  /**< Port 3 Pin 1: CAM_D5_A */

/** Pin P3.2 alternate functions */
#define PIN_P3_2__GPIO                  ALIF_PINMUX(3, 2, 0)  /**< Port 3 Pin 2: GPIO */
#define PIN_P3_2__OSPI0_SS0_B           ALIF_PINMUX(3, 2, 1)  /**< Port 3 Pin 2: OSPI0_SS0_B */
#define PIN_P3_2__PDM_D1_B              ALIF_PINMUX(3, 2, 2)  /**< Port 3 Pin 2: PDM_D1_B */
#define PIN_P3_2__I2S1_SDI_A            ALIF_PINMUX(3, 2, 3)  /**< Port 3 Pin 2: I2S1_SDI_A */
#define PIN_P3_2__I3C_SDA_C             ALIF_PINMUX(3, 2, 4)  /**< Port 3 Pin 2: I3C_SDA_C */
#define PIN_P3_2__QEC0_Z_A              ALIF_PINMUX(3, 2, 5)  /**< Port 3 Pin 2: QEC0_Z_A */
#define PIN_P3_2__LPCAM_D6_B            ALIF_PINMUX(3, 2, 6)  /**< Port 3 Pin 2: LPCAM_D6_B */
#define PIN_P3_2__CAM_D6_A              ALIF_PINMUX(3, 2, 7)  /**< Port 3 Pin 2: CAM_D6_A */

/** Pin P3.3 alternate functions */
#define PIN_P3_3__GPIO                  ALIF_PINMUX(3, 3, 0)  /**< Port 3 Pin 3: GPIO */
#define PIN_P3_3__OSPI0_SS1_B           ALIF_PINMUX(3, 3, 1)  /**< Port 3 Pin 3: OSPI0_SS1_B */
#define PIN_P3_3__PDM_C1_B              ALIF_PINMUX(3, 3, 2)  /**< Port 3 Pin 3: PDM_C1_B */
#define PIN_P3_3__I2S1_SDO_A            ALIF_PINMUX(3, 3, 3)  /**< Port 3 Pin 3: I2S1_SDO_A */
#define PIN_P3_3__I3C_SCL_C             ALIF_PINMUX(3, 3, 4)  /**< Port 3 Pin 3: I3C_SCL_C */
#define PIN_P3_3__QEC1_X_A              ALIF_PINMUX(3, 3, 5)  /**< Port 3 Pin 3: QEC1_X_A */
#define PIN_P3_3__LPCAM_D7_B            ALIF_PINMUX(3, 3, 6)  /**< Port 3 Pin 3: LPCAM_D7_B */
#define PIN_P3_3__CAM_D7_A              ALIF_PINMUX(3, 3, 7)  /**< Port 3 Pin 3: CAM_D7_A */

/** Pin P3.4 alternate functions */
#define PIN_P3_4__GPIO                  ALIF_PINMUX(3, 4, 0)  /**< Port 3 Pin 4: GPIO */
#define PIN_P3_4__OSPI0_RXDS_B          ALIF_PINMUX(3, 4, 1)  /**< Port 3 Pin 4: OSPI0_RXDS_B */
#define PIN_P3_4__UART5_RX_A            ALIF_PINMUX(3, 4, 2)  /**< Port 3 Pin 4: UART5_RX_A */
#define PIN_P3_4__LPPDM_CO_B            ALIF_PINMUX(3, 4, 3)  /**< Port 3 Pin 4: LPPDM_CO_B */
#define PIN_P3_4__I2S1_SCLK_A           ALIF_PINMUX(3, 4, 4)  /**< Port 3 Pin 4: I2S1_SCLK_A */
#define PIN_P3_4__I2C0_SCL_B            ALIF_PINMUX(3, 4, 5)  /**< Port 3 Pin 4: I2C0_SCL_B */
#define PIN_P3_4__QEC1_Y_A              ALIF_PINMUX(3, 4, 6)  /**< Port 3 Pin 4: QEC1_Y_A */
#define PIN_P3_4__CAM_D8_A              ALIF_PINMUX(3, 4, 7)  /**< Port 3 Pin 4: CAM_D8_A */

/** Pin P3.5 alternate functions */
#define PIN_P3_5__GPIO                  ALIF_PINMUX(3, 5, 0)  /**< Port 3 Pin 5: GPIO */
#define PIN_P3_5__OSPI0_SCLKN_A         ALIF_PINMUX(3, 5, 1)  /**< Port 3 Pin 5: OSPI0_SCLKN_A */
#define PIN_P3_5__UART5_TX_A            ALIF_PINMUX(3, 5, 2)  /**< Port 3 Pin 5: UART5_TX_A */
#define PIN_P3_5__LPPDM_DO_B            ALIF_PINMUX(3, 5, 3)  /**< Port 3 Pin 5: LPPDM_DO_B */
#define PIN_P3_5__SPI0_SS1_B            ALIF_PINMUX(3, 5, 4)  /**< Port 3 Pin 5: SPI0_SS1_B */
#define PIN_P3_5__I2C0_SDA_B            ALIF_PINMUX(3, 5, 5)  /**< Port 3 Pin 5: I2C0_SDA_B */
#define PIN_P3_5__QEC1_Z_A              ALIF_PINMUX(3, 5, 6)  /**< Port 3 Pin 5: QEC1_Z_A */
#define PIN_P3_5__CAM_D9_A              ALIF_PINMUX(3, 5, 7)  /**< Port 3 Pin 5: CAM_D9_A */

/** Pin P3.6 alternate functions */
#define PIN_P3_6__GPIO                  ALIF_PINMUX(3, 6, 0)  /**< Port 3 Pin 6: GPIO */
#define PIN_P3_6__HFXO_OUT_A            ALIF_PINMUX(3, 6, 1)  /**< Port 3 Pin 6: HFXO_OUT_A */
#define PIN_P3_6__LPUART_CTS_B          ALIF_PINMUX(3, 6, 2)  /**< Port 3 Pin 6: LPUART_CTS_B */
#define PIN_P3_6__LPPDM_C1_B            ALIF_PINMUX(3, 6, 3)  /**< Port 3 Pin 6: LPPDM_C1_B */
#define PIN_P3_6__SPI0_SS2_B            ALIF_PINMUX(3, 6, 4)  /**< Port 3 Pin 6: SPI0_SS2_B */
#define PIN_P3_6__I2C1_SDA_B            ALIF_PINMUX(3, 6, 5)  /**< Port 3 Pin 6: I2C1_SDA_B */
#define PIN_P3_6__QEC2_X_A              ALIF_PINMUX(3, 6, 6)  /**< Port 3 Pin 6: QEC2_X_A */
#define PIN_P3_6__CAM_D10_A             ALIF_PINMUX(3, 6, 7)  /**< Port 3 Pin 6: CAM_D10_A */

/** Pin P3.7 alternate functions */
#define PIN_P3_7__GPIO                  ALIF_PINMUX(3, 7, 0)  /**< Port 3 Pin 7: GPIO */
#define PIN_P3_7__JTAG0_TRACECLK        ALIF_PINMUX(3, 7, 1)  /**< Port 3 Pin 7: JTAG0_TRACECLK */
#define PIN_P3_7__LPUART_RTS_B          ALIF_PINMUX(3, 7, 2)  /**< Port 3 Pin 7: LPUART_RTS_B */
#define PIN_P3_7__LPPDM_D1_B            ALIF_PINMUX(3, 7, 3)  /**< Port 3 Pin 7: LPPDM_D1_B */
#define PIN_P3_7__SPI1_SS1_A            ALIF_PINMUX(3, 7, 4)  /**< Port 3 Pin 7: SPI1_SS1_A */
#define PIN_P3_7__I2C1_SCL_B            ALIF_PINMUX(3, 7, 5)  /**< Port 3 Pin 7: I2C1_SCL_B */
#define PIN_P3_7__QEC2_Y_A              ALIF_PINMUX(3, 7, 6)  /**< Port 3 Pin 7: QEC2_Y_A */
#define PIN_P3_7__CAM_D11_A             ALIF_PINMUX(3, 7, 7)  /**< Port 3 Pin 7: CAM_D11_A */

/** @} */

/**
 * @name Port 4 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 4.
 * @{
 */

/** Pin P4.0 alternate functions */
#define PIN_P4_0__GPIO                  ALIF_PINMUX(4, 0, 0)  /**< Port 4 Pin 0: GPIO */
#define PIN_P4_0__JTAG0_TDATA0          ALIF_PINMUX(4, 0, 1)  /**< Port 4 Pin 0: JTAG0_TDATA0 */
#define PIN_P4_0__APLL_OUT_A            ALIF_PINMUX(4, 0, 2)  /**< Port 4 Pin 0: APLL_OUT_A */
#define PIN_P4_0__I2S1_WS_A             ALIF_PINMUX(4, 0, 3)  /**< Port 4 Pin 0: I2S1_WS_A */
#define PIN_P4_0__SPI1_SS2_A            ALIF_PINMUX(4, 0, 4)  /**< Port 4 Pin 0: SPI1_SS2_A */
#define PIN_P4_0__QEC2_Z_A              ALIF_PINMUX(4, 0, 5)  /**< Port 4 Pin 0: QEC2_Z_A */
#define PIN_P4_0__CDC_VSYNC_B           ALIF_PINMUX(4, 0, 6)  /**< Port 4 Pin 0: CDC_VSYNC_B */
#define PIN_P4_0__CAM_D12_A             ALIF_PINMUX(4, 0, 7)  /**< Port 4 Pin 0: CAM_D12_A */

/** Pin P4.1 alternate functions */
#define PIN_P4_1__GPIO                  ALIF_PINMUX(4, 1, 0)  /**< Port 4 Pin 1: GPIO */
#define PIN_P4_1__JTAG0_TDATA1          ALIF_PINMUX(4, 1, 1)  /**< Port 4 Pin 1: JTAG0_TDATA1 */
#define PIN_P4_1__I2S0_SDI_B            ALIF_PINMUX(4, 1, 2)  /**< Port 4 Pin 1: I2S0_SDI_B */
#define PIN_P4_1__SPI1_SS3_A            ALIF_PINMUX(4, 1, 3)  /**< Port 4 Pin 1: SPI1_SS3_A */
#define PIN_P4_1__QEC3_X_A              ALIF_PINMUX(4, 1, 4)  /**< Port 4 Pin 1: QEC3_X_A */
#define PIN_P4_1__SD_CLK_D              ALIF_PINMUX(4, 1, 5)  /**< Port 4 Pin 1: SD_CLK_D */
#define PIN_P4_1__CDC_HSYNC_B           ALIF_PINMUX(4, 1, 6)  /**< Port 4 Pin 1: CDC_HSYNC_B */
#define PIN_P4_1__CAM_D13_A             ALIF_PINMUX(4, 1, 7)  /**< Port 4 Pin 1: CAM_D13_A */

/** Pin P4.2 alternate functions */
#define PIN_P4_2__GPIO                  ALIF_PINMUX(4, 2, 0)  /**< Port 4 Pin 2: GPIO */
#define PIN_P4_2__JTAG0_TDATA2          ALIF_PINMUX(4, 2, 1)  /**< Port 4 Pin 2: JTAG0_TDATA2 */
#define PIN_P4_2__LPI2C1_SDA_A          ALIF_PINMUX(4, 2, 2)  /**< Port 4 Pin 2: LPI2C1_SDA_A */
#define PIN_P4_2__I2S0_SDO_B            ALIF_PINMUX(4, 2, 3)  /**< Port 4 Pin 2: I2S0_SDO_B */
#define PIN_P4_2__SPI2_MISO_A           ALIF_PINMUX(4, 2, 4)  /**< Port 4 Pin 2: SPI2_MISO_A */
#define PIN_P4_2__QEC3_Y_A              ALIF_PINMUX(4, 2, 5)  /**< Port 4 Pin 2: QEC3_Y_A */
#define PIN_P4_2__SD_CMD_D              ALIF_PINMUX(4, 2, 6)  /**< Port 4 Pin 2: SD_CMD_D */
#define PIN_P4_2__CAM_D14_A             ALIF_PINMUX(4, 2, 7)  /**< Port 4 Pin 2: CAM_D14_A */

/** Pin P4.3 alternate functions */
#define PIN_P4_3__GPIO                  ALIF_PINMUX(4, 3, 0)  /**< Port 4 Pin 3: GPIO */
#define PIN_P4_3__JTAG0_TDATA3          ALIF_PINMUX(4, 3, 1)  /**< Port 4 Pin 3: JTAG0_TDATA3 */
#define PIN_P4_3__LPI2C1_SCL_A          ALIF_PINMUX(4, 3, 2)  /**< Port 4 Pin 3: LPI2C1_SCL_A */
#define PIN_P4_3__I2S0_SCLK_B           ALIF_PINMUX(4, 3, 3)  /**< Port 4 Pin 3: I2S0_SCLK_B */
#define PIN_P4_3__SPI2_MOSI_A           ALIF_PINMUX(4, 3, 4)  /**< Port 4 Pin 3: SPI2_MOSI_A */
#define PIN_P4_3__QEC3_Z_A              ALIF_PINMUX(4, 3, 5)  /**< Port 4 Pin 3: QEC3_Z_A */
#define PIN_P4_3__SD_RST_D              ALIF_PINMUX(4, 3, 6)  /**< Port 4 Pin 3: SD_RST_D */
#define PIN_P4_3__CAM_D15_A             ALIF_PINMUX(4, 3, 7)  /**< Port 4 Pin 3: CAM_D15_A */

/** Pin P4.4 alternate functions */
#define PIN_P4_4__GPIO                  ALIF_PINMUX(4, 4, 0)  /**< Port 4 Pin 4: GPIO */
#define PIN_P4_4__JTAG0_TCK             ALIF_PINMUX(4, 4, 1)  /**< Port 4 Pin 4: JTAG0_TCK */
#define PIN_P4_4__I2S0_WS_B             ALIF_PINMUX(4, 4, 2)  /**< Port 4 Pin 4: I2S0_WS_B */
#define PIN_P4_4__SPI2_SCLK_A           ALIF_PINMUX(4, 4, 3)  /**< Port 4 Pin 4: SPI2_SCLK_A */
#define PIN_P4_4__FAULT0_A              ALIF_PINMUX(4, 4, 4)  /**< Port 4 Pin 4: FAULT0_A */

/** Pin P4.5 alternate functions */
#define PIN_P4_5__GPIO                  ALIF_PINMUX(4, 5, 0)  /**< Port 4 Pin 5: GPIO */
#define PIN_P4_5__JTAG0_TMS             ALIF_PINMUX(4, 5, 1)  /**< Port 4 Pin 5: JTAG0_TMS */
#define PIN_P4_5__SPI2_SS0_A            ALIF_PINMUX(4, 5, 2)  /**< Port 4 Pin 5: SPI2_SS0_A */
#define PIN_P4_5__FAULT1_A              ALIF_PINMUX(4, 5, 3)  /**< Port 4 Pin 5: FAULT1_A */

/** Pin P4.6 alternate functions */
#define PIN_P4_6__GPIO                  ALIF_PINMUX(4, 6, 0)  /**< Port 4 Pin 6: GPIO */
#define PIN_P4_6__JTAG0_TDI             ALIF_PINMUX(4, 6, 1)  /**< Port 4 Pin 6: JTAG0_TDI */
#define PIN_P4_6__SPI2_SS1_A            ALIF_PINMUX(4, 6, 2)  /**< Port 4 Pin 6: SPI2_SS1_A */
#define PIN_P4_6__FAULT2_A              ALIF_PINMUX(4, 6, 3)  /**< Port 4 Pin 6: FAULT2_A */

/** Pin P4.7 alternate functions */
#define PIN_P4_7__GPIO                  ALIF_PINMUX(4, 7, 0)  /**< Port 4 Pin 7: GPIO */
#define PIN_P4_7__JTAG0_TDO             ALIF_PINMUX(4, 7, 1)  /**< Port 4 Pin 7: JTAG0_TDO */
#define PIN_P4_7__SPI2_SS2_A            ALIF_PINMUX(4, 7, 2)  /**< Port 4 Pin 7: SPI2_SS2_A */
#define PIN_P4_7__FAULT3_A              ALIF_PINMUX(4, 7, 3)  /**< Port 4 Pin 7: FAULT3_A */

/** @} */

/**
 * @name Port 5 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 5.
 * @{
 */

/** Pin P5.0 alternate functions */
#define PIN_P5_0__GPIO                  ALIF_PINMUX(5, 0, 0)  /**< Port 5 Pin 0: GPIO */
#define PIN_P5_0__OSPI1_RXDS_A          ALIF_PINMUX(5, 0, 1)  /**< Port 5 Pin 0: OSPI1_RXDS_A */
#define PIN_P5_0__UART4_RX_C            ALIF_PINMUX(5, 0, 2)  /**< Port 5 Pin 0: UART4_RX_C */
#define PIN_P5_0__PDM_D2_A              ALIF_PINMUX(5, 0, 3)  /**< Port 5 Pin 0: PDM_D2_A */
#define PIN_P5_0__SPI0_MISO_B           ALIF_PINMUX(5, 0, 4)  /**< Port 5 Pin 0: SPI0_MISO_B */
#define PIN_P5_0__I2C2_SDA_B            ALIF_PINMUX(5, 0, 5)  /**< Port 5 Pin 0: I2C2_SDA_B */
#define PIN_P5_0__UT0_T0_B              ALIF_PINMUX(5, 0, 6)  /**< Port 5 Pin 0: UT0_T0_B */
#define PIN_P5_0__SD_D0_A               ALIF_PINMUX(5, 0, 7)  /**< Port 5 Pin 0: SD_D0_A */

/** Pin P5.1 alternate functions */
#define PIN_P5_1__GPIO                  ALIF_PINMUX(5, 1, 0)  /**< Port 5 Pin 1: GPIO */
#define PIN_P5_1__OSPI1_SS0_A           ALIF_PINMUX(5, 1, 1)  /**< Port 5 Pin 1: OSPI1_SS0_A */
#define PIN_P5_1__UART4_TX_C            ALIF_PINMUX(5, 1, 2)  /**< Port 5 Pin 1: UART4_TX_C */
#define PIN_P5_1__PDM_D3_A              ALIF_PINMUX(5, 1, 3)  /**< Port 5 Pin 1: PDM_D3_A */
#define PIN_P5_1__SPI0_MOSI_B           ALIF_PINMUX(5, 1, 4)  /**< Port 5 Pin 1: SPI0_MOSI_B */
#define PIN_P5_1__I2C2_SCL_B            ALIF_PINMUX(5, 1, 5)  /**< Port 5 Pin 1: I2C2_SCL_B */
#define PIN_P5_1__UT0_T1_B              ALIF_PINMUX(5, 1, 6)  /**< Port 5 Pin 1: UT0_T1_B */
#define PIN_P5_1__SD_D1_A               ALIF_PINMUX(5, 1, 7)  /**< Port 5 Pin 1: SD_D1_A */

/** Pin P5.2 alternate functions */
#define PIN_P5_2__GPIO                  ALIF_PINMUX(5, 2, 0)  /**< Port 5 Pin 2: GPIO */
#define PIN_P5_2__OSPI1_SCLKN_A         ALIF_PINMUX(5, 2, 1)  /**< Port 5 Pin 2: OSPI1_SCLKN_A */
#define PIN_P5_2__UART5_RX_C            ALIF_PINMUX(5, 2, 2)  /**< Port 5 Pin 2: UART5_RX_C */
#define PIN_P5_2__PDM_C3_A              ALIF_PINMUX(5, 2, 3)  /**< Port 5 Pin 2: PDM_C3_A */
#define PIN_P5_2__SPI0_SS0_B            ALIF_PINMUX(5, 2, 4)  /**< Port 5 Pin 2: SPI0_SS0_B */
#define PIN_P5_2__LPI2C0_SCL_B          ALIF_PINMUX(5, 2, 5)  /**< Port 5 Pin 2: LPI2C0_SCL_B */
#define PIN_P5_2__UT1_T0_B              ALIF_PINMUX(5, 2, 6)  /**< Port 5 Pin 2: UT1_T0_B */
#define PIN_P5_2__SD_D2_A               ALIF_PINMUX(5, 2, 7)  /**< Port 5 Pin 2: SD_D2_A */

/** Pin P5.3 alternate functions */
#define PIN_P5_3__GPIO                  ALIF_PINMUX(5, 3, 0)  /**< Port 5 Pin 3: GPIO */
#define PIN_P5_3__OSPI1_SCLK_A          ALIF_PINMUX(5, 3, 1)  /**< Port 5 Pin 3: OSPI1_SCLK_A */
#define PIN_P5_3__UART5_TX_C            ALIF_PINMUX(5, 3, 2)  /**< Port 5 Pin 3: UART5_TX_C */
#define PIN_P5_3__SPI0_SCLK_B           ALIF_PINMUX(5, 3, 3)  /**< Port 5 Pin 3: SPI0_SCLK_B */
#define PIN_P5_3__LPI2C0_SDA_B          ALIF_PINMUX(5, 3, 4)  /**< Port 5 Pin 3: LPI2C0_SDA_B */
#define PIN_P5_3__UT1_T1_B              ALIF_PINMUX(5, 3, 5)  /**< Port 5 Pin 3: UT1_T1_B */
#define PIN_P5_3__SD_D3_A               ALIF_PINMUX(5, 3, 6)  /**< Port 5 Pin 3: SD_D3_A */
#define PIN_P5_3__CDC_PCLK_A            ALIF_PINMUX(5, 3, 7)  /**< Port 5 Pin 3: CDC_PCLK_A */

/** Pin P5.4 alternate functions */
#define PIN_P5_4__GPIO                  ALIF_PINMUX(5, 4, 0)  /**< Port 5 Pin 4: GPIO */
#define PIN_P5_4__OSPI1_SS1_A           ALIF_PINMUX(5, 4, 1)  /**< Port 5 Pin 4: OSPI1_SS1_A */
#define PIN_P5_4__UART3_CTS_A           ALIF_PINMUX(5, 4, 2)  /**< Port 5 Pin 4: UART3_CTS_A */
#define PIN_P5_4__PDM_D2_B              ALIF_PINMUX(5, 4, 3)  /**< Port 5 Pin 4: PDM_D2_B */
#define PIN_P5_4__SPI0_SS3_A            ALIF_PINMUX(5, 4, 4)  /**< Port 5 Pin 4: SPI0_SS3_A */
#define PIN_P5_4__UT2_T0_B              ALIF_PINMUX(5, 4, 5)  /**< Port 5 Pin 4: UT2_T0_B */
#define PIN_P5_4__SD_D4_A               ALIF_PINMUX(5, 4, 6)  /**< Port 5 Pin 4: SD_D4_A */
#define PIN_P5_4__CDC_DE_A              ALIF_PINMUX(5, 4, 7)  /**< Port 5 Pin 4: CDC_DE_A */

/** Pin P5.5 alternate functions */
#define PIN_P5_5__GPIO                  ALIF_PINMUX(5, 5, 0)  /**< Port 5 Pin 5: GPIO */
#define PIN_P5_5__OSPI1_SCLK_C          ALIF_PINMUX(5, 5, 1)  /**< Port 5 Pin 5: OSPI1_SCLK_C */
#define PIN_P5_5__UART3_RTS_A           ALIF_PINMUX(5, 5, 2)  /**< Port 5 Pin 5: UART3_RTS_A */
#define PIN_P5_5__PDM_D3_B              ALIF_PINMUX(5, 5, 3)  /**< Port 5 Pin 5: PDM_D3_B */
#define PIN_P5_5__UT2_T1_B              ALIF_PINMUX(5, 5, 4)  /**< Port 5 Pin 5: UT2_T1_B */
#define PIN_P5_5__SD_D5_A               ALIF_PINMUX(5, 5, 5)  /**< Port 5 Pin 5: SD_D5_A */
#define PIN_P5_5__ETH_RXD0_A            ALIF_PINMUX(5, 5, 6)  /**< Port 5 Pin 5: ETH_RXD0_A */
#define PIN_P5_5__CDC_HSYNC_A           ALIF_PINMUX(5, 5, 7)  /**< Port 5 Pin 5: CDC_HSYNC_A */

/** Pin P5.6 alternate functions */
#define PIN_P5_6__GPIO                  ALIF_PINMUX(5, 6, 0)  /**< Port 5 Pin 6: GPIO */
#define PIN_P5_6__UART1_CTS_B           ALIF_PINMUX(5, 6, 1)  /**< Port 5 Pin 6: UART1_CTS_B */
#define PIN_P5_6__I2C2_SCL_C            ALIF_PINMUX(5, 6, 2)  /**< Port 5 Pin 6: I2C2_SCL_C */
#define PIN_P5_6__UT3_T0_B              ALIF_PINMUX(5, 6, 3)  /**< Port 5 Pin 6: UT3_T0_B */
#define PIN_P5_6__SD_D6_A               ALIF_PINMUX(5, 6, 4)  /**< Port 5 Pin 6: SD_D6_A */
#define PIN_P5_6__ETH_RXD1_A            ALIF_PINMUX(5, 6, 5)  /**< Port 5 Pin 6: ETH_RXD1_A */
#define PIN_P5_6__CDC_VSYNC_A           ALIF_PINMUX(5, 6, 6)  /**< Port 5 Pin 6: CDC_VSYNC_A */

/** Pin P5.7 alternate functions */
#define PIN_P5_7__GPIO                  ALIF_PINMUX(5, 7, 0)  /**< Port 5 Pin 7: GPIO */
#define PIN_P5_7__OSPI1_SS0_C           ALIF_PINMUX(5, 7, 1)  /**< Port 5 Pin 7: OSPI1_SS0_C */
#define PIN_P5_7__UART1_RTS_B           ALIF_PINMUX(5, 7, 2)  /**< Port 5 Pin 7: UART1_RTS_B */
#define PIN_P5_7__I2C2_SDA_C            ALIF_PINMUX(5, 7, 3)  /**< Port 5 Pin 7: I2C2_SDA_C */
#define PIN_P5_7__UT3_T1_B              ALIF_PINMUX(5, 7, 4)  /**< Port 5 Pin 7: UT3_T1_B */
#define PIN_P5_7__SD_D7_A               ALIF_PINMUX(5, 7, 5)  /**< Port 5 Pin 7: SD_D7_A */
#define PIN_P5_7__ETH_RST_A             ALIF_PINMUX(5, 7, 6)  /**< Port 5 Pin 7: ETH_RST_A */

/** @} */

/**
 * @name Port 6 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 6.
 * @{
 */

/** Pin P6.0 alternate functions */
#define PIN_P6_0__GPIO                  ALIF_PINMUX(6, 0, 0)  /**< Port 6 Pin 0: GPIO */
#define PIN_P6_0__OSPI0_D0_C            ALIF_PINMUX(6, 0, 1)  /**< Port 6 Pin 0: OSPI0_D0_C */
#define PIN_P6_0__UART4_DE_A            ALIF_PINMUX(6, 0, 2)  /**< Port 6 Pin 0: UART4_DE_A */
#define PIN_P6_0__PDM_D0_C              ALIF_PINMUX(6, 0, 3)  /**< Port 6 Pin 0: PDM_D0_C */
#define PIN_P6_0__UT4_T0_B              ALIF_PINMUX(6, 0, 4)  /**< Port 6 Pin 0: UT4_T0_B */
#define PIN_P6_0__SD_D0_D               ALIF_PINMUX(6, 0, 5)  /**< Port 6 Pin 0: SD_D0_D */
#define PIN_P6_0__ETH_TXD0_A            ALIF_PINMUX(6, 0, 6)  /**< Port 6 Pin 0: ETH_TXD0_A */
#define PIN_P6_0__LPSPI_SS1_A           ALIF_PINMUX(6, 0, 7)  /**< Port 6 Pin 0: LPSPI_SS1_A */

/** Pin P6.1 alternate functions */
#define PIN_P6_1__GPIO                  ALIF_PINMUX(6, 1, 0)  /**< Port 6 Pin 1: GPIO */
#define PIN_P6_1__OSPI0_D1_C            ALIF_PINMUX(6, 1, 1)  /**< Port 6 Pin 1: OSPI0_D1_C */
#define PIN_P6_1__UART5_DE_A            ALIF_PINMUX(6, 1, 2)  /**< Port 6 Pin 1: UART5_DE_A */
#define PIN_P6_1__PDM_C0_C              ALIF_PINMUX(6, 1, 3)  /**< Port 6 Pin 1: PDM_C0_C */
#define PIN_P6_1__UT4_T1_B              ALIF_PINMUX(6, 1, 4)  /**< Port 6 Pin 1: UT4_T1_B */
#define PIN_P6_1__SD_D1_D               ALIF_PINMUX(6, 1, 5)  /**< Port 6 Pin 1: SD_D1_D */
#define PIN_P6_1__ETH_TXD1_A            ALIF_PINMUX(6, 1, 6)  /**< Port 6 Pin 1: ETH_TXD1_A */
#define PIN_P6_1__LPSPI_SS2_A           ALIF_PINMUX(6, 1, 7)  /**< Port 6 Pin 1: LPSPI_SS2_A */

/** Pin P6.2 alternate functions */
#define PIN_P6_2__GPIO                  ALIF_PINMUX(6, 2, 0)  /**< Port 6 Pin 2: GPIO */
#define PIN_P6_2__OSPI0_D2_C            ALIF_PINMUX(6, 2, 1)  /**< Port 6 Pin 2: OSPI0_D2_C */
#define PIN_P6_2__UART2_CTS_A           ALIF_PINMUX(6, 2, 2)  /**< Port 6 Pin 2: UART2_CTS_A */
#define PIN_P6_2__LPI3C_SDA_A           ALIF_PINMUX(6, 2, 3)  /**< Port 6 Pin 2: LPI3C_SDA_A */
#define PIN_P6_2__PDM_D1_C              ALIF_PINMUX(6, 2, 4)  /**< Port 6 Pin 2: PDM_D1_C */
#define PIN_P6_2__UT5_T0_B              ALIF_PINMUX(6, 2, 5)  /**< Port 6 Pin 2: UT5_T0_B */
#define PIN_P6_2__SD_D2_D               ALIF_PINMUX(6, 2, 6)  /**< Port 6 Pin 2: SD_D2_D */
#define PIN_P6_2__ETH_TXEN_A            ALIF_PINMUX(6, 2, 7)  /**< Port 6 Pin 2: ETH_TXEN_A */

/** Pin P6.3 alternate functions */
#define PIN_P6_3__GPIO                  ALIF_PINMUX(6, 3, 0)  /**< Port 6 Pin 3: GPIO */
#define PIN_P6_3__OSPI0_D3_C            ALIF_PINMUX(6, 3, 1)  /**< Port 6 Pin 3: OSPI0_D3_C */
#define PIN_P6_3__UART2_RTS_A           ALIF_PINMUX(6, 3, 2)  /**< Port 6 Pin 3: UART2_RTS_A */
#define PIN_P6_3__LPI3C_SCL_A           ALIF_PINMUX(6, 3, 3)  /**< Port 6 Pin 3: LPI3C_SCL_A */
#define PIN_P6_3__PDM_C1_C              ALIF_PINMUX(6, 3, 4)  /**< Port 6 Pin 3: PDM_C1_C */
#define PIN_P6_3__UT5_T1_B              ALIF_PINMUX(6, 3, 5)  /**< Port 6 Pin 3: UT5_T1_B */
#define PIN_P6_3__SD_D3_D               ALIF_PINMUX(6, 3, 6)  /**< Port 6 Pin 3: SD_D3_D */
#define PIN_P6_3__ETH_IRQ_A             ALIF_PINMUX(6, 3, 7)  /**< Port 6 Pin 3: ETH_IRQ_A */

/** Pin P6.4 alternate functions */
#define PIN_P6_4__GPIO                  ALIF_PINMUX(6, 4, 0)  /**< Port 6 Pin 4: GPIO */
#define PIN_P6_4__OSPI0_D4_C            ALIF_PINMUX(6, 4, 1)  /**< Port 6 Pin 4: OSPI0_D4_C */
#define PIN_P6_4__UART2_CTS_B           ALIF_PINMUX(6, 4, 2)  /**< Port 6 Pin 4: UART2_CTS_B */
#define PIN_P6_4__LRFC_OUT_A            ALIF_PINMUX(6, 4, 3)  /**< Port 6 Pin 4: LRFC_OUT_A */
#define PIN_P6_4__SPI1_SS0_B            ALIF_PINMUX(6, 4, 4)  /**< Port 6 Pin 4: SPI1_SS0_B */
#define PIN_P6_4__UT6_T0_B              ALIF_PINMUX(6, 4, 5)  /**< Port 6 Pin 4: UT6_T0_B */
#define PIN_P6_4__SD_D4_D               ALIF_PINMUX(6, 4, 6)  /**< Port 6 Pin 4: SD_D4_D */
#define PIN_P6_4__ETH_REFCLK_A          ALIF_PINMUX(6, 4, 7)  /**< Port 6 Pin 4: ETH_REFCLK_A */

/** Pin P6.5 alternate functions */
#define PIN_P6_5__GPIO                  ALIF_PINMUX(6, 5, 0)  /**< Port 6 Pin 5: GPIO */
#define PIN_P6_5__OSPI0_D5_C            ALIF_PINMUX(6, 5, 1)  /**< Port 6 Pin 5: OSPI0_D5_C */
#define PIN_P6_5__UART2_RTS_B           ALIF_PINMUX(6, 5, 2)  /**< Port 6 Pin 5: UART2_RTS_B */
#define PIN_P6_5__LPI2C1_SDA_B          ALIF_PINMUX(6, 5, 3)  /**< Port 6 Pin 5: LPI2C1_SDA_B */
#define PIN_P6_5__SPI1_SS1_B            ALIF_PINMUX(6, 5, 4)  /**< Port 6 Pin 5: SPI1_SS1_B */
#define PIN_P6_5__UT6_T1_B              ALIF_PINMUX(6, 5, 5)  /**< Port 6 Pin 5: UT6_T1_B */
#define PIN_P6_5__SD_D5_D               ALIF_PINMUX(6, 5, 6)  /**< Port 6 Pin 5: SD_D5_D */
#define PIN_P6_5__ETH_MDIO_A            ALIF_PINMUX(6, 5, 7)  /**< Port 6 Pin 5: ETH_MDIO_A */

/** Pin P6.6 alternate functions */
#define PIN_P6_6__GPIO                  ALIF_PINMUX(6, 6, 0)  /**< Port 6 Pin 6: GPIO */
#define PIN_P6_6__OSPI0_D6_C            ALIF_PINMUX(6, 6, 1)  /**< Port 6 Pin 6: OSPI0_D6_C */
#define PIN_P6_6__UART0_CTS_B           ALIF_PINMUX(6, 6, 2)  /**< Port 6 Pin 6: UART0_CTS_B */
#define PIN_P6_6__LPI2C1_SCL_B          ALIF_PINMUX(6, 6, 3)  /**< Port 6 Pin 6: LPI2C1_SCL_B */
#define PIN_P6_6__SPI1_SS2_B            ALIF_PINMUX(6, 6, 4)  /**< Port 6 Pin 6: SPI1_SS2_B */
#define PIN_P6_6__UT7_T0_B              ALIF_PINMUX(6, 6, 5)  /**< Port 6 Pin 6: UT7_T0_B */
#define PIN_P6_6__SD_D6_D               ALIF_PINMUX(6, 6, 6)  /**< Port 6 Pin 6: SD_D6_D */
#define PIN_P6_6__ETH_MDC_A             ALIF_PINMUX(6, 6, 7)  /**< Port 6 Pin 6: ETH_MDC_A */

/** Pin P6.7 alternate functions */
#define PIN_P6_7__GPIO                  ALIF_PINMUX(6, 7, 0)  /**< Port 6 Pin 7: GPIO */
#define PIN_P6_7__OSPI0_D7_C            ALIF_PINMUX(6, 7, 1)  /**< Port 6 Pin 7: OSPI0_D7_C */
#define PIN_P6_7__UART0_RTS_B           ALIF_PINMUX(6, 7, 2)  /**< Port 6 Pin 7: UART0_RTS_B */
#define PIN_P6_7__PDM_C2_A              ALIF_PINMUX(6, 7, 3)  /**< Port 6 Pin 7: PDM_C2_A */
#define PIN_P6_7__SPI1_SS3_B            ALIF_PINMUX(6, 7, 4)  /**< Port 6 Pin 7: SPI1_SS3_B */
#define PIN_P6_7__UT7_T1_B              ALIF_PINMUX(6, 7, 5)  /**< Port 6 Pin 7: UT7_T1_B */
#define PIN_P6_7__SD_D7_D               ALIF_PINMUX(6, 7, 6)  /**< Port 6 Pin 7: SD_D7_D */
#define PIN_P6_7__ETH_CRS_DV_A          ALIF_PINMUX(6, 7, 7)  /**< Port 6 Pin 7: ETH_CRS_DV_A */

/** @} */

/**
 * @name Port 7 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 7.
 * @{
 */

/** Pin P7.0 alternate functions */
#define PIN_P7_0__GPIO                  ALIF_PINMUX(7, 0, 0)  /**< Port 7 Pin 0: GPIO */
#define PIN_P7_0__LPUT0_T0_A            ALIF_PINMUX(7, 0, 1)  /**< Port 7 Pin 0: LPUT0_T0_A */
#define PIN_P7_0__CMP3_OUT_A            ALIF_PINMUX(7, 0, 2)  /**< Port 7 Pin 0: CMP3_OUT_A */
#define PIN_P7_0__SPI0_MISO_C           ALIF_PINMUX(7, 0, 3)  /**< Port 7 Pin 0: SPI0_MISO_C */
#define PIN_P7_0__I2C0_SDA_C            ALIF_PINMUX(7, 0, 4)  /**< Port 7 Pin 0: I2C0_SDA_C */
#define PIN_P7_0__UT8_T0_B              ALIF_PINMUX(7, 0, 5)  /**< Port 7 Pin 0: UT8_T0_B */
#define PIN_P7_0__SD_CMD_A              ALIF_PINMUX(7, 0, 6)  /**< Port 7 Pin 0: SD_CMD_A */
#define PIN_P7_0__CAN_RXD_A             ALIF_PINMUX(7, 0, 7)  /**< Port 7 Pin 0: CAN_RXD_A */

/** Pin P7.1 alternate functions */
#define PIN_P7_1__GPIO                  ALIF_PINMUX(7, 1, 0)  /**< Port 7 Pin 1: GPIO */
#define PIN_P7_1__LPUT0_T1_A            ALIF_PINMUX(7, 1, 1)  /**< Port 7 Pin 1: LPUT0_T1_A */
#define PIN_P7_1__CMP2_OUT_A            ALIF_PINMUX(7, 1, 2)  /**< Port 7 Pin 1: CMP2_OUT_A */
#define PIN_P7_1__SPI0_MOSI_C           ALIF_PINMUX(7, 1, 3)  /**< Port 7 Pin 1: SPI0_MOSI_C */
#define PIN_P7_1__I2C0_SCL_C            ALIF_PINMUX(7, 1, 4)  /**< Port 7 Pin 1: I2C0_SCL_C */
#define PIN_P7_1__UT8_T1_B              ALIF_PINMUX(7, 1, 5)  /**< Port 7 Pin 1: UT8_T1_B */
#define PIN_P7_1__SD_CLK_A              ALIF_PINMUX(7, 1, 6)  /**< Port 7 Pin 1: SD_CLK_A */
#define PIN_P7_1__CAN_TXD_A             ALIF_PINMUX(7, 1, 7)  /**< Port 7 Pin 1: CAN_TXD_A */

/** Pin P7.2 alternate functions */
#define PIN_P7_2__GPIO                  ALIF_PINMUX(7, 2, 0)  /**< Port 7 Pin 2: GPIO */
#define PIN_P7_2__LPUT1_T0_A            ALIF_PINMUX(7, 2, 1)  /**< Port 7 Pin 2: LPUT1_T0_A */
#define PIN_P7_2__UART3_CTS_B           ALIF_PINMUX(7, 2, 2)  /**< Port 7 Pin 2: UART3_CTS_B */
#define PIN_P7_2__CMP1_OUT_A            ALIF_PINMUX(7, 2, 3)  /**< Port 7 Pin 2: CMP1_OUT_A */
#define PIN_P7_2__SPI0_SCLK_C           ALIF_PINMUX(7, 2, 4)  /**< Port 7 Pin 2: SPI0_SCLK_C */
#define PIN_P7_2__I2C1_SDA_C            ALIF_PINMUX(7, 2, 5)  /**< Port 7 Pin 2: I2C1_SDA_C */
#define PIN_P7_2__UT9_T0_B              ALIF_PINMUX(7, 2, 6)  /**< Port 7 Pin 2: UT9_T0_B */
#define PIN_P7_2__SD_RST_A              ALIF_PINMUX(7, 2, 7)  /**< Port 7 Pin 2: SD_RST_A */

/** Pin P7.3 alternate functions */
#define PIN_P7_3__GPIO                  ALIF_PINMUX(7, 3, 0)  /**< Port 7 Pin 3: GPIO */
#define PIN_P7_3__LPUT1_T1_A            ALIF_PINMUX(7, 3, 1)  /**< Port 7 Pin 3: LPUT1_T1_A */
#define PIN_P7_3__UART3_RTS_B           ALIF_PINMUX(7, 3, 2)  /**< Port 7 Pin 3: UART3_RTS_B */
#define PIN_P7_3__CMP0_OUT_A            ALIF_PINMUX(7, 3, 3)  /**< Port 7 Pin 3: CMP0_OUT_A */
#define PIN_P7_3__SPI0_SS0_C            ALIF_PINMUX(7, 3, 4)  /**< Port 7 Pin 3: SPI0_SS0_C */
#define PIN_P7_3__I2C1_SCL_C            ALIF_PINMUX(7, 3, 5)  /**< Port 7 Pin 3: I2C1_SCL_C */
#define PIN_P7_3__UT9_T1_B              ALIF_PINMUX(7, 3, 6)  /**< Port 7 Pin 3: UT9_T1_B */
#define PIN_P7_3__CAN_STBY_A            ALIF_PINMUX(7, 3, 7)  /**< Port 7 Pin 3: CAN_STBY_A */

/** Pin P7.4 alternate functions */
#define PIN_P7_4__GPIO                  ALIF_PINMUX(7, 4, 0)  /**< Port 7 Pin 4: GPIO */
#define PIN_P7_4__LPUT2_T0_A            ALIF_PINMUX(7, 4, 1)  /**< Port 7 Pin 4: LPUT2_T0_A */
#define PIN_P7_4__LPUART_CTS_A          ALIF_PINMUX(7, 4, 2)  /**< Port 7 Pin 4: LPUART_CTS_A */
#define PIN_P7_4__LPPDM_C2_A            ALIF_PINMUX(7, 4, 3)  /**< Port 7 Pin 4: LPPDM_C2_A */
#define PIN_P7_4__LPSPI_MISO_A          ALIF_PINMUX(7, 4, 4)  /**< Port 7 Pin 4: LPSPI_MISO_A */
#define PIN_P7_4__LPI2C0_SCL_A          ALIF_PINMUX(7, 4, 5)  /**< Port 7 Pin 4: LPI2C0_SCL_A */
#define PIN_P7_4__UT10_T0_B             ALIF_PINMUX(7, 4, 6)  /**< Port 7 Pin 4: UT10_T0_B */

/** Pin P7.5 alternate functions */
#define PIN_P7_5__GPIO                  ALIF_PINMUX(7, 5, 0)  /**< Port 7 Pin 5: GPIO */
#define PIN_P7_5__LPUT2_T1_A            ALIF_PINMUX(7, 5, 1)  /**< Port 7 Pin 5: LPUT2_T1_A */
#define PIN_P7_5__LPUART_RTS_A          ALIF_PINMUX(7, 5, 2)  /**< Port 7 Pin 5: LPUART_RTS_A */
#define PIN_P7_5__LFXO_OUT_A            ALIF_PINMUX(7, 5, 3)  /**< Port 7 Pin 5: LFXO_OUT_A */
#define PIN_P7_5__LPPDM_D2_A            ALIF_PINMUX(7, 5, 4)  /**< Port 7 Pin 5: LPPDM_D2_A */
#define PIN_P7_5__LPSPI_MOSI_A          ALIF_PINMUX(7, 5, 5)  /**< Port 7 Pin 5: LPSPI_MOSI_A */
#define PIN_P7_5__LPI2C0_SDA_A          ALIF_PINMUX(7, 5, 6)  /**< Port 7 Pin 5: LPI2C0_SDA_A */
#define PIN_P7_5__UT10_T1_B             ALIF_PINMUX(7, 5, 7)  /**< Port 7 Pin 5: UT10_T1_B */

/** Pin P7.6 alternate functions */
#define PIN_P7_6__GPIO                  ALIF_PINMUX(7, 6, 0)  /**< Port 7 Pin 6: GPIO */
#define PIN_P7_6__LPI2C1_SDA_C          ALIF_PINMUX(7, 6, 1)  /**< Port 7 Pin 6: LPI2C1_SDA_C */
#define PIN_P7_6__LPUART_RX_A           ALIF_PINMUX(7, 6, 2)  /**< Port 7 Pin 6: LPUART_RX_A */
#define PIN_P7_6__LPI3C_SDA_B           ALIF_PINMUX(7, 6, 3)  /**< Port 7 Pin 6: LPI3C_SDA_B */
#define PIN_P7_6__LPPDM_C3_A            ALIF_PINMUX(7, 6, 4)  /**< Port 7 Pin 6: LPPDM_C3_A */
#define PIN_P7_6__LPSPI_SCLK_A          ALIF_PINMUX(7, 6, 5)  /**< Port 7 Pin 6: LPSPI_SCLK_A */
#define PIN_P7_6__I3C_SDA_D             ALIF_PINMUX(7, 6, 6)  /**< Port 7 Pin 6: I3C_SDA_D */
#define PIN_P7_6__UT11_T0_B             ALIF_PINMUX(7, 6, 7)  /**< Port 7 Pin 6: UT11_T0_B */

/** Pin P7.7 alternate functions */
#define PIN_P7_7__GPIO                  ALIF_PINMUX(7, 7, 0)  /**< Port 7 Pin 7: GPIO */
#define PIN_P7_7__LPI2C1_SCL_C          ALIF_PINMUX(7, 7, 1)  /**< Port 7 Pin 7: LPI2C1_SCL_C */
#define PIN_P7_7__LPUART_TX_A           ALIF_PINMUX(7, 7, 2)  /**< Port 7 Pin 7: LPUART_TX_A */
#define PIN_P7_7__LPI3C_SCL_B           ALIF_PINMUX(7, 7, 3)  /**< Port 7 Pin 7: LPI3C_SCL_B */
#define PIN_P7_7__LPPDM_D3_A            ALIF_PINMUX(7, 7, 4)  /**< Port 7 Pin 7: LPPDM_D3_A */
#define PIN_P7_7__LPSPI_SS0_A           ALIF_PINMUX(7, 7, 5)  /**< Port 7 Pin 7: LPSPI_SS0_A */
#define PIN_P7_7__I3C_SCL_D             ALIF_PINMUX(7, 7, 6)  /**< Port 7 Pin 7: I3C_SCL_D */
#define PIN_P7_7__UT11_T1_B             ALIF_PINMUX(7, 7, 7)  /**< Port 7 Pin 7: UT11_T1_B */

/** @} */

/**
 * @name Port 8 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 8.
 * @{
 */

/** Pin P8.0 alternate functions */
#define PIN_P8_0__GPIO                  ALIF_PINMUX(8, 0, 0)  /**< Port 8 Pin 0: GPIO */
#define PIN_P8_0__OSPI1_SCLKN_C         ALIF_PINMUX(8, 0, 1)  /**< Port 8 Pin 0: OSPI1_SCLKN_C */
#define PIN_P8_0__AUDIO_CLK_A           ALIF_PINMUX(8, 0, 2)  /**< Port 8 Pin 0: AUDIO_CLK_A */
#define PIN_P8_0__FAULT0_B              ALIF_PINMUX(8, 0, 3)  /**< Port 8 Pin 0: FAULT0_B */
#define PIN_P8_0__LPCAM_D0_A            ALIF_PINMUX(8, 0, 4)  /**< Port 8 Pin 0: LPCAM_D0_A */
#define PIN_P8_0__SD_D0_C               ALIF_PINMUX(8, 0, 5)  /**< Port 8 Pin 0: SD_D0_C */
#define PIN_P8_0__CDC_D0_A              ALIF_PINMUX(8, 0, 6)  /**< Port 8 Pin 0: CDC_D0_A */
#define PIN_P8_0__CAM_D0_B              ALIF_PINMUX(8, 0, 7)  /**< Port 8 Pin 0: CAM_D0_B */

/** Pin P8.1 alternate functions */
#define PIN_P8_1__GPIO                  ALIF_PINMUX(8, 1, 0)  /**< Port 8 Pin 1: GPIO */
#define PIN_P8_1__I2S2_SDI_A            ALIF_PINMUX(8, 1, 1)  /**< Port 8 Pin 1: I2S2_SDI_A */
#define PIN_P8_1__FAULT1_B              ALIF_PINMUX(8, 1, 2)  /**< Port 8 Pin 1: FAULT1_B */
#define PIN_P8_1__LPCAM_D1_A            ALIF_PINMUX(8, 1, 3)  /**< Port 8 Pin 1: LPCAM_D1_A */
#define PIN_P8_1__SD_D1_C               ALIF_PINMUX(8, 1, 4)  /**< Port 8 Pin 1: SD_D1_C */
#define PIN_P8_1__CDC_D1_A              ALIF_PINMUX(8, 1, 5)  /**< Port 8 Pin 1: CDC_D1_A */
#define PIN_P8_1__CAM_D1_B              ALIF_PINMUX(8, 1, 6)  /**< Port 8 Pin 1: CAM_D1_B */
#define PIN_P8_1__OSPI1_SS1_C           ALIF_PINMUX(8, 1, 7)  /**< Port 8 Pin 1: OSPI1_SS1_C */

/** Pin P8.2 alternate functions */
#define PIN_P8_2__GPIO                  ALIF_PINMUX(8, 2, 0)  /**< Port 8 Pin 2: GPIO */
#define PIN_P8_2__I2S2_SDO_A            ALIF_PINMUX(8, 2, 1)  /**< Port 8 Pin 2: I2S2_SDO_A */
#define PIN_P8_2__SPI0_SS3_B            ALIF_PINMUX(8, 2, 2)  /**< Port 8 Pin 2: SPI0_SS3_B */
#define PIN_P8_2__FAULT2_B              ALIF_PINMUX(8, 2, 3)  /**< Port 8 Pin 2: FAULT2_B */
#define PIN_P8_2__LPCAM_D2_A            ALIF_PINMUX(8, 2, 4)  /**< Port 8 Pin 2: LPCAM_D2_A */
#define PIN_P8_2__SD_D2_C               ALIF_PINMUX(8, 2, 5)  /**< Port 8 Pin 2: SD_D2_C */
#define PIN_P8_2__CDC_D2_A              ALIF_PINMUX(8, 2, 6)  /**< Port 8 Pin 2: CDC_D2_A */
#define PIN_P8_2__CAM_D2_B              ALIF_PINMUX(8, 2, 7)  /**< Port 8 Pin 2: CAM_D2_B */

/** Pin P8.3 alternate functions */
#define PIN_P8_3__GPIO                  ALIF_PINMUX(8, 3, 0)  /**< Port 8 Pin 3: GPIO */
#define PIN_P8_3__I2S2_SCLK_A           ALIF_PINMUX(8, 3, 1)  /**< Port 8 Pin 3: I2S2_SCLK_A */
#define PIN_P8_3__SPI1_MISO_B           ALIF_PINMUX(8, 3, 2)  /**< Port 8 Pin 3: SPI1_MISO_B */
#define PIN_P8_3__FAULT3_B              ALIF_PINMUX(8, 3, 3)  /**< Port 8 Pin 3: FAULT3_B */
#define PIN_P8_3__LPCAM_D3_A            ALIF_PINMUX(8, 3, 4)  /**< Port 8 Pin 3: LPCAM_D3_A */
#define PIN_P8_3__SD_D3_C               ALIF_PINMUX(8, 3, 5)  /**< Port 8 Pin 3: SD_D3_C */
#define PIN_P8_3__CDC_D3_A              ALIF_PINMUX(8, 3, 6)  /**< Port 8 Pin 3: CDC_D3_A */
#define PIN_P8_3__CAM_D3_B              ALIF_PINMUX(8, 3, 7)  /**< Port 8 Pin 3: CAM_D3_B */

/** Pin P8.4 alternate functions */
#define PIN_P8_4__GPIO                  ALIF_PINMUX(8, 4, 0)  /**< Port 8 Pin 4: GPIO */
#define PIN_P8_4__I2S2_WS_A             ALIF_PINMUX(8, 4, 1)  /**< Port 8 Pin 4: I2S2_WS_A */
#define PIN_P8_4__SPI1_MOSI_B           ALIF_PINMUX(8, 4, 2)  /**< Port 8 Pin 4: SPI1_MOSI_B */
#define PIN_P8_4__QEC0_X_B              ALIF_PINMUX(8, 4, 3)  /**< Port 8 Pin 4: QEC0_X_B */
#define PIN_P8_4__LPCAM_D4_A            ALIF_PINMUX(8, 4, 4)  /**< Port 8 Pin 4: LPCAM_D4_A */
#define PIN_P8_4__SD_D4_C               ALIF_PINMUX(8, 4, 5)  /**< Port 8 Pin 4: SD_D4_C */
#define PIN_P8_4__CDC_D4_A              ALIF_PINMUX(8, 4, 6)  /**< Port 8 Pin 4: CDC_D4_A */
#define PIN_P8_4__CAM_D4_B              ALIF_PINMUX(8, 4, 7)  /**< Port 8 Pin 4: CAM_D4_B */

/** Pin P8.5 alternate functions */
#define PIN_P8_5__GPIO                  ALIF_PINMUX(8, 5, 0)  /**< Port 8 Pin 5: GPIO */
#define PIN_P8_5__OSPI0_RXDS1_A         ALIF_PINMUX(8, 5, 1)  /**< Port 8 Pin 5: OSPI0_RXDS1_A */
#define PIN_P8_5__SPI1_SCLK_B           ALIF_PINMUX(8, 5, 2)  /**< Port 8 Pin 5: SPI1_SCLK_B */
#define PIN_P8_5__QEC0_Y_B              ALIF_PINMUX(8, 5, 3)  /**< Port 8 Pin 5: QEC0_Y_B */
#define PIN_P8_5__LPCAM_D5_A            ALIF_PINMUX(8, 5, 4)  /**< Port 8 Pin 5: LPCAM_D5_A */
#define PIN_P8_5__SD_D5_C               ALIF_PINMUX(8, 5, 5)  /**< Port 8 Pin 5: SD_D5_C */
#define PIN_P8_5__CDC_D5_A              ALIF_PINMUX(8, 5, 6)  /**< Port 8 Pin 5: CDC_D5_A */
#define PIN_P8_5__CAM_D5_B              ALIF_PINMUX(8, 5, 7)  /**< Port 8 Pin 5: CAM_D5_B */

/** Pin P8.6 alternate functions */
#define PIN_P8_6__GPIO                  ALIF_PINMUX(8, 6, 0)  /**< Port 8 Pin 6: GPIO */
#define PIN_P8_6__OSPI1_RXDS1_B         ALIF_PINMUX(8, 6, 1)  /**< Port 8 Pin 6: OSPI1_RXDS1_B */
#define PIN_P8_6__I2S3_SCLK_B           ALIF_PINMUX(8, 6, 2)  /**< Port 8 Pin 6: I2S3_SCLK_B */
#define PIN_P8_6__QEC0_Z_B              ALIF_PINMUX(8, 6, 3)  /**< Port 8 Pin 6: QEC0_Z_B */
#define PIN_P8_6__LPCAM_D6_A            ALIF_PINMUX(8, 6, 4)  /**< Port 8 Pin 6: LPCAM_D6_A */
#define PIN_P8_6__SD_D6_C               ALIF_PINMUX(8, 6, 5)  /**< Port 8 Pin 6: SD_D6_C */
#define PIN_P8_6__CDC_D6_A              ALIF_PINMUX(8, 6, 6)  /**< Port 8 Pin 6: CDC_D6_A */
#define PIN_P8_6__CAM_D6_B              ALIF_PINMUX(8, 6, 7)  /**< Port 8 Pin 6: CAM_D6_B */

/** Pin P8.7 alternate functions */
#define PIN_P8_7__GPIO                  ALIF_PINMUX(8, 7, 0)  /**< Port 8 Pin 7: GPIO */
#define PIN_P8_7__OSPI0_RXDS1_C         ALIF_PINMUX(8, 7, 1)  /**< Port 8 Pin 7: OSPI0_RXDS1_C */
#define PIN_P8_7__I2S3_WS_B             ALIF_PINMUX(8, 7, 2)  /**< Port 8 Pin 7: I2S3_WS_B */
#define PIN_P8_7__QEC1_X_B              ALIF_PINMUX(8, 7, 3)  /**< Port 8 Pin 7: QEC1_X_B */
#define PIN_P8_7__LPCAM_D7_A            ALIF_PINMUX(8, 7, 4)  /**< Port 8 Pin 7: LPCAM_D7_A */
#define PIN_P8_7__SD_D7_C               ALIF_PINMUX(8, 7, 5)  /**< Port 8 Pin 7: SD_D7_C */
#define PIN_P8_7__CDC_D7_A              ALIF_PINMUX(8, 7, 6)  /**< Port 8 Pin 7: CDC_D7_A */
#define PIN_P8_7__CAM_D7_B              ALIF_PINMUX(8, 7, 7)  /**< Port 8 Pin 7: CAM_D7_B */

/** @} */

/**
 * @name Port 9 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 9.
 * @{
 */

/** Pin P9.0 alternate functions */
#define PIN_P9_0__GPIO                  ALIF_PINMUX(9, 0, 0)  /**< Port 9 Pin 0: GPIO */
#define PIN_P9_0__OSPI1_RXDS1_C         ALIF_PINMUX(9, 0, 1)  /**< Port 9 Pin 0: OSPI1_RXDS1_C */
#define PIN_P9_0__I2S3_SDI_B            ALIF_PINMUX(9, 0, 2)  /**< Port 9 Pin 0: I2S3_SDI_B */
#define PIN_P9_0__QEC1_Y_B              ALIF_PINMUX(9, 0, 3)  /**< Port 9 Pin 0: QEC1_Y_B */
#define PIN_P9_0__SD_CMD_C              ALIF_PINMUX(9, 0, 4)  /**< Port 9 Pin 0: SD_CMD_C */
#define PIN_P9_0__CDC_D8_A              ALIF_PINMUX(9, 0, 5)  /**< Port 9 Pin 0: CDC_D8_A */
#define PIN_P9_0__CAM_D8_B              ALIF_PINMUX(9, 0, 6)  /**< Port 9 Pin 0: CAM_D8_B */
#define PIN_P9_0__LPSPI_SS3_A           ALIF_PINMUX(9, 0, 7)  /**< Port 9 Pin 0: LPSPI_SS3_A */

/** Pin P9.1 alternate functions */
#define PIN_P9_1__GPIO                  ALIF_PINMUX(9, 1, 0)  /**< Port 9 Pin 1: GPIO */
#define PIN_P9_1__LPUART_RX_B           ALIF_PINMUX(9, 1, 1)  /**< Port 9 Pin 1: LPUART_RX_B */
#define PIN_P9_1__I2S3_SDO_B            ALIF_PINMUX(9, 1, 2)  /**< Port 9 Pin 1: I2S3_SDO_B */
#define PIN_P9_1__QEC1_Z_B              ALIF_PINMUX(9, 1, 3)  /**< Port 9 Pin 1: QEC1_Z_B */
#define PIN_P9_1__SD_CLK_C              ALIF_PINMUX(9, 1, 4)  /**< Port 9 Pin 1: SD_CLK_C */
#define PIN_P9_1__CDC_D9_A              ALIF_PINMUX(9, 1, 5)  /**< Port 9 Pin 1: CDC_D9_A */
#define PIN_P9_1__CAM_D9_B              ALIF_PINMUX(9, 1, 6)  /**< Port 9 Pin 1: CAM_D9_B */
#define PIN_P9_1__LPSPI_SS4_A           ALIF_PINMUX(9, 1, 7)  /**< Port 9 Pin 1: LPSPI_SS4_A */

/** Pin P9.2 alternate functions */
#define PIN_P9_2__GPIO                  ALIF_PINMUX(9, 2, 0)  /**< Port 9 Pin 2: GPIO */
#define PIN_P9_2__LPUART_TX_B           ALIF_PINMUX(9, 2, 1)  /**< Port 9 Pin 2: LPUART_TX_B */
#define PIN_P9_2__I2S3_SDI_A            ALIF_PINMUX(9, 2, 2)  /**< Port 9 Pin 2: I2S3_SDI_A */
#define PIN_P9_2__SPI2_MISO_B           ALIF_PINMUX(9, 2, 3)  /**< Port 9 Pin 2: SPI2_MISO_B */
#define PIN_P9_2__QEC2_X_B              ALIF_PINMUX(9, 2, 4)  /**< Port 9 Pin 2: QEC2_X_B */
#define PIN_P9_2__SD_RST_C              ALIF_PINMUX(9, 2, 5)  /**< Port 9 Pin 2: SD_RST_C */
#define PIN_P9_2__CDC_D10_A             ALIF_PINMUX(9, 2, 6)  /**< Port 9 Pin 2: CDC_D10_A */
#define PIN_P9_2__CAM_D10_B             ALIF_PINMUX(9, 2, 7)  /**< Port 9 Pin 2: CAM_D10_B */

/** Pin P9.3 alternate functions */
#define PIN_P9_3__GPIO                  ALIF_PINMUX(9, 3, 0)  /**< Port 9 Pin 3: GPIO */
#define PIN_P9_3__HFXO_OUT_B            ALIF_PINMUX(9, 3, 1)  /**< Port 9 Pin 3: HFXO_OUT_B */
#define PIN_P9_3__UART7_RX_B            ALIF_PINMUX(9, 3, 2)  /**< Port 9 Pin 3: UART7_RX_B */
#define PIN_P9_3__I2S3_SDO_A            ALIF_PINMUX(9, 3, 3)  /**< Port 9 Pin 3: I2S3_SDO_A */
#define PIN_P9_3__SPI2_MOSI_B           ALIF_PINMUX(9, 3, 4)  /**< Port 9 Pin 3: SPI2_MOSI_B */
#define PIN_P9_3__QEC2_Y_B              ALIF_PINMUX(9, 3, 5)  /**< Port 9 Pin 3: QEC2_Y_B */
#define PIN_P9_3__CDC_D11_A             ALIF_PINMUX(9, 3, 6)  /**< Port 9 Pin 3: CDC_D11_A */
#define PIN_P9_3__CAM_D11_B             ALIF_PINMUX(9, 3, 7)  /**< Port 9 Pin 3: CAM_D11_B */

/** Pin P9.4 alternate functions */
#define PIN_P9_4__GPIO                  ALIF_PINMUX(9, 4, 0)  /**< Port 9 Pin 4: GPIO */
#define PIN_P9_4__UART7_TX_B            ALIF_PINMUX(9, 4, 1)  /**< Port 9 Pin 4: UART7_TX_B */
#define PIN_P9_4__I2S3_SCLK_A           ALIF_PINMUX(9, 4, 2)  /**< Port 9 Pin 4: I2S3_SCLK_A */
#define PIN_P9_4__SPI2_SCLK_B           ALIF_PINMUX(9, 4, 3)  /**< Port 9 Pin 4: SPI2_SCLK_B */
#define PIN_P9_4__I2C3_SDA_C            ALIF_PINMUX(9, 4, 4)  /**< Port 9 Pin 4: I2C3_SDA_C */
#define PIN_P9_4__QEC2_Z_B              ALIF_PINMUX(9, 4, 5)  /**< Port 9 Pin 4: QEC2_Z_B */
#define PIN_P9_4__CDC_D12_A             ALIF_PINMUX(9, 4, 6)  /**< Port 9 Pin 4: CDC_D12_A */
#define PIN_P9_4__CAM_D12_B             ALIF_PINMUX(9, 4, 7)  /**< Port 9 Pin 4: CAM_D12_B */

/** Pin P9.5 alternate functions */
#define PIN_P9_5__GPIO                  ALIF_PINMUX(9, 5, 0)  /**< Port 9 Pin 5: GPIO */
#define PIN_P9_5__OSPI1_D0_C            ALIF_PINMUX(9, 5, 1)  /**< Port 9 Pin 5: OSPI1_D0_C */
#define PIN_P9_5__I2S3_WS_A             ALIF_PINMUX(9, 5, 2)  /**< Port 9 Pin 5: I2S3_WS_A */
#define PIN_P9_5__SPI2_SS0_B            ALIF_PINMUX(9, 5, 3)  /**< Port 9 Pin 5: SPI2_SS0_B */
#define PIN_P9_5__I2C3_SCL_C            ALIF_PINMUX(9, 5, 4)  /**< Port 9 Pin 5: I2C3_SCL_C */
#define PIN_P9_5__QEC3_X_B              ALIF_PINMUX(9, 5, 5)  /**< Port 9 Pin 5: QEC3_X_B */
#define PIN_P9_5__CDC_D13_A             ALIF_PINMUX(9, 5, 6)  /**< Port 9 Pin 5: CDC_D13_A */
#define PIN_P9_5__CAM_D13_B             ALIF_PINMUX(9, 5, 7)  /**< Port 9 Pin 5: CAM_D13_B */

/** Pin P9.6 alternate functions */
#define PIN_P9_6__GPIO                  ALIF_PINMUX(9, 6, 0)  /**< Port 9 Pin 6: GPIO */
#define PIN_P9_6__OSPI1_D1_C            ALIF_PINMUX(9, 6, 1)  /**< Port 9 Pin 6: OSPI1_D1_C */
#define PIN_P9_6__AUDIO_CLK_B           ALIF_PINMUX(9, 6, 2)  /**< Port 9 Pin 6: AUDIO_CLK_B */
#define PIN_P9_6__SPI2_SS1_B            ALIF_PINMUX(9, 6, 3)  /**< Port 9 Pin 6: SPI2_SS1_B */
#define PIN_P9_6__I2C3_SDA_B            ALIF_PINMUX(9, 6, 4)  /**< Port 9 Pin 6: I2C3_SDA_B */
#define PIN_P9_6__QEC3_Y_B              ALIF_PINMUX(9, 6, 5)  /**< Port 9 Pin 6: QEC3_Y_B */
#define PIN_P9_6__CDC_D14_A             ALIF_PINMUX(9, 6, 6)  /**< Port 9 Pin 6: CDC_D14_A */
#define PIN_P9_6__CAM_D14_B             ALIF_PINMUX(9, 6, 7)  /**< Port 9 Pin 6: CAM_D14_B */

/** Pin P9.7 alternate functions */
#define PIN_P9_7__GPIO                  ALIF_PINMUX(9, 7, 0)  /**< Port 9 Pin 7: GPIO */
#define PIN_P9_7__OSPI1_D2_C            ALIF_PINMUX(9, 7, 1)  /**< Port 9 Pin 7: OSPI1_D2_C */
#define PIN_P9_7__UART7_DE_B            ALIF_PINMUX(9, 7, 2)  /**< Port 9 Pin 7: UART7_DE_B */
#define PIN_P9_7__SPI2_SS2_B            ALIF_PINMUX(9, 7, 3)  /**< Port 9 Pin 7: SPI2_SS2_B */
#define PIN_P9_7__I2C3_SCL_B            ALIF_PINMUX(9, 7, 4)  /**< Port 9 Pin 7: I2C3_SCL_B */
#define PIN_P9_7__QEC3_Z_B              ALIF_PINMUX(9, 7, 5)  /**< Port 9 Pin 7: QEC3_Z_B */
#define PIN_P9_7__CDC_D15_A             ALIF_PINMUX(9, 7, 6)  /**< Port 9 Pin 7: CDC_D15_A */
#define PIN_P9_7__CAM_D15_B             ALIF_PINMUX(9, 7, 7)  /**< Port 9 Pin 7: CAM_D15_B */

/** @} */

/**
 * @name Port 10 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 10.
 * @{
 */

/** Pin P10.0 alternate functions */
#define PIN_P10_0__GPIO                 ALIF_PINMUX(10, 0, 0)  /**< Port 10 Pin 0: GPIO */
#define PIN_P10_0__OSPI1_D3_C           ALIF_PINMUX(10, 0, 1)  /**< Port 10 Pin 0: OSPI1_D3_C */
#define PIN_P10_0__UART6_DE_B           ALIF_PINMUX(10, 0, 2)  /**< Port 10 Pin 0: UART6_DE_B */
#define PIN_P10_0__SPI2_SS3_B           ALIF_PINMUX(10, 0, 3)  /**< Port 10 Pin 0: SPI2_SS3_B */
#define PIN_P10_0__UT0_T0_C             ALIF_PINMUX(10, 0, 4)  /**< Port 10 Pin 0: UT0_T0_C */
#define PIN_P10_0__LPCAM_HSYNC_A        ALIF_PINMUX(10, 0, 5)  /**< Port 10 Pin 0: LPCAM_HSYNC_A */
#define PIN_P10_0__CDC_D16_A            ALIF_PINMUX(10, 0, 6)  /**< Port 10 Pin 0: CDC_D16_A */
#define PIN_P10_0__CAM_HSYNC_B          ALIF_PINMUX(10, 0, 7)  /**< Port 10 Pin 0: CAM_HSYNC_B */

/** Pin P10.1 alternate functions */
#define PIN_P10_1__GPIO                 ALIF_PINMUX(10, 1, 0)  /**< Port 10 Pin 1: GPIO */
#define PIN_P10_1__OSPI1_D4_C           ALIF_PINMUX(10, 1, 1)  /**< Port 10 Pin 1: OSPI1_D4_C */
#define PIN_P10_1__HSUART0_RX_A         ALIF_PINMUX(10, 1, 2)  /**< Port 10 Pin 1: HSUART0_RX_A */
#define PIN_P10_1__LPI2S_SDI_B          ALIF_PINMUX(10, 1, 3)  /**< Port 10 Pin 1: LPI2S_SDI_B */
#define PIN_P10_1__UT0_T1_C             ALIF_PINMUX(10, 1, 4)  /**< Port 10 Pin 1: UT0_T1_C */
#define PIN_P10_1__LPCAM_VSYNC_A        ALIF_PINMUX(10, 1, 5)  /**< Port 10 Pin 1: LPCAM_VSYNC_A */
#define PIN_P10_1__CDC_D17_A            ALIF_PINMUX(10, 1, 6)  /**< Port 10 Pin 1: CDC_D17_A */
#define PIN_P10_1__CAM_VSYNC_B          ALIF_PINMUX(10, 1, 7)  /**< Port 10 Pin 1: CAM_VSYNC_B */

/** Pin P10.2 alternate functions */
#define PIN_P10_2__GPIO                 ALIF_PINMUX(10, 2, 0)  /**< Port 10 Pin 2: GPIO */
#define PIN_P10_2__OSPI1_D5_C           ALIF_PINMUX(10, 2, 1)  /**< Port 10 Pin 2: OSPI1_D5_C */
#define PIN_P10_2__HSUART0_TX_A         ALIF_PINMUX(10, 2, 2)  /**< Port 10 Pin 2: HSUART0_TX_A */
#define PIN_P10_2__LPI2S_SDO_B          ALIF_PINMUX(10, 2, 3)  /**< Port 10 Pin 2: LPI2S_SDO_B */
#define PIN_P10_2__UT1_T0_C             ALIF_PINMUX(10, 2, 4)  /**< Port 10 Pin 2: UT1_T0_C */
#define PIN_P10_2__LPCAM_PCLK_A         ALIF_PINMUX(10, 2, 5)  /**< Port 10 Pin 2: LPCAM_PCLK_A */
#define PIN_P10_2__CDC_D18_A            ALIF_PINMUX(10, 2, 6)  /**< Port 10 Pin 2: CDC_D18_A */
#define PIN_P10_2__CAM_PCLK_B           ALIF_PINMUX(10, 2, 7)  /**< Port 10 Pin 2: CAM_PCLK_B */

/** Pin P10.3 alternate functions */
#define PIN_P10_3__GPIO                 ALIF_PINMUX(10, 3, 0)  /**< Port 10 Pin 3: GPIO */
#define PIN_P10_3__OSPI1_D6_C           ALIF_PINMUX(10, 3, 1)  /**< Port 10 Pin 3: OSPI1_D6_C */
#define PIN_P10_3__HSUART1_RX_A         ALIF_PINMUX(10, 3, 2)  /**< Port 10 Pin 3: HSUART1_RX_A */
#define PIN_P10_3__LPI2S_SCLK_B         ALIF_PINMUX(10, 3, 3)  /**< Port 10 Pin 3: LPI2S_SCLK_B */
#define PIN_P10_3__UT1_T1_C             ALIF_PINMUX(10, 3, 4)  /**< Port 10 Pin 3: UT1_T1_C */
#define PIN_P10_3__LPCAM_XVCLK_A        ALIF_PINMUX(10, 3, 5)  /**< Port 10 Pin 3: LPCAM_XVCLK_A */
#define PIN_P10_3__CDC_D19_A            ALIF_PINMUX(10, 3, 6)  /**< Port 10 Pin 3: CDC_D19_A */
#define PIN_P10_3__CAM_XVCLK_B          ALIF_PINMUX(10, 3, 7)  /**< Port 10 Pin 3: CAM_XVCLK_B */

/** Pin P10.4 alternate functions */
#define PIN_P10_4__GPIO                 ALIF_PINMUX(10, 4, 0)  /**< Port 10 Pin 4: GPIO */
#define PIN_P10_4__OSPI1_D7_C           ALIF_PINMUX(10, 4, 1)  /**< Port 10 Pin 4: OSPI1_D7_C */
#define PIN_P10_4__HSUART1_TX_A         ALIF_PINMUX(10, 4, 2)  /**< Port 10 Pin 4: HSUART1_TX_A */
#define PIN_P10_4__LPI2S_WS_B           ALIF_PINMUX(10, 4, 3)  /**< Port 10 Pin 4: LPI2S_WS_B */
#define PIN_P10_4__I2C0_SDA_D           ALIF_PINMUX(10, 4, 4)  /**< Port 10 Pin 4: I2C0_SDA_D */
#define PIN_P10_4__UT2_T0_C             ALIF_PINMUX(10, 4, 5)  /**< Port 10 Pin 4: UT2_T0_C */
#define PIN_P10_4__ETH_TXD0_B           ALIF_PINMUX(10, 4, 6)  /**< Port 10 Pin 4: ETH_TXD0_B */
#define PIN_P10_4__CDC_D20_A            ALIF_PINMUX(10, 4, 7)  /**< Port 10 Pin 4: CDC_D20_A */

/** Pin P10.5 alternate functions */
#define PIN_P10_5__GPIO                 ALIF_PINMUX(10, 5, 0)  /**< Port 10 Pin 5: GPIO */
#define PIN_P10_5__UART6_RX_A           ALIF_PINMUX(10, 5, 1)  /**< Port 10 Pin 5: UART6_RX_A */
#define PIN_P10_5__I2S2_SDI_B           ALIF_PINMUX(10, 5, 2)  /**< Port 10 Pin 5: I2S2_SDI_B */
#define PIN_P10_5__SPI3_MISO_B          ALIF_PINMUX(10, 5, 3)  /**< Port 10 Pin 5: SPI3_MISO_B */
#define PIN_P10_5__I2C0_SCL_D           ALIF_PINMUX(10, 5, 4)  /**< Port 10 Pin 5: I2C0_SCL_D */
#define PIN_P10_5__UT2_T1_C             ALIF_PINMUX(10, 5, 5)  /**< Port 10 Pin 5: UT2_T1_C */
#define PIN_P10_5__ETH_TXD1_B           ALIF_PINMUX(10, 5, 6)  /**< Port 10 Pin 5: ETH_TXD1_B */
#define PIN_P10_5__CDC_D21_A            ALIF_PINMUX(10, 5, 7)  /**< Port 10 Pin 5: CDC_D21_A */

/** Pin P10.6 alternate functions */
#define PIN_P10_6__GPIO                 ALIF_PINMUX(10, 6, 0)  /**< Port 10 Pin 6: GPIO */
#define PIN_P10_6__UART6_TX_A           ALIF_PINMUX(10, 6, 1)  /**< Port 10 Pin 6: UART6_TX_A */
#define PIN_P10_6__I2S2_SDO_B           ALIF_PINMUX(10, 6, 2)  /**< Port 10 Pin 6: I2S2_SDO_B */
#define PIN_P10_6__SPI3_MOSI_B          ALIF_PINMUX(10, 6, 3)  /**< Port 10 Pin 6: SPI3_MOSI_B */
#define PIN_P10_6__I2C1_SDA_D           ALIF_PINMUX(10, 6, 4)  /**< Port 10 Pin 6: I2C1_SDA_D */
#define PIN_P10_6__UT3_T0_C             ALIF_PINMUX(10, 6, 5)  /**< Port 10 Pin 6: UT3_T0_C */
#define PIN_P10_6__ETH_TXEN_B           ALIF_PINMUX(10, 6, 6)  /**< Port 10 Pin 6: ETH_TXEN_B */
#define PIN_P10_6__CDC_D22_A            ALIF_PINMUX(10, 6, 7)  /**< Port 10 Pin 6: CDC_D22_A */

/** Pin P10.7 alternate functions */
#define PIN_P10_7__GPIO                 ALIF_PINMUX(10, 7, 0)  /**< Port 10 Pin 7: GPIO */
#define PIN_P10_7__UART7_RX_A           ALIF_PINMUX(10, 7, 1)  /**< Port 10 Pin 7: UART7_RX_A */
#define PIN_P10_7__I2S2_SCLK_B          ALIF_PINMUX(10, 7, 2)  /**< Port 10 Pin 7: I2S2_SCLK_B */
#define PIN_P10_7__SPI3_SCLK_B          ALIF_PINMUX(10, 7, 3)  /**< Port 10 Pin 7: SPI3_SCLK_B */
#define PIN_P10_7__I2C1_SCL_D           ALIF_PINMUX(10, 7, 4)  /**< Port 10 Pin 7: I2C1_SCL_D */
#define PIN_P10_7__UT3_T1_C             ALIF_PINMUX(10, 7, 5)  /**< Port 10 Pin 7: UT3_T1_C */
#define PIN_P10_7__CDC_D23_A            ALIF_PINMUX(10, 7, 6)  /**< Port 10 Pin 7: CDC_D23_A */
#define PIN_P10_7__OSPI1_RXDS0_C        ALIF_PINMUX(10, 7, 7)  /**< Port 10 Pin 7: OSPI1_RXDS0_C */

/** @} */

/**
 * @name Port 11 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 11.
 * @{
 */

/** Pin P11.0 alternate functions */
#define PIN_P11_0__GPIO                 ALIF_PINMUX(11, 0, 0)  /**< Port 11 Pin 0: GPIO */
#define PIN_P11_0__OSPI1_D0_A           ALIF_PINMUX(11, 0, 1)  /**< Port 11 Pin 0: OSPI1_D0_A */
#define PIN_P11_0__UART7_TX_A           ALIF_PINMUX(11, 0, 2)  /**< Port 11 Pin 0: UART7_TX_A */
#define PIN_P11_0__I2S2_WS_B            ALIF_PINMUX(11, 0, 3)  /**< Port 11 Pin 0: I2S2_WS_B */
#define PIN_P11_0__SPI3_SS0_B           ALIF_PINMUX(11, 0, 4)  /**< Port 11 Pin 0: SPI3_SS0_B */
#define PIN_P11_0__UT4_T0_C             ALIF_PINMUX(11, 0, 5)  /**< Port 11 Pin 0: UT4_T0_C */
#define PIN_P11_0__ETH_REFCLK_B         ALIF_PINMUX(11, 0, 6)  /**< Port 11 Pin 0: ETH_REFCLK_B */
#define PIN_P11_0__CDC_D0_B             ALIF_PINMUX(11, 0, 7)  /**< Port 11 Pin 0: CDC_D0_B */

/** Pin P11.1 alternate functions */
#define PIN_P11_1__GPIO                 ALIF_PINMUX(11, 1, 0)  /**< Port 11 Pin 1: GPIO */
#define PIN_P11_1__OSPI1_D1_A           ALIF_PINMUX(11, 1, 1)  /**< Port 11 Pin 1: OSPI1_D1_A */
#define PIN_P11_1__UART7_DE_A           ALIF_PINMUX(11, 1, 2)  /**< Port 11 Pin 1: UART7_DE_A */
#define PIN_P11_1__SPI3_SS1_B           ALIF_PINMUX(11, 1, 3)  /**< Port 11 Pin 1: SPI3_SS1_B */
#define PIN_P11_1__UT4_T1_C             ALIF_PINMUX(11, 1, 4)  /**< Port 11 Pin 1: UT4_T1_C */
#define PIN_P11_1__ETH_MDIO_B           ALIF_PINMUX(11, 1, 5)  /**< Port 11 Pin 1: ETH_MDIO_B */
#define PIN_P11_1__CDC_D1_B             ALIF_PINMUX(11, 1, 6)  /**< Port 11 Pin 1: CDC_D1_B */
#define PIN_P11_1__LPSPI_SS5_A          ALIF_PINMUX(11, 1, 7)  /**< Port 11 Pin 1: LPSPI_SS5_A */

/** Pin P11.2 alternate functions */
#define PIN_P11_2__GPIO                 ALIF_PINMUX(11, 2, 0)  /**< Port 11 Pin 2: GPIO */
#define PIN_P11_2__OSPI1_D2_A           ALIF_PINMUX(11, 2, 1)  /**< Port 11 Pin 2: OSPI1_D2_A */
#define PIN_P11_2__UART6_DE_A           ALIF_PINMUX(11, 2, 2)  /**< Port 11 Pin 2: UART6_DE_A */
#define PIN_P11_2__LPPDM_C2_B           ALIF_PINMUX(11, 2, 3)  /**< Port 11 Pin 2: LPPDM_C2_B */
#define PIN_P11_2__SPI3_SS2_B           ALIF_PINMUX(11, 2, 4)  /**< Port 11 Pin 2: SPI3_SS2_B */
#define PIN_P11_2__UT5_T0_C             ALIF_PINMUX(11, 2, 5)  /**< Port 11 Pin 2: UT5_T0_C */
#define PIN_P11_2__ETH_MDC_B            ALIF_PINMUX(11, 2, 6)  /**< Port 11 Pin 2: ETH_MDC_B */
#define PIN_P11_2__CDC_D2_B             ALIF_PINMUX(11, 2, 7)  /**< Port 11 Pin 2: CDC_D2_B */

/** Pin P11.3 alternate functions */
#define PIN_P11_3__GPIO                 ALIF_PINMUX(11, 3, 0)  /**< Port 11 Pin 3: GPIO */
#define PIN_P11_3__OSPI1_D3_A           ALIF_PINMUX(11, 3, 1)  /**< Port 11 Pin 3: OSPI1_D3_A */
#define PIN_P11_3__UART5_RX_B           ALIF_PINMUX(11, 3, 2)  /**< Port 11 Pin 3: UART5_RX_B */
#define PIN_P11_3__LPPDM_C3_B           ALIF_PINMUX(11, 3, 3)  /**< Port 11 Pin 3: LPPDM_C3_B */
#define PIN_P11_3__SPI3_SS3_B           ALIF_PINMUX(11, 3, 4)  /**< Port 11 Pin 3: SPI3_SS3_B */
#define PIN_P11_3__UT5_T1_C             ALIF_PINMUX(11, 3, 5)  /**< Port 11 Pin 3: UT5_T1_C */
#define PIN_P11_3__ETH_RXD0_B           ALIF_PINMUX(11, 3, 6)  /**< Port 11 Pin 3: ETH_RXD0_B */
#define PIN_P11_3__CDC_D3_B             ALIF_PINMUX(11, 3, 7)  /**< Port 11 Pin 3: CDC_D3_B */

/** Pin P11.4 alternate functions */
#define PIN_P11_4__GPIO                 ALIF_PINMUX(11, 4, 0)  /**< Port 11 Pin 4: GPIO */
#define PIN_P11_4__OSPI1_D4_A           ALIF_PINMUX(11, 4, 1)  /**< Port 11 Pin 4: OSPI1_D4_A */
#define PIN_P11_4__UART5_TX_B           ALIF_PINMUX(11, 4, 2)  /**< Port 11 Pin 4: UART5_TX_B */
#define PIN_P11_4__PDM_C2_B             ALIF_PINMUX(11, 4, 3)  /**< Port 11 Pin 4: PDM_C2_B */
#define PIN_P11_4__LPSPI_MISO_B         ALIF_PINMUX(11, 4, 4)  /**< Port 11 Pin 4: LPSPI_MISO_B */
#define PIN_P11_4__UT6_T0_C             ALIF_PINMUX(11, 4, 5)  /**< Port 11 Pin 4: UT6_T0_C */
#define PIN_P11_4__ETH_RXD1_B           ALIF_PINMUX(11, 4, 6)  /**< Port 11 Pin 4: ETH_RXD1_B */
#define PIN_P11_4__CDC_D4_B             ALIF_PINMUX(11, 4, 7)  /**< Port 11 Pin 4: CDC_D4_B */

/** Pin P11.5 alternate functions */
#define PIN_P11_5__GPIO                 ALIF_PINMUX(11, 5, 0)  /**< Port 11 Pin 5: GPIO */
#define PIN_P11_5__OSPI1_D5_A           ALIF_PINMUX(11, 5, 1)  /**< Port 11 Pin 5: OSPI1_D5_A */
#define PIN_P11_5__UART6_RX_B           ALIF_PINMUX(11, 5, 2)  /**< Port 11 Pin 5: UART6_RX_B */
#define PIN_P11_5__PDM_C3_B             ALIF_PINMUX(11, 5, 3)  /**< Port 11 Pin 5: PDM_C3_B */
#define PIN_P11_5__LPSPI_MOSI_B         ALIF_PINMUX(11, 5, 4)  /**< Port 11 Pin 5: LPSPI_MOSI_B */
#define PIN_P11_5__UT6_T1_C             ALIF_PINMUX(11, 5, 5)  /**< Port 11 Pin 5: UT6_T1_C */
#define PIN_P11_5__ETH_CRS_DV_B         ALIF_PINMUX(11, 5, 6)  /**< Port 11 Pin 5: ETH_CRS_DV_B */
#define PIN_P11_5__CDC_D5_B             ALIF_PINMUX(11, 5, 7)  /**< Port 11 Pin 5: CDC_D5_B */

/** Pin P11.6 alternate functions */
#define PIN_P11_6__GPIO                 ALIF_PINMUX(11, 6, 0)  /**< Port 11 Pin 6: GPIO */
#define PIN_P11_6__OSPI1_D6_A           ALIF_PINMUX(11, 6, 1)  /**< Port 11 Pin 6: OSPI1_D6_A */
#define PIN_P11_6__UART6_TX_B           ALIF_PINMUX(11, 6, 2)  /**< Port 11 Pin 6: UART6_TX_B */
#define PIN_P11_6__LPPDM_D2_B           ALIF_PINMUX(11, 6, 3)  /**< Port 11 Pin 6: LPPDM_D2_B */
#define PIN_P11_6__LPSPI_SCLK_B         ALIF_PINMUX(11, 6, 4)  /**< Port 11 Pin 6: LPSPI_SCLK_B */
#define PIN_P11_6__UT7_T0_C             ALIF_PINMUX(11, 6, 5)  /**< Port 11 Pin 6: UT7_T0_C */
#define PIN_P11_6__ETH_RST_B            ALIF_PINMUX(11, 6, 6)  /**< Port 11 Pin 6: ETH_RST_B */
#define PIN_P11_6__CDC_D6_B             ALIF_PINMUX(11, 6, 7)  /**< Port 11 Pin 6: CDC_D6_B */

/** Pin P11.7 alternate functions */
#define PIN_P11_7__GPIO                 ALIF_PINMUX(11, 7, 0)  /**< Port 11 Pin 7: GPIO */
#define PIN_P11_7__OSPI1_D7_A           ALIF_PINMUX(11, 7, 1)  /**< Port 11 Pin 7: OSPI1_D7_A */
#define PIN_P11_7__UART5_DE_B           ALIF_PINMUX(11, 7, 2)  /**< Port 11 Pin 7: UART5_DE_B */
#define PIN_P11_7__LPPDM_D3_B           ALIF_PINMUX(11, 7, 3)  /**< Port 11 Pin 7: LPPDM_D3_B */
#define PIN_P11_7__LPSPI_SS_B           ALIF_PINMUX(11, 7, 4)  /**< Port 11 Pin 7: LPSPI_SS_B */
#define PIN_P11_7__UT7_T1_C             ALIF_PINMUX(11, 7, 5)  /**< Port 11 Pin 7: UT7_T1_C */
#define PIN_P11_7__ETH_IRQ_B            ALIF_PINMUX(11, 7, 6)  /**< Port 11 Pin 7: ETH_IRQ_B */
#define PIN_P11_7__CDC_D7_B             ALIF_PINMUX(11, 7, 7)  /**< Port 11 Pin 7: CDC_D7_B */

/** @} */

/**
 * @name Port 12 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 12.
 * @{
 */

/** Pin P12.0 alternate functions */
#define PIN_P12_0__GPIO                 ALIF_PINMUX(12, 0, 0)  /**< Port 12 Pin 0: GPIO */
#define PIN_P12_0__OSPI0_SCLK_C         ALIF_PINMUX(12, 0, 1)  /**< Port 12 Pin 0: OSPI0_SCLK_C */
#define PIN_P12_0__AUDIO_CLK_C          ALIF_PINMUX(12, 0, 2)  /**< Port 12 Pin 0: AUDIO_CLK_C */
#define PIN_P12_0__I2S1_SDI_B           ALIF_PINMUX(12, 0, 3)  /**< Port 12 Pin 0: I2S1_SDI_B */
#define PIN_P12_0__UT8_T0_C             ALIF_PINMUX(12, 0, 4)  /**< Port 12 Pin 0: UT8_T0_C */
#define PIN_P12_0__CDC_D8_B             ALIF_PINMUX(12, 0, 5)  /**< Port 12 Pin 0: CDC_D8_B */
#define PIN_P12_0__OSPI1_D8_B           ALIF_PINMUX(12, 0, 6)  /**< Port 12 Pin 0: OSPI1_D8_B */
#define PIN_P12_0__LPUT0_T0_B           ALIF_PINMUX(12, 0, 7)  /**< Port 12 Pin 0: LPUT0_T0_B */

/** Pin P12.1 alternate functions */
#define PIN_P12_1__GPIO                 ALIF_PINMUX(12, 1, 0)  /**< Port 12 Pin 1: GPIO */
#define PIN_P12_1__OSPI0_SCLKN_C        ALIF_PINMUX(12, 1, 1)  /**< Port 12 Pin 1: OSPI0_SCLKN_C */
#define PIN_P12_1__UART4_RX_B           ALIF_PINMUX(12, 1, 2)  /**< Port 12 Pin 1: UART4_RX_B */
#define PIN_P12_1__I2S1_SDO_B           ALIF_PINMUX(12, 1, 3)  /**< Port 12 Pin 1: I2S1_SDO_B */
#define PIN_P12_1__UT8_T1_C             ALIF_PINMUX(12, 1, 4)  /**< Port 12 Pin 1: UT8_T1_C */
#define PIN_P12_1__CDC_D9_B             ALIF_PINMUX(12, 1, 5)  /**< Port 12 Pin 1: CDC_D9_B */
#define PIN_P12_1__OSPI1_D9_B           ALIF_PINMUX(12, 1, 6)  /**< Port 12 Pin 1: OSPI1_D9_B */
#define PIN_P12_1__LPUT0_T1_B           ALIF_PINMUX(12, 1, 7)  /**< Port 12 Pin 1: LPUT0_T1_B */

/** Pin P12.2 alternate functions */
#define PIN_P12_2__GPIO                 ALIF_PINMUX(12, 2, 0)  /**< Port 12 Pin 2: GPIO */
#define PIN_P12_2__OSPI0_RXDS_C         ALIF_PINMUX(12, 2, 1)  /**< Port 12 Pin 2: OSPI0_RXDS_C */
#define PIN_P12_2__UART4_TX_B           ALIF_PINMUX(12, 2, 2)  /**< Port 12 Pin 2: UART4_TX_B */
#define PIN_P12_2__I2S1_SCLK_B          ALIF_PINMUX(12, 2, 3)  /**< Port 12 Pin 2: I2S1_SCLK_B */
#define PIN_P12_2__UT9_T0_C             ALIF_PINMUX(12, 2, 4)  /**< Port 12 Pin 2: UT9_T0_C */
#define PIN_P12_2__CDC_D10_B            ALIF_PINMUX(12, 2, 5)  /**< Port 12 Pin 2: CDC_D10_B */
#define PIN_P12_2__OSPI1_D10_B          ALIF_PINMUX(12, 2, 6)  /**< Port 12 Pin 2: OSPI1_D10_B */
#define PIN_P12_2__LPUT1_T0_B           ALIF_PINMUX(12, 2, 7)  /**< Port 12 Pin 2: LPUT1_T0_B */

/** Pin P12.3 alternate functions */
#define PIN_P12_3__GPIO                 ALIF_PINMUX(12, 3, 0)  /**< Port 12 Pin 3: GPIO */
#define PIN_P12_3__OSPI0_SS0_C          ALIF_PINMUX(12, 3, 1)  /**< Port 12 Pin 3: OSPI0_SS0_C */
#define PIN_P12_3__UART4_DE_B           ALIF_PINMUX(12, 3, 2)  /**< Port 12 Pin 3: UART4_DE_B */
#define PIN_P12_3__I2S1_WS_B            ALIF_PINMUX(12, 3, 3)  /**< Port 12 Pin 3: I2S1_WS_B */
#define PIN_P12_3__UT9_T1_C             ALIF_PINMUX(12, 3, 4)  /**< Port 12 Pin 3: UT9_T1_C */
#define PIN_P12_3__CDC_D11_B            ALIF_PINMUX(12, 3, 5)  /**< Port 12 Pin 3: CDC_D11_B */
#define PIN_P12_3__OSPI1_D11_B          ALIF_PINMUX(12, 3, 6)  /**< Port 12 Pin 3: OSPI1_D11_B */
#define PIN_P12_3__LPUT1_T1_B           ALIF_PINMUX(12, 3, 7)  /**< Port 12 Pin 3: LPUT1_T1_B */

/** Pin P12.4 alternate functions */
#define PIN_P12_4__GPIO                 ALIF_PINMUX(12, 4, 0)  /**< Port 12 Pin 4: GPIO */
#define PIN_P12_4__OSPI0_SS1_C          ALIF_PINMUX(12, 4, 1)  /**< Port 12 Pin 4: OSPI0_SS1_C */
#define PIN_P12_4__SPI3_MISO_A          ALIF_PINMUX(12, 4, 2)  /**< Port 12 Pin 4: SPI3_MISO_A */
#define PIN_P12_4__UT10_T0_C            ALIF_PINMUX(12, 4, 3)  /**< Port 12 Pin 4: UT10_T0_C */
#define PIN_P12_4__CAN_RXD_C            ALIF_PINMUX(12, 4, 4)  /**< Port 12 Pin 4: CAN_RXD_C */
#define PIN_P12_4__CDC_D12_B            ALIF_PINMUX(12, 4, 5)  /**< Port 12 Pin 4: CDC_D12_B */
#define PIN_P12_4__OSPI1_D12_B          ALIF_PINMUX(12, 4, 6)  /**< Port 12 Pin 4: OSPI1_D12_B */
#define PIN_P12_4__LPUT2_T0_B           ALIF_PINMUX(12, 4, 7)  /**< Port 12 Pin 4: LPUT2_T0_B */

/** Pin P12.5 alternate functions */
#define PIN_P12_5__GPIO                 ALIF_PINMUX(12, 5, 0)  /**< Port 12 Pin 5: GPIO */
#define PIN_P12_5__HSUART0_RX_B         ALIF_PINMUX(12, 5, 1)  /**< Port 12 Pin 5: HSUART0_RX_B */
#define PIN_P12_5__SPI3_MOSI_A          ALIF_PINMUX(12, 5, 2)  /**< Port 12 Pin 5: SPI3_MOSI_A */
#define PIN_P12_5__UT10_T1_C            ALIF_PINMUX(12, 5, 3)  /**< Port 12 Pin 5: UT10_T1_C */
#define PIN_P12_5__CAN_TXD_C            ALIF_PINMUX(12, 5, 4)  /**< Port 12 Pin 5: CAN_TXD_C */
#define PIN_P12_5__CDC_D13_B            ALIF_PINMUX(12, 5, 5)  /**< Port 12 Pin 5: CDC_D13_B */
#define PIN_P12_5__OSPI1_D13_B          ALIF_PINMUX(12, 5, 6)  /**< Port 12 Pin 5: OSPI1_D13_B */
#define PIN_P12_5__LPUT2_T1_B           ALIF_PINMUX(12, 5, 7)  /**< Port 12 Pin 5: LPUT2_T1_B */

/** Pin P12.6 alternate functions */
#define PIN_P12_6__GPIO                 ALIF_PINMUX(12, 6, 0)  /**< Port 12 Pin 6: GPIO */
#define PIN_P12_6__HSUART0_TX_B         ALIF_PINMUX(12, 6, 1)  /**< Port 12 Pin 6: HSUART0_TX_B */
#define PIN_P12_6__SPI3_SCLK_A          ALIF_PINMUX(12, 6, 2)  /**< Port 12 Pin 6: SPI3_SCLK_A */
#define PIN_P12_6__UT11_T0_C            ALIF_PINMUX(12, 6, 3)  /**< Port 12 Pin 6: UT11_T0_C */
#define PIN_P12_6__CAN_STBY_C           ALIF_PINMUX(12, 6, 4)  /**< Port 12 Pin 6: CAN_STBY_C */
#define PIN_P12_6__CDC_D14_B            ALIF_PINMUX(12, 6, 5)  /**< Port 12 Pin 6: CDC_D14_B */
#define PIN_P12_6__OSPI1_D14_B          ALIF_PINMUX(12, 6, 6)  /**< Port 12 Pin 6: OSPI1_D14_B */

/** Pin P12.7 alternate functions */
#define PIN_P12_7__GPIO                 ALIF_PINMUX(12, 7, 0)  /**< Port 12 Pin 7: GPIO */
#define PIN_P12_7__OSPI1_RXDS0_B        ALIF_PINMUX(12, 7, 1)  /**< Port 12 Pin 7: OSPI1_RXDS0_B */
#define PIN_P12_7__HSUART1_RX_B         ALIF_PINMUX(12, 7, 2)  /**< Port 12 Pin 7: HSUART1_RX_B */
#define PIN_P12_7__SPI3_SS0_A           ALIF_PINMUX(12, 7, 3)  /**< Port 12 Pin 7: SPI3_SS0_A */
#define PIN_P12_7__UT11_T1_C            ALIF_PINMUX(12, 7, 4)  /**< Port 12 Pin 7: UT11_T1_C */
#define PIN_P12_7__CDC_D15_B            ALIF_PINMUX(12, 7, 5)  /**< Port 12 Pin 7: CDC_D15_B */

/** @} */

/**
 * @name Port 13 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 13.
 * @{
 */

/** Pin P13.0 alternate functions */
#define PIN_P13_0__GPIO                 ALIF_PINMUX(13, 0, 0)  /**< Port 13 Pin 0: GPIO */
#define PIN_P13_0__OSPI1_D0_B           ALIF_PINMUX(13, 0, 1)  /**< Port 13 Pin 0: OSPI1_D0_B */
#define PIN_P13_0__HSUART1_TX_B         ALIF_PINMUX(13, 0, 2)  /**< Port 13 Pin 0: HSUART1_TX_B */
#define PIN_P13_0__SPI3_SS1_A           ALIF_PINMUX(13, 0, 3)  /**< Port 13 Pin 0: SPI3_SS1_A */
#define PIN_P13_0__QEC0_X_C             ALIF_PINMUX(13, 0, 4)  /**< Port 13 Pin 0: QEC0_X_C */
#define PIN_P13_0__SD_D0_B              ALIF_PINMUX(13, 0, 5)  /**< Port 13 Pin 0: SD_D0_B */
#define PIN_P13_0__CDC_D16_B            ALIF_PINMUX(13, 0, 6)  /**< Port 13 Pin 0: CDC_D16_B */
#define PIN_P13_0__OSPI0_D8_A           ALIF_PINMUX(13, 0, 7)  /**< Port 13 Pin 0: OSPI0_D8_A */

/** Pin P13.1 alternate functions */
#define PIN_P13_1__GPIO                 ALIF_PINMUX(13, 1, 0)  /**< Port 13 Pin 1: GPIO */
#define PIN_P13_1__OSPI1_D1_B           ALIF_PINMUX(13, 1, 1)  /**< Port 13 Pin 1: OSPI1_D1_B */
#define PIN_P13_1__SPI3_SS2_A           ALIF_PINMUX(13, 1, 2)  /**< Port 13 Pin 1: SPI3_SS2_A */
#define PIN_P13_1__QEC0_Y_C             ALIF_PINMUX(13, 1, 3)  /**< Port 13 Pin 1: QEC0_Y_C */
#define PIN_P13_1__SD_D1_B              ALIF_PINMUX(13, 1, 4)  /**< Port 13 Pin 1: SD_D1_B */
#define PIN_P13_1__CDC_D17_B            ALIF_PINMUX(13, 1, 5)  /**< Port 13 Pin 1: CDC_D17_B */
#define PIN_P13_1__OSPI0_D9_A           ALIF_PINMUX(13, 1, 6)  /**< Port 13 Pin 1: OSPI0_D9_A */

/** Pin P13.2 alternate functions */
#define PIN_P13_2__GPIO                 ALIF_PINMUX(13, 2, 0)  /**< Port 13 Pin 2: GPIO */
#define PIN_P13_2__OSPI1_D2_B           ALIF_PINMUX(13, 2, 1)  /**< Port 13 Pin 2: OSPI1_D2_B */
#define PIN_P13_2__SPI3_SS3_A           ALIF_PINMUX(13, 2, 2)  /**< Port 13 Pin 2: SPI3_SS3_A */
#define PIN_P13_2__QEC0_Z_C             ALIF_PINMUX(13, 2, 3)  /**< Port 13 Pin 2: QEC0_Z_C */
#define PIN_P13_2__SD_D2_B              ALIF_PINMUX(13, 2, 4)  /**< Port 13 Pin 2: SD_D2_B */
#define PIN_P13_2__CDC_D18_B            ALIF_PINMUX(13, 2, 5)  /**< Port 13 Pin 2: CDC_D18_B */
#define PIN_P13_2__OSPI0_D10_A          ALIF_PINMUX(13, 2, 6)  /**< Port 13 Pin 2: OSPI0_D10_A */

/** Pin P13.3 alternate functions */
#define PIN_P13_3__GPIO                 ALIF_PINMUX(13, 3, 0)  /**< Port 13 Pin 3: GPIO */
#define PIN_P13_3__OSPI1_D3_B           ALIF_PINMUX(13, 3, 1)  /**< Port 13 Pin 3: OSPI1_D3_B */
#define PIN_P13_3__SPI2_SS3_A           ALIF_PINMUX(13, 3, 2)  /**< Port 13 Pin 3: SPI2_SS3_A */
#define PIN_P13_3__QEC1_X_C             ALIF_PINMUX(13, 3, 3)  /**< Port 13 Pin 3: QEC1_X_C */
#define PIN_P13_3__SD_D3_B              ALIF_PINMUX(13, 3, 4)  /**< Port 13 Pin 3: SD_D3_B */
#define PIN_P13_3__CDC_D19_B            ALIF_PINMUX(13, 3, 5)  /**< Port 13 Pin 3: CDC_D19_B */
#define PIN_P13_3__OSPI0_D11_A          ALIF_PINMUX(13, 3, 6)  /**< Port 13 Pin 3: OSPI0_D11_A */

/** Pin P13.4 alternate functions */
#define PIN_P13_4__GPIO                 ALIF_PINMUX(13, 4, 0)  /**< Port 13 Pin 4: GPIO */
#define PIN_P13_4__OSPI1_D4_B           ALIF_PINMUX(13, 4, 1)  /**< Port 13 Pin 4: OSPI1_D4_B */
#define PIN_P13_4__LPI2S_SDI_C          ALIF_PINMUX(13, 4, 2)  /**< Port 13 Pin 4: LPI2S_SDI_C */
#define PIN_P13_4__QEC1_Y_C             ALIF_PINMUX(13, 4, 3)  /**< Port 13 Pin 4: QEC1_Y_C */
#define PIN_P13_4__SD_D4_B              ALIF_PINMUX(13, 4, 4)  /**< Port 13 Pin 4: SD_D4_B */
#define PIN_P13_4__CDC_D20_B            ALIF_PINMUX(13, 4, 5)  /**< Port 13 Pin 4: CDC_D20_B */
#define PIN_P13_4__OSPI0_D12_A          ALIF_PINMUX(13, 4, 6)  /**< Port 13 Pin 4: OSPI0_D12_A */

/** Pin P13.5 alternate functions */
#define PIN_P13_5__GPIO                 ALIF_PINMUX(13, 5, 0)  /**< Port 13 Pin 5: GPIO */
#define PIN_P13_5__OSPI1_D5_B           ALIF_PINMUX(13, 5, 1)  /**< Port 13 Pin 5: OSPI1_D5_B */
#define PIN_P13_5__LPI2S_SDO_C          ALIF_PINMUX(13, 5, 2)  /**< Port 13 Pin 5: LPI2S_SDO_C */
#define PIN_P13_5__QEC1_Z_C             ALIF_PINMUX(13, 5, 3)  /**< Port 13 Pin 5: QEC1_Z_C */
#define PIN_P13_5__SD_D5_B              ALIF_PINMUX(13, 5, 4)  /**< Port 13 Pin 5: SD_D5_B */
#define PIN_P13_5__CDC_D21_B            ALIF_PINMUX(13, 5, 5)  /**< Port 13 Pin 5: CDC_D21_B */
#define PIN_P13_5__OSPI0_D13_A          ALIF_PINMUX(13, 5, 6)  /**< Port 13 Pin 5: OSPI0_D13_A */

/** Pin P13.6 alternate functions */
#define PIN_P13_6__GPIO                 ALIF_PINMUX(13, 6, 0)  /**< Port 13 Pin 6: GPIO */
#define PIN_P13_6__OSPI1_D6_B           ALIF_PINMUX(13, 6, 1)  /**< Port 13 Pin 6: OSPI1_D6_B */
#define PIN_P13_6__LPI2S_SCLK_C         ALIF_PINMUX(13, 6, 2)  /**< Port 13 Pin 6: LPI2S_SCLK_C */
#define PIN_P13_6__QEC2_X_C             ALIF_PINMUX(13, 6, 3)  /**< Port 13 Pin 6: QEC2_X_C */
#define PIN_P13_6__SD_D6_B              ALIF_PINMUX(13, 6, 4)  /**< Port 13 Pin 6: SD_D6_B */
#define PIN_P13_6__CDC_D22_B            ALIF_PINMUX(13, 6, 5)  /**< Port 13 Pin 6: CDC_D22_B */
#define PIN_P13_6__OSPI0_D14_A          ALIF_PINMUX(13, 6, 6)  /**< Port 13 Pin 6: OSPI0_D14_A */

/** Pin P13.7 alternate functions */
#define PIN_P13_7__GPIO                 ALIF_PINMUX(13, 7, 0)  /**< Port 13 Pin 7: GPIO */
#define PIN_P13_7__OSPI1_D7_B           ALIF_PINMUX(13, 7, 1)  /**< Port 13 Pin 7: OSPI1_D7_B */
#define PIN_P13_7__LPI2S_WS_C           ALIF_PINMUX(13, 7, 2)  /**< Port 13 Pin 7: LPI2S_WS_C */
#define PIN_P13_7__QEC2_Y_C             ALIF_PINMUX(13, 7, 3)  /**< Port 13 Pin 7: QEC2_Y_C */
#define PIN_P13_7__SD_D7_B              ALIF_PINMUX(13, 7, 4)  /**< Port 13 Pin 7: SD_D7_B */
#define PIN_P13_7__CDC_D23_B            ALIF_PINMUX(13, 7, 5)  /**< Port 13 Pin 7: CDC_D23_B */
#define PIN_P13_7__OSPI0_D15_A          ALIF_PINMUX(13, 7, 6)  /**< Port 13 Pin 7: OSPI0_D15_A */

/** @} */

/**
 * @name Port 14 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 14.
 * @{
 */

/** Pin P14.0 alternate functions */
#define PIN_P14_0__GPIO                 ALIF_PINMUX(14, 0, 0)  /**< Port 14 Pin 0: GPIO */
#define PIN_P14_0__OSPI1_SCLK_B         ALIF_PINMUX(14, 0, 1)  /**< Port 14 Pin 0: OSPI1_SCLK_B */
#define PIN_P14_0__UART6_RX_C           ALIF_PINMUX(14, 0, 2)  /**< Port 14 Pin 0: UART6_RX_C */
#define PIN_P14_0__QEC2_Z_C             ALIF_PINMUX(14, 0, 3)  /**< Port 14 Pin 0: QEC2_Z_C */
#define PIN_P14_0__SD_CMD_B             ALIF_PINMUX(14, 0, 4)  /**< Port 14 Pin 0: SD_CMD_B */
#define PIN_P14_0__LFRC_OUT_B           ALIF_PINMUX(14, 0, 5)  /**< Port 14 Pin 0: LFRC_OUT_B */

/** Pin P14.1 alternate functions */
#define PIN_P14_1__GPIO                 ALIF_PINMUX(14, 1, 0)  /**< Port 14 Pin 1: GPIO */
#define PIN_P14_1__OSPI1_SCLKN_B        ALIF_PINMUX(14, 1, 1)  /**< Port 14 Pin 1: OSPI1_SCLKN_B */
#define PIN_P14_1__UART6_TX_C           ALIF_PINMUX(14, 1, 2)  /**< Port 14 Pin 1: UART6_TX_C */
#define PIN_P14_1__APLL_OUT_C           ALIF_PINMUX(14, 1, 3)  /**< Port 14 Pin 1: APLL_OUT_C */
#define PIN_P14_1__QEC3_X_C             ALIF_PINMUX(14, 1, 4)  /**< Port 14 Pin 1: QEC3_X_C */
#define PIN_P14_1__SD_CLK_B             ALIF_PINMUX(14, 1, 5)  /**< Port 14 Pin 1: SD_CLK_B */
#define PIN_P14_1__LFXO_OUT_B           ALIF_PINMUX(14, 1, 6)  /**< Port 14 Pin 1: LFXO_OUT_B */

/** Pin P14.2 alternate functions */
#define PIN_P14_2__GPIO                 ALIF_PINMUX(14, 2, 0)  /**< Port 14 Pin 2: GPIO */
#define PIN_P14_2__OSPI1_SS0_B          ALIF_PINMUX(14, 2, 1)  /**< Port 14 Pin 2: OSPI1_SS0_B */
#define PIN_P14_2__UART7_RX_C           ALIF_PINMUX(14, 2, 2)  /**< Port 14 Pin 2: UART7_RX_C */
#define PIN_P14_2__LPI3C_SDA_C          ALIF_PINMUX(14, 2, 3)  /**< Port 14 Pin 2: LPI3C_SDA_C */
#define PIN_P14_2__QEC3_Y_C             ALIF_PINMUX(14, 2, 4)  /**< Port 14 Pin 2: QEC3_Y_C */
#define PIN_P14_2__SD_RST_B             ALIF_PINMUX(14, 2, 5)  /**< Port 14 Pin 2: SD_RST_B */
#define PIN_P14_2__LPSPI_SS5_B          ALIF_PINMUX(14, 2, 6)  /**< Port 14 Pin 2: LPSPI_SS5_B */

/** Pin P14.3 alternate functions */
#define PIN_P14_3__GPIO                 ALIF_PINMUX(14, 3, 0)  /**< Port 14 Pin 3: GPIO */
#define PIN_P14_3__OSPI1_SS1_B          ALIF_PINMUX(14, 3, 1)  /**< Port 14 Pin 3: OSPI1_SS1_B */
#define PIN_P14_3__UART7_TX_C           ALIF_PINMUX(14, 3, 2)  /**< Port 14 Pin 3: UART7_TX_C */
#define PIN_P14_3__LPI3C_SCL_C          ALIF_PINMUX(14, 3, 3)  /**< Port 14 Pin 3: LPI3C_SCL_C */
#define PIN_P14_3__QEC3_Z_C             ALIF_PINMUX(14, 3, 4)  /**< Port 14 Pin 3: QEC3_Z_C */

/** Pin P14.4 alternate functions */
#define PIN_P14_4__GPIO                 ALIF_PINMUX(14, 4, 0)  /**< Port 14 Pin 4: GPIO */
#define PIN_P14_4__CMP3_OUT_B           ALIF_PINMUX(14, 4, 1)  /**< Port 14 Pin 4: CMP3_OUT_B */
#define PIN_P14_4__SPI1_MISO_C          ALIF_PINMUX(14, 4, 2)  /**< Port 14 Pin 4: SPI1_MISO_C */
#define PIN_P14_4__FAULT0_C             ALIF_PINMUX(14, 4, 3)  /**< Port 14 Pin 4: FAULT0_C */
#define PIN_P14_4__OSPI1_RXDS1_A        ALIF_PINMUX(14, 4, 4)  /**< Port 14 Pin 4: OSPI1_RXDS1_A */
#define PIN_P14_4__LPSPI_SS1_B          ALIF_PINMUX(14, 4, 5)  /**< Port 14 Pin 4: LPSPI_SS1_B */

/** Pin P14.5 alternate functions */
#define PIN_P14_5__GPIO                 ALIF_PINMUX(14, 5, 0)  /**< Port 14 Pin 5: GPIO */
#define PIN_P14_5__CMP2_OUT_B           ALIF_PINMUX(14, 5, 1)  /**< Port 14 Pin 5: CMP2_OUT_B */
#define PIN_P14_5__SPI1_MOSI_C          ALIF_PINMUX(14, 5, 2)  /**< Port 14 Pin 5: SPI1_MOSI_C */
#define PIN_P14_5__FAULT1_C             ALIF_PINMUX(14, 5, 3)  /**< Port 14 Pin 5: FAULT1_C */
#define PIN_P14_5__OSPI0_RXDS1_B        ALIF_PINMUX(14, 5, 4)  /**< Port 14 Pin 5: OSPI0_RXDS1_B */
#define PIN_P14_5__LPSPI_SS2_B          ALIF_PINMUX(14, 5, 5)  /**< Port 14 Pin 5: LPSPI_SS2_B */

/** Pin P14.6 alternate functions */
#define PIN_P14_6__GPIO                 ALIF_PINMUX(14, 6, 0)  /**< Port 14 Pin 6: GPIO */
#define PIN_P14_6__CMP1_OUT_B           ALIF_PINMUX(14, 6, 1)  /**< Port 14 Pin 6: CMP1_OUT_B */
#define PIN_P14_6__SPI1_SCLK_C          ALIF_PINMUX(14, 6, 2)  /**< Port 14 Pin 6: SPI1_SCLK_C */
#define PIN_P14_6__FAULT2_C             ALIF_PINMUX(14, 6, 3)  /**< Port 14 Pin 6: FAULT2_C */
#define PIN_P14_6__OSPI1_D15_B          ALIF_PINMUX(14, 6, 4)  /**< Port 14 Pin 6: OSPI1_D15_B */
#define PIN_P14_6__LPSPI_SS3_B          ALIF_PINMUX(14, 6, 5)  /**< Port 14 Pin 6: LPSPI_SS3_B */

/** Pin P14.7 alternate functions */
#define PIN_P14_7__GPIO                 ALIF_PINMUX(14, 7, 0)  /**< Port 14 Pin 7: GPIO */
#define PIN_P14_7__CMP0_OUT_B           ALIF_PINMUX(14, 7, 1)  /**< Port 14 Pin 7: CMP0_OUT_B */
#define PIN_P14_7__SPI1_SS0_C           ALIF_PINMUX(14, 7, 2)  /**< Port 14 Pin 7: SPI1_SS0_C */
#define PIN_P14_7__FAULT3_C             ALIF_PINMUX(14, 7, 3)  /**< Port 14 Pin 7: FAULT3_C */
#define PIN_P14_7__APLL_OUT_B           ALIF_PINMUX(14, 7, 4)  /**< Port 14 Pin 7: APLL_OUT_B */
#define PIN_P14_7__LPSPI_SS4_B          ALIF_PINMUX(14, 7, 5)  /**< Port 14 Pin 7: LPSPI_SS4_B */

/** @} */

/**
 * @name Port 15 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 15.
 * @{
 */

/** Pin P15.0 alternate functions */
#define PIN_P15_0__LPGPIO               ALIF_PINMUX(15, 0, 0)  /**< Port 15 Pin 0: LPGPIO */

/** Pin P15.1 alternate functions */
#define PIN_P15_1__LPGPIO               ALIF_PINMUX(15, 1, 0)  /**< Port 15 Pin 1: LPGPIO */

/** Pin P15.2 alternate functions */
#define PIN_P15_2__LPGPIO               ALIF_PINMUX(15, 2, 0)  /**< Port 15 Pin 2: LPGPIO */

/** Pin P15.3 alternate functions */
#define PIN_P15_3__LPGPIO               ALIF_PINMUX(15, 3, 0)  /**< Port 15 Pin 3: LPGPIO */

/** Pin P15.4 alternate functions */
#define PIN_P15_4__LPGPIO               ALIF_PINMUX(15, 4, 0)  /**< Port 15 Pin 4: LPGPIO */

/** Pin P15.5 alternate functions */
#define PIN_P15_5__LPGPIO               ALIF_PINMUX(15, 5, 0)  /**< Port 15 Pin 5: LPGPIO */

/** Pin P15.6 alternate functions */
#define PIN_P15_6__LPGPIO               ALIF_PINMUX(15, 6, 0)  /**< Port 15 Pin 6: LPGPIO */

/** Pin P15.7 alternate functions */
#define PIN_P15_7__LPGPIO               ALIF_PINMUX(15, 7, 0)  /**< Port 15 Pin 7: LPGPIO */

/** @} */

/**
 * @name Port 16 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 16.
 * @{
 */

/** Pin P16.0 alternate functions */
#define PIN_P16_0__GPIO                 ALIF_PINMUX(16, 0, 0)  /**< Port 16 Pin 0: GPIO */
#define PIN_P16_0__OSPI0_D8_B           ALIF_PINMUX(16, 0, 1)  /**< Port 16 Pin 0: OSPI0_D8_B */

/** Pin P16.1 alternate functions */
#define PIN_P16_1__GPIO                 ALIF_PINMUX(16, 1, 0)  /**< Port 16 Pin 1: GPIO */
#define PIN_P16_1__OSPI0_D9_B           ALIF_PINMUX(16, 1, 1)  /**< Port 16 Pin 1: OSPI0_D9_B */

/** Pin P16.2 alternate functions */
#define PIN_P16_2__GPIO                 ALIF_PINMUX(16, 2, 0)  /**< Port 16 Pin 2: GPIO */
#define PIN_P16_2__OSPI0_D10_B          ALIF_PINMUX(16, 2, 1)  /**< Port 16 Pin 2: OSPI0_D10_B */

/** Pin P16.3 alternate functions */
#define PIN_P16_3__GPIO                 ALIF_PINMUX(16, 3, 0)  /**< Port 16 Pin 3: GPIO */
#define PIN_P16_3__OSPI0_D11_B          ALIF_PINMUX(16, 3, 1)  /**< Port 16 Pin 3: OSPI0_D11_B */

/** Pin P16.4 alternate functions */
#define PIN_P16_4__GPIO                 ALIF_PINMUX(16, 4, 0)  /**< Port 16 Pin 4: GPIO */
#define PIN_P16_4__OSPI0_D12_B          ALIF_PINMUX(16, 4, 1)  /**< Port 16 Pin 4: OSPI0_D12_B */

/** Pin P16.5 alternate functions */
#define PIN_P16_5__GPIO                 ALIF_PINMUX(16, 5, 0)  /**< Port 16 Pin 5: GPIO */
#define PIN_P16_5__OSPI0_D13_B          ALIF_PINMUX(16, 5, 1)  /**< Port 16 Pin 5: OSPI0_D13_B */

/** Pin P16.6 alternate functions */
#define PIN_P16_6__GPIO                 ALIF_PINMUX(16, 6, 0)  /**< Port 16 Pin 6: GPIO */
#define PIN_P16_6__OSPI0_D14_B          ALIF_PINMUX(16, 6, 1)  /**< Port 16 Pin 6: OSPI0_D14_B */

/** Pin P16.7 alternate functions */
#define PIN_P16_7__GPIO                 ALIF_PINMUX(16, 7, 0)  /**< Port 16 Pin 7: GPIO */
#define PIN_P16_7__OSPI0_D15_B          ALIF_PINMUX(16, 7, 1)  /**< Port 16 Pin 7: OSPI0_D15_B */

/** @} */

/**
 * @name Port 17 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 17.
 * @{
 */

/** Pin P17.0 alternate functions */
#define PIN_P17_0__GPIO                 ALIF_PINMUX(17, 0, 0)  /**< Port 17 Pin 0: GPIO */
#define PIN_P17_0__OSPI1_D8_C           ALIF_PINMUX(17, 0, 1)  /**< Port 17 Pin 0: OSPI1_D8_C */

/** Pin P17.1 alternate functions */
#define PIN_P17_1__GPIO                 ALIF_PINMUX(17, 1, 0)  /**< Port 17 Pin 1: GPIO */
#define PIN_P17_1__OSPI1_D9_C           ALIF_PINMUX(17, 1, 1)  /**< Port 17 Pin 1: OSPI1_D9_C */

/** Pin P17.2 alternate functions */
#define PIN_P17_2__GPIO                 ALIF_PINMUX(17, 2, 0)  /**< Port 17 Pin 2: GPIO */
#define PIN_P17_2__OSPI1_D10_C          ALIF_PINMUX(17, 2, 1)  /**< Port 17 Pin 2: OSPI1_D10_C */

/** Pin P17.3 alternate functions */
#define PIN_P17_3__GPIO                 ALIF_PINMUX(17, 3, 0)  /**< Port 17 Pin 3: GPIO */
#define PIN_P17_3__OSPI1_D11_C          ALIF_PINMUX(17, 3, 1)  /**< Port 17 Pin 3: OSPI1_D11_C */

/** Pin P17.4 alternate functions */
#define PIN_P17_4__GPIO                 ALIF_PINMUX(17, 4, 0)  /**< Port 17 Pin 4: GPIO */
#define PIN_P17_4__OSPI1_D12_C          ALIF_PINMUX(17, 4, 1)  /**< Port 17 Pin 4: OSPI1_D12_C */

/** Pin P17.5 alternate functions */
#define PIN_P17_5__GPIO                 ALIF_PINMUX(17, 5, 0)  /**< Port 17 Pin 5: GPIO */
#define PIN_P17_5__OSPI1_D13_C          ALIF_PINMUX(17, 5, 1)  /**< Port 17 Pin 5: OSPI1_D13_C */

/** Pin P17.6 alternate functions */
#define PIN_P17_6__GPIO                 ALIF_PINMUX(17, 6, 0)  /**< Port 17 Pin 6: GPIO */
#define PIN_P17_6__OSPI1_D14_C          ALIF_PINMUX(17, 6, 1)  /**< Port 17 Pin 6: OSPI1_D14_C */

/** Pin P17.7 alternate functions */
#define PIN_P17_7__GPIO                 ALIF_PINMUX(17, 7, 0)  /**< Port 17 Pin 7: GPIO */
#define PIN_P17_7__OSPI1_D15_C          ALIF_PINMUX(17, 7, 1)  /**< Port 17 Pin 7: OSPI1_D15_C */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ALIF_ENSEMBLE_PINCTRL_H_ */
