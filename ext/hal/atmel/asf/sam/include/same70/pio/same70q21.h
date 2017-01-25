/**
 * \file
 *
 * \brief Peripheral I/O description for SAME70Q21
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

#ifndef _SAME70Q21_PIO_H_
#define _SAME70Q21_PIO_H_

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
#define PIN_PC0                     ( 64)  /**< Pin Number for PC0 */
#define PIN_PC1                     ( 65)  /**< Pin Number for PC1 */
#define PIN_PC2                     ( 66)  /**< Pin Number for PC2 */
#define PIN_PC3                     ( 67)  /**< Pin Number for PC3 */
#define PIN_PC4                     ( 68)  /**< Pin Number for PC4 */
#define PIN_PC5                     ( 69)  /**< Pin Number for PC5 */
#define PIN_PC6                     ( 70)  /**< Pin Number for PC6 */
#define PIN_PC7                     ( 71)  /**< Pin Number for PC7 */
#define PIN_PC8                     ( 72)  /**< Pin Number for PC8 */
#define PIN_PC9                     ( 73)  /**< Pin Number for PC9 */
#define PIN_PC10                    ( 74)  /**< Pin Number for PC10 */
#define PIN_PC11                    ( 75)  /**< Pin Number for PC11 */
#define PIN_PC12                    ( 76)  /**< Pin Number for PC12 */
#define PIN_PC13                    ( 77)  /**< Pin Number for PC13 */
#define PIN_PC14                    ( 78)  /**< Pin Number for PC14 */
#define PIN_PC15                    ( 79)  /**< Pin Number for PC15 */
#define PIN_PC16                    ( 80)  /**< Pin Number for PC16 */
#define PIN_PC17                    ( 81)  /**< Pin Number for PC17 */
#define PIN_PC18                    ( 82)  /**< Pin Number for PC18 */
#define PIN_PC19                    ( 83)  /**< Pin Number for PC19 */
#define PIN_PC20                    ( 84)  /**< Pin Number for PC20 */
#define PIN_PC21                    ( 85)  /**< Pin Number for PC21 */
#define PIN_PC22                    ( 86)  /**< Pin Number for PC22 */
#define PIN_PC23                    ( 87)  /**< Pin Number for PC23 */
#define PIN_PC24                    ( 88)  /**< Pin Number for PC24 */
#define PIN_PC25                    ( 89)  /**< Pin Number for PC25 */
#define PIN_PC26                    ( 90)  /**< Pin Number for PC26 */
#define PIN_PC27                    ( 91)  /**< Pin Number for PC27 */
#define PIN_PC28                    ( 92)  /**< Pin Number for PC28 */
#define PIN_PC29                    ( 93)  /**< Pin Number for PC29 */
#define PIN_PC30                    ( 94)  /**< Pin Number for PC30 */
#define PIN_PC31                    ( 95)  /**< Pin Number for PC31 */
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
#define PIN_PE0                     (128)  /**< Pin Number for PE0 */
#define PIN_PE1                     (129)  /**< Pin Number for PE1 */
#define PIN_PE2                     (130)  /**< Pin Number for PE2 */
#define PIN_PE3                     (131)  /**< Pin Number for PE3 */
#define PIN_PE4                     (132)  /**< Pin Number for PE4 */
#define PIN_PE5                     (133)  /**< Pin Number for PE5 */


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
#define PIO_PC0                     (1U << 0)  /**< PIO Mask for PC0 */
#define PIO_PC1                     (1U << 1)  /**< PIO Mask for PC1 */
#define PIO_PC2                     (1U << 2)  /**< PIO Mask for PC2 */
#define PIO_PC3                     (1U << 3)  /**< PIO Mask for PC3 */
#define PIO_PC4                     (1U << 4)  /**< PIO Mask for PC4 */
#define PIO_PC5                     (1U << 5)  /**< PIO Mask for PC5 */
#define PIO_PC6                     (1U << 6)  /**< PIO Mask for PC6 */
#define PIO_PC7                     (1U << 7)  /**< PIO Mask for PC7 */
#define PIO_PC8                     (1U << 8)  /**< PIO Mask for PC8 */
#define PIO_PC9                     (1U << 9)  /**< PIO Mask for PC9 */
#define PIO_PC10                    (1U << 10) /**< PIO Mask for PC10 */
#define PIO_PC11                    (1U << 11) /**< PIO Mask for PC11 */
#define PIO_PC12                    (1U << 12) /**< PIO Mask for PC12 */
#define PIO_PC13                    (1U << 13) /**< PIO Mask for PC13 */
#define PIO_PC14                    (1U << 14) /**< PIO Mask for PC14 */
#define PIO_PC15                    (1U << 15) /**< PIO Mask for PC15 */
#define PIO_PC16                    (1U << 16) /**< PIO Mask for PC16 */
#define PIO_PC17                    (1U << 17) /**< PIO Mask for PC17 */
#define PIO_PC18                    (1U << 18) /**< PIO Mask for PC18 */
#define PIO_PC19                    (1U << 19) /**< PIO Mask for PC19 */
#define PIO_PC20                    (1U << 20) /**< PIO Mask for PC20 */
#define PIO_PC21                    (1U << 21) /**< PIO Mask for PC21 */
#define PIO_PC22                    (1U << 22) /**< PIO Mask for PC22 */
#define PIO_PC23                    (1U << 23) /**< PIO Mask for PC23 */
#define PIO_PC24                    (1U << 24) /**< PIO Mask for PC24 */
#define PIO_PC25                    (1U << 25) /**< PIO Mask for PC25 */
#define PIO_PC26                    (1U << 26) /**< PIO Mask for PC26 */
#define PIO_PC27                    (1U << 27) /**< PIO Mask for PC27 */
#define PIO_PC28                    (1U << 28) /**< PIO Mask for PC28 */
#define PIO_PC29                    (1U << 29) /**< PIO Mask for PC29 */
#define PIO_PC30                    (1U << 30) /**< PIO Mask for PC30 */
#define PIO_PC31                    (1U << 31) /**< PIO Mask for PC31 */
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
#define PIO_PE0                     (1U << 0)  /**< PIO Mask for PE0 */
#define PIO_PE1                     (1U << 1)  /**< PIO Mask for PE1 */
#define PIO_PE2                     (1U << 2)  /**< PIO Mask for PE2 */
#define PIO_PE3                     (1U << 3)  /**< PIO Mask for PE3 */
#define PIO_PE4                     (1U << 4)  /**< PIO Mask for PE4 */
#define PIO_PE5                     (1U << 5)  /**< PIO Mask for PE5 */
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
#define PIO_PC0_IDX                 ( 64)  /**< PIO Index Number for PC0 */
#define PIO_PC1_IDX                 ( 65)  /**< PIO Index Number for PC1 */
#define PIO_PC2_IDX                 ( 66)  /**< PIO Index Number for PC2 */
#define PIO_PC3_IDX                 ( 67)  /**< PIO Index Number for PC3 */
#define PIO_PC4_IDX                 ( 68)  /**< PIO Index Number for PC4 */
#define PIO_PC5_IDX                 ( 69)  /**< PIO Index Number for PC5 */
#define PIO_PC6_IDX                 ( 70)  /**< PIO Index Number for PC6 */
#define PIO_PC7_IDX                 ( 71)  /**< PIO Index Number for PC7 */
#define PIO_PC8_IDX                 ( 72)  /**< PIO Index Number for PC8 */
#define PIO_PC9_IDX                 ( 73)  /**< PIO Index Number for PC9 */
#define PIO_PC10_IDX                ( 74)  /**< PIO Index Number for PC10 */
#define PIO_PC11_IDX                ( 75)  /**< PIO Index Number for PC11 */
#define PIO_PC12_IDX                ( 76)  /**< PIO Index Number for PC12 */
#define PIO_PC13_IDX                ( 77)  /**< PIO Index Number for PC13 */
#define PIO_PC14_IDX                ( 78)  /**< PIO Index Number for PC14 */
#define PIO_PC15_IDX                ( 79)  /**< PIO Index Number for PC15 */
#define PIO_PC16_IDX                ( 80)  /**< PIO Index Number for PC16 */
#define PIO_PC17_IDX                ( 81)  /**< PIO Index Number for PC17 */
#define PIO_PC18_IDX                ( 82)  /**< PIO Index Number for PC18 */
#define PIO_PC19_IDX                ( 83)  /**< PIO Index Number for PC19 */
#define PIO_PC20_IDX                ( 84)  /**< PIO Index Number for PC20 */
#define PIO_PC21_IDX                ( 85)  /**< PIO Index Number for PC21 */
#define PIO_PC22_IDX                ( 86)  /**< PIO Index Number for PC22 */
#define PIO_PC23_IDX                ( 87)  /**< PIO Index Number for PC23 */
#define PIO_PC24_IDX                ( 88)  /**< PIO Index Number for PC24 */
#define PIO_PC25_IDX                ( 89)  /**< PIO Index Number for PC25 */
#define PIO_PC26_IDX                ( 90)  /**< PIO Index Number for PC26 */
#define PIO_PC27_IDX                ( 91)  /**< PIO Index Number for PC27 */
#define PIO_PC28_IDX                ( 92)  /**< PIO Index Number for PC28 */
#define PIO_PC29_IDX                ( 93)  /**< PIO Index Number for PC29 */
#define PIO_PC30_IDX                ( 94)  /**< PIO Index Number for PC30 */
#define PIO_PC31_IDX                ( 95)  /**< PIO Index Number for PC31 */
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
#define PIO_PE0_IDX                 (128)  /**< PIO Index Number for PE0 */
#define PIO_PE1_IDX                 (129)  /**< PIO Index Number for PE1 */
#define PIO_PE2_IDX                 (130)  /**< PIO Index Number for PE2 */
#define PIO_PE3_IDX                 (131)  /**< PIO Index Number for PE3 */
#define PIO_PE4_IDX                 (132)  /**< PIO Index Number for PE4 */
#define PIO_PE5_IDX                 (133)  /**< PIO Index Number for PE5 */

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
#define PIO_PB13X1_DACC_DAC0                       (1u << 13)   /**< DACC signal: DAC0 */
#define PIO_PD0X1_DACC_DAC1                        (1u << 0)    /**< DACC signal: DAC1 */

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
#define PIN_PC12C_MCAN1_CANRX1                     76L          /**< MCAN1 signal: CANRX1 on PC12 mux C*/
#define MUX_PC12C_MCAN1_CANRX1                     2L          
#define PINMUX_PC12C_MCAN1_CANRX1                  ((PIN_PC12C_MCAN1_CANRX1 << 16) | MUX_PC12C_MCAN1_CANRX1)
#define PIO_PC12C_MCAN1_CANRX1                     (1UL << 12) 

#define PIN_PD28B_MCAN1_CANRX1                     124L         /**< MCAN1 signal: CANRX1 on PD28 mux B*/
#define MUX_PD28B_MCAN1_CANRX1                     1L          
#define PINMUX_PD28B_MCAN1_CANRX1                  ((PIN_PD28B_MCAN1_CANRX1 << 16) | MUX_PD28B_MCAN1_CANRX1)
#define PIO_PD28B_MCAN1_CANRX1                     (1UL << 28) 

#define PIN_PC14C_MCAN1_CANTX1                     78L          /**< MCAN1 signal: CANTX1 on PC14 mux C*/
#define MUX_PC14C_MCAN1_CANTX1                     2L          
#define PINMUX_PC14C_MCAN1_CANTX1                  ((PIN_PC14C_MCAN1_CANTX1 << 16) | MUX_PC14C_MCAN1_CANTX1)
#define PIO_PC14C_MCAN1_CANTX1                     (1UL << 14) 

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

#define PIN_PC19B_PWM0_PWMH2                       83L          /**< PWM0 signal: PWMH2 on PC19 mux B*/
#define MUX_PC19B_PWM0_PWMH2                       1L          
#define PINMUX_PC19B_PWM0_PWMH2                    ((PIN_PC19B_PWM0_PWMH2 << 16) | MUX_PC19B_PWM0_PWMH2)
#define PIO_PC19B_PWM0_PWMH2                       (1UL << 19) 

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

#define PIN_PC13B_PWM0_PWMH3                       77L          /**< PWM0 signal: PWMH3 on PC13 mux B*/
#define MUX_PC13B_PWM0_PWMH3                       1L          
#define PINMUX_PC13B_PWM0_PWMH3                    ((PIN_PC13B_PWM0_PWMH3 << 16) | MUX_PC13B_PWM0_PWMH3)
#define PIO_PC13B_PWM0_PWMH3                       (1UL << 13) 

#define PIN_PC21B_PWM0_PWMH3                       85L          /**< PWM0 signal: PWMH3 on PC21 mux B*/
#define MUX_PC21B_PWM0_PWMH3                       1L          
#define PINMUX_PC21B_PWM0_PWMH3                    ((PIN_PC21B_PWM0_PWMH3 << 16) | MUX_PC21B_PWM0_PWMH3)
#define PIO_PC21B_PWM0_PWMH3                       (1UL << 21) 

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

#define PIN_PC0B_PWM0_PWML0                        64L          /**< PWM0 signal: PWML0 on PC0 mux B*/
#define MUX_PC0B_PWM0_PWML0                        1L          
#define PINMUX_PC0B_PWM0_PWML0                     ((PIN_PC0B_PWM0_PWML0 << 16) | MUX_PC0B_PWM0_PWML0)
#define PIO_PC0B_PWM0_PWML0                        (1UL << 0)  

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

#define PIN_PC1B_PWM0_PWML1                        65L          /**< PWM0 signal: PWML1 on PC1 mux B*/
#define MUX_PC1B_PWM0_PWML1                        1L          
#define PINMUX_PC1B_PWM0_PWML1                     ((PIN_PC1B_PWM0_PWML1 << 16) | MUX_PC1B_PWM0_PWML1)
#define PIO_PC1B_PWM0_PWML1                        (1UL << 1)  

