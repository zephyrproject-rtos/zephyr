/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RTL87X2G Pin Control (Pinmux) Header
 *
 * This file defines the pinmux functions and pin assignments for the
 * Realtek RTL87X2G series SoC.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RTL87X2G_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RTL87X2G_PINCTRL_H_

#include "bee-pinctrl.h"

/**
 * @name Pinctrl Pinmux Functions
 * @{
 */
#define BEE_IDLE_MODE          0                    /**< IDLE_MODE pinmux function */
#define BEE_UART0_TX           1                    /**< UART0_TX pinmux function */
#define BEE_UART0_RX           2                    /**< UART0_RX pinmux function */
#define BEE_UART0_CTS          3                    /**< UART0_CTS pinmux function */
#define BEE_UART0_RTS          4                    /**< UART0_RTS pinmux function */
#define BEE_UART1_TX           5                    /**< UART1_TX pinmux function */
#define BEE_UART1_RX           6                    /**< UART1_RX pinmux function */
#define BEE_UART1_CTS          7                    /**< UART1_CTS pinmux function */
#define BEE_UART1_RTS          8                    /**< UART1_RTS pinmux function */
#define BEE_UART2_TX           9                    /**< UART2_TX pinmux function */
#define BEE_UART2_RX           10                   /**< UART2_RX pinmux function */
#define BEE_UART2_CTS          11                   /**< UART2_CTS pinmux function */
#define BEE_UART2_RTS          12                   /**< UART2_RTS pinmux function */
#define BEE_UART3_TX           13                   /**< UART3_TX pinmux function */
#define BEE_UART3_RX           14                   /**< UART3_RX pinmux function */
#define BEE_UART3_CTS          15                   /**< UART3_CTS pinmux function */
#define BEE_UART3_RTS          16                   /**< UART3_RTS pinmux function */
#define BEE_UART4_TX           17                   /**< UART4_TX pinmux function */
#define BEE_UART4_RX           18                   /**< UART4_RX pinmux function */
#define BEE_UART4_CTS          19                   /**< UART4_CTS pinmux function */
#define BEE_UART4_RTS          20                   /**< UART4_RTS pinmux function */
#define BEE_UART5_TX           21                   /**< UART5_TX pinmux function */
#define BEE_UART5_RX           22                   /**< UART5_RX pinmux function */
#define BEE_UART5_CTS          23                   /**< UART5_CTS pinmux function */
#define BEE_UART5_RTS          24                   /**< UART5_RTS pinmux function */
#define BEE_I2C0_CLK           29                   /**< I2C0_CLK pinmux function */
#define BEE_I2C0_DAT           30                   /**< I2C0_DAT pinmux function */
#define BEE_I2C1_CLK           31                   /**< I2C1_CLK pinmux function */
#define BEE_I2C1_DAT           32                   /**< I2C1_DAT pinmux function */
#define BEE_I2C2_CLK           33                   /**< I2C2_CLK pinmux function */
#define BEE_I2C2_DAT           34                   /**< I2C2_DAT pinmux function */
#define BEE_I2C3_CLK           35                   /**< I2C3_CLK pinmux function */
#define BEE_I2C3_DAT           36                   /**< I2C3_DAT pinmux function */
#define BEE_SPI0_CLK_MASTER    37                   /**< SPI0_CLK_MASTER pinmux function */
#define BEE_SPI0_MO_MASTER     38                   /**< SPI0_MO_MASTER pinmux function */
#define BEE_SPI0_MI_MASTER     39                   /**< SPI0_MI_MASTER pinmux function */
#define BEE_SPI0_CSN_0_MASTER  40                   /**< SPI0_CSN_0_MASTER pinmux function */
#define BEE_SPI0_CSN_1_MASTER  41                   /**< SPI0_CSN_1_MASTER pinmux function */
#define BEE_SPI0_CSN_2_MASTER  42                   /**< SPI0_CSN_2_MASTER pinmux function */
#define BEE_SPI0_CSN_0_SLAVE   43                   /**< SPI0_CSN_0_SLAVE pinmux function */
#define BEE_SPI0_CLK_SLAVE     44                   /**< SPI0_CLK_SLAVE pinmux function */
#define BEE_SPI0_SO_SLAVE      45                   /**< SPI0_SO_SLAVE pinmux function */
#define BEE_SPI0_SI_SLAVE      46                   /**< SPI0_SI_SLAVE pinmux function */
#define BEE_SPI1_CLK_MASTER    47                   /**< SPI1_CLK_MASTER pinmux function */
#define BEE_SPI1_MO_MASTER     48                   /**< SPI1_MO_MASTER pinmux function */
#define BEE_SPI1_MI_MASTER     49                   /**< SPI1_MI_MASTER pinmux function */
#define BEE_SPI1_CSN_0_MASTER  50                   /**< SPI1_CSN_0_MASTER pinmux function */
#define BEE_SPI1_CSN_1_MASTER  51                   /**< SPI1_CSN_1_MASTER pinmux function */
#define BEE_SPI1_CSN_2_MASTER  52                   /**< SPI1_CSN_2_MASTER pinmux function */
#define BEE_SPI2W_DATA         53                   /**< SPI2W_DATA pinmux function */
#define BEE_SPI2W_CLK          54                   /**< SPI2W_CLK pinmux function */
#define BEE_SPI2W_CS           55                   /**< SPI2W_CS pinmux function */
#define BEE_ENPWM0_P           65                   /**< ENPWM0_P pinmux function */
#define BEE_ENPWM0_N           66                   /**< ENPWM0_N pinmux function */
#define BEE_ENPWM1_P           67                   /**< ENPWM1_P pinmux function */
#define BEE_ENPWM1_N           68                   /**< ENPWM1_N pinmux function */
#define BEE_ENPWM2_P           69                   /**< ENPWM2_P pinmux function */
#define BEE_ENPWM2_N           70                   /**< ENPWM2_N pinmux function */
#define BEE_ENPWM3_P           71                   /**< ENPWM3_P pinmux function */
#define BEE_ENPWM3_N           72                   /**< ENPWM3_N pinmux function */
#define BEE_TIMER_PWM2         83                   /**< TIMER_PWM2 pinmux function */
#define BEE_TIMER_PWM3         84                   /**< TIMER_PWM3 pinmux function */
#define BEE_ISO7816_RST        85                   /**< ISO7816_RST pinmux function */
#define BEE_ISO7816_CLK        86                   /**< ISO7816_CLK pinmux function */
#define BEE_ISO7816_IO         87                   /**< ISO7816_IO pinmux function */
#define BEE_ISO7816_VCC_EN     88                   /**< ISO7816_VCC_EN pinmux function */
#define BEE_DWGPIO             89                   /**< DWGPIO pinmux function */
#define BEE_IRDA_TX            90                   /**< IRDA_TX pinmux function */
#define BEE_IRDA_RX            91                   /**< IRDA_RX pinmux function */
#define BEE_TIMER_PWM4         92                   /**< TIMER_PWM4 pinmux function */
#define BEE_TIMER_PWM5         93                   /**< TIMER_PWM5 pinmux function */
#define BEE_TIMER_PWM6         94                   /**< TIMER_PWM6 pinmux function */
#define BEE_TIMER_PWM7         95                   /**< TIMER_PWM7 pinmux function */
#define BEE_TIMER_PWM2_P       96                   /**< TIMER_PWM2_P pinmux function */
#define BEE_TIMER_PWM2_N       97                   /**< TIMER_PWM2_N pinmux function */
#define BEE_TIMER_PWM3_P       98                   /**< TIMER_PWM3_P pinmux function */
#define BEE_TIMER_PWM3_N       99                   /**< TIMER_PWM3_N pinmux function */
#define BEE_KEY_COL_0          100                  /**< KEY_COL_0 pinmux function */
#define BEE_KEY_COL_1          101                  /**< KEY_COL_1 pinmux function */
#define BEE_KEY_COL_2          102                  /**< KEY_COL_2 pinmux function */
#define BEE_KEY_COL_3          103                  /**< KEY_COL_3 pinmux function */
#define BEE_KEY_COL_4          104                  /**< KEY_COL_4 pinmux function */
#define BEE_KEY_COL_5          105                  /**< KEY_COL_5 pinmux function */
#define BEE_KEY_COL_6          106                  /**< KEY_COL_6 pinmux function */
#define BEE_KEY_COL_7          107                  /**< KEY_COL_7 pinmux function */
#define BEE_KEY_COL_8          108                  /**< KEY_COL_8 pinmux function */
#define BEE_KEY_COL_9          109                  /**< KEY_COL_9 pinmux function */
#define BEE_KEY_COL_10         110                  /**< KEY_COL_10 pinmux function */
#define BEE_KEY_COL_11         111                  /**< KEY_COL_11 pinmux function */
#define BEE_KEY_COL_12         112                  /**< KEY_COL_12 pinmux function */
#define BEE_KEY_COL_13         113                  /**< KEY_COL_13 pinmux function */
#define BEE_KEY_COL_14         114                  /**< KEY_COL_14 pinmux function */
#define BEE_KEY_COL_15         115                  /**< KEY_COL_15 pinmux function */
#define BEE_KEY_COL_16         116                  /**< KEY_COL_16 pinmux function */
#define BEE_KEY_COL_17         117                  /**< KEY_COL_17 pinmux function */
#define BEE_KEY_COL_18         118                  /**< KEY_COL_18 pinmux function */
#define BEE_KEY_COL_19         119                  /**< KEY_COL_19 pinmux function */
#define BEE_KEY_ROW_0          120                  /**< KEY_ROW_0 pinmux function */
#define BEE_KEY_ROW_1          121                  /**< KEY_ROW_1 pinmux function */
#define BEE_KEY_ROW_2          122                  /**< KEY_ROW_2 pinmux function */
#define BEE_KEY_ROW_3          123                  /**< KEY_ROW_3 pinmux function */
#define BEE_KEY_ROW_4          124                  /**< KEY_ROW_4 pinmux function */
#define BEE_KEY_ROW_5          125                  /**< KEY_ROW_5 pinmux function */
#define BEE_KEY_ROW_6          126                  /**< KEY_ROW_6 pinmux function */
#define BEE_KEY_ROW_7          127                  /**< KEY_ROW_7 pinmux function */
#define BEE_KEY_ROW_8          128                  /**< KEY_ROW_8 pinmux function */
#define BEE_KEY_ROW_9          129                  /**< KEY_ROW_9 pinmux function */
#define BEE_KEY_ROW_10         130                  /**< KEY_ROW_10 pinmux function */
#define BEE_KEY_ROW_11         131                  /**< KEY_ROW_11 pinmux function */
#define BEE_KM4_CLK_DIV_4      138                  /**< KM4_CLK_DIV_4 pinmux function */
#define BEE_CARD_DETECT_N_0    147                  /**< CARD_DETECT_N_0 pinmux function */
#define BEE_BIU_VOLT_REG_0     148                  /**< BIU_VOLT_REG_0 pinmux function */
#define BEE_BACK_END_POWER_0   149                  /**< BACK_END_POWER_0 pinmux function */
#define BEE_CARD_INT_N_SDHC_0  150                  /**< CARD_INT_N_SDHC_0 pinmux function */
#define BEE_A2C_TX             155                  /**< A2C_TX pinmux function */
#define BEE_A2C_RX             156                  /**< A2C_RX pinmux function */
#define BEE_LRC_SPORT1         157                  /**< LRC_SPORT1 pinmux function */
#define BEE_BCLK_SPORT1        158                  /**< BCLK_SPORT1 pinmux function */
#define BEE_ADCDAT_SPORT1      159                  /**< ADCDAT_SPORT1 pinmux function */
#define BEE_DACDAT_SPORT1      160                  /**< DACDAT_SPORT1 pinmux function */
#define BEE_DMIC1_CLK          162                  /**< DMIC1_CLK pinmux function */
#define BEE_DMIC1_DAT          163                  /**< DMIC1_DAT pinmux function */
#define BEE_LRC_I_CODEC_SLAVE  164                  /**< LRC_I_CODEC_SLAVE pinmux function */
#define BEE_BCLK_I_CODEC_SLAVE 165                  /**< BCLK_I_CODEC_SLAVE pinmux function */
#define BEE_SDI_CODEC_SLAVE    166                  /**< SDI_CODEC_SLAVE pinmux function */
#define BEE_SDO_CODEC_SLAVE    167                  /**< SDO_CODEC_SLAVE pinmux function */
#define BEE_LRC_SPORT0         172                  /**< LRC_SPORT0 pinmux function */
#define BEE_BCLK_SPORT0        173                  /**< BCLK_SPORT0 pinmux function */
#define BEE_ADCDAT_SPORT0      174                  /**< ADCDAT_SPORT0 pinmux function */
#define BEE_DACDAT_SPORT0      175                  /**< DACDAT_SPORT0 pinmux function */
#define BEE_MCLK_OUT           176                  /**< MCLK_OUT pinmux function */
#define BEE_MCLK_IN            189                  /**< MCLK_IN pinmux function */
#define BEE_LRC_RX_CODEC_SLAVE 190                  /**< LRC_RX_CODEC_SLAVE pinmux function */
#define BEE_LRC_RX_SPORT0      191                  /**< LRC_RX_SPORT0 pinmux function */
#define BEE_LRC_RX_SPORT1      192                  /**< LRC_RX_SPORT1 pinmux function */
#define BEE_PDM_DATA           196                  /**< PDM_DATA pinmux function */
#define BEE_PDM_CLK            197                  /**< PDM_CLK pinmux function */
#define BEE_I2S1_LRC_TX_SLAVE  198                  /**< I2S1_LRC_TX_SLAVE pinmux function */
#define BEE_I2S1_BCLK_SLAVE    199                  /**< I2S1_BCLK_SLAVE pinmux function */
#define BEE_I2S1_SDI_SLAVE     200                  /**< I2S1_SDI_SLAVE pinmux function */
#define BEE_I2S1_SDO_SLAVE     201                  /**< I2S1_SDO_SLAVE pinmux function */
#define BEE_I2S1_LRC_RX_SLAVE  202                  /**< I2S1_LRC_RX_SLAVE pinmux function */
#define BEE_BT_COEX_I_0        216                  /**< BT_COEX_I_0 pinmux function */
#define BEE_BT_COEX_I_1        217                  /**< BT_COEX_I_1 pinmux function */
#define BEE_BT_COEX_I_2        218                  /**< BT_COEX_I_2 pinmux function */
#define BEE_BT_COEX_I_3        219                  /**< BT_COEX_I_3 pinmux function */
#define BEE_BT_COEX_O_0        220                  /**< BT_COEX_O_0 pinmux function */
#define BEE_BT_COEX_O_1        221                  /**< BT_COEX_O_1 pinmux function */
#define BEE_BT_COEX_O_2        222                  /**< BT_COEX_O_2 pinmux function */
#define BEE_BT_COEX_O_3        223                  /**< BT_COEX_O_3 pinmux function */
#define BEE_PTA_I2C_CLK_SLAVE  224                  /**< PTA_I2C_CLK_SLAVE pinmux function */
#define BEE_PTA_I2C_DAT_SLAVE  225                  /**< PTA_I2C_DAT_SLAVE pinmux function */
#define BEE_PTA_I2C_INT_OUT    226                  /**< PTA_I2C_INT_OUT pinmux function */
#define BEE_EN_EXPA            227                  /**< EN_EXPA pinmux function */
#define BEE_EN_EXLNA           228                  /**< EN_EXLNA pinmux function */
#define BEE_SEL_TPM_SW         229                  /**< SEL_TPM_SW pinmux function */
#define BEE_SEL_TPM_N_SW       230                  /**< SEL_TPM_N_SW pinmux function */
#define BEE_ANT_SW0            231                  /**< ANT_SW0 pinmux function */
#define BEE_ANT_SW1            232                  /**< ANT_SW1 pinmux function */
#define BEE_ANT_SW2            233                  /**< ANT_SW2 pinmux function */
#define BEE_ANT_SW3            234                  /**< ANT_SW3 pinmux function */
#define BEE_ANT_SW4            235                  /**< ANT_SW4 pinmux function */
#define BEE_ANT_SW5            236                  /**< ANT_SW5 pinmux function */
#define BEE_PHY_GPIO_1         237                  /**< PHY_GPIO_1 pinmux function */
#define BEE_PHY_GPIO_2         238                  /**< PHY_GPIO_2 pinmux function */
#define BEE_SLOW_DEBUG_MUX_1   239                  /**< SLOW_DEBUG_MUX_1 pinmux function */
#define BEE_SLOW_DEBUG_MUX_2   240                  /**< SLOW_DEBUG_MUX_2 pinmux function */
#define BEE_TEST_MODE          246                  /**< TEST_MODE pinmux function */
#define BEE_SWD_CLK            253                  /**< SWD_CLK pinmux function */
#define BEE_SWD_DIO            254                  /**< SWD_DIO pinmux function */
#define BEE_DIG_DEBUG          255                  /**< DIG_DEBUG pinmux function */
#define BEE_PINMUX_MAX         (BEE_DIG_DEBUG + 1)  /**< PINMUX_MAX pinmux function */
#define BEE_SW_MODE            (BEE_PINMUX_MAX + 1) /**< SW_MODE pinmux function */
#define BEE_PWR_OFF            (BEE_PINMUX_MAX + 2) /**< PWR_OFF pinmux function */
#define BEE_SDHC0_CLK_P9_4     (BEE_PWR_OFF + 1)    /**< SDHC0_CLK_P9_4 pinmux function */
#define BEE_SDHC0_CMD_P9_3     (BEE_PWR_OFF + 2)    /**< SDHC0_CMD_P9_3 pinmux function */
#define BEE_SDHC0_D0_P10_0     (BEE_PWR_OFF + 3)    /**< SDHC0_D0_P10_0 pinmux function */
#define BEE_SDHC0_D1_P9_7      (BEE_PWR_OFF + 4)    /**< SDHC0_D1_P9_7 pinmux function */
#define BEE_SDHC0_D2_P9_6      (BEE_PWR_OFF + 5)    /**< SDHC0_D2_P9_6 pinmux function */
#define BEE_SDHC0_D3_P9_5      (BEE_PWR_OFF + 6)    /**< SDHC0_D3_P9_5 pinmux function */
#define BEE_SDHC0_D4_P4_4      (BEE_PWR_OFF + 7)    /**< SDHC0_D4_P4_4 pinmux function */
#define BEE_SDHC0_D5_P4_5      (BEE_PWR_OFF + 8)    /**< SDHC0_D5_P4_5 pinmux function */
#define BEE_SDHC0_D6_P4_6      (BEE_PWR_OFF + 9)    /**< SDHC0_D6_P4_6 pinmux function */
#define BEE_SDHC0_D7_P4_7      (BEE_PWR_OFF + 10)   /**< SDHC0_D7_P4_7 pinmux function */
#define BEE_SDHC1_CLK_P9_4     (BEE_PWR_OFF + 11)   /**< SDHC1_CLK_P9_4 pinmux function */
#define BEE_SDHC1_CMD_P9_3     (BEE_PWR_OFF + 12)   /**< SDHC1_CMD_P9_3 pinmux function */
#define BEE_SDHC1_D0_P10_0     (BEE_PWR_OFF + 13)   /**< SDHC1_D0_P10_0 pinmux function */
#define BEE_SDHC1_D1_P9_7      (BEE_PWR_OFF + 14)   /**< SDHC1_D1_P9_7 pinmux function */
#define BEE_SDHC1_D2_P9_6      (BEE_PWR_OFF + 15)   /**< SDHC1_D2_P9_6 pinmux function */
#define BEE_SDHC1_D3_P9_5      (BEE_PWR_OFF + 16)   /**< SDHC1_D3_P9_5 pinmux function */
#define BEE_SDHC1_D4_P4_4      (BEE_PWR_OFF + 17)   /**< SDHC1_D4_P4_4 pinmux function */
#define BEE_SDHC1_D5_P4_5      (BEE_PWR_OFF + 18)   /**< SDHC1_D5_P4_5 pinmux function */
#define BEE_SDHC1_D6_P4_6      (BEE_PWR_OFF + 19)   /**< SDHC1_D6_P4_6 pinmux function */
#define BEE_SDHC1_D7_P4_7      (BEE_PWR_OFF + 20)   /**< SDHC1_D7_P4_7 pinmux function */
#define BEE_QDPH0_IN_NONE      0x0F00               /**< QDPH0_IN_NONE      pinmux function */
#define BEE_QDPH0_IN_P1_3_P1_4 0x0F01               /**< QDPH0_IN_P1_3_P1_4 pinmux function */
#define BEE_QDPH0_IN_P5_6_P5_7 0x0F02               /**< QDPH0_IN_P5_6_P5_7 pinmux function */
#define BEE_QDPH0_IN_P9_0_P9_1 0x0F03               /**< QDPH0_IN_P9_0_P9_1 pinmux function */
#define BEE_PIN_DISCONNECTED   BEE_PIN_MSK          /**< PIN_DISCONNECTED pinmux function */
/** @} */

