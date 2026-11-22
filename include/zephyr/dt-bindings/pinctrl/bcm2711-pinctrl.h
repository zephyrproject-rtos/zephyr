/*
 * Copyright (c) 2025 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BCM2711_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BCM2711_PINCTRL_H_

/* BCM2711 Pin Functions */
#define BCM2711_FSEL_GPIO_IN  0
#define BCM2711_FSEL_GPIO_OUT 1
#define BCM2711_FSEL_ALT0     4
#define BCM2711_FSEL_ALT1     5
#define BCM2711_FSEL_ALT2     6
#define BCM2711_FSEL_ALT3     7
#define BCM2711_FSEL_ALT4     3
#define BCM2711_FSEL_ALT5     2

/* BCM2711 Pull Configuration */
#define BCM2711_PULL_NONE	0
#define BCM2711_PULL_UP		1
#define BCM2711_PULL_DOWN	2

/* BCM2711 pinmux configuration macro */
#define BCM2711_PINMUX(pin, func) \
	(((pin) & 0xFF) | (((func) & 0x7) << 8))

/* Extract pin number from BCM2711 pinmux value */
#define BCM2711_GET_PIN(pinmux) \
	((pinmux) & 0xFF)

/* Extract function from BCM2711 pinmux value */
#define BCM2711_GET_FUNC(pinmux) \
	(((pinmux) >> 8) & 0x7)

/* --- UART0 (PL011): TXD0 / RXD0 / CTS0 / RTS0 --- */
/** @brief UART0 transmit on GPIO 14 via ALT0. */
#define UART0_TX_GPIO14  BCM2711_PINMUX(14, BCM2711_FSEL_ALT0)
/** @brief UART0 receive on GPIO 15 via ALT0. */
#define UART0_RX_GPIO15  BCM2711_PINMUX(15, BCM2711_FSEL_ALT0)
/** @brief UART0 clear to send on GPIO 16 via ALT3. */
#define UART0_CTS_GPIO16 BCM2711_PINMUX(16, BCM2711_FSEL_ALT3)
/** @brief UART0 request to send on GPIO 17 via ALT3. */
#define UART0_RTS_GPIO17 BCM2711_PINMUX(17, BCM2711_FSEL_ALT3)
/** @brief UART0 clear to send on GPIO 30 via ALT3. */
#define UART0_CTS_GPIO30 BCM2711_PINMUX(30, BCM2711_FSEL_ALT3)
/** @brief UART0 request to send on GPIO 31 via ALT3. */
#define UART0_RTS_GPIO31 BCM2711_PINMUX(31, BCM2711_FSEL_ALT3)
/** @brief UART0 transmit on GPIO 32 via ALT3. */
#define UART0_TX_GPIO32  BCM2711_PINMUX(32, BCM2711_FSEL_ALT3)
/** @brief UART0 receive on GPIO 33 via ALT3. */
#define UART0_RX_GPIO33  BCM2711_PINMUX(33, BCM2711_FSEL_ALT3)
/** @brief UART0 transmit on GPIO 36 via ALT2. */
#define UART0_TX_GPIO36  BCM2711_PINMUX(36, BCM2711_FSEL_ALT2)
/** @brief UART0 receive on GPIO 37 via ALT2. */
#define UART0_RX_GPIO37  BCM2711_PINMUX(37, BCM2711_FSEL_ALT2)
/** @brief UART0 request to send on GPIO 38 via ALT2. */
#define UART0_RTS_GPIO38 BCM2711_PINMUX(38, BCM2711_FSEL_ALT2)
/** @brief UART0 clear to send on GPIO 39 via ALT2. */
#define UART0_CTS_GPIO39 BCM2711_PINMUX(39, BCM2711_FSEL_ALT2)

/* --- UART1 (mini UART): TXD1 / RXD1 / CTS1 / RTS1 --- */
/** @brief UART1 transmit on GPIO 14 via ALT5. */
#define UART1_TX_GPIO14  BCM2711_PINMUX(14, BCM2711_FSEL_ALT5)
/** @brief UART1 receive on GPIO 15 via ALT5. */
#define UART1_RX_GPIO15  BCM2711_PINMUX(15, BCM2711_FSEL_ALT5)
/** @brief UART1 clear to send on GPIO 16 via ALT5. */
#define UART1_CTS_GPIO16 BCM2711_PINMUX(16, BCM2711_FSEL_ALT5)
/** @brief UART1 request to send on GPIO 17 via ALT5. */
#define UART1_RTS_GPIO17 BCM2711_PINMUX(17, BCM2711_FSEL_ALT5)
/** @brief UART1 clear to send on GPIO 30 via ALT5. */
#define UART1_CTS_GPIO30 BCM2711_PINMUX(30, BCM2711_FSEL_ALT5)
/** @brief UART1 request to send on GPIO 31 via ALT5. */
#define UART1_RTS_GPIO31 BCM2711_PINMUX(31, BCM2711_FSEL_ALT5)
/** @brief UART1 transmit on GPIO 32 via ALT5. */
#define UART1_TX_GPIO32  BCM2711_PINMUX(32, BCM2711_FSEL_ALT5)
/** @brief UART1 receive on GPIO 33 via ALT5. */
#define UART1_RX_GPIO33  BCM2711_PINMUX(33, BCM2711_FSEL_ALT5)
/** @brief UART1 transmit on GPIO 40 via ALT5. */
#define UART1_TX_GPIO40  BCM2711_PINMUX(40, BCM2711_FSEL_ALT5)
/** @brief UART1 receive on GPIO 41 via ALT5. */
#define UART1_RX_GPIO41  BCM2711_PINMUX(41, BCM2711_FSEL_ALT5)
/** @brief UART1 request to send on GPIO 42 via ALT5. */
#define UART1_RTS_GPIO42 BCM2711_PINMUX(42, BCM2711_FSEL_ALT5)
/** @brief UART1 clear to send on GPIO 43 via ALT5. */
#define UART1_CTS_GPIO43 BCM2711_PINMUX(43, BCM2711_FSEL_ALT5)

/* --- UART2 (PL011): TXD2 / RXD2 / CTS2 / RTS2 --- */
/** @brief UART2 transmit on GPIO 0 via ALT4. */
#define UART2_TX_GPIO0  BCM2711_PINMUX(0, BCM2711_FSEL_ALT4)
/** @brief UART2 receive on GPIO 1 via ALT4. */
#define UART2_RX_GPIO1  BCM2711_PINMUX(1, BCM2711_FSEL_ALT4)
/** @brief UART2 clear to send on GPIO 2 via ALT4. */
#define UART2_CTS_GPIO2 BCM2711_PINMUX(2, BCM2711_FSEL_ALT4)
/** @brief UART2 request to send on GPIO 3 via ALT4. */
#define UART2_RTS_GPIO3 BCM2711_PINMUX(3, BCM2711_FSEL_ALT4)

/* --- UART3 (PL011): TXD3 / RXD3 / CTS3 / RTS3 --- */
/** @brief UART3 transmit on GPIO 4 via ALT4. */
#define UART3_TX_GPIO4  BCM2711_PINMUX(4, BCM2711_FSEL_ALT4)
/** @brief UART3 receive on GPIO 5 via ALT4. */
#define UART3_RX_GPIO5  BCM2711_PINMUX(5, BCM2711_FSEL_ALT4)
/** @brief UART3 clear to send on GPIO 6 via ALT4. */
#define UART3_CTS_GPIO6 BCM2711_PINMUX(6, BCM2711_FSEL_ALT4)
/** @brief UART3 request to send on GPIO 7 via ALT4. */
#define UART3_RTS_GPIO7 BCM2711_PINMUX(7, BCM2711_FSEL_ALT4)

