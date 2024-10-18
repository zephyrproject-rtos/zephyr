/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(ll_sys_if);

#include "ll_intf.h"
#include "ll_sys.h"
#include "linklayer_plat.h"
#include "app_conf.h"
#include "ll_intf_cmn.h"
#include "utilities_common.h"

extern struct k_mutex ble_ctlr_stack_mutex;
extern struct k_work_q ll_work_q;
struct k_work ll_sys_work;

static void ll_sys_sleep_clock_source_selection(void);

void ll_sys_reset(void);

void ll_sys_schedule_bg_process(void)
{
	k_work_submit_to_queue(&ll_work_q, &ll_sys_work);
}

void ll_sys_schedule_bg_process_isr(void)
{
	k_work_submit_to_queue(&ll_work_q, &ll_sys_work);
}

static void ll_sys_bg_process_handler(struct k_work *work)
{
	k_mutex_lock(&ble_ctlr_stack_mutex, K_FOREVER);
	ll_sys_bg_process();
	k_mutex_unlock(&ble_ctlr_stack_mutex);
}

void ll_sys_bg_process_init(void)
{
	k_work_init(&ll_sys_work, &ll_sys_bg_process_handler);
}

void ll_sys_config_params(void)
{
	ll_intf_config_ll_ctx_params(USE_RADIO_LOW_ISR, NEXT_EVENT_SCHEDULING_FROM_ISR);
}

#if (CFG_RADIO_LSE_SLEEP_TIMER_CUSTOM_SCA_RANGE == 0)
uint8_t ll_sys_BLE_sleep_clock_accuracy_selection(void)
{
  uint8_t BLE_sleep_clock_accuracy = 0;
  uint32_t RevID = LL_DBGMCU_GetRevisionID();
  uint32_t linklayer_slp_clk_src = LL_RCC_RADIO_GetSleepTimerClockSource();

  if(linklayer_slp_clk_src == LL_RCC_RADIOSLEEPSOURCE_LSE)
  {
    /* LSE selected as Link Layer sleep clock source.
       Sleep clock accuracy is different regarding the WBA device ID and revision
     */
#if defined(STM32WBA52xx) || defined(STM32WBA54xx) || defined(STM32WBA55xx)
    if(RevID == REV_ID_A)
    {
      BLE_sleep_clock_accuracy = STM32WBA5x_REV_ID_A_SCA_RANGE;
    }
    else if(RevID == REV_ID_B)
    {
      BLE_sleep_clock_accuracy = STM32WBA5x_REV_ID_B_SCA_RANGE;
    }
    else
    {
      /* Revision ID not supported, default value of 500ppm applied */
      BLE_sleep_clock_accuracy = STM32WBA5x_DEFAULT_SCA_RANGE;
    }
#else
    UNUSED(RevID);
#endif /* defined(STM32WBA52xx) || defined(STM32WBA54xx) || defined(STM32WBA55xx) */
  }
  else
  {
    /* LSE is not the Link Layer sleep clock source, sleep clock accurcay default value is 500 ppm */
    BLE_sleep_clock_accuracy = STM32WBA5x_DEFAULT_SCA_RANGE;
  }

  return BLE_sleep_clock_accuracy;
}
#endif /* CFG_RADIO_LSE_SLEEP_TIMER_CUSTOM_SCA_RANGE */

void ll_sys_sleep_clock_source_selection(void)
{
  uint16_t freq_value = 0;
  uint32_t linklayer_slp_clk_src = LL_RCC_RADIOSLEEPSOURCE_NONE;

  linklayer_slp_clk_src = LL_RCC_RADIO_GetSleepTimerClockSource();
  switch(linklayer_slp_clk_src)
  {
    case LL_RCC_RADIOSLEEPSOURCE_LSE:
      linklayer_slp_clk_src = RTC_SLPTMR;
      break;

    case LL_RCC_RADIOSLEEPSOURCE_LSI:
      linklayer_slp_clk_src = RCO_SLPTMR;
      break;

    case LL_RCC_RADIOSLEEPSOURCE_HSE_DIV1000:
      linklayer_slp_clk_src = CRYSTAL_OSCILLATOR_SLPTMR;
      break;

    case LL_RCC_RADIOSLEEPSOURCE_NONE:
      /* No Link Layer sleep clock source selected */
      assert_param(0);
      break;
  }
  ll_intf_cmn_le_select_slp_clk_src((uint8_t)linklayer_slp_clk_src, &freq_value);
}

void ll_sys_reset(void)
{
#if (CFG_RADIO_LSE_SLEEP_TIMER_CUSTOM_SCA_RANGE == 0)
  uint8_t bsca = 0;
#endif /* CFG_RADIO_LSE_SLEEP_TIMER_CUSTOM_SCA_RANGE */

  /* Apply the selected link layer sleep timer source */
  ll_sys_sleep_clock_source_selection();

  /* Configure the link layer sleep clock accuracy if different from the default one */
#if (CFG_RADIO_LSE_SLEEP_TIMER_CUSTOM_SCA_RANGE != 0)
  ll_intf_le_set_sleep_clock_accuracy(CFG_RADIO_LSE_SLEEP_TIMER_CUSTOM_SCA_RANGE);
#else
  bsca = ll_sys_BLE_sleep_clock_accuracy_selection();
  if(bsca != STM32WBA5x_DEFAULT_SCA_RANGE)
  {
    ll_intf_le_set_sleep_clock_accuracy(bsca);
  }
#endif /* CFG_RADIO_LSE_SLEEP_TIMER_CUSTOM_SCA_RANGE */
}
