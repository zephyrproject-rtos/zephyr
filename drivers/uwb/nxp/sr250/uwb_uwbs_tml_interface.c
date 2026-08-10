/*
 * Copyright 2021-2024 NXP
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

#include <uwb_uwbs_tml_interface.h>
#include <uwb_bus_board.h>
#include "uwb_tml_status.h"
#include "zephyr/uwb/uwb_core.h"
#include "uwb_uwbs_driver_config.h"

#include "uwb_tml_common.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nxp_sr250_tml_interface, CONFIG_UWB_NXP_LOG_LEVEL);

#define PAYLOAD_LEN_MSB 0x01
#define PAYLOAD_LEN_LSB 0x02
#define CRC_BYTES_LEN   0x02

#define UCI_RX_HDR_LEN             0x04
#define NORMAL_MODE_LEN_OFFSET     0x03
#define EXTND_LEN_INDICATOR_OFFSET 0x01
#define EXTENDED_LENGTH_OFFSET     0x02

#define DATA_TX_RX_PACKET_ID 0x02

#define UCI_HDR_LEN      0x04
#define HDLL_HEADER_SIZE 0x02
#define PLATFORM_OFFSET  0x01

#define EXTND_LEN_INDICATOR_OFFSET_MASK 0x80

// Wait for 10 Sec for read irq otherwise this will wake up the reader thread multiple times.
#define READ_IRQ_WAIT_TIME (10000)

extern void AddDelayInMicroSec(int delay);

UWBStatus_t uwb_uwbs_tml_init(uwb_uwbs_tml_ctx_t *pCtx)
{
	UWBStatus_t status = kUWBSTATUS_FAILED;
	uwb_bus_status_t bus_status = kUWB_bus_Status_FAILED;

	if (pCtx == NULL) {
		LOG_ERR("uwbs tml context is NULL");
		status = kUWBSTATUS_INVALID_PARAMETER;
		goto exit;
	}
	if (pCtx->isInitialized) {
		/** Already initialized */
#if (UWBIOT_UWBD_SR04X)
		k_sem_reset(&pCtx->mReadyIrqWaitSem);
#endif /* (UWBIOT_UWBD_SR04X) */
		return kUWBSTATUS_SUCCESS;
	}

	memset(pCtx, 0, sizeof(uwb_uwbs_tml_ctx_t));

	// by default set tml mode to UCI
	pCtx->mode = kUWB_UWBS_TML_MODE_UCI;

	status = (UWBStatus_t)k_mutex_init(&(pCtx->mSyncMutex));
	if (status != kUWBSTATUS_SUCCESS) {
		LOG_ERR("Error: uwb_uwbs_tml_init(), could not create mutex mSyncMutex\n");
		goto exit;
	}

	bus_status = uwb_bus_init(&pCtx->busCtx);
	if (bus_status != kUWB_bus_Status_OK) {
		status = kUWBSTATUS_CONNECTION_FAILED;
		LOG_ERR("Error: uwb_uwbs_tml_init(), uwb bus initialisation failed");
		goto exit;
	}

	if (uwb_bus_io_init(&pCtx->busCtx) != kUWB_bus_Status_OK) {
		status = UWBSTATUS_FAILED;
		goto exit;
	}

	pCtx->isInitialized = true;
	status = kUWBSTATUS_SUCCESS;
exit:
	return status;
}

UWBStatus_t uwb_uwbs_tml_setmode(uwb_uwbs_tml_ctx_t *pCtx, uwb_uwbs_tml_mode_t mode)
{
	if (pCtx == NULL) {
		LOG_ERR("uwbs tml context is NULL");
		return kUWBSTATUS_INVALID_PARAMETER;
	}
	pCtx->mode = mode;
	return kUWBSTATUS_SUCCESS;
}

UWBStatus_t uwb_uwbs_tml_deinit(uwb_uwbs_tml_ctx_t *pCtx)
{
	uwb_bus_deinit(&pCtx->busCtx);
	// memset(pCtx, 0, sizeof(uwb_uwbs_tml_ctx_t));
	return kUWBSTATUS_SUCCESS;
}

