/***************************************************************************//**
 * @file em_usart.c
 * @brief Universal synchronous/asynchronous receiver/transmitter (USART/UART)
 *   Peripheral API
 * @version 5.1.2
 *******************************************************************************
 * @section License
 * <b>Copyright 2016 Silicon Laboratories, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/

#include "em_usart.h"
#if defined(USART_COUNT) && (USART_COUNT > 0)

#include "em_cmu.h"
#include "em_bus.h"
#include "em_assert.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup USART
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */


/** Validation of USART register block pointer reference for assert statements. */
#if (USART_COUNT == 1) && defined(USART0)
#define USART_REF_VALID(ref)    ((ref) == USART0)

#elif (USART_COUNT == 1) && defined(USART1)
#define USART_REF_VALID(ref)    ((ref) == USART1)

#elif (USART_COUNT == 2) && defined(USART2)
#define USART_REF_VALID(ref)    (((ref) == USART1) || ((ref) == USART2))

#elif (USART_COUNT == 2)
#define USART_REF_VALID(ref)    (((ref) == USART0) || ((ref) == USART1))

#elif (USART_COUNT == 3)
#define USART_REF_VALID(ref)    (((ref) == USART0) || ((ref) == USART1) || \
                                 ((ref) == USART2))
#elif (USART_COUNT == 4)
#define USART_REF_VALID(ref)    (((ref) == USART0) || ((ref) == USART1) || \
                                 ((ref) == USART2) || ((ref) == USART3))
#elif (USART_COUNT == 5)
#define USART_REF_VALID(ref)    (((ref) == USART0) || ((ref) == USART1) || \
                                 ((ref) == USART2) || ((ref) == USART3) || \
                                 ((ref) == USART4))
#elif (USART_COUNT == 6)
#define USART_REF_VALID(ref)    (((ref) == USART0) || ((ref) == USART1) || \
                                 ((ref) == USART2) || ((ref) == USART3) || \
                                 ((ref) == USART4) || ((ref) == USART5))
#else
#error "Undefined number of USARTs."
#endif

#if defined(USARTRF_COUNT) && (USARTRF_COUNT > 0)
#if (USARTRF_COUNT == 1) && defined(USARTRF0)
#define USARTRF_REF_VALID(ref)  ((ref) == USARTRF0)
#elif (USARTRF_COUNT == 1) && defined(USARTRF1)
#define USARTRF_REF_VALID(ref)  ((ref) == USARTRF1)
#else
#define USARTRF_REF_VALID(ref)  (0)
#endif
#else
#define USARTRF_REF_VALID(ref)  (0)
#endif

#if defined(_EZR32_HAPPY_FAMILY)
#define USART_IRDA_VALID(ref)    ((ref) == USART0)
#elif defined(_EFM32_HAPPY_FAMILY)
#define USART_IRDA_VALID(ref)    (((ref) == USART0) || ((ref) == USART1))
#elif defined(USART0)
#define USART_IRDA_VALID(ref)    ((ref) == USART0)
#elif (USART_COUNT == 1) && defined(USART1)
#define USART_IRDA_VALID(ref)    ((ref) == USART1)
#else
#define USART_IRDA_VALID(ref)    (0)
#endif

#if defined(_SILICON_LABS_32B_SERIES_1)
  #define USART_I2S_VALID(ref)    ((ref) == USART1)
#elif defined(_SILICON_LABS_32B_SERIES_0)
  #if defined(_EZR32_HAPPY_FAMILY)
  #define USART_I2S_VALID(ref)    ((ref) == USART0)
  #elif defined(_EFM32_HAPPY_FAMILY)
  #define USART_I2S_VALID(ref)    (((ref) == USART0) || ((ref) == USART1))
  #elif defined(_EFM32_TINY_FAMILY) || defined(_EFM32_ZERO_FAMILY)
  #define USART_I2S_VALID(ref)    ((ref) == USART1)
  #elif defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_WONDER_FAMILY)
  #define USART_I2S_VALID(ref)    (((ref) == USART1) || ((ref) == USART2))
#endif
#endif

#if (UART_COUNT == 1)
#define UART_REF_VALID(ref)    ((ref) == UART0)
#elif (UART_COUNT == 2)
#define UART_REF_VALID(ref)    (((ref) == UART0) || ((ref) == UART1))
#else
#define UART_REF_VALID(ref)    (0)
#endif

/** @endcond */


/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Configure USART/UART operating in asynchronous mode to use a given
 *   baudrate (or as close as possible to specified baudrate).
 *
 * @param[in] usart
 *   Pointer to USART/UART peripheral register block.
 *
 * @param[in] refFreq
 *   USART/UART reference clock frequency in Hz that will be used. If set to 0,
 *   the currently configured reference clock is assumed.
 *
 * @param[in] baudrate
 *   Baudrate to try to achieve for USART/UART.
 *
 * @param[in] ovs
 *   Oversampling to be used. Normal is 16x oversampling, but lower oversampling
 *   may be used to achieve higher rates or better baudrate accuracy in some
 *   cases. Notice that lower oversampling frequency makes channel more
 *   vulnerable to bit faults during reception due to clock inaccuracies
 *   compared to link partner.
 ******************************************************************************/
