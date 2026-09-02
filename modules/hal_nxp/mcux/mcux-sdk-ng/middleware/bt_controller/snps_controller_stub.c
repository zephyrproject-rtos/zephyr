/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ble_controller.h"

blec_result_t BLEController_SetConnectionInitialTxPowerDbm(int8_t level_dbm)
{
	return kBLEC_CommandDisallowed;
}

blec_result_t BLEController_SetTxPowerDbm(int8_t level_dbm)
{
	return kBLEC_CommandDisallowed;
}

blec_result_t BLEController_Init(blecHostHciRecvCallback_t callback, int8_t requested_max_power_dBm,
				 power_range_dbm_t *selected_power_range)
{
	return kBLEC_CommandDisallowed;
}

blec_result_t BLEController_ProcessHciPacket(blec_hciPacketType_t packetType, void *pPacket,
					     uint16_t packetSize)
{
	return kBLEC_CommandDisallowed;
}

void BLE_LL_IRQHandler(void)
{
}
