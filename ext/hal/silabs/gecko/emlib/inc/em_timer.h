/***************************************************************************//**
 * @file em_timer.h
 * @brief Timer/counter (TIMER) peripheral API
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

#ifndef EM_TIMER_H
#define EM_TIMER_H

#include "em_device.h"
#if defined(TIMER_COUNT) && (TIMER_COUNT > 0)

#include <stdbool.h>
#include "em_assert.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup TIMER
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/** Validation of TIMER register block pointer reference for assert statements. */
#define TIMER_REF_VALID(ref)  TIMER_Valid(ref)

/** Validation of TIMER compare/capture channel number. */
#if defined(_SILICON_LABS_32B_SERIES_0)
#define TIMER_CH_VALID(ch)    ((ch) < 3)
#elif defined(_SILICON_LABS_32B_SERIES_1)
#define TIMER_CH_VALID(ch)    ((ch) < 4)
#elif defined(_SILICON_LABS_32B_SERIES_2)
#define TIMER_CH_VALID(ch)    ((ch) < 3)
#else
#error "Unknown device. Undefined number of channels."
#endif

/** @endcond */

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** Timer compare/capture mode. */
typedef enum {
#if defined (_TIMER_CC_CTRL_MODE_MASK)
  timerCCModeOff     = _TIMER_CC_CTRL_MODE_OFF,           /**< Channel turned off. */
  timerCCModeCapture = _TIMER_CC_CTRL_MODE_INPUTCAPTURE,  /**< Input capture. */
  timerCCModeCompare = _TIMER_CC_CTRL_MODE_OUTPUTCOMPARE, /**< Output compare. */
  timerCCModePWM     = _TIMER_CC_CTRL_MODE_PWM            /**< Pulse-Width modulation. */
#endif
#if defined (_TIMER_CC_CFG_MODE_MASK)
  timerCCModeOff     = _TIMER_CC_CFG_MODE_OFF,            /**< Channel turned off. */
  timerCCModeCapture = _TIMER_CC_CFG_MODE_INPUTCAPTURE,   /**< Input capture. */
  timerCCModeCompare = _TIMER_CC_CFG_MODE_OUTPUTCOMPARE,  /**< Output compare. */
  timerCCModePWM     = _TIMER_CC_CFG_MODE_PWM             /**< Pulse-Width modulation. */
#endif
} TIMER_CCMode_TypeDef;

/** Clock select. */
typedef enum {
#if defined (_TIMER_CTRL_CLKSEL_MASK)
  timerClkSelHFPerClk = _TIMER_CTRL_CLKSEL_PRESCHFPERCLK, /**< Prescaled HFPER clock. */
  timerClkSelCC1      = _TIMER_CTRL_CLKSEL_CC1,           /**< Compare/Capture Channel 1 Input. */
  timerClkSelCascade  = _TIMER_CTRL_CLKSEL_TIMEROUF       /**< Cascaded clocked by underflow or overflow by lower numbered timer. */
#endif
#if defined (_TIMER_CFG_CLKSEL_MASK)
  timerClkSelHFPerClk = _TIMER_CFG_CLKSEL_PRESCEM01GRPACLK, /**< Prescaled EM01GRPA clock. */
  timerClkSelCC1      = _TIMER_CFG_CLKSEL_CC1,              /**< Compare/Capture Channel 1 Input. */
  timerClkSelCascade  = _TIMER_CFG_CLKSEL_TIMEROUF          /**< Cascaded clocked by underflow or overflow by lower numbered timer. */
#endif
} TIMER_ClkSel_TypeDef;

/** Input capture edge select. */
typedef enum {
  /** Rising edges detected. */
  timerEdgeRising  = _TIMER_CC_CTRL_ICEDGE_RISING,

  /** Falling edges detected. */
  timerEdgeFalling = _TIMER_CC_CTRL_ICEDGE_FALLING,

  /** Both edges detected. */
  timerEdgeBoth    = _TIMER_CC_CTRL_ICEDGE_BOTH,

  /** No edge detection, leave signal as is. */
  timerEdgeNone    = _TIMER_CC_CTRL_ICEDGE_NONE
} TIMER_Edge_TypeDef;

/** Input capture event control. */
typedef enum {
  /** PRS output pulse, interrupt flag, and DMA request set on every capture. */
  timerEventEveryEdge    = _TIMER_CC_CTRL_ICEVCTRL_EVERYEDGE,
  /** PRS output pulse, interrupt flag, and DMA request set on every second capture. */
  timerEventEvery2ndEdge = _TIMER_CC_CTRL_ICEVCTRL_EVERYSECONDEDGE,
  /**
   * PRS output pulse, interrupt flag, and DMA request set on rising edge (if
   * input capture edge = BOTH).
   */
  timerEventRising       = _TIMER_CC_CTRL_ICEVCTRL_RISING,
  /**
   * PRS output pulse, interrupt flag, and DMA request set on falling edge (if
   * input capture edge = BOTH).
   */
  timerEventFalling      = _TIMER_CC_CTRL_ICEVCTRL_FALLING
} TIMER_Event_TypeDef;