#define PIN_PC18B_PWM0_PWML1                       82L          /**< PWM0 signal: PWML1 on PC18 mux B*/
#define MUX_PC18B_PWM0_PWML1                       1L          
#define PINMUX_PC18B_PWM0_PWML1                    ((PIN_PC18B_PWM0_PWML1 << 16) | MUX_PC18B_PWM0_PWML1)
#define PIO_PC18B_PWM0_PWML1                       (1UL << 18) 

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

#define PIN_PC2B_PWM0_PWML2                        66L          /**< PWM0 signal: PWML2 on PC2 mux B*/
#define MUX_PC2B_PWM0_PWML2                        1L          
#define PINMUX_PC2B_PWM0_PWML2                     ((PIN_PC2B_PWM0_PWML2 << 16) | MUX_PC2B_PWM0_PWML2)
#define PIO_PC2B_PWM0_PWML2                        (1UL << 2)  

#define PIN_PC20B_PWM0_PWML2                       84L          /**< PWM0 signal: PWML2 on PC20 mux B*/
#define MUX_PC20B_PWM0_PWML2                       1L          
#define PINMUX_PC20B_PWM0_PWML2                    ((PIN_PC20B_PWM0_PWML2 << 16) | MUX_PC20B_PWM0_PWML2)
#define PIO_PC20B_PWM0_PWML2                       (1UL << 20) 

#define PIN_PD26A_PWM0_PWML2                       122L         /**< PWM0 signal: PWML2 on PD26 mux A*/
#define MUX_PD26A_PWM0_PWML2                       0L          
#define PINMUX_PD26A_PWM0_PWML2                    ((PIN_PD26A_PWM0_PWML2 << 16) | MUX_PD26A_PWM0_PWML2)
#define PIO_PD26A_PWM0_PWML2                       (1UL << 26) 

#define PIN_PA15C_PWM0_PWML3                       15L          /**< PWM0 signal: PWML3 on PA15 mux C*/
#define MUX_PA15C_PWM0_PWML3                       2L          
#define PINMUX_PA15C_PWM0_PWML3                    ((PIN_PA15C_PWM0_PWML3 << 16) | MUX_PA15C_PWM0_PWML3)
#define PIO_PA15C_PWM0_PWML3                       (1UL << 15) 

#define PIN_PC3B_PWM0_PWML3                        67L          /**< PWM0 signal: PWML3 on PC3 mux B*/
#define MUX_PC3B_PWM0_PWML3                        1L          
#define PINMUX_PC3B_PWM0_PWML3                     ((PIN_PC3B_PWM0_PWML3 << 16) | MUX_PC3B_PWM0_PWML3)
#define PIO_PC3B_PWM0_PWML3                        (1UL << 3)  

#define PIN_PC15B_PWM0_PWML3                       79L          /**< PWM0 signal: PWML3 on PC15 mux B*/
#define MUX_PC15B_PWM0_PWML3                       1L          
#define PINMUX_PC15B_PWM0_PWML3                    ((PIN_PC15B_PWM0_PWML3 << 16) | MUX_PC15B_PWM0_PWML3)
#define PIO_PC15B_PWM0_PWML3                       (1UL << 15) 

#define PIN_PC22B_PWM0_PWML3                       86L          /**< PWM0 signal: PWML3 on PC22 mux B*/
#define MUX_PC22B_PWM0_PWML3                       1L          
#define PINMUX_PC22B_PWM0_PWML3                    ((PIN_PC22B_PWM0_PWML3 << 16) | MUX_PC22B_PWM0_PWML3)
#define PIO_PC22B_PWM0_PWML3                       (1UL << 22) 

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

/* ========== PIO definition for SDRAMC peripheral ========== */
#define PIN_PC18A_SDRAMC_A0                        82L          /**< SDRAMC signal: A0 on PC18 mux A*/
#define MUX_PC18A_SDRAMC_A0                        0L          
#define PINMUX_PC18A_SDRAMC_A0                     ((PIN_PC18A_SDRAMC_A0 << 16) | MUX_PC18A_SDRAMC_A0)
#define PIO_PC18A_SDRAMC_A0                        (1UL << 18) 

#define PIN_PC18A_SDRAMC_NBS0                      82L          /**< SDRAMC signal: NBS0 on PC18 mux A*/
#define MUX_PC18A_SDRAMC_NBS0                      0L          
#define PINMUX_PC18A_SDRAMC_NBS0                   ((PIN_PC18A_SDRAMC_NBS0 << 16) | MUX_PC18A_SDRAMC_NBS0)
#define PIO_PC18A_SDRAMC_NBS0                      (1UL << 18) 

#define PIN_PC19A_SDRAMC_A1                        83L          /**< SDRAMC signal: A1 on PC19 mux A*/
#define MUX_PC19A_SDRAMC_A1                        0L          
#define PINMUX_PC19A_SDRAMC_A1                     ((PIN_PC19A_SDRAMC_A1 << 16) | MUX_PC19A_SDRAMC_A1)
#define PIO_PC19A_SDRAMC_A1                        (1UL << 19) 

#define PIN_PC20A_SDRAMC_A2                        84L          /**< SDRAMC signal: A2 on PC20 mux A*/
#define MUX_PC20A_SDRAMC_A2                        0L          
#define PINMUX_PC20A_SDRAMC_A2                     ((PIN_PC20A_SDRAMC_A2 << 16) | MUX_PC20A_SDRAMC_A2)
#define PIO_PC20A_SDRAMC_A2                        (1UL << 20) 

#define PIN_PC21A_SDRAMC_A3                        85L          /**< SDRAMC signal: A3 on PC21 mux A*/
#define MUX_PC21A_SDRAMC_A3                        0L          
#define PINMUX_PC21A_SDRAMC_A3                     ((PIN_PC21A_SDRAMC_A3 << 16) | MUX_PC21A_SDRAMC_A3)
#define PIO_PC21A_SDRAMC_A3                        (1UL << 21) 

#define PIN_PC22A_SDRAMC_A4                        86L          /**< SDRAMC signal: A4 on PC22 mux A*/
#define MUX_PC22A_SDRAMC_A4                        0L          
#define PINMUX_PC22A_SDRAMC_A4                     ((PIN_PC22A_SDRAMC_A4 << 16) | MUX_PC22A_SDRAMC_A4)
#define PIO_PC22A_SDRAMC_A4                        (1UL << 22) 

#define PIN_PC23A_SDRAMC_A5                        87L          /**< SDRAMC signal: A5 on PC23 mux A*/
#define MUX_PC23A_SDRAMC_A5                        0L          
#define PINMUX_PC23A_SDRAMC_A5                     ((PIN_PC23A_SDRAMC_A5 << 16) | MUX_PC23A_SDRAMC_A5)
#define PIO_PC23A_SDRAMC_A5                        (1UL << 23) 

#define PIN_PC24A_SDRAMC_A6                        88L          /**< SDRAMC signal: A6 on PC24 mux A*/
#define MUX_PC24A_SDRAMC_A6                        0L          
#define PINMUX_PC24A_SDRAMC_A6                     ((PIN_PC24A_SDRAMC_A6 << 16) | MUX_PC24A_SDRAMC_A6)
#define PIO_PC24A_SDRAMC_A6                        (1UL << 24) 

#define PIN_PC25A_SDRAMC_A7                        89L          /**< SDRAMC signal: A7 on PC25 mux A*/
#define MUX_PC25A_SDRAMC_A7                        0L          
#define PINMUX_PC25A_SDRAMC_A7                     ((PIN_PC25A_SDRAMC_A7 << 16) | MUX_PC25A_SDRAMC_A7)
#define PIO_PC25A_SDRAMC_A7                        (1UL << 25) 

#define PIN_PC26A_SDRAMC_A8                        90L          /**< SDRAMC signal: A8 on PC26 mux A*/
#define MUX_PC26A_SDRAMC_A8                        0L          
#define PINMUX_PC26A_SDRAMC_A8                     ((PIN_PC26A_SDRAMC_A8 << 16) | MUX_PC26A_SDRAMC_A8)
#define PIO_PC26A_SDRAMC_A8                        (1UL << 26) 

#define PIN_PC27A_SDRAMC_A9                        91L          /**< SDRAMC signal: A9 on PC27 mux A*/
#define MUX_PC27A_SDRAMC_A9                        0L          
#define PINMUX_PC27A_SDRAMC_A9                     ((PIN_PC27A_SDRAMC_A9 << 16) | MUX_PC27A_SDRAMC_A9)
#define PIO_PC27A_SDRAMC_A9                        (1UL << 27) 

#define PIN_PC28A_SDRAMC_A10                       92L          /**< SDRAMC signal: A10 on PC28 mux A*/
#define MUX_PC28A_SDRAMC_A10                       0L          
#define PINMUX_PC28A_SDRAMC_A10                    ((PIN_PC28A_SDRAMC_A10 << 16) | MUX_PC28A_SDRAMC_A10)
#define PIO_PC28A_SDRAMC_A10                       (1UL << 28) 

#define PIN_PC29A_SDRAMC_A11                       93L          /**< SDRAMC signal: A11 on PC29 mux A*/
#define MUX_PC29A_SDRAMC_A11                       0L          
#define PINMUX_PC29A_SDRAMC_A11                    ((PIN_PC29A_SDRAMC_A11 << 16) | MUX_PC29A_SDRAMC_A11)
#define PIO_PC29A_SDRAMC_A11                       (1UL << 29) 

#define PIN_PC30A_SDRAMC_A12                       94L          /**< SDRAMC signal: A12 on PC30 mux A*/
#define MUX_PC30A_SDRAMC_A12                       0L          
#define PINMUX_PC30A_SDRAMC_A12                    ((PIN_PC30A_SDRAMC_A12 << 16) | MUX_PC30A_SDRAMC_A12)
#define PIO_PC30A_SDRAMC_A12                       (1UL << 30) 

#define PIN_PC31A_SDRAMC_A13                       95L          /**< SDRAMC signal: A13 on PC31 mux A*/
#define MUX_PC31A_SDRAMC_A13                       0L          
#define PINMUX_PC31A_SDRAMC_A13                    ((PIN_PC31A_SDRAMC_A13 << 16) | MUX_PC31A_SDRAMC_A13)
#define PIO_PC31A_SDRAMC_A13                       (1UL << 31) 

#define PIN_PA18C_SDRAMC_A14                       18L          /**< SDRAMC signal: A14 on PA18 mux C*/
#define MUX_PA18C_SDRAMC_A14                       2L          
#define PINMUX_PA18C_SDRAMC_A14                    ((PIN_PA18C_SDRAMC_A14 << 16) | MUX_PA18C_SDRAMC_A14)
#define PIO_PA18C_SDRAMC_A14                       (1UL << 18) 

#define PIN_PA19C_SDRAMC_A15                       19L          /**< SDRAMC signal: A15 on PA19 mux C*/
#define MUX_PA19C_SDRAMC_A15                       2L          
#define PINMUX_PA19C_SDRAMC_A15                    ((PIN_PA19C_SDRAMC_A15 << 16) | MUX_PA19C_SDRAMC_A15)
#define PIO_PA19C_SDRAMC_A15                       (1UL << 19) 

#define PIN_PA20C_SDRAMC_A16                       20L          /**< SDRAMC signal: A16 on PA20 mux C*/
#define MUX_PA20C_SDRAMC_A16                       2L          
#define PINMUX_PA20C_SDRAMC_A16                    ((PIN_PA20C_SDRAMC_A16 << 16) | MUX_PA20C_SDRAMC_A16)
#define PIO_PA20C_SDRAMC_A16                       (1UL << 20) 

#define PIN_PA20C_SDRAMC_BA0                       20L          /**< SDRAMC signal: BA0 on PA20 mux C*/
#define MUX_PA20C_SDRAMC_BA0                       2L          
#define PINMUX_PA20C_SDRAMC_BA0                    ((PIN_PA20C_SDRAMC_BA0 << 16) | MUX_PA20C_SDRAMC_BA0)
#define PIO_PA20C_SDRAMC_BA0                       (1UL << 20) 

#define PIN_PA0C_SDRAMC_A17                        0L           /**< SDRAMC signal: A17 on PA0 mux C*/
#define MUX_PA0C_SDRAMC_A17                        2L          
#define PINMUX_PA0C_SDRAMC_A17                     ((PIN_PA0C_SDRAMC_A17 << 16) | MUX_PA0C_SDRAMC_A17)
#define PIO_PA0C_SDRAMC_A17                        (1UL << 0)  

#define PIN_PA0C_SDRAMC_BA1                        0L           /**< SDRAMC signal: BA1 on PA0 mux C*/
#define MUX_PA0C_SDRAMC_BA1                        2L          
#define PINMUX_PA0C_SDRAMC_BA1                     ((PIN_PA0C_SDRAMC_BA1 << 16) | MUX_PA0C_SDRAMC_BA1)
#define PIO_PA0C_SDRAMC_BA1                        (1UL << 0)  

#define PIN_PA1C_SDRAMC_A18                        1L           /**< SDRAMC signal: A18 on PA1 mux C*/
#define MUX_PA1C_SDRAMC_A18                        2L          
#define PINMUX_PA1C_SDRAMC_A18                     ((PIN_PA1C_SDRAMC_A18 << 16) | MUX_PA1C_SDRAMC_A18)
#define PIO_PA1C_SDRAMC_A18                        (1UL << 1)  

#define PIN_PA23C_SDRAMC_A19                       23L          /**< SDRAMC signal: A19 on PA23 mux C*/
#define MUX_PA23C_SDRAMC_A19                       2L          
#define PINMUX_PA23C_SDRAMC_A19                    ((PIN_PA23C_SDRAMC_A19 << 16) | MUX_PA23C_SDRAMC_A19)
#define PIO_PA23C_SDRAMC_A19                       (1UL << 23) 

