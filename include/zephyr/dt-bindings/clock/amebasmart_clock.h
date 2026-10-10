/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AMEBASMART_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AMEBASMART_CLOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Realtek Amebasmart clock Devicetree bindings
 */

/**
 * @name LSYS_FEN_GRP0 (LP) domain clocks
 * @{
 */

/** I2C2 clock in LP domain */
#define AMEBA_I2C2_CLK    1

/** I2C1 clock in LP domain */
#define AMEBA_I2C1_CLK    2

/** I2C0 clock in LP domain */
#define AMEBA_I2C0_CLK    3

/** GPIO clock in LP domain */
#define AMEBA_GPIO_CLK    5

/** CTC clock in LP domain */
#define AMEBA_CTC_CLK     6

/** ADC clock in LP domain */
#define AMEBA_ADC_CLK     7

/** LOGUART clock in LP domain */
#define AMEBA_LOGUART_CLK 8

/** IPC_LP clock in LP domain */
#define AMEBA_IPC_LP_CLK  9

/** BTON clock in LP domain */
#define AMEBA_BTON_CLK    10

/** AIP clock in LP domain */
#define AMEBA_AIP_CLK     11

/** THM clock in LP domain */
#define AMEBA_THM_CLK     12

/** DTIM clock in LP domain */
#define AMEBA_DTIM_CLK    13

/** SCE clock in LP domain */
#define AMEBA_SCE_CLK     14

/** FLASH clock in LP domain */
#define AMEBA_FLASH_CLK   15

/** WLON clock in LP domain */
#define AMEBA_WLON_CLK    16

/** NP clock in LP domain */
#define AMEBA_NP_CLK      17

/** LP clock in LP domain */
#define AMEBA_LP_CLK      18

/** HPLFM clock in LP domain */
#define AMEBA_HPLFM_CLK   19

/** LPLFM clock in LP domain */
#define AMEBA_LPLFM_CLK   20

/** SIC clock in LP domain */
#define AMEBA_SIC_CLK     21

/** HPON clock in LP domain */
#define AMEBA_HPON_CLK    22

/** @} */

/**
 * @name LSYS_FEN_GRP1 (HS) domain clocks
 * @{
 */

/** ZGB clock in HS domain */
#define AMEBA_ZGB_CLK     23

/** PSRAM clock in HS domain */
#define AMEBA_PSRAM_CLK   24

/** AC clock in HS domain */
#define AMEBA_AC_CLK      26

/** DDRP clock in HS domain */
#define AMEBA_DDRP_CLK    27

/** DDRC clock in HS domain */
#define AMEBA_DDRC_CLK    28

/** IRDA clock in HS domain */
#define AMEBA_IRDA_CLK    29

/** ECDSA clock in HS domain */
#define AMEBA_ECDSA_CLK   30

/** ED25519 clock in HS domain */
#define AMEBA_ED25519_CLK 31

/** IPC_HP clock in HS domain */
#define AMEBA_IPC_HP_CLK  33

/** LEDC clock in HS domain */
#define AMEBA_LEDC_CLK    34

/** TRNG clock in HS domain */
#define AMEBA_TRNG_CLK    35

/** USB clock in HS domain */
#define AMEBA_USB_CLK     36

/** SDH clock in HS domain */
#define AMEBA_SDH_CLK     37

/** SPI1 clock in HS domain */
#define AMEBA_SPI1_CLK    38

/** SPI0 clock in HS domain */
#define AMEBA_SPI0_CLK    39

/** GDMA clock in HS domain */
#define AMEBA_GDMA_CLK    40

/** LCDC clock in HS domain */
#define AMEBA_LCDC_CLK    41

/** HPERI root clock in HS domain (parent gate of LCDC/MIPI and other HS IPs) */
#define AMEBA_HPERI_CLK   4

/** IPSEC clock in HS domain */
#define AMEBA_IPSEC_CLK   42

/** LX1 clock in HS domain */
#define AMEBA_LX1_CLK     43

/** @} */

/**
 * @name LSYS_FEN_GRP2 (Timer/UART/SPORT) domain clocks
 * @{
 */

