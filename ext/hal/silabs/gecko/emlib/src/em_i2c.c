/***************************************************************************//**
 * @file em_i2c.c
 * @brief Inter-integrated Circuit (I2C) Peripheral API
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

#include "em_i2c.h"
#if defined(I2C_COUNT) && (I2C_COUNT > 0)

#include "em_cmu.h"
#include "em_bus.h"
#include "em_assert.h"

 #include <limits.h>

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup I2C
 * @brief Inter-integrated Circuit (I2C) Peripheral API
 * @details
 *  This module contains functions to control the I2C peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs. The I2C interface allows communication on I2C
 *  buses with the lowest energy consumption possible.
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/** Validation of the I2C register block pointer reference for assert statements. */
#if (I2C_COUNT == 1)
#define I2C_REF_VALID(ref)    ((ref) == I2C0)
#elif (I2C_COUNT == 2)
#define I2C_REF_VALID(ref)    (((ref) == I2C0) || ((ref) == I2C1))
#elif (I2C_COUNT == 3)
#define I2C_REF_VALID(ref)    (((ref) == I2C0) || ((ref) == I2C1) || ((ref) == I2C2))
#endif

/** Error flags indicating that the I2C transfer has failed. */
/* Notice that I2C_IF_TXOF (transmit overflow) is not really possible with */
/* the software-supporting master mode. Likewise, for I2C_IF_RXUF (receive underflow) */
/* RXUF is only likely to occur with the software if using a debugger peeking into */
/* the RXDATA register. Therefore, those types of faults are ignored. */
#define I2C_IF_ERRORS    (I2C_IF_BUSERR | I2C_IF_ARBLOST)
#define I2C_IEN_ERRORS   (I2C_IEN_BUSERR | I2C_IEN_ARBLOST)

/* Maximum I2C transmission rate constant.  */
#if defined(_SILICON_LABS_32B_SERIES_0)
#define I2C_CR_MAX       4
#elif defined(_SILICON_LABS_32B_SERIES_1)
#define I2C_CR_MAX       8
#elif defined(_SILICON_LABS_32B_SERIES_2)
#define I2C_CR_MAX       8
#else
#warning "Max I2C transmission rate constant is not defined"
#endif

/** @endcond */

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/** Master mode transfer states. */
typedef enum {
  i2cStateStartAddrSend,       /**< Send start + (first part of) address. */
  i2cStateAddrWFAckNack,       /**< Wait for ACK/NACK on (the first part of) address. */
  i2cStateAddrWF2ndAckNack,    /**< Wait for ACK/NACK on the second part of a 10 bit address. */
  i2cStateRStartAddrSend,      /**< Send a repeated start + (first part of) address. */
  i2cStateRAddrWFAckNack,      /**< Wait for ACK/NACK on an address sent after a repeated start. */
  i2cStateDataSend,            /**< Send data. */
  i2cStateDataWFAckNack,       /**< Wait for ACK/NACK on data sent. */
  i2cStateWFData,              /**< Wait for data. */
  i2cStateWFStopSent,          /**< Wait for STOP to have been transmitted. */
  i2cStateDone                 /**< Transfer completed successfully. */
} I2C_TransferState_TypeDef;

/** @endcond */

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/** Structure used to store state information on an ongoing master mode transfer. */
typedef struct {
  /** Current state. */
  I2C_TransferState_TypeDef  state;

  /** Result return code. */
  I2C_TransferReturn_TypeDef result;

  /** Offset in the current sequence buffer. */
  uint16_t                   offset;

  /* Index to the current sequence buffer in use. */
  uint8_t                    bufIndx;

  /** Reference to the I2C transfer sequence definition provided by the user. */
  I2C_TransferSeq_TypeDef    *seq;
} I2C_Transfer_TypeDef;

/** @endcond */

