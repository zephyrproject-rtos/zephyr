/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RTL8752H Pin Control (Pinmux) Header
 *
 * This file defines the pinmux functions and pin assignments for the
 * Realtek RTL8752H series SoC.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RTL8752H_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RTL8752H_PINCTRL_H_

#include "bee-pinctrl.h"

/**
 * @name Pinctrl Pinmux Functions
 * @{
 */
#define BEE_IDLE_MODE          0                    /**< IDLE_MODE pinmux function */
#define BEE_UART2_TX           1                    /**< UART2_TX pinmux function */
#define BEE_UART2_RX           2                    /**< UART2_RX pinmux function */
#define BEE_UART2_CTS          3                    /**< UART2_CTS pinmux function */
#define BEE_UART2_RTS          4                    /**< UART2_RTS pinmux function */
#define BEE_I2C0_CLK           5                    /**< I2C0_CLK pinmux function */
#define BEE_I2C0_DAT           6                    /**< I2C0_DAT pinmux function */
#define BEE_I2C1_CLK           7                    /**< I2C1_CLK pinmux function */
#define BEE_I2C1_DAT           8                    /**< I2C1_DAT pinmux function */
#define BEE_PWM2_P             9                    /**< PWM2_P pinmux function */
#define BEE_PWM2_N             10                   /**< PWM2_N pinmux function */
#define BEE_ENPWM0_P           11                   /**< ENPWM0_P pinmux function */
#define BEE_ENPWM0_N           12                   /**< ENPWM0_N pinmux function */
#define BEE_TIM_PWM0           13                   /**< TIM_PWM0 pinmux function */
#define BEE_TIM_PWM1           14                   /**< TIM_PWM1 pinmux function */
#define BEE_TIM_PWM2           15                   /**< TIM_PWM2 pinmux function */
#define BEE_TIM_PWM3           16                   /**< TIM_PWM3 pinmux function */
#define BEE_TIM_PWM4           17                   /**< TIM_PWM4 pinmux function */
#define BEE_TIM_PWM5           18                   /**< TIM_PWM5 pinmux function */
#define BEE_ENPWM0             19                   /**< ENPWM0 pinmux function */
#define BEE_ENPWM1             20                   /**< ENPWM1 pinmux function */
#define BEE_QDEC_PHASE_A_X     21                   /**< QDEC_PHASE_A_X pinmux function */
#define BEE_QDEC_PHASE_B_X     22                   /**< QDEC_PHASE_B_X pinmux function */
#define BEE_QDEC_PHASE_A_Y     23                   /**< QDEC_PHASE_A_Y pinmux function */
#define BEE_QDEC_PHASE_B_Y     24                   /**< QDEC_PHASE_B_Y pinmux function */
#define BEE_QDEC_PHASE_A_Z     25                   /**< QDEC_PHASE_A_Z pinmux function */
#define BEE_QDEC_PHASE_B_Z     26                   /**< QDEC_PHASE_B_Z pinmux function */
#define BEE_UART0_TX           29                   /**< UART0_TX pinmux function */
#define BEE_UART0_RX           30                   /**< UART0_RX pinmux function */
#define BEE_UART0_CTS          31                   /**< UART0_CTS pinmux function */
#define BEE_UART0_RTS          32                   /**< UART0_RTS pinmux function */
#define BEE_IRDA_TX            33                   /**< IRDA_TX pinmux function */
#define BEE_IRDA_RX            34                   /**< IRDA_RX pinmux function */
#define BEE_UART1_TX           35                   /**< UART1_TX pinmux function */
#define BEE_UART1_RX           36                   /**< UART1_RX pinmux function */
#define BEE_UART1_CTS          37                   /**< UART1_CTS pinmux function */
#define BEE_UART1_RTS          38                   /**< UART1_RTS pinmux function */
#define BEE_SPI1_SS_N_0_MASTER 39                   /**< SPI1_SS_N_0_MASTER pinmux function */
#define BEE_SPI1_SS_N_1_MASTER 40                   /**< SPI1_SS_N_1_MASTER pinmux function */
#define BEE_SPI1_SS_N_2_MASTER 41                   /**< SPI1_SS_N_2_MASTER pinmux function */
#define BEE_SPI1_CLK_MASTER    42                   /**< SPI1_CLK_MASTER pinmux function */
#define BEE_SPI1_MO_MASTER     43                   /**< SPI1_MO_MASTER pinmux function */
#define BEE_SPI1_MI_MASTER     44                   /**< SPI1_MI_MASTER pinmux function */
#define BEE_SPI0_SS_N_0_SLAVE  45                   /**< SPI0_SS_N_0_SLAVE pinmux function */
#define BEE_SPI0_CLK_SLAVE     46                   /**< SPI0_CLK_SLAVE pinmux function */
#define BEE_SPI0_SO_SLAVE      47                   /**< SPI0_SO_SLAVE pinmux function */
#define BEE_SPI0_SI_SLAVE      48                   /**< SPI0_SI_SLAVE pinmux function */
#define BEE_SPI0_SS_N_0_MASTER 49                   /**< SPI0_SS_N_0_MASTER pinmux function */
#define BEE_SPI0_CLK_MASTER    50                   /**< SPI0_CLK_MASTER pinmux function */
#define BEE_SPI0_MO_MASTER     51                   /**< SPI0_MO_MASTER pinmux function */
#define BEE_SPI0_MI_MASTER     52                   /**< SPI0_MI_MASTER pinmux function */
#define BEE_SPI2W_DATA         53                   /**< SPI2W_DATA pinmux function */
#define BEE_SPI2W_CLK          54                   /**< SPI2W_CLK pinmux function */
#define BEE_SPI2W_CS           55                   /**< SPI2W_CS pinmux function */
#define BEE_SWD_CLK            56                   /**< SWD_CLK pinmux function */
#define BEE_SWD_DIO            57                   /**< SWD_DIO pinmux function */
#define BEE_KEY_COL_0          58                   /**< KEY_COL_0 pinmux function */
#define BEE_KEY_COL_1          59                   /**< KEY_COL_1 pinmux function */
#define BEE_KEY_COL_2          60                   /**< KEY_COL_2 pinmux function */
#define BEE_KEY_COL_3          61                   /**< KEY_COL_3 pinmux function */
#define BEE_KEY_COL_4          62                   /**< KEY_COL_4 pinmux function */
#define BEE_KEY_COL_5          63                   /**< KEY_COL_5 pinmux function */
#define BEE_KEY_COL_6          64                   /**< KEY_COL_6 pinmux function */
#define BEE_KEY_COL_7          65                   /**< KEY_COL_7 pinmux function */
#define BEE_KEY_COL_8          66                   /**< KEY_COL_8 pinmux function */
#define BEE_KEY_COL_9          67                   /**< KEY_COL_9 pinmux function */
#define BEE_KEY_COL_10         68                   /**< KEY_COL_10 pinmux function */
#define BEE_KEY_COL_11         69                   /**< KEY_COL_11 pinmux function */
#define BEE_KEY_COL_12         70                   /**< KEY_COL_12 pinmux function */
#define BEE_KEY_COL_13         71                   /**< KEY_COL_13 pinmux function */
#define BEE_KEY_COL_14         72                   /**< KEY_COL_14 pinmux function */
#define BEE_KEY_COL_15         73                   /**< KEY_COL_15 pinmux function */
#define BEE_KEY_COL_16         74                   /**< KEY_COL_16 pinmux function */
#define BEE_KEY_COL_17         75                   /**< KEY_COL_17 pinmux function */
#define BEE_KEY_COL_18         76                   /**< KEY_COL_18 pinmux function */
#define BEE_KEY_COL_19         77                   /**< KEY_COL_19 pinmux function */
#define BEE_KEY_ROW_0          78                   /**< KEY_ROW_0 pinmux function */
#define BEE_KEY_ROW_1          79                   /**< KEY_ROW_1 pinmux function */
#define BEE_KEY_ROW_2          80                   /**< KEY_ROW_2 pinmux function */
#define BEE_KEY_ROW_3          81                   /**< KEY_ROW_3 pinmux function */
#define BEE_KEY_ROW_4          82                   /**< KEY_ROW_4 pinmux function */
#define BEE_KEY_ROW_5          83                   /**< KEY_ROW_5 pinmux function */
#define BEE_KEY_ROW_6          84                   /**< KEY_ROW_6 pinmux function */
#define BEE_KEY_ROW_7          85                   /**< KEY_ROW_7 pinmux function */
#define BEE_KEY_ROW_8          86                   /**< KEY_ROW_8 pinmux function */
#define BEE_KEY_ROW_9          87                   /**< KEY_ROW_9 pinmux function */
#define BEE_KEY_ROW_10         88                   /**< KEY_ROW_10 pinmux function */
#define BEE_KEY_ROW_11         89                   /**< KEY_ROW_11 pinmux function */
#define BEE_DWGPIO             90                   /**< DWGPIO pinmux function */
#define BEE_DMIC1_CLK          96                   /**< DMIC1_CLK pinmux function */
#define BEE_DMIC1_DAT          97                   /**< DMIC1_DAT pinmux function */
#define BEE_LRC_I_CODEC_SLAVE  98                   /**< LRC_I_CODEC_SLAVE pinmux function */
#define BEE_BCLK_I_CODEC_SLAVE 99                   /**< BCLK_I_CODEC_SLAVE pinmux function */
#define BEE_SDI_CODEC_SLAVE    100                  /**< SDI_CODEC_SLAVE pinmux function */
#define BEE_SDO_CODEC_SLAVE    101                  /**< SDO_CODEC_SLAVE pinmux function */
#define BEE_BT_COEX_I_0        106                  /**< BT_COEX_I_0 pinmux function */
#define BEE_BT_COEX_I_1        107                  /**< BT_COEX_I_1 pinmux function */
#define BEE_BT_COEX_I_2        108                  /**< BT_COEX_I_2 pinmux function */
#define BEE_BT_COEX_I_3        109                  /**< BT_COEX_I_3 pinmux function */
#define BEE_BT_COEX_O_0        110                  /**< BT_COEX_O_0 pinmux function */
#define BEE_BT_COEX_O_1        111                  /**< BT_COEX_O_1 pinmux function */
#define BEE_BT_COEX_O_2        112                  /**< BT_COEX_O_2 pinmux function */
#define BEE_BT_COEX_O_3        113                  /**< BT_COEX_O_3 pinmux function */
#define BEE_PTA_I2C_CLK_SLAVE  114                  /**< PTA_I2C_CLK_SLAVE pinmux function */
#define BEE_PTA_I2C_DAT_SLAVE  115                  /**< PTA_I2C_DAT_SLAVE pinmux function */
#define BEE_PTA_I2C_INT_OUT    116                  /**< PTA_I2C_INT_OUT pinmux function */
#define BEE_EN_EXPA            117                  /**< EN_EXPA pinmux function */
#define BEE_EN_EXLNA           118                  /**< EN_EXLNA pinmux function */
#define BEE_LRC_SPORT0         123                  /**< LRC_SPORT0 pinmux function */
#define BEE_BCLK_SPORT0        124                  /**< BCLK_SPORT0 pinmux function */
#define BEE_ADCDAT_SPORT0      125                  /**< ADCDAT_SPORT0 pinmux function */
#define BEE_DACDAT_SPORT0      126                  /**< DACDAT_SPORT0 pinmux function */
#define BEE_MCLK               127                  /**< MCLK pinmux function */
#define BEE_PINMUX_MAX         (BEE_MCLK + 1)       /**< PINMUX_MAX pinmux function */
#define BEE_SW_MODE            (BEE_PINMUX_MAX + 1) /**< SW_MODE pinmux function */
#define BEE_PWR_OFF            (BEE_PINMUX_MAX + 2) /**< PWR_OFF pinmux function */
#define BEE_PIN_DISCONNECTED   BEE_PIN_MSK          /**< PIN_DISCONNECTED pinmux function */
/** @} */

