/*
 * Copyright (C) 2026 NXP Semiconductors
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

/* System includes */

#include <stdint.h>
#include <stdio.h>

/* Raspbian includes */
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

/* UWB includes */
#include "uwb_tml_status.h"

#include "uwb_bus_interface.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nxp_bus_interface, CONFIG_UWB_NXP_LOG_LEVEL);

#define SPI_TRANSFER_TIMEOUT 1500

#if DT_NODE_HAS_STATUS(DT_CHOSEN(nxp_uwb_device), okay)
    #define UWB_NODE DT_CHOSEN(nxp_uwb_device)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(uwb0), okay)
    #define UWB_NODE DT_ALIAS(uwb0)
#elif DT_NODE_HAS_STATUS(DT_NODELABEL(sr2xx), okay)
    #define UWB_NODE DT_NODELABEL(sr2xx)
#else
    #error "No UWB device specified via chosen/alias/label"
#endif

/* Verify it's the right compatibility */
#if !DT_NODE_HAS_COMPAT(UWB_NODE, nxp_uwb_device)
    #error "Selected node is not compatible with nxp,uwb-device"
#endif

#define SPI_OP SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB

#define DIRECTIONAL_BYTE_LEN    0x01
#define DIRECTIONAL_BYTE_OFFSET 0x00
#define DIRECTIONAL_BYTE_WRITE  0x00
#define DIRECTIONAL_BYTE_READ   0xFF

static struct spi_dt_spec spi_spec = SPI_DT_SPEC_GET(UWB_NODE, SPI_OP, 0);
static int g_transfer_status;

/* This semaphore is signaled when SPI write is completed successfully*/
struct k_sem g_spi_transfer_sem;

static volatile void *g_spi_async_userdata;

static void spi_callback(const struct device *dev, int status, void *userdata)
{
	g_transfer_status = status;
	(void)k_sem_give(&g_spi_transfer_sem);
}

uwb_bus_status_t uwb_bus_init(uwb_bus_board_ctx_t *p_ctx)
{
	if (p_ctx == NULL) {
		LOG_ERR("uwbs bus context is NULL");
		return kUWB_bus_Status_FAILED;
	}
	p_ctx->masterHandle = &spi_spec;

	if (!spi_is_ready_dt(p_ctx->masterHandle)) {
		LOG_ERR("Error: SPI device is not ready");
		return kUWB_bus_Status_FAILED;
	}

	/* This semaphore is signaled when SPI data is send out completely Rhodes*/
	if (k_sem_init(&g_spi_transfer_sem, 0, 1) != 0) {
		LOG_ERR("Error: uwb_bus_init(), could not create semaphore g_spi_transfer_sem\n");
		return kUWB_bus_Status_FAILED;
	}

	/*This semaphore is signaled in the ISR context.*/
	if (k_sem_init(&p_ctx->mIrqWaitSem, 0, 1) != 0) {
		LOG_ERR("Error: uwb_uwbs_tml_init(), could not create semaphore mWaitIrqSem\n");
		return kUWB_bus_Status_FAILED;
	}
	LOG_DBG("uwb_bus_init Done");
	return kUWB_bus_Status_OK;
}

uwb_bus_status_t uwb_bus_data_tx(uwb_bus_board_ctx_t *p_ctx, uint8_t *p_buf, size_t buf_len)
{
	uwb_bus_status_t bus_status = kUWB_bus_Status_FAILED;
	int status = -1;
	struct spi_buf spi_buffers[2] = {0};
	size_t data_size;

	if (p_ctx == NULL) {
		LOG_ERR("uwbs bus context is NULL");
		goto end;
	}

	if (p_buf == NULL || buf_len == 0) {
		goto end;
	}

	/* Set direction byte as Host write */
	p_buf[DIRECTIONAL_BYTE_OFFSET] = DIRECTIONAL_BYTE_WRITE;
	data_size = buf_len;

	spi_buffers[0].buf = p_buf;
	spi_buffers[0].len = data_size + DIRECTIONAL_BYTE_LEN;
	spi_buffers[1].buf = NULL;

	const struct spi_buf_set tx_buff = {
		.buffers = &spi_buffers[0],
		.count = 1,
	};

	const struct spi_buf_set rx_buff = {
		.buffers = &spi_buffers[1],
		.count = 1,
	};

	status = spi_transceive_cb(p_ctx->masterHandle->bus, &p_ctx->masterHandle->config, &tx_buff,
				   &rx_buff, spi_callback, (void *)g_spi_async_userdata);
	if (status == 0) {
		if (k_sem_take(&g_spi_transfer_sem, Z_TIMEOUT_MS(SPI_TRANSFER_TIMEOUT)) != 0) {
			LOG_ERR("%s : spi transfer timeout", __FUNCTION__);
			goto end;
		}
		if (g_transfer_status != 0) {
			goto end;
		}
		bus_status = kUWB_bus_Status_OK;
	}
end:
	return bus_status;
}

