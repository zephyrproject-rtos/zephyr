/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Empty function stubs to enable building with CONFIG_BUILD_ONLY_NO_BLOBS.
 */

#include <stdint.h>
#include <stdbool.h>

#include <sl_rail.h>
#include <sl_status.h>
#include <sl_rail.h>

sl_rail_status_t sl_rail_verify_tx_power_conversion(const struct sl_rail_tx_power_table_config *cfg)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

void sl_rail_enable_pa_cal(sl_rail_handle_t rail_handle, bool enable)
{
}

sl_rail_status_t sl_rail_config_sleep(sl_rail_handle_t handle,
				      const sl_rail_timer_sync_config_t *config)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_init_power_manager(void)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

int16_t sl_btctrl_hci_receive(uint8_t *data, int16_t len, bool lastFragment)
{
	return 0;
}

void BTLE_LL_Process(uint32_t events)
{
}

int16_t BTLE_LL_SetMaxPower(int16_t power)
{
	return 0;
}

sl_status_t sl_btctrl_init(void)
{
	return SL_STATUS_OK;
}

void sl_btctrl_deinit(void)
{
}

void *sli_btctrl_get_radio_context_handle(void)
{
	return 0;
}

void AGC_IRQHandler(void)
{
}

void BUFC_IRQHandler(void)
{
}

void FRC_IRQHandler(void)
{
}

void FRC_PRI_IRQHandler(void)
{
}

void MODEM_IRQHandler(void)
{
}

void PROTIMER_IRQHandler(void)
{
}

void RAC_RSM_IRQHandler(void)
{
}

void RAC_SEQ_IRQHandler(void)
{
}

void SYNTH_IRQHandler(void)
{
}

void HOSTMAILBOX_IRQHandler(void)
{
}

void RDMAILBOX_IRQHandler(void)
{
}

/* RAIL timer API stubs (e.g. for counter_silabs_protimer when no blobs) */
sl_rail_time_t sl_rail_get_time(sl_rail_handle_t rail_handle)
{
	return 0U;
}

sl_rail_status_t sl_rail_set_multi_timer(sl_rail_handle_t rail_handle,
					 sl_rail_multi_timer_t *timer,
					 sl_rail_time_t expiration_time,
					 sl_rail_time_mode_t mode,
					 sl_rail_multi_timer_callback_t callback,
					 void *user_data)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_cancel_multi_timer(sl_rail_handle_t rail_handle,
					    sl_rail_multi_timer_t *timer)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

bool sl_rail_is_multi_timer_running(sl_rail_handle_t rail_handle,
				    sl_rail_multi_timer_t *timer)
{
	return false;
}