/*******************************************************************************
 *****************************   LOCAL DATA   *******^**************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/**
 * Lookup table for Nlow + Nhigh setting defined by CLHR. Set the undefined
 * index (0x3) to reflect a default setting just in case.
 */
static const uint8_t i2cNSum[] = { 4 + 4, 6 + 3, 11 + 6, 4 + 4 };

/** A transfer state information for an ongoing master mode transfer. */
static I2C_Transfer_TypeDef i2cTransfer[I2C_COUNT];

/** @endcond */

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   *******************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/***************************************************************************//**
 * @brief
 *   Empty received data buffer.
 ******************************************************************************/
static void flushRx(I2C_TypeDef *i2c)
{
  while (i2c->STATUS & I2C_STATUS_RXDATAV) {
    i2c->RXDATA;
  }

#if defined(_SILICON_LABS_32B_SERIES_2)
  /* SW needs to clear RXDATAV IF on Series 2 devices.
     Flag is kept high by HW if buffer is not empty. */
  I2C_IntClear(i2c, I2C_IF_RXDATAV);
#endif
}

/** @endcond */

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Get the current configured I2C bus frequency.
 *
 * @details
 *   This frequency is only relevant when acting as master.
 *
 * @note
 *   The actual frequency is a real number, this function returns a rounded
 *   down (truncated) integer value.
 *
 * @param[in] i2c
 *   A pointer to the I2C peripheral register block.
 *
 * @return
 *   The current I2C frequency in Hz.
 ******************************************************************************/
uint32_t I2C_BusFreqGet(I2C_TypeDef *i2c)
{
  uint32_t freqHfper;
  uint32_t n;

  /* Maximum frequency is given by freqScl = freqHfper/((Nlow + Nhigh)(DIV + 1) + I2C_CR_MAX)
   * For more details, see the reference manual
   * I2C Clock Generation chapter. */
#if defined(_SILICON_LABS_32B_SERIES_2)
  if (i2c == I2C0) {
    freqHfper = CMU_ClockFreqGet(cmuClock_I2C0);
  } else { // i2c == I2C1
    freqHfper = CMU_ClockFreqGet(cmuClock_I2C1);
  }
#else
  freqHfper = CMU_ClockFreqGet(cmuClock_HFPER);
#endif
  /* n = Nlow + Nhigh */
  n = (uint32_t)i2cNSum[(i2c->CTRL & _I2C_CTRL_CLHR_MASK)
                        >> _I2C_CTRL_CLHR_SHIFT];
  return freqHfper / ((n * (i2c->CLKDIV + 1)) + I2C_CR_MAX);
}

/***************************************************************************//**
 * @brief
 *   Set the I2C bus frequency.
 *
 * @details
 *   The bus frequency is only relevant when acting as master. The bus
 *   frequency should not be set higher than the maximum frequency accepted by the
 *   slowest device on the bus.
 *
 *   Notice that, due to asymmetric requirements on low and high I2C clock
 *   cycles in the I2C specification, the maximum frequency allowed
 *   to comply with the specification may be somewhat lower than expected.
 *
 *   See the reference manual, details on I2C clock generation,
 *   for maximum allowed theoretical frequencies for different modes.
 *
 * @param[in] i2c
 *   A pointer to the I2C peripheral register block.
 *
 * @param[in] freqRef
 *   An I2C reference clock frequency in Hz that will be used. If set to 0,
 *   HFPER clock is used. Setting it to a higher than actual configured
 *   value has the consequence of reducing the real I2C frequency.
 *
 * @param[in] freqScl
 *   A bus frequency to set (bus speed may be lower due to integer
 *   prescaling). Safe (according to the I2C specification) maximum frequencies for
 *   standard fast and fast+ modes are available using I2C_FREQ_ defines.
 *   (Using I2C_FREQ_ defines requires corresponding setting of @p type.)
 *   The slowest slave device on a bus must always be considered.
 *
 * @param[in] i2cMode
 *   A clock low-to-high ratio type to use. If not using i2cClockHLRStandard,
 *   make sure all devices on the bus support the specified mode. Using a
 *   non-standard ratio is useful to achieve a higher bus clock in fast and
 *   fast+ modes.
 ******************************************************************************/