/** TIM0 clock in Timer/UART/SPORT domain */
#define AMEBA_TIM0_CLK   44

/** TIM1 clock in Timer/UART/SPORT domain */
#define AMEBA_TIM1_CLK   45

/** TIM2 clock in Timer/UART/SPORT domain */
#define AMEBA_TIM2_CLK   46

/** TIM3 clock in Timer/UART/SPORT domain */
#define AMEBA_TIM3_CLK   47

/** TIM4 clock in Timer/UART/SPORT domain */
#define AMEBA_TIM4_CLK   48

/** TIM5 clock in Timer/UART/SPORT domain */
#define AMEBA_TIM5_CLK   49

/** TIM6 clock in Timer/UART/SPORT domain */
#define AMEBA_TIM6_CLK   50

/** TIM7 clock in Timer/UART/SPORT domain */
#define AMEBA_TIM7_CLK   51

/** TIM8 clock in Timer/UART/SPORT domain */
#define AMEBA_TIM8_CLK   52

/** TIM9 clock in Timer/UART/SPORT domain */
#define AMEBA_TIM9_CLK   53

/** TIM10 clock in Timer/UART/SPORT domain */
#define AMEBA_TIM10_CLK  54

/** TIM11 clock in Timer/UART/SPORT domain */
#define AMEBA_TIM11_CLK  55

/** TIM12 clock in Timer/UART/SPORT domain */
#define AMEBA_TIM12_CLK  56

/** TIM13 clock in Timer/UART/SPORT domain */
#define AMEBA_TIM13_CLK  57

/** SPORT0 clock in Timer/UART/SPORT domain */
#define AMEBA_SPORT0_CLK 58

/** SPORT1 clock in Timer/UART/SPORT domain */
#define AMEBA_SPORT1_CLK 59

/** SPORT2 clock in Timer/UART/SPORT domain */
#define AMEBA_SPORT2_CLK 60

/** SPORT3 clock in Timer/UART/SPORT domain */
#define AMEBA_SPORT3_CLK 61

/** UART0 clock in Timer/UART/SPORT domain */
#define AMEBA_UART0_CLK  62

/** UART1 clock in Timer/UART/SPORT domain */
#define AMEBA_UART1_CLK  63

/** UART2 clock in Timer/UART/SPORT domain */
#define AMEBA_UART2_CLK  64

/** UART3 clock in Timer/UART/SPORT domain */
#define AMEBA_UART3_CLK  65

/** @} */

/**
 * @name AON domain clocks
 * @{
 */

/** RTC clock in AON domain */
#define AMEBA_RTC_CLK    66

/** @} */

/**
 * @brief Maximum clock index (one past the last valid index).
 */
#define AMEBA_CLK_MAX 67 /* clk idx max */

/**
 * @name Peripheral clock helper macros
 * @{
 */

/**
 * @brief Define a clock entry for a peripheral with numerical suffix.
 *
 * Used for peripherals with an index, for example SPI0, SPI1, UART0.
 *
 * @param name Peripheral base name
 * @param n    Peripheral index
 */
#define AMEBA_NUMERICAL_PERIPH(name, n)                                                            \
	[AMEBA_##name##n##_CLK] = {                                                                \
		.parent = AMEBA_RCC_NO_PARENT,                                                     \
		.cke = APBPeriph_##name##n##_CLOCK,                                                \
		.fen = APBPeriph_##name##n,                                                        \
	},

/**
 * @brief Define a clock entry for a single-instance peripheral.
 *
 * Used for peripherals that have only one instance, for example GPIO,
 * SDH, USB, TRNG, etc.
 *
 * @param name Peripheral name
 */
#define AMEBA_SINGLE_PERIPH(name)                                                                  \
	[AMEBA_##name##_CLK] = {                                                                   \
		.parent = AMEBA_RCC_NO_PARENT,                                                     \
		.cke = APBPeriph_##name##_CLOCK,                                                   \
		.fen = APBPeriph_##name,                                                           \
	},

/**
 * @brief I2C clock peripheral mappings.
 */
