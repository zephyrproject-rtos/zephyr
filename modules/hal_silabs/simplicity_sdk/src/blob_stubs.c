/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Empty function stubs to enable building with CONFIG_BUILD_ONLY_NO_BLOBS.
 */

#include <stdint.h>
#include <stdbool.h>

#include <sl_status.h>

struct RAIL_TxPowerCurvesConfigAlt {
};

void RAIL_VerifyTxPowerCurves(const struct RAIL_TxPowerCurvesConfigAlt *config)
{
}

void RAIL_EnablePaCal(bool enable)
{
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

void sl_btctrl_disable_2m_phy(void)
{
}

void sl_btctrl_disable_coded_phy(void)
{
}

uint32_t sl_btctrl_init_mem(uint32_t memsize)
{
	return 0;
}

void sl_btctrl_configure_le_buffer_size(uint8_t count)
{
}

sl_status_t sl_btctrl_init_ll(void)
{
	return SL_STATUS_NOT_AVAILABLE;
}

void sli_btctrl_deinit_mem(void)
{
}

void sl_btctrl_init_adv(void)
{
}

void sl_btctrl_init_adv_ext(void)
{
}

void sl_btctrl_init_scan(void)
{
}

void sl_btctrl_init_scan_ext(void)
{
}

void sl_btctrl_init_conn(void)
{
}

void sl_btctrl_init_phy(void)
{
}

void sl_btctrl_init_basic(void)
{
}

void sl_btctrl_configure_completed_packets_reporting(uint8_t packets, uint8_t events)
{
}

void sl_bthci_init_upper(void)
{
}

void sl_btctrl_hci_parser_init_default(void)
{
}

void sl_btctrl_hci_parser_init_conn(void)
{
}

void sl_btctrl_hci_parser_init_adv(void)
{
}

void sl_btctrl_hci_parser_init_phy(void)
{
}

void sl_bthci_init_vs(void)
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

void RDMAILBOX_IRQHandler(void)
{
}