/** Input edge action. */
typedef enum {
  /** No action taken. */
  timerInputActionNone        = _TIMER_CTRL_FALLA_NONE,

  /** Start counter without reload. */
  timerInputActionStart       = _TIMER_CTRL_FALLA_START,

  /** Stop counter without reload. */
  timerInputActionStop        = _TIMER_CTRL_FALLA_STOP,

  /** Reload and start counter. */
  timerInputActionReloadStart = _TIMER_CTRL_FALLA_RELOADSTART
} TIMER_InputAction_TypeDef;

/** Timer mode. */
typedef enum {
#if defined (_TIMER_CTRL_MODE_MASK)
  timerModeUp     = _TIMER_CTRL_MODE_UP,        /**< Up-counting. */
  timerModeDown   = _TIMER_CTRL_MODE_DOWN,      /**< Down-counting. */
  timerModeUpDown = _TIMER_CTRL_MODE_UPDOWN,    /**< Up/down-counting. */
  timerModeQDec   = _TIMER_CTRL_MODE_QDEC       /**< Quadrature decoder. */
#endif
#if defined (_TIMER_CFG_MODE_MASK)
  timerModeUp     = _TIMER_CFG_MODE_UP,         /**< Up-counting. */
  timerModeDown   = _TIMER_CFG_MODE_DOWN,       /**< Down-counting. */
  timerModeUpDown = _TIMER_CFG_MODE_UPDOWN,     /**< Up/down-counting. */
  timerModeQDec   = _TIMER_CFG_MODE_QDEC        /**< Quadrature decoder. */
#endif
} TIMER_Mode_TypeDef;

/** Compare/capture output action. */
typedef enum {
  /** No action. */
  timerOutputActionNone   = _TIMER_CC_CTRL_CUFOA_NONE,

  /** Toggle on event. */
  timerOutputActionToggle = _TIMER_CC_CTRL_CUFOA_TOGGLE,

  /** Clear on event. */
  timerOutputActionClear  = _TIMER_CC_CTRL_CUFOA_CLEAR,

  /** Set on event. */
  timerOutputActionSet    = _TIMER_CC_CTRL_CUFOA_SET
} TIMER_OutputAction_TypeDef;

/** Prescaler. */
typedef enum {
#if defined (_TIMER_CTRL_PRESC_MASK)
  timerPrescale1    = _TIMER_CTRL_PRESC_DIV1,       /**< Divide by 1. */
  timerPrescale2    = _TIMER_CTRL_PRESC_DIV2,       /**< Divide by 2. */
  timerPrescale4    = _TIMER_CTRL_PRESC_DIV4,       /**< Divide by 4. */
  timerPrescale8    = _TIMER_CTRL_PRESC_DIV8,       /**< Divide by 8. */
  timerPrescale16   = _TIMER_CTRL_PRESC_DIV16,      /**< Divide by 16. */
  timerPrescale32   = _TIMER_CTRL_PRESC_DIV32,      /**< Divide by 32. */
  timerPrescale64   = _TIMER_CTRL_PRESC_DIV64,      /**< Divide by 64. */
  timerPrescale128  = _TIMER_CTRL_PRESC_DIV128,     /**< Divide by 128. */
  timerPrescale256  = _TIMER_CTRL_PRESC_DIV256,     /**< Divide by 256. */
  timerPrescale512  = _TIMER_CTRL_PRESC_DIV512,     /**< Divide by 512. */
  timerPrescale1024 = _TIMER_CTRL_PRESC_DIV1024     /**< Divide by 1024. */
#endif
#if defined (_TIMER_CFG_PRESC_MASK)
  timerPrescale1    = _TIMER_CFG_PRESC_DIV1,        /**< Divide by 1. */
  timerPrescale2    = _TIMER_CFG_PRESC_DIV2,        /**< Divide by 2. */
  timerPrescale4    = _TIMER_CFG_PRESC_DIV4,        /**< Divide by 4. */
  timerPrescale8    = _TIMER_CFG_PRESC_DIV8,        /**< Divide by 8. */
  timerPrescale16   = _TIMER_CFG_PRESC_DIV16,       /**< Divide by 16. */
  timerPrescale32   = _TIMER_CFG_PRESC_DIV32,       /**< Divide by 32. */
  timerPrescale64   = _TIMER_CFG_PRESC_DIV64,       /**< Divide by 64. */
  timerPrescale128  = _TIMER_CFG_PRESC_DIV128,      /**< Divide by 128. */
  timerPrescale256  = _TIMER_CFG_PRESC_DIV256,      /**< Divide by 256. */
  timerPrescale512  = _TIMER_CFG_PRESC_DIV512,      /**< Divide by 512. */
  timerPrescale1024 = _TIMER_CFG_PRESC_DIV1024      /**< Divide by 1024. */
#endif
} TIMER_Prescale_TypeDef;

