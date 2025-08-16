/*
 * Copyright (c) 2025 by Sven Hädrich <sven.haedrich@sevenlab.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DALI_LPC11U6X_H_
#define ZEPHYR_DRIVERS_DALI_LPC11U6X_H_

/*
 * register bit pattern definitions for the LPC11U6x platform.
 *
 *  see UM10732 - LPC11U6x User Manual for reference
 */

/* peripheral memory map */
#define LPC_I2C0_BASE       0x40000000UL
#define LPC_WWDT_BASE       0x40004000UL
#define LPC_USART0_BASE     0x40008000UL
#define LPC_CT16B0_BASE     0x4000C000UL
#define LPC_CT16B1_BASE     0x40010000UL
#define LPC_CT32B0_BASE     0x40014000UL
#define LPC_CT32B1_BASE     0x40018000UL
#define LPC_ADC_BASE        0x4001C000UL
#define LPC_I2C1_BASE       0x40020000UL
#define LPC_RTC_BASE        0x40024000UL
#define LPC_DMATRIGMUX_BASE 0x40028000UL
#define LPC_PMU_BASE        0x40038000UL
#define LPC_FLASHCTRL_BASE  0x4003C000UL
#define LPC_SSP0_BASE       0x40040000UL
#define LPC_IOCON_BASE      0x40044000UL
#define LPC_SYSCON_BASE     0x40048000UL
#define LPC_USART4_BASE     0x4004C000UL
#define LPC_SSP1_BASE       0x40058000UL
#define LPC_GINT0_BASE      0x4005C000UL
#define LPC_GINT1_BASE      0x40060000UL
#define LPC_USART1_BASE     0x4006C000UL
#define LPC_USART2_BASE     0x40070000UL
#define LPC_USART3_BASE     0x40074000UL
#define LPC_USB_BASE        0x40080000UL
#define LPC_CRC_BASE        0x50000000UL
#define LPC_DMA_BASE        0x50004000UL
#define LPC_DMA_CH_BASE     0x50004400UL
#define LPC_SCT0_BASE       0x5000C000UL
#define LPC_SCT1_BASE       0x5000E000UL
#define LPC_GPIO_PORT_BASE  0xA0000000UL
#define LPC_PINT_BASE       0xA0004000UL

