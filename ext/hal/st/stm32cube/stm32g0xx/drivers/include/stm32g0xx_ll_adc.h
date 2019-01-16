/**
  ******************************************************************************
  * @file    stm32g0xx_ll_adc.h
  * @author  MCD Application Team
  * @brief   Header file of ADC LL module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32G0xx_LL_ADC_H
#define STM32G0xx_LL_ADC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx.h"

/** @addtogroup STM32G0xx_LL_Driver
  * @{
  */

#if defined (ADC1)

/** @defgroup ADC_LL ADC
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/
/** @defgroup ADC_LL_Private_Constants ADC Private Constants
  * @{
  */

/* Internal mask for ADC group regular sequencer:                             */
/* To select into literal LL_ADC_REG_RANK_x the relevant bits for:            */
/* - sequencer rank bits position into the selected register                  */

#define ADC_REG_RANK_ID_SQRX_MASK          (ADC_CHANNEL_ID_NUMBER_MASK_POSBIT0)

/* Definition of ADC group regular sequencer bits information to be inserted  */
/* into ADC group regular sequencer ranks literals definition.                */
#define ADC_REG_RANK_1_SQRX_BITOFFSET_POS  ( 0UL) /* Value equivalent to bitfield "ADC_CHSELR_SQ1" position in register */
#define ADC_REG_RANK_2_SQRX_BITOFFSET_POS  ( 4UL) /* Value equivalent to bitfield "ADC_CHSELR_SQ2" position in register */
#define ADC_REG_RANK_3_SQRX_BITOFFSET_POS  ( 8UL) /* Value equivalent to bitfield "ADC_CHSELR_SQ3" position in register */
#define ADC_REG_RANK_4_SQRX_BITOFFSET_POS  (12UL) /* Value equivalent to bitfield "ADC_CHSELR_SQ4" position in register */
#define ADC_REG_RANK_5_SQRX_BITOFFSET_POS  (16UL) /* Value equivalent to bitfield "ADC_CHSELR_SQ5" position in register */
#define ADC_REG_RANK_6_SQRX_BITOFFSET_POS  (20UL) /* Value equivalent to bitfield "ADC_CHSELR_SQ6" position in register */
#define ADC_REG_RANK_7_SQRX_BITOFFSET_POS  (24UL) /* Value equivalent to bitfield "ADC_CHSELR_SQ7" position in register */
#define ADC_REG_RANK_8_SQRX_BITOFFSET_POS  (28UL) /* Value equivalent to bitfield "ADC_CHSELR_SQ8" position in register */



/* Internal mask for ADC group regular trigger:                               */
/* To select into literal LL_ADC_REG_TRIG_x the relevant bits for:            */
/* - regular trigger source                                                   */
/* - regular trigger edge                                                     */
#define ADC_REG_TRIG_EXT_EDGE_DEFAULT       (ADC_CFGR1_EXTEN_0) /* Trigger edge set to rising edge (default setting for compatibility with some ADC on other STM32 families having this setting set by HW default value) */

/* Mask containing trigger source masks for each of possible                  */
/* trigger edge selection duplicated with shifts [0; 4; 8; 12]                */
/* corresponding to {SW start; ext trigger; ext trigger; ext trigger}.        */
#define ADC_REG_TRIG_SOURCE_MASK            (((LL_ADC_REG_TRIG_SOFTWARE & ADC_CFGR1_EXTSEL) << (4U * 0UL)) | \
                                             ((ADC_CFGR1_EXTSEL)                            << (4U * 1UL)) | \
                                             ((ADC_CFGR1_EXTSEL)                            << (4U * 2UL)) | \
                                             ((ADC_CFGR1_EXTSEL)                            << (4U * 3UL))  )

/* Mask containing trigger edge masks for each of possible                    */
/* trigger edge selection duplicated with shifts [0; 4; 8; 12]                */
/* corresponding to {SW start; ext trigger; ext trigger; ext trigger}.        */
#define ADC_REG_TRIG_EDGE_MASK              (((LL_ADC_REG_TRIG_SOFTWARE & ADC_CFGR1_EXTEN) << (4U * 0UL)) | \
                                             ((ADC_REG_TRIG_EXT_EDGE_DEFAULT)              << (4U * 1UL)) | \
                                             ((ADC_REG_TRIG_EXT_EDGE_DEFAULT)              << (4U * 2UL)) | \
                                             ((ADC_REG_TRIG_EXT_EDGE_DEFAULT)              << (4U * 3UL))  )

/* Definition of ADC group regular trigger bits information.                  */
#define ADC_REG_TRIG_EXTSEL_BITOFFSET_POS  ( 6UL) /* Value equivalent to bitfield "ADC_CFGR1_EXTSEL" position in register */
#define ADC_REG_TRIG_EXTEN_BITOFFSET_POS   (10UL) /* Value equivalent to bitfield "ADC_CFGR1_EXTEN" position in register */



/* Internal mask for ADC channel:                                             */
/* To select into literal LL_ADC_CHANNEL_x the relevant bits for:             */
/* - channel identifier defined by number                                     */
/* - channel identifier defined by bitfield                                   */
/* - channel differentiation between external channels (connected to          */
/*   GPIO pins) and internal channels (connected to internal paths)           */
#define ADC_CHANNEL_ID_NUMBER_MASK         (ADC_CFGR1_AWD1CH)
#define ADC_CHANNEL_ID_BITFIELD_MASK       (ADC_CHSELR_CHSEL)
#define ADC_CHANNEL_ID_NUMBER_MASK_SEQ     (ADC_CHSELR_SQ1 << ADC_CHANNEL_ID_NUMBER_BITOFFSET_POS) /* Value equivalent to ADC_CHANNEL_ID_NUMBER_MASK with reduced range: on this STM32 serie, ADC group regular sequencer, if set to mode "fully configurable", can contain channels with a restricted channel number. Refer to function @ref LL_ADC_REG_SetSequencerConfigurable(). */
#define ADC_CHANNEL_ID_NUMBER_BITOFFSET_POS (26UL)/* Value equivalent to bitfield "ADC_CHANNEL_ID_NUMBER_MASK" position in register */
#define ADC_CHANNEL_ID_MASK                (ADC_CHANNEL_ID_NUMBER_MASK | ADC_CHANNEL_ID_BITFIELD_MASK | ADC_CHANNEL_ID_INTERNAL_CH_MASK)
/* Equivalent mask of ADC_CHANNEL_NUMBER_MASK aligned on register LSB (bit 0) */
#define ADC_CHANNEL_ID_NUMBER_MASK_POSBIT0 (0x0000001FUL) /* Equivalent to shift: (ADC_CHANNEL_NUMBER_MASK >> [Position of bitfield "ADC_CHANNEL_NUMBER_MASK" in register]) */

/* Channel differentiation between external and internal channels */
#define ADC_CHANNEL_ID_INTERNAL_CH         (0x80000000UL) /* Marker of internal channel */
#define ADC_CHANNEL_ID_INTERNAL_CH_MASK    (ADC_CHANNEL_ID_INTERNAL_CH)

/* Definition of channels ID number information to be inserted into           */
/* channels literals definition.                                              */
#define ADC_CHANNEL_0_NUMBER               (0x00000000UL)
#define ADC_CHANNEL_1_NUMBER               (                                                                                    ADC_CFGR1_AWD1CH_0)
#define ADC_CHANNEL_2_NUMBER               (                                                               ADC_CFGR1_AWD1CH_1                     )
#define ADC_CHANNEL_3_NUMBER               (                                                               ADC_CFGR1_AWD1CH_1 | ADC_CFGR1_AWD1CH_0)
#define ADC_CHANNEL_4_NUMBER               (                                          ADC_CFGR1_AWD1CH_2                                          )
#define ADC_CHANNEL_5_NUMBER               (                                          ADC_CFGR1_AWD1CH_2                      | ADC_CFGR1_AWD1CH_0)
#define ADC_CHANNEL_6_NUMBER               (                                          ADC_CFGR1_AWD1CH_2 | ADC_CFGR1_AWD1CH_1                     )
#define ADC_CHANNEL_7_NUMBER               (                                          ADC_CFGR1_AWD1CH_2 | ADC_CFGR1_AWD1CH_1 | ADC_CFGR1_AWD1CH_0)
#define ADC_CHANNEL_8_NUMBER               (                     ADC_CFGR1_AWD1CH_3                                                               )
#define ADC_CHANNEL_9_NUMBER               (                     ADC_CFGR1_AWD1CH_3                                           | ADC_CFGR1_AWD1CH_0)
#define ADC_CHANNEL_10_NUMBER              (                     ADC_CFGR1_AWD1CH_3                      | ADC_CFGR1_AWD1CH_1                     )
#define ADC_CHANNEL_11_NUMBER              (                     ADC_CFGR1_AWD1CH_3                      | ADC_CFGR1_AWD1CH_1 | ADC_CFGR1_AWD1CH_0)
#define ADC_CHANNEL_12_NUMBER              (                     ADC_CFGR1_AWD1CH_3 | ADC_CFGR1_AWD1CH_2                                          )
#define ADC_CHANNEL_13_NUMBER              (                     ADC_CFGR1_AWD1CH_3 | ADC_CFGR1_AWD1CH_2                      | ADC_CFGR1_AWD1CH_0)
#define ADC_CHANNEL_14_NUMBER              (                     ADC_CFGR1_AWD1CH_3 | ADC_CFGR1_AWD1CH_2 | ADC_CFGR1_AWD1CH_1                     )
#define ADC_CHANNEL_15_NUMBER              (                     ADC_CFGR1_AWD1CH_3 | ADC_CFGR1_AWD1CH_2 | ADC_CFGR1_AWD1CH_1 | ADC_CFGR1_AWD1CH_0)
#define ADC_CHANNEL_16_NUMBER              (ADC_CFGR1_AWD1CH_4                                                                                    )
#define ADC_CHANNEL_17_NUMBER              (ADC_CFGR1_AWD1CH_4                                                                | ADC_CFGR1_AWD1CH_0)
#define ADC_CHANNEL_18_NUMBER              (ADC_CFGR1_AWD1CH_4                                           | ADC_CFGR1_AWD1CH_1                     )

/* Definition of channels ID bitfield information to be inserted into         */
/* channels literals definition.                                              */
#define ADC_CHANNEL_0_BITFIELD             (ADC_CHSELR_CHSEL0)
#define ADC_CHANNEL_1_BITFIELD             (ADC_CHSELR_CHSEL1)
#define ADC_CHANNEL_2_BITFIELD             (ADC_CHSELR_CHSEL2)
#define ADC_CHANNEL_3_BITFIELD             (ADC_CHSELR_CHSEL3)
#define ADC_CHANNEL_4_BITFIELD             (ADC_CHSELR_CHSEL4)
#define ADC_CHANNEL_5_BITFIELD             (ADC_CHSELR_CHSEL5)
#define ADC_CHANNEL_6_BITFIELD             (ADC_CHSELR_CHSEL6)
#define ADC_CHANNEL_7_BITFIELD             (ADC_CHSELR_CHSEL7)
#define ADC_CHANNEL_8_BITFIELD             (ADC_CHSELR_CHSEL8)
#define ADC_CHANNEL_9_BITFIELD             (ADC_CHSELR_CHSEL9)
#define ADC_CHANNEL_10_BITFIELD            (ADC_CHSELR_CHSEL10)
#define ADC_CHANNEL_11_BITFIELD            (ADC_CHSELR_CHSEL11)
#define ADC_CHANNEL_12_BITFIELD            (ADC_CHSELR_CHSEL12)
#define ADC_CHANNEL_13_BITFIELD            (ADC_CHSELR_CHSEL13)
#define ADC_CHANNEL_14_BITFIELD            (ADC_CHSELR_CHSEL14)
#define ADC_CHANNEL_15_BITFIELD            (ADC_CHSELR_CHSEL15)
#define ADC_CHANNEL_16_BITFIELD            (ADC_CHSELR_CHSEL16)
#define ADC_CHANNEL_17_BITFIELD            (ADC_CHSELR_CHSEL17)
#define ADC_CHANNEL_18_BITFIELD            (ADC_CHSELR_CHSEL18)

/* Internal mask for ADC channel sampling time:                               */
/* To select into literals LL_ADC_SAMPLINGTIME_x                              */
/* the relevant bits for:                                                     */
/* (concatenation of multiple bits used in register SMPR)                     */
/* - ADC channels sampling time: setting channel wise, to map each channel    */
/*   on one of the common sampling time available.                            */
/* - ADC channels common sampling time: set a sampling time into one of the   */
/*   common sampling time available.                                          */
#define ADC_SAMPLING_TIME_CH_MASK          (ADC_CHANNEL_ID_BITFIELD_MASK << ADC_SMPR_SMPSEL0_BITOFFSET_POS)
#define ADC_SAMPLING_TIME_SMP_MASK         (ADC_SMPR_SMP2 | ADC_SMPR_SMP1)
#define ADC_SAMPLING_TIME_SMP_SHIFT_MASK   (ADC_SMPR_SMP2_BITOFFSET_POS | ADC_SMPR_SMP1_BITOFFSET_POS)

/* Internal mask for ADC analog watchdog:                                     */
/* To select into literals LL_ADC_AWD_CHANNELx_xxx the relevant bits for:     */
/* (concatenation of multiple bits used in different analog watchdogs,        */
/* (feature of several watchdogs not available on all STM32 families)).       */
/* - analog watchdog 1: monitored channel defined by number,                  */
/*   selection of ADC group (ADC group regular).                              */
/* - analog watchdog 2 and 3: monitored channel defined by bitfield, no       */
/*   selection on groups.                                                     */

/* Internal register offset for ADC analog watchdog channel configuration */
#define ADC_AWD_CR1_REGOFFSET              (0x00000000UL)
#define ADC_AWD_CR2_REGOFFSET              (0x00100000UL)
#define ADC_AWD_CR3_REGOFFSET              (0x00200000UL)

/* Register offset gap between AWD1 and AWD2-AWD3 configuration registers */
/* (Set separately as ADC_AWD_CRX_REGOFFSET to spare 32 bits space */
#define ADC_AWD_CR12_REGOFFSETGAP_MASK     (ADC_AWD2CR_AWD2CH_0)
#define ADC_AWD_CR12_REGOFFSETGAP_VAL      (0x00000024UL)

#define ADC_AWD_CRX_REGOFFSET_MASK         (ADC_AWD_CR1_REGOFFSET | ADC_AWD_CR2_REGOFFSET | ADC_AWD_CR3_REGOFFSET)
#define ADC_AWD_CRX_REGOFFSET_BITOFFSET_POS (20UL)

#define ADC_AWD_CR1_CHANNEL_MASK           (ADC_CFGR1_AWD1CH | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL)
#define ADC_AWD_CR23_CHANNEL_MASK          (ADC_AWD2CR_AWD2CH)
#define ADC_AWD_CR_ALL_CHANNEL_MASK        (ADC_AWD_CR1_CHANNEL_MASK | ADC_AWD_CR23_CHANNEL_MASK)

#define ADC_AWD_CRX_REGOFFSET_POS          (20UL) /* Position of bits ADC_AWD_CRx_REGOFFSET in ADC_AWD_CRX_REGOFFSET_MASK */

/* Internal register offset for ADC analog watchdog threshold configuration */
#define ADC_AWD_TR1_REGOFFSET              (ADC_AWD_CR1_REGOFFSET)
#define ADC_AWD_TR2_REGOFFSET              (ADC_AWD_CR2_REGOFFSET)
#define ADC_AWD_TR3_REGOFFSET              (ADC_AWD_CR3_REGOFFSET + (1UL << ADC_AWD_CRX_REGOFFSET_BITOFFSET_POS))
#define ADC_AWD_TRX_REGOFFSET_MASK         (ADC_AWD_TR1_REGOFFSET | ADC_AWD_TR2_REGOFFSET | ADC_AWD_TR3_REGOFFSET)
#define ADC_AWD_TRX_REGOFFSET_POS          (ADC_AWD_CRX_REGOFFSET_POS)     /* Position of bits ADC_SQRx_REGOFFSET in ADC_AWD_TRX_REGOFFSET_MASK */
#define ADC_AWD_TRX_BIT_HIGH_MASK          (0x00010000UL)                   /* Selection of 1 bit to discriminate threshold high: mask of bit */
#define ADC_AWD_TRX_BIT_HIGH_POS           (16UL)                           /* Selection of 1 bit to discriminate threshold high: position of bit */
#define ADC_AWD_TRX_BIT_HIGH_SHIFT4        (ADC_AWD_TRX_BIT_HIGH_POS - 4UL) /* Shift of bit ADC_AWD_TRX_BIT_HIGH to position to perform a shift of 4 ranks */
#define ADC_AWD_TRX_REGOFFSET_BITOFFSET_POS (20UL)



/* ADC registers bits positions */
#define ADC_CFGR1_RES_BITOFFSET_POS        ( 3UL) /* Value equivalent to bitfield "ADC_CFGR1_RES" position in register */
#define ADC_CFGR1_AWDSGL_BITOFFSET_POS     (22UL) /* Value equivalent to bitfield "ADC_CFGR1_AWDSGL" position in register */
#define ADC_TR1_HT1_BITOFFSET_POS          (16UL) /* Value equivalent to bitfield "ADC_TR1_HT1" position in register */
#define ADC_CHSELR_CHSEL0_BITOFFSET_POS    ( 0UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL0" position in register */
#define ADC_CHSELR_CHSEL1_BITOFFSET_POS    ( 1UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL1" position in register */
#define ADC_CHSELR_CHSEL2_BITOFFSET_POS    ( 2UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL2" position in register */
#define ADC_CHSELR_CHSEL3_BITOFFSET_POS    ( 3UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL3" position in register */
#define ADC_CHSELR_CHSEL4_BITOFFSET_POS    ( 4UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL4" position in register */
#define ADC_CHSELR_CHSEL5_BITOFFSET_POS    ( 5UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL5" position in register */
#define ADC_CHSELR_CHSEL6_BITOFFSET_POS    ( 6UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL6" position in register */
#define ADC_CHSELR_CHSEL7_BITOFFSET_POS    ( 7UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL7" position in register */
#define ADC_CHSELR_CHSEL8_BITOFFSET_POS    ( 8UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL8" position in register */
#define ADC_CHSELR_CHSEL9_BITOFFSET_POS    ( 9UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL9" position in register */
#define ADC_CHSELR_CHSEL10_BITOFFSET_POS   (10UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL10" position in register */
#define ADC_CHSELR_CHSEL11_BITOFFSET_POS   (11UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL11" position in register */
#define ADC_CHSELR_CHSEL12_BITOFFSET_POS   (12UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL12" position in register */
#define ADC_CHSELR_CHSEL13_BITOFFSET_POS   (13UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL13" position in register */
#define ADC_CHSELR_CHSEL14_BITOFFSET_POS   (14UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL14" position in register */
#define ADC_CHSELR_CHSEL15_BITOFFSET_POS   (15UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL15" position in register */
#define ADC_CHSELR_CHSEL16_BITOFFSET_POS   (16UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL16" position in register */
#define ADC_CHSELR_CHSEL17_BITOFFSET_POS   (17UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL17" position in register */
#define ADC_CHSELR_CHSEL18_BITOFFSET_POS   (18UL) /* Value equivalent to bitfield "ADC_CHSELR_CHSEL18" position in register */
#define ADC_SMPR_SMP1_BITOFFSET_POS        ( 0UL) /* Value equivalent to bitfield "ADC_SMPR_SMP1" position in register */
#define ADC_SMPR_SMP2_BITOFFSET_POS        ( 4UL) /* Value equivalent to bitfield "ADC_SMPR_SMP2" position in register */
#define ADC_SMPR_SMPSEL0_BITOFFSET_POS     ( 8UL) /* Value equivalent to bitfield "ADC_SMPR_SMPSEL0" position in register */


/* ADC registers bits groups */
#define ADC_CR_BITS_PROPERTY_RS            (ADC_CR_ADCAL | ADC_CR_ADSTP | ADC_CR_ADSTART | ADC_CR_ADDIS | ADC_CR_ADEN) /* ADC register CR bits with HW property "rs": Software can read as well as set this bit. Writing '0' has no effect on the bit value. */


/* ADC internal channels related definitions */
/* Internal voltage reference VrefInt */
#define VREFINT_CAL_ADDR                   ((uint16_t*) (0x1FFF75AAUL)) /* Internal voltage reference, address of parameter VREFINT_CAL: VrefInt ADC raw data acquired at temperature 30 DegC (tolerance: +-5 DegC), Vref+ = 3.3 V (tolerance: +-10 mV). */
#define VREFINT_CAL_VREF                   ( 3000UL)                    /* Analog voltage reference (Vref+) voltage with which VrefInt has been calibrated in production (tolerance: +-10 mV) (unit: mV). */
/* Temperature sensor */
#define TEMPSENSOR_CAL1_ADDR               ((uint16_t*) (0x1FFF75A8UL)) /* Internal temperature sensor, address of parameter TS_CAL1: On STM32G0, temperature sensor ADC raw data acquired at temperature  30 DegC (tolerance: +-5 DegC), Vref+ = 3.0 V (tolerance: +-10 mV). */
#define TEMPSENSOR_CAL2_ADDR               ((uint16_t*) (0x1FFF75CAUL)) /* Internal temperature sensor, address of parameter TS_CAL2: On STM32G0, temperature sensor ADC raw data acquired at temperature 130 DegC (tolerance: +-5 DegC), Vref+ = 3.0 V (tolerance: +-10 mV). */
#define TEMPSENSOR_CAL1_TEMP               (( int32_t)   30)            /* Internal temperature sensor, temperature at which temperature sensor has been calibrated in production for data into TEMPSENSOR_CAL1_ADDR (tolerance: +-5 DegC) (unit: DegC). */
#define TEMPSENSOR_CAL2_TEMP               (( int32_t)  130)            /* Internal temperature sensor, temperature at which temperature sensor has been calibrated in production for data into TEMPSENSOR_CAL2_ADDR (tolerance: +-5 DegC) (unit: DegC). */
#define TEMPSENSOR_CAL_VREFANALOG          ( 3000UL)                    /* Analog voltage reference (Vref+) voltage with which temperature sensor has been calibrated in production (tolerance: +-10 mV) (unit: mV). */


/**
  * @}
  */


/* Private macros ------------------------------------------------------------*/
/** @defgroup ADC_LL_Private_Macros ADC Private Macros
  * @{
  */

/**
  * @brief  Driver macro reserved for internal use: set a pointer to
  *         a register from a register basis from which an offset
  *         is applied.
  * @param  __REG__ Register basis from which the offset is applied.
  * @param  __REG_OFFFSET__ Offset to be applied (unit: number of registers).
  * @retval Pointer to register address
  */
#define __ADC_PTR_REG_OFFSET(__REG__, __REG_OFFFSET__)                         \
 ((uint32_t *)((uint32_t) ((uint32_t)(&(__REG__)) + ((__REG_OFFFSET__) << 2UL))))

/**
  * @}
  */


/* Exported types ------------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup ADC_LL_ES_INIT ADC Exported Init structure
  * @{
  */

/**
  * @brief  Structure definition of some features of ADC common parameters
  *         and multimode
  *         (all ADC instances belonging to the same ADC common instance).
  * @note   The setting of these parameters by function @ref LL_ADC_CommonInit()
  *         is conditioned to ADC instances state (all ADC instances
  *         sharing the same ADC common instance):
  *         All ADC instances sharing the same ADC common instance must be
  *         disabled.
  */
typedef struct
{
  uint32_t CommonClock;                 /*!< Set parameter common to several ADC: Clock source and prescaler.
                                             This parameter can be a value of @ref ADC_LL_EC_COMMON_CLOCK_SOURCE
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_ADC_SetCommonClock(). */

} LL_ADC_CommonInitTypeDef;

/**
  * @brief  Structure definition of some features of ADC instance.
  * @note   These parameters have an impact on ADC scope: ADC instance.
  *         Refer to corresponding unitary functions into
  *         @ref ADC_LL_EF_Configuration_ADC_Instance .
  * @note   The setting of these parameters by function @ref LL_ADC_Init()
  *         is conditioned to ADC state:
  *         ADC instance must be disabled.
  *         This condition is applied to all ADC features, for efficiency
  *         and compatibility over all STM32 families. However, the different
  *         features can be set under different ADC state conditions
  *         (setting possible with ADC enabled without conversion on going,
  *         ADC enabled with conversion on going, ...)
  *         Each feature can be updated afterwards with a unitary function
  *         and potentially with ADC in a different state than disabled,
  *         refer to description of each function for setting
  *         conditioned to ADC state.
  */
typedef struct
{
  uint32_t Clock;                       /*!< Set ADC instance clock source and prescaler.
                                             This parameter can be a value of @ref ADC_LL_EC_CLOCK_SOURCE
                                             @note On this STM32 serie, this parameter has some clock ratio constraints:
                                                   ADC clock synchronous (from PCLK) with prescaler 1 must be enabled only if PCLK has a 50% duty clock cycle
                                                   (APB prescaler configured inside the RCC must be bypassed and the system clock must by 50% duty cycle).
                                             
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_ADC_SetClock().
                                             For more details, refer to description of this function. */

  uint32_t Resolution;                  /*!< Set ADC resolution.
                                             This parameter can be a value of @ref ADC_LL_EC_RESOLUTION
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_ADC_SetResolution(). */

  uint32_t DataAlignment;               /*!< Set ADC conversion data alignment.
                                             This parameter can be a value of @ref ADC_LL_EC_DATA_ALIGN
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_ADC_SetDataAlignment(). */

  uint32_t LowPowerMode;                /*!< Set ADC low power mode.
                                             This parameter can be a value of @ref ADC_LL_EC_LP_MODE
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_ADC_SetLowPowerMode(). */

} LL_ADC_InitTypeDef;

/**
  * @brief  Structure definition of some features of ADC group regular.
  * @note   These parameters have an impact on ADC scope: ADC group regular.
  *         Refer to corresponding unitary functions into
  *         @ref ADC_LL_EF_Configuration_ADC_Group_Regular
  *         (functions with prefix "REG").
  * @note   The setting of these parameters by function @ref LL_ADC_REG_Init()
  *         is conditioned to ADC state:
  *         ADC instance must be disabled.
  *         This condition is applied to all ADC features, for efficiency
  *         and compatibility over all STM32 families. However, the different
  *         features can be set under different ADC state conditions
  *         (setting possible with ADC enabled without conversion on going,
  *         ADC enabled with conversion on going, ...)
  *         Each feature can be updated afterwards with a unitary function
  *         and potentially with ADC in a different state than disabled,
  *         refer to description of each function for setting
  *         conditioned to ADC state.
  */
typedef struct
{
  uint32_t TriggerSource;               /*!< Set ADC group regular conversion trigger source: internal (SW start) or from external IP (timer event, external interrupt line).
                                             This parameter can be a value of @ref ADC_LL_EC_REG_TRIGGER_SOURCE
                                             @note On this STM32 serie, setting trigger source to external trigger also set trigger polarity to rising edge
                                                   (default setting for compatibility with some ADC on other STM32 families having this setting set by HW default value).
                                                   In case of need to modify trigger edge, use function @ref LL_ADC_REG_SetTriggerEdge().
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_ADC_REG_SetTriggerSource(). */

  uint32_t SequencerLength;             /*!< Set ADC group regular sequencer length.
                                             This parameter can be a value of @ref ADC_LL_EC_REG_SEQ_SCAN_LENGTH
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_ADC_REG_SetSequencerLength(). */

  uint32_t SequencerDiscont;            /*!< Set ADC group regular sequencer discontinuous mode: sequence subdivided and scan conversions interrupted every selected number of ranks.
                                             This parameter can be a value of @ref ADC_LL_EC_REG_SEQ_DISCONT_MODE
                                             @note This parameter has an effect only if group regular sequencer is enabled
                                                   (depending on the sequencer mode: scan length of 2 ranks or more, or several ADC channels enabled in group regular sequencer. Refer to function @ref LL_ADC_REG_SetSequencerConfigurable() ).
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_ADC_REG_SetSequencerDiscont(). */

  uint32_t ContinuousMode;              /*!< Set ADC continuous conversion mode on ADC group regular, whether ADC conversions are performed in single mode (one conversion per trigger) or in continuous mode (after the first trigger, following conversions launched successively automatically).
                                             This parameter can be a value of @ref ADC_LL_EC_REG_CONTINUOUS_MODE
                                             Note: It is not possible to enable both ADC group regular continuous mode and discontinuous mode.
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_ADC_REG_SetContinuousMode(). */

  uint32_t DMATransfer;                 /*!< Set ADC group regular conversion data transfer: no transfer or transfer by DMA, and DMA requests mode.
                                             This parameter can be a value of @ref ADC_LL_EC_REG_DMA_TRANSFER
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_ADC_REG_SetDMATransfer(). */

  uint32_t Overrun;                     /*!< Set ADC group regular behavior in case of overrun:
                                             data preserved or overwritten.
                                             This parameter can be a value of @ref ADC_LL_EC_REG_OVR_DATA_BEHAVIOR
                                             
                                             This feature can be modified afterwards using unitary function @ref LL_ADC_REG_SetOverrun(). */

} LL_ADC_REG_InitTypeDef;

/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/* Exported constants --------------------------------------------------------*/
/** @defgroup ADC_LL_Exported_Constants ADC Exported Constants
  * @{
  */

/** @defgroup ADC_LL_EC_FLAG ADC flags
  * @brief    Flags defines which can be used with LL_ADC_ReadReg function
  * @{
  */