void I2C_BusFreqSet(I2C_TypeDef *i2c,
                    uint32_t freqRef,
                    uint32_t freqScl,
                    I2C_ClockHLR_TypeDef i2cMode)
{
  uint32_t n, minFreq, denominator;
  int32_t div;

  /* Avoid dividing by 0. */
  EFM_ASSERT(freqScl);
  if (!freqScl) {
    return;
  }

  /* Ensure mode is valid */
  i2cMode &= _I2C_CTRL_CLHR_MASK >> _I2C_CTRL_CLHR_SHIFT;

  /* Set the CLHR (clock low-to-high ratio). */
  i2c->CTRL &= ~_I2C_CTRL_CLHR_MASK;
  BUS_RegMaskedWrite(&i2c->CTRL,
                     _I2C_CTRL_CLHR_MASK,
                     i2cMode << _I2C_CTRL_CLHR_SHIFT);

  if (freqRef == 0) {
#if defined(_SILICON_LABS_32B_SERIES_2)
    if (i2c == I2C0) {
      freqRef = CMU_ClockFreqGet(cmuClock_I2C0);
    } else { // i2c == I2C1
      freqRef = CMU_ClockFreqGet(cmuClock_I2C1);
    }
#else
    freqRef = CMU_ClockFreqGet(cmuClock_HFPER);
#endif
  }

  /* Check the minumum HF peripheral clock. */
  minFreq = UINT_MAX;
  if (i2c->CTRL & I2C_CTRL_SLAVE) {
    switch (i2cMode) {
      case i2cClockHLRStandard:
#if defined(_SILICON_LABS_32B_SERIES_0)
        minFreq = 4200000; break;
#elif defined(_SILICON_LABS_32B_SERIES_1)
        minFreq = 2000000; break;
#elif defined(_SILICON_LABS_32B_SERIES_2)
        minFreq = 2000000; break;
#endif
      case i2cClockHLRAsymetric:
#if defined(_SILICON_LABS_32B_SERIES_0)
        minFreq = 11000000; break;
#elif defined(_SILICON_LABS_32B_SERIES_1)
        minFreq = 5000000; break;
#elif defined(_SILICON_LABS_32B_SERIES_2)
        minFreq = 5000000; break;
#endif
      case i2cClockHLRFast:
#if defined(_SILICON_LABS_32B_SERIES_0)
        minFreq = 24400000; break;
#elif defined(_SILICON_LABS_32B_SERIES_1)
        minFreq = 14000000; break;
#elif defined(_SILICON_LABS_32B_SERIES_2)
        minFreq = 14000000; break;
#endif
      default:
        /* MISRA requires the default case. */
        break;
    }
  } else {
    /* For master mode, platform 1 and 2 share the same
       minimum frequencies. */
    switch (i2cMode) {
      case i2cClockHLRStandard:
        minFreq = 2000000; break;
      case i2cClockHLRAsymetric:
        minFreq = 9000000; break;
      case i2cClockHLRFast:
        minFreq = 20000000; break;
      default:
        /* MISRA requires default case */
        break;
    }
  }

  /* Frequency most be larger-than. */
  EFM_ASSERT(freqRef > minFreq);

  /* SCL frequency is given by:
   * freqScl = freqRef/((Nlow + Nhigh) * (DIV + 1) + I2C_CR_MAX)
   *
   * Therefore,
   * DIV = ((freqRef - (I2C_CR_MAX * freqScl))/((Nlow + Nhigh) * freqScl)) - 1
   *
   * For more details, see the reference manual
   * I2C Clock Generation chapter.  */

  /* n = Nlow + Nhigh */
  n = (uint32_t)i2cNSum[i2cMode];
  denominator = n * freqScl;

  /* Explicitly ensure denominator is never zero. */
  if (denominator == 0) {
    EFM_ASSERT(0);
    return;
  }
  /* Perform integer division so that div is rounded up. */
  div = ((freqRef - (I2C_CR_MAX * freqScl) + denominator - 1)
         / denominator) - 1;
  EFM_ASSERT(div >= 0);
  EFM_ASSERT((uint32_t)div <= _I2C_CLKDIV_DIV_MASK);

  /* The clock divisor must be at least 1 in slave mode according to the reference */
  /* manual (in which case there is normally no need to set the bus frequency). */
  if ((i2c->CTRL & I2C_CTRL_SLAVE) && (div == 0)) {
    div = 1;
  }
  i2c->CLKDIV = (uint32_t)div;
}

