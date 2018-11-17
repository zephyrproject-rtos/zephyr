/***************************************************************************//**
 * @file em_leuart.c
 * @brief Low Energy Universal Asynchronous Receiver/Transmitter (LEUART)
 *   Peripheral API
 * @version 5.6.0
 *******************************************************************************
 * # License
 * <b>Copyright 2016 Silicon Laboratories, Inc. www.silabs.com</b>
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

#include "em_leuart.h"
#if defined(LEUART_COUNT) && (LEUART_COUNT > 0)

#include "em_cmu.h"
#include "em_assert.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup LEUART
 * @brief Low Energy Universal Asynchronous Receiver/Transmitter (LEUART)
 *        Peripheral API
 * @details
 *  This module contains functions to control the LEUART peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs. The LEUART module provides the full UART communication using
 *  a low frequency 32.768 kHz clock and has special features for communication
 *  without the CPU intervention.
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/** A validation of the LEUART register block pointer reference
 *  for assert statements. */
#if (LEUART_COUNT == 1)
#define LEUART_REF_VALID(ref)    ((ref) == LEUART0)
#elif (LEUART_COUNT == 2)
#define LEUART_REF_VALID(ref)    (((ref) == LEUART0) || ((ref) == LEUART1))
#else
#error "Undefined number of low energy UARTs (LEUART)."
#endif

/** @endcond */

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/***************************************************************************//**
 * @brief
 *   Wait for ongoing sync of register(s) to the low-frequency domain to complete.
 *
 * @param[in] leuart
 *   A pointer to the LEUART peripheral register block.
 *
 * @param[in] mask
 *   A bitmask corresponding to SYNCBUSY register defined bits, indicating
 *   registers that must complete any ongoing synchronization.
 ******************************************************************************/
__STATIC_INLINE void LEUART_Sync(LEUART_TypeDef *leuart, uint32_t mask)
{
  /* Avoid deadlock if modifying the same register twice when freeze mode is */
  /* activated. */
  if (leuart->FREEZE & LEUART_FREEZE_REGFREEZE) {
    return;
  }

  /* Wait for any pending previous write operation to have been completed */
  /* in the low-frequency domai. */
  while ((leuart->SYNCBUSY & mask) != 0U) {
  }
}

/** @endcond */

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Calculate the baudrate for the LEUART given reference frequency and clock division.
 *
 * @details
 *   This function returns the baudrate that a LEUART module will use if
 *   configured with the given frequency and clock divisor. Notice that
 *   this function will not use the hardware configuration. It can be used
 *   to determine if a given configuration is sufficiently accurate for the
 *   application.
 *
 * @param[in] refFreq
 *   The LEUART peripheral frequency used.
 *
 * @param[in] clkdiv
 *   The clock division factor to be used.
 *
 * @return
 *   A baudrate with given settings.
 ******************************************************************************/
uint32_t LEUART_BaudrateCalc(uint32_t refFreq, uint32_t clkdiv)
{
  uint32_t divisor;
  uint32_t remainder;
  uint32_t quotient;
  uint32_t br;

  /* Mask out unused bits. */
  clkdiv &= _LEUART_CLKDIV_MASK;

  /* Use integer division to avoid forcing in float division */
  /* utils, and yet keep rounding effect errors to a minimum. */

  /*
   * Baudrate is given by:
   *
   * br = fLEUARTn/(1 + (CLKDIV / 256))
   *
   * which can be rewritten to
   *
   * br = (256 * fLEUARTn)/(256 + CLKDIV)
   *
   * Normally, with fLEUARTn appr 32768 Hz, there is no problem with overflow
   * if using 32 bit arithmetic. However, since fLEUARTn may be derived from
   * HFCORECLK, consider the overflow when using integer arithmetic.
   */

  /*
   * The basic problem with integer division in the above formula is that
   * the dividend (256 * fLEUARTn) may become higher than the maximum 32 bit
   * integer. Yet we want to evaluate the dividend first before dividing
   * to get as small rounding effects as possible.
   * Also, harsh restrictions should be avoided on the maximum fLEUARTn value.
   *
   * For division a/b:
   *
   * a = qb + r
   *
   * where q is the quotient and r is the remainder, both integers.
   *
   * The orignal baudrate formula can be rewritten as:
   *
   * br = 256a / b = 256(qb + r)/b = 256q + 256r/b
   *
   * where a is 'refFreq' and b is 'divisor', referring to variable names.
   */

  divisor   = 256 + clkdiv;
  quotient  = refFreq / divisor;
  remainder = refFreq % divisor;

  /* Since the divisor >= 256, the below cannot exceed the maximum 32 bit value. */
  br = 256 * quotient;

  /*
   * A remainder < (256 + clkdiv), which means the dividend (256 * remainder) worst case is
   * 256*(256 + 0x7ff8) = 0x80F800.
   */
  br += (256 * remainder) / divisor;

  return br;
}