#define LL_ADC_FLAG_ADRDY                  ADC_ISR_ADRDY      /*!< ADC flag ADC instance ready */
#define LL_ADC_FLAG_CCRDY                  ADC_ISR_CCRDY      /*!< ADC flag ADC channel configuration ready */
#define LL_ADC_FLAG_EOC                    ADC_ISR_EOC        /*!< ADC flag ADC group regular end of unitary conversion */
#define LL_ADC_FLAG_EOS                    ADC_ISR_EOS        /*!< ADC flag ADC group regular end of sequence conversions */
#define LL_ADC_FLAG_OVR                    ADC_ISR_OVR        /*!< ADC flag ADC group regular overrun */
#define LL_ADC_FLAG_EOSMP                  ADC_ISR_EOSMP      /*!< ADC flag ADC group regular end of sampling phase */
#define LL_ADC_FLAG_AWD1                   ADC_ISR_AWD1       /*!< ADC flag ADC analog watchdog 1 */
#define LL_ADC_FLAG_AWD2                   ADC_ISR_AWD2       /*!< ADC flag ADC analog watchdog 2 */
#define LL_ADC_FLAG_AWD3                   ADC_ISR_AWD3       /*!< ADC flag ADC analog watchdog 3 */
#define LL_ADC_FLAG_EOCAL                  ADC_ISR_EOCAL      /*!< ADC flag end of calibration */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_IT ADC interruptions for configuration (interruption enable or disable)
  * @brief    IT defines which can be used with LL_ADC_ReadReg and  LL_ADC_WriteReg functions
  * @{
  */
#define LL_ADC_IT_ADRDY                    ADC_IER_ADRDYIE    /*!< ADC interruption ADC instance ready */
#define LL_ADC_IT_CCRDY                    ADC_IER_CCRDYIE    /*!< ADC interruption channel configuration ready */
#define LL_ADC_IT_EOC                      ADC_IER_EOCIE      /*!< ADC interruption ADC group regular end of unitary conversion */
#define LL_ADC_IT_EOS                      ADC_IER_EOSIE      /*!< ADC interruption ADC group regular end of sequence conversions */
#define LL_ADC_IT_OVR                      ADC_IER_OVRIE      /*!< ADC interruption ADC group regular overrun */
#define LL_ADC_IT_EOSMP                    ADC_IER_EOSMPIE    /*!< ADC interruption ADC group regular end of sampling phase */
#define LL_ADC_IT_AWD1                     ADC_IER_AWD1IE     /*!< ADC interruption ADC analog watchdog 1 */
#define LL_ADC_IT_AWD2                     ADC_IER_AWD2IE     /*!< ADC interruption ADC analog watchdog 2 */
#define LL_ADC_IT_AWD3                     ADC_IER_AWD3IE     /*!< ADC interruption ADC analog watchdog 3 */
#define LL_ADC_IT_EOCAL                    ADC_IER_EOCALIE    /*!< ADC interruption ADC end of calibration */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_REGISTERS  ADC registers compliant with specific purpose
  * @{
  */
/* List of ADC registers intended to be used (most commonly) with             */
/* DMA transfer.                                                              */
/* Refer to function @ref LL_ADC_DMA_GetRegAddr().                            */
#define LL_ADC_DMA_REG_REGULAR_DATA          (0x00000000UL) /* ADC group regular conversion data register (corresponding to register DR) to be used with ADC configured in independent mode. Without DMA transfer, register accessed by LL function @ref LL_ADC_REG_ReadConversionData32() and other functions @ref LL_ADC_REG_ReadConversionDatax() */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_COMMON_CLOCK_SOURCE  ADC common - Clock source
  * @{
  */
#define LL_ADC_CLOCK_ASYNC_DIV1            (0x00000000UL)                                        /*!< ADC asynchronous clock without prescaler */
#define LL_ADC_CLOCK_ASYNC_DIV2            (ADC_CCR_PRESC_0)                                     /*!< ADC asynchronous clock with prescaler division by 2. ADC common clock asynchonous prescaler is applied to each ADC instance if the corresponding ADC instance clock is set to clock source asynchronous (refer to function @ref LL_ADC_SetClock() ). */
#define LL_ADC_CLOCK_ASYNC_DIV4            (ADC_CCR_PRESC_1                  )                   /*!< ADC asynchronous clock with prescaler division by 4. ADC common clock asynchonous prescaler is applied to each ADC instance if the corresponding ADC instance clock is set to clock source asynchronous (refer to function @ref LL_ADC_SetClock() ). */
#define LL_ADC_CLOCK_ASYNC_DIV6            (ADC_CCR_PRESC_1 | ADC_CCR_PRESC_0)                   /*!< ADC asynchronous clock with prescaler division by 6. ADC common clock asynchonous prescaler is applied to each ADC instance if the corresponding ADC instance clock is set to clock source asynchronous (refer to function @ref LL_ADC_SetClock() ). */
#define LL_ADC_CLOCK_ASYNC_DIV8            (ADC_CCR_PRESC_2                                    ) /*!< ADC asynchronous clock with prescaler division by 8. ADC common clock asynchonous prescaler is applied to each ADC instance if the corresponding ADC instance clock is set to clock source asynchronous (refer to function @ref LL_ADC_SetClock() ). */
#define LL_ADC_CLOCK_ASYNC_DIV10           (ADC_CCR_PRESC_2                   | ADC_CCR_PRESC_0) /*!< ADC asynchronous clock with prescaler division by 10. ADC common clock asynchonous prescaler is applied to each ADC instance if the corresponding ADC instance clock is set to clock source asynchronous (refer to function @ref LL_ADC_SetClock() ). */
#define LL_ADC_CLOCK_ASYNC_DIV12           (ADC_CCR_PRESC_2 | ADC_CCR_PRESC_1                  ) /*!< ADC asynchronous clock with prescaler division by 12. ADC common clock asynchonous prescaler is applied to each ADC instance if the corresponding ADC instance clock is set to clock source asynchronous (refer to function @ref LL_ADC_SetClock() ). */
#define LL_ADC_CLOCK_ASYNC_DIV16           (ADC_CCR_PRESC_2 | ADC_CCR_PRESC_1 | ADC_CCR_PRESC_0) /*!< ADC asynchronous clock with prescaler division by 16. ADC common clock asynchonous prescaler is applied to each ADC instance if the corresponding ADC instance clock is set to clock source asynchronous (refer to function @ref LL_ADC_SetClock() ). */
#define LL_ADC_CLOCK_ASYNC_DIV32           (ADC_CCR_PRESC_3)                                     /*!< ADC asynchronous clock with prescaler division by 32. ADC common clock asynchonous prescaler is applied to each ADC instance if the corresponding ADC instance clock is set to clock source asynchronous (refer to function @ref LL_ADC_SetClock() ). */
#define LL_ADC_CLOCK_ASYNC_DIV64           (ADC_CCR_PRESC_3 | ADC_CCR_PRESC_0)                   /*!< ADC asynchronous clock with prescaler division by 64. ADC common clock asynchonous prescaler is applied to each ADC instance if the corresponding ADC instance clock is set to clock source asynchronous (refer to function @ref LL_ADC_SetClock() ). */
#define LL_ADC_CLOCK_ASYNC_DIV128          (ADC_CCR_PRESC_3 | ADC_CCR_PRESC_1)                   /*!< ADC asynchronous clock with prescaler division by 128. ADC common clock asynchonous prescaler is applied to each ADC instance if the corresponding ADC instance clock is set to clock source asynchronous (refer to function @ref LL_ADC_SetClock() ). */
#define LL_ADC_CLOCK_ASYNC_DIV256          (ADC_CCR_PRESC_3 | ADC_CCR_PRESC_1 | ADC_CCR_PRESC_0) /*!< ADC asynchronous clock with prescaler division by 256. ADC common clock asynchonous prescaler is applied to each ADC instance if the corresponding ADC instance clock is set to clock source asynchronous (refer to function @ref LL_ADC_SetClock() ). */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_COMMON_CLOCK_FREQ_MODE  ADC common - Clock frequency mode
  * @{
  */
#define LL_ADC_CLOCK_FREQ_MODE_HIGH        (0x00000000UL)         /*!< ADC clock mode to high frequency. On STM32G0, ADC clock frequency above 3.5MHz.  */
#define LL_ADC_CLOCK_FREQ_MODE_LOW         (ADC_CCR_LFMEN)        /*!< ADC clock mode to low frequency. On STM32G0, ADC clock frequency below 3.5MHz.  */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_COMMON_PATH_INTERNAL  ADC common - Measurement path to internal channels
  * @{
  */
/* Note: Other measurement paths to internal channels may be available        */
/*       (connections to other peripherals).                                  */
/*       If they are not listed below, they do not require any specific       */
/*       path enable. In this case, Access to measurement path is done        */
/*       only by selecting the corresponding ADC internal channel.            */
#define LL_ADC_PATH_INTERNAL_NONE          (0x00000000UL)         /*!< ADC measurement pathes all disabled */
#define LL_ADC_PATH_INTERNAL_VREFINT       (ADC_CCR_VREFEN)       /*!< ADC measurement path to internal channel VrefInt */
#define LL_ADC_PATH_INTERNAL_TEMPSENSOR    (ADC_CCR_TSEN)         /*!< ADC measurement path to internal channel temperature sensor */
#define LL_ADC_PATH_INTERNAL_VBAT          (ADC_CCR_VBATEN)       /*!< ADC measurement path to internal channel Vbat */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_CLOCK_SOURCE  ADC instance - Clock source
  * @{
  */
#define LL_ADC_CLOCK_SYNC_PCLK_DIV4        (ADC_CFGR2_CKMODE_1)                                  /*!< ADC synchronous clock derived from AHB clock divided by 4 */
#define LL_ADC_CLOCK_SYNC_PCLK_DIV2        (ADC_CFGR2_CKMODE_0)                                  /*!< ADC synchronous clock derived from AHB clock divided by 2 */
#define LL_ADC_CLOCK_SYNC_PCLK_DIV1        (ADC_CFGR2_CKMODE_1 | ADC_CFGR2_CKMODE_0)             /*!< ADC synchronous clock derived from AHB clock not divided  */
#define LL_ADC_CLOCK_ASYNC                 (0x00000000UL)                                        /*!< ADC asynchronous clock. Asynchronous clock prescaler can be configured using function @ref LL_ADC_SetCommonClock(). */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_RESOLUTION  ADC instance - Resolution
  * @{
  */
#define LL_ADC_RESOLUTION_12B              (0x00000000UL)                      /*!< ADC resolution 12 bits */
#define LL_ADC_RESOLUTION_10B              (                  ADC_CFGR1_RES_0) /*!< ADC resolution 10 bits */
#define LL_ADC_RESOLUTION_8B               (ADC_CFGR1_RES_1                  ) /*!< ADC resolution  8 bits */
#define LL_ADC_RESOLUTION_6B               (ADC_CFGR1_RES_1 | ADC_CFGR1_RES_0) /*!< ADC resolution  6 bits */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_DATA_ALIGN  ADC instance - Data alignment
  * @{
  */
#define LL_ADC_DATA_ALIGN_RIGHT            (0x00000000UL)         /*!< ADC conversion data alignment: right aligned (alignment on data register LSB bit 0)*/
#define LL_ADC_DATA_ALIGN_LEFT             (ADC_CFGR1_ALIGN)      /*!< ADC conversion data alignment: left aligned (aligment on data register MSB bit 15)*/
/**
  * @}
  */

/** @defgroup ADC_LL_EC_LP_MODE  ADC instance - Low power mode
  * @{
  */
#define LL_ADC_LP_MODE_NONE                (0x00000000UL)                      /*!< No ADC low power mode activated */
#define LL_ADC_LP_AUTOWAIT                 (ADC_CFGR1_WAIT)                    /*!< ADC low power mode auto delay: Dynamic low power mode, ADC conversions are performed only when necessary (when previous ADC conversion data is read). See description with function @ref LL_ADC_SetLowPowerMode(). */
#define LL_ADC_LP_AUTOPOWEROFF             (ADC_CFGR1_AUTOFF)                  /*!< ADC low power mode auto power-off: the ADC automatically powers-off after a ADC conversion and automatically wakes up when a new ADC conversion is triggered (with startup time between trigger and start of sampling). See description with function @ref LL_ADC_SetLowPowerMode(). */
#define LL_ADC_LP_AUTOWAIT_AUTOPOWEROFF    (ADC_CFGR1_WAIT | ADC_CFGR1_AUTOFF) /*!< ADC low power modes auto wait and auto power-off combined. See description with function @ref LL_ADC_SetLowPowerMode(). */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_REG_TRIGGER_FREQ  ADC group regular - Trigger frequency mode
  * @{
  */
#define LL_ADC_TRIGGER_FREQ_HIGH           (0x00000000UL)            /*!< ADC trigger frequency mode set to high frequency. Note: ADC trigger frequency mode must be set to low frequency when a duration is exceeded before ADC conversion start trigger event (between ADC enable and ADC conversion start trigger event or between two ADC conversion start trigger event). Duration value: Refer to device datasheet, parameter "tIdle". */
#define LL_ADC_TRIGGER_FREQ_LOW            (ADC_CFGR2_LFTRIG)        /*!< ADC trigger frequency mode set to low frequency. Note: ADC trigger frequency mode must be set to low frequency when a duration is exceeded before ADC conversion start trigger event (between ADC enable and ADC conversion start trigger event or between two ADC conversion start trigger event). Duration value: Refer to device datasheet, parameter "tIdle". */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_SAMPLINGTIME_COMMON  ADC instance - Sampling time common to a group of channels
* @{
*/
#define LL_ADC_SAMPLINGTIME_COMMON_1       (ADC_SMPR_SMP1_BITOFFSET_POS)                             /*!< Set sampling time common to a group of channels: sampling time nb 1 */
#define LL_ADC_SAMPLINGTIME_COMMON_2       (ADC_SMPR_SMP2_BITOFFSET_POS | ADC_SAMPLING_TIME_CH_MASK) /*!< Set sampling time common to a group of channels: sampling time nb 2 */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_GROUPS  ADC instance - Groups
  * @{
  */
#define LL_ADC_GROUP_REGULAR               (0x00000001UL) /*!< ADC group regular (available on all STM32 devices) */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_CHANNEL  ADC instance - Channel number
  * @{
  */
#define LL_ADC_CHANNEL_0                   (ADC_CHANNEL_0_NUMBER  | ADC_CHANNEL_0_BITFIELD ) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN0  */
#define LL_ADC_CHANNEL_1                   (ADC_CHANNEL_1_NUMBER  | ADC_CHANNEL_1_BITFIELD ) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN1  */
#define LL_ADC_CHANNEL_2                   (ADC_CHANNEL_2_NUMBER  | ADC_CHANNEL_2_BITFIELD ) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN2  */
#define LL_ADC_CHANNEL_3                   (ADC_CHANNEL_3_NUMBER  | ADC_CHANNEL_3_BITFIELD ) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN3  */
#define LL_ADC_CHANNEL_4                   (ADC_CHANNEL_4_NUMBER  | ADC_CHANNEL_4_BITFIELD ) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN4  */
#define LL_ADC_CHANNEL_5                   (ADC_CHANNEL_5_NUMBER  | ADC_CHANNEL_5_BITFIELD ) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN5  */
#define LL_ADC_CHANNEL_6                   (ADC_CHANNEL_6_NUMBER  | ADC_CHANNEL_6_BITFIELD ) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN6  */
#define LL_ADC_CHANNEL_7                   (ADC_CHANNEL_7_NUMBER  | ADC_CHANNEL_7_BITFIELD ) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN7  */
#define LL_ADC_CHANNEL_8                   (ADC_CHANNEL_8_NUMBER  | ADC_CHANNEL_8_BITFIELD ) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN8  */
#define LL_ADC_CHANNEL_9                   (ADC_CHANNEL_9_NUMBER  | ADC_CHANNEL_9_BITFIELD ) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN9  */
#define LL_ADC_CHANNEL_10                  (ADC_CHANNEL_10_NUMBER | ADC_CHANNEL_10_BITFIELD) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN10 */
#define LL_ADC_CHANNEL_11                  (ADC_CHANNEL_11_NUMBER | ADC_CHANNEL_11_BITFIELD) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN11 */
#define LL_ADC_CHANNEL_12                  (ADC_CHANNEL_12_NUMBER | ADC_CHANNEL_12_BITFIELD) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN12 */
#define LL_ADC_CHANNEL_13                  (ADC_CHANNEL_13_NUMBER | ADC_CHANNEL_13_BITFIELD) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN13 */
#define LL_ADC_CHANNEL_14                  (ADC_CHANNEL_14_NUMBER | ADC_CHANNEL_14_BITFIELD) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN14 */
#define LL_ADC_CHANNEL_15                  (ADC_CHANNEL_15_NUMBER | ADC_CHANNEL_15_BITFIELD) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN15 */
#define LL_ADC_CHANNEL_16                  (ADC_CHANNEL_16_NUMBER | ADC_CHANNEL_16_BITFIELD) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN16 */
#define LL_ADC_CHANNEL_17                  (ADC_CHANNEL_17_NUMBER | ADC_CHANNEL_17_BITFIELD) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN17 */
#define LL_ADC_CHANNEL_18                  (ADC_CHANNEL_18_NUMBER | ADC_CHANNEL_18_BITFIELD) /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN18 */
#define LL_ADC_CHANNEL_VREFINT             (LL_ADC_CHANNEL_13 | ADC_CHANNEL_ID_INTERNAL_CH)  /*!< ADC internal channel connected to VrefInt: Internal voltage reference. */
#define LL_ADC_CHANNEL_TEMPSENSOR          (LL_ADC_CHANNEL_12 | ADC_CHANNEL_ID_INTERNAL_CH)  /*!< ADC internal channel connected to Temperature sensor. */
#define LL_ADC_CHANNEL_VBAT                (LL_ADC_CHANNEL_14 | ADC_CHANNEL_ID_INTERNAL_CH)  /*!< ADC internal channel connected to Vbat/3: Vbat voltage through a divider ladder of factor 1/3 to have Vbat always below Vdda. */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_REG_TRIGGER_SOURCE  ADC group regular - Trigger source
  * @{
  */
#define LL_ADC_REG_TRIG_SOFTWARE           (0x00000000UL)                                                                                 /*!< ADC group regular conversion trigger internal: SW start. */
#define LL_ADC_REG_TRIG_EXT_TIM1_TRGO2     (                                                               ADC_REG_TRIG_EXT_EDGE_DEFAULT) /*!< ADC group regular conversion trigger from external IP: TIM1 TRGO. Trigger edge set to rising edge (default setting). */
#define LL_ADC_REG_TRIG_EXT_TIM1_CH4       (                                          ADC_CFGR1_EXTSEL_0 | ADC_REG_TRIG_EXT_EDGE_DEFAULT) /*!< ADC group regular conversion trigger from external IP: TIM1 channel 4 event (capture compare: input capture or output capture). Trigger edge set to rising edge (default setting). */
#if defined(TIM2)
#define LL_ADC_REG_TRIG_EXT_TIM2_TRGO      (                     ADC_CFGR1_EXTSEL_1                      | ADC_REG_TRIG_EXT_EDGE_DEFAULT) /*!< ADC group regular conversion trigger from external IP: TIM2 TRGO. Trigger edge set to rising edge (default setting). */
#endif
#define LL_ADC_REG_TRIG_EXT_TIM3_TRGO      (                     ADC_CFGR1_EXTSEL_1 | ADC_CFGR1_EXTSEL_0 | ADC_REG_TRIG_EXT_EDGE_DEFAULT) /*!< ADC group regular conversion trigger from external IP: TIM3 TRGO. Trigger edge set to rising edge (default setting). */
#if defined(TIM6)
#define LL_ADC_REG_TRIG_EXT_TIM6_TRGO      (ADC_CFGR1_EXTSEL_2                      | ADC_CFGR1_EXTSEL_0 | ADC_REG_TRIG_EXT_EDGE_DEFAULT) /*!< ADC group regular conversion trigger from external IP: TIM6 TRGO. Trigger edge set to rising edge (default setting). */
#endif
#if defined(TIM15)
#define LL_ADC_REG_TRIG_EXT_TIM15_TRGO     (ADC_CFGR1_EXTSEL_2                                           | ADC_REG_TRIG_EXT_EDGE_DEFAULT) /*!< ADC group regular conversion trigger from external IP: TIM15 TRGO. Trigger edge set to rising edge (default setting). */
#endif
#define LL_ADC_REG_TRIG_EXT_EXTI_LINE11    (ADC_CFGR1_EXTSEL_2 | ADC_CFGR1_EXTSEL_1 | ADC_CFGR1_EXTSEL_0 | ADC_REG_TRIG_EXT_EDGE_DEFAULT) /*!< ADC group regular conversion trigger from external IP: external interrupt line 11. Trigger edge set to rising edge (default setting). */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_REG_TRIGGER_EDGE  ADC group regular - Trigger edge
  * @{
  */
#define LL_ADC_REG_TRIG_EXT_RISING         (                    ADC_CFGR1_EXTEN_0) /*!< ADC group regular conversion trigger polarity set to rising edge */
#define LL_ADC_REG_TRIG_EXT_FALLING        (ADC_CFGR1_EXTEN_1                    ) /*!< ADC group regular conversion trigger polarity set to falling edge */
#define LL_ADC_REG_TRIG_EXT_RISINGFALLING  (ADC_CFGR1_EXTEN_1 | ADC_CFGR1_EXTEN_0) /*!< ADC group regular conversion trigger polarity set to both rising and falling edges */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_REG_CONTINUOUS_MODE  ADC group regular - Continuous mode
* @{
*/
#define LL_ADC_REG_CONV_SINGLE             (0x00000000UL)          /*!< ADC conversions are performed in single mode: one conversion per trigger */
#define LL_ADC_REG_CONV_CONTINUOUS         (ADC_CFGR1_CONT)        /*!< ADC conversions are performed in continuous mode: after the first trigger, following conversions launched successively automatically */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_REG_DMA_TRANSFER  ADC group regular - DMA transfer of ADC conversion data
  * @{
  */
#define LL_ADC_REG_DMA_TRANSFER_NONE       (0x00000000UL)                        /*!< ADC conversions are not transferred by DMA */
#define LL_ADC_REG_DMA_TRANSFER_LIMITED    (                   ADC_CFGR1_DMAEN)  /*!< ADC conversion data are transferred by DMA, in limited mode (one shot mode): DMA transfer requests are stopped when number of DMA data transfers (number of ADC conversions) is reached. This ADC mode is intended to be used with DMA mode non-circular. */
#define LL_ADC_REG_DMA_TRANSFER_UNLIMITED  (ADC_CFGR1_DMACFG | ADC_CFGR1_DMAEN)  /*!< ADC conversion data are transferred by DMA, in unlimited mode: DMA transfer requests are unlimited, whatever number of DMA data transferred (number of ADC conversions). This ADC mode is intended to be used with DMA mode circular. */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_REG_OVR_DATA_BEHAVIOR  ADC group regular - Overrun behavior on conversion data
* @{
*/
#define LL_ADC_REG_OVR_DATA_PRESERVED      (0x00000000UL)         /*!< ADC group regular behavior in case of overrun: data preserved */
#define LL_ADC_REG_OVR_DATA_OVERWRITTEN    (ADC_CFGR1_OVRMOD)     /*!< ADC group regular behavior in case of overrun: data overwritten */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_REG_SEQ_MODE  ADC group regular - Sequencer configuration flexibility
* @{
*/
#define LL_ADC_REG_SEQ_FIXED               (0x00000000UL)         /*!< Sequencer configured to fully configurable: sequencer length and each rank affectation to a channel are configurable. Refer to description of function @ref LL_ADC_REG_SetSequencerLength(). */
#define LL_ADC_REG_SEQ_CONFIGURABLE        (ADC_CFGR1_CHSELRMOD)  /*!< Sequencer configured to not fully configurable: sequencer length and each rank affectation to a channel are fixed by channel HW number. Refer to description of function @ref LL_ADC_REG_SetSequencerChannels(). */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_REG_SEQ_SCAN_LENGTH  ADC group regular - Sequencer scan length
  * @{
  */
#define LL_ADC_REG_SEQ_SCAN_DISABLE        (ADC_CHSELR_SQ2)       /*!< ADC group regular sequencer disable (equivalent to sequencer of 1 rank: ADC conversion on only 1 channel) */
#define LL_ADC_REG_SEQ_SCAN_ENABLE_2RANKS  (ADC_CHSELR_SQ3)       /*!< ADC group regular sequencer enable with 2 ranks in the sequence */
#define LL_ADC_REG_SEQ_SCAN_ENABLE_3RANKS  (ADC_CHSELR_SQ4)       /*!< ADC group regular sequencer enable with 3 ranks in the sequence */
#define LL_ADC_REG_SEQ_SCAN_ENABLE_4RANKS  (ADC_CHSELR_SQ5)       /*!< ADC group regular sequencer enable with 4 ranks in the sequence */
#define LL_ADC_REG_SEQ_SCAN_ENABLE_5RANKS  (ADC_CHSELR_SQ6)       /*!< ADC group regular sequencer enable with 5 ranks in the sequence */
#define LL_ADC_REG_SEQ_SCAN_ENABLE_6RANKS  (ADC_CHSELR_SQ7)       /*!< ADC group regular sequencer enable with 6 ranks in the sequence */
#define LL_ADC_REG_SEQ_SCAN_ENABLE_7RANKS  (ADC_CHSELR_SQ8)       /*!< ADC group regular sequencer enable with 7 ranks in the sequence */
#define LL_ADC_REG_SEQ_SCAN_ENABLE_8RANKS  (0x00000000UL)         /*!< ADC group regular sequencer enable with 8 ranks in the sequence */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_REG_SEQ_SCAN_DIRECTION  ADC group regular - Sequencer scan direction
  * @{
  */
#define LL_ADC_REG_SEQ_SCAN_DIR_FORWARD    (0x00000000UL)         /*!< On this STM32 serie, parameter relevant only is sequencer set to mode not fully configurable, refer to function @ref LL_ADC_REG_SetSequencerConfigurable(). ADC group regular sequencer scan direction forward: from lowest channel number to highest channel number (scan of all ranks, ADC conversion of ranks with channels enabled in sequencer). On some other STM32 families, this setting is not available and the default scan direction is forward. */
#define LL_ADC_REG_SEQ_SCAN_DIR_BACKWARD   (ADC_CFGR1_SCANDIR)    /*!< On this STM32 serie, parameter relevant only is sequencer set to mode not fully configurable, refer to function @ref LL_ADC_REG_SetSequencerConfigurable(). ADC group regular sequencer scan direction backward: from highest channel number to lowest channel number (scan of all ranks, ADC conversion of ranks with channels enabled in sequencer) */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_REG_SEQ_DISCONT_MODE  ADC group regular - Sequencer discontinuous mode
  * @{
  */
#define LL_ADC_REG_SEQ_DISCONT_DISABLE     (0x00000000UL)                                                               /*!< ADC group regular sequencer discontinuous mode disable */
#define LL_ADC_REG_SEQ_DISCONT_1RANK       (ADC_CFGR1_DISCEN)                                                           /*!< ADC group regular sequencer discontinuous mode enable with sequence interruption every rank */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_REG_SEQ_RANKS  ADC group regular - Sequencer ranks
  * @{
  */
#define LL_ADC_REG_RANK_1                  (ADC_REG_RANK_1_SQRX_BITOFFSET_POS)  /*!< ADC group regular sequencer rank 1 */
#define LL_ADC_REG_RANK_2                  (ADC_REG_RANK_2_SQRX_BITOFFSET_POS)  /*!< ADC group regular sequencer rank 2 */
#define LL_ADC_REG_RANK_3                  (ADC_REG_RANK_3_SQRX_BITOFFSET_POS)  /*!< ADC group regular sequencer rank 3 */
#define LL_ADC_REG_RANK_4                  (ADC_REG_RANK_4_SQRX_BITOFFSET_POS)  /*!< ADC group regular sequencer rank 4 */
#define LL_ADC_REG_RANK_5                  (ADC_REG_RANK_5_SQRX_BITOFFSET_POS)  /*!< ADC group regular sequencer rank 5 */
#define LL_ADC_REG_RANK_6                  (ADC_REG_RANK_6_SQRX_BITOFFSET_POS)  /*!< ADC group regular sequencer rank 6 */
#define LL_ADC_REG_RANK_7                  (ADC_REG_RANK_7_SQRX_BITOFFSET_POS)  /*!< ADC group regular sequencer rank 7 */
#define LL_ADC_REG_RANK_8                  (ADC_REG_RANK_8_SQRX_BITOFFSET_POS)  /*!< ADC group regular sequencer rank 8 */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_CHANNEL_SAMPLINGTIME  Channel - Sampling time
  * @{
  */
#define LL_ADC_SAMPLINGTIME_1CYCLE_5       (0x00000000UL)                                        /*!< Sampling time 1.5 ADC clock cycle */
#define LL_ADC_SAMPLINGTIME_3CYCLES_5      (ADC_SMPR_SMP1_0)                                     /*!< Sampling time 3.5 ADC clock cycles */
#define LL_ADC_SAMPLINGTIME_7CYCLES_5      (ADC_SMPR_SMP1_1)                                     /*!< Sampling time 7.5 ADC clock cycles */
#define LL_ADC_SAMPLINGTIME_12CYCLES_5     (ADC_SMPR_SMP1_1 | ADC_SMPR_SMP1_0)                   /*!< Sampling time 12.5 ADC clock cycles */
#define LL_ADC_SAMPLINGTIME_19CYCLES_5     (ADC_SMPR_SMP1_2)                                     /*!< Sampling time 19.5 ADC clock cycles */
#define LL_ADC_SAMPLINGTIME_39CYCLES_5     (ADC_SMPR_SMP1_2 | ADC_SMPR_SMP1_0)                   /*!< Sampling time 39.5 ADC clock cycles */
#define LL_ADC_SAMPLINGTIME_79CYCLES_5     (ADC_SMPR_SMP1_2 | ADC_SMPR_SMP1_1)                   /*!< Sampling time 79.5 ADC clock cycles */
#define LL_ADC_SAMPLINGTIME_160CYCLES_5    (ADC_SMPR_SMP1_2 | ADC_SMPR_SMP1_1 | ADC_SMPR_SMP1_0) /*!< Sampling time 160.5 ADC clock cycles */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_AWD_NUMBER Analog watchdog - Analog watchdog number
  * @{
  */
#define LL_ADC_AWD1                        (ADC_AWD_CR1_CHANNEL_MASK  | ADC_AWD_CR1_REGOFFSET) /*!< ADC analog watchdog number 1 */
#define LL_ADC_AWD2                        (ADC_AWD_CR23_CHANNEL_MASK | ADC_AWD_CR2_REGOFFSET) /*!< ADC analog watchdog number 2 */
#define LL_ADC_AWD3                        (ADC_AWD_CR23_CHANNEL_MASK | ADC_AWD_CR3_REGOFFSET) /*!< ADC analog watchdog number 3 */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_AWD_CHANNELS  Analog watchdog - Monitored channels
  * @{
  */
#define LL_ADC_AWD_DISABLE                 (0x00000000UL)                                                                             /*!< ADC analog watchdog monitoring disabled */
#define LL_ADC_AWD_ALL_CHANNELS_REG        (ADC_AWD_CR23_CHANNEL_MASK                         | ADC_CFGR1_AWD1EN                    ) /*!< ADC analog watchdog monitoring of all channels, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_0_REG           ((LL_ADC_CHANNEL_0  & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN0, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_1_REG           ((LL_ADC_CHANNEL_1  & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN1, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_2_REG           ((LL_ADC_CHANNEL_2  & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN2, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_3_REG           ((LL_ADC_CHANNEL_3  & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN3, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_4_REG           ((LL_ADC_CHANNEL_4  & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN4, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_5_REG           ((LL_ADC_CHANNEL_5  & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN5, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_6_REG           ((LL_ADC_CHANNEL_6  & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN6, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_7_REG           ((LL_ADC_CHANNEL_7  & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN7, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_8_REG           ((LL_ADC_CHANNEL_8  & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN8, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_9_REG           ((LL_ADC_CHANNEL_9  & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN9, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_10_REG          ((LL_ADC_CHANNEL_10 & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN10, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_11_REG          ((LL_ADC_CHANNEL_11 & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN11, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_12_REG          ((LL_ADC_CHANNEL_12 & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN12, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_13_REG          ((LL_ADC_CHANNEL_13 & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN13, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_14_REG          ((LL_ADC_CHANNEL_14 & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN14, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_15_REG          ((LL_ADC_CHANNEL_15 & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN15, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_16_REG          ((LL_ADC_CHANNEL_16 & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN16, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_17_REG          ((LL_ADC_CHANNEL_17 & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN17, converted by group regular only */
#define LL_ADC_AWD_CHANNEL_18_REG          ((LL_ADC_CHANNEL_18 & ADC_CHANNEL_ID_MASK)         | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC external channel (channel connected to GPIO pin) ADCx_IN18, converted by group regular only */
#define LL_ADC_AWD_CH_VREFINT_REG          ((LL_ADC_CHANNEL_VREFINT    & ADC_CHANNEL_ID_MASK) | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC internal channel connected to VrefInt: Internal voltage reference, converted by group regular only */
#define LL_ADC_AWD_CH_TEMPSENSOR_REG       ((LL_ADC_CHANNEL_TEMPSENSOR & ADC_CHANNEL_ID_MASK) | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC internal channel connected to Temperature sensor, converted by group regular only */
#define LL_ADC_AWD_CH_VBAT_REG             ((LL_ADC_CHANNEL_VBAT       & ADC_CHANNEL_ID_MASK) | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL) /*!< ADC analog watchdog monitoring of ADC internal channel connected to Vbat/3: Vbat voltage through a divider ladder of factor 1/3 to have Vbat always below Vdda, converted by group regular only */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_AWD_THRESHOLDS  Analog watchdog - Thresholds
  * @{
  */
#define LL_ADC_AWD_THRESHOLD_HIGH          (ADC_TR1_HT1              ) /*!< ADC analog watchdog threshold high */
#define LL_ADC_AWD_THRESHOLD_LOW           (              ADC_TR1_LT1) /*!< ADC analog watchdog threshold low */
#define LL_ADC_AWD_THRESHOLDS_HIGH_LOW     (ADC_TR1_HT1 | ADC_TR1_LT1) /*!< ADC analog watchdog both thresholds high and low concatenated into the same data */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_OVS_SCOPE  Oversampling - Oversampling scope
  * @{
  */
#define LL_ADC_OVS_DISABLE                 (0x00000000UL)                                        /*!< ADC oversampling disabled. */
#define LL_ADC_OVS_GRP_REGULAR_CONTINUED   (                                    ADC_CFGR2_OVSE)  /*!< ADC oversampling on conversions of ADC group regular. Literal suffix "continued" is kept for compatibility with other STM32 devices featuring ADC group injected, in this case other oversampling scope parameters are available. */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_OVS_DISCONT_MODE  Oversampling - Discontinuous mode
  * @{
  */
#define LL_ADC_OVS_REG_CONT                (0x00000000UL)         /*!< ADC oversampling discontinuous mode: continuous mode (all conversions of oversampling ratio are done from 1 trigger) */
#define LL_ADC_OVS_REG_DISCONT             (ADC_CFGR2_TOVS)       /*!< ADC oversampling discontinuous mode: discontinuous mode (each conversion of oversampling ratio needs a trigger) */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_OVS_RATIO  Oversampling - Ratio
  * @{
  */
#define LL_ADC_OVS_RATIO_2                 (0x00000000UL)                                           /*!< ADC oversampling ratio of 2 (2 ADC conversions are performed, sum of these conversions data is computed to result as the ADC oversampling conversion data (before potential shift) */
#define LL_ADC_OVS_RATIO_4                 (                                      ADC_CFGR2_OVSR_0) /*!< ADC oversampling ratio of 4 (4 ADC conversions are performed, sum of these conversions data is computed to result as the ADC oversampling conversion data (before potential shift) */
#define LL_ADC_OVS_RATIO_8                 (                   ADC_CFGR2_OVSR_1                   ) /*!< ADC oversampling ratio of 8 (8 ADC conversions are performed, sum of these conversions data is computed to result as the ADC oversampling conversion data (before potential shift) */
#define LL_ADC_OVS_RATIO_16                (                   ADC_CFGR2_OVSR_1 | ADC_CFGR2_OVSR_0) /*!< ADC oversampling ratio of 16 (16 ADC conversions are performed, sum of these conversions data is computed to result as the ADC oversampling conversion data (before potential shift) */
#define LL_ADC_OVS_RATIO_32                (ADC_CFGR2_OVSR_2                                      ) /*!< ADC oversampling ratio of 32 (32 ADC conversions are performed, sum of these conversions data is computed to result as the ADC oversampling conversion data (before potential shift) */
#define LL_ADC_OVS_RATIO_64                (ADC_CFGR2_OVSR_2                    | ADC_CFGR2_OVSR_0) /*!< ADC oversampling ratio of 64 (64 ADC conversions are performed, sum of these conversions data is computed to result as the ADC oversampling conversion data (before potential shift) */
#define LL_ADC_OVS_RATIO_128               (ADC_CFGR2_OVSR_2 | ADC_CFGR2_OVSR_1                   ) /*!< ADC oversampling ratio of 128 (128 ADC conversions are performed, sum of these conversions data is computed to result as the ADC oversampling conversion data (before potential shift) */
#define LL_ADC_OVS_RATIO_256               (ADC_CFGR2_OVSR_2 | ADC_CFGR2_OVSR_1 | ADC_CFGR2_OVSR_0) /*!< ADC oversampling ratio of 256 (256 ADC conversions are performed, sum of these conversions data is computed to result as the ADC oversampling conversion data (before potential shift) */
/**
  * @}
  */

/** @defgroup ADC_LL_EC_OVS_SHIFT  Oversampling - Data shift
  * @{
  */
#define LL_ADC_OVS_SHIFT_NONE              (0x00000000UL)                                                              /*!< ADC oversampling no shift (sum of the ADC conversions data is not divided to result as the ADC oversampling conversion data) */
#define LL_ADC_OVS_SHIFT_RIGHT_1           (                                                         ADC_CFGR2_OVSS_0) /*!< ADC oversampling shift of 1 (sum of the ADC conversions data is divided by 2 to result as the ADC oversampling conversion data) */
#define LL_ADC_OVS_SHIFT_RIGHT_2           (                                      ADC_CFGR2_OVSS_1                   ) /*!< ADC oversampling shift of 2 (sum of the ADC conversions data is divided by 4 to result as the ADC oversampling conversion data) */
#define LL_ADC_OVS_SHIFT_RIGHT_3           (                                      ADC_CFGR2_OVSS_1 | ADC_CFGR2_OVSS_0) /*!< ADC oversampling shift of 3 (sum of the ADC conversions data is divided by 8 to result as the ADC oversampling conversion data) */
#define LL_ADC_OVS_SHIFT_RIGHT_4           (                   ADC_CFGR2_OVSS_2                                      ) /*!< ADC oversampling shift of 4 (sum of the ADC conversions data is divided by 16 to result as the ADC oversampling conversion data) */
#define LL_ADC_OVS_SHIFT_RIGHT_5           (                   ADC_CFGR2_OVSS_2                    | ADC_CFGR2_OVSS_0) /*!< ADC oversampling shift of 5 (sum of the ADC conversions data is divided by 32 to result as the ADC oversampling conversion data) */
#define LL_ADC_OVS_SHIFT_RIGHT_6           (                   ADC_CFGR2_OVSS_2 | ADC_CFGR2_OVSS_1                   ) /*!< ADC oversampling shift of 6 (sum of the ADC conversions data is divided by 64 to result as the ADC oversampling conversion data) */
#define LL_ADC_OVS_SHIFT_RIGHT_7           (                   ADC_CFGR2_OVSS_2 | ADC_CFGR2_OVSS_1 | ADC_CFGR2_OVSS_0) /*!< ADC oversampling shift of 7 (sum of the ADC conversions data is divided by 128 to result as the ADC oversampling conversion data) */
#define LL_ADC_OVS_SHIFT_RIGHT_8           (ADC_CFGR2_OVSS_3                                                         ) /*!< ADC oversampling shift of 8 (sum of the ADC conversions data is divided by 256 to result as the ADC oversampling conversion data) */
/**
  * @}
  */


/** @defgroup ADC_LL_EC_HW_DELAYS  Definitions of ADC hardware constraints delays
  * @note   Only ADC IP HW delays are defined in ADC LL driver driver,
  *         not timeout values.
  *         For details on delays values, refer to descriptions in source code
  *         above each literal definition.
  * @{
  */
  
/* Note: Only ADC IP HW delays are defined in ADC LL driver driver,           */
/*       not timeout values.                                                  */
/*       Timeout values for ADC operations are dependent to device clock      */
/*       configuration (system clock versus ADC clock),                       */
/*       and therefore must be defined in user application.                   */
/*       Indications for estimation of ADC timeout delays, for this           */
/*       STM32 serie:                                                         */
/*       - ADC calibration time: maximum delay is 82/fADC.                    */
/*         (refer to device datasheet, parameter "tCAL")                      */
/*       - ADC enable time: maximum delay is 1 conversion cycle.              */
/*         (refer to device datasheet, parameter "tSTAB")                     */
/*       - ADC disable time: maximum delay should be a few ADC clock cycles   */
/*       - ADC stop conversion time: maximum delay should be a few ADC clock  */
/*         cycles                                                             */
/*       - ADC conversion time: duration depending on ADC clock and ADC       */
/*         configuration.                                                     */
/*         (refer to device reference manual, section "Timing")               */

/* Delay for ADC stabilization time (ADC voltage regulator start-up time)     */
/* Delay set to maximum value (refer to device datasheet,                     */
/* parameter "tADCVREG_STUP").                                                */
/* Unit: us                                                                   */
#define LL_ADC_DELAY_INTERNAL_REGUL_STAB_US ( 20UL)  /*!< Delay for ADC stabilization time (ADC voltage regulator start-up time) */

/* Delay for internal voltage reference stabilization time.                   */
/* Delay set to maximum value (refer to device datasheet,                     */
/* parameter "tstart_vrefint").                                               */
/* Unit: us                                                                   */
#define LL_ADC_DELAY_VREFINT_STAB_US       ( 12UL)  /*!< Delay for internal voltage reference stabilization time */

/* Delay for temperature sensor stabilization time.                           */
/* Literal set to maximum value (refer to device datasheet,                   */
/* parameter "tSTART").                                                       */
/* Unit: us                                                                   */
#define LL_ADC_DELAY_TEMPSENSOR_STAB_US    ( 10UL)  /*!< Delay for temperature sensor stabilization time */

/* Delay required between ADC end of calibration and ADC enable.              */
/* Note: On this STM32 serie, a minimum number of ADC clock cycles            */
/*       are required between ADC end of calibration and ADC enable.          */
/*       Wait time can be computed in user application by waiting for the     */
/*       equivalent number of CPU cycles, by taking into account              */
/*       ratio of CPU clock versus ADC clock prescalers.                      */
/* Unit: ADC clock cycles.                                                    */
#define LL_ADC_DELAY_CALIB_ENABLE_ADC_CYCLES (  2UL)  /*!< Delay required between ADC end of calibration and ADC enable */

/**
  * @}
  */

/**
  * @}
  */


/* Exported macro ------------------------------------------------------------*/
/** @defgroup ADC_LL_Exported_Macros ADC Exported Macros
  * @{
  */

/** @defgroup ADC_LL_EM_WRITE_READ Common write and read registers Macros
  * @{
  */

/**
  * @brief  Write a value in ADC register
  * @param  __INSTANCE__ ADC Instance
  * @param  __REG__ Register to be written
  * @param  __VALUE__ Value to be written in the register
  * @retval None
  */
#define LL_ADC_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
  * @brief  Read a value in ADC register
  * @param  __INSTANCE__ ADC Instance
  * @param  __REG__ Register to be read
  * @retval Register value
  */
#define LL_ADC_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
  * @}
  */

/** @defgroup ADC_LL_EM_HELPER_MACRO ADC helper macro
  * @{
  */

/**
  * @brief  Helper macro to get ADC channel number in decimal format
  *         from literals LL_ADC_CHANNEL_x.
  * @note   Example:
  *           __LL_ADC_CHANNEL_TO_DECIMAL_NB(LL_ADC_CHANNEL_4)
  *           will return decimal number "4".
  * @note   The input can be a value from functions where a channel
  *         number is returned, either defined with number
  *         or with bitfield (only one bit must be set).
  * @param  __CHANNEL__ This parameter can be one of the following values:
  *         @arg @ref LL_ADC_CHANNEL_0
  *         @arg @ref LL_ADC_CHANNEL_1
  *         @arg @ref LL_ADC_CHANNEL_2
  *         @arg @ref LL_ADC_CHANNEL_3
  *         @arg @ref LL_ADC_CHANNEL_4
  *         @arg @ref LL_ADC_CHANNEL_5
  *         @arg @ref LL_ADC_CHANNEL_6
  *         @arg @ref LL_ADC_CHANNEL_7
  *         @arg @ref LL_ADC_CHANNEL_8
  *         @arg @ref LL_ADC_CHANNEL_9
  *         @arg @ref LL_ADC_CHANNEL_10
  *         @arg @ref LL_ADC_CHANNEL_11
  *         @arg @ref LL_ADC_CHANNEL_12
  *         @arg @ref LL_ADC_CHANNEL_13
  *         @arg @ref LL_ADC_CHANNEL_14
  *         @arg @ref LL_ADC_CHANNEL_15         (1)
  *         @arg @ref LL_ADC_CHANNEL_16         (1)
  *         @arg @ref LL_ADC_CHANNEL_17         (1)
  *         @arg @ref LL_ADC_CHANNEL_18
  *         @arg @ref LL_ADC_CHANNEL_VREFINT
  *         @arg @ref LL_ADC_CHANNEL_TEMPSENSOR
  *         @arg @ref LL_ADC_CHANNEL_VBAT
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().
  * @retval Value between Min_Data=0 and Max_Data=18
  */
#define __LL_ADC_CHANNEL_TO_DECIMAL_NB(__CHANNEL__)                                                               \
  ((((__CHANNEL__) & ADC_CHANNEL_ID_BITFIELD_MASK) == 0UL)                                                        \
    ? (                                                                                                           \
       ((__CHANNEL__) & ADC_CHANNEL_ID_NUMBER_MASK) >> ADC_CHANNEL_ID_NUMBER_BITOFFSET_POS                        \
      )                                                                                                           \
      :                                                                                                           \
      (                                                                                                           \
       (((__CHANNEL__) & ADC_CHSELR_CHSEL0) == ADC_CHSELR_CHSEL0) ? (0UL) :                                       \
        (                                                                                                         \
         (((__CHANNEL__) & ADC_CHSELR_CHSEL1) == ADC_CHSELR_CHSEL1) ? (1UL) :                                     \
          (                                                                                                       \
           (((__CHANNEL__) & ADC_CHSELR_CHSEL2) == ADC_CHSELR_CHSEL2) ? (2UL) :                                   \
            (                                                                                                     \
             (((__CHANNEL__) & ADC_CHSELR_CHSEL3) == ADC_CHSELR_CHSEL3) ? (3UL) :                                 \
              (                                                                                                   \
               (((__CHANNEL__) & ADC_CHSELR_CHSEL4) == ADC_CHSELR_CHSEL4) ? (4UL) :                               \
                (                                                                                                 \
                 (((__CHANNEL__) & ADC_CHSELR_CHSEL5) == ADC_CHSELR_CHSEL5) ? (5UL) :                             \
                  (                                                                                               \
                   (((__CHANNEL__) & ADC_CHSELR_CHSEL6) == ADC_CHSELR_CHSEL6) ? (6UL) :                           \
                    (                                                                                             \
                     (((__CHANNEL__) & ADC_CHSELR_CHSEL7) == ADC_CHSELR_CHSEL7) ? (7UL) :                         \
                      (                                                                                           \
                       (((__CHANNEL__) & ADC_CHSELR_CHSEL8) == ADC_CHSELR_CHSEL8) ? (8UL) :                       \
                        (                                                                                         \
                         (((__CHANNEL__) & ADC_CHSELR_CHSEL9) == ADC_CHSELR_CHSEL9) ? (9UL) :                     \
                          (                                                                                       \
                           (((__CHANNEL__) & ADC_CHSELR_CHSEL10) == ADC_CHSELR_CHSEL10) ? (10UL) :                \
                            (                                                                                     \
                             (((__CHANNEL__) & ADC_CHSELR_CHSEL11) == ADC_CHSELR_CHSEL11) ? (11UL) :              \
                              (                                                                                   \
                               (((__CHANNEL__) & ADC_CHSELR_CHSEL12) == ADC_CHSELR_CHSEL12) ? (12UL) :            \
                                (                                                                                 \
                                 (((__CHANNEL__) & ADC_CHSELR_CHSEL13) == ADC_CHSELR_CHSEL13) ? (13UL) :          \
                                  (                                                                               \
                                   (((__CHANNEL__) & ADC_CHSELR_CHSEL14) == ADC_CHSELR_CHSEL14) ? (14UL) :        \
                                    (                                                                             \
                                     (((__CHANNEL__) & ADC_CHSELR_CHSEL15) == ADC_CHSELR_CHSEL15) ? (15UL) :      \
                                      (                                                                           \
                                       (((__CHANNEL__) & ADC_CHSELR_CHSEL16) == ADC_CHSELR_CHSEL16) ? (16UL) :    \
                                        (                                                                         \
                                         (((__CHANNEL__) & ADC_CHSELR_CHSEL17) == ADC_CHSELR_CHSEL17) ? (17UL) :  \
                                          (                                                                       \
                                           (((__CHANNEL__) & ADC_CHSELR_CHSEL18) == ADC_CHSELR_CHSEL18) ? (18UL) :\
                                            (0UL)                                                                 \
                                          )                                                                       \
                                        )                                                                         \
                                      )                                                                           \
                                    )                                                                             \
                                  )                                                                               \
                                )                                                                                 \
                              )                                                                                   \
                            )                                                                                     \
                          )                                                                                       \
                        )                                                                                         \
                      )                                                                                           \
                    )                                                                                             \
                  )                                                                                               \
                )                                                                                                 \
              )                                                                                                   \
            )                                                                                                     \
          )                                                                                                       \
        )                                                                                                         \
      )                                                                                                           \
  )

/**
  * @brief  Helper macro to get ADC channel in literal format LL_ADC_CHANNEL_x
  *         from number in decimal format.
  * @note   Example:
  *           __LL_ADC_DECIMAL_NB_TO_CHANNEL(4)
  *           will return a data equivalent to "LL_ADC_CHANNEL_4".
  * @param  __DECIMAL_NB__ Value between Min_Data=0 and Max_Data=18
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_CHANNEL_0
  *         @arg @ref LL_ADC_CHANNEL_1
  *         @arg @ref LL_ADC_CHANNEL_2
  *         @arg @ref LL_ADC_CHANNEL_3
  *         @arg @ref LL_ADC_CHANNEL_4
  *         @arg @ref LL_ADC_CHANNEL_5
  *         @arg @ref LL_ADC_CHANNEL_6
  *         @arg @ref LL_ADC_CHANNEL_7
  *         @arg @ref LL_ADC_CHANNEL_8
  *         @arg @ref LL_ADC_CHANNEL_9
  *         @arg @ref LL_ADC_CHANNEL_10
  *         @arg @ref LL_ADC_CHANNEL_11
  *         @arg @ref LL_ADC_CHANNEL_12
  *         @arg @ref LL_ADC_CHANNEL_13
  *         @arg @ref LL_ADC_CHANNEL_14
  *         @arg @ref LL_ADC_CHANNEL_15         (1)
  *         @arg @ref LL_ADC_CHANNEL_16         (1)
  *         @arg @ref LL_ADC_CHANNEL_17         (1)
  *         @arg @ref LL_ADC_CHANNEL_18
  *         @arg @ref LL_ADC_CHANNEL_VREFINT    (2)
  *         @arg @ref LL_ADC_CHANNEL_TEMPSENSOR (2)
  *         @arg @ref LL_ADC_CHANNEL_VBAT       (2)
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().\n
  *         (2) For ADC channel read back from ADC register,
  *             comparison with internal channel parameter to be done
  *             using helper macro @ref __LL_ADC_CHANNEL_INTERNAL_TO_EXTERNAL().
  */
#define __LL_ADC_DECIMAL_NB_TO_CHANNEL(__DECIMAL_NB__)                         \
  (                                                                            \
   ((__DECIMAL_NB__) << ADC_CHANNEL_ID_NUMBER_BITOFFSET_POS) |                 \
   (ADC_CHSELR_CHSEL0 << (__DECIMAL_NB__))                                     \
  )

/**
  * @brief  Helper macro to determine whether the selected channel
  *         corresponds to literal definitions of driver.
  * @note   The different literal definitions of ADC channels are:
  *         - ADC internal channel:
  *           LL_ADC_CHANNEL_VREFINT, LL_ADC_CHANNEL_TEMPSENSOR, ...
  *         - ADC external channel (channel connected to a GPIO pin):
  *           LL_ADC_CHANNEL_1, LL_ADC_CHANNEL_2, ...
  * @note   The channel parameter must be a value defined from literal
  *         definition of a ADC internal channel (LL_ADC_CHANNEL_VREFINT,
  *         LL_ADC_CHANNEL_TEMPSENSOR, ...),
  *         ADC external channel (LL_ADC_CHANNEL_1, LL_ADC_CHANNEL_2, ...),
  *         must not be a value from functions where a channel number is
  *         returned from ADC registers,
  *         because internal and external channels share the same channel
  *         number in ADC registers. The differentiation is made only with
  *         parameters definitions of driver.
  * @param  __CHANNEL__ This parameter can be one of the following values:
  *         @arg @ref LL_ADC_CHANNEL_0
  *         @arg @ref LL_ADC_CHANNEL_1
  *         @arg @ref LL_ADC_CHANNEL_2
  *         @arg @ref LL_ADC_CHANNEL_3
  *         @arg @ref LL_ADC_CHANNEL_4
  *         @arg @ref LL_ADC_CHANNEL_5
  *         @arg @ref LL_ADC_CHANNEL_6
  *         @arg @ref LL_ADC_CHANNEL_7
  *         @arg @ref LL_ADC_CHANNEL_8
  *         @arg @ref LL_ADC_CHANNEL_9
  *         @arg @ref LL_ADC_CHANNEL_10
  *         @arg @ref LL_ADC_CHANNEL_11
  *         @arg @ref LL_ADC_CHANNEL_12
  *         @arg @ref LL_ADC_CHANNEL_13
  *         @arg @ref LL_ADC_CHANNEL_14
  *         @arg @ref LL_ADC_CHANNEL_15         (1)
  *         @arg @ref LL_ADC_CHANNEL_16         (1)
  *         @arg @ref LL_ADC_CHANNEL_17         (1)
  *         @arg @ref LL_ADC_CHANNEL_18
  *         @arg @ref LL_ADC_CHANNEL_VREFINT
  *         @arg @ref LL_ADC_CHANNEL_TEMPSENSOR
  *         @arg @ref LL_ADC_CHANNEL_VBAT
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().
  * @retval Value "0" if the channel corresponds to a parameter definition of a ADC external channel (channel connected to a GPIO pin).
  *         Value "1" if the channel corresponds to a parameter definition of a ADC internal channel.
  */
#define __LL_ADC_IS_CHANNEL_INTERNAL(__CHANNEL__)                              \
  (((__CHANNEL__) & ADC_CHANNEL_ID_INTERNAL_CH_MASK) != 0UL)

/**
  * @brief  Helper macro to convert a channel defined from parameter
  *         definition of a ADC internal channel (LL_ADC_CHANNEL_VREFINT,
  *         LL_ADC_CHANNEL_TEMPSENSOR, ...),
  *         to its equivalent parameter definition of a ADC external channel
  *         (LL_ADC_CHANNEL_1, LL_ADC_CHANNEL_2, ...).
  * @note   The channel parameter can be, additionally to a value
  *         defined from parameter definition of a ADC internal channel
  *         (LL_ADC_CHANNEL_VREFINT, LL_ADC_CHANNEL_TEMPSENSOR, ...),
  *         a value defined from parameter definition of
  *         ADC external channel (LL_ADC_CHANNEL_1, LL_ADC_CHANNEL_2, ...)
  *         or a value from functions where a channel number is returned
  *         from ADC registers.
  * @param  __CHANNEL__ This parameter can be one of the following values:
  *         @arg @ref LL_ADC_CHANNEL_0
  *         @arg @ref LL_ADC_CHANNEL_1
  *         @arg @ref LL_ADC_CHANNEL_2
  *         @arg @ref LL_ADC_CHANNEL_3
  *         @arg @ref LL_ADC_CHANNEL_4
  *         @arg @ref LL_ADC_CHANNEL_5
  *         @arg @ref LL_ADC_CHANNEL_6
  *         @arg @ref LL_ADC_CHANNEL_7
  *         @arg @ref LL_ADC_CHANNEL_8
  *         @arg @ref LL_ADC_CHANNEL_9
  *         @arg @ref LL_ADC_CHANNEL_10
  *         @arg @ref LL_ADC_CHANNEL_11
  *         @arg @ref LL_ADC_CHANNEL_12
  *         @arg @ref LL_ADC_CHANNEL_13
  *         @arg @ref LL_ADC_CHANNEL_14
  *         @arg @ref LL_ADC_CHANNEL_15         (1)
  *         @arg @ref LL_ADC_CHANNEL_16         (1)
  *         @arg @ref LL_ADC_CHANNEL_17         (1)
  *         @arg @ref LL_ADC_CHANNEL_18
  *         @arg @ref LL_ADC_CHANNEL_VREFINT
  *         @arg @ref LL_ADC_CHANNEL_TEMPSENSOR
  *         @arg @ref LL_ADC_CHANNEL_VBAT
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_CHANNEL_0
  *         @arg @ref LL_ADC_CHANNEL_1
  *         @arg @ref LL_ADC_CHANNEL_2
  *         @arg @ref LL_ADC_CHANNEL_3
  *         @arg @ref LL_ADC_CHANNEL_4
  *         @arg @ref LL_ADC_CHANNEL_5
  *         @arg @ref LL_ADC_CHANNEL_6
  *         @arg @ref LL_ADC_CHANNEL_7
  *         @arg @ref LL_ADC_CHANNEL_8
  *         @arg @ref LL_ADC_CHANNEL_9
  *         @arg @ref LL_ADC_CHANNEL_10
  *         @arg @ref LL_ADC_CHANNEL_11
  *         @arg @ref LL_ADC_CHANNEL_12
  *         @arg @ref LL_ADC_CHANNEL_13
  *         @arg @ref LL_ADC_CHANNEL_14
  *         @arg @ref LL_ADC_CHANNEL_15
  *         @arg @ref LL_ADC_CHANNEL_16
  *         @arg @ref LL_ADC_CHANNEL_17
  *         @arg @ref LL_ADC_CHANNEL_18
  */
#define __LL_ADC_CHANNEL_INTERNAL_TO_EXTERNAL(__CHANNEL__)                     \
  ((__CHANNEL__) & ~ADC_CHANNEL_ID_INTERNAL_CH_MASK)

/**
  * @brief  Helper macro to determine whether the internal channel
  *         selected is available on the ADC instance selected.
  * @note   The channel parameter must be a value defined from parameter
  *         definition of a ADC internal channel (LL_ADC_CHANNEL_VREFINT,
  *         LL_ADC_CHANNEL_TEMPSENSOR, ...),
  *         must not be a value defined from parameter definition of
  *         ADC external channel (LL_ADC_CHANNEL_1, LL_ADC_CHANNEL_2, ...)
  *         or a value from functions where a channel number is
  *         returned from ADC registers,
  *         because internal and external channels share the same channel
  *         number in ADC registers. The differentiation is made only with
  *         parameters definitions of driver.
  * @param  __ADC_INSTANCE__ ADC instance
  * @param  __CHANNEL__ This parameter can be one of the following values:
  *         @arg @ref LL_ADC_CHANNEL_VREFINT
  *         @arg @ref LL_ADC_CHANNEL_TEMPSENSOR
  *         @arg @ref LL_ADC_CHANNEL_VBAT
  * @retval Value "0" if the internal channel selected is not available on the ADC instance selected.
  *         Value "1" if the internal channel selected is available on the ADC instance selected.
  */
#define __LL_ADC_IS_CHANNEL_INTERNAL_AVAILABLE(__ADC_INSTANCE__, __CHANNEL__)  \
  (                                                                            \
    ((__CHANNEL__) == LL_ADC_CHANNEL_VREFINT)    ||                            \
    ((__CHANNEL__) == LL_ADC_CHANNEL_TEMPSENSOR) ||                            \
    ((__CHANNEL__) == LL_ADC_CHANNEL_VBAT)                                     \
  )

/**
  * @brief  Helper macro to define ADC analog watchdog parameter:
  *         define a single channel to monitor with analog watchdog
  *         from sequencer channel and groups definition.
  * @note   To be used with function @ref LL_ADC_SetAnalogWDMonitChannels().
  *         Example:
  *           LL_ADC_SetAnalogWDMonitChannels(
  *             ADC1, LL_ADC_AWD1,
  *             __LL_ADC_ANALOGWD_CHANNEL_GROUP(LL_ADC_CHANNEL4, LL_ADC_GROUP_REGULAR))
  * @param  __CHANNEL__ This parameter can be one of the following values:
  *         @arg @ref LL_ADC_CHANNEL_0
  *         @arg @ref LL_ADC_CHANNEL_1
  *         @arg @ref LL_ADC_CHANNEL_2
  *         @arg @ref LL_ADC_CHANNEL_3
  *         @arg @ref LL_ADC_CHANNEL_4
  *         @arg @ref LL_ADC_CHANNEL_5
  *         @arg @ref LL_ADC_CHANNEL_6
  *         @arg @ref LL_ADC_CHANNEL_7
  *         @arg @ref LL_ADC_CHANNEL_8
  *         @arg @ref LL_ADC_CHANNEL_9
  *         @arg @ref LL_ADC_CHANNEL_10
  *         @arg @ref LL_ADC_CHANNEL_11
  *         @arg @ref LL_ADC_CHANNEL_12
  *         @arg @ref LL_ADC_CHANNEL_13
  *         @arg @ref LL_ADC_CHANNEL_14
  *         @arg @ref LL_ADC_CHANNEL_15         (1)
  *         @arg @ref LL_ADC_CHANNEL_16         (1)
  *         @arg @ref LL_ADC_CHANNEL_17         (1)
  *         @arg @ref LL_ADC_CHANNEL_18
  *         @arg @ref LL_ADC_CHANNEL_VREFINT    (2)
  *         @arg @ref LL_ADC_CHANNEL_TEMPSENSOR (2)
  *         @arg @ref LL_ADC_CHANNEL_VBAT       (2)
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().\n
  *         (2) For ADC channel read back from ADC register,
  *             comparison with internal channel parameter to be done
  *             using helper macro @ref __LL_ADC_CHANNEL_INTERNAL_TO_EXTERNAL().
  * @param  __GROUP__ This parameter can be one of the following values:
  *         @arg @ref LL_ADC_GROUP_REGULAR
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_AWD_DISABLE
  *         @arg @ref LL_ADC_AWD_ALL_CHANNELS_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_0_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_1_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_2_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_3_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_4_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_5_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_6_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_7_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_8_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_9_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_10_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_11_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_12_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_13_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_14_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_15_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_16_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_17_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_18_REG
  *         @arg @ref LL_ADC_AWD_CH_VREFINT_REG
  *         @arg @ref LL_ADC_AWD_CH_TEMPSENSOR_REG
  *         @arg @ref LL_ADC_AWD_CH_VBAT_REG
  */
#define __LL_ADC_ANALOGWD_CHANNEL_GROUP(__CHANNEL__, __GROUP__)                                           \
  (((__CHANNEL__) & ADC_CHANNEL_ID_MASK) | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL)

/**
  * @brief  Helper macro to set the value of ADC analog watchdog threshold high
  *         or low in function of ADC resolution, when ADC resolution is
  *         different of 12 bits.
  * @note   To be used with function @ref LL_ADC_ConfigAnalogWDThresholds()
  *         or @ref LL_ADC_SetAnalogWDThresholds().
  *         Example, with a ADC resolution of 8 bits, to set the value of
  *         analog watchdog threshold high (on 8 bits):
  *           LL_ADC_SetAnalogWDThresholds
  *            (< ADCx param >,
  *             __LL_ADC_ANALOGWD_SET_THRESHOLD_RESOLUTION(LL_ADC_RESOLUTION_8B, <threshold_value_8_bits>)
  *            );
  * @param  __ADC_RESOLUTION__ This parameter can be one of the following values:
  *         @arg @ref LL_ADC_RESOLUTION_12B
  *         @arg @ref LL_ADC_RESOLUTION_10B
  *         @arg @ref LL_ADC_RESOLUTION_8B
  *         @arg @ref LL_ADC_RESOLUTION_6B
  * @param  __AWD_THRESHOLD__ Value between Min_Data=0x000 and Max_Data=0xFFF
  * @retval Value between Min_Data=0x000 and Max_Data=0xFFF
  */
#define __LL_ADC_ANALOGWD_SET_THRESHOLD_RESOLUTION(__ADC_RESOLUTION__, __AWD_THRESHOLD__) \
  ((__AWD_THRESHOLD__) << ((__ADC_RESOLUTION__) >> (ADC_CFGR1_RES_BITOFFSET_POS - 1U )))

/**
  * @brief  Helper macro to get the value of ADC analog watchdog threshold high
  *         or low in function of ADC resolution, when ADC resolution is 
  *         different of 12 bits.
  * @note   To be used with function @ref LL_ADC_GetAnalogWDThresholds().
  *         Example, with a ADC resolution of 8 bits, to get the value of
  *         analog watchdog threshold high (on 8 bits):
  *           < threshold_value_6_bits > = __LL_ADC_ANALOGWD_GET_THRESHOLD_RESOLUTION
  *            (LL_ADC_RESOLUTION_8B,
  *             LL_ADC_GetAnalogWDThresholds(<ADCx param>, LL_ADC_AWD_THRESHOLD_HIGH)
  *            );
  * @param  __ADC_RESOLUTION__ This parameter can be one of the following values:
  *         @arg @ref LL_ADC_RESOLUTION_12B
  *         @arg @ref LL_ADC_RESOLUTION_10B
  *         @arg @ref LL_ADC_RESOLUTION_8B
  *         @arg @ref LL_ADC_RESOLUTION_6B
  * @param  __AWD_THRESHOLD_12_BITS__ Value between Min_Data=0x000 and Max_Data=0xFFF
  * @retval Value between Min_Data=0x000 and Max_Data=0xFFF
  */
#define __LL_ADC_ANALOGWD_GET_THRESHOLD_RESOLUTION(__ADC_RESOLUTION__, __AWD_THRESHOLD_12_BITS__) \
  ((__AWD_THRESHOLD_12_BITS__) >> ((__ADC_RESOLUTION__) >> (ADC_CFGR1_RES_BITOFFSET_POS - 1U )))

/**
  * @brief  Helper macro to get the ADC analog watchdog threshold high
  *         or low from raw value containing both thresholds concatenated.
  * @note   To be used with function @ref LL_ADC_GetAnalogWDThresholds().
  *         Example, to get analog watchdog threshold high from the register raw value:
  *           __LL_ADC_ANALOGWD_THRESHOLDS_HIGH_LOW(LL_ADC_AWD_THRESHOLD_HIGH, <raw_value_with_both_thresholds>);
  * @param  __AWD_THRESHOLD_TYPE__ This parameter can be one of the following values:
  *         @arg @ref LL_ADC_AWD_THRESHOLD_HIGH
  *         @arg @ref LL_ADC_AWD_THRESHOLD_LOW
  * @param  __AWD_THRESHOLDS__ Value between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF
  * @retval Value between Min_Data=0x000 and Max_Data=0xFFF
  */
#define __LL_ADC_ANALOGWD_THRESHOLDS_HIGH_LOW(__AWD_THRESHOLD_TYPE__, __AWD_THRESHOLDS__)       \
  (((__AWD_THRESHOLDS__) >> (((__AWD_THRESHOLD_TYPE__) & ADC_AWD_TRX_BIT_HIGH_MASK) >> ADC_AWD_TRX_BIT_HIGH_SHIFT4)) & LL_ADC_AWD_THRESHOLD_LOW)

/**
  * @brief  Helper macro to select the ADC common instance
  *         to which is belonging the selected ADC instance.
  * @note   ADC common register instance can be used for:
  *         - Set parameters common to several ADC instances
  *         - Multimode (for devices with several ADC instances)
  *         Refer to functions having argument "ADCxy_COMMON" as parameter.
  * @param  __ADCx__ ADC instance
  * @retval ADC common register instance
  */
#define __LL_ADC_COMMON_INSTANCE(__ADCx__)                                     \
  (ADC1_COMMON)

/**
  * @brief  Helper macro to check if all ADC instances sharing the same
  *         ADC common instance are disabled.
  * @note   This check is required by functions with setting conditioned to
  *         ADC state:
  *         All ADC instances of the ADC common group must be disabled.
  *         Refer to functions having argument "ADCxy_COMMON" as parameter.
  * @note   On devices with only 1 ADC common instance, parameter of this macro
  *         is useless and can be ignored (parameter kept for compatibility
  *         with devices featuring several ADC common instances).
  * @param  __ADCXY_COMMON__ ADC common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_ADC_COMMON_INSTANCE() )
  * @retval Value "0" if all ADC instances sharing the same ADC common instance
  *         are disabled.
  *         Value "1" if at least one ADC instance sharing the same ADC common instance
  *         is enabled.
  */
#define __LL_ADC_IS_ENABLED_ALL_COMMON_INSTANCE(__ADCXY_COMMON__)              \
  LL_ADC_IsEnabled(ADC1)

/**
  * @brief  Helper macro to define the ADC conversion data full-scale digital
  *         value corresponding to the selected ADC resolution.
  * @note   ADC conversion data full-scale corresponds to voltage range
  *         determined by analog voltage references Vref+ and Vref-
  *         (refer to reference manual).
  * @param  __ADC_RESOLUTION__ This parameter can be one of the following values:
  *         @arg @ref LL_ADC_RESOLUTION_12B
  *         @arg @ref LL_ADC_RESOLUTION_10B
  *         @arg @ref LL_ADC_RESOLUTION_8B
  *         @arg @ref LL_ADC_RESOLUTION_6B
  * @retval ADC conversion data full-scale digital value (unit: digital value of ADC conversion data)
  */
#define __LL_ADC_DIGITAL_SCALE(__ADC_RESOLUTION__)                             \
  (0xFFFUL >> ((__ADC_RESOLUTION__) >> (ADC_CFGR1_RES_BITOFFSET_POS - 1UL)))

/**
  * @brief  Helper macro to convert the ADC conversion data from
  *         a resolution to another resolution.
  * @param  __DATA__ ADC conversion data to be converted 
  * @param  __ADC_RESOLUTION_CURRENT__ Resolution of the data to be converted
  *         This parameter can be one of the following values:
  *         @arg @ref LL_ADC_RESOLUTION_12B
  *         @arg @ref LL_ADC_RESOLUTION_10B
  *         @arg @ref LL_ADC_RESOLUTION_8B
  *         @arg @ref LL_ADC_RESOLUTION_6B
  * @param  __ADC_RESOLUTION_TARGET__ Resolution of the data after conversion
  *         This parameter can be one of the following values:
  *         @arg @ref LL_ADC_RESOLUTION_12B
  *         @arg @ref LL_ADC_RESOLUTION_10B
  *         @arg @ref LL_ADC_RESOLUTION_8B
  *         @arg @ref LL_ADC_RESOLUTION_6B
  * @retval ADC conversion data to the requested resolution
  */
#define __LL_ADC_CONVERT_DATA_RESOLUTION(__DATA__,\
                                         __ADC_RESOLUTION_CURRENT__,\
                                         __ADC_RESOLUTION_TARGET__)            \
  (((__DATA__)                                                                 \
    << ((__ADC_RESOLUTION_CURRENT__) >> (ADC_CFGR1_RES_BITOFFSET_POS - 1UL)))  \
   >> ((__ADC_RESOLUTION_TARGET__) >> (ADC_CFGR1_RES_BITOFFSET_POS - 1UL))     \
  )

/**
  * @brief  Helper macro to calculate the voltage (unit: mVolt)
  *         corresponding to a ADC conversion data (unit: digital value).
  * @note   Analog reference voltage (Vref+) must be either known from
  *         user board environment or can be calculated using ADC measurement
  *         and ADC helper macro @ref __LL_ADC_CALC_VREFANALOG_VOLTAGE().
  * @param  __VREFANALOG_VOLTAGE__ Analog reference voltage (unit: mV)
  * @param  __ADC_DATA__ ADC conversion data (resolution 12 bits)
  *                       (unit: digital value).
  * @param  __ADC_RESOLUTION__ This parameter can be one of the following values:
  *         @arg @ref LL_ADC_RESOLUTION_12B
  *         @arg @ref LL_ADC_RESOLUTION_10B
  *         @arg @ref LL_ADC_RESOLUTION_8B
  *         @arg @ref LL_ADC_RESOLUTION_6B
  * @retval ADC conversion data equivalent voltage value (unit: mVolt)
  */
#define __LL_ADC_CALC_DATA_TO_VOLTAGE(__VREFANALOG_VOLTAGE__,\
                                      __ADC_DATA__,\
                                      __ADC_RESOLUTION__)                      \
  ((__ADC_DATA__) * (__VREFANALOG_VOLTAGE__)                                   \
   / __LL_ADC_DIGITAL_SCALE(__ADC_RESOLUTION__)                                \
  )

/**
  * @brief  Helper macro to calculate analog reference voltage (Vref+)
  *         (unit: mVolt) from ADC conversion data of internal voltage
  *         reference VrefInt.
  * @note   Computation is using VrefInt calibration value
  *         stored in system memory for each device during production.
  * @note   This voltage depends on user board environment: voltage level
  *         connected to pin Vref+.
  *         On devices with small package, the pin Vref+ is not present
  *         and internally bonded to pin Vdda.
  * @note   On this STM32 serie, calibration data of internal voltage reference
  *         VrefInt corresponds to a resolution of 12 bits,
  *         this is the recommended ADC resolution to convert voltage of
  *         internal voltage reference VrefInt.
  *         Otherwise, this macro performs the processing to scale
  *         ADC conversion data to 12 bits.
  * @param  __VREFINT_ADC_DATA__ ADC conversion data (resolution 12 bits)
  *         of internal voltage reference VrefInt (unit: digital value).
  * @param  __ADC_RESOLUTION__ This parameter can be one of the following values:
  *         @arg @ref LL_ADC_RESOLUTION_12B
  *         @arg @ref LL_ADC_RESOLUTION_10B
  *         @arg @ref LL_ADC_RESOLUTION_8B
  *         @arg @ref LL_ADC_RESOLUTION_6B
  * @retval Analog reference voltage (unit: mV)
  */
#define __LL_ADC_CALC_VREFANALOG_VOLTAGE(__VREFINT_ADC_DATA__,\
                                         __ADC_RESOLUTION__)                   \
  (((uint32_t)(*VREFINT_CAL_ADDR) * VREFINT_CAL_VREF)                          \
    / __LL_ADC_CONVERT_DATA_RESOLUTION((__VREFINT_ADC_DATA__),                 \
                                       (__ADC_RESOLUTION__),                   \
                                       LL_ADC_RESOLUTION_12B)                  \
  )

/**
  * @brief  Helper macro to calculate the temperature (unit: degree Celsius)
  *         from ADC conversion data of internal temperature sensor.
  * @note   Computation is using temperature sensor calibration values
  *         stored in system memory for each device during production.
  * @note   Calculation formula:
  *           Temperature = ((TS_ADC_DATA - TS_CAL1)
  *                           * (TS_CAL2_TEMP - TS_CAL1_TEMP))
  *                         / (TS_CAL2 - TS_CAL1) + TS_CAL1_TEMP
  *           with TS_ADC_DATA = temperature sensor raw data measured by ADC
  *                Avg_Slope = (TS_CAL2 - TS_CAL1)
  *                            / (TS_CAL2_TEMP - TS_CAL1_TEMP)
  *                TS_CAL1   = equivalent TS_ADC_DATA at temperature
  *                            TEMP_DEGC_CAL1 (calibrated in factory)
  *                TS_CAL2   = equivalent TS_ADC_DATA at temperature
  *                            TEMP_DEGC_CAL2 (calibrated in factory)
  *         Caution: Calculation relevancy under reserve that calibration
  *                  parameters are correct (address and data).
  *                  To calculate temperature using temperature sensor
  *                  datasheet typical values (generic values less, therefore
  *                  less accurate than calibrated values),
  *                  use helper macro @ref __LL_ADC_CALC_TEMPERATURE_TYP_PARAMS().
  * @note   As calculation input, the analog reference voltage (Vref+) must be
  *         defined as it impacts the ADC LSB equivalent voltage.
  * @note   Analog reference voltage (Vref+) must be either known from
  *         user board environment or can be calculated using ADC measurement
  *         and ADC helper macro @ref __LL_ADC_CALC_VREFANALOG_VOLTAGE().
  * @note   On this STM32 serie, calibration data of temperature sensor
  *         corresponds to a resolution of 12 bits,
  *         this is the recommended ADC resolution to convert voltage of
  *         temperature sensor.
  *         Otherwise, this macro performs the processing to scale
  *         ADC conversion data to 12 bits.
  * @param  __VREFANALOG_VOLTAGE__  Analog reference voltage (unit: mV)
  * @param  __TEMPSENSOR_ADC_DATA__ ADC conversion data of internal
  *                                 temperature sensor (unit: digital value).
  * @param  __ADC_RESOLUTION__      ADC resolution at which internal temperature
  *                                 sensor voltage has been measured.
  *         This parameter can be one of the following values:
  *         @arg @ref LL_ADC_RESOLUTION_12B
  *         @arg @ref LL_ADC_RESOLUTION_10B
  *         @arg @ref LL_ADC_RESOLUTION_8B
  *         @arg @ref LL_ADC_RESOLUTION_6B
  * @retval Temperature (unit: degree Celsius)
  */
#define __LL_ADC_CALC_TEMPERATURE(__VREFANALOG_VOLTAGE__,\
                                  __TEMPSENSOR_ADC_DATA__,\
                                  __ADC_RESOLUTION__)                              \
  (((( ((int32_t)((__LL_ADC_CONVERT_DATA_RESOLUTION((__TEMPSENSOR_ADC_DATA__),     \
                                                    (__ADC_RESOLUTION__),          \
                                                    LL_ADC_RESOLUTION_12B)         \
                   * (__VREFANALOG_VOLTAGE__))                                     \
                  / TEMPSENSOR_CAL_VREFANALOG)                                     \
        - (int32_t) *TEMPSENSOR_CAL1_ADDR)                                         \
     ) * (int32_t)(TEMPSENSOR_CAL2_TEMP - TEMPSENSOR_CAL1_TEMP)                    \
    ) / (int32_t)((int32_t)*TEMPSENSOR_CAL2_ADDR - (int32_t)*TEMPSENSOR_CAL1_ADDR) \
   ) + TEMPSENSOR_CAL1_TEMP                                                        \
  )

/**
  * @brief  Helper macro to calculate the temperature (unit: degree Celsius)
  *         from ADC conversion data of internal temperature sensor.
  * @note   Computation is using temperature sensor typical values
  *         (refer to device datasheet).
  * @note   Calculation formula:
  *           Temperature = (TS_TYP_CALx_VOLT(uV) - TS_ADC_DATA * Conversion_uV)
  *                         / Avg_Slope + CALx_TEMP
  *           with TS_ADC_DATA      = temperature sensor raw data measured by ADC
  *                                   (unit: digital value)
  *                Avg_Slope        = temperature sensor slope
  *                                   (unit: uV/Degree Celsius)
  *                TS_TYP_CALx_VOLT = temperature sensor digital value at
  *                                   temperature CALx_TEMP (unit: mV)
  *         Caution: Calculation relevancy under reserve the temperature sensor
  *                  of the current device has characteristics in line with
  *                  datasheet typical values.
  *                  If temperature sensor calibration values are available on
  *                  on this device (presence of macro __LL_ADC_CALC_TEMPERATURE()),
  *                  temperature calculation will be more accurate using
  *                  helper macro @ref __LL_ADC_CALC_TEMPERATURE().
  * @note   As calculation input, the analog reference voltage (Vref+) must be
  *         defined as it impacts the ADC LSB equivalent voltage.
  * @note   Analog reference voltage (Vref+) must be either known from
  *         user board environment or can be calculated using ADC measurement
  *         and ADC helper macro @ref __LL_ADC_CALC_VREFANALOG_VOLTAGE().
  * @note   ADC measurement data must correspond to a resolution of 12 bits
  *         (full scale digital value 4095). If not the case, the data must be
  *         preliminarily rescaled to an equivalent resolution of 12 bits.
  * @param  __TEMPSENSOR_TYP_AVGSLOPE__   Device datasheet data: Temperature sensor slope typical value (unit: uV/DegCelsius).
  *                                       On STM32G0, refer to device datasheet parameter "Avg_Slope".
  * @param  __TEMPSENSOR_TYP_CALX_V__     Device datasheet data: Temperature sensor voltage typical value (at temperature and Vref+ defined in parameters below) (unit: mV).
  *                                       On STM32G0, refer to device datasheet parameter "V30" (corresponding to TS_CAL1).
  * @param  __TEMPSENSOR_CALX_TEMP__      Device datasheet data: Temperature at which temperature sensor voltage (see parameter above) is corresponding (unit: mV)
  * @param  __VREFANALOG_VOLTAGE__        Analog voltage reference (Vref+) voltage (unit: mV)
  * @param  __TEMPSENSOR_ADC_DATA__       ADC conversion data of internal temperature sensor (unit: digital value).
  * @param  __ADC_RESOLUTION__            ADC resolution at which internal temperature sensor voltage has been measured.
  *         This parameter can be one of the following values:
  *         @arg @ref LL_ADC_RESOLUTION_12B
  *         @arg @ref LL_ADC_RESOLUTION_10B
  *         @arg @ref LL_ADC_RESOLUTION_8B
  *         @arg @ref LL_ADC_RESOLUTION_6B
  * @retval Temperature (unit: degree Celsius)
  */
#define __LL_ADC_CALC_TEMPERATURE_TYP_PARAMS(__TEMPSENSOR_TYP_AVGSLOPE__,\
                                             __TEMPSENSOR_TYP_CALX_V__,\
                                             __TEMPSENSOR_CALX_TEMP__,\
                                             __VREFANALOG_VOLTAGE__,\
                                             __TEMPSENSOR_ADC_DATA__,\
                                             __ADC_RESOLUTION__)               \
  ((( (                                                                        \
       (int32_t)((((__TEMPSENSOR_ADC_DATA__) * (__VREFANALOG_VOLTAGE__))       \
                  / __LL_ADC_DIGITAL_SCALE(__ADC_RESOLUTION__))                \
                 * 1000UL)                                                     \
       -                                                                       \
       (int32_t)(((__TEMPSENSOR_TYP_CALX_V__))                                 \
                 * 1000UL)                                                     \
      )                                                                        \
    ) / (int32_t)(__TEMPSENSOR_TYP_AVGSLOPE__)                                 \
   ) + (int32_t)(__TEMPSENSOR_CALX_TEMP__)                                     \
  )

/**
  * @}
  */

/**
  * @}
  */


/* Exported functions --------------------------------------------------------*/
/** @defgroup ADC_LL_Exported_Functions ADC Exported Functions
  * @{
  */

/** @defgroup ADC_LL_EF_DMA_Management ADC DMA management
  * @{
  */
/* Note: LL ADC functions to set DMA transfer are located into sections of    */
/*       configuration of ADC instance, groups and multimode (if available):  */
/*       @ref LL_ADC_REG_SetDMATransfer(), ...                                */

/**
  * @brief  Function to help to configure DMA transfer from ADC: retrieve the
  *         ADC register address from ADC instance and a list of ADC registers
  *         intended to be used (most commonly) with DMA transfer.
  * @note   These ADC registers are data registers:
  *         when ADC conversion data is available in ADC data registers,
  *         ADC generates a DMA transfer request.
  * @note   This macro is intended to be used with LL DMA driver, refer to
  *         function "LL_DMA_ConfigAddresses()".
  *         Example:
  *           LL_DMA_ConfigAddresses(DMA1,
  *                                  LL_DMA_CHANNEL_1,
  *                                  LL_ADC_DMA_GetRegAddr(ADC1, LL_ADC_DMA_REG_REGULAR_DATA),
  *                                  (uint32_t)&< array or variable >,
  *                                  LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
  * @note   For devices with several ADC: in multimode, some devices
  *         use a different data register outside of ADC instance scope
  *         (common data register). This macro manages this register difference,
  *         only ADC instance has to be set as parameter.
  * @rmtoll DR       DATA           LL_ADC_DMA_GetRegAddr
  * @param  ADCx ADC instance
  * @param  Register This parameter can be one of the following values:
  *         @arg @ref LL_ADC_DMA_REG_REGULAR_DATA
  * @retval ADC register address
  */
__STATIC_INLINE uint32_t LL_ADC_DMA_GetRegAddr(ADC_TypeDef *ADCx, uint32_t Register)
{
  /* Prevent unused argument(s) compilation warning */
  (void)(Register);
  
  /* Retrieve address of register DR */
  return (uint32_t)&(ADCx->DR);
}

/**
  * @}
  */

/** @defgroup ADC_LL_EF_Configuration_ADC_Common Configuration of ADC hierarchical scope: common to several ADC instances
  * @{
  */

/**
  * @brief  Set parameter common to several ADC: Clock source and prescaler.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         All ADC instances of the ADC common group must be disabled.
  *         This check can be done with function @ref LL_ADC_IsEnabled() for each
  *         ADC instance or by using helper macro helper macro
  *         @ref __LL_ADC_IS_ENABLED_ALL_COMMON_INSTANCE().
  * @rmtoll CCR      PRESC          LL_ADC_SetCommonClock
  * @param  ADCxy_COMMON ADC common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_ADC_COMMON_INSTANCE() )
  * @param  CommonClock This parameter can be one of the following values:
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV1   (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV2   (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV4   (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV6   (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV8   (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV10  (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV12  (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV16  (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV32  (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV64  (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV128 (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV256 (1)
  *
  *         (1) ADC common clock asynchonous prescaler is applied to 
  *             each ADC instance if the corresponding ADC instance clock  
  *             is set to clock source asynchronous.
  *             (refer to function @ref LL_ADC_SetClock() ).
  * @retval None
  */
__STATIC_INLINE void LL_ADC_SetCommonClock(ADC_Common_TypeDef *ADCxy_COMMON, uint32_t CommonClock)
{
  MODIFY_REG(ADCxy_COMMON->CCR, ADC_CCR_PRESC, CommonClock);
}

/**
  * @brief  Get parameter common to several ADC: Clock source and prescaler.
  * @rmtoll CCR      PRESC          LL_ADC_GetCommonClock
  * @param  ADCxy_COMMON ADC common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_ADC_COMMON_INSTANCE() )
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV1   (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV2   (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV4   (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV6   (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV8   (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV10  (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV12  (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV16  (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV32  (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV64  (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV128 (1)
  *         @arg @ref LL_ADC_CLOCK_ASYNC_DIV256 (1)
  *
  *         (1) ADC common clock asynchonous prescaler is applied to 
  *             each ADC instance if the corresponding ADC instance clock  
  *             is set to clock source asynchronous.
  *             (refer to function @ref LL_ADC_SetClock() ).
  */
__STATIC_INLINE uint32_t LL_ADC_GetCommonClock(ADC_Common_TypeDef *ADCxy_COMMON)
{
  return (uint32_t)(READ_BIT(ADCxy_COMMON->CCR, ADC_CCR_PRESC));
}

/**
  * @brief  Set parameter common to several ADC: Clock low frequency mode.
  *         Refer to reference manual for alignments formats
  *         dependencies to ADC resolutions.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CCR      LFMEN          LL_ADC_SetCommonFrequencyMode
  * @param  ADCxy_COMMON ADC common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_ADC_COMMON_INSTANCE() )
  * @param  Resolution This parameter can be one of the following values:
  *         @arg @ref LL_ADC_CLOCK_FREQ_MODE_HIGH
  *         @arg @ref LL_ADC_CLOCK_FREQ_MODE_LOW
  * @retval None
  */
__STATIC_INLINE void LL_ADC_SetCommonFrequencyMode(ADC_Common_TypeDef *ADCxy_COMMON, uint32_t Resolution)
{
  MODIFY_REG(ADCxy_COMMON->CCR, ADC_CCR_LFMEN, Resolution);
}

/**
  * @brief  Get parameter common to several ADC: Clock low frequency mode.
  *         Refer to reference manual for alignments formats
  *         dependencies to ADC resolutions.
  * @rmtoll CCR      LFMEN          LL_ADC_GetCommonFrequencyMode
  * @param  ADCxy_COMMON ADC common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_ADC_COMMON_INSTANCE() )
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_CLOCK_FREQ_MODE_HIGH
  *         @arg @ref LL_ADC_CLOCK_FREQ_MODE_LOW
  */
__STATIC_INLINE uint32_t LL_ADC_GetCommonFrequencyMode(ADC_Common_TypeDef *ADCxy_COMMON)
{
  return (uint32_t)(READ_BIT(ADCxy_COMMON->CCR, ADC_CCR_LFMEN));
}

/**
  * @brief  Set parameter common to several ADC: measurement path to internal
  *         channels (VrefInt, temperature sensor, ...).
  * @note   One or several values can be selected.
  *         Example: (LL_ADC_PATH_INTERNAL_VREFINT |
  *                   LL_ADC_PATH_INTERNAL_TEMPSENSOR)
  * @note   Stabilization time of measurement path to internal channel:
  *         After enabling internal paths, before starting ADC conversion,
  *         a delay is required for internal voltage reference and
  *         temperature sensor stabilization time.
  *         Refer to device datasheet.
  *         Refer to literal @ref LL_ADC_DELAY_VREFINT_STAB_US.
  *         Refer to literal @ref LL_ADC_DELAY_TEMPSENSOR_STAB_US.
  * @note   ADC internal channel sampling time constraint:
  *         For ADC conversion of internal channels,
  *         a sampling time minimum value is required.
  *         Refer to device datasheet.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         All ADC instances of the ADC common group must be disabled.
  *         This check can be done with function @ref LL_ADC_IsEnabled() for each
  *         ADC instance or by using helper macro helper macro
  *         @ref __LL_ADC_IS_ENABLED_ALL_COMMON_INSTANCE().
  * @rmtoll CCR      VREFEN         LL_ADC_SetCommonPathInternalCh\n
  *         CCR      TSEN           LL_ADC_SetCommonPathInternalCh\n
  *         CCR      VBATEN         LL_ADC_SetCommonPathInternalCh
  * @param  ADCxy_COMMON ADC common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_ADC_COMMON_INSTANCE() )
  * @param  PathInternal This parameter can be a combination of the following values:
  *         @arg @ref LL_ADC_PATH_INTERNAL_NONE
  *         @arg @ref LL_ADC_PATH_INTERNAL_VREFINT
  *         @arg @ref LL_ADC_PATH_INTERNAL_TEMPSENSOR
  *         @arg @ref LL_ADC_PATH_INTERNAL_VBAT
  * @retval None
  */
__STATIC_INLINE void LL_ADC_SetCommonPathInternalCh(ADC_Common_TypeDef *ADCxy_COMMON, uint32_t PathInternal)
{
  MODIFY_REG(ADCxy_COMMON->CCR, ADC_CCR_VREFEN | ADC_CCR_TSEN | ADC_CCR_VBATEN, PathInternal);
}

/**
  * @brief  Get parameter common to several ADC: measurement path to internal
  *         channels (VrefInt, temperature sensor, ...).
  * @note   One or several values can be selected.
  *         Example: (LL_ADC_PATH_INTERNAL_VREFINT |
  *                   LL_ADC_PATH_INTERNAL_TEMPSENSOR)
  * @rmtoll CCR      VREFEN         LL_ADC_GetCommonPathInternalCh\n
  *         CCR      TSEN           LL_ADC_GetCommonPathInternalCh\n
  *         CCR      VBATEN         LL_ADC_GetCommonPathInternalCh
  * @param  ADCxy_COMMON ADC common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_ADC_COMMON_INSTANCE() )
  * @retval Returned value can be a combination of the following values:
  *         @arg @ref LL_ADC_PATH_INTERNAL_NONE
  *         @arg @ref LL_ADC_PATH_INTERNAL_VREFINT
  *         @arg @ref LL_ADC_PATH_INTERNAL_TEMPSENSOR
  *         @arg @ref LL_ADC_PATH_INTERNAL_VBAT
  */
__STATIC_INLINE uint32_t LL_ADC_GetCommonPathInternalCh(ADC_Common_TypeDef *ADCxy_COMMON)
{
  return (uint32_t)(READ_BIT(ADCxy_COMMON->CCR, ADC_CCR_VREFEN | ADC_CCR_TSEN | ADC_CCR_VBATEN));
}

/**
  * @}
  */

/** @defgroup ADC_LL_EF_Configuration_ADC_Instance Configuration of ADC hierarchical scope: ADC instance
  * @{
  */

/**
  * @brief  Set ADC instance clock source and prescaler.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled.
  * @rmtoll CFGR2    CKMODE         LL_ADC_SetClock
  * @param  ADCx ADC instance
  * @param  ClockSource This parameter can be one of the following values:
  *         @arg @ref LL_ADC_CLOCK_SYNC_PCLK_DIV4
  *         @arg @ref LL_ADC_CLOCK_SYNC_PCLK_DIV2
  *         @arg @ref LL_ADC_CLOCK_SYNC_PCLK_DIV1 (2)
  *         @arg @ref LL_ADC_CLOCK_ASYNC (1)
  *         
  *         (1) Asynchronous clock prescaler can be configured using
  *             function @ref LL_ADC_SetCommonClock().\n
  *         (2) Caution: This parameter has some clock ratio constraints:
  *             This configuration must be enabled only if PCLK has a 50%
  *             duty clock cycle (APB prescaler configured inside the RCC
  *             must be bypassed and the system clock must by 50% duty
  *             cycle).
  *             Refer to reference manual.
  * @retval None
  */
__STATIC_INLINE void LL_ADC_SetClock(ADC_TypeDef *ADCx, uint32_t ClockSource)
{
  MODIFY_REG(ADCx->CFGR2, ADC_CFGR2_CKMODE, ClockSource);
}

/**
  * @brief  Get ADC instance clock source and prescaler.
  * @rmtoll CFGR2    CKMODE         LL_ADC_GetClock
  * @param  ADCx ADC instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_CLOCK_SYNC_PCLK_DIV4
  *         @arg @ref LL_ADC_CLOCK_SYNC_PCLK_DIV2
  *         @arg @ref LL_ADC_CLOCK_SYNC_PCLK_DIV1 (2)
  *         @arg @ref LL_ADC_CLOCK_ASYNC (1)
  *         
  *         (1) Asynchronous clock prescaler can be retrieved using
  *             function @ref LL_ADC_GetCommonClock().\n
  *         (2) Caution: This parameter has some clock ratio constraints:
  *             This configuration must be enabled only if PCLK has a 50%
  *             duty clock cycle (APB prescaler configured inside the RCC
  *             must be bypassed and the system clock must by 50% duty
  *             cycle).
  *             Refer to reference manual.
  */
__STATIC_INLINE uint32_t LL_ADC_GetClock(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CFGR2, ADC_CFGR2_CKMODE));
}

/**
  * @brief  Set ADC calibration factor in the mode single-ended
  *         or differential (for devices with differential mode available).
  * @note   This function is intended to set calibration parameters
  *         without having to perform a new calibration using
  *         @ref LL_ADC_StartCalibration().
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be enabled, without calibration on going, without conversion
  *         on going on group regular.
  * @rmtoll CALFACT  CALFACT        LL_ADC_SetCalibrationFactor
  * @param  ADCx ADC instance
  * @param  CalibrationFactor Value between Min_Data=0x00 and Max_Data=0x7F
  * @retval None
  */
__STATIC_INLINE void LL_ADC_SetCalibrationFactor(ADC_TypeDef *ADCx, uint32_t CalibrationFactor)
{
  MODIFY_REG(ADCx->CALFACT,
             ADC_CALFACT_CALFACT,
             CalibrationFactor);
}

/**
  * @brief  Get ADC calibration factor in the mode single-ended
  *         or differential (for devices with differential mode available).
  * @note   Calibration factors are set by hardware after performing
  *         a calibration run using function @ref LL_ADC_StartCalibration().
  * @rmtoll CALFACT  CALFACT        LL_ADC_GetCalibrationFactor
  * @param  ADCx ADC instance
  * @retval Value between Min_Data=0x00 and Max_Data=0x7F
  */
__STATIC_INLINE uint32_t LL_ADC_GetCalibrationFactor(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CALFACT, ADC_CALFACT_CALFACT));
}

/**
  * @brief  Set ADC resolution.
  *         Refer to reference manual for alignments formats
  *         dependencies to ADC resolutions.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR1    RES            LL_ADC_SetResolution
  * @param  ADCx ADC instance
  * @param  Resolution This parameter can be one of the following values:
  *         @arg @ref LL_ADC_RESOLUTION_12B
  *         @arg @ref LL_ADC_RESOLUTION_10B
  *         @arg @ref LL_ADC_RESOLUTION_8B
  *         @arg @ref LL_ADC_RESOLUTION_6B
  * @retval None
  */
__STATIC_INLINE void LL_ADC_SetResolution(ADC_TypeDef *ADCx, uint32_t Resolution)
{
  MODIFY_REG(ADCx->CFGR1, ADC_CFGR1_RES, Resolution);
}

/**
  * @brief  Get ADC resolution.
  *         Refer to reference manual for alignments formats
  *         dependencies to ADC resolutions.
  * @rmtoll CFGR1    RES            LL_ADC_GetResolution
  * @param  ADCx ADC instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_RESOLUTION_12B
  *         @arg @ref LL_ADC_RESOLUTION_10B
  *         @arg @ref LL_ADC_RESOLUTION_8B
  *         @arg @ref LL_ADC_RESOLUTION_6B
  */
__STATIC_INLINE uint32_t LL_ADC_GetResolution(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CFGR1, ADC_CFGR1_RES));
}

/**
  * @brief  Set ADC conversion data alignment.
  * @note   Refer to reference manual for alignments formats
  *         dependencies to ADC resolutions.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR1    ALIGN          LL_ADC_SetDataAlignment
  * @param  ADCx ADC instance
  * @param  DataAlignment This parameter can be one of the following values:
  *         @arg @ref LL_ADC_DATA_ALIGN_RIGHT
  *         @arg @ref LL_ADC_DATA_ALIGN_LEFT
  * @retval None
  */
__STATIC_INLINE void LL_ADC_SetDataAlignment(ADC_TypeDef *ADCx, uint32_t DataAlignment)
{
  MODIFY_REG(ADCx->CFGR1, ADC_CFGR1_ALIGN, DataAlignment);
}

/**
  * @brief  Get ADC conversion data alignment.
  * @note   Refer to reference manual for alignments formats
  *         dependencies to ADC resolutions.
  * @rmtoll CFGR1    ALIGN          LL_ADC_GetDataAlignment
  * @param  ADCx ADC instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_DATA_ALIGN_RIGHT
  *         @arg @ref LL_ADC_DATA_ALIGN_LEFT
  */
__STATIC_INLINE uint32_t LL_ADC_GetDataAlignment(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CFGR1, ADC_CFGR1_ALIGN));
}

/**
  * @brief  Set ADC low power mode.
  * @note   Description of ADC low power modes:
  *         - ADC low power mode "auto wait": Dynamic low power mode,
  *           ADC conversions occurrences are limited to the minimum necessary
  *           in order to reduce power consumption.
  *           New ADC conversion starts only when the previous
  *           unitary conversion data (for ADC group regular)
  *           has been retrieved by user software.
  *           In the meantime, ADC remains idle: does not performs any
  *           other conversion.
  *           This mode allows to automatically adapt the ADC conversions
  *           triggers to the speed of the software that reads the data.
  *           Moreover, this avoids risk of overrun for low frequency
  *           applications.
  *           How to use this low power mode:
  *           - Do not use with interruption or DMA since these modes
  *             have to clear immediately the EOC flag to free the
  *             IRQ vector sequencer.
  *           - Do use with polling: 1. Start conversion,
  *             2. Later on, when conversion data is needed: poll for end of
  *             conversion  to ensure that conversion is completed and
  *             retrieve ADC conversion data. This will trig another
  *             ADC conversion start.
  *         - ADC low power mode "auto power-off" (feature available on
  *           this device if parameter LL_ADC_LP_AUTOPOWEROFF is available):
  *           the ADC automatically powers-off after a conversion and
  *           automatically wakes up when a new conversion is triggered
  *           (with startup time between trigger and start of sampling).
  *           This feature can be combined with low power mode "auto wait".
  * @note   With ADC low power mode "auto wait", the ADC conversion data read
  *         is corresponding to previous ADC conversion start, independently
  *         of delay during which ADC was idle.
  *         Therefore, the ADC conversion data may be outdated: does not
  *         correspond to the current voltage level on the selected
  *         ADC channel.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR1    WAIT           LL_ADC_SetLowPowerMode\n
  *         CFGR1    AUTOFF         LL_ADC_SetLowPowerMode
  * @param  ADCx ADC instance
  * @param  LowPowerMode This parameter can be one of the following values:
  *         @arg @ref LL_ADC_LP_MODE_NONE
  *         @arg @ref LL_ADC_LP_AUTOWAIT
  *         @arg @ref LL_ADC_LP_AUTOPOWEROFF
  *         @arg @ref LL_ADC_LP_AUTOWAIT_AUTOPOWEROFF
  * @retval None
  */
__STATIC_INLINE void LL_ADC_SetLowPowerMode(ADC_TypeDef *ADCx, uint32_t LowPowerMode)
{
  MODIFY_REG(ADCx->CFGR1, (ADC_CFGR1_WAIT | ADC_CFGR1_AUTOFF), LowPowerMode);
}

/**
  * @brief  Get ADC low power mode:
  * @note   Description of ADC low power modes:
  *         - ADC low power mode "auto wait": Dynamic low power mode,
  *           ADC conversions occurrences are limited to the minimum necessary
  *           in order to reduce power consumption.
  *           New ADC conversion starts only when the previous
  *           unitary conversion data (for ADC group regular)
  *           has been retrieved by user software.
  *           In the meantime, ADC remains idle: does not performs any
  *           other conversion.
  *           This mode allows to automatically adapt the ADC conversions
  *           triggers to the speed of the software that reads the data.
  *           Moreover, this avoids risk of overrun for low frequency
  *           applications.
  *           How to use this low power mode:
  *           - Do not use with interruption or DMA since these modes
  *             have to clear immediately the EOC flag to free the
  *             IRQ vector sequencer.
  *           - Do use with polling: 1. Start conversion,
  *             2. Later on, when conversion data is needed: poll for end of
  *             conversion  to ensure that conversion is completed and
  *             retrieve ADC conversion data. This will trig another
  *             ADC conversion start.
  *         - ADC low power mode "auto power-off" (feature available on
  *           this device if parameter LL_ADC_LP_AUTOPOWEROFF is available):
  *           the ADC automatically powers-off after a conversion and
  *           automatically wakes up when a new conversion is triggered
  *           (with startup time between trigger and start of sampling).
  *           This feature can be combined with low power mode "auto wait".
  * @note   With ADC low power mode "auto wait", the ADC conversion data read
  *         is corresponding to previous ADC conversion start, independently
  *         of delay during which ADC was idle.
  *         Therefore, the ADC conversion data may be outdated: does not
  *         correspond to the current voltage level on the selected
  *         ADC channel.
  * @rmtoll CFGR1    WAIT           LL_ADC_GetLowPowerMode\n
  *         CFGR1    AUTOFF         LL_ADC_GetLowPowerMode
  * @param  ADCx ADC instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_LP_MODE_NONE
  *         @arg @ref LL_ADC_LP_AUTOWAIT
  *         @arg @ref LL_ADC_LP_AUTOPOWEROFF
  *         @arg @ref LL_ADC_LP_AUTOWAIT_AUTOPOWEROFF
  */
__STATIC_INLINE uint32_t LL_ADC_GetLowPowerMode(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CFGR1, (ADC_CFGR1_WAIT | ADC_CFGR1_AUTOFF)));
}

/**
  * @brief  Set ADC trigger frequency mode.
  * @note   ADC trigger frequency mode must be set to low frequency when
  *         a duration is exceeded before ADC conversion start trigger event
  *         (between ADC enable and ADC conversion start trigger event
  *         or between two ADC conversion start trigger event).
  *         Duration value: Refer to device datasheet, parameter "tIdle".
  * @note   When ADC trigger frequency mode is set to low frequency, 
  *         some rearm cycles are inserted before performing ADC conversion
  *         start, inducing a delay of 2 ADC clock cycles.
  * @note   Usage of ADC trigger frequency mode with ADC low power mode:
  *         - Low power mode auto wait: Only the first ADC conversion
  *           start trigger inserts the rearm delay.
  *         - Low power mode auto power-off: ADC trigger frequency mode
  *           is discarded.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR2    LFTRIG         LL_ADC_SetTriggerFrequencyMode
  * @param  ADCx ADC instance
  * @param  TriggerFrequencyMode This parameter can be one of the following values:
  *         @arg @ref LL_ADC_TRIGGER_FREQ_HIGH
  *         @arg @ref LL_ADC_TRIGGER_FREQ_LOW
  * @retval None
  */
__STATIC_INLINE void LL_ADC_SetTriggerFrequencyMode(ADC_TypeDef *ADCx, uint32_t TriggerFrequencyMode)
{
  MODIFY_REG(ADCx->CFGR2, ADC_CFGR2_LFTRIG, TriggerFrequencyMode);
}

/**
  * @brief  Get ADC trigger frequency mode.
  * @rmtoll CFGR2    LFTRIG         LL_ADC_GetTriggerFrequencyMode
  * @param  ADCx ADC instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_TRIGGER_FREQ_HIGH
  *         @arg @ref LL_ADC_TRIGGER_FREQ_LOW
  */
__STATIC_INLINE uint32_t LL_ADC_GetTriggerFrequencyMode(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CFGR2, ADC_CFGR2_LFTRIG));
}

/**
  * @brief  Set sampling time common to a group of channels.
  * @note   Unit: ADC clock cycles.
  * @note   On this STM32 serie, sampling time scope is on ADC instance:
  *         Sampling time common to all channels.
  *         (on some other STM32 families, sampling time is channel wise)
  * @note   In case of internal channel (VrefInt, TempSensor, ...) to be
  *         converted:
  *         sampling time constraints must be respected (sampling time can be
  *         adjusted in function of ADC clock frequency and sampling time
  *         setting).
  *         Refer to device datasheet for timings values (parameters TS_vrefint,
  *         TS_temp, ...).
  * @note   Conversion time is the addition of sampling time and processing time.
  *         On this STM32 serie, ADC processing time is:
  *         - 12.5 ADC clock cycles at ADC resolution 12 bits
  *         - 10.5 ADC clock cycles at ADC resolution 10 bits
  *         - 8.5 ADC clock cycles at ADC resolution 8 bits
  *         - 6.5 ADC clock cycles at ADC resolution 6 bits
  * @note   In case of ADC conversion of internal channel (VrefInt,
  *         temperature sensor, ...), a sampling time minimum value
  *         is required.
  *         Refer to device datasheet.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll SMPR     SMP1           LL_ADC_SetSamplingTimeCommonChannels\n
  * @rmtoll SMPR     SMP2           LL_ADC_SetSamplingTimeCommonChannels
  * @param  ADCx ADC instance
  * @param  SamplingTimeY This parameter can be one of the following values:
  *         @arg @ref LL_ADC_SAMPLINGTIME_COMMON_1
  *         @arg @ref LL_ADC_SAMPLINGTIME_COMMON_2
  * @param  SamplingTime This parameter can be one of the following values:
  *         @arg @ref LL_ADC_SAMPLINGTIME_1CYCLE_5
  *         @arg @ref LL_ADC_SAMPLINGTIME_3CYCLES_5
  *         @arg @ref LL_ADC_SAMPLINGTIME_7CYCLES_5
  *         @arg @ref LL_ADC_SAMPLINGTIME_12CYCLES_5
  *         @arg @ref LL_ADC_SAMPLINGTIME_19CYCLES_5
  *         @arg @ref LL_ADC_SAMPLINGTIME_39CYCLES_5
  *         @arg @ref LL_ADC_SAMPLINGTIME_79CYCLES_5
  *         @arg @ref LL_ADC_SAMPLINGTIME_160CYCLES_5
  * @retval None
  */
__STATIC_INLINE void LL_ADC_SetSamplingTimeCommonChannels(ADC_TypeDef *ADCx, uint32_t SamplingTimeY, uint32_t SamplingTime)
{
  MODIFY_REG(ADCx->SMPR,
             ADC_SMPR_SMP1 << (SamplingTimeY & ADC_SAMPLING_TIME_SMP_SHIFT_MASK),
             SamplingTime << (SamplingTimeY & ADC_SAMPLING_TIME_SMP_SHIFT_MASK));
}

/**
  * @brief  Get sampling time common to a group of channels.
  * @note   Unit: ADC clock cycles.
  * @note   On this STM32 serie, sampling time scope is on ADC instance:
  *         Sampling time common to all channels.
  *         (on some other STM32 families, sampling time is channel wise)
  * @note   Conversion time is the addition of sampling time and processing time.
  *         Refer to reference manual for ADC processing time of
  *         this STM32 serie.
  * @rmtoll SMPR     SMP1           LL_ADC_GetSamplingTimeCommonChannels\n
  * @rmtoll SMPR     SMP2           LL_ADC_GetSamplingTimeCommonChannels
  * @param  ADCx ADC instance
  * @param  SamplingTimeY This parameter can be one of the following values:
  *         @arg @ref LL_ADC_SAMPLINGTIME_COMMON_1
  *         @arg @ref LL_ADC_SAMPLINGTIME_COMMON_2
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_SAMPLINGTIME_1CYCLE_5
  *         @arg @ref LL_ADC_SAMPLINGTIME_3CYCLES_5
  *         @arg @ref LL_ADC_SAMPLINGTIME_7CYCLES_5
  *         @arg @ref LL_ADC_SAMPLINGTIME_12CYCLES_5
  *         @arg @ref LL_ADC_SAMPLINGTIME_19CYCLES_5
  *         @arg @ref LL_ADC_SAMPLINGTIME_39CYCLES_5
  *         @arg @ref LL_ADC_SAMPLINGTIME_79CYCLES_5
  *         @arg @ref LL_ADC_SAMPLINGTIME_160CYCLES_5
  */
__STATIC_INLINE uint32_t LL_ADC_GetSamplingTimeCommonChannels(ADC_TypeDef *ADCx, uint32_t SamplingTimeY)
{
  return (uint32_t)((READ_BIT(ADCx->SMPR, ADC_SMPR_SMP1 << (SamplingTimeY & ADC_SAMPLING_TIME_SMP_SHIFT_MASK)))
                    >> (SamplingTimeY & ADC_SAMPLING_TIME_SMP_SHIFT_MASK));
}

/**
  * @}
  */

/** @defgroup ADC_LL_EF_Configuration_ADC_Group_Regular Configuration of ADC hierarchical scope: group regular
  * @{
  */

/**
  * @brief  Set ADC group regular conversion trigger source:
  *         internal (SW start) or from external IP (timer event,
  *         external interrupt line).
  * @note   On this STM32 serie, setting trigger source to external trigger
  *         also set trigger polarity to rising edge 
  *         (default setting for compatibility with some ADC on other
  *         STM32 families having this setting set by HW default value).
  *         In case of need to modify trigger edge, use
  *         function @ref LL_ADC_REG_SetTriggerEdge().
  * @note   On this STM32 serie, ADC trigger frequency mode must be set
  *         in function of frequency of ADC group regular conversion trigger.
  *         Refer to description of function 
  *         @ref LL_ADC_SetTriggerFrequencyMode().
  * @note   Availability of parameters of trigger sources from timer 
  *         depends on timers availability on the selected device.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR1    EXTSEL         LL_ADC_REG_SetTriggerSource\n
  *         CFGR1    EXTEN          LL_ADC_REG_SetTriggerSource
  * @param  ADCx ADC instance
  * @param  TriggerSource This parameter can be one of the following values:
  *         @arg @ref LL_ADC_REG_TRIG_SOFTWARE
  *         @arg @ref LL_ADC_REG_TRIG_EXT_TIM1_TRGO2
  *         @arg @ref LL_ADC_REG_TRIG_EXT_TIM1_CH4
  *         @arg @ref LL_ADC_REG_TRIG_EXT_TIM2_TRGO   (1)
  *         @arg @ref LL_ADC_REG_TRIG_EXT_TIM3_TRGO
  *         @arg @ref LL_ADC_REG_TRIG_EXT_TIM6_TRGO   (1)
  *         @arg @ref LL_ADC_REG_TRIG_EXT_TIM15_TRGO  (1)
  *         @arg @ref LL_ADC_REG_TRIG_EXT_EXTI_LINE11
  *
  *         (1) On STM32G0, parameter not available on all devices
  * @retval None
  */
__STATIC_INLINE void LL_ADC_REG_SetTriggerSource(ADC_TypeDef *ADCx, uint32_t TriggerSource)
{
  MODIFY_REG(ADCx->CFGR1, ADC_CFGR1_EXTEN | ADC_CFGR1_EXTSEL, TriggerSource);
}

/**
  * @brief  Get ADC group regular conversion trigger source:
  *         internal (SW start) or from external IP (timer event,
  *         external interrupt line).
  * @note   To determine whether group regular trigger source is
  *         internal (SW start) or external, without detail
  *         of which peripheral is selected as external trigger,
  *         (equivalent to 
  *         "if(LL_ADC_REG_GetTriggerSource(ADC1) == LL_ADC_REG_TRIG_SOFTWARE)")
  *         use function @ref LL_ADC_REG_IsTriggerSourceSWStart.
  * @note   Availability of parameters of trigger sources from timer 
  *         depends on timers availability on the selected device.
  * @rmtoll CFGR1    EXTSEL         LL_ADC_REG_GetTriggerSource\n
  *         CFGR1    EXTEN          LL_ADC_REG_GetTriggerSource
  * @param  ADCx ADC instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_REG_TRIG_SOFTWARE
  *         @arg @ref LL_ADC_REG_TRIG_EXT_TIM1_TRGO2
  *         @arg @ref LL_ADC_REG_TRIG_EXT_TIM1_CH4
  *         @arg @ref LL_ADC_REG_TRIG_EXT_TIM2_TRGO   (1)
  *         @arg @ref LL_ADC_REG_TRIG_EXT_TIM3_TRGO
  *         @arg @ref LL_ADC_REG_TRIG_EXT_TIM6_TRGO   (1)
  *         @arg @ref LL_ADC_REG_TRIG_EXT_TIM15_TRGO  (1)
  *         @arg @ref LL_ADC_REG_TRIG_EXT_EXTI_LINE11
  *
  *         (1) On STM32G0, parameter not available on all devices
  */
__STATIC_INLINE uint32_t LL_ADC_REG_GetTriggerSource(ADC_TypeDef *ADCx)
{
  register uint32_t TriggerSource = READ_BIT(ADCx->CFGR1, ADC_CFGR1_EXTSEL | ADC_CFGR1_EXTEN);
  
  /* Value for shift of {0; 4; 8; 12} depending on value of bitfield          */
  /* corresponding to ADC_CFGR1_EXTEN {0; 1; 2; 3}.                           */
  register uint32_t ShiftExten = ((TriggerSource & ADC_CFGR1_EXTEN) >> (ADC_REG_TRIG_EXTEN_BITOFFSET_POS - 2UL));
  
  /* Set bitfield corresponding to ADC_CFGR1_EXTEN and ADC_CFGR1_EXTSEL       */
  /* to match with triggers literals definition.                              */
  return ((TriggerSource
           & (ADC_REG_TRIG_SOURCE_MASK >> ShiftExten) & ADC_CFGR1_EXTSEL)
          | ((ADC_REG_TRIG_EDGE_MASK >> ShiftExten) & ADC_CFGR1_EXTEN)
         );
}

/**
  * @brief  Get ADC group regular conversion trigger source internal (SW start)
  *         or external.
  * @note   In case of group regular trigger source set to external trigger,
  *         to determine which peripheral is selected as external trigger,
  *         use function @ref LL_ADC_REG_GetTriggerSource().
  * @rmtoll CFGR1    EXTEN          LL_ADC_REG_IsTriggerSourceSWStart
  * @param  ADCx ADC instance
  * @retval Value "0" if trigger source external trigger
  *         Value "1" if trigger source SW start.
  */
__STATIC_INLINE uint32_t LL_ADC_REG_IsTriggerSourceSWStart(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->CFGR1, ADC_CFGR1_EXTEN) == (LL_ADC_REG_TRIG_SOFTWARE & ADC_CFGR1_EXTEN)) ? 1UL : 0UL);
}

/**
  * @brief  Set ADC group regular conversion trigger polarity.
  * @note   Applicable only for trigger source set to external trigger.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR1    EXTEN          LL_ADC_REG_SetTriggerEdge
  * @param  ADCx ADC instance
  * @param  ExternalTriggerEdge This parameter can be one of the following values:
  *         @arg @ref LL_ADC_REG_TRIG_EXT_RISING
  *         @arg @ref LL_ADC_REG_TRIG_EXT_FALLING
  *         @arg @ref LL_ADC_REG_TRIG_EXT_RISINGFALLING
  * @retval None
  */
__STATIC_INLINE void LL_ADC_REG_SetTriggerEdge(ADC_TypeDef *ADCx, uint32_t ExternalTriggerEdge)
{
  MODIFY_REG(ADCx->CFGR1, ADC_CFGR1_EXTEN, ExternalTriggerEdge);
}

/**
  * @brief  Get ADC group regular conversion trigger polarity.
  * @note   Applicable only for trigger source set to external trigger.
  * @rmtoll CFGR1    EXTEN          LL_ADC_REG_GetTriggerEdge
  * @param  ADCx ADC instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_REG_TRIG_EXT_RISING
  *         @arg @ref LL_ADC_REG_TRIG_EXT_FALLING
  *         @arg @ref LL_ADC_REG_TRIG_EXT_RISINGFALLING
  */
__STATIC_INLINE uint32_t LL_ADC_REG_GetTriggerEdge(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CFGR1, ADC_CFGR1_EXTEN));
}

/**
  * @brief  Set ADC group regular sequencer configuration flexibility.
  * @note   On this STM32 serie, ADC group regular sequencer both modes
  *         "fully configurable" or "not fully configurable" are
  *         available:
  *         - sequencer configured to fully configurable:
  *           sequencer length and each rank
  *           affectation to a channel are configurable.
  *           Refer to description of function
  *           @ref LL_ADC_REG_SetSequencerLength().
  *         - sequencer configured to not fully configurable:
  *           sequencer length and each rank affectation to a channel
  *           are fixed by channel HW number.
  *           Refer to description of function
  *           @ref LL_ADC_REG_SetSequencerChannels().
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR     CHSELRMOD      LL_ADC_REG_SetSequencerConfigurable
  * @param  ADCx ADC instance
  * @param  Configurability This parameter can be one of the following values:
  *         @arg @ref LL_ADC_REG_SEQ_FIXED
  *         @arg @ref LL_ADC_REG_SEQ_CONFIGURABLE
  * @retval None
  */
__STATIC_INLINE void LL_ADC_REG_SetSequencerConfigurable(ADC_TypeDef *ADCx, uint32_t Configurability)
{
  MODIFY_REG(ADCx->CFGR1, ADC_CFGR1_CHSELRMOD, Configurability);
}

/**
  * @brief  Get ADC group regular sequencer configuration flexibility.
  * @note   On this STM32 serie, ADC group regular sequencer both modes
  *         "fully configurable" or "not fully configurable" are
  *         available:
  *         - sequencer configured to fully configurable:
  *           sequencer length and each rank
  *           affectation to a channel are configurable.
  *           Refer to description of function
  *           @ref LL_ADC_REG_SetSequencerLength().
  *         - sequencer configured to not fully configurable:
  *           sequencer length and each rank affectation to a channel
  *           are fixed by channel HW number.
  *           Refer to description of function
  *           @ref LL_ADC_REG_SetSequencerChannels().
  * @rmtoll CFGR     CHSELRMOD      LL_ADC_REG_SetSequencerConfigurable
  * @param  ADCx ADC instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_REG_SEQ_FIXED
  *         @arg @ref LL_ADC_REG_SEQ_CONFIGURABLE
  */
__STATIC_INLINE uint32_t LL_ADC_REG_GetSequencerConfigurable(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CFGR1, ADC_CFGR1_CHSELRMOD));
}

/**
  * @brief  Set ADC group regular sequencer length and scan direction.
  * @note   Description of ADC group regular sequencer features:
  *         - For devices with sequencer fully configurable
  *           (function "LL_ADC_REG_SetSequencerRanks()" available):
  *           sequencer length and each rank affectation to a channel
  *           are configurable.
  *           This function performs configuration of:
  *           - Sequence length: Number of ranks in the scan sequence.
  *           - Sequence direction: Unless specified in parameters, sequencer
  *             scan direction is forward (from rank 1 to rank n).
  *           Sequencer ranks are selected using
  *           function "LL_ADC_REG_SetSequencerRanks()".
  *         - For devices with sequencer not fully configurable
  *           (function "LL_ADC_REG_SetSequencerChannels()" available):
  *           sequencer length and each rank affectation to a channel
  *           are defined by channel number.
  *           This function performs configuration of:
  *           - Sequence length: Number of ranks in the scan sequence is
  *             defined by number of channels set in the sequence,
  *             rank of each channel is fixed by channel HW number.
  *             (channel 0 fixed on rank 0, channel 1 fixed on rank1, ...).
  *           - Sequence direction: Unless specified in parameters, sequencer
  *             scan direction is forward (from lowest channel number to
  *             highest channel number).
  *           Sequencer ranks are selected using
  *           function "LL_ADC_REG_SetSequencerChannels()".
  *           To set scan direction differently, refer to function
  *           @ref LL_ADC_REG_SetSequencerScanDirection().
  * @note   On this STM32 serie, ADC group regular sequencer both modes
  *         "fully configurable" or "not fully configurable" 
  *         are available, they can be chosen using
  *         function @ref LL_ADC_REG_SetSequencerConfigurable().
  * @note   On this STM32 serie, after calling functions
  *         @ref LL_ADC_REG_SetSequencerLength()
  *         or @ref LL_ADC_REG_SetSequencerRanks(),
  *         it is mandatory to wait for the assertion of CCRDY flag
  *         using @ref LL_ADC_IsActiveFlag_CCRDY().
  *         Otherwise, performing some actions (configuration update,
  *         ADC conversion start, ... ) will be ignored.
  *         Refer to reference manual for more details.
  * @note   Sequencer disabled is equivalent to sequencer of 1 rank:
  *         ADC conversion on only 1 channel.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CHSELR   SQ1            LL_ADC_REG_SetSequencerLength\n
  *         CHSELR   SQ2            LL_ADC_REG_SetSequencerLength\n
  *         CHSELR   SQ3            LL_ADC_REG_SetSequencerLength\n
  *         CHSELR   SQ4            LL_ADC_REG_SetSequencerLength\n
  *         CHSELR   SQ5            LL_ADC_REG_SetSequencerLength\n
  *         CHSELR   SQ6            LL_ADC_REG_SetSequencerLength\n
  *         CHSELR   SQ7            LL_ADC_REG_SetSequencerLength\n
  *         CHSELR   SQ8            LL_ADC_REG_SetSequencerLength
  * @param  ADCx ADC instance
  * @param  SequencerNbRanks This parameter can be one of the following values:
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_DISABLE
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_ENABLE_2RANKS
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_ENABLE_3RANKS
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_ENABLE_4RANKS
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_ENABLE_5RANKS
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_ENABLE_6RANKS
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_ENABLE_7RANKS
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_ENABLE_8RANKS
  * @retval None
  */
__STATIC_INLINE void LL_ADC_REG_SetSequencerLength(ADC_TypeDef *ADCx, uint32_t SequencerNbRanks)
{
  SET_BIT(ADCx->CHSELR, SequencerNbRanks);
}

/**
  * @brief  Get ADC group regular sequencer length and scan direction.
  * @note   Description of ADC group regular sequencer features:
  *         - For devices with sequencer fully configurable
  *           (function "LL_ADC_REG_SetSequencerRanks()" available):
  *           sequencer length and each rank affectation to a channel
  *           are configurable.
  *           This function retrieves:
  *           - Sequence length: Number of ranks in the scan sequence.
  *           - Sequence direction: Unless specified in parameters, sequencer
  *             scan direction is forward (from rank 1 to rank n).
  *           Sequencer ranks are selected using
  *           function "LL_ADC_REG_SetSequencerRanks()".
  *         - For devices with sequencer not fully configurable
  *           (function "LL_ADC_REG_SetSequencerChannels()" available):
  *           sequencer length and each rank affectation to a channel
  *           are defined by channel number.
  *           This function retrieves:
  *           - Sequence length: Number of ranks in the scan sequence is
  *             defined by number of channels set in the sequence,
  *             rank of each channel is fixed by channel HW number.
  *             (channel 0 fixed on rank 0, channel 1 fixed on rank1, ...).
  *           - Sequence direction: Unless specified in parameters, sequencer
  *             scan direction is forward (from lowest channel number to
  *             highest channel number).
  *           Sequencer ranks are selected using
  *           function "LL_ADC_REG_SetSequencerChannels()".
  *          To set scan direction differently, refer to function
  *          @ref LL_ADC_REG_SetSequencerScanDirection().
  * @note   On this STM32 serie, ADC group regular sequencer both modes
  *         "fully configurable" or "not fully configurable" 
  *         are available, they can be chosen using
  *         function @ref LL_ADC_REG_SetSequencerConfigurable().
  * @note   Sequencer disabled is equivalent to sequencer of 1 rank:
  *         ADC conversion on only 1 channel.
  * @rmtoll CHSELR   SQ1            LL_ADC_REG_GetSequencerLength\n
  *         CHSELR   SQ2            LL_ADC_REG_GetSequencerLength\n
  *         CHSELR   SQ3            LL_ADC_REG_GetSequencerLength\n
  *         CHSELR   SQ4            LL_ADC_REG_GetSequencerLength\n
  *         CHSELR   SQ5            LL_ADC_REG_GetSequencerLength\n
  *         CHSELR   SQ6            LL_ADC_REG_GetSequencerLength\n
  *         CHSELR   SQ7            LL_ADC_REG_GetSequencerLength\n
  *         CHSELR   SQ8            LL_ADC_REG_GetSequencerLength
  * @param  ADCx ADC instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_DISABLE
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_ENABLE_2RANKS
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_ENABLE_3RANKS
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_ENABLE_4RANKS
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_ENABLE_5RANKS
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_ENABLE_6RANKS
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_ENABLE_7RANKS
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_ENABLE_8RANKS
  */
__STATIC_INLINE uint32_t LL_ADC_REG_GetSequencerLength(ADC_TypeDef *ADCx)
{
  register uint32_t ChannelsRanks = READ_BIT(ADCx->CHSELR, ADC_CHSELR_SQ_ALL);
  register uint32_t SequencerLength = LL_ADC_REG_SEQ_SCAN_ENABLE_8RANKS;
  register uint32_t RankIndex;
  
  /* Parse register for end of sequence identifier */
  for(RankIndex = 0UL; RankIndex < (32U - 4U); RankIndex+=4U)
  {
    if((ChannelsRanks & (ADC_CHSELR_SQ2 << RankIndex)) == (ADC_CHSELR_SQ2 << RankIndex))
    {
      SequencerLength = (ADC_CHSELR_SQ2 << RankIndex);
      break;
    }
  }
  
  return SequencerLength;
}

/**
  * @brief  Set ADC group regular sequencer scan direction.
  * @note   On this STM32 serie, parameter relevant only is sequencer is set
  *         to mode not fully configurable,
  *         refer to function @ref LL_ADC_REG_SetSequencerConfigurable().
  * @note   On some other STM32 families, this setting is not available and
  *         the default scan direction is forward.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR1    SCANDIR        LL_ADC_REG_SetSequencerScanDirection
  * @param  ADCx ADC instance
  * @param  ScanDirection This parameter can be one of the following values:
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_DIR_FORWARD
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_DIR_BACKWARD
  * @retval None
  */
__STATIC_INLINE void LL_ADC_REG_SetSequencerScanDirection(ADC_TypeDef *ADCx, uint32_t ScanDirection)
{
  MODIFY_REG(ADCx->CFGR1, ADC_CFGR1_SCANDIR, ScanDirection);
}

/**
  * @brief  Get ADC group regular sequencer scan direction.
  * @note   On this STM32 serie, parameter relevant only is sequencer is set
  *         to mode not fully configurable,
  *         refer to function @ref LL_ADC_REG_SetSequencerConfigurable().
  * @note   On some other STM32 families, this setting is not available and
  *         the default scan direction is forward.
  * @rmtoll CFGR1    SCANDIR        LL_ADC_REG_GetSequencerScanDirection
  * @param  ADCx ADC instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_DIR_FORWARD
  *         @arg @ref LL_ADC_REG_SEQ_SCAN_DIR_BACKWARD
  */
__STATIC_INLINE uint32_t LL_ADC_REG_GetSequencerScanDirection(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CFGR1, ADC_CFGR1_SCANDIR));
}

/**
  * @brief  Set ADC group regular sequencer discontinuous mode:
  *         sequence subdivided and scan conversions interrupted every selected
  *         number of ranks.
  * @note   It is not possible to enable both ADC group regular 
  *         continuous mode and sequencer discontinuous mode.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR1    DISCEN         LL_ADC_REG_SetSequencerDiscont\n
  * @param  ADCx ADC instance
  * @param  SeqDiscont This parameter can be one of the following values:
  *         @arg @ref LL_ADC_REG_SEQ_DISCONT_DISABLE
  *         @arg @ref LL_ADC_REG_SEQ_DISCONT_1RANK
  * @retval None
  */
__STATIC_INLINE void LL_ADC_REG_SetSequencerDiscont(ADC_TypeDef *ADCx, uint32_t SeqDiscont)
{
  MODIFY_REG(ADCx->CFGR1, ADC_CFGR1_DISCEN, SeqDiscont);
}

/**
  * @brief  Get ADC group regular sequencer discontinuous mode:
  *         sequence subdivided and scan conversions interrupted every selected
  *         number of ranks.
  * @rmtoll CFGR1    DISCEN         LL_ADC_REG_GetSequencerDiscont\n
  * @param  ADCx ADC instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_REG_SEQ_DISCONT_DISABLE
  *         @arg @ref LL_ADC_REG_SEQ_DISCONT_1RANK
  */
__STATIC_INLINE uint32_t LL_ADC_REG_GetSequencerDiscont(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CFGR1, ADC_CFGR1_DISCEN));
}