#define AMEBA_I2C_PERIPHS                                                                          \
	AMEBA_NUMERICAL_PERIPH(I2C, 0) /* AMEBA_I2C0_CLK */                                        \
	AMEBA_NUMERICAL_PERIPH(I2C, 1) /* AMEBA_I2C1_CLK */                                        \
	AMEBA_NUMERICAL_PERIPH(I2C, 2) /* AMEBA_I2C2_CLK */

/**
 * @brief SPI clock peripheral mappings.
 */
#define AMEBA_SPI_PERIPHS                                                                          \
	AMEBA_NUMERICAL_PERIPH(SPI, 0) /* AMEBA_SPI0_CLK */                                        \
	AMEBA_NUMERICAL_PERIPH(SPI, 1) /* AMEBA_SPI1_CLK */

/**
 * @brief TIM clock peripheral mappings.
 */
#define AMEBA_TIM_PERIPHS                                                                          \
	AMEBA_NUMERICAL_PERIPH(TIM, 0)  /* AMEBA_TIM0_CLK */                                       \
	AMEBA_NUMERICAL_PERIPH(TIM, 1)  /* AMEBA_TIM1_CLK */                                       \
	AMEBA_NUMERICAL_PERIPH(TIM, 2)  /* AMEBA_TIM2_CLK */                                       \
	AMEBA_NUMERICAL_PERIPH(TIM, 3)  /* AMEBA_TIM3_CLK */                                       \
	AMEBA_NUMERICAL_PERIPH(TIM, 4)  /* AMEBA_TIM4_CLK */                                       \
	AMEBA_NUMERICAL_PERIPH(TIM, 5)  /* AMEBA_TIM5_CLK */                                       \
	AMEBA_NUMERICAL_PERIPH(TIM, 6)  /* AMEBA_TIM6_CLK */                                       \
	AMEBA_NUMERICAL_PERIPH(TIM, 7)  /* AMEBA_TIM7_CLK */                                       \
	AMEBA_NUMERICAL_PERIPH(TIM, 8)  /* AMEBA_TIM8_CLK */                                       \
	AMEBA_NUMERICAL_PERIPH(TIM, 9)  /* AMEBA_TIM9_CLK */                                       \
	AMEBA_NUMERICAL_PERIPH(TIM, 10) /* AMEBA_TIM10_CLK */                                      \
	AMEBA_NUMERICAL_PERIPH(TIM, 11) /* AMEBA_TIM11_CLK */                                      \
	AMEBA_NUMERICAL_PERIPH(TIM, 12) /* AMEBA_TIM12_CLK */                                      \
	AMEBA_NUMERICAL_PERIPH(TIM, 13) /* AMEBA_TIM13_CLK */

/**
 * @brief SPORT clock peripheral mappings.
 */
#define AMEBA_SPORT_PERIPHS                                                                        \
	AMEBA_NUMERICAL_PERIPH(SPORT, 0) /* AMEBA_SPORT0_CLK */                                    \
	AMEBA_NUMERICAL_PERIPH(SPORT, 1) /* AMEBA_SPORT1_CLK */                                    \
	AMEBA_NUMERICAL_PERIPH(SPORT, 2) /* AMEBA_SPORT2_CLK */                                    \
	AMEBA_NUMERICAL_PERIPH(SPORT, 3) /* AMEBA_SPORT3_CLK */

/**
 * @brief UART clock peripheral mappings.
 */
#define AMEBA_UART_PERIPHS                                                                         \
	AMEBA_NUMERICAL_PERIPH(UART, 0) /* AMEBA_UART0_CLK */                                      \
	AMEBA_NUMERICAL_PERIPH(UART, 1) /* AMEBA_UART1_CLK */                                      \
	AMEBA_NUMERICAL_PERIPH(UART, 2) /* AMEBA_UART2_CLK */                                      \
	AMEBA_NUMERICAL_PERIPH(UART, 3) /* AMEBA_UART3_CLK */

/**
 * @brief LOGUART clock peripheral mapping.
 */
