/***************************************************************************//**
 * @file em_burtc.h
 * @brief Backup Real Time Counter (BURTC) peripheral API
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

#ifndef EM_BURTC_H
#define EM_BURTC_H

#include "em_device.h"
#if defined(BURTC_PRESENT)

#include <stdbool.h>
#include "em_assert.h"
#include "em_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup BURTC
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** BURTC clock divisors. These values are valid for the BURTC prescaler. */
#define burtcClkDiv_1      1     /**< Divide clock by 1. */
#define burtcClkDiv_2      2     /**< Divide clock by 2. */
#define burtcClkDiv_4      4     /**< Divide clock by 4. */
#define burtcClkDiv_8      8     /**< Divide clock by 8. */
#define burtcClkDiv_16     16    /**< Divide clock by 16. */
#define burtcClkDiv_32     32    /**< Divide clock by 32. */
#define burtcClkDiv_64     64    /**< Divide clock by 64. */
#define burtcClkDiv_128    128   /**< Divide clock by 128. */

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

#if defined(_SILICON_LABS_32B_SERIES_0)
/** BURTC clock selection. */
typedef enum {
  /** Ultra low frequency (1 kHz) clock. */
  burtcClkSelULFRCO = BURTC_CTRL_CLKSEL_ULFRCO,
  /** Low frequency RC oscillator. */
  burtcClkSelLFRCO  = BURTC_CTRL_CLKSEL_LFRCO,
  /** Low frequency crystal osciallator. */
  burtcClkSelLFXO   = BURTC_CTRL_CLKSEL_LFXO
} BURTC_ClkSel_TypeDef;

/** BURTC mode of operation. */
typedef enum {
  /** Disable BURTC */
  burtcModeDisable = BURTC_CTRL_MODE_DISABLE,
  /** Enable and start BURTC counter in EM0 to EM2. */
  burtcModeEM2     = BURTC_CTRL_MODE_EM2EN,
  /** Enable and start BURTC counter in EM0 to EM3. */
  burtcModeEM3     = BURTC_CTRL_MODE_EM3EN,
  /** Enable and start BURTC counter in EM0 to EM4. */
  burtcModeEM4     = BURTC_CTRL_MODE_EM4EN,
} BURTC_Mode_TypeDef;

/** BURTC low power mode. */
typedef enum {
  /** Low Power Mode is disabled. */
  burtcLPDisable = BURTC_LPMODE_LPMODE_DISABLE,
  /** Low Power Mode is always enabled. */
  burtcLPEnable  = BURTC_LPMODE_LPMODE_ENABLE,
  /** Low Power Mode when system enters backup mode. */
  burtcLPBU      = BURTC_LPMODE_LPMODE_BUEN
} BURTC_LP_TypeDef;
#endif

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

#if defined(_SILICON_LABS_32B_SERIES_0)
/** BURTC initialization structure for Series 0 devices. */
typedef struct {
  bool                 enable;       /**< Enable BURTC after initialization (starts counter). */

  BURTC_Mode_TypeDef   mode;         /**< Configure energy mode operation. */
  bool                 debugRun;     /**< If true, counter will keep running under debug halt. */
  BURTC_ClkSel_TypeDef clkSel;       /**< Select clock source. */
  uint32_t             clkDiv;       /**< Clock divider; for ULFRCO 1Khz or 2kHz operation. */

  uint32_t             lowPowerComp; /**< Number of least significantt clock bits to ignore in low power mode. */
  bool                 timeStamp;    /**< Enable time stamp on entering backup power domain. */

  bool                 compare0Top;  /**< Set if Compare Value 0 is also top value (counter restart). */

  BURTC_LP_TypeDef     lowPowerMode; /**< Low power operation mode, requires LFXO or LFRCO. */
} BURTC_Init_TypeDef;

/** Default configuration for BURTC initialization structure. */
#define BURTC_INIT_DEFAULT \
  {                        \
    true,                  \
    burtcModeEM2,          \
    false,                 \
    burtcClkSelULFRCO,     \
    burtcClkDiv_1,         \
    0,                     \
    true,                  \
    false,                 \
    burtcLPDisable,        \
  }

#elif defined(_SILICON_LABS_32B_SERIES_2)
/** BURTC initialization structure for Series 2 devices. */
typedef struct {
  bool                 start;        /**< Start BURTC after initialization */
  bool                 debugRun;     /**< If true, counter will keep running under debug halt */
  uint32_t             clkDiv;       /**< Clock divider. Supported range is 1-32768 */
  bool                 compare0Top;  /**< Set if Compare Value 0 is also top value (counter restart) */
  bool                 em4comp;      /**< Enable EM4 wakeup on compare match. */
  bool                 em4overflow;  /**< Enable EM4 wakeup on counter overflow. */
} BURTC_Init_TypeDef;