void USART_BaudrateAsyncSet(USART_TypeDef *usart,
                            uint32_t refFreq,
                            uint32_t baudrate,
                            USART_OVS_TypeDef ovs)
{
  uint32_t clkdiv;
  uint32_t oversample;

  /* Inhibit divide by 0 */
  EFM_ASSERT(baudrate);

  /*
   * We want to use integer division to avoid forcing in float division
   * utils, and yet keep rounding effect errors to a minimum.
   *
   * CLKDIV in asynchronous mode is given by:
   *
   * CLKDIV = 256 * (fHFPERCLK/(oversample * br) - 1)
   * or
   * CLKDIV = (256 * fHFPERCLK)/(oversample * br) - 256
   *
   * The basic problem with integer division in the above formula is that
   * the dividend (256 * fHFPERCLK) may become higher than max 32 bit
   * integer. Yet, we want to evaluate dividend first before dividing in
   * order to get as small rounding effects as possible. We do not want
   * to make too harsh restrictions on max fHFPERCLK value either.
   *
   * One can possibly factorize 256 and oversample/br. However,
   * since the last 6 or 3 bits of CLKDIV are don't care, we can base our
   * integer arithmetic on the below formula
   *
   * CLKDIV / 64 = (4 * fHFPERCLK)/(oversample * br) - 4 (3 bits dont care)
   * or
   * CLKDIV / 8  = (32 * fHFPERCLK)/(oversample * br) - 32 (6 bits dont care)
   *
   * and calculate 1/64 of CLKDIV first. This allows for fHFPERCLK
   * up to 1GHz without overflowing a 32 bit value!
   */

  /* HFPERCLK used to clock all USART/UART peripheral modules */
  if (!refFreq)
  {
    refFreq = CMU_ClockFreqGet(cmuClock_HFPER);
  }

  /* Map oversampling */
  switch (ovs)
  {
    case usartOVS16:
      EFM_ASSERT(baudrate <= (refFreq / 16));
      oversample = 16;
      break;

    case usartOVS8:
      EFM_ASSERT(baudrate <= (refFreq / 8));
      oversample = 8;
      break;

    case usartOVS6:
      EFM_ASSERT(baudrate <= (refFreq / 6));
      oversample = 6;
      break;

    case usartOVS4:
      EFM_ASSERT(baudrate <= (refFreq / 4));
      oversample = 4;
      break;

    default:
      /* Invalid input */
      EFM_ASSERT(0);
      return;
  }

  /* Calculate and set CLKDIV with fractional bits.
   * The addend (oversample*baudrate)/2 in the first line is to round the
   * divisor up by half the divisor before the division in order to reduce the
   * integer division error, which consequently results in a higher baudrate
   * than desired. */
#if defined(_USART_CLKDIV_DIV_MASK) && (_USART_CLKDIV_DIV_MASK >= 0x7FFFF8UL)
  clkdiv  = 32 * refFreq + (oversample * baudrate) / 2;
  clkdiv /= (oversample * baudrate);
  clkdiv -= 32;
  clkdiv *= 8;
#else
  clkdiv  = 4 * refFreq + (oversample * baudrate) / 2;
  clkdiv /= (oversample * baudrate);
  clkdiv -= 4;
  clkdiv *= 64;
#endif

  /* Verify that resulting clock divider is within limits */
  EFM_ASSERT(clkdiv <= _USART_CLKDIV_DIV_MASK);

  /* If EFM_ASSERT is not enabled, make sure we don't write to reserved bits */
  clkdiv &= _USART_CLKDIV_DIV_MASK;

  usart->CTRL  &= ~_USART_CTRL_OVS_MASK;
  usart->CTRL  |= ovs;
  usart->CLKDIV = clkdiv;
}


/***************************************************************************//**
 * @brief
 *   Calculate baudrate for USART/UART given reference frequency, clock division
 *   and oversampling rate (if async mode).
 *
 * @details
 *   This function returns the baudrate that a USART/UART module will use if
 *   configured with the given frequency, clock divisor and mode. Notice that
 *   this function will not use actual HW configuration. It can be used
 *   to determinate if a given configuration is sufficiently accurate for the
 *   application.
 *
 * @param[in] refFreq
 *   USART/UART HF peripheral frequency used.
 *
 * @param[in] clkdiv
 *   Clock division factor to be used.
 *
 * @param[in] syncmode
 *   @li true - synchronous mode operation.
 *   @li false - asynchronous mode operation.
 *
 * @param[in] ovs
 *   Oversampling used if asynchronous mode. Not used if @p syncmode is true.
 *
 * @return
 *   Baudrate with given settings.
 ******************************************************************************/