/**
  * @brief  Set ADC group regular sequence: channel on the selected
  *         scan sequence rank.
  * @note   This function performs configuration of:
  *         - Channels ordering into each rank of scan sequence:
  *           whatever channel can be placed into whatever rank.
  * @note   On this STM32 serie, ADC group regular sequencer is
  *         fully configurable: sequencer length and each rank
  *         affectation to a channel are configurable.
  *         Refer to description of function @ref LL_ADC_REG_SetSequencerLength().
  * @note   Depending on devices and packages, some channels may not be available.
  *         Refer to device datasheet for channels availability.
  * @note   On this STM32 serie, to measure internal channels (VrefInt,
  *         TempSensor, ...), measurement paths to internal channels must be
  *         enabled separately.
  *         This can be done using function @ref LL_ADC_SetCommonPathInternalCh().
  * @note   On this STM32 serie, after calling functions
  *         @ref LL_ADC_REG_SetSequencerLength()
  *         or @ref LL_ADC_REG_SetSequencerRanks(),
  *         it is mandatory to wait for the assertion of CCRDY flag
  *         using @ref LL_ADC_IsActiveFlag_CCRDY().
  *         Otherwise, performing some actions (configuration update,
  *         ADC conversion start, ... ) will be ignored.
  *         Refer to reference manual for more details.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CHSELR   SQ1            LL_ADC_REG_SetSequencerRanks\n
  *         CHSELR   SQ2            LL_ADC_REG_SetSequencerRanks\n
  *         CHSELR   SQ3            LL_ADC_REG_SetSequencerRanks\n
  *         CHSELR   SQ4            LL_ADC_REG_SetSequencerRanks\n
  *         CHSELR   SQ5            LL_ADC_REG_SetSequencerRanks\n
  *         CHSELR   SQ6            LL_ADC_REG_SetSequencerRanks\n
  *         CHSELR   SQ7            LL_ADC_REG_SetSequencerRanks\n
  *         CHSELR   SQ8            LL_ADC_REG_SetSequencerRanks
  * @param  ADCx ADC instance
  * @param  Rank This parameter can be one of the following values:
  *         @arg @ref LL_ADC_REG_RANK_1
  *         @arg @ref LL_ADC_REG_RANK_2
  *         @arg @ref LL_ADC_REG_RANK_3
  *         @arg @ref LL_ADC_REG_RANK_4
  *         @arg @ref LL_ADC_REG_RANK_5
  *         @arg @ref LL_ADC_REG_RANK_6
  *         @arg @ref LL_ADC_REG_RANK_7
  *         @arg @ref LL_ADC_REG_RANK_8
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_ADC_CHANNEL_0
  *         @arg @ref LL_ADC_CHANNEL_1
  *         @arg @ref LL_ADC_CHANNEL_2
  *         @arg @ref LL_ADC_CHANNEL_3
  *         @arg @ref LL_ADC_CHANNEL_4
  *         @arg @ref LL_ADC_CHANNEL_5
  *         @arg @ref LL_ADC_CHANNEL_6
  *         @arg @ref LL_ADC_CHANNEL_7
  *         @arg @ref LL_ADC_CHANNEL_8
  *         @arg @ref LL_ADC_CHANNEL_9
  *         @arg @ref LL_ADC_CHANNEL_10
  *         @arg @ref LL_ADC_CHANNEL_11
  *         @arg @ref LL_ADC_CHANNEL_12
  *         @arg @ref LL_ADC_CHANNEL_13
  *         @arg @ref LL_ADC_CHANNEL_14
  *         @arg @ref LL_ADC_CHANNEL_15         (1)
  *         @arg @ref LL_ADC_CHANNEL_16         (1)
  *         @arg @ref LL_ADC_CHANNEL_17         (1)
  *         @arg @ref LL_ADC_CHANNEL_18
  *         @arg @ref LL_ADC_CHANNEL_VREFINT
  *         @arg @ref LL_ADC_CHANNEL_TEMPSENSOR
  *         @arg @ref LL_ADC_CHANNEL_VBAT
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().
  * @retval None
  */
