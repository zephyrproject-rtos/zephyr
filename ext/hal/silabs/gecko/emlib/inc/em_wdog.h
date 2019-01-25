/***************************************************************************//**
 * @file em_wdog.h
 * @brief Watchdog (WDOG) peripheral API
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

#ifndef EM_WDOG_H
#define EM_WDOG_H

#include "em_device.h"
#if defined(WDOG_COUNT) && (WDOG_COUNT > 0)

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup WDOG
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** Default WDOG instance for deprecated functions. */
#if !defined(DEFAULT_WDOG)
#if defined(WDOG0)
#define DEFAULT_WDOG WDOG0
#elif defined(WDOG)
#define DEFAULT_WDOG WDOG
#endif
#endif

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** Watchdog clock selection. */
#if defined(_WDOG_CTRL_CLKSEL_MASK)
typedef enum {
  wdogClkSelULFRCO = _WDOG_CTRL_CLKSEL_ULFRCO,   /**< Ultra low frequency (1 kHz) clock */
  wdogClkSelLFRCO  = _WDOG_CTRL_CLKSEL_LFRCO,    /**< Low frequency RC oscillator */
  wdogClkSelLFXO   = _WDOG_CTRL_CLKSEL_LFXO      /**< Low frequency crystal oscillator */
} WDOG_ClkSel_TypeDef;
#endif

/** Watchdog period selection. */
typedef enum {
  wdogPeriod_9    = 0x0, /**< 9 clock periods */
  wdogPeriod_17   = 0x1, /**< 17 clock periods */
  wdogPeriod_33   = 0x2, /**< 33 clock periods */
  wdogPeriod_65   = 0x3, /**< 65 clock periods */
  wdogPeriod_129  = 0x4, /**< 129 clock periods */
  wdogPeriod_257  = 0x5, /**< 257 clock periods */
  wdogPeriod_513  = 0x6, /**< 513 clock periods */
  wdogPeriod_1k   = 0x7, /**< 1025 clock periods */
  wdogPeriod_2k   = 0x8, /**< 2049 clock periods */
  wdogPeriod_4k   = 0x9, /**< 4097 clock periods */
  wdogPeriod_8k   = 0xA, /**< 8193 clock periods */
  wdogPeriod_16k  = 0xB, /**< 16385 clock periods */
  wdogPeriod_32k  = 0xC, /**< 32769 clock periods */
  wdogPeriod_64k  = 0xD, /**< 65537 clock periods */
  wdogPeriod_128k = 0xE, /**< 131073 clock periods */
  wdogPeriod_256k = 0xF  /**< 262145 clock periods */
} WDOG_PeriodSel_TypeDef;

#if defined(_WDOG_CTRL_WARNSEL_MASK) \
  || defined(_WDOG_CFG_WARNSEL_MASK)
/** Select watchdog warning timeout period as percentage of timeout. */
typedef enum {
  wdogWarnDisable   = 0,
  wdogWarnTime25pct = 1,
  wdogWarnTime50pct = 2,
  wdogWarnTime75pct = 3,
} WDOG_WarnSel_TypeDef;
#endif

#if defined(_WDOG_CTRL_WINSEL_MASK) \
  || defined(_WDOG_CFG_WINSEL_MASK)
