/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Empty function stubs to enable building with CONFIG_BUILD_ONLY_NO_BLOBS.
 */

#include <stdint.h>
#include <stdbool.h>

#include <rail.h>
#include <sl_status.h>

RAIL_Status_t RAIL_VerifyTxPowerCurves(const struct RAIL_TxPowerCurvesConfigAlt *config)
{
	return RAIL_STATUS_NO_ERROR;
}

void RAIL_EnablePaCal(bool enable)
{
}

RAIL_Status_t RAIL_ConfigSleep(RAIL_Handle_t handle, RAIL_SleepConfig_t config)
{
	return RAIL_STATUS_NO_ERROR;
}

RAIL_Status_t RAIL_InitPowerManager(void)
{
	return RAIL_STATUS_NO_ERROR;
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