/**
 * @name Pinctrl Available Pins
 * @{
 */
#define P0_0 0 /**< GPIO0 */
#define P0_1 1 /**< GPIO1 */
#define P0_2 2 /**< GPIO2 */
/* Note: P0_3 defaults to outputting the Realtek internal log for rtl8752h. */
#define P0_3 3 /**< GPIO3 */
#define P0_4 4 /**< GPIO4 */
#define P0_5 5 /**< GPIO5 */
#define P0_6 6 /**< GPIO6 */
#define P0_7 7 /**< GPIO7 */
/* Note: P1_0/P1_1 default to SWD function for rtl8752h. */
#define P1_0 8 /**< GPIO8 */
#define P1_1 9 /**< GPIO9 */
#define P1_3 11 /**< GPIOA_11 */
#define P1_4 12 /**< GPIOA_12 */
#define P1_6 14 /**< GPIO14 */
#define P1_7 15 /**< GPIO15 */
#define P2_0 16 /**< GPIO16 */
#define P2_1 17 /**< GPIO17 */
#define P2_2 18 /**< GPIO18 */
#define P2_3 19 /**< GPIO19 */
#define P2_4 20 /**< GPIO20 */
#define P2_5 21 /**< GPIO21 */
#define P2_6 22 /**< GPIO22 */
#define P2_7 23 /**< GPIO23 */
#define P3_0 24 /**< GPIO24 */
#define P3_1 25 /**< GPIO25 */
#define P3_2 26 /**< GPIO26 */
#define P3_3 27 /**< GPIO27 */
#define P3_4 28 /**< GPIO28 */
#define P3_5 29 /**< GPIO29 */
#define P3_6 30 /**< GPIO30 */
#define P4_0 32 /**< GPIO13 */
#define P4_1 33 /**< GPIO29 */
#define P4_2 34 /**< GPIO30 */
#define P4_3 35 /**< GPIO31 */
#define H_0  36 /**< GPIO10/MICBIAS */
#define P5_1 37 /**< GPIO11 */
#define P5_2 38 /**< GPIO12 */