/* 4.4 Register description*/
struct lpc_syscon { /*!< (@ 0x40048000) SYSCON Structure                                       */
	volatile uint32_t SYSMEMREMAP;      /*!< (@ 0x40048000) System memory remap */
	volatile uint32_t PRESETCTRL;       /*!< (@ 0x40048004) Peripheral reset control  */
	volatile uint32_t SYSPLLCTRL;       /*!< (@ 0x40048008) System PLL control  */
	volatile const uint32_t SYSPLLSTAT; /*!< (@ 0x4004800C) System PLL status   */
	volatile uint32_t USBPLLCTRL;       /*!< (@ 0x40048010) USB PLL control  */
	volatile const uint32_t USBPLLSTAT; /*!< (@ 0x40048014) USB PLL status   */
	volatile const uint32_t RESERVED0;
	volatile uint32_t
		RTCOSCCTRL; /*!< (@ 0x4004801C) Ultra-low power 32 kHz oscillator output control */
	volatile uint32_t SYSOSCCTRL; /*!< (@ 0x40048020) System oscillator control */
	volatile uint32_t WDTOSCCTRL; /*!< (@ 0x40048024) Watchdog oscillator control */
	volatile const uint32_t RESERVED1[2];
	volatile uint32_t SYSRSTSTAT; /*!< (@ 0x40048030) System reset status register */
	volatile const uint32_t RESERVED2[3];
	volatile uint32_t SYSPLLCLKSEL; /*!< (@ 0x40048040) System PLL clock source select */
	volatile uint32_t SYSPLLCLKUEN; /*!< (@ 0x40048044) System PLL clock source update enable */
	volatile uint32_t USBPLLCLKSEL; /*!< (@ 0x40048048) USB PLL clock source select */
	volatile uint32_t USBPLLCLKUEN; /*!< (@ 0x4004804C) USB PLL clock source update enable */
	volatile const uint32_t RESERVED3[8];
	volatile uint32_t MAINCLKSEL;   /*!< (@ 0x40048070) Main clock source select   */
	volatile uint32_t MAINCLKUEN;   /*!< (@ 0x40048074) Main clock source update enable   */
	volatile uint32_t SYSAHBCLKDIV; /*!< (@ 0x40048078) System clock divider */
	volatile const uint32_t RESERVED4;
	volatile uint32_t SYSAHBCLKCTRL; /*!< (@ 0x40048080) System clock control */
	volatile const uint32_t RESERVED5[4];
	volatile uint32_t SSP0CLKDIV;   /*!< (@ 0x40048094) SSP0 clock divider   */
	volatile uint32_t USART0CLKDIV; /*!< (@ 0x40048098) USART0 clock divider */
	volatile uint32_t SSP1CLKDIV;   /*!< (@ 0x4004809C) SSP1 clock divider   */
	volatile uint32_t FRGCLKDIV;    /*!< (@ 0x400480A0) Clock divider for the common fractional
					   baud rate generator of USART1 to USART4 */
	volatile const uint32_t RESERVED6[7];
	volatile uint32_t USBCLKSEL; /*!< (@ 0x400480C0) USB clock source select */
	volatile uint32_t USBCLKUEN; /*!< (@ 0x400480C4) USB clock source update enable */
	volatile uint32_t USBCLKDIV; /*!< (@ 0x400480C8) USB clock source divider */
	volatile const uint32_t RESERVED7[5];
	volatile uint32_t CLKOUTSEL; /*!< (@ 0x400480E0) CLKOUT clock source select */
	volatile uint32_t CLKOUTUEN; /*!< (@ 0x400480E4) CLKOUT clock source update enable */
	volatile uint32_t CLKOUTDIV; /*!< (@ 0x400480E8) CLKOUT clock divider */
	volatile const uint32_t RESERVED8;
	volatile uint32_t
		UARTFRGDIV; /*!< (@ 0x400480F0) USART fractional generator divider value */
	volatile uint32_t
		UARTFRGMULT; /*!< (@ 0x400480F4) USART fractional generator multiplier value */
	volatile const uint32_t RESERVED9;
	volatile uint32_t EXTTRACECMD; /*!< (@ 0x400480FC) External trace buffer command register */
	volatile const uint32_t pioPORCAP0; /*!< (@ 0x40048100) POR captured pio status 0   */
	volatile const uint32_t pioPORCAP1; /*!< (@ 0x40048104) POR captured pio status 1   */
	volatile const uint32_t pioPORCAP2; /*!< (@ 0x40048108) POR captured pio status 1   */
	volatile const uint32_t RESERVED10[10];
	volatile uint32_t IOCONCLKDIV6;  /*!< (@ 0x40048134) Peripheral clock 6 to the IOCON block
					    for programmable  glitch filter  */
	volatile uint32_t IOCONCLKDIV5;  /*!< (@ 0x40048138) Peripheral clock 5 to the IOCON block
					    for programmable  glitch filter  */
	volatile uint32_t IOCONCLKDIV4;  /*!< (@ 0x4004813C) Peripheral clock 4 to the IOCON block
					    for programmable  glitch filter  */
	volatile uint32_t IOCONCLKDIV3;  /*!< (@ 0x40048140) Peripheral clock 3 to the IOCON block
					    for programmable  glitch filter  */
	volatile uint32_t IOCONCLKDIV2;  /*!< (@ 0x40048144) Peripheral clock 2 to the IOCON block
					    for programmable  glitch filter  */
	volatile uint32_t IOCONCLKDIV1;  /*!< (@ 0x40048148) Peripheral clock 1 to the IOCON block
					    for programmable  glitch filter  */
	volatile uint32_t IOCONCLKDIV0;  /*!< (@ 0x4004814C) Peripheral clock 0 to the IOCON block
					    for programmable  glitch filter  */
	volatile uint32_t BODCTRL;       /*!< (@ 0x40048150) Brown-Out Detect       */
	volatile uint32_t SYSTCKCAL;     /*!< (@ 0x40048154) System tick counter calibration     */
	volatile uint32_t AHBMATRIXPRIO; /*!< (@ 0x40048158) AHB matrix priority configuration */
	volatile const uint32_t RESERVED11[5];
	volatile uint32_t IRQLATENCY; /*!< (@ 0x40048170) IRQ delay. Allows trade-off between
					 interrupt latency and determinism. */
	volatile uint32_t NMISRC;     /*!< (@ 0x40048174) NMI Source Control     */
	volatile uint32_t PINTSEL[8]; /*!< (@ 0x40048178) Gpio Pin Interrupt Select register 0 */
	volatile uint32_t USBCLKCTRL; /*!< (@ 0x40048198) USB clock control */
	volatile const uint32_t USBCLKST; /*!< (@ 0x4004819C) USB clock status    */
	volatile const uint32_t RESERVED12[25];
	volatile uint32_t
		STARTERP0; /*!< (@ 0x40048204) Start logic 0 interrupt wake-up enable register 0 */
	volatile const uint32_t RESERVED13[3];
	volatile uint32_t
		STARTERP1; /*!< (@ 0x40048214) Start logic 1 interrupt wake-up enable register 1 */
	volatile const uint32_t RESERVED14[6];
	volatile uint32_t PDSLEEPCFG; /*!< (@ 0x40048230) Power-down states in deep-sleep mode */
	volatile uint32_t
		PDAWAKECFG; /*!< (@ 0x40048234) Power-down states for wake-up from deep-sleep     */
	volatile uint32_t PDRUNCFG; /*!< (@ 0x40048238) Power configuration register */
	volatile const uint32_t RESERVED15[110];
	volatile const uint32_t DEVICE_ID; /*!< (@ 0x400483F4) Device ID */
};