/**
 * @name Pinctrl Available Pins
 * @{
 */
#define P0_0    0 /**< GPIOA_0 */
#define P0_1    1 /**< GPIOA_1 */
#define P0_2    2 /**< GPIOA_2 */
/* Note: P0_3 defaults to outputting the Realtek internal log for rtl87x2g. */
#define P0_3    3 /**< GPIOA_3 */
#define P0_4    4 /**< GPIOA_4 */
#define P0_5    5 /**< GPIOA_5 */
#define P0_6    6 /**< GPIOA_6 */
#define P0_7    7 /**< GPIOA_7 */
/* Note: P0_0/P0_1 default to SWD function for rtl87x2g. */
#define P1_0    8    /**< GPIOA_8 */
#define P1_1    9    /**< GPIOA_9 */
#define P1_2    10   /**< GPIOA_10 */
#define P1_3    11   /**< GPIOA_11 */
#define P1_4    12   /**< GPIOA_12 */
#define P1_5    13   /**< GPIOA_13 */
#define P1_6    14   /**< GPIOA_14 */
#define P1_7    15   /**< GPIOA_15 */
#define P2_0    16   /**< GPIOA_21 */
#define P2_1    17   /**< GPIOA_22 */
#define P2_2    18   /**< GPIOA_23 */
#define P2_3    19   /**< GPIOA_24 */
#define P2_4    20   /**< GPIOA_25 */
#define P2_5    21   /**< GPIOA_26 */
#define P2_6    22   /**< GPIOA_27 */
#define P2_7    23   /**< GPIOA_28 */
#define P3_0    24   /**< GPIOA_29 */
#define P3_1    25   /**< GPIOA_30 */
#define P3_2    26   /**< GPIOA_31 */
#define P3_3    27   /**< GPIOB_0 */
#define P3_4    28   /**< GPIOB_1 */
#define P3_5    29   /**< GPIOB_2 */
#define P3_6    30   /**< GPIOB_3 */
#define P3_7    31   /**< GPIOB_4 */
#define P4_0    32   /**< GPIOB_5 */
#define P4_1    33   /**< GPIOB_6 */
#define P4_2    34   /**< GPIOB_7 */
#define P4_3    35   /**< GPIOB_8 */
#define P4_4    36   /**< GPIOB_9 */
#define P4_5    37   /**< GPIOB_10 */
#define P4_6    38   /**< GPIOB_11 */
#define P4_7    39   /**< GPIOB_12 */
#define P5_0    40   /**< GPIOB_13 */
#define P5_1    41   /**< GPIOB_14 */
#define P5_2    42   /**< GPIOB_15 */
#define P5_3    43   /**< GPIOB_16 */
#define P5_4    44   /**< GPIOB_17 */
#define P5_5    45   /**< GPIOB_18 */
#define P5_6    46   /**< GPIOB_19 */
#define P5_7    47   /**< GPIOB_20 */
#define P6_0    48   /**< GPIOB_19 */
#define P6_1    49   /**< GPIOB_20 */
#define P6_2    50   /**< GPIOB_21 */
#define P6_3    51   /**< GPIOB_22 */
#define P6_4    52   /**< GPIOB_23 */
#define P6_5    53   /**< GPIOB_24 */
#define P6_6    54   /**< GPIOB_25 */
#define P6_7    55   /**< GPIOB_26 */
#define P7_0    56   /**< GPIOB_27 */
#define P7_1    57   /**< GPIOB_28 */
#define P7_2    58   /**< GPIOB_29 */
#define P7_3    59   /**< GPIOB_30 */
#define P7_4    60   /**< GPIOB_31 */
#define MICBIAS 64   /**< P8_0 GPIOA_16 */
#define XI32K   65   /**< P8_1 GPIOA_17 */
#define XO32K   66   /**< P8_2 GPIOA_18 */
#define DACP    67   /**< P8_3 GPIOA_19 */
#define DACN    68   /**< P8_4 GPIOA_20 */
#define P9_0    72   /**< GPIOB_21 */
#define P9_1    73   /**< GPIOB_22 */
#define P9_2    74   /**< GPIOB_23 */
#define P9_3    75   /**< GPIOB_24 */
#define P9_4    76   /**< GPIOB_25 */
#define P9_5    77   /**< GPIOB_26 */
#define P9_6    78   /**< GPIOB_27 */
#define P9_7    79   /**< GPIOB_28 */
#define P10_0   80   /**< GPIOB_29 */
#define P10_1   81   /**< GPIOB_30 */
#define P10_2   82   /**< GPIOB_31 */
#define ADC_0   P2_0 /**< GPIO16 */
#define ADC_1   P2_1 /**< GPIO17 */
#define ADC_2   P2_2 /**< GPIO18 */
#define ADC_3   P2_3 /**< GPIO19 */
#define ADC_4   P2_4 /**< GPIO20 */
#define ADC_5   P2_5 /**< GPIO21 */
#define ADC_6   P2_6 /**< GPIO22 */
#define ADC_7   P2_7 /**< GPIO23 */
/** @} */

