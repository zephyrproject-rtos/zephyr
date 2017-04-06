/***************************************************************************//**
 * @file em_timer.c
 * @brief Timer/counter (TIMER) Peripheral API
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

#include "em_timer.h"
#if defined(TIMER_COUNT) && (TIMER_COUNT > 0)

#include "em_assert.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup TIMER
 * @brief Timer/Counter (TIMER) Peripheral API
 * @details
 *   The timer module consists of three main parts:
 *   @li General timer config and enable control.
 *   @li Compare/capture control.
 *   @li Dead time insertion control (may not be available for all timers).
 * @{
 ******************************************************************************/


/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Initialize TIMER.
 *
 * @details
 *   Notice that counter top must be configured separately with for instance
 *   TIMER_TopSet(). In addition, compare/capture and dead-time insertion
 *   init must be initialized separately if used. That should probably
 *   be done prior to the use of this function if configuring the TIMER to
 *   start when initialization is completed.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] init
 *   Pointer to TIMER initialization structure.
 ******************************************************************************/
void TIMER_Init(TIMER_TypeDef *timer, const TIMER_Init_TypeDef *init)
{
  EFM_ASSERT(TIMER_REF_VALID(timer));

  /* Stop timer if specified to be disabled (dosn't hurt if already stopped) */
  if (!(init->enable))
  {
    timer->CMD = TIMER_CMD_STOP;
  }

  /* Reset counter */
  timer->CNT = _TIMER_CNT_RESETVALUE;

  timer->CTRL = ((uint32_t)(init->prescale)     << _TIMER_CTRL_PRESC_SHIFT)
                | ((uint32_t)(init->clkSel)     << _TIMER_CTRL_CLKSEL_SHIFT)
                | ((uint32_t)(init->fallAction) << _TIMER_CTRL_FALLA_SHIFT)
                | ((uint32_t)(init->riseAction) << _TIMER_CTRL_RISEA_SHIFT)
                | ((uint32_t)(init->mode)       << _TIMER_CTRL_MODE_SHIFT)
                | (init->debugRun               ?   TIMER_CTRL_DEBUGRUN  : 0)
                | (init->dmaClrAct              ?   TIMER_CTRL_DMACLRACT : 0)
                | (init->quadModeX4             ?   TIMER_CTRL_QDM_X4    : 0)
                | (init->oneShot                ?   TIMER_CTRL_OSMEN     : 0)

#if defined(TIMER_CTRL_X2CNT) && defined(TIMER_CTRL_ATI)
                | (init->count2x                ?   TIMER_CTRL_X2CNT     : 0)
                | (init->ati                    ?   TIMER_CTRL_ATI       : 0)
#endif
                | (init->sync                   ?   TIMER_CTRL_SYNC      : 0);

  /* Start timer if specified to be enabled (dosn't hurt if already started) */
  if (init->enable)
  {
    timer->CMD = TIMER_CMD_START;
  }
}


/***************************************************************************//**
 * @brief
 *   Initialize TIMER compare/capture channel.
 *
 * @details
 *   Notice that if operating channel in compare mode, the CCV and CCVB register
 *   must be set separately as required.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] ch
 *   Compare/capture channel to init for.
 *
 * @param[in] init
 *   Pointer to TIMER initialization structure.
 ******************************************************************************/
void TIMER_InitCC(TIMER_TypeDef *timer,
                  unsigned int ch,
                  const TIMER_InitCC_TypeDef *init)
{
  EFM_ASSERT(TIMER_REF_VALID(timer));
  EFM_ASSERT(TIMER_CH_VALID(ch));

  timer->CC[ch].CTRL =
    ((uint32_t)(init->eventCtrl) << _TIMER_CC_CTRL_ICEVCTRL_SHIFT)
    | ((uint32_t)(init->edge)    << _TIMER_CC_CTRL_ICEDGE_SHIFT)
    | ((uint32_t)(init->prsSel)  << _TIMER_CC_CTRL_PRSSEL_SHIFT)
    | ((uint32_t)(init->cufoa)   << _TIMER_CC_CTRL_CUFOA_SHIFT)
    | ((uint32_t)(init->cofoa)   << _TIMER_CC_CTRL_COFOA_SHIFT)
    | ((uint32_t)(init->cmoa)    << _TIMER_CC_CTRL_CMOA_SHIFT)
    | ((uint32_t)(init->mode)    << _TIMER_CC_CTRL_MODE_SHIFT)
    | (init->filter              ?   TIMER_CC_CTRL_FILT_ENABLE : 0)
    | (init->prsInput            ?   TIMER_CC_CTRL_INSEL_PRS   : 0)
    | (init->coist               ?   TIMER_CC_CTRL_COIST       : 0)
    | (init->outInvert           ?   TIMER_CC_CTRL_OUTINV      : 0);
}


#if defined(_TIMER_DTCTRL_MASK)
/***************************************************************************//**
 * @brief
 *   Initialize the TIMER DTI unit.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 *
 * @param[in] init
 *   Pointer to TIMER DTI initialization structure.
 ******************************************************************************/