#define LPC_SYSCON           ((struct lpc_syscon *)LPC_SYSCON_BASE)
#define SYSAHBCLKCTRL_IOCON  (1U << 16U)
#define SYSAHBCLKCTRL_Gpio   (1U << 6U)
#define SYSAHBCLKCTRL_CT32B0 (1U << 9U)
#define SYSAHBCLKCTRL_CT32B1 (1U << 10U)
#define SYSAHBCLKCTRL_SCT0_1 (1U << 31U)

/* 4.4.2 peripheral reset control register */
#define SSP0_RST_N (1U << 0U)
#define I2C0_RST_N (1U << 1U)
#define SSP1_RST_N (1U << 2U)
#define I2C1_RST_N (1U << 3U)
#define FRG_RST_N  (1U << 4U)
#define SCT0_RST_N (1U << 9U)
#define SCT1_RST_N (1U << 10U)

/* 6. I/O control */
struct lpc_io_control { /*!< (@ 0x40044000) IOCON Structure                                        */
	volatile uint32_t pio0_0;  /*!< (@ 0x40044000) I/O configuration for port pio0  */
	volatile uint32_t pioO0_1;  /*!< (@ 0x40044004) I/O configuration for port pio0  */
	volatile uint32_t pio0_2;  /*!< (@ 0x40044008) I/O configuration for port pio0  */
	volatile uint32_t pio0_3;  /*!< (@ 0x4004400C) I/O configuration for port pio0  */
	volatile uint32_t pio0_4;  /*!< (@ 0x40044010) I/O configuration for port pio0  */
	volatile uint32_t pio0_5;  /*!< (@ 0x40044014) I/O configuration for port pio0  */
	volatile uint32_t pio0_6;  /*!< (@ 0x40044018) I/O configuration for port pio0  */
	volatile uint32_t pio0_7;  /*!< (@ 0x4004401C) I/O configuration for port pio0  */
	volatile uint32_t pio0_8;  /*!< (@ 0x40044020) I/O configuration for port pio0  */
	volatile uint32_t pio0_9;  /*!< (@ 0x40044024) I/O configuration for port pio0  */
	volatile uint32_t pio0_10; /*!< (@ 0x40044028) I/O configuration for port pio0 */
	volatile uint32_t pio0_11; /*!< (@ 0x4004402C) I/O configuration for port pio0 */
	volatile uint32_t pio0_12; /*!< (@ 0x40044030) I/O configuration for port pio0 */
	volatile uint32_t pio0_13; /*!< (@ 0x40044034) I/O configuration for port pio0 */
	volatile uint32_t pio0_14; /*!< (@ 0x40044038) I/O configuration for port pio0 */
	volatile uint32_t pio0_15; /*!< (@ 0x4004403C) I/O configuration for port pio0 */
	volatile uint32_t pio0_16; /*!< (@ 0x40044040) I/O configuration for port pio0 */
	volatile uint32_t pio0_17; /*!< (@ 0x40044044) I/O configuration for port pio0 */
	volatile uint32_t pio0_18; /*!< (@ 0x40044048) I/O configuration for port pio0 */
	volatile uint32_t pio0_19; /*!< (@ 0x4004404C) I/O configuration for port pio0 */
	volatile uint32_t pio0_20; /*!< (@ 0x40044050) I/O configuration for port pio0 */
	volatile uint32_t pio0_21; /*!< (@ 0x40044054) I/O configuration for port pio0 */
	volatile uint32_t pio0_22; /*!< (@ 0x40044058) I/O configuration for port pio0 */
	volatile uint32_t pio0_23; /*!< (@ 0x4004405C) I/O configuration for port pio0 */
	volatile uint32_t pio1_0;  /*!< (@ 0x40044060) I/O configuration for port pio1  */
	volatile uint32_t pio1_1;  /*!< (@ 0x40044064) I/O configuration for port pio1  */
	volatile uint32_t pio1_2;  /*!< (@ 0x40044068) I/O configuration for port pio1  */
	volatile uint32_t pio1_3;  /*!< (@ 0x4004406C) I/O configuration for port pio1  */
	volatile uint32_t pio1_4;  /*!< (@ 0x40044070) I/O configuration for port pio1  */
	volatile uint32_t pio1_5;  /*!< (@ 0x40044074) I/O configuration for port pio1  */
	volatile uint32_t pio1_6;  /*!< (@ 0x40044078) I/O configuration for port pio1  */
	volatile uint32_t pio1_7;  /*!< (@ 0x4004407C) I/O configuration for port pio1  */
	volatile uint32_t pio1_8;  /*!< (@ 0x40044080) I/O configuration for port pio1  */
	volatile uint32_t pio1_9;  /*!< (@ 0x40044084) I/O configuration for port pio1  */
	volatile uint32_t pio1_10; /*!< (@ 0x40044088) I/O configuration for port pio1 */
	volatile uint32_t pio1_11; /*!< (@ 0x4004408C) I/O configuration for port pio1 */
	volatile uint32_t pio1_12; /*!< (@ 0x40044090) I/O configuration for port pio1 */
	volatile uint32_t pio1_13; /*!< (@ 0x40044094) I/O configuration for port pio1 */
	volatile uint32_t pio1_14; /*!< (@ 0x40044098) I/O configuration for port pio1 */
	volatile uint32_t pio1_15; /*!< (@ 0x4004409C) I/O configuration for port pio1 */
	volatile uint32_t pio1_16; /*!< (@ 0x400440A0) I/O configuration for port pio1 */
	volatile uint32_t pio1_17; /*!< (@ 0x400440A4) I/O configuration for port pio1 */
	volatile uint32_t pio1_18; /*!< (@ 0x400440A8) I/O configuration for port pio1 */
	volatile uint32_t pio1_19; /*!< (@ 0x400440AC) I/O configuration for port pio1 */
	volatile uint32_t pio1_20; /*!< (@ 0x400440B0) I/O configuration for port pio1 */
	volatile uint32_t pio1_21; /*!< (@ 0x400440B4) I/O configuration for port pio1 */
	volatile uint32_t pio1_22; /*!< (@ 0x400440B8) I/O configuration for port pio1 */
	volatile uint32_t pio1_23; /*!< (@ 0x400440BC) I/O configuration for port pio1 */
	volatile uint32_t pio1_24; /*!< (@ 0x400440C0) I/O configuration for port pio1 */
	volatile uint32_t pio1_25; /*!< (@ 0x400440C4) I/O configuration for port pio1 */
	volatile uint32_t pio1_26; /*!< (@ 0x400440C8) I/O configuration for port pio1 */
	volatile uint32_t pio1_27; /*!< (@ 0x400440CC) I/O configuration for port pio1 */
	volatile uint32_t pio1_28; /*!< (@ 0x400440D0) I/O configuration for port pio1 */
	volatile uint32_t pio1_29; /*!< (@ 0x400440D4) I/O configuration for port pio1 */
	volatile uint32_t pio1_30; /*!< (@ 0x400440D8) I/O configuration for port pio1 */
	volatile uint32_t pio1_31; /*!< (@ 0x400440DC) I/O configuration for port pio1 */
	volatile const uint32_t reserved0[4];
	volatile uint32_t pio2_0; /*!< (@ 0x400440F0) I/O configuration for port pio2 */
	volatile uint32_t pio2_1; /*!< (@ 0x400440F4) I/O configuration for port pio2 */
	volatile const uint32_t reserved1;
	volatile uint32_t pio2_2;  /*!< (@ 0x400440FC) I/O configuration for port pio2  */
	volatile uint32_t pio2_3;  /*!< (@ 0x40044100) I/O configuration for port pio2  */
	volatile uint32_t pio2_4;  /*!< (@ 0x40044104) I/O configuration for port pio2  */
	volatile uint32_t pio2_5;  /*!< (@ 0x40044108) I/O configuration for port pio2  */
	volatile uint32_t pio2_6;  /*!< (@ 0x4004410C) I/O configuration for port pio2  */
	volatile uint32_t pio2_7;  /*!< (@ 0x40044110) I/O configuration for port pio2  */
	volatile uint32_t pio2_8;  /*!< (@ 0x40044114) I/O configuration for port pio2  */
	volatile uint32_t pio2_9;  /*!< (@ 0x40044118) I/O configuration for port pio2  */
	volatile uint32_t pio2_10; /*!< (@ 0x4004411C) I/O configuration for port pio2 */
	volatile uint32_t pio2_11; /*!< (@ 0x40044120) I/O configuration for port pio2 */
	volatile uint32_t pio2_12; /*!< (@ 0x40044124) I/O configuration for port pio2 */
	volatile uint32_t pio2_13; /*!< (@ 0x40044128) I/O configuration for port pio2 */
	volatile uint32_t pio2_14; /*!< (@ 0x4004412C) I/O configuration for port pio2 */
	volatile uint32_t pio2_15; /*!< (@ 0x40044130) I/O configuration for port pio2 */
	volatile uint32_t pio2_16; /*!< (@ 0x40044134) I/O configuration for port pio2 */
	volatile uint32_t pio2_17; /*!< (@ 0x40044138) I/O configuration for port pio2 */
	volatile uint32_t pio2_18; /*!< (@ 0x4004413C) I/O configuration for port pio2 */
	volatile uint32_t pio2_19; /*!< (@ 0x40044140) I/O configuration for port pio2 */
	volatile uint32_t pio2_20; /*!< (@ 0x40044144) I/O configuration for port pio2 */
	volatile uint32_t pio2_21; /*!< (@ 0x40044148) I/O configuration for port pio2 */
	volatile uint32_t pio2_22; /*!< (@ 0x4004414C) I/O configuration for port pio2 */
	volatile uint32_t pio2_23; /*!< (@ 0x40044150) I/O configuration for port pio2 */
};
#define LPC_IOCON ((struct lpc_io_control *)LPC_IOCON_BASE)

