/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RTS5912_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RTS5912_CLOCK_H_

/* ====================================================================================== */
/* =====================================  I2CCLK  ======================================= */
#define I2CCLK_I2C0CLKPWR_Pos         (0)          /*!< I2C0CLKPWR (Bit 0)                 */
#define I2CCLK_I2C0CLKPWR_Msk         (0x1)        /*!< I2C0CLKPWR (Bitfield-Mask: 0x01)   */
#define I2CCLK_I2C0CLKSRC_Pos         (1)          /*!< I2C0CLKSRC (Bit 1)                 */
#define I2CCLK_I2C0CLKSRC_Msk         (0x2)        /*!< I2C0CLKSRC (Bitfield-Mask: 0x01)   */
#define I2CCLK_I2C0CLKDIV_Pos         (2)          /*!< I2C0CLKDIV (Bit 2)                 */
#define I2CCLK_I2C0CLKDIV_Msk         (0xc)        /*!< I2C0CLKDIV (Bitfield-Mask: 0x03)   */
#define I2CCLK_I2C1CLKPWR_Pos         (4)          /*!< I2C1CLKPWR (Bit 4)                 */
#define I2CCLK_I2C1CLKPWR_Msk         (0x10)       /*!< I2C1CLKPWR (Bitfield-Mask: 0x01)   */
#define I2CCLK_I2C1CLKSRC_Pos         (5)          /*!< I2C1CLKSRC (Bit 5)                 */
#define I2CCLK_I2C1CLKSRC_Msk         (0x20)       /*!< I2C1CLKSRC (Bitfield-Mask: 0x01)   */
#define I2CCLK_I2C1CLKDIV_Pos         (6)          /*!< I2C1CLKDIV (Bit 6)                 */
#define I2CCLK_I2C1CLKDIV_Msk         (0xc0)       /*!< I2C1CLKDIV (Bitfield-Mask: 0x03)   */
#define I2CCLK_I2C2CLKPWR_Pos         (8)          /*!< I2C2CLKPWR (Bit 8)                 */
#define I2CCLK_I2C2CLKPWR_Msk         (0x100)      /*!< I2C2CLKPWR (Bitfield-Mask: 0x01)   */
#define I2CCLK_I2C2CLKSRC_Pos         (9)          /*!< I2C2CLKSRC (Bit 9)                 */
#define I2CCLK_I2C2CLKSRC_Msk         (0x200)      /*!< I2C2CLKSRC (Bitfield-Mask: 0x01)   */
#define I2CCLK_I2C2CLKDIV_Pos         (10)         /*!< I2C2CLKDIV (Bit 10)                */
#define I2CCLK_I2C2CLKDIV_Msk         (0xc00)      /*!< I2C2CLKDIV (Bitfield-Mask: 0x03)   */
#define I2CCLK_I2C3CLKPWR_Pos         (12)         /*!< I2C3CLKPWR (Bit 12)                */
#define I2CCLK_I2C3CLKPWR_Msk         (0x1000)     /*!< I2C3CLKPWR (Bitfield-Mask: 0x01)   */
#define I2CCLK_I2C3CLKSRC_Pos         (13)         /*!< I2C3CLKSRC (Bit 13)                */
#define I2CCLK_I2C3CLKSRC_Msk         (0x2000)     /*!< I2C3CLKSRC (Bitfield-Mask: 0x01)   */
#define I2CCLK_I2C3CLKDIV_Pos         (14)         /*!< I2C3CLKDIV (Bit 14)                */
#define I2CCLK_I2C3CLKDIV_Msk         (0xc000)     /*!< I2C3CLKDIV (Bitfield-Mask: 0x03)   */
#define I2CCLK_I2C4CLKPWR_Pos         (16)         /*!< I2C4CLKPWR (Bit 16)                */
#define I2CCLK_I2C4CLKPWR_Msk         (0x10000)    /*!< I2C4CLKPWR (Bitfield-Mask: 0x01)   */
#define I2CCLK_I2C4CLKSRC_Pos         (17)         /*!< I2C4CLKSRC (Bit 17)                */
#define I2CCLK_I2C4CLKSRC_Msk         (0x20000)    /*!< I2C4CLKSRC (Bitfield-Mask: 0x01)   */
#define I2CCLK_I2C4CLKDIV_Pos         (18)         /*!< I2C4CLKDIV (Bit 18)                */
#define I2CCLK_I2C4CLKDIV_Msk         (0xc0000)    /*!< I2C4CLKDIV (Bitfield-Mask: 0x03)   */
#define I2CCLK_I2C5CLKPWR_Pos         (20)         /*!< I2C5CLKPWR (Bit 20)                */
#define I2CCLK_I2C5CLKPWR_Msk         (0x100000)   /*!< I2C5CLKPWR (Bitfield-Mask: 0x01)   */
#define I2CCLK_I2C5CLKSRC_Pos         (21)         /*!< I2C5CLKSRC (Bit 21)                */
#define I2CCLK_I2C5CLKSRC_Msk         (0x200000)   /*!< I2C5CLKSRC (Bitfield-Mask: 0x01)   */
#define I2CCLK_I2C5CLKDIV_Pos         (22)         /*!< I2C5CLKDIV (Bit 22)                */
#define I2CCLK_I2C5CLKDIV_Msk         (0xc00000)   /*!< I2C5CLKDIV (Bitfield-Mask: 0x03)   */
#define I2CCLK_I2C6CLKPWR_Pos         (24)         /*!< I2C6CLKPWR (Bit 24)                */
#define I2CCLK_I2C6CLKPWR_Msk         (0x1000000)  /*!< I2C6CLKPWR (Bitfield-Mask: 0x01)   */
#define I2CCLK_I2C6CLKSRC_Pos         (25)         /*!< I2C6CLKSRC (Bit 25)                */
#define I2CCLK_I2C6CLKSRC_Msk         (0x2000000)  /*!< I2C6CLKSRC (Bitfield-Mask: 0x01)   */
#define I2CCLK_I2C6CLKDIV_Pos         (26)         /*!< I2C6CLKDIV (Bit 26)                */
#define I2CCLK_I2C6CLKDIV_Msk         (0xc000000)  /*!< I2C6CLKDIV (Bitfield-Mask: 0x03)   */
#define I2CCLK_I2C7CLKPWR_Pos         (28)         /*!< I2C7CLKPWR (Bit 28)                */
#define I2CCLK_I2C7CLKPWR_Msk         (0x10000000) /*!< I2C7CLKPWR (Bitfield-Mask: 0x01)   */
#define I2CCLK_I2C7CLKSRC_Pos         (29)         /*!< I2C7CLKSRC (Bit 29)                */
#define I2CCLK_I2C7CLKSRC_Msk         (0x20000000) /*!< I2C7CLKSRC (Bitfield-Mask: 0x01)   */
#define I2CCLK_I2C7CLKDIV_Pos         (30)         /*!< I2C7CLKDIV (Bit 30)                */
#define I2CCLK_I2C7CLKDIV_Msk         (0xc0000000) /*!< I2C7CLKDIV (Bitfield-Mask: 0x03)   */
/* ===================================  PERICLKPWR0  ==================================== */
#define PERICLKPWR0_GPIOCLKPWR_Pos    (0)          /*!< GPIOCLKPWR (Bit 0)                 */
#define PERICLKPWR0_GPIOCLKPWR_Msk    (0x1)        /*!< GPIOCLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR0_TACHO0CLKPWR_Pos  (1)          /*!< TACHO0CLKPWR (Bit 1)               */
#define PERICLKPWR0_TACHO0CLKPWR_Msk  (0x2)        /*!< TACHO0CLKPWR (Bitfield-Mask: 0x01) */
#define PERICLKPWR0_TACHO1CLKPWR_Pos  (2)          /*!< TACHO1CLKPWR (Bit 2)               */
#define PERICLKPWR0_TACHO1CLKPWR_Msk  (0x4)        /*!< TACHO1CLKPWR (Bitfield-Mask: 0x01) */
#define PERICLKPWR0_TACHO2CLKPWR_Pos  (3)          /*!< TACHO2CLKPWR (Bit 3)               */
#define PERICLKPWR0_TACHO2CLKPWR_Msk  (0x8)        /*!< TACHO2CLKPWR (Bitfield-Mask: 0x01) */
#define PERICLKPWR0_TACHO3CLKPWR_Pos  (4)          /*!< TACHO3CLKPWR (Bit 4)               */
#define PERICLKPWR0_TACHO3CLKPWR_Msk  (0x10)       /*!< TACHO3CLKPWR (Bitfield-Mask: 0x01) */
#define PERICLKPWR0_PS2CLKPWR_Pos     (5)          /*!< PS2CLKPWR (Bit 5)                  */
#define PERICLKPWR0_PS2CLKPWR_Msk     (0x20)       /*!< PS2CLKPWR (Bitfield-Mask: 0x01)    */
#define PERICLKPWR0_KBMCLKPWR_Pos     (6)          /*!< KBMCLKPWR (Bit 6)                  */
#define PERICLKPWR0_KBMCLKPWR_Msk     (0x40)       /*!< KBMCLKPWR (Bitfield-Mask: 0x01)    */
#define PERICLKPWR0_PECICLKPWR_Pos    (7)          /*!< PECICLKPWR (Bit 7)                 */
#define PERICLKPWR0_PECICLKPWR_Msk    (0x80)       /*!< PECICLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR0_PL0CLKPWR_Pos     (8)          /*!< PL0CLKPWR (Bit 8)                  */
#define PERICLKPWR0_PL0CLKPWR_Msk     (0x100)      /*!< PL0CLKPWR (Bitfield-Mask: 0x01)    */
#define PERICLKPWR0_PL1CLKPWR_Pos     (9)          /*!< PL1CLKPWR (Bit 9)                  */
#define PERICLKPWR0_PL1CLKPWR_Msk     (0x200)      /*!< PL1CLKPWR (Bitfield-Mask: 0x01)    */
#define PERICLKPWR0_PWM0CLKPWR_Pos    (10)         /*!< PWM0CLKPWR (Bit 10)                */
#define PERICLKPWR0_PWM0CLKPWR_Msk    (0x400)      /*!< PWM0CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR0_PWM1CLKPWR_Pos    (11)         /*!< PWM1CLKPWR (Bit 11)                */
#define PERICLKPWR0_PWM1CLKPWR_Msk    (0x800)      /*!< PWM1CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR0_PWM2CLKPWR_Pos    (12)         /*!< PWM2CLKPWR (Bit 12)                */
#define PERICLKPWR0_PWM2CLKPWR_Msk    (0x1000)     /*!< PWM2CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR0_PWM3CLKPWR_Pos    (13)         /*!< PWM3CLKPWR (Bit 13)                */
#define PERICLKPWR0_PWM3CLKPWR_Msk    (0x2000)     /*!< PWM3CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR0_PWM4CLKPWR_Pos    (14)         /*!< PWM4CLKPWR (Bit 14)                */
#define PERICLKPWR0_PWM4CLKPWR_Msk    (0x4000)     /*!< PWM4CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR0_PWM5CLKPWR_Pos    (15)         /*!< PWM5CLKPWR (Bit 15)                */
#define PERICLKPWR0_PWM5CLKPWR_Msk    (0x8000)     /*!< PWM5CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR0_PWM6CLKPWR_Pos    (16)         /*!< PWM6CLKPWR (Bit 16)                */
#define PERICLKPWR0_PWM6CLKPWR_Msk    (0x10000)    /*!< PWM6CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR0_PWM7CLKPWR_Pos    (17)         /*!< PWM7CLKPWR (Bit 17)                */
#define PERICLKPWR0_PWM7CLKPWR_Msk    (0x20000)    /*!< PWM7CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR0_PWM8CLKPWR_Pos    (18)         /*!< PWM8CLKPWR (Bit 18)                */
#define PERICLKPWR0_PWM8CLKPWR_Msk    (0x40000)    /*!< PWM8CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR0_PWM9CLKPWR_Pos    (19)         /*!< PWM9CLKPWR (Bit 19)                */
#define PERICLKPWR0_PWM9CLKPWR_Msk    (0x80000)    /*!< PWM9CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR0_PWM10CLKPWR_Pos   (20)         /*!< PWM10CLKPWR (Bit 20)               */
#define PERICLKPWR0_PWM10CLKPWR_Msk   (0x100000)   /*!< PWM10CLKPWR (Bitfield-Mask: 0x01)  */
#define PERICLKPWR0_PWM11CLKPWR_Pos   (21)         /*!< PWM11CLKPWR (Bit 21)               */
#define PERICLKPWR0_PWM11CLKPWR_Msk   (0x200000)   /*!< PWM11CLKPWR (Bitfield-Mask: 0x01)  */
#define PERICLKPWR0_ESPICLKPWR_Pos    (22)         /*!< ESPICLKPWR (Bit 22)                */
#define PERICLKPWR0_ESPICLKPWR_Msk    (0x400000)   /*!< ESPICLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR0_KBCCLKPWR_Pos     (23)         /*!< KBCCLKPWR (Bit 23)                 */
#define PERICLKPWR0_KBCCLKPWR_Msk     (0x800000)   /*!< KBCCLKPWR (Bitfield-Mask: 0x01)    */
#define PERICLKPWR0_ACPICLKPWR_Pos    (24)         /*!< ACPICLKPWR (Bit 24)                */
#define PERICLKPWR0_ACPICLKPWR_Msk    (0x1000000)  /*!< ACPICLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR0_PMPORT0CLKPWR_Pos (25)         /*!< PMPORT0CLKPWR (Bit 25)             */
#define PERICLKPWR0_PMPORT0CLKPWR_Msk (0x2000000)  /*!< PMPORT0CLKPWR (Bitfield-Mask: 0x01)*/
#define PERICLKPWR0_PMPORT1CLKPWR_Pos (26)         /*!< PMPORT1CLKPWR (Bit 26)             */
#define PERICLKPWR0_PMPORT1CLKPWR_Msk (0x4000000)  /*!< PMPORT1CLKPWR (Bitfield-Mask: 0x01)*/
#define PERICLKPWR0_PMPORT2CLKPWR_Pos (27)         /*!< PMPORT2CLKPWR (Bit 27)             */
#define PERICLKPWR0_PMPORT2CLKPWR_Msk (0x8000000)  /*!< PMPORT2CLKPWR (Bitfield-Mask: 0x01)*/
#define PERICLKPWR0_PMPORT3CLKPWR_Pos (28)         /*!< PMPORT3CLKPWR (Bit 28)             */
#define PERICLKPWR0_PMPORT3CLKPWR_Msk (0x10000000) /*!< PMPORT3CLKPWR (Bitfield-Mask: 0x01)*/
#define PERICLKPWR0_P80CLKPWR_Pos     (29)         /*!< P80CLKPWR (Bit 29)                 */
#define PERICLKPWR0_P80CLKPWR_Msk     (0x20000000) /*!< P80CLKPWR (Bitfield-Mask: 0x01)    */
#define PERICLKPWR0_EMI0CLKPWR_Pos    (30)         /*!< EMI0CLKPWR (Bit 30)                */
#define PERICLKPWR0_EMI0CLKPWR_Msk    (0x40000000) /*!< EMI0CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR0_EMI1CLKPWR_Pos    (31)         /*!< EMI1CLKPWR (Bit 31)                */
#define PERICLKPWR0_EMI1CLKPWR_Msk    (0x80000000) /*!< EMI1CLKPWR (Bitfield-Mask: 0x01)   */
/* =====================================  UARTCLK  ====================================== */
#define UARTCLK_PWR_Pos               (0)   /*!< PWR (Bit 0)                        */
#define UARTCLK_PWR_Msk               (0x1) /*!< PWR (Bitfield-Mask: 0x01)          */
#define UARTCLK_SRC_Pos               (1)   /*!< SRC (Bit 1)                        */
#define UARTCLK_SRC_Msk               (0x2) /*!< SRC (Bitfield-Mask: 0x01)          */
#define UARTCLK_DIV_Pos               (2)   /*!< DIV (Bit 2)                        */
#define UARTCLK_DIV_Msk               (0xc) /*!< DIV (Bitfield-Mask: 0x03)          */
/* =====================================  ADCCLK  ======================================= */
#define ADCCLK_PWR_Pos                (0)    /*!< PWR (Bit 0)                        */
#define ADCCLK_PWR_Msk                (0x1)  /*!< PWR (Bitfield-Mask: 0x01)          */
#define ADCCLK_SRC_Pos                (1)    /*!< SRC (Bit 1)                        */
#define ADCCLK_SRC_Msk                (0x2)  /*!< SRC (Bitfield-Mask: 0x01)          */
#define ADCCLK_DIV_Pos                (2)    /*!< DIV (Bit 2)                        */
#define ADCCLK_DIV_Msk                (0x1c) /*!< DIV (Bitfield-Mask: 0x07)          */
/* ===================================  PERICLKPWR1  ==================================== */
#define PERICLKPWR1_EMI2CLKPWR_Pos    (0)        /*!< EMI2CLKPWR (Bit 0)                 */
#define PERICLKPWR1_EMI2CLKPWR_Msk    (0x1)      /*!< EMI2CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR1_EMI3CLKPWR_Pos    (1)        /*!< EMI3CLKPWR (Bit 1)                 */
#define PERICLKPWR1_EMI3CLKPWR_Msk    (0x2)      /*!< EMI3CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR1_EMI4CLKPWR_Pos    (2)        /*!< EMI4CLKPWR (Bit 2)                 */
#define PERICLKPWR1_EMI4CLKPWR_Msk    (0x4)      /*!< EMI4CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR1_EMI5CLKPWR_Pos    (3)        /*!< EMI5CLKPWR (Bit 3)                 */
#define PERICLKPWR1_EMI5CLKPWR_Msk    (0x8)      /*!< EMI5CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR1_EMI6CLKPWR_Pos    (4)        /*!< EMI6CLKPWR (Bit 4)                 */
#define PERICLKPWR1_EMI6CLKPWR_Msk    (0x10)     /*!< EMI6CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR1_EMI7CLKPWR_Pos    (5)        /*!< EMI7CLKPWR (Bit 5)                 */
#define PERICLKPWR1_EMI7CLKPWR_Msk    (0x20)     /*!< EMI7CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR1_I3C0CLKPWR_Pos    (9)        /*!< I3C0CLKPWR (Bit 9)                 */
#define PERICLKPWR1_I3C0CLKPWR_Msk    (0x200)    /*!< I3C0CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR1_I3C1CLKPWR_Pos    (10)       /*!< I3C1CLKPWR (Bit 10)                */
#define PERICLKPWR1_I3C1CLKPWR_Msk    (0x400)    /*!< I3C1CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR1_I2CAUTOCLKPWR_Pos (11)       /*!< I2CAUTOCLKPWR (Bit 11)             */
#define PERICLKPWR1_I2CAUTOCLKPWR_Msk (0x800)    /*!< I2CAUTOCLKPWR (Bitfield-Mask: 0x01)*/
#define PERICLKPWR1_MCCLKPWR_Pos      (12)       /*!< MCCLKPWR (Bit 12)                  */
#define PERICLKPWR1_MCCLKPWR_Msk      (0x1000)   /*!< MCCLKPWR (Bitfield-Mask: 0x01)     */
#define PERICLKPWR1_TMR0CLKPWR_Pos    (13)       /*!< TMR0CLKPWR (Bit 13)                */
#define PERICLKPWR1_TMR0CLKPWR_Msk    (0x2000)   /*!< TMR0CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR1_TMR1CLKPWR_Pos    (14)       /*!< TMR1CLKPWR (Bit 14)                */
#define PERICLKPWR1_TMR1CLKPWR_Msk    (0x4000)   /*!< TMR1CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR1_TMR2CLKPWR_Pos    (15)       /*!< TMR2CLKPWR (Bit 15)                */
#define PERICLKPWR1_TMR2CLKPWR_Msk    (0x8000)   /*!< TMR2CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR1_TMR3CLKPWR_Pos    (16)       /*!< TMR3CLKPWR (Bit 16)                */
#define PERICLKPWR1_TMR3CLKPWR_Msk    (0x10000)  /*!< TMR3CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR1_TMR4CLKPWR_Pos    (17)       /*!< TMR4CLKPWR (Bit 17)                */
#define PERICLKPWR1_TMR4CLKPWR_Msk    (0x20000)  /*!< TMR4CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR1_TMR5CLKPWR_Pos    (18)       /*!< TMR5CLKPWR (Bit 18)                */
#define PERICLKPWR1_TMR5CLKPWR_Msk    (0x40000)  /*!< TMR5CLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR1_RTMRCLKPWR_Pos    (19)       /*!< RTMRCLKPWR (Bit 19)                */
#define PERICLKPWR1_RTMRCLKPWR_Msk    (0x80000)  /*!< RTMRCLKPWR (Bitfield-Mask: 0x01)   */
#define PERICLKPWR1_SLWTMR0CLKPWR_Pos (20)       /*!< SLWTMR0CLKPWR (Bit 20)             */
#define PERICLKPWR1_SLWTMR0CLKPWR_Msk (0x100000) /*!< SLWTMR0CLKPWR (Bitfield-Mask: 0x01)*/
#define PERICLKPWR1_SLWTMR1CLKPWR_Pos (21)       /*!< SLWTMR1CLKPWR (Bit 21)             */
#define PERICLKPWR1_SLWTMR1CLKPWR_Msk (0x200000) /*!< SLWTMR1CLKPWR (Bitfield-Mask: 0x01)*/
/* ===================================  PERICLKPWR2  ==================================== */
#define PERICLKPWR2_RTCCLKPWR_Pos     (0)          /*!< RTCCLKPWR (Bit 0)                  */
#define PERICLKPWR2_RTCCLKPWR_Msk     (0x1)        /*!< RTCCLKPWR (Bitfield-Mask: 0x01)    */
#define PERICLKPWR2_WDTCLKPWR_Pos     (1)          /*!< WDTCLKPWR (Bit 1)                  */
#define PERICLKPWR2_WDTCLKPWR_Msk     (0x2)        /*!< WDTCLKPWR (Bitfield-Mask: 0x01)    */
#define PERICLKPWR2_PWRBTNCLKPWR_Pos  (2)          /*!< PWRBTNCLKPWR (Bit 2)               */
#define PERICLKPWR2_PWRBTNCLKPWR_Msk  (0x4)        /*!< PWRBTNCLKPWR (Bitfield-Mask: 0x01) */
#define PERICLKPWR2_RC32KSRC_Pos      (30)         /*!< RC32KSRC (Bit 30)                  */
#define PERICLKPWR2_RC32KSRC_Msk      (0xc0000000) /*!< RC32KSRC (Bitfield-Mask: 0x03)     */
/* ====================================================================================== */

