/***************************************************************************//**
* \file cyip_tcpwm.h
*
* \brief
* TCPWM IP definitions
*
* \note
* Generator version: 1.3.0.1146
* Database revision: rev#1050929
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef _CYIP_TCPWM_H_
#define _CYIP_TCPWM_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                    TCPWM
*******************************************************************************/

#define TCPWM_CNT_SECTION_SIZE                  0x00000040UL
#define TCPWM_SECTION_SIZE                      0x00010000UL

/**
  * \brief Timer/Counter/PWM Counter Module (TCPWM_CNT)
  */
typedef struct {
  __IOM uint32_t CTRL;                          /*!< 0x00000000 Counter control register */
   __IM uint32_t STATUS;                        /*!< 0x00000004 Counter status register */
  __IOM uint32_t COUNTER;                       /*!< 0x00000008 Counter count register */
  __IOM uint32_t CC;                            /*!< 0x0000000C Counter compare/capture register */
  __IOM uint32_t CC_BUFF;                       /*!< 0x00000010 Counter buffered compare/capture register */
  __IOM uint32_t PERIOD;                        /*!< 0x00000014 Counter period register */
  __IOM uint32_t PERIOD_BUFF;                   /*!< 0x00000018 Counter buffered period register */
   __IM uint32_t RESERVED;
  __IOM uint32_t TR_CTRL0;                      /*!< 0x00000020 Counter trigger control register 0 */
  __IOM uint32_t TR_CTRL1;                      /*!< 0x00000024 Counter trigger control register 1 */
  __IOM uint32_t TR_CTRL2;                      /*!< 0x00000028 Counter trigger control register 2 */
   __IM uint32_t RESERVED1;
  __IOM uint32_t INTR;                          /*!< 0x00000030 Interrupt request register */
  __IOM uint32_t INTR_SET;                      /*!< 0x00000034 Interrupt set request register */
  __IOM uint32_t INTR_MASK;                     /*!< 0x00000038 Interrupt mask register */
   __IM uint32_t INTR_MASKED;                   /*!< 0x0000003C Interrupt masked request register */
} TCPWM_CNT_V1_Type;                            /*!< Size = 64 (0x40) */

/**
  * \brief Timer/Counter/PWM (TCPWM)
  */
typedef struct {
  __IOM uint32_t CTRL;                          /*!< 0x00000000 TCPWM control register */
  __IOM uint32_t CTRL_CLR;                      /*!< 0x00000004 TCPWM control clear register */
  __IOM uint32_t CTRL_SET;                      /*!< 0x00000008 TCPWM control set register */
  __IOM uint32_t CMD_CAPTURE;                   /*!< 0x0000000C TCPWM capture command register */
  __IOM uint32_t CMD_RELOAD;                    /*!< 0x00000010 TCPWM reload command register */
  __IOM uint32_t CMD_STOP;                      /*!< 0x00000014 TCPWM stop command register */
  __IOM uint32_t CMD_START;                     /*!< 0x00000018 TCPWM start command register */
   __IM uint32_t INTR_CAUSE;                    /*!< 0x0000001C TCPWM Counter interrupt cause register */
   __IM uint32_t RESERVED[56];
        TCPWM_CNT_V1_Type CNT[32];              /*!< 0x00000100 Timer/Counter/PWM Counter Module */
} TCPWM_V1_Type;                                /*!< Size = 2304 (0x900) */