UWBStatus_t uwb_uwbs_tml_data_tx(uwb_uwbs_tml_ctx_t *pCtx, uint8_t *pBuf, size_t bufLen)
{
	/**
	 * @brief Buffer handling in the Tx
	 * The Buffer Contains the following bytes in sequence
	 *   - UCI_CMD_INDEX - 1 byte allocation will be done in the Hal.
	 *   - PLATFORM_RELATED_BYTES Extra bytes needed for Platform specific handling
	 *   - ACTUAL PACKET contain the actual UCI/HDLL/HBCI packets including header and payload
	 *   - Always the SPI Write will happen from the DIRECTION_BYTE_OFFSET
	 *   - The zeroth index is filled by the driver with a bidirectional byte, and the UCI
	 * command starts from the first index (index 1) -Example : The "zeroth index" (index 0)
	 * will be filled by the driver with the bidirectional byte, and the UCI command starts from
	 * the first index Example- [00(By directional)]21000005 44332211 00
	 */
	uwb_bus_status_t bus_status = kUWB_bus_Status_FAILED;
	UWBStatus_t status = kUWBSTATUS_FAILED;

	if (pCtx == NULL) {
		LOG_ERR("uwbs tml context is NULL");
		status = kUWBSTATUS_INVALID_PARAMETER;
		goto end;
	}

	pCtx->noOfBytesWritten = -1;

	if (pBuf == NULL || bufLen == 0) {
		LOG_ERR("write buffer is Null or bufLen is 0");
		status = kUWBSTATUS_INVALID_PARAMETER;
		goto end;
	}

	k_mutex_lock(&pCtx->mSyncMutex, K_FOREVER);

#if UWBIOT_OS_ZEPHYR
	// pre write delay to allow the device to recover from the last batch of data
	k_msleep(2);
#endif

	if (pCtx->mode == kUWB_UWBS_TML_MODE_HDLL) {
		bus_status = uwb_bus_data_tx(&pCtx->busCtx, pBuf, bufLen);
		if (bus_status == kUWB_bus_Status_FAILED) {
			LOG_ERR("uwb_bus_data_tx writing HDLL Command failed");
			goto end;
		}
	} else if (pCtx->mode == kUWB_UWBS_TML_MODE_UCI) {
		bus_status = uwb_bus_data_tx(&pCtx->busCtx, pBuf, bufLen);
		if (bus_status == kUWB_bus_Status_FAILED) {
			LOG_ERR("uwb_bus_data_tx writing UCI header failed");
			goto end;
		}
		AddDelayInMicroSec(80);
		pCtx->noOfBytesWritten = bufLen;
	} else {
		LOG_ERR("%s : tml mode not supported", __FUNCTION__);
		goto end;
	}
	status = kUWBSTATUS_SUCCESS;
end:
	k_mutex_unlock(&pCtx->mSyncMutex);
	return status;
}

