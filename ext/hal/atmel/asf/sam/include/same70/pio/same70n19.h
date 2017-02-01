/**
 * \file
 *
 * \brief Peripheral I/O description for SAME70N19
 *
 * Copyright (c) 2016 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \license_stop
 *
 */

#ifndef _SAME70N19_PIO_H_
#define _SAME70N19_PIO_H_

/* ========== Peripheral I/O pin numbers ========== */
#define PIN_PA0                     (  0)  /**< Pin Number for PA0 */
#define PIN_PA1                     (  1)  /**< Pin Number for PA1 */
#define PIN_PA2                     (  2)  /**< Pin Number for PA2 */
#define PIN_PA3                     (  3)  /**< Pin Number for PA3 */
#define PIN_PA4                     (  4)  /**< Pin Number for PA4 */
#define PIN_PA5                     (  5)  /**< Pin Number for PA5 */
#define PIN_PA6                     (  6)  /**< Pin Number for PA6 */
#define PIN_PA7                     (  7)  /**< Pin Number for PA7 */
#define PIN_PA8                     (  8)  /**< Pin Number for PA8 */
#define PIN_PA9                     (  9)  /**< Pin Number for PA9 */
#define PIN_PA10                    ( 10)  /**< Pin Number for PA10 */
#define PIN_PA11                    ( 11)  /**< Pin Number for PA11 */
#define PIN_PA12                    ( 12)  /**< Pin Number for PA12 */
#define PIN_PA13                    ( 13)  /**< Pin Number for PA13 */
#define PIN_PA14                    ( 14)  /**< Pin Number for PA14 */
#define PIN_PA15                    ( 15)  /**< Pin Number for PA15 */
#define PIN_PA16                    ( 16)  /**< Pin Number for PA16 */
#define PIN_PA17                    ( 17)  /**< Pin Number for PA17 */
#define PIN_PA18                    ( 18)  /**< Pin Number for PA18 */
#define PIN_PA19                    ( 19)  /**< Pin Number for PA19 */
#define PIN_PA20                    ( 20)  /**< Pin Number for PA20 */
#define PIN_PA21                    ( 21)  /**< Pin Number for PA21 */
#define PIN_PA22                    ( 22)  /**< Pin Number for PA22 */
#define PIN_PA23                    ( 23)  /**< Pin Number for PA23 */
#define PIN_PA24                    ( 24)  /**< Pin Number for PA24 */
#define PIN_PA25                    ( 25)  /**< Pin Number for PA25 */
#define PIN_PA26                    ( 26)  /**< Pin Number for PA26 */
#define PIN_PA27                    ( 27)  /**< Pin Number for PA27 */
#define PIN_PA28                    ( 28)  /**< Pin Number for PA28 */
#define PIN_PA29                    ( 29)  /**< Pin Number for PA29 */
#define PIN_PA30                    ( 30)  /**< Pin Number for PA30 */
#define PIN_PA31                    ( 31)  /**< Pin Number for PA31 */
#define PIN_PB0                     ( 32)  /**< Pin Number for PB0 */
#define PIN_PB1                     ( 33)  /**< Pin Number for PB1 */
#define PIN_PB2                     ( 34)  /**< Pin Number for PB2 */
#define PIN_PB3                     ( 35)  /**< Pin Number for PB3 */
#define PIN_PB4                     ( 36)  /**< Pin Number for PB4 */
#define PIN_PB5                     ( 37)  /**< Pin Number for PB5 */
#define PIN_PB6                     ( 38)  /**< Pin Number for PB6 */
#define PIN_PB7                     ( 39)  /**< Pin Number for PB7 */
#define PIN_PB8                     ( 40)  /**< Pin Number for PB8 */
#define PIN_PB9                     ( 41)  /**< Pin Number for PB9 */
#define PIN_PB12                    ( 44)  /**< Pin Number for PB12 */
#define PIN_PB13                    ( 45)  /**< Pin Number for PB13 */
#define PIN_PD0                     ( 96)  /**< Pin Number for PD0 */
#define PIN_PD1                     ( 97)  /**< Pin Number for PD1 */
#define PIN_PD2                     ( 98)  /**< Pin Number for PD2 */
#define PIN_PD3                     ( 99)  /**< Pin Number for PD3 */
#define PIN_PD4                     (100)  /**< Pin Number for PD4 */
#define PIN_PD5                     (101)  /**< Pin Number for PD5 */
#define PIN_PD6                     (102)  /**< Pin Number for PD6 */
#define PIN_PD7                     (103)  /**< Pin Number for PD7 */
#define PIN_PD8                     (104)  /**< Pin Number for PD8 */
#define PIN_PD9                     (105)  /**< Pin Number for PD9 */
#define PIN_PD10                    (106)  /**< Pin Number for PD10 */
#define PIN_PD11                    (107)  /**< Pin Number for PD11 */
#define PIN_PD12                    (108)  /**< Pin Number for PD12 */
#define PIN_PD13                    (109)  /**< Pin Number for PD13 */
#define PIN_PD14                    (110)  /**< Pin Number for PD14 */
#define PIN_PD15                    (111)  /**< Pin Number for PD15 */
#define PIN_PD16                    (112)  /**< Pin Number for PD16 */
#define PIN_PD17                    (113)  /**< Pin Number for PD17 */
#define PIN_PD18                    (114)  /**< Pin Number for PD18 */
#define PIN_PD19                    (115)  /**< Pin Number for PD19 */
#define PIN_PD20                    (116)  /**< Pin Number for PD20 */
#define PIN_PD21                    (117)  /**< Pin Number for PD21 */
#define PIN_PD22                    (118)  /**< Pin Number for PD22 */
#define PIN_PD23                    (119)  /**< Pin Number for PD23 */
#define PIN_PD24                    (120)  /**< Pin Number for PD24 */
#define PIN_PD25                    (121)  /**< Pin Number for PD25 */
#define PIN_PD26                    (122)  /**< Pin Number for PD26 */
#define PIN_PD27                    (123)  /**< Pin Number for PD27 */
#define PIN_PD28                    (124)  /**< Pin Number for PD28 */
#define PIN_PD29                    (125)  /**< Pin Number for PD29 */
#define PIN_PD30                    (126)  /**< Pin Number for PD30 */
#define PIN_PD31                    (127)  /**< Pin Number for PD31 */


#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/* ========== Peripheral I/O masks ========== */
#define PIO_PA0                     (1U << 0)  /**< PIO Mask for PA0 */
#define PIO_PA1                     (1U << 1)  /**< PIO Mask for PA1 */
#define PIO_PA2                     (1U << 2)  /**< PIO Mask for PA2 */
#define PIO_PA3                     (1U << 3)  /**< PIO Mask for PA3 */
#define PIO_PA4                     (1U << 4)  /**< PIO Mask for PA4 */
#define PIO_PA5                     (1U << 5)  /**< PIO Mask for PA5 */
#define PIO_PA6                     (1U << 6)  /**< PIO Mask for PA6 */
#define PIO_PA7                     (1U << 7)  /**< PIO Mask for PA7 */
#define PIO_PA8                     (1U << 8)  /**< PIO Mask for PA8 */
#define PIO_PA9                     (1U << 9)  /**< PIO Mask for PA9 */
#define PIO_PA10                    (1U << 10) /**< PIO Mask for PA10 */
#define PIO_PA11                    (1U << 11) /**< PIO Mask for PA11 */
#define PIO_PA12                    (1U << 12) /**< PIO Mask for PA12 */
#define PIO_PA13                    (1U << 13) /**< PIO Mask for PA13 */
#define PIO_PA14                    (1U << 14) /**< PIO Mask for PA14 */
#define PIO_PA15                    (1U << 15) /**< PIO Mask for PA15 */
#define PIO_PA16                    (1U << 16) /**< PIO Mask for PA16 */
#define PIO_PA17                    (1U << 17) /**< PIO Mask for PA17 */
#define PIO_PA18                    (1U << 18) /**< PIO Mask for PA18 */
#define PIO_PA19                    (1U << 19) /**< PIO Mask for PA19 */
#define PIO_PA20                    (1U << 20) /**< PIO Mask for PA20 */
#define PIO_PA21                    (1U << 21) /**< PIO Mask for PA21 */
#define PIO_PA22                    (1U << 22) /**< PIO Mask for PA22 */
#define PIO_PA23                    (1U << 23) /**< PIO Mask for PA23 */
#define PIO_PA24                    (1U << 24) /**< PIO Mask for PA24 */
#define PIO_PA25                    (1U << 25) /**< PIO Mask for PA25 */
#define PIO_PA26                    (1U << 26) /**< PIO Mask for PA26 */
#define PIO_PA27                    (1U << 27) /**< PIO Mask for PA27 */
#define PIO_PA28                    (1U << 28) /**< PIO Mask for PA28 */
#define PIO_PA29                    (1U << 29) /**< PIO Mask for PA29 */
#define PIO_PA30                    (1U << 30) /**< PIO Mask for PA30 */
#define PIO_PA31                    (1U << 31) /**< PIO Mask for PA31 */
#define PIO_PB0                     (1U << 0)  /**< PIO Mask for PB0 */
#define PIO_PB1                     (1U << 1)  /**< PIO Mask for PB1 */
#define PIO_PB2                     (1U << 2)  /**< PIO Mask for PB2 */
#define PIO_PB3                     (1U << 3)  /**< PIO Mask for PB3 */
#define PIO_PB4                     (1U << 4)  /**< PIO Mask for PB4 */
#define PIO_PB5                     (1U << 5)  /**< PIO Mask for PB5 */
#define PIO_PB6                     (1U << 6)  /**< PIO Mask for PB6 */
#define PIO_PB7                     (1U << 7)  /**< PIO Mask for PB7 */
#define PIO_PB8                     (1U << 8)  /**< PIO Mask for PB8 */
#define PIO_PB9                     (1U << 9)  /**< PIO Mask for PB9 */
#define PIO_PB12                    (1U << 12) /**< PIO Mask for PB12 */
#define PIO_PB13                    (1U << 13) /**< PIO Mask for PB13 */
#define PIO_PD0                     (1U << 0)  /**< PIO Mask for PD0 */
#define PIO_PD1                     (1U << 1)  /**< PIO Mask for PD1 */
#define PIO_PD2                     (1U << 2)  /**< PIO Mask for PD2 */
#define PIO_PD3                     (1U << 3)  /**< PIO Mask for PD3 */
#define PIO_PD4                     (1U << 4)  /**< PIO Mask for PD4 */
#define PIO_PD5                     (1U << 5)  /**< PIO Mask for PD5 */
#define PIO_PD6                     (1U << 6)  /**< PIO Mask for PD6 */
#define PIO_PD7                     (1U << 7)  /**< PIO Mask for PD7 */
#define PIO_PD8                     (1U << 8)  /**< PIO Mask for PD8 */
#define PIO_PD9                     (1U << 9)  /**< PIO Mask for PD9 */
#define PIO_PD10                    (1U << 10) /**< PIO Mask for PD10 */
#define PIO_PD11                    (1U << 11) /**< PIO Mask for PD11 */
#define PIO_PD12                    (1U << 12) /**< PIO Mask for PD12 */
#define PIO_PD13                    (1U << 13) /**< PIO Mask for PD13 */
#define PIO_PD14                    (1U << 14) /**< PIO Mask for PD14 */
#define PIO_PD15                    (1U << 15) /**< PIO Mask for PD15 */
#define PIO_PD16                    (1U << 16) /**< PIO Mask for PD16 */
#define PIO_PD17                    (1U << 17) /**< PIO Mask for PD17 */
#define PIO_PD18                    (1U << 18) /**< PIO Mask for PD18 */
#define PIO_PD19                    (1U << 19) /**< PIO Mask for PD19 */
#define PIO_PD20                    (1U << 20) /**< PIO Mask for PD20 */
#define PIO_PD21                    (1U << 21) /**< PIO Mask for PD21 */
#define PIO_PD22                    (1U << 22) /**< PIO Mask for PD22 */
#define PIO_PD23                    (1U << 23) /**< PIO Mask for PD23 */
#define PIO_PD24                    (1U << 24) /**< PIO Mask for PD24 */
#define PIO_PD25                    (1U << 25) /**< PIO Mask for PD25 */
#define PIO_PD26                    (1U << 26) /**< PIO Mask for PD26 */
#define PIO_PD27                    (1U << 27) /**< PIO Mask for PD27 */
#define PIO_PD28                    (1U << 28) /**< PIO Mask for PD28 */
#define PIO_PD29                    (1U << 29) /**< PIO Mask for PD29 */
#define PIO_PD30                    (1U << 30) /**< PIO Mask for PD30 */
#define PIO_PD31                    (1U << 31) /**< PIO Mask for PD31 */
#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */



/* ========== Peripheral I/O indexes ========== */
#define PIO_PA0_IDX                 (  0)  /**< PIO Index Number for PA0 */
#define PIO_PA1_IDX                 (  1)  /**< PIO Index Number for PA1 */
#define PIO_PA2_IDX                 (  2)  /**< PIO Index Number for PA2 */
#define PIO_PA3_IDX                 (  3)  /**< PIO Index Number for PA3 */
#define PIO_PA4_IDX                 (  4)  /**< PIO Index Number for PA4 */
#define PIO_PA5_IDX                 (  5)  /**< PIO Index Number for PA5 */
#define PIO_PA6_IDX                 (  6)  /**< PIO Index Number for PA6 */
#define PIO_PA7_IDX                 (  7)  /**< PIO Index Number for PA7 */
#define PIO_PA8_IDX                 (  8)  /**< PIO Index Number for PA8 */
#define PIO_PA9_IDX                 (  9)  /**< PIO Index Number for PA9 */
#define PIO_PA10_IDX                ( 10)  /**< PIO Index Number for PA10 */
#define PIO_PA11_IDX                ( 11)  /**< PIO Index Number for PA11 */
#define PIO_PA12_IDX                ( 12)  /**< PIO Index Number for PA12 */
#define PIO_PA13_IDX                ( 13)  /**< PIO Index Number for PA13 */
#define PIO_PA14_IDX                ( 14)  /**< PIO Index Number for PA14 */
#define PIO_PA15_IDX                ( 15)  /**< PIO Index Number for PA15 */
#define PIO_PA16_IDX                ( 16)  /**< PIO Index Number for PA16 */
#define PIO_PA17_IDX                ( 17)  /**< PIO Index Number for PA17 */
#define PIO_PA18_IDX                ( 18)  /**< PIO Index Number for PA18 */
#define PIO_PA19_IDX                ( 19)  /**< PIO Index Number for PA19 */
#define PIO_PA20_IDX                ( 20)  /**< PIO Index Number for PA20 */
#define PIO_PA21_IDX                ( 21)  /**< PIO Index Number for PA21 */
#define PIO_PA22_IDX                ( 22)  /**< PIO Index Number for PA22 */
#define PIO_PA23_IDX                ( 23)  /**< PIO Index Number for PA23 */
#define PIO_PA24_IDX                ( 24)  /**< PIO Index Number for PA24 */
#define PIO_PA25_IDX                ( 25)  /**< PIO Index Number for PA25 */
#define PIO_PA26_IDX                ( 26)  /**< PIO Index Number for PA26 */
#define PIO_PA27_IDX                ( 27)  /**< PIO Index Number for PA27 */
#define PIO_PA28_IDX                ( 28)  /**< PIO Index Number for PA28 */
#define PIO_PA29_IDX                ( 29)  /**< PIO Index Number for PA29 */
#define PIO_PA30_IDX                ( 30)  /**< PIO Index Number for PA30 */
#define PIO_PA31_IDX                ( 31)  /**< PIO Index Number for PA31 */
#define PIO_PB0_IDX                 ( 32)  /**< PIO Index Number for PB0 */
#define PIO_PB1_IDX                 ( 33)  /**< PIO Index Number for PB1 */
#define PIO_PB2_IDX                 ( 34)  /**< PIO Index Number for PB2 */
#define PIO_PB3_IDX                 ( 35)  /**< PIO Index Number for PB3 */
#define PIO_PB4_IDX                 ( 36)  /**< PIO Index Number for PB4 */
#define PIO_PB5_IDX                 ( 37)  /**< PIO Index Number for PB5 */
#define PIO_PB6_IDX                 ( 38)  /**< PIO Index Number for PB6 */
#define PIO_PB7_IDX                 ( 39)  /**< PIO Index Number for PB7 */
#define PIO_PB8_IDX                 ( 40)  /**< PIO Index Number for PB8 */
#define PIO_PB9_IDX                 ( 41)  /**< PIO Index Number for PB9 */
#define PIO_PB12_IDX                ( 44)  /**< PIO Index Number for PB12 */
#define PIO_PB13_IDX                ( 45)  /**< PIO Index Number for PB13 */
#define PIO_PD0_IDX                 ( 96)  /**< PIO Index Number for PD0 */
#define PIO_PD1_IDX                 ( 97)  /**< PIO Index Number for PD1 */
#define PIO_PD2_IDX                 ( 98)  /**< PIO Index Number for PD2 */
#define PIO_PD3_IDX                 ( 99)  /**< PIO Index Number for PD3 */
#define PIO_PD4_IDX                 (100)  /**< PIO Index Number for PD4 */
#define PIO_PD5_IDX                 (101)  /**< PIO Index Number for PD5 */
#define PIO_PD6_IDX                 (102)  /**< PIO Index Number for PD6 */
#define PIO_PD7_IDX                 (103)  /**< PIO Index Number for PD7 */
#define PIO_PD8_IDX                 (104)  /**< PIO Index Number for PD8 */
#define PIO_PD9_IDX                 (105)  /**< PIO Index Number for PD9 */
#define PIO_PD10_IDX                (106)  /**< PIO Index Number for PD10 */
#define PIO_PD11_IDX                (107)  /**< PIO Index Number for PD11 */
#define PIO_PD12_IDX                (108)  /**< PIO Index Number for PD12 */
#define PIO_PD13_IDX                (109)  /**< PIO Index Number for PD13 */
#define PIO_PD14_IDX                (110)  /**< PIO Index Number for PD14 */
#define PIO_PD15_IDX                (111)  /**< PIO Index Number for PD15 */
#define PIO_PD16_IDX                (112)  /**< PIO Index Number for PD16 */
#define PIO_PD17_IDX                (113)  /**< PIO Index Number for PD17 */
#define PIO_PD18_IDX                (114)  /**< PIO Index Number for PD18 */
#define PIO_PD19_IDX                (115)  /**< PIO Index Number for PD19 */
#define PIO_PD20_IDX                (116)  /**< PIO Index Number for PD20 */
#define PIO_PD21_IDX                (117)  /**< PIO Index Number for PD21 */
#define PIO_PD22_IDX                (118)  /**< PIO Index Number for PD22 */
#define PIO_PD23_IDX                (119)  /**< PIO Index Number for PD23 */
#define PIO_PD24_IDX                (120)  /**< PIO Index Number for PD24 */
#define PIO_PD25_IDX                (121)  /**< PIO Index Number for PD25 */
#define PIO_PD26_IDX                (122)  /**< PIO Index Number for PD26 */
#define PIO_PD27_IDX                (123)  /**< PIO Index Number for PD27 */
#define PIO_PD28_IDX                (124)  /**< PIO Index Number for PD28 */
#define PIO_PD29_IDX                (125)  /**< PIO Index Number for PD29 */
#define PIO_PD30_IDX                (126)  /**< PIO Index Number for PD30 */
#define PIO_PD31_IDX                (127)  /**< PIO Index Number for PD31 */

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/* ========== PIO definition for AFEC0 peripheral ========== */
#define PIN_PA8B_AFEC0_ADTRG                       8L           /**< AFEC0 signal: ADTRG on PA8 mux B*/
#define MUX_PA8B_AFEC0_ADTRG                       1L          
#define PINMUX_PA8B_AFEC0_ADTRG                    ((PIN_PA8B_AFEC0_ADTRG << 16) | MUX_PA8B_AFEC0_ADTRG)
#define PIO_PA8B_AFEC0_ADTRG                       (1UL << 8)  

/* ========== PIO definition for AFEC1 peripheral ========== */
#define PIN_PD9C_AFEC1_ADTRG                       105L         /**< AFEC1 signal: ADTRG on PD9 mux C*/
#define MUX_PD9C_AFEC1_ADTRG                       2L          
#define PINMUX_PD9C_AFEC1_ADTRG                    ((PIN_PD9C_AFEC1_ADTRG << 16) | MUX_PD9C_AFEC1_ADTRG)
#define PIO_PD9C_AFEC1_ADTRG                       (1UL << 9)  

/* ========== PIO definition for DACC peripheral ========== */
#define PIN_PA2C_DACC_DATRG                        2L           /**< DACC signal: DATRG on PA2 mux C*/
#define MUX_PA2C_DACC_DATRG                        2L          
#define PINMUX_PA2C_DACC_DATRG                     ((PIN_PA2C_DACC_DATRG << 16) | MUX_PA2C_DACC_DATRG)
#define PIO_PA2C_DACC_DATRG                        (1UL << 2)  

/* ========== PIO definition for GMAC peripheral ========== */
#define PIN_PD13A_GMAC_GCOL                        109L         /**< GMAC signal: GCOL on PD13 mux A*/
#define MUX_PD13A_GMAC_GCOL                        0L          
#define PINMUX_PD13A_GMAC_GCOL                     ((PIN_PD13A_GMAC_GCOL << 16) | MUX_PD13A_GMAC_GCOL)
#define PIO_PD13A_GMAC_GCOL                        (1UL << 13) 

#define PIN_PD10A_GMAC_GCRS                        106L         /**< GMAC signal: GCRS on PD10 mux A*/
#define MUX_PD10A_GMAC_GCRS                        0L          
#define PINMUX_PD10A_GMAC_GCRS                     ((PIN_PD10A_GMAC_GCRS << 16) | MUX_PD10A_GMAC_GCRS)
#define PIO_PD10A_GMAC_GCRS                        (1UL << 10) 

#define PIN_PD8A_GMAC_GMDC                         104L         /**< GMAC signal: GMDC on PD8 mux A*/
#define MUX_PD8A_GMAC_GMDC                         0L          
#define PINMUX_PD8A_GMAC_GMDC                      ((PIN_PD8A_GMAC_GMDC << 16) | MUX_PD8A_GMAC_GMDC)
#define PIO_PD8A_GMAC_GMDC                         (1UL << 8)  

#define PIN_PD9A_GMAC_GMDIO                        105L         /**< GMAC signal: GMDIO on PD9 mux A*/
#define MUX_PD9A_GMAC_GMDIO                        0L          
#define PINMUX_PD9A_GMAC_GMDIO                     ((PIN_PD9A_GMAC_GMDIO << 16) | MUX_PD9A_GMAC_GMDIO)
#define PIO_PD9A_GMAC_GMDIO                        (1UL << 9)  

#define PIN_PD14A_GMAC_GRXCK                       110L         /**< GMAC signal: GRXCK on PD14 mux A*/
#define MUX_PD14A_GMAC_GRXCK                       0L          
#define PINMUX_PD14A_GMAC_GRXCK                    ((PIN_PD14A_GMAC_GRXCK << 16) | MUX_PD14A_GMAC_GRXCK)
#define PIO_PD14A_GMAC_GRXCK                       (1UL << 14) 

#define PIN_PD4A_GMAC_GRXDV                        100L         /**< GMAC signal: GRXDV on PD4 mux A*/
#define MUX_PD4A_GMAC_GRXDV                        0L          
#define PINMUX_PD4A_GMAC_GRXDV                     ((PIN_PD4A_GMAC_GRXDV << 16) | MUX_PD4A_GMAC_GRXDV)
#define PIO_PD4A_GMAC_GRXDV                        (1UL << 4)  

#define PIN_PD7A_GMAC_GRXER                        103L         /**< GMAC signal: GRXER on PD7 mux A*/
#define MUX_PD7A_GMAC_GRXER                        0L          
#define PINMUX_PD7A_GMAC_GRXER                     ((PIN_PD7A_GMAC_GRXER << 16) | MUX_PD7A_GMAC_GRXER)
#define PIO_PD7A_GMAC_GRXER                        (1UL << 7)  

#define PIN_PD5A_GMAC_GRX0                         101L         /**< GMAC signal: GRX0 on PD5 mux A*/
#define MUX_PD5A_GMAC_GRX0                         0L          
#define PINMUX_PD5A_GMAC_GRX0                      ((PIN_PD5A_GMAC_GRX0 << 16) | MUX_PD5A_GMAC_GRX0)
#define PIO_PD5A_GMAC_GRX0                         (1UL << 5)  

#define PIN_PD6A_GMAC_GRX1                         102L         /**< GMAC signal: GRX1 on PD6 mux A*/
#define MUX_PD6A_GMAC_GRX1                         0L          
#define PINMUX_PD6A_GMAC_GRX1                      ((PIN_PD6A_GMAC_GRX1 << 16) | MUX_PD6A_GMAC_GRX1)
#define PIO_PD6A_GMAC_GRX1                         (1UL << 6)  

#define PIN_PD11A_GMAC_GRX2                        107L         /**< GMAC signal: GRX2 on PD11 mux A*/
#define MUX_PD11A_GMAC_GRX2                        0L          
#define PINMUX_PD11A_GMAC_GRX2                     ((PIN_PD11A_GMAC_GRX2 << 16) | MUX_PD11A_GMAC_GRX2)
#define PIO_PD11A_GMAC_GRX2                        (1UL << 11) 

#define PIN_PD12A_GMAC_GRX3                        108L         /**< GMAC signal: GRX3 on PD12 mux A*/
#define MUX_PD12A_GMAC_GRX3                        0L          
#define PINMUX_PD12A_GMAC_GRX3                     ((PIN_PD12A_GMAC_GRX3 << 16) | MUX_PD12A_GMAC_GRX3)
#define PIO_PD12A_GMAC_GRX3                        (1UL << 12) 

#define PIN_PB1B_GMAC_GTSUCOMP                     33L          /**< GMAC signal: GTSUCOMP on PB1 mux B*/
#define MUX_PB1B_GMAC_GTSUCOMP                     1L          
#define PINMUX_PB1B_GMAC_GTSUCOMP                  ((PIN_PB1B_GMAC_GTSUCOMP << 16) | MUX_PB1B_GMAC_GTSUCOMP)
#define PIO_PB1B_GMAC_GTSUCOMP                     (1UL << 1)  

#define PIN_PB12B_GMAC_GTSUCOMP                    44L          /**< GMAC signal: GTSUCOMP on PB12 mux B*/
#define MUX_PB12B_GMAC_GTSUCOMP                    1L          
#define PINMUX_PB12B_GMAC_GTSUCOMP                 ((PIN_PB12B_GMAC_GTSUCOMP << 16) | MUX_PB12B_GMAC_GTSUCOMP)
#define PIO_PB12B_GMAC_GTSUCOMP                    (1UL << 12) 

#define PIN_PD11C_GMAC_GTSUCOMP                    107L         /**< GMAC signal: GTSUCOMP on PD11 mux C*/
#define MUX_PD11C_GMAC_GTSUCOMP                    2L          
#define PINMUX_PD11C_GMAC_GTSUCOMP                 ((PIN_PD11C_GMAC_GTSUCOMP << 16) | MUX_PD11C_GMAC_GTSUCOMP)
#define PIO_PD11C_GMAC_GTSUCOMP                    (1UL << 11) 

#define PIN_PD20C_GMAC_GTSUCOMP                    116L         /**< GMAC signal: GTSUCOMP on PD20 mux C*/
#define MUX_PD20C_GMAC_GTSUCOMP                    2L          
#define PINMUX_PD20C_GMAC_GTSUCOMP                 ((PIN_PD20C_GMAC_GTSUCOMP << 16) | MUX_PD20C_GMAC_GTSUCOMP)
#define PIO_PD20C_GMAC_GTSUCOMP                    (1UL << 20) 

#define PIN_PD0A_GMAC_GTXCK                        96L          /**< GMAC signal: GTXCK on PD0 mux A*/
#define MUX_PD0A_GMAC_GTXCK                        0L          
#define PINMUX_PD0A_GMAC_GTXCK                     ((PIN_PD0A_GMAC_GTXCK << 16) | MUX_PD0A_GMAC_GTXCK)
#define PIO_PD0A_GMAC_GTXCK                        (1UL << 0)  

#define PIN_PD1A_GMAC_GTXEN                        97L          /**< GMAC signal: GTXEN on PD1 mux A*/
#define MUX_PD1A_GMAC_GTXEN                        0L          
#define PINMUX_PD1A_GMAC_GTXEN                     ((PIN_PD1A_GMAC_GTXEN << 16) | MUX_PD1A_GMAC_GTXEN)
#define PIO_PD1A_GMAC_GTXEN                        (1UL << 1)  

#define PIN_PD17A_GMAC_GTXER                       113L         /**< GMAC signal: GTXER on PD17 mux A*/
#define MUX_PD17A_GMAC_GTXER                       0L          
#define PINMUX_PD17A_GMAC_GTXER                    ((PIN_PD17A_GMAC_GTXER << 16) | MUX_PD17A_GMAC_GTXER)
#define PIO_PD17A_GMAC_GTXER                       (1UL << 17) 

#define PIN_PD2A_GMAC_GTX0                         98L          /**< GMAC signal: GTX0 on PD2 mux A*/
#define MUX_PD2A_GMAC_GTX0                         0L          
#define PINMUX_PD2A_GMAC_GTX0                      ((PIN_PD2A_GMAC_GTX0 << 16) | MUX_PD2A_GMAC_GTX0)
#define PIO_PD2A_GMAC_GTX0                         (1UL << 2)  

#define PIN_PD3A_GMAC_GTX1                         99L          /**< GMAC signal: GTX1 on PD3 mux A*/
#define MUX_PD3A_GMAC_GTX1                         0L          
#define PINMUX_PD3A_GMAC_GTX1                      ((PIN_PD3A_GMAC_GTX1 << 16) | MUX_PD3A_GMAC_GTX1)
#define PIO_PD3A_GMAC_GTX1                         (1UL << 3)  