/* 6.5.2 pin control registers */
#define IOCON_DAPIN_FUNC_MASK     (7U)
#define IOCON_DAPIN_FUNC_SHIFT    (0U)
#define IOCON_DAPIN_MODE_MASK     (3U)
#define IOCON_DAPIN_MODE_SHIFT    (3U)
#define IOCON_DAPIN_MODE_INACTIVE (0U)
#define IOCON_DAPIN_MODE_PULLDOWN (1U)
#define IOCON_DAPIN_MODE_PULLUP   (2U)
#define IOCON_DAPIN_MODE_REPEATER (0U)
#define IOCON_DAPIN_HYS           (1U << 5U)
#define IOCON_DAPIN_INV           (1U << 6U)
#define IOCON_DAPIN_ADMODE        (1U << 7U)
#define IOCON_DAPIN_SMODE_MASK    (3U)
#define IOCON_DAPIN_SMODE_SHIFT   (11U)
#define IOCON_DAPIN_SMODE_BYPASS  (0U)
#define IOCON_DAPIN_CLKDIV_MASK   (7U)
#define IOCON_DAPIN_CLKDIV_SHIFT  (13U)

#define IOCON_DPIN_FUNC_MASK     (7U)
#define IOCON_DPIN_FUNC_SHIFT    (0U)
#define IOCON_DPIN_MODE_MASK     (3U)
#define IOCON_DPIN_MODE_SHIFT    (3U)
#define IOCON_DPIN_MODE_INACTIVE (0U)
#define IOCON_DPIN_MODE_PULLDOWN (1U)
#define IOCON_DPIN_MODE_PULLUP   (2U)
#define IOCON_DPIN_MODE_REPEATER (0U)
#define IOCON_DPIN_HYS           (1U << 5U)
#define IOCON_DPIN_INV           (1U << 6U)