/***************************************************************************//**
 * @brief
 *   Get the current baudrate for LEUART.
 *
 * @details
 *   This function returns the actual baudrate (not considering the oscillator
 *   inaccuracies) used by the LEUART peripheral.
 *
 * @param[in] leuart
 *   A pointer to the LEUART peripheral register block.
 *
 * @return
 *   The current baudrate.
 ******************************************************************************/
uint32_t LEUART_BaudrateGet(LEUART_TypeDef *leuart)
{
  uint32_t          freq;
  CMU_Clock_TypeDef clock;

  /* Get the current frequency. */
  if (leuart == LEUART0) {
    clock = cmuClock_LEUART0;
  }
#if (LEUART_COUNT > 1)
  else if (leuart == LEUART1) {
    clock = cmuClock_LEUART1;
  }
#endif
  else {
    EFM_ASSERT(0);
    return 0;
  }

  freq = CMU_ClockFreqGet(clock);

  return LEUART_BaudrateCalc(freq, leuart->CLKDIV);
}

/***************************************************************************//**
 * @brief
 *   Configure the baudrate (or as close as possible to a specified baudrate).
 *
 * @note
 *   The baudrate setting requires synchronization into the
 *   low-frequency domain. If the same register is modified before a previous
 *   update has completed, this function will stall until the previous
 *   synchronization has completed.
 *
 * @param[in] leuart
 *   A pointer to the LEUART peripheral register block.
 *
 * @param[in] refFreq
 *   The LEUART reference clock frequency in Hz that will be used. If set to 0,
 *   the currently configured reference clock is assumed.
 *
 * @param[in] baudrate
 *   A baudrate to try to achieve for LEUART.
 ******************************************************************************/
void LEUART_BaudrateSet(LEUART_TypeDef *leuart,
                        uint32_t refFreq,
                        uint32_t baudrate)
{
  uint32_t          clkdiv;
  CMU_Clock_TypeDef clock;

  /* Prevent dividing by 0. */
  EFM_ASSERT(baudrate);

  /*
   * Use integer division to avoid forcing in float division
   * utils, and yet keep rounding effect errors to a minimum.
   *
   * CLKDIV in asynchronous mode is given by:
   *
   * CLKDIV = 256*(fLEUARTn/br - 1) = ((256*fLEUARTn)/br) - 256
   *
   * Normally, with fLEUARTn appr 32768 Hz, there is no problem with overflow
   * if using 32 bit arithmetic. However, since fLEUARTn may be derived from
   * HFCORECLK, consider the overflow when using integer arithmetic.
   *
   * The basic problem with integer division in the above formula is that
   * the dividend (256 * fLEUARTn) may become higher than the maximum 32 bit
   * integer. Yet, the dividend should be evaluated first before dividing
   * to get as small rounding effects as possible.
   * Also, harsh restrictions on the maximum fLEUARTn value should not be made.
   *
   * Since the last 3 bits of CLKDIV are don't care, base the
   * integer arithmetic on the below formula:
   *
   * CLKDIV/8 = ((32*fLEUARTn)/br) - 32
   *
   * and calculate 1/8 of CLKDIV first. This allows for fLEUARTn
   * up to 128 MHz without overflowing a 32 bit value.
   */

  /* Get the current frequency. */
  if (!refFreq) {
    if (leuart == LEUART0) {
      clock = cmuClock_LEUART0;
    }
#if (LEUART_COUNT > 1)
    else if (leuart == LEUART1) {
      clock = cmuClock_LEUART1;
    }
#endif
    else {
      EFM_ASSERT(0);
      return;
    }

    refFreq = CMU_ClockFreqGet(clock);
  }

  /* Calculate and set the CLKDIV with fractional bits. */
  clkdiv  = (32 * refFreq) / baudrate;
  clkdiv -= 32;
  clkdiv *= 8;

  /* Verify that the resulting clock divider is within limits. */
  EFM_ASSERT(clkdiv <= _LEUART_CLKDIV_MASK);

  /* If the EFM_ASSERT is not enabled, make sure not to write to reserved bits. */
  clkdiv &= _LEUART_CLKDIV_MASK;

  /* LF register about to be modified requires sync;  busy check. */
  LEUART_Sync(leuart, LEUART_SYNCBUSY_CLKDIV);

  leuart->CLKDIV = clkdiv;
}