/** Peripheral Reflex System signal. */
typedef uint8_t TIMER_PRSSEL_TypeDef;

/** Deprecated PRS channel selector. New code should use an integer instead of
 *  using these deprecated enum values. */
#define timerPRSSELCh0    0UL
#define timerPRSSELCh1    1UL
#define timerPRSSELCh2    2UL
#define timerPRSSELCh3    3UL
#define timerPRSSELCh4    4UL
#define timerPRSSELCh5    5UL
#define timerPRSSELCh6    6UL
#define timerPRSSELCh7    7UL
#define timerPRSSELCh8    8UL
#define timerPRSSELCh9    9UL
#define timerPRSSELCh10  10UL
#define timerPRSSELCh11  11UL

#if defined (_TIMER_CC_CFG_INSEL_MASK)
typedef enum {
  timerPrsInputNone       =  0x0,
  timerPrsInputSync       =  _TIMER_CC_CFG_INSEL_PRSSYNC,
  timerPrsInputAsyncLevel =  _TIMER_CC_CFG_INSEL_PRSASYNCLEVEL,
  timerPrsInputAsyncPulse =  _TIMER_CC_CFG_INSEL_PRSASYNCPULSE,
} TIMER_PrsInput_TypeDef;
#endif

#if defined(_TIMER_DTFC_DTFA_MASK) || defined(_TIMER_DTFCFG_DTFA_MASK)
/** DT (Dead Time) Fault Actions. */
typedef enum {
#if defined(_TIMER_DTFC_DTFA_MASK)
  timerDtiFaultActionNone     = _TIMER_DTFC_DTFA_NONE,          /**< No action on fault. */
  timerDtiFaultActionInactive = _TIMER_DTFC_DTFA_INACTIVE,      /**< Set outputs inactive. */
  timerDtiFaultActionClear    = _TIMER_DTFC_DTFA_CLEAR,         /**< Clear outputs. */
  timerDtiFaultActionTristate = _TIMER_DTFC_DTFA_TRISTATE       /**< Tristate outputs. */
#endif
#if defined(_TIMER_DTFCFG_DTFA_MASK)
  timerDtiFaultActionNone     = _TIMER_DTFCFG_DTFA_NONE,        /**< No action on fault. */
  timerDtiFaultActionInactive = _TIMER_DTFCFG_DTFA_INACTIVE,    /**< Set outputs inactive. */
  timerDtiFaultActionClear    = _TIMER_DTFCFG_DTFA_CLEAR,       /**< Clear outputs. */
  timerDtiFaultActionTristate = _TIMER_DTFCFG_DTFA_TRISTATE     /**< Tristate outputs. */
#endif
} TIMER_DtiFaultAction_TypeDef;
#endif

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** TIMER initialization structure. */
typedef struct {
  /** Start counting when initialization completed. */
  bool                      enable;

  /** Counter shall keep running during debug halt. */
  bool                      debugRun;

  /** Prescaling factor, if HFPER clock used. */
  TIMER_Prescale_TypeDef    prescale;

  /** Clock selection. */
  TIMER_ClkSel_TypeDef      clkSel;

#if defined(TIMER_CTRL_X2CNT) && (defined(TIMER_CTRL_ATI) || defined(TIMER_CFG_ATI))
  /** 2x Count mode, counter increments/decrements by 2, meant for PWM mode. */
  bool                      count2x;

  /** ATI (Always Track Inputs) makes CCPOL always track
   * the polarity of the inputs. */
  bool                      ati;
#endif

  /** Action on falling input edge. */
  TIMER_InputAction_TypeDef fallAction;

  /** Action on rising input edge. */
  TIMER_InputAction_TypeDef riseAction;

  /** Counting mode. */
  TIMER_Mode_TypeDef        mode;

  /** DMA request clear on active. */
  bool                      dmaClrAct;

  /** Select X2 or X4 quadrature decode mode (if used). */
  bool                      quadModeX4;

  /** Determines if only counting up or down once. */
  bool                      oneShot;

  /** Timer start/stop/reload by other timers. */
  bool                      sync;
} TIMER_Init_TypeDef;

/** Default configuration for TIMER initialization structure. */
#if defined(TIMER_CTRL_X2CNT) && (defined(TIMER_CTRL_ATI) || defined(TIMER_CFG_ATI))
#define TIMER_INIT_DEFAULT                                                            \
  {                                                                                   \
    true,                 /* Enable timer when initialization completes. */           \
    false,                /* Stop counter during debug halt. */                       \
    timerPrescale1,       /* No prescaling. */                                        \
    timerClkSelHFPerClk,  /* Select HFPER clock. */                                   \
    false,                /* Not 2x count mode. */                                    \
    false,                /* No ATI. */                                               \
    timerInputActionNone, /* No action on falling input edge. */                      \
    timerInputActionNone, /* No action on rising input edge. */                       \
    timerModeUp,          /* Up-counting. */                                          \
    false,                /* Do not clear DMA requests when DMA channel is active. */ \
    false,                /* Select X2 quadrature decode mode (if used). */           \
    false,                /* Disable one shot. */                                     \
    false                 /* Not started/stopped/reloaded by other timers. */         \
  }