/**
 * @name GPIO Pinctrl Configuration
 * @{
 */
/* Port 0 */
#define BEE_PSEL_GPIOA_0_P0_0 BEE_PSEL(DWGPIO, P0_0) /**< GPIOA_0 for P0_0 */
#define BEE_PSEL_GPIOA_1_P0_1 BEE_PSEL(DWGPIO, P0_1) /**< GPIOA_1 for P0_1 */
#define BEE_PSEL_GPIOA_2_P0_2 BEE_PSEL(DWGPIO, P0_2) /**< GPIOA_2 for P0_2 */
/* Note: P0_3 defaults to outputting the Realtek internal log for rtl87x2g. */
#define BEE_PSEL_GPIOA_3_P0_3 BEE_PSEL(DWGPIO, P0_3) /**< GPIOA_3 for P0_3 */
#define BEE_PSEL_GPIOA_4_P0_4 BEE_PSEL(DWGPIO, P0_4) /**< GPIOA_4 for P0_4 */
#define BEE_PSEL_GPIOA_5_P0_5 BEE_PSEL(DWGPIO, P0_5) /**< GPIOA_5 for P0_5 */
#define BEE_PSEL_GPIOA_6_P0_6 BEE_PSEL(DWGPIO, P0_6) /**< GPIOA_6 for P0_6 */
#define BEE_PSEL_GPIOA_7_P0_7 BEE_PSEL(DWGPIO, P0_7) /**< GPIOA_7 for P0_7 */