/* 7. General purpose i/o register */
struct lpc_gpio_port {                /*!< (@ 0xA0000000) Gpio_PORT Structure */
	volatile uint8_t byte[88]; /*!< (@ 0xA0000000) Byte pin registers */
	volatile const uint32_t reserved0[42];
	volatile uint32_t word[88]; /*!< (@ 0xA0000100) Word pin registers */
	volatile const uint32_t reserved1[1896];
	volatile uint32_t dir[3]; /*!< (@ 0xA0002000) Port Direction registers */
	volatile const uint32_t reserved2[29];
	volatile uint32_t mask[3]; /*!< (@ 0xA0002080) Port Mask register */
	volatile const uint32_t reserved3[29];
	volatile uint32_t pin[3]; /*!< (@ 0xA0002100) Port pin register */
	volatile const uint32_t reserved4[29];
	volatile uint32_t mpin[3]; /*!< (@ 0xA0002180) Masked port register */
	volatile const uint32_t reserved5[29];
	volatile uint32_t
		set[3]; /*!< (@ 0xA0002200) Write: Set port register Read: port output bits */
	volatile const uint32_t reserved6[29];
	volatile uint32_t clr[3]; /*!< (@ 0xA0002280) Clear port */
	volatile const uint32_t reserved7[29];
	volatile uint32_t not[3]; /*!< (@ 0xA0002300) Toggle port */
};