#else
#define TIMER_INIT_DEFAULT                                                            \
  {                                                                                   \
    true,                 /* Enable timer when initialization completes. */           \
    false,                /* Stop counter during debug halt. */                       \
    timerPrescale1,       /* No prescaling. */                                        \
    timerClkSelHFPerClk,  /* Select HFPER clock. */                                   \
    timerInputActionNone, /* No action on falling input edge. */                      \
    timerInputActionNone, /* No action on rising input edge. */                       \
    timerModeUp,          /* Up-counting. */                                          \
    false,                /* Do not clear DMA requests when DMA channel is active. */ \
    false,                /* Select X2 quadrature decode mode (if used). */           \
    false,                /* Disable one shot. */                                     \
    false                 /* Not started/stopped/reloaded by other timers. */         \
  }
#endif

/** PRS Output configuration. */
typedef enum {
  timerPrsOutputPulse   = 0,                   /**< Pulse PRS output from a channel. */
  timerPrsOutputLevel   = 1,                   /**< PRS output follows CC out level. */
  timerPrsOutputDefault = timerPrsOutputPulse, /**< Default PRS output behavior. */
} TIMER_PrsOutput_t;

/** TIMER compare/capture initialization structure. */
typedef struct {
  /** Input capture event control. */
  TIMER_Event_TypeDef        eventCtrl;

  /** Input capture edge select. */
  TIMER_Edge_TypeDef         edge;

  /**
   * Peripheral reflex system trigger selection. Only applicable if @p prsInput
   * is enabled.
   */
  TIMER_PRSSEL_TypeDef       prsSel;

  /** Counter underflow output action. */
  TIMER_OutputAction_TypeDef cufoa;

  /** Counter overflow output action. */
  TIMER_OutputAction_TypeDef cofoa;

  /** Counter match output action. */
  TIMER_OutputAction_TypeDef cmoa;

  /** Compare/capture channel mode. */
  TIMER_CCMode_TypeDef       mode;

  /** Enable digital filter. */
  bool                       filter;

  /** Select TIMERnCCx (false) or PRS input (true). */
  bool                       prsInput;

  /**
   * Compare output initial state. Only used in Output Compare and PWM mode.
   * When true, the compare/PWM output is set high when the counter is
   * disabled. When counting resumes, this value will represent the initial
   * value for the compare/PWM output. If the bit is cleared, the output
   * will be cleared when the counter is disabled.
   */
  bool                       coist;

  /** Invert output from compare/capture channel. */
  bool                       outInvert;

  /**
   * PRS output configuration. PRS output from a timer can either be a
   * pulse output or a level output that follows the CC out value.
   */
  TIMER_PrsOutput_t          prsOutput;

#if defined(_TIMER_CC_CFG_INSEL_MASK)
  /* When PRS input is used this field is used to configure the type of
   * PRS input. */
  TIMER_PrsInput_TypeDef     prsInputType;
#endif
} TIMER_InitCC_TypeDef;

/** Default configuration for TIMER compare/capture initialization structure. */
#if defined(_TIMER_CC_CFG_INSEL_MASK)
#define TIMER_INITCC_DEFAULT                                                 \
  {                                                                          \
    timerEventEveryEdge,    /* Event on every capture. */                    \
    timerEdgeRising,        /* Input capture edge on rising edge. */         \
    0,                      /* Not used by default, select PRS channel 0. */ \
    timerOutputActionNone,  /* No action on underflow. */                    \
    timerOutputActionNone,  /* No action on overflow. */                     \
    timerOutputActionNone,  /* No action on match. */                        \
    timerCCModeOff,         /* Disable compare/capture channel. */           \
    false,                  /* Disable filter. */                            \
    false,                  /* No PRS input. */                              \
    false,                  /* Clear output when counter disabled. */        \
    false,                  /* Do not invert output. */                      \
    timerPrsOutputDefault,  /* Use default PRS output configuration. */      \
    timerPrsInputNone       /* No PRS input, so input type is none. */       \
  }
#else
#define TIMER_INITCC_DEFAULT                                                 \
  {                                                                          \
    timerEventEveryEdge,    /* Event on every capture. */                    \
    timerEdgeRising,        /* Input capture edge on rising edge. */         \
    0,                      /* Not used by default, select PRS channel 0. */ \
    timerOutputActionNone,  /* No action on underflow. */                    \
    timerOutputActionNone,  /* No action on overflow. */                     \
    timerOutputActionNone,  /* No action on match. */                        \
    timerCCModeOff,         /* Disable compare/capture channel. */           \
    false,                  /* Disable filter. */                            \
    false,                  /* No PRS input. */                              \
    false,                  /* Clear output when counter disabled. */        \
    false,                  /* Do not invert output. */                      \
    timerPrsOutputDefault,  /* Use default PRS output configuration. */      \
  }