#define PIN_PA24C_SDRAMC_A20                       24L          /**< SDRAMC signal: A20 on PA24 mux C*/
#define MUX_PA24C_SDRAMC_A20                       2L          
#define PINMUX_PA24C_SDRAMC_A20                    ((PIN_PA24C_SDRAMC_A20 << 16) | MUX_PA24C_SDRAMC_A20)
#define PIO_PA24C_SDRAMC_A20                       (1UL << 24) 

#define PIN_PC16A_SDRAMC_A21                       80L          /**< SDRAMC signal: A21 on PC16 mux A*/
#define MUX_PC16A_SDRAMC_A21                       0L          
#define PINMUX_PC16A_SDRAMC_A21                    ((PIN_PC16A_SDRAMC_A21 << 16) | MUX_PC16A_SDRAMC_A21)
#define PIO_PC16A_SDRAMC_A21                       (1UL << 16) 

#define PIN_PC16A_SDRAMC_NANDALE                   80L          /**< SDRAMC signal: NANDALE on PC16 mux A*/
#define MUX_PC16A_SDRAMC_NANDALE                   0L          
#define PINMUX_PC16A_SDRAMC_NANDALE                ((PIN_PC16A_SDRAMC_NANDALE << 16) | MUX_PC16A_SDRAMC_NANDALE)
#define PIO_PC16A_SDRAMC_NANDALE                   (1UL << 16) 

#define PIN_PC17A_SDRAMC_A22                       81L          /**< SDRAMC signal: A22 on PC17 mux A*/
#define MUX_PC17A_SDRAMC_A22                       0L          
#define PINMUX_PC17A_SDRAMC_A22                    ((PIN_PC17A_SDRAMC_A22 << 16) | MUX_PC17A_SDRAMC_A22)
#define PIO_PC17A_SDRAMC_A22                       (1UL << 17) 

#define PIN_PC17A_SDRAMC_NANDCLE                   81L          /**< SDRAMC signal: NANDCLE on PC17 mux A*/
#define MUX_PC17A_SDRAMC_NANDCLE                   0L          
#define PINMUX_PC17A_SDRAMC_NANDCLE                ((PIN_PC17A_SDRAMC_NANDCLE << 16) | MUX_PC17A_SDRAMC_NANDCLE)
#define PIO_PC17A_SDRAMC_NANDCLE                   (1UL << 17) 

#define PIN_PA25C_SDRAMC_A23                       25L          /**< SDRAMC signal: A23 on PA25 mux C*/
#define MUX_PA25C_SDRAMC_A23                       2L          
#define PINMUX_PA25C_SDRAMC_A23                    ((PIN_PA25C_SDRAMC_A23 << 16) | MUX_PA25C_SDRAMC_A23)
#define PIO_PA25C_SDRAMC_A23                       (1UL << 25) 

#define PIN_PD17C_SDRAMC_CAS                       113L         /**< SDRAMC signal: CAS on PD17 mux C*/
#define MUX_PD17C_SDRAMC_CAS                       2L          
#define PINMUX_PD17C_SDRAMC_CAS                    ((PIN_PD17C_SDRAMC_CAS << 16) | MUX_PD17C_SDRAMC_CAS)
#define PIO_PD17C_SDRAMC_CAS                       (1UL << 17) 

#define PIN_PC0A_SDRAMC_D0                         64L          /**< SDRAMC signal: D0 on PC0 mux A*/
#define MUX_PC0A_SDRAMC_D0                         0L          
#define PINMUX_PC0A_SDRAMC_D0                      ((PIN_PC0A_SDRAMC_D0 << 16) | MUX_PC0A_SDRAMC_D0)
#define PIO_PC0A_SDRAMC_D0                         (1UL << 0)  

#define PIN_PC1A_SDRAMC_D1                         65L          /**< SDRAMC signal: D1 on PC1 mux A*/
#define MUX_PC1A_SDRAMC_D1                         0L          
#define PINMUX_PC1A_SDRAMC_D1                      ((PIN_PC1A_SDRAMC_D1 << 16) | MUX_PC1A_SDRAMC_D1)
#define PIO_PC1A_SDRAMC_D1                         (1UL << 1)  

#define PIN_PC2A_SDRAMC_D2                         66L          /**< SDRAMC signal: D2 on PC2 mux A*/
#define MUX_PC2A_SDRAMC_D2                         0L          
#define PINMUX_PC2A_SDRAMC_D2                      ((PIN_PC2A_SDRAMC_D2 << 16) | MUX_PC2A_SDRAMC_D2)
#define PIO_PC2A_SDRAMC_D2                         (1UL << 2)  

#define PIN_PC3A_SDRAMC_D3                         67L          /**< SDRAMC signal: D3 on PC3 mux A*/
#define MUX_PC3A_SDRAMC_D3                         0L          
#define PINMUX_PC3A_SDRAMC_D3                      ((PIN_PC3A_SDRAMC_D3 << 16) | MUX_PC3A_SDRAMC_D3)
#define PIO_PC3A_SDRAMC_D3                         (1UL << 3)  

#define PIN_PC4A_SDRAMC_D4                         68L          /**< SDRAMC signal: D4 on PC4 mux A*/
#define MUX_PC4A_SDRAMC_D4                         0L          
#define PINMUX_PC4A_SDRAMC_D4                      ((PIN_PC4A_SDRAMC_D4 << 16) | MUX_PC4A_SDRAMC_D4)
#define PIO_PC4A_SDRAMC_D4                         (1UL << 4)  

#define PIN_PC5A_SDRAMC_D5                         69L          /**< SDRAMC signal: D5 on PC5 mux A*/
#define MUX_PC5A_SDRAMC_D5                         0L          
#define PINMUX_PC5A_SDRAMC_D5                      ((PIN_PC5A_SDRAMC_D5 << 16) | MUX_PC5A_SDRAMC_D5)
#define PIO_PC5A_SDRAMC_D5                         (1UL << 5)  

#define PIN_PC6A_SDRAMC_D6                         70L          /**< SDRAMC signal: D6 on PC6 mux A*/
#define MUX_PC6A_SDRAMC_D6                         0L          
#define PINMUX_PC6A_SDRAMC_D6                      ((PIN_PC6A_SDRAMC_D6 << 16) | MUX_PC6A_SDRAMC_D6)
#define PIO_PC6A_SDRAMC_D6                         (1UL << 6)  

#define PIN_PC7A_SDRAMC_D7                         71L          /**< SDRAMC signal: D7 on PC7 mux A*/
#define MUX_PC7A_SDRAMC_D7                         0L          
#define PINMUX_PC7A_SDRAMC_D7                      ((PIN_PC7A_SDRAMC_D7 << 16) | MUX_PC7A_SDRAMC_D7)
#define PIO_PC7A_SDRAMC_D7                         (1UL << 7)  

#define PIN_PE0A_SDRAMC_D8                         128L         /**< SDRAMC signal: D8 on PE0 mux A*/
#define MUX_PE0A_SDRAMC_D8                         0L          
#define PINMUX_PE0A_SDRAMC_D8                      ((PIN_PE0A_SDRAMC_D8 << 16) | MUX_PE0A_SDRAMC_D8)
#define PIO_PE0A_SDRAMC_D8                         (1UL << 0)  

#define PIN_PE1A_SDRAMC_D9                         129L         /**< SDRAMC signal: D9 on PE1 mux A*/
#define MUX_PE1A_SDRAMC_D9                         0L          
#define PINMUX_PE1A_SDRAMC_D9                      ((PIN_PE1A_SDRAMC_D9 << 16) | MUX_PE1A_SDRAMC_D9)
#define PIO_PE1A_SDRAMC_D9                         (1UL << 1)  

#define PIN_PE2A_SDRAMC_D10                        130L         /**< SDRAMC signal: D10 on PE2 mux A*/
#define MUX_PE2A_SDRAMC_D10                        0L          
#define PINMUX_PE2A_SDRAMC_D10                     ((PIN_PE2A_SDRAMC_D10 << 16) | MUX_PE2A_SDRAMC_D10)
#define PIO_PE2A_SDRAMC_D10                        (1UL << 2)  

#define PIN_PE3A_SDRAMC_D11                        131L         /**< SDRAMC signal: D11 on PE3 mux A*/
#define MUX_PE3A_SDRAMC_D11                        0L          
#define PINMUX_PE3A_SDRAMC_D11                     ((PIN_PE3A_SDRAMC_D11 << 16) | MUX_PE3A_SDRAMC_D11)
#define PIO_PE3A_SDRAMC_D11                        (1UL << 3)  

#define PIN_PE4A_SDRAMC_D12                        132L         /**< SDRAMC signal: D12 on PE4 mux A*/
#define MUX_PE4A_SDRAMC_D12                        0L          
#define PINMUX_PE4A_SDRAMC_D12                     ((PIN_PE4A_SDRAMC_D12 << 16) | MUX_PE4A_SDRAMC_D12)
#define PIO_PE4A_SDRAMC_D12                        (1UL << 4)  

#define PIN_PE5A_SDRAMC_D13                        133L         /**< SDRAMC signal: D13 on PE5 mux A*/
#define MUX_PE5A_SDRAMC_D13                        0L          
#define PINMUX_PE5A_SDRAMC_D13                     ((PIN_PE5A_SDRAMC_D13 << 16) | MUX_PE5A_SDRAMC_D13)
#define PIO_PE5A_SDRAMC_D13                        (1UL << 5)  

#define PIN_PA15A_SDRAMC_D14                       15L          /**< SDRAMC signal: D14 on PA15 mux A*/
#define MUX_PA15A_SDRAMC_D14                       0L          
#define PINMUX_PA15A_SDRAMC_D14                    ((PIN_PA15A_SDRAMC_D14 << 16) | MUX_PA15A_SDRAMC_D14)
#define PIO_PA15A_SDRAMC_D14                       (1UL << 15) 

#define PIN_PA16A_SDRAMC_D15                       16L          /**< SDRAMC signal: D15 on PA16 mux A*/
#define MUX_PA16A_SDRAMC_D15                       0L          
#define PINMUX_PA16A_SDRAMC_D15                    ((PIN_PA16A_SDRAMC_D15 << 16) | MUX_PA16A_SDRAMC_D15)
#define PIO_PA16A_SDRAMC_D15                       (1UL << 16) 

#define PIN_PC9A_SDRAMC_NANDOE                     73L          /**< SDRAMC signal: NANDOE on PC9 mux A*/
#define MUX_PC9A_SDRAMC_NANDOE                     0L          
#define PINMUX_PC9A_SDRAMC_NANDOE                  ((PIN_PC9A_SDRAMC_NANDOE << 16) | MUX_PC9A_SDRAMC_NANDOE)
#define PIO_PC9A_SDRAMC_NANDOE                     (1UL << 9)  

#define PIN_PC10A_SDRAMC_NANDWE                    74L          /**< SDRAMC signal: NANDWE on PC10 mux A*/
#define MUX_PC10A_SDRAMC_NANDWE                    0L          
#define PINMUX_PC10A_SDRAMC_NANDWE                 ((PIN_PC10A_SDRAMC_NANDWE << 16) | MUX_PC10A_SDRAMC_NANDWE)
#define PIO_PC10A_SDRAMC_NANDWE                    (1UL << 10) 

#define PIN_PC14A_SDRAMC_NCS0                      78L          /**< SDRAMC signal: NCS0 on PC14 mux A*/
#define MUX_PC14A_SDRAMC_NCS0                      0L          
#define PINMUX_PC14A_SDRAMC_NCS0                   ((PIN_PC14A_SDRAMC_NCS0 << 16) | MUX_PC14A_SDRAMC_NCS0)
#define PIO_PC14A_SDRAMC_NCS0                      (1UL << 14) 

#define PIN_PC15A_SDRAMC_NCS1                      79L          /**< SDRAMC signal: NCS1 on PC15 mux A*/
#define MUX_PC15A_SDRAMC_NCS1                      0L          
#define PINMUX_PC15A_SDRAMC_NCS1                   ((PIN_PC15A_SDRAMC_NCS1 << 16) | MUX_PC15A_SDRAMC_NCS1)
#define PIO_PC15A_SDRAMC_NCS1                      (1UL << 15) 

#define PIN_PC15A_SDRAMC_SDCS                      79L          /**< SDRAMC signal: SDCS on PC15 mux A*/
#define MUX_PC15A_SDRAMC_SDCS                      0L          
#define PINMUX_PC15A_SDRAMC_SDCS                   ((PIN_PC15A_SDRAMC_SDCS << 16) | MUX_PC15A_SDRAMC_SDCS)
#define PIO_PC15A_SDRAMC_SDCS                      (1UL << 15) 

#define PIN_PD18A_SDRAMC_NCS1                      114L         /**< SDRAMC signal: NCS1 on PD18 mux A*/
#define MUX_PD18A_SDRAMC_NCS1                      0L          
#define PINMUX_PD18A_SDRAMC_NCS1                   ((PIN_PD18A_SDRAMC_NCS1 << 16) | MUX_PD18A_SDRAMC_NCS1)
#define PIO_PD18A_SDRAMC_NCS1                      (1UL << 18) 

#define PIN_PD18A_SDRAMC_SDCS                      114L         /**< SDRAMC signal: SDCS on PD18 mux A*/
#define MUX_PD18A_SDRAMC_SDCS                      0L          
#define PINMUX_PD18A_SDRAMC_SDCS                   ((PIN_PD18A_SDRAMC_SDCS << 16) | MUX_PD18A_SDRAMC_SDCS)
#define PIO_PD18A_SDRAMC_SDCS                      (1UL << 18) 

#define PIN_PA22C_SDRAMC_NCS2                      22L          /**< SDRAMC signal: NCS2 on PA22 mux C*/
#define MUX_PA22C_SDRAMC_NCS2                      2L          
#define PINMUX_PA22C_SDRAMC_NCS2                   ((PIN_PA22C_SDRAMC_NCS2 << 16) | MUX_PA22C_SDRAMC_NCS2)
#define PIO_PA22C_SDRAMC_NCS2                      (1UL << 22) 

