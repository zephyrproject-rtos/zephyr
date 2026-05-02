/*
 * SPDX-FileCopyrightText: Copyright Alif Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ALIF_ENSEMBLE_E1C_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ALIF_ENSEMBLE_E1C_PINCTRL_H_

/**
 * @file
 * @brief Pinctrl definitions for Alif Ensemble E1C series
 *
 * This header provides pin multiplexing definitions for the Alif Ensemble E1C series.
 * Each pin can be configured for up to 8 different functions (GPIO + 7 alternate functions).
 *
 * The pinmux encoding uses an 11-bit value:
 * - Bits [0:2]   - Alternate function (0 = GPIO, 1-7 = AF1-AF7)
 * - Bits [3:5]   - Pin number within port (0-7)
 * - Bits [6:10]  - Port number (0-8 for regular GPIO, 15 for LPGPIO)
 *
 * Pin naming convention: PIN_P{port}_{pin}__{function}
 * Example: PIN_P0_0__UART0_RX_A means Port 0, Pin 0, UART0 RX function (A group)
 *
 * Note: E1C has different hardware peripheral routing compared to E4/E6/E8 series.
 * The same AF value may route to different peripherals on E1C vs other Ensemble series.
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
#define PIN_P0_0__SEUART_RX_A           ALIF_PINMUX(0, 0, 1)  /**< Port 0 Pin 0: SEUART_RX_A */
#define PIN_P0_0__UART0_RX_A            ALIF_PINMUX(0, 0, 2)  /**< Port 0 Pin 0: UART0_RX_A */
#define PIN_P0_0__LPI2S_SDI_A           ALIF_PINMUX(0, 0, 3)  /**< Port 0 Pin 0: LPI2S_SDI_A */
#define PIN_P0_0__SPI0_SS0_A            ALIF_PINMUX(0, 0, 4)  /**< Port 0 Pin 0: SPI0_SS0_A */
#define PIN_P0_0__LPI2C0_SCL_A          ALIF_PINMUX(0, 0, 5)  /**< Port 0 Pin 0: LPI2C0_SCL_A */
#define PIN_P0_0__ANA_S0                ALIF_PINMUX(0, 0, 6)  /**< Port 0 Pin 0: ANA_S0 */

/** Pin P0.1 alternate functions */
#define PIN_P0_1__GPIO                  ALIF_PINMUX(0, 1, 0)  /**< Port 0 Pin 1: GPIO */
#define PIN_P0_1__SEUART_TX_A           ALIF_PINMUX(0, 1, 1)  /**< Port 0 Pin 1: SEUART_TX_A */
#define PIN_P0_1__UART0_TX_A            ALIF_PINMUX(0, 1, 2)  /**< Port 0 Pin 1: UART0_TX_A */
#define PIN_P0_1__LPI2S_SDO_A           ALIF_PINMUX(0, 1, 3)  /**< Port 0 Pin 1: LPI2S_SDO_A */
#define PIN_P0_1__SPI0_SS1_A            ALIF_PINMUX(0, 1, 4)  /**< Port 0 Pin 1: SPI0_SS1_A */
#define PIN_P0_1__LPI2C0_SDA_A          ALIF_PINMUX(0, 1, 5)  /**< Port 0 Pin 1: LPI2C0_SDA_A */
#define PIN_P0_1__ANA_S1                ALIF_PINMUX(0, 1, 6)  /**< Port 0 Pin 1: ANA_S1 */

/** Pin P0.2 alternate functions */
#define PIN_P0_2__GPIO                  ALIF_PINMUX(0, 2, 0)  /**< Port 0 Pin 2: GPIO */
#define PIN_P0_2__JTAG1_TDI             ALIF_PINMUX(0, 2, 1)  /**< Port 0 Pin 2: JTAG1_TDI */
#define PIN_P0_2__OSPI0_SS0_A           ALIF_PINMUX(0, 2, 2)  /**< Port 0 Pin 2: OSPI0_SS0_A */
#define PIN_P0_2__UART0_CTS_A           ALIF_PINMUX(0, 2, 3)  /**< Port 0 Pin 2: UART0_CTS_A */
#define PIN_P0_2__LPI2S_SCLK_A          ALIF_PINMUX(0, 2, 4)  /**< Port 0 Pin 2: LPI2S_SCLK_A */
#define PIN_P0_2__SPI0_SS2_A            ALIF_PINMUX(0, 2, 5)  /**< Port 0 Pin 2: SPI0_SS2_A */
#define PIN_P0_2__ANA_S2                ALIF_PINMUX(0, 2, 6)  /**< Port 0 Pin 2: ANA_S2 */

/** Pin P0.3 alternate functions */
#define PIN_P0_3__GPIO                  ALIF_PINMUX(0, 3, 0)  /**< Port 0 Pin 3: GPIO */
#define PIN_P0_3__JTAG1_TDO             ALIF_PINMUX(0, 3, 1)  /**< Port 0 Pin 3: JTAG1_TDO */
#define PIN_P0_3__OSPI0_SS1_A           ALIF_PINMUX(0, 3, 2)  /**< Port 0 Pin 3: OSPI0_SS1_A */
#define PIN_P0_3__UART0_RTS_A           ALIF_PINMUX(0, 3, 3)  /**< Port 0 Pin 3: UART0_RTS_A */
#define PIN_P0_3__LPI2S_WS_A            ALIF_PINMUX(0, 3, 4)  /**< Port 0 Pin 3: LPI2S_WS_A */
#define PIN_P0_3__SPI1_SS0_A            ALIF_PINMUX(0, 3, 5)  /**< Port 0 Pin 3: SPI1_SS0_A */
#define PIN_P0_3__ANA_S3                ALIF_PINMUX(0, 3, 6)  /**< Port 0 Pin 3: ANA_S3 */

/** Pin P0.4 alternate functions */
#define PIN_P0_4__GPIO                  ALIF_PINMUX(0, 4, 0)  /**< Port 0 Pin 4: GPIO */
#define PIN_P0_4__JTAG1_TRST            ALIF_PINMUX(0, 4, 1)  /**< Port 0 Pin 4: JTAG1_TRST */
#define PIN_P0_4__UART1_RX_A            ALIF_PINMUX(0, 4, 2)  /**< Port 0 Pin 4: UART1_RX_A */
#define PIN_P0_4__SPI1_SS1_A            ALIF_PINMUX(0, 4, 3)  /**< Port 0 Pin 4: SPI1_SS1_A */
#define PIN_P0_4__I2C0_SDA_A            ALIF_PINMUX(0, 4, 4)  /**< Port 0 Pin 4: I2C0_SDA_A */
#define PIN_P0_4__FAULT0_A              ALIF_PINMUX(0, 4, 5)  /**< Port 0 Pin 4: FAULT0_A */
#define PIN_P0_4__ANA_S4                ALIF_PINMUX(0, 4, 6)  /**< Port 0 Pin 4: ANA_S4 */

/** Pin P0.5 alternate functions */
#define PIN_P0_5__GPIO                  ALIF_PINMUX(0, 5, 0)  /**< Port 0 Pin 5: GPIO */
#define PIN_P0_5__JTAG1_TCK             ALIF_PINMUX(0, 5, 1)  /**< Port 0 Pin 5: JTAG1_TCK */
#define PIN_P0_5__UART1_TX_A            ALIF_PINMUX(0, 5, 2)  /**< Port 0 Pin 5: UART1_TX_A */
#define PIN_P0_5__SPI1_SS2_A            ALIF_PINMUX(0, 5, 3)  /**< Port 0 Pin 5: SPI1_SS2_A */
#define PIN_P0_5__I2C0_SCL_A            ALIF_PINMUX(0, 5, 4)  /**< Port 0 Pin 5: I2C0_SCL_A */
#define PIN_P0_5__I3C_SCL_A             ALIF_PINMUX(0, 5, 5)  /**< Port 0 Pin 5: I3C_SCL_A */
#define PIN_P0_5__ANA_S5                ALIF_PINMUX(0, 5, 6)  /**< Port 0 Pin 5: ANA_S5 */

/** Pin P0.6 alternate functions */
#define PIN_P0_6__GPIO                  ALIF_PINMUX(0, 6, 0)  /**< Port 0 Pin 6: GPIO */
#define PIN_P0_6__JTAG1_TMS             ALIF_PINMUX(0, 6, 1)  /**< Port 0 Pin 6: JTAG1_TMS */
#define PIN_P0_6__UART1_CTS_A           ALIF_PINMUX(0, 6, 2)  /**< Port 0 Pin 6: UART1_CTS_A */
#define PIN_P0_6__I2C1_SDA_A            ALIF_PINMUX(0, 6, 3)  /**< Port 0 Pin 6: I2C1_SDA_A */
#define PIN_P0_6__FAULT2_A              ALIF_PINMUX(0, 6, 4)  /**< Port 0 Pin 6: FAULT2_A */
#define PIN_P0_6__I3C_SDA_A             ALIF_PINMUX(0, 6, 5)  /**< Port 0 Pin 6: I3C_SDA_A */
#define PIN_P0_6__ANA_S6                ALIF_PINMUX(0, 6, 6)  /**< Port 0 Pin 6: ANA_S6 */

/** Pin P0.7 alternate functions */
#define PIN_P0_7__GPIO                  ALIF_PINMUX(0, 7, 0)  /**< Port 0 Pin 7: GPIO */
#define PIN_P0_7__UART1_RTS_A           ALIF_PINMUX(0, 7, 1)  /**< Port 0 Pin 7: UART1_RTS_A */
#define PIN_P0_7__I2C1_SCL_A            ALIF_PINMUX(0, 7, 2)  /**< Port 0 Pin 7: I2C1_SCL_A */
#define PIN_P0_7__FAULT3_A              ALIF_PINMUX(0, 7, 3)  /**< Port 0 Pin 7: FAULT3_A */
#define PIN_P0_7__LPCAM_VSYNC_B         ALIF_PINMUX(0, 7, 4)  /**< Port 0 Pin 7: LPCAM_VSYNC_B */
#define PIN_P0_7__ANA_S7                ALIF_PINMUX(0, 7, 6)  /**< Port 0 Pin 7: ANA_S7 */

/** @} */

/**
 * @name Port 1 Pin Definitions
 *
 * Pin multiplexing definitions for GPIO Port 1.
 * @{
 */

/** Pin P1.0 alternate functions */
#define PIN_P1_0__GPIO                  ALIF_PINMUX(1, 0, 0)  /**< Port 1 Pin 0: GPIO */
#define PIN_P1_0__OSPI0_D0_A            ALIF_PINMUX(1, 0, 1)  /**< Port 1 Pin 0: OSPI0_D0_A */
#define PIN_P1_0__UART2_RX_A            ALIF_PINMUX(1, 0, 2)  /**< Port 1 Pin 0: UART2_RX_A */
#define PIN_P1_0__LPPDM_D0_A            ALIF_PINMUX(1, 0, 3)  /**< Port 1 Pin 0: LPPDM_D0_A */
#define PIN_P1_0__LPI2S_SDI_B           ALIF_PINMUX(1, 0, 4)  /**< Port 1 Pin 0: LPI2S_SDI_B */
#define PIN_P1_0__SPI0_MISO_A           ALIF_PINMUX(1, 0, 5)  /**< Port 1 Pin 0: SPI0_MISO_A */
#define PIN_P1_0__LPCAM_D0_A            ALIF_PINMUX(1, 0, 6)  /**< Port 1 Pin 0: LPCAM_D0_A */
#define PIN_P1_0__SD_D0_A               ALIF_PINMUX(1, 0, 7)  /**< Port 1 Pin 0: SD_D0_A */