UWBStatus_t uwb_uwbs_tml_data_rx(uwb_uwbs_tml_ctx_t *pCtx, uint8_t *pBuf, size_t *pBufLen)
{
	/**
	 * @brief Buffer handling in the Rx
	 * The Buffer Contains the following bytes in sequence
	 *   - UCI_CMD_INDEX --> 1 byte allocation will be done in the Hal.
	 *  - The UCI payload represents the actual UCI data, including its header and payload.
	 * After processing the platform byte and updating the buffer index, the data pointer will
	 * point to the UCI payload.
	 *      -  Example:
	 *          - After receiving and reading the complete UCI packet:
	 *              -   "FF60010001 FF01"
	 *          - By applying the following macro: SHIFT_AND_OVERRIDE_HEADER The packet is
	 * transformed to:
	 *              -   "FF60 6001000101"
	 *          - The UCI payload starts from this point. This macro manipulates the buffer so
	 * that the actual UCI data is positioned after adjusting for platform-specific and
	 * bidirectional bytes.
	 */
	uwb_bus_status_t bus_status = kUWB_bus_Status_FAILED;
	UWBStatus_t status = kUWBSTATUS_FAILED;
	uint16_t payloadLen;
	uint16_t len = UCI_RX_HDR_LEN;
	uint16_t totalBytesToRead = 0;
	uint32_t IsExtndLenIndication = 0;
	uwbs_io_state_t gpioValue = kUWBS_IO_State_NA;

	if (pCtx == NULL) {
		LOG_ERR("uwb_uwbs_tml_data_rx : uwbs tml context is NULL");
		return kUWBSTATUS_INVALID_PARAMETER;
	}

	if (pBuf == NULL || pBufLen == NULL) {
		LOG_ERR("uwb_uwbs_tml_data_rx : read buffer is Null");
		return kUWBSTATUS_INVALID_PARAMETER;
	}

	bus_status = uwb_bus_io_irq_wait(&pCtx->busCtx, READ_IRQ_WAIT_TIME);
	if (bus_status == kUWB_bus_Status_FAILED) {
		status = kUWBSTATUS_RESPONSE_TIMEOUT;
		*pBufLen = 0;
		goto exit;
	}

	/* Check for Spurious Interrupt */
	uwb_bus_io_val_get(&pCtx->busCtx, kUWBS_IO_I_UWBS_IRQ, &gpioValue);
	if (gpioValue == kUWBS_IO_State_Low) {
		status = kUWBSTATUS_RESPONSE_TIMEOUT;
		*pBufLen = 0;
		goto exit;
	}
	/* If reader thread is waiting for irq and in between writer thread is proceeded(in PnP
	case) operation mode will be affected so, once irq is available set bus cntx to read mode */

	if (pCtx->mode == kUWB_UWBS_TML_MODE_HDLL) {
		if (*pBufLen == 0) {
			bus_status = uwb_bus_data_rx(&pCtx->busCtx, &pBuf[DIRECTION_BYTE_OFFSET],
						     HDLL_HEADER_SIZE);
			if (bus_status != kUWB_bus_Status_OK) {
				LOG_ERR("uwb_uwbs_tml_data_rx : reading from helios failed");
				*pBufLen = 0;
				goto exit;
			}
			payloadLen =
				(uint16_t)(((pBuf[DIRECTION_BYTE_OFFSET + PAYLOAD_LEN_MSB] & 0x1F)
					    << 8) |
					   pBuf[DIRECTION_BYTE_OFFSET + PAYLOAD_LEN_LSB]);

			/* Validate total packet size fits in uint8_t for *pBufLen */

			if (payloadLen > (UINT8_MAX - HDLL_HEADER_SIZE - CRC_BYTES_LEN)) {
				LOG_ERR("uwb_uwbs_tml_data_rx : HDLL payload too large: %u bytes "
					"(max %u)",
					payloadLen, UINT8_MAX - HDLL_HEADER_SIZE - CRC_BYTES_LEN);
				*pBufLen = 0;
				goto exit;
			}
			totalBytesToRead = (uint16_t)(payloadLen + CRC_BYTES_LEN);

			if (payloadLen != 0) {
				bus_status =
					uwb_bus_data_rx(&pCtx->busCtx,
							&pBuf[DIRECTION_BYTE_OFFSET +
							      HDLL_HEADER_SIZE + PLATFORM_OFFSET],
							totalBytesToRead);
				if (bus_status != kUWB_bus_Status_OK) {
					LOG_ERR("uwb_uwbs_tml_data_rx : reading from helios "
						"failed");
					*pBufLen = 0;
					goto exit;
				}
			}
			*pBufLen = (uint8_t)(HDLL_HEADER_SIZE + payloadLen + CRC_BYTES_LEN);
		} else if (*pBufLen > 0) {
			bus_status = uwb_bus_data_rx(&pCtx->busCtx, pBuf, *pBufLen);
			if (bus_status != kUWB_bus_Status_OK) {
				LOG_ERR("uwb_uwbs_tml_data_rx : reading from helios failed");
				*pBufLen = 0;
				goto exit;
			}
		} else {
			return kUWBSTATUS_INVALID_PARAMETER;
		}
		SHIFT_AND_OVERRIDE_HEADER(pBuf, HDLL_HEADER_SIZE);
		status = kUWBSTATUS_SUCCESS;
	} else if (pCtx->mode == kUWB_UWBS_TML_MODE_UCI) {
		/* Check for Spurious Interrupt */
		uwb_bus_io_val_get(&pCtx->busCtx, kUWBS_IO_I_UWBS_IRQ, &gpioValue);
		if (gpioValue == kUWBS_IO_State_Low) {
			status = kUWBSTATUS_RESPONSE_TIMEOUT;
			*pBufLen = 0;
			goto exit;
		}
		k_mutex_lock(&pCtx->mSyncMutex, K_FOREVER);
		*pBufLen = 0;
		AddDelayInMicroSec(500);
		bus_status = uwb_bus_data_rx(&pCtx->busCtx, &pBuf[DIRECTION_BYTE_OFFSET], len);
		if (bus_status == kUWB_bus_Status_OK) {
			*pBufLen = (size_t)(len);
		} else {
			LOG_ERR("uwb_uwbs_tml_data_rx : reading from helios failed");
			*pBufLen = 0;
			goto end;
		}
		/* For data packet, we get 2 bytes of data always so extended len handling is not
		 * required */
		uci_control_packet_header_t *header =
			(uci_control_packet_header_t *)&pBuf[UCI_CMD_INDEX];
		if (header->mt == UCI_MT_DATA) {
			totalBytesToRead = pBuf[UCI_CMD_INDEX + NORMAL_MODE_LEN_OFFSET];
			totalBytesToRead = (uint16_t)((totalBytesToRead << 8) |
						      pBuf[UCI_CMD_INDEX + EXTENDED_LENGTH_OFFSET]);
		} else {
			IsExtndLenIndication = (pBuf[UCI_CMD_INDEX + EXTND_LEN_INDICATOR_OFFSET] &
						EXTND_LEN_INDICATOR_OFFSET_MASK);

			totalBytesToRead = pBuf[UCI_CMD_INDEX + NORMAL_MODE_LEN_OFFSET];

			if (IsExtndLenIndication) {
				totalBytesToRead =
					(uint16_t)((totalBytesToRead << 8) |
						   pBuf[UCI_CMD_INDEX + EXTENDED_LENGTH_OFFSET]);
			}
		}

		payloadLen = totalBytesToRead;
		if (payloadLen != 0) {
			bus_status = uwb_bus_data_rx(&pCtx->busCtx, &pBuf[len + UCI_CMD_INDEX],
						     payloadLen);
			if (bus_status == kUWB_bus_Status_OK) {
				if ((UINT16_MAX - payloadLen) <= (uint16_t)(*pBufLen)) {
					LOG_ERR("uwb_uwbs_tml_data_rx : buffer length overflow "
						"detected");
					*pBufLen = 0;
					goto exit;
				} else {
					*pBufLen = (uint16_t)(*pBufLen + payloadLen);
				}
			} else {
				LOG_ERR("uwb_uwbs_tml_data_rx : reading from helios failed");
				*pBufLen = 0;
			}
		}
		SHIFT_AND_OVERRIDE_HEADER(pBuf, UCI_RX_HDR_LEN);
		// TODO: Disable the irq check to go low after read. It might possible that
		// irq goes high within some uSec if command contains some notification and MPU miss
		// to detect the IRQ state. eg. in one of the case irq pin toggle for sending ntf
		// within 28usec.
#if 0
        uwbs_io_state_t gpioValue = kUWBS_IO_State_NA;
        uint8_t count                 = 0;
        AddDelayInMicroSec(1);
        uwb_bus_io_val_get(&pCtx->busCtx, kUWBS_IO_I_UWBS_IRQ, &gpioValue);
        while (gpioValue) {
            uwb_bus_io_val_get(&pCtx->busCtx, kUWBS_IO_I_UWBS_IRQ, &gpioValue);
            if (count >= 200) {
                LOG_ERR("count: %d", count);
                goto end;
            }
            // Sleep of 500us * 20 = 10ms as per artf786394
            AddDelayInMicroSec(5);
            count++;
        }
#endif
		// LOG_MAU8_W("UCI RX < ", pBuf, *pBufLen);
		status = kUWBSTATUS_SUCCESS;
end:
		k_mutex_unlock(&pCtx->mSyncMutex);
	} else {
		LOG_ERR("uwb_uwbs_tml_data_tx : tml mode not supported");
	}

exit:
	AddDelayInMicroSec(50);
	return status;
}