uint32_t USART_BaudrateCalc(uint32_t refFreq,
                            uint32_t clkdiv,
                            bool syncmode,
                            USART_OVS_TypeDef ovs)
{
  uint32_t oversample;
  uint64_t divisor;
  uint64_t factor;
  uint64_t remainder;
  uint64_t quotient;
  uint32_t br;

  /* Out of bound clkdiv ? */
  EFM_ASSERT(clkdiv <= _USART_CLKDIV_DIV_MASK);

  /* Mask out unused bits */
  clkdiv &= _USART_CLKDIV_DIV_MASK;

  /* We want to use integer division to avoid forcing in float division */
  /* utils, and yet keep rounding effect errors to a minimum. */

  /* Baudrate calculation depends on if synchronous or asynchronous mode */
  if (syncmode)
  {
    /*
     * Baudrate is given by:
     *
     * br = fHFPERCLK/(2 * (1 + (CLKDIV / 256)))
     *
     * which can be rewritten to
     *
     * br = (128 * fHFPERCLK)/(256 + CLKDIV)
     */
    oversample = 1; /* Not used in sync mode, ie 1 */
    factor     = 128;
  }
  else
  {
    /*
     * Baudrate in asynchronous mode is given by:
     *
     * br = fHFPERCLK/(oversample * (1 + (CLKDIV / 256)))
     *
     * which can be rewritten to
     *
     * br = (256 * fHFPERCLK)/(oversample * (256 + CLKDIV))
     *
     * First of all we can reduce the 256 factor of the dividend with
     * (part of) oversample part of the divisor.
     */

    switch (ovs)
    {
      case usartOVS16:
        oversample = 1;
        factor     = 256 / 16;
        break;

      case usartOVS8:
        oversample = 1;
        factor     = 256 / 8;
        break;

      case usartOVS6:
        oversample = 3;
        factor     = 256 / 2;
        break;

      default:
        oversample = 1;
        factor     = 256 / 4;
        break;
    }
  }

  /*
   * The basic problem with integer division in the above formula is that
   * the dividend (factor * fHFPERCLK) may become larger than a 32 bit
   * integer. Yet we want to evaluate dividend first before dividing in
   * order to get as small rounding effects as possible. We do not want
   * to make too harsh restrictions on max fHFPERCLK value either.
   *
   * For division a/b, we can write
   *
   * a = qb + r
   *
   * where q is the quotient and r is the remainder, both integers.
   *
   * The orignal baudrate formula can be rewritten as
   *
   * br = xa / b = x(qb + r)/b = xq + xr/b
   *
   * where x is 'factor', a is 'refFreq' and b is 'divisor', referring to
   * variable names.
   */

  /*
   * Divisor will never exceed max 32 bit value since
   * clkdiv <= _USART_CLKDIV_DIV_MASK (currently 0x1FFFC0 or 0x7FFFF8)
   * and 'oversample' has been reduced to <= 3.
   */
  divisor = oversample * (256 + clkdiv);

  quotient  = refFreq / divisor;
  remainder = refFreq % divisor;

  /* factor <= 128 and since divisor >= 256, the below cannot exceed max */
  /* 32 bit value. However, factor * remainder can become larger than 32-bit */
  /* because of the size of _USART_CLKDIV_DIV_MASK on some families. */
  br = (uint32_t)(factor * quotient);

  /*
   * factor <= 128 and remainder < (oversample*(256 + clkdiv)), which
   * means dividend (factor * remainder) worst case is
   * 128 * (3 * (256 + _USART_CLKDIV_DIV_MASK)) = 0x1_8001_7400.
   */
  br += (uint32_t)((factor * remainder) / divisor);

  return br;
}


/***************************************************************************//**
 * @brief
 *   Get current baudrate for USART/UART.
 *
 * @details
 *   This function returns the actual baudrate (not considering oscillator
 *   inaccuracies) used by a USART/UART peripheral.
 *
 * @param[in] usart
 *   Pointer to USART/UART peripheral register block.
 *
 * @return
 *   Current baudrate.
 ******************************************************************************/
uint32_t USART_BaudrateGet(USART_TypeDef *usart)
{
  uint32_t          freq;
  USART_OVS_TypeDef ovs;
  bool              syncmode;

  if (usart->CTRL & USART_CTRL_SYNC)
  {
    syncmode = true;
  }
  else
  {
    syncmode = false;
  }

  /* HFPERCLK used to clock all USART/UART peripheral modules */
  freq = CMU_ClockFreqGet(cmuClock_HFPER);
  ovs  = (USART_OVS_TypeDef)(usart->CTRL & _USART_CTRL_OVS_MASK);
  return USART_BaudrateCalc(freq, usart->CLKDIV, syncmode, ovs);
}


/***************************************************************************//**
 * @brief
 *   Configure USART operating in synchronous mode to use a given baudrate
 *   (or as close as possible to specified baudrate).
 *
 * @details
 *   The configuration will be set to use a baudrate <= the specified baudrate
 *   in order to ensure that the baudrate does not exceed the specified value.
 *
 *   Fractional clock division is suppressed, although the HW design allows it.
 *   It could cause half clock cycles to exceed specified limit, and thus
 *   potentially violate specifications for the slave device. In some special
 *   situations fractional clock division may be useful even in synchronous
 *   mode, but in those cases it must be directly adjusted, possibly assisted
 *   by USART_BaudrateCalc():
 *
 * @param[in] usart
 *   Pointer to USART peripheral register block. (Cannot be used on UART
 *   modules.)
 *
 * @param[in] refFreq
 *   USART reference clock frequency in Hz that will be used. If set to 0,
 *   the currently configured reference clock is assumed.
 *
 * @param[in] baudrate
 *   Baudrate to try to achieve for USART.
 ******************************************************************************/