#define RTS5912_SCCON_SYS         (0)
#define RTS5912_SCCON_I2C         (2)
#define RTS5912_SCCON_UART        (3)
#define RTS5912_SCCON_ADC         (4)
#define RTS5912_SCCON_PERIPH_GRP0 (5)
#define RTS5912_SCCON_PERIPH_GRP1 (6)
#define RTS5912_SCCON_PERIPH_GRP2 (7)

#define I2C0_CLKPWR (I2CCLK_I2C0CLKPWR_Pos)
#define I2C1_CLKPWR (I2CCLK_I2C1CLKPWR_Pos)
#define I2C2_CLKPWR (I2CCLK_I2C2CLKPWR_Pos)
#define I2C3_CLKPWR (I2CCLK_I2C3CLKPWR_Pos)
#define I2C4_CLKPWR (I2CCLK_I2C4CLKPWR_Pos)
#define I2C5_CLKPWR (I2CCLK_I2C5CLKPWR_Pos)
#define I2C6_CLKPWR (I2CCLK_I2C6CLKPWR_Pos)
#define I2C7_CLKPWR (I2CCLK_I2C7CLKPWR_Pos)

#define I2C0_PLL   (0x0 << I2CCLK_I2C0CLKSRC_Pos)
#define I2C0_RC25M (0x1 << I2CCLK_I2C0CLKSRC_Pos)
#define I2C1_PLL   (0x0 << I2CCLK_I2C1CLKSRC_Pos)
#define I2C1_RC25M (0x1 << I2CCLK_I2C1CLKSRC_Pos)
#define I2C2_PLL   (0x0 << I2CCLK_I2C2CLKSRC_Pos)
#define I2C2_RC25M (0x1 << I2CCLK_I2C2CLKSRC_Pos)
#define I2C3_PLL   (0x0 << I2CCLK_I2C3CLKSRC_Pos)
#define I2C3_RC25M (0x1 << I2CCLK_I2C3CLKSRC_Pos)
#define I2C4_PLL   (0x0 << I2CCLK_I2C4CLKSRC_Pos)
#define I2C4_RC25M (0x1 << I2CCLK_I2C4CLKSRC_Pos)
#define I2C5_PLL   (0x0 << I2CCLK_I2C5CLKSRC_Pos)
#define I2C5_RC25M (0x1 << I2CCLK_I2C5CLKSRC_Pos)
#define I2C6_PLL   (0x0 << I2CCLK_I2C6CLKSRC_Pos)
#define I2C6_RC25M (0x1 << I2CCLK_I2C6CLKSRC_Pos)
#define I2C7_PLL   (0x0 << I2CCLK_I2C7CLKSRC_Pos)
#define I2C7_RC25M (0x1 << I2CCLK_I2C7CLKSRC_Pos)