/** Pin P1.1 alternate functions */
#define PIN_P1_1__GPIO                  ALIF_PINMUX(1, 1, 0)  /**< Port 1 Pin 1: GPIO */
#define PIN_P1_1__OSPI0_D1_A            ALIF_PINMUX(1, 1, 1)  /**< Port 1 Pin 1: OSPI0_D1_A */
#define PIN_P1_1__UART2_TX_A            ALIF_PINMUX(1, 1, 2)  /**< Port 1 Pin 1: UART2_TX_A */
#define PIN_P1_1__LPPDM_C0_A            ALIF_PINMUX(1, 1, 3)  /**< Port 1 Pin 1: LPPDM_C0_A */
#define PIN_P1_1__LPI2S_SDO_B           ALIF_PINMUX(1, 1, 4)  /**< Port 1 Pin 1: LPI2S_SDO_B */
#define PIN_P1_1__SPI0_MOSI_A           ALIF_PINMUX(1, 1, 5)  /**< Port 1 Pin 1: SPI0_MOSI_A */
#define PIN_P1_1__LPCAM_D1_A            ALIF_PINMUX(1, 1, 6)  /**< Port 1 Pin 1: LPCAM_D1_A */
#define PIN_P1_1__SD_D1_A               ALIF_PINMUX(1, 1, 7)  /**< Port 1 Pin 1: SD_D1_A */

/** Pin P1.2 alternate functions */
#define PIN_P1_2__GPIO                  ALIF_PINMUX(1, 2, 0)  /**< Port 1 Pin 2: GPIO */
#define PIN_P1_2__OSPI0_D2_A            ALIF_PINMUX(1, 2, 1)  /**< Port 1 Pin 2: OSPI0_D2_A */
#define PIN_P1_2__LPPDM_D1_A            ALIF_PINMUX(1, 2, 2)  /**< Port 1 Pin 2: LPPDM_D1_A */
#define PIN_P1_2__LPI2S_SCLK_B          ALIF_PINMUX(1, 2, 3)  /**< Port 1 Pin 2: LPI2S_SCLK_B */
#define PIN_P1_2__SPI0_SCLK_A           ALIF_PINMUX(1, 2, 4)  /**< Port 1 Pin 2: SPI0_SCLK_A */
#define PIN_P1_2__LPCAM_D2_A            ALIF_PINMUX(1, 2, 5)  /**< Port 1 Pin 2: LPCAM_D2_A */
#define PIN_P1_2__SD_D2_A               ALIF_PINMUX(1, 2, 6)  /**< Port 1 Pin 2: SD_D2_A */
#define PIN_P1_2__CX_MWS_SF0            ALIF_PINMUX(1, 2, 7)  /**< Port 1 Pin 2: CX_MWS_SF0 */

/** Pin P1.3 alternate functions */
#define PIN_P1_3__GPIO                  ALIF_PINMUX(1, 3, 0)  /**< Port 1 Pin 3: GPIO */
#define PIN_P1_3__LPUART_RTS_A          ALIF_PINMUX(1, 3, 1)  /**< Port 1 Pin 3: LPUART_RTS_A */
#define PIN_P1_3__LPPDM_C1_A            ALIF_PINMUX(1, 3, 2)  /**< Port 1 Pin 3: LPPDM_C1_A */
#define PIN_P1_3__I2S1_SCLK_SLV_A       ALIF_PINMUX(1, 3, 3)  /**< Port 1 Pin 3: I2S1_SCLK_SLV_A */
#define PIN_P1_3__LPSPI_MISO_A          ALIF_PINMUX(1, 3, 4)  /**< Port 1 Pin 3: LPSPI_MISO_A */
#define PIN_P1_3__CAN1_RXD_A            ALIF_PINMUX(1, 3, 5)  /**< Port 1 Pin 3: CAN1_RXD_A */
#define PIN_P1_3__CX_MWS_SF0            ALIF_PINMUX(1, 3, 6)  /**< Port 1 Pin 3: CX_MWS_SF0 */

/** Pin P1.4 alternate functions */
#define PIN_P1_4__GPIO                  ALIF_PINMUX(1, 4, 0)  /**< Port 1 Pin 4: GPIO */
#define PIN_P1_4__LPPDM_C2_A            ALIF_PINMUX(1, 4, 1)  /**< Port 1 Pin 4: LPPDM_C2_A */
#define PIN_P1_4__I2S0_SDI_A            ALIF_PINMUX(1, 4, 2)  /**< Port 1 Pin 4: I2S0_SDI_A */
#define PIN_P1_4__LPSPI_MOSI_A          ALIF_PINMUX(1, 4, 3)  /**< Port 1 Pin 4: LPSPI_MOSI_A */
#define PIN_P1_4__I2C1_SDA_A            ALIF_PINMUX(1, 4, 4)  /**< Port 1 Pin 4: I2C1_SDA_A */
#define PIN_P1_4__CDC_PCLK_A            ALIF_PINMUX(1, 4, 5)  /**< Port 1 Pin 4: CDC_PCLK_A */
#define PIN_P1_4__CAN1_TXD_A            ALIF_PINMUX(1, 4, 6)  /**< Port 1 Pin 4: CAN1_TXD_A */
#define PIN_P1_4__CX_MWS_SF1            ALIF_PINMUX(1, 4, 7)  /**< Port 1 Pin 4: CX_MWS_SF1 */

/** Pin P1.5 alternate functions */
#define PIN_P1_5__GPIO                  ALIF_PINMUX(1, 5, 0)  /**< Port 1 Pin 5: GPIO */
#define PIN_P1_5__LPPDM_D2_A            ALIF_PINMUX(1, 5, 1)  /**< Port 1 Pin 5: LPPDM_D2_A */
#define PIN_P1_5__I2S0_SDO_A            ALIF_PINMUX(1, 5, 2)  /**< Port 1 Pin 5: I2S0_SDO_A */
#define PIN_P1_5__LPSPI_SCLK_A          ALIF_PINMUX(1, 5, 3)  /**< Port 1 Pin 5: LPSPI_SCLK_A */
#define PIN_P1_5__I2C1_SCLA_A           ALIF_PINMUX(1, 5, 4)  /**< Port 1 Pin 5: I2C1_SCLA_A */
#define PIN_P1_5__SD_RST_A              ALIF_PINMUX(1, 5, 5)  /**< Port 1 Pin 5: SD_RST_A */
#define PIN_P1_5__CDC_DE_A              ALIF_PINMUX(1, 5, 6)  /**< Port 1 Pin 5: CDC_DE_A */
#define PIN_P1_5__CAN1_STBY_A           ALIF_PINMUX(1, 5, 7)  /**< Port 1 Pin 5: CAN1_STBY_A */

/** Pin P1.6 alternate functions */
#define PIN_P1_6__GPIO                  ALIF_PINMUX(1, 6, 0)  /**< Port 1 Pin 6: GPIO */
#define PIN_P1_6__UART2_RX_A            ALIF_PINMUX(1, 6, 1)  /**< Port 1 Pin 6: UART2_RX_A */
#define PIN_P1_6__LPPDM_C3_A            ALIF_PINMUX(1, 6, 2)  /**< Port 1 Pin 6: LPPDM_C3_A */
#define PIN_P1_6__I2S0_SCLK_A           ALIF_PINMUX(1, 6, 3)  /**< Port 1 Pin 6: I2S0_SCLK_A */
#define PIN_P1_6__LPCAM_D6_A            ALIF_PINMUX(1, 6, 4)  /**< Port 1 Pin 6: LPCAM_D6_A */
#define PIN_P1_6__CDC_HSYNC_A           ALIF_PINMUX(1, 6, 5)  /**< Port 1 Pin 6: CDC_HSYNC_A */
#define PIN_P1_6__I3C_SDA_B             ALIF_PINMUX(1, 6, 6)  /**< Port 1 Pin 6: I3C_SDA_B */
#define PIN_P1_6__CX_MWS_SF2            ALIF_PINMUX(1, 6, 7)  /**< Port 1 Pin 6: CX_MWS_SF2 */

/** Pin P1.7 alternate functions */
#define PIN_P1_7__GPIO                  ALIF_PINMUX(1, 7, 0)  /**< Port 1 Pin 7: GPIO */
#define PIN_P1_7__UART2_TX_A            ALIF_PINMUX(1, 7, 1)  /**< Port 1 Pin 7: UART2_TX_A */
#define PIN_P1_7__LPPDM_D3_A            ALIF_PINMUX(1, 7, 2)  /**< Port 1 Pin 7: LPPDM_D3_A */
#define PIN_P1_7__I2S0_WS_A             ALIF_PINMUX(1, 7, 3)  /**< Port 1 Pin 7: I2S0_WS_A */
#define PIN_P1_7__LPCAM_D7_A            ALIF_PINMUX(1, 7, 4)  /**< Port 1 Pin 7: LPCAM_D7_A */
#define PIN_P1_7__CDC_VSYNC_A           ALIF_PINMUX(1, 7, 5)  /**< Port 1 Pin 7: CDC_VSYNC_A */
#define PIN_P1_7__I3C_SCL_B             ALIF_PINMUX(1, 7, 6)  /**< Port 1 Pin 7: I3C_SCL_B */
#define PIN_P1_7__CX_MWS_SF3            ALIF_PINMUX(1, 7, 7)  /**< Port 1 Pin 7: CX_MWS_SF3 */

/** Pin P2.0 alternate functions */
#define PIN_P2_0__GPIO                  ALIF_PINMUX(2, 0, 0)  /**< Port 2 Pin 0: GPIO */
#define PIN_P2_0__OSPI0_SS0_B           ALIF_PINMUX(2, 0, 1)  /**< Port 2 Pin 0: OSPI0_SS0_B */
#define PIN_P2_0__LPUART_RX_A           ALIF_PINMUX(2, 0, 2)  /**< Port 2 Pin 0: LPUART_RX_A */
#define PIN_P2_0__I2S1_SDI_A            ALIF_PINMUX(2, 0, 3)  /**< Port 2 Pin 0: I2S1_SDI_A */
#define PIN_P2_0__LPSPI_SS_A            ALIF_PINMUX(2, 0, 4)  /**< Port 2 Pin 0: LPSPI_SS_A */
#define PIN_P2_0__LPCAM_HSYNC_B         ALIF_PINMUX(2, 0, 5)  /**< Port 2 Pin 0: LPCAM_HSYNC_B */
#define PIN_P2_0__ANA_S8                ALIF_PINMUX(2, 0, 6)  /**< Port 2 Pin 0: ANA_S8 */
#define PIN_P2_0__CX_MWS_SF4            ALIF_PINMUX(2, 0, 7)  /**< Port 2 Pin 0: CX_MWS_SF4 */