/* Port 1 */
/* Note: P0_0/P0_1 default to SWD function for rtl87x2g. */
#define BEE_PSEL_GPIOA_8_P1_0  BEE_PSEL(DWGPIO, P1_0) /**< GPIOA_8 for P1_0 */
#define BEE_PSEL_GPIOA_9_P1_1  BEE_PSEL(DWGPIO, P1_1) /**< GPIOA_9 for P1_1 */
#define BEE_PSEL_GPIOA_10_P1_2 BEE_PSEL(DWGPIO, P1_2) /**< GPIOA_10 for P1_2 */
#define BEE_PSEL_GPIOA_11_P1_3 BEE_PSEL(DWGPIO, P1_3) /**< GPIOA_11 for P1_3 */
#define BEE_PSEL_GPIOA_12_P1_4 BEE_PSEL(DWGPIO, P1_4) /**< GPIOA_12 for P1_4 */
#define BEE_PSEL_GPIOA_13_P1_5 BEE_PSEL(DWGPIO, P1_5) /**< GPIOA_13 for P1_5 */
#define BEE_PSEL_GPIOA_14_P1_6 BEE_PSEL(DWGPIO, P1_6) /**< GPIOA_14 for P1_6 */
#define BEE_PSEL_GPIOA_15_P1_7 BEE_PSEL(DWGPIO, P1_7) /**< GPIOA_15 for P1_7 */