#define PIN_PD15A_GMAC_GTX2                        111L         /**< GMAC signal: GTX2 on PD15 mux A*/
#define MUX_PD15A_GMAC_GTX2                        0L          
#define PINMUX_PD15A_GMAC_GTX2                     ((PIN_PD15A_GMAC_GTX2 << 16) | MUX_PD15A_GMAC_GTX2)
#define PIO_PD15A_GMAC_GTX2                        (1UL << 15) 

#define PIN_PD16A_GMAC_GTX3                        112L         /**< GMAC signal: GTX3 on PD16 mux A*/
#define MUX_PD16A_GMAC_GTX3                        0L          
#define PINMUX_PD16A_GMAC_GTX3                     ((PIN_PD16A_GMAC_GTX3 << 16) | MUX_PD16A_GMAC_GTX3)
#define PIO_PD16A_GMAC_GTX3                        (1UL << 16) 

/* ========== PIO definition for HSMCI peripheral ========== */
#define PIN_PA28C_HSMCI_MCCDA                      28L          /**< HSMCI signal: MCCDA on PA28 mux C*/
#define MUX_PA28C_HSMCI_MCCDA                      2L          
#define PINMUX_PA28C_HSMCI_MCCDA                   ((PIN_PA28C_HSMCI_MCCDA << 16) | MUX_PA28C_HSMCI_MCCDA)
#define PIO_PA28C_HSMCI_MCCDA                      (1UL << 28) 

#define PIN_PA25D_HSMCI_MCCK                       25L          /**< HSMCI signal: MCCK on PA25 mux D*/
#define MUX_PA25D_HSMCI_MCCK                       3L          
#define PINMUX_PA25D_HSMCI_MCCK                    ((PIN_PA25D_HSMCI_MCCK << 16) | MUX_PA25D_HSMCI_MCCK)
#define PIO_PA25D_HSMCI_MCCK                       (1UL << 25) 

#define PIN_PA30C_HSMCI_MCDA0                      30L          /**< HSMCI signal: MCDA0 on PA30 mux C*/
#define MUX_PA30C_HSMCI_MCDA0                      2L          
#define PINMUX_PA30C_HSMCI_MCDA0                   ((PIN_PA30C_HSMCI_MCDA0 << 16) | MUX_PA30C_HSMCI_MCDA0)
#define PIO_PA30C_HSMCI_MCDA0                      (1UL << 30) 

#define PIN_PA31C_HSMCI_MCDA1                      31L          /**< HSMCI signal: MCDA1 on PA31 mux C*/
#define MUX_PA31C_HSMCI_MCDA1                      2L          
#define PINMUX_PA31C_HSMCI_MCDA1                   ((PIN_PA31C_HSMCI_MCDA1 << 16) | MUX_PA31C_HSMCI_MCDA1)
#define PIO_PA31C_HSMCI_MCDA1                      (1UL << 31) 

#define PIN_PA26C_HSMCI_MCDA2                      26L          /**< HSMCI signal: MCDA2 on PA26 mux C*/
#define MUX_PA26C_HSMCI_MCDA2                      2L          
#define PINMUX_PA26C_HSMCI_MCDA2                   ((PIN_PA26C_HSMCI_MCDA2 << 16) | MUX_PA26C_HSMCI_MCDA2)
#define PIO_PA26C_HSMCI_MCDA2                      (1UL << 26) 

#define PIN_PA27C_HSMCI_MCDA3                      27L          /**< HSMCI signal: MCDA3 on PA27 mux C*/
#define MUX_PA27C_HSMCI_MCDA3                      2L          
#define PINMUX_PA27C_HSMCI_MCDA3                   ((PIN_PA27C_HSMCI_MCDA3 << 16) | MUX_PA27C_HSMCI_MCDA3)
#define PIO_PA27C_HSMCI_MCDA3                      (1UL << 27) 

/* ========== PIO definition for ISI peripheral ========== */
#define PIN_PD22D_ISI_D0                           118L         /**< ISI signal: D0 on PD22 mux D*/
#define MUX_PD22D_ISI_D0                           3L          
#define PINMUX_PD22D_ISI_D0                        ((PIN_PD22D_ISI_D0 << 16) | MUX_PD22D_ISI_D0)
#define PIO_PD22D_ISI_D0                           (1UL << 22) 

#define PIN_PD21D_ISI_D1                           117L         /**< ISI signal: D1 on PD21 mux D*/
#define MUX_PD21D_ISI_D1                           3L          
#define PINMUX_PD21D_ISI_D1                        ((PIN_PD21D_ISI_D1 << 16) | MUX_PD21D_ISI_D1)
#define PIO_PD21D_ISI_D1                           (1UL << 21) 

#define PIN_PB3D_ISI_D2                            35L          /**< ISI signal: D2 on PB3 mux D*/
#define MUX_PB3D_ISI_D2                            3L          
#define PINMUX_PB3D_ISI_D2                         ((PIN_PB3D_ISI_D2 << 16) | MUX_PB3D_ISI_D2)
#define PIO_PB3D_ISI_D2                            (1UL << 3)  

#define PIN_PA9B_ISI_D3                            9L           /**< ISI signal: D3 on PA9 mux B*/
#define MUX_PA9B_ISI_D3                            1L          
#define PINMUX_PA9B_ISI_D3                         ((PIN_PA9B_ISI_D3 << 16) | MUX_PA9B_ISI_D3)
#define PIO_PA9B_ISI_D3                            (1UL << 9)  

#define PIN_PA5B_ISI_D4                            5L           /**< ISI signal: D4 on PA5 mux B*/
#define MUX_PA5B_ISI_D4                            1L          
#define PINMUX_PA5B_ISI_D4                         ((PIN_PA5B_ISI_D4 << 16) | MUX_PA5B_ISI_D4)
#define PIO_PA5B_ISI_D4                            (1UL << 5)  

#define PIN_PD11D_ISI_D5                           107L         /**< ISI signal: D5 on PD11 mux D*/
#define MUX_PD11D_ISI_D5                           3L          
#define PINMUX_PD11D_ISI_D5                        ((PIN_PD11D_ISI_D5 << 16) | MUX_PD11D_ISI_D5)
#define PIO_PD11D_ISI_D5                           (1UL << 11) 

#define PIN_PD12D_ISI_D6                           108L         /**< ISI signal: D6 on PD12 mux D*/
#define MUX_PD12D_ISI_D6                           3L          
#define PINMUX_PD12D_ISI_D6                        ((PIN_PD12D_ISI_D6 << 16) | MUX_PD12D_ISI_D6)
#define PIO_PD12D_ISI_D6                           (1UL << 12) 

#define PIN_PA27D_ISI_D7                           27L          /**< ISI signal: D7 on PA27 mux D*/
#define MUX_PA27D_ISI_D7                           3L          
#define PINMUX_PA27D_ISI_D7                        ((PIN_PA27D_ISI_D7 << 16) | MUX_PA27D_ISI_D7)
#define PIO_PA27D_ISI_D7                           (1UL << 27) 

#define PIN_PD27D_ISI_D8                           123L         /**< ISI signal: D8 on PD27 mux D*/
#define MUX_PD27D_ISI_D8                           3L          
#define PINMUX_PD27D_ISI_D8                        ((PIN_PD27D_ISI_D8 << 16) | MUX_PD27D_ISI_D8)
#define PIO_PD27D_ISI_D8                           (1UL << 27) 

#define PIN_PD28D_ISI_D9                           124L         /**< ISI signal: D9 on PD28 mux D*/
#define MUX_PD28D_ISI_D9                           3L          
#define PINMUX_PD28D_ISI_D9                        ((PIN_PD28D_ISI_D9 << 16) | MUX_PD28D_ISI_D9)
#define PIO_PD28D_ISI_D9                           (1UL << 28) 

#define PIN_PD30D_ISI_D10                          126L         /**< ISI signal: D10 on PD30 mux D*/
#define MUX_PD30D_ISI_D10                          3L          
#define PINMUX_PD30D_ISI_D10                       ((PIN_PD30D_ISI_D10 << 16) | MUX_PD30D_ISI_D10)
#define PIO_PD30D_ISI_D10                          (1UL << 30) 

#define PIN_PD31D_ISI_D11                          127L         /**< ISI signal: D11 on PD31 mux D*/
#define MUX_PD31D_ISI_D11                          3L          
#define PINMUX_PD31D_ISI_D11                       ((PIN_PD31D_ISI_D11 << 16) | MUX_PD31D_ISI_D11)
#define PIO_PD31D_ISI_D11                          (1UL << 31) 

#define PIN_PD24D_ISI_HSYNC                        120L         /**< ISI signal: HSYNC on PD24 mux D*/
#define MUX_PD24D_ISI_HSYNC                        3L          
#define PINMUX_PD24D_ISI_HSYNC                     ((PIN_PD24D_ISI_HSYNC << 16) | MUX_PD24D_ISI_HSYNC)
#define PIO_PD24D_ISI_HSYNC                        (1UL << 24) 

#define PIN_PA24D_ISI_PCK                          24L          /**< ISI signal: PCK on PA24 mux D*/
#define MUX_PA24D_ISI_PCK                          3L          
#define PINMUX_PA24D_ISI_PCK                       ((PIN_PA24D_ISI_PCK << 16) | MUX_PA24D_ISI_PCK)
#define PIO_PA24D_ISI_PCK                          (1UL << 24) 

#define PIN_PD25D_ISI_VSYNC                        121L         /**< ISI signal: VSYNC on PD25 mux D*/
#define MUX_PD25D_ISI_VSYNC                        3L          
#define PINMUX_PD25D_ISI_VSYNC                     ((PIN_PD25D_ISI_VSYNC << 16) | MUX_PD25D_ISI_VSYNC)
#define PIO_PD25D_ISI_VSYNC                        (1UL << 25) 

/* ========== PIO definition for MCAN0 peripheral ========== */
#define PIN_PB3A_MCAN0_CANRX0                      35L          /**< MCAN0 signal: CANRX0 on PB3 mux A*/
#define MUX_PB3A_MCAN0_CANRX0                      0L          
#define PINMUX_PB3A_MCAN0_CANRX0                   ((PIN_PB3A_MCAN0_CANRX0 << 16) | MUX_PB3A_MCAN0_CANRX0)
#define PIO_PB3A_MCAN0_CANRX0                      (1UL << 3)  

#define PIN_PB2A_MCAN0_CANTX0                      34L          /**< MCAN0 signal: CANTX0 on PB2 mux A*/
#define MUX_PB2A_MCAN0_CANTX0                      0L          
#define PINMUX_PB2A_MCAN0_CANTX0                   ((PIN_PB2A_MCAN0_CANTX0 << 16) | MUX_PB2A_MCAN0_CANTX0)
#define PIO_PB2A_MCAN0_CANTX0                      (1UL << 2)  

/* ========== PIO definition for MCAN1 peripheral ========== */
#define PIN_PD28B_MCAN1_CANRX1                     124L         /**< MCAN1 signal: CANRX1 on PD28 mux B*/
#define MUX_PD28B_MCAN1_CANRX1                     1L          
#define PINMUX_PD28B_MCAN1_CANRX1                  ((PIN_PD28B_MCAN1_CANRX1 << 16) | MUX_PD28B_MCAN1_CANRX1)
#define PIO_PD28B_MCAN1_CANRX1                     (1UL << 28) 

#define PIN_PD12B_MCAN1_CANTX1                     108L         /**< MCAN1 signal: CANTX1 on PD12 mux B*/
#define MUX_PD12B_MCAN1_CANTX1                     1L          
#define PINMUX_PD12B_MCAN1_CANTX1                  ((PIN_PD12B_MCAN1_CANTX1 << 16) | MUX_PD12B_MCAN1_CANTX1)
#define PIO_PD12B_MCAN1_CANTX1                     (1UL << 12) 

/* ========== PIO definition for PMC peripheral ========== */
#define PIN_PA6B_PMC_PCK0                          6L           /**< PMC signal: PCK0 on PA6 mux B*/
#define MUX_PA6B_PMC_PCK0                          1L          
#define PINMUX_PA6B_PMC_PCK0                       ((PIN_PA6B_PMC_PCK0 << 16) | MUX_PA6B_PMC_PCK0)
#define PIO_PA6B_PMC_PCK0                          (1UL << 6)  

#define PIN_PB12D_PMC_PCK0                         44L          /**< PMC signal: PCK0 on PB12 mux D*/
#define MUX_PB12D_PMC_PCK0                         3L          
#define PINMUX_PB12D_PMC_PCK0                      ((PIN_PB12D_PMC_PCK0 << 16) | MUX_PB12D_PMC_PCK0)
#define PIO_PB12D_PMC_PCK0                         (1UL << 12) 

#define PIN_PB13B_PMC_PCK0                         45L          /**< PMC signal: PCK0 on PB13 mux B*/
#define MUX_PB13B_PMC_PCK0                         1L          
#define PINMUX_PB13B_PMC_PCK0                      ((PIN_PB13B_PMC_PCK0 << 16) | MUX_PB13B_PMC_PCK0)
#define PIO_PB13B_PMC_PCK0                         (1UL << 13) 

#define PIN_PA17B_PMC_PCK1                         17L          /**< PMC signal: PCK1 on PA17 mux B*/
#define MUX_PA17B_PMC_PCK1                         1L          
#define PINMUX_PA17B_PMC_PCK1                      ((PIN_PA17B_PMC_PCK1 << 16) | MUX_PA17B_PMC_PCK1)
#define PIO_PA17B_PMC_PCK1                         (1UL << 17) 

#define PIN_PA21B_PMC_PCK1                         21L          /**< PMC signal: PCK1 on PA21 mux B*/
#define MUX_PA21B_PMC_PCK1                         1L          
#define PINMUX_PA21B_PMC_PCK1                      ((PIN_PA21B_PMC_PCK1 << 16) | MUX_PA21B_PMC_PCK1)
#define PIO_PA21B_PMC_PCK1                         (1UL << 21) 

#define PIN_PA3C_PMC_PCK2                          3L           /**< PMC signal: PCK2 on PA3 mux C*/
#define MUX_PA3C_PMC_PCK2                          2L          
#define PINMUX_PA3C_PMC_PCK2                       ((PIN_PA3C_PMC_PCK2 << 16) | MUX_PA3C_PMC_PCK2)
#define PIO_PA3C_PMC_PCK2                          (1UL << 3)  

#define PIN_PA18B_PMC_PCK2                         18L          /**< PMC signal: PCK2 on PA18 mux B*/
#define MUX_PA18B_PMC_PCK2                         1L          
#define PINMUX_PA18B_PMC_PCK2                      ((PIN_PA18B_PMC_PCK2 << 16) | MUX_PA18B_PMC_PCK2)
#define PIO_PA18B_PMC_PCK2                         (1UL << 18) 

#define PIN_PA31B_PMC_PCK2                         31L          /**< PMC signal: PCK2 on PA31 mux B*/
#define MUX_PA31B_PMC_PCK2                         1L          
#define PINMUX_PA31B_PMC_PCK2                      ((PIN_PA31B_PMC_PCK2 << 16) | MUX_PA31B_PMC_PCK2)
#define PIO_PA31B_PMC_PCK2                         (1UL << 31) 

