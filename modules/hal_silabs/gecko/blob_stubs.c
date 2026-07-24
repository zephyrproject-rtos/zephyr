/*
 * Copyright (c) 2025 sevenlab engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rail.h>

#ifdef CONFIG_BT_SILABS_EFR32
#include <sl_bluetooth_controller.h>
#include <sl_btctrl_hci.h>
#include <sl_btctrl_linklayer.h>
#include <sl_bt_ll_config.h>
#endif

RAIL_Status_t RAIL_ConfigSleep(RAIL_Handle_t railHandle, RAIL_SleepConfig_t sleepConfig)
{
	return RAIL_STATUS_NO_ERROR;
}

void RAIL_EnablePaCal(bool enable)
{
}

RAIL_Status_t RAIL_InitPowerManager(void)
{
	return RAIL_STATUS_NO_ERROR;
}

void RAIL_VerifyTxPowerCurves(const struct RAIL_TxPowerCurvesConfigAlt *config)
{
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

void PRORTC_IRQHandler(void)
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

void RFSENSE_IRQHandler(void)
{
}

void SYNTH_IRQHandler(void)
{
}

#ifdef CONFIG_BT_SILABS_EFR32
void *ll_connSchInit;

void BTLE_LL_Process(uint32_t events)
{
}

int16_t BTLE_LL_SetMaxPower(int16_t power)
{
	return 0;
}

sl_status_t ll_connPowerControlEnable(const sl_bt_ll_power_control_config_t *)
{
	return SL_STATUS_OK;
}

sl_status_t ll_initDefaultPowerLevelRange(int16_t minPower, int16_t maxPower)
{
	return SL_STATUS_OK;
}

sl_status_t sl_btctrl_alloc_periodic_adv(uint8_t num_adv)
{
	return SL_STATUS_OK;
}

sl_status_t sl_btctrl_alloc_periodic_scan(uint8_t num_scan)
{
	return SL_STATUS_OK;
}

void sl_btctrl_configure_completed_packets_reporting(uint8_t packets, uint8_t events)
{
}

void sl_btctrl_configure_le_buffer_size(uint8_t count)
{
}

void sl_btctrl_configure_max_queued_adv_reports(uint8_t num_reports)
{
}

sl_status_t sl_btctrl_config_adv(struct sl_btctrl_adv_config *adv_config)
{
	return SL_STATUS_OK;
}

void sli_btctrl_deinit_mem(void)
{
}

void sl_btctrl_disable_coded_phy(void)
{
}

sl_status_t sl_btctrl_init_afh(uint32_t flags)
{
	return SL_STATUS_OK;
}

void sl_btctrl_init_multiprotocol(void)
{
}

void sl_btctrl_init_past_local_sync_transfer(void)
{
}

void sl_btctrl_init_past_receiver(void)
{
}

void sl_btctrl_init_past_remote_sync_transfer(void)
{
}

int16_t sl_btctrl_hci_receive(uint8_t *data, int16_t len, bool lastFragment)
{
	return 0;
}

bool sl_btctrl_is_initialized(void)
{
	return false;
}

void sl_btctrl_init_adv(void)
{
}

void sl_btctrl_init_adv_ext(void)
{
}

sl_status_t sl_btctrl_allocate_resolving_list_memory(uint8_t resolvingListSize)
{
	return SL_STATUS_OK;
}

void sl_btctrl_hci_parser_init_adv(void)
{
}

void sl_btctrl_hci_parser_init_conn(void)
{
}

void sl_btctrl_hci_parser_init_default(void)
{
}

void sl_btctrl_hci_parser_init_phy(void)
{
}

void sl_btctrl_hci_parser_init_past(void)
{
}

void sl_btctrl_hci_parser_init_privacy(void)
{
}

sl_status_t sl_btctrl_init_basic(uint8_t connections, uint8_t adv_sets, uint8_t whitelist)
{
	return SL_STATUS_OK;
}

void sl_btctrl_init_conn(void)
{
}

void sl_btctrl_init_highpower(void)
{
}

sl_status_t sl_btctrl_init_ll(void)
{
	return SL_STATUS_OK;
}

uint32_t sl_btctrl_init_mem(uint32_t memsize)
{
	return 0;
}

void sl_btctrl_init_phy(void)
{
}

void sl_btctrl_init_privacy(void)
{
}

void sl_btctrl_init_scan(void)
{
}

void sl_btctrl_init_scan_ext(void)
{
}

sl_status_t
sl_btctrl_pawr_advertiser_configure(struct sl_btctrl_pawr_advertiser_config *pawr_adv_config)
{
	return SL_STATUS_OK;
}

sl_status_t
sl_btctrl_pawr_synchronizer_configure(struct sl_btctrl_pawr_synchronizer_config *pawr_sync_config)
{
	return SL_STATUS_OK;
}

void sl_bthci_init_upper(void)
{
}

void sl_bthci_init_vs(void)
{
}

sl_status_t sl_bt_ll_deinit(void)
{
	return SL_STATUS_OK;
}
#endif