__STATIC_INLINE void LL_ADC_REG_SetSequencerRanks(ADC_TypeDef *ADCx, uint32_t Rank, uint32_t Channel)
{
  /* Set bits with content of parameter "Channel" with bits position          */
  /* in register depending on parameter "Rank".                               */
  /* Parameters "Rank" and "Channel" are used with masks because containing   */
  /* other bits reserved for other purpose.                                   */
  MODIFY_REG(ADCx->CHSELR,
             ADC_CHSELR_SQ1 << (Rank & ADC_REG_RANK_ID_SQRX_MASK),
             ((Channel & ADC_CHANNEL_ID_NUMBER_MASK_SEQ) >> ADC_CHANNEL_ID_NUMBER_BITOFFSET_POS) << (Rank & ADC_REG_RANK_ID_SQRX_MASK));
}

/**
  * @brief  Get ADC group regular sequence: channel on the selected
  *         scan sequence rank.
  * @note   On this STM32 serie, ADC group regular sequencer is
  *         fully configurable: sequencer length and each rank
  *         affectation to a channel are configurable.
  *         Refer to description of function @ref LL_ADC_REG_SetSequencerLength().
  * @note   Depending on devices and packages, some channels may not be available.
  *         Refer to device datasheet for channels availability.
  * @note   Usage of the returned channel number:
  *         - To reinject this channel into another function LL_ADC_xxx:
  *           the returned channel number is only partly formatted on definition
  *           of literals LL_ADC_CHANNEL_x. Therefore, it has to be compared
  *           with parts of literals LL_ADC_CHANNEL_x or using
  *           helper macro @ref __LL_ADC_CHANNEL_TO_DECIMAL_NB().
  *           Then the selected literal LL_ADC_CHANNEL_x can be used
  *           as parameter for another function.
  *         - To get the channel number in decimal format:
  *           process the returned value with the helper macro
  *           @ref __LL_ADC_CHANNEL_TO_DECIMAL_NB().
  * @rmtoll CHSELR   SQ1            LL_ADC_REG_GetSequencerRanks\n
  *         CHSELR   SQ2            LL_ADC_REG_GetSequencerRanks\n
  *         CHSELR   SQ3            LL_ADC_REG_GetSequencerRanks\n
  *         CHSELR   SQ4            LL_ADC_REG_GetSequencerRanks\n
  *         CHSELR   SQ5            LL_ADC_REG_GetSequencerRanks\n
  *         CHSELR   SQ6            LL_ADC_REG_GetSequencerRanks\n
  *         CHSELR   SQ7            LL_ADC_REG_GetSequencerRanks\n
  *         CHSELR   SQ8            LL_ADC_REG_GetSequencerRanks
  * @param  ADCx ADC instance
  * @param  Rank This parameter can be one of the following values:
  *         @arg @ref LL_ADC_REG_RANK_1
  *         @arg @ref LL_ADC_REG_RANK_2
  *         @arg @ref LL_ADC_REG_RANK_3
  *         @arg @ref LL_ADC_REG_RANK_4
  *         @arg @ref LL_ADC_REG_RANK_5
  *         @arg @ref LL_ADC_REG_RANK_6
  *         @arg @ref LL_ADC_REG_RANK_7
  *         @arg @ref LL_ADC_REG_RANK_8
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_CHANNEL_0
  *         @arg @ref LL_ADC_CHANNEL_1
  *         @arg @ref LL_ADC_CHANNEL_2
  *         @arg @ref LL_ADC_CHANNEL_3
  *         @arg @ref LL_ADC_CHANNEL_4
  *         @arg @ref LL_ADC_CHANNEL_5
  *         @arg @ref LL_ADC_CHANNEL_6
  *         @arg @ref LL_ADC_CHANNEL_7
  *         @arg @ref LL_ADC_CHANNEL_8
  *         @arg @ref LL_ADC_CHANNEL_9
  *         @arg @ref LL_ADC_CHANNEL_10
  *         @arg @ref LL_ADC_CHANNEL_11
  *         @arg @ref LL_ADC_CHANNEL_12
  *         @arg @ref LL_ADC_CHANNEL_13
  *         @arg @ref LL_ADC_CHANNEL_14
  *         @arg @ref LL_ADC_CHANNEL_15         (1)
  *         @arg @ref LL_ADC_CHANNEL_16         (1)
  *         @arg @ref LL_ADC_CHANNEL_17         (1)
  *         @arg @ref LL_ADC_CHANNEL_18
  *         @arg @ref LL_ADC_CHANNEL_VREFINT    (2)
  *         @arg @ref LL_ADC_CHANNEL_TEMPSENSOR (2)
  *         @arg @ref LL_ADC_CHANNEL_VBAT       (2)
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().\n
  *         (2) For ADC channel read back from ADC register,
  *             comparison with internal channel parameter to be done
  *             using helper macro @ref __LL_ADC_CHANNEL_INTERNAL_TO_EXTERNAL().
  */