#define PIN_PB3B_PMC_PCK2                          35L          /**< PMC signal: PCK2 on PB3 mux B*/
#define MUX_PB3B_PMC_PCK2                          1L          
#define PINMUX_PB3B_PMC_PCK2                       ((PIN_PB3B_PMC_PCK2 << 16) | MUX_PB3B_PMC_PCK2)
#define PIO_PB3B_PMC_PCK2                          (1UL << 3)  

#define PIN_PD31C_PMC_PCK2                         127L         /**< PMC signal: PCK2 on PD31 mux C*/
#define MUX_PD31C_PMC_PCK2                         2L          
#define PINMUX_PD31C_PMC_PCK2                      ((PIN_PD31C_PMC_PCK2 << 16) | MUX_PD31C_PMC_PCK2)
#define PIO_PD31C_PMC_PCK2                         (1UL << 31) 

/* ========== PIO definition for PWM0 peripheral ========== */
#define PIN_PA10B_PWM0_PWMEXTRG0                   10L          /**< PWM0 signal: PWMEXTRG0 on PA10 mux B*/
#define MUX_PA10B_PWM0_PWMEXTRG0                   1L          
#define PINMUX_PA10B_PWM0_PWMEXTRG0                ((PIN_PA10B_PWM0_PWMEXTRG0 << 16) | MUX_PA10B_PWM0_PWMEXTRG0)
#define PIO_PA10B_PWM0_PWMEXTRG0                   (1UL << 10) 

#define PIN_PA22B_PWM0_PWMEXTRG1                   22L          /**< PWM0 signal: PWMEXTRG1 on PA22 mux B*/
#define MUX_PA22B_PWM0_PWMEXTRG1                   1L          
#define PINMUX_PA22B_PWM0_PWMEXTRG1                ((PIN_PA22B_PWM0_PWMEXTRG1 << 16) | MUX_PA22B_PWM0_PWMEXTRG1)
#define PIO_PA22B_PWM0_PWMEXTRG1                   (1UL << 22) 

#define PIN_PA9C_PWM0_PWMFI0                       9L           /**< PWM0 signal: PWMFI0 on PA9 mux C*/
#define MUX_PA9C_PWM0_PWMFI0                       2L          
#define PINMUX_PA9C_PWM0_PWMFI0                    ((PIN_PA9C_PWM0_PWMFI0 << 16) | MUX_PA9C_PWM0_PWMFI0)
#define PIO_PA9C_PWM0_PWMFI0                       (1UL << 9)  

#define PIN_PD8B_PWM0_PWMFI1                       104L         /**< PWM0 signal: PWMFI1 on PD8 mux B*/
#define MUX_PD8B_PWM0_PWMFI1                       1L          
#define PINMUX_PD8B_PWM0_PWMFI1                    ((PIN_PD8B_PWM0_PWMFI1 << 16) | MUX_PD8B_PWM0_PWMFI1)
#define PIO_PD8B_PWM0_PWMFI1                       (1UL << 8)  

#define PIN_PD9B_PWM0_PWMFI2                       105L         /**< PWM0 signal: PWMFI2 on PD9 mux B*/
#define MUX_PD9B_PWM0_PWMFI2                       1L          
#define PINMUX_PD9B_PWM0_PWMFI2                    ((PIN_PD9B_PWM0_PWMFI2 << 16) | MUX_PD9B_PWM0_PWMFI2)
#define PIO_PD9B_PWM0_PWMFI2                       (1UL << 9)  

#define PIN_PA0A_PWM0_PWMH0                        0L           /**< PWM0 signal: PWMH0 on PA0 mux A*/
#define MUX_PA0A_PWM0_PWMH0                        0L          
#define PINMUX_PA0A_PWM0_PWMH0                     ((PIN_PA0A_PWM0_PWMH0 << 16) | MUX_PA0A_PWM0_PWMH0)
#define PIO_PA0A_PWM0_PWMH0                        (1UL << 0)  

#define PIN_PA11B_PWM0_PWMH0                       11L          /**< PWM0 signal: PWMH0 on PA11 mux B*/
#define MUX_PA11B_PWM0_PWMH0                       1L          
#define PINMUX_PA11B_PWM0_PWMH0                    ((PIN_PA11B_PWM0_PWMH0 << 16) | MUX_PA11B_PWM0_PWMH0)
#define PIO_PA11B_PWM0_PWMH0                       (1UL << 11) 

#define PIN_PA23B_PWM0_PWMH0                       23L          /**< PWM0 signal: PWMH0 on PA23 mux B*/
#define MUX_PA23B_PWM0_PWMH0                       1L          
#define PINMUX_PA23B_PWM0_PWMH0                    ((PIN_PA23B_PWM0_PWMH0 << 16) | MUX_PA23B_PWM0_PWMH0)
#define PIO_PA23B_PWM0_PWMH0                       (1UL << 23) 

#define PIN_PB0A_PWM0_PWMH0                        32L          /**< PWM0 signal: PWMH0 on PB0 mux A*/
#define MUX_PB0A_PWM0_PWMH0                        0L          
#define PINMUX_PB0A_PWM0_PWMH0                     ((PIN_PB0A_PWM0_PWMH0 << 16) | MUX_PB0A_PWM0_PWMH0)
#define PIO_PB0A_PWM0_PWMH0                        (1UL << 0)  

#define PIN_PD11B_PWM0_PWMH0                       107L         /**< PWM0 signal: PWMH0 on PD11 mux B*/
#define MUX_PD11B_PWM0_PWMH0                       1L          
#define PINMUX_PD11B_PWM0_PWMH0                    ((PIN_PD11B_PWM0_PWMH0 << 16) | MUX_PD11B_PWM0_PWMH0)
#define PIO_PD11B_PWM0_PWMH0                       (1UL << 11) 

#define PIN_PD20A_PWM0_PWMH0                       116L         /**< PWM0 signal: PWMH0 on PD20 mux A*/
#define MUX_PD20A_PWM0_PWMH0                       0L          
#define PINMUX_PD20A_PWM0_PWMH0                    ((PIN_PD20A_PWM0_PWMH0 << 16) | MUX_PD20A_PWM0_PWMH0)
#define PIO_PD20A_PWM0_PWMH0                       (1UL << 20) 

#define PIN_PA2A_PWM0_PWMH1                        2L           /**< PWM0 signal: PWMH1 on PA2 mux A*/
#define MUX_PA2A_PWM0_PWMH1                        0L          
#define PINMUX_PA2A_PWM0_PWMH1                     ((PIN_PA2A_PWM0_PWMH1 << 16) | MUX_PA2A_PWM0_PWMH1)
#define PIO_PA2A_PWM0_PWMH1                        (1UL << 2)  

#define PIN_PA12B_PWM0_PWMH1                       12L          /**< PWM0 signal: PWMH1 on PA12 mux B*/
#define MUX_PA12B_PWM0_PWMH1                       1L          
#define PINMUX_PA12B_PWM0_PWMH1                    ((PIN_PA12B_PWM0_PWMH1 << 16) | MUX_PA12B_PWM0_PWMH1)
#define PIO_PA12B_PWM0_PWMH1                       (1UL << 12) 

#define PIN_PA24B_PWM0_PWMH1                       24L          /**< PWM0 signal: PWMH1 on PA24 mux B*/
#define MUX_PA24B_PWM0_PWMH1                       1L          
#define PINMUX_PA24B_PWM0_PWMH1                    ((PIN_PA24B_PWM0_PWMH1 << 16) | MUX_PA24B_PWM0_PWMH1)
#define PIO_PA24B_PWM0_PWMH1                       (1UL << 24) 

#define PIN_PB1A_PWM0_PWMH1                        33L          /**< PWM0 signal: PWMH1 on PB1 mux A*/
#define MUX_PB1A_PWM0_PWMH1                        0L          
#define PINMUX_PB1A_PWM0_PWMH1                     ((PIN_PB1A_PWM0_PWMH1 << 16) | MUX_PB1A_PWM0_PWMH1)
#define PIO_PB1A_PWM0_PWMH1                        (1UL << 1)  

#define PIN_PD21A_PWM0_PWMH1                       117L         /**< PWM0 signal: PWMH1 on PD21 mux A*/
#define MUX_PD21A_PWM0_PWMH1                       0L          
#define PINMUX_PD21A_PWM0_PWMH1                    ((PIN_PD21A_PWM0_PWMH1 << 16) | MUX_PD21A_PWM0_PWMH1)
#define PIO_PD21A_PWM0_PWMH1                       (1UL << 21) 

#define PIN_PA13B_PWM0_PWMH2                       13L          /**< PWM0 signal: PWMH2 on PA13 mux B*/
#define MUX_PA13B_PWM0_PWMH2                       1L          
#define PINMUX_PA13B_PWM0_PWMH2                    ((PIN_PA13B_PWM0_PWMH2 << 16) | MUX_PA13B_PWM0_PWMH2)
#define PIO_PA13B_PWM0_PWMH2                       (1UL << 13) 

#define PIN_PA25B_PWM0_PWMH2                       25L          /**< PWM0 signal: PWMH2 on PA25 mux B*/
#define MUX_PA25B_PWM0_PWMH2                       1L          
#define PINMUX_PA25B_PWM0_PWMH2                    ((PIN_PA25B_PWM0_PWMH2 << 16) | MUX_PA25B_PWM0_PWMH2)
#define PIO_PA25B_PWM0_PWMH2                       (1UL << 25) 

#define PIN_PB4B_PWM0_PWMH2                        36L          /**< PWM0 signal: PWMH2 on PB4 mux B*/
#define MUX_PB4B_PWM0_PWMH2                        1L          
#define PINMUX_PB4B_PWM0_PWMH2                     ((PIN_PB4B_PWM0_PWMH2 << 16) | MUX_PB4B_PWM0_PWMH2)
#define PIO_PB4B_PWM0_PWMH2                        (1UL << 4)  

#define PIN_PD22A_PWM0_PWMH2                       118L         /**< PWM0 signal: PWMH2 on PD22 mux A*/
#define MUX_PD22A_PWM0_PWMH2                       0L          
#define PINMUX_PD22A_PWM0_PWMH2                    ((PIN_PD22A_PWM0_PWMH2 << 16) | MUX_PD22A_PWM0_PWMH2)
#define PIO_PD22A_PWM0_PWMH2                       (1UL << 22) 

#define PIN_PA7B_PWM0_PWMH3                        7L           /**< PWM0 signal: PWMH3 on PA7 mux B*/
#define MUX_PA7B_PWM0_PWMH3                        1L          
#define PINMUX_PA7B_PWM0_PWMH3                     ((PIN_PA7B_PWM0_PWMH3 << 16) | MUX_PA7B_PWM0_PWMH3)
#define PIO_PA7B_PWM0_PWMH3                        (1UL << 7)  

#define PIN_PA14B_PWM0_PWMH3                       14L          /**< PWM0 signal: PWMH3 on PA14 mux B*/
#define MUX_PA14B_PWM0_PWMH3                       1L          
#define PINMUX_PA14B_PWM0_PWMH3                    ((PIN_PA14B_PWM0_PWMH3 << 16) | MUX_PA14B_PWM0_PWMH3)
#define PIO_PA14B_PWM0_PWMH3                       (1UL << 14) 

#define PIN_PA17C_PWM0_PWMH3                       17L          /**< PWM0 signal: PWMH3 on PA17 mux C*/
#define MUX_PA17C_PWM0_PWMH3                       2L          
#define PINMUX_PA17C_PWM0_PWMH3                    ((PIN_PA17C_PWM0_PWMH3 << 16) | MUX_PA17C_PWM0_PWMH3)
#define PIO_PA17C_PWM0_PWMH3                       (1UL << 17) 

#define PIN_PD23A_PWM0_PWMH3                       119L         /**< PWM0 signal: PWMH3 on PD23 mux A*/
#define MUX_PD23A_PWM0_PWMH3                       0L          
#define PINMUX_PD23A_PWM0_PWMH3                    ((PIN_PD23A_PWM0_PWMH3 << 16) | MUX_PD23A_PWM0_PWMH3)
#define PIO_PD23A_PWM0_PWMH3                       (1UL << 23) 

#define PIN_PA1A_PWM0_PWML0                        1L           /**< PWM0 signal: PWML0 on PA1 mux A*/
#define MUX_PA1A_PWM0_PWML0                        0L          
#define PINMUX_PA1A_PWM0_PWML0                     ((PIN_PA1A_PWM0_PWML0 << 16) | MUX_PA1A_PWM0_PWML0)
#define PIO_PA1A_PWM0_PWML0                        (1UL << 1)  

#define PIN_PA19B_PWM0_PWML0                       19L          /**< PWM0 signal: PWML0 on PA19 mux B*/
#define MUX_PA19B_PWM0_PWML0                       1L          
#define PINMUX_PA19B_PWM0_PWML0                    ((PIN_PA19B_PWM0_PWML0 << 16) | MUX_PA19B_PWM0_PWML0)
#define PIO_PA19B_PWM0_PWML0                       (1UL << 19) 

#define PIN_PB5B_PWM0_PWML0                        37L          /**< PWM0 signal: PWML0 on PB5 mux B*/
#define MUX_PB5B_PWM0_PWML0                        1L          
#define PINMUX_PB5B_PWM0_PWML0                     ((PIN_PB5B_PWM0_PWML0 << 16) | MUX_PB5B_PWM0_PWML0)
#define PIO_PB5B_PWM0_PWML0                        (1UL << 5)  

#define PIN_PD10B_PWM0_PWML0                       106L         /**< PWM0 signal: PWML0 on PD10 mux B*/
#define MUX_PD10B_PWM0_PWML0                       1L          
#define PINMUX_PD10B_PWM0_PWML0                    ((PIN_PD10B_PWM0_PWML0 << 16) | MUX_PD10B_PWM0_PWML0)
#define PIO_PD10B_PWM0_PWML0                       (1UL << 10) 

#define PIN_PD24A_PWM0_PWML0                       120L         /**< PWM0 signal: PWML0 on PD24 mux A*/
#define MUX_PD24A_PWM0_PWML0                       0L          
#define PINMUX_PD24A_PWM0_PWML0                    ((PIN_PD24A_PWM0_PWML0 << 16) | MUX_PD24A_PWM0_PWML0)
#define PIO_PD24A_PWM0_PWML0                       (1UL << 24) 

#define PIN_PA20B_PWM0_PWML1                       20L          /**< PWM0 signal: PWML1 on PA20 mux B*/
#define MUX_PA20B_PWM0_PWML1                       1L          
#define PINMUX_PA20B_PWM0_PWML1                    ((PIN_PA20B_PWM0_PWML1 << 16) | MUX_PA20B_PWM0_PWML1)
#define PIO_PA20B_PWM0_PWML1                       (1UL << 20) 

#define PIN_PB12A_PWM0_PWML1                       44L          /**< PWM0 signal: PWML1 on PB12 mux A*/
#define MUX_PB12A_PWM0_PWML1                       0L          
#define PINMUX_PB12A_PWM0_PWML1                    ((PIN_PB12A_PWM0_PWML1 << 16) | MUX_PB12A_PWM0_PWML1)
#define PIO_PB12A_PWM0_PWML1                       (1UL << 12) 

