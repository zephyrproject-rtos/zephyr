/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_INF
#include <logging/log.h>
LOG_MODULE_REGISTER(tuning);

#include <sys/byteorder.h>

#include <string.h>
#include <zephyr.h>

#include "audio_core.h"
#include "usb_transport.h"

static void tun_drv_process_command(uint8_t *data, uint32_t len);

/* Buffer size in "words" for the tuning commands and replies */
#define TUN_DRV_CMD_MAX_WORDS	(264)
#define TUN_DRV_CMD_MAX_BYTES	(TUN_DRV_CMD_MAX_WORDS << 2)

#define TUN_DRV_ALERT_Q_LEN	((TUN_DRV_CMD_MAX_BYTES +\
			USB_TPORT_HID_REPORT_DATA_LEN - 1) /\
		USB_TPORT_HID_REPORT_DATA_LEN)

static struct {
	/* room for transport header */
	union usb_hid_report_hdr header;

	/* command buffer for the tuning interface */
	uint8_t buffer[TUN_DRV_CMD_MAX_BYTES];

	/* length of valid data in the buffer */
	uint32_t len;

	/* index into the buffer where incoming data is copied */
	uint32_t index;
} __packed command_buffer;

/* Allocate a synchronization semaphore */
K_SEM_DEFINE(tun_drv_sem, 0, 1);

static int tun_drv_io_thread(void);

K_THREAD_DEFINE(tun_drv_io_thread_id, TUN_DRV_IO_THREAD_STACK_SIZE,
		tun_drv_io_thread, NULL, NULL, NULL, TUN_DRV_IO_THREAD_PRIORITY,
		0, 0);

static int tun_drv_io_thread(void)
{
	LOG_INF("Starting tuning driver I/O thread ...");

	/* initialize the audio core's tuning interface */
	audio_core_tuning_interface_init((uint32_t *)command_buffer.buffer,
			TUN_DRV_CMD_MAX_WORDS);

	/* initialize the tuning transport protocol driver */
	usb_transport_init(tun_drv_process_command);

	while (true) {
		k_sem_take(&tun_drv_sem, K_FOREVER);

		/* notify audio core on tuning command reception */
		audio_core_notify_tuning_cmd();
	}

	return 0;
}

int tun_drv_packet_handler(void)
{
	int ret = 0;
	uint32_t len;
	uint32_t first_word;

	if (audio_core_is_tuning_reply_ready() == false) {
		return ret;
	}

	first_word = *((uint32_t *)command_buffer.buffer);
	len = (first_word >> 16) << 2;

	ret = usb_transport_send_reply(command_buffer.buffer, len);
	if (ret) {
		LOG_ERR("usb_transport_send_reply error %d", ret);
	}
	return ret;
}

static void tun_drv_process_command(uint8_t *data, uint32_t len)
{
	uint32_t first_word;

	if (command_buffer.len == 0) {
		/* extract command length */
		first_word = *((uint32_t *)data);
		command_buffer.len = (first_word >> 16) << 2;

		/* ensure total length does not exceed max */
		if (command_buffer.len > TUN_DRV_CMD_MAX_BYTES) {
			command_buffer.len = TUN_DRV_CMD_MAX_BYTES;
		}
	}

	if ((command_buffer.index + len) >= command_buffer.len) {
		len = command_buffer.len - command_buffer.index;
	}

	if (len) {
		/* copy the received data into the tuning packet buffer */
		memcpy(&command_buffer.buffer[command_buffer.index], data, len);
		command_buffer.index += len;
	}

	if (command_buffer.index == command_buffer.len) {
		/* reset index and length */
		command_buffer.index = command_buffer.len = 0;
		/* notify tuning driver thread */
		k_sem_give(&tun_drv_sem);
	}
}