/***************************************************************************//**
 * @brief
 *   Enable/disable the LEUART receiver and/or transmitter.
 *
 * @details
 *   Notice that this function does not do any configuration. Enabling should
 *   normally be done after the initialization is done (if not enabled as part
 *   of initialization).
 *
 * @note
 *   Enabling/disabling requires synchronization into the low-frequency domain.
 *   If the same register is modified before a previous update has completed,
 *   this function will stall until the previous synchronization has completed.
 *
 * @param[in] leuart
 *   A pointer to the LEUART peripheral register block.
 *
 * @param[in] enable
 *   Select status for receiver/transmitter.
 ******************************************************************************/
void LEUART_Enable(LEUART_TypeDef *leuart, LEUART_Enable_TypeDef enable)
{
  uint32_t tmp;

  /* Make sure that the module exists on the selected chip. */
  EFM_ASSERT(LEUART_REF_VALID(leuart));

  /* Disable as specified. */
  tmp   = ~((uint32_t)(enable));
  tmp  &= (_LEUART_CMD_RXEN_MASK | _LEUART_CMD_TXEN_MASK);
  tmp <<= 1;
  /* Enable as specified. */
  tmp |= (uint32_t)(enable);

  /* LF register about to be modified requires sync; busy check. */
  LEUART_Sync(leuart, LEUART_SYNCBUSY_CMD);

  leuart->CMD = tmp;
}

/***************************************************************************//**
 * @brief
 *   LEUART register synchronization freeze control.
 *
 * @details
 *   Some LEUART registers require synchronization into the low-frequency (LF)
 *   domain. The freeze feature allows for several such registers to be
 *   modified before passing them to the LF domain simultaneously (which
 *   takes place when the freeze mode is disabled).
 *
 * @note
 *   When enabling freeze mode, this function will wait for all current
 *   ongoing LEUART synchronization to the LF domain to complete (Normally
 *   synchronization will not be in progress.) However, for this reason, when
 *   using freeze mode, modifications of registers requiring LF synchronization
 *   should be done within one freeze enable/disable block to avoid unnecessary
 *   stalling.
 *
 * @param[in] leuart
 *   A pointer to the LEUART peripheral register block.
 *
 * @param[in] enable
 *   @li True - enable freeze, modified registers are not propagated to the
 *       LF domain
 *   @li False - disables freeze, modified registers are propagated to the LF
 *       domain
 ******************************************************************************/
