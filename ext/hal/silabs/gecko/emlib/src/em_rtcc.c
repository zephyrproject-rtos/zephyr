/***************************************************************************//**
 * @file
 * @brief Real Time Counter with Calendar (RTCC) Peripheral API
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

#include "em_rtcc.h"
#if defined( RTCC_COUNT ) && ( RTCC_COUNT == 1 )
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
 *   Use this function to configure a RTCC channel.
 *   Select capture/compare mode, match output action, overflow output action
 *   and PRS input configuration.
 *   Refer to the configuration structure @ref RTCC_CCChConf_TypeDef for more
 *   details.
 *
 * @param[in] ch
 *   Channel selector.
 *
 * @param[in] confPtr
 *   Pointer to configuration structure.
 ******************************************************************************/
void RTCC_ChannelInit( int ch, RTCC_CCChConf_TypeDef const *confPtr )
{
  EFM_ASSERT( RTCC_CH_VALID( ch ) );
  EFM_ASSERT( (uint32_t)confPtr->compMask
              < ( _RTCC_CC_CTRL_COMPMASK_MASK >> _RTCC_CC_CTRL_COMPMASK_SHIFT )
              + 1 );

  /** Configure the selected capture/compare channel. */
  RTCC->CC[ch].CTRL = ( (uint32_t)confPtr->chMode << _RTCC_CC_CTRL_MODE_SHIFT )
                      | ( (uint32_t)confPtr->compMatchOutAction << _RTCC_CC_CTRL_CMOA_SHIFT )
                      | ( (uint32_t)confPtr->prsSel << _RTCC_CC_CTRL_PRSSEL_SHIFT )
                      | ( (uint32_t)confPtr->inputEdgeSel << _RTCC_CC_CTRL_ICEDGE_SHIFT )
                      | ( (uint32_t)confPtr->compBase << _RTCC_CC_CTRL_COMPBASE_SHIFT )
                      | ( (uint32_t)confPtr->compMask << _RTCC_CC_CTRL_COMPMASK_SHIFT )
                      | ( (uint32_t)confPtr->dayCompMode << _RTCC_CC_CTRL_DAYCC_SHIFT );
}

/***************************************************************************//**
 * @brief
 *   Enable/disable RTCC.
 *
 * @param[in] enable
 *   True to enable RTCC, false to disable.
 ******************************************************************************/
void RTCC_Enable( bool enable )
{
  /* Bitbanding the enable bit in the CTRL register (atomic). */
  BUS_RegBitWrite((&RTCC->CTRL), _RTCC_CTRL_ENABLE_SHIFT, enable);
}

/***************************************************************************//**
 * @brief
 *   Initialize RTCC.
 *
 * @details
 *   Note that the compare values must be set separately with RTCC_CompareSet().
 *   That should probably be done prior to the use of this function if
 *   configuring the RTCC to start when initialization is completed.
 *
 * @param[in] init
 *   Pointer to RTCC initialization structure.
 ******************************************************************************/
void RTCC_Init( const RTCC_Init_TypeDef *init )
{
  RTCC->CTRL = ( (uint32_t)init->enable << _RTCC_CTRL_ENABLE_SHIFT )
               | ( (uint32_t)init->debugRun << _RTCC_CTRL_DEBUGRUN_SHIFT )
               | ( (uint32_t)init->precntWrapOnCCV0 << _RTCC_CTRL_PRECCV0TOP_SHIFT )
               | ( (uint32_t)init->cntWrapOnCCV1 << _RTCC_CTRL_CCV1TOP_SHIFT )
               | ( (uint32_t)init->presc << _RTCC_CTRL_CNTPRESC_SHIFT )
               | ( (uint32_t)init->prescMode << _RTCC_CTRL_CNTTICK_SHIFT )
#if defined(_RTCC_CTRL_BUMODETSEN_MASK)
               | ( (uint32_t)init->enaBackupModeSet << _RTCC_CTRL_BUMODETSEN_SHIFT )
#endif
               | ( (uint32_t)init->enaOSCFailDetect << _RTCC_CTRL_OSCFDETEN_SHIFT )
               | ( (uint32_t)init->cntMode << _RTCC_CTRL_CNTMODE_SHIFT )
               | ( (uint32_t)init->disLeapYearCorr << _RTCC_CTRL_LYEARCORRDIS_SHIFT );
}

/***************************************************************************//**
 * @brief
 *   Restore RTCC to its reset state.
 ******************************************************************************/
void RTCC_Reset( void )
{
  int i;

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

  for (i = 0; i < 3; i++)
  {
    RTCC->CC[i].CTRL = _RTCC_CC_CTRL_RESETVALUE;
    RTCC->CC[i].CCV  = _RTCC_CC_CCV_RESETVALUE;
    RTCC->CC[i].TIME = _RTCC_CC_TIME_RESETVALUE;
    RTCC->CC[i].DATE = _RTCC_CC_DATE_RESETVALUE;
  }
}

/***************************************************************************//**
 * @brief
 *   Clear STATUS register.
 ******************************************************************************/
void RTCC_StatusClear( void )
{
  while ( RTCC->SYNCBUSY & RTCC_SYNCBUSY_CMD )
  {
    // Wait for syncronization.
  }
  RTCC->CMD = RTCC_CMD_CLRSTATUS;
}

/** @} (end addtogroup RTCC) */
/** @} (end addtogroup emlib) */

#endif /* defined( RTCC_COUNT ) && ( RTCC_COUNT == 1 ) */