/* --- UART4 (PL011): TXD4 / RXD4 / CTS4 / RTS4 --- */
/** @brief UART4 transmit on GPIO 8 via ALT4. */
#define UART4_TX_GPIO8   BCM2711_PINMUX(8, BCM2711_FSEL_ALT4)
/** @brief UART4 receive on GPIO 9 via ALT4. */
#define UART4_RX_GPIO9   BCM2711_PINMUX(9, BCM2711_FSEL_ALT4)
/** @brief UART4 clear to send on GPIO 10 via ALT4. */
#define UART4_CTS_GPIO10 BCM2711_PINMUX(10, BCM2711_FSEL_ALT4)
/** @brief UART4 request to send on GPIO 11 via ALT4. */
#define UART4_RTS_GPIO11 BCM2711_PINMUX(11, BCM2711_FSEL_ALT4)

/* --- UART5 (PL011): TXD5 / RXD5 / CTS5 / RTS5 --- */
/** @brief UART5 transmit on GPIO 12 via ALT4. */
#define UART5_TX_GPIO12  BCM2711_PINMUX(12, BCM2711_FSEL_ALT4)
/** @brief UART5 receive on GPIO 13 via ALT4. */
#define UART5_RX_GPIO13  BCM2711_PINMUX(13, BCM2711_FSEL_ALT4)
/** @brief UART5 clear to send on GPIO 14 via ALT4. */
#define UART5_CTS_GPIO14 BCM2711_PINMUX(14, BCM2711_FSEL_ALT4)
/** @brief UART5 request to send on GPIO 15 via ALT4. */
#define UART5_RTS_GPIO15 BCM2711_PINMUX(15, BCM2711_FSEL_ALT4)

/* --- I2C0 (BSC0): SDA0 / SCL0 --- */
/** @brief I2C0 data on GPIO 0 via ALT0. */
#define I2C0_SDA_GPIO0  BCM2711_PINMUX(0, BCM2711_FSEL_ALT0)
/** @brief I2C0 clock on GPIO 1 via ALT0. */
#define I2C0_SCL_GPIO1  BCM2711_PINMUX(1, BCM2711_FSEL_ALT0)
/** @brief I2C0 data on GPIO 28 via ALT0. */
#define I2C0_SDA_GPIO28 BCM2711_PINMUX(28, BCM2711_FSEL_ALT0)
/** @brief I2C0 clock on GPIO 29 via ALT0. */
#define I2C0_SCL_GPIO29 BCM2711_PINMUX(29, BCM2711_FSEL_ALT0)
/** @brief I2C0 data on GPIO 44 via ALT1. */
#define I2C0_SDA_GPIO44 BCM2711_PINMUX(44, BCM2711_FSEL_ALT1)
/** @brief I2C0 clock on GPIO 45 via ALT1. */
#define I2C0_SCL_GPIO45 BCM2711_PINMUX(45, BCM2711_FSEL_ALT1)
/** @brief I2C0 data on GPIO 46 via ALT0. */
#define I2C0_SDA_GPIO46 BCM2711_PINMUX(46, BCM2711_FSEL_ALT0)
/** @brief I2C0 clock on GPIO 47 via ALT0. */
#define I2C0_SCL_GPIO47 BCM2711_PINMUX(47, BCM2711_FSEL_ALT0)

/* --- I2C1 (BSC1): SDA1 / SCL1 --- */
/** @brief I2C1 data on GPIO 2 via ALT0. */
#define I2C1_SDA_GPIO2  BCM2711_PINMUX(2, BCM2711_FSEL_ALT0)
/** @brief I2C1 clock on GPIO 3 via ALT0. */
#define I2C1_SCL_GPIO3  BCM2711_PINMUX(3, BCM2711_FSEL_ALT0)
/** @brief I2C1 data on GPIO 44 via ALT2. */
#define I2C1_SDA_GPIO44 BCM2711_PINMUX(44, BCM2711_FSEL_ALT2)
/** @brief I2C1 clock on GPIO 45 via ALT2. */
#define I2C1_SCL_GPIO45 BCM2711_PINMUX(45, BCM2711_FSEL_ALT2)
/** @brief I2C1 data on GPIO 46 via ALT1. */
#define I2C1_SDA_GPIO46 BCM2711_PINMUX(46, BCM2711_FSEL_ALT1)
/** @brief I2C1 clock on GPIO 47 via ALT1. */
#define I2C1_SCL_GPIO47 BCM2711_PINMUX(47, BCM2711_FSEL_ALT1)

/* --- I2C3 (BSC3): SDA3 / SCL3 --- */
/** @brief I2C3 data on GPIO 2 via ALT5. */
#define I2C3_SDA_GPIO2 BCM2711_PINMUX(2, BCM2711_FSEL_ALT5)
/** @brief I2C3 clock on GPIO 3 via ALT5. */
#define I2C3_SCL_GPIO3 BCM2711_PINMUX(3, BCM2711_FSEL_ALT5)
/** @brief I2C3 data on GPIO 4 via ALT5. */
#define I2C3_SDA_GPIO4 BCM2711_PINMUX(4, BCM2711_FSEL_ALT5)
/** @brief I2C3 clock on GPIO 5 via ALT5. */
#define I2C3_SCL_GPIO5 BCM2711_PINMUX(5, BCM2711_FSEL_ALT5)

/* --- I2C4 (BSC4): SDA4 / SCL4 --- */
/** @brief I2C4 data on GPIO 6 via ALT5. */
#define I2C4_SDA_GPIO6 BCM2711_PINMUX(6, BCM2711_FSEL_ALT5)
/** @brief I2C4 clock on GPIO 7 via ALT5. */
#define I2C4_SCL_GPIO7 BCM2711_PINMUX(7, BCM2711_FSEL_ALT5)
/** @brief I2C4 data on GPIO 8 via ALT5. */
#define I2C4_SDA_GPIO8 BCM2711_PINMUX(8, BCM2711_FSEL_ALT5)
/** @brief I2C4 clock on GPIO 9 via ALT5. */
#define I2C4_SCL_GPIO9 BCM2711_PINMUX(9, BCM2711_FSEL_ALT5)

/* --- I2C5 (BSC5): SDA5 / SCL5 --- */
/** @brief I2C5 data on GPIO 10 via ALT5. */
#define I2C5_SDA_GPIO10 BCM2711_PINMUX(10, BCM2711_FSEL_ALT5)
/** @brief I2C5 clock on GPIO 11 via ALT5. */
#define I2C5_SCL_GPIO11 BCM2711_PINMUX(11, BCM2711_FSEL_ALT5)
/** @brief I2C5 data on GPIO 12 via ALT5. */
#define I2C5_SDA_GPIO12 BCM2711_PINMUX(12, BCM2711_FSEL_ALT5)
/** @brief I2C5 clock on GPIO 13 via ALT5. */
#define I2C5_SCL_GPIO13 BCM2711_PINMUX(13, BCM2711_FSEL_ALT5)

/* --- I2C6 (BSC6): SDA6 / SCL6 --- */
/** @brief I2C6 data on GPIO 0 via ALT5. */
#define I2C6_SDA_GPIO0  BCM2711_PINMUX(0, BCM2711_FSEL_ALT5)
/** @brief I2C6 clock on GPIO 1 via ALT5. */
#define I2C6_SCL_GPIO1  BCM2711_PINMUX(1, BCM2711_FSEL_ALT5)
/** @brief I2C6 data on GPIO 22 via ALT5. */
#define I2C6_SDA_GPIO22 BCM2711_PINMUX(22, BCM2711_FSEL_ALT5)
/** @brief I2C6 clock on GPIO 23 via ALT5. */
#define I2C6_SCL_GPIO23 BCM2711_PINMUX(23, BCM2711_FSEL_ALT5)