/** Pin P2.1 alternate functions */
#define PIN_P2_1__GPIO                  ALIF_PINMUX(2, 1, 0)  /**< Port 2 Pin 1: GPIO */
#define PIN_P2_1__OSPI0_SS1_B           ALIF_PINMUX(2, 1, 1)  /**< Port 2 Pin 1: OSPI0_SS1_B */
#define PIN_P2_1__LPUART_TX_A           ALIF_PINMUX(2, 1, 2)  /**< Port 2 Pin 1: LPUART_TX_A */
#define PIN_P2_1__I2S1_SDO_A            ALIF_PINMUX(2, 1, 3)  /**< Port 2 Pin 1: I2S1_SDO_A */
#define PIN_P2_1__LPI2C0_SCL_B          ALIF_PINMUX(2, 1, 4)  /**< Port 2 Pin 1: LPI2C0_SCL_B */
#define PIN_P2_1__UT0_T1_A              ALIF_PINMUX(2, 1, 5)  /**< Port 2 Pin 1: UT0_T1_A */
#define PIN_P2_1__ANA_S9                ALIF_PINMUX(2, 1, 6)  /**< Port 2 Pin 1: ANA_S9 */
#define PIN_P2_1__CX_MWS_PAT0           ALIF_PINMUX(2, 1, 7)  /**< Port 2 Pin 1: CX_MWS_PAT0 */

/** Pin P2.2 alternate functions */
#define PIN_P2_2__GPIO                  ALIF_PINMUX(2, 2, 0)  /**< Port 2 Pin 2: GPIO */
#define PIN_P2_2__LPUART_RTS_A          ALIF_PINMUX(2, 2, 1)  /**< Port 2 Pin 2: LPUART_RTS_A */
#define PIN_P2_2__I2S1_SCLK_SLV_A       ALIF_PINMUX(2, 2, 2)  /**< Port 2 Pin 2: I2S1_SCLK_SLV_A */
#define PIN_P2_2__LPI2C0_SDA_B          ALIF_PINMUX(2, 2, 3)  /**< Port 2 Pin 2: LPI2C0_SDA_B */
#define PIN_P2_2__UT1_T0_A              ALIF_PINMUX(2, 2, 4)  /**< Port 2 Pin 2: UT1_T0_A */
#define PIN_P2_2__ANA_S10               ALIF_PINMUX(2, 2, 5)  /**< Port 2 Pin 2: ANA_S10 */
#define PIN_P2_2__CX_MWS_PAT1           ALIF_PINMUX(2, 2, 6)  /**< Port 2 Pin 2: CX_MWS_PAT1 */

/** Pin P2.3 alternate functions */
#define PIN_P2_3__GPIO                  ALIF_PINMUX(2, 3, 0)  /**< Port 2 Pin 3: GPIO */
#define PIN_P2_3__LPUART_CTS_A          ALIF_PINMUX(2, 3, 1)  /**< Port 2 Pin 3: LPUART_CTS_A */
#define PIN_P2_3__I2S1_WS_SLV_A         ALIF_PINMUX(2, 3, 2)  /**< Port 2 Pin 3: I2S1_WS_SLV_A */
#define PIN_P2_3__UT1_T1_A              ALIF_PINMUX(2, 3, 3)  /**< Port 2 Pin 3: UT1_T1_A */
#define PIN_P2_3__SD_RST_A              ALIF_PINMUX(2, 3, 4)  /**< Port 2 Pin 3: SD_RST_A */
#define PIN_P2_3__CAN0_RXD_A            ALIF_PINMUX(2, 3, 5)  /**< Port 2 Pin 3: CAN0_RXD_A */
#define PIN_P2_3__ANA_S11               ALIF_PINMUX(2, 3, 6)  /**< Port 2 Pin 3: ANA_S11 */
#define PIN_P2_3__CX_MWS_SYNC           ALIF_PINMUX(2, 3, 7)  /**< Port 2 Pin 3: CX_MWS_SYNC */

/** Pin P2.4 alternate functions */
#define PIN_P2_4__GPIO                  ALIF_PINMUX(2, 4, 0)  /**< Port 2 Pin 4: GPIO */
#define PIN_P2_4__UART3_RX_A            ALIF_PINMUX(2, 4, 1)  /**< Port 2 Pin 4: UART3_RX_A */
#define PIN_P2_4__I2S0_SDI_B            ALIF_PINMUX(2, 4, 2)  /**< Port 2 Pin 4: I2S0_SDI_B */
#define PIN_P2_4__I2C0_SCL_B            ALIF_PINMUX(2, 4, 3)  /**< Port 2 Pin 4: I2C0_SCL_B */
#define PIN_P2_4__UT2_T0_A              ALIF_PINMUX(2, 4, 4)  /**< Port 2 Pin 4: UT2_T0_A */
#define PIN_P2_4__CAN0_TXD_A            ALIF_PINMUX(2, 4, 5)  /**< Port 2 Pin 4: CAN0_TXD_A */
#define PIN_P2_4__ANA_S12               ALIF_PINMUX(2, 4, 6)  /**< Port 2 Pin 4: ANA_S12 */
#define PIN_P2_4__CX_MWS_TX             ALIF_PINMUX(2, 4, 7)  /**< Port 2 Pin 4: CX_MWS_TX */

/** Pin P2.5 alternate functions */
#define PIN_P2_5__GPIO                  ALIF_PINMUX(2, 5, 0)  /**< Port 2 Pin 5: GPIO */
#define PIN_P2_5__UART3_TX_A            ALIF_PINMUX(2, 5, 1)  /**< Port 2 Pin 5: UART3_TX_A */
#define PIN_P2_5__I2S0_SDO_B            ALIF_PINMUX(2, 5, 2)  /**< Port 2 Pin 5: I2S0_SDO_B */
#define PIN_P2_5__I2C0_SDA_B            ALIF_PINMUX(2, 5, 3)  /**< Port 2 Pin 5: I2C0_SDA_B */
#define PIN_P2_5__UT2_T1_A              ALIF_PINMUX(2, 5, 4)  /**< Port 2 Pin 5: UT2_T1_A */
#define PIN_P2_5__CAN0_STBY_A           ALIF_PINMUX(2, 5, 5)  /**< Port 2 Pin 5: CAN0_STBY_A */
#define PIN_P2_5__ANA_S13               ALIF_PINMUX(2, 5, 6)  /**< Port 2 Pin 5: ANA_S13 */
#define PIN_P2_5__CX_MWS_RX             ALIF_PINMUX(2, 5, 7)  /**< Port 2 Pin 5: CX_MWS_RX */

/** Pin P2.6 alternate functions */
#define PIN_P2_6__GPIO                  ALIF_PINMUX(2, 6, 0)  /**< Port 2 Pin 6: GPIO */
#define PIN_P2_6__UART4_RX_A            ALIF_PINMUX(2, 6, 1)  /**< Port 2 Pin 6: UART4_RX_A */
#define PIN_P2_6__I2S0_SCLK_B           ALIF_PINMUX(2, 6, 2)  /**< Port 2 Pin 6: I2S0_SCLK_B */
#define PIN_P2_6__I2C1_SDA_B            ALIF_PINMUX(2, 6, 3)  /**< Port 2 Pin 6: I2C1_SDA_B */
#define PIN_P2_6__UT3_T0_A              ALIF_PINMUX(2, 6, 4)  /**< Port 2 Pin 6: UT3_T0_A */
#define PIN_P2_6__ANA_S14               ALIF_PINMUX(2, 6, 6)  /**< Port 2 Pin 6: ANA_S14 */

/** Pin P2.7 alternate functions */
#define PIN_P2_7__GPIO                  ALIF_PINMUX(2, 7, 0)  /**< Port 2 Pin 7: GPIO */
#define PIN_P2_7__UART4_TX_A            ALIF_PINMUX(2, 7, 1)  /**< Port 2 Pin 7: UART4_TX_A */
#define PIN_P2_7__I2S0_WS_B             ALIF_PINMUX(2, 7, 2)  /**< Port 2 Pin 7: I2S0_WS_B */
#define PIN_P2_7__I2C1_SCL_B            ALIF_PINMUX(2, 7, 3)  /**< Port 2 Pin 7: I2C1_SCL_B */
#define PIN_P2_7__UT3_T1_A              ALIF_PINMUX(2, 7, 4)  /**< Port 2 Pin 7: UT3_T1_A */
#define PIN_P2_7__ANA_S15               ALIF_PINMUX(2, 7, 6)  /**< Port 2 Pin 7: ANA_S15 */

/** Pin P3.0 alternate functions */
#define PIN_P3_0__GPIO                  ALIF_PINMUX(3, 0, 0)  /**< Port 3 Pin 0: GPIO */
#define PIN_P3_0__OSPI0_D0_A            ALIF_PINMUX(3, 0, 1)  /**< Port 3 Pin 0: OSPI0_D0_A */
#define PIN_P3_0__UART5_RX_A            ALIF_PINMUX(3, 0, 2)  /**< Port 3 Pin 0: UART5_RX_A */
#define PIN_P3_0__LPPDM_C0_B            ALIF_PINMUX(3, 0, 3)  /**< Port 3 Pin 0: LPPDM_C0_B */
#define PIN_P3_0__I2S1_SDI_B            ALIF_PINMUX(3, 0, 4)  /**< Port 3 Pin 0: I2S1_SDI_B */
#define PIN_P3_0__SPI2_MISO_A           ALIF_PINMUX(3, 0, 5)  /**< Port 3 Pin 0: SPI2_MISO_A */
#define PIN_P3_0__CDC_HSYNC_B           ALIF_PINMUX(3, 0, 6)  /**< Port 3 Pin 0: CDC_HSYNC_B */

/** Pin P3.1 alternate functions */
#define PIN_P3_1__GPIO                  ALIF_PINMUX(3, 1, 0)  /**< Port 3 Pin 1: GPIO */
#define PIN_P3_1__OSPI0_D1_A            ALIF_PINMUX(3, 1, 1)  /**< Port 3 Pin 1: OSPI0_D1_A */
#define PIN_P3_1__UART5_TX_A            ALIF_PINMUX(3, 1, 2)  /**< Port 3 Pin 1: UART5_TX_A */
#define PIN_P3_1__LPPDM_D0_B            ALIF_PINMUX(3, 1, 3)  /**< Port 3 Pin 1: LPPDM_D0_B */
#define PIN_P3_1__I2S1_SDO_B            ALIF_PINMUX(3, 1, 4)  /**< Port 3 Pin 1: I2S1_SDO_B */
#define PIN_P3_1__SPI2_MOSI_A           ALIF_PINMUX(3, 1, 5)  /**< Port 3 Pin 1: SPI2_MOSI_A */
#define PIN_P3_1__CDC_VSYNC_B           ALIF_PINMUX(3, 1, 6)  /**< Port 3 Pin 1: CDC_VSYNC_B */

