/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/gnss/rtk/decoder.h>
#include <zephyr/gnss/rtk/rtk_publish.h>
#include <zephyr/gnss/rtk/rtk.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtk_serial, CONFIG_GNSS_RTK_LOG_LEVEL);

static const struct device *rtk_serial_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_rtk_serial));
static struct ring_buf process_ringbuf;
static uint8_t process_buf[2048];

static void gnss_rtk_process_work_handler(struct k_work *work)
{
	static uint8_t work_buf[2048];
	uint32_t len = ring_buf_get(&process_ringbuf, work_buf, sizeof(work_buf));
	uint32_t offset = 0;

	ARG_UNUSED(work);

	do {
		uint8_t *frame;
		size_t frame_len;
		int err;

		err = gnss_rtk_decoder_frame_get(work_buf + offset, len - offset,
						 &frame, &frame_len);
		if (err != 0) {
			return;
		}

		LOG_HEXDUMP_DBG(frame, frame_len, "Frame received");

		/* Publish results */
		struct gnss_rtk_data rtk_data = {
			.data = frame,
			.len = frame_len,
		};

		gnss_rtk_publish_data(&rtk_data);
		offset += frame_len;

	} while (len > offset);
}

static K_WORK_DELAYABLE_DEFINE(gnss_rtk_process_work, gnss_rtk_process_work_handler);

static void rtk_uart_isr_callback(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	(void)uart_irq_update(dev);

	if (uart_irq_rx_ready(dev)) {
		int ret;

		do {
			char c;

			ret = uart_fifo_read(dev, &c, 1);
			if (ret > 0) {
				ret = ring_buf_put(&process_ringbuf, &c, 1);
			}
		} while (ret > 0);

		/** Since messages come through in a burst at a period
		 * (e.g: 1 Hz), wait until all messages are received before
		 * processing.
		 */
		(void)k_work_reschedule(&gnss_rtk_process_work, K_MSEC(10));
	}
}

static int rtk_serial_client_init(void)
{
	int err;

	ring_buf_init(&process_ringbuf, ARRAY_SIZE(process_buf), process_buf);

	err = uart_irq_callback_user_data_set(rtk_serial_dev, rtk_uart_isr_callback, NULL);
	if (err < 0) {
		return err;
	}

	uart_irq_rx_enable(rtk_serial_dev);

	return 0;
}

SYS_INIT(rtk_serial_client_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
