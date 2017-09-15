/**
  ******************************************************************************
  * @file    stm32f3xx_ll_hrtim.h
  * @author  MCD Application Team
  * @brief   Header file of HRTIM LL module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F3xx_LL_HRTIM_H
#define __STM32F3xx_LL_HRTIM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx.h"

/** @addtogroup STM32F3xx_LL_Driver
  * @{
  */

#if defined (HRTIM1)

/** @defgroup HRTIM_LL HRTIM
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup HRTIM_LL_Private_Variables HRTIM Private Variables
  * @{
  */
static const uint16_t REG_OFFSET_TAB_TIMER[] =
{
  0x00U,   /* 0: MASTER  */
  0x80U,   /* 1: TIMER A */
  0x100U,  /* 2: TIMER B  */
  0x180U,  /* 3: TIMER C */
  0x200U,  /* 4: TIMER D  */
  0x280U,  /* 5: TIMER E */
};

static const uint8_t REG_OFFSET_TAB_ADCxR[] =
{
  0x00U,   /* 0: HRTIM_ADC1R */
  0x04U,   /* 1: HRTIM_ADC2R */
  0x08U,   /* 2: HRTIM_ADC3R */
  0x0CU,   /* 3: HRTIM_ADC4R */
};

static const uint16_t REG_OFFSET_TAB_SETxR[] =
{
  0x00U,   /* 0: TA1  */
  0x08U,   /* 1: TA2 */
  0x80U,   /* 2: TB1  */
  0x88U,   /* 3: TB2 */
  0x100U,  /* 4: TC1  */
  0x108U,  /* 5: TC2 */
  0x180U,  /* 6: TD1  */
  0x188U,  /* 7: TD2 */
  0x200U,  /* 8: TE1  */
  0x208U   /* 9: TE2 */
};

static const uint16_t REG_OFFSET_TAB_OUTxR[] =
{
  0x00U,   /* 0: TA1  */
  0x00U,   /* 1: TA2 */
  0x80U,   /* 2: TB1  */
  0x80U,   /* 3: TB2 */
  0x100U,  /* 4: TC1  */
  0x100U,  /* 5: TC2 */
  0x180U,  /* 6: TD1  */
  0x180U,  /* 7: TD2 */
  0x200U,  /* 8: TE1  */
  0x200U   /* 9: TE2 */
};


static const uint8_t REG_OFFSET_TAB_OUT_LEVEL[] =
{
  0x04U,   /* 0: LL_HRTIM_OUT_LEVEL_INACTIVE  */
  0x00U    /* 1: LL_HRTIM_OUT_LEVEL_ACTIVE  */
};

static const uint8_t REG_OFFSET_TAB_EECR[] =
{
  0x00U, /* LL_HRTIM_EVENT_1 */
  0x00U, /* LL_HRTIM_EVENT_2 */
  0x00U, /* LL_HRTIM_EVENT_3 */
  0x00U, /* LL_HRTIM_EVENT_4 */
  0x00U, /* LL_HRTIM_EVENT_5 */
  0x04U, /* LL_HRTIM_EVENT_6 */
  0x04U, /* LL_HRTIM_EVENT_7 */
  0x04U, /* LL_HRTIM_EVENT_8 */
  0x04U, /* LL_HRTIM_EVENT_9 */
  0x04U  /* LL_HRTIM_EVENT_10 */
};

static const uint8_t REG_OFFSET_TAB_FLTINR[] =
{
  0x00U, /* LL_HRTIM_FAULT_1 */
  0x00U, /* LL_HRTIM_FAULT_2 */
  0x00U, /* LL_HRTIM_FAULT_3 */
  0x00U, /* LL_HRTIM_FAULT_4 */
  0x04U  /* LL_HRTIM_FAULT_5 */
};

static const uint32_t REG_MASK_TAB_UPDATETRIG[] =
{
  0x20000000U,  /* 0: MASTER  */
  0x01FE0000U,  /* 1: TIMER A */
  0x01FE0000U,  /* 2: TIMER B */
  0x01FE0000U,  /* 3: TIMER C */
  0x01FE0000U,  /* 4: TIMER D */
  0x01FE0000U,  /* 5: TIMER E */
};

static const uint8_t REG_SHIFT_TAB_UPDATETRIG[] =
{
  12U, /* 0: MASTER  */
  0U,  /* 1: TIMER A */
  0U,  /* 2: TIMER B  */
  0U,  /* 3: TIMER C */
  0U,  /* 4: TIMER D  */
  0U,  /* 5: TIMER E */
};

static const uint8_t REG_SHIFT_TAB_EExSRC[] =
{
  0U,  /* LL_HRTIM_EVENT_1 */
  6U,  /* LL_HRTIM_EVENT_2 */
  12U, /* LL_HRTIM_EVENT_3 */
  18U, /* LL_HRTIM_EVENT_4 */
  24U, /* LL_HRTIM_EVENT_5 */
  0U,  /* LL_HRTIM_EVENT_6 */
  6U,  /* LL_HRTIM_EVENT_7 */
  12U, /* LL_HRTIM_EVENT_8 */
  18U, /* LL_HRTIM_EVENT_9 */
  24U  /* LL_HRTIM_EVENT_10 */
};

static const uint32_t REG_MASK_TAB_UPDATEGATING[] =
{
  HRTIM_MCR_BRSTDMA,   /* 0: MASTER  */
  HRTIM_TIMCR_UPDGAT,  /* 1: TIMER A */
  HRTIM_TIMCR_UPDGAT,  /* 2: TIMER B  */
  HRTIM_TIMCR_UPDGAT,  /* 3: TIMER C */
  HRTIM_TIMCR_UPDGAT,  /* 4: TIMER D  */
  HRTIM_TIMCR_UPDGAT,  /* 5: TIMER E */
};

static const uint8_t REG_SHIFT_TAB_UPDATEGATING[] =
{
  2U, /* 0: MASTER  */
  0U, /* 1: TIMER A */
  0U, /* 2: TIMER B  */
  0U, /* 3: TIMER C */
  0U, /* 4: TIMER D  */
  0U, /* 5: TIMER E */
};

static const uint8_t REG_SHIFT_TAB_OUTxR[] =
{
  0U,  /* 0: TA1  */
  16U, /* 1: TA2 */
  0U,  /* 2: TB1  */
  16U, /* 3: TB2 */
  0U,  /* 4: TC1  */
  16U, /* 5: TC2 */
  0U,  /* 6: TD1  */
  16U, /* 7: TD2 */
  0U,  /* 8: TE1  */
  16U  /* 9: TE2 */
};

static const uint8_t REG_SHIFT_TAB_OxSTAT[] =
{
  0U,  /* 0: TA1  */
  1U,  /* 1: TA2 */
  0U,  /* 2: TB1  */
  1U,  /* 3: TB2 */
  0U,  /* 4: TC1  */
  1U,  /* 5: TC2 */
  0U,  /* 6: TD1  */
  1U,  /* 7: TD2 */
  0U,  /* 8: TE1  */
  1U   /* 9: TE2 */
};

static const uint8_t REG_SHIFT_TAB_FLTxE[] =
{
  0U,  /* LL_HRTIM_FAULT_1 */
  8U,  /* LL_HRTIM_FAULT_2 */
  16U, /* LL_HRTIM_FAULT_3 */
  24U, /* LL_HRTIM_FAULT_4 */
  0U   /* LL_HRTIM_FAULT_5 */
};

/**
  * @}
  */


/* Private constants ---------------------------------------------------------*/
/** @defgroup HRTIM_LL_Private_Constants HRTIM Private Constants
  * @{
  */
#define HRTIM_CR1_UDIS_MASK   ((uint32_t)(HRTIM_CR1_MUDIS  |\
                                          HRTIM_CR1_TAUDIS |\
                                          HRTIM_CR1_TBUDIS |\
                                          HRTIM_CR1_TCUDIS |\
                                          HRTIM_CR1_TDUDIS |\
                                          HRTIM_CR1_TEUDIS))

#define HRTIM_CR2_SWUPD_MASK   ((uint32_t)(HRTIM_CR2_MSWU  |\
                                          HRTIM_CR2_TASWU |\
                                          HRTIM_CR2_TBSWU |\
                                          HRTIM_CR2_TCSWU |\
                                          HRTIM_CR2_TDSWU |\
                                          HRTIM_CR2_TESWU))

#define HRTIM_CR2_SWRST_MASK   ((uint32_t)(HRTIM_CR2_MRST  |\
                                          HRTIM_CR2_TARST |\
                                          HRTIM_CR2_TBRST |\
                                          HRTIM_CR2_TCRST |\
                                          HRTIM_CR2_TDRST |\
                                          HRTIM_CR2_TERST))

#define HRTIM_OENR_OEN_MASK   ((uint32_t)(HRTIM_OENR_TA1OEN |\
                                          HRTIM_OENR_TA2OEN |\
                                          HRTIM_OENR_TB1OEN |\
                                          HRTIM_OENR_TB2OEN |\
                                          HRTIM_OENR_TC1OEN |\
                                          HRTIM_OENR_TC2OEN |\
                                          HRTIM_OENR_TD1OEN |\
                                          HRTIM_OENR_TD2OEN |\
                                          HRTIM_OENR_TE1OEN |\
                                          HRTIM_OENR_TE2OEN))

#define HRTIM_OENR_ODIS_MASK  ((uint32_t)(HRTIM_ODISR_TA1ODIS  |\
                                          HRTIM_ODISR_TA2ODIS  |\
                                          HRTIM_ODISR_TB1ODIS  |\
                                          HRTIM_ODISR_TB2ODIS  |\
                                          HRTIM_ODISR_TC1ODIS  |\
                                          HRTIM_ODISR_TC2ODIS  |\
                                          HRTIM_ODISR_TD1ODIS  |\
                                          HRTIM_ODISR_TD2ODIS  |\
                                          HRTIM_ODISR_TE1ODIS  |\
                                          HRTIM_ODISR_TE2ODIS))

#define HRTIM_OUT_CONFIG_MASK  ((uint32_t)(HRTIM_OUTR_POL1   |\
                                           HRTIM_OUTR_IDLM1  |\
                                           HRTIM_OUTR_IDLES1 |\
                                           HRTIM_OUTR_FAULT1 |\
                                           HRTIM_OUTR_CHP1   |\
                                           HRTIM_OUTR_DIDL1))

#define HRTIM_EE_CONFIG_MASK   ((uint32_t)(HRTIM_EECR1_EE1SRC |\
                                           HRTIM_EECR1_EE1POL |\
                                           HRTIM_EECR1_EE1SNS |\
                                           HRTIM_EECR1_EE1FAST))

#define HRTIM_FLT_CONFIG_MASK   ((uint32_t)(HRTIM_FLTINR1_FLT1P |\
                                            HRTIM_FLTINR1_FLT1SRC))

#define HRTIM_BM_CONFIG_MASK   ((uint32_t)( HRTIM_BMCR_BMPRSC |\
                                            HRTIM_BMCR_BMCLK  |\
                                            HRTIM_BMCR_BMOM))

/**
  * @}
  */


/* Private macros ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup HRTIM_LL_ES_INIT HRTIM Exported Init structure
  * @{
  */
/* TO BE COMPLETED */
/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/* Exported constants --------------------------------------------------------*/
/** @defgroup HRTIM_LL_Exported_Constants HRTIM Exported Constants
  * @{
  */

/** @defgroup HRTIM_EC_GET_FLAG Get Flags Defines
  * @brief    Flags defines which can be used with LL_HRTIM_ReadReg function
  * @{
  */
#define LL_HRTIM_ISR_FLT1                  HRTIM_ISR_FLT1
#define LL_HRTIM_ISR_FLT2                  HRTIM_ISR_FLT2
#define LL_HRTIM_ISR_FLT3                  HRTIM_ISR_FLT3
#define LL_HRTIM_ISR_FLT4                  HRTIM_ISR_FLT4
#define LL_HRTIM_ISR_FLT5                  HRTIM_ISR_FLT5
#define LL_HRTIM_ISR_SYSFLT                HRTIM_ISR_SYSFLT
#define LL_HRTIM_ISR_DLLRDY                HRTIM_ISR_DLLRDY
#define LL_HRTIM_ISR_BMPER                 HRTIM_ISR_BMPER

#define LL_HRTIM_MISR_MCMP1                HRTIM_MISR_MCMP1
#define LL_HRTIM_MISR_MCMP2                HRTIM_MISR_MCMP2
#define LL_HRTIM_MISR_MCMP3                HRTIM_MISR_MCMP3
#define LL_HRTIM_MISR_MCMP4                HRTIM_MISR_MCMP4
#define LL_HRTIM_MISR_MREP                 HRTIM_MISR_MREP
#define LL_HRTIM_MISR_SYNC                 HRTIM_MISR_SYNC
#define LL_HRTIM_MISR_MUPD                 HRTIM_MISR_MUPD

#define LL_HRTIM_TIMISR_CMP1               HRTIM_TIMISR_CMP1
#define LL_HRTIM_TIMISR_CMP2               HRTIM_TIMISR_CMP2
#define LL_HRTIM_TIMISR_CMP3               HRTIM_TIMISR_CMP3
#define LL_HRTIM_TIMISR_CMP4               HRTIM_TIMISR_CMP4
#define LL_HRTIM_TIMISR_REP                HRTIM_TIMISR_REP
#define LL_HRTIM_TIMISR_UPD                HRTIM_TIMISR_UPD
#define LL_HRTIM_TIMISR_CPT1               HRTIM_TIMISR_CPT1
#define LL_HRTIM_TIMISR_CPT2               HRTIM_TIMISR_CPT2
#define LL_HRTIM_TIMISR_SET1               HRTIM_TIMISR_SET1
#define LL_HRTIM_TIMISR_RST1               HRTIM_TIMISR_RST1
#define LL_HRTIM_TIMISR_SET2               HRTIM_TIMISR_SET2
#define LL_HRTIM_TIMISR_RST2               HRTIM_TIMISR_RST2
#define LL_HRTIM_TIMISR_RST                HRTIM_TIMISR_RST
#define LL_HRTIM_TIMISR_DLYPRT             HRTIM_TIMISR_DLYPRT
/**
  * @}
  */

/** @defgroup HRTIM_EC_IT IT Defines
  * @brief    IT defines which can be used with LL_HRTIM_ReadReg and LL_HRTIM_WriteReg functions
  * @{
  */
#define LL_HRTIM_IER_FLT1IE                HRTIM_IER_FLT1IE
#define LL_HRTIM_IER_FLT2IE                HRTIM_IER_FLT2IE
#define LL_HRTIM_IER_FLT3IE                HRTIM_IER_FLT3IE
#define LL_HRTIM_IER_FLT4IE                HRTIM_IER_FLT4IE
#define LL_HRTIM_IER_FLT5IE                HRTIM_IER_FLT5IE
#define LL_HRTIM_IER_SYSFLTIE              HRTIM_IER_SYSFLTIE
#define LL_HRTIM_IER_DLLRDYIE              HRTIM_IER_DLLRDYIE
#define LL_HRTIM_IER_BMPERIE               HRTIM_IER_BMPERIE

#define LL_HRTIM_MDIER_MCMP1IE             HRTIM_MDIER_MCMP1IE
#define LL_HRTIM_MDIER_MCMP2IE             HRTIM_MDIER_MCMP2IE
#define LL_HRTIM_MDIER_MCMP3IE             HRTIM_MDIER_MCMP3IE
#define LL_HRTIM_MDIER_MCMP4IE             HRTIM_MDIER_MCMP4IE
#define LL_HRTIM_MDIER_MREPIE              HRTIM_MDIER_MREPIE
#define LL_HRTIM_MDIER_SYNCIE              HRTIM_MDIER_SYNCIE
#define LL_HRTIM_MDIER_MUPDIE              HRTIM_MDIER_MUPDIE


#define LL_HRTIM_TIMDIER_CMP1IE            HRTIM_TIMDIER_CMP1IE
#define LL_HRTIM_TIMDIER_CMP2IE            HRTIM_TIMDIER_CMP2IE
#define LL_HRTIM_TIMDIER_CMP3IE            HRTIM_TIMDIER_CMP3IE
#define LL_HRTIM_TIMDIER_CMP4IE            HRTIM_TIMDIER_CMP4IE
#define LL_HRTIM_TIMDIER_REPIE             HRTIM_TIMDIER_REPIE
#define LL_HRTIM_TIMDIER_UPDIE             HRTIM_TIMDIER_UPDIE
#define LL_HRTIM_TIMDIER_CPT1IE            HRTIM_TIMDIER_CPT1IE
#define LL_HRTIM_TIMDIER_CPT2IE            HRTIM_TIMDIER_CPT2IE
#define LL_HRTIM_TIMDIER_SET1IE            HRTIM_TIMDIER_SET1IE
#define LL_HRTIM_TIMDIER_RST1IE            HRTIM_TIMDIER_RST1IE
#define LL_HRTIM_TIMDIER_SET2IE            HRTIM_TIMDIER_SET2IE
#define LL_HRTIM_TIMDIER_RST2IE            HRTIM_TIMDIER_RST2IE
#define LL_HRTIM_TIMDIER_RSTIE             HRTIM_TIMDIER_RSTIE
#define LL_HRTIM_TIMDIER_DLYPRTIE          HRTIM_TIMDIER_DLYPRTIE
/**
  * @}
  */

/** @defgroup HRTIM_EC_SYNCIN_SRC  SYNCHRONIZATION INPUT SOURCE
  * @{
  * @brief Constants defining defining the synchronization input source.
  */
#define LL_HRTIM_SYNCIN_SRC_NONE            ((uint32_t)0x00000000U)                      /*!< HRTIM is not synchronized and runs in standalone mode */
#define LL_HRTIM_SYNCIN_SRC_TIM_EVENT       (HRTIM_MCR_SYNC_IN_1)                        /*!< The HRTIM is synchronized with the on-chip timer */
#define LL_HRTIM_SYNCIN_SRC_EXTERNAL_EVENT  (HRTIM_MCR_SYNC_IN_1 | HRTIM_MCR_SYNC_IN_0)  /*!< A positive pulse on SYNCIN input triggers the HRTIM */
/**
  * @}
  */

/** @defgroup HRTIM_EC_SYNCOUT_SRC  SYNCHRONIZATION OUTPUT SOURCE
  * @{
  * @brief Constants defining the source and event to be sent on the synchronization output.
  */
#define LL_HRTIM_SYNCOUT_SRC_MASTER_START  ((uint32_t)0x00000000U)                        /*!< A pulse is sent on the SYNCOUT output upon master timer start event */
#define LL_HRTIM_SYNCOUT_SRC_MASTER_CMP1   (HRTIM_MCR_SYNC_SRC_0)                         /*!< A pulse is sent on the SYNCOUT output upon master timer compare 1 event*/
#define LL_HRTIM_SYNCOUT_SRC_TIMA_START    (HRTIM_MCR_SYNC_SRC_1)                         /*!< A pulse is sent on the SYNCOUT output upon timer A start or reset events */
#define LL_HRTIM_SYNCOUT_SRC_TIMA_CMP1     (HRTIM_MCR_SYNC_SRC_1 | HRTIM_MCR_SYNC_SRC_0)  /*!< A pulse is sent on the SYNCOUT output upon timer A compare 1 event */
/**
  * @}
  */

/** @defgroup HRTIM_EC_SYNCOUT_POLARITY  SYNCHRONIZATION OUTPUT POLARITY
  * @{
  * @brief Constants defining the routing and conditioning of the synchronization output event.
  */
#define LL_HRTIM_SYNCOUT_DISABLED     ((uint32_t)0x00000000U)                         /*!< Synchronization output event is disabled */
#define LL_HRTIM_SYNCOUT_POSITIVE_PULSE (HRTIM_MCR_SYNC_OUT_1)                        /*!< SCOUT pin has a low idle level and issues a positive pulse of 16 fHRTIM clock cycles length for the synchronization */
#define LL_HRTIM_SYNCOUT_NEGATIVE_PULSE (HRTIM_MCR_SYNC_OUT_1 | HRTIM_MCR_SYNC_OUT_0) /*!< SCOUT pin has a high idle level and issues a negative pulse of 16 fHRTIM clock cycles length for the synchronization */
/**
  * @}
  */

/** @defgroup HRTIM_EC_TIMER  TIMER ID
  * @{
  * @brief Constants identifying a timing unit.
  */
#define LL_HRTIM_TIMER_MASTER              HRTIM_MCR_MCEN   /*!< Master timer identifier */
#define LL_HRTIM_TIMER_A                   HRTIM_MCR_TACEN  /*!< Timer A identifier */
#define LL_HRTIM_TIMER_B                   HRTIM_MCR_TBCEN  /*!< Timer B identifier */
#define LL_HRTIM_TIMER_C                   HRTIM_MCR_TCCEN  /*!< Timer C identifier */
#define LL_HRTIM_TIMER_D                   HRTIM_MCR_TDCEN  /*!< Timer D identifier */
#define LL_HRTIM_TIMER_E                   HRTIM_MCR_TECEN  /*!< Timer E identifier */
/**
  * @}
  */

/** @defgroup HRTIM_EC_OUTPUT  OUTPUT ID
  * @{
  * @brief Constants identifying an HRTIM output.
  */
#define LL_HRTIM_OUTPUT_TA1                HRTIM_OENR_TA1OEN  /*!< Timer A - Output 1 identifier */
#define LL_HRTIM_OUTPUT_TA2                HRTIM_OENR_TA2OEN  /*!< Timer A - Output 2 identifier */
#define LL_HRTIM_OUTPUT_TB1                HRTIM_OENR_TB1OEN  /*!< Timer B - Output 1 identifier */
#define LL_HRTIM_OUTPUT_TB2                HRTIM_OENR_TB2OEN  /*!< Timer B - Output 2 identifier */
#define LL_HRTIM_OUTPUT_TC1                HRTIM_OENR_TC1OEN  /*!< Timer C - Output 1 identifier */
#define LL_HRTIM_OUTPUT_TC2                HRTIM_OENR_TC2OEN  /*!< Timer C - Output 2 identifier */
#define LL_HRTIM_OUTPUT_TD1                HRTIM_OENR_TD1OEN  /*!< Timer D - Output 1 identifier */
#define LL_HRTIM_OUTPUT_TD2                HRTIM_OENR_TD2OEN  /*!< Timer D - Output 2 identifier */
#define LL_HRTIM_OUTPUT_TE1                HRTIM_OENR_TE1OEN  /*!< Timer E - Output 1 identifier */
#define LL_HRTIM_OUTPUT_TE2                HRTIM_OENR_TE2OEN  /*!< Timer E - Output 2 identifier */
/**
  * @}
  */

/** @defgroup HRTIM_EC_COMPAREUNIT  COMPARE UNIT ID
  * @{
  * @brief Constants identifying a compare unit.
  */
#define LL_HRTIM_COMPAREUNIT_2             HRTIM_TIMCR_DELCMP2  /*!< Compare unit 2 identifier */
#define LL_HRTIM_COMPAREUNIT_4             HRTIM_TIMCR_DELCMP4  /*!< Compare unit 4 identifier */
/**
  * @}
  */

/** @defgroup HRTIM_EC_CAPTUREUNIT  CAPTURE UNIT ID
  * @{
  * @brief Constants identifying a capture unit.
  */
#define LL_HRTIM_CAPTUREUNIT_1             0  /*!< Capture unit 1 identifier */
#define LL_HRTIM_CAPTUREUNIT_2             1  /*!< Capture unit 2 identifier */
/**
  * @}
  */

/** @defgroup HRTIM_EC_FAULT  FAULT ID
  * @{
  * @brief Constants identifying a fault channel.
  */
#define LL_HRTIM_FAULT_1      HRTIM_FLTR_FLT1EN     /*!< Fault channel 1 identifier */
#define LL_HRTIM_FAULT_2      HRTIM_FLTR_FLT2EN     /*!< Fault channel 2 identifier */
#define LL_HRTIM_FAULT_3      HRTIM_FLTR_FLT3EN     /*!< Fault channel 3 identifier */
#define LL_HRTIM_FAULT_4      HRTIM_FLTR_FLT4EN     /*!< Fault channel 4 identifier */
#define LL_HRTIM_FAULT_5      HRTIM_FLTR_FLT5EN     /*!< Fault channel 5 identifier */
/**
  * @}
  */

/** @defgroup HRTIM_EC_EVENT  EXTERNAL EVENT ID
  * @{
  * @brief Constants identifying an external event channel.
  */
#define LL_HRTIM_EVENT_1        ((uint32_t)0x00000001U)     /*!< External event channel 1 identifier */
#define LL_HRTIM_EVENT_2        ((uint32_t)0x00000002U)     /*!< External event channel 2 identifier */
#define LL_HRTIM_EVENT_3        ((uint32_t)0x00000004U)     /*!< External event channel 3 identifier */
#define LL_HRTIM_EVENT_4        ((uint32_t)0x00000008U)     /*!< External event channel 4 identifier */
#define LL_HRTIM_EVENT_5        ((uint32_t)0x00000010U)     /*!< External event channel 5 identifier */
#define LL_HRTIM_EVENT_6        ((uint32_t)0x00000020U)     /*!< External event channel 6 identifier */
#define LL_HRTIM_EVENT_7        ((uint32_t)0x00000040U)     /*!< External event channel 7 identifier */
#define LL_HRTIM_EVENT_8        ((uint32_t)0x00000080U)     /*!< External event channel 8 identifier */
#define LL_HRTIM_EVENT_9        ((uint32_t)0x00000100U)     /*!< External event channel 9 identifier */
#define LL_HRTIM_EVENT_10       ((uint32_t)0x00000200U)     /*!< External event channel 10 identifier */
/**
  * @}
  */

/** @defgroup HRTIM_EC_OUTPUTSTATE  OUTPUT STATE
  * @{
  * @brief Constants defining the state of an HRTIM output.
  */
#define LL_HRTIM_OUTPUTSTATE_IDLE          ((uint32_t)0x00000001U) /*!< Main operating mode, where the output can take the active or inactive level as programmed in the crossbar unit */
#define LL_HRTIM_OUTPUTSTATE_RUN           ((uint32_t)0x00000002U) /*!< Default operating state (e.g. after an HRTIM reset, when the outputs are disabled by software or during a burst mode operation) */
#define LL_HRTIM_OUTPUTSTATE_FAULT         ((uint32_t)0x00000003U) /*!< Safety state, entered in case of a shut-down request on FAULTx inputs */
/**
  * @}
  */

/** @defgroup HRTIM_EC_ADCTRIG  ADC TRIGGER
  * @{
  * @brief Constants identifying an ADC trigger.
  */
#define LL_HRTIM_ADCTRIG_1              ((uint32_t)0x00000000U) /*!< ADC trigger 1 identifier */
#define LL_HRTIM_ADCTRIG_2              ((uint32_t)0x00000001U)  /*!< ADC trigger 2 identifier */
#define LL_HRTIM_ADCTRIG_3              ((uint32_t)0x00000002U)  /*!< ADC trigger 3 identifier */
#define LL_HRTIM_ADCTRIG_4              ((uint32_t)0x00000003U)  /*!< ADC trigger 4 identifier */
/**
  * @}
  */

/** @defgroup HRTIM_EC_ADCTRIG_UPDATE ADC TRIGGER UPDATE
  * @{
  * @brief constants defining the source triggering the update of the HRTIM_ADCxR register (transfer from preload to active register).
  */
#define LL_HRTIM_ADCTRIG_UPDATE_MASTER  ((uint32_t)0x00000000U)                       /*!< HRTIM_ADCxR register update is triggered by the Master timer */
#define LL_HRTIM_ADCTRIG_UPDATE_TIMER_A (HRTIM_CR1_ADC1USRC_0)                        /*!< HRTIM_ADCxR register update is triggered by the Timer A */
#define LL_HRTIM_ADCTRIG_UPDATE_TIMER_B (HRTIM_CR1_ADC1USRC_1)                        /*!< HRTIM_ADCxR register update is triggered by the Timer B */
#define LL_HRTIM_ADCTRIG_UPDATE_TIMER_C (HRTIM_CR1_ADC1USRC_1 | HRTIM_CR1_ADC1USRC_0) /*!< HRTIM_ADCxR register update is triggered by the Timer C */
#define LL_HRTIM_ADCTRIG_UPDATE_TIMER_D (HRTIM_CR1_ADC1USRC_2)                        /*!< HRTIM_ADCxR register update is triggered by the Timer D */
#define LL_HRTIM_ADCTRIG_UPDATE_TIMER_E (HRTIM_CR1_ADC1USRC_2 | HRTIM_CR1_ADC1USRC_0) /*!< HRTIM_ADCxR register update is triggered by the Timer E */
/**
  * @}
  */

/** @defgroup HRTIM_EC_ADCTRIG_SRC13  ADC TRIGGER 1/3 SOURCE
  * @{
  * @brief constants defining the events triggering ADC conversion for ADC Triggers 1 and 3.
  */
#define LL_HRTIM_ADCTRIG_SRC13_NONE           ((uint32_t)0x00000000U)  /*!< No ADC trigger event */
#define LL_HRTIM_ADCTRIG_SRC13_MCMP1          HRTIM_ADC1R_AD1MC1       /*!< ADC Trigger on master compare 1 */
#define LL_HRTIM_ADCTRIG_SRC13_MCMP2          HRTIM_ADC1R_AD1MC2       /*!< ADC Trigger on master compare 2 */
#define LL_HRTIM_ADCTRIG_SRC13_MCMP3          HRTIM_ADC1R_AD1MC3       /*!< ADC Trigger on master compare 3 */
#define LL_HRTIM_ADCTRIG_SRC13_MCMP4          HRTIM_ADC1R_AD1MC4       /*!< ADC Trigger on master compare 4 */
#define LL_HRTIM_ADCTRIG_SRC13_MPER           HRTIM_ADC1R_AD1MPER      /*!< ADC Trigger on master period */
#define LL_HRTIM_ADCTRIG_SRC13_EEV1           HRTIM_ADC1R_AD1EEV1      /*!< ADC Trigger on external event 1 */
#define LL_HRTIM_ADCTRIG_SRC13_EEV2           HRTIM_ADC1R_AD1EEV2      /*!< ADC Trigger on external event 2 */
#define LL_HRTIM_ADCTRIG_SRC13_EEV3           HRTIM_ADC1R_AD1EEV3      /*!< ADC Trigger on external event 3 */
#define LL_HRTIM_ADCTRIG_SRC13_EEV4           HRTIM_ADC1R_AD1EEV4      /*!< ADC Trigger on external event 4 */
#define LL_HRTIM_ADCTRIG_SRC13_EEV5           HRTIM_ADC1R_AD1EEV5      /*!< ADC Trigger on external event 5 */
#define LL_HRTIM_ADCTRIG_SRC13_TIMACMP2       HRTIM_ADC1R_AD1TAC2      /*!< ADC Trigger on Timer A compare 2 */
#define LL_HRTIM_ADCTRIG_SRC13_TIMACMP3       HRTIM_ADC1R_AD1TAC3      /*!< ADC Trigger on Timer A compare 3 */
#define LL_HRTIM_ADCTRIG_SRC13_TIMACMP4       HRTIM_ADC1R_AD1TAC4      /*!< ADC Trigger on Timer A compare 4 */
#define LL_HRTIM_ADCTRIG_SRC13_TIMAPER        HRTIM_ADC1R_AD1TAPER     /*!< ADC Trigger on Timer A period */
#define LL_HRTIM_ADCTRIG_SRC13_TIMARST        HRTIM_ADC1R_AD1TARST     /*!< ADC Trigger on Timer A reset */
#define LL_HRTIM_ADCTRIG_SRC13_TIMBCMP2       HRTIM_ADC1R_AD1TBC2      /*!< ADC Trigger on Timer B compare 2 */
#define LL_HRTIM_ADCTRIG_SRC13_TIMBCMP3       HRTIM_ADC1R_AD1TBC3      /*!< ADC Trigger on Timer B compare 3 */
#define LL_HRTIM_ADCTRIG_SRC13_TIMBCMP4       HRTIM_ADC1R_AD1TBC4      /*!< ADC Trigger on Timer B compare 4 */
#define LL_HRTIM_ADCTRIG_SRC13_TIMBPER        HRTIM_ADC1R_AD1TBPER     /*!< ADC Trigger on Timer B period */
#define LL_HRTIM_ADCTRIG_SRC13_TIMBRST        HRTIM_ADC1R_AD1TBRST     /*!< ADC Trigger on Timer B reset */
#define LL_HRTIM_ADCTRIG_SRC13_TIMCCMP2       HRTIM_ADC1R_AD1TCC2      /*!< ADC Trigger on Timer C compare 2 */
#define LL_HRTIM_ADCTRIG_SRC13_TIMCCMP3       HRTIM_ADC1R_AD1TCC3      /*!< ADC Trigger on Timer C compare 3 */
#define LL_HRTIM_ADCTRIG_SRC13_TIMCCMP4       HRTIM_ADC1R_AD1TCC4      /*!< ADC Trigger on Timer C compare 4 */
#define LL_HRTIM_ADCTRIG_SRC13_TIMCPER        HRTIM_ADC1R_AD1TCPER     /*!< ADC Trigger on Timer C period */
#define LL_HRTIM_ADCTRIG_SRC13_TIMDCMP2       HRTIM_ADC1R_AD1TDC2      /*!< ADC Trigger on Timer D compare 2 */
#define LL_HRTIM_ADCTRIG_SRC13_TIMDCMP3       HRTIM_ADC1R_AD1TDC3      /*!< ADC Trigger on Timer D compare 3 */
#define LL_HRTIM_ADCTRIG_SRC13_TIMDCMP4       HRTIM_ADC1R_AD1TDC4      /*!< ADC Trigger on Timer D compare 4 */
#define LL_HRTIM_ADCTRIG_SRC13_TIMDPER        HRTIM_ADC1R_AD1TDPER     /*!< ADC Trigger on Timer D period */
#define LL_HRTIM_ADCTRIG_SRC13_TIMECMP2       HRTIM_ADC1R_AD1TEC2      /*!< ADC Trigger on Timer E compare 2 */
#define LL_HRTIM_ADCTRIG_SRC13_TIMECMP3       HRTIM_ADC1R_AD1TEC3      /*!< ADC Trigger on Timer E compare 3 */
#define LL_HRTIM_ADCTRIG_SRC13_TIMECMP4       HRTIM_ADC1R_AD1TEC4      /*!< ADC Trigger on Timer E compare 4 */
#define LL_HRTIM_ADCTRIG_SRC13_TIMEPER        HRTIM_ADC1R_AD1TEPER     /*!< ADC Trigger on Timer E period */
/**
  * @}
  */

/** @defgroup HRTIM_EC_ADCTRIG_SRC24  ADC TRIGGER 2/4 SOURCE
  * @{
  * @brief constants defining the events triggering ADC conversion for ADC Triggers 2 and 4.
  */
#define LL_HRTIM_ADCTRIG_SRC24_NONE           ((uint32_t)0x00000000U)/*!< No ADC trigger event */
#define LL_HRTIM_ADCTRIG_SRC24_MCMP1          HRTIM_ADC2R_AD2MC1     /*!< ADC Trigger on master compare 1 */
#define LL_HRTIM_ADCTRIG_SRC24_MCMP2          HRTIM_ADC2R_AD2MC2     /*!< ADC Trigger on master compare 2 */
#define LL_HRTIM_ADCTRIG_SRC24_MCMP3          HRTIM_ADC2R_AD2MC3     /*!< ADC Trigger on master compare 3 */
#define LL_HRTIM_ADCTRIG_SRC24_MCMP4          HRTIM_ADC2R_AD2MC4     /*!< ADC Trigger on master compare 4 */
#define LL_HRTIM_ADCTRIG_SRC24_MPER           HRTIM_ADC2R_AD2MPER    /*!< ADC Trigger on master period */
#define LL_HRTIM_ADCTRIG_SRC24_EEV6           HRTIM_ADC2R_AD2EEV6    /*!< ADC Trigger on external event 6 */
#define LL_HRTIM_ADCTRIG_SRC24_EEV7           HRTIM_ADC2R_AD2EEV7    /*!< ADC Trigger on external event 7 */
#define LL_HRTIM_ADCTRIG_SRC24_EEV8           HRTIM_ADC2R_AD2EEV8    /*!< ADC Trigger on external event 8 */
#define LL_HRTIM_ADCTRIG_SRC24_EEV9           HRTIM_ADC2R_AD2EEV9    /*!< ADC Trigger on external event 9 */
#define LL_HRTIM_ADCTRIG_SRC24_EEV10          HRTIM_ADC2R_AD2EEV10   /*!< ADC Trigger on external event 10 */
#define LL_HRTIM_ADCTRIG_SRC24_TIMACMP2       HRTIM_ADC2R_AD2TAC2    /*!< ADC Trigger on Timer A compare 2 */
#define LL_HRTIM_ADCTRIG_SRC24_TIMACMP3       HRTIM_ADC2R_AD2TAC3    /*!< ADC Trigger on Timer A compare 3 */
#define LL_HRTIM_ADCTRIG_SRC24_TIMACMP4       HRTIM_ADC2R_AD2TAC4    /*!< ADC Trigger on Timer A compare 4 */
#define LL_HRTIM_ADCTRIG_SRC24_TIMAPER        HRTIM_ADC2R_AD2TAPER   /*!< ADC Trigger on Timer A period */
#define LL_HRTIM_ADCTRIG_SRC24_TIMBCMP2       HRTIM_ADC2R_AD2TBC2    /*!< ADC Trigger on Timer B compare 2 */
#define LL_HRTIM_ADCTRIG_SRC24_TIMBCMP3       HRTIM_ADC2R_AD2TBC3    /*!< ADC Trigger on Timer B compare 3 */
#define LL_HRTIM_ADCTRIG_SRC24_TIMBCMP4       HRTIM_ADC2R_AD2TBC4    /*!< ADC Trigger on Timer B compare 4 */
#define LL_HRTIM_ADCTRIG_SRC24_TIMBPER        HRTIM_ADC2R_AD2TBPER   /*!< ADC Trigger on Timer B period */
#define LL_HRTIM_ADCTRIG_SRC24_TIMCCMP2       HRTIM_ADC2R_AD2TCC2    /*!< ADC Trigger on Timer C compare 2 */
#define LL_HRTIM_ADCTRIG_SRC24_TIMCCMP3       HRTIM_ADC2R_AD2TCC3    /*!< ADC Trigger on Timer C compare 3 */
#define LL_HRTIM_ADCTRIG_SRC24_TIMCCMP4       HRTIM_ADC2R_AD2TCC4    /*!< ADC Trigger on Timer C compare 4 */
#define LL_HRTIM_ADCTRIG_SRC24_TIMCPER        HRTIM_ADC2R_AD2TCPER   /*!< ADC Trigger on Timer C period */
#define LL_HRTIM_ADCTRIG_SRC24_TIMCRST        HRTIM_ADC2R_AD2TCRST   /*!< ADC Trigger on Timer C reset */
#define LL_HRTIM_ADCTRIG_SRC24_TIMDCMP2       HRTIM_ADC2R_AD2TDC2    /*!< ADC Trigger on Timer D compare 2 */
#define LL_HRTIM_ADCTRIG_SRC24_TIMDCMP3       HRTIM_ADC2R_AD2TDC3    /*!< ADC Trigger on Timer D compare 3 */
#define LL_HRTIM_ADCTRIG_SRC24_TIMDCMP4       HRTIM_ADC2R_AD2TDC4    /*!< ADC Trigger on Timer D compare 4 */
#define LL_HRTIM_ADCTRIG_SRC24_TIMDPER        HRTIM_ADC2R_AD2TDPER   /*!< ADC Trigger on Timer D period */
#define LL_HRTIM_ADCTRIG_SRC24_TIMDRST        HRTIM_ADC2R_AD2TDRST   /*!< ADC Trigger on Timer D reset */
#define LL_HRTIM_ADCTRIG_SRC24_TIMECMP2       HRTIM_ADC2R_AD2TEC2    /*!< ADC Trigger on Timer E compare 2 */
#define LL_HRTIM_ADCTRIG_SRC24_TIMECMP3       HRTIM_ADC2R_AD2TEC3    /*!< ADC Trigger on Timer E compare 3 */
#define LL_HRTIM_ADCTRIG_SRC24_TIMECMP4       HRTIM_ADC2R_AD2TEC4    /*!< ADC Trigger on Timer E compare 4 */
#define LL_HRTIM_ADCTRIG_SRC24_TIMERST        HRTIM_ADC2R_AD2TERST   /*!< ADC Trigger on Timer E reset */
/**
  * @}
  */

/** @defgroup HRTIM_EC_DLLCALIBRATION_MODE  DLL CALIBRATION MODE
  * @{
  * @brief Constants defining the DLL calibration mode.
  */
#define LL_HRTIM_DLLCALIBRATION_MODE_SINGLESHOT   ((uint32_t)0x00000000U)/*!<Calibration is perfomed only once */
#define LL_HRTIM_DLLCALIBRATION_MODE_CONTINUOUS   HRTIM_DLLCR_CALEN      /*!<Calibration is performed periodically */
/**
  * @}
  */

/** @defgroup HRTIM_EC_CALIBRATIONRATE  DLL CALIBRATION RATE
  * @{
  * @brief Constants defining the DLL calibration periods (in micro seconds).
  */
#define LL_HRTIM_DLLCALIBRATION_RATE_7300      ((uint32_t)0x00000000U)                        /*!< Periodic DLL calibration: T = 1048576 * tHRTIM (7.3 ms) */
#define LL_HRTIM_DLLCALIBRATION_RATE_910       (HRTIM_DLLCR_CALRTE_0)                         /*!< Periodic DLL calibration: T = 131072 * tHRTIM (910 ms) */
#define LL_HRTIM_DLLCALIBRATION_RATE_114       (HRTIM_DLLCR_CALRTE_1)                         /*!< Periodic DLL calibration: T = 16384 * tHRTIM (114 ms) */
#define LL_HRTIM_DLLCALIBRATION_RATE_14        (HRTIM_DLLCR_CALRTE_1 | HRTIM_DLLCR_CALRTE_0)  /*!< Periodic DLL calibration: T = 2048 * tHRTIM (14 ms) */
/**
  * @}
  */

/** @defgroup HRTIM_EC_PRESCALERRATIO  PRESCALER RATIO
  * @{
  * @brief Constants defining timer high-resolution clock prescaler ratio.
  */
#define LL_HRTIM_PRESCALERRATIO_MUL32      ((uint32_t)0x00000000U) /*!< fHRCK: fHRTIM x 32 = 4.608 GHz - Resolution: 217 ps - Min PWM frequency: 70.3 kHz (fHRTIM=144MHz)      */
#define LL_HRTIM_PRESCALERRATIO_MUL16      ((uint32_t)0x00000001U)  /*!< fHRCK: fHRTIM x 16 = 2.304 GHz - Resolution: 434 ps - Min PWM frequency: 35.1 KHz (fHRTIM=144MHz)      */
#define LL_HRTIM_PRESCALERRATIO_MUL8       ((uint32_t)0x00000002U)  /*!< fHRCK: fHRTIM x 8 = 1.152 GHz - Resolution: 868 ps - Min PWM frequency: 17.6 kHz (fHRTIM=144MHz)      */
#define LL_HRTIM_PRESCALERRATIO_MUL4       ((uint32_t)0x00000003U)  /*!< fHRCK: fHRTIM x 4 = 576 MHz - Resolution: 1.73 ns - Min PWM frequency: 8.8 kHz (fHRTIM=144MHz)      */
#define LL_HRTIM_PRESCALERRATIO_MUL2       ((uint32_t)0x00000004U)  /*!< fHRCK: fHRTIM x 2 = 288 MHz - Resolution: 3.47 ns - Min PWM frequency: 4.4 kHz (fHRTIM=144MHz)      */
#define LL_HRTIM_PRESCALERRATIO_DIV1       ((uint32_t)0x00000005U)  /*!< fHRCK: fHRTIM = 144 MHz - Resolution: 6.95 ns - Min PWM frequency: 2.2 kHz (fHRTIM=144MHz)      */
#define LL_HRTIM_PRESCALERRATIO_DIV2       ((uint32_t)0x00000006U)  /*!< fHRCK: fHRTIM / 2 = 72 MHz - Resolution: 13.88 ns- Min PWM frequency: 1.1 kHz (fHRTIM=144MHz)      */
#define LL_HRTIM_PRESCALERRATIO_DIV4       ((uint32_t)0x00000007U)  /*!< fHRCK: fHRTIM / 4 = 36 MHz - Resolution: 27.7 ns- Min PWM frequency: 550Hz (fHRTIM=144MHz)      */
/**
  * @}
  */

/** @defgroup HRTIM_EC_MODE  COUNTER MODE
  * @{
  * @brief Constants defining timer counter operating mode.
  */
#define LL_HRTIM_MODE_CONTINUOUS           ((uint32_t)0x00000008U)  /*!< The timer operates in continuous (free-running) mode */
#define LL_HRTIM_MODE_SINGLESHOT           ((uint32_t)0x00000000U) /*!< The timer operates in non retriggerable single-shot mode */
#define LL_HRTIM_MODE_RETRIGGERABLE        ((uint32_t)0x00000010U)  /*!< The timer operates in retriggerable single-shot mode */
/**
  * @}
  */

/** @defgroup HRTIM_EC_DACTRIG  DAC TRIGGER
  * @{
  * @brief Constants defining on which output the DAC synchronization event is sent.
  */
#define LL_HRTIM_DACTRIG_NONE           ((uint32_t)0x00000000U)                     /*!< No DAC synchronization event generated */
#define LL_HRTIM_DACTRIG_DACTRIGOUT_1   (HRTIM_MCR_DACSYNC_0)                       /*!< DAC synchronization event generated on DACTrigOut1 output upon timer update */
#define LL_HRTIM_DACTRIG_DACTRIGOUT_2   (HRTIM_MCR_DACSYNC_1)                       /*!< DAC synchronization event generated on DACTrigOut2 output upon timer update */
#define LL_HRTIM_DACTRIG_DACTRIGOUT_3   (HRTIM_MCR_DACSYNC_1 | HRTIM_MCR_DACSYNC_0) /*!< DAC synchronization event generated on DACTrigOut3 output upon timer update */
/**
  * @}
  */

/** @defgroup HRTIM_EC_UPDATETRIG  UPDATE TRIGGER
  * @{
  * @brief Constants defining whether the registers update is done synchronously with any other timer or master update.
  */
#define LL_HRTIM_UPDATETRIG_NONE        ((uint32_t)0x00000000U)/*!< Register update is disabled */
#define LL_HRTIM_UPDATETRIG_MASTER      HRTIM_TIMCR_MSTU       /*!< Register update is triggered by the master timer update */
#define LL_HRTIM_UPDATETRIG_TIMER_A     HRTIM_TIMCR_TAU        /*!< Register update is triggered by the timer A update */
#define LL_HRTIM_UPDATETRIG_TIMER_B     HRTIM_TIMCR_TBU        /*!< Register update is triggered by the timer B update */
#define LL_HRTIM_UPDATETRIG_TIMER_C     HRTIM_TIMCR_TCU        /*!< Register update is triggered by the timer C update*/
#define LL_HRTIM_UPDATETRIG_TIMER_D     HRTIM_TIMCR_TDU        /*!< Register update is triggered by the timer D update */
#define LL_HRTIM_UPDATETRIG_TIMER_E     HRTIM_TIMCR_TEU        /*!< Register update is triggered by the timer E update */
#define LL_HRTIM_UPDATETRIG_REPETITION  HRTIM_TIMCR_TREPU      /*!< Register update is triggered when the counter rolls over and HRTIM_REPx = 0*/
#define LL_HRTIM_UPDATETRIG_RESET       HRTIM_TIMCR_TRSTU      /*!< Register update is triggered by counter reset or roll-over to 0 after reaching the period value in continuous mode */
/**
  * @}
  */

/** @defgroup HRTIM_EC_UPDATEGATING  UPDATE GATING
  * @{
  * @brief Constants defining how the update occurs relatively to the burst DMA transaction and the external update request on update enable inputs 1 to 3.
  */
#define LL_HRTIM_UPDATEGATING_INDEPENDENT     ((uint32_t)0x00000000U)                                               /*!< Update done independently from the DMA burst transfer completion */
#define LL_HRTIM_UPDATEGATING_DMABURST        (HRTIM_TIMCR_UPDGAT_0)                                                /*!< Update done when the DMA burst transfer is completed */
#define LL_HRTIM_UPDATEGATING_DMABURST_UPDATE (HRTIM_TIMCR_UPDGAT_1)                                                /*!< Update done on timer roll-over following a DMA burst transfer completion*/
#define LL_HRTIM_UPDATEGATING_UPDEN1          (HRTIM_TIMCR_UPDGAT_1 | HRTIM_TIMCR_UPDGAT_0)                         /*!< Slave timer only - Update done on a rising edge of HRTIM update enable input 1 */
#define LL_HRTIM_UPDATEGATING_UPDEN2          (HRTIM_TIMCR_UPDGAT_2)                                                /*!< Slave timer only - Update done on a rising edge of HRTIM update enable input 2 */
#define LL_HRTIM_UPDATEGATING_UPDEN3          (HRTIM_TIMCR_UPDGAT_2 | HRTIM_TIMCR_UPDGAT_0)                         /*!< Slave timer only - Update done on a rising edge of HRTIM update enable input 3 */
#define LL_HRTIM_UPDATEGATING_UPDEN1_UPDATE   (HRTIM_TIMCR_UPDGAT_2 | HRTIM_TIMCR_UPDGAT_1)                         /*!< Slave timer only -  Update done on the update event following a rising edge of HRTIM update enable input 1 */
#define LL_HRTIM_UPDATEGATING_UPDEN2_UPDATE   (HRTIM_TIMCR_UPDGAT_2 | HRTIM_TIMCR_UPDGAT_1 | HRTIM_TIMCR_UPDGAT_0)  /*!< Slave timer only - Update done on the update event following a rising edge of HRTIM update enable input 2 */
#define LL_HRTIM_UPDATEGATING_UPDEN3_UPDATE   (HRTIM_TIMCR_UPDGAT_3)                                                /*!< Slave timer only - Update done on the update event following a rising edge of HRTIM update enable input 3 */
/**
  * @}
  */

/** @defgroup HRTIM_EC_COMPAREMODE  COMPARE MODE
  * @{
  * @brief Constants defining whether the compare register is behaving in regular mode (compare match issued as soon as counter equal compare) or in auto-delayed mode.
  */
#define LL_HRTIM_COMPAREMODE_REGULAR          ((uint32_t)0x00000000U)                         /*!< standard compare mode */
#define LL_HRTIM_COMPAREMODE_DELAY_NOTIMEOUT  (HRTIM_TIMCR_DELCMP2_0)                         /*!< Compare event generated only if a capture has occurred */
#define LL_HRTIM_COMPAREMODE_DELAY_CMP1       (HRTIM_TIMCR_DELCMP2_1)                         /*!< Compare event generated if a capture has occurred or after a Compare 1 match (timeout if capture event is missing) */
#define LL_HRTIM_COMPAREMODE_DELAY_CMP3       (HRTIM_TIMCR_DELCMP2_1 | HRTIM_TIMCR_DELCMP2_0) /*!< Compare event generated if a capture has occurred or after a Compare 3 match (timeout if capture event is missing) */
/**
  * @}
  */

/** @defgroup HRTIM_EC_RESETTRIG  RESET TRIGGER
  * @{
  * @brief Constants defining the events that can be selected to trigger the reset of the timer counter.
  */
#define LL_HRTIM_RESETTRIG_NONE        ((uint32_t)0x00000000U)/*!< No counter reset trigger */
#define LL_HRTIM_RESETTRIG_UPDATE      HRTIM_RSTR_UPDATE      /*!< The timer counter is reset upon update event */
#define LL_HRTIM_RESETTRIG_CMP2        HRTIM_RSTR_CMP2        /*!< The timer counter is reset upon Timer Compare 2 event */
#define LL_HRTIM_RESETTRIG_CMP4        HRTIM_RSTR_CMP4        /*!< The timer counter is reset upon Timer Compare 4 event */
#define LL_HRTIM_RESETTRIG_MASTER_PER  HRTIM_RSTR_MSTPER      /*!< The timer counter is reset upon master timer period event */
#define LL_HRTIM_RESETTRIG_MASTER_CMP1 HRTIM_RSTR_MSTCMP1     /*!< The timer counter is reset upon master timer Compare 1 event */
#define LL_HRTIM_RESETTRIG_MASTER_CMP2 HRTIM_RSTR_MSTCMP2     /*!< The timer counter is reset upon master timer Compare 2 event */
#define LL_HRTIM_RESETTRIG_MASTER_CMP3 HRTIM_RSTR_MSTCMP3     /*!< The timer counter is reset upon master timer Compare 3 event */
#define LL_HRTIM_RESETTRIG_MASTER_CMP4 HRTIM_RSTR_MSTCMP4     /*!< The timer counter is reset upon master timer Compare 4 event */
#define LL_HRTIM_RESETTRIG_EEV_1       HRTIM_RSTR_EXTEVNT1    /*!< The timer counter is reset upon external event 1 */
#define LL_HRTIM_RESETTRIG_EEV_2       HRTIM_RSTR_EXTEVNT2    /*!< The timer counter is reset upon external event 2 */
#define LL_HRTIM_RESETTRIG_EEV_3       HRTIM_RSTR_EXTEVNT3    /*!< The timer counter is reset upon external event 3 */
#define LL_HRTIM_RESETTRIG_EEV_4       HRTIM_RSTR_EXTEVNT4    /*!< The timer counter is reset upon external event 4 */
#define LL_HRTIM_RESETTRIG_EEV_5       HRTIM_RSTR_EXTEVNT5    /*!< The timer counter is reset upon external event 5 */
#define LL_HRTIM_RESETTRIG_EEV_6       HRTIM_RSTR_EXTEVNT6    /*!< The timer counter is reset upon external event 6 */
#define LL_HRTIM_RESETTRIG_EEV_7       HRTIM_RSTR_EXTEVNT7    /*!< The timer counter is reset upon external event 7 */
#define LL_HRTIM_RESETTRIG_EEV_8       HRTIM_RSTR_EXTEVNT8    /*!< The timer counter is reset upon external event 8 */
#define LL_HRTIM_RESETTRIG_EEV_9       HRTIM_RSTR_EXTEVNT9    /*!< The timer counter is reset upon external event 9 */
#define LL_HRTIM_RESETTRIG_EEV_10      HRTIM_RSTR_EXTEVNT10   /*!< The timer counter is reset upon external event 10 */
#define LL_HRTIM_RESETTRIG_OTHER1_CMP1 HRTIM_RSTR_TIMBCMP1    /*!< The timer counter is reset upon other timer Compare 1 event */
#define LL_HRTIM_RESETTRIG_OTHER1_CMP2 HRTIM_RSTR_TIMBCMP2    /*!< The timer counter is reset upon other timer Compare 2 event */
#define LL_HRTIM_RESETTRIG_OTHER1_CMP4 HRTIM_RSTR_TIMBCMP4    /*!< The timer counter is reset upon other timer Compare 4 event */
#define LL_HRTIM_RESETTRIG_OTHER2_CMP1 HRTIM_RSTR_TIMCCMP1    /*!< The timer counter is reset upon other timer Compare 1 event */
#define LL_HRTIM_RESETTRIG_OTHER2_CMP2 HRTIM_RSTR_TIMCCMP2    /*!< The timer counter is reset upon other timer Compare 2 event */
#define LL_HRTIM_RESETTRIG_OTHER2_CMP4 HRTIM_RSTR_TIMCCMP4    /*!< The timer counter is reset upon other timer Compare 4 event */
#define LL_HRTIM_RESETTRIG_OTHER3_CMP1 HRTIM_RSTR_TIMDCMP1    /*!< The timer counter is reset upon other timer Compare 1 event */
#define LL_HRTIM_RESETTRIG_OTHER3_CMP2 HRTIM_RSTR_TIMDCMP2    /*!< The timer counter is reset upon other timer Compare 2 event */
#define LL_HRTIM_RESETTRIG_OTHER3_CMP4 HRTIM_RSTR_TIMDCMP4    /*!< The timer counter is reset upon other timer Compare 4 event */
#define LL_HRTIM_RESETTRIG_OTHER4_CMP1 HRTIM_RSTR_TIMECMP1    /*!< The timer counter is reset upon other timer Compare 1 event */
#define LL_HRTIM_RESETTRIG_OTHER4_CMP2 HRTIM_RSTR_TIMECMP2    /*!< The timer counter is reset upon other timer Compare 2 event */
#define LL_HRTIM_RESETTRIG_OTHER4_CMP4 HRTIM_RSTR_TIMECMP4    /*!< The timer counter is reset upon other timer Compare 4 event */
/**
  * @}
  */

/** @defgroup HRTIM_EC_CAPTURETRIG  CAPTURE TRIGGER
  * @{
  * @brief Constants defining the events that can be selected to trigger the capture of the timing unit counter.
  */
#define LL_HRTIM_CAPTURETRIG_NONE         ((uint32_t)0x00000000U)/*!< Capture trigger is disabled */
#define LL_HRTIM_CAPTURETRIG_UPDATE       HRTIM_CPT1CR_UPDCPT    /*!< The update event triggers the Capture */
#define LL_HRTIM_CAPTURETRIG_EEV_1        HRTIM_CPT1CR_EXEV1CPT  /*!< The External event 1 triggers the Capture */
#define LL_HRTIM_CAPTURETRIG_EEV_2        HRTIM_CPT1CR_EXEV2CPT  /*!< The External event 2 triggers the Capture */
#define LL_HRTIM_CAPTURETRIG_EEV_3        HRTIM_CPT1CR_EXEV3CPT  /*!< The External event 3 triggers the Capture */
#define LL_HRTIM_CAPTURETRIG_EEV_4        HRTIM_CPT1CR_EXEV4CPT  /*!< The External event 4 triggers the Capture */
#define LL_HRTIM_CAPTURETRIG_EEV_5        HRTIM_CPT1CR_EXEV5CPT  /*!< The External event 5 triggers the Capture */
#define LL_HRTIM_CAPTURETRIG_EEV_6        HRTIM_CPT1CR_EXEV6CPT  /*!< The External event 6 triggers the Capture */
#define LL_HRTIM_CAPTURETRIG_EEV_7        HRTIM_CPT1CR_EXEV7CPT  /*!< The External event 7 triggers the Capture */
#define LL_HRTIM_CAPTURETRIG_EEV_8        HRTIM_CPT1CR_EXEV8CPT  /*!< The External event 8 triggers the Capture */
#define LL_HRTIM_CAPTURETRIG_EEV_9        HRTIM_CPT1CR_EXEV9CPT  /*!< The External event 9 triggers the Capture */
#define LL_HRTIM_CAPTURETRIG_EEV_10       HRTIM_CPT1CR_EXEV10CPT /*!< The External event 10 triggers the Capture */
#define LL_HRTIM_CAPTURETRIG_TA1_SET      HRTIM_CPT1CR_TA1SET    /*!< Capture is triggered by TA1 output inactive to active transition */
#define LL_HRTIM_CAPTURETRIG_TA1_RESET    HRTIM_CPT1CR_TA1RST    /*!< Capture is triggered by TA1 output active to inactive transition */
#define LL_HRTIM_CAPTURETRIG_TIMA_CMP1    HRTIM_CPT1CR_TIMACMP1  /*!< Timer A Compare 1 triggers Capture */
#define LL_HRTIM_CAPTURETRIG_TIMA_CMP2    HRTIM_CPT1CR_TIMACMP2  /*!< Timer A Compare 2 triggers Capture */
#define LL_HRTIM_CAPTURETRIG_TB1_SET      HRTIM_CPT1CR_TB1SET    /*!< Capture is triggered by TB1 output inactive to active transition */
#define LL_HRTIM_CAPTURETRIG_TB1_RESET    HRTIM_CPT1CR_TB1RST    /*!< Capture is triggered by TB1 output active to inactive transition */
#define LL_HRTIM_CAPTURETRIG_TIMB_CMP1    HRTIM_CPT1CR_TIMBCMP1  /*!< Timer B Compare 1 triggers Capture */
#define LL_HRTIM_CAPTURETRIG_TIMB_CMP2    HRTIM_CPT1CR_TIMBCMP2  /*!< Timer B Compare 2 triggers Capture */
#define LL_HRTIM_CAPTURETRIG_TC1_SET      HRTIM_CPT1CR_TC1SET    /*!< Capture is triggered by TC1 output inactive to active transition */
#define LL_HRTIM_CAPTURETRIG_TC1_RESET    HRTIM_CPT1CR_TC1RST    /*!< Capture is triggered by TC1 output active to inactive transition */
#define LL_HRTIM_CAPTURETRIG_TIMC_CMP1    HRTIM_CPT1CR_TIMCCMP1  /*!< Timer C Compare 1 triggers Capture */
#define LL_HRTIM_CAPTURETRIG_TIMC_CMP2    HRTIM_CPT1CR_TIMCCMP2  /*!< Timer C Compare 2 triggers Capture */
#define LL_HRTIM_CAPTURETRIG_TD1_SET      HRTIM_CPT1CR_TD1SET    /*!< Capture is triggered by TD1 output inactive to active transition */
#define LL_HRTIM_CAPTURETRIG_TD1_RESET    HRTIM_CPT1CR_TD1RST    /*!< Capture is triggered by TD1 output active to inactive transition */
#define LL_HRTIM_CAPTURETRIG_TIMD_CMP1    HRTIM_CPT1CR_TIMDCMP1  /*!< Timer D Compare 1 triggers Capture */
#define LL_HRTIM_CAPTURETRIG_TIMD_CMP2    HRTIM_CPT1CR_TIMDCMP2  /*!< Timer D Compare 2 triggers Capture */
#define LL_HRTIM_CAPTURETRIG_TE1_SET      HRTIM_CPT1CR_TE1SET    /*!< Capture is triggered by TE1 output inactive to active transition */
#define LL_HRTIM_CAPTURETRIG_TE1_RESET    HRTIM_CPT1CR_TE1RST    /*!< Capture is triggered by TE1 output active to inactive transition */
#define LL_HRTIM_CAPTURETRIG_TIME_CMP1    HRTIM_CPT1CR_TIMECMP1  /*!< Timer E Compare 1 triggers Capture */
#define LL_HRTIM_CAPTURETRIG_TIME_CMP2    HRTIM_CPT1CR_TIMECMP2  /*!< Timer E Compare 2 triggers Capture */
/**
  * @}
  */

/** @defgroup HRTIM_EC_DLYPRT  DELAYED PROTECTION (DLYPRT) MODE
  * @{
  * @brief Constants defining all possible delayed protection modes for a timer (also define the source and outputs on which the delayed protection schemes are applied).
  */
#define LL_HRTIM_DLYPRT_DELAYOUT1_EEV6  ((uint32_t)0x00000000U)                                            /*!< Timers A, B, C: Output 1 delayed Idle on external Event 6 */
#define LL_HRTIM_DLYPRT_DELAYOUT2_EEV6  (HRTIM_OUTR_DLYPRT_0)                                             /*!< Timers A, B, C: Output 2 delayed Idle on external Event 6 */
#define LL_HRTIM_DLYPRT_DELAYBOTH_EEV6  (HRTIM_OUTR_DLYPRT_1)                                             /*!< Timers A, B, C: Output 1 and output 2 delayed Idle on external Event 6 */
#define LL_HRTIM_DLYPRT_BALANCED_EEV6   (HRTIM_OUTR_DLYPRT_1 | HRTIM_OUTR_DLYPRT_0)                       /*!< Timers A, B, C: Balanced Idle on external Event 6 */
#define LL_HRTIM_DLYPRT_DELAYOUT1_EEV7  (HRTIM_OUTR_DLYPRT_2)                                             /*!< Timers A, B, C: Output 1 delayed Idle on external Event 7 */
#define LL_HRTIM_DLYPRT_DELAYOUT2_EEV7  (HRTIM_OUTR_DLYPRT_2 | HRTIM_OUTR_DLYPRT_0)                       /*!< Timers A, B, C: Output 2 delayed Idle on external Event 7 */
#define LL_HRTIM_DLYPRT_DELAYBOTH_EEV7  (HRTIM_OUTR_DLYPRT_2 | HRTIM_OUTR_DLYPRT_1)                       /*!< Timers A, B, C: Output 1 and output2 delayed Idle on external Event 7 */
#define LL_HRTIM_DLYPRT_BALANCED_EEV7   (HRTIM_OUTR_DLYPRT_2 | HRTIM_OUTR_DLYPRT_1 | HRTIM_OUTR_DLYPRT_0) /*!< Timers A, B, C: Balanced Idle on external Event 7 */

#define LL_HRTIM_DLYPRT_DELAYOUT1_EEV8  ((uint32_t)0x00000000U)                                             /*!< Timers D, E: Output 1 delayed Idle on external Event 8 */
#define LL_HRTIM_DLYPRT_DELAYOUT2_EEV8  (HRTIM_OUTR_DLYPRT_0)                                               /*!< Timers D, E: Output 2 delayed Idle on external Event 8 */
#define LL_HRTIM_DLYPRT_DELAYBOTH_EEV8  (HRTIM_OUTR_DLYPRT_1)                                               /*!< Timers D, E: Output 1 and output 2 delayed Idle on external Event 8 */
#define LL_HRTIM_DLYPRT_BALANCED_EEV8   (HRTIM_OUTR_DLYPRT_1 | HRTIM_OUTR_DLYPRT_0)                         /*!< Timers D, E: Balanced Idle on external Event 8 */
#define LL_HRTIM_DLYPRT_DELAYOUT1_EEV9  (HRTIM_OUTR_DLYPRT_2)                                               /*!< Timers D, E: Output 1 delayed Idle on external Event 9 */
#define LL_HRTIM_DLYPRT_DELAYOUT2_EEV9  (HRTIM_OUTR_DLYPRT_2 | HRTIM_OUTR_DLYPRT_0)                         /*!< Timers D, E: Output 2 delayed Idle on external Event 9 */
#define LL_HRTIM_DLYPRT_DELAYBOTH_EEV9  (HRTIM_OUTR_DLYPRT_2 | HRTIM_OUTR_DLYPRT_1)                         /*!< Timers D, E: Output 1 and output2 delayed Idle on external Event 9 */
#define LL_HRTIM_DLYPRT_BALANCED_EEV9   (HRTIM_OUTR_DLYPRT_2 | HRTIM_OUTR_DLYPRT_1 | HRTIM_OUTR_DLYPRT_0)   /*!< Timers D, E: Balanced Idle on external Event 9 */
/**
  * @}
  */

/** @defgroup HRTIM_EC_BURSTMODE  BURST MODE
  * @{
  * @brief Constants defining how the timer behaves during a burst mode operation.
  */
#define LL_HRTIM_BURSTMODE_MAINTAINCLOCK (uint32_t)0x000000 /*!< Timer counter clock is maintained and the timer operates normally */
#define LL_HRTIM_BURSTMODE_RESETCOUNTER  (HRTIM_BMCR_MTBM)  /*!< Timer counter clock is stopped and the counter is reset */
/**
  * @}
  */

/** @defgroup HRTIM_EC_BURSTDMA  BURST DMA
  * @{
  * @brief Constants defining the registers that can be written during a burst DMA operation.
  */
#define LL_HRTIM_BURSTDMA_NONE  ((uint32_t)0x00000000U)     /*!< No register is updated by Burst DMA accesses */

#define LL_HRTIM_BURSTDMA_MCR    (HRTIM_BDMUPR_MCR)          /*!< MCR  register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_MICR   (HRTIM_BDMUPR_MICR)         /*!< MICR register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_MDIER  (HRTIM_BDMUPR_MDIER)        /*!< MDIER register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_MCNT   (HRTIM_BDMUPR_MCNT)         /*!< MCNTR register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_MPER   (HRTIM_BDMUPR_MPER)         /*!< MPER register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_MREP   (HRTIM_BDMUPR_MREP)         /*!< MREPR register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_MCMP1  (HRTIM_BDMUPR_MCMP1)        /*!< MCMP1R register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_MCMP2  (HRTIM_BDMUPR_MCMP2)        /*!< MCMP2R register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_MCMP3  (HRTIM_BDMUPR_MCMP3)        /*!< MCMP3R register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_MCMP4  (HRTIM_BDMUPR_MCMP4)        /*!< MCMP4R register is updated by Burst DMA accesses */

#define LL_HRTIM_BURSTDMA_TIMMCR   (HRTIM_BDTUPR_TIMCR)      /*!< TIMxCR register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMICR   (HRTIM_BDTUPR_TIMICR)     /*!< TIMxICR register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMDIER  (HRTIM_BDTUPR_TIMDIER)    /*!< TIMxDIER register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMCNT   (HRTIM_BDTUPR_TIMCNT)     /*!< CNTxCR register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMPER   (HRTIM_BDTUPR_TIMPER)     /*!< PERxR register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMREP   (HRTIM_BDTUPR_TIMREP)     /*!< REPxR register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMCMP1  (HRTIM_BDTUPR_TIMCMP1)    /*!< CMP1xR register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMCMP2  (HRTIM_BDTUPR_TIMCMP2)    /*!< CMP2xR register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMCMP3  (HRTIM_BDTUPR_TIMCMP3)    /*!< CMP3xR register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMCMP4  (HRTIM_BDTUPR_TIMCMP4)    /*!< CMP4xR register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMDTR   (HRTIM_BDTUPR_TIMDTR)     /*!< DTxR register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMSET1R (HRTIM_BDTUPR_TIMSET1R)   /*!< SET1R register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMRST1R (HRTIM_BDTUPR_TIMRST1R)   /*!< RST1R register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMSET2R (HRTIM_BDTUPR_TIMSET2R)   /*!< SET2R register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMRST2R (HRTIM_BDTUPR_TIMRST2R)   /*!< RST1R register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMEEFR1 (HRTIM_BDTUPR_TIMEEFR1)   /*!< EEFxR1 register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMEEFR2 (HRTIM_BDTUPR_TIMEEFR2)   /*!< EEFxR2 register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMRSTR  (HRTIM_BDTUPR_TIMRSTR)    /*!< RSTxR register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMCHPR  (HRTIM_BDTUPR_TIMCHPR)    /*!< CHPxR register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMOUTR  (HRTIM_BDTUPR_TIMOUTR)    /*!< OUTxR register is updated by Burst DMA accesses */
#define LL_HRTIM_BURSTDMA_TIMFLTR  (HRTIM_BDTUPR_TIMFLTR)    /*!< FLTxR register is updated by Burst DMA accesses */
/**
  * @}
  */

/** @defgroup HRTIM_EC_CPPSTAT  CURRENT PUSH-PULL STATUS
  * @{
  * @brief Constants defining on which output the signal is currently applied in push-pull mode.
  */
#define LL_HRTIM_CPPSTAT_OUTPUT1   ((uint32_t) 0x00000000U) /*!< Signal applied on output 1 and output 2 forced inactive */
#define LL_HRTIM_CPPSTAT_OUTPUT2   (HRTIM_TIMISR_CPPSTAT)  /*!< Signal applied on output 2 and output 1 forced inactive */
/**
  * @}
  */

/** @defgroup HRTIM_EC_IPPSTAT  IDLE PUSH-PULL STATUS
  * @{
  * @brief Constants defining on which output the signal was applied, in push-pull mode balanced fault mode or delayed idle mode, when the protection was triggered.
  */
#define LL_HRTIM_IPPSTAT_OUTPUT1   ((uint32_t) 0x00000000U)    /*!< Protection occurred when the output 1 was active and output 2 forced inactive */
#define LL_HRTIM_IPPSTAT_OUTPUT2   (HRTIM_TIMISR_IPPSTAT)     /*!< Protection occurred when the output 2 was active and output 1 forced inactive */
/**
  * @}
  */

/** @defgroup HRTIM_EC_TIM_EEFLTR TIMER EXTERNAL EVENT FILTER
  * @{
  * @brief Constants defining the event filtering applied to external events by a timer.
  */
#define LL_HRTIM_EEFLTR_NONE             ((uint32_t)0x00000000U)
#define LL_HRTIM_EEFLTR_BLANKINGCMP1     (HRTIM_EEFR1_EE1FLTR_0)                                                                                                                           /*!< Blanking from counter reset/roll-over to Compare 1 */
#define LL_HRTIM_EEFLTR_BLANKINGCMP2     (HRTIM_EEFR1_EE1FLTR_1)                                                                                                                           /*!< Blanking from counter reset/roll-over to Compare 2 */
#define LL_HRTIM_EEFLTR_BLANKINGCMP3     (HRTIM_EEFR1_EE1FLTR_1 | HRTIM_EEFR1_EE1FLTR_0)                                                                                                   /*!< Blanking from counter reset/roll-over to Compare 3 */
#define LL_HRTIM_EEFLTR_BLANKINGCMP4     (HRTIM_EEFR1_EE1FLTR_2)                                                                                                                           /*!< Blanking from counter reset/roll-over to Compare 4 */
#define LL_HRTIM_EEFLTR_BLANKINGFLTR1    (HRTIM_EEFR1_EE1FLTR_2 | HRTIM_EEFR1_EE1FLTR_0)                                                                                                   /*!< Blanking from another timing unit: TIMFLTR1 source */
#define LL_HRTIM_EEFLTR_BLANKINGFLTR2    (HRTIM_EEFR1_EE1FLTR_2 | HRTIM_EEFR1_EE1FLTR_1)                                                                                                   /*!< Blanking from another timing unit: TIMFLTR2 source */
#define LL_HRTIM_EEFLTR_BLANKINGFLTR3    (HRTIM_EEFR1_EE1FLTR_2 | HRTIM_EEFR1_EE1FLTR_1 | HRTIM_EEFR1_EE1FLTR_0)                                                                           /*!< Blanking from another timing unit: TIMFLTR3 source */
#define LL_HRTIM_EEFLTR_BLANKINGFLTR4    (HRTIM_EEFR1_EE1FLTR_3)                                                                                                                           /*!< Blanking from another timing unit: TIMFLTR4 source */
#define LL_HRTIM_EEFLTR_BLANKINGFLTR5    (HRTIM_EEFR1_EE1FLTR_3 | HRTIM_EEFR1_EE1FLTR_0)                                                                                                   /*!< Blanking from another timing unit: TIMFLTR5 source */
#define LL_HRTIM_EEFLTR_BLANKINGFLTR6    (HRTIM_EEFR1_EE1FLTR_3 | HRTIM_EEFR1_EE1FLTR_1)                                                                                                   /*!< Blanking from another timing unit: TIMFLTR6 source */
#define LL_HRTIM_EEFLTR_BLANKINGFLTR7    (HRTIM_EEFR1_EE1FLTR_3 | HRTIM_EEFR1_EE1FLTR_1 | HRTIM_EEFR1_EE1FLTR_0)                                                                           /*!< Blanking from another timing unit: TIMFLTR7 source */
#define LL_HRTIM_EEFLTR_BLANKINGFLTR8    (HRTIM_EEFR1_EE1FLTR_3 | HRTIM_EEFR1_EE1FLTR_2)                                                                                                   /*!< Blanking from another timing unit: TIMFLTR8 source */
#define LL_HRTIM_EEFLTR_WINDOWINGCMP2    (HRTIM_EEFR1_EE1FLTR_3 | HRTIM_EEFR1_EE1FLTR_2 | HRTIM_EEFR1_EE1FLTR_0)                                                                           /*!< Windowing from counter reset/roll-over to Compare 2 */
#define LL_HRTIM_EEFLTR_WINDOWINGCMP3    (HRTIM_EEFR1_EE1FLTR_3 | HRTIM_EEFR1_EE1FLTR_2 | HRTIM_EEFR1_EE1FLTR_1)                                                                           /*!< Windowing from counter reset/roll-over to Compare 3 */
#define LL_HRTIM_EEFLTR_WINDOWINGTIM     (HRTIM_EEFR1_EE1FLTR_3 | HRTIM_EEFR1_EE1FLTR_2 | HRTIM_EEFR1_EE1FLTR_1 | HRTIM_EEFR1_EE1FLTR_3 | HRTIM_EEFR1_EE1FLTR_2 | HRTIM_EEFR1_EE1FLTR_0)  /*!< Windowing from another timing unit: TIMWIN source */
/**
  * @}
  */

/** @defgroup HRTIM_EC_TIM_LATCHSTATUS TIMER EXTERNAL EVENT LATCH STATUS
  * @{
  * @brief Constants defining whether or not the external event is memorized (latched) and generated as soon as the blanking period is completed or the window ends.
  */
#define LL_HRTIM_EELATCH_DISABLED    ((uint32_t)0x00000000U) /*!< Event is ignored if it happens during a blank, or passed through during a window */
#define LL_HRTIM_EELATCH_ENABLED     HRTIM_EEFR1_EE1LTCH     /*!< Event is latched and delayed till the end of the blanking or windowing period */
/**
  * @}
  */

/** @defgroup HRTIM_EC_DT_PRESCALER DEADTIME PRESCALER
  * @{
  * @brief Constants defining division ratio between the timer clock frequency (fHRTIM) and the deadtime generator clock (fDTG).
  */
#define LL_HRTIM_DT_PRESCALER_MUL8    ((uint32_t)0x00000000U)                                         /*!< fDTG = fHRTIM * 8 */
#define LL_HRTIM_DT_PRESCALER_MUL4    (HRTIM_DTR_DTPRSC_0)                                            /*!< fDTG = fHRTIM * 4 */
#define LL_HRTIM_DT_PRESCALER_MUL2    (HRTIM_DTR_DTPRSC_1)                                            /*!< fDTG = fHRTIM * 2 */
#define LL_HRTIM_DT_PRESCALER_DIV1    (HRTIM_DTR_DTPRSC_1 | HRTIM_DTR_DTPRSC_0)                       /*!< fDTG = fHRTIM */
#define LL_HRTIM_DT_PRESCALER_DIV2    (HRTIM_DTR_DTPRSC_2)                                            /*!< fDTG = fHRTIM / 2 */
#define LL_HRTIM_DT_PRESCALER_DIV4    (HRTIM_DTR_DTPRSC_2 | HRTIM_DTR_DTPRSC_0)                       /*!< fDTG = fHRTIM / 4 */
#define LL_HRTIM_DT_PRESCALER_DIV8    (HRTIM_DTR_DTPRSC_2 | HRTIM_DTR_DTPRSC_1)                       /*!< fDTG = fHRTIM / 8 */
#define LL_HRTIM_DT_PRESCALER_DIV16   (HRTIM_DTR_DTPRSC_2 | HRTIM_DTR_DTPRSC_1 | HRTIM_DTR_DTPRSC_0)  /*!< fDTG = fHRTIM / 16 */
/**
  * @}
  */

/** @defgroup HRTIM_EC_DT_RISING_SIGN DEADTIME RISING SIGN
  * @{
  * @brief Constants defining whether the deadtime is positive or negative (overlapping signal) on rising edge.
  */
#define LL_HRTIM_DT_RISING_POSITIVE    ((uint32_t)0x00000000U) /*!< Positive deadtime on rising edge */
#define LL_HRTIM_DT_RISING_NEGATIVE    (HRTIM_DTR_SDTR)        /*!< Negative deadtime on rising edge */
/**
  * @}
  */

/** @defgroup HRTIM_EC_DT_FALLING_SIGN DEADTIME FALLING SIGN
  * @{
  * @brief Constants defining whether the deadtime is positive or negative (overlapping signal) on falling edge.
  */
#define LL_HRTIM_DT_FALLING_POSITIVE    ((uint32_t)0x00000000U) /*!< Positive deadtime on falling edge */
#define LL_HRTIM_DT_FALLING_NEGATIVE    (HRTIM_DTR_SDTF)        /*!< Negative deadtime on falling edge */
/**
  * @}
  */

/** @defgroup HRTIM_EC_CHP_PRESCALER CHOPPER MODE PRESCALER
  * @{
  * @brief Constants defining the frequency of the generated high frequency carrier (fCHPFRQ).
  */
#define LL_HRTIM_CHP_PRESCALER_DIV16  ((uint32_t)0x00000000U)                                                                     /*!< fCHPFRQ = fHRTIM / 16  */
#define LL_HRTIM_CHP_PRESCALER_DIV32  (HRTIM_CHPR_CARFRQ_0)                                                                    /*!< fCHPFRQ = fHRTIM / 32  */
#define LL_HRTIM_CHP_PRESCALER_DIV48  (HRTIM_CHPR_CARFRQ_1)                                                                    /*!< fCHPFRQ = fHRTIM / 48  */
#define LL_HRTIM_CHP_PRESCALER_DIV64  (HRTIM_CHPR_CARFRQ_1 | HRTIM_CHPR_CARFRQ_0)                                              /*!< fCHPFRQ = fHRTIM / 64  */
#define LL_HRTIM_CHP_PRESCALER_DIV80  (HRTIM_CHPR_CARFRQ_2)                                                                    /*!< fCHPFRQ = fHRTIM / 80  */
#define LL_HRTIM_CHP_PRESCALER_DIV96  (HRTIM_CHPR_CARFRQ_2 | HRTIM_CHPR_CARFRQ_0)                                              /*!< fCHPFRQ = fHRTIM / 96  */
#define LL_HRTIM_CHP_PRESCALER_DIV112 (HRTIM_CHPR_CARFRQ_2 | HRTIM_CHPR_CARFRQ_1)                                              /*!< fCHPFRQ = fHRTIM / 112  */
#define LL_HRTIM_CHP_PRESCALER_DIV128 (HRTIM_CHPR_CARFRQ_2 | HRTIM_CHPR_CARFRQ_1 | HRTIM_CHPR_CARFRQ_0)                        /*!< fCHPFRQ = fHRTIM / 128  */
#define LL_HRTIM_CHP_PRESCALER_DIV144 (HRTIM_CHPR_CARFRQ_3)                                                                    /*!< fCHPFRQ = fHRTIM / 144  */
#define LL_HRTIM_CHP_PRESCALER_DIV160 (HRTIM_CHPR_CARFRQ_3 | HRTIM_CHPR_CARFRQ_0)                                              /*!< fCHPFRQ = fHRTIM / 160  */
#define LL_HRTIM_CHP_PRESCALER_DIV176 (HRTIM_CHPR_CARFRQ_3 | HRTIM_CHPR_CARFRQ_1)                                              /*!< fCHPFRQ = fHRTIM / 176  */
#define LL_HRTIM_CHP_PRESCALER_DIV192 (HRTIM_CHPR_CARFRQ_3 | HRTIM_CHPR_CARFRQ_1 | HRTIM_CHPR_CARFRQ_0)                        /*!< fCHPFRQ = fHRTIM / 192  */
#define LL_HRTIM_CHP_PRESCALER_DIV208 (HRTIM_CHPR_CARFRQ_3 | HRTIM_CHPR_CARFRQ_2)                                              /*!< fCHPFRQ = fHRTIM / 208  */
#define LL_HRTIM_CHP_PRESCALER_DIV224 (HRTIM_CHPR_CARFRQ_3 | HRTIM_CHPR_CARFRQ_2 | HRTIM_CHPR_CARFRQ_0)                        /*!< fCHPFRQ = fHRTIM / 224  */
#define LL_HRTIM_CHP_PRESCALER_DIV240 (HRTIM_CHPR_CARFRQ_3 | HRTIM_CHPR_CARFRQ_2 | HRTIM_CHPR_CARFRQ_1)                        /*!< fCHPFRQ = fHRTIM / 240  */
#define LL_HRTIM_CHP_PRESCALER_DIV256 (HRTIM_CHPR_CARFRQ_3 | HRTIM_CHPR_CARFRQ_2 | HRTIM_CHPR_CARFRQ_1 | HRTIM_CHPR_CARFRQ_0)  /*!< fCHPFRQ = fHRTIM / 256  */
/**
 * @}
 */

/** @defgroup HRTIM_EC_CHP_DUTYCYCLE CHOPPER MODE DUTY CYCLE
  * @{
  * @brief Constants defining the duty cycle of the generated high frequency carrier. Duty cycle can be adjusted by 1/8 step (from 0/8 up to 7/8).
  */
#define LL_HRTIM_CHP_DUTYCYCLE_0    ((uint32_t)0x00000000U)                                              /*!< Only 1st pulse is present */
#define LL_HRTIM_CHP_DUTYCYCLE_125  (HRTIM_CHPR_CARDTY_0)                                             /*!< Duty cycle of the carrier signal is 12.5 % */
#define LL_HRTIM_CHP_DUTYCYCLE_250  (HRTIM_CHPR_CARDTY_1)                                             /*!< Duty cycle of the carrier signal is 25 % */
#define LL_HRTIM_CHP_DUTYCYCLE_375  (HRTIM_CHPR_CARDTY_1 | HRTIM_CHPR_CARDTY_0)                       /*!< Duty cycle of the carrier signal is 37.5 % */
#define LL_HRTIM_CHP_DUTYCYCLE_500  (HRTIM_CHPR_CARDTY_2)                                             /*!< Duty cycle of the carrier signal is 50 % */
#define LL_HRTIM_CHP_DUTYCYCLE_625  (HRTIM_CHPR_CARDTY_2 | HRTIM_CHPR_CARDTY_0)                       /*!< Duty cycle of the carrier signal is 62.5 % */
#define LL_HRTIM_CHP_DUTYCYCLE_750  (HRTIM_CHPR_CARDTY_2 | HRTIM_CHPR_CARDTY_1)                       /*!< Duty cycle of the carrier signal is 75 % */
#define LL_HRTIM_CHP_DUTYCYCLE_875  (HRTIM_CHPR_CARDTY_2 | HRTIM_CHPR_CARDTY_1 | HRTIM_CHPR_CARDTY_0) /*!< Duty cycle of the carrier signal is 87.5 % */
/**
  * @}
  */

/** @defgroup HRTIM_EC_CHP_PULSEWIDTH CHOPPER MODE PULSE WIDTH
  * @{
  * @brief Constants defining the pulse width of the first pulse of the generated high frequency carrier.
  */
#define LL_HRTIM_CHP_PULSEWIDTH_16   ((uint32_t)0x00000000U)                                                                 /*!< tSTPW = tHRTIM x 16  */
#define LL_HRTIM_CHP_PULSEWIDTH_32   (HRTIM_CHPR_STRPW_0)                                                                 /*!< tSTPW = tHRTIM x 32  */
#define LL_HRTIM_CHP_PULSEWIDTH_48   (HRTIM_CHPR_STRPW_1)                                                                 /*!< tSTPW = tHRTIM x 48  */
#define LL_HRTIM_CHP_PULSEWIDTH_64   (HRTIM_CHPR_STRPW_1 | HRTIM_CHPR_STRPW_0)                                            /*!< tSTPW = tHRTIM x 64  */
#define LL_HRTIM_CHP_PULSEWIDTH_80   (HRTIM_CHPR_STRPW_2)                                                                 /*!< tSTPW = tHRTIM x 80  */
#define LL_HRTIM_CHP_PULSEWIDTH_96   (HRTIM_CHPR_STRPW_2 | HRTIM_CHPR_STRPW_0)                                            /*!< tSTPW = tHRTIM x 96  */
#define LL_HRTIM_CHP_PULSEWIDTH_112  (HRTIM_CHPR_STRPW_2 | HRTIM_CHPR_STRPW_1)                                            /*!< tSTPW = tHRTIM x 112  */
#define LL_HRTIM_CHP_PULSEWIDTH_128  (HRTIM_CHPR_STRPW_2 | HRTIM_CHPR_STRPW_1 | HRTIM_CHPR_STRPW_0)                       /*!< tSTPW = tHRTIM x 128  */
#define LL_HRTIM_CHP_PULSEWIDTH_144  (HRTIM_CHPR_STRPW_3)                                                                 /*!< tSTPW = tHRTIM x 144  */
#define LL_HRTIM_CHP_PULSEWIDTH_160  (HRTIM_CHPR_STRPW_3 | HRTIM_CHPR_STRPW_0)                                            /*!< tSTPW = tHRTIM x 160  */
#define LL_HRTIM_CHP_PULSEWIDTH_176  (HRTIM_CHPR_STRPW_3 | HRTIM_CHPR_STRPW_1)                                            /*!< tSTPW = tHRTIM x 176  */
#define LL_HRTIM_CHP_PULSEWIDTH_192  (HRTIM_CHPR_STRPW_3 | HRTIM_CHPR_STRPW_1 | HRTIM_CHPR_STRPW_0)                       /*!< tSTPW = tHRTIM x 192  */
#define LL_HRTIM_CHP_PULSEWIDTH_208  (HRTIM_CHPR_STRPW_3 | HRTIM_CHPR_STRPW_2)                                            /*!< tSTPW = tHRTIM x 208  */
#define LL_HRTIM_CHP_PULSEWIDTH_224  (HRTIM_CHPR_STRPW_3 | HRTIM_CHPR_STRPW_2 | HRTIM_CHPR_STRPW_0)                       /*!< tSTPW = tHRTIM x 224  */
#define LL_HRTIM_CHP_PULSEWIDTH_240  (HRTIM_CHPR_STRPW_3 | HRTIM_CHPR_STRPW_2 | HRTIM_CHPR_STRPW_1)                       /*!< tSTPW = tHRTIM x 240  */
#define LL_HRTIM_CHP_PULSEWIDTH_256  (HRTIM_CHPR_STRPW_3 | HRTIM_CHPR_STRPW_2 | HRTIM_CHPR_STRPW_1 | HRTIM_CHPR_STRPW_0)  /*!< tSTPW = tHRTIM x 256  */
/**
  * @}
  */

/** @defgroup HRTIM_EC_CROSSBAR_INPUT CROSSBAR INPUT
  * @{
  * @brief Constants defining the events that can be selected to configure the set/reset crossbar of a timer output.
  */
#define LL_HRTIM_CROSSBAR_NONE       ((uint32_t)0x00000000U) /*!< Reset the output set crossbar */
#define LL_HRTIM_CROSSBAR_RESYNC     (HRTIM_SET1R_RESYNC)    /*!< Timer reset event coming solely from software or SYNC input forces an output level transision */
#define LL_HRTIM_CROSSBAR_TIMPER     (HRTIM_SET1R_PER)       /*!< Timer period event forces an output level transision */
#define LL_HRTIM_CROSSBAR_TIMCMP1    (HRTIM_SET1R_CMP1)      /*!< Timer compare 1 event forces an output level transision */
#define LL_HRTIM_CROSSBAR_TIMCMP2    (HRTIM_SET1R_CMP2)      /*!< Timer compare 2 event forces an output level transision */
#define LL_HRTIM_CROSSBAR_TIMCMP3    (HRTIM_SET1R_CMP3)      /*!< Timer compare 3 event forces an output level transision */
#define LL_HRTIM_CROSSBAR_TIMCMP4    (HRTIM_SET1R_CMP4)      /*!< Timer compare 4 event forces an output level transision */
#define LL_HRTIM_CROSSBAR_MASTERPER  (HRTIM_SET1R_MSTPER)    /*!< The master timer period event forces an output level transision */
#define LL_HRTIM_CROSSBAR_MASTERCMP1 (HRTIM_SET1R_MSTCMP1)   /*!< Master Timer compare 1 event forces an output level transision */
#define LL_HRTIM_CROSSBAR_MASTERCMP2 (HRTIM_SET1R_MSTCMP2)   /*!< Master Timer compare 2 event forces an output level transision */
#define LL_HRTIM_CROSSBAR_MASTERCMP3 (HRTIM_SET1R_MSTCMP3)   /*!< Master Timer compare 3 event forces an output level transision */
#define LL_HRTIM_CROSSBAR_MASTERCMP4 (HRTIM_SET1R_MSTCMP4)   /*!< Master Timer compare 4 event forces an output level transision */
#define LL_HRTIM_CROSSBAR_TIMEV_1    (HRTIM_SET1R_TIMEVNT1)  /*!< Timer event 1 forces an output level transision */
#define LL_HRTIM_CROSSBAR_TIMEV_2    (HRTIM_SET1R_TIMEVNT2)  /*!< Timer event 2 forces an output level transision */
#define LL_HRTIM_CROSSBAR_TIMEV_3    (HRTIM_SET1R_TIMEVNT3)  /*!< Timer event 3 forces an output level transision */
#define LL_HRTIM_CROSSBAR_TIMEV_4    (HRTIM_SET1R_TIMEVNT4)  /*!< Timer event 4 forces an output level transision */
#define LL_HRTIM_CROSSBAR_TIMEV_5    (HRTIM_SET1R_TIMEVNT5)  /*!< Timer event 5 forces an output level transision */
#define LL_HRTIM_CROSSBAR_TIMEV_6    (HRTIM_SET1R_TIMEVNT6)  /*!< Timer event 6 forces an output level transision */
#define LL_HRTIM_CROSSBAR_TIMEV_7    (HRTIM_SET1R_TIMEVNT7)  /*!< Timer event 7 forces an output level transision */
#define LL_HRTIM_CROSSBAR_TIMEV_8    (HRTIM_SET1R_TIMEVNT8)  /*!< Timer event 8 forces an output level transision */
#define LL_HRTIM_CROSSBAR_TIMEV_9    (HRTIM_SET1R_TIMEVNT9)  /*!< Timer event 9 forces an output level transision */
#define LL_HRTIM_CROSSBAR_EEV_1      (HRTIM_SET1R_EXTVNT1)   /*!< External event 1 forces an output level transision */
#define LL_HRTIM_CROSSBAR_EEV_2      (HRTIM_SET1R_EXTVNT2)   /*!< External event 2 forces an output level transision */
#define LL_HRTIM_CROSSBAR_EEV_3      (HRTIM_SET1R_EXTVNT3)   /*!< External event 3 forces an output level transision */
#define LL_HRTIM_CROSSBAR_EEV_4      (HRTIM_SET1R_EXTVNT4)   /*!< External event 4 forces an output level transision */
#define LL_HRTIM_CROSSBAR_EEV_5      (HRTIM_SET1R_EXTVNT5)   /*!< External event 5 forces an output level transision */
#define LL_HRTIM_CROSSBAR_EEV_6      (HRTIM_SET1R_EXTVNT6)   /*!< External event 6 forces an output level transision */
#define LL_HRTIM_CROSSBAR_EEV_7      (HRTIM_SET1R_EXTVNT7)   /*!< External event 7 forces an output level transision */
#define LL_HRTIM_CROSSBAR_EEV_8      (HRTIM_SET1R_EXTVNT8)   /*!< External event 8 forces an output level transision */
#define LL_HRTIM_CROSSBAR_EEV_9      (HRTIM_SET1R_EXTVNT9)   /*!< External event 9 forces an output level transision */
#define LL_HRTIM_CROSSBAR_EEV_10     (HRTIM_SET1R_EXTVNT10)  /*!< External event 10 forces an output level transision */
#define LL_HRTIM_CROSSBAR_UPDATE     (HRTIM_SET1R_UPDATE)    /*!< Timer register update event forces an output level transision */
/**
  * @}
  */

/** @defgroup HRTIM_EC_OUT_POLARITY OUPUT_POLARITY
  * @{
  * @brief Constants defining the polarity of a timer output.
  */
#define LL_HRTIM_OUT_POSITIVE_POLARITY    ((uint32_t)0x00000000U) /*!< Output is acitve HIGH */
#define LL_HRTIM_OUT_NEGATIVE_POLARITY     (HRTIM_OUTR_POL1)       /*!< Output is active LOW */
/**
  * @}
  */

/** @defgroup HRTIM_EC_OUT_IDLEMODE OUTPUT IDLE MODE
  * @{
  * @brief Constants defining whether or not the timer output transition to its IDLE state when burst mode is entered.
  */
#define LL_HRTIM_OUT_NO_IDLE             ((uint32_t)0x00000000U)/*!< The output is not affected by the burst mode operation */
#define LL_HRTIM_OUT_IDLE_WHEN_BURST     (HRTIM_OUTR_IDLM1)     /*!< The output is in idle state when requested by the burst mode controller */
/**
 * @}
 */

/** @defgroup HRTIM_EC_OUT_IDLELEVEL OUTPUT IDLE LEVEL
  * @{
  * @brief Constants defining the output level when output is in IDLE state
  */
#define LL_HRTIM_OUT_IDLELEVEL_INACTIVE   ((uint32_t)0x00000000U)/*!< Output at inactive level when in IDLE state */
#define LL_HRTIM_OUT_IDLELEVEL_ACTIVE     (HRTIM_OUTR_IDLES1)   /*!< Output at active level when in IDLE state */
/**
  * @}
  */

/** @defgroup HRTIM_EC_OUT_FAULTSTATE OUTPUT FAULT STATE
 * @{
 * @brief Constants defining the output level when output is in FAULT state.
 */
#define LL_HRTIM_OUT_FAULTSTATE_NO_ACTION ((uint32_t)0x00000000U)                      /*!< The output is not affected by the fault input */
#define LL_HRTIM_OUT_FAULTSTATE_ACTIVE    (HRTIM_OUTR_FAULT1_0)                        /*!< Output at active level when in FAULT state */
#define LL_HRTIM_OUT_FAULTSTATE_INACTIVE  (HRTIM_OUTR_FAULT1_1)                        /*!< Output at inactive level when in FAULT state */
#define LL_HRTIM_OUT_FAULTSTATE_HIGHZ     (HRTIM_OUTR_FAULT1_1 | HRTIM_OUTR_FAULT1_0)  /*!< Output is tri-stated when in FAULT state */
/**
  * @}
  */

/** @defgroup HRTIM_EC_OUT_CHOPPERMODE OUTPUT CHOPPER MODE
  * @{
  * @brief Constants defining whether or not chopper mode is enabled for a timer output.
  */
#define LL_HRTIM_OUT_CHOPPERMODE_DISABLED   ((uint32_t)0x00000000U) /*!< Output signal is not altered  */
#define LL_HRTIM_OUT_CHOPPERMODE_ENABLED    (HRTIM_OUTR_CHP1)       /*!< Output signal is chopped by a carrier signal  */
/**
  * @}
  */

/** @defgroup HRTIM_EC_OUT_BM_ENTRYMODE OUTPUT BURST MODE ENTRY MODE
  * @{
  * @brief Constants defining the idle state entry mode during a burst mode operation. It is possible to delay the burst mode entry and force the output to an inactive state
during a programmable period before the output takes its idle state.
  */
#define LL_HRTIM_OUT_BM_ENTRYMODE_REGULAR   ((uint32_t)0x00000000U)/*!< The programmed Idle state is applied immediately to the Output */
#define LL_HRTIM_OUT_BM_ENTRYMODE_DELAYED   (HRTIM_OUTR_DIDL1)     /*!< Deadtime is inserted on output before entering the idle mode */
/**
  * @}
  */
/** @defgroup HRTIM_EC_OUT_LEVEL OUTPUT LEVEL
  * @{
  * @brief Constants defining the level of a timer output.
  */
#define LL_HRTIM_OUT_LEVEL_INACTIVE   ((uint32_t)0x00000000U)/*!< Corresponds to a logic level 0 for a positive polarity (High) and to a logic level 1 for a negative polarity (Low) */
#define LL_HRTIM_OUT_LEVEL_ACTIVE     ((uint32_t)0x00000001) /*!< Corresponds to a logic level 1 for a positive polarity (High) and to a logic level 0 for a negative polarity (Low) */
/**
  * @}
  */

/** @defgroup HRTIM_EC_EE_SRC EXTERNAL EVENT SOURCE
  * @{
  * @brief Constants defining available sources associated to external events.
  */
#define LL_HRTIM_EE_SRC_1         ((uint32_t)0x00000000U)                        /*!< External event source 1 (EExSrc1)*/
#define LL_HRTIM_EE_SRC_2         (HRTIM_EECR1_EE1SRC_0)                         /*!< External event source 2 (EExSrc2) */
#define LL_HRTIM_EE_SRC_3         (HRTIM_EECR1_EE1SRC_1)                         /*!< External event source 3 (EExSrc3) */
#define LL_HRTIM_EE_SRC_4         (HRTIM_EECR1_EE1SRC_1 | HRTIM_EECR1_EE1SRC_0)  /*!< External event source 4 (EExSrc4) */
/**
  * @}
  */
/** @defgroup HRTIM_EC_EE_POLARITY EXTERNAL EVENT POLARITY
  * @{
  * @brief Constants defining the polarity of an external event.
  */
#define LL_HRTIM_EE_POLARITY_HIGH    ((uint32_t)0x00000000U) /*!< External event is active high */
#define LL_HRTIM_EE_POLARITY_LOW     (HRTIM_EECR1_EE1POL)    /*!< External event is active low */
/**
  * @}
  */

/** @defgroup HRTIM_EC_EE_SENSITIVITY EXTERNAL EVENT SENSITIVITY
  * @{
  * @brief Constants defining the sensitivity (level-sensitive or edge-sensitive) of an external event.
  */
#define LL_HRTIM_EE_SENSITIVITY_LEVEL          ((uint32_t)0x00000000U)                        /*!< External event is active on level */
#define LL_HRTIM_EE_SENSITIVITY_RISINGEDGE     (HRTIM_EECR1_EE1SNS_0)                         /*!< External event is active on Rising edge */
#define LL_HRTIM_EE_SENSITIVITY_FALLINGEDGE    (HRTIM_EECR1_EE1SNS_1)                         /*!< External event is active on Falling edge */
#define LL_HRTIM_EE_SENSITIVITY_BOTHEDGES      (HRTIM_EECR1_EE1SNS_1 | HRTIM_EECR1_EE1SNS_0)  /*!< External event is active on Rising and Falling edges */
/**
  * @}
  */

/** @defgroup HRTIM_EC_EE_FASTMODE EXTERNAL EVENT FAST MODE
  * @{
  * @brief Constants defining whether or not an external event is programmed in fast mode.
  */
#define LL_HRTIM_EE_FASTMODE_DISABLE         ((uint32_t)0x00000000U)  /*!< External Event is re-synchronized by the HRTIM logic before acting on outputs */
#define LL_HRTIM_EE_FASTMODE_ENABLE          (HRTIM_EECR1_EE1FAST)    /*!< External Event is acting asynchronously on outputs (low latency mode) */
/**
  * @}
  */

/** @defgroup HRTIM_EC_EE_FILTER EXTERNAL EVENT DIGITAL FILTER
  * @{
  * @brief Constants defining the frequency used to sample an external event input (fSAMPLING) and the length (N) of the digital filter applied.
  */
#define LL_HRTIM_EE_FILTER_NONE      ((uint32_t)0x00000000U)                                                               /*!< Filter disabled */
#define LL_HRTIM_EE_FILTER_1         (HRTIM_EECR3_EE6F_0)                                                                  /*!< fSAMPLING = fHRTIM, N=2 */
#define LL_HRTIM_EE_FILTER_2         (HRTIM_EECR3_EE6F_1)                                                                  /*!< fSAMPLING = fHRTIM, N=4 */
#define LL_HRTIM_EE_FILTER_3         (HRTIM_EECR3_EE6F_1 | HRTIM_EECR3_EE6F_0)                                             /*!< fSAMPLING = fHRTIM, N=8 */
#define LL_HRTIM_EE_FILTER_4         (HRTIM_EECR3_EE6F_2)                                                                  /*!< fSAMPLING = fEEVS/2, N=6 */
#define LL_HRTIM_EE_FILTER_5         (HRTIM_EECR3_EE6F_2 | HRTIM_EECR3_EE6F_0)                                             /*!< fSAMPLING = fEEVS/2, N=8 */
#define LL_HRTIM_EE_FILTER_6         (HRTIM_EECR3_EE6F_2 | HRTIM_EECR3_EE6F_1)                                             /*!< fSAMPLING = fEEVS/4, N=6 */
#define LL_HRTIM_EE_FILTER_7         (HRTIM_EECR3_EE6F_2 | HRTIM_EECR3_EE6F_1 | HRTIM_EECR3_EE6F_0)                        /*!< fSAMPLING = fEEVS/4, N=8 */
#define LL_HRTIM_EE_FILTER_8         (HRTIM_EECR3_EE6F_3)                                                                  /*!< fSAMPLING = fEEVS/8, N=6 */
#define LL_HRTIM_EE_FILTER_9         (HRTIM_EECR3_EE6F_3 | HRTIM_EECR3_EE6F_0)                                             /*!< fSAMPLING = fEEVS/8, N=8 */
#define LL_HRTIM_EE_FILTER_10        (HRTIM_EECR3_EE6F_3 | HRTIM_EECR3_EE6F_1)                                             /*!< fSAMPLING = fEEVS/16, N=5 */
#define LL_HRTIM_EE_FILTER_11        (HRTIM_EECR3_EE6F_3 | HRTIM_EECR3_EE6F_1 | HRTIM_EECR3_EE6F_0)                        /*!< fSAMPLING = fEEVS/16, N=6 */
#define LL_HRTIM_EE_FILTER_12        (HRTIM_EECR3_EE6F_3 | HRTIM_EECR3_EE6F_2)                                             /*!< fSAMPLING = fEEVS/16, N=8 */
#define LL_HRTIM_EE_FILTER_13        (HRTIM_EECR3_EE6F_3 | HRTIM_EECR3_EE6F_2  | HRTIM_EECR3_EE6F_0)                       /*!< fSAMPLING = fEEVS/32, N=5 */
#define LL_HRTIM_EE_FILTER_14        (HRTIM_EECR3_EE6F_3 | HRTIM_EECR3_EE6F_2  | HRTIM_EECR3_EE6F_1)                       /*!< fSAMPLING = fEEVS/32, N=6 */
#define LL_HRTIM_EE_FILTER_15        (HRTIM_EECR3_EE6F_3 | HRTIM_EECR3_EE6F_2  | HRTIM_EECR3_EE6F_1 | HRTIM_EECR3_EE6F_0)  /*!< fSAMPLING = fEEVS/32, N=8 */
/**
  * @}
  */

/** @defgroup HRTIM_EC_EE_PRESCALER EXTERNAL EVENT PRESCALER
  * @{
  * @brief Constants defining division ratio between the timer clock frequency (fHRTIM) and the external event signal sampling clock (fEEVS) used by the digital filters.
  */
#define LL_HRTIM_EE_PRESCALER_DIV1    ((uint32_t)0x00000000U)                     /*!< fEEVS = fHRTIM */
#define LL_HRTIM_EE_PRESCALER_DIV2    (HRTIM_EECR3_EEVSD_0)                       /*!< fEEVS = fHRTIM / 2 */
#define LL_HRTIM_EE_PRESCALER_DIV4    (HRTIM_EECR3_EEVSD_1)                       /*!< fEEVS = fHRTIM / 4 */
#define LL_HRTIM_EE_PRESCALER_DIV8    (HRTIM_EECR3_EEVSD_1 | HRTIM_EECR3_EEVSD_0) /*!< fEEVS = fHRTIM / 8 */
/**
  * @}
  */

/** @defgroup HRTIM_EC_FLT_SRC FAULT SOURCE
  * @{
  * @brief Constants defining whether a faults is be triggered by any external or internal fault source.
  */
#define LL_HRTIM_FLT_SRC_DIGITALINPUT      ((uint32_t)0x00000000U)    /*!< Fault input is FLT input pin */
#define LL_HRTIM_FLT_SRC_INTERNAL          (HRTIM_FLTINR1_FLT1SRC)    /*!< Fault input is FLT_Int signal (e.g. internal comparator) */
/**
  * @}
  */

/** @defgroup HRTIM_EC_FLT_POLARITY FAULT POLARITY
  * @{
  * @brief Constants defining the polarity of a fault event.
  */
#define LL_HRTIM_FLT_POLARITY_LOW     ((uint32_t)0x00000000U)  /*!< Fault input is active low */
#define LL_HRTIM_FLT_POLARITY_HIGH    (HRTIM_FLTINR1_FLT1P)    /*!< Fault input is active high */
/**
  * @}
  */

/** @defgroup HRTIM_EC_FLT_FILTER FAULT DIGITAL FILTER
  * @{
  * @brief Constants defining the frequency used to sample the fault input (fSAMPLING) and the length (N) of the digital filter applied.
  */
#define LL_HRTIM_FLT_FILTER_NONE      ((uint32_t)0x00000000U)                                                                          /*!< Filter disabled */
#define LL_HRTIM_FLT_FILTER_1         (HRTIM_FLTINR1_FLT1F_0)                                                                          /*!< fSAMPLING= fHRTIM, N=2 */
#define LL_HRTIM_FLT_FILTER_2         (HRTIM_FLTINR1_FLT1F_1)                                                                          /*!< fSAMPLING= fHRTIM, N=4 */
#define LL_HRTIM_FLT_FILTER_3         (HRTIM_FLTINR1_FLT1F_1 | HRTIM_FLTINR1_FLT1F_0)                                                  /*!< fSAMPLING= fHRTIM, N=8 */
#define LL_HRTIM_FLT_FILTER_4         (HRTIM_FLTINR1_FLT1F_2)                                                                          /*!< fSAMPLING= fFLTS/2, N=6 */
#define LL_HRTIM_FLT_FILTER_5         (HRTIM_FLTINR1_FLT1F_2 | HRTIM_FLTINR1_FLT1F_0)                                                  /*!< fSAMPLING= fFLTS/2, N=8 */
#define LL_HRTIM_FLT_FILTER_6         (HRTIM_FLTINR1_FLT1F_2 | HRTIM_FLTINR1_FLT1F_1)                                                  /*!< fSAMPLING= fFLTS/4, N=6 */
#define LL_HRTIM_FLT_FILTER_7         (HRTIM_FLTINR1_FLT1F_2 | HRTIM_FLTINR1_FLT1F_1 | HRTIM_FLTINR1_FLT1F_0)                          /*!< fSAMPLING= fFLTS/4, N=8 */
#define LL_HRTIM_FLT_FILTER_8         (HRTIM_FLTINR1_FLT1F_3)                                                                          /*!< fSAMPLING= fFLTS/8, N=6 */
#define LL_HRTIM_FLT_FILTER_9         (HRTIM_FLTINR1_FLT1F_3 | HRTIM_FLTINR1_FLT1F_0)                                                  /*!< fSAMPLING= fFLTS/8, N=8 */
#define LL_HRTIM_FLT_FILTER_10        (HRTIM_FLTINR1_FLT1F_3 | HRTIM_FLTINR1_FLT1F_1)                                                  /*!< fSAMPLING= fFLTS/16, N=5 */
#define LL_HRTIM_FLT_FILTER_11        (HRTIM_FLTINR1_FLT1F_3 | HRTIM_FLTINR1_FLT1F_1 | HRTIM_FLTINR1_FLT1F_0)                          /*!< fSAMPLING= fFLTS/16, N=6 */
#define LL_HRTIM_FLT_FILTER_12        (HRTIM_FLTINR1_FLT1F_3 | HRTIM_FLTINR1_FLT1F_2)                                                  /*!< fSAMPLING= fFLTS/16, N=8 */
#define LL_HRTIM_FLT_FILTER_13        (HRTIM_FLTINR1_FLT1F_3 | HRTIM_FLTINR1_FLT1F_2 | HRTIM_FLTINR1_FLT1F_0)                          /*!< fSAMPLING= fFLTS/32, N=5 */
#define LL_HRTIM_FLT_FILTER_14        (HRTIM_FLTINR1_FLT1F_3 | HRTIM_FLTINR1_FLT1F_2 | HRTIM_FLTINR1_FLT1F_1)                          /*!< fSAMPLING= fFLTS/32, N=6 */
#define LL_HRTIM_FLT_FILTER_15        (HRTIM_FLTINR1_FLT1F_3 | HRTIM_FLTINR1_FLT1F_2 | HRTIM_FLTINR1_FLT1F_1 | HRTIM_FLTINR1_FLT1F_0)  /*!< fSAMPLING= fFLTS/32, N=8 */
/**
  * @}
  */

/** @defgroup HRTIM_EC_FLT_PRESCALER BURST FAULT PRESCALER
  * @{
  * @brief Constants defining the division ratio between the timer clock frequency (fHRTIM) and the fault signal sampling clock (fFLTS) used  by the digital filters.
  */
#define LL_HRTIM_FLT_PRESCALER_DIV1    ((uint32_t)0x00000000U)                         /*!< fFLTS = fHRTIM */
#define LL_HRTIM_FLT_PRESCALER_DIV2    (HRTIM_FLTINR2_FLTSD_0)                         /*!< fFLTS = fHRTIM / 2 */
#define LL_HRTIM_FLT_PRESCALER_DIV4    (HRTIM_FLTINR2_FLTSD_1)                         /*!< fFLTS = fHRTIM / 4 */
#define LL_HRTIM_FLT_PRESCALER_DIV8    (HRTIM_FLTINR2_FLTSD_1 | HRTIM_FLTINR2_FLTSD_0) /*!< fFLTS = fHRTIM / 8 */
/**
  * @}
  */

/** @defgroup HRTIM_EC_BM_MODE BURST MODE OPERATING MODE
  * @{
  * @brief Constants defining if the burst mode is entered once or if it is continuously operating.
  */
#define LL_HRTIM_BM_MODE_SINGLESHOT ((uint32_t)0x00000000U) /*!< Burst mode operates in single shot mode */
#define LL_HRTIM_BM_MODE_CONTINOUS   (HRTIM_BMCR_BMOM)      /*!< Burst mode operates in continuous mode */
/**
  * @}
  */

/** @defgroup HRTIM_EC_BM_CLKSRC BURST MODE CLOCK SOURCE
  * @{
  * @brief Constants defining the clock source for the burst mode counter.
  */
#define LL_HRTIM_BM_CLKSRC_MASTER     ((uint32_t)0x00000000U)                                         /*!< Master timer counter reset/roll-over is used as clock source for the burst mode counter */
#define LL_HRTIM_BM_CLKSRC_TIMER_A    (HRTIM_BMCR_BMCLK_0)                                            /*!< Timer A counter reset/roll-over is used as clock source for the burst mode counter */
#define LL_HRTIM_BM_CLKSRC_TIMER_B    (HRTIM_BMCR_BMCLK_1)                                            /*!< Timer B counter reset/roll-over is used as clock source for the burst mode counter */
#define LL_HRTIM_BM_CLKSRC_TIMER_C    (HRTIM_BMCR_BMCLK_1 | HRTIM_BMCR_BMCLK_0)                       /*!< Timer C counter reset/roll-over is used as clock source for the burst mode counter */
#define LL_HRTIM_BM_CLKSRC_TIMER_D    (HRTIM_BMCR_BMCLK_2)                                            /*!< Timer D counter reset/roll-over is used as clock source for the burst mode counter */
#define LL_HRTIM_BM_CLKSRC_TIMER_E    (HRTIM_BMCR_BMCLK_2 | HRTIM_BMCR_BMCLK_0)                       /*!< Timer E counter reset/roll-over is used as clock source for the burst mode counter */
#define LL_HRTIM_BM_CLKSRC_TIM16_OC   (HRTIM_BMCR_BMCLK_2 | HRTIM_BMCR_BMCLK_1)                       /*!< On-chip Event 1 (BMClk[1]), acting as a burst mode counter clock */
#define LL_HRTIM_BM_CLKSRC_TIM17_OC   (HRTIM_BMCR_BMCLK_2 | HRTIM_BMCR_BMCLK_1 | HRTIM_BMCR_BMCLK_0)  /*!< On-chip Event 2 (BMClk[2]), acting as a burst mode counter clock */
#define LL_HRTIM_BM_CLKSRC_TIM7_TRGO  (HRTIM_BMCR_BMCLK_3)                                            /*!< On-chip Event 3 (BMClk[3]), acting as a burst mode counter clock */
#define LL_HRTIM_BM_CLKSRC_FHRTIM     (HRTIM_BMCR_BMCLK_3 | HRTIM_BMCR_BMCLK_1)                       /*!< Prescaled fHRTIM clock is used as clock source for the burst mode counter */
/**
  * @}
  */

/** @defgroup HRTIM_EC_BM_PRESCALER BURST MODE PRESCALER
  * @{
  * @brief Constants defining the prescaling ratio of the fHRTIM clock for the burst mode controller (fBRST).
  */
#define LL_HRTIM_BM_PRESCALER_DIV1     ((uint32_t)0x00000000U)                                                                 /*!< fBRST = fHRTIM */
#define LL_HRTIM_BM_PRESCALER_DIV2     (HRTIM_BMCR_BMPRSC_0)                                                                   /*!< fBRST = fHRTIM/2 */
#define LL_HRTIM_BM_PRESCALER_DIV4     (HRTIM_BMCR_BMPRSC_1)                                                                   /*!< fBRST = fHRTIM/4 */
#define LL_HRTIM_BM_PRESCALER_DIV8     (HRTIM_BMCR_BMPRSC_1 | HRTIM_BMCR_BMPRSC_0)                                             /*!< fBRST = fHRTIM/8 */
#define LL_HRTIM_BM_PRESCALER_DIV16    (HRTIM_BMCR_BMPRSC_2)                                                                   /*!< fBRST = fHRTIM/16 */
#define LL_HRTIM_BM_PRESCALER_DIV32    (HRTIM_BMCR_BMPRSC_2 | HRTIM_BMCR_BMPRSC_0)                                             /*!< fBRST = fHRTIM/32 */
#define LL_HRTIM_BM_PRESCALER_DIV64    (HRTIM_BMCR_BMPRSC_2 | HRTIM_BMCR_BMPRSC_1)                                             /*!< fBRST = fHRTIM/64 */
#define LL_HRTIM_BM_PRESCALER_DIV128   (HRTIM_BMCR_BMPRSC_2 | HRTIM_BMCR_BMPRSC_1 | HRTIM_BMCR_BMPRSC_0)                       /*!< fBRST = fHRTIM/128 */
#define LL_HRTIM_BM_PRESCALER_DIV256   (HRTIM_BMCR_BMPRSC_3)                                                                   /*!< fBRST = fHRTIM/256 */
#define LL_HRTIM_BM_PRESCALER_DIV512   (HRTIM_BMCR_BMPRSC_3 | HRTIM_BMCR_BMPRSC_0)                                             /*!< fBRST = fHRTIM/512 */
#define LL_HRTIM_BM_PRESCALER_DIV1024  (HRTIM_BMCR_BMPRSC_3 | HRTIM_BMCR_BMPRSC_1)                                             /*!< fBRST = fHRTIM/1024 */
#define LL_HRTIM_BM_PRESCALER_DIV2048  (HRTIM_BMCR_BMPRSC_3 | HRTIM_BMCR_BMPRSC_1 | HRTIM_BMCR_BMPRSC_0)                       /*!< fBRST = fHRTIM/2048*/
#define LL_HRTIM_BM_PRESCALER_DIV4096  (HRTIM_BMCR_BMPRSC_3 | HRTIM_BMCR_BMPRSC_2)                                             /*!< fBRST = fHRTIM/4096 */
#define LL_HRTIM_BM_PRESCALER_DIV8192  (HRTIM_BMCR_BMPRSC_3 | HRTIM_BMCR_BMPRSC_2 | HRTIM_BMCR_BMPRSC_0)                       /*!< fBRST = fHRTIM/8192 */
#define LL_HRTIM_BM_PRESCALER_DIV16384 (HRTIM_BMCR_BMPRSC_3 | HRTIM_BMCR_BMPRSC_2 | HRTIM_BMCR_BMPRSC_1)                       /*!< fBRST = fHRTIM/16384 */
#define LL_HRTIM_BM_PRESCALER_DIV32768 (HRTIM_BMCR_BMPRSC_3 | HRTIM_BMCR_BMPRSC_2 | HRTIM_BMCR_BMPRSC_1 | HRTIM_BMCR_BMPRSC_0) /*!< fBRST = fHRTIM/32768 */
/**
  * @}
  */

/** @defgroup HRTIM_EC_BM_TRIG HRTIM BURST MODE TRIGGER
  * @{
  * @brief Constants defining the events that can be used to trig the burst mode operation.
  */
#define LL_HRTIM_BM_TRIG_NONE               (uint32_t)0x00000000    /*!<  No trigger */
#define LL_HRTIM_BM_TRIG_MASTER_RESET       (HRTIM_BMTRGR_MSTRST)   /*!<  Master timer reset event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_MASTER_REPETITION  (HRTIM_BMTRGR_MSTREP)   /*!<  Master timer repetition event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_MASTER_CMP1        (HRTIM_BMTRGR_MSTCMP1)  /*!<  Master timer compare 1 event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_MASTER_CMP2        (HRTIM_BMTRGR_MSTCMP2)  /*!<  Master timer compare 2 event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_MASTER_CMP3        (HRTIM_BMTRGR_MSTCMP3)  /*!<  Master timer compare 3 event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_MASTER_CMP4        (HRTIM_BMTRGR_MSTCMP4)  /*!<  Master timer compare 4 event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIMA_RESET         (HRTIM_BMTRGR_TARST)    /*!< Timer A reset event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIMA_REPETITION    (HRTIM_BMTRGR_TAREP)    /*!< Timer A repetition event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIMA_CMP1          (HRTIM_BMTRGR_TACMP1)   /*!< Timer A compare 1 event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIMA_CMP2          (HRTIM_BMTRGR_TACMP2)   /*!< Timer A compare 2 event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIMB_RESET         (HRTIM_BMTRGR_TBRST)    /*!< Timer B reset event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIMB_REPETITION    (HRTIM_BMTRGR_TBREP)    /*!< Timer B repetition event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIMB_CMP1          (HRTIM_BMTRGR_TBCMP1)   /*!< Timer B compare 1 event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIMB_CMP2          (HRTIM_BMTRGR_TBCMP2)   /*!< Timer B compare 2 event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIMC_RESET         (HRTIM_BMTRGR_TCRST)    /*!< Timer C resetevent is starting the burst mode operation  */
#define LL_HRTIM_BM_TRIG_TIMC_REPETITION    (HRTIM_BMTRGR_TCREP)    /*!< Timer C repetition event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIMC_CMP1          (HRTIM_BMTRGR_TCCMP1)   /*!< Timer C compare 1 event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIMC_CMP2          (HRTIM_BMTRGR_TCCMP2)   /*!< Timer C compare 2 event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIMD_RESET         (HRTIM_BMTRGR_TDRST)    /*!< Timer D reset event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIMD_REPETITION    (HRTIM_BMTRGR_TDREP)    /*!< Timer D repetition event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIMD_CMP1          (HRTIM_BMTRGR_TDCMP1)   /*!< Timer D compare 1 event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIMD_CMP2          (HRTIM_BMTRGR_TDCMP2)   /*!< Timer D compare 2 event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIME_RESET         (HRTIM_BMTRGR_TERST)    /*!< Timer E reset event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIME_REPETITION    (HRTIM_BMTRGR_TEREP)    /*!< Timer E repetition event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIME_CMP1          (HRTIM_BMTRGR_TECMP1)   /*!< Timer E compare 1 event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIME_CMP2          (HRTIM_BMTRGR_TECMP2)   /*!< Timer E compare 2 event is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_TIMA_EVENT7        (HRTIM_BMTRGR_TAEEV7)   /*!< Timer A period following an external event 7 (conditioned by TIMA filters) is starting the burst
mode operation  */
#define LL_HRTIM_BM_TRIG_TIMD_EVENT8        (HRTIM_BMTRGR_TDEEV8)   /*!< Timer D period following an external event 8 (conditioned by TIMD filters) is starting the burst
mode operation  */
#define LL_HRTIM_BM_TRIG_EVENT_7            (HRTIM_BMTRGR_EEV7)     /*!< External event 7 conditioned by TIMA filters is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_EVENT_8            (HRTIM_BMTRGR_EEV8)     /*!< External event 8 conditioned by TIMD filters is starting the burst mode operation */
#define LL_HRTIM_BM_TRIG_EVENT_ONCHIP       (HRTIM_BMTRGR_OCHPEV)   /*!< A rising edge on an on-chip Event (for instance from GP timer or comparator) triggers the burst mode
operation */
/**
  * @}
  */

/** @defgroup HRTIM_EC_BM_STATUS HRTIM BURST MODE STATUS
  * @{
  * @brief Constants defining the operating state of the burst mode controller.
  */
#define LL_HRTIM_BM_STATUS_NORMAL        ((uint32_t) 0x00000000U) /*!< Normal operation */
#define LL_HRTIM_BM_STATUS_BURST_ONGOING (HRTIM_BMCR_BMSTAT)     /*!< Burst operation on-going */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup HRTIM_LL_Exported_Macros HRTIM Exported Macros
  * @{
  */

/** @defgroup HRTIM_LL_EM_WRITE_READ Common Write and read registers Macros
  * @{
  */

/**
  * @brief  Write a value in HRTIM register
  * @param  __INSTANCE__ HRTIM Instance
  * @param  __REG__ Register to be written
  * @param  __VALUE__ Value to be written in the register
  * @retval None
  */
#define LL_HRTIM_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
  * @brief  Read a value in HRTIM register
  * @param  __INSTANCE__ HRTIM Instance
  * @param  __REG__ Register to be read
  * @retval Register value
  */
#define LL_HRTIM_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
  * @}
  */

/** @defgroup HRTIM_LL_EM_Exported_Macros Exported_Macros
  * @{
  */
/**
  * @brief  HELPER macro returning the output state from output enable/disable status
  * @param  __OUTPUT_STATUS_EN__ output enable status
  * @param  __OUTPUT_STATUS_DIS__ output Disable status
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUTSTATE_IDLE
  *         @arg @ref LL_HRTIM_OUTPUTSTATE_RUN
  *         @arg @ref LL_HRTIM_OUTPUTSTATE_FAULT
*/
#define __LL_HRTIM_GET_OUTPUT_STATE(__OUTPUT_STATUS_EN__, __OUTPUT_STATUS_DIS__)\
  (((__OUTPUT_STATUS_EN__) == 1) ?  LL_HRTIM_OUTPUTSTATE_RUN :\
   ((__OUTPUT_STATUS_DIS__) == 0) ? LL_HRTIM_OUTPUTSTATE_IDLE : LL_HRTIM_OUTPUTSTATE_FAULT)
/**
  * @}
  */


/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup HRTIM_LL_Exported_Functions HRTIM Exported Functions
  * @{
  */
/** @defgroup HRTIM_EF_HRTIM_Control HRTIM_Control
  * @{
  */

/**
  * @brief  Select the HRTIM synchronization input source.
  * @note This function must not be called when  the concerned timer(s) is (are) enabled .
  * @rmtoll MCR          SYNCIN        LL_HRTIM_SetSyncInSrc
  * @param  HRTIMx High Resolution Timer instance
  * @param  SyncInSrc This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_SYNCIN_SRC_NONE
  *         @arg @ref LL_HRTIM_SYNCIN_SRC_TIM_EVENT
  *         @arg @ref LL_HRTIM_SYNCIN_SRC_EXTERNAL_EVENT
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_SetSyncInSrc(HRTIM_TypeDef *HRTIMx, uint32_t SyncInSrc)
{
  MODIFY_REG(HRTIMx->sMasterRegs.MCR, HRTIM_MCR_SYNC_IN, SyncInSrc);
}

/**
  * @brief  Get actual HRTIM synchronization input source.
  * @rmtoll MCR          SYNCIN        LL_HRTIM_SetSyncInSrc
  * @param  HRTIMx High Resolution Timer instance
  * @retval SyncInSrc Returned value can be one of the following values:
  *         @arg @ref LL_HRTIM_SYNCIN_SRC_NONE
  *         @arg @ref LL_HRTIM_SYNCIN_SRC_TIM_EVENT
  *         @arg @ref LL_HRTIM_SYNCIN_SRC_EXTERNAL_EVENT
  */
__STATIC_INLINE uint32_t LL_HRTIM_GetSyncInSrc(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sMasterRegs.MCR, HRTIM_MCR_SYNC_IN));
}

/**
  * @brief  Configure the HRTIM synchronization output.
  * @rmtoll MCR          SYNCSRC      LL_HRTIM_ConfigSyncOut\n
  *         MCR          SYNCOUT      LL_HRTIM_ConfigSyncOut
  * @param  HRTIMx High Resolution Timer instance
  * @param  Config This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_SYNCOUT_DISABLED
  *         @arg @ref LL_HRTIM_SYNCOUT_POSITIVE_PULSE
  *         @arg @ref LL_HRTIM_SYNCOUT_NEGATIVE_PULSE
  * @param  Src This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_SYNCOUT_SRC_MASTER_START
  *         @arg @ref LL_HRTIM_SYNCOUT_SRC_MASTER_CMP1
  *         @arg @ref LL_HRTIM_SYNCOUT_SRC_TIMA_START
  *         @arg @ref LL_HRTIM_SYNCOUT_SRC_TIMA_CMP1
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ConfigSyncOut(HRTIM_TypeDef *HRTIMx, uint32_t Config, uint32_t Src)
{
  MODIFY_REG(HRTIMx->sMasterRegs.MCR, (HRTIM_MCR_SYNC_OUT | HRTIM_MCR_SYNC_SRC), (Config | Src));
}

/**
  * @brief  Set the routing and conditioning of the synchronization output event.
  * @rmtoll MCR          SYNCOUT      LL_HRTIM_SetSyncOutConfig
  * @note This function can be called only when the master timer is enabled.
  * @param  HRTIMx High Resolution Timer instance
  * @param  SyncOutConfig This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_SYNCOUT_DISABLED
  *         @arg @ref LL_HRTIM_SYNCOUT_POSITIVE_PULSE
  *         @arg @ref LL_HRTIM_SYNCOUT_NEGATIVE_PULSE
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_SetSyncOutConfig(HRTIM_TypeDef *HRTIMx, uint32_t SyncOutConfig)
{
  MODIFY_REG(HRTIMx->sMasterRegs.MCR, HRTIM_MCR_SYNC_OUT, SyncOutConfig);
}

/**
  * @brief  Get actual routing and conditioning of the synchronization output event.
  * @rmtoll MCR          SYNCOUT      LL_HRTIM_GetSyncOutConfig
  * @param  HRTIMx High Resolution Timer instance
  * @retval SyncOutConfig Returned value can be one of the following values:
  *         @arg @ref LL_HRTIM_SYNCOUT_DISABLED
  *         @arg @ref LL_HRTIM_SYNCOUT_POSITIVE_PULSE
  *         @arg @ref LL_HRTIM_SYNCOUT_NEGATIVE_PULSE
  */
__STATIC_INLINE uint32_t LL_HRTIM_GetSyncOutConfig(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sMasterRegs.MCR, HRTIM_MCR_SYNC_OUT));
}

/**
  * @brief  Set the source and event to be sent on the HRTIM synchronization output.
  * @rmtoll MCR          SYNCSRC      LL_HRTIM_SetSyncOutSrc
  * @param  HRTIMx High Resolution Timer instance
  * @param  SyncOutSrc This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_SYNCOUT_SRC_MASTER_START
  *         @arg @ref LL_HRTIM_SYNCOUT_SRC_MASTER_CMP1
  *         @arg @ref LL_HRTIM_SYNCOUT_SRC_TIMA_START
  *         @arg @ref LL_HRTIM_SYNCOUT_SRC_TIMA_CMP1
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_SetSyncOutSrc(HRTIM_TypeDef *HRTIMx, uint32_t SyncOutSrc)
{
  MODIFY_REG(HRTIMx->sMasterRegs.MCR, HRTIM_MCR_SYNC_SRC, SyncOutSrc);
}

/**
  * @brief  Get actual  source and event sent on the HRTIM synchronization output.
  * @rmtoll MCR          SYNCSRC      LL_HRTIM_GetSyncOutSrc
  * @param  HRTIMx High Resolution Timer instance
  * @retval SyncOutSrc Returned value can be one of the following values:
  *         @arg @ref LL_HRTIM_SYNCOUT_SRC_MASTER_START
  *         @arg @ref LL_HRTIM_SYNCOUT_SRC_MASTER_CMP1
  *         @arg @ref LL_HRTIM_SYNCOUT_SRC_TIMA_START
  *         @arg @ref LL_HRTIM_SYNCOUT_SRC_TIMA_CMP1
  */
__STATIC_INLINE uint32_t LL_HRTIM_GetSyncOutSrc(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sMasterRegs.MCR, HRTIM_MCR_SYNC_SRC));
}

/**
  * @brief  Disable (temporarily) update event generation.
  * @rmtoll CR1          MUDIS         LL_HRTIM_SuspendUpdate\n
  *         CR1          TAUDIS        LL_HRTIM_SuspendUpdate\n
  *         CR1          TBUDIS        LL_HRTIM_SuspendUpdate\n
  *         CR1          TCUDIS        LL_HRTIM_SuspendUpdate\n
  *         CR1          TDUDIS        LL_HRTIM_SuspendUpdate\n
  *         CR1          TEUDIS        LL_HRTIM_SuspendUpdate
  * @note Allow to temporarily disable the transfer from preload to active
  *      registers, whatever the selected update event. This allows to modify
  *      several registers in multiple timers.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timers This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_SuspendUpdate(HRTIM_TypeDef *HRTIMx, uint32_t Timers)
{
  SET_BIT(HRTIMx->sCommonRegs.CR1, ((Timers >> HRTIM_MCR_MCEN_Pos) & HRTIM_CR1_UDIS_MASK));
}

/**
  * @brief  Enable update event generation.
  * @rmtoll CR1          MUDIS         LL_HRTIM_ResumeUpdate\n
  *         CR1          TAUDIS        LL_HRTIM_ResumeUpdate\n
  *         CR1          TBUDIS        LL_HRTIM_ResumeUpdate\n
  *         CR1          TCUDIS        LL_HRTIM_ResumeUpdate\n
  *         CR1          TDUDIS        LL_HRTIM_ResumeUpdate\n
  *         CR1          TEUDIS        LL_HRTIM_ResumeUpdate
  * @note The regular update event takes place.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timers This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ResumeUpdate(HRTIM_TypeDef *HRTIMx, uint32_t Timers)
{
  CLEAR_BIT(HRTIMx->sCommonRegs.CR1, ((Timers >> HRTIM_MCR_MCEN_Pos) & HRTIM_CR1_UDIS_MASK));
}

/**
  * @brief  Force an immediate transfer from the preload to the active register .
  * @rmtoll CR2          MSWU          LL_HRTIM_ForceUpdate\n
  *         CR2          TASWU         LL_HRTIM_ForceUpdate\n
  *         CR2          TBSWU         LL_HRTIM_ForceUpdate\n
  *         CR2          TCSWU         LL_HRTIM_ForceUpdate\n
  *         CR2          TDSWU         LL_HRTIM_ForceUpdate\n
  *         CR2          TESWU         LL_HRTIM_ForceUpdate
  * @note Any pending update request is cancelled.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timers This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ForceUpdate(HRTIM_TypeDef *HRTIMx, uint32_t Timers)
{
  SET_BIT(HRTIMx->sCommonRegs.CR2, ((Timers >> HRTIM_MCR_MCEN_Pos) & HRTIM_CR2_SWUPD_MASK));
}

/**
  * @brief  Reset the HRTIM timer(s) counter.
  * @rmtoll CR2          MRST          LL_HRTIM_CounterReset\n
  *         CR2          TARST         LL_HRTIM_CounterReset\n
  *         CR2          TBRST         LL_HRTIM_CounterReset\n
  *         CR2          TCRST         LL_HRTIM_CounterReset\n
  *         CR2          TDRST         LL_HRTIM_CounterReset\n
  *         CR2          TERST         LL_HRTIM_CounterReset
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timers This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_CounterReset(HRTIM_TypeDef *HRTIMx, uint32_t Timers)
{
  SET_BIT(HRTIMx->sCommonRegs.CR2, (((Timers >> HRTIM_MCR_MCEN_Pos) << HRTIM_CR2_MRST_Pos) & HRTIM_CR2_SWRST_MASK));
}

/**
  * @brief  Enable the HRTIM timer(s) output(s) .
  * @rmtoll OENR         TA1OEN        LL_HRTIM_EnableOutput\n
  *         OENR         TA2OEN        LL_HRTIM_EnableOutput\n
  *         OENR         TB1OEN        LL_HRTIM_EnableOutput\n
  *         OENR         TB2OEN        LL_HRTIM_EnableOutput\n
  *         OENR         TC1OEN        LL_HRTIM_EnableOutput\n
  *         OENR         TC2OEN        LL_HRTIM_EnableOutput\n
  *         OENR         TD1OEN        LL_HRTIM_EnableOutput\n
  *         OENR         TD2OEN        LL_HRTIM_EnableOutput\n
  *         OENR         TE1OEN        LL_HRTIM_EnableOutput\n
  *         OENR         TE2OEN        LL_HRTIM_EnableOutput
  * @param  HRTIMx High Resolution Timer instance
  * @param  Outputs This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableOutput(HRTIM_TypeDef *HRTIMx, uint32_t Outputs)
{
  SET_BIT(HRTIMx->sCommonRegs.OENR, (Outputs & HRTIM_OENR_OEN_MASK));
}

/**
  * @brief  Disable the HRTIM timer(s) output(s) .
  * @rmtoll OENR         TA1OEN        LL_HRTIM_DisableOutput\n
  *         OENR         TA2OEN        LL_HRTIM_DisableOutput\n
  *         OENR         TB1OEN        LL_HRTIM_DisableOutput\n
  *         OENR         TB2OEN        LL_HRTIM_DisableOutput\n
  *         OENR         TC1OEN        LL_HRTIM_DisableOutput\n
  *         OENR         TC2OEN        LL_HRTIM_DisableOutput\n
  *         OENR         TD1OEN        LL_HRTIM_DisableOutput\n
  *         OENR         TD2OEN        LL_HRTIM_DisableOutput\n
  *         OENR         TE1OEN        LL_HRTIM_DisableOutput\n
  *         OENR         TE2OEN        LL_HRTIM_DisableOutput
  * @param  HRTIMx High Resolution Timer instance
  * @param  Outputs This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableOutput(HRTIM_TypeDef *HRTIMx, uint32_t Outputs)
{
  SET_BIT(HRTIMx->sCommonRegs.ODISR, (Outputs & HRTIM_OENR_ODIS_MASK));
}

/**
  * @brief  Indicates whether the HRTIM timer output is enabled.
  * @rmtoll OENR         TA1OEN        LL_HRTIM_IsEnabledOutput\n
  *         OENR         TA2OEN        LL_HRTIM_IsEnabledOutput\n
  *         OENR         TB1OEN        LL_HRTIM_IsEnabledOutput\n
  *         OENR         TB2OEN        LL_HRTIM_IsEnabledOutput\n
  *         OENR         TC1OEN        LL_HRTIM_IsEnabledOutput\n
  *         OENR         TC2OEN        LL_HRTIM_IsEnabledOutput\n
  *         OENR         TD1OEN        LL_HRTIM_IsEnabledOutput\n
  *         OENR         TD2OEN        LL_HRTIM_IsEnabledOutput\n
  *         OENR         TE1OEN        LL_HRTIM_IsEnabledOutput\n
  *         OENR         TE2OEN        LL_HRTIM_IsEnabledOutput
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @retval State of TxyOEN bit in HRTIM_OENR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledOutput(HRTIM_TypeDef *HRTIMx, uint32_t Output)
{
  return (READ_BIT(HRTIMx->sCommonRegs.OENR, Output) == Output);
}

/**
  * @brief  Indicates whether the HRTIM timer output is disabled.
  * @rmtoll ODISR        TA1ODIS        LL_HRTIM_IsDisabledOutput\n
  *         ODISR        TA2ODIS        LL_HRTIM_IsDisabledOutput\n
  *         ODISR        TB1ODIS        LL_HRTIM_IsDisabledOutput\n
  *         ODISR        TB2ODIS        LL_HRTIM_IsDisabledOutput\n
  *         ODISR        TC1ODIS        LL_HRTIM_IsDisabledOutput\n
  *         ODISR        TC2ODIS        LL_HRTIM_IsDisabledOutput\n
  *         ODISR        TD1ODIS        LL_HRTIM_IsDisabledOutput\n
  *         ODISR        TD2ODIS        LL_HRTIM_IsDisabledOutput\n
  *         ODISR        TE1ODIS        LL_HRTIM_IsDisabledOutput\n
  *         ODISR        TE2ODIS        LL_HRTIM_IsDisabledOutput
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @retval State of TxyODS bit in HRTIM_ODSR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsDisabledOutput(HRTIM_TypeDef *HRTIMx, uint32_t Output)
{
  return (READ_BIT(HRTIMx->sCommonRegs.ODISR, Output) == Output);
}

/**
  * @brief  Configure an ADC trigger.
  * @rmtoll CR1          ADC1USRC        LL_HRTIM_ConfigADCTrig\n
  *         CR1          ADC2USRC        LL_HRTIM_ConfigADCTrig\n
  *         CR1          ADC3USRC        LL_HRTIM_ConfigADCTrig\n
  *         CR1          ADC4USRC        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1MC4         LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1MPER        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1EEV1        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1EEV2        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1EEV3        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1EEV4        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1EEV5        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TAC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TAC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TAC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TAPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TARST       LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TBC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TBC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TBC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TBPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TBRST       LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TCC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TCC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TCC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TCPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TDC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TDC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TDC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TDPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TEC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TEC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TEC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC1R        ADC1TEPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2MC1         LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2MC2         LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2MC3         LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2MC4         LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2MPER        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2EEV6        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2EEV7        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2EEV8        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2EEV9        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2EEV10       LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TAC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TAC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TAC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TAPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TBC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TBC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TBC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TBPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TCC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TCC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TCC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TCPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TCRST       LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TDC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TDC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TDC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TDPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TDRST       LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TEC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TEC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TEC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC2R        ADC2TERST       LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3MC1         LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3MC2         LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3MC3         LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3MC4         LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3MPER        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3EEV1        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3EEV2        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3EEV3        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3EEV4        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3EEV5        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TAC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TAC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TAC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TAPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TARST       LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TBC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TBC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TBC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TBPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TBRST       LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TCC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TCC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TCC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TCPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TDC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TDC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TDC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TDPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TEC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TEC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TEC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC3R        ADC3TEPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4MC1         LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4MC2         LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4MC3         LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4MC4         LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4MPER        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4EEV6        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4EEV7        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4EEV8        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4EEV9        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4EEV10       LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TAC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TAC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TAC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TAPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TBC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TBC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TBC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TBPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TCC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TCC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TCC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TCPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TCRST       LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TDC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TDC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TDC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TDPER       LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TDRST       LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TEC2        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TEC3        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TEC4        LL_HRTIM_ConfigADCTrig\n
  *         ADC4R        ADC4TERST       LL_HRTIM_ConfigADCTrig
  * @param  HRTIMx High Resolution Timer instance
  * @param  ADCTrig This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_ADCTRIG_1
  *         @arg @ref LL_HRTIM_ADCTRIG_2
  *         @arg @ref LL_HRTIM_ADCTRIG_3
  *         @arg @ref LL_HRTIM_ADCTRIG_4
  * @param  Update This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_MASTER
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_TIMER_A
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_TIMER_B
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_TIMER_C
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_TIMER_D
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_TIMER_E
  * @param  Src This parameter can be a combination of the following values:
  *
  *         For ADC trigger 1 and ADC trigger 3:
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_NONE
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_MCMP1
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_MCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_MCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_MCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_MPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMACMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMACMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMACMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMAPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_EEV1
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_EEV2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_EEV3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_EEV4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_EEV5
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMARST
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMBCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMBCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMBCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMBPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMBRST
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMCCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMCCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMCCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMCPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMDCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMDCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMDCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMDPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMECMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMECMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMECMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMEPER
  *
  *         For ADC trigger 2 and ADC trigger 4:
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_NONE
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_MCMP1
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_MCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_MCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_MCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_MPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_EEV6
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_EEV7
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_EEV8
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_EEV9
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_EEV10
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMACMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMACMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMACMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMAPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMBCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMBCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMBCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMBPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMCCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMCCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMCCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMCPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMCRST
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMDCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMDCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMDCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMDPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMDRST
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMECMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMECMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMECMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMERST
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ConfigADCTrig(HRTIM_TypeDef *HRTIMx, uint32_t ADCTrig, uint32_t Update, uint32_t Src)
{
  register uint32_t shift = 3 * ADCTrig;
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.ADC1R) +
                                                    REG_OFFSET_TAB_ADCxR[ADCTrig]));
  MODIFY_REG(HRTIMx->sCommonRegs.CR1, (HRTIM_CR1_ADC1USRC << shift), (Update << shift));
  WRITE_REG(*pReg, Src);
}

/**
  * @brief  Associate the ADCx trigger to a timer triggering the update of the HRTIM_ADCxR register.
  * @rmtoll CR1          ADC1USRC         LL_HRTIM_SetADCTrigUpdate\n
  *         CR1          ADC2USRC         LL_HRTIM_SetADCTrigUpdate\n
  *         CR1          ADC3USRC         LL_HRTIM_SetADCTrigUpdate\n
  *         CR1          ADC4USRC         LL_HRTIM_SetADCTrigUpdate
  * @note When the preload is disabled in the source timer, the HRTIM_ADCxR
  *       registers are not preloaded either: a write access will result in an
  *       immediate update of the trigger source.
  * @param  HRTIMx High Resolution Timer instance
  * @param  ADCTrig This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_ADCTRIG_1
  *         @arg @ref LL_HRTIM_ADCTRIG_2
  *         @arg @ref LL_HRTIM_ADCTRIG_3
  *         @arg @ref LL_HRTIM_ADCTRIG_4
  * @param  Update This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_MASTER
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_TIMER_A
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_TIMER_B
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_TIMER_C
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_TIMER_D
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_SetADCTrigUpdate(HRTIM_TypeDef *HRTIMx, uint32_t ADCTrig, uint32_t Update)
{
  register uint32_t shift = 3 * ADCTrig;
  MODIFY_REG(HRTIMx->sCommonRegs.CR1, (HRTIM_CR1_ADC1USRC << shift), (Update << shift));
}

/**
  * @brief  Get the source timer triggering the update of the HRTIM_ADCxR register.
  * @rmtoll CR1          ADC1USRC        LL_HRTIM_GetADCTrigUpdate\n
  *         CR1          ADC2USRC        LL_HRTIM_GetADCTrigUpdate\n
  *         CR1          ADC3USRC        LL_HRTIM_GetADCTrigUpdate\n
  *         CR1          ADC4USRC        LL_HRTIM_GetADCTrigUpdate
  * @param  HRTIMx High Resolution Timer instance
  * @param  ADCTrig This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_ADCTRIG_1
  *         @arg @ref LL_HRTIM_ADCTRIG_2
  *         @arg @ref LL_HRTIM_ADCTRIG_3
  *         @arg @ref LL_HRTIM_ADCTRIG_4
  * @retval Update Returned value can be one of the following values:
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_MASTER
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_TIMER_A
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_TIMER_B
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_TIMER_C
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_TIMER_D
  *         @arg @ref LL_HRTIM_ADCTRIG_UPDATE_TIMER_E
  */
__STATIC_INLINE uint32_t LL_HRTIM_GetADCTrigUpdate(HRTIM_TypeDef *HRTIMx, uint32_t ADCTrig)
{
  register uint32_t shift = 3 * ADCTrig;
  return (READ_BIT(HRTIMx->sCommonRegs.CR1, (HRTIM_CR1_ADC1USRC << shift)) >> shift);
}

/**
  * @brief  Specify which events (timer events and/or external events) are used as triggers for ADC conversion.
  * @rmtoll ADC1R        ADC1MC4         LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1MPER        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1EEV1        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1EEV2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1EEV3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1EEV4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1EEV5        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TAC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TAC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TAC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TAPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TARST       LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TBC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TBC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TBC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TBPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TBRST       LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TCC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TCC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TCC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TCPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TDC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TDC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TDC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TDPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TEC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TEC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TEC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC1R        ADC1TEPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2MC1         LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2MC2         LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2MC3         LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2MC4         LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2MPER        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2EEV6        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2EEV7        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2EEV8        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2EEV9        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2EEV10       LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TAC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TAC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TAC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TAPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TBC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TBC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TBC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TBPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TCC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TCC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TCC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TCPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TCRST       LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TDC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TDC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TDC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TDPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TDRST       LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TEC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TEC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TEC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC2R        ADC2TERST       LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3MC1         LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3MC2         LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3MC3         LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3MC4         LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3MPER        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3EEV1        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3EEV2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3EEV3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3EEV4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3EEV5        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TAC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TAC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TAC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TAPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TARST       LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TBC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TBC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TBC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TBPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TBRST       LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TCC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TCC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TCC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TCPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TDC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TDC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TDC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TDPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TEC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TEC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TEC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC3R        ADC3TEPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4MC1         LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4MC2         LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4MC3         LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4MC4         LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4MPER        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4EEV6        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4EEV7        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4EEV8        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4EEV9        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4EEV10       LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TAC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TAC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TAC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TAPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TBC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TBC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TBC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TBPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TCC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TCC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TCC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TCPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TCRST       LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TDC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TDC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TDC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TDPER       LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TDRST       LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TEC2        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TEC3        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TEC4        LL_HRTIM_SetADCTrigSrc\n
  *         ADC4R        ADC4TERST       LL_HRTIM_SetADCTrigSrc
  * @param  HRTIMx High Resolution Timer instance
  * @param  ADCTrig This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_ADCTRIG_1
  *         @arg @ref LL_HRTIM_ADCTRIG_2
  *         @arg @ref LL_HRTIM_ADCTRIG_3
  *         @arg @ref LL_HRTIM_ADCTRIG_4
  * @param  Src This parameter can be a combination of the following values:
  *
  *         For ADC trigger 1 and ADC trigger 3:
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_NONE
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_MCMP1
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_MCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_MCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_MCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_MPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMACMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMACMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMACMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMAPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_EEV1
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_EEV2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_EEV3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_EEV4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_EEV5
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMARST
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMBCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMBCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMBCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMBPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMBRST
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMCCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMCCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMCCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMCPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMDCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMDCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMDCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMDPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMECMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMECMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMECMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMEPER
  *
  *         For ADC trigger 2 and ADC trigger 4:
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_NONE
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_MCMP1
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_MCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_MCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_MCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_MPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_EEV6
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_EEV7
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_EEV8
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_EEV9
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_EEV10
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMACMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMACMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMACMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMAPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMBCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMBCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMBCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMBPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMCCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMCCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMCCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMCPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMCRST
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMDCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMDCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMDCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMDPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMDRST
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMECMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMECMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMECMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMERST
* @retval None
  */
__STATIC_INLINE void LL_HRTIM_SetADCTrigSrc(HRTIM_TypeDef *HRTIMx, uint32_t ADCTrig, uint32_t Src)
{
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.ADC1R) +
                                                    REG_OFFSET_TAB_ADCxR[ADCTrig]));
  WRITE_REG(*pReg, Src);
}

/**
  * @brief  Indicate which events (timer events and/or external events) are currently used as triggers for ADC conversion.
  * @rmtoll ADC1R        ADC1MC4         LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1MPER        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1EEV1        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1EEV2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1EEV3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1EEV4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1EEV5        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TAC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TAC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TAC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TAPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TARST       LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TBC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TBC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TBC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TBPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TBRST       LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TCC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TCC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TCC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TCPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TDC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TDC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TDC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TDPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TEC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TEC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TEC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC1R        ADC1TEPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2MC1         LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2MC2         LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2MC3         LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2MC4         LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2MPER        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2EEV6        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2EEV7        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2EEV8        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2EEV9        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2EEV10       LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TAC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TAC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TAC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TAPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TBC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TBC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TBC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TBPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TCC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TCC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TCC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TCPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TCRST       LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TDC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TDC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TDC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TDPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TDRST       LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TEC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TEC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TEC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC2R        ADC2TERST       LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3MC1         LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3MC2         LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3MC3         LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3MC4         LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3MPER        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3EEV1        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3EEV2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3EEV3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3EEV4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3EEV5        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TAC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TAC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TAC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TAPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TARST       LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TBC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TBC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TBC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TBPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TBRST       LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TCC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TCC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TCC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TCPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TDC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TDC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TDC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TDPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TEC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TEC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TEC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC3R        ADC3TEPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4MC1         LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4MC2         LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4MC3         LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4MC4         LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4MPER        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4EEV6        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4EEV7        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4EEV8        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4EEV9        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4EEV10       LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TAC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TAC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TAC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TAPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TBC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TBC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TBC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TBPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TCC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TCC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TCC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TCPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TCRST       LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TDC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TDC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TDC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TDPER       LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TDRST       LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TEC2        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TEC3        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TEC4        LL_HRTIM_GetADCTrigSrc\n
  *         ADC4R        ADC4TERST       LL_HRTIM_GetADCTrigSrc
  * @param  HRTIMx High Resolution Timer instance
  * @param  ADCTrig This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_ADCTRIG_1
  *         @arg @ref LL_HRTIM_ADCTRIG_2
  *         @arg @ref LL_HRTIM_ADCTRIG_3
  *         @arg @ref LL_HRTIM_ADCTRIG_4
  * @retval Src This parameter can be a combination of the following values:
  *
  *         For ADC trigger 1 and ADC trigger 3:
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_NONE
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_MCMP1
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_MCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_MCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_MCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_MPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMACMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMACMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMACMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMAPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_EEV1
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_EEV2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_EEV3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_EEV4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_EEV5
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMARST
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMBCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMBCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMBCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMBPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMBRST
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMCCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMCCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMCCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMCPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMDCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMDCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMDCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMDPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMECMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMECMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMECMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC13_TIMEPER
  *
  *         For ADC trigger 2 and ADC trigger 4:
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_NONE
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_MCMP1
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_MCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_MCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_MCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_MPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_EEV6
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_EEV7
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_EEV8
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_EEV9
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_EEV10
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMACMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMACMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMACMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMAPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMBCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMBCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMBCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMBPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMCCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMCCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMCCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMCPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMCRST
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMDCMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMDCMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMDCMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMDPER
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMDRST
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMECMP2
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMECMP3
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMECMP4
  *         @arg @ref LL_HRTIM_ADCTRIG_SRC24_TIMERST
  */
__STATIC_INLINE uint32_t LL_HRTIM_GetADCTrigSrc(HRTIM_TypeDef *HRTIMx, uint32_t ADCTrig)
{
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.ADC1R) +
                                                    REG_OFFSET_TAB_ADCxR[ADCTrig]));
  return (*pReg);
}

/**
  * @brief  Configure the DLL calibration mode.
  * @rmtoll DLLCR        CALEN         LL_HRTIM_ConfigDLLCalibration\n
  *         DLLCR        CALRTE        LL_HRTIM_ConfigDLLCalibration
  * @param  HRTIMx High Resolution Timer instance
  * @param  Mode This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_DLLCALIBRATION_MODE_SINGLESHOT
  *         @arg @ref LL_HRTIM_DLLCALIBRATION_MODE_CONTINUOUS
   * @param  Period This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_DLLCALIBRATION_RATE_7300
  *         @arg @ref LL_HRTIM_DLLCALIBRATION_RATE_910
  *         @arg @ref LL_HRTIM_DLLCALIBRATION_RATE_114
  *         @arg @ref LL_HRTIM_DLLCALIBRATION_RATE_14
 * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ConfigDLLCalibration(HRTIM_TypeDef *HRTIMx, uint32_t Mode, uint32_t Period)
{
  MODIFY_REG(HRTIMx->sCommonRegs.DLLCR, (HRTIM_DLLCR_CALEN | HRTIM_DLLCR_CALRTE), (Mode | Period));
}

/**
  * @brief  Launch DLL calibration
  * @rmtoll DLLCR        CAL           LL_HRTIM_StartDLLCalibration
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_StartDLLCalibration(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.DLLCR, HRTIM_DLLCR_CAL);
}

/**
  * @}
  */

/** @defgroup HRTIM_EF_HRTIM_Timer_Control HRTIM_Timer_Control
  * @{
  */

/**
  * @brief  Enable timer(s) counter.
  * @rmtoll MDIER        TECEN         LL_HRTIM_TIM_CounterEnable\n
  *         MDIER        TDCEN         LL_HRTIM_TIM_CounterEnable\n
  *         MDIER        TCCEN         LL_HRTIM_TIM_CounterEnable\n
  *         MDIER        TBCEN         LL_HRTIM_TIM_CounterEnable\n
  *         MDIER        TACEN         LL_HRTIM_TIM_CounterEnable\n
  *         MDIER        MCEN          LL_HRTIM_TIM_CounterEnable
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timers This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_CounterEnable(HRTIM_TypeDef *HRTIMx, uint32_t Timers)
{
  SET_BIT(HRTIMx->sMasterRegs.MCR, Timers);
}

/**
  * @brief  Disable timer(s) counter.
  * @rmtoll MDIER        TECEN         LL_HRTIM_TIM_CounterDisable\n
  *         MDIER        TDCEN         LL_HRTIM_TIM_CounterDisable\n
  *         MDIER        TCCEN         LL_HRTIM_TIM_CounterDisable\n
  *         MDIER        TBCEN         LL_HRTIM_TIM_CounterDisable\n
  *         MDIER        TACEN         LL_HRTIM_TIM_CounterDisable\n
  *         MDIER        MCEN          LL_HRTIM_TIM_CounterDisable
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timers This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_CounterDisable(HRTIM_TypeDef *HRTIMx, uint32_t Timers)
{
  CLEAR_BIT(HRTIMx->sMasterRegs.MCR, Timers);
}

/**
  * @brief  Indicate whether the timer counter is enabled.
  * @rmtoll MDIER        TECEN         LL_HRTIM_TIM_IsCounterEnabled\n
  *         MDIER        TDCEN         LL_HRTIM_TIM_IsCounterEnabled\n
  *         MDIER        TCCEN         LL_HRTIM_TIM_IsCounterEnabled\n
  *         MDIER        TBCEN         LL_HRTIM_TIM_IsCounterEnabled\n
  *         MDIER        TACEN         LL_HRTIM_TIM_IsCounterEnabled\n
  *         MDIER        MCEN          LL_HRTIM_TIM_IsCounterEnabled
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MCEN or TxCEN bit HRTIM_MCR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_IsCounterEnabled(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  return (READ_BIT(HRTIMx->sMasterRegs.MCR, Timer) == (Timer));
}

/**
  * @brief  Set the timer clock prescaler ratio.
  * @rmtoll MCR        CKPSC         LL_HRTIM_TIM_SetPrescaler\n
  *         TIMxCR     CKPSC         LL_HRTIM_TIM_SetPrescaler
  * @note The counter clock equivalent frequency (CK_CNT) is equal to fHRCK / 2^CKPSC[2:0].
  * @note The prescaling ratio cannot be modified once the timer counter is enabled.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_PRESCALERRATIO_MUL32
  *         @arg @ref LL_HRTIM_PRESCALERRATIO_MUL16
  *         @arg @ref LL_HRTIM_PRESCALERRATIO_MUL8
  *         @arg @ref LL_HRTIM_PRESCALERRATIO_MUL4
  *         @arg @ref LL_HRTIM_PRESCALERRATIO_MUL2
  *         @arg @ref LL_HRTIM_PRESCALERRATIO_DIV1
  *         @arg @ref LL_HRTIM_PRESCALERRATIO_DIV2
  *         @arg @ref LL_HRTIM_PRESCALERRATIO_DIV4
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetPrescaler(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Prescaler)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(*pReg, HRTIM_MCR_CK_PSC, Prescaler);
}

/**
  * @brief  Get the timer clock prescaler ratio
  * @rmtoll MCR        CKPSC         LL_HRTIM_TIM_GetPrescaler\n
  *         TIMxCR     CKPSC         LL_HRTIM_TIM_GetPrescaler
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval Prescaler Returned value can be one of the following values:
  *         @arg @ref LL_HRTIM_PRESCALERRATIO_MUL32
  *         @arg @ref LL_HRTIM_PRESCALERRATIO_MUL16
  *         @arg @ref LL_HRTIM_PRESCALERRATIO_MUL8
  *         @arg @ref LL_HRTIM_PRESCALERRATIO_MUL4
  *         @arg @ref LL_HRTIM_PRESCALERRATIO_MUL2
  *         @arg @ref LL_HRTIM_PRESCALERRATIO_DIV1
  *         @arg @ref LL_HRTIM_PRESCALERRATIO_DIV2
  *         @arg @ref LL_HRTIM_PRESCALERRATIO_DIV4
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetPrescaler(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MCR_CK_PSC));
}

/**
  * @brief  Set the counter operating mode mode (single-shot, continuous or re-triggerable).
  * @rmtoll MCR        CONT         LL_HRTIM_TIM_SetCounterMode\n
  *         MCR        RETRIG       LL_HRTIM_TIM_SetCounterMode\n
  *         TIMxCR     CONT         LL_HRTIM_TIM_SetCounterMode\n
  *         TIMxCR     RETRIG       LL_HRTIM_TIM_SetCounterMode
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Mode This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_MODE_CONTINUOUS
  *         @arg @ref LL_HRTIM_MODE_SINGLESHOT
  *         @arg @ref LL_HRTIM_MODE_RETRIGGERABLE
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetCounterMode(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Mode)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(*pReg, (HRTIM_TIMCR_RETRIG | HRTIM_MCR_CONT), Mode);
}

/**
  * @brief  Get the counter operating mode mode
  * @rmtoll MCR        CONT         LL_HRTIM_TIM_GetCounterMode\n
  *         MCR        RETRIG       LL_HRTIM_TIM_GetCounterMode\n
  *         TIMxCR     CONT         LL_HRTIM_TIM_GetCounterMode\n
  *         TIMxCR     RETRIG       LL_HRTIM_TIM_GetCounterMode
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval Mode Returned value can be one of the following values:
  *         @arg @ref LL_HRTIM_MODE_CONTINUOUS
  *         @arg @ref LL_HRTIM_MODE_SINGLESHOT
  *         @arg @ref LL_HRTIM_MODE_RETRIGGERABLE
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetCounterMode(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, (HRTIM_MCR_RETRIG | HRTIM_MCR_CONT)));
}

/**
  * @brief  Enable the half duty-cycle mode.
  * @rmtoll MCR        HALF         LL_HRTIM_TIM_EnableHalfMode\n
  *         TIMxCR     HALF         LL_HRTIM_TIM_EnableHalfMode
  * @note When the half mode is enabled, HRTIM_MCMP1R (or HRTIM_CMP1xR)
  *       active register is automatically updated with HRTIM_MPER/2
  *       (or HRTIM_PERxR/2) value when HRTIM_MPER (or HRTIM_PERxR) register is written.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_EnableHalfMode(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MCR_HALF);
}

/**
  * @brief  Disable the half duty-cycle mode.
  * @rmtoll MCR        HALF         LL_HRTIM_TIM_DisableHalfMode\n
  *         TIMxCR     HALF         LL_HRTIM_TIM_DisableHalfMode
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_DisableHalfMode(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_MCR_HALF);
}

/**
  * @brief  Indicate whether half duty-cycle mode is enabled for a given timer.
  * @rmtoll MCR        HALF         LL_HRTIM_TIM_IsEnabledHalfMode\n
  *         TIMxCR     HALF         LL_HRTIM_TIM_IsEnabledHalfMode
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of HALF bit to 1 in HRTIM_MCR or HRTIM_TIMxCR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_IsEnabledHalfMode(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MCR_HALF) == HRTIM_MCR_HALF);
}

/**
  * @brief  Enable the timer start when receiving a synchronization input event.
  * @rmtoll MCR        SYNCSTRTM        LL_HRTIM_TIM_EnableStartOnSync\n
  *         TIMxCR     SYNSTRTA         LL_HRTIM_TIM_EnableStartOnSync
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_EnableStartOnSync(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MCR_SYNCSTRTM);
}

/**
  * @brief  Disable the timer start when receiving a synchronization input event.
  * @rmtoll MCR        SYNCSTRTM        LL_HRTIM_TIM_DisableStartOnSync\n
  *         TIMxCR     SYNSTRTA         LL_HRTIM_TIM_DisableStartOnSync
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_DisableStartOnSync(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_MCR_SYNCSTRTM);
}

/**
  * @brief  Indicate whether the timer start when receiving a synchronization input event.
  * @rmtoll MCR        SYNCSTRTM        LL_HRTIM_TIM_IsEnabledStartOnSync\n
  *         TIMxCR     SYNSTRTA         LL_HRTIM_TIM_IsEnabledStartOnSync
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of SYNCSTRTx bit in HRTIM_MCR or HRTIM_TIMxCR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_IsEnabledStartOnSync(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MCR_SYNCSTRTM) == HRTIM_MCR_SYNCSTRTM);
}

/**
  * @brief  Enable the timer reset when receiving a synchronization input event.
  * @rmtoll MCR        SYNCRSTM        LL_HRTIM_TIM_EnableResetOnSync\n
  *         TIMxCR     SYNCRSTA        LL_HRTIM_TIM_EnableResetOnSync
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_EnableResetOnSync(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MCR_SYNCRSTM);
}

/**
  * @brief  Disable the timer reset when receiving a synchronization input event.
  * @rmtoll MCR        SYNCRSTM        LL_HRTIM_TIM_DisableResetOnSync\n
  *         TIMxCR     SYNCRSTA        LL_HRTIM_TIM_DisableResetOnSync
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_DisableResetOnSync(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_MCR_SYNCRSTM);
}

/**
  * @brief  Indicate whether the timer reset when receiving a synchronization input event.
  * @rmtoll MCR        SYNCRSTM        LL_HRTIM_TIM_IsEnabledResetOnSync\n
  *         TIMxCR     SYNCRSTA        LL_HRTIM_TIM_IsEnabledResetOnSync
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_IsEnabledResetOnSync(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MCR_SYNCRSTM) == HRTIM_MCR_SYNCRSTM);
}

/**
  * @brief  Set the HRTIM output the DAC synchronization event is generated on (DACtrigOutx).
  * @rmtoll MCR        DACSYNC        LL_HRTIM_TIM_SetDACTrig\n
  *         TIMxCR     DACSYNC        LL_HRTIM_TIM_SetDACTrig
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  DACTrig This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_DACTRIG_NONE
  *         @arg @ref LL_HRTIM_DACTRIG_DACTRIGOUT_1
  *         @arg @ref LL_HRTIM_DACTRIG_DACTRIGOUT_2
  *         @arg @ref LL_HRTIM_DACTRIG_DACTRIGOUT_3
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetDACTrig(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t DACTrig)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(*pReg, HRTIM_MCR_DACSYNC, DACTrig);
}

/**
  * @brief  Get the HRTIM output the DAC synchronization event is generated on (DACtrigOutx).
  * @rmtoll MCR        DACSYNC        LL_HRTIM_TIM_GetDACTrig\n
  *         TIMxCR     DACSYNC        LL_HRTIM_TIM_GetDACTrig
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval DACTrig Returned value can be one of the following values:
  *         @arg @ref LL_HRTIM_DACTRIG_NONE
  *         @arg @ref LL_HRTIM_DACTRIG_DACTRIGOUT_1
  *         @arg @ref LL_HRTIM_DACTRIG_DACTRIGOUT_2
  *         @arg @ref LL_HRTIM_DACTRIG_DACTRIGOUT_3
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetDACTrig(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MCR_DACSYNC));
}

/**
  * @brief  Enable the timer registers preload mechanism.
  * @rmtoll MCR        PREEN        LL_HRTIM_TIM_EnablePreload\n
  *         TIMxCR     PREEN        LL_HRTIM_TIM_EnablePreload
  * @note When the preload mode is enabled, accessed registers are shadow registers.
  *       Their content is transferred into the active register after an update request,
  *       either software or synchronized with an event.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_EnablePreload(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MCR_PREEN);
}

/**
  * @brief  Disable the timer registers preload mechanism.
  * @rmtoll MCR        PREEN        LL_HRTIM_TIM_DisablePreload\n
  *         TIMxCR     PREEN        LL_HRTIM_TIM_DisablePreload
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_DisablePreload(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_MCR_PREEN);
}

/**
  * @brief  Indicate whether the timer registers preload mechanism is enabled.
  * @rmtoll MCR        PREEN        LL_HRTIM_TIM_IsEnabledPreload\n
  *         TIMxCR     PREEN        LL_HRTIM_TIM_IsEnabledPreload
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of PREEN bit in HRTIM_MCR or HRTIM_TIMxCR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_IsEnabledPreload(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MCR_PREEN) == HRTIM_MCR_PREEN);
}

/**
  * @brief  Set the timer register update trigger.
  * @rmtoll MCR           MREPU      LL_HRTIM_TIM_SetUpdateTrig\n
  *         TIMxCR        TAU        LL_HRTIM_TIM_SetUpdateTrig\n
  *         TIMxCR        TBU        LL_HRTIM_TIM_SetUpdateTrig\n
  *         TIMxCR        TCU        LL_HRTIM_TIM_SetUpdateTrig\n
  *         TIMxCR        TDU        LL_HRTIM_TIM_SetUpdateTrig\n
  *         TIMxCR        TEU        LL_HRTIM_TIM_SetUpdateTrig\n
  *         TIMxCR        MSTU       LL_HRTIM_TIM_SetUpdateTrig
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  UpdateTrig This parameter can be one of the following values:
  *
  *         For the master timer this parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_UPDATETRIG_NONE
  *         @arg @ref LL_HRTIM_UPDATETRIG_REPETITION
  *
  *         For timer A..E this parameter can be:
  *         @arg @ref LL_HRTIM_UPDATETRIG_NONE
  *         or a combination of the following values:
  *         @arg @ref LL_HRTIM_UPDATETRIG_MASTER
  *         @arg @ref LL_HRTIM_UPDATETRIG_TIMER_A
  *         @arg @ref LL_HRTIM_UPDATETRIG_TIMER_B
  *         @arg @ref LL_HRTIM_UPDATETRIG_TIMER_C
  *         @arg @ref LL_HRTIM_UPDATETRIG_TIMER_D
  *         @arg @ref LL_HRTIM_UPDATETRIG_TIMER_E
  *         @arg @ref LL_HRTIM_UPDATETRIG_REPETITION
  *         @arg @ref LL_HRTIM_UPDATETRIG_RESET
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetUpdateTrig(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t UpdateTrig)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(*pReg, REG_MASK_TAB_UPDATETRIG[iTimer], UpdateTrig << REG_SHIFT_TAB_UPDATETRIG[iTimer]);
}

/**
  * @brief  Set the timer register update trigger.
  * @rmtoll MCR           MREPU      LL_HRTIM_TIM_GetUpdateTrig\n
  *         TIMxCR        TBU        LL_HRTIM_TIM_GetUpdateTrig\n
  *         TIMxCR        TCU        LL_HRTIM_TIM_GetUpdateTrig\n
  *         TIMxCR        TDU        LL_HRTIM_TIM_GetUpdateTrig\n
  *         TIMxCR        TEU        LL_HRTIM_TIM_GetUpdateTrig\n
  *         TIMxCR        MSTU       LL_HRTIM_TIM_GetUpdateTrig
 * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval UpdateTrig Returned value can be one of the following values:
  *
  *         For the master timer this parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_UPDATETRIG_NONE
  *         @arg @ref LL_HRTIM_UPDATETRIG_REPETITION
  *
  *         For timer A..E this parameter can be:
  *         @arg @ref LL_HRTIM_UPDATETRIG_NONE
  *         or a combination of the following values:
  *         @arg @ref LL_HRTIM_UPDATETRIG_MASTER
  *         @arg @ref LL_HRTIM_UPDATETRIG_TIMER_A
  *         @arg @ref LL_HRTIM_UPDATETRIG_TIMER_B
  *         @arg @ref LL_HRTIM_UPDATETRIG_TIMER_C
  *         @arg @ref LL_HRTIM_UPDATETRIG_TIMER_D
  *         @arg @ref LL_HRTIM_UPDATETRIG_TIMER_E
  *         @arg @ref LL_HRTIM_UPDATETRIG_REPETITION
  *         @arg @ref LL_HRTIM_UPDATETRIG_RESET
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetUpdateTrig(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, REG_MASK_TAB_UPDATETRIG[iTimer]) >>  REG_SHIFT_TAB_UPDATETRIG[iTimer]);
}

/**
  * @brief  Set  the timer registers update condition (how the registers update occurs relatively to the burst DMA  transaction or an external update request received on one of the update enable inputs (UPD_EN[3:1])).
  * @rmtoll MCR           BRSTDMA      LL_HRTIM_TIM_SetUpdateGating\n
  *         TIMxCR        UPDGAT       LL_HRTIM_TIM_SetUpdateGating
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  UpdateGating This parameter can be one of the following values:
  *
  *         For the master timer this parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_UPDATEGATING_INDEPENDENT
  *         @arg @ref LL_HRTIM_UPDATEGATING_DMABURST
  *         @arg @ref LL_HRTIM_UPDATEGATING_DMABURST_UPDATE
  *
  *         For the timer A..E this parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_UPDATEGATING_INDEPENDENT
  *         @arg @ref LL_HRTIM_UPDATEGATING_DMABURST
  *         @arg @ref LL_HRTIM_UPDATEGATING_DMABURST_UPDATE
  *         @arg @ref LL_HRTIM_UPDATEGATING_UPDEN1
  *         @arg @ref LL_HRTIM_UPDATEGATING_UPDEN2
  *         @arg @ref LL_HRTIM_UPDATEGATING_UPDEN3
  *         @arg @ref LL_HRTIM_UPDATEGATING_UPDEN1_UPDATE
  *         @arg @ref LL_HRTIM_UPDATEGATING_UPDEN2_UPDATE
  *         @arg @ref LL_HRTIM_UPDATEGATING_UPDEN3_UPDATE
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetUpdateGating(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t UpdateGating)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(*pReg, REG_MASK_TAB_UPDATEGATING[iTimer], (UpdateGating << REG_SHIFT_TAB_UPDATEGATING[iTimer]));
}

/**
  * @brief  Get  the timer registers update condition.
  * @rmtoll MCR           BRSTDMA      LL_HRTIM_TIM_GetUpdateGating\n
  *         TIMxCR        UPDGAT       LL_HRTIM_TIM_GetUpdateGating
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval UpdateGating Returned value can be one of the following values:
  *
  *         For the master timer this parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_UPDATEGATING_INDEPENDENT
  *         @arg @ref LL_HRTIM_UPDATEGATING_DMABURST
  *         @arg @ref LL_HRTIM_UPDATEGATING_DMABURST_UPDATE
  *
  *         For the timer A..E this parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_UPDATEGATING_INDEPENDENT
  *         @arg @ref LL_HRTIM_UPDATEGATING_DMABURST
  *         @arg @ref LL_HRTIM_UPDATEGATING_DMABURST_UPDATE
  *         @arg @ref LL_HRTIM_UPDATEGATING_UPDEN1
  *         @arg @ref LL_HRTIM_UPDATEGATING_UPDEN2
  *         @arg @ref LL_HRTIM_UPDATEGATING_UPDEN3
  *         @arg @ref LL_HRTIM_UPDATEGATING_UPDEN1_UPDATE
  *         @arg @ref LL_HRTIM_UPDATEGATING_UPDEN2_UPDATE
  *         @arg @ref LL_HRTIM_UPDATEGATING_UPDEN3_UPDATE
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetUpdateGating(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCR) + REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, REG_MASK_TAB_UPDATEGATING[iTimer]) >>  REG_SHIFT_TAB_UPDATEGATING[iTimer]);
}

/**
  * @brief  Enable the push-pull mode.
  * @rmtoll TIMxCR        PSHPLL       LL_HRTIM_TIM_EnablePushPullMode
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_EnablePushPullMode(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].TIMxCR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMCR_PSHPLL);
}

/**
  * @brief  Disable the push-pull mode.
  * @rmtoll TIMxCR        PSHPLL       LL_HRTIM_TIM_DisablePushPullMode
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_DisablePushPullMode(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].TIMxCR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMCR_PSHPLL);
}

/**
  * @brief  Indicate whether the push-pull mode is enabled.
  * @rmtoll TIMxCR        PSHPLL       LL_HRTIM_TIM_IsEnabledPushPullMode\n
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of PSHPLL bit in HRTIM_TIMxCR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_IsEnabledPushPullMode(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].TIMxCR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMCR_PSHPLL) == HRTIM_TIMCR_PSHPLL);
}

/**
  * @brief  Set the functioning mode of the compare unit (CMP2 or CMP4 can operate in standard mode or in auto delayed mode).
  * @rmtoll TIMxCR        DELCMP2       LL_HRTIM_TIM_SetCompareMode\n
  *         TIMxCR        DELCMP4       LL_HRTIM_TIM_SetCompareMode
  * @note In auto-delayed mode  the compare match occurs independently from the timer counter value.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  CompareUnit This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_COMPAREUNIT_2
  *         @arg @ref LL_HRTIM_COMPAREUNIT_4
  * @param  Mode This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_COMPAREMODE_REGULAR
  *         @arg @ref LL_HRTIM_COMPAREMODE_DELAY_NOTIMEOUT
  *         @arg @ref LL_HRTIM_COMPAREMODE_DELAY_CMP1
  *         @arg @ref LL_HRTIM_COMPAREMODE_DELAY_CMP3
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetCompareMode(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t CompareUnit,
                                                 uint32_t Mode)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].TIMxCR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  register uint32_t shift = POSITION_VAL(CompareUnit) - POSITION_VAL(LL_HRTIM_COMPAREUNIT_2);
  MODIFY_REG(* pReg, (HRTIM_TIMCR_DELCMP2 << shift), (Mode << shift));
}

/**
  * @brief  Get the functioning mode of the compare unit.
  * @rmtoll TIMxCR        DELCMP2       LL_HRTIM_TIM_GetCompareMode\n
  *         TIMxCR        DELCMP4       LL_HRTIM_TIM_GetCompareMode
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  CompareUnit This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_COMPAREUNIT_2
  *         @arg @ref LL_HRTIM_COMPAREUNIT_4
  * @retval Mode Returned value can be one of the following values:
  *         @arg @ref LL_HRTIM_COMPAREMODE_REGULAR
  *         @arg @ref LL_HRTIM_COMPAREMODE_DELAY_NOTIMEOUT
  *         @arg @ref LL_HRTIM_COMPAREMODE_DELAY_CMP1
  *         @arg @ref LL_HRTIM_COMPAREMODE_DELAY_CMP3
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetCompareMode(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t CompareUnit)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].TIMxCR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  register uint32_t shift = POSITION_VAL(CompareUnit) - POSITION_VAL(LL_HRTIM_COMPAREUNIT_2);
  return (READ_BIT(*pReg, (HRTIM_TIMCR_DELCMP2 << shift)) >>  shift);
}

/**
  * @brief  Set the timer counter value.
  * @rmtoll MCNTR        MCNT       LL_HRTIM_TIM_SetCounter\n
  *         CNTxR        CNTx       LL_HRTIM_TIM_SetCounter
  * @note  This function can only be called when the timer is stopped.
  * @note  For HR clock prescaling ratio below 32 (CKPSC[2:0] < 5), the least
  *        significant bits of the counter are not significant. They cannot be
  *        written and return 0 when read.
  * @note The timer behavior is not guaranteed if the counter value is set above
  *       the period.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Counter Value between 0 and 0xFFFF
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetCounter(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Counter)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCNTR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(* pReg, HRTIM_MCNTR_MCNTR, Counter);
}

/**
  * @brief  Get actual timer counter value.
  * @rmtoll MCNTR        MCNT       LL_HRTIM_TIM_GetCounter\n
  *         CNTxR        CNTx       LL_HRTIM_TIM_GetCounter
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval Counter Value between 0 and 0xFFFF
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetCounter(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCNTR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MCNTR_MCNTR));
}

/**
  * @brief  Set the timer period value.
  * @rmtoll MPER        MPER       LL_HRTIM_TIM_SetPeriod\n
  *         PERxR       PERx       LL_HRTIM_TIM_SetPeriod
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Period Value between 0 and 0xFFFF
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetPeriod(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Period)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MPER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(* pReg, HRTIM_MPER_MPER, Period);
}

/**
  * @brief  Get actual timer period value.
  * @rmtoll MPER        MPER       LL_HRTIM_TIM_GetPeriod\n
  *         PERxR       PERx       LL_HRTIM_TIM_GetPeriod
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval Period Value between 0 and 0xFFFF
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetPeriod(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MPER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MPER_MPER));
}

/**
  * @brief  Set the timer repetition period value.
  * @rmtoll MREP        MREP       LL_HRTIM_TIM_SetRepetition\n
  *         REPxR       REPx       LL_HRTIM_TIM_SetRepetition
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Repetition Value between 0 and 0xFF
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetRepetition(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Repetition)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MREP) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(* pReg, HRTIM_MREP_MREP, Repetition);
}

/**
  * @brief  Get actual timer repetition period value.
  * @rmtoll MREP        MREP       LL_HRTIM_TIM_GetRepetition\n
  *         REPxR       REPx       LL_HRTIM_TIM_GetRepetition
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval Repetition Value between 0 and 0xFF
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetRepetition(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MREP) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MREP_MREP));
}

/**
  * @brief  Set the compare value of the compare unit 1.
  * @rmtoll MCMP1R      MCMP1       LL_HRTIM_TIM_SetCompare1\n
  *         CMP1xR      CMP1x       LL_HRTIM_TIM_SetCompare1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  CompareValue Compare value must be above or equal to 3
  *         periods of the fHRTIM clock, that is 0x60 if CKPSC[2:0] = 0,
  *         0x30 if CKPSC[2:0] = 1, 0x18 if CKPSC[2:0] = 2,...
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetCompare1(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t CompareValue)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCMP1R) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(* pReg, HRTIM_MCMP1R_MCMP1R, CompareValue);
}

/**
  * @brief  Get actual compare value of the compare unit 1.
  * @rmtoll MCMP1R      MCMP1       LL_HRTIM_TIM_GetCompare1\n
  *         CMP1xR      CMP1x       LL_HRTIM_TIM_GetCompare1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval CompareValue Compare value must be above or equal to 3
  *         periods of the fHRTIM clock, that is 0x60 if CKPSC[2:0] = 0,
  *         0x30 if CKPSC[2:0] = 1, 0x18 if CKPSC[2:0] = 2,...
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetCompare1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCMP1R) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MCMP1R_MCMP1R));
}

/**
  * @brief  Set the compare value of the compare unit 2.
  * @rmtoll MCMP2R      MCMP2       LL_HRTIM_TIM_SetCompare2\n
  *         CMP2xR      CMP2x       LL_HRTIM_TIM_SetCompare2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  CompareValue Compare value must be above or equal to 3
  *         periods of the fHRTIM clock, that is 0x60 if CKPSC[2:0] = 0,
  *         0x30 if CKPSC[2:0] = 1, 0x18 if CKPSC[2:0] = 2,...
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetCompare2(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t CompareValue)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCMP2R) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(* pReg, HRTIM_MCMP1R_MCMP2R, CompareValue);
}

/**
  * @brief  Get actual compare value of the compare unit 2.
  * @rmtoll MCMP2R      MCMP2       LL_HRTIM_TIM_GetCompare2\n
  *         CMP2xR      CMP2x       LL_HRTIM_TIM_GetCompare2\n
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval CompareValue Compare value must be above or equal to 3
  *         periods of the fHRTIM clock, that is 0x60 if CKPSC[2:0] = 0,
  *         0x30 if CKPSC[2:0] = 1, 0x18 if CKPSC[2:0] = 2,...
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetCompare2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCMP2R) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MCMP1R_MCMP2R));
}

/**
  * @brief  Set the compare value of the compare unit 3.
  * @rmtoll MCMP3R      MCMP3       LL_HRTIM_TIM_SetCompare3\n
  *         CMP3xR      CMP3x       LL_HRTIM_TIM_SetCompare3
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  CompareValue Compare value must be above or equal to 3
  *         periods of the fHRTIM clock, that is 0x60 if CKPSC[2:0] = 0,
  *         0x30 if CKPSC[2:0] = 1, 0x18 if CKPSC[2:0] = 2,...
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetCompare3(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t CompareValue)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCMP3R) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(* pReg, HRTIM_MCMP1R_MCMP3R, CompareValue);
}

/**
  * @brief  Get actual compare value of the compare unit 3.
  * @rmtoll MCMP3R      MCMP3       LL_HRTIM_TIM_GetCompare3\n
  *         CMP3xR      CMP3x       LL_HRTIM_TIM_GetCompare3
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval CompareValue Compare value must be above or equal to 3
  *         periods of the fHRTIM clock, that is 0x60 if CKPSC[2:0] = 0,
  *         0x30 if CKPSC[2:0] = 1, 0x18 if CKPSC[2:0] = 2,...
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetCompare3(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCMP3R) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MCMP1R_MCMP3R));
}

/**
  * @brief  Set the compare value of the compare unit 4.
  * @rmtoll MCMP4R      MCMP4       LL_HRTIM_TIM_SetCompare4\n
  *         CMP4xR      CMP4x       LL_HRTIM_TIM_SetCompare4
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  CompareValue Compare value must be above or equal to 3
  *         periods of the fHRTIM clock, that is 0x60 if CKPSC[2:0] = 0,
  *         0x30 if CKPSC[2:0] = 1, 0x18 if CKPSC[2:0] = 2,...
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetCompare4(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t CompareValue)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCMP4R) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(* pReg, HRTIM_MCMP1R_MCMP4R, CompareValue);
}

/**
  * @brief  Get actual compare value of the compare unit 4.
  * @rmtoll MCMP4R      MCMP4       LL_HRTIM_TIM_GetCompare4\n
  *         CMP4xR      CMP4x       LL_HRTIM_TIM_GetCompare4
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval CompareValue Compare value must be above or equal to 3
  *         periods of the fHRTIM clock, that is 0x60 if CKPSC[2:0] = 0,
  *         0x30 if CKPSC[2:0] = 1, 0x18 if CKPSC[2:0] = 2,...
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetCompare4(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MCMP4R) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MCMP1R_MCMP4R));
}

/**
  * @brief  Set the reset trigger of a timer counter.
  * @rmtoll RSTxR      UPDT           LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      CMP2           LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      CMP4           LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      MSTPER         LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      MSTCMP1        LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      MSTCMP2        LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      MSTCMP3        LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      MSTCMP4        LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      EXTEVNT1       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      EXTEVNT2       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      EXTEVNT3       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      EXTEVNT4       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      EXTEVNT5       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      EXTEVNT6       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      EXTEVNT7       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      EXTEVNT8       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      EXTEVNT9       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      EXTEVNT10      LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      TIMBCMP1       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      TIMBCMP2       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      TIMBCMP4       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      TIMCCMP1       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      TIMCCMP2       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      TIMCCMP4       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      TIMDCMP1       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      TIMDCMP2       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      TIMDCMP4       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      TIMECMP1       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      TIMECMP2       LL_HRTIM_TIM_SetResetTrig\n
  *         RSTxR      TIMECMP4       LL_HRTIM_TIM_SetResetTrig
  * @note The reset of the timer counter can be triggered by up to 30 events
  *       that can be selected among the following sources:
  *         @arg The timing unit: Compare 2, Compare 4 and Update (3 events).
  *         @arg The master timer: Reset and Compare 1..4 (5 events).
  *         @arg The external events EXTEVNT1..10 (10 events).
  *         @arg All other timing units (e.g. Timer B..E for timer A): Compare 1, 2 and 4 (12 events).
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  ResetTrig This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_RESETTRIG_NONE
  *         @arg @ref LL_HRTIM_RESETTRIG_UPDATE
  *         @arg @ref LL_HRTIM_RESETTRIG_CMP2
  *         @arg @ref LL_HRTIM_RESETTRIG_CMP4
  *         @arg @ref LL_HRTIM_RESETTRIG_MASTER_PER
  *         @arg @ref LL_HRTIM_RESETTRIG_MASTER_CMP1
  *         @arg @ref LL_HRTIM_RESETTRIG_MASTER_CMP2
  *         @arg @ref LL_HRTIM_RESETTRIG_MASTER_CMP3
  *         @arg @ref LL_HRTIM_RESETTRIG_MASTER_CMP4
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_1
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_2
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_3
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_4
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_5
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_6
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_7
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_8
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_9
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_10
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER1_CMP1
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER1_CMP2
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER1_CMP4
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER2_CMP1
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER2_CMP2
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER2_CMP4
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER3_CMP1
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER3_CMP2
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER3_CMP4
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER4_CMP1
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER4_CMP2
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER4_CMP4
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetResetTrig(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t ResetTrig)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].RSTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  WRITE_REG(*pReg, ResetTrig);
}

/**
  * @brief  Get actual reset trigger of a timer counter.
  * @rmtoll RSTxR      UPDT           LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      CMP2           LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      CMP4           LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      MSTPER         LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      MSTCMP1        LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      MSTCMP2        LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      MSTCMP3        LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      MSTCMP4        LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      EXTEVNT1       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      EXTEVNT2       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      EXTEVNT3       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      EXTEVNT4       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      EXTEVNT5       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      EXTEVNT6       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      EXTEVNT7       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      EXTEVNT8       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      EXTEVNT9       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      EXTEVNT10      LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      TIMBCMP1       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      TIMBCMP2       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      TIMBCMP4       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      TIMCCMP1       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      TIMCCMP2       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      TIMCCMP4       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      TIMDCMP1       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      TIMDCMP2       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      TIMDCMP4       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      TIMECMP1       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      TIMECMP2       LL_HRTIM_TIM_GetResetTrig\n
  *         RSTxR      TIMECMP4       LL_HRTIM_TIM_GetResetTrig
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval ResetTrig Returned value can be one of the following values:
  *         @arg @ref LL_HRTIM_RESETTRIG_NONE
  *         @arg @ref LL_HRTIM_RESETTRIG_UPDATE
  *         @arg @ref LL_HRTIM_RESETTRIG_CMP2
  *         @arg @ref LL_HRTIM_RESETTRIG_CMP4
  *         @arg @ref LL_HRTIM_RESETTRIG_MASTER_PER
  *         @arg @ref LL_HRTIM_RESETTRIG_MASTER_CMP1
  *         @arg @ref LL_HRTIM_RESETTRIG_MASTER_CMP2
  *         @arg @ref LL_HRTIM_RESETTRIG_MASTER_CMP3
  *         @arg @ref LL_HRTIM_RESETTRIG_MASTER_CMP4
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_1
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_2
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_3
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_4
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_5
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_6
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_7
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_8
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_9
  *         @arg @ref LL_HRTIM_RESETTRIG_EEV_10
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER1_CMP1
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER1_CMP2
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER1_CMP4
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER2_CMP1
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER2_CMP2
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER2_CMP4
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER3_CMP1
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER3_CMP2
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER3_CMP4
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER4_CMP1
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER4_CMP2
  *         @arg @ref LL_HRTIM_RESETTRIG_OTHER4_CMP4
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetResetTrig(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].RSTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_REG(*pReg));
}

/**
  * @brief  Get captured value for capture unit 1.
  * @rmtoll CPT1xR      CPT1x           LL_HRTIM_TIM_GetCapture1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval Captured value
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetCapture1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].CPT1xR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_REG(*pReg));
}

/**
  * @brief  Get captured value for capture unit 2.
  * @rmtoll CPT2xR      CPT2x           LL_HRTIM_TIM_GetCapture2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval Captured value
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetCapture2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].CPT2xR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_REG(*pReg));
}

/**
  * @brief  Set the trigger of a capture unit for a given timer.
  * @rmtoll CPT1xCR      SWCPT            LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      UPDCPT           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      EXEV1CPT         LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      EXEV2CPT         LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      EXEV3CPT         LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      EXEV4CPT         LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      EXEV5CPT         LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      EXEV6CPT         LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      EXEV7CPT         LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      EXEV8CPT         LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      EXEV9CPT         LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      EXEV10CPT        LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TA1SET           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TA1RST           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TACMP1           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TACMP2           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TB1SET           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TB1RST           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TBCMP1           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TBCMP2           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TC1SET           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TC1RST           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TCCMP1           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TCCMP2           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TD1SET           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TD1RST           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TDCMP1           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TDCMP2           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TE1SET           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TE1RST           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TECMP1           LL_HRTIM_TIM_SetCaptureTrig\n
  *         CPT1xCR      TECMP2           LL_HRTIM_TIM_SetCaptureTrig
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  CaptureUnit This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_CAPTUREUNIT_1
  *         @arg @ref LL_HRTIM_CAPTUREUNIT_2
  * @param  CaptureTrig This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_CAPTURETRIG_NONE
  *         @arg @ref LL_HRTIM_CAPTURETRIG_UPDATE
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_1
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_2
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_3
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_4
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_5
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_6
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_7
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_8
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_9
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_10
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TA1_SET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TA1_RESET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIMA_CMP1
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIMA_CMP2
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TB1_SET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TB1_RESET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIMB_CMP1
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIMB_CMP2
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TC1_SET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TC1_RESET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIMC_CMP1
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIMC_CMP2
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TD1_SET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TD1_RESET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIMD_CMP1
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIMD_CMP2
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TE1_SET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TE1_RESET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIME_CMP1
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIME_CMP2
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetCaptureTrig(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t CaptureUnit,
                                                 uint32_t CaptureTrig)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].CPT1xCR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer] + CaptureUnit * 4));
  WRITE_REG(*pReg, CaptureTrig);
}

/**
  * @brief  Get actual trigger of a capture unit for a given timer.
  * @rmtoll CPT1xCR      SWCPT            LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      UPDCPT           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      EXEV1CPT         LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      EXEV2CPT         LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      EXEV3CPT         LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      EXEV4CPT         LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      EXEV5CPT         LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      EXEV6CPT         LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      EXEV7CPT         LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      EXEV8CPT         LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      EXEV9CPT         LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      EXEV10CPT        LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TA1SET           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TA1RST           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TACMP1           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TACMP2           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TB1SET           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TB1RST           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TBCMP1           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TBCMP2           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TC1SET           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TC1RST           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TCCMP1           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TCCMP2           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TD1SET           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TD1RST           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TDCMP1           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TDCMP2           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TE1SET           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TE1RST           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TECMP1           LL_HRTIM_TIM_GetCaptureTrig\n
  *         CPT1xCR      TECMP2           LL_HRTIM_TIM_GetCaptureTrig
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  CaptureUnit This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_CAPTUREUNIT_1
  *         @arg @ref LL_HRTIM_CAPTUREUNIT_2
  * @retval CaptureTrig This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_CAPTURETRIG_NONE
  *         @arg @ref LL_HRTIM_CAPTURETRIG_UPDATE
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_1
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_2
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_3
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_4
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_5
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_6
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_7
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_8
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_9
  *         @arg @ref LL_HRTIM_CAPTURETRIG_EEV_10
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TA1_SET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TA1_RESET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIMA_CMP1
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIMA_CMP2
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TB1_SET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TB1_RESET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIMB_CMP1
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIMB_CMP2
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TC1_SET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TC1_RESET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIMC_CMP1
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIMC_CMP2
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TD1_SET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TD1_RESET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIMD_CMP1
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIMD_CMP2
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TE1_SET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TE1_RESET
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIME_CMP1
  *         @arg @ref LL_HRTIM_CAPTURETRIG_TIME_CMP2
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetCaptureTrig(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t CaptureUnit)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].CPT1xCR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer] + CaptureUnit * 4));
  return (READ_REG(*pReg));
}

/**
  * @brief  Enable deadtime insertion for a given timer.
  * @rmtoll OUTxR      DTEN           LL_HRTIM_TIM_EnableDeadTime
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_EnableDeadTime(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_OUTR_DTEN);
}

/**
  * @brief  Disable deadtime insertion for a given timer.
  * @rmtoll OUTxR      DTEN           LL_HRTIM_TIM_DisableDeadTime
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_DisableDeadTime(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_OUTR_DTEN);
}

/**
  * @brief  Indicate whether deadtime insertion is enabled for a given timer.
  * @rmtoll OUTxR      DTEN           LL_HRTIM_TIM_IsEnabledDeadTime
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of DTEN bit in HRTIM_OUTxR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_IsEnabledDeadTime(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_OUTR_DTEN) == HRTIM_OUTR_DTEN);
}

/**
  * @brief  Set the delayed protection (DLYPRT) mode.
  * @rmtoll OUTxR      DLYPRTEN       LL_HRTIM_TIM_SetDLYPRTMode\n
  *         OUTxR      DLYPRT         LL_HRTIM_TIM_SetDLYPRTMode
  * @note   This function must be called prior enabling the delayed protection
  * @note   Balanced Idle mode is only available in push-pull mode
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  DLYPRTMode Delayed protection (DLYPRT) mode
  *
  *         For timers A, B and C this parameter can be one of the following vallues:
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYOUT1_EEV6
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYOUT2_EEV6
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYBOTH_EEV6
  *         @arg @ref LL_HRTIM_DLYPRT_BALANCED_EEV6
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYOUT1_EEV7
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYOUT2_EEV7
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYBOTH_EEV7
  *         @arg @ref LL_HRTIM_DLYPRT_BALANCED_EEV7
  *
  *         For timers D and E this parameter can be one of the following vallues:
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYOUT1_EEV8
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYOUT2_EEV8
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYBOTH_EEV8
  *         @arg @ref LL_HRTIM_DLYPRT_BALANCED_EEV8
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYOUT1_EEV9
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYOUT2_EEV9
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYBOTH_EEV9
  *         @arg @ref LL_HRTIM_DLYPRT_BALANCED_EEV9
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetDLYPRTMode(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t DLYPRTMode)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(*pReg, HRTIM_OUTR_DLYPRT, DLYPRTMode);
}

/**
  * @brief  Get the delayed protection (DLYPRT) mode.
  * @rmtoll OUTxR      DLYPRTEN       LL_HRTIM_TIM_GetDLYPRTMode\n
  *         OUTxR      DLYPRT         LL_HRTIM_TIM_GetDLYPRTMode
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval DLYPRTMode Delayed protection (DLYPRT) mode
  *
  *         For timers A, B and C this parameter can be one of the following vallues:
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYOUT1_EEV6
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYOUT2_EEV6
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYBOTH_EEV6
  *         @arg @ref LL_HRTIM_DLYPRT_BALANCED_EEV6
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYOUT1_EEV7
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYOUT2_EEV7
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYBOTH_EEV7
  *         @arg @ref LL_HRTIM_DLYPRT_BALANCED_EEV7
  *
  *         For timers D and E this parameter can be one of the following vallues:
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYOUT1_EEV8
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYOUT2_EEV8
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYBOTH_EEV8
  *         @arg @ref LL_HRTIM_DLYPRT_BALANCED_EEV8
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYOUT1_EEV9
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYOUT2_EEV9
  *         @arg @ref LL_HRTIM_DLYPRT_DELAYBOTH_EEV9
  *         @arg @ref LL_HRTIM_DLYPRT_BALANCED_EEV9
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetDLYPRTMode(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_OUTR_DLYPRT));
}

/**
  * @brief  Enable delayed protection (DLYPRT) for a given timer.
  * @rmtoll OUTxR      DLYPRTEN       LL_HRTIM_TIM_EnableDLYPRT
  * @note   This function must not be called once the concerned timer is enabled
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_EnableDLYPRT(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_OUTR_DLYPRTEN);
}

/**
  * @brief  Disable delayed protection (DLYPRT) for a given timer.
  * @rmtoll OUTxR      DLYPRTEN       LL_HRTIM_TIM_DisableDLYPRT
  * @note   This function must not be called once the concerned timer is enabled
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_DisableDLYPRT(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_OUTR_DLYPRTEN);
}

/**
  * @brief  Indicate whether delayed protection (DLYPRT) is enabled for a given timer.
  * @rmtoll OUTxR      DLYPRTEN       LL_HRTIM_TIM_IsEnabledDLYPRT
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of DLYPRTEN bit in HRTIM_OUTxR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_IsEnabledDLYPRT(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_OUTR_DLYPRTEN) == HRTIM_OUTR_DLYPRTEN);
}

/**
  * @brief  Enable the fault channel(s) for a given timer.
  * @rmtoll FLTxR      FLT1EN       LL_HRTIM_TIM_EnableFault\n
  *         FLTxR      FLT2EN       LL_HRTIM_TIM_EnableFault\n
  *         FLTxR      FLT3EN       LL_HRTIM_TIM_EnableFault\n
  *         FLTxR      FLT4EN       LL_HRTIM_TIM_EnableFault\n
  *         FLTxR      FLT5EN       LL_HRTIM_TIM_EnableFault
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Faults This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_FAULT_1
  *         @arg @ref LL_HRTIM_FAULT_2
  *         @arg @ref LL_HRTIM_FAULT_3
  *         @arg @ref LL_HRTIM_FAULT_4
  *         @arg @ref LL_HRTIM_FAULT_5
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_EnableFault(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Faults)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].FLTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, Faults);
}

/**
  * @brief  Disable the fault channel(s) for a given timer.
  * @rmtoll FLTxR      FLT1EN       LL_HRTIM_TIM_DisableFault\n
  *         FLTxR      FLT2EN       LL_HRTIM_TIM_DisableFault\n
  *         FLTxR      FLT3EN       LL_HRTIM_TIM_DisableFault\n
  *         FLTxR      FLT4EN       LL_HRTIM_TIM_DisableFault\n
  *         FLTxR      FLT5EN       LL_HRTIM_TIM_DisableFault
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Faults This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_FAULT_1
  *         @arg @ref LL_HRTIM_FAULT_2
  *         @arg @ref LL_HRTIM_FAULT_3
  *         @arg @ref LL_HRTIM_FAULT_4
  *         @arg @ref LL_HRTIM_FAULT_5
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_DisableFault(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Faults)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].FLTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, Faults);
}

/**
  * @brief  Indicate whether the fault channel is enabled for a given timer.
  * @rmtoll FLTxR      FLT1EN       LL_HRTIM_TIM_IsEnabledFault\n
  *         FLTxR      FLT2EN       LL_HRTIM_TIM_IsEnabledFault\n
  *         FLTxR      FLT3EN       LL_HRTIM_TIM_IsEnabledFault\n
  *         FLTxR      FLT4EN       LL_HRTIM_TIM_IsEnabledFault\n
  *         FLTxR      FLT5EN       LL_HRTIM_TIM_IsEnabledFault
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Fault This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FAULT_1
  *         @arg @ref LL_HRTIM_FAULT_2
  *         @arg @ref LL_HRTIM_FAULT_3
  *         @arg @ref LL_HRTIM_FAULT_4
  *         @arg @ref LL_HRTIM_FAULT_5
  * @retval State of FLTxEN bit in HRTIM_FLTxR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_IsEnabledFault(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Fault)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].FLTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, Fault) == (Fault));
}

/**
  * @brief  Lock the fault conditioning set-up for a given timer.
  * @rmtoll FLTxR      FLTLCK       LL_HRTIM_TIM_LockFault
  * @note Timer fault-related set-up is frozen until the next HRTIM or system reset
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_LockFault(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].FLTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_FLTR_FLTLCK);
}

/**
  * @brief  Define how the timer behaves during a burst mode operation.
  * @rmtoll BMCR      MTBM       LL_HRTIM_TIM_SetBurstModeOption\n
  *         BMCR      TABM       LL_HRTIM_TIM_SetBurstModeOption\n
  *         BMCR      TBBM       LL_HRTIM_TIM_SetBurstModeOption\n
  *         BMCR      TCBM       LL_HRTIM_TIM_SetBurstModeOption\n
  *         BMCR      TDBM       LL_HRTIM_TIM_SetBurstModeOption\n
  *         BMCR      TEBM       LL_HRTIM_TIM_SetBurstModeOption
  * @note This function must not be called when the burst mode is enabled
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  BurtsModeOption This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_BURSTMODE_MAINTAINCLOCK
  *         @arg @ref LL_HRTIM_BURSTMODE_RESETCOUNTER
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetBurstModeOption(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t BurtsModeOption)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  MODIFY_REG(HRTIMx->sCommonRegs.BMCR, Timer, BurtsModeOption << iTimer);
}

/**
  * @brief  Retrieve how the timer behaves during a burst mode operation.
  * @rmtoll BMCR      MCR        LL_HRTIM_TIM_GetBurstModeOption\n
  *         BMCR      TABM       LL_HRTIM_TIM_GetBurstModeOption\n
  *         BMCR      TBBM       LL_HRTIM_TIM_GetBurstModeOption\n
  *         BMCR      TCBM       LL_HRTIM_TIM_GetBurstModeOption\n
  *         BMCR      TDBM       LL_HRTIM_TIM_GetBurstModeOption\n
  *         BMCR      TEBM       LL_HRTIM_TIM_GetBurstModeOption
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval BurtsMode This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_BURSTMODE_MAINTAINCLOCK
  *         @arg @ref LL_HRTIM_BURSTMODE_RESETCOUNTER
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetBurstModeOption(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  return (READ_BIT(HRTIMx->sCommonRegs.BMCR, Timer) >> iTimer);
}

/**
  * @brief  Program which registers are to be written by Burst DMA transfers.
  * @rmtoll BDMUPDR      MTBM        LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDMUPDR      MICR        LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDMUPDR      MDIER       LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDMUPDR      MCNT        LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDMUPDR      MPER        LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDMUPDR      MREP        LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDMUPDR      MCMP1       LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDMUPDR      MCMP2       LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDMUPDR      MCMP3       LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDMUPDR      MCMP4       LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxCR      LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxICR     LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxDIER    LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxCNT     LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxPER     LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxREP     LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxCMP1    LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxCMP2    LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxCMP3    LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxCMP4    LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxDTR     LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxSET1R   LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxRST1R   LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxSET2R   LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxRST2R   LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIAEEFR1    LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxEEFR2   LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxRSTR    LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxOUTR    LL_HRTIM_TIM_ConfigBurstDMA\n
  *         BDTxUPDR     TIMxLTCH    LL_HRTIM_TIM_ConfigBurstDMA
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Registers Registers to be updated by the DMA request
  *
  *         For Master timer this parameter can be can be a combination of the following values:
  *         @arg @ref LL_HRTIM_BURSTDMA_NONE
  *         @arg @ref LL_HRTIM_BURSTDMA_MCR
  *         @arg @ref LL_HRTIM_BURSTDMA_MICR
  *         @arg @ref LL_HRTIM_BURSTDMA_MDIER
  *         @arg @ref LL_HRTIM_BURSTDMA_MCNT
  *         @arg @ref LL_HRTIM_BURSTDMA_MPER
  *         @arg @ref LL_HRTIM_BURSTDMA_MREP
  *         @arg @ref LL_HRTIM_BURSTDMA_MCMP1
  *         @arg @ref LL_HRTIM_BURSTDMA_MCMP2
  *         @arg @ref LL_HRTIM_BURSTDMA_MCMP3
  *         @arg @ref LL_HRTIM_BURSTDMA_MCMP4
  *
  *         For Timers A..E this parameter can be can be a combination of the following values:
  *         @arg @ref LL_HRTIM_BURSTDMA_NONE
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMMCR
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMICR
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMDIER
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMCNT
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMPER
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMREP
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMCMP1
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMCMP2
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMCMP3
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMCMP4
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMDTR
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMSET1R
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMRST1R
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMSET2R
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMRST2R
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMEEFR1
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMEEFR2
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMRSTR
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMCHPR
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMOUTR
  *         @arg @ref LL_HRTIM_BURSTDMA_TIMFLTR
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_ConfigBurstDMA(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Registers)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.BDMUPR) + 4 * iTimer));
  WRITE_REG(*pReg, Registers);
}

/**
  * @brief  Indicate on which output the signal is currently applied.
  * @rmtoll TIMxISR      CPPSTAT        LL_HRTIM_TIM_GetCurrentPushPullStatus
  * @note Only significant when the timer operates in push-pull mode.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval CPPSTAT This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_CPPSTAT_OUTPUT1
  *         @arg @ref LL_HRTIM_CPPSTAT_OUTPUT2
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetCurrentPushPullStatus(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MISR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMISR_CPPSTAT));
}

/**
  * @brief  Indicate on which output the signal was applied, in push-pull mode, balanced fault mode or delayed idle mode, when the protection was triggered.
  * @rmtoll TIMxISR      IPPSTAT        LL_HRTIM_TIM_GetIdlePushPullStatus
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval IPPSTAT This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_IPPSTAT_OUTPUT1
  *         @arg @ref LL_HRTIM_IPPSTAT_OUTPUT2
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetIdlePushPullStatus(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MISR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMISR_IPPSTAT));
}

/**
  * @brief  Set the event filter for a given timer.
  * @rmtoll EEFxR1      EE1LTCH        LL_HRTIM_TIM_SetEventFilter\n
  *         EEFxR1      EE2LTCH        LL_HRTIM_TIM_SetEventFilter\n
  *         EEFxR1      EE3LTCH        LL_HRTIM_TIM_SetEventFilter\n
  *         EEFxR1      EE4LTCH        LL_HRTIM_TIM_SetEventFilter\n
  *         EEFxR1      EE5LTCH        LL_HRTIM_TIM_SetEventFilter\n
  *         EEFxR2      EE6LTCH        LL_HRTIM_TIM_SetEventFilter\n
  *         EEFxR2      EE7LTCH        LL_HRTIM_TIM_SetEventFilter\n
  *         EEFxR2      EE8LTCH        LL_HRTIM_TIM_SetEventFilter\n
  *         EEFxR2      EE9LTCH        LL_HRTIM_TIM_SetEventFilter\n
  *         EEFxR2      EE10LTCH       LL_HRTIM_TIM_SetEventFilter
  * @note This function must not be called when the timer counter is enabled.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Event This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EVENT_1
  *         @arg @ref LL_HRTIM_EVENT_2
  *         @arg @ref LL_HRTIM_EVENT_3
  *         @arg @ref LL_HRTIM_EVENT_4
  *         @arg @ref LL_HRTIM_EVENT_5
  *         @arg @ref LL_HRTIM_EVENT_6
  *         @arg @ref LL_HRTIM_EVENT_7
  *         @arg @ref LL_HRTIM_EVENT_8
  *         @arg @ref LL_HRTIM_EVENT_9
  *         @arg @ref LL_HRTIM_EVENT_10
  * @param  Filter This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EEFLTR_NONE
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGCMP1
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGCMP2
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGCMP3
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGCMP4
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGFLTR1
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGFLTR2
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGFLTR3
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGFLTR4
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGFLTR5
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGFLTR6
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGFLTR7
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGFLTR8
  *         @arg @ref LL_HRTIM_EEFLTR_WINDOWINGCMP2
  *         @arg @ref LL_HRTIM_EEFLTR_WINDOWINGCMP3
  *         @arg @ref LL_HRTIM_EEFLTR_WINDOWINGTIM
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetEventFilter(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Event, uint32_t Filter)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - POSITION_VAL(LL_HRTIM_TIMER_A));
  register uint32_t iEvent = (uint8_t)(POSITION_VAL(Event) - POSITION_VAL(LL_HRTIM_EVENT_1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].EEFxR1) +
                                                    REG_OFFSET_TAB_TIMER[iTimer] + REG_OFFSET_TAB_EECR[iEvent]));
  MODIFY_REG(*pReg, (HRTIM_EEFR1_EE1FLTR << REG_SHIFT_TAB_EExSRC[iEvent]), (Filter << REG_SHIFT_TAB_EExSRC[iEvent]));
}

/**
  * @brief  Get actual event filter settings for a given timer.
  * @rmtoll EEFxR1      EE1FLTR        LL_HRTIM_TIM_GetEventFilter\n
  *         EEFxR1      EE2FLTR        LL_HRTIM_TIM_GetEventFilter\n
  *         EEFxR1      EE3FLTR        LL_HRTIM_TIM_GetEventFilter\n
  *         EEFxR1      EE4FLTR        LL_HRTIM_TIM_GetEventFilter\n
  *         EEFxR1      EE5FLTR        LL_HRTIM_TIM_GetEventFilter\n
  *         EEFxR2      EE6FLTR        LL_HRTIM_TIM_GetEventFilter\n
  *         EEFxR2      EE7FLTR        LL_HRTIM_TIM_GetEventFilter\n
  *         EEFxR2      EE8FLTR        LL_HRTIM_TIM_GetEventFilter\n
  *         EEFxR2      EE9FLTR        LL_HRTIM_TIM_GetEventFilter\n
  *         EEFxR2      EE10FLTR       LL_HRTIM_TIM_GetEventFilter
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Event This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EVENT_1
  *         @arg @ref LL_HRTIM_EVENT_2
  *         @arg @ref LL_HRTIM_EVENT_3
  *         @arg @ref LL_HRTIM_EVENT_4
  *         @arg @ref LL_HRTIM_EVENT_5
  *         @arg @ref LL_HRTIM_EVENT_6
  *         @arg @ref LL_HRTIM_EVENT_7
  *         @arg @ref LL_HRTIM_EVENT_8
  *         @arg @ref LL_HRTIM_EVENT_9
  *         @arg @ref LL_HRTIM_EVENT_10
  * @retval Filter This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EEFLTR_NONE
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGCMP1
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGCMP2
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGCMP3
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGCMP4
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGFLTR1
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGFLTR2
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGFLTR3
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGFLTR4
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGFLTR5
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGFLTR6
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGFLTR7
  *         @arg @ref LL_HRTIM_EEFLTR_BLANKINGFLTR8
  *         @arg @ref LL_HRTIM_EEFLTR_WINDOWINGCMP2
  *         @arg @ref LL_HRTIM_EEFLTR_WINDOWINGCMP3
  *         @arg @ref LL_HRTIM_EEFLTR_WINDOWINGTIM
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetEventFilter(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Event)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - POSITION_VAL(LL_HRTIM_TIMER_A));
  register uint32_t iEvent = (uint8_t)(POSITION_VAL(Event) - POSITION_VAL(LL_HRTIM_EVENT_1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].EEFxR1) +
                                                    REG_OFFSET_TAB_TIMER[iTimer] + REG_OFFSET_TAB_EECR[iEvent]));
  return (READ_BIT(*pReg, HRTIM_EEFR1_EE1FLTR << REG_SHIFT_TAB_EExSRC[iEvent]) >>  REG_SHIFT_TAB_EExSRC[iEvent]);
}

/**
  * @brief  Enable or disable event latch mechanism for a given timer.
  * @rmtoll EEFxR1      EE1LTCH        LL_HRTIM_TIM_SetEventLatchStatus\n
  *         EEFxR1      EE2LTCH        LL_HRTIM_TIM_SetEventLatchStatus\n
  *         EEFxR1      EE3LTCH        LL_HRTIM_TIM_SetEventLatchStatus\n
  *         EEFxR1      EE4LTCH        LL_HRTIM_TIM_SetEventLatchStatus\n
  *         EEFxR1      EE5LTCH        LL_HRTIM_TIM_SetEventLatchStatus\n
  *         EEFxR2      EE6LTCH        LL_HRTIM_TIM_SetEventLatchStatus\n
  *         EEFxR2      EE7LTCH        LL_HRTIM_TIM_SetEventLatchStatus\n
  *         EEFxR2      EE8LTCH        LL_HRTIM_TIM_SetEventLatchStatus\n
  *         EEFxR2      EE9LTCH        LL_HRTIM_TIM_SetEventLatchStatus\n
  *         EEFxR2      EE10LTCH       LL_HRTIM_TIM_SetEventLatchStatus
  * @note This function must not be called when the timer counter is enabled.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Event This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EVENT_1
  *         @arg @ref LL_HRTIM_EVENT_2
  *         @arg @ref LL_HRTIM_EVENT_3
  *         @arg @ref LL_HRTIM_EVENT_4
  *         @arg @ref LL_HRTIM_EVENT_5
  *         @arg @ref LL_HRTIM_EVENT_6
  *         @arg @ref LL_HRTIM_EVENT_7
  *         @arg @ref LL_HRTIM_EVENT_8
  *         @arg @ref LL_HRTIM_EVENT_9
  *         @arg @ref LL_HRTIM_EVENT_10
  * @param  LatchStatus This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EELATCH_DISABLED
  *         @arg @ref LL_HRTIM_EELATCH_ENABLED
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_TIM_SetEventLatchStatus(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Event,
                                                      uint32_t LatchStatus)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - POSITION_VAL(LL_HRTIM_TIMER_A));
  register uint32_t iEvent = (uint8_t)(POSITION_VAL(Event) - POSITION_VAL(LL_HRTIM_EVENT_1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].EEFxR1) +
                                                    REG_OFFSET_TAB_TIMER[iTimer] + REG_OFFSET_TAB_EECR[iEvent]));
  MODIFY_REG(*pReg, (HRTIM_EEFR1_EE1LTCH << REG_SHIFT_TAB_EExSRC[iEvent]), (LatchStatus << REG_SHIFT_TAB_EExSRC[iEvent]));
}

/**
  * @brief  Get actual event latch status for a given timer.
  * @rmtoll EEFxR1      EE1LTCH        LL_HRTIM_TIM_GetEventLatchStatus\n
  *         EEFxR1      EE2LTCH        LL_HRTIM_TIM_GetEventLatchStatus\n
  *         EEFxR1      EE3LTCH        LL_HRTIM_TIM_GetEventLatchStatus\n
  *         EEFxR1      EE4LTCH        LL_HRTIM_TIM_GetEventLatchStatus\n
  *         EEFxR1      EE5LTCH        LL_HRTIM_TIM_GetEventLatchStatus\n
  *         EEFxR2      EE6LTCH        LL_HRTIM_TIM_GetEventLatchStatus\n
  *         EEFxR2      EE7LTCH        LL_HRTIM_TIM_GetEventLatchStatus\n
  *         EEFxR2      EE8LTCH        LL_HRTIM_TIM_GetEventLatchStatus\n
  *         EEFxR2      EE9LTCH        LL_HRTIM_TIM_GetEventLatchStatus\n
  *         EEFxR2      EE10LTCH       LL_HRTIM_TIM_GetEventLatchStatus
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Event This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EVENT_1
  *         @arg @ref LL_HRTIM_EVENT_2
  *         @arg @ref LL_HRTIM_EVENT_3
  *         @arg @ref LL_HRTIM_EVENT_4
  *         @arg @ref LL_HRTIM_EVENT_5
  *         @arg @ref LL_HRTIM_EVENT_6
  *         @arg @ref LL_HRTIM_EVENT_7
  *         @arg @ref LL_HRTIM_EVENT_8
  *         @arg @ref LL_HRTIM_EVENT_9
  *         @arg @ref LL_HRTIM_EVENT_10
  * @retval LatchStatus This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EELATCH_DISABLED
  *         @arg @ref LL_HRTIM_EELATCH_ENABLED
  */
__STATIC_INLINE uint32_t LL_HRTIM_TIM_GetEventLatchStatus(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Event)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - POSITION_VAL(LL_HRTIM_TIMER_A));
  register uint32_t iEvent = (uint8_t)(POSITION_VAL(Event) - POSITION_VAL(LL_HRTIM_EVENT_1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].EEFxR1) +
                                                    REG_OFFSET_TAB_TIMER[iTimer] + REG_OFFSET_TAB_EECR[iEvent]));
  return (READ_BIT(*pReg, HRTIM_EEFR1_EE1LTCH << REG_SHIFT_TAB_EExSRC[iEvent]) >>  REG_SHIFT_TAB_EExSRC[iEvent]);
}

/**
  * @}
  */

/** @defgroup HRTIM_EF_Dead_Time_Configuration Dead_Time_Configuration
  * @{
  */

/**
  * @brief  Configure the dead time insertion feature for a given timer.
  * @rmtoll DTxR      DTPRSC     LL_HRTIM_DT_Config\n
  *         DTxR      SDTF       LL_HRTIM_DT_Config\n
  *         DTxR      SDRT       LL_HRTIM_DT_Config
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Configuration This parameter must be a combination of all the following values:
  *         @arg @ref LL_HRTIM_DT_PRESCALER_MUL8 or ... or @ref LL_HRTIM_DT_PRESCALER_DIV16
  *         @arg @ref LL_HRTIM_DT_RISING_POSITIVE or @ref LL_HRTIM_DT_RISING_NEGATIVE
  *         @arg @ref LL_HRTIM_DT_FALLING_POSITIVE or @ref LL_HRTIM_DT_FALLING_NEGATIVE
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DT_Config(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Configuration)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].DTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(*pReg, HRTIM_DTR_SDTF | HRTIM_DTR_DTPRSC | HRTIM_DTR_SDTR, Configuration);
}

/**
  * @brief  Set the deadtime prescaler value.
  * @rmtoll DTxR      DTPRSC     LL_HRTIM_DT_SetPrescaler
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_DT_PRESCALER_MUL8
  *         @arg @ref LL_HRTIM_DT_PRESCALER_MUL4
  *         @arg @ref LL_HRTIM_DT_PRESCALER_MUL2
  *         @arg @ref LL_HRTIM_DT_PRESCALER_DIV1
  *         @arg @ref LL_HRTIM_DT_PRESCALER_DIV2
  *         @arg @ref LL_HRTIM_DT_PRESCALER_DIV4
  *         @arg @ref LL_HRTIM_DT_PRESCALER_DIV8
  *         @arg @ref LL_HRTIM_DT_PRESCALER_DIV16
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DT_SetPrescaler(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Prescaler)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].DTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(*pReg, HRTIM_DTR_DTPRSC, Prescaler);
}

/**
  * @brief  Get actual deadtime prescaler value.
  * @rmtoll DTxR      DTPRSC     LL_HRTIM_DT_GetPrescaler
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_DT_PRESCALER_MUL8
  *         @arg @ref LL_HRTIM_DT_PRESCALER_MUL4
  *         @arg @ref LL_HRTIM_DT_PRESCALER_MUL2
  *         @arg @ref LL_HRTIM_DT_PRESCALER_DIV1
  *         @arg @ref LL_HRTIM_DT_PRESCALER_DIV2
  *         @arg @ref LL_HRTIM_DT_PRESCALER_DIV4
  *         @arg @ref LL_HRTIM_DT_PRESCALER_DIV8
  *         @arg @ref LL_HRTIM_DT_PRESCALER_DIV16
  */
__STATIC_INLINE uint32_t LL_HRTIM_DT_GetPrescaler(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].DTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_DTR_DTPRSC));
}

/**
  * @brief  Set the deadtime rising value.
  * @rmtoll DTxR      DTR       LL_HRTIM_DT_SetRisingValue
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  RisingValue Value between 0 and 0x1FF
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DT_SetRisingValue(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t RisingValue)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].DTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(*pReg, HRTIM_DTR_DTR, RisingValue);
}

/**
  * @brief  Get actual deadtime rising value.
  * @rmtoll DTxR      DTR       LL_HRTIM_DT_GetRisingValue
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
 * @retval RisingValue Value between 0 and 0x1FF
  */
__STATIC_INLINE uint32_t LL_HRTIM_DT_GetRisingValue(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].DTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_DTR_DTR));
}

/**
  * @brief  Set the deadtime sign on rising edge.
  * @rmtoll DTxR      SDTR       LL_HRTIM_DT_SetRisingSign
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  RisingSign This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_DT_RISING_POSITIVE
  *         @arg @ref LL_HRTIM_DT_RISING_NEGATIVE
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DT_SetRisingSign(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t RisingSign)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].DTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(*pReg, HRTIM_DTR_SDTR, RisingSign);
}

/**
  * @brief  Get actual deadtime sign on rising edge.
  * @rmtoll DTxR      SDTR       LL_HRTIM_DT_GetRisingSign
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval RisingSign This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_DT_RISING_POSITIVE
  *         @arg @ref LL_HRTIM_DT_RISING_NEGATIVE
  */
__STATIC_INLINE uint32_t LL_HRTIM_DT_GetRisingSign(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].DTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_DTR_SDTR));
}

/**
  * @brief  Set the deadime falling value.
  * @rmtoll DTxR      DTF       LL_HRTIM_DT_SetFallingValue
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  FallingValue Value between 0 and 0x1FF
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DT_SetFallingValue(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t FallingValue)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].DTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(*pReg, HRTIM_DTR_DTF, FallingValue << HRTIM_DTR_DTF_Pos);
}

/**
  * @brief  Get actual deadtime falling value
  * @rmtoll DTxR      DTF       LL_HRTIM_DT_GetFallingValue
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
 * @retval FallingValue Value between 0 and 0x1FF
  */
__STATIC_INLINE uint32_t LL_HRTIM_DT_GetFallingValue(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].DTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return ((READ_BIT(*pReg, HRTIM_DTR_DTF)) >> HRTIM_DTR_DTF_Pos);
}

/**
  * @brief  Set the deadtime sign on falling edge.
  * @rmtoll DTxR      SDTF       LL_HRTIM_DT_SetFallingSign
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  FallingSign This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_DT_FALLING_POSITIVE
  *         @arg @ref LL_HRTIM_DT_FALLING_NEGATIVE
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DT_SetFallingSign(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t FallingSign)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].DTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(*pReg, HRTIM_DTR_SDTF, FallingSign);
}

/**
  * @brief  Get actual deadtime sign on falling edge.
  * @rmtoll DTxR      SDTF       LL_HRTIM_DT_GetFallingSign
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval FallingSign This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_DT_FALLING_POSITIVE
  *         @arg @ref LL_HRTIM_DT_FALLING_NEGATIVE
  */
__STATIC_INLINE uint32_t LL_HRTIM_DT_GetFallingSign(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].DTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_DTR_SDTF));
}

/**
  * @brief  Lock the deadtime value and sign on rising edge.
  * @rmtoll DTxR      DTRLK       LL_HRTIM_DT_LockRising
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DT_LockRising(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].DTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_DTR_DTRLK);
}

/**
  * @brief  Lock the deadtime sign on rising edge.
  * @rmtoll DTxR      DTRSLK       LL_HRTIM_DT_LockRisingSign
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DT_LockRisingSign(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].DTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_DTR_DTRSLK);
}

/**
  * @brief  Lock the deadtime value and sign on falling edge.
  * @rmtoll DTxR      DTFLK       LL_HRTIM_DT_LockFalling
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DT_LockFalling(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].DTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_DTR_DTFLK);
}

/**
  * @brief  Lock the deadtime sign on falling edge.
  * @rmtoll DTxR      DTFSLK       LL_HRTIM_DT_LockFallingSign
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DT_LockFallingSign(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].DTxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_DTR_DTFSLK);
}

/**
  * @}
  */

/** @defgroup HRTIM_EF_Chopper_Mode_Configuration Chopper_Mode_Configuration
  * @{
  */

/**
  * @brief  Configure the chopper stage for a given timer.
  * @rmtoll CHPxR      CARFRQ       LL_HRTIM_CHP_Config\n
  *         CHPxR      CARDTY       LL_HRTIM_CHP_Config\n
  *         CHPxR      STRTPW       LL_HRTIM_CHP_Config
  * @note This function must not be called if the chopper mode is already
  *       enabled for one of the timer outputs.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Configuration This parameter must be a combination of all the following values:
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV16 or ... or @ref LL_HRTIM_CHP_PRESCALER_DIV256
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_0 or ... or @ref LL_HRTIM_CHP_DUTYCYCLE_875
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_16 or ... or @ref LL_HRTIM_CHP_PULSEWIDTH_256
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_CHP_Config(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Configuration)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].CHPxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(*pReg, HRTIM_CHPR_STRPW | HRTIM_CHPR_CARDTY | HRTIM_CHPR_CARFRQ, Configuration);
}

/**
  * @brief  Set prescaler determining the carrier frequency to be added on top
  *         of the timer output signals when chopper mode is enabled.
  * @rmtoll CHPxR      CARFRQ       LL_HRTIM_CHP_SetPrescaler
  * @note This function must not be called if the chopper mode is already
  *       enabled for one of the timer outputs.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV16
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV32
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV48
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV64
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV80
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV96
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV112
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV128
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV144
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV160
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV176
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV192
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV208
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV224
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV240
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV256
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_CHP_SetPrescaler(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t Prescaler)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].CHPxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(*pReg, HRTIM_CHPR_CARFRQ, Prescaler);
}

/**
  * @brief  Get actual chopper stage prescaler value.
  * @rmtoll CHPxR      CARFRQ       LL_HRTIM_CHP_GetPrescaler
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV16
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV32
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV48
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV64
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV80
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV96
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV112
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV128
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV144
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV160
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV176
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV192
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV208
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV224
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV240
  *         @arg @ref LL_HRTIM_CHP_PRESCALER_DIV256
  */
__STATIC_INLINE uint32_t LL_HRTIM_CHP_GetPrescaler(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].CHPxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_CHPR_CARFRQ));
}

/**
  * @brief  Set the chopper duty cycle.
  * @rmtoll CHPxR      CARDTY       LL_HRTIM_CHP_SetDutyCycle
  * @note Duty cycle can be adjusted by 1/8 step (from 0/8 up to 7/8)
  * @note This function must not be called if the chopper mode is already
  *       enabled for one of the timer outputs.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  DutyCycle This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_0
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_125
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_250
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_375
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_500
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_625
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_750
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_875
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_CHP_SetDutyCycle(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t DutyCycle)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].CHPxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(*pReg, HRTIM_CHPR_CARDTY, DutyCycle);
}

/**
  * @brief  Get actual chopper duty cycle.
  * @rmtoll CHPxR      CARDTY       LL_HRTIM_CHP_GetDutyCycle
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval DutyCycle This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_0
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_125
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_250
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_375
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_500
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_625
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_750
  *         @arg @ref LL_HRTIM_CHP_DUTYCYCLE_875
  */
__STATIC_INLINE uint32_t LL_HRTIM_CHP_GetDutyCycle(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].CHPxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_CHPR_CARDTY));
}

/**
  * @brief  Set the start pulse width.
  * @rmtoll CHPxR      STRPW       LL_HRTIM_CHP_SetPulseWidth
  * @note This function must not be called if the chopper mode is already
  *       enabled for one of the timer outputs.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @param  PulseWidth This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_16
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_32
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_48
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_64
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_80
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_96
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_112
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_128
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_144
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_160
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_176
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_192
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_208
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_224
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_240
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_256
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_CHP_SetPulseWidth(HRTIM_TypeDef *HRTIMx, uint32_t Timer, uint32_t PulseWidth)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].CHPxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  MODIFY_REG(*pReg, HRTIM_CHPR_STRPW, PulseWidth);
}

/**
  * @brief  Get actual start pulse width.
  * @rmtoll CHPxR      STRPW       LL_HRTIM_CHP_GetPulseWidth
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval PulseWidth This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_16
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_32
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_48
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_64
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_80
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_96
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_112
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_128
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_144
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_160
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_176
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_192
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_208
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_224
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_240
  *         @arg @ref LL_HRTIM_CHP_PULSEWIDTH_256
  */
__STATIC_INLINE uint32_t LL_HRTIM_CHP_GetPulseWidth(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_TACEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].CHPxR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_CHPR_STRPW));
}

/**
  * @}
  */

/** @defgroup HRTIM_EF_Output_Management Output_Management
  * @{
  */

/**
  * @brief  Set the timer output set source.
  * @rmtoll SETx1R      SST          LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      RESYNC       LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      PER          LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      CMP1         LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      CMP2         LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      CMP3         LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      CMP4         LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      MSTPER       LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      MSTCMP1      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      MSTCMP2      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      MSTCMP3      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      MSTCMP4      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT1     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT2     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT3     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT4     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT5     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT6     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT7     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT8     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT9     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT1      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT2      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT3      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT4      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT5      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT6      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT7      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT8      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT9      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT10     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      UPDATE       LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      SST          LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      RESYNC       LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      PER          LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      CMP1         LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      CMP2         LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      CMP3         LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      CMP4         LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      MSTPER       LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      MSTCMP1      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      MSTCMP2      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      MSTCMP3      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      MSTCMP4      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT1     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT2     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT3     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT4     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT5     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT6     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT7     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT8     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      TIMEVNT9     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT1      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT2      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT3      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT4      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT5      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT6      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT7      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT8      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT9      LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      EXEVNT10     LL_HRTIM_OUT_SetOutputSetSrc\n
  *         SETx1R      UPDATE       LL_HRTIM_OUT_SetOutputSetSrc
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @param SetSrc This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_CROSSBAR_NONE
  *         @arg @ref LL_HRTIM_CROSSBAR_RESYNC
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMPER
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMCMP1
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMCMP2
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMCMP3
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMCMP4
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERPER
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERCMP1
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERCMP2
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERCMP3
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERCMP4
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_1
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_2
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_3
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_4
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_5
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_6
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_7
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_8
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_9
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_1
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_2
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_3
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_4
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_5
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_6
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_7
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_8
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_9
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_10
  *         @arg @ref LL_HRTIM_CROSSBAR_UPDATE
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_OUT_SetOutputSetSrc(HRTIM_TypeDef *HRTIMx, uint32_t Output, uint32_t SetSrc)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].SETx1R) +
                                                    REG_OFFSET_TAB_SETxR[iOutput]));
  WRITE_REG(*pReg, SetSrc);
}

/**
  * @brief  Get the timer output set source.
  * @rmtoll SETx1R      SST          LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      RESYNC       LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      PER          LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      CMP1         LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      CMP2         LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      CMP3         LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      CMP4         LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      MSTPER       LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      MSTCMP1      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      MSTCMP2      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      MSTCMP3      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      MSTCMP4      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT1     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT2     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT3     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT4     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT5     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT6     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT7     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT8     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT9     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT1      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT2      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT3      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT4      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT5      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT6      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT7      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT8      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT9      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT10     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      UPDATE       LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      SST          LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      RESYNC       LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      PER          LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      CMP1         LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      CMP2         LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      CMP3         LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      CMP4         LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      MSTPER       LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      MSTCMP1      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      MSTCMP2      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      MSTCMP3      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      MSTCMP4      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT1     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT2     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT3     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT4     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT5     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT6     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT7     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT8     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      TIMEVNT9     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT1      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT2      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT3      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT4      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT5      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT6      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT7      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT8      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT9      LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      EXEVNT10     LL_HRTIM_OUT_GetOutputSetSrc\n
  *         SETx1R      UPDATE       LL_HRTIM_OUT_GetOutputSetSrc
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @retval SetSrc This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_CROSSBAR_NONE
  *         @arg @ref LL_HRTIM_CROSSBAR_RESYNC
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMPER
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMCMP1
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMCMP2
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMCMP3
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMCMP4
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERPER
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERCMP1
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERCMP2
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERCMP3
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERCMP4
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_1
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_2
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_3
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_4
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_5
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_6
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_7
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_8
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_9
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_1
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_2
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_3
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_4
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_5
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_6
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_7
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_8
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_9
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_10
  *         @arg @ref LL_HRTIM_CROSSBAR_UPDATE
  */
__STATIC_INLINE uint32_t LL_HRTIM_OUT_GetOutputSetSrc(HRTIM_TypeDef *HRTIMx, uint32_t Output)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].SETx1R) +
                                                    REG_OFFSET_TAB_SETxR[iOutput]));
  return (uint32_t) READ_REG(*pReg);
}

/**
  * @brief  Set the timer output reset source.
  * @rmtoll RSTx1R      RST          LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      RESYNC       LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      PER          LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      CMP1         LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      CMP2         LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      CMP3         LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      CMP4         LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      MSTPER       LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      MSTCMP1      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      MSTCMP2      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      MSTCMP3      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      MSTCMP4      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT1     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT2     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT3     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT4     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT5     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT6     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT7     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT8     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT9     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT1      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT2      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT3      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT4      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT5      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT6      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT7      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT8      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT9      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT10     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      UPDATE       LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      RST          LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      RESYNC       LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      PER          LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      CMP1         LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      CMP2         LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      CMP3         LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      CMP4         LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      MSTPER       LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      MSTCMP1      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      MSTCMP2      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      MSTCMP3      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      MSTCMP4      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT1     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT2     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT3     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT4     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT5     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT6     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT7     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT8     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      TIMEVNT9     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT1      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT2      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT3      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT4      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT5      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT6      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT7      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT8      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT9      LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      EXEVNT10     LL_HRTIM_OUT_SetOutputResetSrc\n
  *         RSTx1R      UPDATE       LL_HRTIM_OUT_SetOutputResetSrc
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @param ResetSrc This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_CROSSBAR_NONE
  *         @arg @ref LL_HRTIM_CROSSBAR_RESYNC
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMPER
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMCMP1
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMCMP2
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMCMP3
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMCMP4
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERPER
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERCMP1
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERCMP2
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERCMP3
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERCMP4
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_1
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_2
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_3
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_4
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_5
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_6
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_7
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_8
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_9
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_1
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_2
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_3
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_4
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_5
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_6
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_7
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_8
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_9
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_10
  *         @arg @ref LL_HRTIM_CROSSBAR_UPDATE
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_OUT_SetOutputResetSrc(HRTIM_TypeDef *HRTIMx, uint32_t Output, uint32_t ResetSrc)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].RSTx1R) +
                                                    REG_OFFSET_TAB_SETxR[iOutput]));
  WRITE_REG(*pReg, ResetSrc);
}

/**
  * @brief  Get the timer output set source.
  * @rmtoll RSTx1R      RST          LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      RESYNC       LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      PER          LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      CMP1         LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      CMP2         LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      CMP3         LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      CMP4         LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      MSTPER       LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      MSTCMP1      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      MSTCMP2      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      MSTCMP3      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      MSTCMP4      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT1     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT2     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT3     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT4     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT5     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT6     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT7     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT8     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT9     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT1      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT2      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT3      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT4      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT5      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT6      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT7      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT8      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT9      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT10     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      UPDATE       LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      RST          LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      RESYNC       LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      PER          LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      CMP1         LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      CMP2         LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      CMP3         LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      CMP4         LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      MSTPER       LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      MSTCMP1      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      MSTCMP2      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      MSTCMP3      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      MSTCMP4      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT1     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT2     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT3     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT4     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT5     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT6     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT7     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT8     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      TIMEVNT9     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT1      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT2      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT3      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT4      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT5      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT6      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT7      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT8      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT9      LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      EXEVNT10     LL_HRTIM_OUT_GetOutputResetSrc\n
  *         RSTx1R      UPDATE       LL_HRTIM_OUT_GetOutputResetSrc
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @retval ResetSrc This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_CROSSBAR_NONE
  *         @arg @ref LL_HRTIM_CROSSBAR_RESYNC
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMPER
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMCMP1
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMCMP2
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMCMP3
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMCMP4
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERPER
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERCMP1
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERCMP2
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERCMP3
  *         @arg @ref LL_HRTIM_CROSSBAR_MASTERCMP4
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_1
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_2
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_3
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_4
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_5
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_6
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_7
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_8
  *         @arg @ref LL_HRTIM_CROSSBAR_TIMEV_9
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_1
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_2
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_3
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_4
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_5
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_6
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_7
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_8
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_9
  *         @arg @ref LL_HRTIM_CROSSBAR_EEV_10
  *         @arg @ref LL_HRTIM_CROSSBAR_UPDATE
  */
__STATIC_INLINE uint32_t LL_HRTIM_OUT_GetOutputResetSrc(HRTIM_TypeDef *HRTIMx, uint32_t Output)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].RSTx1R) +
                                                    REG_OFFSET_TAB_SETxR[iOutput]));
  return (uint32_t) READ_REG(*pReg);
}

/**
  * @brief  Configure a timer output.
  * @rmtoll OUTxR      POL1          LL_HRTIM_OUT_Config\n
  *         OUTxR      IDLEM1        LL_HRTIM_OUT_Config\n
  *         OUTxR      IDLES1        LL_HRTIM_OUT_Config\n
  *         OUTxR      FAULT1        LL_HRTIM_OUT_Config\n
  *         OUTxR      CHP1          LL_HRTIM_OUT_Config\n
  *         OUTxR      DIDL1         LL_HRTIM_OUT_Config\n
  *         OUTxR      POL2          LL_HRTIM_OUT_Config\n
  *         OUTxR      IDLEM2        LL_HRTIM_OUT_Config\n
  *         OUTxR      IDLES2        LL_HRTIM_OUT_Config\n
  *         OUTxR      FAULT2        LL_HRTIM_OUT_Config\n
  *         OUTxR      CHP2          LL_HRTIM_OUT_Config\n
  *         OUTxR      DIDL2         LL_HRTIM_OUT_Config
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @param  Configuration This parameter must be a combination of all the following values:
  *         @arg @ref LL_HRTIM_OUT_POSITIVE_POLARITY or @ref LL_HRTIM_OUT_NEGATIVE_POLARITY
  *         @arg @ref LL_HRTIM_OUT_NO_IDLE or @ref LL_HRTIM_OUT_IDLE_WHEN_BURST
  *         @arg @ref LL_HRTIM_OUT_IDLELEVEL_INACTIVE or @ref LL_HRTIM_OUT_IDLELEVEL_ACTIVE
  *         @arg @ref LL_HRTIM_OUT_FAULTSTATE_NO_ACTION or @ref LL_HRTIM_OUT_FAULTSTATE_ACTIVE or @ref LL_HRTIM_OUT_FAULTSTATE_INACTIVE or @ref LL_HRTIM_OUT_FAULTSTATE_HIGHZ
  *         @arg @ref LL_HRTIM_OUT_CHOPPERMODE_DISABLED or @ref LL_HRTIM_OUT_CHOPPERMODE_ENABLED
  *         @arg @ref LL_HRTIM_OUT_BM_ENTRYMODE_REGULAR or @ref LL_HRTIM_OUT_BM_ENTRYMODE_DELAYED
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_OUT_Config(HRTIM_TypeDef *HRTIMx, uint32_t Output, uint32_t Configuration)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_OUTxR[iOutput]));
  MODIFY_REG(*pReg, (HRTIM_OUT_CONFIG_MASK << REG_SHIFT_TAB_OUTxR[iOutput]),
             (Configuration << REG_SHIFT_TAB_OUTxR[iOutput]));
}

/**
  * @brief  Set the polarity of a timer output.
  * @rmtoll OUTxR      POL1          LL_HRTIM_OUT_SetPolarity\n
  *         OUTxR      POL2          LL_HRTIM_OUT_SetPolarity
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @param  Polarity This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUT_POSITIVE_POLARITY
  *         @arg @ref LL_HRTIM_OUT_NEGATIVE_POLARITY
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_OUT_SetPolarity(HRTIM_TypeDef *HRTIMx, uint32_t Output, uint32_t Polarity)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_OUTxR[iOutput]));
  MODIFY_REG(*pReg, (HRTIM_OUTR_POL1 << REG_SHIFT_TAB_OUTxR[iOutput]), (Polarity << REG_SHIFT_TAB_OUTxR[iOutput]));
}

/**
  * @brief  Get actual polarity of the timer output.
  * @rmtoll OUTxR      POL1          LL_HRTIM_OUT_GetPolarity\n
  *         OUTxR      POL2          LL_HRTIM_OUT_GetPolarity
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @retval Polarity This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUT_POSITIVE_POLARITY
  *         @arg @ref LL_HRTIM_OUT_NEGATIVE_POLARITY
  */
__STATIC_INLINE uint32_t LL_HRTIM_OUT_GetPolarity(HRTIM_TypeDef *HRTIMx, uint32_t Output)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_OUTxR[iOutput]));
  return (READ_BIT(*pReg, (HRTIM_OUTR_POL1 << REG_SHIFT_TAB_OUTxR[iOutput])) >> REG_SHIFT_TAB_OUTxR[iOutput]);
}

/**
  * @brief  Set the output IDLE mode.
  * @rmtoll OUTxR      IDLEM1          LL_HRTIM_OUT_SetIdleMode\n
  *         OUTxR      IDLEM2          LL_HRTIM_OUT_SetIdleMode
  * @note This function must not be called when the burst mode is active
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @param  IdleMode This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUT_NO_IDLE
  *         @arg @ref LL_HRTIM_OUT_IDLE_WHEN_BURST
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_OUT_SetIdleMode(HRTIM_TypeDef *HRTIMx, uint32_t Output, uint32_t IdleMode)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_OUTxR[iOutput]));
  MODIFY_REG(*pReg, (HRTIM_OUTR_IDLM1 << REG_SHIFT_TAB_OUTxR[iOutput]), (IdleMode << REG_SHIFT_TAB_OUTxR[iOutput]));
}

/**
  * @brief  Get actual output IDLE mode.
  * @rmtoll OUTxR      IDLEM1          LL_HRTIM_OUT_GetIdleMode\n
  *         OUTxR      IDLEM2          LL_HRTIM_OUT_GetIdleMode
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @retval IdleMode This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUT_NO_IDLE
  *         @arg @ref LL_HRTIM_OUT_IDLE_WHEN_BURST
  */
__STATIC_INLINE uint32_t LL_HRTIM_OUT_GetIdleMode(HRTIM_TypeDef *HRTIMx, uint32_t Output)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_OUTxR[iOutput]));
  return (READ_BIT(*pReg, (HRTIM_OUTR_IDLM1 << REG_SHIFT_TAB_OUTxR[iOutput])) >> REG_SHIFT_TAB_OUTxR[iOutput]);
}

/**
  * @brief  Set the output IDLE level.
  * @rmtoll OUTxR      IDLES1          LL_HRTIM_OUT_SetIdleLevel\n
  *         OUTxR      IDLES2          LL_HRTIM_OUT_SetIdleLevel
  * @note This function must be called prior enabling the timer.
  * @note Idle level isn't relevant when the output idle mode is set to LL_HRTIM_OUT_NO_IDLE.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @param  IdleLevel This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUT_IDLELEVEL_INACTIVE
  *         @arg @ref LL_HRTIM_OUT_IDLELEVEL_ACTIVE
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_OUT_SetIdleLevel(HRTIM_TypeDef *HRTIMx, uint32_t Output, uint32_t IdleLevel)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_OUTxR[iOutput]));
  MODIFY_REG(*pReg, (HRTIM_OUTR_IDLES1 << REG_SHIFT_TAB_OUTxR[iOutput]), (IdleLevel << REG_SHIFT_TAB_OUTxR[iOutput]));
}

/**
  * @brief  Get actual output IDLE level.
  * @rmtoll OUTxR      IDLES1          LL_HRTIM_OUT_GetIdleLevel\n
  *         OUTxR      IDLES2          LL_HRTIM_OUT_GetIdleLevel
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @retval IdleLevel This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUT_IDLELEVEL_INACTIVE
  *         @arg @ref LL_HRTIM_OUT_IDLELEVEL_ACTIVE
  */
__STATIC_INLINE uint32_t LL_HRTIM_OUT_GetIdleLevel(HRTIM_TypeDef *HRTIMx, uint32_t Output)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_OUTxR[iOutput]));
  return (READ_BIT(*pReg, (HRTIM_OUTR_IDLES1 << REG_SHIFT_TAB_OUTxR[iOutput])) >> REG_SHIFT_TAB_OUTxR[iOutput]);
}

/**
  * @brief  Set the output FAULT state.
  * @rmtoll OUTxR      FAULT1          LL_HRTIM_OUT_SetFaultState\n
  *         OUTxR      FAULT2          LL_HRTIM_OUT_SetFaultState
  * @note This function must not called when the timer is enabled and a fault
  *       channel is enabled at timer level.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @param  FaultState This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUT_FAULTSTATE_NO_ACTION
  *         @arg @ref LL_HRTIM_OUT_FAULTSTATE_ACTIVE
  *         @arg @ref LL_HRTIM_OUT_FAULTSTATE_INACTIVE
  *         @arg @ref LL_HRTIM_OUT_FAULTSTATE_HIGHZ
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_OUT_SetFaultState(HRTIM_TypeDef *HRTIMx, uint32_t Output, uint32_t FaultState)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_OUTxR[iOutput]));
  MODIFY_REG(*pReg, (HRTIM_OUTR_FAULT1 << REG_SHIFT_TAB_OUTxR[iOutput]), (FaultState << REG_SHIFT_TAB_OUTxR[iOutput]));
}

/**
  * @brief  Get actual FAULT state.
  * @rmtoll OUTxR      FAULT1          LL_HRTIM_OUT_GetFaultState\n
  *         OUTxR      FAULT2          LL_HRTIM_OUT_GetFaultState
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @retval FaultState This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUT_FAULTSTATE_NO_ACTION
  *         @arg @ref LL_HRTIM_OUT_FAULTSTATE_ACTIVE
  *         @arg @ref LL_HRTIM_OUT_FAULTSTATE_INACTIVE
  *         @arg @ref LL_HRTIM_OUT_FAULTSTATE_HIGHZ
  */
__STATIC_INLINE uint32_t LL_HRTIM_OUT_GetFaultState(HRTIM_TypeDef *HRTIMx, uint32_t Output)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_OUTxR[iOutput]));
  return (READ_BIT(*pReg, (HRTIM_OUTR_FAULT1 << REG_SHIFT_TAB_OUTxR[iOutput])) >> REG_SHIFT_TAB_OUTxR[iOutput]);
}

/**
  * @brief  Set the output chopper mode.
  * @rmtoll OUTxR      CHP1          LL_HRTIM_OUT_SetChopperMode\n
  *         OUTxR      CHP2          LL_HRTIM_OUT_SetChopperMode
  * @note This function must not called when the timer is enabled.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @param  ChopperMode This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUT_CHOPPERMODE_DISABLED
  *         @arg @ref LL_HRTIM_OUT_CHOPPERMODE_ENABLED
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_OUT_SetChopperMode(HRTIM_TypeDef *HRTIMx, uint32_t Output, uint32_t ChopperMode)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_OUTxR[iOutput]));
  MODIFY_REG(*pReg, (HRTIM_OUTR_CHP1 << REG_SHIFT_TAB_OUTxR[iOutput]), (ChopperMode << REG_SHIFT_TAB_OUTxR[iOutput]));
}

/**
  * @brief  Get actual output chopper mode
  * @rmtoll OUTxR      CHP1          LL_HRTIM_OUT_GetChopperMode\n
  *         OUTxR      CHP2          LL_HRTIM_OUT_GetChopperMode
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @retval ChopperMode This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUT_CHOPPERMODE_DISABLED
  *         @arg @ref LL_HRTIM_OUT_CHOPPERMODE_ENABLED
  */
__STATIC_INLINE uint32_t LL_HRTIM_OUT_GetChopperMode(HRTIM_TypeDef *HRTIMx, uint32_t Output)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_OUTxR[iOutput]));
  return (READ_BIT(*pReg, (HRTIM_OUTR_CHP1 << REG_SHIFT_TAB_OUTxR[iOutput])) >> REG_SHIFT_TAB_OUTxR[iOutput]);
}

/**
  * @brief  Set the output burst mode entry mode.
  * @rmtoll OUTxR      DIDL1          LL_HRTIM_OUT_SetBMEntryMode\n
  *         OUTxR      DIDL2          LL_HRTIM_OUT_SetBMEntryMode
  * @note This function must not called when the timer is enabled.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @param  BMEntryMode This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUT_BM_ENTRYMODE_REGULAR
  *         @arg @ref LL_HRTIM_OUT_BM_ENTRYMODE_DELAYED
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_OUT_SetBMEntryMode(HRTIM_TypeDef *HRTIMx, uint32_t Output, uint32_t BMEntryMode)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_OUTxR[iOutput]));
  MODIFY_REG(*pReg, (HRTIM_OUTR_DIDL1 << REG_SHIFT_TAB_OUTxR[iOutput]), (BMEntryMode << REG_SHIFT_TAB_OUTxR[iOutput]));
}

/**
  * @brief  Get actual output burst mode entry mode.
  * @rmtoll OUTxR      DIDL1          LL_HRTIM_OUT_GetBMEntryMode\n
  *         OUTxR      DIDL2          LL_HRTIM_OUT_GetBMEntryMode
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @retval BMEntryMode This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUT_BM_ENTRYMODE_REGULAR
  *         @arg @ref LL_HRTIM_OUT_BM_ENTRYMODE_DELAYED
  */
__STATIC_INLINE uint32_t LL_HRTIM_OUT_GetBMEntryMode(HRTIM_TypeDef *HRTIMx, uint32_t Output)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].OUTxR) +
                                                    REG_OFFSET_TAB_OUTxR[iOutput]));
  return (READ_BIT(*pReg, (HRTIM_OUTR_DIDL1 << REG_SHIFT_TAB_OUTxR[iOutput])) >> REG_SHIFT_TAB_OUTxR[iOutput]);
}

/**
  * @brief  Get the level (active or inactive) of the designated output when the
  *         delayed protection was triggered.
  * @rmtoll TIMxISR      O1SRSR          LL_HRTIM_OUT_GetDLYPRTOutStatus\n
  *         TIMxISR      O2SRSR          LL_HRTIM_OUT_GetDLYPRTOutStatus
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @retval OutputLevel This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUT_LEVEL_INACTIVE
  *         @arg @ref LL_HRTIM_OUT_LEVEL_ACTIVE
  */
__STATIC_INLINE uint32_t LL_HRTIM_OUT_GetDLYPRTOutStatus(HRTIM_TypeDef *HRTIMx, uint32_t Output)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].TIMxISR) +
                                                    REG_OFFSET_TAB_OUTxR[iOutput]));
  return ((READ_BIT(*pReg, (HRTIM_TIMISR_O1STAT << REG_SHIFT_TAB_OxSTAT[iOutput])) >> REG_SHIFT_TAB_OxSTAT[iOutput]) >>
          HRTIM_TIMISR_O1STAT_Pos);
}

/**
  * @brief  Force the timer output to its active or inactive level.
  * @rmtoll SETx1R      SST          LL_HRTIM_OUT_ForceLevel\n
  *         RSTx1R      SRT          LL_HRTIM_OUT_ForceLevel\n
  *         SETx2R      SST          LL_HRTIM_OUT_ForceLevel\n
  *         RSTx2R      SRT          LL_HRTIM_OUT_ForceLevel
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @param  OutputLevel This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUT_LEVEL_INACTIVE
  *         @arg @ref LL_HRTIM_OUT_LEVEL_ACTIVE
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_OUT_ForceLevel(HRTIM_TypeDef *HRTIMx, uint32_t Output, uint32_t OutputLevel)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].SETx1R) +
                                                    REG_OFFSET_TAB_SETxR[iOutput] + REG_OFFSET_TAB_OUT_LEVEL[OutputLevel]));
  SET_BIT(*pReg, HRTIM_SET1R_SST);
}

/**
  * @brief  Get actual output level, before the output stage (chopper, polarity).
  * @rmtoll TIMxISR     O1CPY          LL_HRTIM_OUT_GetLevel\n
  *         TIMxISR     O2CPY          LL_HRTIM_OUT_GetLevel
  * @param  HRTIMx High Resolution Timer instance
  * @param  Output This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUTPUT_TA1
  *         @arg @ref LL_HRTIM_OUTPUT_TA2
  *         @arg @ref LL_HRTIM_OUTPUT_TB1
  *         @arg @ref LL_HRTIM_OUTPUT_TB2
  *         @arg @ref LL_HRTIM_OUTPUT_TC1
  *         @arg @ref LL_HRTIM_OUTPUT_TC2
  *         @arg @ref LL_HRTIM_OUTPUT_TD1
  *         @arg @ref LL_HRTIM_OUTPUT_TD2
  *         @arg @ref LL_HRTIM_OUTPUT_TE1
  *         @arg @ref LL_HRTIM_OUTPUT_TE2
  * @retval OutputLevel This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_OUT_LEVEL_INACTIVE
  *         @arg @ref LL_HRTIM_OUT_LEVEL_ACTIVE
  */
__STATIC_INLINE uint32_t LL_HRTIM_OUT_GetLevel(HRTIM_TypeDef *HRTIMx, uint32_t Output)
{
  register uint32_t iOutput = (uint8_t)(POSITION_VAL(Output) - POSITION_VAL(LL_HRTIM_OUTPUT_TA1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sTimerxRegs[0].TIMxISR) +
                                                    REG_OFFSET_TAB_OUTxR[iOutput]));
  return ((READ_BIT(*pReg, (HRTIM_TIMISR_O1CPY << REG_SHIFT_TAB_OxSTAT[iOutput])) >> REG_SHIFT_TAB_OxSTAT[iOutput]) >>
          HRTIM_TIMISR_O1CPY_Pos);
}

/**
  * @}
  */

/** @defgroup HRTIM_EF_External_Event_management External_Event_management
  * @{
  */

/**
  * @brief  Configure external event conditioning.
  * @rmtoll EECR1     EE1SRC          LL_HRTIM_EE_Config\n
  *         EECR1     EE1POL          LL_HRTIM_EE_Config\n
  *         EECR1     EE1SNS          LL_HRTIM_EE_Config\n
  *         EECR1     EE1FAST         LL_HRTIM_EE_Config\n
  *         EECR1     EE2SRC          LL_HRTIM_EE_Config\n
  *         EECR1     EE2POL          LL_HRTIM_EE_Config\n
  *         EECR1     EE2SNS          LL_HRTIM_EE_Config\n
  *         EECR1     EE2FAST         LL_HRTIM_EE_Config\n
  *         EECR1     EE3SRC          LL_HRTIM_EE_Config\n
  *         EECR1     EE3POL          LL_HRTIM_EE_Config\n
  *         EECR1     EE3SNS          LL_HRTIM_EE_Config\n
  *         EECR1     EE3FAST         LL_HRTIM_EE_Config\n
  *         EECR1     EE4SRC          LL_HRTIM_EE_Config\n
  *         EECR1     EE4POL          LL_HRTIM_EE_Config\n
  *         EECR1     EE4SNS          LL_HRTIM_EE_Config\n
  *         EECR1     EE4FAST         LL_HRTIM_EE_Config\n
  *         EECR1     EE5SRC          LL_HRTIM_EE_Config\n
  *         EECR1     EE5POL          LL_HRTIM_EE_Config\n
  *         EECR1     EE5SNS          LL_HRTIM_EE_Config\n
  *         EECR1     EE5FAST         LL_HRTIM_EE_Config\n
  *         EECR2     EE6SRC          LL_HRTIM_EE_Config\n
  *         EECR2     EE6POL          LL_HRTIM_EE_Config\n
  *         EECR2     EE6SNS          LL_HRTIM_EE_Config\n
  *         EECR2     EE6FAST         LL_HRTIM_EE_Config\n
  *         EECR2     EE7SRC          LL_HRTIM_EE_Config\n
  *         EECR2     EE7POL          LL_HRTIM_EE_Config\n
  *         EECR2     EE7SNS          LL_HRTIM_EE_Config\n
  *         EECR2     EE7FAST         LL_HRTIM_EE_Config\n
  *         EECR2     EE8SRC          LL_HRTIM_EE_Config\n
  *         EECR2     EE8POL          LL_HRTIM_EE_Config\n
  *         EECR2     EE8SNS          LL_HRTIM_EE_Config\n
  *         EECR2     EE8FAST         LL_HRTIM_EE_Config\n
  *         EECR2     EE9SRC          LL_HRTIM_EE_Config\n
  *         EECR2     EE9POL          LL_HRTIM_EE_Config\n
  *         EECR2     EE9SNS          LL_HRTIM_EE_Config\n
  *         EECR2     EE9FAST         LL_HRTIM_EE_Config\n
  *         EECR2     EE10SRC         LL_HRTIM_EE_Config\n
  *         EECR2     EE10POL         LL_HRTIM_EE_Config\n
  *         EECR2     EE10SNS         LL_HRTIM_EE_Config\n
  *         EECR2     EE10FAST        LL_HRTIM_EE_Config
  * @note This function must not be called when the timer counter is enabled.
  * @note Event source (EExSrc1..EExSRC4) mapping depends on configured event channel.
  * @note Fast mode is available only for LL_HRTIM_EVENT_1..5.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Event This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EVENT_1
  *         @arg @ref LL_HRTIM_EVENT_2
  *         @arg @ref LL_HRTIM_EVENT_3
  *         @arg @ref LL_HRTIM_EVENT_4
  *         @arg @ref LL_HRTIM_EVENT_5
  *         @arg @ref LL_HRTIM_EVENT_6
  *         @arg @ref LL_HRTIM_EVENT_7
  *         @arg @ref LL_HRTIM_EVENT_8
  *         @arg @ref LL_HRTIM_EVENT_9
  *         @arg @ref LL_HRTIM_EVENT_10
  * @param  Configuration This parameter must be a combination of all the following values:
  *         @arg @ref LL_HRTIM_EE_SRC_1 or @ref LL_HRTIM_EE_SRC_2 or @ref LL_HRTIM_EE_SRC_3 or @ref LL_HRTIM_EE_SRC_4
  *         @arg @ref LL_HRTIM_EE_POLARITY_HIGH or @ref LL_HRTIM_EE_POLARITY_LOW
  *         @arg @ref LL_HRTIM_EE_SENSITIVITY_LEVEL or @ref LL_HRTIM_EE_SENSITIVITY_RISINGEDGE or @ref LL_HRTIM_EE_SENSITIVITY_FALLINGEDGE or @ref LL_HRTIM_EE_SENSITIVITY_BOTHEDGES
  *         @arg @ref LL_HRTIM_EE_FASTMODE_DISABLE or @ref LL_HRTIM_EE_FASTMODE_ENABLE
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EE_Config(HRTIM_TypeDef *HRTIMx, uint32_t Event, uint32_t Configuration)
{
  register uint32_t iEvent = (uint8_t)(POSITION_VAL(Event) - POSITION_VAL(LL_HRTIM_EVENT_1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.EECR1) +
                                                    REG_OFFSET_TAB_EECR[iEvent]));
  MODIFY_REG(*pReg, (HRTIM_EE_CONFIG_MASK << REG_SHIFT_TAB_EExSRC[iEvent]),
             (Configuration << REG_SHIFT_TAB_EExSRC[iEvent]));
}

/**
  * @brief  Set the external event source.
  * @rmtoll EECR1     EE1SRC          LL_HRTIM_EE_SetSrc\n
  *         EECR1     EE2SRC          LL_HRTIM_EE_SetSrc\n
  *         EECR1     EE3SRC          LL_HRTIM_EE_SetSrc\n
  *         EECR1     EE4SRC          LL_HRTIM_EE_SetSrc\n
  *         EECR1     EE5SRC          LL_HRTIM_EE_SetSrc\n
  *         EECR2     EE6SRC          LL_HRTIM_EE_SetSrc\n
  *         EECR2     EE7SRC          LL_HRTIM_EE_SetSrc\n
  *         EECR2     EE8SRC          LL_HRTIM_EE_SetSrc\n
  *         EECR2     EE9SRC          LL_HRTIM_EE_SetSrc\n
  *         EECR2     EE10SRC         LL_HRTIM_EE_SetSrc
  * @param  HRTIMx High Resolution Timer instance
  * @param  Event This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EVENT_1
  *         @arg @ref LL_HRTIM_EVENT_2
  *         @arg @ref LL_HRTIM_EVENT_3
  *         @arg @ref LL_HRTIM_EVENT_4
  *         @arg @ref LL_HRTIM_EVENT_5
  *         @arg @ref LL_HRTIM_EVENT_6
  *         @arg @ref LL_HRTIM_EVENT_7
  *         @arg @ref LL_HRTIM_EVENT_8
  *         @arg @ref LL_HRTIM_EVENT_9
  *         @arg @ref LL_HRTIM_EVENT_10
  * @param  Src This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EE_SRC_1
  *         @arg @ref LL_HRTIM_EE_SRC_2
  *         @arg @ref LL_HRTIM_EE_SRC_3
  *         @arg @ref LL_HRTIM_EE_SRC_4
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EE_SetSrc(HRTIM_TypeDef *HRTIMx, uint32_t Event, uint32_t Src)
{
  register uint32_t iEvent = (uint8_t)(POSITION_VAL(Event) - POSITION_VAL(LL_HRTIM_EVENT_1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.EECR1) +
                                                    REG_OFFSET_TAB_EECR[iEvent]));
  MODIFY_REG(*pReg, (HRTIM_EECR1_EE1SRC << REG_SHIFT_TAB_EExSRC[iEvent]), (Src << REG_SHIFT_TAB_EExSRC[iEvent]));
}

/**
  * @brief  Get actual external event source.
  * @rmtoll EECR1     EE1SRC          LL_HRTIM_EE_GetSrc\n
  *         EECR1     EE2SRC          LL_HRTIM_EE_GetSrc\n
  *         EECR1     EE3SRC          LL_HRTIM_EE_GetSrc\n
  *         EECR1     EE4SRC          LL_HRTIM_EE_GetSrc\n
  *         EECR1     EE5SRC          LL_HRTIM_EE_GetSrc\n
  *         EECR2     EE6SRC          LL_HRTIM_EE_GetSrc\n
  *         EECR2     EE7SRC          LL_HRTIM_EE_GetSrc\n
  *         EECR2     EE8SRC          LL_HRTIM_EE_GetSrc\n
  *         EECR2     EE9SRC          LL_HRTIM_EE_GetSrc\n
  *         EECR2     EE10SRC         LL_HRTIM_EE_GetSrc
  * @param  HRTIMx High Resolution Timer instance
  * @param  Event This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EVENT_1
  *         @arg @ref LL_HRTIM_EVENT_2
  *         @arg @ref LL_HRTIM_EVENT_3
  *         @arg @ref LL_HRTIM_EVENT_4
  *         @arg @ref LL_HRTIM_EVENT_5
  *         @arg @ref LL_HRTIM_EVENT_6
  *         @arg @ref LL_HRTIM_EVENT_7
  *         @arg @ref LL_HRTIM_EVENT_8
  *         @arg @ref LL_HRTIM_EVENT_9
  *         @arg @ref LL_HRTIM_EVENT_10
  * @retval EventSrc This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EE_SRC_1
  *         @arg @ref LL_HRTIM_EE_SRC_2
  *         @arg @ref LL_HRTIM_EE_SRC_3
  *         @arg @ref LL_HRTIM_EE_SRC_4
  */
__STATIC_INLINE uint32_t LL_HRTIM_EE_GetSrc(HRTIM_TypeDef *HRTIMx, uint32_t Event)
{
  register uint32_t iEvent = (uint8_t)(POSITION_VAL(Event) - POSITION_VAL(LL_HRTIM_EVENT_1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.EECR1) +
                                                    REG_OFFSET_TAB_EECR[iEvent]));
  return (READ_BIT(*pReg, HRTIM_EECR1_EE1SRC << REG_SHIFT_TAB_EExSRC[iEvent]) >>  REG_SHIFT_TAB_EExSRC[iEvent]);
}

/**
  * @brief  Set the polarity of an external event.
  * @rmtoll EECR1     EE1POL          LL_HRTIM_EE_SetPolarity\n
  *         EECR1     EE2POL          LL_HRTIM_EE_SetPolarity\n
  *         EECR1     EE3POL          LL_HRTIM_EE_SetPolarity\n
  *         EECR1     EE4POL          LL_HRTIM_EE_SetPolarity\n
  *         EECR1     EE5POL          LL_HRTIM_EE_SetPolarity\n
  *         EECR2     EE6POL          LL_HRTIM_EE_SetPolarity\n
  *         EECR2     EE7POL          LL_HRTIM_EE_SetPolarity\n
  *         EECR2     EE8POL          LL_HRTIM_EE_SetPolarity\n
  *         EECR2     EE9POL          LL_HRTIM_EE_SetPolarity\n
  *         EECR2     EE10POL         LL_HRTIM_EE_SetPolarity
  * @note This function must not be called when the timer counter is enabled.
  * @note Event polarity is only significant when event detection is level-sensitive.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Event This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EVENT_1
  *         @arg @ref LL_HRTIM_EVENT_2
  *         @arg @ref LL_HRTIM_EVENT_3
  *         @arg @ref LL_HRTIM_EVENT_4
  *         @arg @ref LL_HRTIM_EVENT_5
  *         @arg @ref LL_HRTIM_EVENT_6
  *         @arg @ref LL_HRTIM_EVENT_7
  *         @arg @ref LL_HRTIM_EVENT_8
  *         @arg @ref LL_HRTIM_EVENT_9
  *         @arg @ref LL_HRTIM_EVENT_10
  * @param  Polarity This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EE_POLARITY_HIGH
  *         @arg @ref LL_HRTIM_EE_POLARITY_LOW
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EE_SetPolarity(HRTIM_TypeDef *HRTIMx, uint32_t Event, uint32_t Polarity)
{
  register uint32_t iEvent = (uint8_t)(POSITION_VAL(Event) - POSITION_VAL(LL_HRTIM_EVENT_1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.EECR1) +
                                                    REG_OFFSET_TAB_EECR[iEvent]));
  MODIFY_REG(*pReg, (HRTIM_EECR1_EE1POL << REG_SHIFT_TAB_EExSRC[iEvent]), (Polarity << REG_SHIFT_TAB_EExSRC[iEvent]));
}

/**
  * @brief  Get actual polarity setting of an external event.
  * @rmtoll EECR1     EE1POL          LL_HRTIM_EE_GetPolarity\n
  *         EECR1     EE2POL          LL_HRTIM_EE_GetPolarity\n
  *         EECR1     EE3POL          LL_HRTIM_EE_GetPolarity\n
  *         EECR1     EE4POL          LL_HRTIM_EE_GetPolarity\n
  *         EECR1     EE5POL          LL_HRTIM_EE_GetPolarity\n
  *         EECR2     EE6POL          LL_HRTIM_EE_GetPolarity\n
  *         EECR2     EE7POL          LL_HRTIM_EE_GetPolarity\n
  *         EECR2     EE8POL          LL_HRTIM_EE_GetPolarity\n
  *         EECR2     EE9POL          LL_HRTIM_EE_GetPolarity\n
  *         EECR2     EE10POL         LL_HRTIM_EE_GetPolarity
  * @param  HRTIMx High Resolution Timer instance
  * @param  Event This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EVENT_1
  *         @arg @ref LL_HRTIM_EVENT_2
  *         @arg @ref LL_HRTIM_EVENT_3
  *         @arg @ref LL_HRTIM_EVENT_4
  *         @arg @ref LL_HRTIM_EVENT_5
  *         @arg @ref LL_HRTIM_EVENT_6
  *         @arg @ref LL_HRTIM_EVENT_7
  *         @arg @ref LL_HRTIM_EVENT_8
  *         @arg @ref LL_HRTIM_EVENT_9
  *         @arg @ref LL_HRTIM_EVENT_10
  * @retval Polarity This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EE_POLARITY_HIGH
  *         @arg @ref LL_HRTIM_EE_POLARITY_LOW
  */
__STATIC_INLINE uint32_t LL_HRTIM_EE_GetPolarity(HRTIM_TypeDef *HRTIMx, uint32_t Event)
{
  register uint32_t iEvent = (uint8_t)(POSITION_VAL(Event) - POSITION_VAL(LL_HRTIM_EVENT_1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.EECR1) +
                                                    REG_OFFSET_TAB_EECR[iEvent]));
  return (READ_BIT(*pReg, HRTIM_EECR1_EE1POL << REG_SHIFT_TAB_EExSRC[iEvent]) >>  REG_SHIFT_TAB_EExSRC[iEvent]);
}

/**
  * @brief  Set the sensitivity of an external event.
  * @rmtoll EECR1     EE1SNS          LL_HRTIM_EE_SetSensitivity\n
  *         EECR1     EE2SNS          LL_HRTIM_EE_SetSensitivity\n
  *         EECR1     EE3SNS          LL_HRTIM_EE_SetSensitivity\n
  *         EECR1     EE4SNS          LL_HRTIM_EE_SetSensitivity\n
  *         EECR1     EE5SNS          LL_HRTIM_EE_SetSensitivity\n
  *         EECR2     EE6SNS          LL_HRTIM_EE_SetSensitivity\n
  *         EECR2     EE7SNS          LL_HRTIM_EE_SetSensitivity\n
  *         EECR2     EE8SNS          LL_HRTIM_EE_SetSensitivity\n
  *         EECR2     EE9SNS          LL_HRTIM_EE_SetSensitivity\n
  *         EECR2     EE10SNS         LL_HRTIM_EE_SetSensitivity
  * @param  HRTIMx High Resolution Timer instance
  * @param  Event This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EVENT_1
  *         @arg @ref LL_HRTIM_EVENT_2
  *         @arg @ref LL_HRTIM_EVENT_3
  *         @arg @ref LL_HRTIM_EVENT_4
  *         @arg @ref LL_HRTIM_EVENT_5
  *         @arg @ref LL_HRTIM_EVENT_6
  *         @arg @ref LL_HRTIM_EVENT_7
  *         @arg @ref LL_HRTIM_EVENT_8
  *         @arg @ref LL_HRTIM_EVENT_9
  *         @arg @ref LL_HRTIM_EVENT_10
  * @param  Sensitivity This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EE_SENSITIVITY_LEVEL
  *         @arg @ref LL_HRTIM_EE_SENSITIVITY_RISINGEDGE
  *         @arg @ref LL_HRTIM_EE_SENSITIVITY_FALLINGEDGE
  *         @arg @ref LL_HRTIM_EE_SENSITIVITY_BOTHEDGES
  * @retval None
  */

__STATIC_INLINE void LL_HRTIM_EE_SetSensitivity(HRTIM_TypeDef *HRTIMx, uint32_t Event, uint32_t Sensitivity)
{
  register uint32_t iEvent = (uint8_t)(POSITION_VAL(Event) - POSITION_VAL(LL_HRTIM_EVENT_1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.EECR1) +
                                                    REG_OFFSET_TAB_EECR[iEvent]));
  MODIFY_REG(*pReg, (HRTIM_EECR1_EE1SNS << REG_SHIFT_TAB_EExSRC[iEvent]), (Sensitivity << REG_SHIFT_TAB_EExSRC[iEvent]));
}

/**
  * @brief  Get actual sensitivity setting of an external event.
  * @rmtoll EECR1     EE1SNS          LL_HRTIM_EE_GetSensitivity\n
  *         EECR1     EE2SNS          LL_HRTIM_EE_GetSensitivity\n
  *         EECR1     EE3SNS          LL_HRTIM_EE_GetSensitivity\n
  *         EECR1     EE4SNS          LL_HRTIM_EE_GetSensitivity\n
  *         EECR1     EE5SNS          LL_HRTIM_EE_GetSensitivity\n
  *         EECR2     EE6SNS          LL_HRTIM_EE_GetSensitivity\n
  *         EECR2     EE7SNS          LL_HRTIM_EE_GetSensitivity\n
  *         EECR2     EE8SNS          LL_HRTIM_EE_GetSensitivity\n
  *         EECR2     EE9SNS          LL_HRTIM_EE_GetSensitivity\n
  *         EECR2     EE10SNS         LL_HRTIM_EE_GetSensitivity
  * @param  HRTIMx High Resolution Timer instance
  * @param  Event This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EVENT_1
  *         @arg @ref LL_HRTIM_EVENT_2
  *         @arg @ref LL_HRTIM_EVENT_3
  *         @arg @ref LL_HRTIM_EVENT_4
  *         @arg @ref LL_HRTIM_EVENT_5
  *         @arg @ref LL_HRTIM_EVENT_6
  *         @arg @ref LL_HRTIM_EVENT_7
  *         @arg @ref LL_HRTIM_EVENT_8
  *         @arg @ref LL_HRTIM_EVENT_9
  *         @arg @ref LL_HRTIM_EVENT_10
  * @retval Polarity This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EE_SENSITIVITY_LEVEL
  *         @arg @ref LL_HRTIM_EE_SENSITIVITY_RISINGEDGE
  *         @arg @ref LL_HRTIM_EE_SENSITIVITY_FALLINGEDGE
  *         @arg @ref LL_HRTIM_EE_SENSITIVITY_BOTHEDGES
  */
__STATIC_INLINE uint32_t LL_HRTIM_EE_GetSensitivity(HRTIM_TypeDef *HRTIMx, uint32_t Event)
{
  register uint32_t iEvent = (uint8_t)(POSITION_VAL(Event) - POSITION_VAL(LL_HRTIM_EVENT_1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.EECR1) +
                                                    REG_OFFSET_TAB_EECR[iEvent]));
  return (READ_BIT(*pReg, HRTIM_EECR1_EE1SNS << REG_SHIFT_TAB_EExSRC[iEvent]) >>  REG_SHIFT_TAB_EExSRC[iEvent]);
}

/**
  * @brief  Set the fast mode of an external event.
  * @rmtoll EECR1     EE1FAST         LL_HRTIM_EE_SetFastMode\n
  *         EECR1     EE2FAST         LL_HRTIM_EE_SetFastMode\n
  *         EECR1     EE3FAST         LL_HRTIM_EE_SetFastMode\n
  *         EECR1     EE4FAST         LL_HRTIM_EE_SetFastMode\n
  *         EECR1     EE5FAST         LL_HRTIM_EE_SetFastMode\n
  *         EECR2     EE6FAST         LL_HRTIM_EE_SetFastMode\n
  *         EECR2     EE7FAST         LL_HRTIM_EE_SetFastMode\n
  *         EECR2     EE8FAST         LL_HRTIM_EE_SetFastMode\n
  *         EECR2     EE9FAST         LL_HRTIM_EE_SetFastMode\n
  *         EECR2     EE10FAST        LL_HRTIM_EE_SetFastMode
  * @note This function must not be called when the timer counter is enabled.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Event This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EVENT_1
  *         @arg @ref LL_HRTIM_EVENT_2
  *         @arg @ref LL_HRTIM_EVENT_3
  *         @arg @ref LL_HRTIM_EVENT_4
  *         @arg @ref LL_HRTIM_EVENT_5
  * @param  FastMode This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EE_FASTMODE_DISABLE
  *         @arg @ref LL_HRTIM_EE_FASTMODE_ENABLE
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EE_SetFastMode(HRTIM_TypeDef *HRTIMx, uint32_t Event, uint32_t FastMode)
{
  register uint32_t iEvent = (uint8_t)(POSITION_VAL(Event) - POSITION_VAL(LL_HRTIM_EVENT_1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.EECR1) +
                                                    REG_OFFSET_TAB_EECR[iEvent]));
  MODIFY_REG(*pReg, (HRTIM_EECR1_EE1FAST << REG_SHIFT_TAB_EExSRC[iEvent]), (FastMode << REG_SHIFT_TAB_EExSRC[iEvent]));
}

/**
  * @brief  Get actual fast mode setting of an external event.
  * @rmtoll EECR1     EE1FAST         LL_HRTIM_EE_GetFastMode\n
  *         EECR1     EE2FAST         LL_HRTIM_EE_GetFastMode\n
  *         EECR1     EE3FAST         LL_HRTIM_EE_GetFastMode\n
  *         EECR1     EE4FAST         LL_HRTIM_EE_GetFastMode\n
  *         EECR1     EE5FAST         LL_HRTIM_EE_GetFastMode\n
  *         EECR2     EE6FAST         LL_HRTIM_EE_GetFastMode\n
  *         EECR2     EE7FAST         LL_HRTIM_EE_GetFastMode\n
  *         EECR2     EE8FAST         LL_HRTIM_EE_GetFastMode\n
  *         EECR2     EE9FAST         LL_HRTIM_EE_GetFastMode\n
  *         EECR2     EE10FAST        LL_HRTIM_EE_GetFastMode
  * @param  HRTIMx High Resolution Timer instance
  * @param  Event This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EVENT_1
  *         @arg @ref LL_HRTIM_EVENT_2
  *         @arg @ref LL_HRTIM_EVENT_3
  *         @arg @ref LL_HRTIM_EVENT_4
  *         @arg @ref LL_HRTIM_EVENT_5
  * @retval FastMode This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EE_FASTMODE_DISABLE
  *         @arg @ref LL_HRTIM_EE_FASTMODE_ENABLE
  */
__STATIC_INLINE uint32_t LL_HRTIM_EE_GetFastMode(HRTIM_TypeDef *HRTIMx, uint32_t Event)
{
  register uint32_t iEvent = (uint8_t)(POSITION_VAL(Event) - POSITION_VAL(LL_HRTIM_EVENT_1));
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.EECR1) +
                                                    REG_OFFSET_TAB_EECR[iEvent]));
  return (READ_BIT(*pReg, HRTIM_EECR1_EE1FAST << REG_SHIFT_TAB_EExSRC[iEvent]) >>  REG_SHIFT_TAB_EExSRC[iEvent]);
}

/**
  * @brief  Set the digital noise filter of a external event.
  * @rmtoll EECR3     EE6F         LL_HRTIM_EE_SetFilter\n
  *         EECR3     EE7F         LL_HRTIM_EE_SetFilter\n
  *         EECR3     EE8F         LL_HRTIM_EE_SetFilter\n
  *         EECR3     EE9F         LL_HRTIM_EE_SetFilter\n
  *         EECR3     EE10F        LL_HRTIM_EE_SetFilter
  * @param  HRTIMx High Resolution Timer instance
  * @param  Event This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EVENT_6
  *         @arg @ref LL_HRTIM_EVENT_7
  *         @arg @ref LL_HRTIM_EVENT_8
  *         @arg @ref LL_HRTIM_EVENT_9
  *         @arg @ref LL_HRTIM_EVENT_10
  * @param  Filter This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EE_FILTER_NONE
  *         @arg @ref LL_HRTIM_EE_FILTER_1
  *         @arg @ref LL_HRTIM_EE_FILTER_2
  *         @arg @ref LL_HRTIM_EE_FILTER_3
  *         @arg @ref LL_HRTIM_EE_FILTER_4
  *         @arg @ref LL_HRTIM_EE_FILTER_5
  *         @arg @ref LL_HRTIM_EE_FILTER_6
  *         @arg @ref LL_HRTIM_EE_FILTER_7
  *         @arg @ref LL_HRTIM_EE_FILTER_8
  *         @arg @ref LL_HRTIM_EE_FILTER_9
  *         @arg @ref LL_HRTIM_EE_FILTER_10
  *         @arg @ref LL_HRTIM_EE_FILTER_11
  *         @arg @ref LL_HRTIM_EE_FILTER_12
  *         @arg @ref LL_HRTIM_EE_FILTER_13
  *         @arg @ref LL_HRTIM_EE_FILTER_14
  *         @arg @ref LL_HRTIM_EE_FILTER_15
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EE_SetFilter(HRTIM_TypeDef *HRTIMx, uint32_t Event, uint32_t Filter)
{
  register uint32_t iEvent = (uint8_t)(POSITION_VAL(Event) - POSITION_VAL(LL_HRTIM_EVENT_1));
  MODIFY_REG(HRTIMx->sCommonRegs.EECR3, (HRTIM_EECR3_EE6F << REG_SHIFT_TAB_EExSRC[iEvent]),
             (Filter << REG_SHIFT_TAB_EExSRC[iEvent]));
}

/**
  * @brief  Get actual digital noise filter setting of a external event.
  * @rmtoll EECR3     EE6F         LL_HRTIM_EE_GetFilter\n
  *         EECR3     EE7F         LL_HRTIM_EE_GetFilter\n
  *         EECR3     EE8F         LL_HRTIM_EE_GetFilter\n
  *         EECR3     EE9F         LL_HRTIM_EE_GetFilter\n
  *         EECR3     EE10F        LL_HRTIM_EE_GetFilter
  * @param  HRTIMx High Resolution Timer instance
  * @param  Event This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EVENT_6
  *         @arg @ref LL_HRTIM_EVENT_7
  *         @arg @ref LL_HRTIM_EVENT_8
  *         @arg @ref LL_HRTIM_EVENT_9
  *         @arg @ref LL_HRTIM_EVENT_10
  * @retval Filter This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EE_FILTER_NONE
  *         @arg @ref LL_HRTIM_EE_FILTER_1
  *         @arg @ref LL_HRTIM_EE_FILTER_2
  *         @arg @ref LL_HRTIM_EE_FILTER_3
  *         @arg @ref LL_HRTIM_EE_FILTER_4
  *         @arg @ref LL_HRTIM_EE_FILTER_5
  *         @arg @ref LL_HRTIM_EE_FILTER_6
  *         @arg @ref LL_HRTIM_EE_FILTER_7
  *         @arg @ref LL_HRTIM_EE_FILTER_8
  *         @arg @ref LL_HRTIM_EE_FILTER_9
  *         @arg @ref LL_HRTIM_EE_FILTER_10
  *         @arg @ref LL_HRTIM_EE_FILTER_11
  *         @arg @ref LL_HRTIM_EE_FILTER_12
  *         @arg @ref LL_HRTIM_EE_FILTER_13
  *         @arg @ref LL_HRTIM_EE_FILTER_14
  *         @arg @ref LL_HRTIM_EE_FILTER_15
  */
__STATIC_INLINE uint32_t LL_HRTIM_EE_GetFilter(HRTIM_TypeDef *HRTIMx, uint32_t Event)
{
  register uint32_t iEvent = (uint8_t)(POSITION_VAL(Event) - POSITION_VAL(LL_HRTIM_EVENT_6));
  return (READ_BIT(HRTIMx->sCommonRegs.EECR3,
                   (HRTIM_EECR3_EE6F << REG_SHIFT_TAB_EExSRC[iEvent])) >>  REG_SHIFT_TAB_EExSRC[iEvent]);
}

/**
  * @brief  Set the external event prescaler.
  * @rmtoll EECR3     EEVSD        LL_HRTIM_EE_SetPrescaler
  * @param  HRTIMx High Resolution Timer instance
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EE_PRESCALER_DIV1
  *         @arg @ref LL_HRTIM_EE_PRESCALER_DIV2
  *         @arg @ref LL_HRTIM_EE_PRESCALER_DIV4
  *         @arg @ref LL_HRTIM_EE_PRESCALER_DIV8
  * @retval None
  */

__STATIC_INLINE void LL_HRTIM_EE_SetPrescaler(HRTIM_TypeDef *HRTIMx, uint32_t Prescaler)
{
  MODIFY_REG(HRTIMx->sCommonRegs.EECR3, HRTIM_EECR3_EEVSD, Prescaler);
}

/**
  * @brief  Get actual external event prescaler setting.
  * @rmtoll EECR3     EEVSD        LL_HRTIM_EE_GetPrescaler
  * @param  HRTIMx High Resolution Timer instance
  * @retval Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_EE_PRESCALER_DIV1
  *         @arg @ref LL_HRTIM_EE_PRESCALER_DIV2
  *         @arg @ref LL_HRTIM_EE_PRESCALER_DIV4
  *         @arg @ref LL_HRTIM_EE_PRESCALER_DIV8
  */

__STATIC_INLINE uint32_t LL_HRTIM_EE_GetPrescaler(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.EECR3, HRTIM_EECR3_EEVSD));
}

/**
  * @}
  */

/** @defgroup HRTIM_EF_Fault_management Fault_management
  * @{
  */

/**
  * @brief  Configure fault signal conditioning.
  * @rmtoll FLTINR1     FLT1P        LL_HRTIM_FLT_Config\n
  *         FLTINR1     FLT1SRC      LL_HRTIM_FLT_Config\n
  *         FLTINR1     FLT2P        LL_HRTIM_FLT_Config\n
  *         FLTINR1     FLT2SRC      LL_HRTIM_FLT_Config\n
  *         FLTINR1     FLT3P        LL_HRTIM_FLT_Config\n
  *         FLTINR1     FLT3SRC      LL_HRTIM_FLT_Config\n
  *         FLTINR1     FLT4P        LL_HRTIM_FLT_Config\n
  *         FLTINR1     FLT4SRC      LL_HRTIM_FLT_Config\n
  *         FLTINR2     FLT5P        LL_HRTIM_FLT_Config\n
  *         FLTINR2     FLT5SRC      LL_HRTIM_FLT_Config
  * @note This function must not be called when the fault channel is enabled.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Fault This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FAULT_1
  *         @arg @ref LL_HRTIM_FAULT_2
  *         @arg @ref LL_HRTIM_FAULT_3
  *         @arg @ref LL_HRTIM_FAULT_4
  *         @arg @ref LL_HRTIM_FAULT_5
  * @param  Configuration This parameter must be a combination of all the following values:
  *         @arg @ref LL_HRTIM_FLT_SRC_DIGITALINPUT or @ref LL_HRTIM_FLT_SRC_INTERNAL
  *         @arg @ref LL_HRTIM_FLT_POLARITY_LOW or @ref LL_HRTIM_FLT_POLARITY_HIGH
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_FLT_Config(HRTIM_TypeDef *HRTIMx, uint32_t Fault, uint32_t Configuration)
{
  register uint32_t iFault = (uint8_t)POSITION_VAL(Fault);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.FLTINR1) +
                                                    REG_OFFSET_TAB_FLTINR[iFault]));
  MODIFY_REG(*pReg, (HRTIM_FLT_CONFIG_MASK << REG_SHIFT_TAB_FLTxE[iFault]),
             (Configuration << REG_SHIFT_TAB_FLTxE[iFault]));
}

/**
  * @brief  Set the source of a fault signal.
  * @rmtoll FLTINR1     FLT1SRC      LL_HRTIM_FLT_SetSrc\n
  *         FLTINR1     FLT2SRC      LL_HRTIM_FLT_SetSrc\n
  *         FLTINR1     FLT3SRC      LL_HRTIM_FLT_SetSrc\n
  *         FLTINR1     FLT4SRC      LL_HRTIM_FLT_SetSrc\n
  *         FLTINR2     FLT5SRC      LL_HRTIM_FLT_SetSrc
  * @note This function must not be called when the fault channel is enabled.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Fault This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FAULT_1
  *         @arg @ref LL_HRTIM_FAULT_2
  *         @arg @ref LL_HRTIM_FAULT_3
  *         @arg @ref LL_HRTIM_FAULT_4
  *         @arg @ref LL_HRTIM_FAULT_5
  * @param  Src This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FLT_SRC_DIGITALINPUT
  *         @arg @ref LL_HRTIM_FLT_SRC_INTERNAL
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_FLT_SetSrc(HRTIM_TypeDef *HRTIMx, uint32_t Fault, uint32_t Src)
{
  register uint32_t iFault = (uint8_t)POSITION_VAL(Fault);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.FLTINR1) +
                                                    REG_OFFSET_TAB_FLTINR[iFault]));
  MODIFY_REG(*pReg, (HRTIM_FLTINR1_FLT1SRC << REG_SHIFT_TAB_FLTxE[iFault]), (Src << REG_SHIFT_TAB_FLTxE[iFault]));
}

/**
  * @brief  Get actual source of a fault signal.
  * @rmtoll FLTINR1     FLT1SRC      LL_HRTIM_FLT_GetSrc\n
  *         FLTINR1     FLT2SRC      LL_HRTIM_FLT_GetSrc\n
  *         FLTINR1     FLT3SRC      LL_HRTIM_FLT_GetSrc\n
  *         FLTINR1     FLT4SRC      LL_HRTIM_FLT_GetSrc\n
  *         FLTINR2     FLT5SRC      LL_HRTIM_FLT_GetSrc
  * @param  HRTIMx High Resolution Timer instance
  * @param  Fault This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FAULT_1
  *         @arg @ref LL_HRTIM_FAULT_2
  *         @arg @ref LL_HRTIM_FAULT_3
  *         @arg @ref LL_HRTIM_FAULT_4
  *         @arg @ref LL_HRTIM_FAULT_5
  * @retval Src This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FLT_SRC_DIGITALINPUT
  *         @arg @ref LL_HRTIM_FLT_SRC_INTERNAL
  */
__STATIC_INLINE uint32_t LL_HRTIM_FLT_GetSrc(HRTIM_TypeDef *HRTIMx, uint32_t Fault)
{
  register uint32_t iFault = (uint8_t)POSITION_VAL(Fault);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.FLTINR1) +
                                                    REG_OFFSET_TAB_FLTINR[iFault]));
  return (READ_BIT(*pReg, (HRTIM_FLTINR1_FLT1SRC << REG_SHIFT_TAB_FLTxE[iFault])) >>  REG_SHIFT_TAB_FLTxE[iFault]);
}

/**
  * @brief  Set the polarity of a fault signal.
  * @rmtoll FLTINR1     FLT1P        LL_HRTIM_FLT_SetPolarity\n
  *         FLTINR1     FLT2P        LL_HRTIM_FLT_SetPolarity\n
  *         FLTINR1     FLT3P        LL_HRTIM_FLT_SetPolarity\n
  *         FLTINR1     FLT4P        LL_HRTIM_FLT_SetPolarity\n
  *         FLTINR2     FLT5P        LL_HRTIM_FLT_SetPolarity
  * @note This function must not be called when the fault channel is enabled.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Fault This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FAULT_1
  *         @arg @ref LL_HRTIM_FAULT_2
  *         @arg @ref LL_HRTIM_FAULT_3
  *         @arg @ref LL_HRTIM_FAULT_4
  *         @arg @ref LL_HRTIM_FAULT_5
  * @param  Polarity This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FLT_POLARITY_LOW
  *         @arg @ref LL_HRTIM_FLT_POLARITY_HIGH
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_FLT_SetPolarity(HRTIM_TypeDef *HRTIMx, uint32_t Fault, uint32_t Polarity)
{
  register uint32_t iFault = (uint8_t)POSITION_VAL(Fault);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.FLTINR1) +
                                                    REG_OFFSET_TAB_FLTINR[iFault]));
  MODIFY_REG(*pReg, (HRTIM_FLTINR1_FLT1P << REG_SHIFT_TAB_FLTxE[iFault]), (Polarity << REG_SHIFT_TAB_FLTxE[iFault]));
}

/**
  * @brief  Get actual polarity of a fault signal.
  * @rmtoll FLTINR1     FLT1P        LL_HRTIM_FLT_GetPolarity\n
  *         FLTINR1     FLT2P        LL_HRTIM_FLT_GetPolarity\n
  *         FLTINR1     FLT3P        LL_HRTIM_FLT_GetPolarity\n
  *         FLTINR1     FLT4P        LL_HRTIM_FLT_GetPolarity\n
  *         FLTINR2     FLT5P        LL_HRTIM_FLT_GetPolarity
  * @param  HRTIMx High Resolution Timer instance
  * @param  Fault This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FAULT_1
  *         @arg @ref LL_HRTIM_FAULT_2
  *         @arg @ref LL_HRTIM_FAULT_3
  *         @arg @ref LL_HRTIM_FAULT_4
  *         @arg @ref LL_HRTIM_FAULT_5
  * @retval Polarity This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FLT_POLARITY_LOW
  *         @arg @ref LL_HRTIM_FLT_POLARITY_HIGH
  */
__STATIC_INLINE uint32_t LL_HRTIM_FLT_GetPolarity(HRTIM_TypeDef *HRTIMx, uint32_t Fault)
{
  register uint32_t iFault = (uint8_t)POSITION_VAL(Fault);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.FLTINR1) +
                                                    REG_OFFSET_TAB_FLTINR[iFault]));
  return (READ_BIT(*pReg, (HRTIM_FLTINR1_FLT1P << REG_SHIFT_TAB_FLTxE[iFault])) >>  REG_SHIFT_TAB_FLTxE[iFault]);
}

/**
  * @brief  Set the digital noise filter of a fault signal.
  * @rmtoll FLTINR1     FLT1F      LL_HRTIM_FLT_SetFilter\n
  *         FLTINR1     FLT2F      LL_HRTIM_FLT_SetFilter\n
  *         FLTINR1     FLT3F      LL_HRTIM_FLT_SetFilter\n
  *         FLTINR1     FLT4F      LL_HRTIM_FLT_SetFilter\n
  *         FLTINR2     FLT5F      LL_HRTIM_FLT_SetFilter
  * @note This function must not be called when the fault channel is enabled.
  * @param  HRTIMx High Resolution Timer instance
  * @param  Fault This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FAULT_1
  *         @arg @ref LL_HRTIM_FAULT_2
  *         @arg @ref LL_HRTIM_FAULT_3
  *         @arg @ref LL_HRTIM_FAULT_4
  *         @arg @ref LL_HRTIM_FAULT_5
  * @param  Filter This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FLT_FILTER_NONE
  *         @arg @ref LL_HRTIM_FLT_FILTER_1
  *         @arg @ref LL_HRTIM_FLT_FILTER_2
  *         @arg @ref LL_HRTIM_FLT_FILTER_3
  *         @arg @ref LL_HRTIM_FLT_FILTER_4
  *         @arg @ref LL_HRTIM_FLT_FILTER_5
  *         @arg @ref LL_HRTIM_FLT_FILTER_6
  *         @arg @ref LL_HRTIM_FLT_FILTER_7
  *         @arg @ref LL_HRTIM_FLT_FILTER_8
  *         @arg @ref LL_HRTIM_FLT_FILTER_9
  *         @arg @ref LL_HRTIM_FLT_FILTER_10
  *         @arg @ref LL_HRTIM_FLT_FILTER_11
  *         @arg @ref LL_HRTIM_FLT_FILTER_12
  *         @arg @ref LL_HRTIM_FLT_FILTER_13
  *         @arg @ref LL_HRTIM_FLT_FILTER_14
  *         @arg @ref LL_HRTIM_FLT_FILTER_15
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_FLT_SetFilter(HRTIM_TypeDef *HRTIMx, uint32_t Fault, uint32_t Filter)
{
  register uint32_t iFault = (uint8_t)POSITION_VAL(Fault);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.FLTINR1) +
                                                    REG_OFFSET_TAB_FLTINR[iFault]));
  MODIFY_REG(*pReg, (HRTIM_FLTINR1_FLT1F << REG_SHIFT_TAB_FLTxE[iFault]), (Filter << REG_SHIFT_TAB_FLTxE[iFault]));
}

/**
  * @brief  Get actual digital noise filter setting of a fault signal.
  * @rmtoll FLTINR1     FLT1F      LL_HRTIM_FLT_GetFilter\n
  *         FLTINR1     FLT2F      LL_HRTIM_FLT_GetFilter\n
  *         FLTINR1     FLT3F      LL_HRTIM_FLT_GetFilter\n
  *         FLTINR1     FLT4F      LL_HRTIM_FLT_GetFilter\n
  *         FLTINR2     FLT5F      LL_HRTIM_FLT_GetFilter
  * @param  HRTIMx High Resolution Timer instance
  * @param  Fault This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FAULT_1
  *         @arg @ref LL_HRTIM_FAULT_2
  *         @arg @ref LL_HRTIM_FAULT_3
  *         @arg @ref LL_HRTIM_FAULT_4
  *         @arg @ref LL_HRTIM_FAULT_5
  * @retval Filter This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FLT_FILTER_NONE
  *         @arg @ref LL_HRTIM_FLT_FILTER_1
  *         @arg @ref LL_HRTIM_FLT_FILTER_2
  *         @arg @ref LL_HRTIM_FLT_FILTER_3
  *         @arg @ref LL_HRTIM_FLT_FILTER_4
  *         @arg @ref LL_HRTIM_FLT_FILTER_5
  *         @arg @ref LL_HRTIM_FLT_FILTER_6
  *         @arg @ref LL_HRTIM_FLT_FILTER_7
  *         @arg @ref LL_HRTIM_FLT_FILTER_8
  *         @arg @ref LL_HRTIM_FLT_FILTER_9
  *         @arg @ref LL_HRTIM_FLT_FILTER_10
  *         @arg @ref LL_HRTIM_FLT_FILTER_11
  *         @arg @ref LL_HRTIM_FLT_FILTER_12
  *         @arg @ref LL_HRTIM_FLT_FILTER_13
  *         @arg @ref LL_HRTIM_FLT_FILTER_14
  *         @arg @ref LL_HRTIM_FLT_FILTER_15
  */
__STATIC_INLINE uint32_t LL_HRTIM_FLT_GetFilter(HRTIM_TypeDef *HRTIMx, uint32_t Fault)
{
  register uint32_t iFault = (uint8_t)POSITION_VAL(Fault);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.FLTINR1) +
                                                    REG_OFFSET_TAB_FLTINR[iFault]));
  return (READ_BIT(*pReg, (HRTIM_FLTINR1_FLT1F << REG_SHIFT_TAB_FLTxE[iFault])) >>  REG_SHIFT_TAB_FLTxE[iFault]);
}

/**
  * @brief  Set the fault circuitry prescaler.
  * @rmtoll FLTINR2     FLTSD      LL_HRTIM_FLT_SetPrescaler
  * @param  HRTIMx High Resolution Timer instance
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FLT_PRESCALER_DIV1
  *         @arg @ref LL_HRTIM_FLT_PRESCALER_DIV2
  *         @arg @ref LL_HRTIM_FLT_PRESCALER_DIV4
  *         @arg @ref LL_HRTIM_FLT_PRESCALER_DIV8
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_FLT_SetPrescaler(HRTIM_TypeDef *HRTIMx, uint32_t Prescaler)
{
  MODIFY_REG(HRTIMx->sCommonRegs.FLTINR2, HRTIM_FLTINR2_FLTSD, Prescaler);
}

/**
  * @brief  Get actual fault circuitry prescaler setting.
  * @rmtoll FLTINR2     FLTSD      LL_HRTIM_FLT_GetPrescaler
  * @param  HRTIMx High Resolution Timer instance
  * @retval Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FLT_PRESCALER_DIV1
  *         @arg @ref LL_HRTIM_FLT_PRESCALER_DIV2
  *         @arg @ref LL_HRTIM_FLT_PRESCALER_DIV4
  *         @arg @ref LL_HRTIM_FLT_PRESCALER_DIV8
  */
__STATIC_INLINE uint32_t LL_HRTIM_FLT_GetPrescaler(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.FLTINR2, HRTIM_FLTINR2_FLTSD));

}

/**
  * @brief  Lock the fault signal conditioning settings.
  * @rmtoll FLTINR1     FLT1LCK      LL_HRTIM_FLT_Lock\n
  *         FLTINR1     FLT2LCK      LL_HRTIM_FLT_Lock\n
  *         FLTINR1     FLT3LCK      LL_HRTIM_FLT_Lock\n
  *         FLTINR1     FLT4LCK      LL_HRTIM_FLT_Lock\n
  *         FLTINR2     FLT5LCK      LL_HRTIM_FLT_Lock
  * @param  HRTIMx High Resolution Timer instance
  * @param  Fault This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FAULT_1
  *         @arg @ref LL_HRTIM_FAULT_2
  *         @arg @ref LL_HRTIM_FAULT_3
  *         @arg @ref LL_HRTIM_FAULT_4
  *         @arg @ref LL_HRTIM_FAULT_5
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_FLT_Lock(HRTIM_TypeDef *HRTIMx, uint32_t Fault)
{
  register uint32_t iFault = (uint8_t)POSITION_VAL(Fault);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.FLTINR1) +
                                                    REG_OFFSET_TAB_FLTINR[iFault]));
  SET_BIT(*pReg, (HRTIM_FLTINR1_FLT1LCK << REG_SHIFT_TAB_FLTxE[iFault]));
}

/**
  * @brief  Enable the fault circuitry for the designated fault input.
  * @rmtoll FLTINR1     FLT1E      LL_HRTIM_FLT_Enable\n
  *         FLTINR1     FLT2E      LL_HRTIM_FLT_Enable\n
  *         FLTINR1     FLT3E      LL_HRTIM_FLT_Enable\n
  *         FLTINR1     FLT4E      LL_HRTIM_FLT_Enable\n
  *         FLTINR2     FLT5E      LL_HRTIM_FLT_Enable
  * @param  HRTIMx High Resolution Timer instance
  * @param  Fault This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FAULT_1
  *         @arg @ref LL_HRTIM_FAULT_2
  *         @arg @ref LL_HRTIM_FAULT_3
  *         @arg @ref LL_HRTIM_FAULT_4
  *         @arg @ref LL_HRTIM_FAULT_5
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_FLT_Enable(HRTIM_TypeDef *HRTIMx, uint32_t Fault)
{
  register uint32_t iFault = (uint8_t)POSITION_VAL(Fault);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.FLTINR1) +
                                                    REG_OFFSET_TAB_FLTINR[iFault]));
  SET_BIT(*pReg, (HRTIM_FLTINR1_FLT1E << REG_SHIFT_TAB_FLTxE[iFault]));
}

/**
  * @brief  Disable the fault circuitry for for the designated fault input.
  * @rmtoll FLTINR1     FLT1E      LL_HRTIM_FLT_Disable\n
  *         FLTINR1     FLT2E      LL_HRTIM_FLT_Disable\n
  *         FLTINR1     FLT3E      LL_HRTIM_FLT_Disable\n
  *         FLTINR1     FLT4E      LL_HRTIM_FLT_Disable\n
  *         FLTINR2     FLT5E      LL_HRTIM_FLT_Disable
  * @param  HRTIMx High Resolution Timer instance
  * @param  Fault This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FAULT_1
  *         @arg @ref LL_HRTIM_FAULT_2
  *         @arg @ref LL_HRTIM_FAULT_3
  *         @arg @ref LL_HRTIM_FAULT_4
  *         @arg @ref LL_HRTIM_FAULT_5
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_FLT_Disable(HRTIM_TypeDef *HRTIMx, uint32_t Fault)
{
  register uint32_t iFault = (uint8_t)POSITION_VAL(Fault);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.FLTINR1) +
                                                    REG_OFFSET_TAB_FLTINR[iFault]));
  CLEAR_BIT(*pReg, (HRTIM_FLTINR1_FLT1E << REG_SHIFT_TAB_FLTxE[iFault]));
}

/**
  * @brief  Indicate whether the fault circuitry is enabled for a given fault input.
  * @rmtoll FLTINR1     FLT1E      LL_HRTIM_FLT_IsEnabled\n
  *         FLTINR1     FLT2E      LL_HRTIM_FLT_IsEnabled\n
  *         FLTINR1     FLT3E      LL_HRTIM_FLT_IsEnabled\n
  *         FLTINR1     FLT4E      LL_HRTIM_FLT_IsEnabled\n
  *         FLTINR2     FLT5E      LL_HRTIM_FLT_IsEnabled
  * @param  HRTIMx High Resolution Timer instance  * @param  HRTIMx High Resolution Timer instance
  * @param  Fault This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_FAULT_1
  *         @arg @ref LL_HRTIM_FAULT_2
  *         @arg @ref LL_HRTIM_FAULT_3
  *         @arg @ref LL_HRTIM_FAULT_4
  *         @arg @ref LL_HRTIM_FAULT_5
  * @retval State of FLTxEN bit in HRTIM_FLTINRx register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_FLT_IsEnabled(HRTIM_TypeDef *HRTIMx, uint32_t Fault)
{
  register uint32_t iFault = (uint8_t)POSITION_VAL(Fault);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sCommonRegs.FLTINR1) +
                                                    REG_OFFSET_TAB_FLTINR[iFault]));
  return ((READ_BIT(*pReg, (HRTIM_FLTINR1_FLT1E << REG_SHIFT_TAB_FLTxE[iFault])) >> REG_SHIFT_TAB_FLTxE[iFault]) ==
          (HRTIM_IER_FLT1));
}

/**
  * @}
  */

/** @defgroup HRTIM_EF_Burst_Mode_management Burst_Mode_management
  * @{
  */

/**
  * @brief  Configure the burst mode controller.
  * @rmtoll BMCR     BMOM        LL_HRTIM_BM_Config\n
  *         BMCR     BMCLK       LL_HRTIM_BM_Config\n
  *         BMCR     BMPRSC      LL_HRTIM_BM_Config
  * @param  HRTIMx High Resolution Timer instance
  * @param  Configuration This parameter must be a combination of all the following values:
  *         @arg @ref LL_HRTIM_BM_MODE_SINGLESHOT or @ref LL_HRTIM_BM_MODE_CONTINOUS
  *         @arg @ref LL_HRTIM_BM_CLKSRC_MASTER or ... or @ref LL_HRTIM_BM_CLKSRC_FHRTIM
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV1 or ... @ref LL_HRTIM_BM_PRESCALER_DIV32768
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_BM_Config(HRTIM_TypeDef *HRTIMx, uint32_t Configuration)
{
  MODIFY_REG(HRTIMx->sCommonRegs.BMCR, HRTIM_BM_CONFIG_MASK, Configuration);
}

/**
  * @brief  Set the burst mode controller operating mode.
  * @rmtoll BMCR     BMOM        LL_HRTIM_BM_SetMode
  * @param  HRTIMx High Resolution Timer instance
  * @param  Mode This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_BM_MODE_SINGLESHOT
  *         @arg @ref LL_HRTIM_BM_MODE_CONTINOUS
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_BM_SetMode(HRTIM_TypeDef *HRTIMx, uint32_t Mode)
{
  MODIFY_REG(HRTIMx->sCommonRegs.BMCR, HRTIM_BMCR_BMOM, Mode);
}

/**
  * @brief  Get actual burst mode controller operating mode.
  * @rmtoll BMCR     BMOM        LL_HRTIM_BM_GetMode
  * @param  HRTIMx High Resolution Timer instance
  * @retval Mode This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_BM_MODE_SINGLESHOT
  *         @arg @ref LL_HRTIM_BM_MODE_CONTINOUS
  */
__STATIC_INLINE uint32_t LL_HRTIM_BM_GetMode(HRTIM_TypeDef *HRTIMx)
{
  return (uint32_t)READ_BIT(HRTIMx->sCommonRegs.BMCR, HRTIM_BMCR_BMOM);
}

/**
  * @brief  Set the burst mode controller clock source.
  * @rmtoll BMCR     BMCLK       LL_HRTIM_BM_SetClockSrc
  * @param  HRTIMx High Resolution Timer instance
  * @param  ClockSrc This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_BM_CLKSRC_MASTER
  *         @arg @ref LL_HRTIM_BM_CLKSRC_TIMER_A
  *         @arg @ref LL_HRTIM_BM_CLKSRC_TIMER_B
  *         @arg @ref LL_HRTIM_BM_CLKSRC_TIMER_C
  *         @arg @ref LL_HRTIM_BM_CLKSRC_TIMER_D
  *         @arg @ref LL_HRTIM_BM_CLKSRC_TIMER_E
  *         @arg @ref LL_HRTIM_BM_CLKSRC_TIM16_OC
  *         @arg @ref LL_HRTIM_BM_CLKSRC_TIM17_OC
  *         @arg @ref LL_HRTIM_BM_CLKSRC_TIM7_TRGO
  *         @arg @ref LL_HRTIM_BM_CLKSRC_FHRTIM
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_BM_SetClockSrc(HRTIM_TypeDef *HRTIMx, uint32_t ClockSrc)
{
  MODIFY_REG(HRTIMx->sCommonRegs.BMCR, HRTIM_BMCR_BMCLK, ClockSrc);
}

/**
  * @brief  Get actual burst mode controller clock source.
  * @rmtoll BMCR     BMCLK       LL_HRTIM_BM_GetClockSrc
  * @param  HRTIMx High Resolution Timer instance
  * @retval ClockSrc This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_BM_CLKSRC_MASTER
  *         @arg @ref LL_HRTIM_BM_CLKSRC_TIMER_A
  *         @arg @ref LL_HRTIM_BM_CLKSRC_TIMER_B
  *         @arg @ref LL_HRTIM_BM_CLKSRC_TIMER_C
  *         @arg @ref LL_HRTIM_BM_CLKSRC_TIMER_D
  *         @arg @ref LL_HRTIM_BM_CLKSRC_TIMER_E
  *         @arg @ref LL_HRTIM_BM_CLKSRC_TIM16_OC
  *         @arg @ref LL_HRTIM_BM_CLKSRC_TIM17_OC
  *         @arg @ref LL_HRTIM_BM_CLKSRC_TIM7_TRGO
  *         @arg @ref LL_HRTIM_BM_CLKSRC_FHRTIM
  */
__STATIC_INLINE uint32_t LL_HRTIM_BM_GetClockSrc(HRTIM_TypeDef *HRTIMx)
{
  return (uint32_t)READ_BIT(HRTIMx->sCommonRegs.BMCR, HRTIM_BMCR_BMCLK);
}

/**
  * @brief  Set the burst mode controller prescaler.
  * @rmtoll BMCR     BMPRSC      LL_HRTIM_BM_SetPrescaler
  * @param  HRTIMx High Resolution Timer instance
  * @param  Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV1
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV2
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV4
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV8
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV16
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV32
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV64
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV128
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV256
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV512
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV1024
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV2048
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV4096
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV8192
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV16384
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV32768
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_BM_SetPrescaler(HRTIM_TypeDef *HRTIMx, uint32_t Prescaler)
{
  MODIFY_REG(HRTIMx->sCommonRegs.BMCR, HRTIM_BMCR_BMPRSC, Prescaler);
}

/**
  * @brief  Get actual burst mode controller prescaler setting.
  * @rmtoll BMCR     BMPRSC      LL_HRTIM_BM_GetPrescaler
  * @param  HRTIMx High Resolution Timer instance
  * @retval Prescaler This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV1
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV2
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV4
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV8
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV16
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV32
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV64
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV128
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV256
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV512
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV1024
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV2048
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV4096
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV8192
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV16384
  *         @arg @ref LL_HRTIM_BM_PRESCALER_DIV32768
  */
__STATIC_INLINE uint32_t LL_HRTIM_BM_GetPrescaler(HRTIM_TypeDef *HRTIMx)
{
  return (uint32_t)READ_BIT(HRTIMx->sCommonRegs.BMCR, HRTIM_BMCR_BMPRSC);
}

/**
  * @brief  Enable burst mode compare and period registers preload.
  * @rmtoll BMCR     BMPREN      LL_HRTIM_BM_EnablePreload
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_BM_EnablePreload(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.BMCR, HRTIM_BMCR_BMPREN);
}

/**
  * @brief  Disable burst mode compare and period registers preload.
  * @rmtoll BMCR     BMPREN      LL_HRTIM_BM_DisablePreload
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_BM_DisablePreload(HRTIM_TypeDef *HRTIMx)
{
  CLEAR_BIT(HRTIMx->sCommonRegs.BMCR, HRTIM_BMCR_BMPREN);
}

/**
  * @brief  Indicate whether burst mode compare and period registers are preloaded.
  * @rmtoll BMCR     BMPREN      LL_HRTIM_BM_IsEnabledPreload
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of BMPREN bit in HRTIM_BMCR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_BM_IsEnabledPreload(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.BMCR, HRTIM_BMCR_BMPREN) == HRTIM_BMCR_BMPREN);
}

/**
  * @brief  Set the burst mode controller trigger
  * @rmtoll BMTRGR     SW           LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     MSTRST       LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     MSTREP       LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     MSTCMP1      LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     MSTCMP2      LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     MSTCMP3      LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     MSTCMP4      LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TARST        LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TAREP        LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TACMP1       LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TACMP2       LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TBRST        LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TBREP        LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TBCMP1       LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TBCMP2       LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TCRST        LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TCREP        LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TCCMP1       LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TCCMP2       LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TDRST        LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TDREP        LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TDCMP1       LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TDCMP2       LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TERST        LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TEREP        LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TECMP1       LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TECMP2       LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TAEEV7       LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     TAEEV8       LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     EEV7         LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     EEV8         LL_HRTIM_BM_SetTrig\n
  *         BMTRGR     OCHIPEV      LL_HRTIM_BM_SetTrig
  * @param  HRTIMx High Resolution Timer instance
  * @param  Trig This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_BM_TRIG_NONE
  *         @arg @ref LL_HRTIM_BM_TRIG_MASTER_RESET
  *         @arg @ref LL_HRTIM_BM_TRIG_MASTER_REPETITION
  *         @arg @ref LL_HRTIM_BM_TRIG_MASTER_CMP1
  *         @arg @ref LL_HRTIM_BM_TRIG_MASTER_CMP2
  *         @arg @ref LL_HRTIM_BM_TRIG_MASTER_CMP3
  *         @arg @ref LL_HRTIM_BM_TRIG_MASTER_CMP4
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMA_RESET
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMA_REPETITION
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMA_CMP1
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMA_CMP2
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMB_RESET
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMB_REPETITION
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMB_CMP1
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMB_CMP2
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMC_RESET
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMC_REPETITION
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMC_CMP1
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMC_CMP2
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMD_RESET
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMD_REPETITION
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMD_CMP1
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMD_CMP2
  *         @arg @ref LL_HRTIM_BM_TRIG_TIME_RESET
  *         @arg @ref LL_HRTIM_BM_TRIG_TIME_REPETITION
  *         @arg @ref LL_HRTIM_BM_TRIG_TIME_CMP1
  *         @arg @ref LL_HRTIM_BM_TRIG_TIME_CMP2
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMA_EVENT7
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMD_EVENT8
  *         @arg @ref LL_HRTIM_BM_TRIG_EVENT_7
  *         @arg @ref LL_HRTIM_BM_TRIG_EVENT_8
  *         @arg @ref LL_HRTIM_BM_TRIG_EVENT_ONCHIP
    * @retval None
  */
__STATIC_INLINE void LL_HRTIM_BM_SetTrig(HRTIM_TypeDef *HRTIMx, uint32_t Trig)
{
  WRITE_REG(HRTIMx->sCommonRegs.BMTRGR, Trig);
}

/**
  * @brief  Get actual burst mode controller trigger.
  * @rmtoll BMTRGR     SW           LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     MSTRST       LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     MSTREP       LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     MSTCMP1      LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     MSTCMP2      LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     MSTCMP3      LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     MSTCMP4      LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TARST        LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TAREP        LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TACMP1       LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TACMP2       LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TBRST        LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TBREP        LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TBCMP1       LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TBCMP2       LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TCRST        LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TCREP        LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TCCMP1       LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TCCMP2       LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TDRST        LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TDREP        LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TDCMP1       LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TDCMP2       LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TERST        LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TEREP        LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TECMP1       LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TECMP2       LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TAEEV7       LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     TAEEV8       LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     EEV7         LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     EEV8         LL_HRTIM_BM_GetTrig\n
  *         BMTRGR     OCHIPEV      LL_HRTIM_BM_GetTrig
  * @param  HRTIMx High Resolution Timer instance
  * @retval Trig This parameter can be a combination of the following values:
  *         @arg @ref LL_HRTIM_BM_TRIG_NONE
  *         @arg @ref LL_HRTIM_BM_TRIG_MASTER_RESET
  *         @arg @ref LL_HRTIM_BM_TRIG_MASTER_REPETITION
  *         @arg @ref LL_HRTIM_BM_TRIG_MASTER_CMP1
  *         @arg @ref LL_HRTIM_BM_TRIG_MASTER_CMP2
  *         @arg @ref LL_HRTIM_BM_TRIG_MASTER_CMP3
  *         @arg @ref LL_HRTIM_BM_TRIG_MASTER_CMP4
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMA_RESET
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMA_REPETITION
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMA_CMP1
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMA_CMP2
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMB_RESET
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMB_REPETITION
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMB_CMP1
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMB_CMP2
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMC_RESET
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMC_REPETITION
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMC_CMP1
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMC_CMP2
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMD_RESET
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMD_REPETITION
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMD_CMP1
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMD_CMP2
  *         @arg @ref LL_HRTIM_BM_TRIG_TIME_RESET
  *         @arg @ref LL_HRTIM_BM_TRIG_TIME_REPETITION
  *         @arg @ref LL_HRTIM_BM_TRIG_TIME_CMP1
  *         @arg @ref LL_HRTIM_BM_TRIG_TIME_CMP2
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMA_EVENT7
  *         @arg @ref LL_HRTIM_BM_TRIG_TIMD_EVENT8
  *         @arg @ref LL_HRTIM_BM_TRIG_EVENT_7
  *         @arg @ref LL_HRTIM_BM_TRIG_EVENT_8
  *         @arg @ref LL_HRTIM_BM_TRIG_EVENT_ONCHIP
  */
__STATIC_INLINE uint32_t LL_HRTIM_BM_GetTrig(HRTIM_TypeDef *HRTIMx)
{
  return (uint32_t)READ_REG(HRTIMx->sCommonRegs.BMTRGR);
}

/**
  * @brief  Set the burst mode controller compare value.
  * @rmtoll BMCMPR     BMCMP      LL_HRTIM_BM_SetCompare
  * @param  HRTIMx High Resolution Timer instance
  * @param  CompareValue Compare value must be above or equal to 3
  *         periods of the fHRTIM clock, that is 0x60 if CKPSC[2:0] = 0,
  *         0x30 if CKPSC[2:0] = 1, 0x18 if CKPSC[2:0] = 2,...
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_BM_SetCompare(HRTIM_TypeDef *HRTIMx, uint32_t CompareValue)
{
  WRITE_REG(HRTIMx->sCommonRegs.BMCMPR, CompareValue);
}

/**
  * @brief  Get actual burst mode controller compare value.
  * @rmtoll BMCMPR     BMCMP      LL_HRTIM_BM_GetCompare
  * @param  HRTIMx High Resolution Timer instance
  * @retval CompareValue Compare value must be above or equal to 3
  *         periods of the fHRTIM clock, that is 0x60 if CKPSC[2:0] = 0,
  *         0x30 if CKPSC[2:0] = 1, 0x18 if CKPSC[2:0] = 2,...
  */
__STATIC_INLINE uint32_t LL_HRTIM_BM_GetCompare(HRTIM_TypeDef *HRTIMx)
{
  return (uint32_t)READ_REG(HRTIMx->sCommonRegs.BMCMPR);
}

/**
  * @brief  Set the burst mode controller period.
  * @rmtoll BMPER     BMPER      LL_HRTIM_BM_SetPeriod
  * @param  HRTIMx High Resolution Timer instance
  * @param  Period The period value must be above or equal to 3 periods of the fHRTIM clock,
  *         that is 0x60 if CKPSC[2:0] = 0, 0x30 if CKPSC[2:0] = 1, 0x18 if CKPSC[2:0] = 2,...
  *         The maximum value is 0x0000 FFDF.
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_BM_SetPeriod(HRTIM_TypeDef *HRTIMx, uint32_t Period)
{
  WRITE_REG(HRTIMx->sCommonRegs.BMPER, Period);
}

/**
  * @brief  Get actual burst mode controller period.
  * @rmtoll BMPER     BMPER      LL_HRTIM_BM_GetPeriod
  * @param  HRTIMx High Resolution Timer instance
  * @retval The period value must be above or equal to 3 periods of the fHRTIM clock,
  *         that is 0x60 if CKPSC[2:0] = 0, 0x30 if CKPSC[2:0] = 1, 0x18 if CKPSC[2:0] = 2,...
  *         The maximum value is 0x0000 FFDF.
  */
__STATIC_INLINE uint32_t LL_HRTIM_BM_GetPeriod(HRTIM_TypeDef *HRTIMx)
{
  return (uint32_t)READ_REG(HRTIMx->sCommonRegs.BMPER);
}

/**
  * @brief  Enable the burst mode controller
  * @rmtoll BMCR     BME      LL_HRTIM_BM_Enable
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_BM_Enable(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.BMCR, HRTIM_BMCR_BME);
}

/**
  * @brief  Disable the burst mode controller
  * @rmtoll BMCR     BME      LL_HRTIM_BM_Disable
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_BM_Disable(HRTIM_TypeDef *HRTIMx)
{
  CLEAR_BIT(HRTIMx->sCommonRegs.BMCR, HRTIM_BMCR_BME);
}

/**
  * @brief  Indicate whether the burst mode controller is enabled.
  * @rmtoll BMCR     BME      LL_HRTIM_BM_IsEnabled
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of BME bit in HRTIM_BMCR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_BM_IsEnabled(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.BMCR, HRTIM_BMCR_BME) == HRTIM_BMCR_BME);
}

/**
  * @brief  Trigger the burst operation (software trigger)
  * @rmtoll BMTRGR     SW           LL_HRTIM_BM_Start
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_BM_Start(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.BMTRGR, HRTIM_BMTRGR_SW);
}

/**
  * @brief  Stop the burst mode operation.
  * @rmtoll BMCR     BMSTAT           LL_HRTIM_BM_Stop
  * @note Causes a burst mode early termination.
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_BM_Stop(HRTIM_TypeDef *HRTIMx)
{
  CLEAR_BIT(HRTIMx->sCommonRegs.BMCR, HRTIM_BMCR_BMSTAT);
}

/**
  * @brief  Get actual burst mode status
  * @rmtoll BMCR     BMSTAT           LL_HRTIM_BM_GetStatus
  * @param  HRTIMx High Resolution Timer instance
  * @retval Status This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_BM_STATUS_NORMAL
  *         @arg @ref LL_HRTIM_BM_STATUS_BURST_ONGOING
  */
__STATIC_INLINE uint32_t LL_HRTIM_BM_GetStatus(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.BMCR, HRTIM_BMCR_BMSTAT));
}

/**
  * @}
  */

/** @defgroup HRTIM_EF_FLAG_Management FLAG_Management
  * @{
  */

/**
  * @brief  Clear the Fault 1 interrupt flag.
  * @rmtoll ICR     FLT1C           LL_HRTIM_ClearFlag_FLT1
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_FLT1(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.ICR, HRTIM_ICR_FLT1C);
}

/**
  * @brief  Indicate whether Fault 1 interrupt occurred.
  * @rmtoll ICR     FLT1           LL_HRTIM_IsActiveFlag_FLT1
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of FLT1 bit in HRTIM_ISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_FLT1(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.ISR, HRTIM_ISR_FLT1) == (HRTIM_ISR_FLT1));
}

/**
  * @brief  Clear the Fault 2 interrupt flag.
  * @rmtoll ICR     FLT2C           LL_HRTIM_ClearFlag_FLT2
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_FLT2(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.ICR, HRTIM_ICR_FLT2C);
}

/**
  * @brief  Indicate whether Fault 2 interrupt occurred.
  * @rmtoll ICR     FLT2           LL_HRTIM_IsActiveFlag_FLT2
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of FLT2 bit in HRTIM_ISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_FLT2(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.ISR, HRTIM_ISR_FLT2) == (HRTIM_ISR_FLT2));
}

/**
  * @brief  Clear the Fault 3 interrupt flag.
  * @rmtoll ICR     FLT3C           LL_HRTIM_ClearFlag_FLT3
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_FLT3(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.ICR, HRTIM_ICR_FLT3C);
}

/**
  * @brief  Indicate whether Fault 3 interrupt occurred.
  * @rmtoll ICR     FLT3           LL_HRTIM_IsActiveFlag_FLT3
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of FLT3 bit in HRTIM_ISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_FLT3(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.ISR, HRTIM_ISR_FLT3) == (HRTIM_ISR_FLT3));
}

/**
  * @brief  Clear the Fault 4 interrupt flag.
  * @rmtoll ICR     FLT4C           LL_HRTIM_ClearFlag_FLT4
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_FLT4(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.ICR, HRTIM_ICR_FLT4C);
}

/**
  * @brief  Indicate whether Fault 4 interrupt occurred.
  * @rmtoll ICR     FLT4           LL_HRTIM_IsActiveFlag_FLT4
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of FLT4 bit in HRTIM_ISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_FLT4(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.ISR, HRTIM_ISR_FLT4) == (HRTIM_ISR_FLT4));
}

/**
  * @brief  Clear the Fault 5 interrupt flag.
  * @rmtoll ICR     FLT5C           LL_HRTIM_ClearFlag_FLT5
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_FLT5(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.ICR, HRTIM_ICR_FLT5C);
}

/**
  * @brief  Indicate whether Fault 5 interrupt occurred.
  * @rmtoll ICR     FLT5           LL_HRTIM_IsActiveFlag_FLT5
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of FLT5 bit in HRTIM_ISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_FLT5(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.ISR, HRTIM_ISR_FLT5) == (HRTIM_ISR_FLT5));
}

/**
  * @brief  Clear the System Fault interrupt flag.
  * @rmtoll ICR     SYSFLTC           LL_HRTIM_ClearFlag_SYSFLT
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_SYSFLT(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.ICR, HRTIM_ICR_SYSFLTC);
}

/**
  * @brief  Indicate whether System Fault interrupt occurred.
  * @rmtoll ISR     SYSFLT           LL_HRTIM_IsActiveFlag_SYSFLT
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of SYSFLT bit in HRTIM_ISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_SYSFLT(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.ISR, HRTIM_ISR_SYSFLT) == (HRTIM_ISR_SYSFLT));
}

/**
  * @brief  Clear the DLL ready interrupt flag.
  * @rmtoll ICR     DLLRDYC           LL_HRTIM_ClearFlag_DLLRDY
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_DLLRDY(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.ICR, HRTIM_ICR_DLLRDYC);
}

/**
  * @brief  Indicate whether DLL ready  interrupt occurred.
  * @rmtoll ISR     DLLRDY           LL_HRTIM_IsActiveFlag_DLLRDY
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of DLLRDY bit in HRTIM_ISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_DLLRDY(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.ISR, HRTIM_ISR_DLLRDY) == (HRTIM_ISR_DLLRDY));
}

/**
  * @brief  Clear the Burst Mode period interrupt flag.
  * @rmtoll ICR     BMPERC           LL_HRTIM_ClearFlag_BMPER
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_BMPER(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.ICR, HRTIM_ICR_BMPERC);
}

/**
  * @brief  Indicate whether Burst Mode period interrupt occurred.
  * @rmtoll ISR     BMPER           LL_HRTIM_IsActiveFlag_BMPER
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of BMPER bit in HRTIM_ISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_BMPER(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.ISR, HRTIM_ISR_BMPER) == (HRTIM_ISR_BMPER));
}

/**
  * @brief  Clear the Synchronization Input interrupt flag.
  * @rmtoll MICR     SYNCC           LL_HRTIM_ClearFlag_SYNC
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_SYNC(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sMasterRegs.MICR, HRTIM_MICR_SYNC);
}

/**
  * @brief  Indicate whether the Synchronization Input interrupt occurred.
  * @rmtoll MISR     SYNC           LL_HRTIM_IsActiveFlag_SYNC
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of SYNC bit in HRTIM_MISR register  (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_SYNC(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sMasterRegs.MISR, HRTIM_MISR_SYNC) == (HRTIM_MISR_SYNC));
}

/**
  * @brief  Clear the update interrupt flag for a given timer (including the master timer) .
  * @rmtoll MICR        MUPDC          LL_HRTIM_ClearFlag_UPDATE\n
  *         TIMxICR     UPDC           LL_HRTIM_ClearFlag_UPDATE
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_UPDATE(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MICR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MICR_MUPD);
}

/**
  * @brief  Indicate whether the update interrupt has occurred for a given timer (including the master timer) .
  * @rmtoll MISR        MUPD          LL_HRTIM_IsActiveFlag_UPDATE\n
  *         TIMxISR     UPD           LL_HRTIM_IsActiveFlag_UPDATE
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MUPD/UPD bit in HRTIM_MISR/HRTIM_TIMxISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_UPDATE(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MISR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MISR_MUPD) == (HRTIM_MISR_MUPD));
}

/**
  * @brief  Clear the repetition interrupt flag for a given timer (including the master timer) .
  * @rmtoll MICR        MREPC          LL_HRTIM_ClearFlag_REP\n
  *         TIMxICR     REPC           LL_HRTIM_ClearFlag_REP
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_REP(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MICR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MICR_MREP);

}

/**
  * @brief  Indicate whether the repetition  interrupt has occurred for a given timer (including the master timer) .
  * @rmtoll MISR        MREP          LL_HRTIM_IsActiveFlag_REP\n
  *         TIMxISR     REP           LL_HRTIM_IsActiveFlag_REP
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MREP/REP bit in HRTIM_MISR/HRTIM_TIMxISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_REP(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MISR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MISR_MREP) == (HRTIM_MISR_MREP));
}

/**
  * @brief  Clear the compare 1 match interrupt for a given timer (including the master timer).
  * @rmtoll MICR        MCMP1C          LL_HRTIM_ClearFlag_CMP1\n
  *         TIMxICR     CMP1C           LL_HRTIM_ClearFlag_CMP1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_CMP1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MICR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MICR_MCMP1);
}

/**
  * @brief  Indicate whether the compare match 1  interrupt has occurred for a given timer (including the master timer) .
  * @rmtoll MISR        MCMP1          LL_HRTIM_IsActiveFlag_CMP1\n
  *         TIMxISR     CMP1           LL_HRTIM_IsActiveFlag_CMP1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MCMP1/CMP1 bit in HRTIM_MISR/HRTIM_TIMxISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_CMP1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MISR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MISR_MCMP1) == (HRTIM_MISR_MCMP1));
}

/**
  * @brief  Clear the compare 2 match interrupt for a given timer (including the master timer).
  * @rmtoll MICR        MCMP2C          LL_HRTIM_ClearFlag_CMP2\n
  *         TIMxICR     CMP2C           LL_HRTIM_ClearFlag_CMP2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_CMP2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MICR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MICR_MCMP2);
}

/**
  * @brief  Indicate whether the compare match 2  interrupt has occurred for a given timer (including the master timer) .
  * @rmtoll MISR        MCMP2          LL_HRTIM_IsActiveFlag_CMP2\n
  *         TIMxISR     CMP2           LL_HRTIM_IsActiveFlag_CMP2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MCMP2/CMP2 bit in HRTIM_MISR/HRTIM_TIMxISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_CMP2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MISR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MISR_MCMP2) == (HRTIM_MISR_MCMP2));
}

/**
  * @brief  Clear the compare 3 match interrupt for a given timer (including the master timer).
  * @rmtoll MICR        MCMP3C          LL_HRTIM_ClearFlag_CMP3\n
  *         TIMxICR     CMP3C           LL_HRTIM_ClearFlag_CMP3
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_CMP3(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MICR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MICR_MCMP3);
}

/**
  * @brief  Indicate whether the compare match 3  interrupt has occurred for a given timer (including the master timer) .
  * @rmtoll MISR        MCMP3          LL_HRTIM_IsActiveFlag_CMP3\n
  *         TIMxISR     CMP3           LL_HRTIM_IsActiveFlag_CMP3
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MCMP3/CMP3 bit in HRTIM_MISR/HRTIM_TIMxISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_CMP3(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MISR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MISR_MCMP3) == (HRTIM_MISR_MCMP3));
}

/**
  * @brief  Clear the compare 4 match interrupt for a given timer (including the master timer).
  * @rmtoll MICR        MCMP4C          LL_HRTIM_ClearFlag_CMP4\n
  *         TIMxICR     CMP4C           LL_HRTIM_ClearFlag_CMP4
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_CMP4(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MICR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MICR_MCMP4);
}

/**
  * @brief  Indicate whether the compare match 4  interrupt has occurred for a given timer (including the master timer) .
  * @rmtoll MISR        MCMP4          LL_HRTIM_IsActiveFlag_CMP4\n
  *         TIMxISR     CMP4           LL_HRTIM_IsActiveFlag_CMP4
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MCMP4/CMP4 bit in HRTIM_MISR/HRTIM_TIMxISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_CMP4(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MISR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MISR_MCMP4) == (HRTIM_MISR_MCMP4));
}

/**
  * @brief  Clear the capture 1 interrupt flag for a given timer.
  * @rmtoll TIMxICR     CPT1C           LL_HRTIM_ClearFlag_CPT1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_CPT1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MICR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMICR_CPT1C);
}

/**
  * @brief  Indicate whether the capture 1 interrupt occurred for a given timer.
  * @rmtoll TIMxISR     CPT1           LL_HRTIM_IsActiveFlag_CPT1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of CPT1 bit in HRTIM_TIMxISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_CPT1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MISR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMISR_CPT1) == (HRTIM_TIMISR_CPT1));
}

/**
  * @brief  Clear the capture 2 interrupt flag for a given timer.
  * @rmtoll TIMxICR     CPT2C           LL_HRTIM_ClearFlag_CPT2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_CPT2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MICR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMICR_CPT2C);
}

/**
  * @brief  Indicate whether the capture 2 interrupt occurred for a given timer.
  * @rmtoll TIMxISR     CPT2           LL_HRTIM_IsActiveFlag_CPT2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of CPT2 bit in HRTIM_TIMxISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_CPT2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MISR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMISR_CPT2) == (HRTIM_TIMISR_CPT2));
}

/**
  * @brief  Clear the output 1 set interrupt flag for a given timer.
  * @rmtoll TIMxICR     SET1C           LL_HRTIM_ClearFlag_SET1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_SET1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MICR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMICR_SET1C);
}

/**
  * @brief  Indicate whether the output 1 set interrupt occurred for a given timer.
  * @rmtoll TIMxISR     SET1           LL_HRTIM_IsActiveFlag_SET1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of SETx1 bit in HRTIM_TIMxISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_SET1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MISR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMISR_SET1) == (HRTIM_TIMISR_SET1));
}

/**
  * @brief  Clear the output 1 reset interrupt flag for a given timer.
  * @rmtoll TIMxICR     RST1C           LL_HRTIM_ClearFlag_RST1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_RST1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MICR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMICR_RST1C);
}

/**
  * @brief  Indicate whether the output 1 reset interrupt occurred for a given timer.
  * @rmtoll TIMxISR     RST1           LL_HRTIM_IsActiveFlag_RST1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of RSTx1 bit in HRTIM_TIMxISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_RST1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MISR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMISR_RST1) == (HRTIM_TIMISR_RST1));
}

/**
  * @brief  Clear the output 2 set interrupt flag for a given timer.
  * @rmtoll TIMxICR     SET2C           LL_HRTIM_ClearFlag_SET2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_SET2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MICR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMICR_SET2C);
}

/**
  * @brief  Indicate whether the output 2 set interrupt occurred for a given timer.
  * @rmtoll TIMxISR     SET2           LL_HRTIM_IsActiveFlag_SET2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of SETx2 bit in HRTIM_TIMxISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_SET2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MISR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMISR_SET2) == (HRTIM_TIMISR_SET2));
}

/**
  * @brief  Clear the output 2reset interrupt flag for a given timer.
  * @rmtoll TIMxICR     RST2C           LL_HRTIM_ClearFlag_RST2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_RST2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MICR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMICR_RST2C);
}

/**
  * @brief  Indicate whether the output 2 reset interrupt occurred for a given timer.
  * @rmtoll TIMxISR     RST2           LL_HRTIM_IsActiveFlag_RST2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of RSTx2 bit in HRTIM_TIMxISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_RST2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MISR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMISR_RST2) == (HRTIM_TIMISR_RST2));
}

/**
  * @brief  Clear the reset and/or roll-over interrupt flag for a given timer.
  * @rmtoll TIMxICR     RSTC           LL_HRTIM_ClearFlag_RST
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_RST(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MICR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMICR_RSTC);
}

/**
  * @brief  Indicate whether the  reset and/or roll-over interrupt occurred for a given timer.
  * @rmtoll TIMxISR     RST           LL_HRTIM_IsActiveFlag_RST
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of RST bit in HRTIM_TIMxISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_RST(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MISR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMISR_RST) == (HRTIM_TIMISR_RST));
}

/**
  * @brief  Clear the delayed protection interrupt flag for a given timer.
  * @rmtoll TIMxICR     DLYPRTC           LL_HRTIM_ClearFlag_DLYPRT
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_ClearFlag_DLYPRT(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MICR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMICR_DLYPRT1C);
}

/**
  * @brief  Indicate whether the  delayed protection interrupt occurred for a given timer.
  * @rmtoll TIMxISR     DLYPRT           LL_HRTIM_IsActiveFlag_DLYPRT
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of DLYPRT bit in HRTIM_TIMxISR register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsActiveFlag_DLYPRT(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MISR) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMISR_DLYPRT) == (HRTIM_TIMISR_DLYPRT));
}

/**
  * @}
  */

/** @defgroup HRTIM_EF_IT_Management IT_Management
  * @{
  */

/**
  * @brief  Enable the fault 1 interrupt.
  * @rmtoll IER     FLT1IE           LL_HRTIM_EnableIT_FLT1
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_FLT1(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_FLT1);
}

/**
  * @brief  Disable the fault 1 interrupt.
  * @rmtoll IER     FLT1IE           LL_HRTIM_DisableIT_FLT1
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_FLT1(HRTIM_TypeDef *HRTIMx)
{
  CLEAR_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_FLT1);
}

/**
  * @brief  Indicate whether the fault 1 interrupt is enabled.
  * @rmtoll IER     FLT1IE           LL_HRTIM_IsEnabledIT_FLT1
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of FLT1IE bit in HRTIM_IER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_FLT1(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_FLT1) == (HRTIM_IER_FLT1));
}

/**
  * @brief  Enable the fault 2 interrupt.
  * @rmtoll IER     FLT2IE           LL_HRTIM_EnableIT_FLT2
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_FLT2(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_FLT2);
}

/**
  * @brief  Disable the fault 2 interrupt.
  * @rmtoll IER     FLT2IE           LL_HRTIM_DisableIT_FLT2
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_FLT2(HRTIM_TypeDef *HRTIMx)
{
  CLEAR_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_FLT2);
}

/**
  * @brief  Indicate whether the fault 2 interrupt is enabled.
  * @rmtoll IER     FLT2IE           LL_HRTIM_IsEnabledIT_FLT2
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of FLT2IE bit in HRTIM_IER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_FLT2(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_FLT2) == (HRTIM_IER_FLT2));
}

/**
  * @brief  Enable the fault 3 interrupt.
  * @rmtoll IER     FLT3IE           LL_HRTIM_EnableIT_FLT3
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_FLT3(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_FLT3);
}

/**
  * @brief  Disable the fault 3 interrupt.
  * @rmtoll IER     FLT3IE           LL_HRTIM_DisableIT_FLT3
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_FLT3(HRTIM_TypeDef *HRTIMx)
{
  CLEAR_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_FLT3);
}

/**
  * @brief  Indicate whether the fault 3 interrupt is enabled.
  * @rmtoll IER     FLT3IE           LL_HRTIM_IsEnabledIT_FLT3
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of FLT3IE bit in HRTIM_IER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_FLT3(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_FLT3) == (HRTIM_IER_FLT3));
}

/**
  * @brief  Enable the fault 4 interrupt.
  * @rmtoll IER     FLT4IE           LL_HRTIM_EnableIT_FLT4
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_FLT4(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_FLT4);
}

/**
  * @brief  Disable the fault 4 interrupt.
  * @rmtoll IER     FLT4IE           LL_HRTIM_DisableIT_FLT4
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_FLT4(HRTIM_TypeDef *HRTIMx)
{
  CLEAR_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_FLT4);
}

/**
  * @brief  Indicate whether the fault 4 interrupt is enabled.
  * @rmtoll IER     FLT4IE           LL_HRTIM_IsEnabledIT_FLT4
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of FLT4IE bit in HRTIM_IER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_FLT4(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_FLT4) == (HRTIM_IER_FLT4));
}

/**
  * @brief  Enable the fault 5 interrupt.
  * @rmtoll IER     FLT5IE           LL_HRTIM_EnableIT_FLT5
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_FLT5(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_FLT5);
}

/**
  * @brief  Disable the fault 5 interrupt.
  * @rmtoll IER     FLT5IE           LL_HRTIM_DisableIT_FLT5
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_FLT5(HRTIM_TypeDef *HRTIMx)
{
  CLEAR_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_FLT5);
}

/**
  * @brief  Indicate whether the fault 5 interrupt is enabled.
  * @rmtoll IER     FLT5IE           LL_HRTIM_IsEnabledIT_FLT5
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of FLT5IE bit in HRTIM_IER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_FLT5(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_FLT5) == (HRTIM_IER_FLT5));
}

/**
  * @brief  Enable the system fault interrupt.
  * @rmtoll IER     SYSFLTIE           LL_HRTIM_EnableIT_SYSFLT
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_SYSFLT(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_SYSFLT);
}

/**
  * @brief  Disable the system fault interrupt.
  * @rmtoll IER     SYSFLTIE           LL_HRTIM_DisableIT_SYSFLT
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_SYSFLT(HRTIM_TypeDef *HRTIMx)
{
  CLEAR_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_SYSFLT);
}

/**
  * @brief  Indicate whether the system fault interrupt is enabled.
  * @rmtoll IER     SYSFLTIE           LL_HRTIM_IsEnabledIT_SYSFLT
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of SYSFLTIE bit in HRTIM_IER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_SYSFLT(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_SYSFLT) == (HRTIM_IER_SYSFLT));
}

/**
  * @brief  Enable the DLL ready interrupt.
  * @rmtoll IER     DLLRDYIE           LL_HRTIM_EnableIT_DLLRDY
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_DLLRDY(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_DLLRDY);
}

/**
  * @brief  Disable the DLL ready interrupt.
  * @rmtoll IER     DLLRDYIE           LL_HRTIM_DisableIT_DLLRDY
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_DLLRDY(HRTIM_TypeDef *HRTIMx)
{
  CLEAR_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_DLLRDY);
}

/**
  * @brief  Indicate whether the DLL ready interrupt is enabled.
  * @rmtoll IER     DLLRDYIE           LL_HRTIM_IsEnabledIT_DLLRDY
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of DLLRDYIE bit in HRTIM_IER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_DLLRDY(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_DLLRDY) == (HRTIM_IER_DLLRDY));
}

/**
  * @brief  Enable the burst mode period interrupt.
  * @rmtoll IER     BMPERIE           LL_HRTIM_EnableIT_BMPER
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_BMPER(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_BMPER);
}

/**
  * @brief  Disable the burst mode period interrupt.
  * @rmtoll IER     BMPERIE           LL_HRTIM_DisableIT_BMPER
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_BMPER(HRTIM_TypeDef *HRTIMx)
{
  CLEAR_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_BMPER);
}

/**
  * @brief  Indicate whether the burst mode period interrupt is enabled.
  * @rmtoll IER     BMPERIE           LL_HRTIM_IsEnabledIT_BMPER
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of BMPERIE bit in HRTIM_IER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_BMPER(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sCommonRegs.IER, HRTIM_IER_BMPER) == (HRTIM_IER_BMPER));
}

/**
  * @brief  Enable the synchronization input interrupt.
  * @rmtoll MDIER     SYNCIE           LL_HRTIM_EnableIT_SYNC
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_SYNC(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sMasterRegs.MDIER, HRTIM_MDIER_SYNCIE);
}

/**
  * @brief  Disable the synchronization input interrupt.
  * @rmtoll MDIER     SYNCIE           LL_HRTIM_DisableIT_SYNC
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_SYNC(HRTIM_TypeDef *HRTIMx)
{
  CLEAR_BIT(HRTIMx->sMasterRegs.MDIER, HRTIM_MDIER_SYNCIE);
}

/**
  * @brief  Indicate whether the synchronization input interrupt is enabled.
  * @rmtoll MDIER     SYNCIE           LL_HRTIM_IsEnabledIT_SYNC
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of SYNCIE bit in HRTIM_MDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_SYNC(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sMasterRegs.MDIER, HRTIM_MDIER_SYNCIE) == (HRTIM_MDIER_SYNCIE));
}

/**
  * @brief  Enable the update interrupt for a given timer.
  * @rmtoll MDIER        MUPDIE           LL_HRTIM_EnableIT_UPDATE\n
  *         TIMxDIER     UPDIE            LL_HRTIM_EnableIT_UPDATE
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_UPDATE(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MDIER_MUPDIE);
}

/**
  * @brief  Disable the update interrupt for a given timer.
  * @rmtoll MDIER        MUPDIE           LL_HRTIM_DisableIT_UPDATE\n
  *         TIMxDIER     UPDIE            LL_HRTIM_DisableIT_UPDATE
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_UPDATE(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_MDIER_MUPDIE);
}

/**
  * @brief  Indicate whether the update interrupt is enabled for a given timer.
  * @rmtoll MDIER        MUPDIE           LL_HRTIM_IsEnabledIT_UPDATE\n
  *         TIMxDIER     UPDIE            LL_HRTIM_IsEnabledIT_UPDATE
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MUPDIE/UPDIE bit in HRTIM_MDIER/HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_UPDATE(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MDIER_MUPDIE) == (HRTIM_MDIER_MUPDIE));
}

/**
  * @brief  Enable the repetition interrupt for a given timer.
  * @rmtoll MDIER        MREPIE           LL_HRTIM_EnableIT_REP\n
  *         TIMxDIER     REPIE            LL_HRTIM_EnableIT_REP
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_REP(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MDIER_MREPIE);
}

/**
  * @brief  Disable the repetition interrupt for a given timer.
  * @rmtoll MDIER        MREPIE           LL_HRTIM_DisableIT_REP\n
  *         TIMxDIER     REPIE            LL_HRTIM_DisableIT_REP
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_REP(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_MDIER_MREPIE);
}

/**
  * @brief  Indicate whether the repetition interrupt is enabled for a given timer.
  * @rmtoll MDIER        MREPIE           LL_HRTIM_IsEnabledIT_REP\n
  *         TIMxDIER     REPIE            LL_HRTIM_IsEnabledIT_REP
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MREPIE/REPIE bit in HRTIM_MDIER/HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_REP(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MDIER_MREPIE) == (HRTIM_MDIER_MREPIE));
}

/**
  * @brief  Enable the compare 1 interrupt for a given timer.
  * @rmtoll MDIER        MCMP1IE           LL_HRTIM_EnableIT_CMP1\n
  *         TIMxDIER     CMP1IE            LL_HRTIM_EnableIT_CMP1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_CMP1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MDIER_MCMP1IE);
}

/**
  * @brief  Disable the compare 1 interrupt for a given timer.
  * @rmtoll MDIER        MCMP1IE           LL_HRTIM_DisableIT_CMP1\n
  *         TIMxDIER     CMP1IE            LL_HRTIM_DisableIT_CMP1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_CMP1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_MDIER_MCMP1IE);
}

/**
  * @brief  Indicate whether the compare 1 interrupt is enabled for a given timer.
  * @rmtoll MDIER        MCMP1IE           LL_HRTIM_IsEnabledIT_CMP1\n
  *         TIMxDIER     CMP1IE            LL_HRTIM_IsEnabledIT_CMP1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MCMP1IE/CMP1IE bit in HRTIM_MDIER/HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_CMP1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MDIER_MCMP1IE) == (HRTIM_MDIER_MCMP1IE));
}

/**
  * @brief  Enable the compare 2 interrupt for a given timer.
  * @rmtoll MDIER        MCMP2IE           LL_HRTIM_EnableIT_CMP2\n
  *         TIMxDIER     CMP2IE            LL_HRTIM_EnableIT_CMP2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_CMP2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MDIER_MCMP2IE);
}

/**
  * @brief  Disable the compare 2 interrupt for a given timer.
  * @rmtoll MDIER        MCMP2IE           LL_HRTIM_DisableIT_CMP2\n
  *         TIMxDIER     CMP2IE            LL_HRTIM_DisableIT_CMP2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_CMP2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_MDIER_MCMP2IE);
}

/**
  * @brief  Indicate whether the compare 2 interrupt is enabled for a given timer.
  * @rmtoll MDIER        MCMP2IE           LL_HRTIM_IsEnabledIT_CMP2\n
  *         TIMxDIER     CMP2IE            LL_HRTIM_IsEnabledIT_CMP2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MCMP2IE/CMP2IE bit in HRTIM_MDIER/HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_CMP2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MDIER_MCMP2IE) == (HRTIM_MDIER_MCMP2IE));
}

/**
  * @brief  Enable the compare 3 interrupt for a given timer.
  * @rmtoll MDIER        MCMP3IE           LL_HRTIM_EnableIT_CMP3\n
  *         TIMxDIER     CMP3IE            LL_HRTIM_EnableIT_CMP3
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_CMP3(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MDIER_MCMP3IE);
}

/**
  * @brief  Disable the compare 3 interrupt for a given timer.
  * @rmtoll MDIER        MCMP3IE           LL_HRTIM_DisableIT_CMP3\n
  *         TIMxDIER     CMP3IE            LL_HRTIM_DisableIT_CMP3
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_CMP3(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_MDIER_MCMP3IE);
}

/**
  * @brief  Indicate whether the compare 3 interrupt is enabled for a given timer.
  * @rmtoll MDIER        MCMP3IE           LL_HRTIM_IsEnabledIT_CMP3\n
  *         TIMxDIER     CMP3IE            LL_HRTIM_IsEnabledIT_CMP3
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MCMP3IE/CMP3IE bit in HRTIM_MDIER/HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_CMP3(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MDIER_MCMP3IE) == (HRTIM_MDIER_MCMP3IE));
}

/**
  * @brief  Enable the compare 4 interrupt for a given timer.
  * @rmtoll MDIER        MCMP4IE           LL_HRTIM_EnableIT_CMP4\n
  *         TIMxDIER     CMP4IE            LL_HRTIM_EnableIT_CMP4
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_CMP4(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MDIER_MCMP4IE);
}

/**
  * @brief  Disable the compare 4 interrupt for a given timer.
  * @rmtoll MDIER        MCMP4IE           LL_HRTIM_DisableIT_CMP4\n
  *         TIMxDIER     CMP4IE            LL_HRTIM_DisableIT_CMP4
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_CMP4(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_MDIER_MCMP4IE);
}

/**
  * @brief  Indicate whether the compare 4 interrupt is enabled for a given timer.
  * @rmtoll MDIER        MCMP4IE           LL_HRTIM_IsEnabledIT_CMP4\n
  *         TIMxDIER     CMP4IE            LL_HRTIM_IsEnabledIT_CMP4
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MCMP4IE/CMP4IE bit in HRTIM_MDIER/HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_CMP4(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MDIER_MCMP4IE) == (HRTIM_MDIER_MCMP4IE));
}

/**
  * @brief  Enable the capture 1 interrupt for a given timer.
  * @rmtoll TIMxDIER     CPT1IE            LL_HRTIM_EnableIT_CPT1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_CPT1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMDIER_CPT1IE);
}

/**
  * @brief  Enable the capture 1 interrupt for a given timer.
  * @rmtoll TIMxDIER     CPT1IE            LL_HRTIM_DisableIT_CPT1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_CPT1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMDIER_CPT1IE);
}

/**
  * @brief  Indicate whether the capture 1 interrupt is enabled for a given timer.
  * @rmtoll TIMxDIER     CPT1IE            LL_HRTIM_IsEnabledIT_CPT1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of CPT1IE bit in HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_CPT1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMDIER_CPT1IE) == (HRTIM_TIMDIER_CPT1IE));
}

/**
  * @brief  Enable the capture 2 interrupt for a given timer.
  * @rmtoll TIMxDIER     CPT2IE            LL_HRTIM_EnableIT_CPT2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_CPT2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMDIER_CPT2IE);
}

/**
  * @brief  Enable the capture 2 interrupt for a given timer.
  * @rmtoll TIMxDIER     CPT2IE            LL_HRTIM_DisableIT_CPT2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_CPT2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMDIER_CPT2IE);
}

/**
  * @brief  Indicate whether the capture 2 interrupt is enabled for a given timer.
  * @rmtoll TIMxDIER     CPT2IE            LL_HRTIM_IsEnabledIT_CPT2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of CPT2IE bit in HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_CPT2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMDIER_CPT2IE) == (HRTIM_TIMDIER_CPT2IE));
}

/**
  * @brief  Enable the output 1 set interrupt for a given timer.
  * @rmtoll TIMxDIER     SET1IE            LL_HRTIM_EnableIT_SET1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_SET1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMDIER_SET1IE);
}

/**
  * @brief  Disable the output 1 set interrupt for a given timer.
  * @rmtoll TIMxDIER     SET1IE            LL_HRTIM_DisableIT_SET1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_SET1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMDIER_SET1IE);
}

/**
  * @brief  Indicate whether the output 1 set interrupt is enabled for a given timer.
  * @rmtoll TIMxDIER     SET1IE            LL_HRTIM_IsEnabledIT_SET1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of SET1xIE bit in HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_SET1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMDIER_SET1IE) == (HRTIM_TIMDIER_SET1IE));
}

/**
  * @brief  Enable the output 1 reset interrupt for a given timer.
  * @rmtoll TIMxDIER     RST1IE            LL_HRTIM_EnableIT_RST1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_RST1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMDIER_RST1IE);
}

/**
  * @brief  Disable the output 1 reset interrupt for a given timer.
  * @rmtoll TIMxDIER     RST1IE            LL_HRTIM_DisableIT_RST1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_RST1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMDIER_RST1IE);
}

/**
  * @brief  Indicate whether the output 1 reset interrupt is enabled for a given timer.
  * @rmtoll TIMxDIER     RST1IE            LL_HRTIM_IsEnabledIT_RST1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of RST1xIE bit in HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_RST1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMDIER_RST1IE) == (HRTIM_TIMDIER_RST1IE));
}

/**
  * @brief  Enable the output 2 set interrupt for a given timer.
  * @rmtoll TIMxDIER     SET2IE            LL_HRTIM_EnableIT_SET2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_SET2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMDIER_SET2IE);
}

/**
  * @brief  Disable the output 2 set interrupt for a given timer.
  * @rmtoll TIMxDIER     SET2IE            LL_HRTIM_DisableIT_SET2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_SET2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMDIER_SET2IE);
}

/**
  * @brief  Indicate whether the output 2 set interrupt is enabled for a given timer.
  * @rmtoll TIMxDIER     SET2IE            LL_HRTIM_IsEnabledIT_SET2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of SET2xIE bit in HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_SET2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMDIER_SET2IE) == (HRTIM_TIMDIER_SET2IE));
}

/**
  * @brief  Enable the output 2 reset interrupt for a given timer.
  * @rmtoll TIMxDIER     RST2IE            LL_HRTIM_EnableIT_RST2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_RST2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMDIER_RST2IE);
}

/**
  * @brief  Disable the output 2 reset interrupt for a given timer.
  * @rmtoll TIMxDIER     RST2IE            LL_HRTIM_DisableIT_RST2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_RST2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMDIER_RST2IE);
}

/**
  * @brief  Indicate whether the output 2 reset LL_HRTIM_IsEnabledIT_RST2 is enabled for a given timer.
  * @rmtoll TIMxDIER     RST2IE            LL_HRTIM_DisableIT_RST2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of RST2xIE bit in HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_RST2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMDIER_RST2IE) == (HRTIM_TIMDIER_RST2IE));
}

/**
  * @brief  Enable the reset/roll-over interrupt for a given timer.
  * @rmtoll TIMxDIER     RSTIE            LL_HRTIM_EnableIT_RST
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_RST(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMDIER_RSTIE);
}

/**
  * @brief  Disable the reset/roll-over interrupt for a given timer.
  * @rmtoll TIMxDIER     RSTIE            LL_HRTIM_DisableIT_RST
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_RST(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMDIER_RSTIE);
}

/**
  * @brief  Indicate whether the reset/roll-over interrupt is enabled for a given timer.
  * @rmtoll TIMxDIER     RSTIE            LL_HRTIM_IsEnabledIT_RST
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of RSTIE bit in HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_RST(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMDIER_RSTIE) == (HRTIM_TIMDIER_RSTIE));
}

/**
  * @brief  Enable the delayed protection interrupt for a given timer.
  * @rmtoll TIMxDIER     DLYPRTIE            LL_HRTIM_EnableIT_DLYPRT
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableIT_DLYPRT(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMDIER_DLYPRTIE);
}

/**
  * @brief  Disable the delayed protection interrupt for a given timer.
  * @rmtoll TIMxDIER     DLYPRTIE            LL_HRTIM_DisableIT_DLYPRT
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableIT_DLYPRT(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMDIER_DLYPRTIE);
}

/**
  * @brief  Indicate whether the delayed protection interrupt is enabled for a given timer.
  * @rmtoll TIMxDIER     DLYPRTIE            LL_HRTIM_IsEnabledIT_DLYPRT
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of DLYPRTIE bit in HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledIT_DLYPRT(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMDIER_DLYPRTIE) == (HRTIM_TIMDIER_DLYPRTIE));
}

/**
  * @}
  */

/** @defgroup HRTIM_EF_DMA_Management DMA_Management
  * @{
  */

/**
  * @brief  Enable the synchronization input DMA request.
  * @rmtoll MDIER     SYNCDE            LL_HRTIM_EnableDMAReq_SYNC
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableDMAReq_SYNC(HRTIM_TypeDef *HRTIMx)
{
  SET_BIT(HRTIMx->sMasterRegs.MDIER, HRTIM_MDIER_SYNCDE);
}

/**
  * @brief  Disable the synchronization input DMA request
  * @rmtoll MDIER     SYNCDE            LL_HRTIM_DisableDMAReq_SYNC
  * @param  HRTIMx High Resolution Timer instance
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableDMAReq_SYNC(HRTIM_TypeDef *HRTIMx)
{
  CLEAR_BIT(HRTIMx->sMasterRegs.MDIER, HRTIM_MDIER_SYNCDE);
}

/**
  * @brief  Indicate whether the synchronization input DMA request is enabled.
  * @rmtoll MDIER     SYNCDE            LL_HRTIM_IsEnabledDMAReq_SYNC
  * @param  HRTIMx High Resolution Timer instance
  * @retval State of SYNCDE bit in HRTIM_MDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledDMAReq_SYNC(HRTIM_TypeDef *HRTIMx)
{
  return (READ_BIT(HRTIMx->sMasterRegs.MDIER, HRTIM_MDIER_SYNCDE) == (HRTIM_MDIER_SYNCDE));
}

/**
  * @brief  Enable the update DMA request for a given timer.
  * @rmtoll MDIER        MUPDDE            LL_HRTIM_EnableDMAReq_UPDATE\n
  *         TIMxDIER     UPDDE             LL_HRTIM_EnableDMAReq_UPDATE
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableDMAReq_UPDATE(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MDIER_MUPDDE);
}

/**
  * @brief  Disable the update DMA request for a given timer.
  * @rmtoll MDIER        MUPDDE            LL_HRTIM_DisableDMAReq_UPDATE\n
  *         TIMxDIER     UPDDE             LL_HRTIM_DisableDMAReq_UPDATE
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableDMAReq_UPDATE(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_MDIER_MUPDDE);
}

/**
  * @brief  Indicate whether the update DMA request is enabled for a given timer.
  * @rmtoll MDIER        MUPDDE            LL_HRTIM_IsEnabledDMAReq_UPDATE\n
  *         TIMxDIER     UPDDE             LL_HRTIM_IsEnabledDMAReq_UPDATE
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MUPDDE/UPDDE bit in HRTIM_MDIER/HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledDMAReq_UPDATE(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MDIER_MUPDDE) == (HRTIM_MDIER_MUPDDE));
}

/**
  * @brief  Enable the repetition DMA request for a given timer.
  * @rmtoll MDIER        MREPDE            LL_HRTIM_EnableDMAReq_REP\n
  *         TIMxDIER     REPDE             LL_HRTIM_EnableDMAReq_REP
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableDMAReq_REP(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MDIER_MREPDE);
}

/**
  * @brief  Disable the repetition DMA request for a given timer.
  * @rmtoll MDIER        MREPDE            LL_HRTIM_DisableDMAReq_REP\n
  *         TIMxDIER     REPDE             LL_HRTIM_DisableDMAReq_REP
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableDMAReq_REP(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_MDIER_MREPDE);
}

/**
  * @brief  Indicate whether the repetition DMA request is enabled for a given timer.
  * @rmtoll MDIER        MREPDE            LL_HRTIM_IsEnabledDMAReq_REP\n
  *         TIMxDIER     REPDE             LL_HRTIM_IsEnabledDMAReq_REP
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MREPDE/REPDE bit in HRTIM_MDIER/HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledDMAReq_REP(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MDIER_MREPDE) == (HRTIM_MDIER_MREPDE));
}

/**
  * @brief  Enable the compare 1 DMA request for a given timer.
  * @rmtoll MDIER        MCMP1DE            LL_HRTIM_EnableDMAReq_CMP1\n
  *         TIMxDIER     CMP1DE             LL_HRTIM_EnableDMAReq_CMP1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableDMAReq_CMP1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MDIER_MCMP1DE);
}

/**
  * @brief  Disable the compare 1 DMA request for a given timer.
  * @rmtoll MDIER        MCMP1DE            LL_HRTIM_DisableDMAReq_CMP1\n
  *         TIMxDIER     CMP1DE             LL_HRTIM_DisableDMAReq_CMP1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableDMAReq_CMP1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_MDIER_MCMP1DE);
}

/**
  * @brief  Indicate whether the compare 1 DMA request is enabled for a given timer.
  * @rmtoll MDIER        MCMP1DE            LL_HRTIM_IsEnabledDMAReq_CMP1\n
  *         TIMxDIER     CMP1DE             LL_HRTIM_IsEnabledDMAReq_CMP1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MCMP1DE/CMP1DE bit in HRTIM_MDIER/HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledDMAReq_CMP1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MDIER_MCMP1DE) == (HRTIM_MDIER_MCMP1DE));
}

/**
  * @brief  Enable the compare 2 DMA request for a given timer.
  * @rmtoll MDIER        MCMP2DE            LL_HRTIM_EnableDMAReq_CMP2\n
  *         TIMxDIER     CMP2DE             LL_HRTIM_EnableDMAReq_CMP2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableDMAReq_CMP2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MDIER_MCMP2DE);
}

/**
  * @brief  Disable the compare 2 DMA request for a given timer.
  * @rmtoll MDIER        MCMP2DE            LL_HRTIM_DisableDMAReq_CMP2\n
  *         TIMxDIER     CMP2DE             LL_HRTIM_DisableDMAReq_CMP2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableDMAReq_CMP2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_MDIER_MCMP2DE);
}

/**
  * @brief  Indicate whether the compare 2 DMA request is enabled for a given timer.
  * @rmtoll MDIER        MCMP2DE            LL_HRTIM_IsEnabledDMAReq_CMP2\n
  *         TIMxDIER     CMP2DE             LL_HRTIM_IsEnabledDMAReq_CMP2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MCMP2DE/CMP2DE bit in HRTIM_MDIER/HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledDMAReq_CMP2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MDIER_MCMP2DE) == (HRTIM_MDIER_MCMP2DE));
}

/**
  * @brief  Enable the compare 3 DMA request for a given timer.
  * @rmtoll MDIER        MCMP3DE            LL_HRTIM_EnableDMAReq_CMP3\n
  *         TIMxDIER     CMP3DE             LL_HRTIM_EnableDMAReq_CMP3
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableDMAReq_CMP3(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MDIER_MCMP3DE);
}

/**
  * @brief  Disable the compare 3 DMA request for a given timer.
  * @rmtoll MDIER        MCMP3DE            LL_HRTIM_DisableDMAReq_CMP3\n
  *         TIMxDIER     CMP3DE             LL_HRTIM_DisableDMAReq_CMP3
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableDMAReq_CMP3(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_MDIER_MCMP3DE);
}

/**
  * @brief  Indicate whether the compare 3 DMA request is enabled for a given timer.
  * @rmtoll MDIER        MCMP3DE            LL_HRTIM_IsEnabledDMAReq_CMP3\n
  *         TIMxDIER     CMP3DE             LL_HRTIM_IsEnabledDMAReq_CMP3
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MCMP3DE/CMP3DE bit in HRTIM_MDIER/HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledDMAReq_CMP3(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MDIER_MCMP3DE) == (HRTIM_MDIER_MCMP3DE));
}

/**
  * @brief  Enable the compare 4 DMA request for a given timer.
  * @rmtoll MDIER        MCMP4DE            LL_HRTIM_EnableDMAReq_CMP4\n
  *         TIMxDIER     CMP4DE             LL_HRTIM_EnableDMAReq_CMP4
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableDMAReq_CMP4(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_MDIER_MCMP4DE);
}

/**
  * @brief  Disable the compare 4 DMA request for a given timer.
  * @rmtoll MDIER        MCMP4DE            LL_HRTIM_DisableDMAReq_CMP4\n
  *         TIMxDIER     CMP4DE             LL_HRTIM_DisableDMAReq_CMP4
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableDMAReq_CMP4(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_MDIER_MCMP4DE);
}

/**
  * @brief  Indicate whether the compare 4 DMA request is enabled for a given timer.
  * @rmtoll MDIER        MCMP4DE            LL_HRTIM_IsEnabledDMAReq_CMP4\n
  *         TIMxDIER     CMP4DE             LL_HRTIM_IsEnabledDMAReq_CMP4
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_MASTER
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of MCMP4DE/CMP4DE bit in HRTIM_MDIER/HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledDMAReq_CMP4(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_MDIER_MCMP4DE) == (HRTIM_MDIER_MCMP4DE));
}

/**
  * @brief  Enable the capture 1 DMA request for a given timer.
  * @rmtoll TIMxDIER     CPT1DE             LL_HRTIM_EnableDMAReq_CPT1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableDMAReq_CPT1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMDIER_CPT1DE);
}

/**
  * @brief  Disable the capture 1 DMA request for a given timer.
  * @rmtoll TIMxDIER     CPT1DE             LL_HRTIM_DisableDMAReq_CPT1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableDMAReq_CPT1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMDIER_CPT1DE);
}

/**
  * @brief  Indicate whether the capture 1 DMA request is enabled for a given timer.
  * @rmtoll TIMxDIER     CPT1DE             LL_HRTIM_IsEnabledDMAReq_CPT1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of CPT1DE bit in HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledDMAReq_CPT1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMDIER_CPT1DE) == (HRTIM_TIMDIER_CPT1DE));
}

/**
  * @brief  Enable the capture 2 DMA request for a given timer.
  * @rmtoll TIMxDIER     CPT2DE             LL_HRTIM_EnableDMAReq_CPT2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableDMAReq_CPT2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMDIER_CPT2DE);
}

/**
  * @brief  Disable the capture 2 DMA request for a given timer.
  * @rmtoll TIMxDIER     CPT2DE             LL_HRTIM_DisableDMAReq_CPT2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableDMAReq_CPT2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMDIER_CPT2DE);
}

/**
  * @brief  Indicate whether the capture 2 DMA request is enabled for a given timer.
  * @rmtoll TIMxDIER     CPT2DE             LL_HRTIM_IsEnabledDMAReq_CPT2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of CPT2DE bit in HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledDMAReq_CPT2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMDIER_CPT2DE) == (HRTIM_TIMDIER_CPT2DE));
}

/**
  * @brief  Enable the output 1 set  DMA request for a given timer.
  * @rmtoll TIMxDIER     SET1DE             LL_HRTIM_EnableDMAReq_SET1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableDMAReq_SET1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMDIER_SET1DE);
}

/**
  * @brief  Disable the output 1 set  DMA request for a given timer.
  * @rmtoll TIMxDIER     SET1DE             LL_HRTIM_DisableDMAReq_SET1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableDMAReq_SET1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMDIER_SET1DE);
}

/**
  * @brief  Indicate whether the output 1 set  DMA request is enabled for a given timer.
  * @rmtoll TIMxDIER     SET1DE             LL_HRTIM_IsEnabledDMAReq_SET1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of SET1xDE bit in HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledDMAReq_SET1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMDIER_SET1DE) == (HRTIM_TIMDIER_SET1DE));
}

/**
  * @brief  Enable the output 1 reset  DMA request for a given timer.
  * @rmtoll TIMxDIER     RST1DE             LL_HRTIM_EnableDMAReq_RST1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableDMAReq_RST1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMDIER_RST1DE);
}

/**
  * @brief  Disable the output 1 reset  DMA request for a given timer.
  * @rmtoll TIMxDIER     RST1DE             LL_HRTIM_DisableDMAReq_RST1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableDMAReq_RST1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMDIER_RST1DE);
}

/**
  * @brief  Indicate whether the output 1 reset interrupt is enabled for a given timer.
  * @rmtoll TIMxDIER     RST1DE             LL_HRTIM_IsEnabledDMAReq_RST1
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of RST1xDE bit in HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledDMAReq_RST1(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMDIER_RST1DE) == (HRTIM_TIMDIER_RST1DE));
}

/**
  * @brief  Enable the output 2 set  DMA request for a given timer.
  * @rmtoll TIMxDIER     SET2DE             LL_HRTIM_EnableDMAReq_SET2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableDMAReq_SET2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMDIER_SET2DE);
}

/**
  * @brief  Disable the output 2 set  DMA request for a given timer.
  * @rmtoll TIMxDIER     SET2DE             LL_HRTIM_DisableDMAReq_SET2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableDMAReq_SET2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMDIER_SET2DE);
}

/**
  * @brief  Indicate whether the output 2 set  DMA request is enabled for a given timer.
  * @rmtoll TIMxDIER     SET2DE             LL_HRTIM_IsEnabledDMAReq_SET2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of SET2xDE bit in HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledDMAReq_SET2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMDIER_SET2DE) == (HRTIM_TIMDIER_SET2DE));
}

/**
  * @brief  Enable the output 2 reset  DMA request for a given timer.
  * @rmtoll TIMxDIER     RST2DE             LL_HRTIM_EnableDMAReq_RST2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableDMAReq_RST2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMDIER_RST2DE);
}

/**
  * @brief  Disable the output 2 reset  DMA request for a given timer.
  * @rmtoll TIMxDIER     RST2DE             LL_HRTIM_DisableDMAReq_RST2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableDMAReq_RST2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMDIER_RST2DE);
}

/**
  * @brief  Indicate whether the output 2 reset  DMA request is enabled for a given timer.
  * @rmtoll TIMxDIER     RST2DE             LL_HRTIM_IsEnabledDMAReq_RST2
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of RST2xDE bit in HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledDMAReq_RST2(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMDIER_RST2DE) == (HRTIM_TIMDIER_RST2DE));
}

/**
  * @brief  Enable the reset/roll-over DMA request for a given timer.
  * @rmtoll TIMxDIER     RSTDE             LL_HRTIM_EnableDMAReq_RST
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableDMAReq_RST(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMDIER_RSTDE);
}

/**
  * @brief  Disable the reset/roll-over DMA request for a given timer.
  * @rmtoll TIMxDIER     RSTDE             LL_HRTIM_DisableDMAReq_RST
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableDMAReq_RST(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMDIER_RSTDE);
}

/**
  * @brief  Indicate whether the reset/roll-over DMA request is enabled for a given timer.
  * @rmtoll TIMxDIER     RSTDE             LL_HRTIM_IsEnabledDMAReq_RST
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of RSTDE bit in HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledDMAReq_RST(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMDIER_RSTDE) == (HRTIM_TIMDIER_RSTDE));
}

/**
  * @brief  Enable the delayed protection DMA request for a given timer.
  * @rmtoll TIMxDIER     DLYPRTDE             LL_HRTIM_EnableDMAReq_DLYPRT
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_EnableDMAReq_DLYPRT(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  SET_BIT(*pReg, HRTIM_TIMDIER_DLYPRTDE);
}

/**
  * @brief  Disable the delayed protection DMA request for a given timer.
  * @rmtoll TIMxDIER     DLYPRTDE             LL_HRTIM_DisableDMAReq_DLYPRT
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval None
  */
__STATIC_INLINE void LL_HRTIM_DisableDMAReq_DLYPRT(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  CLEAR_BIT(*pReg, HRTIM_TIMDIER_DLYPRTDE);
}

/**
  * @brief  Indicate whether the delayed protection DMA request is enabled for a given timer.
  * @rmtoll TIMxDIER     DLYPRTDE             LL_HRTIM_IsEnabledDMAReq_DLYPRT
  * @param  HRTIMx High Resolution Timer instance
  * @param  Timer This parameter can be one of the following values:
  *         @arg @ref LL_HRTIM_TIMER_A
  *         @arg @ref LL_HRTIM_TIMER_B
  *         @arg @ref LL_HRTIM_TIMER_C
  *         @arg @ref LL_HRTIM_TIMER_D
  *         @arg @ref LL_HRTIM_TIMER_E
  * @retval State of DLYPRTDE bit in HRTIM_TIMxDIER register (1 or 0).
  */
__STATIC_INLINE uint32_t LL_HRTIM_IsEnabledDMAReq_DLYPRT(HRTIM_TypeDef *HRTIMx, uint32_t Timer)
{
  register uint32_t iTimer = (uint8_t)(POSITION_VAL(Timer) - HRTIM_MCR_MCEN_Pos);
  register uint32_t *pReg = (uint32_t *)((uint32_t)((uint32_t)(&HRTIMx->sMasterRegs.MDIER) +
                                                    REG_OFFSET_TAB_TIMER[iTimer]));
  return (READ_BIT(*pReg, HRTIM_TIMDIER_DLYPRTDE) == (HRTIM_TIMDIER_DLYPRTDE));
}

/**
  * @}
  */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup HRTIM_LL_EF_Init Initialisation and deinitialisation functions
  * @{
  */
ErrorStatus LL_HRTIM_DeInit(HRTIM_TypeDef* HRTIMx);
/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/**
  * @}
  */

/**
  * @}
  */

#endif /* HRTIM1 */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F3xx_LL_HRTIM_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