#define LPC_GPIO_PORT ((struct lpc_gpio_port  *)LPC_GPIO_PORT_BASE)

/* 18.6.1 configuration register */
#define SCT_CONFIG_UNIFY       (1U << 0U)
#define SCT_CONFIG_AUTOLIMIT_L (1U << 17U)
#define SCT_CONFIG_AUTOLIMIT_H (1U << 18U)

/* 18.6.2 control register */
#define SCT_CTRL_DOWN_L      (1U << 0U)
#define SCT_CTRL_STOP_L      (1U << 1U)
#define SCT_CTRL_HALT_L      (1U << 2U)
#define SCT_CTRL_CLRCNTR_L   (1U << 3U)
#define SCT_CTRL_BIDIR_L     (1U << 4U)
#define SCT_CTRL_PRE_L_MASK  (0x7f)
#define SCT_CTRL_PRE_L_SHIFT (5U)
#define SCT_CTRL_DOWN_H      (1U << 16U)
#define SCT_CTRL_STOP_H      (1U << 17U)
#define SCT_CTRL_HALT_H      (1U << 18U)
#define SCT_CTRL_CLRCNTR_H   (1U << 19U)
#define SCT_CTRL_BIDIR_H     (1U << 20U)
#define SCT_CTRL_PRE_H_MASK  (0x7f)
#define SCT_CTRL_PRE_H_SHIFT (21U)

/* 18.6.11 output register */
#define SCT_OUTPUT_OUT0 (1U << 0U)
#define SCT_OUTPUT_OUT1 (1U << 1U)
#define SCT_OUTPUT_OUT2 (1U << 2U)
#define SCT_OUTPUT_OUT3 (1U << 3U)

/* 18.6.23 event state register */
#define SCT_EV_STATE_MASK0 (1U << 0U)
#define SCT_EV_STATE_MASK1 (1U << 1U)
#define SCT_EV_STATE_MASK2 (1U << 2U)
#define SCT_EV_STATE_MASK3 (1U << 3U)
#define SCT_EV_STATE_MASK4 (1U << 4U)
#define SCT_EV_STATE_MASK5 (1U << 5U)
#define SCT_EV_STATE_MASK6 (1U << 6U)

/* 18.6.24 sct event control register */
#define SCT_EV_CTRL_COMBMODE_OR    (0U << 12U)
#define SCT_EV_CTRL_COMBMODE_MATCH (1U << 12U)
#define SCT_EV_CTRL_COMBMODE_IO    (2U << 12U)
#define SCT_EV_CTRL_COMBMODE_AND   (3U << 12U)
#define SCT_EV_CTRL_OUTSEL         (1U << 5U)
#define SCT_EV_CTRL_IOSEL_SHIFT    (6U)
#define SCT_EV_CTRL_MATCHMEM       (1U << 20U)