#define PIN_PC12A_SDRAMC_NCS3                      76L          /**< SDRAMC signal: NCS3 on PC12 mux A*/
#define MUX_PC12A_SDRAMC_NCS3                      0L          
#define PINMUX_PC12A_SDRAMC_NCS3                   ((PIN_PC12A_SDRAMC_NCS3 << 16) | MUX_PC12A_SDRAMC_NCS3)
#define PIO_PC12A_SDRAMC_NCS3                      (1UL << 12) 

#define PIN_PD19A_SDRAMC_NCS3                      115L         /**< SDRAMC signal: NCS3 on PD19 mux A*/
#define MUX_PD19A_SDRAMC_NCS3                      0L          
#define PINMUX_PD19A_SDRAMC_NCS3                   ((PIN_PD19A_SDRAMC_NCS3 << 16) | MUX_PD19A_SDRAMC_NCS3)
#define PIO_PD19A_SDRAMC_NCS3                      (1UL << 19) 

#define PIN_PC11A_SDRAMC_NRD                       75L          /**< SDRAMC signal: NRD on PC11 mux A*/
#define MUX_PC11A_SDRAMC_NRD                       0L          
#define PINMUX_PC11A_SDRAMC_NRD                    ((PIN_PC11A_SDRAMC_NRD << 16) | MUX_PC11A_SDRAMC_NRD)
#define PIO_PC11A_SDRAMC_NRD                       (1UL << 11) 

#define PIN_PC13A_SDRAMC_NWAIT                     77L          /**< SDRAMC signal: NWAIT on PC13 mux A*/
#define MUX_PC13A_SDRAMC_NWAIT                     0L          
#define PINMUX_PC13A_SDRAMC_NWAIT                  ((PIN_PC13A_SDRAMC_NWAIT << 16) | MUX_PC13A_SDRAMC_NWAIT)
#define PIO_PC13A_SDRAMC_NWAIT                     (1UL << 13) 

#define PIN_PC8A_SDRAMC_NWR0                       72L          /**< SDRAMC signal: NWR0 on PC8 mux A*/
#define MUX_PC8A_SDRAMC_NWR0                       0L          
#define PINMUX_PC8A_SDRAMC_NWR0                    ((PIN_PC8A_SDRAMC_NWR0 << 16) | MUX_PC8A_SDRAMC_NWR0)
#define PIO_PC8A_SDRAMC_NWR0                       (1UL << 8)  

#define PIN_PC8A_SDRAMC_NWE                        72L          /**< SDRAMC signal: NWE on PC8 mux A*/
#define MUX_PC8A_SDRAMC_NWE                        0L          
#define PINMUX_PC8A_SDRAMC_NWE                     ((PIN_PC8A_SDRAMC_NWE << 16) | MUX_PC8A_SDRAMC_NWE)
#define PIO_PC8A_SDRAMC_NWE                        (1UL << 8)  

#define PIN_PD15C_SDRAMC_NWR1                      111L         /**< SDRAMC signal: NWR1 on PD15 mux C*/
#define MUX_PD15C_SDRAMC_NWR1                      2L          
#define PINMUX_PD15C_SDRAMC_NWR1                   ((PIN_PD15C_SDRAMC_NWR1 << 16) | MUX_PD15C_SDRAMC_NWR1)
#define PIO_PD15C_SDRAMC_NWR1                      (1UL << 15) 

#define PIN_PD15C_SDRAMC_NBS1                      111L         /**< SDRAMC signal: NBS1 on PD15 mux C*/
#define MUX_PD15C_SDRAMC_NBS1                      2L          
#define PINMUX_PD15C_SDRAMC_NBS1                   ((PIN_PD15C_SDRAMC_NBS1 << 16) | MUX_PD15C_SDRAMC_NBS1)
#define PIO_PD15C_SDRAMC_NBS1                      (1UL << 15) 

#define PIN_PD16C_SDRAMC_RAS                       112L         /**< SDRAMC signal: RAS on PD16 mux C*/
#define MUX_PD16C_SDRAMC_RAS                       2L          
#define PINMUX_PD16C_SDRAMC_RAS                    ((PIN_PD16C_SDRAMC_RAS << 16) | MUX_PD16C_SDRAMC_RAS)
#define PIO_PD16C_SDRAMC_RAS                       (1UL << 16) 

#define PIN_PC13C_SDRAMC_SDA10                     77L          /**< SDRAMC signal: SDA10 on PC13 mux C*/
#define MUX_PC13C_SDRAMC_SDA10                     2L          
#define PINMUX_PC13C_SDRAMC_SDA10                  ((PIN_PC13C_SDRAMC_SDA10 << 16) | MUX_PC13C_SDRAMC_SDA10)
#define PIO_PC13C_SDRAMC_SDA10                     (1UL << 13) 

#define PIN_PD13C_SDRAMC_SDA10                     109L         /**< SDRAMC signal: SDA10 on PD13 mux C*/
#define MUX_PD13C_SDRAMC_SDA10                     2L          
#define PINMUX_PD13C_SDRAMC_SDA10                  ((PIN_PD13C_SDRAMC_SDA10 << 16) | MUX_PD13C_SDRAMC_SDA10)
#define PIO_PD13C_SDRAMC_SDA10                     (1UL << 13) 

#define PIN_PD23C_SDRAMC_SDCK                      119L         /**< SDRAMC signal: SDCK on PD23 mux C*/
#define MUX_PD23C_SDRAMC_SDCK                      2L          
#define PINMUX_PD23C_SDRAMC_SDCK                   ((PIN_PD23C_SDRAMC_SDCK << 16) | MUX_PD23C_SDRAMC_SDCK)
#define PIO_PD23C_SDRAMC_SDCK                      (1UL << 23) 

#define PIN_PD14C_SDRAMC_SDCKE                     110L         /**< SDRAMC signal: SDCKE on PD14 mux C*/
#define MUX_PD14C_SDRAMC_SDCKE                     2L          
#define PINMUX_PD14C_SDRAMC_SDCKE                  ((PIN_PD14C_SDRAMC_SDCKE << 16) | MUX_PD14C_SDRAMC_SDCKE)
#define PIO_PD14C_SDRAMC_SDCKE                     (1UL << 14) 

#define PIN_PD29C_SDRAMC_SDWE                      125L         /**< SDRAMC signal: SDWE on PD29 mux C*/
#define MUX_PD29C_SDRAMC_SDWE                      2L          
#define PINMUX_PD29C_SDRAMC_SDWE                   ((PIN_PD29C_SDRAMC_SDWE << 16) | MUX_PD29C_SDRAMC_SDWE)
#define PIO_PD29C_SDRAMC_SDWE                      (1UL << 29) 

/* ========== PIO definition for SMC peripheral ========== */
#define PIN_PC18A_SMC_A0                           82L          /**< SMC signal: A0 on PC18 mux A*/
#define MUX_PC18A_SMC_A0                           0L          
#define PINMUX_PC18A_SMC_A0                        ((PIN_PC18A_SMC_A0 << 16) | MUX_PC18A_SMC_A0)
#define PIO_PC18A_SMC_A0                           (1UL << 18) 

#define PIN_PC18A_SMC_NBS0                         82L          /**< SMC signal: NBS0 on PC18 mux A*/
#define MUX_PC18A_SMC_NBS0                         0L          
#define PINMUX_PC18A_SMC_NBS0                      ((PIN_PC18A_SMC_NBS0 << 16) | MUX_PC18A_SMC_NBS0)
#define PIO_PC18A_SMC_NBS0                         (1UL << 18) 

#define PIN_PC19A_SMC_A1                           83L          /**< SMC signal: A1 on PC19 mux A*/
#define MUX_PC19A_SMC_A1                           0L          
#define PINMUX_PC19A_SMC_A1                        ((PIN_PC19A_SMC_A1 << 16) | MUX_PC19A_SMC_A1)
#define PIO_PC19A_SMC_A1                           (1UL << 19) 

#define PIN_PC20A_SMC_A2                           84L          /**< SMC signal: A2 on PC20 mux A*/
#define MUX_PC20A_SMC_A2                           0L          
#define PINMUX_PC20A_SMC_A2                        ((PIN_PC20A_SMC_A2 << 16) | MUX_PC20A_SMC_A2)
#define PIO_PC20A_SMC_A2                           (1UL << 20) 

#define PIN_PC21A_SMC_A3                           85L          /**< SMC signal: A3 on PC21 mux A*/
#define MUX_PC21A_SMC_A3                           0L          
#define PINMUX_PC21A_SMC_A3                        ((PIN_PC21A_SMC_A3 << 16) | MUX_PC21A_SMC_A3)
#define PIO_PC21A_SMC_A3                           (1UL << 21) 

#define PIN_PC22A_SMC_A4                           86L          /**< SMC signal: A4 on PC22 mux A*/
#define MUX_PC22A_SMC_A4                           0L          
#define PINMUX_PC22A_SMC_A4                        ((PIN_PC22A_SMC_A4 << 16) | MUX_PC22A_SMC_A4)
#define PIO_PC22A_SMC_A4                           (1UL << 22) 

#define PIN_PC23A_SMC_A5                           87L          /**< SMC signal: A5 on PC23 mux A*/
#define MUX_PC23A_SMC_A5                           0L          
#define PINMUX_PC23A_SMC_A5                        ((PIN_PC23A_SMC_A5 << 16) | MUX_PC23A_SMC_A5)
#define PIO_PC23A_SMC_A5                           (1UL << 23) 

#define PIN_PC24A_SMC_A6                           88L          /**< SMC signal: A6 on PC24 mux A*/
#define MUX_PC24A_SMC_A6                           0L          
#define PINMUX_PC24A_SMC_A6                        ((PIN_PC24A_SMC_A6 << 16) | MUX_PC24A_SMC_A6)
#define PIO_PC24A_SMC_A6                           (1UL << 24) 

#define PIN_PC25A_SMC_A7                           89L          /**< SMC signal: A7 on PC25 mux A*/
#define MUX_PC25A_SMC_A7                           0L          
#define PINMUX_PC25A_SMC_A7                        ((PIN_PC25A_SMC_A7 << 16) | MUX_PC25A_SMC_A7)
#define PIO_PC25A_SMC_A7                           (1UL << 25) 

#define PIN_PC26A_SMC_A8                           90L          /**< SMC signal: A8 on PC26 mux A*/
#define MUX_PC26A_SMC_A8                           0L          
#define PINMUX_PC26A_SMC_A8                        ((PIN_PC26A_SMC_A8 << 16) | MUX_PC26A_SMC_A8)
#define PIO_PC26A_SMC_A8                           (1UL << 26) 

#define PIN_PC27A_SMC_A9                           91L          /**< SMC signal: A9 on PC27 mux A*/
#define MUX_PC27A_SMC_A9                           0L          
#define PINMUX_PC27A_SMC_A9                        ((PIN_PC27A_SMC_A9 << 16) | MUX_PC27A_SMC_A9)
#define PIO_PC27A_SMC_A9                           (1UL << 27) 

#define PIN_PC28A_SMC_A10                          92L          /**< SMC signal: A10 on PC28 mux A*/
#define MUX_PC28A_SMC_A10                          0L          
#define PINMUX_PC28A_SMC_A10                       ((PIN_PC28A_SMC_A10 << 16) | MUX_PC28A_SMC_A10)
#define PIO_PC28A_SMC_A10                          (1UL << 28) 

#define PIN_PC29A_SMC_A11                          93L          /**< SMC signal: A11 on PC29 mux A*/
#define MUX_PC29A_SMC_A11                          0L          
#define PINMUX_PC29A_SMC_A11                       ((PIN_PC29A_SMC_A11 << 16) | MUX_PC29A_SMC_A11)
#define PIO_PC29A_SMC_A11                          (1UL << 29) 

#define PIN_PC30A_SMC_A12                          94L          /**< SMC signal: A12 on PC30 mux A*/
#define MUX_PC30A_SMC_A12                          0L          
#define PINMUX_PC30A_SMC_A12                       ((PIN_PC30A_SMC_A12 << 16) | MUX_PC30A_SMC_A12)
#define PIO_PC30A_SMC_A12                          (1UL << 30) 

#define PIN_PC31A_SMC_A13                          95L          /**< SMC signal: A13 on PC31 mux A*/
#define MUX_PC31A_SMC_A13                          0L          
#define PINMUX_PC31A_SMC_A13                       ((PIN_PC31A_SMC_A13 << 16) | MUX_PC31A_SMC_A13)
#define PIO_PC31A_SMC_A13                          (1UL << 31) 

#define PIN_PA18C_SMC_A14                          18L          /**< SMC signal: A14 on PA18 mux C*/
#define MUX_PA18C_SMC_A14                          2L          
#define PINMUX_PA18C_SMC_A14                       ((PIN_PA18C_SMC_A14 << 16) | MUX_PA18C_SMC_A14)
#define PIO_PA18C_SMC_A14                          (1UL << 18) 

#define PIN_PA19C_SMC_A15                          19L          /**< SMC signal: A15 on PA19 mux C*/
#define MUX_PA19C_SMC_A15                          2L          
#define PINMUX_PA19C_SMC_A15                       ((PIN_PA19C_SMC_A15 << 16) | MUX_PA19C_SMC_A15)
#define PIO_PA19C_SMC_A15                          (1UL << 19) 

#define PIN_PA20C_SMC_A16                          20L          /**< SMC signal: A16 on PA20 mux C*/
#define MUX_PA20C_SMC_A16                          2L          
#define PINMUX_PA20C_SMC_A16                       ((PIN_PA20C_SMC_A16 << 16) | MUX_PA20C_SMC_A16)
#define PIO_PA20C_SMC_A16                          (1UL << 20) 

#define PIN_PA20C_SMC_BA0                          20L          /**< SMC signal: BA0 on PA20 mux C*/
#define MUX_PA20C_SMC_BA0                          2L          
#define PINMUX_PA20C_SMC_BA0                       ((PIN_PA20C_SMC_BA0 << 16) | MUX_PA20C_SMC_BA0)
#define PIO_PA20C_SMC_BA0                          (1UL << 20) 