void LEUART_FreezeEnable(LEUART_TypeDef *leuart, bool enable)
{
  if (enable) {
    /*
     * Wait for any ongoing LF synchronization to complete to
     * protect against the rare case when a user
     * - modifies a register requiring LF sync
     * - then enables freeze before LF sync completed
     * - then modifies the same register again
     * since modifying a register while it is in sync progress should be
     * avoided.
     */
    while (leuart->SYNCBUSY != 0U) {
    }

    leuart->FREEZE = LEUART_FREEZE_REGFREEZE;
  } else {
    leuart->FREEZE = 0;
  }
}

/***************************************************************************//**
 * @brief
 *   Initialize LEUART.
 *
 * @details
 *   This function will configure basic settings to operate in normal
 *   asynchronous mode. Consider using LEUART_Reset() prior to this function if
 *   the state of configuration is not known, since only configuration settings
 *   specified by @p init are set.
 *
 *   Special control setup not covered by this function may be done either
 *   before or after using this function (but normally before enabling)
 *   by direct modification of the CTRL register.
 *
 *   Notice that pins used by the LEUART module must be properly configured
 *   by the user explicitly for the LEUART to work as intended.
 *   (When configuring pins consider the sequence of
 *   configuration to avoid unintended pulses/glitches on output
 *   pins.)
 *
 * @note
 *   Initializing requires synchronization into the low-frequency domain.
 *   If the same register is modified before a previous update has completed,
 *   this function will stall until the previous synchronization has completed.
 *
 * @param[in] leuart
 *   A pointer to the LEUART peripheral register block.
 *
 * @param[in] init
 *   A pointer to the initialization structure used to configure basic async setup.
 ******************************************************************************/
void LEUART_Init(LEUART_TypeDef *leuart, LEUART_Init_TypeDef const *init)
{
  /* Make sure the module exists on the selected chip. */
  EFM_ASSERT(LEUART_REF_VALID(leuart));

  /* LF register about to be modified requires sync; busy check. */
  LEUART_Sync(leuart, LEUART_SYNCBUSY_CMD);

  /* Ensure disabled while configuring. */
  leuart->CMD = LEUART_CMD_RXDIS | LEUART_CMD_TXDIS;

  /* Freeze registers to avoid stalling for the LF synchronization. */
  LEUART_FreezeEnable(leuart, true);

  /* Configure databits and stopbits. */
  leuart->CTRL = (leuart->CTRL & ~(_LEUART_CTRL_PARITY_MASK
                                   | _LEUART_CTRL_STOPBITS_MASK))
                 | (uint32_t)(init->databits)
                 | (uint32_t)(init->parity)
                 | (uint32_t)(init->stopbits);

  /* Configure the baudrate. */
  LEUART_BaudrateSet(leuart, init->refFreq, init->baudrate);

  /* Finally enable (as specified). */
  leuart->CMD = (uint32_t)init->enable;

  /* Unfreeze registers and pass new settings on to LEUART. */
  LEUART_FreezeEnable(leuart, false);
}

/***************************************************************************//**
 * @brief
 *   Reset LEUART to the same state that it was in after a hardware reset.
 *
 * @param[in] leuart
 *   A pointer to the LEUART peripheral register block.
 ******************************************************************************/
void LEUART_Reset(LEUART_TypeDef *leuart)
{
  /* Make sure the module exists on the selected chip. */
  EFM_ASSERT(LEUART_REF_VALID(leuart));

  /* Freeze registers to avoid stalling for LF synchronization. */
  LEUART_FreezeEnable(leuart, true);

  /* Make sure disabled first, before resetting other registers. */
  leuart->CMD = LEUART_CMD_RXDIS | LEUART_CMD_TXDIS | LEUART_CMD_RXBLOCKDIS
                | LEUART_CMD_CLEARTX | LEUART_CMD_CLEARRX;
  leuart->CTRL       = _LEUART_CTRL_RESETVALUE;
  leuart->CLKDIV     = _LEUART_CLKDIV_RESETVALUE;
  leuart->STARTFRAME = _LEUART_STARTFRAME_RESETVALUE;
  leuart->SIGFRAME   = _LEUART_SIGFRAME_RESETVALUE;
  leuart->IEN        = _LEUART_IEN_RESETVALUE;
  leuart->IFC        = _LEUART_IFC_MASK;
  leuart->PULSECTRL  = _LEUART_PULSECTRL_RESETVALUE;
#if defined(_LEUART_ROUTEPEN_MASK)
  leuart->ROUTEPEN   = _LEUART_ROUTEPEN_RESETVALUE;
  leuart->ROUTELOC0  = _LEUART_ROUTELOC0_RESETVALUE;
#else
  leuart->ROUTE      = _LEUART_ROUTE_RESETVALUE;
#endif

  /* Unfreeze registers and pass new settings on to LEUART. */
  LEUART_FreezeEnable(leuart, false);
}

