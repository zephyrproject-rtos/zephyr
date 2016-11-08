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

#ifndef _SAM3X8E_PIO_
#define _SAM3X8E_PIO_

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
#define PIO_PC0              (1u << 0)  /**< \brief Pin Controlled by PC0 */
#define PIO_PC1              (1u << 1)  /**< \brief Pin Controlled by PC1 */
#define PIO_PC2              (1u << 2)  /**< \brief Pin Controlled by PC2 */
#define PIO_PC3              (1u << 3)  /**< \brief Pin Controlled by PC3 */
#define PIO_PC4              (1u << 4)  /**< \brief Pin Controlled by PC4 */
#define PIO_PC5              (1u << 5)  /**< \brief Pin Controlled by PC5 */
#define PIO_PC6              (1u << 6)  /**< \brief Pin Controlled by PC6 */
#define PIO_PC7              (1u << 7)  /**< \brief Pin Controlled by PC7 */
#define PIO_PC8              (1u << 8)  /**< \brief Pin Controlled by PC8 */
#define PIO_PC9              (1u << 9)  /**< \brief Pin Controlled by PC9 */
#define PIO_PC10             (1u << 10) /**< \brief Pin Controlled by PC10 */
#define PIO_PC11             (1u << 11) /**< \brief Pin Controlled by PC11 */
#define PIO_PC12             (1u << 12) /**< \brief Pin Controlled by PC12 */
#define PIO_PC13             (1u << 13) /**< \brief Pin Controlled by PC13 */
#define PIO_PC14             (1u << 14) /**< \brief Pin Controlled by PC14 */
#define PIO_PC15             (1u << 15) /**< \brief Pin Controlled by PC15 */
#define PIO_PC16             (1u << 16) /**< \brief Pin Controlled by PC16 */
#define PIO_PC17             (1u << 17) /**< \brief Pin Controlled by PC17 */
#define PIO_PC18             (1u << 18) /**< \brief Pin Controlled by PC18 */
#define PIO_PC19             (1u << 19) /**< \brief Pin Controlled by PC19 */
#define PIO_PC20             (1u << 20) /**< \brief Pin Controlled by PC20 */
#define PIO_PC21             (1u << 21) /**< \brief Pin Controlled by PC21 */
#define PIO_PC22             (1u << 22) /**< \brief Pin Controlled by PC22 */
#define PIO_PC23             (1u << 23) /**< \brief Pin Controlled by PC23 */
#define PIO_PC24             (1u << 24) /**< \brief Pin Controlled by PC24 */
#define PIO_PC25             (1u << 25) /**< \brief Pin Controlled by PC25 */
#define PIO_PC26             (1u << 26) /**< \brief Pin Controlled by PC26 */
#define PIO_PC27             (1u << 27) /**< \brief Pin Controlled by PC27 */
#define PIO_PC28             (1u << 28) /**< \brief Pin Controlled by PC28 */
#define PIO_PC29             (1u << 29) /**< \brief Pin Controlled by PC29 */
#define PIO_PC30             (1u << 30) /**< \brief Pin Controlled by PC30 */
#define PIO_PD0              (1u << 0)  /**< \brief Pin Controlled by PD0 */
#define PIO_PD1              (1u << 1)  /**< \brief Pin Controlled by PD1 */
#define PIO_PD2              (1u << 2)  /**< \brief Pin Controlled by PD2 */
#define PIO_PD3              (1u << 3)  /**< \brief Pin Controlled by PD3 */
#define PIO_PD4              (1u << 4)  /**< \brief Pin Controlled by PD4 */
#define PIO_PD5              (1u << 5)  /**< \brief Pin Controlled by PD5 */
#define PIO_PD6              (1u << 6)  /**< \brief Pin Controlled by PD6 */
#define PIO_PD7              (1u << 7)  /**< \brief Pin Controlled by PD7 */
#define PIO_PD8              (1u << 8)  /**< \brief Pin Controlled by PD8 */
#define PIO_PD9              (1u << 9)  /**< \brief Pin Controlled by PD9 */
#define PIO_PD10             (1u << 10) /**< \brief Pin Controlled by PD10 */
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
/* ========== Pio definition for EBI peripheral ========== */
#define PIO_PC21A_A0         (1u << 21) /**< \brief Ebi signal: A0/NBS0 */
#define PIO_PC21A_NBS0       (1u << 21) /**< \brief Ebi signal: A0/NBS0 */
#define PIO_PC22A_A1         (1u << 22) /**< \brief Ebi signal: A1 */
#define PIO_PD0A_A10         (1u << 0)  /**< \brief Ebi signal: A10 */
#define PIO_PD22A_A10        (1u << 22) /**< \brief Ebi signal: A10 */
#define PIO_PD1A_A11         (1u << 1)  /**< \brief Ebi signal: A11 */
#define PIO_PD23A_A11        (1u << 23) /**< \brief Ebi signal: A11 */
#define PIO_PD2A_A12         (1u << 2)  /**< \brief Ebi signal: A12 */
#define PIO_PD24A_A12        (1u << 24) /**< \brief Ebi signal: A12 */
#define PIO_PD3A_A13         (1u << 3)  /**< \brief Ebi signal: A13 */
#define PIO_PD25A_A13        (1u << 25) /**< \brief Ebi signal: A13 */
#define PIO_PD4A_A14         (1u << 4)  /**< \brief Ebi signal: A14 */
#define PIO_PD26A_A14        (1u << 26) /**< \brief Ebi signal: A14 */
#define PIO_PD5A_A15         (1u << 5)  /**< \brief Ebi signal: A15 */
#define PIO_PD27A_A15        (1u << 27) /**< \brief Ebi signal: A15 */
#define PIO_PD6A_A16         (1u << 6)  /**< \brief Ebi signal: A16/BA0 */
#define PIO_PD6A_BA0         (1u << 6)  /**< \brief Ebi signal: A16/BA0 */
#define PIO_PD28A_A16        (1u << 28) /**< \brief Ebi signal: A16/BA0 */
#define PIO_PD28A_BA0        (1u << 28) /**< \brief Ebi signal: A16/BA0 */
#define PIO_PD7A_A17         (1u << 7)  /**< \brief Ebi signal: A17/BA1 */
#define PIO_PD7A_BA1         (1u << 7)  /**< \brief Ebi signal: A17/BA1 */
#define PIO_PD29A_A17        (1u << 29) /**< \brief Ebi signal: A17/BA1 */
#define PIO_PD29A_BA1        (1u << 29) /**< \brief Ebi signal: A17/BA1 */
#define PIO_PA25B_A18        (1u << 25) /**< \brief Ebi signal: A18 */
#define PIO_PB10B_A18        (1u << 10) /**< \brief Ebi signal: A18 */
#define PIO_PD30A_A18        (1u << 30) /**< \brief Ebi signal: A18 */
#define PIO_PA26B_A19        (1u << 26) /**< \brief Ebi signal: A19 */
#define PIO_PB11B_A19        (1u << 11) /**< \brief Ebi signal: A19 */
#define PIO_PE0A_A19         (1u << 0)  /**< \brief Ebi signal: A19 */
#define PIO_PC23A_A2         (1u << 23) /**< \brief Ebi signal: A2 */
#define PIO_PA18B_A20        (1u << 18) /**< \brief Ebi signal: A20 */
#define PIO_PA27B_A20        (1u << 27) /**< \brief Ebi signal: A20 */
#define PIO_PE1A_A20         (1u << 1)  /**< \brief Ebi signal: A20 */
#define PIO_PD8A_A21         (1u << 8)  /**< \brief Ebi signal: A21/NANDALE */
#define PIO_PD8A_NANDALE     (1u << 8)  /**< \brief Ebi signal: A21/NANDALE */
#define PIO_PE2A_A21         (1u << 2)  /**< \brief Ebi signal: A21/NANDALE */
#define PIO_PE2A_NANDALE     (1u << 2)  /**< \brief Ebi signal: A21/NANDALE */
#define PIO_PD9A_A22         (1u << 9)  /**< \brief Ebi signal: A22/NANDCLE */
#define PIO_PD9A_NANDCLE     (1u << 9)  /**< \brief Ebi signal: A22/NANDCLE */
#define PIO_PE3A_A22         (1u << 3)  /**< \brief Ebi signal: A22/NANDCLE */
#define PIO_PE3A_NANDCLE     (1u << 3)  /**< \brief Ebi signal: A22/NANDCLE */
#define PIO_PE4A_A23         (1u << 4)  /**< \brief Ebi signal: A23 */
#define PIO_PC24A_A3         (1u << 24) /**< \brief Ebi signal: A3 */
#define PIO_PC25A_A4         (1u << 25) /**< \brief Ebi signal: A4 */
#define PIO_PC26A_A5         (1u << 26) /**< \brief Ebi signal: A5 */
#define PIO_PD17A_A5         (1u << 17) /**< \brief Ebi signal: A5 */
#define PIO_PC27A_A6         (1u << 27) /**< \brief Ebi signal: A6 */
#define PIO_PD18A_A6         (1u << 18) /**< \brief Ebi signal: A6 */
#define PIO_PC28A_A7         (1u << 28) /**< \brief Ebi signal: A7 */
#define PIO_PD19A_A7         (1u << 19) /**< \brief Ebi signal: A7 */
#define PIO_PC29A_A8         (1u << 29) /**< \brief Ebi signal: A8 */
#define PIO_PD20A_A8         (1u << 20) /**< \brief Ebi signal: A8 */
#define PIO_PC30A_A9         (1u << 30) /**< \brief Ebi signal: A9 */
#define PIO_PD21A_A9         (1u << 21) /**< \brief Ebi signal: A9 */
#define PIO_PD16A_CAS        (1u << 16) /**< \brief Ebi signal: CAS */
#define PIO_PC2A_D0          (1u << 2)  /**< \brief Ebi signal: D0 */
#define PIO_PC3A_D1          (1u << 3)  /**< \brief Ebi signal: D1 */
#define PIO_PC12A_D10        (1u << 12) /**< \brief Ebi signal: D10 */
#define PIO_PC13A_D11        (1u << 13) /**< \brief Ebi signal: D11 */
#define PIO_PC14A_D12        (1u << 14) /**< \brief Ebi signal: D12 */
#define PIO_PC15A_D13        (1u << 15) /**< \brief Ebi signal: D13 */
#define PIO_PC16A_D14        (1u << 16) /**< \brief Ebi signal: D14 */
#define PIO_PC17A_D15        (1u << 17) /**< \brief Ebi signal: D15 */
#define PIO_PC4A_D2          (1u << 4)  /**< \brief Ebi signal: D2 */
#define PIO_PC5A_D3          (1u << 5)  /**< \brief Ebi signal: D3 */
#define PIO_PC6A_D4          (1u << 6)  /**< \brief Ebi signal: D4 */
#define PIO_PC7A_D5          (1u << 7)  /**< \brief Ebi signal: D5 */
#define PIO_PC8A_D6          (1u << 8)  /**< \brief Ebi signal: D6 */
#define PIO_PC9A_D7          (1u << 9)  /**< \brief Ebi signal: D7 */
#define PIO_PC10A_D8         (1u << 10) /**< \brief Ebi signal: D8 */
#define PIO_PC11A_D9         (1u << 11) /**< \brief Ebi signal: D9 */
#define PIO_PC19A_NANDOE     (1u << 19) /**< \brief Ebi signal: NANDOE */
#define PIO_PA2B_NANDRDY     (1u << 2)  /**< \brief Ebi signal: NANDRDY */
#define PIO_PC20A_NANDWE     (1u << 20) /**< \brief Ebi signal: NANDWE */
#define PIO_PA6B_NCS0        (1u << 6)  /**< \brief Ebi signal: NCS0 */
#define PIO_PA7B_NCS1        (1u << 7)  /**< \brief Ebi signal: NCS1 */
#define PIO_PB24B_NCS2       (1u << 24) /**< \brief Ebi signal: NCS2 */
#define PIO_PB27A_NCS3       (1u << 27) /**< \brief Ebi signal: NCS3 */
#define PIO_PE5A_NCS4        (1u << 5)  /**< \brief Ebi signal: NCS4 */
#define PIO_PE6A_NCS5        (1u << 6)  /**< \brief Ebi signal: NCS5 */
#define PIO_PE18B_NCS6       (1u << 18) /**< \brief Ebi signal: NCS6 */
#define PIO_PE27A_NCS7       (1u << 27) /**< \brief Ebi signal: NCS7 */
#define PIO_PA29B_NRD        (1u << 29) /**< \brief Ebi signal: NRD */
#define PIO_PA4B_NWAIT       (1u << 4)  /**< \brief Ebi signal: NWAIT */
#define PIO_PC18A_NWR0       (1u << 18) /**< \brief Ebi signal: NWR0/NWE */
#define PIO_PC18A_NWE        (1u << 18) /**< \brief Ebi signal: NWR0/NWE */
#define PIO_PD10A_NWR1       (1u << 10) /**< \brief Ebi signal: NWR1/NBS1 */
#define PIO_PD10A_NBS1       (1u << 10) /**< \brief Ebi signal: NWR1/NBS1 */
#define PIO_PD15A_RAS        (1u << 15) /**< \brief Ebi signal: RAS */
#define PIO_PD11A_SDA10      (1u << 11) /**< \brief Ebi signal: SDA10 */
#define PIO_PD13A_SDCKE      (1u << 13) /**< \brief Ebi signal: SDCKE */
#define PIO_PD12A_SDCS       (1u << 12) /**< \brief Ebi signal: SDCS */
#define PIO_PD14A_SDWE       (1u << 14) /**< \brief Ebi signal: SDWE */
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
/* ========== Pio definition for TC2 peripheral ========== */
#define PIO_PC27B_TCLK6      (1u << 27) /**< \brief Tc2 signal: TCLK6 */
#define PIO_PC30B_TCLK7      (1u << 30) /**< \brief Tc2 signal: TCLK7 */
#define PIO_PD9B_TCLK8       (1u << 9)  /**< \brief Tc2 signal: TCLK8 */
#define PIO_PC25B_TIOA6      (1u << 25) /**< \brief Tc2 signal: TIOA6 */
#define PIO_PC28B_TIOA7      (1u << 28) /**< \brief Tc2 signal: TIOA7 */
#define PIO_PD7B_TIOA8       (1u << 7)  /**< \brief Tc2 signal: TIOA8 */
#define PIO_PC26B_TIOB6      (1u << 26) /**< \brief Tc2 signal: TIOB6 */
#define PIO_PC29B_TIOB7      (1u << 29) /**< \brief Tc2 signal: TIOB7 */
#define PIO_PD8B_TIOB8       (1u << 8)  /**< \brief Tc2 signal: TIOB8 */
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
/* ========== Pio definition for USART3 peripheral ========== */
#define PIO_PF4A_CTS3        (1u << 4)  /**< \brief Usart3 signal: CTS3 */
#define PIO_PF5A_RTS3        (1u << 5)  /**< \brief Usart3 signal: RTS3 */
#define PIO_PD5B_RXD3        (1u << 5)  /**< \brief Usart3 signal: RXD3 */
#define PIO_PE16B_SCK3       (1u << 16) /**< \brief Usart3 signal: SCK3 */
#define PIO_PD4B_TXD3        (1u << 4)  /**< \brief Usart3 signal: TXD3 */
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
#define PIO_PC0_IDX          64
#define PIO_PC1_IDX          65
#define PIO_PC2_IDX          66
#define PIO_PC3_IDX          67
#define PIO_PC4_IDX          68
#define PIO_PC5_IDX          69
#define PIO_PC6_IDX          70
#define PIO_PC7_IDX          71
#define PIO_PC8_IDX          72
#define PIO_PC9_IDX          73
#define PIO_PC10_IDX         74
#define PIO_PC11_IDX         75
#define PIO_PC12_IDX         76
#define PIO_PC13_IDX         77
#define PIO_PC14_IDX         78
#define PIO_PC15_IDX         79
#define PIO_PC16_IDX         80
#define PIO_PC17_IDX         81
#define PIO_PC18_IDX         82
#define PIO_PC19_IDX         83
#define PIO_PC20_IDX         84
#define PIO_PC21_IDX         85
#define PIO_PC22_IDX         86
#define PIO_PC23_IDX         87
#define PIO_PC24_IDX         88
#define PIO_PC25_IDX         89
#define PIO_PC26_IDX         90
#define PIO_PC27_IDX         91
#define PIO_PC28_IDX         92
#define PIO_PC29_IDX         93
#define PIO_PC30_IDX         94
#define PIO_PD0_IDX          96
#define PIO_PD1_IDX          97
#define PIO_PD2_IDX          98
#define PIO_PD3_IDX          99
#define PIO_PD4_IDX          100
#define PIO_PD5_IDX          101
#define PIO_PD6_IDX          102
#define PIO_PD7_IDX          103
#define PIO_PD8_IDX          104
#define PIO_PD9_IDX          105
#define PIO_PD10_IDX         106

#endif /* _SAM3X8E_PIO_ */