#define PIN_PA0C_SMC_A17                           0L           /**< SMC signal: A17 on PA0 mux C*/
#define MUX_PA0C_SMC_A17                           2L          
#define PINMUX_PA0C_SMC_A17                        ((PIN_PA0C_SMC_A17 << 16) | MUX_PA0C_SMC_A17)
#define PIO_PA0C_SMC_A17                           (1UL << 0)  

#define PIN_PA0C_SMC_BA1                           0L           /**< SMC signal: BA1 on PA0 mux C*/
#define MUX_PA0C_SMC_BA1                           2L          
#define PINMUX_PA0C_SMC_BA1                        ((PIN_PA0C_SMC_BA1 << 16) | MUX_PA0C_SMC_BA1)
#define PIO_PA0C_SMC_BA1                           (1UL << 0)  

#define PIN_PA1C_SMC_A18                           1L           /**< SMC signal: A18 on PA1 mux C*/
#define MUX_PA1C_SMC_A18                           2L          
#define PINMUX_PA1C_SMC_A18                        ((PIN_PA1C_SMC_A18 << 16) | MUX_PA1C_SMC_A18)
#define PIO_PA1C_SMC_A18                           (1UL << 1)  

#define PIN_PA23C_SMC_A19                          23L          /**< SMC signal: A19 on PA23 mux C*/
#define MUX_PA23C_SMC_A19                          2L          
#define PINMUX_PA23C_SMC_A19                       ((PIN_PA23C_SMC_A19 << 16) | MUX_PA23C_SMC_A19)
#define PIO_PA23C_SMC_A19                          (1UL << 23) 

#define PIN_PA24C_SMC_A20                          24L          /**< SMC signal: A20 on PA24 mux C*/
#define MUX_PA24C_SMC_A20                          2L          
#define PINMUX_PA24C_SMC_A20                       ((PIN_PA24C_SMC_A20 << 16) | MUX_PA24C_SMC_A20)
#define PIO_PA24C_SMC_A20                          (1UL << 24) 

#define PIN_PC16A_SMC_A21                          80L          /**< SMC signal: A21 on PC16 mux A*/
#define MUX_PC16A_SMC_A21                          0L          
#define PINMUX_PC16A_SMC_A21                       ((PIN_PC16A_SMC_A21 << 16) | MUX_PC16A_SMC_A21)
#define PIO_PC16A_SMC_A21                          (1UL << 16) 

#define PIN_PC16A_SMC_NANDALE                      80L          /**< SMC signal: NANDALE on PC16 mux A*/
#define MUX_PC16A_SMC_NANDALE                      0L          
#define PINMUX_PC16A_SMC_NANDALE                   ((PIN_PC16A_SMC_NANDALE << 16) | MUX_PC16A_SMC_NANDALE)
#define PIO_PC16A_SMC_NANDALE                      (1UL << 16) 

#define PIN_PC17A_SMC_A22                          81L          /**< SMC signal: A22 on PC17 mux A*/
#define MUX_PC17A_SMC_A22                          0L          
#define PINMUX_PC17A_SMC_A22                       ((PIN_PC17A_SMC_A22 << 16) | MUX_PC17A_SMC_A22)
#define PIO_PC17A_SMC_A22                          (1UL << 17) 

#define PIN_PC17A_SMC_NANDCLE                      81L          /**< SMC signal: NANDCLE on PC17 mux A*/
#define MUX_PC17A_SMC_NANDCLE                      0L          
#define PINMUX_PC17A_SMC_NANDCLE                   ((PIN_PC17A_SMC_NANDCLE << 16) | MUX_PC17A_SMC_NANDCLE)
#define PIO_PC17A_SMC_NANDCLE                      (1UL << 17) 

#define PIN_PA25C_SMC_A23                          25L          /**< SMC signal: A23 on PA25 mux C*/
#define MUX_PA25C_SMC_A23                          2L          
#define PINMUX_PA25C_SMC_A23                       ((PIN_PA25C_SMC_A23 << 16) | MUX_PA25C_SMC_A23)
#define PIO_PA25C_SMC_A23                          (1UL << 25) 

#define PIN_PD17C_SMC_CAS                          113L         /**< SMC signal: CAS on PD17 mux C*/
#define MUX_PD17C_SMC_CAS                          2L          
#define PINMUX_PD17C_SMC_CAS                       ((PIN_PD17C_SMC_CAS << 16) | MUX_PD17C_SMC_CAS)
#define PIO_PD17C_SMC_CAS                          (1UL << 17) 

#define PIN_PC0A_SMC_D0                            64L          /**< SMC signal: D0 on PC0 mux A*/
#define MUX_PC0A_SMC_D0                            0L          
#define PINMUX_PC0A_SMC_D0                         ((PIN_PC0A_SMC_D0 << 16) | MUX_PC0A_SMC_D0)
#define PIO_PC0A_SMC_D0                            (1UL << 0)  

#define PIN_PC1A_SMC_D1                            65L          /**< SMC signal: D1 on PC1 mux A*/
#define MUX_PC1A_SMC_D1                            0L          
#define PINMUX_PC1A_SMC_D1                         ((PIN_PC1A_SMC_D1 << 16) | MUX_PC1A_SMC_D1)
#define PIO_PC1A_SMC_D1                            (1UL << 1)  

#define PIN_PC2A_SMC_D2                            66L          /**< SMC signal: D2 on PC2 mux A*/
#define MUX_PC2A_SMC_D2                            0L          
#define PINMUX_PC2A_SMC_D2                         ((PIN_PC2A_SMC_D2 << 16) | MUX_PC2A_SMC_D2)
#define PIO_PC2A_SMC_D2                            (1UL << 2)  

#define PIN_PC3A_SMC_D3                            67L          /**< SMC signal: D3 on PC3 mux A*/
#define MUX_PC3A_SMC_D3                            0L          
#define PINMUX_PC3A_SMC_D3                         ((PIN_PC3A_SMC_D3 << 16) | MUX_PC3A_SMC_D3)
#define PIO_PC3A_SMC_D3                            (1UL << 3)  

#define PIN_PC4A_SMC_D4                            68L          /**< SMC signal: D4 on PC4 mux A*/
#define MUX_PC4A_SMC_D4                            0L          
#define PINMUX_PC4A_SMC_D4                         ((PIN_PC4A_SMC_D4 << 16) | MUX_PC4A_SMC_D4)
#define PIO_PC4A_SMC_D4                            (1UL << 4)  

#define PIN_PC5A_SMC_D5                            69L          /**< SMC signal: D5 on PC5 mux A*/
#define MUX_PC5A_SMC_D5                            0L          
#define PINMUX_PC5A_SMC_D5                         ((PIN_PC5A_SMC_D5 << 16) | MUX_PC5A_SMC_D5)
#define PIO_PC5A_SMC_D5                            (1UL << 5)  

#define PIN_PC6A_SMC_D6                            70L          /**< SMC signal: D6 on PC6 mux A*/
#define MUX_PC6A_SMC_D6                            0L          
#define PINMUX_PC6A_SMC_D6                         ((PIN_PC6A_SMC_D6 << 16) | MUX_PC6A_SMC_D6)
#define PIO_PC6A_SMC_D6                            (1UL << 6)  

#define PIN_PC7A_SMC_D7                            71L          /**< SMC signal: D7 on PC7 mux A*/
#define MUX_PC7A_SMC_D7                            0L          
#define PINMUX_PC7A_SMC_D7                         ((PIN_PC7A_SMC_D7 << 16) | MUX_PC7A_SMC_D7)
#define PIO_PC7A_SMC_D7                            (1UL << 7)  

#define PIN_PE0A_SMC_D8                            128L         /**< SMC signal: D8 on PE0 mux A*/
#define MUX_PE0A_SMC_D8                            0L          
#define PINMUX_PE0A_SMC_D8                         ((PIN_PE0A_SMC_D8 << 16) | MUX_PE0A_SMC_D8)
#define PIO_PE0A_SMC_D8                            (1UL << 0)  

#define PIN_PE1A_SMC_D9                            129L         /**< SMC signal: D9 on PE1 mux A*/
#define MUX_PE1A_SMC_D9                            0L          
#define PINMUX_PE1A_SMC_D9                         ((PIN_PE1A_SMC_D9 << 16) | MUX_PE1A_SMC_D9)
#define PIO_PE1A_SMC_D9                            (1UL << 1)  

#define PIN_PE2A_SMC_D10                           130L         /**< SMC signal: D10 on PE2 mux A*/
#define MUX_PE2A_SMC_D10                           0L          
#define PINMUX_PE2A_SMC_D10                        ((PIN_PE2A_SMC_D10 << 16) | MUX_PE2A_SMC_D10)
#define PIO_PE2A_SMC_D10                           (1UL << 2)  

#define PIN_PE3A_SMC_D11                           131L         /**< SMC signal: D11 on PE3 mux A*/
#define MUX_PE3A_SMC_D11                           0L          
#define PINMUX_PE3A_SMC_D11                        ((PIN_PE3A_SMC_D11 << 16) | MUX_PE3A_SMC_D11)
#define PIO_PE3A_SMC_D11                           (1UL << 3)  

#define PIN_PE4A_SMC_D12                           132L         /**< SMC signal: D12 on PE4 mux A*/
#define MUX_PE4A_SMC_D12                           0L          
#define PINMUX_PE4A_SMC_D12                        ((PIN_PE4A_SMC_D12 << 16) | MUX_PE4A_SMC_D12)
#define PIO_PE4A_SMC_D12                           (1UL << 4)  

#define PIN_PE5A_SMC_D13                           133L         /**< SMC signal: D13 on PE5 mux A*/
#define MUX_PE5A_SMC_D13                           0L          
#define PINMUX_PE5A_SMC_D13                        ((PIN_PE5A_SMC_D13 << 16) | MUX_PE5A_SMC_D13)
#define PIO_PE5A_SMC_D13                           (1UL << 5)  

#define PIN_PA15A_SMC_D14                          15L          /**< SMC signal: D14 on PA15 mux A*/
#define MUX_PA15A_SMC_D14                          0L          
#define PINMUX_PA15A_SMC_D14                       ((PIN_PA15A_SMC_D14 << 16) | MUX_PA15A_SMC_D14)
#define PIO_PA15A_SMC_D14                          (1UL << 15) 

#define PIN_PA16A_SMC_D15                          16L          /**< SMC signal: D15 on PA16 mux A*/
#define MUX_PA16A_SMC_D15                          0L          
#define PINMUX_PA16A_SMC_D15                       ((PIN_PA16A_SMC_D15 << 16) | MUX_PA16A_SMC_D15)
#define PIO_PA16A_SMC_D15                          (1UL << 16) 

#define PIN_PC9A_SMC_NANDOE                        73L          /**< SMC signal: NANDOE on PC9 mux A*/
#define MUX_PC9A_SMC_NANDOE                        0L          
#define PINMUX_PC9A_SMC_NANDOE                     ((PIN_PC9A_SMC_NANDOE << 16) | MUX_PC9A_SMC_NANDOE)
#define PIO_PC9A_SMC_NANDOE                        (1UL << 9)  

#define PIN_PC10A_SMC_NANDWE                       74L          /**< SMC signal: NANDWE on PC10 mux A*/
#define MUX_PC10A_SMC_NANDWE                       0L          
#define PINMUX_PC10A_SMC_NANDWE                    ((PIN_PC10A_SMC_NANDWE << 16) | MUX_PC10A_SMC_NANDWE)
#define PIO_PC10A_SMC_NANDWE                       (1UL << 10) 

#define PIN_PC14A_SMC_NCS0                         78L          /**< SMC signal: NCS0 on PC14 mux A*/
#define MUX_PC14A_SMC_NCS0                         0L          
#define PINMUX_PC14A_SMC_NCS0                      ((PIN_PC14A_SMC_NCS0 << 16) | MUX_PC14A_SMC_NCS0)
#define PIO_PC14A_SMC_NCS0                         (1UL << 14) 

#define PIN_PC15A_SMC_NCS1                         79L          /**< SMC signal: NCS1 on PC15 mux A*/
#define MUX_PC15A_SMC_NCS1                         0L          
#define PINMUX_PC15A_SMC_NCS1                      ((PIN_PC15A_SMC_NCS1 << 16) | MUX_PC15A_SMC_NCS1)
#define PIO_PC15A_SMC_NCS1                         (1UL << 15) 

#define PIN_PC15A_SMC_SDCS                         79L          /**< SMC signal: SDCS on PC15 mux A*/
#define MUX_PC15A_SMC_SDCS                         0L          
#define PINMUX_PC15A_SMC_SDCS                      ((PIN_PC15A_SMC_SDCS << 16) | MUX_PC15A_SMC_SDCS)
#define PIO_PC15A_SMC_SDCS                         (1UL << 15) 

#define PIN_PD18A_SMC_NCS1                         114L         /**< SMC signal: NCS1 on PD18 mux A*/
#define MUX_PD18A_SMC_NCS1                         0L          
#define PINMUX_PD18A_SMC_NCS1                      ((PIN_PD18A_SMC_NCS1 << 16) | MUX_PD18A_SMC_NCS1)
#define PIO_PD18A_SMC_NCS1                         (1UL << 18) 

#define PIN_PD18A_SMC_SDCS                         114L         /**< SMC signal: SDCS on PD18 mux A*/
#define MUX_PD18A_SMC_SDCS                         0L          
#define PINMUX_PD18A_SMC_SDCS                      ((PIN_PD18A_SMC_SDCS << 16) | MUX_PD18A_SMC_SDCS)
#define PIO_PD18A_SMC_SDCS                         (1UL << 18) 

#define PIN_PA22C_SMC_NCS2                         22L          /**< SMC signal: NCS2 on PA22 mux C*/
#define MUX_PA22C_SMC_NCS2                         2L          
#define PINMUX_PA22C_SMC_NCS2                      ((PIN_PA22C_SMC_NCS2 << 16) | MUX_PA22C_SMC_NCS2)
#define PIO_PA22C_SMC_NCS2                         (1UL << 22) 