/* --- SPI0: SPI0_CE0_N..CE2_N / MISO / MOSI / SCLK --- */
/** @brief SPI0 chip select 1 on GPIO 7 via ALT0. */
#define SPI0_CE1_GPIO7   BCM2711_PINMUX(7, BCM2711_FSEL_ALT0)
/** @brief SPI0 chip select 0 on GPIO 8 via ALT0. */
#define SPI0_CE0_GPIO8   BCM2711_PINMUX(8, BCM2711_FSEL_ALT0)
/** @brief SPI0 MISO on GPIO 9 via ALT0. */
#define SPI0_MISO_GPIO9  BCM2711_PINMUX(9, BCM2711_FSEL_ALT0)
/** @brief SPI0 MOSI on GPIO 10 via ALT0. */
#define SPI0_MOSI_GPIO10 BCM2711_PINMUX(10, BCM2711_FSEL_ALT0)
/** @brief SPI0 clock on GPIO 11 via ALT0. */
#define SPI0_SCLK_GPIO11 BCM2711_PINMUX(11, BCM2711_FSEL_ALT0)
/** @brief SPI0 chip select 1 on GPIO 35 via ALT0. */
#define SPI0_CE1_GPIO35  BCM2711_PINMUX(35, BCM2711_FSEL_ALT0)
/** @brief SPI0 chip select 0 on GPIO 36 via ALT0. */
#define SPI0_CE0_GPIO36  BCM2711_PINMUX(36, BCM2711_FSEL_ALT0)
/** @brief SPI0 MISO on GPIO 37 via ALT0. */
#define SPI0_MISO_GPIO37 BCM2711_PINMUX(37, BCM2711_FSEL_ALT0)
/** @brief SPI0 MOSI on GPIO 38 via ALT0. */
#define SPI0_MOSI_GPIO38 BCM2711_PINMUX(38, BCM2711_FSEL_ALT0)
/** @brief SPI0 clock on GPIO 39 via ALT0. */
#define SPI0_SCLK_GPIO39 BCM2711_PINMUX(39, BCM2711_FSEL_ALT0)
/** @brief SPI0 MISO on GPIO 40 via ALT4. */
#define SPI0_MISO_GPIO40 BCM2711_PINMUX(40, BCM2711_FSEL_ALT4)
/** @brief SPI0 MOSI on GPIO 41 via ALT4. */
#define SPI0_MOSI_GPIO41 BCM2711_PINMUX(41, BCM2711_FSEL_ALT4)
/** @brief SPI0 clock on GPIO 42 via ALT4. */
#define SPI0_SCLK_GPIO42 BCM2711_PINMUX(42, BCM2711_FSEL_ALT4)
/** @brief SPI0 chip select 0 on GPIO 43 via ALT4. */
#define SPI0_CE0_GPIO43  BCM2711_PINMUX(43, BCM2711_FSEL_ALT4)
/** @brief SPI0 chip select 1 on GPIO 44 via ALT4. */
#define SPI0_CE1_GPIO44  BCM2711_PINMUX(44, BCM2711_FSEL_ALT4)
/** @brief SPI0 chip select 2 on GPIO 45 via ALT4. */
#define SPI0_CE2_GPIO45  BCM2711_PINMUX(45, BCM2711_FSEL_ALT4)
/** @brief SPI0 chip select 0 on GPIO 46 via ALT2. */
#define SPI0_CE0_GPIO46  BCM2711_PINMUX(46, BCM2711_FSEL_ALT2)
/** @brief SPI0 MISO on GPIO 47 via ALT2. */
#define SPI0_MISO_GPIO47 BCM2711_PINMUX(47, BCM2711_FSEL_ALT2)
/** @brief SPI0 MOSI on GPIO 48 via ALT2. */
#define SPI0_MOSI_GPIO48 BCM2711_PINMUX(48, BCM2711_FSEL_ALT2)
/** @brief SPI0 clock on GPIO 49 via ALT2. */
#define SPI0_SCLK_GPIO49 BCM2711_PINMUX(49, BCM2711_FSEL_ALT2)

/* --- SPI1 (auxiliary): SPI1_CE0_N..CE2_N / MISO / MOSI / SCLK --- */
/** @brief SPI1 chip select 2 on GPIO 16 via ALT4. */
#define SPI1_CE2_GPIO16  BCM2711_PINMUX(16, BCM2711_FSEL_ALT4)
/** @brief SPI1 chip select 1 on GPIO 17 via ALT4. */
#define SPI1_CE1_GPIO17  BCM2711_PINMUX(17, BCM2711_FSEL_ALT4)
/** @brief SPI1 chip select 0 on GPIO 18 via ALT4. */
#define SPI1_CE0_GPIO18  BCM2711_PINMUX(18, BCM2711_FSEL_ALT4)
/** @brief SPI1 MISO on GPIO 19 via ALT4. */
#define SPI1_MISO_GPIO19 BCM2711_PINMUX(19, BCM2711_FSEL_ALT4)
/** @brief SPI1 MOSI on GPIO 20 via ALT4. */
#define SPI1_MOSI_GPIO20 BCM2711_PINMUX(20, BCM2711_FSEL_ALT4)
/** @brief SPI1 clock on GPIO 21 via ALT4. */
#define SPI1_SCLK_GPIO21 BCM2711_PINMUX(21, BCM2711_FSEL_ALT4)

/* --- SPI2 (auxiliary): SPI2_CE0_N / CE1_N / MISO / MOSI / SCLK (GPIO46-50, ALT5) --- */
/** @brief SPI2 chip select 1 on GPIO 46 via ALT5. */
#define SPI2_CE1_GPIO46  BCM2711_PINMUX(46, BCM2711_FSEL_ALT5)
/** @brief SPI2 chip select 0 on GPIO 47 via ALT5. */
#define SPI2_CE0_GPIO47  BCM2711_PINMUX(47, BCM2711_FSEL_ALT5)
/** @brief SPI2 clock on GPIO 48 via ALT5. */
#define SPI2_SCLK_GPIO48 BCM2711_PINMUX(48, BCM2711_FSEL_ALT5)
/** @brief SPI2 MOSI on GPIO 49 via ALT5. */
#define SPI2_MOSI_GPIO49 BCM2711_PINMUX(49, BCM2711_FSEL_ALT5)
/** @brief SPI2 MISO on GPIO 50 via ALT5. */
#define SPI2_MISO_GPIO50 BCM2711_PINMUX(50, BCM2711_FSEL_ALT5)

/* --- SPI3: SPI3_CE0_N / CE1_N / MISO / MOSI / SCLK --- */
/** @brief SPI3 chip select 0 on GPIO 0 via ALT3. */
#define SPI3_CE0_GPIO0  BCM2711_PINMUX(0, BCM2711_FSEL_ALT3)
/** @brief SPI3 MISO on GPIO 1 via ALT3. */
#define SPI3_MISO_GPIO1 BCM2711_PINMUX(1, BCM2711_FSEL_ALT3)
/** @brief SPI3 MOSI on GPIO 2 via ALT3. */
#define SPI3_MOSI_GPIO2 BCM2711_PINMUX(2, BCM2711_FSEL_ALT3)
/** @brief SPI3 clock on GPIO 3 via ALT3. */
#define SPI3_SCLK_GPIO3 BCM2711_PINMUX(3, BCM2711_FSEL_ALT3)
/** @brief SPI3 chip select 1 on GPIO 24 via ALT5. */
#define SPI3_CE1_GPIO24 BCM2711_PINMUX(24, BCM2711_FSEL_ALT5)