/***************************************************************************//**
 * @brief
 *   Enable/disable I2C.
 *
 * @note
 *   After enabling the I2C (from being disabled), the I2C is in BUSY state.
 *
 * @param[in] i2c
 *   A pointer to the I2C peripheral register block.
 *
 * @param[in] enable
 *   True to enable counting, false to disable.
 ******************************************************************************/
void I2C_Enable(I2C_TypeDef *i2c, bool enable)
{
  EFM_ASSERT(I2C_REF_VALID(i2c));

#if defined (_I2C_EN_MASK)
  BUS_RegBitWrite(&(i2c->EN), _I2C_EN_EN_SHIFT, enable);
#else
  BUS_RegBitWrite(&(i2c->CTRL), _I2C_CTRL_EN_SHIFT, enable);
#endif
}

/***************************************************************************//**
 * @brief
 *   Initialize I2C.
 *
 * @param[in] i2c
 *   A pointer to the I2C peripheral register block.
 *
 * @param[in] init
 *   A pointer to the I2C initialization structure.
 ******************************************************************************/
void I2C_Init(I2C_TypeDef *i2c, const I2C_Init_TypeDef *init)
{
  EFM_ASSERT(I2C_REF_VALID(i2c));

  i2c->IEN = 0;
  I2C_IntClear(i2c, _I2C_IF_MASK);

  /* Set SLAVE select mode. */
  BUS_RegBitWrite(&(i2c->CTRL), _I2C_CTRL_SLAVE_SHIFT, init->master ? 0 : 1);

  I2C_BusFreqSet(i2c, init->refFreq, init->freq, init->clhr);

  I2C_Enable(i2c, init->enable);
}

/***************************************************************************//**
 * @brief
 *   Reset I2C to the same state that it was in after a hardware reset.
 *
 * @note
 *   The ROUTE register is NOT reset by this function to allow for
 *   centralized setup of this feature.
 *
 * @param[in] i2c
 *   A pointer to the I2C peripheral register block.
 ******************************************************************************/
void I2C_Reset(I2C_TypeDef *i2c)
{
  // Cancel ongoing operations and clear TX buffer
  i2c->CMD       = I2C_CMD_CLEARPC | I2C_CMD_CLEARTX | I2C_CMD_ABORT;
  i2c->CTRL      = _I2C_CTRL_RESETVALUE;
  i2c->CLKDIV    = _I2C_CLKDIV_RESETVALUE;
  i2c->SADDR     = _I2C_SADDR_RESETVALUE;
  i2c->SADDRMASK = _I2C_SADDRMASK_RESETVALUE;
  i2c->IEN       = _I2C_IEN_RESETVALUE;
#if defined (_I2C_EN_EN_MASK)
  i2c->EN      = _I2C_EN_RESETVALUE;
#endif

  // Empty received data buffer
  flushRx(i2c);
  I2C_IntClear(i2c, _I2C_IF_MASK);
  /* Do not reset the route register; setting should be done independently. */
}