/* 32-bit counters/timers */
struct lpc_ct32 { /*!< (@ 0x40014000) CT32B0 Structure                                     */
	volatile uint32_t ir; /*!< (@ 0x40014000) Interrupt Register. The IR can be written to    */
	/* clear interrupts. The IR can be read to identify which of eight possible interrupt */
	/* sources are pending.  */
	volatile uint32_t tcr; /*!< (@ 0x40014004) Timer Control Register. The TCR is used to */
	/* control the Timer Counter functions. The Timer Counter can be disabled or reset */
	/* through the TCR. */
	volatile uint32_t tc; /*!< (@ 0x40014008) Timer Counter. The 32-bit TC is incremented */
	/* every PR+1 cycles of PCLK. The TC is controlled through the TCR.  */
	volatile uint32_t pr; /*!< (@ 0x4001400C) Prescale Register. When the Prescale Counter */
	/* (below) is equal to this value, the next clock increments the TC and clears the PC.*/
	volatile uint32_t pc; /*!< (@ 0x40014010) Prescale Counter. The 32-bit PC is a counter */
	/* which is incremented to the value stored in PR. When the value in PR is reached, */
	/* the TC is incremented and the PC is cleared. The PC is observable and */
	/* controllable through the bus interface.  */
	volatile uint32_t mcr; /*!< (@ 0x40014014) Match Control Register. The MCR is used to */
	/* control if an interrupt is generated and if the TC is reset when a Match occurs. */
	volatile uint32_t mr0; /*!< (@ 0x40014018) Match Register 0. MR0 can be enabled through */
	/* the MCR to reset the TC, stop both the TC and PC, and/or generate an interrupt */
	/* every time MR0 matches the TC. */
	volatile uint32_t mr1; /*!< (@ 0x4001401C) Match Register 0. MR0 can be enabled through */
	/* the MCR to reset the TC, stop both the TC and PC, and/or generate an interrupt */
	/* every time MR0 matches the TC. */
	volatile uint32_t mr2; /*!< (@ 0x40014020) Match Register 0. MR0 can be enabled through */
	/* the MCR to reset the TC, stop both the TC and PC, and/or generate an interrupt */
	/* every time MR0 matches the TC. */
	volatile uint32_t mr3; /*!< (@ 0x40014024) Match Register 0. MR0 can be enabled through */
	/* the MCR to reset the TC, stop both the TC and PC, and/or generate an interrupt */
	/* every time MR0 matches the TC. */
	volatile uint32_t ccr; /*!< (@ 0x40014028) Capture Control Register. The CCR controls */
	/* which edges of the capture inputs are used to load the Capture Registers and */
	/* whether or not an interrupt is generated when a capture takes place. */
	volatile const uint32_t
		cr0; /*!< (@ 0x4001402C) Capture Register 0. CR0 is loaded with the value */
	/* of TC when there is an event on the CT32B0_CAP0 input.  */
	volatile const uint32_t reserved0;
	volatile uint32_t
		cr1; /*!< (@ 0x40014034) Capture Register 1. CR1 is loaded with the value */
	/* of TC when there is an event on the CT32B0_CAP1 input. */
	volatile const uint32_t reserved1;
	volatile uint32_t emr; /*!< (@ 0x4001403C) External Match Register. The EMR controls the */
	/* match function and the external match pins CT32Bn_MAT[3:0]. */
	volatile const uint32_t reserved2[12];
	volatile uint32_t
		ctcr; /*!< (@ 0x40014070) Count Control Register. The CTCR selects between */
	/* Timer and Counter mode, and in Counter mode selects the signal and edge(s) for */
	/* counting. */
	volatile uint32_t pwmc; /*!< (@ 0x40014074) PWM Control Register. The PWMCON enables PWM */
				/* mode for the external match pins CT32Bn_MAT[3:0]. */
};
#define LPC_CT32B0 ((struct lpc_ct32 *)LPC_CT32B0_BASE)
#define LPC_CT32B1 ((struct lpc_ct32 *)LPC_CT32B1_BASE)

/* 20.6.1 interrupt register */
#define CT32_IR_MR0INT (1U << 0U)
#define CT32_IR_MR1INT (1U << 1U)
#define CT32_IR_MR2INT (1U << 2U)
#define CT32_IR_MR3INT (1U << 3U)
#define CT32_IR_CR0INT (1U << 4U)
#define CT32_IR_CR1INT (1U << 5U)
#define CT32_IR_CR2INT (1U << 6U)