/***************************************************************************//**
 * @brief
 *   Receive one 8 bit frame, (or part of 9 bit frame).
 *
 * @details
 *   This function is normally used to receive one frame when operating with
 *   frame length 8 bits. See LEUART_RxExt() for reception of
 *   9 bit frames.
 *
 *   Notice that possible parity/stop bits are not considered a part of the specified
 *   frame bit length.
 *
 * @note
 *   This function will stall if the buffer is empty until data is received.
 *
 * @param[in] leuart
 *   A pointer to the LEUART peripheral register block.
 *
 * @return
 *   Data received.
 ******************************************************************************/
uint8_t LEUART_Rx(LEUART_TypeDef *leuart)
{
  while (!(leuart->STATUS & LEUART_STATUS_RXDATAV)) {
  }

  return (uint8_t)leuart->RXDATA;
}

/***************************************************************************//**
 * @brief
 *   Receive one 8-9 bit frame with extended information.
 *
 * @details
 *   This function is normally used to receive one frame and additional RX
 *   status information is required.
 *
 * @note
 *   This function will stall if buffer is empty until data is received.
 *
 * @param[in] leuart
 *   A pointer to the LEUART peripheral register block.
 *
 * @return
 *   Data received.
 ******************************************************************************/
uint16_t LEUART_RxExt(LEUART_TypeDef *leuart)
{
  while (!(leuart->STATUS & LEUART_STATUS_RXDATAV)) {
  }

  return (uint16_t)leuart->RXDATAX;
}

/***************************************************************************//**
 * @brief
 *   Transmit one frame.
 *
 * @details
 *   Depending on the frame length configuration, 8 (least significant) bits from
 *   @p data are transmitted. If the frame length is 9, 8 bits are transmitted from
 *   @p data and one bit as specified by the CTRL register, BIT8DV field.
 *   See LEUART_TxExt() for transmitting 9 bit frame with full control of
 *   all 9 bits.
 *
 *   Notice that possible parity/stop bits in asynchronous mode are not
 *   considered a part of the specified frame bit length.
 *
 * @note
 *   This function will stall if buffer is full until the buffer becomes available.
 *
 * @param[in] leuart
 *   A pointer to the LEUART peripheral register block.
 *
 * @param[in] data
 *   Data to transmit. See details above for more info.
 ******************************************************************************/
void LEUART_Tx(LEUART_TypeDef *leuart, uint8_t data)
{
  /* Check that transmit buffer is empty. */
  while (!(leuart->STATUS & LEUART_STATUS_TXBL)) {
  }

  /* LF register about to be modified requires sync; busy check. */
  LEUART_Sync(leuart, LEUART_SYNCBUSY_TXDATA);

  leuart->TXDATA = (uint32_t)data;
}

/***************************************************************************//**
 * @brief
 *   Transmit one 8-9 bit frame with extended control.
 *
 * @details
 *   Notice that possible parity/stop bits in asynchronous mode are not
 *   considered a part of the specified frame bit length.
 *
 * @note
 *   This function will stall if the buffer is full until the buffer becomes available.
 *
 * @param[in] leuart
 *   A pointer to the LEUART peripheral register block.
 *
 * @param[in] data
 *   Data to transmit with extended control. Least significant bit contains
 *   frame bits and additional control bits are available as documented in
 *   the reference manual (set to 0 if not used).
 ******************************************************************************/