/** Pin P3.2 alternate functions */
#define PIN_P3_2__GPIO                  ALIF_PINMUX(3, 2, 0)  /**< Port 3 Pin 2: GPIO */
#define PIN_P3_2__OSPI0_D2_A            ALIF_PINMUX(3, 2, 1)  /**< Port 3 Pin 2: OSPI0_D2_A */
#define PIN_P3_2__UART3_RX_B            ALIF_PINMUX(3, 2, 2)  /**< Port 3 Pin 2: UART3_RX_B */
#define PIN_P3_2__LPPDM_C1_B            ALIF_PINMUX(3, 2, 3)  /**< Port 3 Pin 2: LPPDM_C1_B */
#define PIN_P3_2__I2S1_SCLK_SLV_B       ALIF_PINMUX(3, 2, 4)  /**< Port 3 Pin 2: I2S1_SCLK_SLV_B */
#define PIN_P3_2__SPI2_SCLK_A           ALIF_PINMUX(3, 2, 5)  /**< Port 3 Pin 2: SPI2_SCLK_A */
#define PIN_P3_2__CDC_PCLK_B            ALIF_PINMUX(3, 2, 6)  /**< Port 3 Pin 2: CDC_PCLK_B */

/** Pin P3.3 alternate functions */
#define PIN_P3_3__GPIO                  ALIF_PINMUX(3, 3, 0)  /**< Port 3 Pin 3: GPIO */
#define PIN_P3_3__OSPI0_D3_A            ALIF_PINMUX(3, 3, 1)  /**< Port 3 Pin 3: OSPI0_D3_A */
#define PIN_P3_3__UART3_TX_B            ALIF_PINMUX(3, 3, 2)  /**< Port 3 Pin 3: UART3_TX_B */
#define PIN_P3_3__LPPDM_D1_B            ALIF_PINMUX(3, 3, 3)  /**< Port 3 Pin 3: LPPDM_D1_B */
#define PIN_P3_3__I2S1_WS_SLV_B         ALIF_PINMUX(3, 3, 4)  /**< Port 3 Pin 3: I2S1_WS_SLV_B */
#define PIN_P3_3__SPI2_SS0_A            ALIF_PINMUX(3, 3, 5)  /**< Port 3 Pin 3: SPI2_SS0_A */
#define PIN_P3_3__CDC_DE_B              ALIF_PINMUX(3, 3, 6)  /**< Port 3 Pin 3: CDC_DE_B */

/** Pin P3.4 alternate functions */
#define PIN_P3_4__GPIO                  ALIF_PINMUX(3, 4, 0)  /**< Port 3 Pin 4: GPIO */
#define PIN_P3_4__OSPI0_D4_A            ALIF_PINMUX(3, 4, 1)  /**< Port 3 Pin 4: OSPI0_D4_A */
#define PIN_P3_4__LPPDM_C2_B            ALIF_PINMUX(3, 4, 2)  /**< Port 3 Pin 4: LPPDM_C2_B */
#define PIN_P3_4__SPI1_MISO_A           ALIF_PINMUX(3, 4, 3)  /**< Port 3 Pin 4: SPI1_MISO_A */
#define PIN_P3_4__LPCAM_VSYNC_A         ALIF_PINMUX(3, 4, 4)  /**< Port 3 Pin 4: LPCAM_VSYNC_A */
#define PIN_P3_4__SD_CMD_A              ALIF_PINMUX(3, 4, 5)  /**< Port 3 Pin 4: SD_CMD_A */
#define PIN_P3_4__CMP0_OUT_A            ALIF_PINMUX(3, 4, 6)  /**< Port 3 Pin 4: CMP0_OUT_A */

/** Pin P3.5 alternate functions */
#define PIN_P3_5__GPIO                  ALIF_PINMUX(3, 5, 0)  /**< Port 3 Pin 5: GPIO */
#define PIN_P3_5__OSPI0_D5_A            ALIF_PINMUX(3, 5, 1)  /**< Port 3 Pin 5: OSPI0_D5_A */
#define PIN_P3_5__LPPDM_D2_B            ALIF_PINMUX(3, 5, 2)  /**< Port 3 Pin 5: LPPDM_D2_B */
#define PIN_P3_5__SPI1_MOSI_A           ALIF_PINMUX(3, 5, 3)  /**< Port 3 Pin 5: SPI1_MOSI_A */
#define PIN_P3_5__LPCAM_HSYNC_A         ALIF_PINMUX(3, 5, 4)  /**< Port 3 Pin 5: LPCAM_HSYNC_A */
#define PIN_P3_5__SD_CLK_A              ALIF_PINMUX(3, 5, 5)  /**< Port 3 Pin 5: SD_CLK_A */
#define PIN_P3_5__CMP1_OUT_A            ALIF_PINMUX(3, 5, 6)  /**< Port 3 Pin 5: CMP1_OUT_A */

/** Pin P3.6 alternate functions */
#define PIN_P3_6__GPIO                  ALIF_PINMUX(3, 6, 0)  /**< Port 3 Pin 6: GPIO */
#define PIN_P3_6__OSPI0_D6_A            ALIF_PINMUX(3, 6, 1)  /**< Port 3 Pin 6: OSPI0_D6_A */
#define PIN_P3_6__UART4_RX_B            ALIF_PINMUX(3, 6, 2)  /**< Port 3 Pin 6: UART4_RX_B */
#define PIN_P3_6__LPPDM_C3_B            ALIF_PINMUX(3, 6, 3)  /**< Port 3 Pin 6: LPPDM_C3_B */
#define PIN_P3_6__SPI1_SCLK_A           ALIF_PINMUX(3, 6, 4)  /**< Port 3 Pin 6: SPI1_SCLK_A */
#define PIN_P3_6__LPCAM_PCLK_A          ALIF_PINMUX(3, 6, 5)  /**< Port 3 Pin 6: LPCAM_PCLK_A */
#define PIN_P3_6__EXT_RTS_A             ALIF_PINMUX(3, 6, 6)  /**< Port 3 Pin 6: EXT_RTS_A */
#define PIN_P3_6__CX_WLAN_TX            ALIF_PINMUX(3, 6, 7)  /**< Port 3 Pin 6: CX_WLAN_TX */

/** Pin P3.7 alternate functions */
#define PIN_P3_7__GPIO                  ALIF_PINMUX(3, 7, 0)  /**< Port 3 Pin 7: GPIO */
#define PIN_P3_7__OSPI0_D7_A            ALIF_PINMUX(3, 7, 1)  /**< Port 3 Pin 7: OSPI0_D7_A */
#define PIN_P3_7__UART4_TX_B            ALIF_PINMUX(3, 7, 2)  /**< Port 3 Pin 7: UART4_TX_B */
#define PIN_P3_7__LPPDM_D3_B            ALIF_PINMUX(3, 7, 3)  /**< Port 3 Pin 7: LPPDM_D3_B */
#define PIN_P3_7__LPCAM_XVCLK_A         ALIF_PINMUX(3, 7, 4)  /**< Port 3 Pin 7: LPCAM_XVCLK_A */
#define PIN_P3_7__SD_RST_B              ALIF_PINMUX(3, 7, 5)  /**< Port 3 Pin 7: SD_RST_B */
#define PIN_P3_7__EXT_CTS_A             ALIF_PINMUX(3, 7, 6)  /**< Port 3 Pin 7: EXT_CTS_A */
#define PIN_P3_7__CX_WLAN_RX            ALIF_PINMUX(3, 7, 7)  /**< Port 3 Pin 7: CX_WLAN_RX */

/** Pin P4.0 alternate functions */
#define PIN_P4_0__GPIO                  ALIF_PINMUX(4, 0, 0)  /**< Port 4 Pin 0: GPIO */
#define PIN_P4_0__OSPI0_SCLKN_A         ALIF_PINMUX(4, 0, 1)  /**< Port 4 Pin 0: OSPI0_SCLKN_A */
#define PIN_P4_0__UART0_RX_B            ALIF_PINMUX(4, 0, 2)  /**< Port 4 Pin 0: UART0_RX_B */
#define PIN_P4_0__SPI2_SS1_A            ALIF_PINMUX(4, 0, 3)  /**< Port 4 Pin 0: SPI2_SS1_A */
#define PIN_P4_0__UT0_T0_A              ALIF_PINMUX(4, 0, 4)  /**< Port 4 Pin 0: UT0_T0_A */
#define PIN_P4_0__LPCAM_D0_A            ALIF_PINMUX(4, 0, 5)  /**< Port 4 Pin 0: LPCAM_D0_A */
#define PIN_P4_0__SD_D0_A               ALIF_PINMUX(4, 0, 6)  /**< Port 4 Pin 0: SD_D0_A */
#define PIN_P4_0__EXT_RX_A              ALIF_PINMUX(4, 0, 7)  /**< Port 4 Pin 0: EXT_RX_A */

/** Pin P4.1 alternate functions */
#define PIN_P4_1__GPIO                  ALIF_PINMUX(4, 1, 0)  /**< Port 4 Pin 1: GPIO */
#define PIN_P4_1__OSPI0_RXDS_A          ALIF_PINMUX(4, 1, 1)  /**< Port 4 Pin 1: OSPI0_RXDS_A */
#define PIN_P4_1__UART0_TX_B            ALIF_PINMUX(4, 1, 2)  /**< Port 4 Pin 1: UART0_TX_B */
#define PIN_P4_1__SPI2_SS2_A            ALIF_PINMUX(4, 1, 3)  /**< Port 4 Pin 1: SPI2_SS2_A */
#define PIN_P4_1__UT0_T1_A              ALIF_PINMUX(4, 1, 4)  /**< Port 4 Pin 1: UT0_T1_A */
#define PIN_P4_1__LPCAM_D1_A            ALIF_PINMUX(4, 1, 5)  /**< Port 4 Pin 1: LPCAM_D1_A */
#define PIN_P4_1__SD_D1_A               ALIF_PINMUX(4, 1, 6)  /**< Port 4 Pin 1: SD_D1_A */
#define PIN_P4_1__EXT_TX_A              ALIF_PINMUX(4, 1, 7)  /**< Port 4 Pin 1: EXT_TX_A */