#define I2C0_CLKDIV1 (0 << I2CCLK_I2C0CLKDIV_Pos)
#define I2C0_CLKDIV2 (1 << I2CCLK_I2C0CLKDIV_Pos)
#define I2C0_CLKDIV4 (2 << I2CCLK_I2C0CLKDIV_Pos)
#define I2C0_CLKDIV8 (3 << I2CCLK_I2C0CLKDIV_Pos)

#define I2C1_CLKDIV1 (0 << I2CCLK_I2C1CLKDIV_Pos)
#define I2C1_CLKDIV2 (1 << I2CCLK_I2C1CLKDIV_Pos)
#define I2C1_CLKDIV4 (2 << I2CCLK_I2C1CLKDIV_Pos)
#define I2C1_CLKDIV8 (3 << I2CCLK_I2C1CLKDIV_Pos)

#define I2C2_CLKDIV1 (0 << I2CCLK_I2C2CLKDIV_Pos)
#define I2C2_CLKDIV2 (1 << I2CCLK_I2C2CLKDIV_Pos)
#define I2C2_CLKDIV4 (2 << I2CCLK_I2C2CLKDIV_Pos)
#define I2C2_CLKDIV8 (3 << I2CCLK_I2C2CLKDIV_Pos)

#define I2C3_CLKDIV1 (0 << I2CCLK_I2C3CLKDIV_Pos)
#define I2C3_CLKDIV2 (1 << I2CCLK_I2C3CLKDIV_Pos)
#define I2C3_CLKDIV4 (2 << I2CCLK_I2C3CLKDIV_Pos)
#define I2C3_CLKDIV8 (3 << I2CCLK_I2C3CLKDIV_Pos)