#define AMEBA_LOGUART_PERIPHS AMEBA_SINGLE_PERIPH(LOGUART)        /* AMEBA_LOGUART_CLK */

/**
 * @brief GPIO clock peripheral mapping.
 */
#define AMEBA_GPIO_PERIPHS    AMEBA_SINGLE_PERIPH(GPIO)           /* AMEBA_GPIO_CLK */

/**
 * @brief ADC clock peripheral mapping.
 */
#define AMEBA_ADC_PERIPHS     AMEBA_SINGLE_PERIPH(ADC)            /* AMEBA_ADC_CLK */

/**
 * @brief CTC clock peripheral mapping.
 */
#define AMEBA_CTC_PERIPHS     AMEBA_SINGLE_PERIPH(CTC)            /* AMEBA_CTC_CLK */

/**
 * @brief IPC_LP clock peripheral mapping.
 */
#define AMEBA_IPC_LP_PERIPHS  AMEBA_SINGLE_PERIPH(IPC_LP)         /* AMEBA_IPC_LP_CLK */

/**
 * @brief BTON clock peripheral mapping.
 */
#define AMEBA_BTON_PERIPHS    AMEBA_SINGLE_PERIPH(BTON)           /* AMEBA_BTON_CLK */

/**
 * @brief DTIM clock peripheral mapping.
 */
#define AMEBA_DTIM_PERIPHS    AMEBA_SINGLE_PERIPH(DTIM)           /* AMEBA_DTIM_CLK */

/**
 * @brief FLASH clock peripheral mapping.
 */
#define AMEBA_FLASH_PERIPHS   AMEBA_SINGLE_PERIPH(FLASH)          /* AMEBA_FLASH_CLK */

/**
 * @brief WLON clock peripheral mapping.
 */
#define AMEBA_WLON_PERIPHS    AMEBA_SINGLE_PERIPH(WLON)           /* AMEBA_WLON_CLK */

/**
 * @brief PSRAM clock peripheral mapping.
 */
#define AMEBA_PSRAM_PERIPHS   AMEBA_SINGLE_PERIPH(PSRAM)          /* AMEBA_PSRAM_CLK */

/**
 * @brief AC clock peripheral mapping.
 */
#define AMEBA_AC_PERIPHS      AMEBA_SINGLE_PERIPH(AC)             /* AMEBA_AC_CLK */

/**
 * @brief IRDA clock peripheral mapping.
 */
#define AMEBA_IRDA_PERIPHS    AMEBA_SINGLE_PERIPH(IRDA)           /* AMEBA_IRDA_CLK */

/**
 * @brief ECDSA clock peripheral mapping.
 */
#define AMEBA_ECDSA_PERIPHS   AMEBA_SINGLE_PERIPH(ECDSA)          /* AMEBA_ECDSA_CLK */

/**
 * @brief ED25519 clock peripheral mapping.
 */
#define AMEBA_ED25519_PERIPHS AMEBA_SINGLE_PERIPH(ED25519)        /* AMEBA_ED25519_CLK */

/**
 * @brief IPC_HP clock peripheral mapping.
 */
#define AMEBA_IPC_HP_PERIPHS  AMEBA_SINGLE_PERIPH(IPC_HP)         /* AMEBA_IPC_HP_CLK */

/**
 * @brief LEDC clock peripheral mapping.
 */
#define AMEBA_LEDC_PERIPHS    AMEBA_SINGLE_PERIPH(LEDC)           /* AMEBA_LEDC_CLK */

/**
 * @brief TRNG clock peripheral mapping.
 */
#define AMEBA_TRNG_PERIPHS    AMEBA_SINGLE_PERIPH(TRNG)           /* AMEBA_TRNG_CLK */

/**
 * @brief USB clock peripheral mapping.
 */
#define AMEBA_USB_PERIPHS     AMEBA_SINGLE_PERIPH(USB)            /* AMEBA_USB_CLK */

/**
 * @brief SDH clock peripheral mapping.
 */
#define AMEBA_SDH_PERIPHS     AMEBA_SINGLE_PERIPH(SDH)            /* AMEBA_SDH_CLK */

