/**************************************************************************//**
 * @file efm32hg_dmareq.h
 * @brief EFM32HG_DMAREQ register and bit field definitions
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
 * @defgroup EFM32HG_DMAREQ_BitFields
 * @{
 *****************************************************************************/
#define DMAREQ_ADC0_SINGLE            ((8 << 16) + 0)  /**< DMA channel select for ADC0_SINGLE */
#define DMAREQ_ADC0_SCAN              ((8 << 16) + 1)  /**< DMA channel select for ADC0_SCAN */
#define DMAREQ_USART0_RXDATAV         ((12 << 16) + 0) /**< DMA channel select for USART0_RXDATAV */
#define DMAREQ_USART0_TXBL            ((12 << 16) + 1) /**< DMA channel select for USART0_TXBL */
#define DMAREQ_USART0_TXEMPTY         ((12 << 16) + 2) /**< DMA channel select for USART0_TXEMPTY */
#define DMAREQ_USART1_RXDATAV         ((13 << 16) + 0) /**< DMA channel select for USART1_RXDATAV */
#define DMAREQ_USART1_TXBL            ((13 << 16) + 1) /**< DMA channel select for USART1_TXBL */
#define DMAREQ_USART1_TXEMPTY         ((13 << 16) + 2) /**< DMA channel select for USART1_TXEMPTY */
#define DMAREQ_USART1_RXDATAVRIGHT    ((13 << 16) + 3) /**< DMA channel select for USART1_RXDATAVRIGHT */
#define DMAREQ_USART1_TXBLRIGHT       ((13 << 16) + 4) /**< DMA channel select for USART1_TXBLRIGHT */
#define DMAREQ_LEUART0_RXDATAV        ((16 << 16) + 0) /**< DMA channel select for LEUART0_RXDATAV */
#define DMAREQ_LEUART0_TXBL           ((16 << 16) + 1) /**< DMA channel select for LEUART0_TXBL */
#define DMAREQ_LEUART0_TXEMPTY        ((16 << 16) + 2) /**< DMA channel select for LEUART0_TXEMPTY */
#define DMAREQ_I2C0_RXDATAV           ((20 << 16) + 0) /**< DMA channel select for I2C0_RXDATAV */
#define DMAREQ_I2C0_TXBL              ((20 << 16) + 1) /**< DMA channel select for I2C0_TXBL */
#define DMAREQ_TIMER0_UFOF            ((24 << 16) + 0) /**< DMA channel select for TIMER0_UFOF */
#define DMAREQ_TIMER0_CC0             ((24 << 16) + 1) /**< DMA channel select for TIMER0_CC0 */
#define DMAREQ_TIMER0_CC1             ((24 << 16) + 2) /**< DMA channel select for TIMER0_CC1 */
#define DMAREQ_TIMER0_CC2             ((24 << 16) + 3) /**< DMA channel select for TIMER0_CC2 */
#define DMAREQ_TIMER1_UFOF            ((25 << 16) + 0) /**< DMA channel select for TIMER1_UFOF */
#define DMAREQ_TIMER1_CC0             ((25 << 16) + 1) /**< DMA channel select for TIMER1_CC0 */
#define DMAREQ_TIMER1_CC1             ((25 << 16) + 2) /**< DMA channel select for TIMER1_CC1 */
#define DMAREQ_TIMER1_CC2             ((25 << 16) + 3) /**< DMA channel select for TIMER1_CC2 */
#define DMAREQ_TIMER2_UFOF            ((26 << 16) + 0) /**< DMA channel select for TIMER2_UFOF */
#define DMAREQ_TIMER2_CC0             ((26 << 16) + 1) /**< DMA channel select for TIMER2_CC0 */
#define DMAREQ_TIMER2_CC1             ((26 << 16) + 2) /**< DMA channel select for TIMER2_CC1 */
#define DMAREQ_TIMER2_CC2             ((26 << 16) + 3) /**< DMA channel select for TIMER2_CC2 */
#define DMAREQ_MSC_WDATA              ((48 << 16) + 0) /**< DMA channel select for MSC_WDATA */
#define DMAREQ_AES_DATAWR             ((49 << 16) + 0) /**< DMA channel select for AES_DATAWR */
#define DMAREQ_AES_XORDATAWR          ((49 << 16) + 1) /**< DMA channel select for AES_XORDATAWR */
#define DMAREQ_AES_DATARD             ((49 << 16) + 2) /**< DMA channel select for AES_DATARD */
#define DMAREQ_AES_KEYWR              ((49 << 16) + 3) /**< DMA channel select for AES_KEYWR */

/** @} End of group EFM32HG_DMAREQ */
/** @} End of group Parts */