#define I2C4_CLKDIV1 (0 << I2CCLK_I2C4CLKDIV_Pos)
#define I2C4_CLKDIV2 (1 << I2CCLK_I2C4CLKDIV_Pos)
#define I2C4_CLKDIV4 (2 << I2CCLK_I2C4CLKDIV_Pos)
#define I2C4_CLKDIV8 (3 << I2CCLK_I2C4CLKDIV_Pos)

#define I2C5_CLKDIV1 (0 << I2CCLK_I2C5CLKDIV_Pos)
#define I2C5_CLKDIV2 (1 << I2CCLK_I2C5CLKDIV_Pos)
#define I2C5_CLKDIV4 (2 << I2CCLK_I2C5CLKDIV_Pos)
#define I2C5_CLKDIV8 (3 << I2CCLK_I2C5CLKDIV_Pos)

#define I2C6_CLKDIV1 (0 << I2CCLK_I2C6CLKDIV_Pos)
#define I2C6_CLKDIV2 (1 << I2CCLK_I2C6CLKDIV_Pos)
#define I2C6_CLKDIV4 (2 << I2CCLK_I2C6CLKDIV_Pos)
#define I2C6_CLKDIV8 (3 << I2CCLK_I2C6CLKDIV_Pos)

#define I2C7_CLKDIV1 (0 << I2CCLK_I2C7CLKDIV_Pos)
#define I2C7_CLKDIV2 (1 << I2CCLK_I2C7CLKDIV_Pos)
#define I2C7_CLKDIV4 (2 << I2CCLK_I2C7CLKDIV_Pos)
#define I2C7_CLKDIV8 (3 << I2CCLK_I2C7CLKDIV_Pos)