/** Pin P4.2 alternate functions */
#define PIN_P4_2__GPIO                  ALIF_PINMUX(4, 2, 0)  /**< Port 4 Pin 2: GPIO */
#define PIN_P4_2__HFXO_OUT_A            ALIF_PINMUX(4, 2, 1)  /**< Port 4 Pin 2: HFXO_OUT_A */
#define PIN_P4_2__OSPI0_SCLK_A          ALIF_PINMUX(4, 2, 2)  /**< Port 4 Pin 2: OSPI0_SCLK_A */
#define PIN_P4_2__UART4_DE_A            ALIF_PINMUX(4, 2, 3)  /**< Port 4 Pin 2: UART4_DE_A */
#define PIN_P4_2__UT1_T0_A              ALIF_PINMUX(4, 2, 4)  /**< Port 4 Pin 2: UT1_T0_A */
#define PIN_P4_2__LPCAM_D2_A            ALIF_PINMUX(4, 2, 5)  /**< Port 4 Pin 2: LPCAM_D2_A */
#define PIN_P4_2__SD_D2_A               ALIF_PINMUX(4, 2, 6)  /**< Port 4 Pin 2: SD_D2_A */
#define PIN_P4_2__EXT_TRACE_A           ALIF_PINMUX(4, 2, 7)  /**< Port 4 Pin 2: EXT_TRACE_A */

/** Pin P4.3 alternate functions */
#define PIN_P4_3__GPIO                  ALIF_PINMUX(4, 3, 0)  /**< Port 4 Pin 3: GPIO */
#define PIN_P4_3__JTAG0_NSRST           ALIF_PINMUX(4, 3, 1)  /**< Port 4 Pin 3: JTAG0_NSRST */
#define PIN_P4_3__CS_MWS_ID0            ALIF_PINMUX(4, 3, 2)  /**< Port 4 Pin 3: CS_MWS_ID0 */
#define PIN_P4_3__UT1_T1_A              ALIF_PINMUX(4, 3, 3)  /**< Port 4 Pin 3: UT1_T1_A */
#define PIN_P4_3__CMP0_OUT_B            ALIF_PINMUX(4, 3, 6)  /**< Port 4 Pin 3: CMP0_OUT_B */

/** Pin P4.4 alternate functions */
#define PIN_P4_4__GPIO                  ALIF_PINMUX(4, 4, 0)  /**< Port 4 Pin 4: GPIO */
#define PIN_P4_4__JTAG0_TCK             ALIF_PINMUX(4, 4, 1)  /**< Port 4 Pin 4: JTAG0_TCK */
#define PIN_P4_4__FAULT0_B              ALIF_PINMUX(4, 4, 2)  /**< Port 4 Pin 4: FAULT0_B */
#define PIN_P4_4__CS_MWS_ID1            ALIF_PINMUX(4, 4, 3)  /**< Port 4 Pin 4: CS_MWS_ID1 */
#define PIN_P4_4__UT2_T0_A              ALIF_PINMUX(4, 4, 4)  /**< Port 4 Pin 4: UT2_T0_A */

/** Pin P4.5 alternate functions */
#define PIN_P4_5__GPIO                  ALIF_PINMUX(4, 5, 0)  /**< Port 4 Pin 5: GPIO */
#define PIN_P4_5__JTAG0_TMS             ALIF_PINMUX(4, 5, 1)  /**< Port 4 Pin 5: JTAG0_TMS */
#define PIN_P4_5__FAULT1_B              ALIF_PINMUX(4, 5, 2)  /**< Port 4 Pin 5: FAULT1_B */
#define PIN_P4_5__CS_MWS_ID2            ALIF_PINMUX(4, 5, 3)  /**< Port 4 Pin 5: CS_MWS_ID2 */
#define PIN_P4_5__UT2_T1_A              ALIF_PINMUX(4, 5, 4)  /**< Port 4 Pin 5: UT2_T1_A */

/** Pin P4.6 alternate functions */
#define PIN_P4_6__GPIO                  ALIF_PINMUX(4, 6, 0)  /**< Port 4 Pin 6: GPIO */
#define PIN_P4_6__JTAG0_TDI             ALIF_PINMUX(4, 6, 1)  /**< Port 4 Pin 6: JTAG0_TDI */
#define PIN_P4_6__FAULT2_B              ALIF_PINMUX(4, 6, 2)  /**< Port 4 Pin 6: FAULT2_B */
#define PIN_P4_6__LPI2S_WS_B            ALIF_PINMUX(4, 6, 3)  /**< Port 4 Pin 6: LPI2S_WS_B */
#define PIN_P4_6__UT3_T0_A              ALIF_PINMUX(4, 6, 4)  /**< Port 4 Pin 6: UT3_T0_A */

/** Pin P4.7 alternate functions */
#define PIN_P4_7__GPIO                  ALIF_PINMUX(4, 7, 0)  /**< Port 4 Pin 7: GPIO */
#define PIN_P4_7__JTAG0_TDO             ALIF_PINMUX(4, 7, 1)  /**< Port 4 Pin 7: JTAG0_TDO */
#define PIN_P4_7__FAULT3_B              ALIF_PINMUX(4, 7, 2)  /**< Port 4 Pin 7: FAULT3_B */
#define PIN_P4_7__UT3_T1_A              ALIF_PINMUX(4, 7, 3)  /**< Port 4 Pin 7: UT3_T1_A */
#define PIN_P4_7__CS_MWS_ID4            ALIF_PINMUX(4, 7, 4)  /**< Port 4 Pin 7: CS_MWS_ID4 */
#define PIN_P4_7__CMP1_OUT_B            ALIF_PINMUX(4, 7, 6)  /**< Port 4 Pin 7: CMP1_OUT_B */

/** Pin P5.0 alternate functions */
#define PIN_P5_0__GPIO                  ALIF_PINMUX(5, 0, 0)  /**< Port 5 Pin 0: GPIO */
#define PIN_P5_0__OSPI0_D0_B            ALIF_PINMUX(5, 0, 1)  /**< Port 5 Pin 0: OSPI0_D0_B */
#define PIN_P5_0__UART1_RX_B            ALIF_PINMUX(5, 0, 2)  /**< Port 5 Pin 0: UART1_RX_B */
#define PIN_P5_0__SPI0_MISO_B           ALIF_PINMUX(5, 0, 3)  /**< Port 5 Pin 0: SPI0_MISO_B */
#define PIN_P5_0__CDC_D0_A              ALIF_PINMUX(5, 0, 4)  /**< Port 5 Pin 0: CDC_D0_A */
#define PIN_P5_0__CAN0_RXD_B            ALIF_PINMUX(5, 0, 5)  /**< Port 5 Pin 0: CAN0_RXD_B */
#define PIN_P5_0__DEBUG_PORT0           ALIF_PINMUX(5, 0, 6)  /**< Port 5 Pin 0: DEBUG_PORT0 */

/** Pin P5.1 alternate functions */
#define PIN_P5_1__GPIO                  ALIF_PINMUX(5, 1, 0)  /**< Port 5 Pin 1: GPIO */
#define PIN_P5_1__OSPI0_D1_B            ALIF_PINMUX(5, 1, 1)  /**< Port 5 Pin 1: OSPI0_D1_B */
#define PIN_P5_1__UART1_TX_B            ALIF_PINMUX(5, 1, 2)  /**< Port 5 Pin 1: UART1_TX_B */
#define PIN_P5_1__SPI0_MOSI_B           ALIF_PINMUX(5, 1, 3)  /**< Port 5 Pin 1: SPI0_MOSI_B */
#define PIN_P5_1__CDC_D1_A              ALIF_PINMUX(5, 1, 4)  /**< Port 5 Pin 1: CDC_D1_A */
#define PIN_P5_1__CAN0_TXD_B            ALIF_PINMUX(5, 1, 5)  /**< Port 5 Pin 1: CAN0_TXD_B */
#define PIN_P5_1__DEBUG_PORT1           ALIF_PINMUX(5, 1, 6)  /**< Port 5 Pin 1: DEBUG_PORT1 */

/** Pin P5.2 alternate functions */
#define PIN_P5_2__GPIO                  ALIF_PINMUX(5, 2, 0)  /**< Port 5 Pin 2: GPIO */
#define PIN_P5_2__HFXO_OUT_B            ALIF_PINMUX(5, 2, 1)  /**< Port 5 Pin 2: HFXO_OUT_B */
#define PIN_P5_2__OSPI0_D2_B            ALIF_PINMUX(5, 2, 2)  /**< Port 5 Pin 2: OSPI0_D2_B */
#define PIN_P5_2__UART2_RX_B            ALIF_PINMUX(5, 2, 3)  /**< Port 5 Pin 2: UART2_RX_B */
#define PIN_P5_2__SPI0_SCLK_B           ALIF_PINMUX(5, 2, 4)  /**< Port 5 Pin 2: SPI0_SCLK_B */
#define PIN_P5_2__CDC_D2_A              ALIF_PINMUX(5, 2, 5)  /**< Port 5 Pin 2: CDC_D2_A */
#define PIN_P5_2__DEBUG_PORT2           ALIF_PINMUX(5, 2, 6)  /**< Port 5 Pin 2: DEBUG_PORT2 */

/** Pin P5.3 alternate functions */
#define PIN_P5_3__GPIO                  ALIF_PINMUX(5, 3, 0)  /**< Port 5 Pin 3: GPIO */
#define PIN_P5_3__OSPI0_D3_B            ALIF_PINMUX(5, 3, 1)  /**< Port 5 Pin 3: OSPI0_D3_B */
#define PIN_P5_3__UART2_TX_B            ALIF_PINMUX(5, 3, 2)  /**< Port 5 Pin 3: UART2_TX_B */
#define PIN_P5_3__SPI0_SS0_B            ALIF_PINMUX(5, 3, 3)  /**< Port 5 Pin 3: SPI0_SS0_B */
#define PIN_P5_3__CDC_D3_A              ALIF_PINMUX(5, 3, 4)  /**< Port 5 Pin 3: CDC_D3_A */
#define PIN_P5_3__DEBUG_PORT3           ALIF_PINMUX(5, 3, 5)  /**< Port 5 Pin 3: DEBUG_PORT3 */

/** Pin P5.4 alternate functions */
#define PIN_P5_4__GPIO                  ALIF_PINMUX(5, 4, 0)  /**< Port 5 Pin 4: GPIO */
#define PIN_P5_4__OSPI0_D4_B            ALIF_PINMUX(5, 4, 1)  /**< Port 5 Pin 4: OSPI0_D4_B */
#define PIN_P5_4__UART3_CTS_A           ALIF_PINMUX(5, 4, 2)  /**< Port 5 Pin 4: UART3_CTS_A */
#define PIN_P5_4__LPPDM_C0_C            ALIF_PINMUX(5, 4, 3)  /**< Port 5 Pin 4: LPPDM_C0_C */
#define PIN_P5_4__SPI0_SS1_B            ALIF_PINMUX(5, 4, 4)  /**< Port 5 Pin 4: SPI0_SS1_B */
#define PIN_P5_4__CDC_D4_A              ALIF_PINMUX(5, 4, 5)  /**< Port 5 Pin 4: CDC_D4_A */
#define PIN_P5_4__DEBUG_PORT4           ALIF_PINMUX(5, 4, 6)  /**< Port 5 Pin 4: DEBUG_PORT4 */