void USART_BaudrateSyncSet(USART_TypeDef *usart, uint32_t refFreq, uint32_t baudrate)
{
  uint32_t clkdiv;

  /* Inhibit divide by 0 */
  EFM_ASSERT(baudrate);

  /*
   * CLKDIV in synchronous mode is given by:
   *
   * CLKDIV = 256 * (fHFPERCLK/(2 * br) - 1)
   */

  /* HFPERCLK used to clock all USART/UART peripheral modules */
  if (!refFreq)
  {
    refFreq = CMU_ClockFreqGet(cmuClock_HFPER);
  }

  clkdiv = (refFreq - 1) / (2 * baudrate);
  clkdiv = clkdiv << 8;

  /* Verify that resulting clock divider is within limits */
  EFM_ASSERT(!(clkdiv & ~_USART_CLKDIV_DIV_MASK));

  /* If EFM_ASSERT is not enabled, make sure we don't write to reserved bits */
  clkdiv &= _USART_CLKDIV_DIV_MASK;

  BUS_RegMaskedWrite(&usart->CLKDIV, _USART_CLKDIV_DIV_MASK, clkdiv);
}


/***************************************************************************//**
 * @brief
 *   Enable/disable USART/UART receiver and/or transmitter.
 *
 * @details
 *   Notice that this function does not do any configuration. Enabling should
 *   normally be done after initialization is done (if not enabled as part
 *   of init).
 *
 * @param[in] usart
 *   Pointer to USART/UART peripheral register block.
 *
 * @param[in] enable
 *   Select status for receiver/transmitter.
 ******************************************************************************/
void USART_Enable(USART_TypeDef *usart, USART_Enable_TypeDef enable)
{
  uint32_t tmp;

  /* Make sure the module exists on the selected chip */
  EFM_ASSERT( USART_REF_VALID(usart)
              || USARTRF_REF_VALID(usart)
              || UART_REF_VALID(usart) );

  /* Disable as specified */
  tmp        = ~((uint32_t) (enable));
  tmp       &= _USART_CMD_RXEN_MASK | _USART_CMD_TXEN_MASK;
  usart->CMD = tmp << 1;

  /* Enable as specified */
  usart->CMD = (uint32_t) (enable);
}


/***************************************************************************//**
 * @brief
 *   Init USART/UART for normal asynchronous mode.
 *
 * @details
 *   This function will configure basic settings in order to operate in normal
 *   asynchronous mode.
 *
 *   Special control setup not covered by this function must be done after
 *   using this function by direct modification of the CTRL register.
 *
 *   Notice that pins used by the USART/UART module must be properly configured
 *   by the user explicitly, in order for the USART/UART to work as intended.
 *   (When configuring pins, one should remember to consider the sequence of
 *   configuration, in order to avoid unintended pulses/glitches on output
 *   pins.)
 *
 * @param[in] usart
 *   Pointer to USART/UART peripheral register block.
 *
 * @param[in] init
 *   Pointer to initialization structure used to configure basic async setup.
 ******************************************************************************/
void USART_InitAsync(USART_TypeDef *usart, const USART_InitAsync_TypeDef *init)
{
  /* Make sure the module exists on the selected chip */
  EFM_ASSERT( USART_REF_VALID(usart)
              || USARTRF_REF_VALID(usart)
              || UART_REF_VALID(usart) );

  /* Init USART registers to HW reset state. */
  USART_Reset(usart);

#if defined(USART_INPUT_RXPRS) && defined(USART_CTRL_MVDIS)
  /* Disable majority vote if specified. */
  if (init->mvdis)
  {
    usart->CTRL |= USART_CTRL_MVDIS;
  }

  /* Configure PRS input mode. */
  if (init->prsRxEnable)
  {
    usart->INPUT = (uint32_t) init->prsRxCh | USART_INPUT_RXPRS;
  }
#endif

  /* Configure databits, stopbits and parity */
  usart->FRAME = (uint32_t)init->databits
                 | (uint32_t)init->stopbits
                 | (uint32_t)init->parity;

  /* Configure baudrate */
  USART_BaudrateAsyncSet(usart, init->refFreq, init->baudrate, init->oversampling);

#if defined(_USART_TIMING_CSHOLD_MASK)
  usart->TIMING = ((init->autoCsHold << _USART_TIMING_CSHOLD_SHIFT)
                   & _USART_TIMING_CSHOLD_MASK)
                  | ((init->autoCsSetup << _USART_TIMING_CSSETUP_SHIFT)
                     & _USART_TIMING_CSSETUP_MASK);
  if (init->autoCsEnable)
  {
    usart->CTRL |= USART_CTRL_AUTOCS;
  }
#endif
  /* Finally enable (as specified) */
  usart->CMD = (uint32_t)init->enable;
}


