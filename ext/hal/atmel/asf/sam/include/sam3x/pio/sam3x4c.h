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

#ifndef _SAM3X4C_PIO_
#define _SAM3X4C_PIO_

#define PIO_PA0              (1u << 0)  /**< \brief Pin Controlled by PA0 */
#define PIO_PA1              (1u << 1)  /**< \brief Pin Controlled by PA1 */
#define PIO_PA2              (1u << 2)  /**< \brief Pin Controlled by PA2 */
#define PIO_PA3              (1u << 3)  /**< \brief Pin Controlled by PA3 */
#define PIO_PA4              (1u << 4)  /**< \brief Pin Controlled by PA4 */
#define PIO_PA5              (1u << 5)  /**< \brief Pin Controlled by PA5 */
#define PIO_PA6              (1u << 6)  /**< \brief Pin Controlled by PA6 */
#define PIO_PA7              (1u << 7)  /**< \brief Pin Controlled by PA7 */
#define PIO_PA8              (1u << 8)  /**< \brief Pin Controlled by PA8 */
#define PIO_PA9              (1u << 9)  /**< \brief Pin Controlled by PA9 */
#define PIO_PA10             (1u << 10) /**< \brief Pin Controlled by PA10 */
#define PIO_PA11             (1u << 11) /**< \brief Pin Controlled by PA11 */
#define PIO_PA12             (1u << 12) /**< \brief Pin Controlled by PA12 */
#define PIO_PA13             (1u << 13) /**< \brief Pin Controlled by PA13 */
#define PIO_PA14             (1u << 14) /**< \brief Pin Controlled by PA14 */
#define PIO_PA15             (1u << 15) /**< \brief Pin Controlled by PA15 */
#define PIO_PA16             (1u << 16) /**< \brief Pin Controlled by PA16 */
#define PIO_PA17             (1u << 17) /**< \brief Pin Controlled by PA17 */
#define PIO_PA18             (1u << 18) /**< \brief Pin Controlled by PA18 */
#define PIO_PA19             (1u << 19) /**< \brief Pin Controlled by PA19 */
#define PIO_PA20             (1u << 20) /**< \brief Pin Controlled by PA20 */
#define PIO_PA21             (1u << 21) /**< \brief Pin Controlled by PA21 */
#define PIO_PA22             (1u << 22) /**< \brief Pin Controlled by PA22 */
#define PIO_PA23             (1u << 23) /**< \brief Pin Controlled by PA23 */
#define PIO_PA24             (1u << 24) /**< \brief Pin Controlled by PA24 */
#define PIO_PA25             (1u << 25) /**< \brief Pin Controlled by PA25 */
#define PIO_PA26             (1u << 26) /**< \brief Pin Controlled by PA26 */
#define PIO_PA27             (1u << 27) /**< \brief Pin Controlled by PA27 */
#define PIO_PA28             (1u << 28) /**< \brief Pin Controlled by PA28 */
#define PIO_PA29             (1u << 29) /**< \brief Pin Controlled by PA29 */
#define PIO_PB0              (1u << 0)  /**< \brief Pin Controlled by PB0 */
#define PIO_PB1              (1u << 1)  /**< \brief Pin Controlled by PB1 */
#define PIO_PB2              (1u << 2)  /**< \brief Pin Controlled by PB2 */
#define PIO_PB3              (1u << 3)  /**< \brief Pin Controlled by PB3 */
#define PIO_PB4              (1u << 4)  /**< \brief Pin Controlled by PB4 */
#define PIO_PB5              (1u << 5)  /**< \brief Pin Controlled by PB5 */
#define PIO_PB6              (1u << 6)  /**< \brief Pin Controlled by PB6 */
#define PIO_PB7              (1u << 7)  /**< \brief Pin Controlled by PB7 */
#define PIO_PB8              (1u << 8)  /**< \brief Pin Controlled by PB8 */
#define PIO_PB9              (1u << 9)  /**< \brief Pin Controlled by PB9 */
#define PIO_PB10             (1u << 10) /**< \brief Pin Controlled by PB10 */
#define PIO_PB11             (1u << 11) /**< \brief Pin Controlled by PB11 */
#define PIO_PB12             (1u << 12) /**< \brief Pin Controlled by PB12 */
#define PIO_PB13             (1u << 13) /**< \brief Pin Controlled by PB13 */
#define PIO_PB14             (1u << 14) /**< \brief Pin Controlled by PB14 */
#define PIO_PB15             (1u << 15) /**< \brief Pin Controlled by PB15 */
#define PIO_PB16             (1u << 16) /**< \brief Pin Controlled by PB16 */
#define PIO_PB17             (1u << 17) /**< \brief Pin Controlled by PB17 */
#define PIO_PB18             (1u << 18) /**< \brief Pin Controlled by PB18 */
#define PIO_PB19             (1u << 19) /**< \brief Pin Controlled by PB19 */
#define PIO_PB20             (1u << 20) /**< \brief Pin Controlled by PB20 */
#define PIO_PB21             (1u << 21) /**< \brief Pin Controlled by PB21 */
#define PIO_PB22             (1u << 22) /**< \brief Pin Controlled by PB22 */
#define PIO_PB23             (1u << 23) /**< \brief Pin Controlled by PB23 */
#define PIO_PB24             (1u << 24) /**< \brief Pin Controlled by PB24 */
#define PIO_PB25             (1u << 25) /**< \brief Pin Controlled by PB25 */
#define PIO_PB26             (1u << 26) /**< \brief Pin Controlled by PB26 */
#define PIO_PB27             (1u << 27) /**< \brief Pin Controlled by PB27 */
#define PIO_PB28             (1u << 28) /**< \brief Pin Controlled by PB28 */
#define PIO_PB29             (1u << 29) /**< \brief Pin Controlled by PB29 */
#define PIO_PB30             (1u << 30) /**< \brief Pin Controlled by PB30 */
#define PIO_PB31             (1u << 31) /**< \brief Pin Controlled by PB31 */
/* ========== Pio definition for ADC peripheral ========== */
#define PIO_PA2X1_AD0        (1u << 2)  /**< \brief Adc signal: AD0 */
#define PIO_PA3X1_AD1        (1u << 3)  /**< \brief Adc signal: AD1/WKUP1 */
#define PIO_PA3X1_WKUP1      (1u << 3)  /**< \brief Adc signal: AD1/WKUP1 */
#define PIO_PB17X1_AD10      (1u << 17) /**< \brief Adc signal: AD10 */
#define PIO_PB18X1_AD11      (1u << 18) /**< \brief Adc signal: AD11 */
#define PIO_PB19X1_AD12      (1u << 19) /**< \brief Adc signal: AD12 */
#define PIO_PB20X1_AD13      (1u << 20) /**< \brief Adc signal: AD13 */
#define PIO_PB21X1_AD14      (1u << 21) /**< \brief Adc signal: AD14/WKUP13 */
#define PIO_PB21X1_WKUP13    (1u << 21) /**< \brief Adc signal: AD14/WKUP13 */
#define PIO_PA4X1_AD2        (1u << 4)  /**< \brief Adc signal: AD2 */
#define PIO_PA6X1_AD3        (1u << 6)  /**< \brief Adc signal: AD3 */
#define PIO_PA22X1_AD4       (1u << 22) /**< \brief Adc signal: AD4 */
#define PIO_PA23X1_AD5       (1u << 23) /**< \brief Adc signal: AD5 */
#define PIO_PA24X1_AD6       (1u << 24) /**< \brief Adc signal: AD6 */
#define PIO_PA16X1_AD7       (1u << 16) /**< \brief Adc signal: AD7 */
#define PIO_PB12X1_AD8       (1u << 12) /**< \brief Adc signal: AD8 */
#define PIO_PB13X1_AD9       (1u << 13) /**< \brief Adc signal: AD9 */
#define PIO_PA11B_ADTRG      (1u << 11) /**< \brief Adc signal: ADTRG */
/* ========== Pio definition for CAN0 peripheral ========== */
#define PIO_PA1A_CANRX0      (1u << 1)  /**< \brief Can0 signal: CANRX0 */
#define PIO_PA0A_CANTX0      (1u << 0)  /**< \brief Can0 signal: CANTX0 */
/* ========== Pio definition for CAN1 peripheral ========== */
#define PIO_PB15A_CANRX1     (1u << 15) /**< \brief Can1 signal: CANRX1 */
#define PIO_PB14A_CANTX1     (1u << 14) /**< \brief Can1 signal: CANTX1 */
/* ========== Pio definition for DACC peripheral ========== */
#define PIO_PB15X1_DAC0      (1u << 15) /**< \brief Dacc signal: DAC0/WKUP12 */
#define PIO_PB15X1_WKUP12    (1u << 15) /**< \brief Dacc signal: DAC0/WKUP12 */
#define PIO_PB16X1_DAC1      (1u << 16) /**< \brief Dacc signal: DAC1 */
#define PIO_PA10B_DATRG      (1u << 10) /**< \brief Dacc signal: DATRG */
/* ========== Pio definition for EMAC peripheral ========== */
#define PIO_PC13B_ECOL       (1u << 13) /**< \brief Emac signal: ECOL */
#define PIO_PC10B_ECRS       (1u << 10) /**< \brief Emac signal: ECRS */
#define PIO_PB4A_ECRSDV      (1u << 4)  /**< \brief Emac signal: ECRSDV/ERXDV */
#define PIO_PB4A_ERXDV       (1u << 4)  /**< \brief Emac signal: ECRSDV/ERXDV */
#define PIO_PB8A_EMDC        (1u << 8)  /**< \brief Emac signal: EMDC */
#define PIO_PB9A_EMDIO       (1u << 9)  /**< \brief Emac signal: EMDIO */
#define PIO_PB5A_ERX0        (1u << 5)  /**< \brief Emac signal: ERX0 */
#define PIO_PB6A_ERX1        (1u << 6)  /**< \brief Emac signal: ERX1 */
#define PIO_PC11B_ERX2       (1u << 11) /**< \brief Emac signal: ERX2 */
#define PIO_PC12B_ERX3       (1u << 12) /**< \brief Emac signal: ERX3 */
#define PIO_PC14B_ERXCK      (1u << 14) /**< \brief Emac signal: ERXCK */
#define PIO_PB7A_ERXER       (1u << 7)  /**< \brief Emac signal: ERXER */
#define PIO_PB2A_ETX0        (1u << 2)  /**< \brief Emac signal: ETX0 */
#define PIO_PB3A_ETX1        (1u << 3)  /**< \brief Emac signal: ETX1 */
#define PIO_PC15B_ETX2       (1u << 15) /**< \brief Emac signal: ETX2 */
#define PIO_PC16B_ETX3       (1u << 16) /**< \brief Emac signal: ETX3 */
#define PIO_PB0A_ETXCK       (1u << 0)  /**< \brief Emac signal: ETXCK */
#define PIO_PB1A_ETXEN       (1u << 1)  /**< \brief Emac signal: ETXEN */
#define PIO_PC17B_ETXER      (1u << 17) /**< \brief Emac signal: ETXER */
/* ========== Pio definition for HSMCI peripheral ========== */
#define PIO_PA20A_MCCDA      (1u << 20) /**< \brief Hsmci signal: MCCDA */
#define PIO_PE20B_MCCDB      (1u << 20) /**< \brief Hsmci signal: MCCDB */
#define PIO_PA19A_MCCK       (1u << 19) /**< \brief Hsmci signal: MCCK */
#define PIO_PA21A_MCDA0      (1u << 21) /**< \brief Hsmci signal: MCDA0 */
#define PIO_PA22A_MCDA1      (1u << 22) /**< \brief Hsmci signal: MCDA1 */
#define PIO_PA23A_MCDA2      (1u << 23) /**< \brief Hsmci signal: MCDA2 */
#define PIO_PA24A_MCDA3      (1u << 24) /**< \brief Hsmci signal: MCDA3 */
#define PIO_PD0B_MCDA4       (1u << 0)  /**< \brief Hsmci signal: MCDA4 */
#define PIO_PD1B_MCDA5       (1u << 1)  /**< \brief Hsmci signal: MCDA5 */
#define PIO_PD2B_MCDA6       (1u << 2)  /**< \brief Hsmci signal: MCDA6 */
#define PIO_PD3B_MCDA7       (1u << 3)  /**< \brief Hsmci signal: MCDA7 */
#define PIO_PE22B_MCDB0      (1u << 22) /**< \brief Hsmci signal: MCDB0 */
#define PIO_PE24B_MCDB1      (1u << 24) /**< \brief Hsmci signal: MCDB1 */
#define PIO_PE26B_MCDB2      (1u << 26) /**< \brief Hsmci signal: MCDB2 */
#define PIO_PE27B_MCDB3      (1u << 27) /**< \brief Hsmci signal: MCDB3 */
/* ========== Pio definition for PMC peripheral ========== */
#define PIO_PA1B_PCK0        (1u << 1)  /**< \brief Pmc signal: PCK0 */
#define PIO_PB22B_PCK0       (1u << 22) /**< \brief Pmc signal: PCK0 */
#define PIO_PA24B_PCK1       (1u << 24) /**< \brief Pmc signal: PCK1 */
#define PIO_PA30B_PCK1       (1u << 30) /**< \brief Pmc signal: PCK1 */
#define PIO_PA28B_PCK2       (1u << 28) /**< \brief Pmc signal: PCK2 */
#define PIO_PA31B_PCK2       (1u << 31) /**< \brief Pmc signal: PCK2 */
/* ========== Pio definition for PWM peripheral ========== */
#define PIO_PA5B_PWMFI0      (1u << 5)  /**< \brief Pwm signal: PWMFI0 */
#define PIO_PA3B_PWMFI1      (1u << 3)  /**< \brief Pwm signal: PWMFI1 */
#define PIO_PD6B_PWMFI2      (1u << 6)  /**< \brief Pwm signal: PWMFI2 */
#define PIO_PA8B_PWMH0       (1u << 8)  /**< \brief Pwm signal: PWMH0 */
#define PIO_PB12B_PWMH0      (1u << 12) /**< \brief Pwm signal: PWMH0 */
#define PIO_PC3B_PWMH0       (1u << 3)  /**< \brief Pwm signal: PWMH0 */
#define PIO_PE15A_PWMH0      (1u << 15) /**< \brief Pwm signal: PWMH0 */
#define PIO_PA19B_PWMH1      (1u << 19) /**< \brief Pwm signal: PWMH1 */
#define PIO_PB13B_PWMH1      (1u << 13) /**< \brief Pwm signal: PWMH1 */
#define PIO_PC5B_PWMH1       (1u << 5)  /**< \brief Pwm signal: PWMH1 */
#define PIO_PE16A_PWMH1      (1u << 16) /**< \brief Pwm signal: PWMH1 */
#define PIO_PA13B_PWMH2      (1u << 13) /**< \brief Pwm signal: PWMH2 */
#define PIO_PB14B_PWMH2      (1u << 14) /**< \brief Pwm signal: PWMH2 */
#define PIO_PC7B_PWMH2       (1u << 7)  /**< \brief Pwm signal: PWMH2 */
#define PIO_PA9B_PWMH3       (1u << 9)  /**< \brief Pwm signal: PWMH3 */
#define PIO_PB15B_PWMH3      (1u << 15) /**< \brief Pwm signal: PWMH3 */
#define PIO_PC9B_PWMH3       (1u << 9)  /**< \brief Pwm signal: PWMH3 */
#define PIO_PF3A_PWMH3       (1u << 3)  /**< \brief Pwm signal: PWMH3 */
#define PIO_PC20B_PWMH4      (1u << 20) /**< \brief Pwm signal: PWMH4 */
#define PIO_PE20A_PWMH4      (1u << 20) /**< \brief Pwm signal: PWMH4 */
#define PIO_PC19B_PWMH5      (1u << 19) /**< \brief Pwm signal: PWMH5 */
#define PIO_PE22A_PWMH5      (1u << 22) /**< \brief Pwm signal: PWMH5 */
#define PIO_PC18B_PWMH6      (1u << 18) /**< \brief Pwm signal: PWMH6 */
#define PIO_PE24A_PWMH6      (1u << 24) /**< \brief Pwm signal: PWMH6 */
#define PIO_PE26A_PWMH7      (1u << 26) /**< \brief Pwm signal: PWMH7 */
#define PIO_PA21B_PWML0      (1u << 21) /**< \brief Pwm signal: PWML0 */
#define PIO_PB16B_PWML0      (1u << 16) /**< \brief Pwm signal: PWML0 */
#define PIO_PC2B_PWML0       (1u << 2)  /**< \brief Pwm signal: PWML0 */
#define PIO_PE18A_PWML0      (1u << 18) /**< \brief Pwm signal: PWML0 */
#define PIO_PA12B_PWML1      (1u << 12) /**< \brief Pwm signal: PWML1 */
#define PIO_PB17B_PWML1      (1u << 17) /**< \brief Pwm signal: PWML1 */
#define PIO_PC4B_PWML1       (1u << 4)  /**< \brief Pwm signal: PWML1 */
#define PIO_PA20B_PWML2      (1u << 20) /**< \brief Pwm signal: PWML2 */
#define PIO_PB18B_PWML2      (1u << 18) /**< \brief Pwm signal: PWML2 */
#define PIO_PC6B_PWML2       (1u << 6)  /**< \brief Pwm signal: PWML2 */
#define PIO_PE17A_PWML2      (1u << 17) /**< \brief Pwm signal: PWML2 */
#define PIO_PA0B_PWML3       (1u << 0)  /**< \brief Pwm signal: PWML3 */
#define PIO_PB19B_PWML3      (1u << 19) /**< \brief Pwm signal: PWML3 */
#define PIO_PC8B_PWML3       (1u << 8)  /**< \brief Pwm signal: PWML3 */
#define PIO_PB6B_PWML4       (1u << 6)  /**< \brief Pwm signal: PWML4 */
#define PIO_PC21B_PWML4      (1u << 21) /**< \brief Pwm signal: PWML4 */
#define PIO_PE19A_PWML4      (1u << 19) /**< \brief Pwm signal: PWML4 */
#define PIO_PB7B_PWML5       (1u << 7)  /**< \brief Pwm signal: PWML5 */
#define PIO_PC22B_PWML5      (1u << 22) /**< \brief Pwm signal: PWML5 */
#define PIO_PE21A_PWML5      (1u << 21) /**< \brief Pwm signal: PWML5 */
#define PIO_PB8B_PWML6       (1u << 8)  /**< \brief Pwm signal: PWML6 */
#define PIO_PC23B_PWML6      (1u << 23) /**< \brief Pwm signal: PWML6 */
#define PIO_PE23A_PWML6      (1u << 23) /**< \brief Pwm signal: PWML6 */
#define PIO_PB9B_PWML7       (1u << 9)  /**< \brief Pwm signal: PWML7 */
#define PIO_PC24B_PWML7      (1u << 24) /**< \brief Pwm signal: PWML7 */
#define PIO_PE25A_PWML7      (1u << 25) /**< \brief Pwm signal: PWML7 */
/* ========== Pio definition for SPI0 peripheral ========== */
#define PIO_PA25A_SPI0_MISO  (1u << 25) /**< \brief Spi0 signal: SPI0_MISO */
#define PIO_PA26A_SPI0_MOSI  (1u << 26) /**< \brief Spi0 signal: SPI0_MOSI */
#define PIO_PA28A_SPI0_NPCS0 (1u << 28) /**< \brief Spi0 signal: SPI0_NPCS0 */
#define PIO_PA29A_SPI0_NPCS1 (1u << 29) /**< \brief Spi0 signal: SPI0_NPCS1 */
#define PIO_PB20B_SPI0_NPCS1 (1u << 20) /**< \brief Spi0 signal: SPI0_NPCS1 */
#define PIO_PA30A_SPI0_NPCS2 (1u << 30) /**< \brief Spi0 signal: SPI0_NPCS2 */
#define PIO_PB21B_SPI0_NPCS2 (1u << 21) /**< \brief Spi0 signal: SPI0_NPCS2 */
#define PIO_PA31A_SPI0_NPCS3 (1u << 31) /**< \brief Spi0 signal: SPI0_NPCS3 */
#define PIO_PB23B_SPI0_NPCS3 (1u << 23) /**< \brief Spi0 signal: SPI0_NPCS3 */
#define PIO_PA27A_SPI0_SPCK  (1u << 27) /**< \brief Spi0 signal: SPI0_SPCK */
/* ========== Pio definition for SSC peripheral ========== */
#define PIO_PB18A_RD         (1u << 18) /**< \brief Ssc signal: RD */
#define PIO_PB17A_RF         (1u << 17) /**< \brief Ssc signal: RF */
#define PIO_PB19A_RK         (1u << 19) /**< \brief Ssc signal: RK */
#define PIO_PA16B_TD         (1u << 16) /**< \brief Ssc signal: TD */
#define PIO_PA15B_TF         (1u << 15) /**< \brief Ssc signal: TF */
#define PIO_PA14B_TK         (1u << 14) /**< \brief Ssc signal: TK */
/* ========== Pio definition for TC0 peripheral ========== */
#define PIO_PB26B_TCLK0      (1u << 26) /**< \brief Tc0 signal: TCLK0 */
#define PIO_PA4A_TCLK1       (1u << 4)  /**< \brief Tc0 signal: TCLK1 */
#define PIO_PA7A_TCLK2       (1u << 7)  /**< \brief Tc0 signal: TCLK2 */
#define PIO_PB25B_TIOA0      (1u << 25) /**< \brief Tc0 signal: TIOA0 */
#define PIO_PA2A_TIOA1       (1u << 2)  /**< \brief Tc0 signal: TIOA1 */
#define PIO_PA5A_TIOA2       (1u << 5)  /**< \brief Tc0 signal: TIOA2 */
#define PIO_PB27B_TIOB0      (1u << 27) /**< \brief Tc0 signal: TIOB0 */
#define PIO_PA3A_TIOB1       (1u << 3)  /**< \brief Tc0 signal: TIOB1 */
#define PIO_PA6A_TIOB2       (1u << 6)  /**< \brief Tc0 signal: TIOB2 */
/* ========== Pio definition for TC1 peripheral ========== */
#define PIO_PA22B_TCLK3      (1u << 22) /**< \brief Tc1 signal: TCLK3 */
#define PIO_PA23B_TCLK4      (1u << 23) /**< \brief Tc1 signal: TCLK4 */
#define PIO_PB16A_TCLK5      (1u << 16) /**< \brief Tc1 signal: TCLK5 */
#define PIO_PB0B_TIOA3       (1u << 0)  /**< \brief Tc1 signal: TIOA3 */
#define PIO_PE9A_TIOA3       (1u << 9)  /**< \brief Tc1 signal: TIOA3 */
#define PIO_PB2B_TIOA4       (1u << 2)  /**< \brief Tc1 signal: TIOA4 */
#define PIO_PE11A_TIOA4      (1u << 11) /**< \brief Tc1 signal: TIOA4 */
#define PIO_PB4B_TIOA5       (1u << 4)  /**< \brief Tc1 signal: TIOA5 */
#define PIO_PE13A_TIOA5      (1u << 13) /**< \brief Tc1 signal: TIOA5 */
#define PIO_PB1B_TIOB3       (1u << 1)  /**< \brief Tc1 signal: TIOB3 */
#define PIO_PE10A_TIOB3      (1u << 10) /**< \brief Tc1 signal: TIOB3 */
#define PIO_PB3B_TIOB4       (1u << 3)  /**< \brief Tc1 signal: TIOB4 */
#define PIO_PE12A_TIOB4      (1u << 12) /**< \brief Tc1 signal: TIOB4 */
#define PIO_PB5B_TIOB5       (1u << 5)  /**< \brief Tc1 signal: TIOB5 */
#define PIO_PE14A_TIOB5      (1u << 14) /**< \brief Tc1 signal: TIOB5 */
/* ========== Pio definition for TWI0 peripheral ========== */
#define PIO_PA18A_TWCK0      (1u << 18) /**< \brief Twi0 signal: TWCK0 */
#define PIO_PA17A_TWD0       (1u << 17) /**< \brief Twi0 signal: TWD0 */
/* ========== Pio definition for TWI1 peripheral ========== */
#define PIO_PB13A_TWCK1      (1u << 13) /**< \brief Twi1 signal: TWCK1 */
#define PIO_PB12A_TWD1       (1u << 12) /**< \brief Twi1 signal: TWD1 */
/* ========== Pio definition for UART peripheral ========== */
#define PIO_PA8A_URXD        (1u << 8)  /**< \brief Uart signal: URXD */
#define PIO_PA9A_UTXD        (1u << 9)  /**< \brief Uart signal: UTXD */
/* ========== Pio definition for UOTGHS peripheral ========== */
#define PIO_PB11A_UOTGID     (1u << 11) /**< \brief Uotghs signal: UOTGID */
#define PIO_PB10A_UOTGVBOF   (1u << 10) /**< \brief Uotghs signal: UOTGVBOF */
/* ========== Pio definition for USART0 peripheral ========== */
#define PIO_PB26A_CTS0       (1u << 26) /**< \brief Usart0 signal: CTS0 */
#define PIO_PB25A_RTS0       (1u << 25) /**< \brief Usart0 signal: RTS0 */
#define PIO_PA10A_RXD0       (1u << 10) /**< \brief Usart0 signal: RXD0 */
#define PIO_PA17B_SCK0       (1u << 17) /**< \brief Usart0 signal: SCK0 */
#define PIO_PA11A_TXD0       (1u << 11) /**< \brief Usart0 signal: TXD0 */
/* ========== Pio definition for USART1 peripheral ========== */
#define PIO_PA15A_CTS1       (1u << 15) /**< \brief Usart1 signal: CTS1 */
#define PIO_PA14A_RTS1       (1u << 14) /**< \brief Usart1 signal: RTS1 */
#define PIO_PA12A_RXD1       (1u << 12) /**< \brief Usart1 signal: RXD1 */
#define PIO_PA16A_SCK1       (1u << 16) /**< \brief Usart1 signal: SCK1 */
#define PIO_PA13A_TXD1       (1u << 13) /**< \brief Usart1 signal: TXD1 */
/* ========== Pio definition for USART2 peripheral ========== */
#define PIO_PB23A_CTS2       (1u << 23) /**< \brief Usart2 signal: CTS2 */
#define PIO_PB22A_RTS2       (1u << 22) /**< \brief Usart2 signal: RTS2 */
#define PIO_PB21A_RXD2       (1u << 21) /**< \brief Usart2 signal: RXD2 */
#define PIO_PB24A_SCK2       (1u << 24) /**< \brief Usart2 signal: SCK2 */
#define PIO_PB20A_TXD2       (1u << 20) /**< \brief Usart2 signal: TXD2 */
/* ========== Pio indexes ========== */
#define PIO_PA0_IDX          0
#define PIO_PA1_IDX          1
#define PIO_PA2_IDX          2
#define PIO_PA3_IDX          3
#define PIO_PA4_IDX          4
#define PIO_PA5_IDX          5
#define PIO_PA6_IDX          6
#define PIO_PA7_IDX          7
#define PIO_PA8_IDX          8
#define PIO_PA9_IDX          9
#define PIO_PA10_IDX         10
#define PIO_PA11_IDX         11
#define PIO_PA12_IDX         12
#define PIO_PA13_IDX         13
#define PIO_PA14_IDX         14
#define PIO_PA15_IDX         15
#define PIO_PA16_IDX         16
#define PIO_PA17_IDX         17
#define PIO_PA18_IDX         18
#define PIO_PA19_IDX         19
#define PIO_PA20_IDX         20
#define PIO_PA21_IDX         21
#define PIO_PA22_IDX         22
#define PIO_PA23_IDX         23
#define PIO_PA24_IDX         24
#define PIO_PA25_IDX         25
#define PIO_PA26_IDX         26
#define PIO_PA27_IDX         27
#define PIO_PA28_IDX         28
#define PIO_PA29_IDX         29
#define PIO_PB0_IDX          32
#define PIO_PB1_IDX          33
#define PIO_PB2_IDX          34
#define PIO_PB3_IDX          35
#define PIO_PB4_IDX          36
#define PIO_PB5_IDX          37
#define PIO_PB6_IDX          38
#define PIO_PB7_IDX          39
#define PIO_PB8_IDX          40
#define PIO_PB9_IDX          41
#define PIO_PB10_IDX         42
#define PIO_PB11_IDX         43
#define PIO_PB12_IDX         44
#define PIO_PB13_IDX         45
#define PIO_PB14_IDX         46
#define PIO_PB15_IDX         47
#define PIO_PB16_IDX         48
#define PIO_PB17_IDX         49
#define PIO_PB18_IDX         50
#define PIO_PB19_IDX         51
#define PIO_PB20_IDX         52
#define PIO_PB21_IDX         53
#define PIO_PB22_IDX         54
#define PIO_PB23_IDX         55
#define PIO_PB24_IDX         56
#define PIO_PB25_IDX         57
#define PIO_PB26_IDX         58
#define PIO_PB27_IDX         59
#define PIO_PB28_IDX         60
#define PIO_PB29_IDX         61
#define PIO_PB30_IDX         62
#define PIO_PB31_IDX         63

#endif /* _SAM3X4C_PIO_ */