/* --- SPI4: SPI4_CE0_N / CE1_N / MISO / MOSI / SCLK --- */
/** @brief SPI4 chip select 0 on GPIO 4 via ALT3. */
#define SPI4_CE0_GPIO4  BCM2711_PINMUX(4, BCM2711_FSEL_ALT3)
/** @brief SPI4 MISO on GPIO 5 via ALT3. */
#define SPI4_MISO_GPIO5 BCM2711_PINMUX(5, BCM2711_FSEL_ALT3)
/** @brief SPI4 MOSI on GPIO 6 via ALT3. */
#define SPI4_MOSI_GPIO6 BCM2711_PINMUX(6, BCM2711_FSEL_ALT3)
/** @brief SPI4 clock on GPIO 7 via ALT3. */
#define SPI4_SCLK_GPIO7 BCM2711_PINMUX(7, BCM2711_FSEL_ALT3)
/** @brief SPI4 chip select 1 on GPIO 25 via ALT5. */
#define SPI4_CE1_GPIO25 BCM2711_PINMUX(25, BCM2711_FSEL_ALT5)

/* --- SPI5: SPI5_CE0_N / CE1_N / MISO / MOSI / SCLK --- */
/** @brief SPI5 chip select 0 on GPIO 12 via ALT3. */
#define SPI5_CE0_GPIO12  BCM2711_PINMUX(12, BCM2711_FSEL_ALT3)
/** @brief SPI5 MISO on GPIO 13 via ALT3. */
#define SPI5_MISO_GPIO13 BCM2711_PINMUX(13, BCM2711_FSEL_ALT3)
/** @brief SPI5 MOSI on GPIO 14 via ALT3. */
#define SPI5_MOSI_GPIO14 BCM2711_PINMUX(14, BCM2711_FSEL_ALT3)
/** @brief SPI5 clock on GPIO 15 via ALT3. */
#define SPI5_SCLK_GPIO15 BCM2711_PINMUX(15, BCM2711_FSEL_ALT3)
/** @brief SPI5 chip select 1 on GPIO 26 via ALT5. */
#define SPI5_CE1_GPIO26  BCM2711_PINMUX(26, BCM2711_FSEL_ALT5)

/* --- SPI6: SPI6_CE0_N / CE1_N / MISO / MOSI / SCLK --- */
/** @brief SPI6 chip select 0 on GPIO 18 via ALT3. */
#define SPI6_CE0_GPIO18  BCM2711_PINMUX(18, BCM2711_FSEL_ALT3)
/** @brief SPI6 MISO on GPIO 19 via ALT3. */
#define SPI6_MISO_GPIO19 BCM2711_PINMUX(19, BCM2711_FSEL_ALT3)
/** @brief SPI6 MOSI on GPIO 20 via ALT3. */
#define SPI6_MOSI_GPIO20 BCM2711_PINMUX(20, BCM2711_FSEL_ALT3)
/** @brief SPI6 clock on GPIO 21 via ALT3. */
#define SPI6_SCLK_GPIO21 BCM2711_PINMUX(21, BCM2711_FSEL_ALT3)
/** @brief SPI6 chip select 1 on GPIO 27 via ALT5. */
#define SPI6_CE1_GPIO27  BCM2711_PINMUX(27, BCM2711_FSEL_ALT5)

/* --- PWM0: PWM0_0 / PWM0_1 --- */
/** @brief PWM0 channel 0 on GPIO 12 via ALT0. */
#define PWM0_0_GPIO12 BCM2711_PINMUX(12, BCM2711_FSEL_ALT0)
/** @brief PWM0 channel 1 on GPIO 13 via ALT0. */
#define PWM0_1_GPIO13 BCM2711_PINMUX(13, BCM2711_FSEL_ALT0)
/** @brief PWM0 channel 0 on GPIO 18 via ALT5. */
#define PWM0_0_GPIO18 BCM2711_PINMUX(18, BCM2711_FSEL_ALT5)
/** @brief PWM0 channel 1 on GPIO 19 via ALT5. */
#define PWM0_1_GPIO19 BCM2711_PINMUX(19, BCM2711_FSEL_ALT5)
/** @brief PWM0 channel 1 on GPIO 45 via ALT0. */
#define PWM0_1_GPIO45 BCM2711_PINMUX(45, BCM2711_FSEL_ALT0)
/** @brief PWM0 channel 0 on GPIO 52 via ALT1. */
#define PWM0_0_GPIO52 BCM2711_PINMUX(52, BCM2711_FSEL_ALT1)
/** @brief PWM0 channel 1 on GPIO 53 via ALT1. */
#define PWM0_1_GPIO53 BCM2711_PINMUX(53, BCM2711_FSEL_ALT1)

/* --- PWM1: PWM1_0 / PWM1_1 --- */
/** @brief PWM1 channel 0 on GPIO 40 via ALT0. */
#define PWM1_0_GPIO40 BCM2711_PINMUX(40, BCM2711_FSEL_ALT0)
/** @brief PWM1 channel 1 on GPIO 41 via ALT0. */
#define PWM1_1_GPIO41 BCM2711_PINMUX(41, BCM2711_FSEL_ALT0)

/* --- General purpose clocks: GPCLK0 / GPCLK1 / GPCLK2 --- */
/** @brief General purpose clock 0 on GPIO 4 via ALT0. */
#define GPCLK0_GPIO4  BCM2711_PINMUX(4, BCM2711_FSEL_ALT0)
/** @brief General purpose clock 1 on GPIO 5 via ALT0. */
#define GPCLK1_GPIO5  BCM2711_PINMUX(5, BCM2711_FSEL_ALT0)
/** @brief General purpose clock 2 on GPIO 6 via ALT0. */
#define GPCLK2_GPIO6  BCM2711_PINMUX(6, BCM2711_FSEL_ALT0)
/** @brief General purpose clock 0 on GPIO 20 via ALT5. */
#define GPCLK0_GPIO20 BCM2711_PINMUX(20, BCM2711_FSEL_ALT5)
/** @brief General purpose clock 1 on GPIO 21 via ALT5. */
#define GPCLK1_GPIO21 BCM2711_PINMUX(21, BCM2711_FSEL_ALT5)
/** @brief General purpose clock 0 on GPIO 32 via ALT0. */
#define GPCLK0_GPIO32 BCM2711_PINMUX(32, BCM2711_FSEL_ALT0)
/** @brief General purpose clock 0 on GPIO 34 via ALT0. */
#define GPCLK0_GPIO34 BCM2711_PINMUX(34, BCM2711_FSEL_ALT0)
/** @brief General purpose clock 1 on GPIO 42 via ALT0. */
#define GPCLK1_GPIO42 BCM2711_PINMUX(42, BCM2711_FSEL_ALT0)
/** @brief General purpose clock 2 on GPIO 43 via ALT0. */
#define GPCLK2_GPIO43 BCM2711_PINMUX(43, BCM2711_FSEL_ALT0)
/** @brief General purpose clock 1 on GPIO 44 via ALT0. */
#define GPCLK1_GPIO44 BCM2711_PINMUX(44, BCM2711_FSEL_ALT0)
/** @brief General purpose clock 0 on GPIO 49 via ALT1. */
#define GPCLK0_GPIO49 BCM2711_PINMUX(49, BCM2711_FSEL_ALT1)
/** @brief General purpose clock 1 on GPIO 50 via ALT1. */
#define GPCLK1_GPIO50 BCM2711_PINMUX(50, BCM2711_FSEL_ALT1)
/** @brief General purpose clock 2 on GPIO 51 via ALT1. */
#define GPCLK2_GPIO51 BCM2711_PINMUX(51, BCM2711_FSEL_ALT1)