/***************************************************************************//**
 * @brief
 *   Init USART for synchronous mode.
 *
 * @details
 *   This function will configure basic settings in order to operate in
 *   synchronous mode.
 *
 *   Special control setup not covered by this function must be done after
 *   using this function by direct modification of the CTRL register.
 *
 *   Notice that pins used by the USART module must be properly configured
 *   by the user explicitly, in order for the USART to work as intended.
 *   (When configuring pins, one should remember to consider the sequence of
 *   configuration, in order to avoid unintended pulses/glitches on output
 *   pins.)
 *
 * @param[in] usart
 *   Pointer to USART peripheral register block. (UART does not support this
 *   mode.)
 *
 * @param[in] init
 *   Pointer to initialization structure used to configure basic async setup.
 ******************************************************************************/
void USART_InitSync(USART_TypeDef *usart, const USART_InitSync_TypeDef *init)
{
  /* Make sure the module exists on the selected chip */
  EFM_ASSERT( USART_REF_VALID(usart) || USARTRF_REF_VALID(usart) );

  /* Init USART registers to HW reset state. */
  USART_Reset(usart);

  /* Set bits for synchronous mode */
  usart->CTRL |= (USART_CTRL_SYNC)
                 | (uint32_t)init->clockMode
                 | (init->msbf ? USART_CTRL_MSBF : 0);

#if defined(_USART_CTRL_AUTOTX_MASK)
  usart->CTRL |= init->autoTx ? USART_CTRL_AUTOTX : 0;
#endif

#if defined(_USART_INPUT_RXPRS_MASK)
  /* Configure PRS input mode. */
  if (init->prsRxEnable)
  {
    usart->INPUT = (uint32_t)init->prsRxCh | USART_INPUT_RXPRS;
  }
#endif

  /* Configure databits, leave stopbits and parity at reset default (not used) */
  usart->FRAME = (uint32_t)init->databits
                 | USART_FRAME_STOPBITS_DEFAULT
                 | USART_FRAME_PARITY_DEFAULT;

  /* Configure baudrate */
  USART_BaudrateSyncSet(usart, init->refFreq, init->baudrate);

  /* Finally enable (as specified) */
  if (init->master)
  {
    usart->CMD = USART_CMD_MASTEREN;
  }

#if defined(_USART_TIMING_CSHOLD_MASK)
  usart->TIMING = ((init->autoCsHold << _USART_TIMING_CSHOLD_SHIFT)
                   & _USART_TIMING_CSHOLD_MASK)
                  | ((init->autoCsSetup << _USART_TIMING_CSSETUP_SHIFT)
                     & _USART_TIMING_CSSETUP_MASK);
  if (init->autoCsEnable)
  {
    usart->CTRL |= USART_CTRL_AUTOCS;
  }
#endif

  usart->CMD = (uint32_t)init->enable;
}

/***************************************************************************//**
 * @brief
 *   Init USART for asynchronous IrDA mode.
 *
 * @details
 *   This function will configure basic settings in order to operate in
 *   asynchronous IrDA mode.
 *
 *   Special control setup not covered by this function must be done after
 *   using this function by direct modification of the CTRL and IRCTRL
 *   registers.
 *
 *   Notice that pins used by the USART/UART module must be properly configured
 *   by the user explicitly, in order for the USART/UART to work as intended.
 *   (When configuring pins, one should remember to consider the sequence of
 *   configuration, in order to avoid unintended pulses/glitches on output
 *   pins.)
 *
 * @param[in] usart
 *   Pointer to USART peripheral register block.
 *
 * @param[in] init
 *   Pointer to initialization structure used to configure async IrDA setup.
 *
 * @note
 *   Not all USART instances support IrDA. See the datasheet for your device.
 *
 ******************************************************************************/
void USARTn_InitIrDA(USART_TypeDef *usart, const USART_InitIrDA_TypeDef *init)
{
  EFM_ASSERT(USART_IRDA_VALID(usart));

  /* Init USART as async device */
  USART_InitAsync(usart, &(init->async));

  /* Set IrDA modulation to RZI (return-to-zero-inverted) */
  usart->CTRL |= USART_CTRL_TXINV;

  /* Invert Rx signal before demodulator if enabled */
  if (init->irRxInv)
  {
    usart->CTRL |= USART_CTRL_RXINV;
  }

  /* Configure IrDA */
  usart->IRCTRL |= (uint32_t)init->irPw
                   | (uint32_t)init->irPrsSel
                   | ((uint32_t)init->irFilt << _USART_IRCTRL_IRFILT_SHIFT)
                   | ((uint32_t)init->irPrsEn << _USART_IRCTRL_IRPRSEN_SHIFT);

  /* Enable IrDA */
  usart->IRCTRL |= USART_IRCTRL_IREN;
}