UWBStatus_t uwb_uwbs_tml_reset(uwb_uwbs_tml_ctx_t *pCtx)
{
	UWBStatus_t status = kUWBSTATUS_FAILED;
	uint8_t resp[20] = {0};
	size_t resp_len = 0;

	if (kUWB_bus_Status_OK != uwb_bus_reset(&pCtx->busCtx)) {
		return kUWBSTATUS_FAILED;
	}

	/* 1st Read HDLL Ready Notification */
	pCtx->mode = kUWB_UWBS_TML_MODE_HDLL;
	status = uwb_uwbs_tml_data_rx(pCtx, resp, &resp_len);
	if (status != kUWBSTATUS_SUCCESS) {
		LOG_ERR("uwb_uwbs_tml_reset : uwb_uwbs_tml_data_rx failed");
		goto end;
	}

	// TODO: calculate crc and match with received one
	/* expected 0004810102007DC8 */
	if (resp_len == 0x08 && (*resp == 0x00) && (*(resp + 1) == 0x04) && (*(resp + 6) == 0x7D) &&
	    (*(resp + 7) == 0xC8)) {
		/* Device is ready */
		status = kUWBSTATUS_SUCCESS;
	} else {
		status = kUWBSTATUS_FAILED;
	}
end:
	return status;
}