/** Pin P5.5 alternate functions */
#define PIN_P5_5__GPIO                  ALIF_PINMUX(5, 5, 0)  /**< Port 5 Pin 5: GPIO */
#define PIN_P5_5__OSPI0_D5_B            ALIF_PINMUX(5, 5, 1)  /**< Port 5 Pin 5: OSPI0_D5_B */
#define PIN_P5_5__UART3_RTS_A           ALIF_PINMUX(5, 5, 2)  /**< Port 5 Pin 5: UART3_RTS_A */
#define PIN_P5_5__LPPDM_C1_C            ALIF_PINMUX(5, 5, 3)  /**< Port 5 Pin 5: LPPDM_C1_C */
#define PIN_P5_5__SPI0_SS2_B            ALIF_PINMUX(5, 5, 4)  /**< Port 5 Pin 5: SPI0_SS2_B */
#define PIN_P5_5__CDC_D5_A              ALIF_PINMUX(5, 5, 5)  /**< Port 5 Pin 5: CDC_D5_A */
#define PIN_P5_5__DEBUG_PORT5           ALIF_PINMUX(5, 5, 6)  /**< Port 5 Pin 5: DEBUG_PORT5 */

/** Pin P5.6 alternate functions */
#define PIN_P5_6__GPIO                  ALIF_PINMUX(5, 6, 0)  /**< Port 5 Pin 6: GPIO */
#define PIN_P5_6__OSPI0_D6_B            ALIF_PINMUX(5, 6, 1)  /**< Port 5 Pin 6: OSPI0_D6_B */
#define PIN_P5_6__UART1_CTS_B           ALIF_PINMUX(5, 6, 2)  /**< Port 5 Pin 6: UART1_CTS_B */
#define PIN_P5_6__LPPDM_D0_C            ALIF_PINMUX(5, 6, 3)  /**< Port 5 Pin 6: LPPDM_D0_C */
#define PIN_P5_6__LPSPI_MISO_B          ALIF_PINMUX(5, 6, 4)  /**< Port 5 Pin 6: LPSPI_MISO_B */
#define PIN_P5_6__CDC_D6_A              ALIF_PINMUX(5, 6, 5)  /**< Port 5 Pin 6: CDC_D6_A */
#define PIN_P5_6__DEBUG_PORT6           ALIF_PINMUX(5, 6, 6)  /**< Port 5 Pin 6: DEBUG_PORT6 */

/** Pin P5.7 alternate functions */
#define PIN_P5_7__GPIO                  ALIF_PINMUX(5, 7, 0)  /**< Port 5 Pin 7: GPIO */
#define PIN_P5_7__OSPI0_D7_B            ALIF_PINMUX(5, 7, 1)  /**< Port 5 Pin 7: OSPI0_D7_B */
#define PIN_P5_7__UART1_RTS_B           ALIF_PINMUX(5, 7, 2)  /**< Port 5 Pin 7: UART1_RTS_B */
#define PIN_P5_7__LPPDM_D1_C            ALIF_PINMUX(5, 7, 3)  /**< Port 5 Pin 7: LPPDM_D1_C */
#define PIN_P5_7__LPSPI_MOSI_B          ALIF_PINMUX(5, 7, 4)  /**< Port 5 Pin 7: LPSPI_MOSI_B */
#define PIN_P5_7__CDC_D7_A              ALIF_PINMUX(5, 7, 5)  /**< Port 5 Pin 7: CDC_D7_A */
#define PIN_P5_7__DEBUG_PORT7           ALIF_PINMUX(5, 7, 6)  /**< Port 5 Pin 7: DEBUG_PORT7 */

/** Pin P6.0 alternate functions */
#define PIN_P6_0__GPIO                  ALIF_PINMUX(6, 0, 0)  /**< Port 6 Pin 0: GPIO */
#define PIN_P6_0__OSPI0_D0_C            ALIF_PINMUX(6, 0, 1)  /**< Port 6 Pin 0: OSPI0_D0_C */
#define PIN_P6_0__UART4_DE_B            ALIF_PINMUX(6, 0, 2)  /**< Port 6 Pin 0: UART4_DE_B */
#define PIN_P6_0__LPSPI_SS_B            ALIF_PINMUX(6, 0, 3)  /**< Port 6 Pin 0: LPSPI_SS_B */
#define PIN_P6_0__UT0_T0_B              ALIF_PINMUX(6, 0, 4)  /**< Port 6 Pin 0: UT0_T0_B */
#define PIN_P6_0__SD_D0_B               ALIF_PINMUX(6, 0, 5)  /**< Port 6 Pin 0: SD_D0_B */
#define PIN_P6_0__CDC_D8_A              ALIF_PINMUX(6, 0, 6)  /**< Port 6 Pin 0: CDC_D8_A */

/** Pin P6.1 alternate functions */
#define PIN_P6_1__GPIO                  ALIF_PINMUX(6, 1, 0)  /**< Port 6 Pin 1: GPIO */
#define PIN_P6_1__OSPI0_D1_C            ALIF_PINMUX(6, 1, 1)  /**< Port 6 Pin 1: OSPI0_D1_C */
#define PIN_P6_1__UART5_DE_A            ALIF_PINMUX(6, 1, 2)  /**< Port 6 Pin 1: UART5_DE_A */
#define PIN_P6_1__SPI1_MISO_B           ALIF_PINMUX(6, 1, 3)  /**< Port 6 Pin 1: SPI1_MISO_B */
#define PIN_P6_1__UT0_T1_B              ALIF_PINMUX(6, 1, 4)  /**< Port 6 Pin 1: UT0_T1_B */
#define PIN_P6_1__SD_D1_B               ALIF_PINMUX(6, 1, 5)  /**< Port 6 Pin 1: SD_D1_B */
#define PIN_P6_1__CDC_D9_A              ALIF_PINMUX(6, 1, 6)  /**< Port 6 Pin 1: CDC_D9_A */

/** Pin P6.2 alternate functions */
#define PIN_P6_2__GPIO                  ALIF_PINMUX(6, 2, 0)  /**< Port 6 Pin 2: GPIO */
#define PIN_P6_2__OSPI0_D2_C            ALIF_PINMUX(6, 2, 1)  /**< Port 6 Pin 2: OSPI0_D2_C */
#define PIN_P6_2__UART2_CTS_A           ALIF_PINMUX(6, 2, 2)  /**< Port 6 Pin 2: UART2_CTS_A */
#define PIN_P6_2__SPI1_MOSI_B           ALIF_PINMUX(6, 2, 3)  /**< Port 6 Pin 2: SPI1_MOSI_B */
#define PIN_P6_2__UT1_T0_B              ALIF_PINMUX(6, 2, 4)  /**< Port 6 Pin 2: UT1_T0_B */
#define PIN_P6_2__SD_D2_B               ALIF_PINMUX(6, 2, 5)  /**< Port 6 Pin 2: SD_D2_B */
#define PIN_P6_2__CDC_D10_A             ALIF_PINMUX(6, 2, 6)  /**< Port 6 Pin 2: CDC_D10_A */

/** Pin P6.3 alternate functions */
#define PIN_P6_3__GPIO                  ALIF_PINMUX(6, 3, 0)  /**< Port 6 Pin 3: GPIO */
#define PIN_P6_3__OSPI0_D3_C            ALIF_PINMUX(6, 3, 1)  /**< Port 6 Pin 3: OSPI0_D3_C */
#define PIN_P6_3__UART2_RTS_A           ALIF_PINMUX(6, 3, 2)  /**< Port 6 Pin 3: UART2_RTS_A */
#define PIN_P6_3__SPI1_SCLK_B           ALIF_PINMUX(6, 3, 3)  /**< Port 6 Pin 3: SPI1_SCLK_B */
#define PIN_P6_3__UT1_T1_B              ALIF_PINMUX(6, 3, 4)  /**< Port 6 Pin 3: UT1_T1_B */
#define PIN_P6_3__SD_D3_B               ALIF_PINMUX(6, 3, 5)  /**< Port 6 Pin 3: SD_D3_B */
#define PIN_P6_3__CDC_D11_A             ALIF_PINMUX(6, 3, 6)  /**< Port 6 Pin 3: CDC_D11_A */

/** Pin P6.4 alternate functions */
#define PIN_P6_4__GPIO                  ALIF_PINMUX(6, 4, 0)  /**< Port 6 Pin 4: GPIO */
#define PIN_P6_4__OSPI0_D4_C            ALIF_PINMUX(6, 4, 1)  /**< Port 6 Pin 4: OSPI0_D4_C */
#define PIN_P6_4__UART2_CTS_B           ALIF_PINMUX(6, 4, 2)  /**< Port 6 Pin 4: UART2_CTS_B */
#define PIN_P6_4__SPI1_SS0_B            ALIF_PINMUX(6, 4, 3)  /**< Port 6 Pin 4: SPI1_SS0_B */
#define PIN_P6_4__UT2_T0_B              ALIF_PINMUX(6, 4, 4)  /**< Port 6 Pin 4: UT2_T0_B */
#define PIN_P6_4__SD_D4_B               ALIF_PINMUX(6, 4, 5)  /**< Port 6 Pin 4: SD_D4_B */
#define PIN_P6_4__CDC_D12_A             ALIF_PINMUX(6, 4, 6)  /**< Port 6 Pin 4: CDC_D12_A */

/** Pin P6.5 alternate functions */
#define PIN_P6_5__GPIO                  ALIF_PINMUX(6, 5, 0)  /**< Port 6 Pin 5: GPIO */
#define PIN_P6_5__OSPI0_D5_C            ALIF_PINMUX(6, 5, 1)  /**< Port 6 Pin 5: OSPI0_D5_C */
#define PIN_P6_5__UART2_RTS_B           ALIF_PINMUX(6, 5, 2)  /**< Port 6 Pin 5: UART2_RTS_B */
#define PIN_P6_5__SPI1_SS1_B            ALIF_PINMUX(6, 5, 3)  /**< Port 6 Pin 5: SPI1_SS1_B */
#define PIN_P6_5__UT2_T1_B              ALIF_PINMUX(6, 5, 4)  /**< Port 6 Pin 5: UT2_T1_B */
#define PIN_P6_5__SD_D5_B               ALIF_PINMUX(6, 5, 5)  /**< Port 6 Pin 5: SD_D5_B */
#define PIN_P6_5__CDC_D13_A             ALIF_PINMUX(6, 5, 6)  /**< Port 6 Pin 5: CDC_D13_A */

/** Pin P6.6 alternate functions */
#define PIN_P6_6__GPIO                  ALIF_PINMUX(6, 6, 0)  /**< Port 6 Pin 6: GPIO */
#define PIN_P6_6__OSPI0_D6_C            ALIF_PINMUX(6, 6, 1)  /**< Port 6 Pin 6: OSPI0_D6_C */
#define PIN_P6_6__UART0_CTS_B           ALIF_PINMUX(6, 6, 2)  /**< Port 6 Pin 6: UART0_CTS_B */
#define PIN_P6_6__SPI1_SS2_B            ALIF_PINMUX(6, 6, 3)  /**< Port 6 Pin 6: SPI1_SS2_B */
#define PIN_P6_6__UT3_T0_B              ALIF_PINMUX(6, 6, 4)  /**< Port 6 Pin 6: UT3_T0_B */
#define PIN_P6_6__SD_D6_B               ALIF_PINMUX(6, 6, 5)  /**< Port 6 Pin 6: SD_D6_B */
#define PIN_P6_6__CDC_D14_A             ALIF_PINMUX(6, 6, 6)  /**< Port 6 Pin 6: CDC_D14_A */

