/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_obj_firmware_pull
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>

#include "lwm2m_pull_context.h"
#include "lwm2m_engine.h"

static void set_update_result_from_error(int error_code)
{
	if (!error_code) {
		lwm2m_firmware_set_update_state(STATE_DOWNLOADED);
		return;
	}

	if (error_code == -ENOMEM) {
		lwm2m_firmware_set_update_result(RESULT_OUT_OF_MEM);
	} else if (error_code == -ENOSPC) {
		lwm2m_firmware_set_update_result(RESULT_NO_STORAGE);
	} else if (error_code == -EFAULT) {
		lwm2m_firmware_set_update_result(RESULT_INTEGRITY_FAILED);
	} else if (error_code == -ENOMSG) {
		lwm2m_firmware_set_update_result(RESULT_CONNECTION_LOST);
	} else if (error_code == -ENOTSUP) {
		lwm2m_firmware_set_update_result(RESULT_INVALID_URI);
	} else if (error_code == -EPROTONOSUPPORT) {
		lwm2m_firmware_set_update_result(RESULT_UNSUP_PROTO);
	} else {
		lwm2m_firmware_set_update_result(RESULT_UPDATE_FAILED);
	}
}

static struct firmware_pull_context fota_context = {
	.firmware_ctx = {
		.sock_fd = -1
	},
	.result_cb = set_update_result_from_error
};

/* TODO: */
int lwm2m_firmware_cancel_transfer(void)
{
	return 0;
}

int lwm2m_firmware_start_transfer(char *package_uri)
{
	fota_context.write_cb = lwm2m_firmware_get_write_cb();

	/* start file transfer work */
	strncpy(fota_context.uri, package_uri, LWM2M_PACKAGE_URI_LEN - 1);
	lwm2m_pull_context_start_transfer(&fota_context);
	lwm2m_firmware_set_update_state(STATE_DOWNLOADING);

	return 0;
}

/**
 * @brief Get the block context of the current firmware block.
 *
 * @return A pointer to the firmware block context
 */
struct coap_block_context *lwm2m_firmware_get_block_context()
{
	return &fota_context.block_ctx;
}