/** @} */

/**
 * @name GPIO Pinctrl Configuration
 * @{
 */
/* Port 0 */
#define BEE_PSEL_GPIOA_0_P0_0 BEE_PSEL(DWGPIO, P0_0) /**< GPIOA_0 for P0_0 */
#define BEE_PSEL_GPIOA_1_P0_1 BEE_PSEL(DWGPIO, P0_1) /**< GPIOA_1 for P0_1 */
#define BEE_PSEL_GPIOA_2_P0_2 BEE_PSEL(DWGPIO, P0_2) /**< GPIOA_2 for P0_2 */
/* Note: P0_3 defaults to outputting the Realtek internal log for rtl8752h. */
#define BEE_PSEL_GPIOA_3_P0_3 BEE_PSEL(DWGPIO, P0_3) /**< GPIOA_3 for P0_3 */
#define BEE_PSEL_GPIOA_4_P0_4 BEE_PSEL(DWGPIO, P0_4) /**< GPIOA_4 for P0_4 */
#define BEE_PSEL_GPIOA_5_P0_5 BEE_PSEL(DWGPIO, P0_5) /**< GPIOA_5 for P0_5 */
#define BEE_PSEL_GPIOA_6_P0_6 BEE_PSEL(DWGPIO, P0_6) /**< GPIOA_6 for P0_6 */
#define BEE_PSEL_GPIOA_7_P0_7 BEE_PSEL(DWGPIO, P0_7) /**< GPIOA_7 for P0_7 */

