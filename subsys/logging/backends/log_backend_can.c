/*
 * Copyright (c) 2025 Paul Wedeck <paulwedeck@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/logging/log_backend_can.h>
#include <zephyr/drivers/can.h>
#include <zephyr/spinlock.h>

#include <stdatomic.h>
#include <inttypes.h>

#define MAX_MSG_LEN        64
#define MAX_LEGACY_MSG_LEN 8
static uint8_t output_buf[MAX_MSG_LEN];

#if DT_HAS_CHOSEN(zephyr_log_canbus)
static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_log_canbus));
#elif DT_HAS_CHOSEN(zephyr_canbus)
static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
#else
#error CONFIG_LOG_BACKEND_CAN requires either zephyr,canbus or zephyr,log-canbus
#endif

struct backend_data {
	uint32_t format;
	uint32_t canid;
	uint8_t canflags;
};

struct k_spinlock backend_data_lock;
static struct backend_data backend_data = {
	.format = CONFIG_LOG_BACKEND_CAN_OUTPUT_DEFAULT,
	.canid = CONFIG_LOG_BACKEND_CAN_ID,
	.canflags = (IS_ENABLED(CONFIG_LOG_BACKEND_USE_EXTID) ? CAN_FRAME_IDE : 0) |
		    (IS_ENABLED(CONFIG_LOG_BACKEND_CAN_USE_FD) ? CAN_FRAME_FDF : 0) |
		    (IS_ENABLED(CONFIG_LOG_BACKEND_CAN_USE_FD_BRS) ? CAN_FRAME_BRS : 0),
};

static int line_out(uint8_t *data, size_t length, void *output_ctx)
{
	ARG_UNUSED(output_ctx);

	int ret;
	size_t max_frame_len, target_frame_len, actual_frame_len;
	struct can_frame frame;

	K_SPINLOCK(&backend_data_lock) {
		frame.id = backend_data.canid;
		frame.flags = backend_data.canflags;
	}

	max_frame_len = MAX_LEGACY_MSG_LEN;
#ifdef CONFIG_CAN_FD_MODE
	if (frame.flags & CAN_FRAME_FDF) {
		max_frame_len = MAX_MSG_LEN;
	}
#endif
	target_frame_len = min(max_frame_len, length);

	/* Not all frame sizes from 0 to MAX_MSG_LEN can be represented in a can frame */
	frame.dlc = can_bytes_to_dlc(target_frame_len);
	actual_frame_len = can_dlc_to_bytes(frame.dlc);
	if (actual_frame_len > target_frame_len) {
		frame.dlc--;
		actual_frame_len = can_dlc_to_bytes(frame.dlc);
	}
	memcpy(frame.data, data, actual_frame_len);

	ret = can_send(can_dev, &frame, K_FOREVER, NULL, NULL);
	ARG_UNUSED(ret);

	return actual_frame_len;
}

LOG_OUTPUT_DEFINE(log_output_can, line_out, output_buf, sizeof(output_buf));

bool log_backend_can_set_frameopts(uint32_t id, uint8_t flags)
{
	uint32_t idmask = (flags & CAN_FRAME_IDE) ? CAN_EXT_ID_MASK : CAN_STD_ID_MASK;

	if ((flags & (CAN_FRAME_IDE | CAN_FRAME_FDF | CAN_FRAME_BRS)) != flags) {
		return false;
	}

	if ((id & idmask) != id) {
		return false;
	}

#ifdef CONFIG_CAN_FD_MODE
	/* only send CAN-FD frames if it is supported */
	if ((flags & CAN_FRAME_FDF) && !(can_get_mode(can_dev) & CAN_MODE_FD)) {
		return false;
	}

	/* bitrate switching (BRS) is a CAN-FD feature */
	if ((flags & CAN_FRAME_BRS) && !(flags & CAN_FRAME_FDF)) {
		return false;
	}
#else
	if ((flags & (CAN_FRAME_FDF | CAN_FRAME_BRS)) != 0) {
		return false;
	}
#endif

	K_SPINLOCK(&backend_data_lock) {
		backend_data.canid = id;
		backend_data.canflags = flags;
	}
	return true;
}

static void init(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);

	enum can_state state;

	if (can_get_state(can_dev, &state, NULL) < 0 || state == CAN_STATE_STOPPED) {
		/* enable CAN-FD if it is supported */
		can_set_mode(can_dev, CAN_MODE_FD);
		can_start(can_dev);
	}

	if (!(can_get_mode(can_dev) & CAN_MODE_FD)) {
		K_SPINLOCK(&backend_data_lock) {
			backend_data.canflags &= ~(CAN_FRAME_FDF | CAN_FRAME_BRS);
		}
	}
}

static void panic(struct log_backend const *const backend)
{
	ARG_UNUSED(backend);

	log_backend_std_panic(&log_output_can);
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_backend_std_dropped(&log_output_can, cnt);
}

static void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	uint32_t current_format;

	K_SPINLOCK(&backend_data_lock) {
		current_format = backend_data.format;
	}

	uint32_t flags = log_backend_std_get_flags() & ~LOG_OUTPUT_FLAG_COLORS;

	log_format_func_t log_output_func = log_format_func_t_get(current_format);

	log_output_func(&log_output_can, &msg->log, flags);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	K_SPINLOCK(&backend_data_lock) {
		backend_data.format = log_type;
	}

	return 0;
}

const struct log_backend_api log_backend_can_api = {
	.panic = panic,
	.dropped = dropped,
	.init = init,
	.process = process,
	.format_set = format_set,
};

LOG_BACKEND_DEFINE(log_backend_can, log_backend_can_api,
		   IS_ENABLED(CONFIG_LOG_BACKEND_CAN_AUTOSTART));