#endif

#if defined(_TIMER_DTCTRL_MASK)
/** TIMER Dead Time Insertion (DTI) initialization structure. */
typedef struct {
  /** Enable DTI or leave it disabled until @ref TIMER_EnableDTI() is called. */
  bool                          enable;

  /** DTI Output Polarity. */
  bool                          activeLowOut;

  /** DTI Complementary Output Invert. */
  bool                          invertComplementaryOut;

  /** Enable Automatic Start-up functionality (when debugger exits). */
  bool                          autoRestart;

  /** Enable/disable PRS as DTI input. */
  bool                          enablePrsSource;

  /** Select which PRS channel as DTI input. Only valid if @p enablePrsSource
     is enabled. */
  TIMER_PRSSEL_TypeDef          prsSel;

  /** DTI prescaling factor, if HFPER clock used. */
  TIMER_Prescale_TypeDef        prescale;

  /** DTI Rise Time */
  unsigned int                  riseTime;

  /** DTI Fall Time */
  unsigned int                  fallTime;

  /** DTI outputs enable bit mask, consisting of one bit per DTI
      output signal, i.e., CC0, CC1, CC2, CDTI0, CDTI1, and CDTI2.
      This value should consist of one or more TIMER_DTOGEN_DTOGnnnEN flags
      (defined in \<part_name\>_timer.h) OR'ed together. */
  uint32_t                      outputsEnableMask;

  /** Enable core lockup as a fault source. */
  bool                          enableFaultSourceCoreLockup;

  /** Enable debugger as a fault source. */
  bool                          enableFaultSourceDebugger;

  /** Enable PRS fault source 0 (@p faultSourcePrsSel0). */
  bool                          enableFaultSourcePrsSel0;

  /** Select which PRS signal to be PRS fault source 0. */
  TIMER_PRSSEL_TypeDef          faultSourcePrsSel0;

  /** Enable PRS fault source 1 (@p faultSourcePrsSel1). */
  bool                          enableFaultSourcePrsSel1;

  /** Select which PRS signal to be PRS fault source 1. */
  TIMER_PRSSEL_TypeDef          faultSourcePrsSel1;

  /** Fault Action */
  TIMER_DtiFaultAction_TypeDef  faultAction;
} TIMER_InitDTI_TypeDef;

/** Default configuration for TIMER DTI initialization structure. */
#define TIMER_INITDTI_DEFAULT                                                      \
  {                                                                                \
    true,                   /* Enable the DTI. */                                  \
    false,                  /* CC[0|1|2] outputs are active high. */               \
    false,                  /* CDTI[0|1|2] outputs are not inverted. */            \
    false,                  /* No auto restart when debugger exits. */             \
    false,                  /* No PRS source selected. */                          \
    0,                      /* Not used by default, select PRS channel 0. */       \
    timerPrescale1,         /* No prescaling.  */                                  \
    0,                      /* No rise time. */                                    \
    0,                      /* No fall time. */                                    \
    TIMER_DTOGEN_DTOGCC0EN | TIMER_DTOGEN_DTOGCDTI0EN, /* Enable CC0 and CDTI0. */ \
    true,                   /* Enable core lockup as fault source. */              \
    true,                   /* Enable debugger as fault source. */                 \
    false,                  /* Disable PRS fault source 0. */                      \
    0,                      /* Not used by default, select PRS channel 0. */       \
    false,                  /* Disable PRS fault source 1. */                      \
    0,                      /* Not used by default, select PRS channel 0. */       \
    timerDtiFaultActionInactive, /* No fault action. */                            \
  }
#endif /* _TIMER_DTCTRL_MASK */

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Validate TIMER register block pointer.
 *
 * @param[in] ref
 *   Pointer to TIMER peripheral register block.
 *
 * @return
 *   True if ref points to a valid timer, false otherwise.
 ******************************************************************************/
__STATIC_INLINE bool TIMER_Valid(const TIMER_TypeDef *ref)
{
  return (ref == TIMER0)
#if defined(TIMER1)
         || (ref == TIMER1)
#endif
#if defined(TIMER2)
         || (ref == TIMER2)
#endif
#if defined(TIMER3)
         || (ref == TIMER3)
#endif
#if defined(TIMER4)
         || (ref == TIMER4)
#endif
#if defined(TIMER5)
         || (ref == TIMER5)
#endif
#if defined(TIMER6)
         || (ref == TIMER6)
#endif
#if defined(WTIMER0)
         || (ref == WTIMER0)
#endif
#if defined(WTIMER1)
         || (ref == WTIMER1)
#endif
#if defined(WTIMER2)
         || (ref == WTIMER2)
#endif
#if defined(WTIMER3)
         || (ref == WTIMER3)
#endif
  ;
}