/* Port 2 */
#define BEE_PSEL_GPIOA_21_P2_0 BEE_PSEL(DWGPIO, P2_0) /**< GPIOA_21 for P2_0 */
#define BEE_PSEL_GPIOA_22_P2_1 BEE_PSEL(DWGPIO, P2_1) /**< GPIOA_22 for P2_1 */
#define BEE_PSEL_GPIOA_23_P2_2 BEE_PSEL(DWGPIO, P2_2) /**< GPIOA_23 for P2_2 */
#define BEE_PSEL_GPIOA_24_P2_3 BEE_PSEL(DWGPIO, P2_3) /**< GPIOA_24 for P2_3 */
#define BEE_PSEL_GPIOA_25_P2_4 BEE_PSEL(DWGPIO, P2_4) /**< GPIOA_25 for P2_4 */
#define BEE_PSEL_GPIOA_26_P2_5 BEE_PSEL(DWGPIO, P2_5) /**< GPIOA_26 for P2_5 */
#define BEE_PSEL_GPIOA_27_P2_6 BEE_PSEL(DWGPIO, P2_6) /**< GPIOA_27 for P2_6 */
#define BEE_PSEL_GPIOA_28_P2_7 BEE_PSEL(DWGPIO, P2_7) /**< GPIOA_28 for P2_7 */