/* --- PCM audio: PCM_CLK / PCM_FS / PCM_DIN / PCM_DOUT --- */
/** @brief PCM clock on GPIO 18 via ALT0. */
#define PCM_CLK_GPIO18  BCM2711_PINMUX(18, BCM2711_FSEL_ALT0)
/** @brief PCM frame sync on GPIO 19 via ALT0. */
#define PCM_FS_GPIO19   BCM2711_PINMUX(19, BCM2711_FSEL_ALT0)
/** @brief PCM data in on GPIO 20 via ALT0. */
#define PCM_DIN_GPIO20  BCM2711_PINMUX(20, BCM2711_FSEL_ALT0)
/** @brief PCM data out on GPIO 21 via ALT0. */
#define PCM_DOUT_GPIO21 BCM2711_PINMUX(21, BCM2711_FSEL_ALT0)
/** @brief PCM clock on GPIO 28 via ALT2. */
#define PCM_CLK_GPIO28  BCM2711_PINMUX(28, BCM2711_FSEL_ALT2)
/** @brief PCM frame sync on GPIO 29 via ALT2. */
#define PCM_FS_GPIO29   BCM2711_PINMUX(29, BCM2711_FSEL_ALT2)
/** @brief PCM data in on GPIO 30 via ALT2. */
#define PCM_DIN_GPIO30  BCM2711_PINMUX(30, BCM2711_FSEL_ALT2)
/** @brief PCM data out on GPIO 31 via ALT2. */
#define PCM_DOUT_GPIO31 BCM2711_PINMUX(31, BCM2711_FSEL_ALT2)
/** @brief PCM clock on GPIO 50 via ALT2. */
#define PCM_CLK_GPIO50  BCM2711_PINMUX(50, BCM2711_FSEL_ALT2)
/** @brief PCM frame sync on GPIO 51 via ALT2. */
#define PCM_FS_GPIO51   BCM2711_PINMUX(51, BCM2711_FSEL_ALT2)
/** @brief PCM data in on GPIO 52 via ALT2. */
#define PCM_DIN_GPIO52  BCM2711_PINMUX(52, BCM2711_FSEL_ALT2)
/** @brief PCM data out on GPIO 53 via ALT2. */
#define PCM_DOUT_GPIO53 BCM2711_PINMUX(53, BCM2711_FSEL_ALT2)

/* --- EMMC SD0: SD0_CLK / SD0_CMD / SD0_DAT0..3 (GPIO22-27, ALT0) --- */
/** @brief EMMC SD0 clock on GPIO 22 via ALT0. */
#define SD0_CLK_GPIO22  BCM2711_PINMUX(22, BCM2711_FSEL_ALT0)
/** @brief EMMC SD0 command on GPIO 23 via ALT0. */
#define SD0_CMD_GPIO23  BCM2711_PINMUX(23, BCM2711_FSEL_ALT0)
/** @brief EMMC SD0 data line 0 on GPIO 24 via ALT0. */
#define SD0_DAT0_GPIO24 BCM2711_PINMUX(24, BCM2711_FSEL_ALT0)
/** @brief EMMC SD0 data line 1 on GPIO 25 via ALT0. */
#define SD0_DAT1_GPIO25 BCM2711_PINMUX(25, BCM2711_FSEL_ALT0)
/** @brief EMMC SD0 data line 2 on GPIO 26 via ALT0. */
#define SD0_DAT2_GPIO26 BCM2711_PINMUX(26, BCM2711_FSEL_ALT0)
/** @brief EMMC SD0 data line 3 on GPIO 27 via ALT0. */
#define SD0_DAT3_GPIO27 BCM2711_PINMUX(27, BCM2711_FSEL_ALT0)

/* --- EMMC SD1: SD1_CLK / SD1_CMD / SD1_DAT0..7 --- */
/** @brief EMMC SD1 clock on GPIO 22 via ALT3. */
#define SD1_CLK_GPIO22  BCM2711_PINMUX(22, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 command on GPIO 23 via ALT3. */
#define SD1_CMD_GPIO23  BCM2711_PINMUX(23, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 data line 0 on GPIO 24 via ALT3. */
#define SD1_DAT0_GPIO24 BCM2711_PINMUX(24, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 data line 1 on GPIO 25 via ALT3. */
#define SD1_DAT1_GPIO25 BCM2711_PINMUX(25, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 data line 2 on GPIO 26 via ALT3. */
#define SD1_DAT2_GPIO26 BCM2711_PINMUX(26, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 data line 3 on GPIO 27 via ALT3. */
#define SD1_DAT3_GPIO27 BCM2711_PINMUX(27, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 clock on GPIO 34 via ALT3. */
#define SD1_CLK_GPIO34  BCM2711_PINMUX(34, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 command on GPIO 35 via ALT3. */
#define SD1_CMD_GPIO35  BCM2711_PINMUX(35, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 data line 0 on GPIO 36 via ALT3. */
#define SD1_DAT0_GPIO36 BCM2711_PINMUX(36, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 data line 1 on GPIO 37 via ALT3. */
#define SD1_DAT1_GPIO37 BCM2711_PINMUX(37, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 data line 2 on GPIO 38 via ALT3. */
#define SD1_DAT2_GPIO38 BCM2711_PINMUX(38, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 data line 3 on GPIO 39 via ALT3. */
#define SD1_DAT3_GPIO39 BCM2711_PINMUX(39, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 data line 4 on GPIO 40 via ALT3. */
#define SD1_DAT4_GPIO40 BCM2711_PINMUX(40, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 data line 5 on GPIO 41 via ALT3. */
#define SD1_DAT5_GPIO41 BCM2711_PINMUX(41, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 data line 6 on GPIO 42 via ALT3. */
#define SD1_DAT6_GPIO42 BCM2711_PINMUX(42, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 data line 7 on GPIO 43 via ALT3. */
#define SD1_DAT7_GPIO43 BCM2711_PINMUX(43, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 clock on GPIO 48 via ALT3. */
#define SD1_CLK_GPIO48  BCM2711_PINMUX(48, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 command on GPIO 49 via ALT3. */
#define SD1_CMD_GPIO49  BCM2711_PINMUX(49, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 data line 0 on GPIO 50 via ALT3. */
#define SD1_DAT0_GPIO50 BCM2711_PINMUX(50, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 data line 1 on GPIO 51 via ALT3. */
#define SD1_DAT1_GPIO51 BCM2711_PINMUX(51, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 data line 2 on GPIO 52 via ALT3. */
#define SD1_DAT2_GPIO52 BCM2711_PINMUX(52, BCM2711_FSEL_ALT3)
/** @brief EMMC SD1 data line 3 on GPIO 53 via ALT3. */
#define SD1_DAT3_GPIO53 BCM2711_PINMUX(53, BCM2711_FSEL_ALT3)