/***************************************************************************//**
 * @brief
 *   Get the Max count of the timer.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @return
 *   The max count value of the timer. This is 0xFFFF for 16 bit timers
 *   and 0xFFFFFFFF for 32 bit timers.
 ******************************************************************************/
__STATIC_INLINE uint32_t TIMER_MaxCount(const TIMER_TypeDef *ref)
{
#if defined(WTIMER_PRESENT)
  if ((ref == WTIMER0)
#if defined(WTIMER1)
      || (ref == WTIMER1)
#endif
#if defined(WTIMER2)
      || (ref == WTIMER2)
#endif
#if defined(WTIMER3)
      || (ref == WTIMER3)
#endif
      ) {
    return 0xFFFFFFFFUL;
  }
#else
  (void) ref;
#endif
  return 0xFFFFUL;
}

/***************************************************************************//**
 * @brief
 *   Get compare/capture value for compare/capture channel.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] ch
 *   Compare/capture channel to access.
 *
 * @return
 *   Current capture value.
 ******************************************************************************/
__STATIC_INLINE uint32_t TIMER_CaptureGet(TIMER_TypeDef *timer, unsigned int ch)
{
#if defined (_TIMER_CC_CFG_MASK)
  if ((timer->CC[ch].CFG & _TIMER_CC_CFG_MODE_MASK) == TIMER_CC_CFG_MODE_INPUTCAPTURE) {
    return timer->CC[ch].ICF;
  } else {
    return timer->CC[ch].OC;
  }
#else
  return timer->CC[ch].CCV;
#endif
}

/***************************************************************************//**
 * @brief
 *   Get buffered compare/capture value for compare/capture channel.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] ch
 *   Compare/capture channel to access.
 *
 * @return
 *   Current buffered capture value.
 ******************************************************************************/
__STATIC_INLINE uint32_t TIMER_CaptureBufGet(TIMER_TypeDef *timer, unsigned int ch)
{
#if defined (_TIMER_CC_CFG_MASK)
  if ((timer->CC[ch].CFG & _TIMER_CC_CFG_MODE_MASK) == TIMER_CC_CFG_MODE_INPUTCAPTURE) {
    return timer->CC[ch].ICOF;
  } else {
    return timer->CC[ch].OCB;
  }
#else
  return timer->CC[ch].CCVB;
#endif
}

/***************************************************************************//**
 * @brief
 *   Set compare value buffer for compare/capture channel when operating in
 *   compare or PWM mode.
 *
 * @details
 *   The compare value buffer holds the value which will be written to
 *   TIMERn_CCx_CCV on an update event if the buffer has been updated since
 *   the last event.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] ch
 *   Compare/capture channel to access.
 *
 * @param[in] val
 *   Value to set in compare value buffer register.
 ******************************************************************************/
__STATIC_INLINE void TIMER_CompareBufSet(TIMER_TypeDef *timer,
                                         unsigned int ch,
                                         uint32_t val)
{
  EFM_ASSERT(val <= TIMER_MaxCount(timer));
#if defined (_TIMER_CC_CFG_MASK)
  EFM_ASSERT(timer->EN & TIMER_EN_EN);
  timer->CC[ch].OCB = val;
#else
  timer->CC[ch].CCVB = val;
#endif
}

/***************************************************************************//**
 * @brief
 *   Set compare value for compare/capture channel when operating in compare
 *   or PWM mode.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] ch
 *   Compare/capture channel to access.
 *
 * @param[in] val
 *   Value to set in compare value register.
 ******************************************************************************/
__STATIC_INLINE void TIMER_CompareSet(TIMER_TypeDef *timer,
                                      unsigned int ch,
                                      uint32_t val)
{
  EFM_ASSERT(val <= TIMER_MaxCount(timer));
#if defined (_TIMER_CC_CFG_MASK)
  EFM_ASSERT(timer->EN & TIMER_EN_EN);
  timer->CC[ch].OC = val;
#else
  timer->CC[ch].CCV = val;
#endif
}

/***************************************************************************//**
 * @brief
 *   Get TIMER counter value.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @return
 *   Current TIMER counter value.
 ******************************************************************************/
__STATIC_INLINE uint32_t TIMER_CounterGet(TIMER_TypeDef *timer)
{
  return timer->CNT;
}

/***************************************************************************//**
 * @brief
 *   Set TIMER counter value.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] val
 *   Value to set counter to.
 ******************************************************************************/
__STATIC_INLINE void TIMER_CounterSet(TIMER_TypeDef *timer, uint32_t val)
{
  EFM_ASSERT(val <= TIMER_MaxCount(timer));
#if defined(TIMER_HAS_SET_CLEAR)
  bool enabled = (timer->EN & TIMER_EN_EN) != 0UL;
  timer->EN_SET = TIMER_EN_EN;
#endif
  timer->CNT = val;
#if defined(TIMER_HAS_SET_CLEAR)
  if (!enabled) {
    timer->EN_CLR = TIMER_EN_EN;
  }
#endif
}