#define PIN_PC12A_SMC_NCS3                         76L          /**< SMC signal: NCS3 on PC12 mux A*/
#define MUX_PC12A_SMC_NCS3                         0L          
#define PINMUX_PC12A_SMC_NCS3                      ((PIN_PC12A_SMC_NCS3 << 16) | MUX_PC12A_SMC_NCS3)
#define PIO_PC12A_SMC_NCS3                         (1UL << 12) 

#define PIN_PD19A_SMC_NCS3                         115L         /**< SMC signal: NCS3 on PD19 mux A*/
#define MUX_PD19A_SMC_NCS3                         0L          
#define PINMUX_PD19A_SMC_NCS3                      ((PIN_PD19A_SMC_NCS3 << 16) | MUX_PD19A_SMC_NCS3)
#define PIO_PD19A_SMC_NCS3                         (1UL << 19) 

#define PIN_PC11A_SMC_NRD                          75L          /**< SMC signal: NRD on PC11 mux A*/
#define MUX_PC11A_SMC_NRD                          0L          
#define PINMUX_PC11A_SMC_NRD                       ((PIN_PC11A_SMC_NRD << 16) | MUX_PC11A_SMC_NRD)
#define PIO_PC11A_SMC_NRD                          (1UL << 11) 

#define PIN_PC13A_SMC_NWAIT                        77L          /**< SMC signal: NWAIT on PC13 mux A*/
#define MUX_PC13A_SMC_NWAIT                        0L          
#define PINMUX_PC13A_SMC_NWAIT                     ((PIN_PC13A_SMC_NWAIT << 16) | MUX_PC13A_SMC_NWAIT)
#define PIO_PC13A_SMC_NWAIT                        (1UL << 13) 

#define PIN_PC8A_SMC_NWR0                          72L          /**< SMC signal: NWR0 on PC8 mux A*/
#define MUX_PC8A_SMC_NWR0                          0L          
#define PINMUX_PC8A_SMC_NWR0                       ((PIN_PC8A_SMC_NWR0 << 16) | MUX_PC8A_SMC_NWR0)
#define PIO_PC8A_SMC_NWR0                          (1UL << 8)  

#define PIN_PC8A_SMC_NWE                           72L          /**< SMC signal: NWE on PC8 mux A*/
#define MUX_PC8A_SMC_NWE                           0L          
#define PINMUX_PC8A_SMC_NWE                        ((PIN_PC8A_SMC_NWE << 16) | MUX_PC8A_SMC_NWE)
#define PIO_PC8A_SMC_NWE                           (1UL << 8)  

#define PIN_PD15C_SMC_NWR1                         111L         /**< SMC signal: NWR1 on PD15 mux C*/
#define MUX_PD15C_SMC_NWR1                         2L          
#define PINMUX_PD15C_SMC_NWR1                      ((PIN_PD15C_SMC_NWR1 << 16) | MUX_PD15C_SMC_NWR1)
#define PIO_PD15C_SMC_NWR1                         (1UL << 15) 

#define PIN_PD15C_SMC_NBS1                         111L         /**< SMC signal: NBS1 on PD15 mux C*/
#define MUX_PD15C_SMC_NBS1                         2L          
#define PINMUX_PD15C_SMC_NBS1                      ((PIN_PD15C_SMC_NBS1 << 16) | MUX_PD15C_SMC_NBS1)
#define PIO_PD15C_SMC_NBS1                         (1UL << 15) 

#define PIN_PD16C_SMC_RAS                          112L         /**< SMC signal: RAS on PD16 mux C*/
#define MUX_PD16C_SMC_RAS                          2L          
#define PINMUX_PD16C_SMC_RAS                       ((PIN_PD16C_SMC_RAS << 16) | MUX_PD16C_SMC_RAS)
#define PIO_PD16C_SMC_RAS                          (1UL << 16) 

#define PIN_PC13C_SMC_SDA10                        77L          /**< SMC signal: SDA10 on PC13 mux C*/
#define MUX_PC13C_SMC_SDA10                        2L          
#define PINMUX_PC13C_SMC_SDA10                     ((PIN_PC13C_SMC_SDA10 << 16) | MUX_PC13C_SMC_SDA10)
#define PIO_PC13C_SMC_SDA10                        (1UL << 13) 

#define PIN_PD13C_SMC_SDA10                        109L         /**< SMC signal: SDA10 on PD13 mux C*/
#define MUX_PD13C_SMC_SDA10                        2L          
#define PINMUX_PD13C_SMC_SDA10                     ((PIN_PD13C_SMC_SDA10 << 16) | MUX_PD13C_SMC_SDA10)
#define PIO_PD13C_SMC_SDA10                        (1UL << 13) 

#define PIN_PD23C_SMC_SDCK                         119L         /**< SMC signal: SDCK on PD23 mux C*/
#define MUX_PD23C_SMC_SDCK                         2L          
#define PINMUX_PD23C_SMC_SDCK                      ((PIN_PD23C_SMC_SDCK << 16) | MUX_PD23C_SMC_SDCK)
#define PIO_PD23C_SMC_SDCK                         (1UL << 23) 

#define PIN_PD14C_SMC_SDCKE                        110L         /**< SMC signal: SDCKE on PD14 mux C*/
#define MUX_PD14C_SMC_SDCKE                        2L          
#define PINMUX_PD14C_SMC_SDCKE                     ((PIN_PD14C_SMC_SDCKE << 16) | MUX_PD14C_SMC_SDCKE)
#define PIO_PD14C_SMC_SDCKE                        (1UL << 14) 

#define PIN_PD29C_SMC_SDWE                         125L         /**< SMC signal: SDWE on PD29 mux C*/
#define MUX_PD29C_SMC_SDWE                         2L          
#define PINMUX_PD29C_SMC_SDWE                      ((PIN_PD29C_SMC_SDWE << 16) | MUX_PD29C_SMC_SDWE)
#define PIO_PD29C_SMC_SDWE                         (1UL << 29) 

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
#define PIN_PC26C_SPI1_MISO                        90L          /**< SPI1 signal: MISO on PC26 mux C*/
#define MUX_PC26C_SPI1_MISO                        2L          
#define PINMUX_PC26C_SPI1_MISO                     ((PIN_PC26C_SPI1_MISO << 16) | MUX_PC26C_SPI1_MISO)
#define PIO_PC26C_SPI1_MISO                        (1UL << 26) 

#define PIN_PC27C_SPI1_MOSI                        91L          /**< SPI1 signal: MOSI on PC27 mux C*/
#define MUX_PC27C_SPI1_MOSI                        2L          
#define PINMUX_PC27C_SPI1_MOSI                     ((PIN_PC27C_SPI1_MOSI << 16) | MUX_PC27C_SPI1_MOSI)
#define PIO_PC27C_SPI1_MOSI                        (1UL << 27) 

#define PIN_PC25C_SPI1_NPCS0                       89L          /**< SPI1 signal: NPCS0 on PC25 mux C*/
#define MUX_PC25C_SPI1_NPCS0                       2L          
#define PINMUX_PC25C_SPI1_NPCS0                    ((PIN_PC25C_SPI1_NPCS0 << 16) | MUX_PC25C_SPI1_NPCS0)
#define PIO_PC25C_SPI1_NPCS0                       (1UL << 25) 

#define PIN_PC28C_SPI1_NPCS1                       92L          /**< SPI1 signal: NPCS1 on PC28 mux C*/
#define MUX_PC28C_SPI1_NPCS1                       2L          
#define PINMUX_PC28C_SPI1_NPCS1                    ((PIN_PC28C_SPI1_NPCS1 << 16) | MUX_PC28C_SPI1_NPCS1)
#define PIO_PC28C_SPI1_NPCS1                       (1UL << 28) 

#define PIN_PD0C_SPI1_NPCS1                        96L          /**< SPI1 signal: NPCS1 on PD0 mux C*/
#define MUX_PD0C_SPI1_NPCS1                        2L          
#define PINMUX_PD0C_SPI1_NPCS1                     ((PIN_PD0C_SPI1_NPCS1 << 16) | MUX_PD0C_SPI1_NPCS1)
#define PIO_PD0C_SPI1_NPCS1                        (1UL << 0)  

#define PIN_PC29C_SPI1_NPCS2                       93L          /**< SPI1 signal: NPCS2 on PC29 mux C*/
#define MUX_PC29C_SPI1_NPCS2                       2L          
#define PINMUX_PC29C_SPI1_NPCS2                    ((PIN_PC29C_SPI1_NPCS2 << 16) | MUX_PC29C_SPI1_NPCS2)
#define PIO_PC29C_SPI1_NPCS2                       (1UL << 29) 

#define PIN_PD1C_SPI1_NPCS2                        97L          /**< SPI1 signal: NPCS2 on PD1 mux C*/
#define MUX_PD1C_SPI1_NPCS2                        2L          
#define PINMUX_PD1C_SPI1_NPCS2                     ((PIN_PD1C_SPI1_NPCS2 << 16) | MUX_PD1C_SPI1_NPCS2)
#define PIO_PD1C_SPI1_NPCS2                        (1UL << 1)  

#define PIN_PC30C_SPI1_NPCS3                       94L          /**< SPI1 signal: NPCS3 on PC30 mux C*/
#define MUX_PC30C_SPI1_NPCS3                       2L          
#define PINMUX_PC30C_SPI1_NPCS3                    ((PIN_PC30C_SPI1_NPCS3 << 16) | MUX_PC30C_SPI1_NPCS3)
#define PIO_PC30C_SPI1_NPCS3                       (1UL << 30) 

#define PIN_PD2C_SPI1_NPCS3                        98L          /**< SPI1 signal: NPCS3 on PD2 mux C*/
#define MUX_PD2C_SPI1_NPCS3                        2L          
#define PINMUX_PD2C_SPI1_NPCS3                     ((PIN_PD2C_SPI1_NPCS3 << 16) | MUX_PD2C_SPI1_NPCS3)
#define PIO_PD2C_SPI1_NPCS3                        (1UL << 2)  

#define PIN_PC24C_SPI1_SPCK                        88L          /**< SPI1 signal: SPCK on PC24 mux C*/
#define MUX_PC24C_SPI1_SPCK                        2L          
#define PINMUX_PC24C_SPI1_SPCK                     ((PIN_PC24C_SPI1_SPCK << 16) | MUX_PC24C_SPI1_SPCK)
#define PIO_PC24C_SPI1_SPCK                        (1UL << 24) 

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

/* ========== PIO definition for TC1 peripheral ========== */
#define PIN_PC25B_TC1_TCLK3                        89L          /**< TC1 signal: TCLK3 on PC25 mux B*/
#define MUX_PC25B_TC1_TCLK3                        1L          
#define PINMUX_PC25B_TC1_TCLK3                     ((PIN_PC25B_TC1_TCLK3 << 16) | MUX_PC25B_TC1_TCLK3)
#define PIO_PC25B_TC1_TCLK3                        (1UL << 25) 

#define PIN_PC28B_TC1_TCLK4                        92L          /**< TC1 signal: TCLK4 on PC28 mux B*/
#define MUX_PC28B_TC1_TCLK4                        1L          
#define PINMUX_PC28B_TC1_TCLK4                     ((PIN_PC28B_TC1_TCLK4 << 16) | MUX_PC28B_TC1_TCLK4)
#define PIO_PC28B_TC1_TCLK4                        (1UL << 28) 

#define PIN_PC31B_TC1_TCLK5                        95L          /**< TC1 signal: TCLK5 on PC31 mux B*/
#define MUX_PC31B_TC1_TCLK5                        1L          
#define PINMUX_PC31B_TC1_TCLK5                     ((PIN_PC31B_TC1_TCLK5 << 16) | MUX_PC31B_TC1_TCLK5)
#define PIO_PC31B_TC1_TCLK5                        (1UL << 31) 

#define PIN_PC23B_TC1_TIOA3                        87L          /**< TC1 signal: TIOA3 on PC23 mux B*/
#define MUX_PC23B_TC1_TIOA3                        1L          
#define PINMUX_PC23B_TC1_TIOA3                     ((PIN_PC23B_TC1_TIOA3 << 16) | MUX_PC23B_TC1_TIOA3)
#define PIO_PC23B_TC1_TIOA3                        (1UL << 23) 

#define PIN_PC26B_TC1_TIOA4                        90L          /**< TC1 signal: TIOA4 on PC26 mux B*/
#define MUX_PC26B_TC1_TIOA4                        1L          
#define PINMUX_PC26B_TC1_TIOA4                     ((PIN_PC26B_TC1_TIOA4 << 16) | MUX_PC26B_TC1_TIOA4)
#define PIO_PC26B_TC1_TIOA4                        (1UL << 26) 

#define PIN_PC29B_TC1_TIOA5                        93L          /**< TC1 signal: TIOA5 on PC29 mux B*/
#define MUX_PC29B_TC1_TIOA5                        1L          
#define PINMUX_PC29B_TC1_TIOA5                     ((PIN_PC29B_TC1_TIOA5 << 16) | MUX_PC29B_TC1_TIOA5)
#define PIO_PC29B_TC1_TIOA5                        (1UL << 29) 

#define PIN_PC24B_TC1_TIOB3                        88L          /**< TC1 signal: TIOB3 on PC24 mux B*/
#define MUX_PC24B_TC1_TIOB3                        1L          
#define PINMUX_PC24B_TC1_TIOB3                     ((PIN_PC24B_TC1_TIOB3 << 16) | MUX_PC24B_TC1_TIOB3)
#define PIO_PC24B_TC1_TIOB3                        (1UL << 24) 

#define PIN_PC27B_TC1_TIOB4                        91L          /**< TC1 signal: TIOB4 on PC27 mux B*/
#define MUX_PC27B_TC1_TIOB4                        1L          
#define PINMUX_PC27B_TC1_TIOB4                     ((PIN_PC27B_TC1_TIOB4 << 16) | MUX_PC27B_TC1_TIOB4)
#define PIO_PC27B_TC1_TIOB4                        (1UL << 27) 