void LEUART_TxExt(LEUART_TypeDef *leuart, uint16_t data)
{
  /* Check that transmit buffer is empty. */
  while (!(leuart->STATUS & LEUART_STATUS_TXBL)) {
  }

  /* LF register about to be modified requires sync; busy check. */
  LEUART_Sync(leuart, LEUART_SYNCBUSY_TXDATAX);

  leuart->TXDATAX = (uint32_t)data;
}

/***************************************************************************//**
 * @brief
 *   Enables handling of LEUART TX by DMA in EM2.
 *
 * @param[in] leuart
 *   A pointer to the LEUART peripheral register block.
 *
 * @param[in] enable
 *   True - enables functionality
 *   False - disables functionality
 *
 ******************************************************************************/
void LEUART_TxDmaInEM2Enable(LEUART_TypeDef *leuart, bool enable)
{
  /* LF register about to be modified requires sync; busy check. */
  LEUART_Sync(leuart, LEUART_SYNCBUSY_CTRL | LEUART_SYNCBUSY_CMD);

#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_80)
  /* LEUART_E201: Changing the value of TXDMAWU while TXEN=1 could potentially
   * cause unpredictable behavior. */
  bool txEnabled = (leuart->STATUS & _LEUART_STATUS_TXENS_MASK) != 0U;
  if (txEnabled) {
    /* Wait for potential transmit to complete. */
    while ((leuart->STATUS & LEUART_STATUS_TXIDLE) == 0U) {
    }

    leuart->CMD = LEUART_CMD_TXDIS;
    LEUART_Sync(leuart, LEUART_SYNCBUSY_CMD);
  }

  if (enable) {
    leuart->CTRL |= LEUART_CTRL_TXDMAWU;
  } else {
    leuart->CTRL &= ~LEUART_CTRL_TXDMAWU;
  }

  if (txEnabled) {
    leuart->CMD = LEUART_CMD_TXEN;
  }
#else
  if (enable) {
    leuart->CTRL |= LEUART_CTRL_TXDMAWU;
  } else {
    leuart->CTRL &= ~LEUART_CTRL_TXDMAWU;
  }
#endif
}

/***************************************************************************//**
 * @brief
 *   Enables handling of LEUART RX by DMA in EM2.
 *
 * @param[in] leuart
 *   A pointer to the LEUART peripheral register block.
 *
 * @param[in] enable
 *   True - enables functionality
 *   False - disables functionality
 *
 ******************************************************************************/
void LEUART_RxDmaInEM2Enable(LEUART_TypeDef *leuart, bool enable)
{
  /* LF register about to be modified requires sync; busy check. */
  LEUART_Sync(leuart, LEUART_SYNCBUSY_CTRL | LEUART_SYNCBUSY_CMD);

#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_80)
  /* LEUART_E201: Changing the value of RXDMAWU while RXEN=1 could potentially
   * cause unpredictable behavior. */
  bool rxEnabled = (leuart->STATUS & _LEUART_STATUS_RXENS_MASK) != 0U;

  if (rxEnabled) {
    leuart->CMD = LEUART_CMD_RXDIS;
    LEUART_Sync(leuart, LEUART_SYNCBUSY_CMD);
  }

  if (enable) {
    leuart->CTRL |= LEUART_CTRL_RXDMAWU;
  } else {
    leuart->CTRL &= ~LEUART_CTRL_RXDMAWU;
  }

  if (rxEnabled) {
    leuart->CMD = LEUART_CMD_RXEN;
  }
#else
  if (enable) {
    leuart->CTRL |= LEUART_CTRL_RXDMAWU;
  } else {
    leuart->CTRL &= ~LEUART_CTRL_RXDMAWU;
  }
#endif
}

/** @} (end addtogroup LEUART) */
/** @} (end addtogroup emlib) */
#endif /* defined(LEUART_COUNT) && (LEUART_COUNT > 0) */
