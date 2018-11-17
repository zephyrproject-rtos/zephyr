/***************************************************************************//**
 * @file em_cmu.h
 * @brief Clock management unit (CMU) API
 * @version 5.6.0
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories, Inc. www.silabs.com</b>
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
#ifndef EM_CMU_H
#define EM_CMU_H

#include "em_device.h"
#if defined(CMU_PRESENT)

#include <stdbool.h>
#include "em_assert.h"
#include "em_bus.h"
#include "em_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup CMU
 * @{
 ******************************************************************************/

#if defined(_SILICON_LABS_32B_SERIES_2)

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** Clock divider configuration */
typedef uint32_t CMU_ClkDiv_TypeDef;

/** HFRCODPLL frequency bands */
typedef enum {
  cmuHFRCODPLLFreq_1M0Hz            = 1000000U,         /**< 1MHz RC band.  */
  cmuHFRCODPLLFreq_2M0Hz            = 2000000U,         /**< 2MHz RC band.  */
  cmuHFRCODPLLFreq_4M0Hz            = 4000000U,         /**< 4MHz RC band.  */
  cmuHFRCODPLLFreq_7M0Hz            = 7000000U,         /**< 7MHz RC band.  */
  cmuHFRCODPLLFreq_13M0Hz           = 13000000U,        /**< 13MHz RC band. */
  cmuHFRCODPLLFreq_16M0Hz           = 16000000U,        /**< 16MHz RC band. */
  cmuHFRCODPLLFreq_19M0Hz           = 19000000U,        /**< 19MHz RC band. */
  cmuHFRCODPLLFreq_26M0Hz           = 26000000U,        /**< 26MHz RC band. */
  cmuHFRCODPLLFreq_32M0Hz           = 32000000U,        /**< 32MHz RC band. */
  cmuHFRCODPLLFreq_38M0Hz           = 38000000U,        /**< 38MHz RC band. */
  cmuHFRCODPLLFreq_48M0Hz           = 48000000U,        /**< 48MHz RC band. */
  cmuHFRCODPLLFreq_56M0Hz           = 56000000U,        /**< 56MHz RC band. */
  cmuHFRCODPLLFreq_64M0Hz           = 64000000U,        /**< 64MHz RC band. */
  cmuHFRCODPLLFreq_80M0Hz           = 80000000U,        /**< 80MHz RC band. */
  cmuHFRCODPLLFreq_UserDefined      = 0,
} CMU_HFRCODPLLFreq_TypeDef;

/** HFRCODPLL maximum frequency */
#define CMU_HFRCODPLL_MIN       cmuHFRCODPLLFreq_1M0Hz
/** HFRCODPLL minimum frequency */
#define CMU_HFRCODPLL_MAX       cmuHFRCODPLLFreq_80M0Hz

/** HFRCOEM23 frequency bands */
typedef enum {
  cmuHFRCOEM23Freq_1M0Hz            = 1000000U,         /**< 1MHz RC band.  */
  cmuHFRCOEM23Freq_2M0Hz            = 2000000U,         /**< 2MHz RC band.  */
  cmuHFRCOEM23Freq_4M0Hz            = 4000000U,         /**< 4MHz RC band.  */
  cmuHFRCOEM23Freq_13M0Hz           = 13000000U,        /**< 13MHz RC band. */
  cmuHFRCOEM23Freq_16M0Hz           = 16000000U,        /**< 16MHz RC band. */
  cmuHFRCOEM23Freq_19M0Hz           = 19000000U,        /**< 19MHz RC band. */
  cmuHFRCOEM23Freq_26M0Hz           = 26000000U,        /**< 26MHz RC band. */
  cmuHFRCOEM23Freq_32M0Hz           = 32000000U,        /**< 32MHz RC band. */
  cmuHFRCOEM23Freq_40M0Hz           = 40000000U,        /**< 40MHz RC band. */
  cmuHFRCOEM23Freq_UserDefined      = 0,
} CMU_HFRCOEM23Freq_TypeDef;

/** HFRCOEM23 maximum frequency */
#define CMU_HFRCOEM23_MIN       cmuHFRCOEM23Freq_1M0Hz
/** HFRCOEM23 minimum frequency */
#define CMU_HFRCOEM23_MAX       cmuHFRCOEM23Freq_40M0Hz

/** Clock points in CMU clock-tree. */
typedef enum {
  /*******************/
  /* Clock branches  */
  /*******************/

  cmuClock_SYSCLK,                  /**< System clock.  */
  cmuClock_HCLK,                    /**< Core and AHB bus interface clock. */
  cmuClock_EXPCLK,                  /**< Export clock. */
  cmuClock_PCLK,                    /**< Peripheral APB bus interface clock. */
  cmuClock_LSPCLK,                  /**< Low speed peripheral APB bus interface clock. */
  cmuClock_IADCCLK,                 /**< IADC clock. */
  cmuClock_EM01GRPACLK,             /**< EM01GRPA clock. */
  cmuClock_EM23GRPACLK,             /**< EM23GRPA clock. */
  cmuClock_EM4GRPACLK,              /**< EM4GRPA clock. */
  cmuClock_WDOG0CLK,                /**< WDOG0 clock. */
  cmuClock_WDOG1CLK,                /**< WDOG1 clock. */
  cmuClock_DPLLREFCLK,              /**< DPLL reference clock. */
  cmuClock_TRACECLK,                /**< Debug trace clock. */
  cmuClock_RTCCCLK,                 /**< RTCC clock. */

  /*********************/
  /* Peripheral clocks */
  /*********************/

  cmuClock_CORE,                    /**< Cortex-M33 core clock. */
  cmuClock_SYSTICK,                 /**< Optional Cortex-M33 SYSTICK clock. */
  cmuClock_ACMP0,                   /**< ACMP0 clock. */
  cmuClock_ACMP1,                   /**< ACMP1 clock. */
  cmuClock_BURTC,                   /**< BURTC clock. */
  cmuClock_GPCRC,                   /**< GPCRC clock. */
  cmuClock_GPIO,                    /**< GPIO clock. */
  cmuClock_I2C0,                    /**< I2C0 clock. */
  cmuClock_I2C1,                    /**< I2C1 clock. */
  cmuClock_IADC0,                   /**< IADC clock. */
  cmuClock_LDMA,                    /**< RTCC clock. */
  cmuClock_LETIMER0,                /**< LETIMER clock. */
  cmuClock_PRS,                     /**< PRS clock. */
  cmuClock_RTCC,                    /**< RTCC clock. */
  cmuClock_TIMER0,                  /**< TIMER0 clock. */
  cmuClock_TIMER1,                  /**< TIMER1 clock. */
  cmuClock_TIMER2,                  /**< TIMER2 clock. */
  cmuClock_TIMER3,                  /**< TIMER3 clock. */
  cmuClock_USART0,                  /**< USART0 clock. */
  cmuClock_USART1,                  /**< USART1 clock. */
  cmuClock_USART2,                  /**< USART2 clock. */
  cmuClock_WDOG0,                   /**< WDOG0 clock. */
  cmuClock_WDOG1,                   /**< WDOG1 clock. */
} CMU_Clock_TypeDef;

/** Oscillator types. */
typedef enum {
  cmuOsc_LFXO,        /**< Low frequency crystal oscillator. */
  cmuOsc_LFRCO,       /**< Low frequency RC oscillator. */
  cmuOsc_FSRCO,       /**< Fast startup fixed frequency RC oscillator. */
  cmuOsc_HFXO,        /**< High frequency crystal oscillator. */
  cmuOsc_HFRCODPLL,   /**< High frequency RC and DPLL oscillator. */
  cmuOsc_HFRCOEM23,   /**< High frequency deep sleep RC oscillator. */
  cmuOsc_ULFRCO,      /**< Ultra low frequency RC oscillator. */
} CMU_Osc_TypeDef;

/** Selectable clock sources. */
typedef enum {
  cmuSelect_Error,       /**< Usage error. */
  cmuSelect_Disabled,    /**< Clock selector disabled. */
  cmuSelect_FSRCO,       /**< Fast startup fixed frequency RC oscillator. */
  cmuSelect_HFXO,        /**< High frequency crystal oscillator. */
  cmuSelect_HFRCODPLL,   /**< High frequency RC and DPLL oscillator. */
  cmuSelect_HFRCOEM23,   /**< High frequency deep sleep RC oscillator. */
  cmuSelect_CLKIN0,      /**< External clock input. */
  cmuSelect_LFXO,        /**< Low frequency crystal oscillator. */
  cmuSelect_LFRCO,       /**< Low frequency RC oscillator. */
  cmuSelect_ULFRCO,      /**< Ultra low frequency RC oscillator. */
  cmuSelect_PCLK,        /**< Peripheral APB bus interface clock. */
  cmuSelect_HCLK,        /**< Core and AHB bus interface clock. */
  cmuSelect_HCLKDIV1024, /**< Prescaled HCLK frequency clock. */
  cmuSelect_EM01GRPACLK, /**< EM01GRPA clock. */
  cmuSelect_EXPCLK,      /**< Pin export clock. */
  cmuSelect_PRS          /**< PRS input as clock. */
} CMU_Select_TypeDef;

/** DPLL reference clock edge detect selector. */
typedef enum {
  cmuDPLLEdgeSel_Fall = 0,    /**< Detect falling edge of reference clock. */
  cmuDPLLEdgeSel_Rise = 1     /**< Detect rising edge of reference clock. */
} CMU_DPLLEdgeSel_TypeDef;

/** DPLL lock mode selector. */
typedef enum {
  cmuDPLLLockMode_Freq  = _DPLL_CFG_MODE_FLL,   /**< Frequency lock mode. */
  cmuDPLLLockMode_Phase = _DPLL_CFG_MODE_PLL    /**< Phase lock mode. */
} CMU_DPLLLockMode_TypeDef;

/** LFXO oscillator modes. */
typedef enum {
  cmuLfxoOscMode_Crystal       = _LFXO_CFG_MODE_XTAL,      /**< Crystal oscillator. */
  cmuLfxoOscMode_AcCoupledSine = _LFXO_CFG_MODE_BUFEXTCLK, /**< External AC coupled sine. */
  cmuLfxoOscMode_External      = _LFXO_CFG_MODE_DIGEXTCLK, /**< External digital clock. */
} CMU_LfxoOscMode_TypeDef;

/** LFXO start-up timeout delay. */
typedef enum {
  cmuLfxoStartupDelay_2Cycles   = _LFXO_CFG_TIMEOUT_CYCLES2,   /**< 2 cycles start-up delay. */
  cmuLfxoStartupDelay_256Cycles = _LFXO_CFG_TIMEOUT_CYCLES256, /**< 256 cycles start-up delay. */
  cmuLfxoStartupDelay_1KCycles  = _LFXO_CFG_TIMEOUT_CYCLES1K,  /**< 1K cycles start-up delay. */
  cmuLfxoStartupDelay_2KCycles  = _LFXO_CFG_TIMEOUT_CYCLES2K,  /**< 2K cycles start-up delay. */
  cmuLfxoStartupDelay_4KCycles  = _LFXO_CFG_TIMEOUT_CYCLES4K,  /**< 4K cycles start-up delay. */
  cmuLfxoStartupDelay_8KCycles  = _LFXO_CFG_TIMEOUT_CYCLES8K,  /**< 8K cycles start-up delay. */
  cmuLfxoStartupDelay_16KCycles = _LFXO_CFG_TIMEOUT_CYCLES16K, /**< 16K cycles start-up delay. */
  cmuLfxoStartupDelay_32KCycles = _LFXO_CFG_TIMEOUT_CYCLES32K, /**< 32K cycles start-up delay. */
} CMU_LfxoStartupDelay_TypeDef;

/** HFXO oscillator modes. */
typedef enum {
  cmuHfxoOscMode_Crystal      = _HFXO_CFG_MODE_XTAL,   /**< Crystal oscillator. */
  cmuHfxoOscMode_ExternalSine = _HFXO_CFG_MODE_EXTCLK, /**< External digital clock. */
} CMU_HfxoOscMode_TypeDef;

/** HFXO core bias LSB change timeout. */
typedef enum {
  cmuHfxoCbLsbTimeout_8us    = _HFXO_XTALCFG_TIMEOUTCBLSB_T8US,    /**< 8 us timeout. */
  cmuHfxoCbLsbTimeout_20us   = _HFXO_XTALCFG_TIMEOUTCBLSB_T20US,   /**< 20 us timeout. */
  cmuHfxoCbLsbTimeout_41us   = _HFXO_XTALCFG_TIMEOUTCBLSB_T41US,   /**< 41 us timeout. */
  cmuHfxoCbLsbTimeout_62us   = _HFXO_XTALCFG_TIMEOUTCBLSB_T62US,   /**< 62 us timeout. */
  cmuHfxoCbLsbTimeout_83us   = _HFXO_XTALCFG_TIMEOUTCBLSB_T83US,   /**< 83 us timeout. */
  cmuHfxoCbLsbTimeout_104us  = _HFXO_XTALCFG_TIMEOUTCBLSB_T104US,  /**< 104 us timeout. */
  cmuHfxoCbLsbTimeout_125us  = _HFXO_XTALCFG_TIMEOUTCBLSB_T125US,  /**< 125 us timeout. */
  cmuHfxoCbLsbTimeout_166us  = _HFXO_XTALCFG_TIMEOUTCBLSB_T166US,  /**< 166 us timeout. */
  cmuHfxoCbLsbTimeout_208us  = _HFXO_XTALCFG_TIMEOUTCBLSB_T208US,  /**< 208 us timeout. */
  cmuHfxoCbLsbTimeout_250us  = _HFXO_XTALCFG_TIMEOUTCBLSB_T250US,  /**< 250 us timeout. */
  cmuHfxoCbLsbTimeout_333us  = _HFXO_XTALCFG_TIMEOUTCBLSB_T333US,  /**< 333 us timeout. */
  cmuHfxoCbLsbTimeout_416us  = _HFXO_XTALCFG_TIMEOUTCBLSB_T416US,  /**< 416 us timeout. */
  cmuHfxoCbLsbTimeout_833us  = _HFXO_XTALCFG_TIMEOUTCBLSB_T833US,  /**< 833 us timeout. */
  cmuHfxoCbLsbTimeout_1250us = _HFXO_XTALCFG_TIMEOUTCBLSB_T1250US, /**< 1250 us timeout. */
  cmuHfxoCbLsbTimeout_2083us = _HFXO_XTALCFG_TIMEOUTCBLSB_T2083US, /**< 2083 us timeout. */
  cmuHfxoCbLsbTimeout_3750us = _HFXO_XTALCFG_TIMEOUTCBLSB_T3750US, /**< 3750 us timeout. */
} CMU_HfxoCbLsbTimeout_TypeDef;