#define PIN_PD25A_PWM0_PWML1                       121L         /**< PWM0 signal: PWML1 on PD25 mux A*/
#define MUX_PD25A_PWM0_PWML1                       0L          
#define PINMUX_PD25A_PWM0_PWML1                    ((PIN_PD25A_PWM0_PWML1 << 16) | MUX_PD25A_PWM0_PWML1)
#define PIO_PD25A_PWM0_PWML1                       (1UL << 25) 

#define PIN_PA16C_PWM0_PWML2                       16L          /**< PWM0 signal: PWML2 on PA16 mux C*/
#define MUX_PA16C_PWM0_PWML2                       2L          
#define PINMUX_PA16C_PWM0_PWML2                    ((PIN_PA16C_PWM0_PWML2 << 16) | MUX_PA16C_PWM0_PWML2)
#define PIO_PA16C_PWM0_PWML2                       (1UL << 16) 

#define PIN_PA30A_PWM0_PWML2                       30L          /**< PWM0 signal: PWML2 on PA30 mux A*/
#define MUX_PA30A_PWM0_PWML2                       0L          
#define PINMUX_PA30A_PWM0_PWML2                    ((PIN_PA30A_PWM0_PWML2 << 16) | MUX_PA30A_PWM0_PWML2)
#define PIO_PA30A_PWM0_PWML2                       (1UL << 30) 

#define PIN_PB13A_PWM0_PWML2                       45L          /**< PWM0 signal: PWML2 on PB13 mux A*/
#define MUX_PB13A_PWM0_PWML2                       0L          
#define PINMUX_PB13A_PWM0_PWML2                    ((PIN_PB13A_PWM0_PWML2 << 16) | MUX_PB13A_PWM0_PWML2)
#define PIO_PB13A_PWM0_PWML2                       (1UL << 13) 

#define PIN_PD26A_PWM0_PWML2                       122L         /**< PWM0 signal: PWML2 on PD26 mux A*/
#define MUX_PD26A_PWM0_PWML2                       0L          
#define PINMUX_PD26A_PWM0_PWML2                    ((PIN_PD26A_PWM0_PWML2 << 16) | MUX_PD26A_PWM0_PWML2)
#define PIO_PD26A_PWM0_PWML2                       (1UL << 26) 

#define PIN_PA15C_PWM0_PWML3                       15L          /**< PWM0 signal: PWML3 on PA15 mux C*/
#define MUX_PA15C_PWM0_PWML3                       2L          
#define PINMUX_PA15C_PWM0_PWML3                    ((PIN_PA15C_PWM0_PWML3 << 16) | MUX_PA15C_PWM0_PWML3)
#define PIO_PA15C_PWM0_PWML3                       (1UL << 15) 

#define PIN_PD27A_PWM0_PWML3                       123L         /**< PWM0 signal: PWML3 on PD27 mux A*/
#define MUX_PD27A_PWM0_PWML3                       0L          
#define PINMUX_PD27A_PWM0_PWML3                    ((PIN_PD27A_PWM0_PWML3 << 16) | MUX_PD27A_PWM0_PWML3)
#define PIO_PD27A_PWM0_PWML3                       (1UL << 27) 

/* ========== PIO definition for PWM1 peripheral ========== */
#define PIN_PA30B_PWM1_PWMEXTRG0                   30L          /**< PWM1 signal: PWMEXTRG0 on PA30 mux B*/
#define MUX_PA30B_PWM1_PWMEXTRG0                   1L          
#define PINMUX_PA30B_PWM1_PWMEXTRG0                ((PIN_PA30B_PWM1_PWMEXTRG0 << 16) | MUX_PA30B_PWM1_PWMEXTRG0)
#define PIO_PA30B_PWM1_PWMEXTRG0                   (1UL << 30) 

#define PIN_PA18A_PWM1_PWMEXTRG1                   18L          /**< PWM1 signal: PWMEXTRG1 on PA18 mux A*/
#define MUX_PA18A_PWM1_PWMEXTRG1                   0L          
#define PINMUX_PA18A_PWM1_PWMEXTRG1                ((PIN_PA18A_PWM1_PWMEXTRG1 << 16) | MUX_PA18A_PWM1_PWMEXTRG1)
#define PIO_PA18A_PWM1_PWMEXTRG1                   (1UL << 18) 

#define PIN_PA21C_PWM1_PWMFI0                      21L          /**< PWM1 signal: PWMFI0 on PA21 mux C*/
#define MUX_PA21C_PWM1_PWMFI0                      2L          
#define PINMUX_PA21C_PWM1_PWMFI0                   ((PIN_PA21C_PWM1_PWMFI0 << 16) | MUX_PA21C_PWM1_PWMFI0)
#define PIO_PA21C_PWM1_PWMFI0                      (1UL << 21) 

#define PIN_PA26D_PWM1_PWMFI1                      26L          /**< PWM1 signal: PWMFI1 on PA26 mux D*/
#define MUX_PA26D_PWM1_PWMFI1                      3L          
#define PINMUX_PA26D_PWM1_PWMFI1                   ((PIN_PA26D_PWM1_PWMFI1 << 16) | MUX_PA26D_PWM1_PWMFI1)
#define PIO_PA26D_PWM1_PWMFI1                      (1UL << 26) 

#define PIN_PA28D_PWM1_PWMFI2                      28L          /**< PWM1 signal: PWMFI2 on PA28 mux D*/
#define MUX_PA28D_PWM1_PWMFI2                      3L          
#define PINMUX_PA28D_PWM1_PWMFI2                   ((PIN_PA28D_PWM1_PWMFI2 << 16) | MUX_PA28D_PWM1_PWMFI2)
#define PIO_PA28D_PWM1_PWMFI2                      (1UL << 28) 

#define PIN_PA12C_PWM1_PWMH0                       12L          /**< PWM1 signal: PWMH0 on PA12 mux C*/
#define MUX_PA12C_PWM1_PWMH0                       2L          
#define PINMUX_PA12C_PWM1_PWMH0                    ((PIN_PA12C_PWM1_PWMH0 << 16) | MUX_PA12C_PWM1_PWMH0)
#define PIO_PA12C_PWM1_PWMH0                       (1UL << 12) 

#define PIN_PD1B_PWM1_PWMH0                        97L          /**< PWM1 signal: PWMH0 on PD1 mux B*/
#define MUX_PD1B_PWM1_PWMH0                        1L          
#define PINMUX_PD1B_PWM1_PWMH0                     ((PIN_PD1B_PWM1_PWMH0 << 16) | MUX_PD1B_PWM1_PWMH0)
#define PIO_PD1B_PWM1_PWMH0                        (1UL << 1)  

#define PIN_PA14C_PWM1_PWMH1                       14L          /**< PWM1 signal: PWMH1 on PA14 mux C*/
#define MUX_PA14C_PWM1_PWMH1                       2L          
#define PINMUX_PA14C_PWM1_PWMH1                    ((PIN_PA14C_PWM1_PWMH1 << 16) | MUX_PA14C_PWM1_PWMH1)
#define PIO_PA14C_PWM1_PWMH1                       (1UL << 14) 

#define PIN_PD3B_PWM1_PWMH1                        99L          /**< PWM1 signal: PWMH1 on PD3 mux B*/
#define MUX_PD3B_PWM1_PWMH1                        1L          
#define PINMUX_PD3B_PWM1_PWMH1                     ((PIN_PD3B_PWM1_PWMH1 << 16) | MUX_PD3B_PWM1_PWMH1)
#define PIO_PD3B_PWM1_PWMH1                        (1UL << 3)  

#define PIN_PA31D_PWM1_PWMH2                       31L          /**< PWM1 signal: PWMH2 on PA31 mux D*/
#define MUX_PA31D_PWM1_PWMH2                       3L          
#define PINMUX_PA31D_PWM1_PWMH2                    ((PIN_PA31D_PWM1_PWMH2 << 16) | MUX_PA31D_PWM1_PWMH2)
#define PIO_PA31D_PWM1_PWMH2                       (1UL << 31) 

#define PIN_PD5B_PWM1_PWMH2                        101L         /**< PWM1 signal: PWMH2 on PD5 mux B*/
#define MUX_PD5B_PWM1_PWMH2                        1L          
#define PINMUX_PD5B_PWM1_PWMH2                     ((PIN_PD5B_PWM1_PWMH2 << 16) | MUX_PD5B_PWM1_PWMH2)
#define PIO_PD5B_PWM1_PWMH2                        (1UL << 5)  

#define PIN_PA8A_PWM1_PWMH3                        8L           /**< PWM1 signal: PWMH3 on PA8 mux A*/
#define MUX_PA8A_PWM1_PWMH3                        0L          
#define PINMUX_PA8A_PWM1_PWMH3                     ((PIN_PA8A_PWM1_PWMH3 << 16) | MUX_PA8A_PWM1_PWMH3)
#define PIO_PA8A_PWM1_PWMH3                        (1UL << 8)  

#define PIN_PD7B_PWM1_PWMH3                        103L         /**< PWM1 signal: PWMH3 on PD7 mux B*/
#define MUX_PD7B_PWM1_PWMH3                        1L          
#define PINMUX_PD7B_PWM1_PWMH3                     ((PIN_PD7B_PWM1_PWMH3 << 16) | MUX_PD7B_PWM1_PWMH3)
#define PIO_PD7B_PWM1_PWMH3                        (1UL << 7)  

#define PIN_PA11C_PWM1_PWML0                       11L          /**< PWM1 signal: PWML0 on PA11 mux C*/
#define MUX_PA11C_PWM1_PWML0                       2L          
#define PINMUX_PA11C_PWM1_PWML0                    ((PIN_PA11C_PWM1_PWML0 << 16) | MUX_PA11C_PWM1_PWML0)
#define PIO_PA11C_PWM1_PWML0                       (1UL << 11) 

#define PIN_PD0B_PWM1_PWML0                        96L          /**< PWM1 signal: PWML0 on PD0 mux B*/
#define MUX_PD0B_PWM1_PWML0                        1L          
#define PINMUX_PD0B_PWM1_PWML0                     ((PIN_PD0B_PWM1_PWML0 << 16) | MUX_PD0B_PWM1_PWML0)
#define PIO_PD0B_PWM1_PWML0                        (1UL << 0)  

#define PIN_PA13C_PWM1_PWML1                       13L          /**< PWM1 signal: PWML1 on PA13 mux C*/
#define MUX_PA13C_PWM1_PWML1                       2L          
#define PINMUX_PA13C_PWM1_PWML1                    ((PIN_PA13C_PWM1_PWML1 << 16) | MUX_PA13C_PWM1_PWML1)
#define PIO_PA13C_PWM1_PWML1                       (1UL << 13) 

#define PIN_PD2B_PWM1_PWML1                        98L          /**< PWM1 signal: PWML1 on PD2 mux B*/
#define MUX_PD2B_PWM1_PWML1                        1L          
#define PINMUX_PD2B_PWM1_PWML1                     ((PIN_PD2B_PWM1_PWML1 << 16) | MUX_PD2B_PWM1_PWML1)
#define PIO_PD2B_PWM1_PWML1                        (1UL << 2)  

#define PIN_PA23D_PWM1_PWML2                       23L          /**< PWM1 signal: PWML2 on PA23 mux D*/
#define MUX_PA23D_PWM1_PWML2                       3L          
#define PINMUX_PA23D_PWM1_PWML2                    ((PIN_PA23D_PWM1_PWML2 << 16) | MUX_PA23D_PWM1_PWML2)
#define PIO_PA23D_PWM1_PWML2                       (1UL << 23) 

#define PIN_PD4B_PWM1_PWML2                        100L         /**< PWM1 signal: PWML2 on PD4 mux B*/
#define MUX_PD4B_PWM1_PWML2                        1L          
#define PINMUX_PD4B_PWM1_PWML2                     ((PIN_PD4B_PWM1_PWML2 << 16) | MUX_PD4B_PWM1_PWML2)
#define PIO_PD4B_PWM1_PWML2                        (1UL << 4)  

#define PIN_PA5A_PWM1_PWML3                        5L           /**< PWM1 signal: PWML3 on PA5 mux A*/
#define MUX_PA5A_PWM1_PWML3                        0L          
#define PINMUX_PA5A_PWM1_PWML3                     ((PIN_PA5A_PWM1_PWML3 << 16) | MUX_PA5A_PWM1_PWML3)
#define PIO_PA5A_PWM1_PWML3                        (1UL << 5)  

#define PIN_PD6B_PWM1_PWML3                        102L         /**< PWM1 signal: PWML3 on PD6 mux B*/
#define MUX_PD6B_PWM1_PWML3                        1L          
#define PINMUX_PD6B_PWM1_PWML3                     ((PIN_PD6B_PWM1_PWML3 << 16) | MUX_PD6B_PWM1_PWML3)
#define PIO_PD6B_PWM1_PWML3                        (1UL << 6)  

/* ========== PIO definition for QSPI peripheral ========== */
#define PIN_PA11A_QSPI_QCS                         11L          /**< QSPI signal: QCS on PA11 mux A*/
#define MUX_PA11A_QSPI_QCS                         0L          
#define PINMUX_PA11A_QSPI_QCS                      ((PIN_PA11A_QSPI_QCS << 16) | MUX_PA11A_QSPI_QCS)
#define PIO_PA11A_QSPI_QCS                         (1UL << 11) 

#define PIN_PA13A_QSPI_QIO0                        13L          /**< QSPI signal: QIO0 on PA13 mux A*/
#define MUX_PA13A_QSPI_QIO0                        0L          
#define PINMUX_PA13A_QSPI_QIO0                     ((PIN_PA13A_QSPI_QIO0 << 16) | MUX_PA13A_QSPI_QIO0)
#define PIO_PA13A_QSPI_QIO0                        (1UL << 13) 

#define PIN_PA12A_QSPI_QIO1                        12L          /**< QSPI signal: QIO1 on PA12 mux A*/
#define MUX_PA12A_QSPI_QIO1                        0L          
#define PINMUX_PA12A_QSPI_QIO1                     ((PIN_PA12A_QSPI_QIO1 << 16) | MUX_PA12A_QSPI_QIO1)
#define PIO_PA12A_QSPI_QIO1                        (1UL << 12) 

#define PIN_PA17A_QSPI_QIO2                        17L          /**< QSPI signal: QIO2 on PA17 mux A*/
#define MUX_PA17A_QSPI_QIO2                        0L          
#define PINMUX_PA17A_QSPI_QIO2                     ((PIN_PA17A_QSPI_QIO2 << 16) | MUX_PA17A_QSPI_QIO2)
#define PIO_PA17A_QSPI_QIO2                        (1UL << 17) 

#define PIN_PD31A_QSPI_QIO3                        127L         /**< QSPI signal: QIO3 on PD31 mux A*/
#define MUX_PD31A_QSPI_QIO3                        0L          
#define PINMUX_PD31A_QSPI_QIO3                     ((PIN_PD31A_QSPI_QIO3 << 16) | MUX_PD31A_QSPI_QIO3)
#define PIO_PD31A_QSPI_QIO3                        (1UL << 31) 