__STATIC_INLINE uint32_t LL_ADC_REG_GetSequencerRanks(ADC_TypeDef *ADCx, uint32_t Rank)
{
  return (uint32_t) ((READ_BIT(ADCx->CHSELR,
                               ADC_CHSELR_SQ1 << (Rank & ADC_REG_RANK_ID_SQRX_MASK))
                      >> (Rank & ADC_REG_RANK_ID_SQRX_MASK)
                     ) << (ADC_CHANNEL_ID_NUMBER_BITOFFSET_POS)
                    );
}

/**
  * @brief  Set ADC group regular sequence: channel on rank corresponding to
  *         channel number.
  * @note   This function performs:
  *         - Channels ordering into each rank of scan sequence:
  *           rank of each channel is fixed by channel HW number
  *           (channel 0 fixed on rank 0, channel 1 fixed on rank1, ...).
  *         - Set channels selected by overwriting the current sequencer
  *           configuration.
  * @note   On this STM32 serie, ADC group regular sequencer both modes
  *         "fully configurable" or "not fully configurable" 
  *         are available, they can be chosen using
  *         function @ref LL_ADC_REG_SetSequencerConfigurable().
  *         This function can be used with setting "not fully configurable".
  *         Refer to description of functions @ref LL_ADC_REG_SetSequencerConfigurable()
  *         and @ref LL_ADC_REG_SetSequencerLength().
  * @note   Depending on devices and packages, some channels may not be available.
  *         Refer to device datasheet for channels availability.
  * @note   On this STM32 serie, to measure internal channels (VrefInt,
  *         TempSensor, ...), measurement paths to internal channels must be
  *         enabled separately.
  *         This can be done using function @ref LL_ADC_SetCommonPathInternalCh().
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @note   One or several values can be selected.
  *         Example: (LL_ADC_CHANNEL_4 | LL_ADC_CHANNEL_12 | ...)
  * @rmtoll CHSELR   CHSEL0         LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL1         LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL2         LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL3         LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL4         LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL5         LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL6         LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL7         LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL8         LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL9         LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL10        LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL11        LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL12        LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL13        LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL14        LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL15        LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL16        LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL17        LL_ADC_REG_SetSequencerChannels\n
  *         CHSELR   CHSEL18        LL_ADC_REG_SetSequencerChannels
  * @param  ADCx ADC instance
  * @param  Channel This parameter can be a combination of the following values:
  *         @arg @ref LL_ADC_CHANNEL_0
  *         @arg @ref LL_ADC_CHANNEL_1
  *         @arg @ref LL_ADC_CHANNEL_2
  *         @arg @ref LL_ADC_CHANNEL_3
  *         @arg @ref LL_ADC_CHANNEL_4
  *         @arg @ref LL_ADC_CHANNEL_5
  *         @arg @ref LL_ADC_CHANNEL_6
  *         @arg @ref LL_ADC_CHANNEL_7
  *         @arg @ref LL_ADC_CHANNEL_8
  *         @arg @ref LL_ADC_CHANNEL_9
  *         @arg @ref LL_ADC_CHANNEL_10
  *         @arg @ref LL_ADC_CHANNEL_11
  *         @arg @ref LL_ADC_CHANNEL_12
  *         @arg @ref LL_ADC_CHANNEL_13
  *         @arg @ref LL_ADC_CHANNEL_14
  *         @arg @ref LL_ADC_CHANNEL_15         (1)
  *         @arg @ref LL_ADC_CHANNEL_16         (1)
  *         @arg @ref LL_ADC_CHANNEL_17         (1)
  *         @arg @ref LL_ADC_CHANNEL_18
  *         @arg @ref LL_ADC_CHANNEL_VREFINT
  *         @arg @ref LL_ADC_CHANNEL_TEMPSENSOR
  *         @arg @ref LL_ADC_CHANNEL_VBAT
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().
  * @retval None
  */