/** HFXO steady state timeout. */
typedef enum {
  cmuHfxoSteadyStateTimeout_16us   = _HFXO_XTALCFG_TIMEOUTSTEADY_T16US,   /**< 16 us timeout. */
  cmuHfxoSteadyStateTimeout_41us   = _HFXO_XTALCFG_TIMEOUTSTEADY_T41US,   /**< 41 us timeout. */
  cmuHfxoSteadyStateTimeout_83us   = _HFXO_XTALCFG_TIMEOUTSTEADY_T83US,   /**< 83 us timeout. */
  cmuHfxoSteadyStateTimeout_125us  = _HFXO_XTALCFG_TIMEOUTSTEADY_T125US,  /**< 125 us timeout. */
  cmuHfxoSteadyStateTimeout_166us  = _HFXO_XTALCFG_TIMEOUTSTEADY_T166US,  /**< 166 us timeout. */
  cmuHfxoSteadyStateTimeout_208us  = _HFXO_XTALCFG_TIMEOUTSTEADY_T208US,  /**< 208 us timeout. */
  cmuHfxoSteadyStateTimeout_250us  = _HFXO_XTALCFG_TIMEOUTSTEADY_T250US,  /**< 250 us timeout. */
  cmuHfxoSteadyStateTimeout_333us  = _HFXO_XTALCFG_TIMEOUTSTEADY_T333US,  /**< 333 us timeout. */
  cmuHfxoSteadyStateTimeout_416us  = _HFXO_XTALCFG_TIMEOUTSTEADY_T416US,  /**< 416 us timeout. */
  cmuHfxoSteadyStateTimeout_500us  = _HFXO_XTALCFG_TIMEOUTSTEADY_T500US,  /**< 500 us timeout. */
  cmuHfxoSteadyStateTimeout_666us  = _HFXO_XTALCFG_TIMEOUTSTEADY_T666US,  /**< 666 us timeout. */
  cmuHfxoSteadyStateTimeout_833us  = _HFXO_XTALCFG_TIMEOUTSTEADY_T833US,  /**< 833 us timeout. */
  cmuHfxoSteadyStateTimeout_1666us = _HFXO_XTALCFG_TIMEOUTSTEADY_T1666US, /**< 1666 us timeout. */
  cmuHfxoSteadyStateTimeout_2500us = _HFXO_XTALCFG_TIMEOUTSTEADY_T2500US, /**< 2500 us timeout. */
  cmuHfxoSteadyStateTimeout_4166us = _HFXO_XTALCFG_TIMEOUTSTEADY_T4166US, /**< 4166 us timeout. */
  cmuHfxoSteadyStateTimeout_7500us = _HFXO_XTALCFG_TIMEOUTSTEADY_T7500US, /**< 7500 us timeout. */
} CMU_HfxoSteadyStateTimeout_TypeDef;

/** HFXO core degeneration control. */
typedef enum {
  cmuHfxoCoreDegen_None = _HFXO_XTALCTRL_COREDGENANA_NONE,    /**< No core degeneration. */
  cmuHfxoCoreDegen_33   = _HFXO_XTALCTRL_COREDGENANA_DGEN33,  /**< Core degeneration control 33. */
  cmuHfxoCoreDegen_50   = _HFXO_XTALCTRL_COREDGENANA_DGEN50,  /**< Core degeneration control 50. */
  cmuHfxoCoreDegen_100  = _HFXO_XTALCTRL_COREDGENANA_DGEN100, /**< Core degeneration control 100. */
} CMU_HfxoCoreDegen_TypeDef;

/** HFXO XI and XO pin fixed capacitor control. */
typedef enum {
  cmuHfxoCtuneFixCap_None = _HFXO_XTALCTRL_CTUNEFIXANA_NONE,  /**< No fixed capacitors. */
  cmuHfxoCtuneFixCap_Xi   = _HFXO_XTALCTRL_CTUNEFIXANA_XI,    /**< Fixed capacitor on XI pin. */
  cmuHfxoCtuneFixCap_Xo   = _HFXO_XTALCTRL_CTUNEFIXANA_XO,    /**< Fixed capacitor on XO pin. */
  cmuHfxoCtuneFixCap_Both = _HFXO_XTALCTRL_CTUNEFIXANA_BOTH,  /**< Fixed capacitor on both pins. */
} CMU_HfxoCtuneFixCap_TypeDef;

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** LFXO initialization structure. Init values should be obtained from a
    configuration tool, app. note or xtal datasheet.  */
typedef struct {
  uint8_t   gain;                       /**< Startup gain. */
  uint8_t   capTune;                    /**< Internal capacitance tuning. */
  CMU_LfxoStartupDelay_TypeDef timeout; /**< Startup delay. */
  CMU_LfxoOscMode_TypeDef mode;         /**< Oscillator mode. */
  bool      highAmplitudeEn;            /**< High amplitude enable. */
  bool      agcEn;                      /**< AGC enable. */
  bool      failDetEM4WUEn;             /**< EM4 wakeup on failure enable. */
  bool      failDetEn;              /**< Oscillator failure detection enable. */
  bool      disOnDemand;                /**< Disable on-demand requests. */
  bool      forceEn;                    /**< Force oscillator enable. */
  bool      regLock;                    /**< Lock register access. */
} CMU_LFXOInit_TypeDef;

/** Default LFXO initialization values for XTAL mode. */
#define CMU_LFXOINIT_DEFAULT                      \
  {                                               \
    1,                                            \
    38,                                           \
    cmuLfxoStartupDelay_32KCycles,                \
    cmuLfxoOscMode_Crystal,                       \
    false,                  /* highAmplitudeEn */ \
    true,                   /* agcEn           */ \
    false,                  /* failDetEM4WUEn  */ \
    false,                  /* failDetEn       */ \
    false,                  /* DisOndemand     */ \
    false,                  /* ForceEn         */ \
    false                   /* Lock registers  */ \
  }

/** Default LFXO initialization values for external clock mode. */
#define CMU_LFXOINIT_EXTERNAL_CLOCK               \
  {                                               \
    0U,                                           \
    0U,                                           \
    cmuLfxoStartupDelay_2Cycles,                  \
    cmuLfxoOscMode_External,                      \
    false,                  /* highAmplitudeEn */ \
    false,                  /* agcEn           */ \
    false,                  /* failDetEM4WUEn  */ \
    false,                  /* failDetEn       */ \
    false,                  /* DisOndemand     */ \
    false,                  /* ForceEn         */ \
    false                   /* Lock registers  */ \
  }

/** Default LFXO initialization values for external sine mode. */
#define CMU_LFXOINIT_EXTERNAL_SINE                \
  {                                               \
    0U,                                           \
    0U,                                           \
    cmuLfxoStartupDelay_2Cycles,                  \
    cmuLfxoOscMode_AcCoupledSine,                 \
    false,                  /* highAmplitudeEn */ \
    false,                  /* agcEn           */ \
    false,                  /* failDetEM4WUEn  */ \
    false,                  /* failDetEn       */ \
    false,                  /* DisOndemand     */ \
    false,                  /* ForceEn         */ \
    false                   /* Lock registers  */ \
  }

/** HFXO initialization structure. Init values should be obtained from a
    configuration tool, app. note or xtal datasheet. */
typedef struct {
  CMU_HfxoCbLsbTimeout_TypeDef        timeoutCbLsb;            /**< Core bias change timeout. */
  CMU_HfxoSteadyStateTimeout_TypeDef  timeoutSteadyFirstLock;  /**< Steady state timeout duration for first lock. */
  CMU_HfxoSteadyStateTimeout_TypeDef  timeoutSteady;           /**< Steady state timeout duration. */
  uint8_t                             ctuneXoStartup;          /**< XO pin startup tuning capacitance. */
  uint8_t                             ctuneXiStartup;          /**< XI pin startup tuning capacitance. */
  uint8_t                             coreBiasStartup;         /**< Core bias startup current. */
  uint8_t                             imCoreBiasStartup;       /**< Core bias intermediate startup current. */
  CMU_HfxoCoreDegen_TypeDef           coreDegenAna;            /**< Core degeneration control. */
  CMU_HfxoCtuneFixCap_TypeDef         ctuneFixAna;             /**< Fixed tuning capacitance on XI/XO. */
  uint8_t                             ctuneXoAna;              /**< Tuning capacitance on XO. */
  uint8_t                             ctuneXiAna;              /**< Tuning capacitance on XI. */
  uint8_t                             coreBiasAna;             /**< Core bias current. */
  bool                                enXiDcBiasAna;           /**< Enable XI internal DC bias. */
  CMU_HfxoOscMode_TypeDef             mode;                    /**< Oscillator mode. */
  bool                                forceXo2GndAna;          /**< Force XO pin to ground. */
  bool                                forceXi2GndAna;          /**< Force XI pin to ground. */
  bool                                disOnDemand;             /**< Disable on-demand requests. */
  bool                                forceEn;                 /**< Force oscillator enable. */
  bool                                regLock;                 /**< Lock register access. */
} CMU_HFXOInit_TypeDef;

/** Default HFXO initialization values for XTAL mode. */
#define CMU_HFXOINIT_DEFAULT                                        \
  {                                                                 \
    cmuHfxoCbLsbTimeout_416us,                                      \
    cmuHfxoSteadyStateTimeout_833us,  /* First lock              */ \
    cmuHfxoSteadyStateTimeout_83us,   /* Subsequent locks        */ \
    0U,                         /* ctuneXoStartup                */ \
    0U,                         /* ctuneXiStartup                */ \
    32U,                        /* coreBiasStartup               */ \
    32U,                        /* imCoreBiasStartup             */ \
    cmuHfxoCoreDegen_None,                                          \
    cmuHfxoCtuneFixCap_Both,                                        \
    60U,                        /* ctuneXoAna                    */ \
    60U,                        /* ctuneXiAna                    */ \
    60U,                        /* coreBiasAna                   */ \
    false,                      /* enXiDcBiasAna                 */ \
    cmuHfxoOscMode_Crystal,                                         \
    false,                      /* forceXo2GndAna                */ \
    false,                      /* forceXi2GndAna                */ \
    false,                      /* DisOndemand                   */ \
    false,                      /* ForceEn                       */ \
    false                       /* Lock registers                */ \
  }

/** Default HFXO initialization values for external sine mode. */
#define CMU_HFXOINIT_EXTERNAL_SINE                                            \
  {                                                                           \
    (CMU_HfxoCbLsbTimeout_TypeDef)0,       /* timeoutCbLsb                 */ \
    (CMU_HfxoSteadyStateTimeout_TypeDef)0, /* timeoutSteady, first lock    */ \
    (CMU_HfxoSteadyStateTimeout_TypeDef)0, /* timeoutSteady, subseq. locks */ \
    0U,                         /* ctuneXoStartup                */           \
    0U,                         /* ctuneXiStartup                */           \
    0U,                         /* coreBiasStartup               */           \
    0U,                         /* imCoreBiasStartup             */           \
    cmuHfxoCoreDegen_None,                                                    \
    cmuHfxoCtuneFixCap_None,                                                  \
    0U,                         /* ctuneXoAna                    */           \
    0U,                         /* ctuneXiAna                    */           \
    0U,                         /* coreBiasAna                   */           \
    false, /* enXiDcBiasAna, false=DC true=AC coupling of signal */           \
    cmuHfxoOscMode_ExternalSine,                                              \
    false,                      /* forceXo2GndAna                */           \
    false,                      /* forceXi2GndAna                */           \
    false,                      /* DisOndemand                   */           \
    false,                      /* ForceEn                       */           \
    false                       /* Lock registers                */           \
  }

/** DPLL initialization structure. Frequency will be Fref*(N+1)/(M+1). */
typedef struct {
  uint32_t  frequency;                  /**< PLL frequency value, max 80 MHz. */
  uint16_t  n;                          /**< Factor N. 300 <= N <= 4095       */
  uint16_t  m;                          /**< Factor M. M <= 4095              */
  CMU_Select_TypeDef        refClk;     /**< Reference clock selector.        */
  CMU_DPLLEdgeSel_TypeDef   edgeSel;    /**< Reference clock edge detect selector. */
  CMU_DPLLLockMode_TypeDef  lockMode;   /**< DPLL lock mode selector.         */
  bool      autoRecover;                /**< Enable automatic lock recovery.  */
  bool      ditherEn;                   /**< Enable dither functionalityery.  */
} CMU_DPLLInit_TypeDef;

/**
 * DPLL initialization values for 39,998,805 Hz using LFXO as reference
 * clock, M=2 and N=3661.
 */
#define CMU_DPLL_LFXO_TO_40MHZ                                             \
  {                                                                        \
    39998805,                     /* Target frequency.                  */ \
    3661,                         /* Factor N.                          */ \
    2,                            /* Factor M.                          */ \
    cmuSelect_LFXO,               /* Select LFXO as reference clock.    */ \
    cmuDPLLEdgeSel_Fall,          /* Select falling edge of ref clock.  */ \
    cmuDPLLLockMode_Freq,         /* Use frequency lock mode.           */ \
    true,                         /* Enable automatic lock recovery.    */ \
    false                         /* Don't enable dither function.      */ \
  }

