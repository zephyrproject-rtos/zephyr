/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME app_fw_update
#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/net/lwm2m.h>
#include <zephyr/sys/crc.h>
#include "lwm2m_engine.h"

static uint8_t firmware_buf[64];
static uint32_t crc;

/* Array with supported PULL firmware update protocols */
static uint8_t supported_protocol[1];

static int firmware_update_cb(uint16_t obj_inst_id,
			      uint8_t *args, uint16_t args_len)
{
	LOG_INF("UPDATE, (CRC %u)", crc);

	lwm2m_set_u8(&LWM2M_OBJ(5, 0, 3), STATE_IDLE);
	lwm2m_set_u8(&LWM2M_OBJ(5, 0, 5), RESULT_SUCCESS);
	return 0;
}

static void *firmware_get_buf(uint16_t obj_inst_id, uint16_t res_id,
			      uint16_t res_inst_id, size_t *data_len)
{
	*data_len = sizeof(firmware_buf);
	return firmware_buf;
}

static int firmware_block_received_cb(uint16_t obj_inst_id, uint16_t res_id,
				      uint16_t res_inst_id, uint8_t *data,
				      uint16_t data_len, bool last_block,
				      size_t total_size, size_t offset)
{
	if (offset == 0) {
		crc = crc32_ieee(data, data_len);
	} else {
		crc = crc32_ieee_update(crc, data, data_len);
	}
	LOG_INF("FIRMWARE: BLOCK RECEIVED: offset:%zd len:%u last_block:%d crc: %u",
		offset, data_len, last_block, crc);

	/* Add extra delay so short block-wise may timeout */
	k_sleep(K_MSEC(100));
	return 0;
}

static int firmware_cancel_cb(const uint16_t obj_inst_id)
{
	LOG_INF("FIRMWARE: Update canceled");
	return 0;
}

static int init_firmware_update(void)
{
	/* setup data buffer for block-wise transfer */
	lwm2m_register_pre_write_callback(&LWM2M_OBJ(5, 0, 0), firmware_get_buf);
	lwm2m_firmware_set_write_cb(firmware_block_received_cb);

	/* register cancel callback */
	lwm2m_firmware_set_cancel_cb(firmware_cancel_cb);
	lwm2m_firmware_set_update_cb(firmware_update_cb);

	lwm2m_create_res_inst(&LWM2M_OBJ(5, 0, 8, 0));
	lwm2m_set_res_buf(&LWM2M_OBJ(5, 0, 8, 0), &supported_protocol[0],
					sizeof(supported_protocol[0]),
					sizeof(supported_protocol[0]), 0);

	return 0;
}
LWM2M_APP_INIT(init_firmware_update);