#define UART0_CLKPWR (UARTCLK_PWR_Pos)

#define UART0_RC25M (0x0 << UARTCLK_SRC_Pos)
#define UART0_PLL   (0x1 << UARTCLK_SRC_Pos)

#define UART0_CLKDIV1 (0 << UARTCLK_DIV_Pos)
#define UART0_CLKDIV2 (1 << UARTCLK_DIV_Pos)
#define UART0_CLKDIV4 (2 << UARTCLK_DIV_Pos)
#define UART0_CLKDIV8 (3 << UARTCLK_DIV_Pos)

#define ADC0_CLKPWR (ADCCLK_PWR_Pos)

#define ADC0_RC25M (0x0 << ADCCLK_SRC_Pos)
#define ADC0_PLL   (0x1 << ADCCLK_SRC_Pos)

#define ADC0_CLKDIV1 (0 << ADCCLK_DIV_Pos)
#define ADC0_CLKDIV2 (1 << ADCCLK_DIV_Pos)
#define ADC0_CLKDIV3 (2 << ADCCLK_DIV_Pos)
#define ADC0_CLKDIV4 (3 << ADCCLK_DIV_Pos)
#define ADC0_CLKDIV6 (4 << ADCCLK_DIV_Pos)
#define ADC0_CLKDIV8 (5 << ADCCLK_DIV_Pos)