/***************************************************************************//**
 * @brief
 *   Start/stop TIMER.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] enable
 *   Set to true to enable counting; set to false otherwise.
 ******************************************************************************/
__STATIC_INLINE void TIMER_Enable(TIMER_TypeDef *timer, bool enable)
{
  EFM_ASSERT(TIMER_REF_VALID(timer));

  if (enable) {
    timer->CMD = TIMER_CMD_START;
  } else {
    timer->CMD = TIMER_CMD_STOP;
  }
}

void TIMER_Init(TIMER_TypeDef *timer, const TIMER_Init_TypeDef *init);
void TIMER_InitCC(TIMER_TypeDef *timer,
                  unsigned int ch,
                  const TIMER_InitCC_TypeDef *init);

#if defined(_TIMER_DTCTRL_MASK)
void TIMER_InitDTI(TIMER_TypeDef *timer, const TIMER_InitDTI_TypeDef *init);

/***************************************************************************//**
 * @brief
 *   Enable or disable DTI unit.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] enable
 *   Set to true to enable DTI unit; set to false otherwise.
 ******************************************************************************/
__STATIC_INLINE void TIMER_EnableDTI(TIMER_TypeDef *timer, bool enable)
{
#if defined(TIMER_HAS_SET_CLEAR)
  uint32_t timerEn = timer->EN & TIMER_EN_EN;
  timer->EN_CLR = TIMER_EN_EN;
  if (enable) {
    timer->DTCFG_SET = TIMER_DTCFG_DTEN;
  } else {
    timer->DTCFG_CLR = TIMER_DTCFG_DTEN;
  }
  timer->EN_SET = timerEn;
#else
  EFM_ASSERT(TIMER0 == timer);

  if (enable) {
    timer->DTCTRL |= TIMER_DTCTRL_DTEN;
  } else {
    timer->DTCTRL &= ~TIMER_DTCTRL_DTEN;
  }
#endif
}

/***************************************************************************//**
 * @brief
 *   Get DTI fault source flags status.
 *
 * @note
 *   Event bits are not cleared by this function.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @return
 *   Status of the DTI fault source flags. Returns one or more valid
 *   DTI fault source flags (TIMER_DTFAULT_nnn) OR'ed together.
 ******************************************************************************/
__STATIC_INLINE uint32_t TIMER_GetDTIFault(TIMER_TypeDef *timer)
{
  EFM_ASSERT(TIMER0 == timer);
  return timer->DTFAULT;
}

/***************************************************************************//**
 * @brief
 *   Clear DTI fault source flags.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] flags
 *   DTI fault source(s) to clear. Use one or more valid DTI fault
 *   source flags (TIMER_DTFAULT_nnn) OR'ed together.
 ******************************************************************************/
__STATIC_INLINE void TIMER_ClearDTIFault(TIMER_TypeDef *timer, uint32_t flags)

{
  EFM_ASSERT(TIMER0 == timer);
#if defined (TIMER_EN_EN)
  EFM_ASSERT(timer->EN & TIMER_EN_EN);
#endif

  timer->DTFAULTC = flags;
}
#endif /* _TIMER_DTCTRL_MASK */

/***************************************************************************//**
 * @brief
 *   Clear one or more pending TIMER interrupts.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] flags
 *   Pending TIMER interrupt source(s) to clear. Use one or more valid
 *   interrupt flags for the TIMER module (TIMER_IF_nnn) OR'ed together.
 ******************************************************************************/
__STATIC_INLINE void TIMER_IntClear(TIMER_TypeDef *timer, uint32_t flags)
{
#if defined (TIMER_HAS_SET_CLEAR)
  timer->IF_CLR = flags;
#else
  timer->IFC = flags;
#endif
}

/***************************************************************************//**
 * @brief
 *   Disable one or more TIMER interrupts.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] flags
 *   TIMER interrupt source(s) to disable. Use one or more valid
 *   interrupt flags for the TIMER module (TIMER_IF_nnn) OR'ed together.
 ******************************************************************************/
__STATIC_INLINE void TIMER_IntDisable(TIMER_TypeDef *timer, uint32_t flags)
{
  timer->IEN &= ~flags;
}

/***************************************************************************//**
 * @brief
 *   Enable one or more TIMER interrupts.
 *
 * @note
 *   Depending on the use, a pending interrupt may already be set prior to
 *   enabling the interrupt.  To ignore a pending interrupt, consider using
 *   TIMER_IntClear() prior to enabling the interrupt.

 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] flags
 *   TIMER interrupt source(s) to enable. Use one or more valid
 *   interrupt flags for the TIMER module (TIMER_IF_nnn) OR'ed together.
 ******************************************************************************/
__STATIC_INLINE void TIMER_IntEnable(TIMER_TypeDef *timer, uint32_t flags)
{
  timer->IEN |= flags;
}

