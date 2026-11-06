/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <sample_usbd.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/class/usbd_uac2.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(uac2_loopback, LOG_LEVEL_INF);

#define PLAYBACK_TERMINAL_ID UAC2_ENTITY_ID(DT_NODELABEL(usb_in_terminal))
#define CAPTURE_TERMINAL_ID  UAC2_ENTITY_ID(DT_NODELABEL(usb_out_terminal))

#define SAMPLE_RATE          48000
#define SAMPLE_WIDTH_BYTES   2
#define CHANNEL_COUNT        2
#define FRAME_SIZE           (SAMPLE_RATE / 1000 * SAMPLE_WIDTH_BYTES * CHANNEL_COUNT)
#define BUFFER_COUNT         4

K_MEM_SLAB_DEFINE_STATIC(audio_slab, ROUND_UP(FRAME_SIZE, UDC_BUF_GRANULARITY),
			 BUFFER_COUNT, UDC_BUF_ALIGN);

struct loopback_context {
	void *pending;
	uint16_t pending_size;
	bool capture_enabled;
};

static void *get_recv_buf(const struct device *dev, uint8_t terminal,
			  uint16_t size, void *user_data)
{
	void *buf;

	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (terminal != PLAYBACK_TERMINAL_ID || size > FRAME_SIZE ||
	    k_mem_slab_alloc(&audio_slab, &buf, K_NO_WAIT) != 0) {
		return NULL;
	}

	return buf;
}

static void data_received(const struct device *dev, uint8_t terminal,
			  void *buf, uint16_t size, void *user_data)
{
	struct loopback_context *ctx = user_data;

	ARG_UNUSED(dev);

	if (terminal != PLAYBACK_TERMINAL_ID || size == 0) {
		k_mem_slab_free(&audio_slab, buf);
		return;
	}

	if (ctx->pending != NULL) {
		k_mem_slab_free(&audio_slab, ctx->pending);
	}

	ctx->pending = buf;
	ctx->pending_size = size;
}

static void buffer_released(const struct device *dev, uint8_t terminal,
			    void *buf, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (terminal == CAPTURE_TERMINAL_ID) {
		k_mem_slab_free(&audio_slab, buf);
	}
}

static void terminal_updated(const struct device *dev, uint8_t terminal,
			     bool enabled, bool microframes, void *user_data)
{
	struct loopback_context *ctx = user_data;

	ARG_UNUSED(dev);
	ARG_UNUSED(microframes);

	if (terminal == CAPTURE_TERMINAL_ID) {
		ctx->capture_enabled = enabled;
	}
}

static void start_of_frame(const struct device *dev, void *user_data)
{
	struct loopback_context *ctx = user_data;
	void *buf = ctx->pending;
	uint16_t size = ctx->pending_size;

	ctx->pending = NULL;
	ctx->pending_size = 0;

	if (!ctx->capture_enabled) {
		if (buf != NULL) {
			k_mem_slab_free(&audio_slab, buf);
		}
		return;
	}

	if (buf == NULL) {
		if (k_mem_slab_alloc(&audio_slab, &buf, K_NO_WAIT) != 0) {
			return;
		}

		size = FRAME_SIZE;
		memset(buf, 0, size);
	}

	if (usbd_uac2_send(dev, CAPTURE_TERMINAL_ID, buf, size) != 0) {
		k_mem_slab_free(&audio_slab, buf);
	}
}

static const struct uac2_ops audio_ops = {
	.sof_cb = start_of_frame,
	.terminal_update_cb = terminal_updated,
	.get_recv_buf = get_recv_buf,
	.data_recv_cb = data_received,
	.buf_release_cb = buffer_released,
};

static struct loopback_context loopback;

int main(void)
{
	const struct device *uac2 = DEVICE_DT_GET(DT_NODELABEL(uac2_loopback));
	struct usbd_context *sample_usbd;
	int ret;

	if (!device_is_ready(uac2)) {
		LOG_ERR("UAC2 device is not ready");
		return -ENODEV;
	}

	usbd_uac2_set_ops(uac2, &audio_ops, &loopback);

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		return -ENODEV;
	}

	ret = usbd_enable(sample_usbd);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB device support: %d", ret);
		return ret;
	}

	LOG_INF("USB audio loopback enabled");
	return 0;
}