#define PERIPH_GRP0_GPIO_CLKPWR    (PERICLKPWR0_GPIOCLKPWR_Pos)
#define PERIPH_GRP0_TACH0_CLKPWR   (PERICLKPWR0_TACHO0CLKPWR_Pos)
#define PERIPH_GRP0_TACH1_CLKPWR   (PERICLKPWR0_TACHO1CLKPWR_Pos)
#define PERIPH_GRP0_TACH2_CLKPWR   (PERICLKPWR0_TACHO2CLKPWR_Pos)
#define PERIPH_GRP0_TACH3_CLKPWR   (PERICLKPWR0_TACHO3CLKPWR_Pos)
#define PERIPH_GRP0_PS2_CLKPWR     (PERICLKPWR0_PS2CLKPWR_Pos)
#define PERIPH_GRP0_KBM_CLKPWR     (PERICLKPWR0_KBMCLKPWR_Pos)
#define PERIPH_GRP0_PECI_CLKPWR    (PERICLKPWR0_PECICLKPWR_Pos)
#define PERIPH_GRP0_LEDPWM0_CLKPWR (PERICLKPWR0_PL0CLKPWR_Pos)
#define PERIPH_GRP0_LEDPWM1_CLKPWR (PERICLKPWR0_PL1CLKPWR_Pos)
#define PERIPH_GRP0_PWM0_CLKPWR    (PERICLKPWR0_PWM0CLKPWR_Pos)
#define PERIPH_GRP0_PWM1_CLKPWR    (PERICLKPWR0_PWM1CLKPWR_Pos)
#define PERIPH_GRP0_PWM2_CLKPWR    (PERICLKPWR0_PWM2CLKPWR_Pos)
#define PERIPH_GRP0_PWM3_CLKPWR    (PERICLKPWR0_PWM3CLKPWR_Pos)
#define PERIPH_GRP0_PWM4_CLKPWR    (PERICLKPWR0_PWM4CLKPWR_Pos)
#define PERIPH_GRP0_PWM5_CLKPWR    (PERICLKPWR0_PWM5CLKPWR_Pos)
#define PERIPH_GRP0_PWM6_CLKPWR    (PERICLKPWR0_PWM6CLKPWR_Pos)
#define PERIPH_GRP0_PWM7_CLKPWR    (PERICLKPWR0_PWM7CLKPWR_Pos)
#define PERIPH_GRP0_PWM8_CLKPWR    (PERICLKPWR0_PWM8CLKPWR_Pos)
#define PERIPH_GRP0_PWM9_CLKPWR    (PERICLKPWR0_PWM9CLKPWR_Pos)
#define PERIPH_GRP0_PWM10_CLKPWR   (PERICLKPWR0_PWM10CLKPWR_Pos)
#define PERIPH_GRP0_PWM11_CLKPWR   (PERICLKPWR0_PWM11CLKPWR_Pos)
#define PERIPH_GRP0_ESPI_CLKPWR    (PERICLKPWR0_ESPICLKPWR_Pos)
#define PERIPH_GRP0_KBC_CLKPWR     (PERICLKPWR0_KBCCLKPWR_Pos)
#define PERIPH_GRP0_ACPI_CLKPWR    (PERICLKPWR0_ACPICLKPWR_Pos)
#define PERIPH_GRP0_PMPORT0_CLKPWR (PERICLKPWR0_PMPORT0CLKPWR_Pos)
#define PERIPH_GRP0_PMPORT1_CLKPWR (PERICLKPWR0_PMPORT1CLKPWR_Pos)
#define PERIPH_GRP0_PMPORT2_CLKPWR (PERICLKPWR0_PMPORT2CLKPWR_Pos)
#define PERIPH_GRP0_PMPORT3_CLKPWR (PERICLKPWR0_PMPORT3CLKPWR_Pos)
#define PERIPH_GRP0_P80_CLKPWR     (PERICLKPWR0_P80CLKPWR_Pos)
#define PERIPH_GRP0_EMI0_CLKPWR    (PERICLKPWR0_EMI0CLKPWR_Pos)
#define PERIPH_GRP0_EMI1_CLKPWR    (PERICLKPWR0_EMI1CLKPWR_Pos)