__STATIC_INLINE void LL_ADC_REG_SetSequencerChannels(ADC_TypeDef *ADCx, uint32_t Channel)
{
  /* Parameter "Channel" is used with masks because containing                */
  /* other bits reserved for other purpose.                                   */
  WRITE_REG(ADCx->CHSELR, (Channel & ADC_CHANNEL_ID_BITFIELD_MASK));
}

/**
  * @brief  Add channel to ADC group regular sequence: channel on rank corresponding to
  *         channel number.
  * @note   This function performs:
  *         - Channels ordering into each rank of scan sequence:
  *           rank of each channel is fixed by channel HW number
  *           (channel 0 fixed on rank 0, channel 1 fixed on rank1, ...).
  *         - Set channels selected by adding them to the current sequencer
  *           configuration.
  * @note   On this STM32 serie, ADC group regular sequencer both modes
  *         "fully configurable" or "not fully configurable" 
  *         are available, they can be chosen using
  *         function @ref LL_ADC_REG_SetSequencerConfigurable().
  *         This function can be used with setting "not fully configurable".
  *         Refer to description of functions @ref LL_ADC_REG_SetSequencerConfigurable()
  *         and @ref LL_ADC_REG_SetSequencerLength().
  * @note   Depending on devices and packages, some channels may not be available.
  *         Refer to device datasheet for channels availability.
  * @note   On this STM32 serie, to measure internal channels (VrefInt,
  *         TempSensor, ...), measurement paths to internal channels must be
  *         enabled separately.
  *         This can be done using function @ref LL_ADC_SetCommonPathInternalCh().
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @note   One or several values can be selected.
  *         Example: (LL_ADC_CHANNEL_4 | LL_ADC_CHANNEL_12 | ...)
  * @rmtoll CHSELR   CHSEL0         LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL1         LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL2         LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL3         LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL4         LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL5         LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL6         LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL7         LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL8         LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL9         LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL10        LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL11        LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL12        LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL13        LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL14        LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL15        LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL16        LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL17        LL_ADC_REG_SetSequencerChAdd\n
  *         CHSELR   CHSEL18        LL_ADC_REG_SetSequencerChAdd
  * @param  ADCx ADC instance
  * @param  Channel This parameter can be a combination of the following values:
  *         @arg @ref LL_ADC_CHANNEL_0
  *         @arg @ref LL_ADC_CHANNEL_1
  *         @arg @ref LL_ADC_CHANNEL_2
  *         @arg @ref LL_ADC_CHANNEL_3
  *         @arg @ref LL_ADC_CHANNEL_4
  *         @arg @ref LL_ADC_CHANNEL_5
  *         @arg @ref LL_ADC_CHANNEL_6
  *         @arg @ref LL_ADC_CHANNEL_7
  *         @arg @ref LL_ADC_CHANNEL_8
  *         @arg @ref LL_ADC_CHANNEL_9
  *         @arg @ref LL_ADC_CHANNEL_10
  *         @arg @ref LL_ADC_CHANNEL_11
  *         @arg @ref LL_ADC_CHANNEL_12
  *         @arg @ref LL_ADC_CHANNEL_13
  *         @arg @ref LL_ADC_CHANNEL_14
  *         @arg @ref LL_ADC_CHANNEL_15         (1)
  *         @arg @ref LL_ADC_CHANNEL_16         (1)
  *         @arg @ref LL_ADC_CHANNEL_17         (1)
  *         @arg @ref LL_ADC_CHANNEL_18
  *         @arg @ref LL_ADC_CHANNEL_VREFINT
  *         @arg @ref LL_ADC_CHANNEL_TEMPSENSOR
  *         @arg @ref LL_ADC_CHANNEL_VBAT
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().
  * @retval None
  */
__STATIC_INLINE void LL_ADC_REG_SetSequencerChAdd(ADC_TypeDef *ADCx, uint32_t Channel)
{
  /* Parameter "Channel" is used with masks because containing                */
  /* other bits reserved for other purpose.                                   */
  SET_BIT(ADCx->CHSELR, (Channel & ADC_CHANNEL_ID_BITFIELD_MASK));
}

/**
  * @brief  Remove channel to ADC group regular sequence: channel on rank corresponding to
  *         channel number.
  * @note   This function performs:
  *         - Channels ordering into each rank of scan sequence:
  *           rank of each channel is fixed by channel HW number
  *           (channel 0 fixed on rank 0, channel 1 fixed on rank1, ...).
  *         - Set channels selected by removing them to the current sequencer
  *           configuration.
  * @note   On this STM32 serie, ADC group regular sequencer both modes
  *         "fully configurable" or "not fully configurable" 
  *         are available, they can be chosen using
  *         function @ref LL_ADC_REG_SetSequencerConfigurable().
  *         This function can be used with setting "not fully configurable".
  *         Refer to description of functions @ref LL_ADC_REG_SetSequencerConfigurable()
  *         and @ref LL_ADC_REG_SetSequencerLength().
  * @note   Depending on devices and packages, some channels may not be available.
  *         Refer to device datasheet for channels availability.
  * @note   On this STM32 serie, to measure internal channels (VrefInt,
  *         TempSensor, ...), measurement paths to internal channels must be
  *         enabled separately.
  *         This can be done using function @ref LL_ADC_SetCommonPathInternalCh().
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @note   One or several values can be selected.
  *         Example: (LL_ADC_CHANNEL_4 | LL_ADC_CHANNEL_12 | ...)
  * @rmtoll CHSELR   CHSEL0         LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL1         LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL2         LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL3         LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL4         LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL5         LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL6         LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL7         LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL8         LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL9         LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL10        LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL11        LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL12        LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL13        LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL14        LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL15        LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL16        LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL17        LL_ADC_REG_SetSequencerChRem\n
  *         CHSELR   CHSEL18        LL_ADC_REG_SetSequencerChRem
  * @param  ADCx ADC instance
  * @param  Channel This parameter can be a combination of the following values:
  *         @arg @ref LL_ADC_CHANNEL_0
  *         @arg @ref LL_ADC_CHANNEL_1
  *         @arg @ref LL_ADC_CHANNEL_2
  *         @arg @ref LL_ADC_CHANNEL_3
  *         @arg @ref LL_ADC_CHANNEL_4
  *         @arg @ref LL_ADC_CHANNEL_5
  *         @arg @ref LL_ADC_CHANNEL_6
  *         @arg @ref LL_ADC_CHANNEL_7
  *         @arg @ref LL_ADC_CHANNEL_8
  *         @arg @ref LL_ADC_CHANNEL_9
  *         @arg @ref LL_ADC_CHANNEL_10
  *         @arg @ref LL_ADC_CHANNEL_11
  *         @arg @ref LL_ADC_CHANNEL_12
  *         @arg @ref LL_ADC_CHANNEL_13
  *         @arg @ref LL_ADC_CHANNEL_14
  *         @arg @ref LL_ADC_CHANNEL_15         (1)
  *         @arg @ref LL_ADC_CHANNEL_16         (1)
  *         @arg @ref LL_ADC_CHANNEL_17         (1)
  *         @arg @ref LL_ADC_CHANNEL_18
  *         @arg @ref LL_ADC_CHANNEL_VREFINT
  *         @arg @ref LL_ADC_CHANNEL_TEMPSENSOR
  *         @arg @ref LL_ADC_CHANNEL_VBAT
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().
  * @retval None
  */
__STATIC_INLINE void LL_ADC_REG_SetSequencerChRem(ADC_TypeDef *ADCx, uint32_t Channel)
{
  /* Parameter "Channel" is used with masks because containing                */
  /* other bits reserved for other purpose.                                   */
  CLEAR_BIT(ADCx->CHSELR, (Channel & ADC_CHANNEL_ID_BITFIELD_MASK));
}

/**
  * @brief  Get ADC group regular sequence: channel on rank corresponding to
  *         channel number.
  * @note   This function performs:
  *         - Channels order reading into each rank of scan sequence:
  *           rank of each channel is fixed by channel HW number
  *           (channel 0 fixed on rank 0, channel 1 fixed on rank1, ...).
  * @note   On this STM32 serie, ADC group regular sequencer both modes
  *         "fully configurable" or "not fully configurable" 
  *         are available, they can be chosen using
  *         function @ref LL_ADC_REG_SetSequencerConfigurable().
  *         This function can be used with setting "not fully configurable".
  *         Refer to description of functions @ref LL_ADC_REG_SetSequencerConfigurable()
  *         and @ref LL_ADC_REG_SetSequencerLength().
  * @note   Depending on devices and packages, some channels may not be available.
  *         Refer to device datasheet for channels availability.
  * @note   On this STM32 serie, to measure internal channels (VrefInt,
  *         TempSensor, ...), measurement paths to internal channels must be
  *         enabled separately.
  *         This can be done using function @ref LL_ADC_SetCommonPathInternalCh().
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @note   One or several values can be retrieved.
  *         Example: (LL_ADC_CHANNEL_4 | LL_ADC_CHANNEL_12 | ...)
  * @rmtoll CHSELR   CHSEL0         LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL1         LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL2         LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL3         LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL4         LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL5         LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL6         LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL7         LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL8         LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL9         LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL10        LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL11        LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL12        LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL13        LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL14        LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL15        LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL16        LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL17        LL_ADC_REG_GetSequencerChannels\n
  *         CHSELR   CHSEL18        LL_ADC_REG_GetSequencerChannels
  * @param  ADCx ADC instance
  * @retval Returned value can be a combination of the following values:
  *         @arg @ref LL_ADC_CHANNEL_0
  *         @arg @ref LL_ADC_CHANNEL_1
  *         @arg @ref LL_ADC_CHANNEL_2
  *         @arg @ref LL_ADC_CHANNEL_3
  *         @arg @ref LL_ADC_CHANNEL_4
  *         @arg @ref LL_ADC_CHANNEL_5
  *         @arg @ref LL_ADC_CHANNEL_6
  *         @arg @ref LL_ADC_CHANNEL_7
  *         @arg @ref LL_ADC_CHANNEL_8
  *         @arg @ref LL_ADC_CHANNEL_9
  *         @arg @ref LL_ADC_CHANNEL_10
  *         @arg @ref LL_ADC_CHANNEL_11
  *         @arg @ref LL_ADC_CHANNEL_12
  *         @arg @ref LL_ADC_CHANNEL_13
  *         @arg @ref LL_ADC_CHANNEL_14
  *         @arg @ref LL_ADC_CHANNEL_15         (1)
  *         @arg @ref LL_ADC_CHANNEL_16         (1)
  *         @arg @ref LL_ADC_CHANNEL_17         (1)
  *         @arg @ref LL_ADC_CHANNEL_18
  *         @arg @ref LL_ADC_CHANNEL_VREFINT
  *         @arg @ref LL_ADC_CHANNEL_TEMPSENSOR
  *         @arg @ref LL_ADC_CHANNEL_VBAT
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().
  */
__STATIC_INLINE uint32_t LL_ADC_REG_GetSequencerChannels(ADC_TypeDef *ADCx)
{
  register uint32_t ChannelsBitfield = READ_BIT(ADCx->CHSELR, ADC_CHSELR_CHSEL);
  
  return (   (((ChannelsBitfield & ADC_CHSELR_CHSEL0) >> ADC_CHSELR_CHSEL0_BITOFFSET_POS) * LL_ADC_CHANNEL_0)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL1) >> ADC_CHSELR_CHSEL1_BITOFFSET_POS) * LL_ADC_CHANNEL_1)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL2) >> ADC_CHSELR_CHSEL2_BITOFFSET_POS) * LL_ADC_CHANNEL_2)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL3) >> ADC_CHSELR_CHSEL3_BITOFFSET_POS) * LL_ADC_CHANNEL_3)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL4) >> ADC_CHSELR_CHSEL4_BITOFFSET_POS) * LL_ADC_CHANNEL_4)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL5) >> ADC_CHSELR_CHSEL5_BITOFFSET_POS) * LL_ADC_CHANNEL_5)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL6) >> ADC_CHSELR_CHSEL6_BITOFFSET_POS) * LL_ADC_CHANNEL_6)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL7) >> ADC_CHSELR_CHSEL7_BITOFFSET_POS) * LL_ADC_CHANNEL_7)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL8) >> ADC_CHSELR_CHSEL8_BITOFFSET_POS) * LL_ADC_CHANNEL_8)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL9) >> ADC_CHSELR_CHSEL9_BITOFFSET_POS) * LL_ADC_CHANNEL_9)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL10) >> ADC_CHSELR_CHSEL10_BITOFFSET_POS) * LL_ADC_CHANNEL_10)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL11) >> ADC_CHSELR_CHSEL11_BITOFFSET_POS) * LL_ADC_CHANNEL_11)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL12) >> ADC_CHSELR_CHSEL12_BITOFFSET_POS) * LL_ADC_CHANNEL_12)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL13) >> ADC_CHSELR_CHSEL13_BITOFFSET_POS) * LL_ADC_CHANNEL_13)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL14) >> ADC_CHSELR_CHSEL14_BITOFFSET_POS) * LL_ADC_CHANNEL_14)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL15) >> ADC_CHSELR_CHSEL15_BITOFFSET_POS) * LL_ADC_CHANNEL_15)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL16) >> ADC_CHSELR_CHSEL16_BITOFFSET_POS) * LL_ADC_CHANNEL_16)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL17) >> ADC_CHSELR_CHSEL17_BITOFFSET_POS) * LL_ADC_CHANNEL_17)
#if defined(ADC_CCR_VBATEN)
           | (((ChannelsBitfield & ADC_CHSELR_CHSEL18) >> ADC_CHSELR_CHSEL18_BITOFFSET_POS) * LL_ADC_CHANNEL_18)
#endif
         );
}
/**
  * @brief  Set ADC continuous conversion mode on ADC group regular.
  * @note   Description of ADC continuous conversion mode:
  *         - single mode: one conversion per trigger
  *         - continuous mode: after the first trigger, following
  *           conversions launched successively automatically.
  * @note   It is not possible to enable both ADC group regular 
  *         continuous mode and sequencer discontinuous mode.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR1    CONT           LL_ADC_REG_SetContinuousMode
  * @param  ADCx ADC instance
  * @param  Continuous This parameter can be one of the following values:
  *         @arg @ref LL_ADC_REG_CONV_SINGLE
  *         @arg @ref LL_ADC_REG_CONV_CONTINUOUS
  * @retval None
  */
__STATIC_INLINE void LL_ADC_REG_SetContinuousMode(ADC_TypeDef *ADCx, uint32_t Continuous)
{
  MODIFY_REG(ADCx->CFGR1, ADC_CFGR1_CONT, Continuous);
}

/**
  * @brief  Get ADC continuous conversion mode on ADC group regular.
  * @note   Description of ADC continuous conversion mode:
  *         - single mode: one conversion per trigger
  *         - continuous mode: after the first trigger, following
  *           conversions launched successively automatically.
  * @rmtoll CFGR1    CONT           LL_ADC_REG_GetContinuousMode
  * @param  ADCx ADC instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_REG_CONV_SINGLE
  *         @arg @ref LL_ADC_REG_CONV_CONTINUOUS
  */
__STATIC_INLINE uint32_t LL_ADC_REG_GetContinuousMode(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CFGR1, ADC_CFGR1_CONT));
}

/**
  * @brief  Set ADC group regular conversion data transfer: no transfer or
  *         transfer by DMA, and DMA requests mode.
  * @note   If transfer by DMA selected, specifies the DMA requests
  *         mode:
  *         - Limited mode (One shot mode): DMA transfer requests are stopped
  *           when number of DMA data transfers (number of
  *           ADC conversions) is reached.
  *           This ADC mode is intended to be used with DMA mode non-circular.
  *         - Unlimited mode: DMA transfer requests are unlimited,
  *           whatever number of DMA data transfers (number of
  *           ADC conversions).
  *           This ADC mode is intended to be used with DMA mode circular.
  * @note   If ADC DMA requests mode is set to unlimited and DMA is set to
  *         mode non-circular:
  *         when DMA transfers size will be reached, DMA will stop transfers of
  *         ADC conversions data ADC will raise an overrun error
  *        (overrun flag and interruption if enabled).
  * @note   To configure DMA source address (peripheral address),
  *         use function @ref LL_ADC_DMA_GetRegAddr().
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR1    DMAEN          LL_ADC_REG_SetDMATransfer\n
  *         CFGR1    DMACFG         LL_ADC_REG_SetDMATransfer
  * @param  ADCx ADC instance
  * @param  DMATransfer This parameter can be one of the following values:
  *         @arg @ref LL_ADC_REG_DMA_TRANSFER_NONE
  *         @arg @ref LL_ADC_REG_DMA_TRANSFER_LIMITED
  *         @arg @ref LL_ADC_REG_DMA_TRANSFER_UNLIMITED
  * @retval None
  */
__STATIC_INLINE void LL_ADC_REG_SetDMATransfer(ADC_TypeDef *ADCx, uint32_t DMATransfer)
{
  MODIFY_REG(ADCx->CFGR1, ADC_CFGR1_DMAEN | ADC_CFGR1_DMACFG, DMATransfer);
}

/**
  * @brief  Get ADC group regular conversion data transfer: no transfer or
  *         transfer by DMA, and DMA requests mode.
  * @note   If transfer by DMA selected, specifies the DMA requests
  *         mode:
  *         - Limited mode (One shot mode): DMA transfer requests are stopped
  *           when number of DMA data transfers (number of
  *           ADC conversions) is reached.
  *           This ADC mode is intended to be used with DMA mode non-circular.
  *         - Unlimited mode: DMA transfer requests are unlimited,
  *           whatever number of DMA data transfers (number of
  *           ADC conversions).
  *           This ADC mode is intended to be used with DMA mode circular.
  * @note   If ADC DMA requests mode is set to unlimited and DMA is set to
  *         mode non-circular:
  *         when DMA transfers size will be reached, DMA will stop transfers of
  *         ADC conversions data ADC will raise an overrun error
  *         (overrun flag and interruption if enabled).
  * @note   To configure DMA source address (peripheral address),
  *         use function @ref LL_ADC_DMA_GetRegAddr().
  * @rmtoll CFGR1    DMAEN          LL_ADC_REG_GetDMATransfer\n
  *         CFGR1    DMACFG         LL_ADC_REG_GetDMATransfer
  * @param  ADCx ADC instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_REG_DMA_TRANSFER_NONE
  *         @arg @ref LL_ADC_REG_DMA_TRANSFER_LIMITED
  *         @arg @ref LL_ADC_REG_DMA_TRANSFER_UNLIMITED
  */
__STATIC_INLINE uint32_t LL_ADC_REG_GetDMATransfer(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CFGR1, ADC_CFGR1_DMAEN | ADC_CFGR1_DMACFG));
}

/**
  * @brief  Set ADC group regular behavior in case of overrun:
  *         data preserved or overwritten.
  * @note   Compatibility with devices without feature overrun:
  *         other devices without this feature have a behavior
  *         equivalent to data overwritten.
  *         The default setting of overrun is data preserved.
  *         Therefore, for compatibility with all devices, parameter
  *         overrun should be set to data overwritten.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR1    OVRMOD         LL_ADC_REG_SetOverrun
  * @param  ADCx ADC instance
  * @param  Overrun This parameter can be one of the following values:
  *         @arg @ref LL_ADC_REG_OVR_DATA_PRESERVED
  *         @arg @ref LL_ADC_REG_OVR_DATA_OVERWRITTEN
  * @retval None
  */
__STATIC_INLINE void LL_ADC_REG_SetOverrun(ADC_TypeDef *ADCx, uint32_t Overrun)
{
  MODIFY_REG(ADCx->CFGR1, ADC_CFGR1_OVRMOD, Overrun);
}

/**
  * @brief  Get ADC group regular behavior in case of overrun:
  *         data preserved or overwritten.
  * @rmtoll CFGR1    OVRMOD         LL_ADC_REG_GetOverrun
  * @param  ADCx ADC instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_REG_OVR_DATA_PRESERVED
  *         @arg @ref LL_ADC_REG_OVR_DATA_OVERWRITTEN
  */
__STATIC_INLINE uint32_t LL_ADC_REG_GetOverrun(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CFGR1, ADC_CFGR1_OVRMOD));
}

/**
  * @}
  */

/** @defgroup ADC_LL_EF_Configuration_Channels Configuration of ADC hierarchical scope: channels
  * @{
  */

/**
  * @brief  Set sampling time of the selected ADC channel
  *         Unit: ADC clock cycles.
  * @note   On this device, sampling time is on channel scope: independently
  *         of channel mapped on ADC group regular or injected.
  * @note   In case of internal channel (VrefInt, TempSensor, ...) to be
  *         converted:
  *         sampling time constraints must be respected (sampling time can be
  *         adjusted in function of ADC clock frequency and sampling time
  *         setting).
  *         Refer to device datasheet for timings values (parameters TS_vrefint,
  *         TS_temp, ...).
  * @note   Conversion time is the addition of sampling time and processing time.
  *         Refer to reference manual for ADC processing time of
  *         this STM32 serie.
  * @note   In case of ADC conversion of internal channel (VrefInt,
  *         temperature sensor, ...), a sampling time minimum value
  *         is required.
  *         Refer to device datasheet.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll SMPR     SMPSEL0        LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL1        LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL2        LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL3        LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL4        LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL5        LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL6        LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL7        LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL8        LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL9        LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL10       LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL11       LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL12       LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL13       LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL14       LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL15       LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL16       LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL17       LL_ADC_SetChannelSamplingTime\n
  *         SMPR     SMPSEL18       LL_ADC_SetChannelSamplingTime
  * @param  ADCx ADC instance
  * @param  Channel This parameter can be a combination of the following values:
  *         @arg @ref LL_ADC_CHANNEL_0
  *         @arg @ref LL_ADC_CHANNEL_1
  *         @arg @ref LL_ADC_CHANNEL_2
  *         @arg @ref LL_ADC_CHANNEL_3
  *         @arg @ref LL_ADC_CHANNEL_4
  *         @arg @ref LL_ADC_CHANNEL_5
  *         @arg @ref LL_ADC_CHANNEL_6
  *         @arg @ref LL_ADC_CHANNEL_7
  *         @arg @ref LL_ADC_CHANNEL_8
  *         @arg @ref LL_ADC_CHANNEL_9
  *         @arg @ref LL_ADC_CHANNEL_10
  *         @arg @ref LL_ADC_CHANNEL_11
  *         @arg @ref LL_ADC_CHANNEL_12
  *         @arg @ref LL_ADC_CHANNEL_13
  *         @arg @ref LL_ADC_CHANNEL_14
  *         @arg @ref LL_ADC_CHANNEL_15         (1)
  *         @arg @ref LL_ADC_CHANNEL_16         (1)
  *         @arg @ref LL_ADC_CHANNEL_17         (1)
  *         @arg @ref LL_ADC_CHANNEL_18
  *         @arg @ref LL_ADC_CHANNEL_VREFINT
  *         @arg @ref LL_ADC_CHANNEL_TEMPSENSOR
  *         @arg @ref LL_ADC_CHANNEL_VBAT
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().
  * @param  SamplingTimeY This parameter can be one of the following values:
  *         @arg @ref LL_ADC_SAMPLINGTIME_COMMON_1
  *         @arg @ref LL_ADC_SAMPLINGTIME_COMMON_2
  * @retval None
  */
__STATIC_INLINE void LL_ADC_SetChannelSamplingTime(ADC_TypeDef *ADCx, uint32_t Channel, uint32_t SamplingTimeY)
{
  /* Parameter "Channel" is used with masks because containing                */
  /* other bits reserved for other purpose.                                   */
  MODIFY_REG(ADCx->SMPR,
             (Channel << ADC_SMPR_SMPSEL0_BITOFFSET_POS),
             (Channel << ADC_SMPR_SMPSEL0_BITOFFSET_POS) & (SamplingTimeY & ADC_SAMPLING_TIME_CH_MASK)
            );
}

