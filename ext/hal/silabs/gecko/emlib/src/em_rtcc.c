/***************************************************************************//**
 * @file
 * @brief Real Time Counter with Calendar (RTCC) Peripheral API
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

#include "em_rtcc.h"
#if defined(RTCC_COUNT) && (RTCC_COUNT == 1)
#include "em_bus.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup RTCC
 * @brief Real Time Counter (RTCC) Peripheral API
 * @details
 *  This module contains functions to control the RTCC peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs. The RTCC ensures timekeeping in low energy modes.
 *  The RTCC also includes a BCD calendar mode for easy time and date keeping.
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Configure the selected capture/compare channel of the RTCC.
 *
 * @details
 *   Use this function to configure an RTCC channel.
 *   Select capture/compare mode, match output action, overflow output action,
 *   and PRS input configuration.
 *   See the configuration structure @ref RTCC_CCChConf_TypeDef for more
 *   details.
 *
 * @param[in] ch
 *   A channel selector.
 *
 * @param[in] confPtr
 *   A pointer to the configuration structure.
 ******************************************************************************/
void RTCC_ChannelInit(int ch, RTCC_CCChConf_TypeDef const *confPtr)
{
  EFM_ASSERT(RTCC_CH_VALID(ch));

#if defined(_SILICON_LABS_32B_SERIES_1)
  EFM_ASSERT((uint32_t)confPtr->compMask
             < (_RTCC_CC_CTRL_COMPMASK_MASK >> _RTCC_CC_CTRL_COMPMASK_SHIFT)
             + 1U);

  /** Configure the selected capture/compare channel. */
  RTCC->CC[ch].CTRL = ((uint32_t)confPtr->chMode << _RTCC_CC_CTRL_MODE_SHIFT)
                      | ((uint32_t)confPtr->compMatchOutAction << _RTCC_CC_CTRL_CMOA_SHIFT)
                      | ((uint32_t)confPtr->prsSel << _RTCC_CC_CTRL_PRSSEL_SHIFT)
                      | ((uint32_t)confPtr->inputEdgeSel << _RTCC_CC_CTRL_ICEDGE_SHIFT)
                      | ((uint32_t)confPtr->compBase << _RTCC_CC_CTRL_COMPBASE_SHIFT)
                      | ((uint32_t)confPtr->compMask << _RTCC_CC_CTRL_COMPMASK_SHIFT)
                      | ((uint32_t)confPtr->dayCompMode << _RTCC_CC_CTRL_DAYCC_SHIFT);
#else

  /** Configure the selected capture/compare channel. */
  RTCC->CC[ch].CTRL = ( (uint32_t)confPtr->chMode << _RTCC_CC_CTRL_MODE_SHIFT)
                      | ( (uint32_t)confPtr->compMatchOutAction << _RTCC_CC_CTRL_CMOA_SHIFT)
                      | ( (uint32_t)confPtr->inputEdgeSel << _RTCC_CC_CTRL_ICEDGE_SHIFT)
                      | ( (uint32_t)confPtr->compBase << _RTCC_CC_CTRL_COMPBASE_SHIFT);
  if (confPtr->chMode == rtccCapComChModeCapture) {
    volatile uint32_t *reg = &PRS->CONSUMER_RTCC_CC0;
    reg[ch] = confPtr->prsSel;
  }
#endif
}

/***************************************************************************//**
 * @brief
 *   Enable/disable RTCC counting.
 *
 * @param[in] enable
 *   True to enable RTCC counting, false to disable counting.
 ******************************************************************************/
void RTCC_Enable(bool enable)
{
#if defined (_RTCC_CTRL_ENABLE_SHIFT)
  /* Bitbanding the enable bit in the CTRL register (atomic). */
  BUS_RegBitWrite((&RTCC->CTRL), _RTCC_CTRL_ENABLE_SHIFT, (uint32_t)enable);
#elif defined (RTCC_CMD_START)

  /* Quick exit if we want to disable RTCC and it's already disabled. */
  if ((RTCC->EN == 0U) && !enable) {
    return;
  }

  if (RTCC->EN != 0U) {
    /* Modifying the enable bit while synchronization is active will BusFault */
    RTCC_SyncWait();
  }

  if (enable) {
    RTCC->EN_SET = RTCC_EN_EN;
    RTCC_Start();
  } else {
    RTCC_Stop();
    RTCC_SyncWait();
    RTCC->EN_CLR = RTCC_EN_EN;
  }
#endif
}

/***************************************************************************//**
 * @brief
 *   Initialize RTCC.
 *
 * @details
 *   Note that the compare values must be set separately with RTCC_CompareSet(),
 *   which should probably be done prior to the use of this function if
 *   configuring the RTCC to start when initialization is completed.
 *
 * @param[in] init
 *   A pointer to the RTCC initialization structure.
 ******************************************************************************/