#define PERIPH_GRP1_EMI2_CLKPWR    (PERICLKPWR1_EMI2CLKPWR_Pos)
#define PERIPH_GRP1_EMI3_CLKPWR    (PERICLKPWR1_EMI3CLKPWR_Pos)
#define PERIPH_GRP1_EMI4_CLKPWR    (PERICLKPWR1_EMI4CLKPWR_Pos)
#define PERIPH_GRP1_EMI5_CLKPWR    (PERICLKPWR1_EMI5CLKPWR_Pos)
#define PERIPH_GRP1_EMI6_CLKPWR    (PERICLKPWR1_EMI6CLKPWR_Pos)
#define PERIPH_GRP1_EMI7_CLKPWR    (PERICLKPWR1_EMI7CLKPWR_Pos)
#define PERIPH_GRP1_I3C0_CLKPWR    (PERICLKPWR1_I3C0CLKPWR_Pos)
#define PERIPH_GRP1_I3C1_CLKPWR    (PERICLKPWR1_I3C1CLKPWR_Pos)
#define PERIPH_GRP1_I2CAUTO_CLKPWR (PERICLKPWR1_I2CAUTOCLKPWR_Pos)
#define PERIPH_GRP1_MC_CLKPWR      (PERICLKPWR1_MCCLKPWR_Pos)
#define PERIPH_GRP1_TMR0_CLKPWR    (PERICLKPWR1_TMR0CLKPWR_Pos)
#define PERIPH_GRP1_TMR1_CLKPWR    (PERICLKPWR1_TMR1CLKPWR_Pos)
#define PERIPH_GRP1_TMR2_CLKPWR    (PERICLKPWR1_TMR2CLKPWR_Pos)
#define PERIPH_GRP1_TMR3_CLKPWR    (PERICLKPWR1_TMR3CLKPWR_Pos)
#define PERIPH_GRP1_TMR4_CLKPWR    (PERICLKPWR1_TMR4CLKPWR_Pos)
#define PERIPH_GRP1_TMR5_CLKPWR    (PERICLKPWR1_TMR5CLKPWR_Pos)
#define PERIPH_GRP1_RTMR_CLKPWR    (PERICLKPWR1_RTMRCLKPWR_Pos)
#define PERIPH_GRP1_SLWTMR0_CLKPWR (PERICLKPWR1_SLWTMR0CLKPWR_Pos)
#define PERIPH_GRP1_SLWTMR1_CLKPWR (PERICLKPWR1_SLWTMR1CLKPWR_Pos)

#define PERIPH_GRP2_RTC_CLKPWR       (PERICLKPWR2_RTCCLKPWR_Pos)
#define PERIPH_GRP2_WDT_CLKPWR       (PERICLKPWR2_WDTCLKPWR_Pos)
#define PERIPH_GRP2_WDTPWRBTN_CLKPWR (PERICLKPWR2_PWRBTNCLKPWR_Pos)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RTS5912_CLOCK_H_ */