/**  Select watchdog illegal window limit. */
typedef enum {
  wdogIllegalWindowDisable     = 0,
  wdogIllegalWindowTime12_5pct = 1,
  wdogIllegalWindowTime25_0pct = 2,
  wdogIllegalWindowTime37_5pct = 3,
  wdogIllegalWindowTime50_0pct = 4,
  wdogIllegalWindowTime62_5pct = 5,
  wdogIllegalWindowTime75_0pct = 6,
  wdogIllegalWindowTime87_5pct = 7,
} WDOG_WinSel_TypeDef;
#endif

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** Watchdog initialization structure. */
typedef struct {
  /** Enable watchdog when initialization completed. */
  bool                   enable;

  /** Counter keeps running during debug halt. */
  bool                   debugRun;

  /** Counter keeps running when in EM2. */
  bool                   em2Run;

  /** Counter keeps running when in EM3. */
  bool                   em3Run;

  /** Block EMU from entering EM4. */
  bool                   em4Block;

  /** Block SW from disabling LFRCO/LFXO oscillators. */
#if defined(_WDOG_CTRL_SWOSCBLOCK_MASK)
  bool                   swoscBlock;
#endif

  /** Block SW from modifying the configuration (a reset is needed to reconfigure). */
  bool                   lock;

  /** Clock source to use for watchdog. */
#if defined(_WDOG_CTRL_CLKSEL_MASK)
  WDOG_ClkSel_TypeDef    clkSel;
#endif

  /** Watchdog timeout period. */
  WDOG_PeriodSel_TypeDef perSel;

#if defined(_WDOG_CTRL_WARNSEL_MASK) \
  || defined(_WDOG_CFG_WARNSEL_MASK)
  /** Select warning time as % of the watchdog timeout */
  WDOG_WarnSel_TypeDef   warnSel;
#endif

#if defined(_WDOG_CTRL_WINSEL_MASK) \
  || defined(_WDOG_CFG_WINSEL_MASK)
  /** Select illegal window time as % of the watchdog timeout */
  WDOG_WinSel_TypeDef    winSel;
#endif

#if defined(_WDOG_CTRL_WDOGRSTDIS_MASK) \
  || defined(_WDOG_CFG_WDOGRSTDIS_MASK)
  /** Disable watchdog reset output if true */
  bool                   resetDisable;
#endif
} WDOG_Init_TypeDef;

/** Suggested default configuration for WDOG initialization structure. */
#if defined(_WDOG_CFG_MASK)
#define WDOG_INIT_DEFAULT                                                       \
  {                                                                             \
    true,                     /* Start watchdog when initialization is done. */ \
    false,                    /* WDOG is not counting during debug halt. */     \
    false,                    /* WDOG is not counting when in EM2. */           \
    false,                    /* WDOG is not counting when in EM3. */           \
    false,                    /* EM4 can be entered. */                         \
    false,                    /* Do not lock WDOG configuration. */             \
    wdogPeriod_256k,          /* Set longest possible timeout period. */        \
    wdogWarnDisable,          /* Disable warning interrupt. */                  \
    wdogIllegalWindowDisable, /* Disable illegal window interrupt. */           \
    false                     /* Do not disable reset. */                       \
  }
#elif defined(_WDOG_CTRL_WARNSEL_MASK)   \
  && defined(_WDOG_CTRL_WDOGRSTDIS_MASK) \
  && defined(_WDOG_CTRL_WINSEL_MASK)
#define WDOG_INIT_DEFAULT                                                       \
  {                                                                             \
    true,                     /* Start watchdog when initialization is done. */ \
    false,                    /* WDOG is not counting during debug halt. */     \
    false,                    /* WDOG is not counting when in EM2. */           \
    false,                    /* WDOG is not counting when in EM3. */           \
    false,                    /* EM4 can be entered. */                         \
    false,                    /* Do not block disabling LFRCO/LFXO in CMU. */   \
    false,                    /* Do not lock WDOG configuration. */             \
    wdogClkSelULFRCO,         /* Select 1kHZ WDOG oscillator. */                \
    wdogPeriod_256k,          /* Set longest possible timeout period. */        \
    wdogWarnDisable,          /* Disable warning interrupt. */                  \
    wdogIllegalWindowDisable, /* Disable illegal window interrupt. */           \
    false                     /* Do not disable reset. */                       \
  }
#else
#define WDOG_INIT_DEFAULT                                                       \
  {                                                                             \
    true,                     /* Start watchdog when initialization is done. */ \
    false,                    /* WDOG is not counting during debug halt. */     \
    false,                    /* WDOG is not counting when in EM2. */           \
    false,                    /* WDOG is not counting when in EM3. */           \
    false,                    /* EM4 can be entered. */                         \
    false,                    /* Do not block disabling LFRCO/LFXO in CMU. */   \
    false,                    /* Do not lock WDOG configuration. */             \
    wdogClkSelULFRCO,         /* Select 1kHZ WDOG oscillator. */                \
    wdogPeriod_256k           /* Set longest possible timeout period. */        \
  }
#endif

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