/**
 * Default configurations for DPLL initialization. When using this macro
 * you need to modify the N and M factor and the desired frequency to match
 * the components placed on the board.
 */
#define CMU_DPLLINIT_DEFAULT                                               \
  {                                                                        \
    80000000,                     /* Target frequency.                  */ \
    (4000 - 1),                   /* Factor N.                          */ \
    (1920 - 1),                   /* Factor M.                          */ \
    cmuSelect_HFXO,               /* Select HFXO as reference clock.    */ \
    cmuDPLLEdgeSel_Fall,          /* Select falling edge of ref clock.  */ \
    cmuDPLLLockMode_Freq,         /* Use frequency lock mode.           */ \
    true,                         /* Enable automatic lock recovery.    */ \
    false                         /* Don't enable dither function.      */ \
  }

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/
uint32_t                   CMU_Calibrate(uint32_t cycles,
                                         CMU_Select_TypeDef reference);
void                       CMU_CalibrateConfig(uint32_t downCycles,
                                               CMU_Select_TypeDef downSel,
                                               CMU_Select_TypeDef upSel);
uint32_t                   CMU_CalibrateCountGet(void);
void                       CMU_ClkOutPinConfig(uint32_t           clkno,
                                               CMU_Select_TypeDef sel,
                                               CMU_ClkDiv_TypeDef clkdiv,
                                               GPIO_Port_TypeDef  port,
                                               unsigned int       pin);
CMU_ClkDiv_TypeDef         CMU_ClockDivGet(CMU_Clock_TypeDef clock);
void                       CMU_ClockDivSet(CMU_Clock_TypeDef clock,
                                           CMU_ClkDiv_TypeDef div);
uint32_t                   CMU_ClockFreqGet(CMU_Clock_TypeDef clock);
CMU_Select_TypeDef         CMU_ClockSelectGet(CMU_Clock_TypeDef clock);
void                       CMU_ClockSelectSet(CMU_Clock_TypeDef clock,
                                              CMU_Select_TypeDef ref);
CMU_HFRCODPLLFreq_TypeDef  CMU_HFRCODPLLBandGet(void);
void                       CMU_HFRCODPLLBandSet(CMU_HFRCODPLLFreq_TypeDef freq);
CMU_HFRCOEM23Freq_TypeDef  CMU_HFRCOEM23BandGet(void);
void                       CMU_HFRCOEM23BandSet(CMU_HFRCOEM23Freq_TypeDef freq);
bool                       CMU_DPLLLock(const CMU_DPLLInit_TypeDef *init);
void                       CMU_HFXOInit(const CMU_HFXOInit_TypeDef *hfxoInit);
void                       CMU_LFXOInit(const CMU_LFXOInit_TypeDef *lfxoInit);
uint32_t                   CMU_OscillatorTuningGet(CMU_Osc_TypeDef osc);
void                       CMU_OscillatorTuningSet(CMU_Osc_TypeDef osc,
                                                   uint32_t val);
void                       CMU_UpdateWaitStates(uint32_t freq, int vscale);

/***************************************************************************//**
 * @brief
 *   Configures continuous calibration mode.
 * @param[in] enable
 *   If true, enables continuous calibration, if false disables continuous
 *   calibration.
 ******************************************************************************/
__STATIC_INLINE void CMU_CalibrateCont(bool enable)
{
  BUS_RegBitWrite(&CMU->CALCTRL, _CMU_CALCTRL_CONT_SHIFT, (uint32_t)enable);
}

/***************************************************************************//**
 * @brief
 *   Starts calibration.
 * @note
 *   This call is usually invoked after @ref CMU_CalibrateConfig() and possibly
 *   @ref CMU_CalibrateCont().
 ******************************************************************************/
__STATIC_INLINE void CMU_CalibrateStart(void)
{
  CMU->CALCMD = CMU_CALCMD_CALSTART;
}

/***************************************************************************//**
 * @brief
 *   Stop calibration counters.
 ******************************************************************************/
__STATIC_INLINE void CMU_CalibrateStop(void)
{
  CMU->CALCMD = CMU_CALCMD_CALSTOP;
}

/***************************************************************************//**
 * @brief
 *   Enable/disable a clock.
 *
 * @note
 *   This is a dummy function to solve backward compatibility issues.
 *
 * @param[in] clock
 *   The clock to enable/disable.
 *
 * @param[in] enable
 *   @li true - enable specified clock.
 *   @li false - disable specified clock.
 ******************************************************************************/
__STATIC_INLINE void CMU_ClockEnable(CMU_Clock_TypeDef clock, bool enable)
{
  (void)clock;
  (void)enable;
}

/***************************************************************************//**
 * @brief
 *   Unlock the DPLL.
 * @note
 *   The HFRCODPLL oscillator is not turned off.
 ******************************************************************************/
__STATIC_INLINE void CMU_DPLLUnlock(void)
{
  DPLL0->EN_CLR = DPLL_EN_EN;
}

/***************************************************************************//**
 * @brief
 *   Clear one or more pending CMU interrupt flags.
 *
 * @param[in] flags
 *   CMU interrupt sources to clear.
 ******************************************************************************/
__STATIC_INLINE void CMU_IntClear(uint32_t flags)
{
  CMU->IF_CLR = flags;
}

/***************************************************************************//**
 * @brief
 *   Disable one or more CMU interrupt sources.
 *
 * @param[in] flags
 *   CMU interrupt sources to disable.
 ******************************************************************************/
__STATIC_INLINE void CMU_IntDisable(uint32_t flags)
{
  CMU->IEN_CLR = flags;
}

/***************************************************************************//**
 * @brief
 *   Enable one or more CMU interrupt sources.
 *
 * @note
 *   Depending on the use, a pending interrupt may already be set prior to
 *   enabling the interrupt. Consider using @ref CMU_IntClear() prior to
 *   enabling if such a pending interrupt should be ignored.
 *
 * @param[in] flags
 *   CMU interrupt sources to enable.
 ******************************************************************************/
__STATIC_INLINE void CMU_IntEnable(uint32_t flags)
{
  CMU->IEN_SET = flags;
}

/***************************************************************************//**
 * @brief
 *   Get pending CMU interrupt sources.
 *
 * @return
 *   CMU interrupt sources pending.
 ******************************************************************************/
__STATIC_INLINE uint32_t CMU_IntGet(void)
{
  return CMU->IF;
}

/***************************************************************************//**
 * @brief
 *   Get enabled and pending CMU interrupt flags.
 *
 * @details
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @note
 *   The event bits are not cleared by the use of this function.
 *
 * @return
 *   Pending and enabled CMU interrupt sources.
 *   The return value is the bitwise AND of
 *   - the enabled interrupt sources in CMU_IEN and
 *   - the pending interrupt flags CMU_IF
 ******************************************************************************/
__STATIC_INLINE uint32_t CMU_IntGetEnabled(void)
{
  uint32_t ien;

  ien = CMU->IEN;
  return CMU->IF & ien;
}

/**************************************************************************//**
 * @brief
 *   Set one or more pending CMU interrupt sources.
 *
 * @param[in] flags
 *   CMU interrupt sources to set to pending.
 *****************************************************************************/
__STATIC_INLINE void CMU_IntSet(uint32_t flags)
{
  CMU->IF_SET = flags;
}

/***************************************************************************//**
 * @brief
 *   Lock CMU register access in order to protect registers contents against
 *   unintended modification.
 *
 * @details
 *   Please refer to the reference manual for CMU registers that will be
 *   locked.
 *
 * @note
 *   If locking the CMU registers, they must be unlocked prior to using any
 *   CMU API functions modifying CMU registers protected by the lock.
 ******************************************************************************/
__STATIC_INLINE void CMU_Lock(void)
{
  CMU->LOCK = ~CMU_LOCK_LOCKKEY_UNLOCK;
}

/***************************************************************************//**
 * @brief
 *   Enable/disable oscillator.
 *
 * @note
 *   This is a dummy function to solve backward compatibility issues.
 *
 * @param[in] osc
 *   The oscillator to enable/disable.
 *
 * @param[in] enable
 *   @li true - enable specified oscillator.
 *   @li false - disable specified oscillator.
 *
 * @param[in] wait
 *   Only used if @p enable is true.
 *   @li true - wait for oscillator start-up time to timeout before returning.
 *   @li false - do not wait for oscillator start-up time to timeout before
 *     returning.
 ******************************************************************************/
__STATIC_INLINE void CMU_OscillatorEnable(CMU_Osc_TypeDef osc,
                                          bool enable,
                                          bool wait)
{
  (void)osc;
  (void)enable;
  (void)wait;
}

/***************************************************************************//**
 * @brief
 *   Unlock CMU register access so that writing to registers is possible.
 ******************************************************************************/
__STATIC_INLINE void CMU_Unlock(void)
{
  CMU->LOCK = CMU_LOCK_LOCKKEY_UNLOCK;
}

/***************************************************************************//**
 * @brief
 *   Lock WDOG register access in order to protect registers contents against
 *   unintended modification.
 *
 * @note
 *   If locking the WDOG registers, they must be unlocked prior to using any
 *   emlib API functions modifying registers protected by the lock.
 ******************************************************************************/
__STATIC_INLINE void CMU_WdogLock(void)
{
  CMU->WDOGLOCK = ~CMU_WDOGLOCK_LOCKKEY_UNLOCK;
}

/***************************************************************************//**
 * @brief
 *   Unlock WDOG register access so that writing to registers is possible.
 ******************************************************************************/
__STATIC_INLINE void CMU_WdogUnlock(void)
{
  CMU->WDOGLOCK = CMU_WDOGLOCK_LOCKKEY_UNLOCK;
}

#else // defined(_SILICON_LABS_32B_SERIES_2)

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/* Select register IDs for internal use. */
#define CMU_NOSEL_REG              0
#define CMU_HFCLKSEL_REG           1
#define CMU_LFACLKSEL_REG          2
#define CMU_LFBCLKSEL_REG          3
#define CMU_LFCCLKSEL_REG          4
#define CMU_LFECLKSEL_REG          5
#define CMU_DBGCLKSEL_REG          6
#define CMU_USBCCLKSEL_REG         7
#define CMU_ADC0ASYNCSEL_REG       8
#define CMU_ADC1ASYNCSEL_REG       9
#define CMU_SDIOREFSEL_REG        10
#define CMU_QSPI0REFSEL_REG       11
#define CMU_USBRCLKSEL_REG        12

#define CMU_SEL_REG_POS            0U
#define CMU_SEL_REG_MASK           0xfU

/* Divisor/prescaler register IDs for internal use. */
#define CMU_NODIV_REG              0
#define CMU_NOPRESC_REG            0
#define CMU_HFPRESC_REG            1
#define CMU_HFCLKDIV_REG           1
#define CMU_HFEXPPRESC_REG         2
#define CMU_HFCLKLEPRESC_REG       3
#define CMU_HFPERPRESC_REG         4
#define CMU_HFPERCLKDIV_REG        4
#define CMU_HFPERPRESCB_REG        5
#define CMU_HFPERPRESCC_REG        6
#define CMU_HFCOREPRESC_REG        7
#define CMU_HFCORECLKDIV_REG       7
#define CMU_LFAPRESC0_REG          8
#define CMU_LFBPRESC0_REG          9
#define CMU_LFEPRESC0_REG         10
#define CMU_ADCASYNCDIV_REG       11

#define CMU_PRESC_REG_POS          4U
#define CMU_DIV_REG_POS            CMU_PRESC_REG_POS
#define CMU_PRESC_REG_MASK         0xfU
#define CMU_DIV_REG_MASK           CMU_PRESC_REG_MASK

/* Enable register IDs for internal use. */
#define CMU_NO_EN_REG              0
#define CMU_CTRL_EN_REG            1
#define CMU_HFPERCLKDIV_EN_REG     1
#define CMU_HFPERCLKEN0_EN_REG     2
#define CMU_HFCORECLKEN0_EN_REG    3
#define CMU_HFBUSCLKEN0_EN_REG     5
#define CMU_LFACLKEN0_EN_REG       6
#define CMU_LFBCLKEN0_EN_REG       7
#define CMU_LFCCLKEN0_EN_REG       8
#define CMU_LFECLKEN0_EN_REG       9
#define CMU_PCNT_EN_REG            10
#define CMU_SDIOREF_EN_REG         11
#define CMU_QSPI0REF_EN_REG        12
#define CMU_QSPI1REF_EN_REG        13
#define CMU_HFPERCLKEN1_EN_REG     14
#define CMU_USBRCLK_EN_REG         15

#define CMU_EN_REG_POS             8U
#define CMU_EN_REG_MASK            0xfU

/* Enable register bit positions, for internal use. */
#define CMU_EN_BIT_POS             12U
#define CMU_EN_BIT_MASK            0x1fU

/* Clock branch bitfield positions, for internal use. */
#define CMU_HF_CLK_BRANCH          0
#define CMU_HFCORE_CLK_BRANCH      1
#define CMU_HFPER_CLK_BRANCH       2
#define CMU_HFPERB_CLK_BRANCH      3
#define CMU_HFPERC_CLK_BRANCH      4
#define CMU_HFBUS_CLK_BRANCH       5
#define CMU_HFEXP_CLK_BRANCH       6
#define CMU_DBG_CLK_BRANCH         7
#define CMU_AUX_CLK_BRANCH         8
#define CMU_RTC_CLK_BRANCH         9
#define CMU_RTCC_CLK_BRANCH        10
#define CMU_LETIMER0_CLK_BRANCH    11
#define CMU_LEUART0_CLK_BRANCH     12
#define CMU_LEUART1_CLK_BRANCH     13
#define CMU_LFA_CLK_BRANCH         14
#define CMU_LFB_CLK_BRANCH         15
#define CMU_LFC_CLK_BRANCH         16
#define CMU_LFE_CLK_BRANCH         17
#define CMU_USBC_CLK_BRANCH        18
#define CMU_USBLE_CLK_BRANCH       19
#define CMU_LCDPRE_CLK_BRANCH      20
#define CMU_LCD_CLK_BRANCH         21
#define CMU_LESENSE_CLK_BRANCH     22
#define CMU_CSEN_LF_CLK_BRANCH     23
#define CMU_ADC0ASYNC_CLK_BRANCH   24
#define CMU_ADC1ASYNC_CLK_BRANCH   25
#define CMU_SDIOREF_CLK_BRANCH     26
#define CMU_QSPI0REF_CLK_BRANCH    27
#define CMU_USBR_CLK_BRANCH        28