/* TCPWM_CNT.CTRL */
#define TCPWM_CNT_CTRL_AUTO_RELOAD_CC_Pos       0UL
#define TCPWM_CNT_CTRL_AUTO_RELOAD_CC_Msk       0x1UL
#define TCPWM_CNT_CTRL_AUTO_RELOAD_PERIOD_Pos   1UL
#define TCPWM_CNT_CTRL_AUTO_RELOAD_PERIOD_Msk   0x2UL
#define TCPWM_CNT_CTRL_PWM_SYNC_KILL_Pos        2UL
#define TCPWM_CNT_CTRL_PWM_SYNC_KILL_Msk        0x4UL
#define TCPWM_CNT_CTRL_PWM_STOP_ON_KILL_Pos     3UL
#define TCPWM_CNT_CTRL_PWM_STOP_ON_KILL_Msk     0x8UL
#define TCPWM_CNT_CTRL_GENERIC_Pos              8UL
#define TCPWM_CNT_CTRL_GENERIC_Msk              0xFF00UL
#define TCPWM_CNT_CTRL_UP_DOWN_MODE_Pos         16UL
#define TCPWM_CNT_CTRL_UP_DOWN_MODE_Msk         0x30000UL
#define TCPWM_CNT_CTRL_ONE_SHOT_Pos             18UL
#define TCPWM_CNT_CTRL_ONE_SHOT_Msk             0x40000UL
#define TCPWM_CNT_CTRL_QUADRATURE_MODE_Pos      20UL
#define TCPWM_CNT_CTRL_QUADRATURE_MODE_Msk      0x300000UL
#define TCPWM_CNT_CTRL_MODE_Pos                 24UL
#define TCPWM_CNT_CTRL_MODE_Msk                 0x7000000UL
/* TCPWM_CNT.STATUS */
#define TCPWM_CNT_STATUS_DOWN_Pos               0UL
#define TCPWM_CNT_STATUS_DOWN_Msk               0x1UL
#define TCPWM_CNT_STATUS_GENERIC_Pos            8UL
#define TCPWM_CNT_STATUS_GENERIC_Msk            0xFF00UL
#define TCPWM_CNT_STATUS_RUNNING_Pos            31UL
#define TCPWM_CNT_STATUS_RUNNING_Msk            0x80000000UL
/* TCPWM_CNT.COUNTER */
#define TCPWM_CNT_COUNTER_COUNTER_Pos           0UL
#define TCPWM_CNT_COUNTER_COUNTER_Msk           0xFFFFFFFFUL
/* TCPWM_CNT.CC */
#define TCPWM_CNT_CC_CC_Pos                     0UL
#define TCPWM_CNT_CC_CC_Msk                     0xFFFFFFFFUL
/* TCPWM_CNT.CC_BUFF */
#define TCPWM_CNT_CC_BUFF_CC_Pos                0UL
#define TCPWM_CNT_CC_BUFF_CC_Msk                0xFFFFFFFFUL
/* TCPWM_CNT.PERIOD */
#define TCPWM_CNT_PERIOD_PERIOD_Pos             0UL
#define TCPWM_CNT_PERIOD_PERIOD_Msk             0xFFFFFFFFUL
/* TCPWM_CNT.PERIOD_BUFF */
#define TCPWM_CNT_PERIOD_BUFF_PERIOD_Pos        0UL
#define TCPWM_CNT_PERIOD_BUFF_PERIOD_Msk        0xFFFFFFFFUL
/* TCPWM_CNT.TR_CTRL0 */
#define TCPWM_CNT_TR_CTRL0_CAPTURE_SEL_Pos      0UL
#define TCPWM_CNT_TR_CTRL0_CAPTURE_SEL_Msk      0xFUL
#define TCPWM_CNT_TR_CTRL0_COUNT_SEL_Pos        4UL
#define TCPWM_CNT_TR_CTRL0_COUNT_SEL_Msk        0xF0UL
#define TCPWM_CNT_TR_CTRL0_RELOAD_SEL_Pos       8UL
#define TCPWM_CNT_TR_CTRL0_RELOAD_SEL_Msk       0xF00UL
#define TCPWM_CNT_TR_CTRL0_STOP_SEL_Pos         12UL
#define TCPWM_CNT_TR_CTRL0_STOP_SEL_Msk         0xF000UL
#define TCPWM_CNT_TR_CTRL0_START_SEL_Pos        16UL
#define TCPWM_CNT_TR_CTRL0_START_SEL_Msk        0xF0000UL
/* TCPWM_CNT.TR_CTRL1 */
#define TCPWM_CNT_TR_CTRL1_CAPTURE_EDGE_Pos     0UL
#define TCPWM_CNT_TR_CTRL1_CAPTURE_EDGE_Msk     0x3UL
#define TCPWM_CNT_TR_CTRL1_COUNT_EDGE_Pos       2UL
#define TCPWM_CNT_TR_CTRL1_COUNT_EDGE_Msk       0xCUL
#define TCPWM_CNT_TR_CTRL1_RELOAD_EDGE_Pos      4UL
#define TCPWM_CNT_TR_CTRL1_RELOAD_EDGE_Msk      0x30UL
#define TCPWM_CNT_TR_CTRL1_STOP_EDGE_Pos        6UL
#define TCPWM_CNT_TR_CTRL1_STOP_EDGE_Msk        0xC0UL
#define TCPWM_CNT_TR_CTRL1_START_EDGE_Pos       8UL
#define TCPWM_CNT_TR_CTRL1_START_EDGE_Msk       0x300UL
/* TCPWM_CNT.TR_CTRL2 */
#define TCPWM_CNT_TR_CTRL2_CC_MATCH_MODE_Pos    0UL
#define TCPWM_CNT_TR_CTRL2_CC_MATCH_MODE_Msk    0x3UL
#define TCPWM_CNT_TR_CTRL2_OVERFLOW_MODE_Pos    2UL
#define TCPWM_CNT_TR_CTRL2_OVERFLOW_MODE_Msk    0xCUL
#define TCPWM_CNT_TR_CTRL2_UNDERFLOW_MODE_Pos   4UL
#define TCPWM_CNT_TR_CTRL2_UNDERFLOW_MODE_Msk   0x30UL
/* TCPWM_CNT.INTR */
#define TCPWM_CNT_INTR_TC_Pos                   0UL
#define TCPWM_CNT_INTR_TC_Msk                   0x1UL
#define TCPWM_CNT_INTR_CC_MATCH_Pos             1UL
#define TCPWM_CNT_INTR_CC_MATCH_Msk             0x2UL
/* TCPWM_CNT.INTR_SET */
#define TCPWM_CNT_INTR_SET_TC_Pos               0UL
#define TCPWM_CNT_INTR_SET_TC_Msk               0x1UL
#define TCPWM_CNT_INTR_SET_CC_MATCH_Pos         1UL
#define TCPWM_CNT_INTR_SET_CC_MATCH_Msk         0x2UL
/* TCPWM_CNT.INTR_MASK */
#define TCPWM_CNT_INTR_MASK_TC_Pos              0UL
#define TCPWM_CNT_INTR_MASK_TC_Msk              0x1UL
#define TCPWM_CNT_INTR_MASK_CC_MATCH_Pos        1UL
#define TCPWM_CNT_INTR_MASK_CC_MATCH_Msk        0x2UL
/* TCPWM_CNT.INTR_MASKED */
#define TCPWM_CNT_INTR_MASKED_TC_Pos            0UL
#define TCPWM_CNT_INTR_MASKED_TC_Msk            0x1UL
#define TCPWM_CNT_INTR_MASKED_CC_MATCH_Pos      1UL
#define TCPWM_CNT_INTR_MASKED_CC_MATCH_Msk      0x2UL


