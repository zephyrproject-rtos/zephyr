/**************************************************************************//**
 * @file efm32hg_af_pins.h
 * @brief EFM32HG_AF_PINS register and bit field definitions
 * @version 5.1.2
 ******************************************************************************
 * @section License
 * <b>Copyright 2017 Silicon Laboratories, Inc. http://www.silabs.com</b>
 ******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.@n
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.@n
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Laboratories, Inc.
 * has no obligation to support this Software. Silicon Laboratories, Inc. is
 * providing the Software "AS IS", with no express or implied warranties of any
 * kind, including, but not limited to, any implied warranties of
 * merchantability or fitness for any particular purpose or warranties against
 * infringement of any proprietary rights of a third party.
 *
 * Silicon Laboratories, Inc. will not be liable for any consequential,
 * incidental, or special damages, or any other relief, or for any claim by
 * any third party, arising from your use of this Software.
 *
 *****************************************************************************/
/**************************************************************************//**
* @addtogroup Parts
* @{
******************************************************************************/
/**************************************************************************//**
 * @defgroup EFM32HG_AF_Pins
 * @{
 *****************************************************************************/

/** AF pin number for location number i */
#define AF_USB_DMPU_PIN(i)        ((i) == 0 ? 0 :  -1)
#define AF_CMU_CLK0_PIN(i)        ((i) == 0 ? 2 : (i) == 1 ? -1 : (i) == 2 ? 7 : (i) == 3 ? 2 :  -1)
#define AF_CMU_CLK1_PIN(i)        ((i) == 0 ? 1 : (i) == 1 ? -1 : (i) == 2 ? 12 : (i) == 3 ? 11 :  -1)
#define AF_TIMER0_CC0_PIN(i)      ((i) == 0 ? 0 : (i) == 1 ? 0 : (i) == 2 ? -1 : (i) == 3 ? -1 : (i) == 4 ? 0 : (i) == 5 ? 0 : (i) == 6 ? 1 :  -1)
#define AF_TIMER0_CC1_PIN(i)      ((i) == 0 ? 1 : (i) == 1 ? 1 : (i) == 2 ? -1 : (i) == 3 ? -1 : (i) == 4 ? 0 : (i) == 5 ? 1 : (i) == 6 ? 0 :  -1)
#define AF_TIMER0_CC2_PIN(i)      ((i) == 0 ? 2 : (i) == 1 ? 2 : (i) == 2 ? -1 : (i) == 3 ? -1 : (i) == 4 ? 1 : (i) == 5 ? 2 : (i) == 6 ? 2 :  -1)
#define AF_TIMER0_CDTI0_PIN(i)    ((i) == 0 ? -1 : (i) == 1 ? 13 : (i) == 2 ? -1 : (i) == 3 ? -1 : (i) == 4 ? 2 : (i) == 5 ? 3 : (i) == 6 ? 13 :  -1)
#define AF_TIMER0_CDTI1_PIN(i)    ((i) == 0 ? -1 : (i) == 1 ? 14 : (i) == 2 ? -1 : (i) == 3 ? -1 : (i) == 4 ? 3 : (i) == 5 ? 4 : (i) == 6 ? 14 :  -1)
#define AF_TIMER0_CDTI2_PIN(i)    ((i) == 0 ? -1 : (i) == 1 ? 15 : (i) == 2 ? -1 : (i) == 3 ? -1 : (i) == 4 ? 4 : (i) == 5 ? 5 : (i) == 6 ? 15 :  -1)
#define AF_TIMER1_CC0_PIN(i)      ((i) == 0 ? 13 : (i) == 1 ? 10 : (i) == 2 ? -1 : (i) == 3 ? 7 : (i) == 4 ? 6 :  -1)
#define AF_TIMER1_CC1_PIN(i)      ((i) == 0 ? 14 : (i) == 1 ? 11 : (i) == 2 ? -1 : (i) == 3 ? 8 : (i) == 4 ? 7 :  -1)
#define AF_TIMER1_CC2_PIN(i)      ((i) == 0 ? 15 : (i) == 1 ? 12 : (i) == 2 ? -1 : (i) == 3 ? 11 : (i) == 4 ? 13 :  -1)
#define AF_TIMER1_CDTI0_PIN(i)    (-1)
#define AF_TIMER1_CDTI1_PIN(i)    (-1)
#define AF_TIMER1_CDTI2_PIN(i)    (-1)
#define AF_TIMER2_CC0_PIN(i)      ((i) == 0 ? 8 : (i) == 1 ? -1 : (i) == 2 ? 8 : (i) == 3 ? 2 :  -1)
#define AF_TIMER2_CC1_PIN(i)      ((i) == 0 ? 9 : (i) == 1 ? -1 : (i) == 2 ? 9 : (i) == 3 ? 12 :  -1)
#define AF_TIMER2_CC2_PIN(i)      ((i) == 0 ? 10 : (i) == 1 ? -1 : (i) == 2 ? 10 : (i) == 3 ? 13 :  -1)
#define AF_TIMER2_CDTI0_PIN(i)    (-1)
#define AF_TIMER2_CDTI1_PIN(i)    (-1)
#define AF_TIMER2_CDTI2_PIN(i)    (-1)
#define AF_ACMP0_OUT_PIN(i)       ((i) == 0 ? 13 : (i) == 1 ? -1 : (i) == 2 ? 6 : (i) == 3 ? 11 :  -1)
#define AF_USART0_TX_PIN(i)       ((i) == 0 ? 10 : (i) == 1 ? -1 : (i) == 2 ? 11 : (i) == 3 ? 13 : (i) == 4 ? 7 : (i) == 5 ? 0 : (i) == 6 ? 0 :  -1)
#define AF_USART0_RX_PIN(i)       ((i) == 0 ? 11 : (i) == 1 ? -1 : (i) == 2 ? 10 : (i) == 3 ? 12 : (i) == 4 ? 8 : (i) == 5 ? 1 : (i) == 6 ? 1 :  -1)
#define AF_USART0_CLK_PIN(i)      ((i) == 0 ? 12 : (i) == 1 ? -1 : (i) == 2 ? 9 : (i) == 3 ? 15 : (i) == 4 ? 13 : (i) == 5 ? 13 : (i) == 6 ? 12 :  -1)
#define AF_USART0_CS_PIN(i)       ((i) == 0 ? 13 : (i) == 1 ? -1 : (i) == 2 ? 8 : (i) == 3 ? 14 : (i) == 4 ? 14 : (i) == 5 ? 14 : (i) == 6 ? 13 :  -1)
#define AF_USART1_TX_PIN(i)       ((i) == 0 ? 0 : (i) == 1 ? -1 : (i) == 2 ? 7 : (i) == 3 ? 7 : (i) == 4 ? 2 : (i) == 5 ? 1 :  -1)
#define AF_USART1_RX_PIN(i)       ((i) == 0 ? 1 : (i) == 1 ? -1 : (i) == 2 ? 6 : (i) == 3 ? 6 : (i) == 4 ? 0 : (i) == 5 ? 2 :  -1)
#define AF_USART1_CLK_PIN(i)      ((i) == 0 ? 7 : (i) == 1 ? -1 : (i) == 2 ? 0 : (i) == 3 ? 15 : (i) == 4 ? 11 : (i) == 5 ? 3 :  -1)
#define AF_USART1_CS_PIN(i)       ((i) == 0 ? 8 : (i) == 1 ? -1 : (i) == 2 ? 1 : (i) == 3 ? 14 : (i) == 4 ? 14 : (i) == 5 ? 0 :  -1)
#define AF_PRS_CH0_PIN(i)         ((i) == 0 ? 0 : (i) == 1 ? 3 : (i) == 2 ? 14 : (i) == 3 ? 2 :  -1)
#define AF_PRS_CH1_PIN(i)         ((i) == 0 ? 1 : (i) == 1 ? 4 : (i) == 2 ? 15 : (i) == 3 ? 12 :  -1)
#define AF_PRS_CH2_PIN(i)         ((i) == 0 ? 0 : (i) == 1 ? 5 : (i) == 2 ? 10 : (i) == 3 ? 13 :  -1)
#define AF_PRS_CH3_PIN(i)         ((i) == 0 ? 1 : (i) == 1 ? -1 : (i) == 2 ? 11 : (i) == 3 ? 0 :  -1)
#define AF_LEUART0_TX_PIN(i)      ((i) == 0 ? 4 : (i) == 1 ? 13 : (i) == 2 ? -1 : (i) == 3 ? 0 : (i) == 4 ? 2 : (i) == 5 ? 14 :  -1)
#define AF_LEUART0_RX_PIN(i)      ((i) == 0 ? 5 : (i) == 1 ? 14 : (i) == 2 ? -1 : (i) == 3 ? 1 : (i) == 4 ? 0 : (i) == 5 ? 15 :  -1)
#define AF_PCNT0_S0IN_PIN(i)      ((i) == 0 ? 13 : (i) == 1 ? -1 : (i) == 2 ? 0 : (i) == 3 ? 6 : (i) == 4 ? 0 :  -1)
#define AF_PCNT0_S1IN_PIN(i)      ((i) == 0 ? 14 : (i) == 1 ? -1 : (i) == 2 ? 1 : (i) == 3 ? 7 : (i) == 4 ? 11 :  -1)
#define AF_I2C0_SDA_PIN(i)        ((i) == 0 ? 0 : (i) == 1 ? 6 : (i) == 2 ? -1 : (i) == 3 ? -1 : (i) == 4 ? 0 : (i) == 5 ? 0 : (i) == 6 ? 12 :  -1)
#define AF_I2C0_SCL_PIN(i)        ((i) == 0 ? 1 : (i) == 1 ? 7 : (i) == 2 ? -1 : (i) == 3 ? -1 : (i) == 4 ? 1 : (i) == 5 ? 1 : (i) == 6 ? 13 :  -1)
#define AF_DBG_SWDIO_PIN(i)       ((i) == 0 ? 1 :  -1)
#define AF_DBG_SWCLK_PIN(i)       ((i) == 0 ? 0 :  -1)

/** @} End of group EFM32HG_AF_Pins */
/** @} End of group Parts */