#define CMU_CLK_BRANCH_POS         17U
#define CMU_CLK_BRANCH_MASK        0x1fU

#if defined(_EMU_CMD_EM01VSCALE0_MASK)
/* Maximum clock frequency for VSCALE voltages. */
#define CMU_VSCALEEM01_LOWPOWER_VOLTAGE_CLOCK_MAX     20000000UL
#endif

#if defined(USB_PRESENT) && defined(_CMU_HFCORECLKEN0_USBC_MASK)
#define USBC_CLOCK_PRESENT
#endif
#if defined(USB_PRESENT) && defined(_CMU_USBCTRL_MASK)
#define USBR_CLOCK_PRESENT
#endif

/** @endcond */

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** Clock divisors. These values are valid for prescalers. */
#define cmuClkDiv_1     1     /**< Divide clock by 1. */
#define cmuClkDiv_2     2     /**< Divide clock by 2. */
#define cmuClkDiv_4     4     /**< Divide clock by 4. */
#define cmuClkDiv_8     8     /**< Divide clock by 8. */
#define cmuClkDiv_16    16    /**< Divide clock by 16. */
#define cmuClkDiv_32    32    /**< Divide clock by 32. */
#define cmuClkDiv_64    64    /**< Divide clock by 64. */
#define cmuClkDiv_128   128   /**< Divide clock by 128. */
#define cmuClkDiv_256   256   /**< Divide clock by 256. */
#define cmuClkDiv_512   512   /**< Divide clock by 512. */
#define cmuClkDiv_1024  1024  /**< Divide clock by 1024. */
#define cmuClkDiv_2048  2048  /**< Divide clock by 2048. */
#define cmuClkDiv_4096  4096  /**< Divide clock by 4096. */
#define cmuClkDiv_8192  8192  /**< Divide clock by 8192. */
#define cmuClkDiv_16384 16384 /**< Divide clock by 16384. */
#define cmuClkDiv_32768 32768 /**< Divide clock by 32768. */

/** Clock divider configuration */
typedef uint32_t CMU_ClkDiv_TypeDef;

#if defined(_SILICON_LABS_32B_SERIES_1)
/** Clockprescaler configuration */
typedef uint32_t CMU_ClkPresc_TypeDef;
#endif

#if defined(_CMU_HFRCOCTRL_BAND_MASK)
/** High-frequency system RCO bands */
typedef enum {
  cmuHFRCOBand_1MHz  = _CMU_HFRCOCTRL_BAND_1MHZ,      /**< 1 MHz HFRCO band  */
  cmuHFRCOBand_7MHz  = _CMU_HFRCOCTRL_BAND_7MHZ,      /**< 7 MHz HFRCO band  */
  cmuHFRCOBand_11MHz = _CMU_HFRCOCTRL_BAND_11MHZ,     /**< 11 MHz HFRCO band */
  cmuHFRCOBand_14MHz = _CMU_HFRCOCTRL_BAND_14MHZ,     /**< 14 MHz HFRCO band */
  cmuHFRCOBand_21MHz = _CMU_HFRCOCTRL_BAND_21MHZ,     /**< 21 MHz HFRCO band */
#if defined(CMU_HFRCOCTRL_BAND_28MHZ)
  cmuHFRCOBand_28MHz = _CMU_HFRCOCTRL_BAND_28MHZ,     /**< 28 MHz HFRCO band */
#endif
} CMU_HFRCOBand_TypeDef;
#endif /* _CMU_HFRCOCTRL_BAND_MASK */

#if defined(_CMU_AUXHFRCOCTRL_BAND_MASK)
/** AUX high-frequency RCO bands */
typedef enum {
  cmuAUXHFRCOBand_1MHz  = _CMU_AUXHFRCOCTRL_BAND_1MHZ,  /**< 1 MHz RC band  */
  cmuAUXHFRCOBand_7MHz  = _CMU_AUXHFRCOCTRL_BAND_7MHZ,  /**< 7 MHz RC band  */
  cmuAUXHFRCOBand_11MHz = _CMU_AUXHFRCOCTRL_BAND_11MHZ, /**< 11 MHz RC band */
  cmuAUXHFRCOBand_14MHz = _CMU_AUXHFRCOCTRL_BAND_14MHZ, /**< 14 MHz RC band */
  cmuAUXHFRCOBand_21MHz = _CMU_AUXHFRCOCTRL_BAND_21MHZ, /**< 21 MHz RC band */
#if defined(CMU_AUXHFRCOCTRL_BAND_28MHZ)
  cmuAUXHFRCOBand_28MHz = _CMU_AUXHFRCOCTRL_BAND_28MHZ, /**< 28 MHz RC band */
#endif
} CMU_AUXHFRCOBand_TypeDef;
#endif

#if defined(_CMU_USHFRCOCONF_BAND_MASK)
/** Universal serial high-frequency RC bands */
typedef enum {
  /** 24 MHz RC band. */
  cmuUSHFRCOBand_24MHz = _CMU_USHFRCOCONF_BAND_24MHZ,
  /** 48 MHz RC band. */
  cmuUSHFRCOBand_48MHz = _CMU_USHFRCOCONF_BAND_48MHZ,
} CMU_USHFRCOBand_TypeDef;
#endif

#if defined(_CMU_USHFRCOCTRL_FREQRANGE_MASK)
/** High-USHFRCO bands */
typedef enum {
  cmuUSHFRCOFreq_16M0Hz           = 16000000U,            /**< 16 MHz RC band  */
  cmuUSHFRCOFreq_32M0Hz           = 32000000U,            /**< 32 MHz RC band  */
  cmuUSHFRCOFreq_48M0Hz           = 48000000U,            /**< 48 MHz RC band  */
  cmuUSHFRCOFreq_50M0Hz           = 50000000U,            /**< 50 MHz RC band  */
  cmuUSHFRCOFreq_UserDefined      = 0,
} CMU_USHFRCOFreq_TypeDef;
#define CMU_USHFRCO_MIN           cmuUSHFRCOFreq_16M0Hz
#define CMU_USHFRCO_MAX           cmuUSHFRCOFreq_50M0Hz
#endif

#if defined(_CMU_HFRCOCTRL_FREQRANGE_MASK)
/** High-frequency system RCO bands */
typedef enum {
  cmuHFRCOFreq_1M0Hz            = 1000000U,             /**< 1 MHz RC band   */
  cmuHFRCOFreq_2M0Hz            = 2000000U,             /**< 2 MHz RC band   */
  cmuHFRCOFreq_4M0Hz            = 4000000U,             /**< 4 MHz RC band   */
  cmuHFRCOFreq_7M0Hz            = 7000000U,             /**< 7 MHz RC band   */
  cmuHFRCOFreq_13M0Hz           = 13000000U,            /**< 13 MHz RC band  */
  cmuHFRCOFreq_16M0Hz           = 16000000U,            /**< 16 MHz RC band  */
  cmuHFRCOFreq_19M0Hz           = 19000000U,            /**< 19 MHz RC band  */
  cmuHFRCOFreq_26M0Hz           = 26000000U,            /**< 26 MHz RC band  */
  cmuHFRCOFreq_32M0Hz           = 32000000U,            /**< 32 MHz RC band  */
  cmuHFRCOFreq_38M0Hz           = 38000000U,            /**< 38 MHz RC band  */
#if defined(_DEVINFO_HFRCOCAL13_MASK)
  cmuHFRCOFreq_48M0Hz           = 48000000U,            /**< 48 MHz RC band  */
#endif
#if defined(_DEVINFO_HFRCOCAL14_MASK)
  cmuHFRCOFreq_56M0Hz           = 56000000U,            /**< 56 MHz RC band  */
#endif
#if defined(_DEVINFO_HFRCOCAL15_MASK)
  cmuHFRCOFreq_64M0Hz           = 64000000U,            /**< 64 MHz RC band  */
#endif
#if defined(_DEVINFO_HFRCOCAL16_MASK)
  cmuHFRCOFreq_72M0Hz           = 72000000U,            /**< 72 MHz RC band  */
#endif
  cmuHFRCOFreq_UserDefined      = 0,
} CMU_HFRCOFreq_TypeDef;
#define CMU_HFRCO_MIN           cmuHFRCOFreq_1M0Hz
#if defined(_DEVINFO_HFRCOCAL16_MASK)
#define CMU_HFRCO_MAX           cmuHFRCOFreq_72M0Hz
#elif defined(_DEVINFO_HFRCOCAL15_MASK)
#define CMU_HFRCO_MAX           cmuHFRCOFreq_64M0Hz
#elif defined(_DEVINFO_HFRCOCAL14_MASK)
#define CMU_HFRCO_MAX           cmuHFRCOFreq_56M0Hz
#elif defined(_DEVINFO_HFRCOCAL13_MASK)
#define CMU_HFRCO_MAX           cmuHFRCOFreq_48M0Hz
#else
#define CMU_HFRCO_MAX           cmuHFRCOFreq_38M0Hz
#endif
#endif

#if defined(_CMU_AUXHFRCOCTRL_FREQRANGE_MASK)
/** AUX high-frequency RCO bands */
typedef enum {
  cmuAUXHFRCOFreq_1M0Hz         = 1000000U,             /**< 1 MHz RC band   */
  cmuAUXHFRCOFreq_2M0Hz         = 2000000U,             /**< 2 MHz RC band   */
  cmuAUXHFRCOFreq_4M0Hz         = 4000000U,             /**< 4 MHz RC band   */
  cmuAUXHFRCOFreq_7M0Hz         = 7000000U,             /**< 7 MHz RC band   */
  cmuAUXHFRCOFreq_13M0Hz        = 13000000U,            /**< 13 MHz RC band  */
  cmuAUXHFRCOFreq_16M0Hz        = 16000000U,            /**< 16 MHz RC band  */
  cmuAUXHFRCOFreq_19M0Hz        = 19000000U,            /**< 19 MHz RC band  */
  cmuAUXHFRCOFreq_26M0Hz        = 26000000U,            /**< 26 MHz RC band  */
  cmuAUXHFRCOFreq_32M0Hz        = 32000000U,            /**< 32 MHz RC band  */
  cmuAUXHFRCOFreq_38M0Hz        = 38000000U,            /**< 38 MHz RC band  */
#if defined(_DEVINFO_AUXHFRCOCAL13_MASK)
  cmuAUXHFRCOFreq_48M0Hz        = 48000000U,            /**< 48 MHz RC band  */
#endif
#if defined(_DEVINFO_AUXHFRCOCAL14_MASK)
  cmuAUXHFRCOFreq_50M0Hz        = 50000000U,            /**< 50 MHz RC band  */
#endif
  cmuAUXHFRCOFreq_UserDefined   = 0,
} CMU_AUXHFRCOFreq_TypeDef;
#define CMU_AUXHFRCO_MIN        cmuAUXHFRCOFreq_1M0Hz
#if defined(_DEVINFO_AUXHFRCOCAL14_MASK)
#define CMU_AUXHFRCO_MAX        cmuAUXHFRCOFreq_50M0Hz
#elif defined(_DEVINFO_AUXHFRCOCAL13_MASK)
#define CMU_AUXHFRCO_MAX        cmuAUXHFRCOFreq_48M0Hz
#else
#define CMU_AUXHFRCO_MAX        cmuAUXHFRCOFreq_38M0Hz
#endif
#endif

/** Clock points in CMU. See CMU overview in the reference manual. */
typedef enum {
  /*******************/
  /* HF clock branch */
  /*******************/

  /** High-frequency clock */
#if defined(_CMU_CTRL_HFCLKDIV_MASK) \
  || defined(_CMU_HFPRESC_MASK)
  cmuClock_HF = (CMU_HFCLKDIV_REG << CMU_DIV_REG_POS)
                | (CMU_HFCLKSEL_REG << CMU_SEL_REG_POS)
                | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                | (0 << CMU_EN_BIT_POS)
                | (CMU_HF_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#else
  cmuClock_HF = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                | (CMU_HFCLKSEL_REG << CMU_SEL_REG_POS)
                | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                | (0 << CMU_EN_BIT_POS)
                | (CMU_HF_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

  /** Debug clock */
  cmuClock_DBG = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_DBGCLKSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                 | (0 << CMU_EN_BIT_POS)
                 | (CMU_DBG_CLK_BRANCH << CMU_CLK_BRANCH_POS),

  /** AUX clock */
  cmuClock_AUX = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                 | (0 << CMU_EN_BIT_POS)
                 | (CMU_AUX_CLK_BRANCH << CMU_CLK_BRANCH_POS),

#if defined(_CMU_HFEXPPRESC_MASK)
  /**********************/
  /* HF export sub-branch */
  /**********************/

  /** Export clock */
  cmuClock_EXPORT = (CMU_HFEXPPRESC_REG << CMU_PRESC_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                    | (0 << CMU_EN_BIT_POS)
                    | (CMU_HFEXP_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(_CMU_HFBUSCLKEN0_MASK)
/**********************************/
/* HF bus clock sub-branch */
/**********************************/

  /** High-frequency bus clock */
  cmuClock_BUS = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                 | (0 << CMU_EN_BIT_POS)
                 | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),

#if defined(CMU_HFBUSCLKEN0_CRYPTO)
  /** Cryptography accelerator clock */
  cmuClock_CRYPTO = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFBUSCLKEN0_CRYPTO_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFBUSCLKEN0_CRYPTO0)
  /** Cryptography accelerator 0 clock */
  cmuClock_CRYPTO0 = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_HFBUSCLKEN0_CRYPTO0_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFBUSCLKEN0_CRYPTO1)
  /** Cryptography accelerator 1 clock */
  cmuClock_CRYPTO1 = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_HFBUSCLKEN0_CRYPTO1_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFBUSCLKEN0_LDMA)
  /** Direct-memory access controller clock */
  cmuClock_LDMA = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFBUSCLKEN0_LDMA_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFBUSCLKEN0_QSPI0)
  /** Quad SPI clock */
  cmuClock_QSPI0 = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFBUSCLKEN0_QSPI0_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFBUSCLKEN0_GPCRC)
  /** General-purpose cyclic redundancy checksum clock */
  cmuClock_GPCRC = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFBUSCLKEN0_GPCRC_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFBUSCLKEN0_GPIO)
  /** General-purpose input/output clock */
  cmuClock_GPIO = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFBUSCLKEN0_GPIO_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

  /** Low-energy clock divided down from HFBUSCLK */
  cmuClock_HFLE = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFBUSCLKEN0_LE_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),