#if defined(_USART_I2SCTRL_MASK)
/***************************************************************************//**
 * @brief
 *   Init USART for I2S mode.
 *
 * @details
 *   This function will configure basic settings in order to operate in I2S
 *   mode.
 *
 *   Special control setup not covered by this function must be done after
 *   using this function by direct modification of the CTRL and I2SCTRL
 *   registers.
 *
 *   Notice that pins used by the USART module must be properly configured
 *   by the user explicitly, in order for the USART to work as intended.
 *   (When configuring pins, one should remember to consider the sequence of
 *   configuration, in order to avoid unintended pulses/glitches on output
 *   pins.)
 *
 * @param[in] usart
 *   Pointer to USART peripheral register block. (UART does not support this
 *   mode.)
 *
 * @param[in] init
 *   Pointer to initialization structure used to configure basic I2S setup.
 *
 * @note
 *   This function does not apply to all USART's. Refer to chip manuals.
 *
 ******************************************************************************/
void USART_InitI2s(USART_TypeDef *usart, USART_InitI2s_TypeDef *init)
{
  USART_Enable_TypeDef enable;

  /* Make sure the module exists on the selected chip */
  EFM_ASSERT(USART_I2S_VALID(usart));

  /* Override the enable setting. */
  enable            = init->sync.enable;
  init->sync.enable = usartDisable;

  /* Init USART as a sync device. */
  USART_InitSync(usart, &init->sync);

  /* Configure and enable I2CCTRL register acording to selected mode. */
  usart->I2SCTRL = (uint32_t)init->format
                   | (uint32_t)init->justify
                   | (init->delay    ? USART_I2SCTRL_DELAY    : 0)
                   | (init->dmaSplit ? USART_I2SCTRL_DMASPLIT : 0)
                   | (init->mono     ? USART_I2SCTRL_MONO     : 0)
                   | USART_I2SCTRL_EN;

  if (enable != usartDisable)
  {
    USART_Enable(usart, enable);
  }
}
#endif


/***************************************************************************//**
 * @brief
 *   Initialize automatic transmissions using PRS channel as trigger
 * @note
 *   Initialize USART with USART_Init() before setting up PRS configuration
 *
 * @param[in] usart Pointer to USART to configure
 * @param[in] init Pointer to initialization structure
 ******************************************************************************/
void USART_InitPrsTrigger(USART_TypeDef *usart, const USART_PrsTriggerInit_TypeDef *init)
{
  uint32_t trigctrl;

  /* Clear values that will be reconfigured  */
  trigctrl = usart->TRIGCTRL & ~(_USART_TRIGCTRL_RXTEN_MASK
                                 | _USART_TRIGCTRL_TXTEN_MASK
#if defined(USART_TRIGCTRL_AUTOTXTEN)
                                 | _USART_TRIGCTRL_AUTOTXTEN_MASK
#endif
                                 | _USART_TRIGCTRL_TSEL_MASK);

#if defined(USART_TRIGCTRL_AUTOTXTEN)
  if (init->autoTxTriggerEnable)
  {
    trigctrl |= USART_TRIGCTRL_AUTOTXTEN;
  }
#endif
  if (init->txTriggerEnable)
  {
    trigctrl |= USART_TRIGCTRL_TXTEN;
  }
  if (init->rxTriggerEnable)
  {
    trigctrl |= USART_TRIGCTRL_RXTEN;
  }
  trigctrl |= init->prsTriggerChannel;

  /* Enable new configuration */
  usart->TRIGCTRL = trigctrl;
}


/***************************************************************************//**
 * @brief
 *   Reset USART/UART to same state as after a HW reset.
 *
 * @param[in] usart
 *   Pointer to USART/UART peripheral register block.
 ******************************************************************************/
void USART_Reset(USART_TypeDef *usart)
{
  /* Make sure the module exists on the selected chip */
  EFM_ASSERT( USART_REF_VALID(usart)
              || USARTRF_REF_VALID(usart)
              || UART_REF_VALID(usart) );

  /* Make sure disabled first, before resetting other registers */
  usart->CMD = USART_CMD_RXDIS | USART_CMD_TXDIS | USART_CMD_MASTERDIS
               | USART_CMD_RXBLOCKDIS | USART_CMD_TXTRIDIS | USART_CMD_CLEARTX
               | USART_CMD_CLEARRX;
  usart->CTRL      = _USART_CTRL_RESETVALUE;
  usart->FRAME     = _USART_FRAME_RESETVALUE;
  usart->TRIGCTRL  = _USART_TRIGCTRL_RESETVALUE;
  usart->CLKDIV    = _USART_CLKDIV_RESETVALUE;
  usart->IEN       = _USART_IEN_RESETVALUE;
  usart->IFC       = _USART_IFC_MASK;
#if defined(_USART_ROUTEPEN_MASK) || defined(_UART_ROUTEPEN_MASK)
  usart->ROUTEPEN  = _USART_ROUTEPEN_RESETVALUE;
  usart->ROUTELOC0 = _USART_ROUTELOC0_RESETVALUE;
  usart->ROUTELOC1 = _USART_ROUTELOC1_RESETVALUE;
#else
  usart->ROUTE     = _USART_ROUTE_RESETVALUE;
#endif

  if (USART_IRDA_VALID(usart))
  {
    usart->IRCTRL = _USART_IRCTRL_RESETVALUE;
  }

#if defined(_USART_INPUT_RESETVALUE)
  usart->INPUT = _USART_INPUT_RESETVALUE;
#endif

#if defined(_USART_I2SCTRL_RESETVALUE)
  if (USART_I2S_VALID(usart))
  {
    usart->I2SCTRL = _USART_I2SCTRL_RESETVALUE;
  }
#endif
}