/* 20.6.6 match control register */
#define CT32_MCR_MR0I (1U << 0U)
#define CT32_MCR_MR0R (1U << 1U)
#define CT32_MCR_MR0S (1U << 2U)
#define CT32_MCR_MR1I (1U << 3U)
#define CT32_MCR_MR1R (1U << 4U)
#define CT32_MCR_MR1S (1U << 5U)
#define CT32_MCR_MR2I (1U << 6U)
#define CT32_MCR_MR2R (1U << 7U)
#define CT32_MCR_MR2S (1U << 8U)
#define CT32_MCR_MR3I (1U << 9U)
#define CT32_MCR_MR3R (1U << 10U)
#define CT32_MCR_MR3S (1U << 11U)

/* 20.6.8 capture control register */
#define CT32_CCR_CAP0RE (1U << 0U)
#define CT32_CCR_CAP0FE (1U << 1U)
#define CT32_CCR_CAP0I  (1U << 2U)
#define CT32_CCR_CAP1RE (1U << 3U)
#define CT32_CCR_CAP1FE (1U << 4U)
#define CT32_CCR_CAP1I  (1U << 5U)
#define CT32_CCR_CAP2RE (1U << 6U)
#define CT32_CCR_CAP2FE (1U << 7U)
#define CT32_CCR_CAP2I  (1U << 8U)

/* 20.6.10 external match register */
#define CT32_EMR_EM0           (1U << 0U)
#define CT32_EMR_EM1           (1U << 1U)
#define CT32_EMR_EM2           (1U << 2U)
#define CT32_EMR_EM3           (1U << 3U)
#define CT32_EMR_EMCTR_NOTHING (0U)
#define CT32_EMR_EMCTR_CLEAR   (1U)
#define CT32_EMR_EMCTR_SET     (2U)
#define CT32_EMR_EMCTR_TOGGLE  (3U)
#define CT32_EMR_EMC0_SHIFT    (4U)
#define CT32_EMR_EMC1_SHIFT    (6U)
#define CT32_EMR_EMC2_SHIFT    (8U)
#define CT32_EMR_EMC3_SHIFT    (10U)

/* 20.6.2 timer control register */
#define CT32_TCR_CEN  (1U << 0U)
#define CT32_TCR_CRST (1U << 1U)

/* 35.1.1 pin functions */
#define IOCON_PIN0_11_FUNC_TDI  (0U)
#define IOCON_PIN0_11_FUNC_PIO  (1U)
#define IOCON_PIN0_11_FUNC_ADC  (2U)
#define IOCON_PIN0_11_FUNC_MAT3 (3U)
#define IOCON_PIN0_11_FUNC_RTS  (4U)
#define IOCON_PIN0_11_FUNC_SCLK (5U)
#define IOCON_PIN0_12_FUNC_TMS  (0U)
#define IOCON_PIN0_12_FUNC_PIO  (1U)
#define IOCON_PIN0_12_FUNC_ADC  (2U)
#define IOCON_PIN0_12_FUNC_CAP0 (3U)
#define IOCON_PIN0_12_FUNC_CTS  (4U)
#define IOCON_PIN1_13_FUNC_PIO  (0U)
#define IOCON_PIN1_13_FUNC_CTS  (1U)
#define IOCON_PIN1_13_FUNC_OUT3 (2U)
#define IOCON_PIN1_19_FUNC_PIO  (0U)
#define IOCON_PIN1_19_FUNC_CTS  (1U)
#define IOCON_PIN1_19_FUNC_OUT0 (2U)
#define IOCON_PIN2_2_FUNC_PIO   (0U)
#define IOCON_PIN2_2_FUNC_RTS   (1U)
#define IOCON_PIN2_2_FUNC_SCLK  (2U)
#define IOCON_PIN2_2_FUNC_OUT1  (3U)
#define IOCON_PIN2_7_FUNC_PIO   (0U)
#define IOCON_PIN2_7_FUNC_SCK   (1U)
#define IOCON_PIN2_7_FUNC_OUT2  (2U)
#define IOCON_PIN2_16_FUNC_PIO  (3U)
#define IOCON_PIN2_16_FUNC_OUT0 (1U)
#define IOCON_PIN2_17_FUNC_PIO  (0U)
#define IOCON_PIN2_17_FUNC_OUT1 (1U)
#define IOCON_PIN2_18_FUNC_PIO  (0U)
#define IOCON_PIN2_18_FUNC_OUT2 (1U)

#endif /* ZEPHYR_DRIVERS_DALI_LPC11U6X_H_ */