#if defined(CMU_HFBUSCLKEN0_PRS)
  /** Peripheral reflex system clock */
  cmuClock_PRS = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_HFBUSCLKEN0_PRS_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_HFBUS_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif
#endif

  /**********************************/
  /* HF peripheral clock sub-branch */
  /**********************************/

  /** High-frequency peripheral clock */
#if defined(_CMU_HFPRESC_MASK)
  cmuClock_HFPER = (CMU_HFPERPRESC_REG << CMU_PRESC_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_CTRL_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_CTRL_HFPERCLKEN_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#else
  cmuClock_HFPER = (CMU_HFPERCLKDIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKDIV_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKDIV_HFPERCLKEN_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(_CMU_HFPERPRESCB_MASK)
  /** Branch B figh-frequency peripheral clock */
  cmuClock_HFPERB = (CMU_HFPERPRESCB_REG << CMU_PRESC_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_CTRL_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_CTRL_HFPERCLKEN_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPERB_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(_CMU_HFPERPRESCC_MASK)
  /** Branch C figh-frequency peripheral clock */
  cmuClock_HFPERC = (CMU_HFPERPRESCC_REG << CMU_PRESC_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_CTRL_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_CTRL_HFPERCLKEN_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPERC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_USART0)
  /** Universal sync/async receiver/transmitter 0 clock */
  cmuClock_USART0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_USART0_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_USARTRF0)
  /** Universal sync/async receiver/transmitter 0 clock */
  cmuClock_USARTRF0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                      | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                      | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                      | (_CMU_HFPERCLKEN0_USARTRF0_SHIFT << CMU_EN_BIT_POS)
                      | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_USARTRF1)
  /** Universal sync/async receiver/transmitter 0 clock */
  cmuClock_USARTRF1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                      | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                      | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                      | (_CMU_HFPERCLKEN0_USARTRF1_SHIFT << CMU_EN_BIT_POS)
                      | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_USART1)
  /** Universal sync/async receiver/transmitter 1 clock */
  cmuClock_USART1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_USART1_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_USART2)
  /** Universal sync/async receiver/transmitter 2 clock */
  cmuClock_USART2 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_USART2_SHIFT << CMU_EN_BIT_POS)
  #if defined(_CMU_HFPERPRESCB_MASK)
                    | (CMU_HFPERB_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #else
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #endif
#endif

#if defined(CMU_HFPERCLKEN0_USART3)
  /** Universal sync/async receiver/transmitter 3 clock */
  cmuClock_USART3 = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_USART3_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_USART4)
  /** Universal sync/async receiver/transmitter 4 clock */
  cmuClock_USART4 = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_USART4_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_USART5)
  /** Universal sync/async receiver/transmitter 5 clock */
  cmuClock_USART5 = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_USART5_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_UART0)
  /** Universal async receiver/transmitter 0 clock */
  cmuClock_UART0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN0_UART0_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#elif defined(_CMU_HFPERCLKEN1_UART0_MASK)
  /** Universal async receiver/transmitter 0 clock */
  cmuClock_UART0 = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN1_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN1_UART0_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_UART1)
  /** Universal async receiver/transmitter 1 clock */
  cmuClock_UART1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN0_UART1_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#elif defined(_CMU_HFPERCLKEN1_UART1_MASK)
  /** Universal async receiver/transmitter 1 clock */
  cmuClock_UART1 = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN1_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN1_UART1_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_TIMER0)
  /** Timer 0 clock */
  cmuClock_TIMER0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_TIMER0_SHIFT << CMU_EN_BIT_POS)
  #if defined(_CMU_HFPERPRESCB_MASK)
                    | (CMU_HFPERB_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #else
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #endif
#endif

#if defined(CMU_HFPERCLKEN0_TIMER1)
  /** Timer 1 clock */
  cmuClock_TIMER1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_TIMER1_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_TIMER2)
  /** Timer 2 clock */
  cmuClock_TIMER2 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_TIMER2_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_TIMER3)
  /** Timer 3 clock */
  cmuClock_TIMER3 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_TIMER3_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_TIMER4)
  /** Timer 4 clock */
  cmuClock_TIMER4 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_TIMER4_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_TIMER5)
  /** Timer 5 clock */
  cmuClock_TIMER5 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_TIMER5_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_TIMER6)
  /** Timer 6 clock */
  cmuClock_TIMER6 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                    | (_CMU_HFPERCLKEN0_TIMER6_SHIFT << CMU_EN_BIT_POS)
                    | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_WTIMER0)
  /** Wide-timer 0 clock */
  cmuClock_WTIMER0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_HFPERCLKEN0_WTIMER0_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#elif defined(CMU_HFPERCLKEN1_WTIMER0)
  /** Wide-timer 0 clock */
  cmuClock_WTIMER0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_HFPERCLKEN1_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_HFPERCLKEN1_WTIMER0_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_WTIMER1)
  /** Wide-timer 1 clock */
  cmuClock_WTIMER1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_HFPERCLKEN0_WTIMER1_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#elif defined(CMU_HFPERCLKEN1_WTIMER1)
  /** Wide-timer 1 clock */
  cmuClock_WTIMER1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_HFPERCLKEN1_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_HFPERCLKEN1_WTIMER1_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN1_WTIMER2)
  /** Wide-timer 2 clock */
  cmuClock_WTIMER2 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_HFPERCLKEN1_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_HFPERCLKEN1_WTIMER2_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN1_WTIMER3)
  /** Wide-timer 3 clock */
  cmuClock_WTIMER3 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_HFPERCLKEN1_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_HFPERCLKEN1_WTIMER3_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_CRYOTIMER)
  /** CRYOtimer clock */
  cmuClock_CRYOTIMER = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                       | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                       | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                       | (_CMU_HFPERCLKEN0_CRYOTIMER_SHIFT << CMU_EN_BIT_POS)
  #if defined(_CMU_HFPERPRESCC_MASK)
                       | (CMU_HFPERC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #else
                       | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #endif
#endif

#if defined(CMU_HFPERCLKEN0_ACMP0)
  /** Analog comparator 0 clock */
  cmuClock_ACMP0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN0_ACMP0_SHIFT << CMU_EN_BIT_POS)
  #if defined(_CMU_HFPERPRESCC_MASK)
                   | (CMU_HFPERC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #else
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #endif
#endif

#if defined(CMU_HFPERCLKEN0_ACMP1)
  /** Analog comparator 1 clock */
  cmuClock_ACMP1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN0_ACMP1_SHIFT << CMU_EN_BIT_POS)
  #if defined(_CMU_HFPERPRESCC_MASK)
                   | (CMU_HFPERC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #else
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #endif
#endif

#if defined(CMU_HFPERCLKEN0_ACMP2)
  /** Analog comparator 2 clock */
  cmuClock_ACMP2 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN0_ACMP2_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPERC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_ACMP3)
  /** Analog comparator 3 clock */
  cmuClock_ACMP3 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN0_ACMP3_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPERC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_PRS)
  /** Peripheral-reflex system clock */
  cmuClock_PRS = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_HFPERCLKEN0_PRS_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_DAC0)
  /** Digital-to-analog converter 0 clock */
  cmuClock_DAC0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_DAC0_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_VDAC0)
  /** Voltage digital-to-analog converter 0 clock */
  cmuClock_VDAC0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN0_VDAC0_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#elif defined(CMU_HFPERCLKEN1_VDAC0)
  /** Voltage digital-to-analog converter 0 clock */
  cmuClock_VDAC0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN1_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN1_VDAC0_SHIFT << CMU_EN_BIT_POS)
  #if defined(_CMU_HFPERPRESCC_MASK)
                   | (CMU_HFPERC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #else
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #endif
#endif

#if defined(CMU_HFPERCLKEN0_IDAC0)
  /** Current digital-to-analog converter 0 clock */
  cmuClock_IDAC0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN0_IDAC0_SHIFT << CMU_EN_BIT_POS)
  #if defined(_CMU_HFPERPRESCC_MASK)
                   | (CMU_HFPERC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #else
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #endif
#endif

#if defined(CMU_HFPERCLKEN0_GPIO)
  /** General-purpose input/output clock */
  cmuClock_GPIO = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_GPIO_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_VCMP)
  /** Voltage comparator clock */
  cmuClock_VCMP = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_VCMP_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_ADC0)
  /** Analog-to-digital converter 0 clock */
  cmuClock_ADC0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_ADC0_SHIFT << CMU_EN_BIT_POS)
  #if defined(_CMU_HFPERPRESCC_MASK)
                  | (CMU_HFPERC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #else
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #endif
#endif

#if defined(CMU_HFPERCLKEN0_ADC1)
  /** Analog-to-digital converter 1 clock */
  cmuClock_ADC1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_ADC1_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFPERC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_I2C0)
  /** I2C 0 clock */
  cmuClock_I2C0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_I2C0_SHIFT << CMU_EN_BIT_POS)
  #if defined(_CMU_HFPERPRESCC_MASK)
                  | (CMU_HFPERC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #else
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #endif
#endif

#if defined(CMU_HFPERCLKEN0_I2C1)
  /** I2C 1 clock */
  cmuClock_I2C1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_I2C1_SHIFT << CMU_EN_BIT_POS)
  #if defined(_CMU_HFPERPRESCC_MASK)
                  | (CMU_HFPERC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #else
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
  #endif
#endif

#if defined(CMU_HFPERCLKEN0_I2C2)
  /** I2C 2 clock */
  cmuClock_I2C2 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN0_I2C2_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFPERC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_CSEN)
  /** Capacitive Sense HF clock */
  cmuClock_CSEN_HF = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_HFPERCLKEN0_CSEN_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#elif defined(CMU_HFPERCLKEN1_CSEN)
  /** Capacitive Sense HF clock */
  cmuClock_CSEN_HF = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_HFPERCLKEN1_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_HFPERCLKEN1_CSEN_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_HFPERC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFPERCLKEN0_TRNG0)
  /** True random number generator clock */
  cmuClock_TRNG0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_HFPERCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_HFPERCLKEN0_TRNG0_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(_CMU_HFPERCLKEN1_CAN0_MASK)
  /** Controller Area Network 0 clock */
  cmuClock_CAN0 = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN1_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN1_CAN0_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(_CMU_HFPERCLKEN1_CAN1_MASK)
  /** Controller Area Network 1 clock. */
  cmuClock_CAN1 = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFPERCLKEN1_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFPERCLKEN1_CAN1_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFPER_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

  /**********************/
  /* HF core sub-branch */
  /**********************/

  /** Core clock */
  cmuClock_CORE = (CMU_HFCORECLKDIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                  | (0 << CMU_EN_BIT_POS)
                  | (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS),

#if defined(CMU_HFCORECLKEN0_AES)
  /** Advanced encryption standard accelerator clock */
  cmuClock_AES = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_HFCORECLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_HFCORECLKEN0_AES_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFCORECLKEN0_DMA)
  /** Direct memory access controller clock */
  cmuClock_DMA = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_HFCORECLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_HFCORECLKEN0_DMA_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFCORECLKEN0_LE)
  /** Low-energy clock divided down from HFCORECLK */
  cmuClock_HFLE = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFCORECLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFCORECLKEN0_LE_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFCORECLKEN0_EBI)
  /** External bus interface clock */
  cmuClock_EBI = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_HFCORECLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_HFCORECLKEN0_EBI_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#elif defined(_CMU_HFBUSCLKEN0_EBI_MASK)
  /** External bus interface clock */
  cmuClock_EBI = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_HFBUSCLKEN0_EBI_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(_CMU_HFBUSCLKEN0_ETH_MASK)
  /** Ethernet clock */
  cmuClock_ETH = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_HFBUSCLKEN0_ETH_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(_CMU_HFBUSCLKEN0_SDIO_MASK)
  /** SDIO clock */
  cmuClock_SDIO = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFBUSCLKEN0_SDIO_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(USBC_CLOCK_PRESENT)
  /** USB Core clock */
  cmuClock_USBC = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_USBCCLKSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_HFCORECLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_HFCORECLKEN0_USBC_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_USBC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif
#if defined (USBR_CLOCK_PRESENT)
  /** USB Rate clock */
  cmuClock_USBR = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                  | (CMU_USBRCLKSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_USBRCLK_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_USBCTRL_USBCLKEN_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_USBR_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_HFCORECLKEN0_USB)
  /** USB clock */
  cmuClock_USB = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_HFCORECLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_HFCORECLKEN0_USB_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#elif defined(CMU_HFBUSCLKEN0_USB)
  /** USB clock */
  cmuClock_USB = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_HFBUSCLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_HFBUSCLKEN0_USB_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_HFCORE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

  /***************/
  /* LF A branch */
  /***************/

  /** Low-frequency A clock */
  cmuClock_LFA = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_LFACLKSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                 | (0 << CMU_EN_BIT_POS)
                 | (CMU_LFA_CLK_BRANCH << CMU_CLK_BRANCH_POS),