uwb_bus_status_t uwb_bus_data_rx(uwb_bus_board_ctx_t *p_ctx, uint8_t *p_buf, size_t buf_len)
{
	uwb_bus_status_t bus_status = kUWB_bus_Status_FAILED;
	int status = -1;
	struct spi_buf spi_buffers[2];
	uint8_t DataBuff[2] = {0};

	if (p_ctx == NULL) {
		LOG_ERR("uwbs bus context is NULL");
		goto end;
	}

	if (p_buf == NULL || buf_len == 0) {
		LOG_ERR("uwb_bus_data_rx failed");
		goto end;
	}

	/* Set directional byte for read operation */
	DataBuff[DIRECTIONAL_BYTE_OFFSET] = DIRECTIONAL_BYTE_READ;
	spi_buffers[0].buf = DataBuff;
	spi_buffers[0].len = DIRECTIONAL_BYTE_LEN;

	spi_buffers[1].buf = p_buf;
	spi_buffers[1].len = buf_len + DIRECTIONAL_BYTE_LEN;

	const struct spi_buf_set tx_buff = {
		.buffers = &spi_buffers[0],
		.count = 1,
	};

	const struct spi_buf_set rx_buff = {
		.buffers = &spi_buffers[1],
		.count = 1,
	};

	status = spi_transceive_cb(p_ctx->masterHandle->bus, &p_ctx->masterHandle->config, &tx_buff,
				   &rx_buff, spi_callback, (void *)g_spi_async_userdata);
	if (status == 0) {
		if (k_sem_take(&g_spi_transfer_sem, Z_TIMEOUT_MS(SPI_TRANSFER_TIMEOUT)) != 0) {
			LOG_ERR("%s : spi transfer timeout", __FUNCTION__);
			goto end;
		}
		if (g_transfer_status != 0) {
			goto end;
		}
		bus_status = kUWB_bus_Status_OK;
	}
end:
	return bus_status;
}

uwb_bus_status_t uwb_bus_deinit(uwb_bus_board_ctx_t *p_ctx)
{
	if (p_ctx == NULL) {
		LOG_ERR("uwbs bus context is NULL");
		return kUWB_bus_Status_FAILED;
	}
	k_sem_reset(&g_spi_transfer_sem);
	k_sem_reset(&p_ctx->mIrqWaitSem);
	return kUWB_bus_Status_OK;

	// k_sem_give(&p_ctx->mIrqWaitSem);
	// k_msleep(2);
	// memset(p_ctx, 0, sizeof(uwb_bus_board_ctx_t));
	// return kUWB_bus_Status_OK;
}

uwb_bus_status_t uwb_bus_reset(uwb_bus_board_ctx_t *p_ctx)
{
	return kUWB_bus_Status_OK;
}

void uwb_port_DelayinMicroSec(int delay)
{
	k_sleep(K_USEC(delay));
}

uwb_bus_status_t uwb_bus_data_trx(uwb_bus_board_ctx_t *p_ctx, uint8_t *p_buf, size_t buf_len)
{
	uwb_bus_status_t bus_status = kUWB_bus_Status_FAILED;
	int status = -1;
	struct spi_buf spi_buffers[2];
	uint8_t DataBuff[2] = {0};

	if (p_ctx == NULL) {
		LOG_ERR("uwbs bus context is NULL");
		goto end;
	}

	if (p_buf == NULL || buf_len == 0) {
		LOG_ERR("uwb_bus_data_trx failed");
		goto end;
	}

	/* Data Receive */
	if (p_ctx->op_mode == READ_MODE) {
		LOG_DBG("uwb_bus_data_trx : READ_MODE ");
		/* set direction as Host Read */
		DataBuff[DIRECTIONAL_BYTE_OFFSET] = DIRECTIONAL_BYTE_READ;
		spi_buffers[0].buf = DataBuff;
		spi_buffers[0].len = DIRECTIONAL_BYTE_LEN;

		spi_buffers[1].buf = &p_buf[0];
		spi_buffers[1].len = buf_len + DIRECTIONAL_BYTE_LEN;

		const struct spi_buf_set tx_buff = {
			.buffers = &spi_buffers[0],
			.count = 1,
		};

		const struct spi_buf_set rx_buff = {
			.buffers = &spi_buffers[1],
			.count = 1,
		};

		status = spi_transceive_cb(p_ctx->masterHandle->bus, &p_ctx->masterHandle->config,
					   &tx_buff, &rx_buff, spi_callback,
					   (void *)g_spi_async_userdata);
		if (status == 0) {
			if (k_sem_take(&g_spi_transfer_sem,
				       Z_TIMEOUT_MS(MAX_UWBS_SPI_TRANSFER_TIMEOUT)) != 0) {
				LOG_ERR("%s : spi transfer timeout", __FUNCTION__);
				goto end;
			}
			if (g_transfer_status != 0) {
				goto end;
			}
			bus_status = kUWB_bus_Status_OK;
		}
	}
	/* Data Transmit */
	else if (p_ctx->op_mode == WRITE_MODE) {
		LOG_DBG("uwb_bus_data_trx : WRITE_MODE ");
		spi_buffers[0].buf = &p_buf[0];
		spi_buffers[0].len = buf_len + DIRECTIONAL_BYTE_LEN;
		spi_buffers[1].buf = NULL;

		const struct spi_buf_set tx_buff = {
			.buffers = &spi_buffers[0],
			.count = 1,
		};

		const struct spi_buf_set rx_buff = {
			.buffers = &spi_buffers[1],
			.count = 1,
		};

		status = spi_transceive_cb(p_ctx->masterHandle->bus, &p_ctx->masterHandle->config,
					   &tx_buff, &rx_buff, spi_callback,
					   (void *)g_spi_async_userdata);
		if (status == 0) {
			if (k_sem_take(&g_spi_transfer_sem,
				       Z_TIMEOUT_MS(MAX_UWBS_SPI_TRANSFER_TIMEOUT)) != 0) {
				LOG_ERR("%s : spi transfer timeout", __FUNCTION__);
				goto end;
			}
			if (g_transfer_status != 0) {
				goto end;
			}
			bus_status = kUWB_bus_Status_OK;
		}
	} else {
		/* Invalid mode  */
		LOG_ERR("Invalid Mode");
	}

end:
	return bus_status;
}
