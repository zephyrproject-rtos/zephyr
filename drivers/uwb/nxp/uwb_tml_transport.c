/*
 * Copyright 2012-2020,2022-2024,2026 NXP.
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
#include <stdlib.h>
#include <limits.h>

#include "uwb_tml_transport.h"
#include "uwb_tml_status.h"
#include "uwb_uwbs_tml_interface.h"
#include "uwb_uwbs_tml_io.h"
#include "uwb_nxp_host.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nxp_tml, CONFIG_UWB_NXP_LOG_LEVEL);

#ifndef CONFIG_NXP_UWB_DEVICE_TX_BUFFER_MAX
#define CONFIG_NXP_UWB_DEVICE_TX_BUFFER_MAX 2048
#endif /** CONFIG_NXP_UWB_DEVICE_TX_BUFFER_MAX */

#define DIRECTIONAL_BYTE_WRITE 0x00

/** Global Variables */
/* UCI HAL Control structure */
static int g_init_done = 0xFF;
/** UWB Subsystem tml interface context */
uwb_uwbs_tml_ctx_t g_uwbs_tml_ctx;

#if UWBIOT_UWBD_SR2XXT
static uint8_t g_tx_buffer[CONFIG_NXP_UWB_DEVICE_TX_BUFFER_MAX];
#endif /** UWBIOT_UWBD_SR2XXT */

int uwb_transport_open()
{
	UWBSTATUS ret_status = UWBSTATUS_INVALID_DEVICE;

	if (uwb_uwbs_tml_init(&g_uwbs_tml_ctx) == kUWBSTATUS_SUCCESS) {
		g_init_done = 0;
		ret_status = UWBSTATUS_SUCCESS;
	}
	return ret_status;
}

UWBSTATUS uwb_transport_io_set(uwbs_io_t ioPin, uint8_t value)
{
	if ((uwb_bus_io_val_set(&g_uwbs_tml_ctx.busCtx, ioPin, (uwbs_io_state_t)value)) ==
	    kUWB_bus_Status_OK) {
		return UWBSTATUS_SUCCESS;
	}
	return UWBSTATUS_INVALID_DEVICE;
}

void uwb_transport_close()
{
	uwb_tml_reset(ABORT_READ_PENDING);
	if (kUWBSTATUS_SUCCESS != uwb_uwbs_tml_deinit(&g_uwbs_tml_ctx)) {
		LOG_ERR("uwb_transport_deinit : uwb_uwbs_tml_deinit failed");
	}
	g_init_done = 0xFF;
}

int uwb_transport_uci_read(uint8_t *pBuffer, int bytes_to_read)
{
	size_t buf_len = bytes_to_read;
	uwb_tml_set_mode_uci();
	UWBStatus_t status = uwb_uwbs_tml_data_rx(&g_uwbs_tml_ctx, pBuffer, &buf_len);
	if (status != kUWBSTATUS_SUCCESS) {
#if UWBIOT_UWBD_SR1XXT_SR2XXT
		if (status == kUWBSTATUS_RESPONSE_TIMEOUT) {
			LOG_DBG("uwb_transport_uci_read : Read IRQ Timedout");
			return UWBSTATUS_IRQ_READ_TIMEOUT;
		} else
#endif // UWBIOT_UWBD_SR1XXT_SR2XXT
		{
			LOG_DBG("uwb_transport_uci_read : uwb_uwbs_tml_data_rx failed");
			return 0;
		}
	}
	if (buf_len > INT_MAX) {
		LOG_DBG("%s : Data Length exceeds INT_MAX", __FUNCTION__);
		return 0;
	}
#if UWBIOT_UWBD_SR2XXT && UWBIOT_TML_SPI
	else {
		/** Remove directional byte */
		memmove(pBuffer, pBuffer + 2, buf_len);
	}
#endif /** UWBIOT_UWBD_SR2XXT */
	return (int)buf_len;
}

int uwb_transport_uci_write(uint8_t *pBuffer, uint16_t bytes_to_write)
{
	int numWrote = 0;
	uwb_tml_set_mode_uci();
#if UWBIOT_UWBD_SR2XXT && UWBIOT_TML_SPI
	if (bytes_to_write >= CONFIG_NXP_UWB_DEVICE_TX_BUFFER_MAX) {
		LOG_ERR("Too much data");
		return -EINVAL;
	}
	memcpy(g_tx_buffer + 1, pBuffer, bytes_to_write);
	g_tx_buffer[0] = DIRECTIONAL_BYTE_WRITE;
	uint8_t *p_buffer_with_direction = g_tx_buffer;
#else
	uint8_t *p_buffer_with_direction = pBuffer;
#endif /* UWBIOT_UWBD_SR2XXT */
	const UWBStatus_t writeStatus =
		uwb_uwbs_tml_data_tx(&g_uwbs_tml_ctx, p_buffer_with_direction, bytes_to_write);

	if (writeStatus == kUWBSTATUS_SUCCESS) {
		numWrote = g_uwbs_tml_ctx.noOfBytesWritten;
	} else if (writeStatus == kUWBSTATUS_BUSY) {
		LOG_DBG("uwb_transport_uci_write : uwb_uwbs_tml_data_tx failed");
		numWrote = -2;
	} else {
		LOG_DBG("uwb_transport_uci_write : uwb_uwbs_tml_data_tx failed");
		numWrote = -1;
	}

	return numWrote;
}

UWBSTATUS uwb_tml_reset_uwbs(void)
{
	UWBSTATUS status = UWBSTATUS_SUCCESS;
	if (uwb_uwbs_tml_reset(&g_uwbs_tml_ctx) != kUWBSTATUS_SUCCESS) {
		LOG_ERR("uwb_tml_reset_uwbs: uwb_uwbs_tml_reset failed");
		status = UWBSTATUS_FAILED;
	}
	return status;
}

int uwb_tml_reset(long level)
{
	LOG_DBG("uwb_tml_reset(), VEN level %ld", level);
#if ((UWBIOT_OS_NATIVE) && !((UWBIOT_TML_PNP) || (UWBIOT_TML_SOCKET)))
	/** Read abort for the Kernel mode */
	if (uwb_bus_io_uwbs_irq_disable(&g_uwbs_tml_ctx.busCtx) != kUWB_bus_Status_OK) {
		LOG_ERR("uwb_bus_io_uwbs_irq_disable failed");
	}
#endif // UWBIOT_OS_NATIVE
	return 1;
}

void uwb_tml_set_mode_uci(void)
{
	if (uwb_uwbs_tml_setmode(&g_uwbs_tml_ctx, kUWB_UWBS_TML_MODE_UCI) != kUWBSTATUS_SUCCESS) {
		LOG_ERR("uwb_tml_set_mode_uci : uwb_uwbs_tml_setmode failed");
	}
}