/* --- DPI display: PCLK / DE / LCD_VSYNC / LCD_HSYNC / DPI_D0..23 (GPIO0-27, ALT2) --- */
/** @brief DPI pixel clock on GPIO 0 via ALT2. */
#define DPI_PCLK_GPIO0  BCM2711_PINMUX(0, BCM2711_FSEL_ALT2)
/** @brief DPI data enable on GPIO 1 via ALT2. */
#define DPI_DE_GPIO1    BCM2711_PINMUX(1, BCM2711_FSEL_ALT2)
/** @brief DPI vertical sync on GPIO 2 via ALT2. */
#define DPI_VSYNC_GPIO2 BCM2711_PINMUX(2, BCM2711_FSEL_ALT2)
/** @brief DPI horizontal sync on GPIO 3 via ALT2. */
#define DPI_HSYNC_GPIO3 BCM2711_PINMUX(3, BCM2711_FSEL_ALT2)
/** @brief DPI data line 0 on GPIO 4 via ALT2. */
#define DPI_D0_GPIO4    BCM2711_PINMUX(4, BCM2711_FSEL_ALT2)
/** @brief DPI data line 1 on GPIO 5 via ALT2. */
#define DPI_D1_GPIO5    BCM2711_PINMUX(5, BCM2711_FSEL_ALT2)
/** @brief DPI data line 2 on GPIO 6 via ALT2. */
#define DPI_D2_GPIO6    BCM2711_PINMUX(6, BCM2711_FSEL_ALT2)
/** @brief DPI data line 3 on GPIO 7 via ALT2. */
#define DPI_D3_GPIO7    BCM2711_PINMUX(7, BCM2711_FSEL_ALT2)
/** @brief DPI data line 4 on GPIO 8 via ALT2. */
#define DPI_D4_GPIO8    BCM2711_PINMUX(8, BCM2711_FSEL_ALT2)
/** @brief DPI data line 5 on GPIO 9 via ALT2. */
#define DPI_D5_GPIO9    BCM2711_PINMUX(9, BCM2711_FSEL_ALT2)
/** @brief DPI data line 6 on GPIO 10 via ALT2. */
#define DPI_D6_GPIO10   BCM2711_PINMUX(10, BCM2711_FSEL_ALT2)
/** @brief DPI data line 7 on GPIO 11 via ALT2. */
#define DPI_D7_GPIO11   BCM2711_PINMUX(11, BCM2711_FSEL_ALT2)
/** @brief DPI data line 8 on GPIO 12 via ALT2. */
#define DPI_D8_GPIO12   BCM2711_PINMUX(12, BCM2711_FSEL_ALT2)
/** @brief DPI data line 9 on GPIO 13 via ALT2. */
#define DPI_D9_GPIO13   BCM2711_PINMUX(13, BCM2711_FSEL_ALT2)
/** @brief DPI data line 10 on GPIO 14 via ALT2. */
#define DPI_D10_GPIO14  BCM2711_PINMUX(14, BCM2711_FSEL_ALT2)
/** @brief DPI data line 11 on GPIO 15 via ALT2. */
#define DPI_D11_GPIO15  BCM2711_PINMUX(15, BCM2711_FSEL_ALT2)
/** @brief DPI data line 12 on GPIO 16 via ALT2. */
#define DPI_D12_GPIO16  BCM2711_PINMUX(16, BCM2711_FSEL_ALT2)
/** @brief DPI data line 13 on GPIO 17 via ALT2. */
#define DPI_D13_GPIO17  BCM2711_PINMUX(17, BCM2711_FSEL_ALT2)
/** @brief DPI data line 14 on GPIO 18 via ALT2. */
#define DPI_D14_GPIO18  BCM2711_PINMUX(18, BCM2711_FSEL_ALT2)
/** @brief DPI data line 15 on GPIO 19 via ALT2. */
#define DPI_D15_GPIO19  BCM2711_PINMUX(19, BCM2711_FSEL_ALT2)
/** @brief DPI data line 16 on GPIO 20 via ALT2. */
#define DPI_D16_GPIO20  BCM2711_PINMUX(20, BCM2711_FSEL_ALT2)
/** @brief DPI data line 17 on GPIO 21 via ALT2. */
#define DPI_D17_GPIO21  BCM2711_PINMUX(21, BCM2711_FSEL_ALT2)
/** @brief DPI data line 18 on GPIO 22 via ALT2. */
#define DPI_D18_GPIO22  BCM2711_PINMUX(22, BCM2711_FSEL_ALT2)
/** @brief DPI data line 19 on GPIO 23 via ALT2. */
#define DPI_D19_GPIO23  BCM2711_PINMUX(23, BCM2711_FSEL_ALT2)
/** @brief DPI data line 20 on GPIO 24 via ALT2. */
#define DPI_D20_GPIO24  BCM2711_PINMUX(24, BCM2711_FSEL_ALT2)
/** @brief DPI data line 21 on GPIO 25 via ALT2. */
#define DPI_D21_GPIO25  BCM2711_PINMUX(25, BCM2711_FSEL_ALT2)
/** @brief DPI data line 22 on GPIO 26 via ALT2. */
#define DPI_D22_GPIO26  BCM2711_PINMUX(26, BCM2711_FSEL_ALT2)
/** @brief DPI data line 23 on GPIO 27 via ALT2. */
#define DPI_D23_GPIO27  BCM2711_PINMUX(27, BCM2711_FSEL_ALT2)