/* Port 1 */
/* Note: P1_0/P1_1 default to SWD function for rtl8752h. */
#define BEE_PSEL_GPIOA_8_P1_0  BEE_PSEL(DWGPIO, P1_0) /**< GPIOA_8 for P1_0 */
#define BEE_PSEL_GPIOA_9_P1_1  BEE_PSEL(DWGPIO, P1_1) /**< GPIOA_9 for P1_1 */
#define BEE_PSEL_GPIOA_11_P1_3 BEE_PSEL(DWGPIO, P1_3) /**< GPIOA_11 for P1_3 */
#define BEE_PSEL_GPIOA_12_P1_4 BEE_PSEL(DWGPIO, P1_4) /**< GPIOA_12 for P1_4 */
#define BEE_PSEL_GPIOA_14_P1_6 BEE_PSEL(DWGPIO, P1_6) /**< GPIOA_14 for P1_6 */
#define BEE_PSEL_GPIOA_15_P1_7 BEE_PSEL(DWGPIO, P1_7) /**< GPIOA_15 for P1_7 */

/* Port 2 */
#define BEE_PSEL_GPIOA_16_P2_0 BEE_PSEL(DWGPIO, P2_0) /**< GPIOA_16 for P2_0 */
#define BEE_PSEL_GPIOA_17_P2_1 BEE_PSEL(DWGPIO, P2_1) /**< GPIOA_17 for P2_1 */
#define BEE_PSEL_GPIOA_18_P2_2 BEE_PSEL(DWGPIO, P2_2) /**< GPIOA_18 for P2_2 */
#define BEE_PSEL_GPIOA_19_P2_3 BEE_PSEL(DWGPIO, P2_3) /**< GPIOA_19 for P2_3 */
#define BEE_PSEL_GPIOA_20_P2_4 BEE_PSEL(DWGPIO, P2_4) /**< GPIOA_20 for P2_4 */
#define BEE_PSEL_GPIOA_21_P2_5 BEE_PSEL(DWGPIO, P2_5) /**< GPIOA_21 for P2_5 */
#define BEE_PSEL_GPIOA_22_P2_6 BEE_PSEL(DWGPIO, P2_6) /**< GPIOA_22 for P2_6 */
#define BEE_PSEL_GPIOA_23_P2_7 BEE_PSEL(DWGPIO, P2_7) /**< GPIOA_23 for P2_7 */