#define PIN_PA14A_QSPI_QSCK                        14L          /**< QSPI signal: QSCK on PA14 mux A*/
#define MUX_PA14A_QSPI_QSCK                        0L          
#define PINMUX_PA14A_QSPI_QSCK                     ((PIN_PA14A_QSPI_QSCK << 16) | MUX_PA14A_QSPI_QSCK)
#define PIO_PA14A_QSPI_QSCK                        (1UL << 14) 

/* ========== PIO definition for SPI0 peripheral ========== */
#define PIN_PD20B_SPI0_MISO                        116L         /**< SPI0 signal: MISO on PD20 mux B*/
#define MUX_PD20B_SPI0_MISO                        1L          
#define PINMUX_PD20B_SPI0_MISO                     ((PIN_PD20B_SPI0_MISO << 16) | MUX_PD20B_SPI0_MISO)
#define PIO_PD20B_SPI0_MISO                        (1UL << 20) 

#define PIN_PD21B_SPI0_MOSI                        117L         /**< SPI0 signal: MOSI on PD21 mux B*/
#define MUX_PD21B_SPI0_MOSI                        1L          
#define PINMUX_PD21B_SPI0_MOSI                     ((PIN_PD21B_SPI0_MOSI << 16) | MUX_PD21B_SPI0_MOSI)
#define PIO_PD21B_SPI0_MOSI                        (1UL << 21) 

#define PIN_PB2D_SPI0_NPCS0                        34L          /**< SPI0 signal: NPCS0 on PB2 mux D*/
#define MUX_PB2D_SPI0_NPCS0                        3L          
#define PINMUX_PB2D_SPI0_NPCS0                     ((PIN_PB2D_SPI0_NPCS0 << 16) | MUX_PB2D_SPI0_NPCS0)
#define PIO_PB2D_SPI0_NPCS0                        (1UL << 2)  

#define PIN_PA31A_SPI0_NPCS1                       31L          /**< SPI0 signal: NPCS1 on PA31 mux A*/
#define MUX_PA31A_SPI0_NPCS1                       0L          
#define PINMUX_PA31A_SPI0_NPCS1                    ((PIN_PA31A_SPI0_NPCS1 << 16) | MUX_PA31A_SPI0_NPCS1)
#define PIO_PA31A_SPI0_NPCS1                       (1UL << 31) 

#define PIN_PD25B_SPI0_NPCS1                       121L         /**< SPI0 signal: NPCS1 on PD25 mux B*/
#define MUX_PD25B_SPI0_NPCS1                       1L          
#define PINMUX_PD25B_SPI0_NPCS1                    ((PIN_PD25B_SPI0_NPCS1 << 16) | MUX_PD25B_SPI0_NPCS1)
#define PIO_PD25B_SPI0_NPCS1                       (1UL << 25) 

#define PIN_PD12C_SPI0_NPCS2                       108L         /**< SPI0 signal: NPCS2 on PD12 mux C*/
#define MUX_PD12C_SPI0_NPCS2                       2L          
#define PINMUX_PD12C_SPI0_NPCS2                    ((PIN_PD12C_SPI0_NPCS2 << 16) | MUX_PD12C_SPI0_NPCS2)
#define PIO_PD12C_SPI0_NPCS2                       (1UL << 12) 

#define PIN_PD27B_SPI0_NPCS3                       123L         /**< SPI0 signal: NPCS3 on PD27 mux B*/
#define MUX_PD27B_SPI0_NPCS3                       1L          
#define PINMUX_PD27B_SPI0_NPCS3                    ((PIN_PD27B_SPI0_NPCS3 << 16) | MUX_PD27B_SPI0_NPCS3)
#define PIO_PD27B_SPI0_NPCS3                       (1UL << 27) 

#define PIN_PD22B_SPI0_SPCK                        118L         /**< SPI0 signal: SPCK on PD22 mux B*/
#define MUX_PD22B_SPI0_SPCK                        1L          
#define PINMUX_PD22B_SPI0_SPCK                     ((PIN_PD22B_SPI0_SPCK << 16) | MUX_PD22B_SPI0_SPCK)
#define PIO_PD22B_SPI0_SPCK                        (1UL << 22) 

/* ========== PIO definition for SPI1 peripheral ========== */
#define PIN_PD0C_SPI1_NPCS1                        96L          /**< SPI1 signal: NPCS1 on PD0 mux C*/
#define MUX_PD0C_SPI1_NPCS1                        2L          
#define PINMUX_PD0C_SPI1_NPCS1                     ((PIN_PD0C_SPI1_NPCS1 << 16) | MUX_PD0C_SPI1_NPCS1)
#define PIO_PD0C_SPI1_NPCS1                        (1UL << 0)  

#define PIN_PD1C_SPI1_NPCS2                        97L          /**< SPI1 signal: NPCS2 on PD1 mux C*/
#define MUX_PD1C_SPI1_NPCS2                        2L          
#define PINMUX_PD1C_SPI1_NPCS2                     ((PIN_PD1C_SPI1_NPCS2 << 16) | MUX_PD1C_SPI1_NPCS2)
#define PIO_PD1C_SPI1_NPCS2                        (1UL << 1)  

#define PIN_PD2C_SPI1_NPCS3                        98L          /**< SPI1 signal: NPCS3 on PD2 mux C*/
#define MUX_PD2C_SPI1_NPCS3                        2L          
#define PINMUX_PD2C_SPI1_NPCS3                     ((PIN_PD2C_SPI1_NPCS3 << 16) | MUX_PD2C_SPI1_NPCS3)
#define PIO_PD2C_SPI1_NPCS3                        (1UL << 2)  

/* ========== PIO definition for SSC peripheral ========== */
#define PIN_PA10C_SSC_RD                           10L          /**< SSC signal: RD on PA10 mux C*/
#define MUX_PA10C_SSC_RD                           2L          
#define PINMUX_PA10C_SSC_RD                        ((PIN_PA10C_SSC_RD << 16) | MUX_PA10C_SSC_RD)
#define PIO_PA10C_SSC_RD                           (1UL << 10) 

#define PIN_PD24B_SSC_RF                           120L         /**< SSC signal: RF on PD24 mux B*/
#define MUX_PD24B_SSC_RF                           1L          
#define PINMUX_PD24B_SSC_RF                        ((PIN_PD24B_SSC_RF << 16) | MUX_PD24B_SSC_RF)
#define PIO_PD24B_SSC_RF                           (1UL << 24) 

#define PIN_PA22A_SSC_RK                           22L          /**< SSC signal: RK on PA22 mux A*/
#define MUX_PA22A_SSC_RK                           0L          
#define PINMUX_PA22A_SSC_RK                        ((PIN_PA22A_SSC_RK << 16) | MUX_PA22A_SSC_RK)
#define PIO_PA22A_SSC_RK                           (1UL << 22) 

#define PIN_PB5D_SSC_TD                            37L          /**< SSC signal: TD on PB5 mux D*/
#define MUX_PB5D_SSC_TD                            3L          
#define PINMUX_PB5D_SSC_TD                         ((PIN_PB5D_SSC_TD << 16) | MUX_PB5D_SSC_TD)
#define PIO_PB5D_SSC_TD                            (1UL << 5)  

#define PIN_PD10C_SSC_TD                           106L         /**< SSC signal: TD on PD10 mux C*/
#define MUX_PD10C_SSC_TD                           2L          
#define PINMUX_PD10C_SSC_TD                        ((PIN_PD10C_SSC_TD << 16) | MUX_PD10C_SSC_TD)
#define PIO_PD10C_SSC_TD                           (1UL << 10) 

#define PIN_PD26B_SSC_TD                           122L         /**< SSC signal: TD on PD26 mux B*/
#define MUX_PD26B_SSC_TD                           1L          
#define PINMUX_PD26B_SSC_TD                        ((PIN_PD26B_SSC_TD << 16) | MUX_PD26B_SSC_TD)
#define PIO_PD26B_SSC_TD                           (1UL << 26) 

#define PIN_PB0D_SSC_TF                            32L          /**< SSC signal: TF on PB0 mux D*/
#define MUX_PB0D_SSC_TF                            3L          
#define PINMUX_PB0D_SSC_TF                         ((PIN_PB0D_SSC_TF << 16) | MUX_PB0D_SSC_TF)
#define PIO_PB0D_SSC_TF                            (1UL << 0)  

#define PIN_PB1D_SSC_TK                            33L          /**< SSC signal: TK on PB1 mux D*/
#define MUX_PB1D_SSC_TK                            3L          
#define PINMUX_PB1D_SSC_TK                         ((PIN_PB1D_SSC_TK << 16) | MUX_PB1D_SSC_TK)
#define PIO_PB1D_SSC_TK                            (1UL << 1)  

/* ========== PIO definition for TC0 peripheral ========== */
#define PIN_PA4B_TC0_TCLK0                         4L           /**< TC0 signal: TCLK0 on PA4 mux B*/
#define MUX_PA4B_TC0_TCLK0                         1L          
#define PINMUX_PA4B_TC0_TCLK0                      ((PIN_PA4B_TC0_TCLK0 << 16) | MUX_PA4B_TC0_TCLK0)
#define PIO_PA4B_TC0_TCLK0                         (1UL << 4)  

#define PIN_PA28B_TC0_TCLK1                        28L          /**< TC0 signal: TCLK1 on PA28 mux B*/
#define MUX_PA28B_TC0_TCLK1                        1L          
#define PINMUX_PA28B_TC0_TCLK1                     ((PIN_PA28B_TC0_TCLK1 << 16) | MUX_PA28B_TC0_TCLK1)
#define PIO_PA28B_TC0_TCLK1                        (1UL << 28) 

#define PIN_PA29B_TC0_TCLK2                        29L          /**< TC0 signal: TCLK2 on PA29 mux B*/
#define MUX_PA29B_TC0_TCLK2                        1L          
#define PINMUX_PA29B_TC0_TCLK2                     ((PIN_PA29B_TC0_TCLK2 << 16) | MUX_PA29B_TC0_TCLK2)
#define PIO_PA29B_TC0_TCLK2                        (1UL << 29) 

#define PIN_PA0B_TC0_TIOA0                         0L           /**< TC0 signal: TIOA0 on PA0 mux B*/
#define MUX_PA0B_TC0_TIOA0                         1L          
#define PINMUX_PA0B_TC0_TIOA0                      ((PIN_PA0B_TC0_TIOA0 << 16) | MUX_PA0B_TC0_TIOA0)
#define PIO_PA0B_TC0_TIOA0                         (1UL << 0)  

#define PIN_PA15B_TC0_TIOA1                        15L          /**< TC0 signal: TIOA1 on PA15 mux B*/
#define MUX_PA15B_TC0_TIOA1                        1L          
#define PINMUX_PA15B_TC0_TIOA1                     ((PIN_PA15B_TC0_TIOA1 << 16) | MUX_PA15B_TC0_TIOA1)
#define PIO_PA15B_TC0_TIOA1                        (1UL << 15) 

#define PIN_PA26B_TC0_TIOA2                        26L          /**< TC0 signal: TIOA2 on PA26 mux B*/
#define MUX_PA26B_TC0_TIOA2                        1L          
#define PINMUX_PA26B_TC0_TIOA2                     ((PIN_PA26B_TC0_TIOA2 << 16) | MUX_PA26B_TC0_TIOA2)
#define PIO_PA26B_TC0_TIOA2                        (1UL << 26) 

#define PIN_PA1B_TC0_TIOB0                         1L           /**< TC0 signal: TIOB0 on PA1 mux B*/
#define MUX_PA1B_TC0_TIOB0                         1L          
#define PINMUX_PA1B_TC0_TIOB0                      ((PIN_PA1B_TC0_TIOB0 << 16) | MUX_PA1B_TC0_TIOB0)
#define PIO_PA1B_TC0_TIOB0                         (1UL << 1)  

#define PIN_PA16B_TC0_TIOB1                        16L          /**< TC0 signal: TIOB1 on PA16 mux B*/
#define MUX_PA16B_TC0_TIOB1                        1L          
#define PINMUX_PA16B_TC0_TIOB1                     ((PIN_PA16B_TC0_TIOB1 << 16) | MUX_PA16B_TC0_TIOB1)
#define PIO_PA16B_TC0_TIOB1                        (1UL << 16) 

#define PIN_PA27B_TC0_TIOB2                        27L          /**< TC0 signal: TIOB2 on PA27 mux B*/
#define MUX_PA27B_TC0_TIOB2                        1L          
#define PINMUX_PA27B_TC0_TIOB2                     ((PIN_PA27B_TC0_TIOB2 << 16) | MUX_PA27B_TC0_TIOB2)
#define PIO_PA27B_TC0_TIOB2                        (1UL << 27) 

/* ========== PIO definition for TC3 peripheral ========== */
#define PIN_PD24C_TC3_TCLK11                       120L         /**< TC3 signal: TCLK11 on PD24 mux C*/
#define MUX_PD24C_TC3_TCLK11                       2L          
#define PINMUX_PD24C_TC3_TCLK11                    ((PIN_PD24C_TC3_TCLK11 << 16) | MUX_PD24C_TC3_TCLK11)
#define PIO_PD24C_TC3_TCLK11                       (1UL << 24) 

#define PIN_PD21C_TC3_TIOA11                       117L         /**< TC3 signal: TIOA11 on PD21 mux C*/
#define MUX_PD21C_TC3_TIOA11                       2L          
#define PINMUX_PD21C_TC3_TIOA11                    ((PIN_PD21C_TC3_TIOA11 << 16) | MUX_PD21C_TC3_TIOA11)
#define PIO_PD21C_TC3_TIOA11                       (1UL << 21) 

#define PIN_PD22C_TC3_TIOB11                       118L         /**< TC3 signal: TIOB11 on PD22 mux C*/
#define MUX_PD22C_TC3_TIOB11                       2L          
#define PINMUX_PD22C_TC3_TIOB11                    ((PIN_PD22C_TC3_TIOB11 << 16) | MUX_PD22C_TC3_TIOB11)
#define PIO_PD22C_TC3_TIOB11                       (1UL << 22) 

/* ========== PIO definition for TWIHS0 peripheral ========== */
#define PIN_PA4A_TWIHS0_TWCK0                      4L           /**< TWIHS0 signal: TWCK0 on PA4 mux A*/
#define MUX_PA4A_TWIHS0_TWCK0                      0L          
#define PINMUX_PA4A_TWIHS0_TWCK0                   ((PIN_PA4A_TWIHS0_TWCK0 << 16) | MUX_PA4A_TWIHS0_TWCK0)
#define PIO_PA4A_TWIHS0_TWCK0                      (1UL << 4)  

#define PIN_PA3A_TWIHS0_TWD0                       3L           /**< TWIHS0 signal: TWD0 on PA3 mux A*/
#define MUX_PA3A_TWIHS0_TWD0                       0L          
#define PINMUX_PA3A_TWIHS0_TWD0                    ((PIN_PA3A_TWIHS0_TWD0 << 16) | MUX_PA3A_TWIHS0_TWD0)
#define PIO_PA3A_TWIHS0_TWD0                       (1UL << 3)  