/* Port 3 */
#define BEE_PSEL_GPIOA_29_P3_0 BEE_PSEL(DWGPIO, P3_0) /**< GPIOA_29 for P3_0 */
#define BEE_PSEL_GPIOA_30_P3_1 BEE_PSEL(DWGPIO, P3_1) /**< GPIOA_30 for P3_1 */
#define BEE_PSEL_GPIOA_31_P3_2 BEE_PSEL(DWGPIO, P3_2) /**< GPIOA_31 for P3_2 */
#define BEE_PSEL_GPIOB_0_P3_3  BEE_PSEL(DWGPIO, P3_3) /**< GPIOB_0 for P3_3 */
#define BEE_PSEL_GPIOB_1_P3_4  BEE_PSEL(DWGPIO, P3_4) /**< GPIOB_1 for P3_4 */
#define BEE_PSEL_GPIOB_2_P3_5  BEE_PSEL(DWGPIO, P3_5) /**< GPIOB_2 for P3_5 */
#define BEE_PSEL_GPIOB_3_P3_6  BEE_PSEL(DWGPIO, P3_6) /**< GPIOB_3 for P3_6 */
#define BEE_PSEL_GPIOB_4_P3_7  BEE_PSEL(DWGPIO, P3_7) /**< GPIOB_4 for P3_7 */