/* TCPWM.CTRL */
#define TCPWM_CTRL_COUNTER_ENABLED_Pos          0UL
#define TCPWM_CTRL_COUNTER_ENABLED_Msk          0xFFFFFFFFUL
/* TCPWM.CTRL_CLR */
#define TCPWM_CTRL_CLR_COUNTER_ENABLED_Pos      0UL
#define TCPWM_CTRL_CLR_COUNTER_ENABLED_Msk      0xFFFFFFFFUL
/* TCPWM.CTRL_SET */
#define TCPWM_CTRL_SET_COUNTER_ENABLED_Pos      0UL
#define TCPWM_CTRL_SET_COUNTER_ENABLED_Msk      0xFFFFFFFFUL
/* TCPWM.CMD_CAPTURE */
#define TCPWM_CMD_CAPTURE_COUNTER_CAPTURE_Pos   0UL
#define TCPWM_CMD_CAPTURE_COUNTER_CAPTURE_Msk   0xFFFFFFFFUL
/* TCPWM.CMD_RELOAD */
#define TCPWM_CMD_RELOAD_COUNTER_RELOAD_Pos     0UL
#define TCPWM_CMD_RELOAD_COUNTER_RELOAD_Msk     0xFFFFFFFFUL
/* TCPWM.CMD_STOP */
#define TCPWM_CMD_STOP_COUNTER_STOP_Pos         0UL
#define TCPWM_CMD_STOP_COUNTER_STOP_Msk         0xFFFFFFFFUL
/* TCPWM.CMD_START */
#define TCPWM_CMD_START_COUNTER_START_Pos       0UL
#define TCPWM_CMD_START_COUNTER_START_Msk       0xFFFFFFFFUL
/* TCPWM.INTR_CAUSE */
#define TCPWM_INTR_CAUSE_COUNTER_INT_Pos        0UL
#define TCPWM_INTR_CAUSE_COUNTER_INT_Msk        0xFFFFFFFFUL


#endif /* _CYIP_TCPWM_H_ */


/* [] END OF FILE */