/* ========== PIO definition for TWIHS1 peripheral ========== */
#define PIN_PB5A_TWIHS1_TWCK1                      37L          /**< TWIHS1 signal: TWCK1 on PB5 mux A*/
#define MUX_PB5A_TWIHS1_TWCK1                      0L          
#define PINMUX_PB5A_TWIHS1_TWCK1                   ((PIN_PB5A_TWIHS1_TWCK1 << 16) | MUX_PB5A_TWIHS1_TWCK1)
#define PIO_PB5A_TWIHS1_TWCK1                      (1UL << 5)  

#define PIN_PB4A_TWIHS1_TWD1                       36L          /**< TWIHS1 signal: TWD1 on PB4 mux A*/
#define MUX_PB4A_TWIHS1_TWD1                       0L          
#define PINMUX_PB4A_TWIHS1_TWD1                    ((PIN_PB4A_TWIHS1_TWD1 << 16) | MUX_PB4A_TWIHS1_TWD1)
#define PIO_PB4A_TWIHS1_TWD1                       (1UL << 4)  

/* ========== PIO definition for TWIHS2 peripheral ========== */
#define PIN_PD28C_TWIHS2_TWCK2                     124L         /**< TWIHS2 signal: TWCK2 on PD28 mux C*/
#define MUX_PD28C_TWIHS2_TWCK2                     2L          
#define PINMUX_PD28C_TWIHS2_TWCK2                  ((PIN_PD28C_TWIHS2_TWCK2 << 16) | MUX_PD28C_TWIHS2_TWCK2)
#define PIO_PD28C_TWIHS2_TWCK2                     (1UL << 28) 

#define PIN_PD27C_TWIHS2_TWD2                      123L         /**< TWIHS2 signal: TWD2 on PD27 mux C*/
#define MUX_PD27C_TWIHS2_TWD2                      2L          
#define PINMUX_PD27C_TWIHS2_TWD2                   ((PIN_PD27C_TWIHS2_TWD2 << 16) | MUX_PD27C_TWIHS2_TWD2)
#define PIO_PD27C_TWIHS2_TWD2                      (1UL << 27) 

/* ========== PIO definition for UART0 peripheral ========== */
#define PIN_PA9A_UART0_URXD0                       9L           /**< UART0 signal: URXD0 on PA9 mux A*/
#define MUX_PA9A_UART0_URXD0                       0L          
#define PINMUX_PA9A_UART0_URXD0                    ((PIN_PA9A_UART0_URXD0 << 16) | MUX_PA9A_UART0_URXD0)
#define PIO_PA9A_UART0_URXD0                       (1UL << 9)  

#define PIN_PA10A_UART0_UTXD0                      10L          /**< UART0 signal: UTXD0 on PA10 mux A*/
#define MUX_PA10A_UART0_UTXD0                      0L          
#define PINMUX_PA10A_UART0_UTXD0                   ((PIN_PA10A_UART0_UTXD0 << 16) | MUX_PA10A_UART0_UTXD0)
#define PIO_PA10A_UART0_UTXD0                      (1UL << 10) 

/* ========== PIO definition for UART1 peripheral ========== */
#define PIN_PA5C_UART1_URXD1                       5L           /**< UART1 signal: URXD1 on PA5 mux C*/
#define MUX_PA5C_UART1_URXD1                       2L          
#define PINMUX_PA5C_UART1_URXD1                    ((PIN_PA5C_UART1_URXD1 << 16) | MUX_PA5C_UART1_URXD1)
#define PIO_PA5C_UART1_URXD1                       (1UL << 5)  

#define PIN_PA4C_UART1_UTXD1                       4L           /**< UART1 signal: UTXD1 on PA4 mux C*/
#define MUX_PA4C_UART1_UTXD1                       2L          
#define PINMUX_PA4C_UART1_UTXD1                    ((PIN_PA4C_UART1_UTXD1 << 16) | MUX_PA4C_UART1_UTXD1)
#define PIO_PA4C_UART1_UTXD1                       (1UL << 4)  

#define PIN_PA6C_UART1_UTXD1                       6L           /**< UART1 signal: UTXD1 on PA6 mux C*/
#define MUX_PA6C_UART1_UTXD1                       2L          
#define PINMUX_PA6C_UART1_UTXD1                    ((PIN_PA6C_UART1_UTXD1 << 16) | MUX_PA6C_UART1_UTXD1)
#define PIO_PA6C_UART1_UTXD1                       (1UL << 6)  

#define PIN_PD26D_UART1_UTXD1                      122L         /**< UART1 signal: UTXD1 on PD26 mux D*/
#define MUX_PD26D_UART1_UTXD1                      3L          
#define PINMUX_PD26D_UART1_UTXD1                   ((PIN_PD26D_UART1_UTXD1 << 16) | MUX_PD26D_UART1_UTXD1)
#define PIO_PD26D_UART1_UTXD1                      (1UL << 26) 

/* ========== PIO definition for UART2 peripheral ========== */
#define PIN_PD25C_UART2_URXD2                      121L         /**< UART2 signal: URXD2 on PD25 mux C*/
#define MUX_PD25C_UART2_URXD2                      2L          
#define PINMUX_PD25C_UART2_URXD2                   ((PIN_PD25C_UART2_URXD2 << 16) | MUX_PD25C_UART2_URXD2)
#define PIO_PD25C_UART2_URXD2                      (1UL << 25) 

#define PIN_PD26C_UART2_UTXD2                      122L         /**< UART2 signal: UTXD2 on PD26 mux C*/
#define MUX_PD26C_UART2_UTXD2                      2L          
#define PINMUX_PD26C_UART2_UTXD2                   ((PIN_PD26C_UART2_UTXD2 << 16) | MUX_PD26C_UART2_UTXD2)
#define PIO_PD26C_UART2_UTXD2                      (1UL << 26) 

/* ========== PIO definition for UART3 peripheral ========== */
#define PIN_PD28A_UART3_URXD3                      124L         /**< UART3 signal: URXD3 on PD28 mux A*/
#define MUX_PD28A_UART3_URXD3                      0L          
#define PINMUX_PD28A_UART3_URXD3                   ((PIN_PD28A_UART3_URXD3 << 16) | MUX_PD28A_UART3_URXD3)
#define PIO_PD28A_UART3_URXD3                      (1UL << 28) 

#define PIN_PD30A_UART3_UTXD3                      126L         /**< UART3 signal: UTXD3 on PD30 mux A*/
#define MUX_PD30A_UART3_UTXD3                      0L          
#define PINMUX_PD30A_UART3_UTXD3                   ((PIN_PD30A_UART3_UTXD3 << 16) | MUX_PD30A_UART3_UTXD3)
#define PIO_PD30A_UART3_UTXD3                      (1UL << 30) 

#define PIN_PD31B_UART3_UTXD3                      127L         /**< UART3 signal: UTXD3 on PD31 mux B*/
#define MUX_PD31B_UART3_UTXD3                      1L          
#define PINMUX_PD31B_UART3_UTXD3                   ((PIN_PD31B_UART3_UTXD3 << 16) | MUX_PD31B_UART3_UTXD3)
#define PIO_PD31B_UART3_UTXD3                      (1UL << 31) 

/* ========== PIO definition for UART4 peripheral ========== */
#define PIN_PD18C_UART4_URXD4                      114L         /**< UART4 signal: URXD4 on PD18 mux C*/
#define MUX_PD18C_UART4_URXD4                      2L          
#define PINMUX_PD18C_UART4_URXD4                   ((PIN_PD18C_UART4_URXD4 << 16) | MUX_PD18C_UART4_URXD4)
#define PIO_PD18C_UART4_URXD4                      (1UL << 18) 

#define PIN_PD3C_UART4_UTXD4                       99L          /**< UART4 signal: UTXD4 on PD3 mux C*/
#define MUX_PD3C_UART4_UTXD4                       2L          
#define PINMUX_PD3C_UART4_UTXD4                    ((PIN_PD3C_UART4_UTXD4 << 16) | MUX_PD3C_UART4_UTXD4)
#define PIO_PD3C_UART4_UTXD4                       (1UL << 3)  

#define PIN_PD19C_UART4_UTXD4                      115L         /**< UART4 signal: UTXD4 on PD19 mux C*/
#define MUX_PD19C_UART4_UTXD4                      2L          
#define PINMUX_PD19C_UART4_UTXD4                   ((PIN_PD19C_UART4_UTXD4 << 16) | MUX_PD19C_UART4_UTXD4)
#define PIO_PD19C_UART4_UTXD4                      (1UL << 19) 

/* ========== PIO definition for USART0 peripheral ========== */
#define PIN_PB2C_USART0_CTS0                       34L          /**< USART0 signal: CTS0 on PB2 mux C*/
#define MUX_PB2C_USART0_CTS0                       2L          
#define PINMUX_PB2C_USART0_CTS0                    ((PIN_PB2C_USART0_CTS0 << 16) | MUX_PB2C_USART0_CTS0)
#define PIO_PB2C_USART0_CTS0                       (1UL << 2)  

#define PIN_PD0D_USART0_DCD0                       96L          /**< USART0 signal: DCD0 on PD0 mux D*/
#define MUX_PD0D_USART0_DCD0                       3L          
#define PINMUX_PD0D_USART0_DCD0                    ((PIN_PD0D_USART0_DCD0 << 16) | MUX_PD0D_USART0_DCD0)
#define PIO_PD0D_USART0_DCD0                       (1UL << 0)  

#define PIN_PD2D_USART0_DSR0                       98L          /**< USART0 signal: DSR0 on PD2 mux D*/
#define MUX_PD2D_USART0_DSR0                       3L          
#define PINMUX_PD2D_USART0_DSR0                    ((PIN_PD2D_USART0_DSR0 << 16) | MUX_PD2D_USART0_DSR0)
#define PIO_PD2D_USART0_DSR0                       (1UL << 2)  

#define PIN_PD1D_USART0_DTR0                       97L          /**< USART0 signal: DTR0 on PD1 mux D*/
#define MUX_PD1D_USART0_DTR0                       3L          
#define PINMUX_PD1D_USART0_DTR0                    ((PIN_PD1D_USART0_DTR0 << 16) | MUX_PD1D_USART0_DTR0)
#define PIO_PD1D_USART0_DTR0                       (1UL << 1)  

#define PIN_PD3D_USART0_RI0                        99L          /**< USART0 signal: RI0 on PD3 mux D*/
#define MUX_PD3D_USART0_RI0                        3L          
#define PINMUX_PD3D_USART0_RI0                     ((PIN_PD3D_USART0_RI0 << 16) | MUX_PD3D_USART0_RI0)
#define PIO_PD3D_USART0_RI0                        (1UL << 3)  

#define PIN_PB3C_USART0_RTS0                       35L          /**< USART0 signal: RTS0 on PB3 mux C*/
#define MUX_PB3C_USART0_RTS0                       2L          
#define PINMUX_PB3C_USART0_RTS0                    ((PIN_PB3C_USART0_RTS0 << 16) | MUX_PB3C_USART0_RTS0)
#define PIO_PB3C_USART0_RTS0                       (1UL << 3)  

#define PIN_PB0C_USART0_RXD0                       32L          /**< USART0 signal: RXD0 on PB0 mux C*/
#define MUX_PB0C_USART0_RXD0                       2L          
#define PINMUX_PB0C_USART0_RXD0                    ((PIN_PB0C_USART0_RXD0 << 16) | MUX_PB0C_USART0_RXD0)
#define PIO_PB0C_USART0_RXD0                       (1UL << 0)  

#define PIN_PB13C_USART0_SCK0                      45L          /**< USART0 signal: SCK0 on PB13 mux C*/
#define MUX_PB13C_USART0_SCK0                      2L          
#define PINMUX_PB13C_USART0_SCK0                   ((PIN_PB13C_USART0_SCK0 << 16) | MUX_PB13C_USART0_SCK0)
#define PIO_PB13C_USART0_SCK0                      (1UL << 13) 

#define PIN_PB1C_USART0_TXD0                       33L          /**< USART0 signal: TXD0 on PB1 mux C*/
#define MUX_PB1C_USART0_TXD0                       2L          
#define PINMUX_PB1C_USART0_TXD0                    ((PIN_PB1C_USART0_TXD0 << 16) | MUX_PB1C_USART0_TXD0)
#define PIO_PB1C_USART0_TXD0                       (1UL << 1)  

/* ========== PIO definition for USART1 peripheral ========== */
#define PIN_PA25A_USART1_CTS1                      25L          /**< USART1 signal: CTS1 on PA25 mux A*/
#define MUX_PA25A_USART1_CTS1                      0L          
#define PINMUX_PA25A_USART1_CTS1                   ((PIN_PA25A_USART1_CTS1 << 16) | MUX_PA25A_USART1_CTS1)
#define PIO_PA25A_USART1_CTS1                      (1UL << 25) 

#define PIN_PA26A_USART1_DCD1                      26L          /**< USART1 signal: DCD1 on PA26 mux A*/
#define MUX_PA26A_USART1_DCD1                      0L          
#define PINMUX_PA26A_USART1_DCD1                   ((PIN_PA26A_USART1_DCD1 << 16) | MUX_PA26A_USART1_DCD1)
#define PIO_PA26A_USART1_DCD1                      (1UL << 26) 

#define PIN_PA28A_USART1_DSR1                      28L          /**< USART1 signal: DSR1 on PA28 mux A*/
#define MUX_PA28A_USART1_DSR1                      0L          
#define PINMUX_PA28A_USART1_DSR1                   ((PIN_PA28A_USART1_DSR1 << 16) | MUX_PA28A_USART1_DSR1)
#define PIO_PA28A_USART1_DSR1                      (1UL << 28) 

#define PIN_PA27A_USART1_DTR1                      27L          /**< USART1 signal: DTR1 on PA27 mux A*/
#define MUX_PA27A_USART1_DTR1                      0L          
#define PINMUX_PA27A_USART1_DTR1                   ((PIN_PA27A_USART1_DTR1 << 16) | MUX_PA27A_USART1_DTR1)
#define PIO_PA27A_USART1_DTR1                      (1UL << 27) 

#define PIN_PA3B_USART1_LONCOL1                    3L           /**< USART1 signal: LONCOL1 on PA3 mux B*/
#define MUX_PA3B_USART1_LONCOL1                    1L          
#define PINMUX_PA3B_USART1_LONCOL1                 ((PIN_PA3B_USART1_LONCOL1 << 16) | MUX_PA3B_USART1_LONCOL1)
#define PIO_PA3B_USART1_LONCOL1                    (1UL << 3)  

#define PIN_PA29A_USART1_RI1                       29L          /**< USART1 signal: RI1 on PA29 mux A*/
#define MUX_PA29A_USART1_RI1                       0L          
#define PINMUX_PA29A_USART1_RI1                    ((PIN_PA29A_USART1_RI1 << 16) | MUX_PA29A_USART1_RI1)
#define PIO_PA29A_USART1_RI1                       (1UL << 29) 

#define PIN_PA24A_USART1_RTS1                      24L          /**< USART1 signal: RTS1 on PA24 mux A*/
#define MUX_PA24A_USART1_RTS1                      0L          
#define PINMUX_PA24A_USART1_RTS1                   ((PIN_PA24A_USART1_RTS1 << 16) | MUX_PA24A_USART1_RTS1)
#define PIO_PA24A_USART1_RTS1                      (1UL << 24) 