/**
 * @brief GDMA clock peripheral mapping.
 */
#define AMEBA_GDMA_PERIPHS    AMEBA_SINGLE_PERIPH(GDMA)           /* AMEBA_GDMA_CLK */

/**
 * @brief LCDC clock peripheral mapping.
 *
 * The HAL names the CKE bit LCDCMIPI_CLOCK because LCDC and MIPI share it.
 */
#define AMEBA_LCDC_PERIPHS                                                                         \
	[AMEBA_LCDC_CLK] = {                                                                       \
		.parent = AMEBA_HPERI_CLK,                                                         \
		.cke = APBPeriph_LCDCMIPI_CLOCK,                                                   \
		.fen = APBPeriph_LCDC,                                                             \
	},

/**
 * @brief HPERI root clock peripheral mapping.
 *
 * HS-domain root gating LCDC/MIPI, modelled as the LCDC parent so enabling LCDC
 * does not depend on MIPI-DSI attach order. Only the CKE bit gates it.
 */
#define AMEBA_HPERI_PERIPHS                                                                        \
	[AMEBA_HPERI_CLK] = {                                                                      \
		.parent = AMEBA_RCC_NO_PARENT,                                                     \
		.cke = APBPeriph_HPERI_CLOCK,                                                      \
		.fen = APBPeriph_NULL,                                                             \
	},

/**
 * @brief IPSEC clock peripheral mapping.
 */
#define AMEBA_IPSEC_PERIPHS   AMEBA_SINGLE_PERIPH(IPSEC)          /* AMEBA_IPSEC_CLK */

/**
 * @brief RTC clock peripheral mapping (AON domain).
 *
 * RCC_PeriphClockCmd ignores AON group 3, so clock_control_on() is a hardware no-op.
 * The boot ROM already enables the always-on AON domain.
 */
#define AMEBA_RTC_PERIPHS     AMEBA_SINGLE_PERIPH(RTC)            /* AMEBA_RTC_CLK */

/**
 * @brief Aggregated core peripheral clock mappings.
 *
 * This macro expands to mappings of all core peripherals used by
 * the clock control implementation.
 */
#define AMEBA_CORE_PERIPHS                                                                         \
	AMEBA_I2C_PERIPHS                                                                          \
	AMEBA_LOGUART_PERIPHS                                                                      \
	AMEBA_GPIO_PERIPHS                                                                         \
	AMEBA_ADC_PERIPHS                                                                          \
	AMEBA_CTC_PERIPHS                                                                          \
	AMEBA_IPC_LP_PERIPHS                                                                       \
	AMEBA_BTON_PERIPHS                                                                         \
	AMEBA_DTIM_PERIPHS                                                                         \
	AMEBA_FLASH_PERIPHS                                                                        \
	AMEBA_WLON_PERIPHS                                                                         \
	AMEBA_PSRAM_PERIPHS                                                                        \
	AMEBA_AC_PERIPHS                                                                           \
	AMEBA_IRDA_PERIPHS                                                                         \
	AMEBA_ECDSA_PERIPHS                                                                        \
	AMEBA_ED25519_PERIPHS                                                                      \
	AMEBA_IPC_HP_PERIPHS                                                                       \
	AMEBA_LEDC_PERIPHS                                                                         \
	AMEBA_TRNG_PERIPHS                                                                         \
	AMEBA_USB_PERIPHS                                                                          \
	AMEBA_SDH_PERIPHS                                                                          \
	AMEBA_SPI_PERIPHS                                                                          \
	AMEBA_GDMA_PERIPHS                                                                         \
	AMEBA_LCDC_PERIPHS                                                                         \
	AMEBA_HPERI_PERIPHS                                                                        \
	AMEBA_IPSEC_PERIPHS                                                                        \
	AMEBA_TIM_PERIPHS                                                                          \
	AMEBA_SPORT_PERIPHS                                                                        \
	AMEBA_UART_PERIPHS                                                                         \
	AMEBA_RTC_PERIPHS

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AMEBASMART_CLOCK_H_ */
