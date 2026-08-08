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

#if defined(CONFIG_IEEE802154_SILABS_EFR32)
#include <sl_rail_ieee802154.h>
#endif

const uint16_t sl_rail_builtin_rx_packet_queue_entries;
const uint16_t sl_rail_builtin_rx_fifo_bytes;
sl_rail_packet_queue_entry_t *const sl_rail_builtin_rx_packet_queue_ptr;
sl_rail_fifo_buffer_align_t *const sl_rail_builtin_rx_fifo_ptr;

sl_rail_status_t sl_rail_verify_tx_power_conversion(const struct sl_rail_tx_power_table_config *cfg)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

void sl_rail_enable_pa_cal(sl_rail_handle_t rail_handle, bool enable)
{
}

sl_rail_status_t sl_rail_set_tx_pa_voltage(sl_rail_handle_t rail_handle, uint16_t voltage_mv)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_set_tx_pa_ramp_time(sl_rail_handle_t rail_handle, uint16_t ramp_time_us)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_config_sleep(sl_rail_handle_t handle,
				      const sl_rail_timer_sync_config_t *config)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_config_events(sl_rail_handle_t rail_handle, sl_rail_events_t mask,
				       sl_rail_events_t events)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_config_rx_options(sl_rail_handle_t rail_handle,
					   sl_rail_rx_options_t rx_options_mask,
					   sl_rail_rx_options_t rx_options)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_config_cal(sl_rail_handle_t rail_handle,
				    sl_rail_cal_mask_t cal_enable_mask)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

const sl_rail_config_t *sl_rail_get_config(sl_rail_handle_t rail_handle)
{
	return NULL;
}

sl_rail_status_t sl_rail_init(sl_rail_handle_t *p_rail_handle,
			      const sl_rail_config_t *p_rail_config,
			      sl_rail_init_complete_callback_t init_complete_callback)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_init_power_manager(void)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_set_tx_power_dbm(sl_rail_handle_t rail_handle,
					  sl_rail_tx_power_t power_ddbm)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_set_pti_protocol(sl_rail_handle_t rail_handle,
					  sl_rail_pti_protocol_t protocol)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

uint32_t sl_rail_get_symbol_rate(sl_rail_handle_t rail_handle)
{
	return 0;
}

int16_t sl_rail_get_rssi(sl_rail_handle_t rail_handle, sl_rail_time_t wait_timeout_us)
{
	return 0;
}

sl_rail_radio_state_t sl_rail_get_radio_state(sl_rail_handle_t rail_handle)
{
	return SL_RAIL_RF_STATE_INACTIVE;
}

sl_rail_status_t sl_rail_start_tx(sl_rail_handle_t rail_handle, uint16_t channel,
				  sl_rail_tx_options_t tx_options,
				  const sl_rail_scheduler_info_t *p_scheduler_info)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_start_rx(sl_rail_handle_t rail_handle, uint16_t channel,
				  const sl_rail_scheduler_info_t *p_scheduler_info)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

uint16_t sl_rail_write_tx_fifo(sl_rail_handle_t rail_handle, const uint8_t *p_data,
			       uint16_t write_bytes, bool reset)
{
	return 0;
}

sl_rail_status_t sl_rail_start_cca_csma_tx(sl_rail_handle_t rail_handle, uint16_t channel,
					   sl_rail_tx_options_t tx_options,
					   const sl_rail_csma_config_t *p_csma_config,
					   const sl_rail_scheduler_info_t *p_scheduler_info)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_rx_packet_handle_t sl_rail_get_rx_packet_info(sl_rail_handle_t rail_handle,
						      sl_rail_rx_packet_handle_t packet_handle,
						      sl_rail_rx_packet_info_t *p_packet_info)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_get_rx_time_sync_word_end(sl_rail_handle_t rail_handle,
						   sl_rail_rx_packet_details_t *p_packet_details)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_get_rx_incoming_packet_info(sl_rail_handle_t rail_handle,
						     sl_rail_rx_packet_info_t *p_packet_info)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_get_rx_packet_details(sl_rail_handle_t rail_handle,
					       sl_rail_rx_packet_handle_t packet_handle,
					       sl_rail_rx_packet_details_t *p_packet_details)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_copy_rx_packet(sl_rail_handle_t rail_handle, uint8_t *p_dest,
					const sl_rail_rx_packet_info_t *p_packet_info)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_get_scheduler_status(sl_rail_handle_t rail_handle,
					      sl_rail_scheduler_status_t *p_scheduler_status,
					      sl_rail_status_t *p_rail_status)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_calibrate(sl_rail_handle_t rail_handle, sl_rail_cal_values_t *p_cal_values,
				   sl_rail_cal_mask_t cal_force_mask)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_idle(sl_rail_handle_t rail_handle, sl_rail_idle_mode_t mode, bool wait)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_yield_radio(sl_rail_handle_t rail_handle)
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