#define PIN_PC30B_TC1_TIOB5                        94L          /**< TC1 signal: TIOB5 on PC30 mux B*/
#define MUX_PC30B_TC1_TIOB5                        1L          
#define PINMUX_PC30B_TC1_TIOB5                     ((PIN_PC30B_TC1_TIOB5 << 16) | MUX_PC30B_TC1_TIOB5)
#define PIO_PC30B_TC1_TIOB5                        (1UL << 30) 

/* ========== PIO definition for TC2 peripheral ========== */
#define PIN_PC7B_TC2_TCLK6                         71L          /**< TC2 signal: TCLK6 on PC7 mux B*/
#define MUX_PC7B_TC2_TCLK6                         1L          
#define PINMUX_PC7B_TC2_TCLK6                      ((PIN_PC7B_TC2_TCLK6 << 16) | MUX_PC7B_TC2_TCLK6)
#define PIO_PC7B_TC2_TCLK6                         (1UL << 7)  

#define PIN_PC10B_TC2_TCLK7                        74L          /**< TC2 signal: TCLK7 on PC10 mux B*/
#define MUX_PC10B_TC2_TCLK7                        1L          
#define PINMUX_PC10B_TC2_TCLK7                     ((PIN_PC10B_TC2_TCLK7 << 16) | MUX_PC10B_TC2_TCLK7)
#define PIO_PC10B_TC2_TCLK7                        (1UL << 10) 

#define PIN_PC14B_TC2_TCLK8                        78L          /**< TC2 signal: TCLK8 on PC14 mux B*/
#define MUX_PC14B_TC2_TCLK8                        1L          
#define PINMUX_PC14B_TC2_TCLK8                     ((PIN_PC14B_TC2_TCLK8 << 16) | MUX_PC14B_TC2_TCLK8)
#define PIO_PC14B_TC2_TCLK8                        (1UL << 14) 

#define PIN_PC5B_TC2_TIOA6                         69L          /**< TC2 signal: TIOA6 on PC5 mux B*/
#define MUX_PC5B_TC2_TIOA6                         1L          
#define PINMUX_PC5B_TC2_TIOA6                      ((PIN_PC5B_TC2_TIOA6 << 16) | MUX_PC5B_TC2_TIOA6)
#define PIO_PC5B_TC2_TIOA6                         (1UL << 5)  

#define PIN_PC8B_TC2_TIOA7                         72L          /**< TC2 signal: TIOA7 on PC8 mux B*/
#define MUX_PC8B_TC2_TIOA7                         1L          
#define PINMUX_PC8B_TC2_TIOA7                      ((PIN_PC8B_TC2_TIOA7 << 16) | MUX_PC8B_TC2_TIOA7)
#define PIO_PC8B_TC2_TIOA7                         (1UL << 8)  

#define PIN_PC11B_TC2_TIOA8                        75L          /**< TC2 signal: TIOA8 on PC11 mux B*/
#define MUX_PC11B_TC2_TIOA8                        1L          
#define PINMUX_PC11B_TC2_TIOA8                     ((PIN_PC11B_TC2_TIOA8 << 16) | MUX_PC11B_TC2_TIOA8)
#define PIO_PC11B_TC2_TIOA8                        (1UL << 11) 

#define PIN_PC6B_TC2_TIOB6                         70L          /**< TC2 signal: TIOB6 on PC6 mux B*/
#define MUX_PC6B_TC2_TIOB6                         1L          
#define PINMUX_PC6B_TC2_TIOB6                      ((PIN_PC6B_TC2_TIOB6 << 16) | MUX_PC6B_TC2_TIOB6)
#define PIO_PC6B_TC2_TIOB6                         (1UL << 6)  

#define PIN_PC9B_TC2_TIOB7                         73L          /**< TC2 signal: TIOB7 on PC9 mux B*/
#define MUX_PC9B_TC2_TIOB7                         1L          
#define PINMUX_PC9B_TC2_TIOB7                      ((PIN_PC9B_TC2_TIOB7 << 16) | MUX_PC9B_TC2_TIOB7)
#define PIO_PC9B_TC2_TIOB7                         (1UL << 9)  

#define PIN_PC12B_TC2_TIOB8                        76L          /**< TC2 signal: TIOB8 on PC12 mux B*/
#define MUX_PC12B_TC2_TIOB8                        1L          
#define PINMUX_PC12B_TC2_TIOB8                     ((PIN_PC12B_TC2_TIOB8 << 16) | MUX_PC12B_TC2_TIOB8)
#define PIO_PC12B_TC2_TIOB8                        (1UL << 12) 

/* ========== PIO definition for TC3 peripheral ========== */
#define PIN_PE2B_TC3_TCLK9                         130L         /**< TC3 signal: TCLK9 on PE2 mux B*/
#define MUX_PE2B_TC3_TCLK9                         1L          
#define PINMUX_PE2B_TC3_TCLK9                      ((PIN_PE2B_TC3_TCLK9 << 16) | MUX_PE2B_TC3_TCLK9)
#define PIO_PE2B_TC3_TCLK9                         (1UL << 2)  

#define PIN_PE5B_TC3_TCLK10                        133L         /**< TC3 signal: TCLK10 on PE5 mux B*/
#define MUX_PE5B_TC3_TCLK10                        1L          
#define PINMUX_PE5B_TC3_TCLK10                     ((PIN_PE5B_TC3_TCLK10 << 16) | MUX_PE5B_TC3_TCLK10)
#define PIO_PE5B_TC3_TCLK10                        (1UL << 5)  

#define PIN_PD24C_TC3_TCLK11                       120L         /**< TC3 signal: TCLK11 on PD24 mux C*/
#define MUX_PD24C_TC3_TCLK11                       2L          
#define PINMUX_PD24C_TC3_TCLK11                    ((PIN_PD24C_TC3_TCLK11 << 16) | MUX_PD24C_TC3_TCLK11)
#define PIO_PD24C_TC3_TCLK11                       (1UL << 24) 

#define PIN_PE0B_TC3_TIOA9                         128L         /**< TC3 signal: TIOA9 on PE0 mux B*/
#define MUX_PE0B_TC3_TIOA9                         1L          
#define PINMUX_PE0B_TC3_TIOA9                      ((PIN_PE0B_TC3_TIOA9 << 16) | MUX_PE0B_TC3_TIOA9)
#define PIO_PE0B_TC3_TIOA9                         (1UL << 0)  

#define PIN_PE3B_TC3_TIOA10                        131L         /**< TC3 signal: TIOA10 on PE3 mux B*/
#define MUX_PE3B_TC3_TIOA10                        1L          
#define PINMUX_PE3B_TC3_TIOA10                     ((PIN_PE3B_TC3_TIOA10 << 16) | MUX_PE3B_TC3_TIOA10)
#define PIO_PE3B_TC3_TIOA10                        (1UL << 3)  

#define PIN_PD21C_TC3_TIOA11                       117L         /**< TC3 signal: TIOA11 on PD21 mux C*/
#define MUX_PD21C_TC3_TIOA11                       2L          
#define PINMUX_PD21C_TC3_TIOA11                    ((PIN_PD21C_TC3_TIOA11 << 16) | MUX_PD21C_TC3_TIOA11)
#define PIO_PD21C_TC3_TIOA11                       (1UL << 21) 

#define PIN_PE1B_TC3_TIOB9                         129L         /**< TC3 signal: TIOB9 on PE1 mux B*/
#define MUX_PE1B_TC3_TIOB9                         1L          
#define PINMUX_PE1B_TC3_TIOB9                      ((PIN_PE1B_TC3_TIOB9 << 16) | MUX_PE1B_TC3_TIOB9)
#define PIO_PE1B_TC3_TIOB9                         (1UL << 1)  

#define PIN_PE4B_TC3_TIOB10                        132L         /**< TC3 signal: TIOB10 on PE4 mux B*/
#define MUX_PE4B_TC3_TIOB10                        1L          
#define PINMUX_PE4B_TC3_TIOB10                     ((PIN_PE4B_TC3_TIOB10 << 16) | MUX_PE4B_TC3_TIOB10)
#define PIO_PE4B_TC3_TIOB10                        (1UL << 4)  

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
#define PIN_PC18A_EBI_A0                           82L          /**< EBI signal: A0 on PC18 mux A*/
#define MUX_PC18A_EBI_A0                           0L          
#define PINMUX_PC18A_EBI_A0                        ((PIN_PC18A_EBI_A0 << 16) | MUX_PC18A_EBI_A0)
#define PIO_PC18A_EBI_A0                           (1UL << 18) 

#define PIN_PC18A_EBI_NBS0                         82L          /**< EBI signal: NBS0 on PC18 mux A*/
#define MUX_PC18A_EBI_NBS0                         0L          
#define PINMUX_PC18A_EBI_NBS0                      ((PIN_PC18A_EBI_NBS0 << 16) | MUX_PC18A_EBI_NBS0)
#define PIO_PC18A_EBI_NBS0                         (1UL << 18) 

#define PIN_PC19A_EBI_A1                           83L          /**< EBI signal: A1 on PC19 mux A*/
#define MUX_PC19A_EBI_A1                           0L          
#define PINMUX_PC19A_EBI_A1                        ((PIN_PC19A_EBI_A1 << 16) | MUX_PC19A_EBI_A1)
#define PIO_PC19A_EBI_A1                           (1UL << 19) 

#define PIN_PC20A_EBI_A2                           84L          /**< EBI signal: A2 on PC20 mux A*/
#define MUX_PC20A_EBI_A2                           0L          
#define PINMUX_PC20A_EBI_A2                        ((PIN_PC20A_EBI_A2 << 16) | MUX_PC20A_EBI_A2)
#define PIO_PC20A_EBI_A2                           (1UL << 20) 

#define PIN_PC21A_EBI_A3                           85L          /**< EBI signal: A3 on PC21 mux A*/
#define MUX_PC21A_EBI_A3                           0L          
#define PINMUX_PC21A_EBI_A3                        ((PIN_PC21A_EBI_A3 << 16) | MUX_PC21A_EBI_A3)
#define PIO_PC21A_EBI_A3                           (1UL << 21) 

#define PIN_PC22A_EBI_A4                           86L          /**< EBI signal: A4 on PC22 mux A*/
#define MUX_PC22A_EBI_A4                           0L          
#define PINMUX_PC22A_EBI_A4                        ((PIN_PC22A_EBI_A4 << 16) | MUX_PC22A_EBI_A4)
#define PIO_PC22A_EBI_A4                           (1UL << 22) 

#define PIN_PC23A_EBI_A5                           87L          /**< EBI signal: A5 on PC23 mux A*/
#define MUX_PC23A_EBI_A5                           0L          
#define PINMUX_PC23A_EBI_A5                        ((PIN_PC23A_EBI_A5 << 16) | MUX_PC23A_EBI_A5)
#define PIO_PC23A_EBI_A5                           (1UL << 23) 

#define PIN_PC24A_EBI_A6                           88L          /**< EBI signal: A6 on PC24 mux A*/
#define MUX_PC24A_EBI_A6                           0L          
#define PINMUX_PC24A_EBI_A6                        ((PIN_PC24A_EBI_A6 << 16) | MUX_PC24A_EBI_A6)
#define PIO_PC24A_EBI_A6                           (1UL << 24) 

#define PIN_PC25A_EBI_A7                           89L          /**< EBI signal: A7 on PC25 mux A*/
#define MUX_PC25A_EBI_A7                           0L          
#define PINMUX_PC25A_EBI_A7                        ((PIN_PC25A_EBI_A7 << 16) | MUX_PC25A_EBI_A7)
#define PIO_PC25A_EBI_A7                           (1UL << 25) 

#define PIN_PC26A_EBI_A8                           90L          /**< EBI signal: A8 on PC26 mux A*/
#define MUX_PC26A_EBI_A8                           0L          
#define PINMUX_PC26A_EBI_A8                        ((PIN_PC26A_EBI_A8 << 16) | MUX_PC26A_EBI_A8)
#define PIO_PC26A_EBI_A8                           (1UL << 26) 

#define PIN_PC27A_EBI_A9                           91L          /**< EBI signal: A9 on PC27 mux A*/
#define MUX_PC27A_EBI_A9                           0L          
#define PINMUX_PC27A_EBI_A9                        ((PIN_PC27A_EBI_A9 << 16) | MUX_PC27A_EBI_A9)
#define PIO_PC27A_EBI_A9                           (1UL << 27) 

#define PIN_PC28A_EBI_A10                          92L          /**< EBI signal: A10 on PC28 mux A*/
#define MUX_PC28A_EBI_A10                          0L          
#define PINMUX_PC28A_EBI_A10                       ((PIN_PC28A_EBI_A10 << 16) | MUX_PC28A_EBI_A10)
#define PIO_PC28A_EBI_A10                          (1UL << 28) 

#define PIN_PC29A_EBI_A11                          93L          /**< EBI signal: A11 on PC29 mux A*/
#define MUX_PC29A_EBI_A11                          0L          
#define PINMUX_PC29A_EBI_A11                       ((PIN_PC29A_EBI_A11 << 16) | MUX_PC29A_EBI_A11)
#define PIO_PC29A_EBI_A11                          (1UL << 29) 

#define PIN_PC30A_EBI_A12                          94L          /**< EBI signal: A12 on PC30 mux A*/
#define MUX_PC30A_EBI_A12                          0L          
#define PINMUX_PC30A_EBI_A12                       ((PIN_PC30A_EBI_A12 << 16) | MUX_PC30A_EBI_A12)
#define PIO_PC30A_EBI_A12                          (1UL << 30) 

#define PIN_PC31A_EBI_A13                          95L          /**< EBI signal: A13 on PC31 mux A*/
#define MUX_PC31A_EBI_A13                          0L          
#define PINMUX_PC31A_EBI_A13                       ((PIN_PC31A_EBI_A13 << 16) | MUX_PC31A_EBI_A13)
#define PIO_PC31A_EBI_A13                          (1UL << 31) 

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

