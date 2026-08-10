/*
 * Copyright 2026 NXP.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "uwb_tml_transport.h"
#include "uwb_tml_status.h"
#include <uwb_uwbs_tml_interface.h>
#include "uwb_uwbs_tml_io.h"
#include "uwb_uwbs_driver_config.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nxp_sr250_tml, CONFIG_UWB_NXP_LOG_LEVEL);

UWBSTATUS uwb_tml_set_mode_fwdld(void)
{
	if (uwb_uwbs_tml_setmode(&gUwbsTmlCtx, kUWB_UWBS_TML_MODE_HDLL) != kUWBSTATUS_SUCCESS) {
		LOG_ERR("uwb_tml_set_mode_fwdld : uwb_uwbs_tml_setmode failed");
		return kUWBSTATUS_FAILED;
	}
	return kUWBSTATUS_SUCCESS;
}

int uwb_tml_hdll_read(uint8_t *pBuffer, uint16_t *pRspBufLen)
{
	size_t bufLen = 0;
	if (uwb_tml_set_mode_fwdld() != kUWBSTATUS_SUCCESS) {
		LOG_ERR("uwb_tml_hdll_read : uwb_tml_set_mode_fwdld failed");
		return -1;
	}
#if UWBIOT_TML_PNP || UWBIOT_TML_SOCKET
	if (uwb_uwbs_tml_helios_get_hdll_edl_ntf(&gUwbsTmlCtx, pBuffer, &bufLen) !=
	    kUWBSTATUS_SUCCESS) {
		LOG_DBG("uwb_tml_hdll_read : uwb_uwbs_tml_data_rx failed");
		return -1;
	}
#else
	if (uwb_uwbs_tml_data_rx(&gUwbsTmlCtx, pBuffer, &bufLen) != kUWBSTATUS_SUCCESS) {
		LOG_DBG("uwb_tml_hdll_read : uwb_uwbs_tml_data_rx failed");
		return -1;
	}
#endif
	*pRspBufLen = (uint16_t)bufLen;

	if (bufLen <= UINT16_MAX) {
		*pRspBufLen = (uint16_t)bufLen;
	} else {
		LOG_ERR("Buffer length (%u) exceeds UINT16_MAX", bufLen);
		return -1;
	}

	return 0;
}

UWBSTATUS uwb_tml_hdll_transceive(uint8_t *pWriteBuf, size_t writeBufLen, uint8_t *pRespBuf,
				  size_t *pRspBufLen)
{
	UWBSTATUS status = UWBSTATUS_FAILED;
	if (uwb_tml_set_mode_fwdld() != kUWBSTATUS_SUCCESS) {
		LOG_ERR("uwb_tml_hdll_transceive : uwb_tml_set_mode_fwdld failed");
		return status;
	}

	LOG_HEXDUMP_DBG(&pWriteBuf[UCI_CMD_INDEX], writeBufLen, "HDLL Tx >");

	*pRspBufLen = 0;
	status = uwb_uwbs_tml_data_tx(&gUwbsTmlCtx, pWriteBuf, writeBufLen);
	if (status != kUWBSTATUS_SUCCESS) {
		LOG_ERR("uwb_uwbs_tml_hdll_data_trx write data failed");
		goto end;
	}
#if UWBIOT_TML_PNP
/* FIXME: SID MM - already under SR2XXT file, remove redundant check */
#if UWBIOT_UWBD_SR2XXT
	// Delay increased to 30ms for SR200T as unexpected behaviour seen on USB port
	// USB port fails to open without any error notification if delay time is too low
	k_msleep(30);
#else
	k_msleep(10);
#endif
#elif UWBIOT_TML_SOCKET
	k_msleep(20);
#elif UWBIOT_UWBD_SR2XXT && UWBIOT_TML_SPI
#if __ZEPHYR__
	/* Add small delay before read operation to ensure hardware stability */
	k_msleep(10);
#endif /** __ZEPHYR__ */
#endif
	status = uwb_uwbs_tml_data_rx(&gUwbsTmlCtx, pRespBuf, pRspBufLen);
	if (kUWBSTATUS_RESPONSE_TIMEOUT == status) {
		*pRspBufLen = 0;
		status = uwb_uwbs_tml_data_rx(&gUwbsTmlCtx, pRespBuf, pRspBufLen);
	}
	if (status != kUWBSTATUS_SUCCESS) {
		LOG_ERR("uwb_uwbs_tml_hdll_data_trx read data failed");
		goto end;
	}

	LOG_HEXDUMP_DBG(&pRespBuf[ACTUAL_PACKET_START], *pRspBufLen, "HDLL Rx <");
end:
	AddDelayInMicroSec(50);
	return status;
}

#if UWBIOT_TML_PNP || UWBIOT_TML_SOCKET
UWBSTATUS uwb_tml_hdll_reset(bool isFWDownloadDone)
{
	UWBSTATUS status = UWBSTATUS_FAILED;
	if (uwb_tml_set_mode_fwdld() != kUWBSTATUS_SUCCESS) {
		LOG_ERR("uwb_tml_hdll_reset : uwb_tml_set_mode_fwdld failed");
		return status;
	}
	if (uwb_uwbs_tml_helios_hdll_reset(&gUwbsTmlCtx, isFWDownloadDone) == kUWBSTATUS_SUCCESS) {
		status = UWBSTATUS_SUCCESS;
	}
	return status;
}
#endif // (UWBIOT_TML_PNP || UWBIOT_TML_SOCKET)

// TODO: Temporary for sn220 and sr2xx simultaneous irq enablement
#if UWBIOT_SESN_SNXXX
void uwb_tml_io_enable_uwb_irq()
{
	uwb_bus_io_uwbs_irq_enable(&gUwbsTmlCtx.busCtx);
}
#endif

void uwb_tml_chip_reset(void)
{
	uwb_transport_io_set(kUWBS_IO_O_RSTN, 0);
	k_msleep(10);
	uwb_transport_io_set(kUWBS_IO_O_RSTN, 1);
	k_msleep(10);
}