/* Port 4 */
#define BEE_PSEL_GPIOB_5_P4_0  BEE_PSEL(DWGPIO, P4_0) /**< GPIOB_5 for P4_0 */
#define BEE_PSEL_GPIOB_6_P4_1  BEE_PSEL(DWGPIO, P4_1) /**< GPIOB_6 for P4_1 */
#define BEE_PSEL_GPIOB_7_P4_2  BEE_PSEL(DWGPIO, P4_2) /**< GPIOB_7 for P4_2 */
#define BEE_PSEL_GPIOB_8_P4_3  BEE_PSEL(DWGPIO, P4_3) /**< GPIOB_8 for P4_3 */
#define BEE_PSEL_GPIOB_9_P4_4  BEE_PSEL(DWGPIO, P4_4) /**< GPIOB_9 for P4_4 */
#define BEE_PSEL_GPIOB_10_P4_5 BEE_PSEL(DWGPIO, P4_5) /**< GPIOB_10 for P4_5 */
#define BEE_PSEL_GPIOB_11_P4_6 BEE_PSEL(DWGPIO, P4_6) /**< GPIOB_11 for P4_6 */
#define BEE_PSEL_GPIOB_12_P4_7 BEE_PSEL(DWGPIO, P4_7) /**< GPIOB_12 for P4_7 */

/* Port 5 */
#define BEE_PSEL_GPIOB_13_P5_0 BEE_PSEL(DWGPIO, P5_0) /**< GPIOB_13 for P5_0 */
#define BEE_PSEL_GPIOB_14_P5_1 BEE_PSEL(DWGPIO, P5_1) /**< GPIOB_14 for P5_1 */
#define BEE_PSEL_GPIOB_15_P5_2 BEE_PSEL(DWGPIO, P5_2) /**< GPIOB_15 for P5_2 */
#define BEE_PSEL_GPIOB_16_P5_3 BEE_PSEL(DWGPIO, P5_3) /**< GPIOB_16 for P5_3 */
#define BEE_PSEL_GPIOB_17_P5_4 BEE_PSEL(DWGPIO, P5_4) /**< GPIOB_17 for P5_4 */
#define BEE_PSEL_GPIOB_18_P5_5 BEE_PSEL(DWGPIO, P5_5) /**< GPIOB_18 for P5_5 */
#define BEE_PSEL_GPIOB_19_P5_6 BEE_PSEL(DWGPIO, P5_6) /**< GPIOB_19 for P5_6 */
#define BEE_PSEL_GPIOB_20_P5_7 BEE_PSEL(DWGPIO, P5_7) /**< GPIOB_20 for P5_7 */