/* --- Secondary memory interface: SA0..5 / SOE_N / SWE_N / SD0..17 --- */
/** @brief SMI address line 5 on GPIO 0 via ALT1. */
#define SMI_SA5_GPIO0    BCM2711_PINMUX(0, BCM2711_FSEL_ALT1)
/** @brief SMI address line 4 on GPIO 1 via ALT1. */
#define SMI_SA4_GPIO1    BCM2711_PINMUX(1, BCM2711_FSEL_ALT1)
/** @brief SMI address line 3 on GPIO 2 via ALT1. */
#define SMI_SA3_GPIO2    BCM2711_PINMUX(2, BCM2711_FSEL_ALT1)
/** @brief SMI address line 2 on GPIO 3 via ALT1. */
#define SMI_SA2_GPIO3    BCM2711_PINMUX(3, BCM2711_FSEL_ALT1)
/** @brief SMI address line 1 on GPIO 4 via ALT1. */
#define SMI_SA1_GPIO4    BCM2711_PINMUX(4, BCM2711_FSEL_ALT1)
/** @brief SMI address line 0 on GPIO 5 via ALT1. */
#define SMI_SA0_GPIO5    BCM2711_PINMUX(5, BCM2711_FSEL_ALT1)
/** @brief SMI output enable / SE on GPIO 6 via ALT1. */
#define SMI_SOE_N_GPIO6  BCM2711_PINMUX(6, BCM2711_FSEL_ALT1)
/** @brief SMI write enable / SRW_N on GPIO 7 via ALT1. */
#define SMI_SWE_N_GPIO7  BCM2711_PINMUX(7, BCM2711_FSEL_ALT1)
/** @brief SMI data line 0 on GPIO 8 via ALT1. */
#define SMI_SD0_GPIO8    BCM2711_PINMUX(8, BCM2711_FSEL_ALT1)
/** @brief SMI data line 1 on GPIO 9 via ALT1. */
#define SMI_SD1_GPIO9    BCM2711_PINMUX(9, BCM2711_FSEL_ALT1)
/** @brief SMI data line 2 on GPIO 10 via ALT1. */
#define SMI_SD2_GPIO10   BCM2711_PINMUX(10, BCM2711_FSEL_ALT1)
/** @brief SMI data line 3 on GPIO 11 via ALT1. */
#define SMI_SD3_GPIO11   BCM2711_PINMUX(11, BCM2711_FSEL_ALT1)
/** @brief SMI data line 4 on GPIO 12 via ALT1. */
#define SMI_SD4_GPIO12   BCM2711_PINMUX(12, BCM2711_FSEL_ALT1)
/** @brief SMI data line 5 on GPIO 13 via ALT1. */
#define SMI_SD5_GPIO13   BCM2711_PINMUX(13, BCM2711_FSEL_ALT1)
/** @brief SMI data line 6 on GPIO 14 via ALT1. */
#define SMI_SD6_GPIO14   BCM2711_PINMUX(14, BCM2711_FSEL_ALT1)
/** @brief SMI data line 7 on GPIO 15 via ALT1. */
#define SMI_SD7_GPIO15   BCM2711_PINMUX(15, BCM2711_FSEL_ALT1)
/** @brief SMI data line 8 on GPIO 16 via ALT1. */
#define SMI_SD8_GPIO16   BCM2711_PINMUX(16, BCM2711_FSEL_ALT1)
/** @brief SMI data line 9 on GPIO 17 via ALT1. */
#define SMI_SD9_GPIO17   BCM2711_PINMUX(17, BCM2711_FSEL_ALT1)
/** @brief SMI data line 10 on GPIO 18 via ALT1. */
#define SMI_SD10_GPIO18  BCM2711_PINMUX(18, BCM2711_FSEL_ALT1)
/** @brief SMI data line 11 on GPIO 19 via ALT1. */
#define SMI_SD11_GPIO19  BCM2711_PINMUX(19, BCM2711_FSEL_ALT1)
/** @brief SMI data line 12 on GPIO 20 via ALT1. */
#define SMI_SD12_GPIO20  BCM2711_PINMUX(20, BCM2711_FSEL_ALT1)
/** @brief SMI data line 13 on GPIO 21 via ALT1. */
#define SMI_SD13_GPIO21  BCM2711_PINMUX(21, BCM2711_FSEL_ALT1)
/** @brief SMI data line 14 on GPIO 22 via ALT1. */
#define SMI_SD14_GPIO22  BCM2711_PINMUX(22, BCM2711_FSEL_ALT1)
/** @brief SMI data line 15 on GPIO 23 via ALT1. */
#define SMI_SD15_GPIO23  BCM2711_PINMUX(23, BCM2711_FSEL_ALT1)
/** @brief SMI data line 16 on GPIO 24 via ALT1. */
#define SMI_SD16_GPIO24  BCM2711_PINMUX(24, BCM2711_FSEL_ALT1)
/** @brief SMI data line 17 on GPIO 25 via ALT1. */
#define SMI_SD17_GPIO25  BCM2711_PINMUX(25, BCM2711_FSEL_ALT1)
/** @brief SMI address line 5 on GPIO 28 via ALT1. */
#define SMI_SA5_GPIO28   BCM2711_PINMUX(28, BCM2711_FSEL_ALT1)
/** @brief SMI address line 4 on GPIO 29 via ALT1. */
#define SMI_SA4_GPIO29   BCM2711_PINMUX(29, BCM2711_FSEL_ALT1)
/** @brief SMI address line 3 on GPIO 30 via ALT1. */
#define SMI_SA3_GPIO30   BCM2711_PINMUX(30, BCM2711_FSEL_ALT1)
/** @brief SMI address line 2 on GPIO 31 via ALT1. */
#define SMI_SA2_GPIO31   BCM2711_PINMUX(31, BCM2711_FSEL_ALT1)
/** @brief SMI address line 1 on GPIO 32 via ALT1. */
#define SMI_SA1_GPIO32   BCM2711_PINMUX(32, BCM2711_FSEL_ALT1)
/** @brief SMI address line 0 on GPIO 33 via ALT1. */
#define SMI_SA0_GPIO33   BCM2711_PINMUX(33, BCM2711_FSEL_ALT1)
/** @brief SMI output enable / SE on GPIO 34 via ALT1. */
#define SMI_SOE_N_GPIO34 BCM2711_PINMUX(34, BCM2711_FSEL_ALT1)
/** @brief SMI write enable / SRW_N on GPIO 35 via ALT1. */
#define SMI_SWE_N_GPIO35 BCM2711_PINMUX(35, BCM2711_FSEL_ALT1)
/** @brief SMI data line 0 on GPIO 36 via ALT1. */
#define SMI_SD0_GPIO36   BCM2711_PINMUX(36, BCM2711_FSEL_ALT1)
/** @brief SMI data line 1 on GPIO 37 via ALT1. */
#define SMI_SD1_GPIO37   BCM2711_PINMUX(37, BCM2711_FSEL_ALT1)
/** @brief SMI data line 2 on GPIO 38 via ALT1. */
#define SMI_SD2_GPIO38   BCM2711_PINMUX(38, BCM2711_FSEL_ALT1)
/** @brief SMI data line 3 on GPIO 39 via ALT1. */
#define SMI_SD3_GPIO39   BCM2711_PINMUX(39, BCM2711_FSEL_ALT1)
/** @brief SMI data line 4 on GPIO 40 via ALT1. */
#define SMI_SD4_GPIO40   BCM2711_PINMUX(40, BCM2711_FSEL_ALT1)
/** @brief SMI data line 5 on GPIO 41 via ALT1. */
#define SMI_SD5_GPIO41   BCM2711_PINMUX(41, BCM2711_FSEL_ALT1)
/** @brief SMI data line 6 on GPIO 42 via ALT1. */
#define SMI_SD6_GPIO42   BCM2711_PINMUX(42, BCM2711_FSEL_ALT1)
/** @brief SMI data line 7 on GPIO 43 via ALT1. */
#define SMI_SD7_GPIO43   BCM2711_PINMUX(43, BCM2711_FSEL_ALT1)

/* --- BSC / SPI target: BSCSL CE_N / MISO / SDA_MOSI / SCL_SCLK (GPIO8-11, ALT3) --- */
/** @brief SPI target chip select on GPIO 8 via ALT3. */
#define BSCSL_CE_N_GPIO8      BCM2711_PINMUX(8, BCM2711_FSEL_ALT3)
/** @brief SPI target MISO on GPIO 9 via ALT3. */
#define BSCSL_MISO_GPIO9      BCM2711_PINMUX(9, BCM2711_FSEL_ALT3)
/** @brief I2C target data / SPI target MOSI on GPIO 10 via ALT3. */
#define BSCSL_SDA_MOSI_GPIO10 BCM2711_PINMUX(10, BCM2711_FSEL_ALT3)
/** @brief I2C target clock / SPI target clock on GPIO 11 via ALT3. */
#define BSCSL_SCL_SCLK_GPIO11 BCM2711_PINMUX(11, BCM2711_FSEL_ALT3)

