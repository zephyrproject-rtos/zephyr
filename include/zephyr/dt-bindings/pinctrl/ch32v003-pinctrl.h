/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CH32V003_PINCTRL_H__
#define __CH32V003_PINCTRL_H__

/* Pin number 0-15 */
#define CH32V003_PINCTRL_PIN_SHIFT   0
/* If set, pin is an output of some type */
#define CH32V003_PINCTRL_OUTPUT_BIT  3
/* Port number with 0 = PA */
#define CH32V003_PINCTRL_PORT_SHIFT  4
/* Bit number of the first AFIO PCFR1. 0x1F means unset. */
#define CH32V003_PINCTRL_AFIO0_SHIFT 6
/* Bit number of the second AFIO PCFR1. 0x1F means unset. */
#define CH32V003_PINCTRL_AFIO1_SHIFT 11
#define CH32V003_PINCTRL_AFIO_UNSET  0x1F

#define SPI1_NSS_PC1_0	   0xffe9
#define SPI1_CK_PC5_0	   0xffed
#define SPI1_MISO_PC7_0	   0xffe7
#define SPI1_MOSI_PC6_0	   0xffee
#define SPI1_NSS_PC0_1	   0xf828
#define SPI1_CK_PC5_1	   0xf82d
#define SPI1_MISO_PC7_1	   0xf827
#define SPI1_MOSI_PC6_1	   0xf82e
#define I2C1_SCL_PC2_0	   0xffea
#define I2C1_SDA_PC1_0	   0xffe9
#define I2C1_SCL_PD1_1	   0xf879
#define I2C1_SDA_PD0_1	   0xf878
#define I2C1_SCL_PC5_2	   0xbfed
#define I2C1_SDA_PC6_2	   0xbfee
#define USART1_CK_PD4_0	   0xfffc
#define USART1_TX_PD5_0	   0xfffd
#define USART1_RX_PD6_0	   0xfff6
#define USART1_CTS_PD3_0   0xfffb
#define USART1_RTS_PC2_0   0xffe2
#define USART1_SW_RX_PD5_0 0xfffd
#define USART1_CK_PD7_1	   0xf8bf
#define USART1_TX_PD0_1	   0xf8b8
#define USART1_RX_PD1_1	   0xf8b1
#define USART1_CTS_PC3_1   0xf8ab
#define USART1_RTS_PC2_1   0xf8a2
#define USART1_SW_RX_PD0_1 0xf8b8
#define USART1_CK_PD7_2	   0xafff
#define USART1_TX_PD6_2	   0xaffe
#define USART1_RX_PD5_2	   0xaff5
#define USART1_CTS_PC6_2   0xafee
#define USART1_RTS_PC7_2   0xafe7
#define USART1_SW_RX_PD6_2 0xaffe
#define USART1_CK_PC5_3	   0xa8ad
#define USART1_TX_PC0_3	   0xa8a8
#define USART1_RX_PC1_3	   0xa8a1
#define USART1_CTS_PC6_3   0xa8ae
#define USART1_RTS_PC7_3   0xa8a7
#define USART1_SW_RX_PC0_3 0xa8a8
#define TIM1_ETR_PC5_0	   0xffe5
#define TIM1_CH1_PD2_0	   0xfffa
#define TIM1_CH2_PA1_0	   0xffc9
#define TIM1_CH3_PC3_0	   0xffeb
#define TIM1_CH4_PC4_0	   0xffec
#define TIM1_BKIN_PC2_0	   0xffe2
#define TIM1_CH1N_PD0_0	   0xfff8
#define TIM1_CH2N_PA2_0	   0xffca
#define TIM1_CH3N_PD1_0	   0xfff9
#define TIM1_ETR_PC5_1	   0xf9a5
#define TIM1_CH1_PC6_1	   0xf9ae
#define TIM1_CH2_PC7_1	   0xf9af
#define TIM1_CH3_PC0_1	   0xf9a8
#define TIM1_CH4_PD3_1	   0xf9bb
#define TIM1_BKIN_PC1_1	   0xf9a1
#define TIM1_CH1N_PC3_1	   0xf9ab
#define TIM1_CH2N_PC4_1	   0xf9ac
#define TIM1_CH3N_PD1_1	   0xf9b9
#define TIM1_ETR_PD4_2	   0x3ff4
#define TIM1_CH1_PD2_2	   0x3ffa
#define TIM1_CH2_PA1_2	   0x3fc9
#define TIM1_CH3_PC3_2	   0x3feb
#define TIM1_CH4_PC4_2	   0x3fec
#define TIM1_BKIN_PC2_2	   0x3fe2
#define TIM1_CH1N_PD0_2	   0x3ff8
#define TIM1_CH2N_PA2_2	   0x3fca
#define TIM1_CH3N_PD1_2	   0x3ff9
#define TIM1_ETR_PC2_3	   0x39a2
#define TIM1_CH1_PC4_3	   0x39ac
#define TIM1_CH2_PC7_3	   0x39af
#define TIM1_CH3_PC5_3	   0x39ad
#define TIM1_CH4_PD4_3	   0x39bc
#define TIM1_BKIN_PC1_3	   0x39a1
#define TIM1_CH1N_PC3_3	   0x39ab
#define TIM1_CH2N_PD2_3	   0x39ba
#define TIM1_CH3N_PC6_3	   0x39ae
#define TIM2_CH2_PD3_0	   0xfffb
#define TIM2_CH3_PC0_0	   0xffe8
#define TIM2_CH4_PD7_0	   0xffff
#define TIM2_CH2_PC2_1	   0xfa2a
#define TIM2_CH3_PD2_1	   0xfa3a
#define TIM2_CH4_PC1_1	   0xfa29
#define TIM2_CH2_PD3_2	   0x4ffb
#define TIM2_CH3_PC0_2	   0x4fe8
#define TIM2_CH4_PD7_2	   0x4fff
#define TIM2_CH2_PC7_3	   0x4a2f
#define TIM2_CH3_PD6_3	   0x4a3e
#define TIM2_CH4_PD5_3	   0x4a3d
#define ADC1_ETRGINJ_PD3_0 0xfff3
#define ADC1_ETRGINJ_PC2_1 0xfc62
#define ADC1_ETRGREG_PD3_0 0xfff3
#define ADC1_ETRGREG_PC2_1 0xfca2

#endif /* __RP2040_PINCTRL_H__ */