/***************************************************************************//**
 * @brief
 *   Receive one 4-8 bit frame, (or part of 10-16 bit frame).
 *
 * @details
 *   This function is normally used to receive one frame when operating with
 *   frame length 4-8 bits. Please refer to @ref USART_RxExt() for reception of
 *   9 bit frames.
 *
 *   Notice that possible parity/stop bits in asynchronous mode are not
 *   considered part of specified frame bit length.
 *
 * @note
 *   This function will stall if the buffer is empty, until data is received.
 *   Alternatively the user can explicitly check whether data is available, and
 *   if data is avaliable, call @ref USART_RxDataGet() to read the RXDATA
 *   register directly.
 *
 * @param[in] usart
 *   Pointer to USART/UART peripheral register block.
 *
 * @return
 *   Data received.
 ******************************************************************************/
uint8_t USART_Rx(USART_TypeDef *usart)
{
  while (!(usart->STATUS & USART_STATUS_RXDATAV))
    ;

  return (uint8_t)usart->RXDATA;
}


/***************************************************************************//**
 * @brief
 *   Receive two 4-8 bit frames, or one 10-16 bit frame.
 *
 * @details
 *   This function is normally used to receive one frame when operating with
 *   frame length 10-16 bits. Please refer to @ref USART_RxDoubleExt() for
 *   reception of two 9 bit frames.
 *
 *   Notice that possible parity/stop bits in asynchronous mode are not
 *   considered part of specified frame bit length.
 *
 * @note
 *   This function will stall if buffer is empty, until data is received.
 *   Alternatively the user can explicitly check whether data is available, and
 *   if data is avaliable, call @ref USART_RxDoubleGet() to read the RXDOUBLE
 *   register directly.
 *
 * @param[in] usart
 *   Pointer to USART/UART peripheral register block.
 *
 * @return
 *   Data received.
 ******************************************************************************/
uint16_t USART_RxDouble(USART_TypeDef *usart)
{
  while (!(usart->STATUS & USART_STATUS_RXFULL))
    ;

  return (uint16_t)usart->RXDOUBLE;
}


/***************************************************************************//**
 * @brief
 *   Receive two 4-9 bit frames, or one 10-16 bit frame with extended
 *   information.
 *
 * @details
 *   This function is normally used to receive one frame when operating with
 *   frame length 10-16 bits and additional RX status information is required.
 *
 *   Notice that possible parity/stop bits in asynchronous mode are not
 *   considered part of specified frame bit length.
 *
 * @note
 *   This function will stall if buffer is empty, until data is received.
 *   Alternatively the user can explicitly check whether data is available, and
 *   if data is avaliable, call @ref USART_RxDoubleXGet() to read the RXDOUBLEX
 *   register directly.
 *
 * @param[in] usart
 *   Pointer to USART/UART peripheral register block.
 *
 * @return
 *   Data received.
 ******************************************************************************/
uint32_t USART_RxDoubleExt(USART_TypeDef *usart)
{
  while (!(usart->STATUS & USART_STATUS_RXFULL))
    ;

  return usart->RXDOUBLEX;
}


/***************************************************************************//**
 * @brief
 *   Receive one 4-9 bit frame, (or part of 10-16 bit frame) with extended
 *   information.
 *
 * @details
 *   This function is normally used to receive one frame when operating with
 *   frame length 4-9 bits and additional RX status information is required.
 *
 *   Notice that possible parity/stop bits in asynchronous mode are not
 *   considered part of specified frame bit length.
 *
 * @note
 *   This function will stall if buffer is empty, until data is received.
 *   Alternatively the user can explicitly check whether data is available, and
 *   if data is avaliable, call @ref USART_RxDataXGet() to read the RXDATAX
 *   register directly.
 *
 * @param[in] usart
 *   Pointer to USART/UART peripheral register block.
 *
 * @return
 *   Data received.
 ******************************************************************************/
uint16_t USART_RxExt(USART_TypeDef *usart)
{
  while (!(usart->STATUS & USART_STATUS_RXDATAV))
    ;

  return (uint16_t)usart->RXDATAX;
}


/***************************************************************************//**
 * @brief
 *   Perform one 8 bit frame SPI transfer.
 *
 * @note
 *   This function will stall if the transmit buffer is full. When a transmit
 *   buffer becomes available, data is written and the function will wait until
 *   the data is fully transmitted. The SPI return value is then read out and
 *   returned.
 *
 * @param[in] usart
 *   Pointer to USART peripheral register block.
 *
 * @param[in] data
 *   Data to transmit.
 *
 * @return
 *   Data received.
 ******************************************************************************/