void TIMER_InitDTI(TIMER_TypeDef *timer, const TIMER_InitDTI_TypeDef *init)
{
  EFM_ASSERT(TIMER0 == timer);

  /* Make sure the DTI unit is disabled while initializing. */
  TIMER_EnableDTI (timer, false);

  /* Setup the DTCTRL register.
     The enable bit will be set at the end of the function if specified. */
  timer->DTCTRL =
    (init->autoRestart              ?   TIMER_DTCTRL_DTDAS   : 0)
    | (init->activeLowOut           ?   TIMER_DTCTRL_DTIPOL  : 0)
    | (init->invertComplementaryOut ?   TIMER_DTCTRL_DTCINV  : 0)
    | (init->enablePrsSource        ?   TIMER_DTCTRL_DTPRSEN : 0)
    | ((uint32_t)(init->prsSel)     << _TIMER_DTCTRL_DTPRSSEL_SHIFT);

  /* Setup the DTTIME register. */
  timer->DTTIME =
    ((uint32_t)(init->prescale)   << _TIMER_DTTIME_DTPRESC_SHIFT)
    | ((uint32_t)(init->riseTime) << _TIMER_DTTIME_DTRISET_SHIFT)
    | ((uint32_t)(init->fallTime) << _TIMER_DTTIME_DTFALLT_SHIFT);

  /* Setup the DTFC register. */
  timer->DTFC =
    (init->enableFaultSourceCoreLockup      ?   TIMER_DTFC_DTLOCKUPFEN : 0)
    | (init->enableFaultSourceDebugger      ?   TIMER_DTFC_DTDBGFEN    : 0)
    | (init->enableFaultSourcePrsSel0       ?   TIMER_DTFC_DTPRS0FEN   : 0)
    | (init->enableFaultSourcePrsSel1       ?   TIMER_DTFC_DTPRS1FEN   : 0)
    | ((uint32_t)(init->faultAction)        << _TIMER_DTFC_DTFA_SHIFT)
    | ((uint32_t)(init->faultSourcePrsSel0) << _TIMER_DTFC_DTPRS0FSEL_SHIFT)
    | ((uint32_t)(init->faultSourcePrsSel1) << _TIMER_DTFC_DTPRS1FSEL_SHIFT);

  /* Setup the DTOGEN register. */
  timer->DTOGEN = init->outputsEnableMask;

  /* Clear any previous DTI faults.  */
  TIMER_ClearDTIFault(timer, TIMER_GetDTIFault(timer));

  /* Enable/disable before returning. */
  TIMER_EnableDTI (timer, init->enable);
}
#endif


/***************************************************************************//**
 * @brief
 *   Reset TIMER to same state as after a HW reset.
 *
 * @note
 *   The ROUTE register is NOT reset by this function, in order to allow for
 *   centralized setup of this feature.
 *
 * @param[in] timer
 *   Pointer to TIMER peripheral register block.
 ******************************************************************************/
void TIMER_Reset(TIMER_TypeDef *timer)
{
  int i;

  EFM_ASSERT(TIMER_REF_VALID(timer));

  /* Make sure disabled first, before resetting other registers */
  timer->CMD = TIMER_CMD_STOP;

  timer->CTRL = _TIMER_CTRL_RESETVALUE;
  timer->IEN  = _TIMER_IEN_RESETVALUE;
  timer->IFC  = _TIMER_IFC_MASK;
  timer->TOPB = _TIMER_TOPB_RESETVALUE;
  /* Write TOP after TOPB to invalidate TOPB (clear TIMER_STATUS_TOPBV) */
  timer->TOP  = _TIMER_TOP_RESETVALUE;
  timer->CNT  = _TIMER_CNT_RESETVALUE;
  /* Do not reset route register, setting should be done independently */
  /* (Note: ROUTE register may be locked by DTLOCK register.) */

  for (i = 0; TIMER_CH_VALID(i); i++)
  {
    timer->CC[i].CTRL = _TIMER_CC_CTRL_RESETVALUE;
    timer->CC[i].CCV  = _TIMER_CC_CCV_RESETVALUE;
    timer->CC[i].CCVB = _TIMER_CC_CCVB_RESETVALUE;
  }

  /* Reset dead time insertion module, no effect on timers without DTI */

#if defined(TIMER_DTLOCK_LOCKKEY_UNLOCK)
  /* Unlock DTI registers first in case locked */
  timer->DTLOCK = TIMER_DTLOCK_LOCKKEY_UNLOCK;

  timer->DTCTRL   = _TIMER_DTCTRL_RESETVALUE;
  timer->DTTIME   = _TIMER_DTTIME_RESETVALUE;
  timer->DTFC     = _TIMER_DTFC_RESETVALUE;
  timer->DTOGEN   = _TIMER_DTOGEN_RESETVALUE;
  timer->DTFAULTC = _TIMER_DTFAULTC_MASK;
#endif
}


/** @} (end addtogroup TIMER) */
/** @} (end addtogroup emlib) */
#endif /* defined(TIMER_COUNT) && (TIMER_COUNT > 0) */
