/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    debug_config.h
  * @author  MCD Application Team
  * @brief   Real Time Debug module general configuration file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
#ifndef DEBUG_CONFIG_H
#define DEBUG_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#if(CFG_RT_DEBUG_GPIO_MODULE == 1)

/*********************************/
/** GPIO debug signal selection **/
/*********************************/
#if(CONFIG_BT_STM32WBA_USE_RT_DEBUG_SCM_SYSTEM_CLOCK_CONFIG)
#define USE_RT_DEBUG_SCM_SYSTEM_CLOCK_CONFIG                  (1)
#define GPIO_DEBUG_SCM_SYSTEM_CLOCK_CONFIG                    GPIO_DT_SPEC_GET(DT_PATH(zephyr_user),rt_dbg_scm_sys_clock_config_gpios)
#else
#define USE_RT_DEBUG_SCM_SYSTEM_CLOCK_CONFIG                  (0)
#endif

#if(CONFIG_BT_STM32WBA_USE_RT_DEBUG_SCM_SETUP)
#define USE_RT_DEBUG_SCM_SETUP                                (1)
#define GPIO_DEBUG_SCM_SETUP                                  GPIO_DT_SPEC_GET(DT_PATH(zephyr_user),rt_dbg_scm_setup_gpios)
#else
#define USE_RT_DEBUG_SCM_SETUP                                (0)
#endif

#if(CONFIG_BT_STM32WBA_USE_RT_DEBUG_SCM_HSERDY_ISR)
#define USE_RT_DEBUG_SCM_HSERDY_ISR                           (1)
#define GPIO_DEBUG_SCM_HSERDY_ISR                             GPIO_DT_SPEC_GET(DT_PATH(zephyr_user),rt_dbg_scm_hserdy_isr_gpios)
#else
#define USE_RT_DEBUG_SCM_HSERDY_ISR                           (0)
#endif

#if(CONFIG_BT_STM32WBA_USE_RT_DEBUG_ADC_ACTIVATION)
#define USE_RT_DEBUG_ADC_ACTIVATION                           (1)
#define GPIO_DEBUG_ADC_ACTIVATION                             GPIO_DT_SPEC_GET(DT_PATH(zephyr_user),rt_dbg_adc_activation_gpios)
#else
#define USE_RT_DEBUG_ADC_ACTIVATION                           (0)
#endif

#if(CONFIG_BT_STM32WBA_USE_RT_DEBUG_ADC_TEMPERATURE_ACQUISITION)
#define USE_RT_DEBUG_ADC_TEMPERATURE_ACQUISITION              (1)
#define GPIO_DEBUG_ADC_TEMPERATURE_ACQUISITION                GPIO_DT_SPEC_GET(DT_PATH(zephyr_user),rt_dbg_adc_temp_acquisition_gpios)
#else
#define USE_RT_DEBUG_ADC_TEMPERATURE_ACQUISITION              (0)
#endif

#if(CONFIG_BT_STM32WBA_USE_RT_DEBUG_LOW_POWER_STOP_MODE_ACTIVE)
#define USE_RT_DEBUG_LOW_POWER_STOP_MODE_ACTIVE               (1)
#define GPIO_DEBUG_LOW_POWER_STOP_MODE_ACTIVE                 GPIO_DT_SPEC_GET(DT_PATH(zephyr_user),rt_dbg_lowpower_stop_mode_active_gpios)
#else
#define USE_RT_DEBUG_LOW_POWER_STOP_MODE_ACTIVE               (0)
#endif

#if(CONFIG_BT_STM32WBA_USE_RT_DEBUG_LOW_POWER_STANDBY_MODE_ACTIVE)
#define USE_RT_DEBUG_LOW_POWER_STANDBY_MODE_ACTIVE            (1)
#define GPIO_DEBUG_LOW_POWER_STANDBY_MODE_ACTIVE              GPIO_DT_SPEC_GET(DT_PATH(zephyr_user),rt_dbg_lowpower_standby_mode_active_gpios)
#else
#define USE_RT_DEBUG_LOW_POWER_STANDBY_MODE_ACTIVE            (0)
#endif