#if defined(CMU_LFACLKEN0_RTC)
  /** Real time counter clock */
  cmuClock_RTC = (CMU_LFAPRESC0_REG << CMU_DIV_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_LFACLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_LFACLKEN0_RTC_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_RTC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_LFACLKEN0_LETIMER0)
  /** Low-energy timer 0 clock */
  cmuClock_LETIMER0 = (CMU_LFAPRESC0_REG << CMU_DIV_REG_POS)
                      | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                      | (CMU_LFACLKEN0_EN_REG << CMU_EN_REG_POS)
                      | (_CMU_LFACLKEN0_LETIMER0_SHIFT << CMU_EN_BIT_POS)
                      | (CMU_LETIMER0_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_LFACLKEN0_LETIMER1)
  /** Low-energy timer 1 clock */
  cmuClock_LETIMER1 = (CMU_LFAPRESC0_REG << CMU_DIV_REG_POS)
                      | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                      | (CMU_LFACLKEN0_EN_REG << CMU_EN_REG_POS)
                      | (_CMU_LFACLKEN0_LETIMER1_SHIFT << CMU_EN_BIT_POS)
                      | (CMU_LETIMER0_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_LFACLKEN0_LCD)
  /** Liquid crystal display, pre FDIV clock */
  cmuClock_LCDpre = (CMU_LFAPRESC0_REG << CMU_DIV_REG_POS)
                    | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                    | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                    | (0 << CMU_EN_BIT_POS)
                    | (CMU_LCDPRE_CLK_BRANCH << CMU_CLK_BRANCH_POS),

  /** Liquid crystal display clock. Note that FDIV prescaler
   * must be set by special API. */
  cmuClock_LCD = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_LFACLKEN0_EN_REG << CMU_EN_REG_POS)
                 | (_CMU_LFACLKEN0_LCD_SHIFT << CMU_EN_BIT_POS)
                 | (CMU_LCD_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_PCNTCTRL_PCNT0CLKEN)
  /** Pulse counter 0 clock */
  cmuClock_PCNT0 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_PCNT_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_PCNTCTRL_PCNT0CLKEN_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_LFA_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_PCNTCTRL_PCNT1CLKEN)
  /** Pulse counter 1 clock */
  cmuClock_PCNT1 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_PCNT_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_PCNTCTRL_PCNT1CLKEN_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_LFA_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_PCNTCTRL_PCNT2CLKEN)
  /** Pulse counter 2 clock */
  cmuClock_PCNT2 = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_PCNT_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_PCNTCTRL_PCNT2CLKEN_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_LFA_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif
#if defined(CMU_LFACLKEN0_LESENSE)
  /** LESENSE clock */
  cmuClock_LESENSE = (CMU_LFAPRESC0_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_LFACLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_LFACLKEN0_LESENSE_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_LESENSE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

  /***************/
  /* LF B branch */
  /***************/

  /** Low-frequency B clock */
  cmuClock_LFB = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_LFBCLKSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                 | (0 << CMU_EN_BIT_POS)
                 | (CMU_LFB_CLK_BRANCH << CMU_CLK_BRANCH_POS),

#if defined(CMU_LFBCLKEN0_LEUART0)
  /** Low-energy universal asynchronous receiver/transmitter 0 clock */
  cmuClock_LEUART0 = (CMU_LFBPRESC0_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_LFBCLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_LFBCLKEN0_LEUART0_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_LEUART0_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_LFBCLKEN0_CSEN)
  /** Capacitive Sense LF clock */
  cmuClock_CSEN_LF = (CMU_LFBPRESC0_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_LFBCLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_LFBCLKEN0_CSEN_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_CSEN_LF_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_LFBCLKEN0_LEUART1)
  /** Low-energy universal asynchronous receiver/transmitter 1 clock */
  cmuClock_LEUART1 = (CMU_LFBPRESC0_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_LFBCLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_LFBCLKEN0_LEUART1_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_LEUART1_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(CMU_LFBCLKEN0_SYSTICK)
  /** Cortex SYSTICK LF clock */
  cmuClock_SYSTICK = (CMU_LFBPRESC0_REG << CMU_DIV_REG_POS)
                     | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_LFBCLKEN0_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_LFBCLKEN0_SYSTICK_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_LEUART0_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(_CMU_LFCCLKEN0_MASK)
  /***************/
  /* LF C branch */
  /***************/

  /** Low-frequency C clock */
  cmuClock_LFC = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                 | (CMU_LFCCLKSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                 | (0 << CMU_EN_BIT_POS)
                 | (CMU_LFC_CLK_BRANCH << CMU_CLK_BRANCH_POS),

#if defined(CMU_LFCCLKEN0_USBLE)
  /** USB LE clock */
  cmuClock_USBLE = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_LFCCLKSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_LFCCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_LFCCLKEN0_USBLE_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_USBLE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#elif defined(CMU_LFCCLKEN0_USB)
  /** USB LE clock */
  cmuClock_USBLE = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                   | (CMU_LFCCLKSEL_REG << CMU_SEL_REG_POS)
                   | (CMU_LFCCLKEN0_EN_REG << CMU_EN_REG_POS)
                   | (_CMU_LFCCLKEN0_USB_SHIFT << CMU_EN_BIT_POS)
                   | (CMU_USBLE_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif
#endif

#if defined(_CMU_LFECLKEN0_MASK)
  /***************/
  /* LF E branch */
  /***************/

  /** Low-frequency E clock */
  cmuClock_LFE = (CMU_NOPRESC_REG << CMU_PRESC_REG_POS)
                 | (CMU_LFECLKSEL_REG << CMU_SEL_REG_POS)
                 | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                 | (0 << CMU_EN_BIT_POS)
                 | (CMU_LFE_CLK_BRANCH << CMU_CLK_BRANCH_POS),

  /** Real-time counter and calendar clock */
#if defined (CMU_LFECLKEN0_RTCC)
  cmuClock_RTCC = (CMU_LFEPRESC0_REG << CMU_PRESC_REG_POS)
                  | (CMU_NOSEL_REG << CMU_SEL_REG_POS)
                  | (CMU_LFECLKEN0_EN_REG << CMU_EN_REG_POS)
                  | (_CMU_LFECLKEN0_RTCC_SHIFT << CMU_EN_BIT_POS)
                  | (CMU_RTCC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif
#endif

  /**********************************/
  /* Asynchronous peripheral clocks */
  /**********************************/

#if defined(_CMU_ADCCTRL_ADC0CLKSEL_MASK)
  /** ADC0 asynchronous clock */
  cmuClock_ADC0ASYNC = (CMU_ADCASYNCDIV_REG << CMU_DIV_REG_POS)
                       | (CMU_ADC0ASYNCSEL_REG << CMU_SEL_REG_POS)
                       | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                       | (0 << CMU_EN_BIT_POS)
                       | (CMU_ADC0ASYNC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(_CMU_ADCCTRL_ADC1CLKSEL_MASK)
  /** ADC1 asynchronous clock */
  cmuClock_ADC1ASYNC = (CMU_ADCASYNCDIV_REG << CMU_DIV_REG_POS)
                       | (CMU_ADC1ASYNCSEL_REG << CMU_SEL_REG_POS)
                       | (CMU_NO_EN_REG << CMU_EN_REG_POS)
                       | (0 << CMU_EN_BIT_POS)
                       | (CMU_ADC1ASYNC_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(_CMU_SDIOCTRL_SDIOCLKDIS_MASK)
  /** SDIO reference clock */
  cmuClock_SDIOREF = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                     | (CMU_SDIOREFSEL_REG << CMU_SEL_REG_POS)
                     | (CMU_SDIOREF_EN_REG << CMU_EN_REG_POS)
                     | (_CMU_SDIOCTRL_SDIOCLKDIS_SHIFT << CMU_EN_BIT_POS)
                     | (CMU_SDIOREF_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif

#if defined(_CMU_QSPICTRL_QSPI0CLKDIS_MASK)
  /** QSPI0 reference clock */
  cmuClock_QSPI0REF = (CMU_NODIV_REG << CMU_DIV_REG_POS)
                      | (CMU_QSPI0REFSEL_REG << CMU_SEL_REG_POS)
                      | (CMU_QSPI0REF_EN_REG << CMU_EN_REG_POS)
                      | (_CMU_QSPICTRL_QSPI0CLKDIS_SHIFT << CMU_EN_BIT_POS)
                      | (CMU_QSPI0REF_CLK_BRANCH << CMU_CLK_BRANCH_POS),
#endif
} CMU_Clock_TypeDef;

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
/* Deprecated CMU_Clock_TypeDef member */
#define cmuClock_CORELE cmuClock_HFLE
/** @endcond */

/** Oscillator types. */
typedef enum {
  cmuOsc_LFXO,     /**< Low-frequency crystal oscillator. */
  cmuOsc_LFRCO,    /**< Low-frequency RC oscillator. */
  cmuOsc_HFXO,     /**< High-frequency crystal oscillator. */
  cmuOsc_HFRCO,    /**< High-frequency RC oscillator. */
  cmuOsc_AUXHFRCO, /**< Auxiliary high-frequency RC oscillator. */
#if defined(_CMU_STATUS_USHFRCOENS_MASK)
  cmuOsc_USHFRCO,  /**< Universal serial high-frequency RC oscillator */
#endif
#if defined(CMU_LFCLKSEL_LFAE_ULFRCO) || defined(CMU_LFACLKSEL_LFA_ULFRCO)
  cmuOsc_ULFRCO,   /**< Ultra low-frequency RC oscillator. */
#endif
#if defined(CMU_HFCLKSTATUS_SELECTED_CLKIN0)
  cmuOsc_CLKIN0,   /**< External oscillator. */
#endif
} CMU_Osc_TypeDef;

/** Oscillator modes. */
typedef enum {
  cmuOscMode_Crystal,   /**< Crystal oscillator. */
  cmuOscMode_AcCoupled, /**< AC-coupled buffer. */
  cmuOscMode_External,  /**< External digital clock. */
} CMU_OscMode_TypeDef;

/** Selectable clock sources. */
typedef enum {
  cmuSelect_Error,                      /**< Usage error. */
  cmuSelect_Disabled,                   /**< Clock selector disabled. */
  cmuSelect_LFXO,                       /**< Low-frequency crystal oscillator. */
  cmuSelect_LFRCO,                      /**< Low-frequency RC oscillator. */
  cmuSelect_HFXO,                       /**< High-frequency crystal oscillator. */
  cmuSelect_HFRCO,                      /**< High-frequency RC oscillator. */
  cmuSelect_HFCLKLE,                    /**< High-frequency LE clock divided by 2 or 4. */
  cmuSelect_AUXHFRCO,                   /**< Auxilliary clock source can be used for debug clock. */
  cmuSelect_HFSRCCLK,                   /**< High-frequency source clock. */
  cmuSelect_HFCLK,                      /**< Divided HFCLK on Giant for debug clock, undivided on
                                             Tiny Gecko and for USBC (not used on Gecko). */
#if defined(CMU_STATUS_USHFRCOENS)
  cmuSelect_USHFRCO,                    /**< Universal serial high-frequency RC oscillator. */
#endif
#if defined(CMU_CMD_HFCLKSEL_USHFRCODIV2)
  cmuSelect_USHFRCODIV2,                /**< Universal serial high-frequency RC oscillator / 2. */
#endif
#if defined(CMU_HFXOCTRL_HFXOX2EN)
  cmuSelect_HFXOX2,                     /**< High-frequency crystal oscillator x 2. */
#endif
#if defined(CMU_LFCLKSEL_LFAE_ULFRCO) || defined(CMU_LFACLKSEL_LFA_ULFRCO)
  cmuSelect_ULFRCO,                     /**< Ultra low-frequency RC oscillator. */
#endif
#if defined(CMU_HFCLKSTATUS_SELECTED_HFRCODIV2)
  cmuSelect_HFRCODIV2,                 /**< High-frequency RC oscillator divided by 2. */
#endif
#if defined(CMU_HFCLKSTATUS_SELECTED_CLKIN0)
  cmuSelect_CLKIN0,                    /**< External clock input. */
#endif
} CMU_Select_TypeDef;

#if defined(CMU_HFCORECLKEN0_LE)
/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
/* Deprecated CMU_Select_TypeDef member */
#define cmuSelect_CORELEDIV2    cmuSelect_HFCLKLE
/** @endcond */
#endif

#if defined(_CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_MASK) || defined(_CMU_HFXOCTRL_PEAKDETMODE_MASK)
/** HFXO tuning modes */
typedef enum {
  cmuHFXOTuningMode_Auto               = 0,
  cmuHFXOTuningMode_PeakDetectCommand  = CMU_CMD_HFXOPEAKDETSTART,   /**< Run peak detect optimization only. */
#if defined(CMU_CMD_HFXOSHUNTOPTSTART)
  cmuHFXOTuningMode_ShuntCommand       = CMU_CMD_HFXOSHUNTOPTSTART,  /**< Run shunt current optimization only. */
  cmuHFXOTuningMode_PeakShuntCommand   = CMU_CMD_HFXOPEAKDETSTART    /**< Run peak and shunt current optimization. */
                                         | CMU_CMD_HFXOSHUNTOPTSTART,
#endif
} CMU_HFXOTuningMode_TypeDef;
#endif

#if defined(_CMU_CTRL_LFXOBOOST_MASK)
/** LFXO Boost values. */
typedef enum {
  cmuLfxoBoost70         = 0x0,
  cmuLfxoBoost100        = 0x2,
#if defined(_EMU_AUXCTRL_REDLFXOBOOST_MASK)
  cmuLfxoBoost70Reduced  = 0x1,
  cmuLfxoBoost100Reduced = 0x3,
#endif
} CMU_LFXOBoost_TypeDef;
#endif

#if defined(CMU_OSCENCMD_DPLLEN)
/** DPLL reference clock selector. */
typedef enum {
  cmuDPLLClkSel_Hfxo   = _CMU_DPLLCTRL_REFSEL_HFXO,   /**< HFXO is DPLL reference clock. */
  cmuDPLLClkSel_Lfxo   = _CMU_DPLLCTRL_REFSEL_LFXO,   /**< LFXO is DPLL reference clock. */
  cmuDPLLClkSel_Clkin0 = _CMU_DPLLCTRL_REFSEL_CLKIN0  /**< CLKIN0 is DPLL reference clock. */
} CMU_DPLLClkSel_TypeDef;

/** DPLL reference clock edge detect selector. */
typedef enum {
  cmuDPLLEdgeSel_Fall = _CMU_DPLLCTRL_EDGESEL_FALL,   /**< Detect falling edge of reference clock. */
  cmuDPLLEdgeSel_Rise = _CMU_DPLLCTRL_EDGESEL_RISE    /**< Detect rising edge of reference clock. */
} CMU_DPLLEdgeSel_TypeDef;

/** DPLL lock mode selector. */
typedef enum {
  cmuDPLLLockMode_Freq  = _CMU_DPLLCTRL_MODE_FREQLL,  /**< Frequency lock mode. */
  cmuDPLLLockMode_Phase = _CMU_DPLLCTRL_MODE_PHASELL  /**< Phase lock mode. */
} CMU_DPLLLockMode_TypeDef;
#endif // CMU_OSCENCMD_DPLLEN

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** LFXO initialization structure. Initialization values should be obtained from a configuration tool,
    app note, or xtal data sheet.  */
typedef struct {
#if defined(_CMU_LFXOCTRL_MASK)
  uint8_t ctune;                        /**< CTUNE (load capacitance) value */
  uint8_t gain;                         /**< Gain/max startup margin */
#else
  CMU_LFXOBoost_TypeDef boost;          /**< LFXO boost */
#endif
  uint8_t timeout;                      /**< Startup delay */
  CMU_OscMode_TypeDef mode;             /**< Oscillator mode */
} CMU_LFXOInit_TypeDef;

#if defined(_CMU_LFXOCTRL_MASK)
/** Default LFXO initialization values for platform 2 devices, which contain a
 *  separate LFXOCTRL register. */
#define CMU_LFXOINIT_DEFAULT                                                  \
  {                                                                           \
    _CMU_LFXOCTRL_TUNING_DEFAULT,   /* Default CTUNE value, 0 */              \
    _CMU_LFXOCTRL_GAIN_DEFAULT,     /* Default gain, 2 */                     \
    _CMU_LFXOCTRL_TIMEOUT_DEFAULT,  /* Default start-up delay, 32 K cycles */ \
    cmuOscMode_Crystal,             /* Crystal oscillator */                  \
  }
#define CMU_LFXOINIT_EXTERNAL_CLOCK                                             \
  {                                                                             \
    0,                              /* No CTUNE value needed */                 \
    0,                              /* No LFXO startup gain */                  \
    _CMU_LFXOCTRL_TIMEOUT_2CYCLES,  /* Minimal lfxo start-up delay, 2 cycles */ \
    cmuOscMode_External,            /* External digital clock */                \
  }
#else
/** Default LFXO initialization values for platform 1 devices. */
#define CMU_LFXOINIT_DEFAULT       \
  {                                \
    cmuLfxoBoost70,                \
    _CMU_CTRL_LFXOTIMEOUT_DEFAULT, \
    cmuOscMode_Crystal,            \
  }
#define CMU_LFXOINIT_EXTERNAL_CLOCK \
  {                                 \
    cmuLfxoBoost70,                 \
    _CMU_CTRL_LFXOTIMEOUT_8CYCLES,  \
    cmuOscMode_External,            \
  }
#endif

/** HFXO initialization structure. Initialization values should be obtained from a configuration tool,
    app note, or xtal data sheet.  */
typedef struct {
#if defined(_SILICON_LABS_32B_SERIES_1) && (_SILICON_LABS_GECKO_INTERNAL_SDID >= 100)
  uint16_t ctuneStartup;                /**< Startup phase CTUNE (load capacitance) value */
  uint16_t ctuneSteadyState;            /**< Steady-state phase CTUNE (load capacitance) value */
  uint16_t xoCoreBiasTrimStartup;       /**< Startup XO core bias current trim */
  uint16_t xoCoreBiasTrimSteadyState;   /**< Steady-state XO core bias current trim */
  uint8_t timeoutPeakDetect;            /**< Timeout - peak detection */
  uint8_t timeoutSteady;                /**< Timeout - steady-state */
  uint8_t timeoutStartup;               /**< Timeout - startup */
#elif defined(_CMU_HFXOCTRL_MASK)
  bool lowPowerMode;                    /**< Enable low-power mode */
  bool autoStartEm01;                   /**< @deprecated Use @ref CMU_HFXOAutostartEnable instead. */
  bool autoSelEm01;                     /**< @deprecated Use @ref CMU_HFXOAutostartEnable instead. */
  bool autoStartSelOnRacWakeup;         /**< @deprecated Use @ref CMU_HFXOAutostartEnable instead. */
  uint16_t ctuneStartup;                /**< Startup phase CTUNE (load capacitance) value */
  uint16_t ctuneSteadyState;            /**< Steady-state phase CTUNE (load capacitance) value */
  uint8_t regIshSteadyState;            /**< Shunt steady-state current */
  uint8_t xoCoreBiasTrimStartup;        /**< Startup XO core bias current trim */
  uint8_t xoCoreBiasTrimSteadyState;    /**< Steady-state XO core bias current trim */
  uint8_t thresholdPeakDetect;          /**< Peak detection threshold */
  uint8_t timeoutShuntOptimization;     /**< Timeout - shunt optimization */
  uint8_t timeoutPeakDetect;            /**< Timeout - peak detection */
  uint8_t timeoutSteady;                /**< Timeout - steady-state */
  uint8_t timeoutStartup;               /**< Timeout - startup */
#else
  uint8_t boost;                        /**< HFXO Boost, 0=50% 1=70%, 2=80%, 3=100% */
  uint8_t timeout;                      /**< Startup delay */
  bool glitchDetector;                  /**< Enable/disable glitch detector */
#endif
  CMU_OscMode_TypeDef mode;             /**< Oscillator mode */
} CMU_HFXOInit_TypeDef;

#if defined(_SILICON_LABS_32B_SERIES_1) && (_SILICON_LABS_GECKO_INTERNAL_SDID >= 100)
#define CMU_HFXOINIT_DEFAULT                       \
  {                                                \
    _CMU_HFXOSTARTUPCTRL_CTUNE_DEFAULT,            \
    _CMU_HFXOSTEADYSTATECTRL_CTUNE_DEFAULT,        \
    _CMU_HFXOSTARTUPCTRL_IBTRIMXOCORE_DEFAULT,     \
    _CMU_HFXOSTEADYSTATECTRL_IBTRIMXOCORE_DEFAULT, \
    _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_DEFAULT,   \
    _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_DEFAULT,    \
    _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_DEFAULT,   \
    cmuOscMode_Crystal,                            \
  }
#define CMU_HFXOINIT_EXTERNAL_CLOCK                \
  {                                                \
    _CMU_HFXOSTARTUPCTRL_CTUNE_DEFAULT,            \
    _CMU_HFXOSTEADYSTATECTRL_CTUNE_DEFAULT,        \
    _CMU_HFXOSTARTUPCTRL_IBTRIMXOCORE_DEFAULT,     \
    _CMU_HFXOSTEADYSTATECTRL_IBTRIMXOCORE_DEFAULT, \
    _CMU_HFXOTIMEOUTCTRL_PEAKDETTIMEOUT_DEFAULT,   \
    _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_DEFAULT,    \
    _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_DEFAULT,   \
    cmuOscMode_External,                           \
  }
#elif defined(_CMU_HFXOCTRL_MASK)
/**
 * Default HFXO initialization values for Platform 2 devices, which contain a
 * separate HFXOCTRL register.
 */
#if defined(_EFR_DEVICE)
#define CMU_HFXOINIT_DEFAULT                                        \
  {                                                                 \
    false,      /* Low-noise mode for EFR32 */                      \
    false,      /* @deprecated no longer in use */                  \
    false,      /* @deprecated no longer in use */                  \
    false,      /* @deprecated no longer in use */                  \
    _CMU_HFXOSTARTUPCTRL_CTUNE_DEFAULT,                             \
    _CMU_HFXOSTEADYSTATECTRL_CTUNE_DEFAULT,                         \
    0xA,        /* Default Shunt steady-state current */            \
    0x20,       /* Matching errata fix in @ref CHIP_Init() */       \
    0x7,        /* Recommended steady-state XO core bias current */ \
    0x6,        /* Recommended peak detection threshold */          \
    0x2,        /* Recommended shunt optimization timeout */        \
    0xA,        /* Recommended peak detection timeout  */           \
    0x4,        /* Recommended steady timeout */                    \
    _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_DEFAULT,                    \
    cmuOscMode_Crystal,                                             \
  }
#else /* EFM32 device */
#define CMU_HFXOINIT_DEFAULT                                         \
  {                                                                  \
    true,       /* Low-power mode for EFM32 */                       \
    false,      /* @deprecated no longer in use */                   \
    false,      /* @deprecated no longer in use */                   \
    false,      /* @deprecated no longer in use */                   \
    _CMU_HFXOSTARTUPCTRL_CTUNE_DEFAULT,                              \
    _CMU_HFXOSTEADYSTATECTRL_CTUNE_DEFAULT,                          \
    0xA,        /* Default shunt steady-state current */             \
    0x20,       /* Matching errata fix in @ref CHIP_Init() */        \
    0x7,        /* Recommended steady-state osc core bias current */ \
    0x6,        /* Recommended peak detection threshold */           \
    0x2,        /* Recommended shunt optimization timeout */         \
    0xA,        /* Recommended peak detection timeout  */            \
    0x4,        /* Recommended steady timeout */                     \
    _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_DEFAULT,                     \
    cmuOscMode_Crystal,                                              \
  }
#endif /* _EFR_DEVICE */
#define CMU_HFXOINIT_EXTERNAL_CLOCK                                            \
  {                                                                            \
    true,       /* Low-power mode */                                           \
    false,      /* @deprecated no longer in use */                             \
    false,      /* @deprecated no longer in use */                             \
    false,      /* @deprecated no longer in use */                             \
    0,          /* Startup CTUNE=0 recommended for external clock */           \
    0,          /* Steady  CTUNE=0 recommended for external clock */           \
    0xA,        /* Default shunt steady-state current */                       \
    0,          /* Startup IBTRIMXOCORE=0 recommended for external clock */    \
    0,          /* Steady  IBTRIMXOCORE=0 recommended for external clock */    \
    0x6,        /* Recommended peak detection threshold */                     \
    0x2,        /* Recommended shunt optimization timeout */                   \
    0x0,        /* Peak-detect not recommended for external clock usage */     \
    _CMU_HFXOTIMEOUTCTRL_STEADYTIMEOUT_2CYCLES, /* Minimal steady timeout */   \
    _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_2CYCLES, /* Minimal startup timeout */ \
    cmuOscMode_External,                                                       \
  }
#else /* _CMU_HFXOCTRL_MASK */
/**
 * Default HFXO initialization values for Platform 1 devices.
 */
#define CMU_HFXOINIT_DEFAULT                                   \
  {                                                            \
    _CMU_CTRL_HFXOBOOST_DEFAULT, /* 100% HFXO boost */         \
    _CMU_CTRL_HFXOTIMEOUT_DEFAULT, /* 16 K startup delay */    \
    false,                       /* Disable glitch detector */ \
    cmuOscMode_Crystal,          /* Crystal oscillator */      \
  }
#define CMU_HFXOINIT_EXTERNAL_CLOCK                                      \
  {                                                                      \
    0,                           /* Minimal HFXO boost, 50% */           \
    _CMU_CTRL_HFXOTIMEOUT_8CYCLES, /* Minimal startup delay, 8 cycles */ \
    false,                       /* Disable glitch detector */           \
    cmuOscMode_External,         /* External digital clock */            \
  }
#endif /* _CMU_HFXOCTRL_MASK */

#if defined(CMU_OSCENCMD_DPLLEN)
/** DPLL initialization structure. Frequency will be Fref*(N+1)/(M+1). */
typedef struct {
  uint32_t  frequency;                  /**< PLL frequency value, max 40 MHz. */
  uint16_t  n;                          /**< Factor N. 300 <= N <= 4095       */
  uint16_t  m;                          /**< Factor M. M <= 4095              */
  uint8_t   ssInterval;                 /**< Spread spectrum update interval. */
  uint8_t   ssAmplitude;                /**< Spread spectrum amplitude.       */
  CMU_DPLLClkSel_TypeDef    refClk;     /**< Reference clock selector.        */
  CMU_DPLLEdgeSel_TypeDef   edgeSel;    /**< Reference clock edge detect selector. */
  CMU_DPLLLockMode_TypeDef  lockMode;   /**< DPLL lock mode selector.         */
  bool      autoRecover;                /**< Enable automatic lock recovery.  */
} CMU_DPLLInit_TypeDef;

/**
 * DPLL initialization values for 39,998,805 Hz using LFXO as reference
 * clock, M=2 and N=3661.
 */
#define CMU_DPLL_LFXO_TO_40MHZ                                             \
  {                                                                        \
    39998805,                     /* Target frequency.                  */ \
    3661,                         /* Factor N.                          */ \
    2,                            /* Factor M.                          */ \
    0,                            /* No spread spectrum clocking.       */ \
    0,                            /* No spread spectrum clocking.       */ \
    cmuDPLLClkSel_Lfxo,           /* Select LFXO as reference clock.    */ \
    cmuDPLLEdgeSel_Fall,          /* Select falling edge of ref clock.  */ \
    cmuDPLLLockMode_Freq,         /* Use frequency lock mode.           */ \
    true                          /* Enable automatic lock recovery.    */ \
  }
#endif // CMU_OSCENCMD_DPLLEN

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

#if defined(_CMU_AUXHFRCOCTRL_BAND_MASK)
CMU_AUXHFRCOBand_TypeDef  CMU_AUXHFRCOBandGet(void);
void                      CMU_AUXHFRCOBandSet(CMU_AUXHFRCOBand_TypeDef band);

#elif defined(_CMU_AUXHFRCOCTRL_FREQRANGE_MASK)
CMU_AUXHFRCOFreq_TypeDef  CMU_AUXHFRCOBandGet(void);
void                      CMU_AUXHFRCOBandSet(CMU_AUXHFRCOFreq_TypeDef setFreq);
#endif

uint32_t              CMU_Calibrate(uint32_t HFCycles, CMU_Osc_TypeDef reference);

#if defined(_CMU_CALCTRL_UPSEL_MASK) && defined(_CMU_CALCTRL_DOWNSEL_MASK)
void                  CMU_CalibrateConfig(uint32_t downCycles, CMU_Osc_TypeDef downSel,
                                          CMU_Osc_TypeDef upSel);
#endif

uint32_t              CMU_CalibrateCountGet(void);
void                  CMU_ClockEnable(CMU_Clock_TypeDef clock, bool enable);
CMU_ClkDiv_TypeDef    CMU_ClockDivGet(CMU_Clock_TypeDef clock);
void                  CMU_ClockDivSet(CMU_Clock_TypeDef clock, CMU_ClkDiv_TypeDef div);
uint32_t              CMU_ClockFreqGet(CMU_Clock_TypeDef clock);

#if defined(_SILICON_LABS_32B_SERIES_1)
void                  CMU_ClockPrescSet(CMU_Clock_TypeDef clock, CMU_ClkPresc_TypeDef presc);
uint32_t              CMU_ClockPrescGet(CMU_Clock_TypeDef clock);
#endif

void                  CMU_ClockSelectSet(CMU_Clock_TypeDef clock, CMU_Select_TypeDef ref);
CMU_Select_TypeDef    CMU_ClockSelectGet(CMU_Clock_TypeDef clock);

#if defined(CMU_OSCENCMD_DPLLEN)
bool                  CMU_DPLLLock(const CMU_DPLLInit_TypeDef *init);
#endif
void                  CMU_FreezeEnable(bool enable);

#if defined(_CMU_HFRCOCTRL_BAND_MASK)
CMU_HFRCOBand_TypeDef CMU_HFRCOBandGet(void);
void                  CMU_HFRCOBandSet(CMU_HFRCOBand_TypeDef band);

#elif defined(_CMU_HFRCOCTRL_FREQRANGE_MASK)
CMU_HFRCOFreq_TypeDef CMU_HFRCOBandGet(void);
void                  CMU_HFRCOBandSet(CMU_HFRCOFreq_TypeDef setFreq);
#endif

#if defined(_CMU_HFRCOCTRL_SUDELAY_MASK)
uint32_t              CMU_HFRCOStartupDelayGet(void);
void                  CMU_HFRCOStartupDelaySet(uint32_t delay);
#endif

#if defined(_CMU_USHFRCOCTRL_FREQRANGE_MASK)
CMU_USHFRCOFreq_TypeDef CMU_USHFRCOBandGet(void);
void                  CMU_USHFRCOBandSet(CMU_USHFRCOFreq_TypeDef setFreq);
#endif

#if defined(_CMU_HFXOCTRL_AUTOSTARTEM0EM1_MASK)
void                  CMU_HFXOAutostartEnable(uint32_t userSel,
                                              bool enEM0EM1Start,
                                              bool enEM0EM1StartSel);
#endif

void                  CMU_HFXOInit(const CMU_HFXOInit_TypeDef *hfxoInit);

uint32_t              CMU_LCDClkFDIVGet(void);
void                  CMU_LCDClkFDIVSet(uint32_t div);
void                  CMU_LFXOInit(const CMU_LFXOInit_TypeDef *lfxoInit);

void                  CMU_OscillatorEnable(CMU_Osc_TypeDef osc, bool enable, bool wait);
uint32_t              CMU_OscillatorTuningGet(CMU_Osc_TypeDef osc);
void                  CMU_OscillatorTuningSet(CMU_Osc_TypeDef osc, uint32_t val);

#if defined(_CMU_HFXOCTRL_PEAKDETSHUNTOPTMODE_MASK) || defined(_CMU_HFXOCTRL_PEAKDETMODE_MASK)
bool CMU_OscillatorTuningWait(CMU_Osc_TypeDef osc, CMU_HFXOTuningMode_TypeDef mode);
bool CMU_OscillatorTuningOptimize(CMU_Osc_TypeDef osc,
                                  CMU_HFXOTuningMode_TypeDef mode,
                                  bool wait);
#endif

#if (_SILICON_LABS_32B_SERIES < 2)
bool                  CMU_PCNTClockExternalGet(unsigned int instance);
void                  CMU_PCNTClockExternalSet(unsigned int instance, bool external);
#endif

#if defined(_CMU_USHFRCOCONF_BAND_MASK)
CMU_USHFRCOBand_TypeDef   CMU_USHFRCOBandGet(void);
void                      CMU_USHFRCOBandSet(CMU_USHFRCOBand_TypeDef band);
#endif
void                  CMU_UpdateWaitStates(uint32_t freq, int vscale);

#if defined(CMU_CALCTRL_CONT)
/***************************************************************************//**
 * @brief
 *   Configure continuous calibration mode.
 * @param[in] enable
 *   If true, enables continuous calibration, if false disables continuous
 *   calibration.
 ******************************************************************************/
__STATIC_INLINE void CMU_CalibrateCont(bool enable)
{
  BUS_RegBitWrite(&CMU->CALCTRL, _CMU_CALCTRL_CONT_SHIFT, (uint32_t)enable);
}
#endif

/***************************************************************************//**
 * @brief
 *   Start calibration.
 * @note
 *   This call is usually invoked after @ref CMU_CalibrateConfig() and possibly
 *   @ref CMU_CalibrateCont().
 ******************************************************************************/
__STATIC_INLINE void CMU_CalibrateStart(void)
{
  CMU->CMD = CMU_CMD_CALSTART;
}

#if defined(CMU_CMD_CALSTOP)
/***************************************************************************//**
 * @brief
 *   Stop the calibration counters.
 ******************************************************************************/
__STATIC_INLINE void CMU_CalibrateStop(void)
{
  CMU->CMD = CMU_CMD_CALSTOP;
}
#endif

/***************************************************************************//**
 * @brief
 *   Convert dividend to logarithmic value. It only works for even
 *   numbers equal to 2^n.
 *
 * @param[in] div
 *   An unscaled dividend.
 *
 * @return
 *   Logarithm of 2, as used by fixed prescalers.
 ******************************************************************************/
__STATIC_INLINE uint32_t CMU_DivToLog2(CMU_ClkDiv_TypeDef div)
{
  uint32_t log2;

  /* Fixed 2^n prescalers take argument of 32768 or less. */
  EFM_ASSERT((div > 0U) && (div <= 32768U));

  /* Count leading zeroes and "reverse" result */
  log2 = 31UL - __CLZ(div);

  return log2;
}

#if defined(CMU_OSCENCMD_DPLLEN)
/***************************************************************************//**
 * @brief
 *   Unlock DPLL.
 * @note
 *   HFRCO is not turned off.
 ******************************************************************************/
__STATIC_INLINE void CMU_DPLLUnlock(void)
{
  CMU->OSCENCMD  = CMU_OSCENCMD_DPLLDIS;
}
#endif

/***************************************************************************//**
 * @brief
 *   Clear one or more pending CMU interrupts.
 *
 * @param[in] flags
 *   CMU interrupt sources to clear.
 ******************************************************************************/
__STATIC_INLINE void CMU_IntClear(uint32_t flags)
{
  CMU->IFC = flags;
}

/***************************************************************************//**
 * @brief
 *   Disable one or more CMU interrupts.
 *
 * @param[in] flags
 *   CMU interrupt sources to disable.
 ******************************************************************************/
__STATIC_INLINE void CMU_IntDisable(uint32_t flags)
{
  CMU->IEN &= ~flags;
}

/***************************************************************************//**
 * @brief
 *   Enable one or more CMU interrupts.
 *
 * @note
 *   Depending on use case, a pending interrupt may already be set prior to
 *   enabling the interrupt. Consider using @ref CMU_IntClear() prior to enabling
 *   if the pending interrupt should be ignored.
 *
 * @param[in] flags
 *   CMU interrupt sources to enable.
 ******************************************************************************/
__STATIC_INLINE void CMU_IntEnable(uint32_t flags)
{
  CMU->IEN |= flags;
}

/***************************************************************************//**
 * @brief
 *   Get pending CMU interrupts.
 *
 * @return
 *   CMU interrupt sources pending.
 ******************************************************************************/
__STATIC_INLINE uint32_t CMU_IntGet(void)
{
  return CMU->IF;
}

/***************************************************************************//**
 * @brief
 *   Get enabled and pending CMU interrupt flags.
 *
 * @details
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @note
 *   This function does not clear event bits.
 *
 * @return
 *   Pending and enabled CMU interrupt sources.
 *   The return value is the bitwise AND of
 *   - the enabled interrupt sources in CMU_IEN and
 *   - the pending interrupt flags CMU_IF
 ******************************************************************************/
__STATIC_INLINE uint32_t CMU_IntGetEnabled(void)
{
  uint32_t ien;

  ien = CMU->IEN;
  return CMU->IF & ien;
}

/**************************************************************************//**
 * @brief
 *   Set one or more pending CMU interrupts.
 *
 * @param[in] flags
 *   CMU interrupt sources to set to pending.
 *****************************************************************************/
__STATIC_INLINE void CMU_IntSet(uint32_t flags)
{
  CMU->IFS = flags;
}

/***************************************************************************//**
 * @brief
 *   Lock the CMU to protect some of its registers against unintended
 *   modification.
 *
 * @details
 *   See the reference manual for CMU registers that will be
 *   locked.
 *
 * @note
 *   If locking the CMU registers, they must be unlocked prior to using any
 *   CMU API functions modifying CMU registers protected by the lock.
 ******************************************************************************/
__STATIC_INLINE void CMU_Lock(void)
{
  CMU->LOCK = CMU_LOCK_LOCKKEY_LOCK;
}

/***************************************************************************//**
 * @brief
 *   Convert logarithm of 2 prescaler to division factor.
 *
 * @param[in] log2
 *   Logarithm of 2, as used by fixed prescalers.
 *
 * @return
 *   Dividend.
 ******************************************************************************/
__STATIC_INLINE uint32_t CMU_Log2ToDiv(uint32_t log2)
{
  EFM_ASSERT(log2 < 32U);
  return 1UL << log2;
}

#if defined(_SILICON_LABS_32B_SERIES_1)
/***************************************************************************//**
 * @brief
 *   Convert prescaler dividend to a logarithmic value. It only works for even
 *   numbers equal to 2^n.
 *
 * @param[in] presc
 *   An unscaled dividend (dividend = presc + 1).
 *
 * @return
 *   Logarithm of 2, as used by fixed 2^n prescalers.
 ******************************************************************************/
__STATIC_INLINE uint32_t CMU_PrescToLog2(CMU_ClkPresc_TypeDef presc)
{
  uint32_t log2;

  /* Integer prescalers take argument less than 32768. */
  EFM_ASSERT(presc < 32768U);

  /* Count leading zeroes and "reverse" result. */
  log2 = 31UL - __CLZ(presc + (uint32_t) 1);

  /* Check that prescaler is a 2^n number. */
  EFM_ASSERT(presc == (CMU_Log2ToDiv(log2) - 1U));

  return log2;
}
#endif

/***************************************************************************//**
 * @brief
 *   Unlock the CMU so that writing to locked registers again is possible.
 ******************************************************************************/
__STATIC_INLINE void CMU_Unlock(void)
{
  CMU->LOCK = CMU_LOCK_LOCKKEY_UNLOCK;
}

#if defined(_CMU_HFRCOCTRL_FREQRANGE_MASK)
/***************************************************************************//**
 * @brief
 *   Get the current HFRCO frequency.
 *
 * @deprecated
 *   A deprecated function. New code should use @ref CMU_HFRCOBandGet().
 *
 * @return
 *   HFRCO frequency.
 ******************************************************************************/
__STATIC_INLINE CMU_HFRCOFreq_TypeDef CMU_HFRCOFreqGet(void)
{
  return CMU_HFRCOBandGet();
}

/***************************************************************************//**
 * @brief
 *   Set HFRCO calibration for the selected target frequency.
 *
 * @deprecated
 *   A deprecated function. New code should use @ref CMU_HFRCOBandSet().
 *
 * @param[in] setFreq
 *   HFRCO frequency to set.
 ******************************************************************************/
__STATIC_INLINE void CMU_HFRCOFreqSet(CMU_HFRCOFreq_TypeDef setFreq)
{
  CMU_HFRCOBandSet(setFreq);
}
#endif

#if defined(_CMU_AUXHFRCOCTRL_FREQRANGE_MASK)
/***************************************************************************//**
 * @brief
 *   Get the current AUXHFRCO frequency.
 *
 * @deprecated
 *   A deprecated function. New code should use @ref CMU_AUXHFRCOBandGet().
 *
 * @return
 *   AUXHFRCO frequency.
 ******************************************************************************/
__STATIC_INLINE CMU_AUXHFRCOFreq_TypeDef CMU_AUXHFRCOFreqGet(void)
{
  return CMU_AUXHFRCOBandGet();
}

/***************************************************************************//**
 * @brief
 *   Set AUXHFRCO calibration for the selected target frequency.
 *
 * @deprecated
 *   A deprecated function. New code should use @ref CMU_AUXHFRCOBandSet().
 *
 * @param[in] setFreq
 *   AUXHFRCO frequency to set.
 ******************************************************************************/
__STATIC_INLINE void CMU_AUXHFRCOFreqSet(CMU_AUXHFRCOFreq_TypeDef setFreq)
{
  CMU_AUXHFRCOBandSet(setFreq);
}
#endif

#endif // defined(_SILICON_LABS_32B_SERIES_2)

/** @} (end addtogroup CMU) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(CMU_PRESENT) */
#endif /* EM_CMU_H */