/**
  * @brief  Get sampling time of the selected ADC channel
  *         Unit: ADC clock cycles.
  * @note   On this device, sampling time is on channel scope: independently
  *         of channel mapped on ADC group regular or injected.
  * @note   Conversion time is the addition of sampling time and processing time.
  *         Refer to reference manual for ADC processing time of
  *         this STM32 serie.
  * @rmtoll SMPR     SMPSEL0        LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL1        LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL2        LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL3        LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL4        LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL5        LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL6        LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL7        LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL8        LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL9        LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL10       LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL11       LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL12       LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL13       LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL14       LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL15       LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL16       LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL17       LL_ADC_GetChannelSamplingTime\n
  *         SMPR     SMPSEL18       LL_ADC_GetChannelSamplingTime
  * @param  ADCx ADC instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_ADC_CHANNEL_0
  *         @arg @ref LL_ADC_CHANNEL_1
  *         @arg @ref LL_ADC_CHANNEL_2
  *         @arg @ref LL_ADC_CHANNEL_3
  *         @arg @ref LL_ADC_CHANNEL_4
  *         @arg @ref LL_ADC_CHANNEL_5
  *         @arg @ref LL_ADC_CHANNEL_6
  *         @arg @ref LL_ADC_CHANNEL_7
  *         @arg @ref LL_ADC_CHANNEL_8
  *         @arg @ref LL_ADC_CHANNEL_9
  *         @arg @ref LL_ADC_CHANNEL_10
  *         @arg @ref LL_ADC_CHANNEL_11
  *         @arg @ref LL_ADC_CHANNEL_12
  *         @arg @ref LL_ADC_CHANNEL_13
  *         @arg @ref LL_ADC_CHANNEL_14
  *         @arg @ref LL_ADC_CHANNEL_15         (1)
  *         @arg @ref LL_ADC_CHANNEL_16         (1)
  *         @arg @ref LL_ADC_CHANNEL_17         (1)
  *         @arg @ref LL_ADC_CHANNEL_18
  *         @arg @ref LL_ADC_CHANNEL_VREFINT
  *         @arg @ref LL_ADC_CHANNEL_TEMPSENSOR
  *         @arg @ref LL_ADC_CHANNEL_VBAT
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_SAMPLINGTIME_COMMON_1
  *         @arg @ref LL_ADC_SAMPLINGTIME_COMMON_2
  */
__STATIC_INLINE uint32_t LL_ADC_GetChannelSamplingTime(ADC_TypeDef *ADCx, uint32_t Channel)
{
  register uint32_t smpr = READ_REG(ADCx->SMPR);
  
  /* Retrieve sampling time bit corresponding to the selected channel            */
  /* and shift it to position 0.                                                 */ 
  register uint32_t smp_channel_posbit0 = ((smpr & ADC_SAMPLING_TIME_CH_MASK)
                                           >> ((((Channel & ADC_CHANNEL_ID_NUMBER_MASK) >> ADC_CHANNEL_ID_NUMBER_BITOFFSET_POS) + ADC_SMPR_SMPSEL0_BITOFFSET_POS) & 0x1FUL));
  
  /* Select sampling time bitfield depending on sampling time bit value 0 or 1.  */
  return(  (~(smp_channel_posbit0) * LL_ADC_SAMPLINGTIME_COMMON_1)
         | (smp_channel_posbit0 * LL_ADC_SAMPLINGTIME_COMMON_2)   );
}

/**
  * @}
  */

/** @defgroup ADC_LL_EF_Configuration_ADC_AnalogWatchdog Configuration of ADC transversal scope: analog watchdog
  * @{
  */

/**
  * @brief  Set ADC analog watchdog monitored channels:
  *         a single channel, multiple channels or all channels,
  *         on ADC group regular.
  * @note   Once monitored channels are selected, analog watchdog
  *         is enabled.
  * @note   In case of need to define a single channel to monitor
  *         with analog watchdog from sequencer channel definition,
  *         use helper macro @ref __LL_ADC_ANALOGWD_CHANNEL_GROUP().
  * @note   On this STM32 serie, there are 2 kinds of analog watchdog
  *         instance:
  *         - AWD standard (instance AWD1):
  *           - channels monitored: can monitor 1 channel or all channels.
  *           - groups monitored: ADC group regular.
  *           - resolution: resolution is not limited (corresponds to
  *             ADC resolution configured).
  *         - AWD flexible (instances AWD2, AWD3):
  *           - channels monitored: flexible on channels monitored, selection is
  *             channel wise, from from 1 to all channels.
  *             Specificity of this analog watchdog: Multiple channels can
  *             be selected. For example:
  *             (LL_ADC_AWD_CHANNEL4_REG_INJ | LL_ADC_AWD_CHANNEL5_REG_INJ | ...)
  *           - groups monitored: not selection possible (monitoring on both
  *             groups regular and injected).
  *             Channels selected are monitored on groups regular and injected:
  *             LL_ADC_AWD_CHANNELxx_REG_INJ (do not use parameters
  *             LL_ADC_AWD_CHANNELxx_REG and LL_ADC_AWD_CHANNELxx_INJ)
  *           - resolution: resolution is not limited (corresponds to
  *             ADC resolution configured).
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR1    AWD1CH         LL_ADC_SetAnalogWDMonitChannels\n
  *         CFGR1    AWD1SGL        LL_ADC_SetAnalogWDMonitChannels\n
  *         CFGR1    AWD1EN         LL_ADC_SetAnalogWDMonitChannels\n
  *         AWD2CR   AWD2CH         LL_ADC_SetAnalogWDMonitChannels\n
  *         AWD3CR   AWD3CH         LL_ADC_SetAnalogWDMonitChannels
  * @param  ADCx ADC instance
  * @param  AWDy This parameter can be one of the following values:
  *         @arg @ref LL_ADC_AWD1
  *         @arg @ref LL_ADC_AWD2
  *         @arg @ref LL_ADC_AWD3
  * @param  AWDChannelGroup This parameter can be one of the following values:
  *         @arg @ref LL_ADC_AWD_DISABLE
  *         @arg @ref LL_ADC_AWD_ALL_CHANNELS_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_0_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_1_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_2_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_3_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_4_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_5_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_6_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_7_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_8_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_9_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_10_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_11_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_12_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_13_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_14_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_15_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_16_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_17_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_18_REG
  *         @arg @ref LL_ADC_AWD_CH_VREFINT_REG
  *         @arg @ref LL_ADC_AWD_CH_TEMPSENSOR_REG
  *         @arg @ref LL_ADC_AWD_CH_VBAT_REG
  * @retval None
  */
__STATIC_INLINE void LL_ADC_SetAnalogWDMonitChannels(ADC_TypeDef *ADCx, uint32_t AWDy, uint32_t AWDChannelGroup)
{
  /* Set bits with content of parameter "AWDChannelGroup" with bits position  */
  /* in register and register position depending on parameter "AWDy".         */
  /* Parameters "AWDChannelGroup" and "AWDy" are used with masks because      */
  /* containing other bits reserved for other purpose.                        */
  register uint32_t *preg;
  
  if(AWDy == LL_ADC_AWD1)
  {
    preg = __ADC_PTR_REG_OFFSET(ADCx->CFGR1, 0UL);
  }
  else
  {
    preg = __ADC_PTR_REG_OFFSET(ADCx->AWD2CR, ((AWDy & ADC_AWD_CRX_REGOFFSET_MASK)) >> (ADC_AWD_CRX_REGOFFSET_BITOFFSET_POS + 1UL));
  }
  
  MODIFY_REG(*preg,
             (AWDy & ADC_AWD_CR_ALL_CHANNEL_MASK),
             AWDChannelGroup & AWDy);
}

/**
  * @brief  Get ADC analog watchdog monitored channel.
  * @note   Usage of the returned channel number:
  *         - To reinject this channel into another function LL_ADC_xxx:
  *           the returned channel number is only partly formatted on definition
  *           of literals LL_ADC_CHANNEL_x. Therefore, it has to be compared
  *           with parts of literals LL_ADC_CHANNEL_x or using
  *           helper macro @ref __LL_ADC_CHANNEL_TO_DECIMAL_NB().
  *           Then the selected literal LL_ADC_CHANNEL_x can be used
  *           as parameter for another function.
  *         - To get the channel number in decimal format:
  *           process the returned value with the helper macro
  *           @ref __LL_ADC_CHANNEL_TO_DECIMAL_NB().
  *           Applicable only when the analog watchdog is set to monitor
  *           one channel.
  * @note   On this STM32 serie, there are 2 kinds of analog watchdog
  *         instance:
  *         - AWD standard (instance AWD1):
  *           - channels monitored: can monitor 1 channel or all channels.
  *           - groups monitored: ADC group regular.
  *           - resolution: resolution is not limited (corresponds to
  *             ADC resolution configured).
  *         - AWD flexible (instances AWD2, AWD3):
  *           - channels monitored: flexible on channels monitored, selection is
  *             channel wise, from from 1 to all channels.
  *             Specificity of this analog watchdog: Multiple channels can
  *             be selected. For example:
  *             (LL_ADC_AWD_CHANNEL4_REG_INJ | LL_ADC_AWD_CHANNEL5_REG_INJ | ...)
  *           - groups monitored: not selection possible (monitoring on both
  *             groups regular and injected).
  *             Channels selected are monitored on groups regular and injected:
  *             LL_ADC_AWD_CHANNELxx_REG_INJ (do not use parameters
  *             LL_ADC_AWD_CHANNELxx_REG and LL_ADC_AWD_CHANNELxx_INJ)
  *           - resolution: resolution is not limited (corresponds to
  *             ADC resolution configured).
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR1    AWD1CH         LL_ADC_GetAnalogWDMonitChannels\n
  *         CFGR1    AWD1SGL        LL_ADC_GetAnalogWDMonitChannels\n
  *         CFGR1    AWD1EN         LL_ADC_GetAnalogWDMonitChannels\n
  *         AWD2CR   AWD2CH         LL_ADC_GetAnalogWDMonitChannels\n
  *         AWD3CR   AWD3CH         LL_ADC_GetAnalogWDMonitChannels
  * @param  ADCx ADC instance
  * @param  AWDy This parameter can be one of the following values:
  *         @arg @ref LL_ADC_AWD1
  *         @arg @ref LL_ADC_AWD2 (1)
  *         @arg @ref LL_ADC_AWD3 (1)
  *         
  *         (1) On this AWD number, monitored channel can be retrieved
  *             if only 1 channel is programmed (or none or all channels).
  *             This function cannot retrieve monitored channel if
  *             multiple channels are programmed simultaneously
  *             by bitfield.
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_AWD_DISABLE
  *         @arg @ref LL_ADC_AWD_ALL_CHANNELS_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_0_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_1_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_2_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_3_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_4_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_5_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_6_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_7_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_8_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_9_REG 
  *         @arg @ref LL_ADC_AWD_CHANNEL_10_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_11_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_12_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_13_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_14_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_15_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_16_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_17_REG
  *         @arg @ref LL_ADC_AWD_CHANNEL_18_REG
  */
__STATIC_INLINE uint32_t LL_ADC_GetAnalogWDMonitChannels(ADC_TypeDef *ADCx, uint32_t AWDy)
{
  register uint32_t *preg = __ADC_PTR_REG_OFFSET(ADCx->CFGR1, ((AWDy & ADC_AWD_CRX_REGOFFSET_MASK) >> ADC_AWD_CRX_REGOFFSET_POS)
                                                              + ((AWDy & ADC_AWD_CR12_REGOFFSETGAP_MASK) * ADC_AWD_CR12_REGOFFSETGAP_VAL));
  
  register uint32_t AnalogWDMonitChannels = (READ_BIT(*preg, AWDy) & AWDy & ADC_AWD_CR_ALL_CHANNEL_MASK);
  
  /* If "AnalogWDMonitChannels" == 0, then the selected AWD is disabled       */
  /* (parameter value LL_ADC_AWD_DISABLE).                                    */
  /* Else, the selected AWD is enabled and is monitoring a group of channels  */
  /* or a single channel.                                                     */
  if(AnalogWDMonitChannels != 0UL)
  {
    if(AWDy == LL_ADC_AWD1)
    {
      if((AnalogWDMonitChannels & ADC_CFGR1_AWD1SGL) == 0UL)
      {
        /* AWD monitoring a group of channels */
        AnalogWDMonitChannels = ((  AnalogWDMonitChannels
                                  | (ADC_AWD_CR23_CHANNEL_MASK)
                                 )
                                 & (~(ADC_CFGR1_AWD1CH))
                                );
      }
      else
      {
        /* AWD monitoring a single channel */
        AnalogWDMonitChannels = (AnalogWDMonitChannels
                                 | (ADC_AWD2CR_AWD2CH_0 << (AnalogWDMonitChannels >> ADC_CFGR1_AWD1CH_Pos))
                                );
      }
    }
    else
    {
      if((AnalogWDMonitChannels & ADC_AWD_CR23_CHANNEL_MASK) == ADC_AWD_CR23_CHANNEL_MASK)
      {
        /* AWD monitoring a group of channels */
        AnalogWDMonitChannels = (  ADC_AWD_CR23_CHANNEL_MASK
                                 | (ADC_CFGR1_AWD1EN)
                                );
      }
      else
      {
        /* AWD monitoring a single channel */
        /* AWD monitoring a group of channels */
        AnalogWDMonitChannels = (  AnalogWDMonitChannels
                                 | (ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL)
                                 | (__LL_ADC_CHANNEL_TO_DECIMAL_NB(AnalogWDMonitChannels) << ADC_CFGR1_AWD1CH_Pos)
                                );
      }
    }
  }
  
  return AnalogWDMonitChannels;
}

/**
  * @brief  Set ADC analog watchdog thresholds value of both thresholds
  *         high and low.
  * @note   If value of only one threshold high or low must be set,
  *         use function @ref LL_ADC_SetAnalogWDThresholds().
  * @note   In case of ADC resolution different of 12 bits,
  *         analog watchdog thresholds data require a specific shift.
  *         Use helper macro @ref __LL_ADC_ANALOGWD_SET_THRESHOLD_RESOLUTION().
  * @note   On this STM32 serie, there are 2 kinds of analog watchdog
  *         instance:
  *         - AWD standard (instance AWD1):
  *           - channels monitored: can monitor 1 channel or all channels.
  *           - groups monitored: ADC group regular.
  *           - resolution: resolution is not limited (corresponds to
  *             ADC resolution configured).
  *         - AWD flexible (instances AWD2, AWD3):
  *           - channels monitored: flexible on channels monitored, selection is
  *             channel wise, from from 1 to all channels.
  *             Specificity of this analog watchdog: Multiple channels can
  *             be selected. For example:
  *             (LL_ADC_AWD_CHANNEL4_REG_INJ | LL_ADC_AWD_CHANNEL5_REG_INJ | ...)
  *           - groups monitored: not selection possible (monitoring on both
  *             groups regular and injected).
  *             Channels selected are monitored on groups regular and injected:
  *             LL_ADC_AWD_CHANNELxx_REG_INJ (do not use parameters
  *             LL_ADC_AWD_CHANNELxx_REG and LL_ADC_AWD_CHANNELxx_INJ)
  *           - resolution: resolution is not limited (corresponds to
  *             ADC resolution configured).
  * @note   If ADC oversampling is enabled, ADC analog watchdog thresholds are
  *         impacted: the comparison of analog watchdog thresholds is done on
  *         oversampling final computation (after ratio and shift application):
  *         ADC data register bitfield [15:4] (12 most significant bits).
  *         Examples:
  *         - Oversampling ratio and shift selected to have ADC conversion data
  *           on 12 bits (ratio 16 and shift 4, or ratio 32 and shift 5, ...):
  *           ADC analog watchdog thresholds must be divided by 16.
  *         - Oversampling ratio and shift selected to have ADC conversion data
  *           on 14 bits (ratio 16 and shift 2, or ratio 32 and shift 3, ...):
  *           ADC analog watchdog thresholds must be divided by 4.
  *         - Oversampling ratio and shift selected to have ADC conversion data
  *           on 16 bits (ratio 16 and shift none, or ratio 32 and shift 1, ...):
  *           ADC analog watchdog thresholds match directly to ADC data register.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll TR1      HT1            LL_ADC_ConfigAnalogWDThresholds\n
  *         TR2      HT2            LL_ADC_ConfigAnalogWDThresholds\n
  *         TR3      HT3            LL_ADC_ConfigAnalogWDThresholds\n
  *         TR1      LT1            LL_ADC_ConfigAnalogWDThresholds\n
  *         TR2      LT2            LL_ADC_ConfigAnalogWDThresholds\n
  *         TR3      LT3            LL_ADC_ConfigAnalogWDThresholds
  * @param  ADCx ADC instance
  * @param  AWDy This parameter can be one of the following values:
  *         @arg @ref LL_ADC_AWD1
  *         @arg @ref LL_ADC_AWD2
  *         @arg @ref LL_ADC_AWD3
  * @param  AWDThresholdHighValue Value between Min_Data=0x000 and Max_Data=0xFFF
  * @param  AWDThresholdLowValue Value between Min_Data=0x000 and Max_Data=0xFFF
  * @retval None
  */
__STATIC_INLINE void LL_ADC_ConfigAnalogWDThresholds(ADC_TypeDef *ADCx, uint32_t AWDy, uint32_t AWDThresholdHighValue, uint32_t AWDThresholdLowValue)
{
  /* Set bits with content of parameter "AWDThresholdxxxValue" with bits      */
  /* position in register and register position depending on parameter        */
  /* "AWDy".                                                                  */
  /* Parameters "AWDy" and "AWDThresholdxxxValue" are used with masks because */
  /* containing other bits reserved for other purpose.                        */
  register uint32_t *preg = __ADC_PTR_REG_OFFSET(ADCx->TR1, (((AWDy & ADC_AWD_TRX_REGOFFSET_MASK)) >> (ADC_AWD_TRX_REGOFFSET_BITOFFSET_POS)) + ((ADC_AWD_CR3_REGOFFSET & AWDy) >> (ADC_AWD_CRX_REGOFFSET_BITOFFSET_POS + 1UL)));
  
  MODIFY_REG(*preg,
             ADC_TR1_HT1 | ADC_TR1_LT1,
             (AWDThresholdHighValue << ADC_TR1_HT1_BITOFFSET_POS) | AWDThresholdLowValue);
}

/**
  * @brief  Set ADC analog watchdog threshold value of threshold
  *         high or low.
  * @note   If values of both thresholds high or low must be set,
  *         use function @ref LL_ADC_ConfigAnalogWDThresholds().
  * @note   In case of ADC resolution different of 12 bits,
  *         analog watchdog thresholds data require a specific shift.
  *         Use helper macro @ref __LL_ADC_ANALOGWD_SET_THRESHOLD_RESOLUTION().
  * @note   On this STM32 serie, there are 2 kinds of analog watchdog
  *         instance:
  *         - AWD standard (instance AWD1):
  *           - channels monitored: can monitor 1 channel or all channels.
  *           - groups monitored: ADC group regular.
  *           - resolution: resolution is not limited (corresponds to
  *             ADC resolution configured).
  *         - AWD flexible (instances AWD2, AWD3):
  *           - channels monitored: flexible on channels monitored, selection is
  *             channel wise, from from 1 to all channels.
  *             Specificity of this analog watchdog: Multiple channels can
  *             be selected. For example:
  *             (LL_ADC_AWD_CHANNEL4_REG_INJ | LL_ADC_AWD_CHANNEL5_REG_INJ | ...)
  *           - groups monitored: not selection possible (monitoring on both
  *             groups regular and injected).
  *             Channels selected are monitored on groups regular and injected:
  *             LL_ADC_AWD_CHANNELxx_REG_INJ (do not use parameters
  *             LL_ADC_AWD_CHANNELxx_REG and LL_ADC_AWD_CHANNELxx_INJ)
  *           - resolution: resolution is not limited (corresponds to
  *             ADC resolution configured).
  * @note   If ADC oversampling is enabled, ADC analog watchdog thresholds are
  *         impacted: the comparison of analog watchdog thresholds is done on
  *         oversampling final computation (after ratio and shift application):
  *         ADC data register bitfield [15:4] (12 most significant bits).
  *         Examples:
  *         - Oversampling ratio and shift selected to have ADC conversion data
  *           on 12 bits (ratio 16 and shift 4, or ratio 32 and shift 5, ...):
  *           ADC analog watchdog thresholds must be divided by 16.
  *         - Oversampling ratio and shift selected to have ADC conversion data
  *           on 14 bits (ratio 16 and shift 2, or ratio 32 and shift 3, ...):
  *           ADC analog watchdog thresholds must be divided by 4.
  *         - Oversampling ratio and shift selected to have ADC conversion data
  *           on 16 bits (ratio 16 and shift none, or ratio 32 and shift 1, ...):
  *           ADC analog watchdog thresholds match directly to ADC data register.
  * @note   On this STM32 serie, setting of this feature is not conditioned to
  *         ADC state:
  *         ADC can be disabled, enabled with or without conversion on going
  *         on ADC group regular.
  * @rmtoll TR1      HT1            LL_ADC_SetAnalogWDThresholds\n
  *         TR2      HT2            LL_ADC_SetAnalogWDThresholds\n
  *         TR3      HT3            LL_ADC_SetAnalogWDThresholds\n
  *         TR1      LT1            LL_ADC_SetAnalogWDThresholds\n
  *         TR2      LT2            LL_ADC_SetAnalogWDThresholds\n
  *         TR3      LT3            LL_ADC_SetAnalogWDThresholds
  * @param  ADCx ADC instance
  * @param  AWDy This parameter can be one of the following values:
  *         @arg @ref LL_ADC_AWD1
  *         @arg @ref LL_ADC_AWD2
  *         @arg @ref LL_ADC_AWD3
  * @param  AWDThresholdsHighLow This parameter can be one of the following values:
  *         @arg @ref LL_ADC_AWD_THRESHOLD_HIGH
  *         @arg @ref LL_ADC_AWD_THRESHOLD_LOW
  * @param  AWDThresholdValue Value between Min_Data=0x000 and Max_Data=0xFFF
  * @retval None
  */
__STATIC_INLINE void LL_ADC_SetAnalogWDThresholds(ADC_TypeDef *ADCx, uint32_t AWDy, uint32_t AWDThresholdsHighLow, uint32_t AWDThresholdValue)
{
  /* Set bits with content of parameter "AWDThresholdValue" with bits         */
  /* position in register and register position depending on parameters       */
  /* "AWDThresholdsHighLow" and "AWDy".                                       */
  /* Parameters "AWDy" and "AWDThresholdValue" are used with masks because    */
  /* containing other bits reserved for other purpose.                        */
  register uint32_t *preg = __ADC_PTR_REG_OFFSET(ADCx->TR1, (((AWDy & ADC_AWD_TRX_REGOFFSET_MASK)) >> (ADC_AWD_TRX_REGOFFSET_BITOFFSET_POS)) + ((ADC_AWD_CR3_REGOFFSET & AWDy) >> (ADC_AWD_CRX_REGOFFSET_BITOFFSET_POS + 1UL)));
  
  MODIFY_REG(*preg,
             AWDThresholdsHighLow,
             AWDThresholdValue << ((AWDThresholdsHighLow & ADC_AWD_TRX_BIT_HIGH_MASK) >> ADC_AWD_TRX_BIT_HIGH_SHIFT4));
}

/**
  * @brief  Get ADC analog watchdog threshold value of threshold high,
  *         threshold low or raw data with ADC thresholds high and low
  *         concatenated.
  * @note   If raw data with ADC thresholds high and low is retrieved,
  *         the data of each threshold high or low can be isolated
  *         using helper macro:
  *         @ref __LL_ADC_ANALOGWD_THRESHOLDS_HIGH_LOW().
  * @note   In case of ADC resolution different of 12 bits,
  *         analog watchdog thresholds data require a specific shift.
  *         Use helper macro @ref __LL_ADC_ANALOGWD_GET_THRESHOLD_RESOLUTION().
  * @rmtoll TR1      HT1            LL_ADC_GetAnalogWDThresholds\n
  *         TR2      HT2            LL_ADC_GetAnalogWDThresholds\n
  *         TR3      HT3            LL_ADC_GetAnalogWDThresholds\n
  *         TR1      LT1            LL_ADC_GetAnalogWDThresholds\n
  *         TR2      LT2            LL_ADC_GetAnalogWDThresholds\n
  *         TR3      LT3            LL_ADC_GetAnalogWDThresholds
  * @param  ADCx ADC instance
  * @param  AWDy This parameter can be one of the following values:
  *         @arg @ref LL_ADC_AWD1
  *         @arg @ref LL_ADC_AWD2
  *         @arg @ref LL_ADC_AWD3
  * @param  AWDThresholdsHighLow This parameter can be one of the following values:
  *         @arg @ref LL_ADC_AWD_THRESHOLD_HIGH
  *         @arg @ref LL_ADC_AWD_THRESHOLD_LOW
  *         @arg @ref LL_ADC_AWD_THRESHOLDS_HIGH_LOW
  * @retval Value between Min_Data=0x000 and Max_Data=0xFFF
*/
__STATIC_INLINE uint32_t LL_ADC_GetAnalogWDThresholds(ADC_TypeDef *ADCx, uint32_t AWDy, uint32_t AWDThresholdsHighLow)
{
  /* Set bits with content of parameter "AWDThresholdValue" with bits         */
  /* position in register and register position depending on parameters       */
  /* "AWDThresholdsHighLow" and "AWDy".                                       */
  /* Parameters "AWDy" and "AWDThresholdValue" are used with masks because    */
  /* containing other bits reserved for other purpose.                        */
  register const uint32_t *preg = __ADC_PTR_REG_OFFSET(ADCx->TR1, (((AWDy & ADC_AWD_TRX_REGOFFSET_MASK)) >> (ADC_AWD_TRX_REGOFFSET_BITOFFSET_POS)) + ((ADC_AWD_CR3_REGOFFSET & AWDy) >> (ADC_AWD_CRX_REGOFFSET_BITOFFSET_POS + 1UL)));
  
  return (uint32_t)(READ_BIT(*preg,
                             (AWDThresholdsHighLow | ADC_TR1_LT1))
                    >> (((AWDThresholdsHighLow & ADC_AWD_TRX_BIT_HIGH_MASK) >> ADC_AWD_TRX_BIT_HIGH_SHIFT4) & ~(AWDThresholdsHighLow & ADC_TR1_LT1))
                   );
}

/**
  * @}
  */

/** @defgroup ADC_LL_EF_Configuration_ADC_oversampling Configuration of ADC transversal scope: oversampling
  * @{
  */

/**
  * @brief  Set ADC oversampling scope.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR2    OVSE           LL_ADC_SetOverSamplingScope
  * @param  ADCx ADC instance
  * @param  OvsScope This parameter can be one of the following values:
  *         @arg @ref LL_ADC_OVS_DISABLE
  *         @arg @ref LL_ADC_OVS_GRP_REGULAR_CONTINUED
  * @retval None
  */
__STATIC_INLINE void LL_ADC_SetOverSamplingScope(ADC_TypeDef *ADCx, uint32_t OvsScope)
{
  MODIFY_REG(ADCx->CFGR2, ADC_CFGR2_OVSE, OvsScope);
}

/**
  * @brief  Get ADC oversampling scope.
  * @rmtoll CFGR2    OVSE           LL_ADC_GetOverSamplingScope
  * @param  ADCx ADC instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_OVS_DISABLE
  *         @arg @ref LL_ADC_OVS_GRP_REGULAR_CONTINUED
  */
__STATIC_INLINE uint32_t LL_ADC_GetOverSamplingScope(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CFGR2, ADC_CFGR2_OVSE));
}

/**
  * @brief  Set ADC oversampling discontinuous mode (triggered mode)
  *         on the selected ADC group.
  * @note   Number of oversampled conversions are done either in:
  *         - continuous mode (all conversions of oversampling ratio
  *           are done from 1 trigger)
  *         - discontinuous mode (each conversion of oversampling ratio
  *           needs a trigger)
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR2    TOVS           LL_ADC_SetOverSamplingDiscont
  * @param  ADCx ADC instance
  * @param  OverSamplingDiscont This parameter can be one of the following values:
  *         @arg @ref LL_ADC_OVS_REG_CONT
  *         @arg @ref LL_ADC_OVS_REG_DISCONT
  * @retval None
  */
__STATIC_INLINE void LL_ADC_SetOverSamplingDiscont(ADC_TypeDef *ADCx, uint32_t OverSamplingDiscont)
{
  MODIFY_REG(ADCx->CFGR2, ADC_CFGR2_TOVS, OverSamplingDiscont);
}

/**
  * @brief  Get ADC oversampling discontinuous mode (triggered mode)
  *         on the selected ADC group.
  * @note   Number of oversampled conversions are done either in:
  *         - continuous mode (all conversions of oversampling ratio
  *           are done from 1 trigger)
  *         - discontinuous mode (each conversion of oversampling ratio
  *           needs a trigger)
  * @rmtoll CFGR2    TOVS           LL_ADC_GetOverSamplingDiscont
  * @param  ADCx ADC instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_ADC_OVS_REG_CONT
  *         @arg @ref LL_ADC_OVS_REG_DISCONT
  */
__STATIC_INLINE uint32_t LL_ADC_GetOverSamplingDiscont(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CFGR2, ADC_CFGR2_TOVS));
}

/**
  * @brief  Set ADC oversampling
  * @note   This function set the 2 items of oversampling configuration:
  *         - ratio
  *         - shift
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be disabled or enabled without conversion on going
  *         on group regular.
  * @rmtoll CFGR2    OVSS           LL_ADC_ConfigOverSamplingRatioShift\n
  *         CFGR2    OVSR           LL_ADC_ConfigOverSamplingRatioShift
  * @param  ADCx ADC instance
  * @param  Ratio This parameter can be one of the following values:
  *         @arg @ref LL_ADC_OVS_RATIO_2
  *         @arg @ref LL_ADC_OVS_RATIO_4
  *         @arg @ref LL_ADC_OVS_RATIO_8
  *         @arg @ref LL_ADC_OVS_RATIO_16
  *         @arg @ref LL_ADC_OVS_RATIO_32
  *         @arg @ref LL_ADC_OVS_RATIO_64
  *         @arg @ref LL_ADC_OVS_RATIO_128
  *         @arg @ref LL_ADC_OVS_RATIO_256
  * @param  Shift This parameter can be one of the following values:
  *         @arg @ref LL_ADC_OVS_SHIFT_NONE
  *         @arg @ref LL_ADC_OVS_SHIFT_RIGHT_1
  *         @arg @ref LL_ADC_OVS_SHIFT_RIGHT_2
  *         @arg @ref LL_ADC_OVS_SHIFT_RIGHT_3
  *         @arg @ref LL_ADC_OVS_SHIFT_RIGHT_4
  *         @arg @ref LL_ADC_OVS_SHIFT_RIGHT_5
  *         @arg @ref LL_ADC_OVS_SHIFT_RIGHT_6
  *         @arg @ref LL_ADC_OVS_SHIFT_RIGHT_7
  *         @arg @ref LL_ADC_OVS_SHIFT_RIGHT_8
  * @retval None
  */
__STATIC_INLINE void LL_ADC_ConfigOverSamplingRatioShift(ADC_TypeDef *ADCx, uint32_t Ratio, uint32_t Shift)
{
  MODIFY_REG(ADCx->CFGR2, (ADC_CFGR2_OVSS | ADC_CFGR2_OVSR), (Shift | Ratio));
}

/**
  * @brief  Get ADC oversampling ratio
  * @rmtoll CFGR2    OVSR           LL_ADC_GetOverSamplingRatio
  * @param  ADCx ADC instance
  * @retval Ratio This parameter can be one of the following values:
  *         @arg @ref LL_ADC_OVS_RATIO_2
  *         @arg @ref LL_ADC_OVS_RATIO_4
  *         @arg @ref LL_ADC_OVS_RATIO_8
  *         @arg @ref LL_ADC_OVS_RATIO_16
  *         @arg @ref LL_ADC_OVS_RATIO_32
  *         @arg @ref LL_ADC_OVS_RATIO_64
  *         @arg @ref LL_ADC_OVS_RATIO_128
  *         @arg @ref LL_ADC_OVS_RATIO_256
*/
__STATIC_INLINE uint32_t LL_ADC_GetOverSamplingRatio(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CFGR2, ADC_CFGR2_OVSR));
}

