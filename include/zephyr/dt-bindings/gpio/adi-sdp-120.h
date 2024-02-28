/**
 * Copyright (c) 2024 Analog Devices Inc.
 * Copyright (c) 2024 Baylibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file
 * @brief SDP-120 GPIO index definitions
 *
 * Defines meant to be used in conjunction with the "adi,sdp-120"
 * ADI SDP-120 mapping.
 *
 * Example usage:
 *
 * @code{.dts}
 * &spi1 {
 *         cs-gpios = <&sdp_120 SDP_120_SPI_SS_N GPIO_ACTIVE_LOW>;
 *
 *         example_device: example-dev@0 {
 *                 compatible = "vnd,spi-device";
 *                 reg = <0>;
 *         };
 * };
 * @endcode
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ADI_SDP_120_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ADI_SDP_120_H_

/* GPIO */

/**
 * @brief IO[n] signal on a SDP-120 GPIO nexus node following
 */

#define SDP_120_IO(n) (n-1)

/* SPI */
#define SDP_120_SPI_D2				SDP_120_IO(33)	/* SPI_D2 */
#define SDP_120_SPI_D3				SDP_120_IO(34)	/* SPI_D3 */
#define SDP_120_SERIAL_INT			SDP_120_IO(35)	/* SERIAL_INT */
#define SDP_120_SPI_SEL_B_N			SDP_120_IO(37)	/* SPI_SEL_B_N */
#define SDP_120_SPI_SEL_C_N			SDP_120_IO(38)	/* SPI_SEL_C_N */
#define SDP_120_SPI_SS_N			SDP_120_IO(39)	/* SPI_SS_N */

/* GPIO */
#define SDP_120_GPIO0				SDP_120_IO(43) /* GPIO0 */
#define SDP_120_GPIO2				SDP_120_IO(44) /* GPIO2 */
#define SDP_120_GPIO4				SDP_120_IO(45) /* GPIO4 */
#define SDP_120_GPIO6				SDP_120_IO(47) /* GPIO6 */

/* TMR */
#define SDP_120_TMR_A				SDP_120_IO(48) /* TMR_A */

/* USART */
#define SDP_120_UART_RX				SDP_120_IO(59) /* UART2_RX */
#define SDP_120_UART_TX				SDP_120_IO(62) /* UART2_TX */

/* TMR */
#define SDP_120_TMR_D				SDP_120_IO(72) /* TMR_D */
#define SDP_120_TMR_B				SDP_120_IO(73) /* TMR_B */

/* GPIO */
#define SDP_120_GPIO7				SDP_120_IO(74) /* GPIO7 */
#define SDP_120_GPIO5				SDP_120_IO(76) /* GPIO5 */
#define SDP_120_GPIO3				SDP_120_IO(77) /* GPIO3 */
#define SDP_120_GPIO1				SDP_120_IO(78) /* GPIO1  */

/* I2C */
#define SDP_120_SCL_0				SDP_120_IO(79) /* SCL_0 */
#define SDP_120_SDA_0				SDP_120_IO(80) /* SDA_0 */

/* SPI */
#define SDP_120_SPI_CLK				SDP_120_IO(82) /* SPI_CLK */
#define SDP_120_SPI_MISO			SDP_120_IO(83) /* SPI_MISO */
#define SDP_120_SPI_MOSI			SDP_120_IO(84) /* SPI_MOSI */
#define SDP_120_SPI_SEL_A_N			SDP_120_IO(85) /* SPI_SEL_A_N */

/* SPORT - no driver yet */
#define SDP_120_SPI_SPORT_TSCLK			SDP_120_IO(87) /* SPORT_TSCLK */
#define SDP_120_SPI_SPORT_DT0			SDP_120_IO(88) /* SPORT_DT0 */
#define SDP_120_SPI_SPORT_TFS			SDP_120_IO(89) /* SPORT_TFS */
#define SDP_120_SPI_SPORT_RFS			SDP_120_IO(90) /* SPORT_RFS */
#define SDP_120_SPI_SPORT_DR0			SDP_120_IO(91) /* SPORT_DR0 */
#define SDP_120_SPI_SPORT_RSCLK			SDP_120_IO(92) /* SPORT_RSCLK */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ADI_SDP_120_H_ */