#define PIN_PA21A_USART1_RXD1                      21L          /**< USART1 signal: RXD1 on PA21 mux A*/
#define MUX_PA21A_USART1_RXD1                      0L          
#define PINMUX_PA21A_USART1_RXD1                   ((PIN_PA21A_USART1_RXD1 << 16) | MUX_PA21A_USART1_RXD1)
#define PIO_PA21A_USART1_RXD1                      (1UL << 21) 

#define PIN_PA23A_USART1_SCK1                      23L          /**< USART1 signal: SCK1 on PA23 mux A*/
#define MUX_PA23A_USART1_SCK1                      0L          
#define PINMUX_PA23A_USART1_SCK1                   ((PIN_PA23A_USART1_SCK1 << 16) | MUX_PA23A_USART1_SCK1)
#define PIO_PA23A_USART1_SCK1                      (1UL << 23) 

#define PIN_PB4D_USART1_TXD1                       36L          /**< USART1 signal: TXD1 on PB4 mux D*/
#define MUX_PB4D_USART1_TXD1                       3L          
#define PINMUX_PB4D_USART1_TXD1                    ((PIN_PB4D_USART1_TXD1 << 16) | MUX_PB4D_USART1_TXD1)
#define PIO_PB4D_USART1_TXD1                       (1UL << 4)  

/* ========== PIO definition for USART2 peripheral ========== */
#define PIN_PD19B_USART2_CTS2                      115L         /**< USART2 signal: CTS2 on PD19 mux B*/
#define MUX_PD19B_USART2_CTS2                      1L          
#define PINMUX_PD19B_USART2_CTS2                   ((PIN_PD19B_USART2_CTS2 << 16) | MUX_PD19B_USART2_CTS2)
#define PIO_PD19B_USART2_CTS2                      (1UL << 19) 

#define PIN_PD4D_USART2_DCD2                       100L         /**< USART2 signal: DCD2 on PD4 mux D*/
#define MUX_PD4D_USART2_DCD2                       3L          
#define PINMUX_PD4D_USART2_DCD2                    ((PIN_PD4D_USART2_DCD2 << 16) | MUX_PD4D_USART2_DCD2)
#define PIO_PD4D_USART2_DCD2                       (1UL << 4)  

#define PIN_PD6D_USART2_DSR2                       102L         /**< USART2 signal: DSR2 on PD6 mux D*/
#define MUX_PD6D_USART2_DSR2                       3L          
#define PINMUX_PD6D_USART2_DSR2                    ((PIN_PD6D_USART2_DSR2 << 16) | MUX_PD6D_USART2_DSR2)
#define PIO_PD6D_USART2_DSR2                       (1UL << 6)  

#define PIN_PD5D_USART2_DTR2                       101L         /**< USART2 signal: DTR2 on PD5 mux D*/
#define MUX_PD5D_USART2_DTR2                       3L          
#define PINMUX_PD5D_USART2_DTR2                    ((PIN_PD5D_USART2_DTR2 << 16) | MUX_PD5D_USART2_DTR2)
#define PIO_PD5D_USART2_DTR2                       (1UL << 5)  

#define PIN_PD7D_USART2_RI2                        103L         /**< USART2 signal: RI2 on PD7 mux D*/
#define MUX_PD7D_USART2_RI2                        3L          
#define PINMUX_PD7D_USART2_RI2                     ((PIN_PD7D_USART2_RI2 << 16) | MUX_PD7D_USART2_RI2)
#define PIO_PD7D_USART2_RI2                        (1UL << 7)  

#define PIN_PD18B_USART2_RTS2                      114L         /**< USART2 signal: RTS2 on PD18 mux B*/
#define MUX_PD18B_USART2_RTS2                      1L          
#define PINMUX_PD18B_USART2_RTS2                   ((PIN_PD18B_USART2_RTS2 << 16) | MUX_PD18B_USART2_RTS2)
#define PIO_PD18B_USART2_RTS2                      (1UL << 18) 

#define PIN_PD15B_USART2_RXD2                      111L         /**< USART2 signal: RXD2 on PD15 mux B*/
#define MUX_PD15B_USART2_RXD2                      1L          
#define PINMUX_PD15B_USART2_RXD2                   ((PIN_PD15B_USART2_RXD2 << 16) | MUX_PD15B_USART2_RXD2)
#define PIO_PD15B_USART2_RXD2                      (1UL << 15) 

#define PIN_PD17B_USART2_SCK2                      113L         /**< USART2 signal: SCK2 on PD17 mux B*/
#define MUX_PD17B_USART2_SCK2                      1L          
#define PINMUX_PD17B_USART2_SCK2                   ((PIN_PD17B_USART2_SCK2 << 16) | MUX_PD17B_USART2_SCK2)
#define PIO_PD17B_USART2_SCK2                      (1UL << 17) 

#define PIN_PD16B_USART2_TXD2                      112L         /**< USART2 signal: TXD2 on PD16 mux B*/
#define MUX_PD16B_USART2_TXD2                      1L          
#define PINMUX_PD16B_USART2_TXD2                   ((PIN_PD16B_USART2_TXD2 << 16) | MUX_PD16B_USART2_TXD2)
#define PIO_PD16B_USART2_TXD2                      (1UL << 16) 

/* ========== PIO definition for EBI peripheral ========== */
#define PIN_PA18C_EBI_A14                          18L          /**< EBI signal: A14 on PA18 mux C*/
#define MUX_PA18C_EBI_A14                          2L          
#define PINMUX_PA18C_EBI_A14                       ((PIN_PA18C_EBI_A14 << 16) | MUX_PA18C_EBI_A14)
#define PIO_PA18C_EBI_A14                          (1UL << 18) 

#define PIN_PA19C_EBI_A15                          19L          /**< EBI signal: A15 on PA19 mux C*/
#define MUX_PA19C_EBI_A15                          2L          
#define PINMUX_PA19C_EBI_A15                       ((PIN_PA19C_EBI_A15 << 16) | MUX_PA19C_EBI_A15)
#define PIO_PA19C_EBI_A15                          (1UL << 19) 

#define PIN_PA20C_EBI_A16                          20L          /**< EBI signal: A16 on PA20 mux C*/
#define MUX_PA20C_EBI_A16                          2L          
#define PINMUX_PA20C_EBI_A16                       ((PIN_PA20C_EBI_A16 << 16) | MUX_PA20C_EBI_A16)
#define PIO_PA20C_EBI_A16                          (1UL << 20) 

#define PIN_PA20C_EBI_BA0                          20L          /**< EBI signal: BA0 on PA20 mux C*/
#define MUX_PA20C_EBI_BA0                          2L          
#define PINMUX_PA20C_EBI_BA0                       ((PIN_PA20C_EBI_BA0 << 16) | MUX_PA20C_EBI_BA0)
#define PIO_PA20C_EBI_BA0                          (1UL << 20) 

#define PIN_PA0C_EBI_A17                           0L           /**< EBI signal: A17 on PA0 mux C*/
#define MUX_PA0C_EBI_A17                           2L          
#define PINMUX_PA0C_EBI_A17                        ((PIN_PA0C_EBI_A17 << 16) | MUX_PA0C_EBI_A17)
#define PIO_PA0C_EBI_A17                           (1UL << 0)  

#define PIN_PA0C_EBI_BA1                           0L           /**< EBI signal: BA1 on PA0 mux C*/
#define MUX_PA0C_EBI_BA1                           2L          
#define PINMUX_PA0C_EBI_BA1                        ((PIN_PA0C_EBI_BA1 << 16) | MUX_PA0C_EBI_BA1)
#define PIO_PA0C_EBI_BA1                           (1UL << 0)  

#define PIN_PA1C_EBI_A18                           1L           /**< EBI signal: A18 on PA1 mux C*/
#define MUX_PA1C_EBI_A18                           2L          
#define PINMUX_PA1C_EBI_A18                        ((PIN_PA1C_EBI_A18 << 16) | MUX_PA1C_EBI_A18)
#define PIO_PA1C_EBI_A18                           (1UL << 1)  

#define PIN_PA23C_EBI_A19                          23L          /**< EBI signal: A19 on PA23 mux C*/
#define MUX_PA23C_EBI_A19                          2L          
#define PINMUX_PA23C_EBI_A19                       ((PIN_PA23C_EBI_A19 << 16) | MUX_PA23C_EBI_A19)
#define PIO_PA23C_EBI_A19                          (1UL << 23) 

#define PIN_PA24C_EBI_A20                          24L          /**< EBI signal: A20 on PA24 mux C*/
#define MUX_PA24C_EBI_A20                          2L          
#define PINMUX_PA24C_EBI_A20                       ((PIN_PA24C_EBI_A20 << 16) | MUX_PA24C_EBI_A20)
#define PIO_PA24C_EBI_A20                          (1UL << 24) 

#define PIN_PA25C_EBI_A23                          25L          /**< EBI signal: A23 on PA25 mux C*/
#define MUX_PA25C_EBI_A23                          2L          
#define PINMUX_PA25C_EBI_A23                       ((PIN_PA25C_EBI_A23 << 16) | MUX_PA25C_EBI_A23)
#define PIO_PA25C_EBI_A23                          (1UL << 25) 

#define PIN_PD17C_EBI_CAS                          113L         /**< EBI signal: CAS on PD17 mux C*/
#define MUX_PD17C_EBI_CAS                          2L          
#define PINMUX_PD17C_EBI_CAS                       ((PIN_PD17C_EBI_CAS << 16) | MUX_PD17C_EBI_CAS)
#define PIO_PD17C_EBI_CAS                          (1UL << 17) 

#define PIN_PA15A_EBI_D14                          15L          /**< EBI signal: D14 on PA15 mux A*/
#define MUX_PA15A_EBI_D14                          0L          
#define PINMUX_PA15A_EBI_D14                       ((PIN_PA15A_EBI_D14 << 16) | MUX_PA15A_EBI_D14)
#define PIO_PA15A_EBI_D14                          (1UL << 15) 

#define PIN_PA16A_EBI_D15                          16L          /**< EBI signal: D15 on PA16 mux A*/
#define MUX_PA16A_EBI_D15                          0L          
#define PINMUX_PA16A_EBI_D15                       ((PIN_PA16A_EBI_D15 << 16) | MUX_PA16A_EBI_D15)
#define PIO_PA16A_EBI_D15                          (1UL << 16) 

#define PIN_PD18A_EBI_NCS1                         114L         /**< EBI signal: NCS1 on PD18 mux A*/
#define MUX_PD18A_EBI_NCS1                         0L          
#define PINMUX_PD18A_EBI_NCS1                      ((PIN_PD18A_EBI_NCS1 << 16) | MUX_PD18A_EBI_NCS1)
#define PIO_PD18A_EBI_NCS1                         (1UL << 18) 

#define PIN_PD18A_EBI_SDCS                         114L         /**< EBI signal: SDCS on PD18 mux A*/
#define MUX_PD18A_EBI_SDCS                         0L          
#define PINMUX_PD18A_EBI_SDCS                      ((PIN_PD18A_EBI_SDCS << 16) | MUX_PD18A_EBI_SDCS)
#define PIO_PD18A_EBI_SDCS                         (1UL << 18) 

#define PIN_PA22C_EBI_NCS2                         22L          /**< EBI signal: NCS2 on PA22 mux C*/
#define MUX_PA22C_EBI_NCS2                         2L          
#define PINMUX_PA22C_EBI_NCS2                      ((PIN_PA22C_EBI_NCS2 << 16) | MUX_PA22C_EBI_NCS2)
#define PIO_PA22C_EBI_NCS2                         (1UL << 22) 

#define PIN_PD19A_EBI_NCS3                         115L         /**< EBI signal: NCS3 on PD19 mux A*/
#define MUX_PD19A_EBI_NCS3                         0L          
#define PINMUX_PD19A_EBI_NCS3                      ((PIN_PD19A_EBI_NCS3 << 16) | MUX_PD19A_EBI_NCS3)
#define PIO_PD19A_EBI_NCS3                         (1UL << 19) 

#define PIN_PD15C_EBI_NWR1                         111L         /**< EBI signal: NWR1 on PD15 mux C*/
#define MUX_PD15C_EBI_NWR1                         2L          
#define PINMUX_PD15C_EBI_NWR1                      ((PIN_PD15C_EBI_NWR1 << 16) | MUX_PD15C_EBI_NWR1)
#define PIO_PD15C_EBI_NWR1                         (1UL << 15) 

#define PIN_PD15C_EBI_NBS1                         111L         /**< EBI signal: NBS1 on PD15 mux C*/
#define MUX_PD15C_EBI_NBS1                         2L          
#define PINMUX_PD15C_EBI_NBS1                      ((PIN_PD15C_EBI_NBS1 << 16) | MUX_PD15C_EBI_NBS1)
#define PIO_PD15C_EBI_NBS1                         (1UL << 15) 

#define PIN_PD16C_EBI_RAS                          112L         /**< EBI signal: RAS on PD16 mux C*/
#define MUX_PD16C_EBI_RAS                          2L          
#define PINMUX_PD16C_EBI_RAS                       ((PIN_PD16C_EBI_RAS << 16) | MUX_PD16C_EBI_RAS)
#define PIO_PD16C_EBI_RAS                          (1UL << 16) 

#define PIN_PD13C_EBI_SDA10                        109L         /**< EBI signal: SDA10 on PD13 mux C*/
#define MUX_PD13C_EBI_SDA10                        2L          
#define PINMUX_PD13C_EBI_SDA10                     ((PIN_PD13C_EBI_SDA10 << 16) | MUX_PD13C_EBI_SDA10)
#define PIO_PD13C_EBI_SDA10                        (1UL << 13) 

#define PIN_PD23C_EBI_SDCK                         119L         /**< EBI signal: SDCK on PD23 mux C*/
#define MUX_PD23C_EBI_SDCK                         2L          
#define PINMUX_PD23C_EBI_SDCK                      ((PIN_PD23C_EBI_SDCK << 16) | MUX_PD23C_EBI_SDCK)
#define PIO_PD23C_EBI_SDCK                         (1UL << 23) 

#define PIN_PD14C_EBI_SDCKE                        110L         /**< EBI signal: SDCKE on PD14 mux C*/
#define MUX_PD14C_EBI_SDCKE                        2L          
#define PINMUX_PD14C_EBI_SDCKE                     ((PIN_PD14C_EBI_SDCKE << 16) | MUX_PD14C_EBI_SDCKE)
#define PIO_PD14C_EBI_SDCKE                        (1UL << 14) 

#define PIN_PD29C_EBI_SDWE                         125L         /**< EBI signal: SDWE on PD29 mux C*/
#define MUX_PD29C_EBI_SDWE                         2L          
#define PINMUX_PD29C_EBI_SDWE                      ((PIN_PD29C_EBI_SDWE << 16) | MUX_PD29C_EBI_SDWE)
#define PIO_PD29C_EBI_SDWE                         (1UL << 29) 


#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _SAME70N19_PIO_H_ */