/**
  * @brief  Get ADC oversampling shift
  * @rmtoll CFGR2    OVSS           LL_ADC_GetOverSamplingShift
  * @param  ADCx ADC instance
  * @retval Shift This parameter can be one of the following values:
  *         @arg @ref LL_ADC_OVS_SHIFT_NONE
  *         @arg @ref LL_ADC_OVS_SHIFT_RIGHT_1
  *         @arg @ref LL_ADC_OVS_SHIFT_RIGHT_2
  *         @arg @ref LL_ADC_OVS_SHIFT_RIGHT_3
  *         @arg @ref LL_ADC_OVS_SHIFT_RIGHT_4
  *         @arg @ref LL_ADC_OVS_SHIFT_RIGHT_5
  *         @arg @ref LL_ADC_OVS_SHIFT_RIGHT_6
  *         @arg @ref LL_ADC_OVS_SHIFT_RIGHT_7
  *         @arg @ref LL_ADC_OVS_SHIFT_RIGHT_8
*/
__STATIC_INLINE uint32_t LL_ADC_GetOverSamplingShift(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->CFGR2, ADC_CFGR2_OVSS));
}

/**
  * @}
  */

/** @defgroup ADC_LL_EF_Operation_ADC_Instance Operation on ADC hierarchical scope: ADC instance
  * @{
  */

/**
  * @brief  Enable ADC instance internal voltage regulator.
  * @note   On this STM32 serie, there are three possibilities to enable
  *         the voltage regulator:
  *         - by enabling it manually
  *           using function @ref LL_ADC_EnableInternalRegulator().
  *         - by launching a calibration
  *           using function @ref LL_ADC_StartCalibration().
  *         - by enabling the ADC
  *           using function @ref LL_ADC_Enable().
  * @note   On this STM32 serie, after ADC internal voltage regulator enable,
  *         a delay for ADC internal voltage regulator stabilization
  *         is required before performing a ADC calibration or ADC enable.
  *         Refer to device datasheet, parameter "tADCVREG_STUP".
  *         Refer to literal @ref LL_ADC_DELAY_INTERNAL_REGUL_STAB_US.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be ADC disabled.
  * @rmtoll CR       ADVREGEN       LL_ADC_EnableInternalRegulator
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_EnableInternalRegulator(ADC_TypeDef *ADCx)
{
  /* Note: Write register with some additional bits forced to state reset     */
  /*       instead of modifying only the selected bit for this function,      */
  /*       to not interfere with bits with HW property "rs".                  */
  MODIFY_REG(ADCx->CR,
             ADC_CR_BITS_PROPERTY_RS,
             ADC_CR_ADVREGEN);
}

/**
  * @brief  Disable ADC internal voltage regulator.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be ADC disabled.
  * @rmtoll CR       ADVREGEN       LL_ADC_DisableInternalRegulator
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_DisableInternalRegulator(ADC_TypeDef *ADCx)
{
  CLEAR_BIT(ADCx->CR, (ADC_CR_ADVREGEN | ADC_CR_BITS_PROPERTY_RS));
}

/**
  * @brief  Get the selected ADC instance internal voltage regulator state.
  * @rmtoll CR       ADVREGEN       LL_ADC_IsInternalRegulatorEnabled
  * @param  ADCx ADC instance
  * @retval 0: internal regulator is disabled, 1: internal regulator is enabled.
  */
__STATIC_INLINE uint32_t LL_ADC_IsInternalRegulatorEnabled(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->CR, ADC_CR_ADVREGEN) == (ADC_CR_ADVREGEN)) ? 1UL : 0UL);
}

/**
  * @brief  Enable the selected ADC instance.
  * @note   On this STM32 serie, after ADC enable, a delay for 
  *         ADC internal analog stabilization is required before performing a
  *         ADC conversion start.
  *         Refer to device datasheet, parameter tSTAB.
  * @note   On this STM32 serie, flag LL_ADC_FLAG_ADRDY is raised when the ADC
  *         is enabled and when conversion clock is active.
  *         (not only core clock: this ADC has a dual clock domain)
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be ADC disabled and ADC internal voltage regulator enabled.
  * @rmtoll CR       ADEN           LL_ADC_Enable
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_Enable(ADC_TypeDef *ADCx)
{
  /* Note: Write register with some additional bits forced to state reset     */
  /*       instead of modifying only the selected bit for this function,      */
  /*       to not interfere with bits with HW property "rs".                  */
  MODIFY_REG(ADCx->CR,
             ADC_CR_BITS_PROPERTY_RS,
             ADC_CR_ADEN);
}

/**
  * @brief  Disable the selected ADC instance.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be not disabled. Must be enabled without conversion on going
  *         on group regular.
  * @rmtoll CR       ADDIS          LL_ADC_Disable
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_Disable(ADC_TypeDef *ADCx)
{
  /* Note: Write register with some additional bits forced to state reset     */
  /*       instead of modifying only the selected bit for this function,      */
  /*       to not interfere with bits with HW property "rs".                  */
  MODIFY_REG(ADCx->CR,
             ADC_CR_BITS_PROPERTY_RS,
             ADC_CR_ADDIS);
}

/**
  * @brief  Get the selected ADC instance enable state.
  * @note   On this STM32 serie, flag LL_ADC_FLAG_ADRDY is raised when the ADC
  *         is enabled and when conversion clock is active.
  *         (not only core clock: this ADC has a dual clock domain)
  * @rmtoll CR       ADEN           LL_ADC_IsEnabled
  * @param  ADCx ADC instance
  * @retval 0: ADC is disabled, 1: ADC is enabled.
  */
__STATIC_INLINE uint32_t LL_ADC_IsEnabled(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->CR, ADC_CR_ADEN) == (ADC_CR_ADEN)) ? 1UL : 0UL);
}

/**
  * @brief  Get the selected ADC instance disable state.
  * @rmtoll CR       ADDIS          LL_ADC_IsDisableOngoing
  * @param  ADCx ADC instance
  * @retval 0: no ADC disable command on going.
  */
__STATIC_INLINE uint32_t LL_ADC_IsDisableOngoing(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->CR, ADC_CR_ADDIS) == (ADC_CR_ADDIS)) ? 1UL : 0UL);
}

/**
  * @brief  Start ADC calibration in the mode single-ended
  *         or differential (for devices with differential mode available).
  * @note   On this STM32 serie, a minimum number of ADC clock cycles
  *         are required between ADC end of calibration and ADC enable.
  *         Refer to literal @ref LL_ADC_DELAY_CALIB_ENABLE_ADC_CYCLES.
  * @note   In case of usage of ADC with DMA transfer:
  *         On this STM32 serie, ADC DMA transfer request should be disabled
  *         during calibration:
  *         Calibration factor is available in data register
  *         and also transfered by DMA.
  *         To not insert ADC calibration factor among ADC conversion data
  *         in array variable, DMA transfer must be disabled during
  *         calibration.
  *         (DMA transfer setting backup and disable before calibration,
  *         DMA transfer setting restore after calibration.
  *         Refer to functions @ref LL_ADC_REG_GetDMATransfer(),
  *         @ref LL_ADC_REG_SetDMATransfer() ).
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be ADC disabled.
  * @rmtoll CR       ADCAL          LL_ADC_StartCalibration
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_StartCalibration(ADC_TypeDef *ADCx)
{
  /* Note: Write register with some additional bits forced to state reset     */
  /*       instead of modifying only the selected bit for this function,      */
  /*       to not interfere with bits with HW property "rs".                  */
  MODIFY_REG(ADCx->CR,
             ADC_CR_BITS_PROPERTY_RS,
             ADC_CR_ADCAL);
}

/**
  * @brief  Get ADC calibration state.
  * @rmtoll CR       ADCAL          LL_ADC_IsCalibrationOnGoing
  * @param  ADCx ADC instance
  * @retval 0: calibration complete, 1: calibration in progress.
  */
__STATIC_INLINE uint32_t LL_ADC_IsCalibrationOnGoing(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->CR, ADC_CR_ADCAL) == (ADC_CR_ADCAL)) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup ADC_LL_EF_Operation_ADC_Group_Regular Operation on ADC hierarchical scope: group regular
  * @{
  */

/**
  * @brief  Start ADC group regular conversion.
  * @note   On this STM32 serie, this function is relevant for both 
  *         internal trigger (SW start) and external trigger:
  *         - If ADC trigger has been set to software start, ADC conversion
  *           starts immediately.
  *         - If ADC trigger has been set to external trigger, ADC conversion
  *           will start at next trigger event (on the selected trigger edge)
  *           following the ADC start conversion command.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be enabled without conversion on going on group regular,
  *         without conversion stop command on going on group regular,
  *         without ADC disable command on going.
  * @rmtoll CR       ADSTART        LL_ADC_REG_StartConversion
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_REG_StartConversion(ADC_TypeDef *ADCx)
{
  /* Note: Write register with some additional bits forced to state reset     */
  /*       instead of modifying only the selected bit for this function,      */
  /*       to not interfere with bits with HW property "rs".                  */
  MODIFY_REG(ADCx->CR,
             ADC_CR_BITS_PROPERTY_RS,
             ADC_CR_ADSTART);
}

/**
  * @brief  Stop ADC group regular conversion.
  * @note   On this STM32 serie, setting of this feature is conditioned to
  *         ADC state:
  *         ADC must be enabled with conversion on going on group regular,
  *         without ADC disable command on going.
  * @rmtoll CR       ADSTP          LL_ADC_REG_StopConversion
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_REG_StopConversion(ADC_TypeDef *ADCx)
{
  /* Note: Write register with some additional bits forced to state reset     */
  /*       instead of modifying only the selected bit for this function,      */
  /*       to not interfere with bits with HW property "rs".                  */
  MODIFY_REG(ADCx->CR,
             ADC_CR_BITS_PROPERTY_RS,
             ADC_CR_ADSTP);
}

/**
  * @brief  Get ADC group regular conversion state.
  * @rmtoll CR       ADSTART        LL_ADC_REG_IsConversionOngoing
  * @param  ADCx ADC instance
  * @retval 0: no conversion is on going on ADC group regular.
  */
__STATIC_INLINE uint32_t LL_ADC_REG_IsConversionOngoing(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->CR, ADC_CR_ADSTART) == (ADC_CR_ADSTART)) ? 1UL : 0UL);
}

/**
  * @brief  Get ADC group regular command of conversion stop state
  * @rmtoll CR       ADSTP          LL_ADC_REG_IsStopConversionOngoing
  * @param  ADCx ADC instance
  * @retval 0: no command of conversion stop is on going on ADC group regular.
  */
__STATIC_INLINE uint32_t LL_ADC_REG_IsStopConversionOngoing(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->CR, ADC_CR_ADSTP) == (ADC_CR_ADSTP)) ? 1UL : 0UL);
}

/**
  * @brief  Get ADC group regular conversion data, range fit for
  *         all ADC configurations: all ADC resolutions and
  *         all oversampling increased data width (for devices
  *         with feature oversampling).
  * @rmtoll DR       DATA           LL_ADC_REG_ReadConversionData32
  * @param  ADCx ADC instance
  * @retval Value between Min_Data=0x00000000 and Max_Data=0xFFFFFFFF
  */
__STATIC_INLINE uint32_t LL_ADC_REG_ReadConversionData32(ADC_TypeDef *ADCx)
{
  return (uint32_t)(READ_BIT(ADCx->DR, ADC_DR_DATA));
}

/**
  * @brief  Get ADC group regular conversion data, range fit for
  *         ADC resolution 12 bits.
  * @note   For devices with feature oversampling: Oversampling
  *         can increase data width, function for extended range
  *         may be needed: @ref LL_ADC_REG_ReadConversionData32.
  * @rmtoll DR       DATA           LL_ADC_REG_ReadConversionData12
  * @param  ADCx ADC instance
  * @retval Value between Min_Data=0x000 and Max_Data=0xFFF
  */
__STATIC_INLINE uint16_t LL_ADC_REG_ReadConversionData12(ADC_TypeDef *ADCx)
{
  return (uint16_t)(READ_BIT(ADCx->DR, ADC_DR_DATA));
}

/**
  * @brief  Get ADC group regular conversion data, range fit for
  *         ADC resolution 10 bits.
  * @note   For devices with feature oversampling: Oversampling
  *         can increase data width, function for extended range
  *         may be needed: @ref LL_ADC_REG_ReadConversionData32.
  * @rmtoll DR       DATA           LL_ADC_REG_ReadConversionData10
  * @param  ADCx ADC instance
  * @retval Value between Min_Data=0x000 and Max_Data=0x3FF
  */
__STATIC_INLINE uint16_t LL_ADC_REG_ReadConversionData10(ADC_TypeDef *ADCx)
{
  return (uint16_t)(READ_BIT(ADCx->DR, ADC_DR_DATA));
}

/**
  * @brief  Get ADC group regular conversion data, range fit for
  *         ADC resolution 8 bits.
  * @note   For devices with feature oversampling: Oversampling
  *         can increase data width, function for extended range
  *         may be needed: @ref LL_ADC_REG_ReadConversionData32.
  * @rmtoll DR       DATA           LL_ADC_REG_ReadConversionData8
  * @param  ADCx ADC instance
  * @retval Value between Min_Data=0x00 and Max_Data=0xFF
  */
__STATIC_INLINE uint8_t LL_ADC_REG_ReadConversionData8(ADC_TypeDef *ADCx)
{
  return (uint8_t)(READ_BIT(ADCx->DR, ADC_DR_DATA));
}

/**
  * @brief  Get ADC group regular conversion data, range fit for
  *         ADC resolution 6 bits.
  * @note   For devices with feature oversampling: Oversampling
  *         can increase data width, function for extended range
  *         may be needed: @ref LL_ADC_REG_ReadConversionData32.
  * @rmtoll DR       DATA           LL_ADC_REG_ReadConversionData6
  * @param  ADCx ADC instance
  * @retval Value between Min_Data=0x00 and Max_Data=0x3F
  */
__STATIC_INLINE uint8_t LL_ADC_REG_ReadConversionData6(ADC_TypeDef *ADCx)
{
  return (uint8_t)(READ_BIT(ADCx->DR, ADC_DR_DATA));
}

/**
  * @}
  */

/** @defgroup ADC_LL_EF_FLAG_Management ADC flag management
  * @{
  */

/**
  * @brief  Get flag ADC ready.
  * @note   On this STM32 serie, flag LL_ADC_FLAG_ADRDY is raised when the ADC
  *         is enabled and when conversion clock is active.
  *         (not only core clock: this ADC has a dual clock domain)
  * @rmtoll ISR      ADRDY          LL_ADC_IsActiveFlag_ADRDY
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsActiveFlag_ADRDY(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->ISR, LL_ADC_FLAG_ADRDY) == (LL_ADC_FLAG_ADRDY)) ? 1UL : 0UL);
}

/**
  * @brief  Get flag ADC channel configuration ready.
  * @note   Duration of ADC channel configuration ready: CCRDY handshake
  *         requires 1APB + 2 ADC + 3 APB cycles after the channel configuration
  *         has been changed.
  * @rmtoll ISR      CCRDY          LL_ADC_IsActiveFlag_CCRDY
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsActiveFlag_CCRDY(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->ISR, LL_ADC_FLAG_CCRDY) == (LL_ADC_FLAG_CCRDY)) ? 1UL : 0UL);
}

/**
  * @brief  Get flag ADC group regular end of unitary conversion.
  * @rmtoll ISR      EOC            LL_ADC_IsActiveFlag_EOC
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsActiveFlag_EOC(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->ISR, ADC_ISR_EOC) == (ADC_ISR_EOC)) ? 1UL : 0UL);
}

/**
  * @brief  Get flag ADC group regular end of sequence conversions.
  * @rmtoll ISR      EOS            LL_ADC_IsActiveFlag_EOS
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsActiveFlag_EOS(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->ISR, LL_ADC_FLAG_EOS) == (LL_ADC_FLAG_EOS)) ? 1UL : 0UL);
}

/**
  * @brief  Get flag ADC group regular overrun.
  * @rmtoll ISR      OVR            LL_ADC_IsActiveFlag_OVR
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsActiveFlag_OVR(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->ISR, LL_ADC_FLAG_OVR) == (LL_ADC_FLAG_OVR)) ? 1UL : 0UL);
}

/**
  * @brief  Get flag ADC group regular end of sampling phase.
  * @rmtoll ISR      EOSMP          LL_ADC_IsActiveFlag_EOSMP
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsActiveFlag_EOSMP(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->ISR, LL_ADC_FLAG_EOSMP) == (LL_ADC_FLAG_EOSMP)) ? 1UL : 0UL);
}

/**
  * @brief  Get flag ADC analog watchdog 1 flag
  * @rmtoll ISR      AWD1           LL_ADC_IsActiveFlag_AWD1
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsActiveFlag_AWD1(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->ISR, LL_ADC_FLAG_AWD1) == (LL_ADC_FLAG_AWD1)) ? 1UL : 0UL);
}

/**
  * @brief  Get flag ADC analog watchdog 2.
  * @rmtoll ISR      AWD2           LL_ADC_IsActiveFlag_AWD2
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsActiveFlag_AWD2(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->ISR, LL_ADC_FLAG_AWD2) == (LL_ADC_FLAG_AWD2)) ? 1UL : 0UL);
}

/**
  * @brief  Get flag ADC analog watchdog 3.
  * @rmtoll ISR      AWD3           LL_ADC_IsActiveFlag_AWD3
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsActiveFlag_AWD3(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->ISR, LL_ADC_FLAG_AWD3) == (LL_ADC_FLAG_AWD3)) ? 1UL : 0UL);
}

/**
  * @brief  Get flag ADC end of calibration.
  * @rmtoll ISR      EOCAL          LL_ADC_IsActiveFlag_EOCAL
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsActiveFlag_EOCAL(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->ISR, LL_ADC_FLAG_EOCAL) == (LL_ADC_FLAG_EOCAL)) ? 1UL : 0UL);
}

/**
  * @brief  Clear flag ADC ready.
  * @note   On this STM32 serie, flag LL_ADC_FLAG_ADRDY is raised when the ADC
  *         is enabled and when conversion clock is active.
  *         (not only core clock: this ADC has a dual clock domain)
  * @rmtoll ISR      ADRDY          LL_ADC_ClearFlag_ADRDY
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_ClearFlag_ADRDY(ADC_TypeDef *ADCx)
{
  WRITE_REG(ADCx->ISR, LL_ADC_FLAG_ADRDY);
}

/**
  * @brief  Clear flag ADC channel configuration ready.
  * @rmtoll ISR      CCRDY          LL_ADC_ClearFlag_CCRDY
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE void LL_ADC_ClearFlag_CCRDY(ADC_TypeDef *ADCx)
{
  WRITE_REG(ADCx->ISR, LL_ADC_FLAG_CCRDY);
}

/**
  * @brief  Clear flag ADC group regular end of unitary conversion.
  * @rmtoll ISR      EOC            LL_ADC_ClearFlag_EOC
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_ClearFlag_EOC(ADC_TypeDef *ADCx)
{
  WRITE_REG(ADCx->ISR, LL_ADC_FLAG_EOC);
}

/**
  * @brief  Clear flag ADC group regular end of sequence conversions.
  * @rmtoll ISR      EOS            LL_ADC_ClearFlag_EOS
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_ClearFlag_EOS(ADC_TypeDef *ADCx)
{
  WRITE_REG(ADCx->ISR, LL_ADC_FLAG_EOS);
}

/**
  * @brief  Clear flag ADC group regular overrun.
  * @rmtoll ISR      OVR            LL_ADC_ClearFlag_OVR
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_ClearFlag_OVR(ADC_TypeDef *ADCx)
{
  WRITE_REG(ADCx->ISR, LL_ADC_FLAG_OVR);
}

/**
  * @brief  Clear flag ADC group regular end of sampling phase.
  * @rmtoll ISR      EOSMP          LL_ADC_ClearFlag_EOSMP
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_ClearFlag_EOSMP(ADC_TypeDef *ADCx)
{
  WRITE_REG(ADCx->ISR, LL_ADC_FLAG_EOSMP);
}

/**
  * @brief  Clear flag ADC analog watchdog 1.
  * @rmtoll ISR      AWD1           LL_ADC_ClearFlag_AWD1
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_ClearFlag_AWD1(ADC_TypeDef *ADCx)
{
  WRITE_REG(ADCx->ISR, LL_ADC_FLAG_AWD1);
}

/**
  * @brief  Clear flag ADC analog watchdog 2.
  * @rmtoll ISR      AWD2           LL_ADC_ClearFlag_AWD2
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_ClearFlag_AWD2(ADC_TypeDef *ADCx)
{
  WRITE_REG(ADCx->ISR, LL_ADC_FLAG_AWD2);
}

/**
  * @brief  Clear flag ADC analog watchdog 3.
  * @rmtoll ISR      AWD3           LL_ADC_ClearFlag_AWD3
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_ClearFlag_AWD3(ADC_TypeDef *ADCx)
{
  WRITE_REG(ADCx->ISR, LL_ADC_FLAG_AWD3);
}

/**
  * @brief  Clear flag ADC end of calibration.
  * @rmtoll ISR      EOCAL          LL_ADC_ClearFlag_EOCAL
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_ClearFlag_EOCAL(ADC_TypeDef *ADCx)
{
  WRITE_REG(ADCx->ISR, LL_ADC_FLAG_EOCAL);
}

/**
  * @}
  */

/** @defgroup ADC_LL_EF_IT_Management ADC IT management
  * @{
  */

/**
  * @brief  Enable ADC ready.
  * @rmtoll IER      ADRDYIE        LL_ADC_EnableIT_ADRDY
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_EnableIT_ADRDY(ADC_TypeDef *ADCx)
{
  SET_BIT(ADCx->IER, LL_ADC_IT_ADRDY);
}

/**
  * @brief  Enable interruption ADC channel configuration ready.
  * @rmtoll IER      ADRDYIE        LL_ADC_EnableIT_CCRDY
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE void LL_ADC_EnableIT_CCRDY(ADC_TypeDef *ADCx)
{
  SET_BIT(ADCx->IER, LL_ADC_FLAG_CCRDY);
}

/**
  * @brief  Enable interruption ADC group regular end of unitary conversion.
  * @rmtoll IER      EOCIE          LL_ADC_EnableIT_EOC
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_EnableIT_EOC(ADC_TypeDef *ADCx)
{
  SET_BIT(ADCx->IER, LL_ADC_IT_EOC);
}

/**
  * @brief  Enable interruption ADC group regular end of sequence conversions.
  * @rmtoll IER      EOSIE          LL_ADC_EnableIT_EOS
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_EnableIT_EOS(ADC_TypeDef *ADCx)
{
  SET_BIT(ADCx->IER, LL_ADC_IT_EOS);
}

/**
  * @brief  Enable ADC group regular interruption overrun.
  * @rmtoll IER      OVRIE          LL_ADC_EnableIT_OVR
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_EnableIT_OVR(ADC_TypeDef *ADCx)
{
  SET_BIT(ADCx->IER, LL_ADC_IT_OVR);
}

/**
  * @brief  Enable interruption ADC group regular end of sampling.
  * @rmtoll IER      EOSMPIE        LL_ADC_EnableIT_EOSMP
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_EnableIT_EOSMP(ADC_TypeDef *ADCx)
{
  SET_BIT(ADCx->IER, LL_ADC_IT_EOSMP);
}

/**
  * @brief  Enable interruption ADC analog watchdog 1.
  * @rmtoll IER      AWD1IE         LL_ADC_EnableIT_AWD1
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_EnableIT_AWD1(ADC_TypeDef *ADCx)
{
  SET_BIT(ADCx->IER, LL_ADC_IT_AWD1);
}

/**
  * @brief  Enable interruption ADC analog watchdog 2.
  * @rmtoll IER      AWD2IE         LL_ADC_EnableIT_AWD2
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_EnableIT_AWD2(ADC_TypeDef *ADCx)
{
  SET_BIT(ADCx->IER, LL_ADC_IT_AWD2);
}

/**
  * @brief  Enable interruption ADC analog watchdog 3.
  * @rmtoll IER      AWD3IE         LL_ADC_EnableIT_AWD3
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_EnableIT_AWD3(ADC_TypeDef *ADCx)
{
  SET_BIT(ADCx->IER, LL_ADC_IT_AWD3);
}

/**
  * @brief  Enable interruption ADC end of calibration.
  * @rmtoll IER      EOCALIE        LL_ADC_EnableIT_EOCAL
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_EnableIT_EOCAL(ADC_TypeDef *ADCx)
{
  SET_BIT(ADCx->IER, LL_ADC_IT_EOCAL);
}

/**
  * @brief  Disable interruption ADC ready.
  * @rmtoll IER      ADRDYIE        LL_ADC_DisableIT_ADRDY
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_DisableIT_ADRDY(ADC_TypeDef *ADCx)
{
  CLEAR_BIT(ADCx->IER, LL_ADC_IT_ADRDY);
}

/**
  * @brief  Disable interruption ADC channel configuration ready.
  * @rmtoll IER      ADRDYIE        LL_ADC_DisableIT_CCRDY
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE void LL_ADC_DisableIT_CCRDY(ADC_TypeDef *ADCx)
{
  CLEAR_BIT(ADCx->IER, LL_ADC_FLAG_CCRDY);
}

/**
  * @brief  Disable interruption ADC group regular end of unitary conversion.
  * @rmtoll IER      EOCIE          LL_ADC_DisableIT_EOC
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_DisableIT_EOC(ADC_TypeDef *ADCx)
{
  CLEAR_BIT(ADCx->IER, LL_ADC_IT_EOC);
}

/**
  * @brief  Disable interruption ADC group regular end of sequence conversions.
  * @rmtoll IER      EOSIE          LL_ADC_DisableIT_EOS
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_DisableIT_EOS(ADC_TypeDef *ADCx)
{
  CLEAR_BIT(ADCx->IER, LL_ADC_IT_EOS);
}

/**
  * @brief  Disable interruption ADC group regular overrun.
  * @rmtoll IER      OVRIE          LL_ADC_DisableIT_OVR
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_DisableIT_OVR(ADC_TypeDef *ADCx)
{
  CLEAR_BIT(ADCx->IER, LL_ADC_IT_OVR);
}

/**
  * @brief  Disable interruption ADC group regular end of sampling.
  * @rmtoll IER      EOSMPIE        LL_ADC_DisableIT_EOSMP
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_DisableIT_EOSMP(ADC_TypeDef *ADCx)
{
  CLEAR_BIT(ADCx->IER, LL_ADC_IT_EOSMP);
}

/**
  * @brief  Disable interruption ADC analog watchdog 1.
  * @rmtoll IER      AWD1IE         LL_ADC_DisableIT_AWD1
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_DisableIT_AWD1(ADC_TypeDef *ADCx)
{
  CLEAR_BIT(ADCx->IER, LL_ADC_IT_AWD1);
}

/**
  * @brief  Disable interruption ADC analog watchdog 2.
  * @rmtoll IER      AWD2IE         LL_ADC_DisableIT_AWD2
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_DisableIT_AWD2(ADC_TypeDef *ADCx)
{
  CLEAR_BIT(ADCx->IER, LL_ADC_IT_AWD2);
}

/**
  * @brief  Disable interruption ADC analog watchdog 3.
  * @rmtoll IER      AWD3IE         LL_ADC_DisableIT_AWD3
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_DisableIT_AWD3(ADC_TypeDef *ADCx)
{
  CLEAR_BIT(ADCx->IER, LL_ADC_IT_AWD3);
}

/**
  * @brief  Disable interruption ADC end of calibration.
  * @rmtoll IER      EOCALIE        LL_ADC_DisableIT_EOCAL
  * @param  ADCx ADC instance
  * @retval None
  */
__STATIC_INLINE void LL_ADC_DisableIT_EOCAL(ADC_TypeDef *ADCx)
{
  CLEAR_BIT(ADCx->IER, LL_ADC_IT_EOCAL);
}

/**
  * @brief  Get state of interruption ADC ready
  *         (0: interrupt disabled, 1: interrupt enabled).
  * @rmtoll IER      ADRDYIE        LL_ADC_IsEnabledIT_ADRDY
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsEnabledIT_ADRDY(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->IER, LL_ADC_IT_ADRDY) == (LL_ADC_IT_ADRDY)) ? 1UL : 0UL);
}

/**
  * @brief  Get state of interruption ADC channel configuration ready.
  * @rmtoll IER      ADRDYIE        LL_ADC_IsEnabledIT_CCRDY
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsEnabledIT_CCRDY(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->IER, LL_ADC_FLAG_CCRDY) == (LL_ADC_FLAG_CCRDY)) ? 1UL : 0UL);
}

/**
  * @brief  Get state of interruption ADC group regular end of unitary conversion
  *         (0: interrupt disabled, 1: interrupt enabled).
  * @rmtoll IER      EOCIE          LL_ADC_IsEnabledIT_EOC
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsEnabledIT_EOC(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->IER, LL_ADC_IT_EOC) == (LL_ADC_IT_EOC)) ? 1UL : 0UL);
}

/**
  * @brief  Get state of interruption ADC group regular end of sequence conversions
  *         (0: interrupt disabled, 1: interrupt enabled).
  * @rmtoll IER      EOSIE          LL_ADC_IsEnabledIT_EOS
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsEnabledIT_EOS(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->IER, LL_ADC_IT_EOS) == (LL_ADC_IT_EOS)) ? 1UL : 0UL);
}

/**
  * @brief  Get state of interruption ADC group regular overrun
  *         (0: interrupt disabled, 1: interrupt enabled).
  * @rmtoll IER      OVRIE          LL_ADC_IsEnabledIT_OVR
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsEnabledIT_OVR(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->IER, LL_ADC_IT_OVR) == (LL_ADC_IT_OVR)) ? 1UL : 0UL);
}

/**
  * @brief  Get state of interruption ADC group regular end of sampling
  *         (0: interrupt disabled, 1: interrupt enabled).
  * @rmtoll IER      EOSMPIE        LL_ADC_IsEnabledIT_EOSMP
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsEnabledIT_EOSMP(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->IER, LL_ADC_IT_EOSMP) == (LL_ADC_IT_EOSMP)) ? 1UL : 0UL);
}

/**
  * @brief  Get state of interruption ADC analog watchdog 1
  *         (0: interrupt disabled, 1: interrupt enabled).
  * @rmtoll IER      AWD1IE         LL_ADC_IsEnabledIT_AWD1
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsEnabledIT_AWD1(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->IER, LL_ADC_IT_AWD1) == (LL_ADC_IT_AWD1)) ? 1UL : 0UL);
}

/**
  * @brief  Get state of interruption Get ADC analog watchdog 2
  *         (0: interrupt disabled, 1: interrupt enabled).
  * @rmtoll IER      AWD2IE         LL_ADC_IsEnabledIT_AWD2
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsEnabledIT_AWD2(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->IER, LL_ADC_IT_AWD2) == (LL_ADC_IT_AWD2)) ? 1UL : 0UL);
}

/**
  * @brief  Get state of interruption Get ADC analog watchdog 3
  *         (0: interrupt disabled, 1: interrupt enabled).
  * @rmtoll IER      AWD3IE         LL_ADC_IsEnabledIT_AWD3
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsEnabledIT_AWD3(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->IER, LL_ADC_IT_AWD3) == (LL_ADC_IT_AWD3)) ? 1UL : 0UL);
}

/**
  * @brief  Get state of interruption ADC end of calibration
  *         (0: interrupt disabled, 1: interrupt enabled).
  * @rmtoll IER      EOCALIE        LL_ADC_IsEnabledIT_EOCAL
  * @param  ADCx ADC instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_ADC_IsEnabledIT_EOCAL(ADC_TypeDef *ADCx)
{
  return ((READ_BIT(ADCx->IER, LL_ADC_IT_EOCAL) == (LL_ADC_IT_EOCAL)) ? 1UL : 0UL);
}

/**
  * @}
  */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup ADC_LL_EF_Init Initialization and de-initialization functions
  * @{
  */

/* Initialization of some features of ADC common parameters and multimode */
ErrorStatus LL_ADC_CommonDeInit(ADC_Common_TypeDef *ADCxy_COMMON);
ErrorStatus LL_ADC_CommonInit(ADC_Common_TypeDef *ADCxy_COMMON, LL_ADC_CommonInitTypeDef *ADC_CommonInitStruct);
void        LL_ADC_CommonStructInit(LL_ADC_CommonInitTypeDef *ADC_CommonInitStruct);

/* De-initialization of ADC instance */
ErrorStatus LL_ADC_DeInit(ADC_TypeDef *ADCx);

/* Initialization of some features of ADC instance */
ErrorStatus LL_ADC_Init(ADC_TypeDef *ADCx, LL_ADC_InitTypeDef *ADC_InitStruct);
void        LL_ADC_StructInit(LL_ADC_InitTypeDef *ADC_InitStruct);

/* Initialization of some features of ADC instance and ADC group regular */
ErrorStatus LL_ADC_REG_Init(ADC_TypeDef *ADCx, LL_ADC_REG_InitTypeDef *ADC_REG_InitStruct);
void        LL_ADC_REG_StructInit(LL_ADC_REG_InitTypeDef *ADC_REG_InitStruct);

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

#endif /* ADC1 */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32G0xx_LL_ADC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