/***************************************************************************//**
 * @brief
 *   Continue an initiated I2C transfer (single master mode only).
 *
 * @details
 *   This function is used repeatedly after a I2C_TransferInit() to
 *   complete a transfer. It may be used in polled mode as the below example
 *   shows:
 * @verbatim
 * I2C_TransferReturn_TypeDef ret;
 *
 * // Do a polled transfer
 * ret = I2C_TransferInit(I2C0, seq);
 * while (ret == i2cTransferInProgress)
 * {
 *   ret = I2C_Transfer(I2C0);
 * }
 * @endverbatim
 *  It may also be used in interrupt driven mode, where this function is invoked
 *  from the interrupt handler. Notice that, if used in interrupt mode, NVIC
 *  interrupts must be configured and enabled for the I2C bus used. I2C
 *  peripheral specific interrupts are managed by this software.
 *
 * @note
 *   Only single master mode is supported.
 *
 * @param[in] i2c
 *   A pointer to the I2C peripheral register block.
 *
 * @return
 *   Returns status for an ongoing transfer.
 *   @li #i2cTransferInProgress - indicates that transfer not finished.
 *   @li #i2cTransferDone - transfer completed successfully.
 *   @li otherwise some sort of error has occurred.
 *
 ******************************************************************************/
I2C_TransferReturn_TypeDef I2C_Transfer(I2C_TypeDef *i2c)
{
  uint32_t                tmp;
  uint32_t                pending;
  I2C_Transfer_TypeDef    *transfer;
  I2C_TransferSeq_TypeDef *seq;

  EFM_ASSERT(I2C_REF_VALID(i2c));

  /* Support up to 2 I2C buses. */
  if (i2c == I2C0) {
    transfer = i2cTransfer;
  }
#if (I2C_COUNT > 1)
  else if (i2c == I2C1) {
    transfer = i2cTransfer + 1;
  }
#endif
#if (I2C_COUNT > 2)
  else if (i2c == I2C2) {
    transfer = i2cTransfer + 2;
  }
#endif
  else {
    return i2cTransferUsageFault;
  }

  seq = transfer->seq;
  for (;; ) {
    pending = i2c->IF;

    /* If some sort of fault, abort transfer. */
    if (pending & I2C_IF_ERRORS) {
      if (pending & I2C_IF_ARBLOST) {
        /* If an arbitration fault, indicates either a slave device */
        /* not responding as expected, or other master which is not */
        /* supported by this software. */
        transfer->result = i2cTransferArbLost;
      } else if (pending & I2C_IF_BUSERR) {
        /* A bus error indicates a misplaced start or stop, which should */
        /* not occur in master mode controlled by this software. */
        transfer->result = i2cTransferBusErr;
      }

      /* Ifan error occurs, it is difficult to know */
      /* an exact cause and how to resolve. It will be up to a wrapper */
      /* to determine how to handle a fault/recovery if possible. */
      transfer->state = i2cStateDone;
      goto done;
    }

    switch (transfer->state) {
      /***************************************************/
      /* Send the first start+address (first byte if 10 bit). */
      /***************************************************/
      case i2cStateStartAddrSend:
        if (seq->flags & I2C_FLAG_10BIT_ADDR) {
          tmp = (((uint32_t)(seq->addr) >> 8) & 0x06) | 0xf0;

          /* In 10 bit address mode, the address following the first */
          /* start always indicates write. */
        } else {
          tmp = (uint32_t)(seq->addr) & 0xfe;

          if (seq->flags & I2C_FLAG_READ) {
            /* Indicate read request */
            tmp |= 1;
          }
        }

        transfer->state = i2cStateAddrWFAckNack;
        i2c->TXDATA     = tmp;/* Data not transmitted until the START is sent. */
        i2c->CMD        = I2C_CMD_START;
        goto done;

      /*******************************************************/
      /* Wait for ACK/NACK on the address (first byte if 10 bit). */
      /*******************************************************/
      case i2cStateAddrWFAckNack:
        if (pending & I2C_IF_NACK) {
          I2C_IntClear(i2c, I2C_IF_NACK);
          transfer->result = i2cTransferNack;
          transfer->state  = i2cStateWFStopSent;
          i2c->CMD         = I2C_CMD_STOP;
        } else if (pending & I2C_IF_ACK) {
          I2C_IntClear(i2c, I2C_IF_ACK);

          /* If a 10 bit address, send the 2nd byte of the address. */
          if (seq->flags & I2C_FLAG_10BIT_ADDR) {
            transfer->state = i2cStateAddrWF2ndAckNack;
            i2c->TXDATA     = (uint32_t)(seq->addr) & 0xff;
          } else {
            /* Determine whether receiving or sending data. */
            if (seq->flags & I2C_FLAG_READ) {
              transfer->state = i2cStateWFData;
              if (seq->buf[transfer->bufIndx].len == 1) {
                i2c->CMD  = I2C_CMD_NACK;
              }
            } else {
              transfer->state = i2cStateDataSend;
              continue;
            }
          }
        }
        goto done;

      /******************************************************/
      /* Wait for ACK/NACK on the second byte of a 10 bit address. */
      /******************************************************/
      case i2cStateAddrWF2ndAckNack:
        if (pending & I2C_IF_NACK) {
          I2C_IntClear(i2c, I2C_IF_NACK);
          transfer->result = i2cTransferNack;
          transfer->state  = i2cStateWFStopSent;
          i2c->CMD         = I2C_CMD_STOP;
        } else if (pending & I2C_IF_ACK) {
          I2C_IntClear(i2c, I2C_IF_ACK);

          /* If using a plain read sequence with a 10 bit address, switch to send */
          /* a repeated start. */
          if (seq->flags & I2C_FLAG_READ) {
            transfer->state = i2cStateRStartAddrSend;
          }
          /* Otherwise, expected to write 0 or more bytes. */
          else {
            transfer->state = i2cStateDataSend;
          }
          continue;
        }
        goto done;

      /*******************************/
      /* Send a repeated start+address */
      /*******************************/
      case i2cStateRStartAddrSend:
        if (seq->flags & I2C_FLAG_10BIT_ADDR) {
          tmp = ((seq->addr >> 8) & 0x06) | 0xf0;
        } else {
          tmp = seq->addr & 0xfe;
        }

        /* If this is a write+read combined sequence, read is about to start. */
        if (seq->flags & I2C_FLAG_WRITE_READ) {
          /* Indicate a read request. */
          tmp |= 1;
        }

        transfer->state = i2cStateRAddrWFAckNack;
        /* The START command has to be written first since repeated start. Otherwise, */
        /* data would be sent first. */
        i2c->CMD    = I2C_CMD_START;
        i2c->TXDATA = tmp;
        goto done;

      /**********************************************************************/
      /* Wait for ACK/NACK on the repeated start+address (first byte if 10 bit) */
      /**********************************************************************/
      case i2cStateRAddrWFAckNack:
        if (pending & I2C_IF_NACK) {
          I2C_IntClear(i2c, I2C_IF_NACK);
          transfer->result = i2cTransferNack;
          transfer->state  = i2cStateWFStopSent;
          i2c->CMD         = I2C_CMD_STOP;
        } else if (pending & I2C_IF_ACK) {
          I2C_IntClear(i2c, I2C_IF_ACK);

          /* Determine whether receiving or sending data. */
          if (seq->flags & I2C_FLAG_WRITE_READ) {
            transfer->state = i2cStateWFData;
          } else {
            transfer->state = i2cStateDataSend;
            continue;
          }
        }
        goto done;

      /*****************************/
      /* Send a data byte to the slave */
      /*****************************/
      case i2cStateDataSend:
        /* Reached end of data buffer. */
        if (transfer->offset >= seq->buf[transfer->bufIndx].len) {
          /* Move to the next message part. */
          transfer->offset = 0;
          transfer->bufIndx++;

          /* Send a repeated start when switching to read mode on the 2nd buffer. */
          if (seq->flags & I2C_FLAG_WRITE_READ) {
            transfer->state = i2cStateRStartAddrSend;
            continue;
          }

          /* Only writing from one buffer or finished both buffers. */
          if ((seq->flags & I2C_FLAG_WRITE) || (transfer->bufIndx > 1)) {
            transfer->state = i2cStateWFStopSent;
            i2c->CMD        = I2C_CMD_STOP;
            goto done;
          }

          /* Reprocess in case the next buffer is empty. */
          continue;
        }

        /* Send byte. */
        i2c->TXDATA     = (uint32_t)(seq->buf[transfer->bufIndx].data[transfer->offset++]);
        transfer->state = i2cStateDataWFAckNack;
        goto done;

      /*********************************************************/
      /* Wait for ACK/NACK from the slave after sending data to it. */
      /*********************************************************/
      case i2cStateDataWFAckNack:
        if (pending & I2C_IF_NACK) {
          I2C_IntClear(i2c, I2C_IF_NACK);
          transfer->result = i2cTransferNack;
          transfer->state  = i2cStateWFStopSent;
          i2c->CMD         = I2C_CMD_STOP;
        } else if (pending & I2C_IF_ACK) {
          I2C_IntClear(i2c, I2C_IF_ACK);
          transfer->state = i2cStateDataSend;
          continue;
        }
        goto done;

      /****************************/
      /* Wait for data from slave */
      /****************************/
      case i2cStateWFData:
        if (pending & I2C_IF_RXDATAV) {
          uint8_t       data;
          unsigned int  rxLen = seq->buf[transfer->bufIndx].len;

          /* Must read out data not to block further progress. */
          data = (uint8_t)(i2c->RXDATA);

          /* SW needs to clear RXDATAV IF on Series 2 devices.
             Flag is kept high by HW if buffer is not empty. */
#if defined(_SILICON_LABS_32B_SERIES_2)
          I2C_IntClear(i2c, I2C_IF_RXDATAV);
#endif

          /* Make sure that there is no storing beyond the end of the buffer (just in case). */
          if (transfer->offset < rxLen) {
            seq->buf[transfer->bufIndx].data[transfer->offset++] = data;
          }

          /* If all requested data is read, the sequence should end. */
          if (transfer->offset >= rxLen) {
            /* If receiving only one byte, transmit
               the NACK now before stopping. */
            if (1 == rxLen) {
              i2c->CMD  = I2C_CMD_NACK;
            }

            transfer->state = i2cStateWFStopSent;
            i2c->CMD        = I2C_CMD_STOP;
          } else {
            /* Send ACK and wait for the next byte. */
            i2c->CMD = I2C_CMD_ACK;

            if ( (1 < rxLen) && (transfer->offset == (rxLen - 1)) ) {
              /* If receiving more than one byte and this is the next
                 to last byte, transmit the NACK now before receiving
                 the last byte. */
              i2c->CMD  = I2C_CMD_NACK;
            }
          }
        }
        goto done;

      /***********************************/
      /* Wait for STOP to have been sent */
      /***********************************/
      case i2cStateWFStopSent:
        if (pending & I2C_IF_MSTOP) {
          I2C_IntClear(i2c, I2C_IF_MSTOP);
          transfer->state = i2cStateDone;
        }
        goto done;

      /******************************/
      /* An unexpected state, software fault */
      /******************************/
      default:
        transfer->result = i2cTransferSwFault;
        transfer->state  = i2cStateDone;
        goto done;
    }
  }

  done:

  if (transfer->state == i2cStateDone) {
    /* Disable interrupt sources when done. */
    i2c->IEN = 0;

    /* Update the result unless a fault has already occurred. */
    if (transfer->result == i2cTransferInProgress) {
      transfer->result = i2cTransferDone;
    }
  }
  /* Until transfer is done, keep returning i2cTransferInProgress. */
  else {
    return i2cTransferInProgress;
  }

  return transfer->result;
}