#if(CONFIG_BT_STM32WBA_USE_RT_DEBUG_SCHDLR_HNDL_NXT_TRACE)
#define USE_RT_DEBUG_SCHDLR_HNDL_NXT_TRACE                    (1)
#define GPIO_DEBUG_SCHDLR_HNDL_NXT_TRACE                      GPIO_DT_SPEC_GET(DT_PATH(zephyr_user),rt_dbg_schdlr_hndl_nxt_trace_gpios)
#else
#define USE_RT_DEBUG_SCHDLR_HNDL_NXT_TRACE                    (0)
#endif

#if(CONFIG_BT_STM32WBA_USE_RT_DEBUG_SCHDLR_EVNT_RGSTR)
#define USE_RT_DEBUG_SCHDLR_EVNT_RGSTR                        (1)
#define GPIO_DEBUG_SCHDLR_EVNT_RGSTR                          GPIO_DT_SPEC_GET(DT_PATH(zephyr_user),rt_dbg_schdr_evnt_rgstr_gpios)
#else
#define USE_RT_DEBUG_SCHDLR_EVNT_RGSTR                        (0)
#endif

#if(CONFIG_BT_STM32WBA_USE_RT_DEBUG_SCHDLR_HNDL_MISSED_EVNT)
#define USE_RT_DEBUG_SCHDLR_HNDL_MISSED_EVNT                  (1)
#define GPIO_DEBUG_SCHDLR_HNDL_MISSED_EVNT                    GPIO_DT_SPEC_GET(DT_PATH(zephyr_user),rt_dbg_schdr_hndl_missed_evnt_gpios)
#else
#define USE_RT_DEBUG_SCHDLR_HNDL_MISSED_EVNT                  (0)
#endif

#if(CONFIG_BT_STM32WBA_USE_RT_DEBUG_SCHDLR_EXEC_EVNT_TRACE)
#define USE_RT_DEBUG_SCHDLR_EXEC_EVNT_TRACE                   (1)
#define GPIO_DEBUG_SCHDLR_EXEC_EVNT_TRACE                     GPIO_DT_SPEC_GET(DT_PATH(zephyr_user),rt_dbg_schdlr_exec_evnt_trace_gpios)
#else
#define USE_RT_DEBUG_SCHDLR_EXEC_EVNT_TRACE                   (0)
#endif

#if(CONFIG_BT_STM32WBA_USE_RT_DEBUG_LLWCC_CMN_HG_ISR)
#define USE_RT_DEBUG_LLWCC_CMN_HG_ISR                         (1)
#define GPIO_DEBUG_LLWCC_CMN_HG_ISR                           GPIO_DT_SPEC_GET(DT_PATH(zephyr_user),rt_dbg_llwcc_cmn_hg_isr_gpios)
#else
#define USE_RT_DEBUG_LLWCC_CMN_HG_ISR                         (0)
#endif

#if(CONFIG_BT_STM32WBA_USE_RT_DEBUG_LLHWC_CMN_LW_ISR)
#define USE_RT_DEBUG_LLHWC_CMN_LW_ISR                         (1)
#define GPIO_DEBUG_LLHWC_CMN_LW_ISR                           GPIO_DT_SPEC_GET(DT_PATH(zephyr_user),rt_dbg_llhwc_cmn_lw_isr_gpios)
#else
#define USE_RT_DEBUG_LLHWC_CMN_LW_ISR                         (0)
#endif

#if(CONFIG_BT_STM32WBA_USE_RT_DEBUG_PHY_CLBR_ISR)
#define USE_RT_DEBUG_PHY_CLBR_ISR                             (1)
#define GPIO_DEBUG_PHY_CLBR_ISR                               GPIO_DT_SPEC_GET(DT_PATH(zephyr_user),rt_dbg_phy_clbr_isr_gpios)
#else
#define USE_RT_DEBUG_PHY_CLBR_ISR                             (0)
#endif

#if(CONFIG_BT_STM32WBA_USE_RT_DEBUG_PHY_CLBR_EXEC)
#define USE_RT_DEBUG_PHY_CLBR_EXEC                            (1)
#define GPIO_DEBUG_PHY_CLBR_EXEC                              GPIO_DT_SPEC_GET(DT_PATH(zephyr_user),rt_dbg_phy_clbr_exec_gpios)
#else
#define USE_RT_DEBUG_PHY_CLBR_EXEC                            (0)
#endif

#endif /* CFG_RT_DEBUG_GPIO_MODULE */
#include "debug_signals.h"
extern const struct gpio_dt_spec general_debug_table[RT_DEBUG_SIGNALS_TOTAL_NUM];

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_CONFIG_H */