/** Pin P6.7 alternate functions */
#define PIN_P6_7__GPIO                  ALIF_PINMUX(6, 7, 0)  /**< Port 6 Pin 7: GPIO */
#define PIN_P6_7__OSPI0_D7_C            ALIF_PINMUX(6, 7, 1)  /**< Port 6 Pin 7: OSPI0_D7_C */
#define PIN_P6_7__UART0_RTS_B           ALIF_PINMUX(6, 7, 2)  /**< Port 6 Pin 7: UART0_RTS_B */
#define PIN_P6_7__SPI2_SS2_C            ALIF_PINMUX(6, 7, 3)  /**< Port 6 Pin 7: SPI2_SS2_C */
#define PIN_P6_7__UT3_T1_B              ALIF_PINMUX(6, 7, 4)  /**< Port 6 Pin 7: UT3_T1_B */
#define PIN_P6_7__SD_D7_B               ALIF_PINMUX(6, 7, 5)  /**< Port 6 Pin 7: SD_D7_B */
#define PIN_P6_7__CDC_D15_A             ALIF_PINMUX(6, 7, 6)  /**< Port 6 Pin 7: CDC_D15_A */

/** Pin P7.0 alternate functions */
#define PIN_P7_0__GPIO                  ALIF_PINMUX(7, 0, 0)  /**< Port 7 Pin 0: GPIO */
#define PIN_P7_0__OSPI0_SCLK_C          ALIF_PINMUX(7, 0, 1)  /**< Port 7 Pin 0: OSPI0_SCLK_C */
#define PIN_P7_0__LPUART_RX_B           ALIF_PINMUX(7, 0, 2)  /**< Port 7 Pin 0: LPUART_RX_B */
#define PIN_P7_0__SPI2_MISO_B           ALIF_PINMUX(7, 0, 3)  /**< Port 7 Pin 0: SPI2_MISO_B */
#define PIN_P7_0__I2C0_SDA_C            ALIF_PINMUX(7, 0, 4)  /**< Port 7 Pin 0: I2C0_SDA_C */
#define PIN_P7_0__LPCAM_D0_B            ALIF_PINMUX(7, 0, 5)  /**< Port 7 Pin 0: LPCAM_D0_B */
#define PIN_P7_0__CDC_D16_A             ALIF_PINMUX(7, 0, 6)  /**< Port 7 Pin 0: CDC_D16_A */

/** Pin P7.1 alternate functions */
#define PIN_P7_1__GPIO                  ALIF_PINMUX(7, 1, 0)  /**< Port 7 Pin 1: GPIO */
#define PIN_P7_1__OSPI0_SCLKN_C         ALIF_PINMUX(7, 1, 1)  /**< Port 7 Pin 1: OSPI0_SCLKN_C */
#define PIN_P7_1__LPUART_TX_B           ALIF_PINMUX(7, 1, 2)  /**< Port 7 Pin 1: LPUART_TX_B */
#define PIN_P7_1__SPI2_MOSI_B           ALIF_PINMUX(7, 1, 3)  /**< Port 7 Pin 1: SPI2_MOSI_B */
#define PIN_P7_1__I2C0_SCL_C            ALIF_PINMUX(7, 1, 4)  /**< Port 7 Pin 1: I2C0_SCL_C */
#define PIN_P7_1__LPCAM_D1_B            ALIF_PINMUX(7, 1, 5)  /**< Port 7 Pin 1: LPCAM_D1_B */
#define PIN_P7_1__CDC_D17_A             ALIF_PINMUX(7, 1, 6)  /**< Port 7 Pin 1: CDC_D17_A */

/** Pin P7.2 alternate functions */
#define PIN_P7_2__GPIO                  ALIF_PINMUX(7, 2, 0)  /**< Port 7 Pin 2: GPIO */
#define PIN_P7_2__OSPI0_SS0_C           ALIF_PINMUX(7, 2, 1)  /**< Port 7 Pin 2: OSPI0_SS0_C */
#define PIN_P7_2__LPUART_CTS_B          ALIF_PINMUX(7, 2, 2)  /**< Port 7 Pin 2: LPUART_CTS_B */
#define PIN_P7_2__SPI2_SCLK_B           ALIF_PINMUX(7, 2, 3)  /**< Port 7 Pin 2: SPI2_SCLK_B */
#define PIN_P7_2__I2C1_SDA_C            ALIF_PINMUX(7, 2, 4)  /**< Port 7 Pin 2: I2C1_SDA_C */
#define PIN_P7_2__LPCAM_D2_B            ALIF_PINMUX(7, 2, 5)  /**< Port 7 Pin 2: LPCAM_D2_B */
#define PIN_P7_2__CDC_D18_A             ALIF_PINMUX(7, 2, 6)  /**< Port 7 Pin 2: CDC_D18_A */

/** Pin P7.3 alternate functions */
#define PIN_P7_3__GPIO                  ALIF_PINMUX(7, 3, 0)  /**< Port 7 Pin 3: GPIO */
#define PIN_P7_3__OSPI0_SS1_C           ALIF_PINMUX(7, 3, 1)  /**< Port 7 Pin 3: OSPI0_SS1_C */
#define PIN_P7_3__LPUART_RTS_B          ALIF_PINMUX(7, 3, 2)  /**< Port 7 Pin 3: LPUART_RTS_B */
#define PIN_P7_3__SPI2_SS0_B            ALIF_PINMUX(7, 3, 3)  /**< Port 7 Pin 3: SPI2_SS0_B */
#define PIN_P7_3__I2C1_SCL_C            ALIF_PINMUX(7, 3, 4)  /**< Port 7 Pin 3: I2C1_SCL_C */
#define PIN_P7_3__LPCAM_D3_B            ALIF_PINMUX(7, 3, 5)  /**< Port 7 Pin 3: LPCAM_D3_B */
#define PIN_P7_3__CDC_D19_A             ALIF_PINMUX(7, 3, 6)  /**< Port 7 Pin 3: CDC_D19_A */

/** Pin P7.4 alternate functions */
#define PIN_P7_4__GPIO                  ALIF_PINMUX(7, 4, 0)  /**< Port 7 Pin 4: GPIO */
#define PIN_P7_4__UART3_CTS_B           ALIF_PINMUX(7, 4, 1)  /**< Port 7 Pin 4: UART3_CTS_B */
#define PIN_P7_4__LPPDM_C2_C            ALIF_PINMUX(7, 4, 2)  /**< Port 7 Pin 4: LPPDM_C2_C */
#define PIN_P7_4__SPI2_SS1_B            ALIF_PINMUX(7, 4, 3)  /**< Port 7 Pin 4: SPI2_SS1_B */
#define PIN_P7_4__LPI2C0_SCL_C          ALIF_PINMUX(7, 4, 4)  /**< Port 7 Pin 4: LPI2C0_SCL_C */
#define PIN_P7_4__LPCAM_D4_B            ALIF_PINMUX(7, 4, 5)  /**< Port 7 Pin 4: LPCAM_D4_B */
#define PIN_P7_4__CDC_D20_A             ALIF_PINMUX(7, 4, 6)  /**< Port 7 Pin 4: CDC_D20_A */

/** Pin P7.5 alternate functions */
#define PIN_P7_5__GPIO                  ALIF_PINMUX(7, 5, 0)  /**< Port 7 Pin 5: GPIO */
#define PIN_P7_5__UART3_RTS_B           ALIF_PINMUX(7, 5, 1)  /**< Port 7 Pin 5: UART3_RTS_B */
#define PIN_P7_5__LPPDM_D2_C            ALIF_PINMUX(7, 5, 2)  /**< Port 7 Pin 5: LPPDM_D2_C */
#define PIN_P7_5__SPI2_SS2_B            ALIF_PINMUX(7, 5, 3)  /**< Port 7 Pin 5: SPI2_SS2_B */
#define PIN_P7_5__LPI2C0_SDA_C          ALIF_PINMUX(7, 5, 4)  /**< Port 7 Pin 5: LPI2C0_SDA_C */
#define PIN_P7_5__LPCAM_D5_B            ALIF_PINMUX(7, 5, 5)  /**< Port 7 Pin 5: LPCAM_D5_B */
#define PIN_P7_5__CDC_D21_A             ALIF_PINMUX(7, 5, 6)  /**< Port 7 Pin 5: CDC_D21_A */

/** Pin P7.6 alternate functions */
#define PIN_P7_6__GPIO                  ALIF_PINMUX(7, 6, 0)  /**< Port 7 Pin 6: GPIO */
#define PIN_P7_6__SEUART_RX_C           ALIF_PINMUX(7, 6, 1)  /**< Port 7 Pin 6: SEUART_RX_C */
#define PIN_P7_6__UART5_RX_B            ALIF_PINMUX(7, 6, 2)  /**< Port 7 Pin 6: UART5_RX_B */
#define PIN_P7_6__LPPDM_C3_C            ALIF_PINMUX(7, 6, 3)  /**< Port 7 Pin 6: LPPDM_C3_C */
#define PIN_P7_6__SPI0_MISO_C           ALIF_PINMUX(7, 6, 4)  /**< Port 7 Pin 6: SPI0_MISO_C */
#define PIN_P7_6__LPCAM_D6_B            ALIF_PINMUX(7, 6, 5)  /**< Port 7 Pin 6: LPCAM_D6_B */
#define PIN_P7_6__CDC_D22_A             ALIF_PINMUX(7, 6, 6)  /**< Port 7 Pin 6: CDC_D22_A */

/** Pin P7.7 alternate functions */
#define PIN_P7_7__GPIO                  ALIF_PINMUX(7, 7, 0)  /**< Port 7 Pin 7: GPIO */
#define PIN_P7_7__SEUART_TX_C           ALIF_PINMUX(7, 7, 1)  /**< Port 7 Pin 7: SEUART_TX_C */
#define PIN_P7_7__UART5_TX_B            ALIF_PINMUX(7, 7, 2)  /**< Port 7 Pin 7: UART5_TX_B */
#define PIN_P7_7__LPPDM_D3_C            ALIF_PINMUX(7, 7, 3)  /**< Port 7 Pin 7: LPPDM_D3_C */
#define PIN_P7_7__SPI0_MOSI_C           ALIF_PINMUX(7, 7, 4)  /**< Port 7 Pin 7: SPI0_MOSI_C */
#define PIN_P7_7__LPCAM_D7_B            ALIF_PINMUX(7, 7, 5)  /**< Port 7 Pin 7: LPCAM_D7_B */
#define PIN_P7_7__CDC_D23_A             ALIF_PINMUX(7, 7, 6)  /**< Port 7 Pin 7: CDC_D23_A */

