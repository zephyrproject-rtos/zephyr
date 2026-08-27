/*
 * Copyright (c) 2021 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Peripheral reset identifiers for Raspberry Pi RP2040
 * @ingroup reset_controller_raspberrypi_rp2040
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_RP2040_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_RP2040_RESET_H_

/**
 * @defgroup reset_controller_raspberrypi_rp2040 Raspberry Pi RP2040 reset controller helpers
 * @brief Peripheral reset-cell identifiers for RP2040 SoC.
 * @ingroup reset_controller_interface
 *
 * Devicetree macros for peripheral reset cells on Raspberry Pi RP2040 devices, for use with the
 * <tt>raspberrypi,pico-reset</tt> compatible reset controller.
 *
 * Reset identifiers follow the pattern @c RPI_PICO_RESETS_RESET_\<PERIPHERAL\>, where
 * @c \<PERIPHERAL\> is an RP2040 peripheral name (for example, @c RPI_PICO_RESETS_RESET_UART0
 * resets UART0). Each identifier is the bit position of the peripheral in the RP2040 RESETS
 * registers. Pass these identifiers directly to a @c resets property.
 *
 * @code{.dts}
 * #include <zephyr/dt-bindings/reset/rp2040_reset.h>
 *
 * &uart0 {
 *         // ...
 *         resets = <&reset RPI_PICO_RESETS_RESET_UART0>;
 *         // ...
 * };
 * @endcode
 * @{
 */

/** @cond INTERNAL_HIDDEN */

#define RPI_PICO_RESETS_RESET_ADC        0
#define RPI_PICO_RESETS_RESET_BUSCTRL    1
#define RPI_PICO_RESETS_RESET_DMA        2
#define RPI_PICO_RESETS_RESET_I2C0       3
#define RPI_PICO_RESETS_RESET_I2C1       4
#define RPI_PICO_RESETS_RESET_IO_BANK0   5
#define RPI_PICO_RESETS_RESET_IO_QSPI    6
#define RPI_PICO_RESETS_RESET_JTAG       7
#define RPI_PICO_RESETS_RESET_PADS_BANK0 8
#define RPI_PICO_RESETS_RESET_PADS_QSPI  9
#define RPI_PICO_RESETS_RESET_PIO0       10
#define RPI_PICO_RESETS_RESET_PIO1       11
#define RPI_PICO_RESETS_RESET_PLL_SYS    12
#define RPI_PICO_RESETS_RESET_PLL_USB    13
#define RPI_PICO_RESETS_RESET_PWM        14
#define RPI_PICO_RESETS_RESET_RTC        15
#define RPI_PICO_RESETS_RESET_SPI0       16
#define RPI_PICO_RESETS_RESET_SPI1       17
#define RPI_PICO_RESETS_RESET_SYSCFG     18
#define RPI_PICO_RESETS_RESET_SYSINFO    19
#define RPI_PICO_RESETS_RESET_TBMAN      20
#define RPI_PICO_RESETS_RESET_TIMER      21
#define RPI_PICO_RESETS_RESET_UART0      22
#define RPI_PICO_RESETS_RESET_UART1      23
#define RPI_PICO_RESETS_RESET_USBCTRL    24

/** @endcond */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_RP2040_RESET_H_ */