/***************************************************************************//**
 * @brief
 *   Prepare and start an I2C transfer (single master mode only).
 *
 * @details
 *   This function must be invoked to start an I2C transfer
 *   sequence. To complete the transfer, I2C_Transfer() must
 *   be used either in polled mode or by adding a small driver wrapper using
 *   interrupts.
 *
 * @note
 *   Only single master mode is supported.
 *
 * @param[in] i2c
 *   A pointer to the I2C peripheral register block.
 *
 * @param[in] seq
 *   A pointer to the sequence structure defining the I2C transfer to take place. The
 *   referenced structure must exist until the transfer has fully completed.
 *
 * @return
 *   Returns the status for an ongoing transfer:
 *   @li #i2cTransferInProgress - indicates that the transfer is not finished.
 *   @li Otherwise, an error has occurred.
 ******************************************************************************/
I2C_TransferReturn_TypeDef I2C_TransferInit(I2C_TypeDef *i2c,
                                            I2C_TransferSeq_TypeDef *seq)
{
  I2C_Transfer_TypeDef *transfer;

  EFM_ASSERT(I2C_REF_VALID(i2c));
  EFM_ASSERT(seq);

  /* Support up to 2 I2C buses. */
  if (i2c == I2C0) {
    transfer = i2cTransfer;
  }
#if (I2C_COUNT > 1)
  else if (i2c == I2C1) {
    transfer = i2cTransfer + 1;
  }
#endif
#if (I2C_COUNT > 2)
  else if (i2c == I2C2) {
    transfer = i2cTransfer + 2;
  }
#endif
  else {
    return i2cTransferUsageFault;
  }

  /* Check if in a busy state. Since this software assumes a single master, */
  /* issue an abort. The BUSY state is normal after a reset. */
  if (i2c->STATE & I2C_STATE_BUSY) {
    i2c->CMD = I2C_CMD_ABORT;
  }

  /* Do not try to read 0 bytes. It is not */
  /* possible according to the I2C spec, since the slave will always start */
  /* sending the first byte ACK on an address. The read operation can */
  /* only be stopped by NACKing a received byte, i.e., minimum 1 byte. */
  if (((seq->flags & I2C_FLAG_READ) && !(seq->buf[0].len))
      || ((seq->flags & I2C_FLAG_WRITE_READ) && !(seq->buf[1].len))
      ) {
    return i2cTransferUsageFault;
  }

  /* Prepare for a transfer. */
  transfer->state   = i2cStateStartAddrSend;
  transfer->result  = i2cTransferInProgress;
  transfer->offset  = 0;
  transfer->bufIndx = 0;
  transfer->seq     = seq;

  /* Ensure buffers are empty. */
  i2c->CMD = I2C_CMD_CLEARPC | I2C_CMD_CLEARTX;
  flushRx(i2c);

  /* Clear all pending interrupts prior to starting a transfer. */
  I2C_IntClear(i2c, _I2C_IF_MASK);

  /* Enable relevant interrupts. */
  /* Notice that the I2C interrupt must also be enabled in the NVIC, but */
  /* that is left for an additional driver wrapper. */
  i2c->IEN |= I2C_IEN_NACK | I2C_IEN_ACK | I2C_IEN_MSTOP
              | I2C_IEN_RXDATAV | I2C_IEN_ERRORS;

  /* Start a transfer. */
  return I2C_Transfer(i2c);
}

/** @} (end addtogroup I2C) */
/** @} (end addtogroup emlib) */
#endif /* defined(I2C_COUNT) && (I2C_COUNT > 0) */