sl_rail_status_t sl_rail_set_multi_timer(sl_rail_handle_t rail_handle, sl_rail_multi_timer_t *timer,
					 sl_rail_time_t expiration_time, sl_rail_time_mode_t mode,
					 sl_rail_multi_timer_callback_t callback, void *user_data)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_cancel_multi_timer(sl_rail_handle_t rail_handle,
					    sl_rail_multi_timer_t *timer)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

bool sl_rail_is_multi_timer_running(sl_rail_handle_t rail_handle, sl_rail_multi_timer_t *timer)
{
	return false;
}

sl_rail_status_t sl_rail_config_multi_timer(sl_rail_handle_t rail_handle, bool enable)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_start_scheduled_tx(sl_rail_handle_t rail_handle,
	uint16_t channel,
	sl_rail_tx_options_t tx_options,
	const sl_rail_scheduled_tx_config_t *p_scheduled_tx_config,
	const sl_rail_scheduler_info_t *p_scheduler_info)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_start_scheduled_cca_csma_tx(sl_rail_handle_t rail_handle,
	uint16_t channel,
	sl_rail_tx_options_t tx_options,
	const sl_rail_scheduled_tx_config_t *p_scheduled_tx_config,
	const sl_rail_csma_config_t *p_csma_config,
	const sl_rail_scheduler_info_t *p_scheduler_info)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

#if defined(CONFIG_IEEE802154_SILABS_EFR32)
void sl_openthread_init(void)
{
	/* Stub for the OpenThread PAL blob; declared in ieee802154_silabs_efr32.h. */
}

sl_rail_status_t sl_rail_ieee802154_enable_early_frame_pending(sl_rail_handle_t rail_handle,
							       bool enable)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_ieee802154_enable_data_frame_pending(sl_rail_handle_t rail_handle,
							      bool enable)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_ieee802154_init(sl_rail_handle_t rail_handle,
					 const sl_rail_ieee802154_config_t *p_config)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_ieee802154_config_2p4_ghz_radio(sl_rail_handle_t rail_handle)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_ieee802154_config_e_options(sl_rail_handle_t rail_handle,
						     sl_rail_ieee802154_e_options_t mask,
						     sl_rail_ieee802154_e_options_t options)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t
sl_rail_ieee802154_set_rx_to_enh_ack_tx(sl_rail_handle_t rail_handle,
					sl_rail_transition_time_t *p_rx_to_enh_ack_tx)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_ieee802154_config_cca_mode(sl_rail_handle_t rail_handle,
						    sl_rail_ieee802154_cca_mode_t cca_mode)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_ieee802154_set_promiscuous_mode(sl_rail_handle_t rail_handle, bool enable)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_ieee802154_set_pan_coordinator(sl_rail_handle_t rail_handle,
							bool is_pan_coordinator)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_ieee802154_set_pan_id(sl_rail_handle_t rail_handle, uint16_t pan_id,
					       uint8_t index)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_ieee802154_set_short_address(sl_rail_handle_t rail_handle,
						      uint16_t short_addr, uint8_t index)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_ieee802154_set_long_address(sl_rail_handle_t rail_handle,
						     const uint8_t *p_long_addr, uint8_t index)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_ieee802154_write_enh_ack(sl_rail_handle_t rail_handle,
						  const uint8_t *p_ack_data,
						  uint16_t ack_data_bytes)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_ieee802154_get_address(sl_rail_handle_t rail_handle,
						sl_rail_ieee802154_address_t *p_address)
{
	return SL_RAIL_STATUS_NO_ERROR;
}

sl_rail_status_t sl_rail_ieee802154_toggle_frame_pending(sl_rail_handle_t rail_handle)
{
	return SL_RAIL_STATUS_NO_ERROR;
}
#endif