#define PIN_PC16A_EBI_A21                          80L          /**< EBI signal: A21 on PC16 mux A*/
#define MUX_PC16A_EBI_A21                          0L          
#define PINMUX_PC16A_EBI_A21                       ((PIN_PC16A_EBI_A21 << 16) | MUX_PC16A_EBI_A21)
#define PIO_PC16A_EBI_A21                          (1UL << 16) 

#define PIN_PC16A_EBI_NANDALE                      80L          /**< EBI signal: NANDALE on PC16 mux A*/
#define MUX_PC16A_EBI_NANDALE                      0L          
#define PINMUX_PC16A_EBI_NANDALE                   ((PIN_PC16A_EBI_NANDALE << 16) | MUX_PC16A_EBI_NANDALE)
#define PIO_PC16A_EBI_NANDALE                      (1UL << 16) 

#define PIN_PC17A_EBI_A22                          81L          /**< EBI signal: A22 on PC17 mux A*/
#define MUX_PC17A_EBI_A22                          0L          
#define PINMUX_PC17A_EBI_A22                       ((PIN_PC17A_EBI_A22 << 16) | MUX_PC17A_EBI_A22)
#define PIO_PC17A_EBI_A22                          (1UL << 17) 

#define PIN_PC17A_EBI_NANDCLE                      81L          /**< EBI signal: NANDCLE on PC17 mux A*/
#define MUX_PC17A_EBI_NANDCLE                      0L          
#define PINMUX_PC17A_EBI_NANDCLE                   ((PIN_PC17A_EBI_NANDCLE << 16) | MUX_PC17A_EBI_NANDCLE)
#define PIO_PC17A_EBI_NANDCLE                      (1UL << 17) 

#define PIN_PA25C_EBI_A23                          25L          /**< EBI signal: A23 on PA25 mux C*/
#define MUX_PA25C_EBI_A23                          2L          
#define PINMUX_PA25C_EBI_A23                       ((PIN_PA25C_EBI_A23 << 16) | MUX_PA25C_EBI_A23)
#define PIO_PA25C_EBI_A23                          (1UL << 25) 

#define PIN_PD17C_EBI_CAS                          113L         /**< EBI signal: CAS on PD17 mux C*/
#define MUX_PD17C_EBI_CAS                          2L          
#define PINMUX_PD17C_EBI_CAS                       ((PIN_PD17C_EBI_CAS << 16) | MUX_PD17C_EBI_CAS)
#define PIO_PD17C_EBI_CAS                          (1UL << 17) 

#define PIN_PC0A_EBI_D0                            64L          /**< EBI signal: D0 on PC0 mux A*/
#define MUX_PC0A_EBI_D0                            0L          
#define PINMUX_PC0A_EBI_D0                         ((PIN_PC0A_EBI_D0 << 16) | MUX_PC0A_EBI_D0)
#define PIO_PC0A_EBI_D0                            (1UL << 0)  

#define PIN_PC1A_EBI_D1                            65L          /**< EBI signal: D1 on PC1 mux A*/
#define MUX_PC1A_EBI_D1                            0L          
#define PINMUX_PC1A_EBI_D1                         ((PIN_PC1A_EBI_D1 << 16) | MUX_PC1A_EBI_D1)
#define PIO_PC1A_EBI_D1                            (1UL << 1)  

#define PIN_PC2A_EBI_D2                            66L          /**< EBI signal: D2 on PC2 mux A*/
#define MUX_PC2A_EBI_D2                            0L          
#define PINMUX_PC2A_EBI_D2                         ((PIN_PC2A_EBI_D2 << 16) | MUX_PC2A_EBI_D2)
#define PIO_PC2A_EBI_D2                            (1UL << 2)  

#define PIN_PC3A_EBI_D3                            67L          /**< EBI signal: D3 on PC3 mux A*/
#define MUX_PC3A_EBI_D3                            0L          
#define PINMUX_PC3A_EBI_D3                         ((PIN_PC3A_EBI_D3 << 16) | MUX_PC3A_EBI_D3)
#define PIO_PC3A_EBI_D3                            (1UL << 3)  

#define PIN_PC4A_EBI_D4                            68L          /**< EBI signal: D4 on PC4 mux A*/
#define MUX_PC4A_EBI_D4                            0L          
#define PINMUX_PC4A_EBI_D4                         ((PIN_PC4A_EBI_D4 << 16) | MUX_PC4A_EBI_D4)
#define PIO_PC4A_EBI_D4                            (1UL << 4)  

#define PIN_PC5A_EBI_D5                            69L          /**< EBI signal: D5 on PC5 mux A*/
#define MUX_PC5A_EBI_D5                            0L          
#define PINMUX_PC5A_EBI_D5                         ((PIN_PC5A_EBI_D5 << 16) | MUX_PC5A_EBI_D5)
#define PIO_PC5A_EBI_D5                            (1UL << 5)  

#define PIN_PC6A_EBI_D6                            70L          /**< EBI signal: D6 on PC6 mux A*/
#define MUX_PC6A_EBI_D6                            0L          
#define PINMUX_PC6A_EBI_D6                         ((PIN_PC6A_EBI_D6 << 16) | MUX_PC6A_EBI_D6)
#define PIO_PC6A_EBI_D6                            (1UL << 6)  

#define PIN_PC7A_EBI_D7                            71L          /**< EBI signal: D7 on PC7 mux A*/
#define MUX_PC7A_EBI_D7                            0L          
#define PINMUX_PC7A_EBI_D7                         ((PIN_PC7A_EBI_D7 << 16) | MUX_PC7A_EBI_D7)
#define PIO_PC7A_EBI_D7                            (1UL << 7)  

#define PIN_PE0A_EBI_D8                            128L         /**< EBI signal: D8 on PE0 mux A*/
#define MUX_PE0A_EBI_D8                            0L          
#define PINMUX_PE0A_EBI_D8                         ((PIN_PE0A_EBI_D8 << 16) | MUX_PE0A_EBI_D8)
#define PIO_PE0A_EBI_D8                            (1UL << 0)  

#define PIN_PE1A_EBI_D9                            129L         /**< EBI signal: D9 on PE1 mux A*/
#define MUX_PE1A_EBI_D9                            0L          
#define PINMUX_PE1A_EBI_D9                         ((PIN_PE1A_EBI_D9 << 16) | MUX_PE1A_EBI_D9)
#define PIO_PE1A_EBI_D9                            (1UL << 1)  

#define PIN_PE2A_EBI_D10                           130L         /**< EBI signal: D10 on PE2 mux A*/
#define MUX_PE2A_EBI_D10                           0L          
#define PINMUX_PE2A_EBI_D10                        ((PIN_PE2A_EBI_D10 << 16) | MUX_PE2A_EBI_D10)
#define PIO_PE2A_EBI_D10                           (1UL << 2)  

#define PIN_PE3A_EBI_D11                           131L         /**< EBI signal: D11 on PE3 mux A*/
#define MUX_PE3A_EBI_D11                           0L          
#define PINMUX_PE3A_EBI_D11                        ((PIN_PE3A_EBI_D11 << 16) | MUX_PE3A_EBI_D11)
#define PIO_PE3A_EBI_D11                           (1UL << 3)  

#define PIN_PE4A_EBI_D12                           132L         /**< EBI signal: D12 on PE4 mux A*/
#define MUX_PE4A_EBI_D12                           0L          
#define PINMUX_PE4A_EBI_D12                        ((PIN_PE4A_EBI_D12 << 16) | MUX_PE4A_EBI_D12)
#define PIO_PE4A_EBI_D12                           (1UL << 4)  

#define PIN_PE5A_EBI_D13                           133L         /**< EBI signal: D13 on PE5 mux A*/
#define MUX_PE5A_EBI_D13                           0L          
#define PINMUX_PE5A_EBI_D13                        ((PIN_PE5A_EBI_D13 << 16) | MUX_PE5A_EBI_D13)
#define PIO_PE5A_EBI_D13                           (1UL << 5)  

#define PIN_PA15A_EBI_D14                          15L          /**< EBI signal: D14 on PA15 mux A*/
#define MUX_PA15A_EBI_D14                          0L          
#define PINMUX_PA15A_EBI_D14                       ((PIN_PA15A_EBI_D14 << 16) | MUX_PA15A_EBI_D14)
#define PIO_PA15A_EBI_D14                          (1UL << 15) 

#define PIN_PA16A_EBI_D15                          16L          /**< EBI signal: D15 on PA16 mux A*/
#define MUX_PA16A_EBI_D15                          0L          
#define PINMUX_PA16A_EBI_D15                       ((PIN_PA16A_EBI_D15 << 16) | MUX_PA16A_EBI_D15)
#define PIO_PA16A_EBI_D15                          (1UL << 16) 

#define PIN_PC9A_EBI_NANDOE                        73L          /**< EBI signal: NANDOE on PC9 mux A*/
#define MUX_PC9A_EBI_NANDOE                        0L          
#define PINMUX_PC9A_EBI_NANDOE                     ((PIN_PC9A_EBI_NANDOE << 16) | MUX_PC9A_EBI_NANDOE)
#define PIO_PC9A_EBI_NANDOE                        (1UL << 9)  

#define PIN_PC10A_EBI_NANDWE                       74L          /**< EBI signal: NANDWE on PC10 mux A*/
#define MUX_PC10A_EBI_NANDWE                       0L          
#define PINMUX_PC10A_EBI_NANDWE                    ((PIN_PC10A_EBI_NANDWE << 16) | MUX_PC10A_EBI_NANDWE)
#define PIO_PC10A_EBI_NANDWE                       (1UL << 10) 

#define PIN_PC14A_EBI_NCS0                         78L          /**< EBI signal: NCS0 on PC14 mux A*/
#define MUX_PC14A_EBI_NCS0                         0L          
#define PINMUX_PC14A_EBI_NCS0                      ((PIN_PC14A_EBI_NCS0 << 16) | MUX_PC14A_EBI_NCS0)
#define PIO_PC14A_EBI_NCS0                         (1UL << 14) 

#define PIN_PC15A_EBI_NCS1                         79L          /**< EBI signal: NCS1 on PC15 mux A*/
#define MUX_PC15A_EBI_NCS1                         0L          
#define PINMUX_PC15A_EBI_NCS1                      ((PIN_PC15A_EBI_NCS1 << 16) | MUX_PC15A_EBI_NCS1)
#define PIO_PC15A_EBI_NCS1                         (1UL << 15) 

#define PIN_PC15A_EBI_SDCS                         79L          /**< EBI signal: SDCS on PC15 mux A*/
#define MUX_PC15A_EBI_SDCS                         0L          
#define PINMUX_PC15A_EBI_SDCS                      ((PIN_PC15A_EBI_SDCS << 16) | MUX_PC15A_EBI_SDCS)
#define PIO_PC15A_EBI_SDCS                         (1UL << 15) 

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

#define PIN_PC12A_EBI_NCS3                         76L          /**< EBI signal: NCS3 on PC12 mux A*/
#define MUX_PC12A_EBI_NCS3                         0L          
#define PINMUX_PC12A_EBI_NCS3                      ((PIN_PC12A_EBI_NCS3 << 16) | MUX_PC12A_EBI_NCS3)
#define PIO_PC12A_EBI_NCS3                         (1UL << 12) 

#define PIN_PD19A_EBI_NCS3                         115L         /**< EBI signal: NCS3 on PD19 mux A*/
#define MUX_PD19A_EBI_NCS3                         0L          
#define PINMUX_PD19A_EBI_NCS3                      ((PIN_PD19A_EBI_NCS3 << 16) | MUX_PD19A_EBI_NCS3)
#define PIO_PD19A_EBI_NCS3                         (1UL << 19) 

#define PIN_PC11A_EBI_NRD                          75L          /**< EBI signal: NRD on PC11 mux A*/
#define MUX_PC11A_EBI_NRD                          0L          
#define PINMUX_PC11A_EBI_NRD                       ((PIN_PC11A_EBI_NRD << 16) | MUX_PC11A_EBI_NRD)
#define PIO_PC11A_EBI_NRD                          (1UL << 11) 

#define PIN_PC13A_EBI_NWAIT                        77L          /**< EBI signal: NWAIT on PC13 mux A*/
#define MUX_PC13A_EBI_NWAIT                        0L          
#define PINMUX_PC13A_EBI_NWAIT                     ((PIN_PC13A_EBI_NWAIT << 16) | MUX_PC13A_EBI_NWAIT)
#define PIO_PC13A_EBI_NWAIT                        (1UL << 13) 

#define PIN_PC8A_EBI_NWR0                          72L          /**< EBI signal: NWR0 on PC8 mux A*/
#define MUX_PC8A_EBI_NWR0                          0L          
#define PINMUX_PC8A_EBI_NWR0                       ((PIN_PC8A_EBI_NWR0 << 16) | MUX_PC8A_EBI_NWR0)
#define PIO_PC8A_EBI_NWR0                          (1UL << 8)  

#define PIN_PC8A_EBI_NWE                           72L          /**< EBI signal: NWE on PC8 mux A*/
#define MUX_PC8A_EBI_NWE                           0L          
#define PINMUX_PC8A_EBI_NWE                        ((PIN_PC8A_EBI_NWE << 16) | MUX_PC8A_EBI_NWE)
#define PIO_PC8A_EBI_NWE                           (1UL << 8)  

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

#define PIN_PC13C_EBI_SDA10                        77L          /**< EBI signal: SDA10 on PC13 mux C*/
#define MUX_PC13C_EBI_SDA10                        2L          
#define PINMUX_PC13C_EBI_SDA10                     ((PIN_PC13C_EBI_SDA10 << 16) | MUX_PC13C_EBI_SDA10)
#define PIO_PC13C_EBI_SDA10                        (1UL << 13) 

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

#endif /* _SAME70Q21_PIO_H_ */