/** Default configuration for BURTC init structure */
#define BURTC_INIT_DEFAULT \
  {                        \
    true,                  \
    false,                 \
    1,                     \
    0,                     \
    false,                 \
    false,                 \
  }
#endif

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Clear one or more pending BURTC interrupts.
 *
 * @param[in] flags
 *   BURTC interrupt sources to clear. Use a set of interrupt flags OR-ed
 *   together to clear multiple interrupt sources for the BURTC module
 *   (BURTC_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE void BURTC_IntClear(uint32_t flags)
{
#if defined(BURTC_HAS_SET_CLEAR)
  BURTC->IF_CLR = flags;
#else
  BURTC->IFC = flags;
#endif
}

/***************************************************************************//**
 * @brief
 *   Disable one or more BURTC interrupts.
 *
 * @param[in] flags
 *   BURTC interrupt sources to disable. Use a set of interrupt flags OR-ed
 *   together to disable multiple interrupt sources for the BURTC module
 *   (BURTC_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE void BURTC_IntDisable(uint32_t flags)
{
#if defined(BURTC_HAS_SET_CLEAR)
  BURTC->IEN_CLR = flags;
#else
  BURTC->IEN &= ~(flags);
#endif
}

/***************************************************************************//**
 * @brief
 *   Enable one or more BURTC interrupts.
 *
 * @note
 *   Depending on use, a pending interrupt may already be set prior to
 *   enabling the interrupt. Consider using BURTC_IntClear() prior to enabling
 *   if a pending interrupt should be ignored.
 *
 * @param[in] flags
 *   BURTC interrupt sources to enable. Use a set of interrupt flags OR-ed
 *   together to set multiple interrupt sources for the BURTC module
 *   (BURTC_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE void BURTC_IntEnable(uint32_t flags)
{
#if defined(BURTC_HAS_SET_CLEAR)
  BURTC->IEN_SET = flags;
#else
  BURTC->IEN |= flags;
#endif
}

/***************************************************************************//**
 * @brief
 *   Get pending BURTC interrupt flags.
 *
 * @note
 *   This function does not clear the event bits.
 *
 * @return
 *   Pending BURTC interrupt sources. Returns a set of interrupt flags OR-ed
 *   together for multiple interrupt sources in the BURTC module (BURTC_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t BURTC_IntGet(void)
{
  return BURTC->IF;
}

/***************************************************************************//**
 * @brief
 *   Get enabled and pending BURTC interrupt flags.
 *
 * @note
 *   The event bits are not cleared by the use of this function.
 *
 * @return
 *   Pending BURTC interrupt sources that is also enabled. Returns a set of
 *   interrupt flags OR-ed together for multiple interrupt sources in the
 *   BURTC module (BURTC_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t BURTC_IntGetEnabled(void)
{
  uint32_t tmp;

  /* Get enabled interrupts */
  tmp = BURTC->IEN;

  /* Return set interrupts */
  return BURTC->IF & tmp;
}

/***************************************************************************//**
 * @brief
 *   Set one or more pending BURTC interrupts from SW.
 *
 * @param[in] flags
 *   BURTC interrupt sources to set to pending. Use a set of interrupt flags
 *   OR-ed together to set multiple interrupt sources for the BURTC module
 *   (BURTC_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE void BURTC_IntSet(uint32_t flags)
{
#if defined(BURTC_HAS_SET_CLEAR)
  BURTC->IF_SET = flags;
#else
  BURTC->IFS = flags;
#endif
}

/***************************************************************************//**
 * @brief
 *   Status of BURTC RAM, timestamp and LP Mode
 *
 * @return A mask logially OR-ed status bits
 ******************************************************************************/
__STATIC_INLINE uint32_t BURTC_Status(void)
{
  return BURTC->STATUS;
}

#if defined(BURTC_CMD_CLRSTATUS)
/***************************************************************************//**
 * @brief
 *   Clear and reset BURTC status register
 ******************************************************************************/
__STATIC_INLINE void BURTC_StatusClear(void)
{
  BURTC->CMD = BURTC_CMD_CLRSTATUS;
}
#endif

/***************************************************************************//**
 * @brief
 *   Wait for the BURTC to complete all synchronization of register changes
 *   and commands.
 ******************************************************************************/
__STATIC_INLINE void BURTC_SyncWait(void)
{
#if defined(_SILICON_LABS_32B_SERIES_2)
  while ((BURTC->EN != 0U) && (BURTC->SYNCBUSY != 0U)) {
    /* Wait for previous synchronization to finish */
  }
#else
  while (BURTC->SYNCBUSY != 0U) {
    /* Wait for previous synchronization to finish */
  }
#endif
}