/** Pin P8.0 alternate functions */
#define PIN_P8_0__GPIO                  ALIF_PINMUX(8, 0, 0)  /**< Port 8 Pin 0: GPIO */
#define PIN_P8_0__UART0_RX_C            ALIF_PINMUX(8, 0, 1)  /**< Port 8 Pin 0: UART0_RX_C */
#define PIN_P8_0__I2S0_SDI_C            ALIF_PINMUX(8, 0, 2)  /**< Port 8 Pin 0: I2S0_SDI_C */
#define PIN_P8_0__SPI0_SCLK_C           ALIF_PINMUX(8, 0, 3)  /**< Port 8 Pin 0: SPI0_SCLK_C */
#define PIN_P8_0__UT0_T0_C              ALIF_PINMUX(8, 0, 4)  /**< Port 8 Pin 0: UT0_T0_C */
#define PIN_P8_0__SD_CMD_C              ALIF_PINMUX(8, 0, 5)  /**< Port 8 Pin 0: SD_CMD_C */
#define PIN_P8_0__I3C_SCL_C             ALIF_PINMUX(8, 0, 6)  /**< Port 8 Pin 0: I3C_SCL_C */

/** Pin P8.1 alternate functions */
#define PIN_P8_1__GPIO                  ALIF_PINMUX(8, 1, 0)  /**< Port 8 Pin 1: GPIO */
#define PIN_P8_1__UART0_TX_C            ALIF_PINMUX(8, 1, 1)  /**< Port 8 Pin 1: UART0_TX_C */
#define PIN_P8_1__I2S0_SDO_C            ALIF_PINMUX(8, 1, 2)  /**< Port 8 Pin 1: I2S0_SDO_C */
#define PIN_P8_1__SPI0_SS0_C            ALIF_PINMUX(8, 1, 3)  /**< Port 8 Pin 1: SPI0_SS0_C */
#define PIN_P8_1__UT0_T1_C              ALIF_PINMUX(8, 1, 4)  /**< Port 8 Pin 1: UT0_T1_C */
#define PIN_P8_1__SD_CLK_C              ALIF_PINMUX(8, 1, 5)  /**< Port 8 Pin 1: SD_CLK_C */
#define PIN_P8_1__I3C_SDA_C             ALIF_PINMUX(8, 1, 6)  /**< Port 8 Pin 1: I3C_SDA_C */

/** Pin P8.2 alternate functions */
#define PIN_P8_2__GPIO                  ALIF_PINMUX(8, 2, 0)  /**< Port 8 Pin 2: GPIO */
#define PIN_P8_2__SEUART_RX_B           ALIF_PINMUX(8, 2, 1)  /**< Port 8 Pin 2: SEUART_RX_B */
#define PIN_P8_2__UART1_RX_C            ALIF_PINMUX(8, 2, 2)  /**< Port 8 Pin 2: UART1_RX_C */
#define PIN_P8_2__I2S0_SCLK_C           ALIF_PINMUX(8, 2, 3)  /**< Port 8 Pin 2: I2S0_SCLK_C */
#define PIN_P8_2__SPI0_SS1_C            ALIF_PINMUX(8, 2, 4)  /**< Port 8 Pin 2: SPI0_SS1_C */
#define PIN_P8_2__UT1_T0_C              ALIF_PINMUX(8, 2, 5)  /**< Port 8 Pin 2: UT1_T0_C */
#define PIN_P8_2__SD_RST_C              ALIF_PINMUX(8, 2, 6)  /**< Port 8 Pin 2: SD_RST_C */
#define PIN_P8_2__CAN0_RXD_C            ALIF_PINMUX(8, 2, 7)  /**< Port 8 Pin 2: CAN0_RXD_C */

/** Pin P8.3 alternate functions */
#define PIN_P8_3__GPIO                  ALIF_PINMUX(8, 3, 0)  /**< Port 8 Pin 3: GPIO */
#define PIN_P8_3__SEUART_TX_B           ALIF_PINMUX(8, 3, 1)  /**< Port 8 Pin 3: SEUART_TX_B */
#define PIN_P8_3__UART1_TX_C            ALIF_PINMUX(8, 3, 2)  /**< Port 8 Pin 3: UART1_TX_C */
#define PIN_P8_3__I2S0_WS_C             ALIF_PINMUX(8, 3, 3)  /**< Port 8 Pin 3: I2S0_WS_C */
#define PIN_P8_3__SPI0_SS2_C            ALIF_PINMUX(8, 3, 4)  /**< Port 8 Pin 3: SPI0_SS2_C */
#define PIN_P8_3__UT1_T1_C              ALIF_PINMUX(8, 3, 5)  /**< Port 8 Pin 3: UT1_T1_C */
#define PIN_P8_3__CAN0_TXD_C            ALIF_PINMUX(8, 3, 6)  /**< Port 8 Pin 3: CAN0_TXD_C */
#define PIN_P8_3__EXT_RTS_B             ALIF_PINMUX(8, 3, 7)  /**< Port 8 Pin 3: EXT_RTS_B */

/** Pin P8.4 alternate functions */
#define PIN_P8_4__GPIO                  ALIF_PINMUX(8, 4, 0)  /**< Port 8 Pin 4: GPIO */
#define PIN_P8_4__UART2_RX_C            ALIF_PINMUX(8, 4, 1)  /**< Port 8 Pin 4: UART2_RX_C */
#define PIN_P8_4__I2S1_SDI_C            ALIF_PINMUX(8, 4, 2)  /**< Port 8 Pin 4: I2S1_SDI_C */
#define PIN_P8_4__SPI1_MISO_C           ALIF_PINMUX(8, 4, 3)  /**< Port 8 Pin 4: SPI1_MISO_C */
#define PIN_P8_4__UT2_T0_C              ALIF_PINMUX(8, 4, 4)  /**< Port 8 Pin 4: UT2_T0_C */
#define PIN_P8_4__LPCAM_VSYNC_C         ALIF_PINMUX(8, 4, 5)  /**< Port 8 Pin 4: LPCAM_VSYNC_C */
#define PIN_P8_4__CAN0_STBY_C           ALIF_PINMUX(8, 4, 6)  /**< Port 8 Pin 4: CAN0_STBY_C */
#define PIN_P8_4__EXT_CTS_B             ALIF_PINMUX(8, 4, 7)  /**< Port 8 Pin 4: EXT_CTS_B */

/** Pin P8.5 alternate functions */
#define PIN_P8_5__GPIO                  ALIF_PINMUX(8, 5, 0)  /**< Port 8 Pin 5: GPIO */
#define PIN_P8_5__UART2_TX_C            ALIF_PINMUX(8, 5, 1)  /**< Port 8 Pin 5: UART2_TX_C */
#define PIN_P8_5__I2S1_SDO_C            ALIF_PINMUX(8, 5, 2)  /**< Port 8 Pin 5: I2S1_SDO_C */
#define PIN_P8_5__SPI1_MOSI_C           ALIF_PINMUX(8, 5, 3)  /**< Port 8 Pin 5: SPI1_MOSI_C */
#define PIN_P8_5__UT2_T1_C              ALIF_PINMUX(8, 5, 4)  /**< Port 8 Pin 5: UT2_T1_C */
#define PIN_P8_5__LPCAM_HSYNC_C         ALIF_PINMUX(8, 5, 5)  /**< Port 8 Pin 5: LPCAM_HSYNC_C */
#define PIN_P8_5__CAN1_RXD_B            ALIF_PINMUX(8, 5, 6)  /**< Port 8 Pin 5: CAN1_RXD_B */
#define PIN_P8_5__EXT_TRACE_B           ALIF_PINMUX(8, 5, 7)  /**< Port 8 Pin 5: EXT_TRACE_B */

/** Pin P8.6 alternate functions */
#define PIN_P8_6__GPIO                  ALIF_PINMUX(8, 6, 0)  /**< Port 8 Pin 6: GPIO */
#define PIN_P8_6__UART3_RX_C            ALIF_PINMUX(8, 6, 1)  /**< Port 8 Pin 6: UART3_RX_C */
#define PIN_P8_6__I2S1_SCLK_SLV_C       ALIF_PINMUX(8, 6, 2)  /**< Port 8 Pin 6: I2S1_SCLK_SLV_C */
#define PIN_P8_6__SPI1_SCLK_C           ALIF_PINMUX(8, 6, 3)  /**< Port 8 Pin 6: SPI1_SCLK_C */
#define PIN_P8_6__UT3_T0_C              ALIF_PINMUX(8, 6, 4)  /**< Port 8 Pin 6: UT3_T0_C */
#define PIN_P8_6__LPCAM_PCLK_C          ALIF_PINMUX(8, 6, 5)  /**< Port 8 Pin 6: LPCAM_PCLK_C */
#define PIN_P8_6__CAN1_TXD_B            ALIF_PINMUX(8, 6, 6)  /**< Port 8 Pin 6: CAN1_TXD_B */
#define PIN_P8_6__EXT_RX_B              ALIF_PINMUX(8, 6, 7)  /**< Port 8 Pin 6: EXT_RX_B */

/** Pin P8.7 alternate functions */
#define PIN_P8_7__GPIO                  ALIF_PINMUX(8, 7, 0)  /**< Port 8 Pin 7: GPIO */
#define PIN_P8_7__UART3_TX_C            ALIF_PINMUX(8, 7, 1)  /**< Port 8 Pin 7: UART3_TX_C */
#define PIN_P8_7__I2S1_WS_SLV_C         ALIF_PINMUX(8, 7, 2)  /**< Port 8 Pin 7: I2S1_WS_SLV_C */
#define PIN_P8_7__SPI1_SS0_C            ALIF_PINMUX(8, 7, 3)  /**< Port 8 Pin 7: SPI1_SS0_C */
#define PIN_P8_7__UT3_T1_C              ALIF_PINMUX(8, 7, 4)  /**< Port 8 Pin 7: UT3_T1_C */
#define PIN_P8_7__LPCAM_XVCLK_C         ALIF_PINMUX(8, 7, 5)  /**< Port 8 Pin 7: LPCAM_XVCLK_C */
#define PIN_P8_7__CAN1_STBY_B           ALIF_PINMUX(8, 7, 6)  /**< Port 8 Pin 7: CAN1_STBY_B */
#define PIN_P8_7__EXT_TX_B              ALIF_PINMUX(8, 7, 7)  /**< Port 8 Pin 7: EXT_TX_B */

/** @} */

/**
 * @name LPGPIO Pin Definitions
 *
 * Pin multiplexing definitions for LPGPIO.
 * Note: Uses 15 in encoding for driver compatibility.
 * @{
 */

/** Pin LPGPIO.0 alternate functions */
#define PIN_P15_0__LPGPIO               ALIF_PINMUX(15, 0, 0)  /**< LPGPIO Pin 0: GPIO */

/** Pin LPGPIO.1 alternate functions */
#define PIN_P15_1__LPGPIO               ALIF_PINMUX(15, 1, 0)  /**< LPGPIO Pin 1: GPIO */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ALIF_ENSEMBLE_E1C_PINCTRL_H_ */