void WDOGn_Enable(WDOG_TypeDef *wdog, bool enable);
void WDOGn_Feed(WDOG_TypeDef *wdog);
void WDOGn_Init(WDOG_TypeDef *wdog, const WDOG_Init_TypeDef *init);
void WDOGn_Lock(WDOG_TypeDef *wdog);
void WDOGn_Unlock(WDOG_TypeDef *wdog);

#if defined(_WDOG_IF_MASK)
/***************************************************************************//**
 * @brief
 *   Clear one or more pending WDOG interrupts.
 *
 * @param[in] wdog
 *   Pointer to WDOG peripheral register block.
 *
 * @param[in] flags
 *   WDOG interrupt sources to clear. Use a set of interrupt flags OR-ed
 *   together to clear multiple interrupt sources.
 ******************************************************************************/
__STATIC_INLINE void WDOGn_IntClear(WDOG_TypeDef *wdog, uint32_t flags)
{
#if defined(WDOG_HAS_SET_CLEAR)
  wdog->IF_CLR = flags;
#else
  wdog->IFC = flags;
#endif
}

/***************************************************************************//**
 * @brief
 *   Disable one or more WDOG interrupts.
 *
 * @param[in] wdog
 *   Pointer to WDOG peripheral register block.
 *
 * @param[in] flags
 *   WDOG interrupt sources to disable. Use a set of interrupt flags OR-ed
 *   together to disable multiple interrupt.
 ******************************************************************************/
__STATIC_INLINE void WDOGn_IntDisable(WDOG_TypeDef *wdog, uint32_t flags)
{
#if defined(WDOG_HAS_SET_CLEAR)
  wdog->IEN_CLR = flags;
#else
  wdog->IEN &= ~flags;
#endif
}

/***************************************************************************//**
 * @brief
 *   Enable one or more WDOG interrupts.
 *
 * @note
 *   Depending on the use, a pending interrupt may already be set prior to
 *   enabling the interrupt. To ignore a pending interrupt, consider using
 *   WDOG_IntClear() prior to enabling the interrupt.
 *
 * @param[in] wdog
 *   Pointer to WDOG peripheral register block.
 *
 * @param[in] flags
 *   WDOG interrupt sources to enable. Use a set of interrupt flags OR-ed
 *   together to set multiple interrupt.
 ******************************************************************************/
__STATIC_INLINE void WDOGn_IntEnable(WDOG_TypeDef *wdog, uint32_t flags)
{
#if defined(WDOG_HAS_SET_CLEAR)
  wdog->IEN_SET = flags;
#else
  wdog->IEN |= flags;
#endif
}

/***************************************************************************//**
 * @brief
 *   Get pending WDOG interrupt flags.
 *
 * @note
 *   The event bits are not cleared by the use of this function.
 *
 * @param[in] wdog
 *   Pointer to WDOG peripheral register block.
 *
 * @return
 *   Pending WDOG interrupt sources. Returns a set of interrupt flags OR-ed
 *   together for the interrupt sources set.
 ******************************************************************************/
__STATIC_INLINE uint32_t WDOGn_IntGet(WDOG_TypeDef *wdog)
{
  return wdog->IF;
}

/***************************************************************************//**
 * @brief
 *   Get enabled and pending WDOG interrupt flags.
 *
 * @details
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @param[in] wdog
 *   Pointer to WDOG peripheral register block.
 *
 * @return
 *   Pending and enabled WDOG interrupt sources. Returns a set of interrupt
 *   flags OR-ed together for the interrupt sources set.
 ******************************************************************************/
__STATIC_INLINE uint32_t WDOGn_IntGetEnabled(WDOG_TypeDef *wdog)
{
  uint32_t tmp;

  tmp = wdog->IEN;

  /* Bitwise AND of pending and enabled interrupt flags. */
  return wdog->IF & tmp;
}

/***************************************************************************//**
 * @brief
 *   Set one or more pending WDOG interrupts from SW.
 *
 * @param[in] wdog
 *   Pointer to WDOG peripheral register block.
 *
 * @param[in] flags
 *   WDOG interrupt sources to set to pending. Use a set of interrupt flags
 *   (WDOG_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE void WDOGn_IntSet(WDOG_TypeDef *wdog, uint32_t flags)
{
#if defined(WDOG_HAS_SET_CLEAR)
  wdog->IF_SET = flags;
#else
  wdog->IFS = flags;
#endif
}
#endif

/***************************************************************************//**
 * @brief
 *   Get enabled status of the watchdog.
 *
 * @param[in] wdog
 *   Pointer to WDOG peripheral register block.
 ******************************************************************************/