/* Port 6 */
#define BEE_PSEL_GPIOB_19_P6_0 BEE_PSEL(DWGPIO, P6_0) /**< GPIOB_19 for P6_0 */
#define BEE_PSEL_GPIOB_20_P6_1 BEE_PSEL(DWGPIO, P6_1) /**< GPIOB_20 for P6_1 */
#define BEE_PSEL_GPIOB_21_P6_2 BEE_PSEL(DWGPIO, P6_2) /**< GPIOB_21 for P6_2 */
#define BEE_PSEL_GPIOB_22_P6_3 BEE_PSEL(DWGPIO, P6_3) /**< GPIOB_22 for P6_3 */
#define BEE_PSEL_GPIOB_23_P6_4 BEE_PSEL(DWGPIO, P6_4) /**< GPIOB_23 for P6_4 */
#define BEE_PSEL_GPIOB_24_P6_5 BEE_PSEL(DWGPIO, P6_5) /**< GPIOB_24 for P6_5 */
#define BEE_PSEL_GPIOB_25_P6_6 BEE_PSEL(DWGPIO, P6_6) /**< GPIOB_25 for P6_6 */
#define BEE_PSEL_GPIOB_26_P6_7 BEE_PSEL(DWGPIO, P6_7) /**< GPIOB_26 for P6_7 */

/* Port 7 */
#define BEE_PSEL_GPIOB_27_P7_0 BEE_PSEL(DWGPIO, P7_0) /**< GPIOB_27 for P7_0 */
#define BEE_PSEL_GPIOB_28_P7_1 BEE_PSEL(DWGPIO, P7_1) /**< GPIOB_28 for P7_1 */
#define BEE_PSEL_GPIOB_29_P7_2 BEE_PSEL(DWGPIO, P7_2) /**< GPIOB_29 for P7_2 */
#define BEE_PSEL_GPIOB_30_P7_3 BEE_PSEL(DWGPIO, P7_3) /**< GPIOB_30 for P7_3 */
#define BEE_PSEL_GPIOB_31_P7_4 BEE_PSEL(DWGPIO, P7_4) /**< GPIOB_31 for P7_4 */

/* Special Functions / Port 8 Equivalent */
/* Note: Using P8_x extracted from comments */
#define BEE_PSEL_GPIOA_16_P8_0 BEE_PSEL(DWGPIO, MICBIAS) /**< GPIOA_16 for P8_0 */
#define BEE_PSEL_GPIOA_17_P8_1 BEE_PSEL(DWGPIO, XI32K)   /**< GPIOA_17 for P8_1 */
#define BEE_PSEL_GPIOA_18_P8_2 BEE_PSEL(DWGPIO, XO32K)   /**< GPIOA_18 for P8_2 */
#define BEE_PSEL_GPIOA_19_P8_3 BEE_PSEL(DWGPIO, DACP)    /**< GPIOA_19 for P8_3 */
#define BEE_PSEL_GPIOA_20_P8_4 BEE_PSEL(DWGPIO, DACN)    /**< GPIOA_20 for P8_4 */

/* Port 9 */
#define BEE_PSEL_GPIOB_21_P9_0 BEE_PSEL(DWGPIO, P9_0) /**< GPIOB_21 for P9_0 */
#define BEE_PSEL_GPIOB_22_P9_1 BEE_PSEL(DWGPIO, P9_1) /**< GPIOB_22 for P9_1 */
#define BEE_PSEL_GPIOB_23_P9_2 BEE_PSEL(DWGPIO, P9_2) /**< GPIOB_23 for P9_2 */
#define BEE_PSEL_GPIOB_24_P9_3 BEE_PSEL(DWGPIO, P9_3) /**< GPIOB_24 for P9_3 */
#define BEE_PSEL_GPIOB_25_P9_4 BEE_PSEL(DWGPIO, P9_4) /**< GPIOB_25 for P9_4 */
#define BEE_PSEL_GPIOB_26_P9_5 BEE_PSEL(DWGPIO, P9_5) /**< GPIOB_26 for P9_5 */
#define BEE_PSEL_GPIOB_27_P9_6 BEE_PSEL(DWGPIO, P9_6) /**< GPIOB_27 for P9_6 */
#define BEE_PSEL_GPIOB_28_P9_7 BEE_PSEL(DWGPIO, P9_7) /**< GPIOB_28 for P9_7 */

/* Port 10 */
#define BEE_PSEL_GPIOB_29_P10_0 BEE_PSEL(DWGPIO, P10_0) /**< GPIOB_29 for P10_0 */
#define BEE_PSEL_GPIOB_30_P10_1 BEE_PSEL(DWGPIO, P10_1) /**< GPIOB_30 for P10_1 */
#define BEE_PSEL_GPIOB_31_P10_2 BEE_PSEL(DWGPIO, P10_2) /**< GPIOB_31 for P10_2 */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RTL87X2G_PINCTRL_H_ */