/***************************************************************************//**
 * @brief
 *   Get pending TIMER interrupt flags.
 *
 * @note
 *   Event bits are not cleared by this function.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @return
 *   TIMER interrupt source(s) pending. Returns one or more valid
 *   interrupt flags for the TIMER module (TIMER_IF_nnn) OR'ed together.
 ******************************************************************************/
__STATIC_INLINE uint32_t TIMER_IntGet(TIMER_TypeDef *timer)
{
  return timer->IF;
}

/***************************************************************************//**
 * @brief
 *   Get enabled and pending TIMER interrupt flags.
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @note
 *   Interrupt flags are not cleared by this function.
 *
 * @return
 *   Pending and enabled TIMER interrupt sources.
 *   The return value is the bitwise AND combination of
 *   - the OR combination of enabled interrupt sources in TIMERx_IEN_nnn
 *     register (TIMERx_IEN_nnn) and
 *   - the OR combination of valid interrupt flags of the TIMER module
 *     (TIMERx_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t TIMER_IntGetEnabled(TIMER_TypeDef *timer)
{
  uint32_t ien;

  /* Store TIMER->IEN in temporary variable in order to define explicit order
   * of volatile accesses. */
  ien = timer->IEN;

  /* Bitwise AND of pending and enabled interrupts */
  return timer->IF & ien;
}

/***************************************************************************//**
 * @brief
 *   Set one or more pending TIMER interrupts from SW.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] flags
 *   TIMER interrupt source(s) to set to pending. Use one or more valid
 *   interrupt flags for the TIMER module (TIMER_IF_nnn) OR'ed together.
 ******************************************************************************/
__STATIC_INLINE void TIMER_IntSet(TIMER_TypeDef *timer, uint32_t flags)
{
#if defined (TIMER_HAS_SET_CLEAR)
  timer->IF_SET = flags;
#else
  timer->IFS = flags;
#endif
}

#if defined(_TIMER_DTLOCK_LOCKKEY_LOCK)
/***************************************************************************//**
 * @brief
 *   Lock some of the TIMER registers in order to protect them from being
 *   modified.
 *
 * @details
 *   Refer to the reference manual for TIMER registers that will be locked.
 *
 * @note
 *   If locking the TIMER registers, they must be unlocked prior to using any
 *   TIMER API function that modifies TIMER registers protected by the lock.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 ******************************************************************************/
__STATIC_INLINE void TIMER_Lock(TIMER_TypeDef *timer)
{
  EFM_ASSERT(TIMER0 == timer);
#if defined (TIMER_EN_EN)
  EFM_ASSERT(timer->EN & TIMER_EN_EN);
#endif

  timer->DTLOCK = TIMER_DTLOCK_LOCKKEY_LOCK;
}
#endif

void TIMER_Reset(TIMER_TypeDef *timer);

/***************************************************************************//**
 * @brief
 *   Set top value buffer for timer.
 *
 * @details
 *   When top value buffer register is updated, value is loaded into
 *   top value register at the next wrap around. This feature is useful
 *   in order to update top value safely when timer is running.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] val
 *   Value to set in top value buffer register.
 ******************************************************************************/
__STATIC_INLINE void TIMER_TopBufSet(TIMER_TypeDef *timer, uint32_t val)
{
  EFM_ASSERT(val <= TIMER_MaxCount(timer));
#if defined (TIMER_EN_EN)
  EFM_ASSERT(timer->EN & TIMER_EN_EN);
#endif

  timer->TOPB = val;
}

/***************************************************************************//**
 * @brief
 *   Get top value setting for timer.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @return
 *   Current top value.
 ******************************************************************************/
__STATIC_INLINE uint32_t TIMER_TopGet(TIMER_TypeDef *timer)
{
  return timer->TOP;
}

/***************************************************************************//**
 * @brief
 *   Set top value for timer.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] val
 *   Value to set in top value register.
 ******************************************************************************/
__STATIC_INLINE void TIMER_TopSet(TIMER_TypeDef *timer, uint32_t val)
{
  EFM_ASSERT(val <= TIMER_MaxCount(timer));
#if defined (TIMER_EN_EN)
  EFM_ASSERT(timer->EN & TIMER_EN_EN);
#endif

  timer->TOP = val;
}

#if defined(TIMER_DTLOCK_LOCKKEY_UNLOCK)
/***************************************************************************//**
 * @brief
 *   Unlock TIMER to enable writing to locked registers again.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 ******************************************************************************/
__STATIC_INLINE void TIMER_Unlock(TIMER_TypeDef *timer)
{
  EFM_ASSERT(TIMER0 == timer);
#if defined (TIMER_EN_EN)
  EFM_ASSERT(timer->EN & TIMER_EN_EN);
#endif

  timer->DTLOCK = TIMER_DTLOCK_LOCKKEY_UNLOCK;
}
#endif

/** @} (end addtogroup TIMER) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(TIMER_COUNT) && (TIMER_COUNT > 0) */
#endif /* EM_TIMER_H */