__STATIC_INLINE bool WDOGn_IsEnabled(WDOG_TypeDef *wdog)
{
#if defined(_WDOG_EN_MASK)
  return (wdog->EN & _WDOG_EN_EN_MASK) == WDOG_EN_EN;
#else
  return (wdog->CTRL & _WDOG_CTRL_EN_MASK) == WDOG_CTRL_EN;
#endif
}

/***************************************************************************//**
 * @brief
 *   Get locked status of the watchdog.
 *
 * @param[in] wdog
 *   Pointer to WDOG peripheral register block.
 ******************************************************************************/
__STATIC_INLINE bool WDOGn_IsLocked(WDOG_TypeDef *wdog)
{
#if defined(_WDOG_STATUS_MASK)
  return (wdog->STATUS & _WDOG_STATUS_LOCK_MASK) == WDOG_STATUS_LOCK_LOCKED;
#else
  return (wdog->CTRL & _WDOG_CTRL_LOCK_MASK) == WDOG_CTRL_LOCK;
#endif
}

/***************************************************************************//**
 * @brief
 *   Enable/disable the watchdog timer.
 *
 * @deprecated
 *   Deprecated function. New code should use @ref WDOGn_Enable().
 *   This function uses @ref DEFAULT_WDOG.
 *
 * @param[in] enable
 *   Set to true to enable watchdog, false to disable. Watchdog cannot be
 *   disabled if watchdog has been locked.
 ******************************************************************************/
__STATIC_INLINE void WDOG_Enable(bool enable)
{
  WDOGn_Enable(DEFAULT_WDOG, enable);
}

/***************************************************************************//**
 * @brief
 *   Feed the watchdog.
 *
 * @deprecated
 *   Deprecated function. New code should use @ref WDOGn_Feed().
 *   This function uses @ref DEFAULT_WDOG.
 ******************************************************************************/
__STATIC_INLINE void WDOG_Feed(void)
{
  WDOGn_Feed(DEFAULT_WDOG);
}

/***************************************************************************//**
 * @brief
 *   Initialize watchdog (assuming the watchdog configuration has not been
 *   locked).
 *
 * @deprecated
 *   Deprecated function. New code should use @ref WDOGn_Init().
 *   This function uses @ref DEFAULT_WDOG.
 *
 * @param[in] init
 *   Structure holding watchdog configuration. A default setting
 *   #WDOG_INIT_DEFAULT is available for initialization.
 ******************************************************************************/
__STATIC_INLINE void WDOG_Init(const WDOG_Init_TypeDef *init)
{
  WDOGn_Init(DEFAULT_WDOG, init);
}

/***************************************************************************//**
 * @brief
 *   Lock the watchdog configuration.
 *
 * @deprecated
 *   Deprecated function. New code should use @ref WDOGn_Lock().
 *   This function uses @ref DEFAULT_WDOG.
 ******************************************************************************/
__STATIC_INLINE void WDOG_Lock(void)
{
  WDOGn_Lock(DEFAULT_WDOG);
}

/***************************************************************************//**
 * @brief
 *   Get enabled status of the watchdog.
 *
 * @deprecated
 *   Deprecated function. New code should use @ref WDOGn_IsEnabled().
 *   This function uses @ref DEFAULT_WDOG.
 ******************************************************************************/
__STATIC_INLINE bool WDOG_IsEnabled(void)
{
  return WDOGn_IsEnabled(DEFAULT_WDOG);
}

/***************************************************************************//**
 * @brief
 *   Get locked status of the watchdog.
 *
 * @deprecated
 *   Deprecated function. New code should use @ref WDOGn_IsLocked().
 *   This function uses @ref DEFAULT_WDOG.
 ******************************************************************************/
__STATIC_INLINE bool WDOG_IsLocked(void)
{
  return WDOGn_IsLocked(DEFAULT_WDOG);
}

/** @} (end addtogroup WDOG) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(WDOG_COUNT) && (WDOG_COUNT > 0) */
#endif /* EM_WDOG_H */
