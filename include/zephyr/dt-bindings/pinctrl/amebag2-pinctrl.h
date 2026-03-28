/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AMEBAG2_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AMEBAG2_PINCTRL_H_

/**
 * @file
 * @brief Realtek AmebaG2 Pinctrl Devicetree bindings
 */

/**
 * @name Pinmux Function definitions
 * @{
 */
#define AMEBA_GPIO           0   /**< GPIO function */
#define AMEBA_LOG_UART       1   /**< Log UART function */
#define AMEBA_SPIC0_FLASH    2   /**< SPIC0 Flash function */
#define AMEBA_SPIC1_FLASH    3   /**< SPIC1 Flash function */
#define AMEBA_SPIC1_PSRAM    4   /**< SPIC1 PSRAM function */
#define AMEBA_ADC            5   /**< ADC function */
#define AMEBA_CAP_TOUCH      5   /**< Cap touch function */
#define AMEBA_TSSI           6   /**< TSSI function */
#define AMEBA_USB            7   /**< USB function */
#define AMEBA_RMII           8   /**< RMII function */
#define AMEBA_SPI0           9   /**< SPI0 function */
#define AMEBA_SPI1           10  /**< SPI1 function */
#define AMEBA_SWD            11  /**< SWD function */
#define AMEBA_SDIO_MST       12  /**< SDIO Master function */
#define AMEBA_SDIO_SLV       13  /**< SDIO Slave function */
#define AMEBA_SIC            14  /**< SIC function */
#define AMEBA_BT_IO          15  /**< BT IO function */
#define AMEBA_BT             16  /**< BT function */
#define AMEBA_DEBUG          17  /**< Debug function */
#define AMEBA_I2S0_BCLK      32  /**< I2S0 BCLK function */
#define AMEBA_I2S0_MCLK      33  /**< I2S0 MCLK function */
#define AMEBA_I2S0_WS        34  /**< I2S0 WS function */
#define AMEBA_I2S0_DIO0      35  /**< I2S0 DIO0 function */
#define AMEBA_I2S0_DIO1      36  /**< I2S0 DIO1 function */
#define AMEBA_I2S0_DIO2      37  /**< I2S0 DIO2 function */
#define AMEBA_I2S0_DIO3      38  /**< I2S0 DIO3 function */
#define AMEBA_LCD_RGB_DCLK   39  /**< LCD RGB DCLK function */
#define AMEBA_LCD_MCU_WR     39  /**< LCD MCU WR function */
#define AMEBA_LCD_RGB_DE     40  /**< LCD RGB DE function */
#define AMEBA_LCD_MCU_CSX    40  /**< LCD MCU CSX function */
#define AMEBA_LCD_RGB_HSYNC  41  /**< LCD RGB HSYNC function */
#define AMEBA_LCD_MCU_RD     41  /**< LCD MCU RD function */
#define AMEBA_LCD_RGB_TE     42  /**< LCD RGB TE function */
#define AMEBA_LCD_MCU_TE     42  /**< LCD MCU TE function */
#define AMEBA_LCD_RGB_VSYNC  43  /**< LCD RGB VSYNC function */
#define AMEBA_LCD_MCU_VSYNC  43  /**< LCD MCU VSYNC function */
#define AMEBA_LCD_MCU_DCX    44  /**< LCD MCU DCX function */
#define AMEBA_LCD_D0         45  /**< LCD D0 function */
#define AMEBA_LCD_D1         46  /**< LCD D1 function */
#define AMEBA_LCD_D2         47  /**< LCD D2 function */
#define AMEBA_LCD_D3         48  /**< LCD D3 function */
#define AMEBA_LCD_D4         49  /**< LCD D4 function */
#define AMEBA_LCD_D5         50  /**< LCD D5 function */
#define AMEBA_LCD_D6         51  /**< LCD D6 function */
#define AMEBA_LCD_D7         52  /**< LCD D7 function */
#define AMEBA_LCD_D8         53  /**< LCD D8 function */
#define AMEBA_LCD_D9         54  /**< LCD D9 function */
#define AMEBA_LCD_D10        55  /**< LCD D10 function */
#define AMEBA_LCD_D11        56  /**< LCD D11 function */
#define AMEBA_LCD_D12        57  /**< LCD D12 function */
#define AMEBA_LCD_D13        58  /**< LCD D13 function */
#define AMEBA_LCD_D14        59  /**< LCD D14 function */
#define AMEBA_LCD_D15        60  /**< LCD D15 function */
#define AMEBA_LCD_D16        61  /**< LCD D16 function */
#define AMEBA_LCD_D17        62  /**< LCD D17 function */
#define AMEBA_LCD_D18        63  /**< LCD D18 function */
#define AMEBA_LCD_D19        64  /**< LCD D19 function */
#define AMEBA_LCD_D20        65  /**< LCD D20 function */
#define AMEBA_LCD_D21        66  /**< LCD D21 function */
#define AMEBA_LCD_D22        67  /**< LCD D22 function */
#define AMEBA_LCD_D23        68  /**< LCD D23 function */
#define AMEBA_SD_M_CLK       69  /**< SD Master CLK function */
#define AMEBA_SD_M_CMD       70  /**< SD Master CMD function */
#define AMEBA_SD_M_D0        71  /**< SD Master D0 function */
#define AMEBA_SD_M_D1        72  /**< SD Master D1 function */
#define AMEBA_SD_M_D2        73  /**< SD Master D2 function */
#define AMEBA_SD_M_D3        74  /**< SD Master D3 function */
#define AMEBA_SPI0_CLK       75  /**< SPI0 CLK function */
#define AMEBA_SPI0_MISO      76  /**< SPI0 MISO function */
#define AMEBA_SPI0_MOSI      77  /**< SPI0 MOSI function */
#define AMEBA_SPI0_CS        78  /**< SPI0 CS function */
#define AMEBA_SPI1_CLK       79  /**< SPI1 CLK function */
#define AMEBA_SPI1_MISO      80  /**< SPI1 MISO function */
#define AMEBA_SPI1_MOSI      81  /**< SPI1 MOSI function */
#define AMEBA_SPI1_CS        82  /**< SPI1 CS function */
#define AMEBA_SWD_CLK        83  /**< SWD CLK function */
#define AMEBA_SWD_DAT        84  /**< SWD DAT function */
#define AMEBA_DMIC_CLK       85  /**< DMIC CLK function */
#define AMEBA_DMIC_DATA      86  /**< DMIC DATA function */
#define AMEBA_LED_SCL        87  /**< LED SCL function */
#define AMEBA_LED_SDA        88  /**< LED SDA function */
#define AMEBA_I2C0_SCL       89  /**< I2C0 SCL function */
#define AMEBA_I2C0_SDA       90  /**< I2C0 SDA function */
#define AMEBA_I2C1_SCL       91  /**< I2C1 SCL function */
#define AMEBA_I2C1_SDA       92  /**< I2C1 SDA function */
#define AMEBA_RMII_MDIO      93  /**< RMII MDIO function */
#define AMEBA_RMII_MDC       94  /**< RMII MDC function */
#define AMEBA_UART0_TXD      95  /**< UART0 TXD function */
#define AMEBA_UART0_RXD      96  /**< UART0 RXD function */
#define AMEBA_UART0_CTS      97  /**< UART0 CTS function */
#define AMEBA_UART0_RTS      98  /**< UART0 RTS function */
#define AMEBA_UART1_TXD      99  /**< UART1 TXD function */
#define AMEBA_UART1_RXD      100 /**< UART1 RXD function */
#define AMEBA_UART2_TXD      101 /**< UART2 TXD function */
#define AMEBA_UART2_RXD      102 /**< UART2 RXD function */
#define AMEBA_UART3_TXD      103 /**< UART3 TXD function */
#define AMEBA_UART3_RXD      104 /**< UART3 RXD function */
#define AMEBA_UART3_CTS      105 /**< UART3 CTS function */
#define AMEBA_UART3_RTS      106 /**< UART3 RTS function */
#define AMEBA_A2C0_TX        107 /**< A2C0 TX function */
#define AMEBA_A2C0_RX        108 /**< A2C0 RX function */
#define AMEBA_A2C1_TX        109 /**< A2C1 TX function */
#define AMEBA_A2C1_RX        110 /**< A2C1 RX function */
#define AMEBA_TIM4_PWM0      111 /**< TIM4 PWM0 function */
#define AMEBA_TIM4_PWM1      112 /**< TIM4 PWM1 function */
#define AMEBA_TIM4_PWM2      113 /**< TIM4 PWM2 function */
#define AMEBA_TIM4_PWM3      114 /**< TIM4 PWM3 function */
#define AMEBA_TIM5_PWM0      115 /**< TIM5 PWM0 function */
#define AMEBA_TIM5_PWM1      116 /**< TIM5 PWM1 function */
#define AMEBA_TIM5_PWM2      117 /**< TIM5 PWM2 function */
#define AMEBA_TIM5_PWM3      118 /**< TIM5 PWM3 function */
#define AMEBA_TIM6_PWM0      119 /**< TIM6 PWM0 function */
#define AMEBA_TIM6_PWM1      120 /**< TIM6 PWM1 function */
#define AMEBA_TIM6_PWM2      121 /**< TIM6 PWM2 function */
#define AMEBA_TIM6_PWM3      122 /**< TIM6 PWM3 function */
#define AMEBA_TIM7_PWM0      123 /**< TIM7 PWM0 function */
#define AMEBA_TIM7_PWM1      124 /**< TIM7 PWM1 function */
#define AMEBA_TIM7_PWM2      125 /**< TIM7 PWM2 function */
#define AMEBA_TIM7_PWM3      126 /**< TIM7 PWM3 function */
#define AMEBA_PWM_TIM4_TRIG  127 /**< PWM TIM4 TRIG function */
#define AMEBA_PWM_TIM5_TRIG  128 /**< PWM TIM5 TRIG function */
#define AMEBA_PWM_TIM6_TRIG  129 /**< PWM TIM6 TRIG function */
#define AMEBA_PWM_TIM7_TRIG  130 /**< PWM TIM7 TRIG function */
#define AMEBA_CAPT_TIM8_TRIG 131 /**< CAPT TIM8 TRIG function */
#define AMEBA_IR_TX          132 /**< IR TX function */
#define AMEBA_IR_RX          133 /**< IR RX function */
#define AMEBA_ANT_SEL_P      134 /**< ANT SEL P function */
#define AMEBA_ANT_SEL_N      135 /**< ANT SEL N function */
#define AMEBA_TRSW_P         136 /**< TRSW P function */
#define AMEBA_TRSW_N         137 /**< TRSW N function */
#define AMEBA_PA_EN0         138 /**< PA EN0 function */
#define AMEBA_LNA_EN0        139 /**< LNA EN0 function */
#define AMEBA_PA_EN1         140 /**< PA EN1 function */
#define AMEBA_LNA_EN1        141 /**< LNA EN1 function */
#define AMEBA_BT_CLK_REQ     142 /**< BT CLK REQ function */
#define AMEBA_WLAN_ACT       143 /**< WLAN ACT function */
#define AMEBA_BT_ACT         144 /**< BT ACT function */
#define AMEBA_BT_STE         145 /**< BT STE function */
#define AMEBA_BT_CK          146 /**< BT CK function */
#define AMEBA_ZB_REQ         147 /**< ZB REQ function */
#define AMEBA_ZB_GRANT       148 /**< ZB GRANT function */
#define AMEBA_ZB_PRI         149 /**< ZB PRI function */
#define AMEBA_BT_ANT_SW0     150 /**< BT ANT SW0 function */
#define AMEBA_BT_ANT_SW1     151 /**< BT ANT SW1 function */
#define AMEBA_BT_ANT_SW2     152 /**< BT ANT SW2 function */
#define AMEBA_BT_ANT_SW3     153 /**< BT ANT SW3 function */
#define AMEBA_EXT_CLK50M_IN  154 /**< EXT CLK50M IN function */
#define AMEBA_EXT_CLK_OUT    155 /**< EXT CLK OUT function */
#define AMEBA_UART_LOG_RXD   156 /**< UART LOG RXD function */
#define AMEBA_UART_LOG_TXD   157 /**< UART LOG TXD function */
#define AMEBA_SIC_CLK        158 /**< SIC CLK function */
#define AMEBA_SIC_DAT        159 /**< SIC DAT function */
#define AMEBA_BT_FWLOG       160 /**< BT FWLOG function */
#define AMEBA_BT_I2C_SDA     161 /**< BT I2C SDA function */
#define AMEBA_BT_I2C_SCL     162 /**< BT I2C SCL function */
/** @} */

/**
 * @brief Encode port and pin number.
 */
#define AMEBA_PORT_PIN(port, line) ((((port) - 'A') << 5) + (line))

/**
 * @brief Encode pinmux configuration.
 *
 * Format: bit[14:13] port, bit[12:8] pin, bit[7:0] function ID
 */
#define AMEBA_PINMUX(port, line, funcid) (((AMEBA_PORT_PIN(port, line)) << 8) | (funcid))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_AMEBAG2_PINCTRL_H_ */
