/* ---------------------------------------------------------------------------- */
/*                  Atmel Microcontroller Software Support                      */
/*                       SAM Software Package License                           */
/* ---------------------------------------------------------------------------- */
/* Copyright (c) %copyright_year%, Atmel Corporation                                        */
/*                                                                              */
/* All rights reserved.                                                         */
/*                                                                              */
/* Redistribution and use in source and binary forms, with or without           */
/* modification, are permitted provided that the following condition is met:    */
/*                                                                              */
/* - Redistributions of source code must retain the above copyright notice,     */
/* this list of conditions and the disclaimer below.                            */
/*                                                                              */
/* Atmel's name may not be used to endorse or promote products derived from     */
/* this software without specific prior written permission.                     */
/*                                                                              */
/* DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR   */
/* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE   */
/* DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,      */
/* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,  */
/* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    */
/* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING         */
/* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, */
/* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           */
/* ---------------------------------------------------------------------------- */

#ifndef _SAM4S4A_PIO_
#define _SAM4S4A_PIO_

#define PIO_PA0             (1u << 0)  /**< \brief Pin Controlled by PA0 */
#define PIO_PA1             (1u << 1)  /**< \brief Pin Controlled by PA1 */
#define PIO_PA2             (1u << 2)  /**< \brief Pin Controlled by PA2 */
#define PIO_PA3             (1u << 3)  /**< \brief Pin Controlled by PA3 */
#define PIO_PA4             (1u << 4)  /**< \brief Pin Controlled by PA4 */
#define PIO_PA5             (1u << 5)  /**< \brief Pin Controlled by PA5 */
#define PIO_PA6             (1u << 6)  /**< \brief Pin Controlled by PA6 */
#define PIO_PA7             (1u << 7)  /**< \brief Pin Controlled by PA7 */
#define PIO_PA8             (1u << 8)  /**< \brief Pin Controlled by PA8 */
#define PIO_PA9             (1u << 9)  /**< \brief Pin Controlled by PA9 */
#define PIO_PA10            (1u << 10) /**< \brief Pin Controlled by PA10 */
#define PIO_PA11            (1u << 11) /**< \brief Pin Controlled by PA11 */
#define PIO_PA12            (1u << 12) /**< \brief Pin Controlled by PA12 */
#define PIO_PA13            (1u << 13) /**< \brief Pin Controlled by PA13 */
#define PIO_PA14            (1u << 14) /**< \brief Pin Controlled by PA14 */
#define PIO_PA15            (1u << 15) /**< \brief Pin Controlled by PA15 */
#define PIO_PA16            (1u << 16) /**< \brief Pin Controlled by PA16 */
#define PIO_PA17            (1u << 17) /**< \brief Pin Controlled by PA17 */
#define PIO_PA18            (1u << 18) /**< \brief Pin Controlled by PA18 */
#define PIO_PA19            (1u << 19) /**< \brief Pin Controlled by PA19 */
#define PIO_PA20            (1u << 20) /**< \brief Pin Controlled by PA20 */
#define PIO_PB0             (1u << 0)  /**< \brief Pin Controlled by PB0 */
#define PIO_PB1             (1u << 1)  /**< \brief Pin Controlled by PB1 */
#define PIO_PB2             (1u << 2)  /**< \brief Pin Controlled by PB2 */
#define PIO_PB3             (1u << 3)  /**< \brief Pin Controlled by PB3 */
#define PIO_PB4             (1u << 4)  /**< \brief Pin Controlled by PB4 */
#define PIO_PB5             (1u << 5)  /**< \brief Pin Controlled by PB5 */
#define PIO_PB6             (1u << 6)  /**< \brief Pin Controlled by PB6 */
#define PIO_PB7             (1u << 7)  /**< \brief Pin Controlled by PB7 */
#define PIO_PB8             (1u << 8)  /**< \brief Pin Controlled by PB8 */
#define PIO_PB9             (1u << 9)  /**< \brief Pin Controlled by PB9 */
#define PIO_PB10            (1u << 10) /**< \brief Pin Controlled by PB10 */
#define PIO_PB11            (1u << 11) /**< \brief Pin Controlled by PB11 */
#define PIO_PB12            (1u << 12) /**< \brief Pin Controlled by PB12 */
/* ========== Pio definition for ADC peripheral ========== */
#define PIO_PA17X1_AD0      (1u << 17) /**< \brief Adc signal: AD0 */
#define PIO_PA18X1_AD1      (1u << 18) /**< \brief Adc signal: AD1 */
#define PIO_PC13X1_AD10     (1u << 13) /**< \brief Adc signal: AD10 */
#define PIO_PC15X1_AD11     (1u << 15) /**< \brief Adc signal: AD11 */
#define PIO_PC12X1_AD12     (1u << 12) /**< \brief Adc signal: AD12 */
#define PIO_PC29X1_AD13     (1u << 29) /**< \brief Adc signal: AD13 */
#define PIO_PC30X1_AD14     (1u << 30) /**< \brief Adc signal: AD14 */
#define PIO_PA19X1_AD2      (1u << 19) /**< \brief Adc signal: AD2/WKUP9 */
#define PIO_PA19X1_WKUP9    (1u << 19) /**< \brief Adc signal: AD2/WKUP9 */
#define PIO_PA20X1_AD3      (1u << 20) /**< \brief Adc signal: AD3/WKUP10 */
#define PIO_PA20X1_WKUP10   (1u << 20) /**< \brief Adc signal: AD3/WKUP10 */
#define PIO_PB0X1_AD4       (1u << 0)  /**< \brief Adc signal: AD4/RTCOUT0 */
#define PIO_PB0X1_RTCOUT0   (1u << 0)  /**< \brief Adc signal: AD4/RTCOUT0 */
#define PIO_PB1X1_AD5       (1u << 1)  /**< \brief Adc signal: AD5/RTCOUT1 */
#define PIO_PB1X1_RTCOUT1   (1u << 1)  /**< \brief Adc signal: AD5/RTCOUT1 */
#define PIO_PB2X1_AD6       (1u << 2)  /**< \brief Adc signal: AD6/WKUP12 */
#define PIO_PB2X1_WKUP12    (1u << 2)  /**< \brief Adc signal: AD6/WKUP12 */
#define PIO_PB3X1_AD7       (1u << 3)  /**< \brief Adc signal: AD7 */
#define PIO_PA21X1_AD8      (1u << 21) /**< \brief Adc signal: AD8 */
#define PIO_PA22X1_AD9      (1u << 22) /**< \brief Adc signal: AD9 */
#define PIO_PA8B_ADTRG      (1u << 8)  /**< \brief Adc signal: ADTRG */
/* ========== Pio definition for PIOA peripheral ========== */
#define PIO_PA24X1_PIODC0   (1u << 24) /**< \brief Pioa signal: PIODC0 */
#define PIO_PA25X1_PIODC1   (1u << 25) /**< \brief Pioa signal: PIODC1 */
#define PIO_PA26X1_PIODC2   (1u << 26) /**< \brief Pioa signal: PIODC2 */
#define PIO_PA27X1_PIODC3   (1u << 27) /**< \brief Pioa signal: PIODC3 */
#define PIO_PA28X1_PIODC4   (1u << 28) /**< \brief Pioa signal: PIODC4 */
#define PIO_PA29X1_PIODC5   (1u << 29) /**< \brief Pioa signal: PIODC5 */
#define PIO_PA31X1_PIODC7   (1u << 31) /**< \brief Pioa signal: PIODC7 */
#define PIO_PA23X1_PIODCCLK (1u << 23) /**< \brief Pioa signal: PIODCCLK */
#define PIO_PA30X1_WKUP11   (1u << 30) /**< \brief Pioa signal: WKUP11/PIODC6 */
#define PIO_PA30X1_PIODC6   (1u << 30) /**< \brief Pioa signal: WKUP11/PIODC6 */
#define PIO_PA15X1_WKUP14   (1u << 15) /**< \brief Pioa signal: WKUP14/PIODCEN1 */
#define PIO_PA15X1_PIODCEN1 (1u << 15) /**< \brief Pioa signal: WKUP14/PIODCEN1 */
#define PIO_PA16X1_WKUP15   (1u << 16) /**< \brief Pioa signal: WKUP15/PIODCEN2 */
#define PIO_PA16X1_PIODCEN2 (1u << 16) /**< \brief Pioa signal: WKUP15/PIODCEN2 */
/* ========== Pio definition for PMC peripheral ========== */
#define PIO_PA6B_PCK0       (1u << 6)  /**< \brief Pmc signal: PCK0 */
#define PIO_PB13B_PCK0      (1u << 13) /**< \brief Pmc signal: PCK0 */
#define PIO_PA17B_PCK1      (1u << 17) /**< \brief Pmc signal: PCK1 */
#define PIO_PA21B_PCK1      (1u << 21) /**< \brief Pmc signal: PCK1 */
#define PIO_PA18B_PCK2      (1u << 18) /**< \brief Pmc signal: PCK2 */
#define PIO_PA31B_PCK2      (1u << 31) /**< \brief Pmc signal: PCK2 */
#define PIO_PB3B_PCK2       (1u << 3)  /**< \brief Pmc signal: PCK2 */
/* ========== Pio definition for PWM peripheral ========== */
#define PIO_PA9C_PWMFI0     (1u << 9)  /**< \brief Pwm signal: PWMFI0 */
#define PIO_PA10C_PWMFI1    (1u << 10) /**< \brief Pwm signal: PWMFI1 */
#define PIO_PA18D_PWMFI2    (1u << 18) /**< \brief Pwm signal: PWMFI2 */
#define PIO_PA0A_PWMH0      (1u << 0)  /**< \brief Pwm signal: PWMH0 */
#define PIO_PA11B_PWMH0     (1u << 11) /**< \brief Pwm signal: PWMH0 */
#define PIO_PA23B_PWMH0     (1u << 23) /**< \brief Pwm signal: PWMH0 */
#define PIO_PB0A_PWMH0      (1u << 0)  /**< \brief Pwm signal: PWMH0 */
#define PIO_PC18B_PWMH0     (1u << 18) /**< \brief Pwm signal: PWMH0 */
#define PIO_PA1A_PWMH1      (1u << 1)  /**< \brief Pwm signal: PWMH1 */
#define PIO_PA12B_PWMH1     (1u << 12) /**< \brief Pwm signal: PWMH1 */
#define PIO_PA24B_PWMH1     (1u << 24) /**< \brief Pwm signal: PWMH1 */
#define PIO_PB1A_PWMH1      (1u << 1)  /**< \brief Pwm signal: PWMH1 */
#define PIO_PC19B_PWMH1     (1u << 19) /**< \brief Pwm signal: PWMH1 */
#define PIO_PA2A_PWMH2      (1u << 2)  /**< \brief Pwm signal: PWMH2 */
#define PIO_PA13B_PWMH2     (1u << 13) /**< \brief Pwm signal: PWMH2 */
#define PIO_PA25B_PWMH2     (1u << 25) /**< \brief Pwm signal: PWMH2 */
#define PIO_PB4B_PWMH2      (1u << 4)  /**< \brief Pwm signal: PWMH2 */
#define PIO_PC20B_PWMH2     (1u << 20) /**< \brief Pwm signal: PWMH2 */
#define PIO_PA7B_PWMH3      (1u << 7)  /**< \brief Pwm signal: PWMH3 */
#define PIO_PA14B_PWMH3     (1u << 14) /**< \brief Pwm signal: PWMH3 */
#define PIO_PA17C_PWMH3     (1u << 17) /**< \brief Pwm signal: PWMH3 */
#define PIO_PB14B_PWMH3     (1u << 14) /**< \brief Pwm signal: PWMH3 */
#define PIO_PC21B_PWMH3     (1u << 21) /**< \brief Pwm signal: PWMH3 */
#define PIO_PA19B_PWML0     (1u << 19) /**< \brief Pwm signal: PWML0 */
#define PIO_PB5B_PWML0      (1u << 5)  /**< \brief Pwm signal: PWML0 */
#define PIO_PC0B_PWML0      (1u << 0)  /**< \brief Pwm signal: PWML0 */
#define PIO_PC13B_PWML0     (1u << 13) /**< \brief Pwm signal: PWML0 */
#define PIO_PA20B_PWML1     (1u << 20) /**< \brief Pwm signal: PWML1 */
#define PIO_PB12A_PWML1     (1u << 12) /**< \brief Pwm signal: PWML1 */
#define PIO_PC1B_PWML1      (1u << 1)  /**< \brief Pwm signal: PWML1 */
#define PIO_PC15B_PWML1     (1u << 15) /**< \brief Pwm signal: PWML1 */
#define PIO_PA16C_PWML2     (1u << 16) /**< \brief Pwm signal: PWML2 */
#define PIO_PA30A_PWML2     (1u << 30) /**< \brief Pwm signal: PWML2 */
#define PIO_PB13A_PWML2     (1u << 13) /**< \brief Pwm signal: PWML2 */
#define PIO_PC2B_PWML2      (1u << 2)  /**< \brief Pwm signal: PWML2 */
#define PIO_PA15C_PWML3     (1u << 15) /**< \brief Pwm signal: PWML3 */
#define PIO_PC3B_PWML3      (1u << 3)  /**< \brief Pwm signal: PWML3 */
#define PIO_PC22B_PWML3     (1u << 22) /**< \brief Pwm signal: PWML3 */
/* ========== Pio definition for SPI peripheral ========== */
#define PIO_PA12A_MISO      (1u << 12) /**< \brief Spi signal: MISO */
#define PIO_PA13A_MOSI      (1u << 13) /**< \brief Spi signal: MOSI */
#define PIO_PA11A_NPCS0     (1u << 11) /**< \brief Spi signal: NPCS0 */
#define PIO_PA9B_NPCS1      (1u << 9)  /**< \brief Spi signal: NPCS1 */
#define PIO_PA31A_NPCS1     (1u << 31) /**< \brief Spi signal: NPCS1 */
#define PIO_PB14A_NPCS1     (1u << 14) /**< \brief Spi signal: NPCS1 */
#define PIO_PC4B_NPCS1      (1u << 4)  /**< \brief Spi signal: NPCS1 */
#define PIO_PA10B_NPCS2     (1u << 10) /**< \brief Spi signal: NPCS2 */
#define PIO_PA30B_NPCS2     (1u << 30) /**< \brief Spi signal: NPCS2 */
#define PIO_PB2B_NPCS2      (1u << 2)  /**< \brief Spi signal: NPCS2 */
#define PIO_PA3B_NPCS3      (1u << 3)  /**< \brief Spi signal: NPCS3 */
#define PIO_PA5B_NPCS3      (1u << 5)  /**< \brief Spi signal: NPCS3 */
#define PIO_PA22B_NPCS3     (1u << 22) /**< \brief Spi signal: NPCS3 */
#define PIO_PA14A_SPCK      (1u << 14) /**< \brief Spi signal: SPCK */
/* ========== Pio definition for SSC peripheral ========== */
#define PIO_PA18A_RD        (1u << 18) /**< \brief Ssc signal: RD */
#define PIO_PA20A_RF        (1u << 20) /**< \brief Ssc signal: RF */
#define PIO_PA19A_RK        (1u << 19) /**< \brief Ssc signal: RK */
#define PIO_PA17A_TD        (1u << 17) /**< \brief Ssc signal: TD */
#define PIO_PA15A_TF        (1u << 15) /**< \brief Ssc signal: TF */
#define PIO_PA16A_TK        (1u << 16) /**< \brief Ssc signal: TK */
/* ========== Pio definition for TC0 peripheral ========== */
#define PIO_PA4B_TCLK0      (1u << 4)  /**< \brief Tc0 signal: TCLK0 */
#define PIO_PA28B_TCLK1     (1u << 28) /**< \brief Tc0 signal: TCLK1 */
#define PIO_PA29B_TCLK2     (1u << 29) /**< \brief Tc0 signal: TCLK2 */
#define PIO_PA0B_TIOA0      (1u << 0)  /**< \brief Tc0 signal: TIOA0 */
#define PIO_PA15B_TIOA1     (1u << 15) /**< \brief Tc0 signal: TIOA1 */
#define PIO_PA26B_TIOA2     (1u << 26) /**< \brief Tc0 signal: TIOA2 */
#define PIO_PA1B_TIOB0      (1u << 1)  /**< \brief Tc0 signal: TIOB0 */
#define PIO_PA16B_TIOB1     (1u << 16) /**< \brief Tc0 signal: TIOB1 */
#define PIO_PA27B_TIOB2     (1u << 27) /**< \brief Tc0 signal: TIOB2 */
/* ========== Pio definition for TWI0 peripheral ========== */
#define PIO_PA4A_TWCK0      (1u << 4)  /**< \brief Twi0 signal: TWCK0 */
#define PIO_PA3A_TWD0       (1u << 3)  /**< \brief Twi0 signal: TWD0 */
/* ========== Pio definition for TWI1 peripheral ========== */
#define PIO_PB5A_TWCK1      (1u << 5)  /**< \brief Twi1 signal: TWCK1 */
#define PIO_PB4A_TWD1       (1u << 4)  /**< \brief Twi1 signal: TWD1 */
/* ========== Pio definition for UART0 peripheral ========== */
#define PIO_PA9A_URXD0      (1u << 9)  /**< \brief Uart0 signal: URXD0 */
#define PIO_PA10A_UTXD0     (1u << 10) /**< \brief Uart0 signal: UTXD0 */
/* ========== Pio definition for UART1 peripheral ========== */
#define PIO_PB2A_URXD1      (1u << 2)  /**< \brief Uart1 signal: URXD1 */
#define PIO_PB3A_UTXD1      (1u << 3)  /**< \brief Uart1 signal: UTXD1 */
/* ========== Pio definition for USART0 peripheral ========== */
#define PIO_PA8A_CTS0       (1u << 8)  /**< \brief Usart0 signal: CTS0 */
#define PIO_PA7A_RTS0       (1u << 7)  /**< \brief Usart0 signal: RTS0 */
#define PIO_PA5A_RXD0       (1u << 5)  /**< \brief Usart0 signal: RXD0 */
#define PIO_PA2B_SCK0       (1u << 2)  /**< \brief Usart0 signal: SCK0 */
#define PIO_PA6A_TXD0       (1u << 6)  /**< \brief Usart0 signal: TXD0 */
/* ========== Pio indexes ========== */
#define PIO_PA0_IDX         0
#define PIO_PA1_IDX         1
#define PIO_PA2_IDX         2
#define PIO_PA3_IDX         3
#define PIO_PA4_IDX         4
#define PIO_PA5_IDX         5
#define PIO_PA6_IDX         6
#define PIO_PA7_IDX         7
#define PIO_PA8_IDX         8
#define PIO_PA9_IDX         9
#define PIO_PA10_IDX        10
#define PIO_PA11_IDX        11
#define PIO_PA12_IDX        12
#define PIO_PA13_IDX        13
#define PIO_PA14_IDX        14
#define PIO_PA15_IDX        15
#define PIO_PA16_IDX        16
#define PIO_PA17_IDX        17
#define PIO_PA18_IDX        18
#define PIO_PA19_IDX        19
#define PIO_PA20_IDX        20
#define PIO_PB0_IDX         32
#define PIO_PB1_IDX         33
#define PIO_PB2_IDX         34
#define PIO_PB3_IDX         35
#define PIO_PB4_IDX         36
#define PIO_PB5_IDX         37
#define PIO_PB6_IDX         38
#define PIO_PB7_IDX         39
#define PIO_PB8_IDX         40
#define PIO_PB9_IDX         41
#define PIO_PB10_IDX        42
#define PIO_PB11_IDX        43
#define PIO_PB12_IDX        44

#endif /* _SAM4S4A_PIO_ */