void RTCC_Init(const RTCC_Init_TypeDef *init)
{
#if defined (_RTCC_CTRL_MASK)
  RTCC->CTRL = ((init->enable ? 1UL : 0UL) << _RTCC_CTRL_ENABLE_SHIFT)
               | ((init->debugRun ? 1UL : 0UL) << _RTCC_CTRL_DEBUGRUN_SHIFT)
               | ((init->precntWrapOnCCV0 ? 1UL : 0UL) << _RTCC_CTRL_PRECCV0TOP_SHIFT)
               | ((init->cntWrapOnCCV1 ? 1UL : 0UL) << _RTCC_CTRL_CCV1TOP_SHIFT)
               | ((uint32_t)init->presc << _RTCC_CTRL_CNTPRESC_SHIFT)
               | ((uint32_t)init->prescMode << _RTCC_CTRL_CNTTICK_SHIFT)
#if defined(_RTCC_CTRL_BUMODETSEN_MASK)
               | ((uint32_t)init->enaBackupModeSet << _RTCC_CTRL_BUMODETSEN_SHIFT)
#endif
               | ((init->enaOSCFailDetect ? 1UL : 0UL) << _RTCC_CTRL_OSCFDETEN_SHIFT)
               | ((uint32_t)init->cntMode << _RTCC_CTRL_CNTMODE_SHIFT)
               | ((init->disLeapYearCorr ? 1UL : 0UL) << _RTCC_CTRL_LYEARCORRDIS_SHIFT);

#elif defined (_RTCC_CFG_MASK)
  if (RTCC->EN != 0U) {
    RTCC_SyncWait();
  }
  RTCC->EN_CLR = RTCC_EN_EN;
  RTCC->CFG = ((init->debugRun ? 1UL : 0UL) << _RTCC_CFG_DEBUGRUN_SHIFT)
              | ( (init->precntWrapOnCCV0 ? 1UL : 0UL) << _RTCC_CFG_PRECNTCCV0TOP_SHIFT)
              | ( (init->cntWrapOnCCV1 ? 1UL : 0UL) << _RTCC_CFG_CNTCCV1TOP_SHIFT)
              | ( (uint32_t)init->presc << _RTCC_CFG_CNTPRESC_SHIFT)
              | ( (uint32_t)init->prescMode << _RTCC_CFG_CNTTICK_SHIFT);
  RTCC->EN_SET = RTCC_EN_EN;
  RTCC->CMD = init->enable ? RTCC_CMD_START : RTCC_CMD_STOP;
#endif
}

/***************************************************************************//**
 * @brief
 *   Restore RTCC to its reset state.
 ******************************************************************************/
void RTCC_Reset(void)
{
  unsigned int i;

#if defined(_RTCC_CTRL_MASK)
  /* Restore all RTCC registers to their default values. */
  RTCC_Unlock();
  RTCC->CTRL    = _RTCC_CTRL_RESETVALUE;
  RTCC->PRECNT  = _RTCC_PRECNT_RESETVALUE;
  RTCC->CNT     = _RTCC_CNT_RESETVALUE;
  RTCC->TIME    = _RTCC_TIME_RESETVALUE;
  RTCC->DATE    = _RTCC_DATE_RESETVALUE;
  RTCC->IEN     = _RTCC_IEN_RESETVALUE;
  RTCC->IFC     = _RTCC_IFC_MASK;
  RTCC_StatusClear();
  RTCC->EM4WUEN = _RTCC_EM4WUEN_RESETVALUE;

  for (i = 0; i < RTCC_CC_NUM; i++) {
    RTCC->CC[i].CTRL = _RTCC_CC_CTRL_RESETVALUE;
    RTCC->CC[i].CCV  = _RTCC_CC_CCV_RESETVALUE;
    RTCC->CC[i].TIME = _RTCC_CC_TIME_RESETVALUE;
    RTCC->CC[i].DATE = _RTCC_CC_DATE_RESETVALUE;
  }

#elif defined(_RTCC_CFG_MASK)

  /* Restore all RTCC registers to their default values. */
  RTCC_Unlock();
  RTCC->EN_SET = RTCC_EN_EN;
  RTCC_Stop();
  RTCC_SyncWait();
  RTCC->PRECNT  = _RTCC_PRECNT_RESETVALUE;
  RTCC->CNT     = _RTCC_CNT_RESETVALUE;
  RTCC->IEN     = _RTCC_IEN_RESETVALUE;
  RTCC_IntClear(_RTCC_IF_MASK);
  RTCC_StatusClear();

  for (i = 0; i < RTCC_CC_NUM; i++) {
    RTCC->CC[i].CTRL = _RTCC_CC_CTRL_RESETVALUE;
    RTCC->CC[i].OCVALUE  = _RTCC_CC_OCVALUE_RESETVALUE;
  }
  RTCC_SyncWait();
  RTCC->EN_CLR = RTCC_EN_EN;
  RTCC->CFG    = _RTCC_CFG_RESETVALUE;
#endif
}

/***************************************************************************//**
 * @brief
 *   Clear the STATUS register.
 ******************************************************************************/
void RTCC_StatusClear(void)
{
#if defined (RTCC_CMD_CLRSTATUS)
  while ((RTCC->SYNCBUSY & RTCC_SYNCBUSY_CMD) != 0U) {
    // Wait for synchronization.
  }
  RTCC->CMD = RTCC_CMD_CLRSTATUS;
#endif
}

/** @} (end addtogroup RTCC) */
/** @} (end addtogroup emlib) */

#endif /* defined( RTCC_COUNT ) && ( RTCC_COUNT == 1 ) */