/* Port 3 */
#define BEE_PSEL_GPIOA_24_P3_0 BEE_PSEL(DWGPIO, P3_0) /**< GPIOA_24 for P3_0 */
#define BEE_PSEL_GPIOA_25_P3_1 BEE_PSEL(DWGPIO, P3_1) /**< GPIOA_25 for P3_1 */
#define BEE_PSEL_GPIOA_26_P3_2 BEE_PSEL(DWGPIO, P3_2) /**< GPIOA_26 for P3_2 */
#define BEE_PSEL_GPIOA_27_P3_3 BEE_PSEL(DWGPIO, P3_3) /**< GPIOA_27 for P3_3 */
#define BEE_PSEL_GPIOA_28_P3_4 BEE_PSEL(DWGPIO, P3_4) /**< GPIOA_28 for P3_4 */
#define BEE_PSEL_GPIOA_29_P3_5 BEE_PSEL(DWGPIO, P3_5) /**< GPIOA_29 for P3_5 */
#define BEE_PSEL_GPIOA_30_P3_6 BEE_PSEL(DWGPIO, P3_6) /**< GPIOA_30 for P3_6 */

/* Port 4 */
#define BEE_PSEL_GPIOA_13_P4_0 BEE_PSEL(DWGPIO, P4_0) /**< GPIOA_13 for P4_0 */
#define BEE_PSEL_GPIOA_29_P4_1 BEE_PSEL(DWGPIO, P4_1) /**< GPIOA_29 for P4_1 */
#define BEE_PSEL_GPIOA_30_P4_2 BEE_PSEL(DWGPIO, P4_2) /**< GPIOA_30 for P4_2 */
#define BEE_PSEL_GPIOA_31_P4_3 BEE_PSEL(DWGPIO, P4_3) /**< GPIOA_31 for P4_3 */

/* Port 5 */
#define BEE_PSEL_GPIOA_11_P5_1 BEE_PSEL(DWGPIO, P5_1) /**< GPIOA_11 for P5_1 */
#define BEE_PSEL_GPIOA_12_P5_2 BEE_PSEL(DWGPIO, P5_2) /**< GPIOA_12 for P5_2 */

/* Other Ports (H) */
#define BEE_PSEL_GPIOA_10_H_0 BEE_PSEL(DWGPIO, H_0) /**< GPIOA_10 for H_0 */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RTL8752H_PINCTRL_H_ */
