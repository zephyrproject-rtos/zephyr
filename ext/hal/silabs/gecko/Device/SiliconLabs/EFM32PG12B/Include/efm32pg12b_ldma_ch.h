/**************************************************************************//**
 * @file efm32pg12b_ldma_ch.h
 * @brief EFM32PG12B_LDMA_CH register and bit field definitions
 * @version 5.6.0
 ******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories, Inc. www.silabs.com</b>
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

#if defined(__ICCARM__)
#pragma system_include       /* Treat file as system include file. */
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang system_header  /* Treat file as system include file. */
#endif

/**************************************************************************//**
* @addtogroup Parts
* @{
******************************************************************************/
/**************************************************************************//**
 * @brief LDMA_CH LDMA CH Register
 * @ingroup EFM32PG12B_LDMA
 *****************************************************************************/
typedef struct {
  __IOM uint32_t REQSEL;        /**< Channel Peripheral Request Select Register  */
  __IOM uint32_t CFG;           /**< Channel Configuration Register  */
  __IOM uint32_t LOOP;          /**< Channel Loop Counter Register  */
  __IOM uint32_t CTRL;          /**< Channel Descriptor Control Word Register  */
  __IOM uint32_t SRC;           /**< Channel Descriptor Source Data Address Register  */
  __IOM uint32_t DST;           /**< Channel Descriptor Destination Data Address Register  */
  __IOM uint32_t LINK;          /**< Channel Descriptor Link Structure Address Register  */
  uint32_t       RESERVED0[5U]; /**< Reserved future */
} LDMA_CH_TypeDef;

/** @} End of group Parts */
