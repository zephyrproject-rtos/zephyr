/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RX pin function selector devicetree bindings
 *
 * This header provides pin control definitions for Renesas RX series
 * microcontrollers used in devicetree source files.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RX_H__
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RX_H__

/** @brief Port number bit position */
#define RX_PORT_NUM_POS  0
/** @brief Port number mask */
#define RX_PORT_NUM_MASK 0x1f

/** @brief Pin number bit position */
#define RX_PIN_NUM_POS  6
/** @brief Pin number mask */
#define RX_PIN_NUM_MASK 0xf

/** @brief Pin function selector: high impedance, JTAG/SWD */
#define RX_PSEL_HIZ_JTAG_SWD      0x00
/** @brief Pin function selector: RTC input capture */
#define RX_PSEL_RTCIC             0x00
/** @brief Pin function selector: GPT group 0 */
#define RX_PSEL_GPT0              0x01
/** @brief Pin function selector: GPT group 1 */
#define RX_PSEL_GPT1              0x02
/** @brief Pin function selector: GPT group 2 */
#define RX_PSEL_GPT2              0x03
/** @brief Pin function selector: GPT group 3 */
#define RX_PSEL_GPT3              0x04
/** @brief Pin function selector: TMR */
#define RX_PSEL_TMR               0x05
/** @brief Pin function selector: RTC */
#define RX_PSEL_RTC               0x06
/** @brief Pin function selector: CAC */
#define RX_PSEL_CAC               0x07
/** @brief Pin function selector: POEG */
#define RX_PSEL_POEG              0x08
/** @brief Pin function selector: ADC */
#define RX_PSEL_ADC               0x09
/** @brief Pin function selector: SCI channel 0 */
#define RX_PSEL_SCI_0             0x0A
/** @brief Pin function selector: SCI channel 2 */
#define RX_PSEL_SCI_2             0x0A
/** @brief Pin function selector: SCI channel 4 */
#define RX_PSEL_SCI_4             0x0A
/** @brief Pin function selector: SCI channel 6 */
#define RX_PSEL_SCI_6             0x0A
/** @brief Pin function selector: SCI channel 8 */
#define RX_PSEL_SCI_8             0x0A
/** @brief Pin function selector: SCI channel 10 */
#define RX_PSEL_SCI_10            0x0A
/** @brief Pin function selector: SCI channel 12 */
#define RX_PSEL_SCI_12            0x0A
/** @brief Pin function selector: SCI channel 0 CTS/DE */
#define RX_PSEL_SCI_0_CTS_DE      0x0B
/** @brief Pin function selector: SCI channel 2 CTS/DE */
#define RX_PSEL_SCI_2_CTS_DE      0x0B
/** @brief Pin function selector: SCI channel 4 CTS/DE */
#define RX_PSEL_SCI_4_CTS_DE      0x0B
/** @brief Pin function selector: SCI channel 6 CTS/DE */
#define RX_PSEL_SCI_6_CTS_DE      0x0B
/** @brief Pin function selector: SCI channel 8 CTS/DE */
#define RX_PSEL_SCI_8_CTS_DE      0x0B
/** @brief Pin function selector: SCI channel 10 CTS/DE */
#define RX_PSEL_SCI_10_CTS_DE     0x0B
/** @brief Pin function selector: SCI channel 12 CTS/DE */
#define RX_PSEL_SCI_12_CTS_DE     0x0B
/** @brief Pin function selector: SCI channel 0 HBS */
#define RX_PSEL_SCI_0_HBS         0x0C
/** @brief Pin function selector: SCI channel 2 HBS */
#define RX_PSEL_SCI_2_HBS         0x0C
/** @brief Pin function selector: SCI channel 4 HBS */
#define RX_PSEL_SCI_4_HBS         0x0C
/** @brief Pin function selector: SCI channel 6 HBS */
#define RX_PSEL_SCI_6_HBS         0x0C
/** @brief Pin function selector: SCI channel 8 HBS */
#define RX_PSEL_SCI_8_HBS         0x0C
/** @brief Pin function selector: SCI channel 10 HBS */
#define RX_PSEL_SCI_10_HBS        0x0C
/** @brief Pin function selector: SCI channel 12 HBS */
#define RX_PSEL_SCI_12_HBS        0x0C
/** @brief Pin function selector: SCI channel 1 */
#define RX_PSEL_SCI_1             0x0D
/** @brief Pin function selector: SCI channel 3 */
#define RX_PSEL_SCI_3             0x0D
/** @brief Pin function selector: SCI channel 5 */
#define RX_PSEL_SCI_5             0x0D
/** @brief Pin function selector: SCI channel 7 */
#define RX_PSEL_SCI_7             0x0D
/** @brief Pin function selector: SCI channel 9 */
#define RX_PSEL_SCI_9             0x0D
/** @brief Pin function selector: SCI channel 11 */
#define RX_PSEL_SCI_11            0x0D
/** @brief Pin function selector: SCI channel 1 CTS/DE */
#define RX_PSEL_SCI_1_CTS_DE      0x0E
/** @brief Pin function selector: SCI channel 3 CTS/DE */
#define RX_PSEL_SCI_3_CTS_DE      0x0E
/** @brief Pin function selector: SCI channel 5 CTS/DE */
#define RX_PSEL_SCI_5_CTS_DE      0x0E
/** @brief Pin function selector: SCI channel 7 CTS/DE */
#define RX_PSEL_SCI_7_CTS_DE      0x0E
/** @brief Pin function selector: SCI channel 9 CTS/DE */
#define RX_PSEL_SCI_9_CTS_DE      0x0E
/** @brief Pin function selector: SCI channel 11 CTS/DE */
#define RX_PSEL_SCI_11_CTS_DE     0x0E
/** @brief Pin function selector: SCI channel 1 HBS */
#define RX_PSEL_SCI_1_HBS         0x0F
/** @brief Pin function selector: SCI channel 3 HBS */
#define RX_PSEL_SCI_3_HBS         0x0F
/** @brief Pin function selector: SCI channel 5 HBS */
#define RX_PSEL_SCI_5_HBS         0x0F
/** @brief Pin function selector: SCI channel 7 HBS */
#define RX_PSEL_SCI_7_HBS         0x0F
/** @brief Pin function selector: SCI channel 9 HBS */
#define RX_PSEL_SCI_9_HBS         0x0F
/** @brief Pin function selector: SCI channel 11 HBS */
#define RX_PSEL_SCI_11_HBS        0x0F
/** @brief Pin function selector: SPI */
#define RX_PSEL_SPI               0x10
/** @brief Pin function selector: I3C */
#define RX_PSEL_I3C               0x11
/** @brief Pin function selector: CANFD */
#define RX_PSEL_CANFD             0x12
/** @brief Pin function selector: USB full-speed */
#define RX_PSEL_USBFS             0x13
/** @brief Pin function selector: USB high-speed */
#define RX_PSEL_USBHS             0x14
/** @brief Pin function selector: DSMIF */
#define RX_PSEL_DSMIF             0x15
/** @brief Pin function selector: PDMIF */
#define RX_PSEL_PDMIF             0x16
/** @brief Pin function selector: SSIE */
#define RX_PSEL_SSIE              0x17
/** @brief Pin function selector: PCIF */
#define RX_PSEL_PCIF              0x18
/** @brief Pin function selector: SDHI */
#define RX_PSEL_SDHI              0x19
/** @brief Pin function selector: xSPI channel 0, group 1A */
#define RX_PSEL_XSPI0_1A          0x1A
/** @brief Pin function selector: xSPI channel 0, group 1B */
#define RX_PSEL_XSPI0_1B          0x1B
/** @brief Pin function selector: clock output */
#define RX_PSEL_CLKOUT            0x1F
/** @brief Pin function selector: Ethernet RMAC GMII/MII */
#define RX_PSEL_ETH_RMAC_GMII_MII 0x20
/** @brief Pin function selector: Ethernet RMAC RMII */
#define RX_PSEL_ETH_RMAC_RMII     0x21
/** @brief Pin function selector: Ethernet RMAC RGMII */
#define RX_PSEL_ETH_RMAC_RGMII    0x22
/** @brief Pin function selector: Ethernet PTP */
#define RX_PSEL_ETH_PTP           0x23
/** @brief Pin function selector: Ethernet ESC port 0 */
#define RX_PSEL_ETH_ESC0          0x24
/** @brief Pin function selector: Ethernet ESC port 1 */
#define RX_PSEL_ETH_ESC1          0x25
/** @brief Pin function selector: external bus SDRAM, group 0 */
#define RX_PSEL_EXBUS_SDRAM_0     0x3D
/** @brief Pin function selector: external bus SDRAM, group 1 */
#define RX_PSEL_EXBUS_SDRAM_1     0x3E
/** @brief Pin function selector: PIO */
#define RX_PSEL_PIO               0x40

/** @brief Pin function selector bit position */
#define RX_PSEL_POS  11
/** @brief Pin function selector mask */
#define RX_PSEL_MASK 0x7f

/** @brief Mode bit position */
#define RX_MODE_POS  18
/** @brief Mode mask */
#define RX_MODE_MASK 0x1

/**
 * @brief Create a pin function selector configuration value
 *
 * @param psel Pin function selector value
 * @param port_num Port number
 * @param pin_num Pin number
 * @return Configured pin control value
 */
#define RX_PSEL(psel, port_num, pin_num)                                                           \
	(1 << RX_MODE_POS | psel << RX_PSEL_POS | port_num << RX_PORT_NUM_POS |                    \
	 pin_num << RX_PIN_NUM_POS)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RX_H__ */