/* --- ARM JTAG: TRST / RTCK / TDO / TCK / TDI / TMS (ALT4) --- */
/** @brief ARM JTAG reset on GPIO 22 via ALT4. */
#define ARM_TRST_GPIO22 BCM2711_PINMUX(22, BCM2711_FSEL_ALT4)
/** @brief ARM JTAG return clock on GPIO 23 via ALT4. */
#define ARM_RTCK_GPIO23 BCM2711_PINMUX(23, BCM2711_FSEL_ALT4)
/** @brief ARM JTAG data out on GPIO 24 via ALT4. */
#define ARM_TDO_GPIO24  BCM2711_PINMUX(24, BCM2711_FSEL_ALT4)
/** @brief ARM JTAG clock on GPIO 25 via ALT4. */
#define ARM_TCK_GPIO25  BCM2711_PINMUX(25, BCM2711_FSEL_ALT4)
/** @brief ARM JTAG data in on GPIO 26 via ALT4. */
#define ARM_TDI_GPIO26  BCM2711_PINMUX(26, BCM2711_FSEL_ALT4)
/** @brief ARM JTAG mode select on GPIO 27 via ALT4. */
#define ARM_TMS_GPIO27  BCM2711_PINMUX(27, BCM2711_FSEL_ALT4)
/** @brief ARM JTAG reset on GPIO 48 via ALT4. */
#define ARM_TRST_GPIO48 BCM2711_PINMUX(48, BCM2711_FSEL_ALT4)
/** @brief ARM JTAG return clock on GPIO 49 via ALT4. */
#define ARM_RTCK_GPIO49 BCM2711_PINMUX(49, BCM2711_FSEL_ALT4)
/** @brief ARM JTAG data out on GPIO 50 via ALT4. */
#define ARM_TDO_GPIO50  BCM2711_PINMUX(50, BCM2711_FSEL_ALT4)
/** @brief ARM JTAG clock on GPIO 51 via ALT4. */
#define ARM_TCK_GPIO51  BCM2711_PINMUX(51, BCM2711_FSEL_ALT4)
/** @brief ARM JTAG data in on GPIO 52 via ALT4. */
#define ARM_TDI_GPIO52  BCM2711_PINMUX(52, BCM2711_FSEL_ALT4)
/** @brief ARM JTAG mode select on GPIO 53 via ALT4. */
#define ARM_TMS_GPIO53  BCM2711_PINMUX(53, BCM2711_FSEL_ALT4)

/* --- Ethernet: RGMII / MII_A control signals --- */
/** @brief MII receive error on GPIO 28 via ALT4. */
#define MII_A_RX_ERR_GPIO28     BCM2711_PINMUX(28, BCM2711_FSEL_ALT4)
/** @brief RGMII MDIO on GPIO 28 via ALT5. */
#define RGMII_MDIO_GPIO28       BCM2711_PINMUX(28, BCM2711_FSEL_ALT5)
/** @brief MII transmit error on GPIO 29 via ALT4. */
#define MII_A_TX_ERR_GPIO29     BCM2711_PINMUX(29, BCM2711_FSEL_ALT4)
/** @brief RGMII MDC on GPIO 29 via ALT5. */
#define RGMII_MDC_GPIO29        BCM2711_PINMUX(29, BCM2711_FSEL_ALT5)
/** @brief MII carrier sense on GPIO 30 via ALT4. */
#define MII_A_CRS_GPIO30        BCM2711_PINMUX(30, BCM2711_FSEL_ALT4)
/** @brief MII collision on GPIO 31 via ALT4. */
#define MII_A_COL_GPIO31        BCM2711_PINMUX(31, BCM2711_FSEL_ALT4)
/** @brief RGMII interrupt on GPIO 34 via ALT5. */
#define RGMII_IRQ_GPIO34        BCM2711_PINMUX(34, BCM2711_FSEL_ALT5)
/** @brief RGMII start/stop on GPIO 35 via ALT4. */
#define RGMII_START_STOP_GPIO35 BCM2711_PINMUX(35, BCM2711_FSEL_ALT4)
/** @brief RGMII receive OK on GPIO 36 via ALT4. */
#define RGMII_RX_OK_GPIO36      BCM2711_PINMUX(36, BCM2711_FSEL_ALT4)
/** @brief MII receive error on GPIO 36 via ALT5. */
#define MII_A_RX_ERR_GPIO36     BCM2711_PINMUX(36, BCM2711_FSEL_ALT5)
/** @brief RGMII MDIO on GPIO 37 via ALT4. */
#define RGMII_MDIO_GPIO37       BCM2711_PINMUX(37, BCM2711_FSEL_ALT4)
/** @brief MII transmit error on GPIO 37 via ALT5. */
#define MII_A_TX_ERR_GPIO37     BCM2711_PINMUX(37, BCM2711_FSEL_ALT5)
/** @brief RGMII MDC on GPIO 38 via ALT4. */
#define RGMII_MDC_GPIO38        BCM2711_PINMUX(38, BCM2711_FSEL_ALT4)
/** @brief MII carrier sense on GPIO 38 via ALT5. */
#define MII_A_CRS_GPIO38        BCM2711_PINMUX(38, BCM2711_FSEL_ALT5)
/** @brief RGMII interrupt on GPIO 39 via ALT4. */
#define RGMII_IRQ_GPIO39        BCM2711_PINMUX(39, BCM2711_FSEL_ALT4)
/** @brief MII collision on GPIO 39 via ALT5. */
#define MII_A_COL_GPIO39        BCM2711_PINMUX(39, BCM2711_FSEL_ALT5)

/* --- SD card control: SD_CARD_PRES / WRPROT / LED / VOLT / PWR0 --- */
/** @brief SD card present on GPIO 32 via ALT4. */
#define SD_CARD_PRES_GPIO32   BCM2711_PINMUX(32, BCM2711_FSEL_ALT4)
/** @brief SD card write protect on GPIO 33 via ALT4. */
#define SD_CARD_WRPROT_GPIO33 BCM2711_PINMUX(33, BCM2711_FSEL_ALT4)
/** @brief SD card activity LED on GPIO 34 via ALT4. */
#define SD_CARD_LED_GPIO34    BCM2711_PINMUX(34, BCM2711_FSEL_ALT4)
/** @brief SD card voltage select on GPIO 44 via ALT5. */
#define SD_CARD_VOLT_GPIO44   BCM2711_PINMUX(44, BCM2711_FSEL_ALT5)
/** @brief SD card power 0 on GPIO 45 via ALT5. */
#define SD_CARD_PWR0_GPIO45   BCM2711_PINMUX(45, BCM2711_FSEL_ALT5)
/** @brief SD card activity LED on GPIO 51 via ALT5. */
#define SD_CARD_LED_GPIO51    BCM2711_PINMUX(51, BCM2711_FSEL_ALT5)

/* --- Legacy SDHost: SDCLK / SDCMD / SDDAT0..3 (GPIO48-53, ALT0) --- */
/** @brief SDHost clock on GPIO 48 via ALT0. */
#define SDHOST_CLK_GPIO48   BCM2711_PINMUX(48, BCM2711_FSEL_ALT0)
/** @brief SDHost command on GPIO 49 via ALT0. */
#define SDHOST_CMD_GPIO49   BCM2711_PINMUX(49, BCM2711_FSEL_ALT0)
/** @brief SDHost data line 0 on GPIO 50 via ALT0. */
#define SDHOST_DAT0_GPIO50  BCM2711_PINMUX(50, BCM2711_FSEL_ALT0)
/** @brief SDHost data line 1 on GPIO 51 via ALT0. */
#define SDHOST_DAT1_GPIO51  BCM2711_PINMUX(51, BCM2711_FSEL_ALT0)
/** @brief SDHost data line 2 on GPIO 52 via ALT0. */
#define SDHOST_DAT2_GPIO52  BCM2711_PINMUX(52, BCM2711_FSEL_ALT0)
/** @brief SDHost data line 3 on GPIO 53 via ALT0. */
#define SDHOST_DAT3_GPIO53  BCM2711_PINMUX(53, BCM2711_FSEL_ALT0)

/* --- Infrared receiver: IR_IN (GPIO54, ALT0) --- */
/** @brief Infrared receiver input on GPIO 54 via ALT0. */
#define IR_IN_GPIO54 BCM2711_PINMUX(54, BCM2711_FSEL_ALT0)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_BCM2711_PINCTRL_H_ */