uint8_t USART_SpiTransfer(USART_TypeDef *usart, uint8_t data)
{
  while (!(usart->STATUS & USART_STATUS_TXBL))
    ;
  usart->TXDATA = (uint32_t)data;
  while (!(usart->STATUS & USART_STATUS_TXC))
    ;
  return (uint8_t)usart->RXDATA;
}


/***************************************************************************//**
 * @brief
 *   Transmit one 4-9 bit frame.
 *
 * @details
 *   Depending on frame length configuration, 4-8 (least significant) bits from
 *   @p data are transmitted. If frame length is 9, 8 bits are transmitted from
 *   @p data and one bit as specified by CTRL register, BIT8DV field. Please
 *   refer to USART_TxExt() for transmitting 9 bit frame with full control of
 *   all 9 bits.
 *
 *   Notice that possible parity/stop bits in asynchronous mode are not
 *   considered part of specified frame bit length.
 *
 * @note
 *   This function will stall if buffer is full, until buffer becomes available.
 *
 * @param[in] usart
 *   Pointer to USART/UART peripheral register block.
 *
 * @param[in] data
 *   Data to transmit. See details above for further info.
 ******************************************************************************/
void USART_Tx(USART_TypeDef *usart, uint8_t data)
{
  /* Check that transmit buffer is empty */
  while (!(usart->STATUS & USART_STATUS_TXBL))
    ;
  usart->TXDATA = (uint32_t)data;
}


/***************************************************************************//**
 * @brief
 *   Transmit two 4-9 bit frames, or one 10-16 bit frame.
 *
 * @details
 *   Depending on frame length configuration, 4-8 (least significant) bits from
 *   each byte in @p data are transmitted. If frame length is 9, 8 bits are
 *   transmitted from each byte in @p data adding one bit as specified by CTRL
 *   register, BIT8DV field, to each byte. Please refer to USART_TxDoubleExt()
 *   for transmitting two 9 bit frames with full control of all 9 bits.
 *
 *   If frame length is 10-16, 10-16 (least significant) bits from @p data
 *   are transmitted.
 *
 *   Notice that possible parity/stop bits in asynchronous mode are not
 *   considered part of specified frame bit length.
 *
 * @note
 *   This function will stall if buffer is full, until buffer becomes available.
 *
 * @param[in] usart
 *   Pointer to USART/UART peripheral register block.
 *
 * @param[in] data
 *   Data to transmit, the least significant byte holds the frame transmitted
 *   first. See details above for further info.
 ******************************************************************************/
void USART_TxDouble(USART_TypeDef *usart, uint16_t data)
{
  /* Check that transmit buffer is empty */
  while (!(usart->STATUS & USART_STATUS_TXBL))
    ;
  usart->TXDOUBLE = (uint32_t)data;
}


/***************************************************************************//**
 * @brief
 *   Transmit two 4-9 bit frames, or one 10-16 bit frame with extended control.
 *
 * @details
 *   Notice that possible parity/stop bits in asynchronous mode are not
 *   considered part of specified frame bit length.
 *
 * @note
 *   This function will stall if buffer is full, until buffer becomes available.
 *
 * @param[in] usart
 *   Pointer to USART/UART peripheral register block.
 *
 * @param[in] data
 *   Data to transmit with extended control. Contains two 16 bit words
 *   concatenated. Least significant word holds frame transitted first. If frame
 *   length is 4-9, two frames with 4-9 least significant bits from each 16 bit
 *   word are transmitted.
 * @par
 *   If frame length is 10-16 bits, 8 data bits are taken from the least
 *   significant 16 bit word, and the remaining bits from the other 16 bit word.
 * @par
 *   Additional control bits are available as documented in the reference
 *   manual (set to 0 if not used). For 10-16 bit frame length, these control
 *   bits are taken from the most significant 16 bit word.
 ******************************************************************************/
void USART_TxDoubleExt(USART_TypeDef *usart, uint32_t data)
{
  /* Check that transmit buffer is empty */
  while (!(usart->STATUS & USART_STATUS_TXBL))
    ;
  usart->TXDOUBLEX = data;
}


/***************************************************************************//**
 * @brief
 *   Transmit one 4-9 bit frame with extended control.
 *
 * @details
 *   Notice that possible parity/stop bits in asynchronous mode are not
 *   considered part of specified frame bit length.
 *
 * @note
 *   This function will stall if buffer is full, until buffer becomes available.
 *
 * @param[in] usart
 *   Pointer to USART/UART peripheral register block.
 *
 * @param[in] data
 *   Data to transmit with extended control. Least significant bits contains
 *   frame bits, and additional control bits are available as documented in
 *   the reference manual (set to 0 if not used).
 ******************************************************************************/
void USART_TxExt(USART_TypeDef *usart, uint16_t data)
{
  /* Check that transmit buffer is empty */
  while (!(usart->STATUS & USART_STATUS_TXBL))
    ;
  usart->TXDATAX = (uint32_t)data;
}


/** @} (end addtogroup USART) */
/** @} (end addtogroup emlib) */
#endif /* defined(USART_COUNT) && (USART_COUNT > 0) */