#if defined(_SILICON_LABS_32B_SERIES_2)
/***************************************************************************//**
 * @brief
 *   Start BURTC counter.
 *
 * @detail
 *   This function will send a start command to the BURTC peripheral. The BURTC
 *   peripheral will use some LF clock ticks before the command is executed.
 *   The @ref BURTC_SyncWait() function can be used to wait for the start command
 *   to be executed.
 *
 * @note
 *   This function requires the BURTC to be enabled.
 ******************************************************************************/
__STATIC_INLINE void BURTC_Start(void)
{
  BURTC_SyncWait();
  BURTC->CMD = BURTC_CMD_START;
}

/***************************************************************************//**
 * @brief
 *   Stop the BURTC counter.
 *
 * @detail
 *   This function will send a stop command to the BURTC peripheral. The BURTC
 *   peripheral will use some LF clock ticks before the command is executed.
 *   The @ref BURTC_SyncWait() function can be used to wait for the stop command
 *   to be executed.
 *
 * @note
 *   This function requires the BURTC to be enabled.
 ******************************************************************************/
__STATIC_INLINE void BURTC_Stop(void)
{
  BURTC_SyncWait();
  BURTC->CMD = BURTC_CMD_STOP;
}
#endif

/***************************************************************************//**
 * @brief Get BURTC counter
 *
 * @return
 *   BURTC counter value
 ******************************************************************************/
__STATIC_INLINE uint32_t BURTC_CounterGet(void)
{
  return BURTC->CNT;
}

#if defined(_SILICON_LABS_32B_SERIES_0)
/***************************************************************************//**
 * @brief Get BURTC timestamp for entering BU
 *
 * @return
 *   BURTC Time Stamp value
 ******************************************************************************/
__STATIC_INLINE uint32_t BURTC_TimestampGet(void)
{
  return BURTC->TIMESTAMP;
}

/***************************************************************************//**
 * @brief Freeze register updates until enabled
 * @param[in] enable If true, registers are not updated until enabled again.
 ******************************************************************************/
__STATIC_INLINE void BURTC_FreezeEnable(bool enable)
{
  BUS_RegBitWrite(&BURTC->FREEZE, _BURTC_FREEZE_REGFREEZE_SHIFT, enable);
}

/***************************************************************************//**
 * @brief Shut down power to rentention register bank.
 * @param[in] enable
 *     If true, shuts off power to retention registers.
 * @note
 *    When power rentention is disabled, it cannot be enabled again (until
 *    reset).
 ******************************************************************************/
__STATIC_INLINE void BURTC_Powerdown(bool enable)
{
  BUS_RegBitWrite(&BURTC->POWERDOWN, _BURTC_POWERDOWN_RAM_SHIFT, enable);
}

/***************************************************************************//**
 * @brief
 *   Set a value in one of the retention registers
 *
 * @param[in] num
 *   Register to set
 * @param[in] data
 *   Value to put into register
 ******************************************************************************/
__STATIC_INLINE void BURTC_RetRegSet(uint32_t num, uint32_t data)
{
  EFM_ASSERT(num <= 127);

  BURTC->RET[num].REG = data;
}

/***************************************************************************//**
 * @brief
 *   Read a value from one of the retention registers
 *
 * @param[in] num
 *   Retention Register to read
 ******************************************************************************/
__STATIC_INLINE uint32_t BURTC_RetRegGet(uint32_t num)
{
  EFM_ASSERT(num <= 127);

  return BURTC->RET[num].REG;
}
#endif

/***************************************************************************//**
 * @brief
 *   Lock BURTC registers, will protect from writing new config settings
 ******************************************************************************/
__STATIC_INLINE void BURTC_Lock(void)
{
  BURTC->LOCK = 0x0;
}

/***************************************************************************//**
 * @brief
 *   Unlock BURTC registers, enable write access to change configuration
 ******************************************************************************/
__STATIC_INLINE void BURTC_Unlock(void)
{
  BURTC->LOCK = BURTC_LOCK_LOCKKEY_UNLOCK;
}

void BURTC_Reset(void);
void BURTC_Init(const BURTC_Init_TypeDef *burtcInit);
void BURTC_Enable(bool enable);
void BURTC_CounterReset(void);
void BURTC_CompareSet(unsigned int comp, uint32_t value);
uint32_t BURTC_CompareGet(unsigned int comp);
uint32_t BURTC_ClockFreqGet(void);

/** @} (end addtogroup BURTC) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* BURTC_PRESENT */
#endif /* EM_BURTC_H */
