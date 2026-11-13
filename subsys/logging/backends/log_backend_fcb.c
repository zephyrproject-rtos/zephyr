/*
 * Copyright (c) 2026 Sentek Pty Ltd
 *
 */

#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_backend_std.h>
#include "fcb_log.h"
#include "log_backend_fcb.h"
#include <zephyr/pm/device.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/pm/device.h>

#ifndef CONFIG_LOG_BACKEND_FCB_BUFFER_SIZE
#define CONFIG_LOG_BACKEND_FCB_BUFFER_SIZE 0
#endif

#ifndef CONFIG_LOG_BACKEND_FCB_MESSAGE_SIZE
#define CONFIG_LOG_BACKEND_FCB_MESSAGE_SIZE 0
#endif

#ifndef CONFIG_LOG_BACKEND_FCB_OUTPUT_BUFFER_SIZE
#define CONFIG_LOG_BACKEND_FCB_OUTPUT_BUFFER_SIZE 0
#endif

#ifndef CONFIG_LOG_BACKEND_FCB_RETRY_DELAY_MS
/* Long enough to detect host presence */
#define CONFIG_LOG_BACKEND_FCB_RETRY_DELAY_MS 10
#endif

#ifndef CONFIG_LOG_BACKEND_FCB_RETRY_CNT
/* Big enough to detect host presence */
#define CONFIG_LOG_BACKEND_FCB_RETRY_CNT 10
#endif

#define DROP_MAX 99

#define DROP_MSG "messages dropped:    \r\n"

#define DROP_MSG_LEN (sizeof(DROP_MSG) - 1)

#define MESSAGE_SIZE CONFIG_LOG_BACKEND_FCB_MESSAGE_SIZE

#define CHAR_BUF_SIZE                                                                              \
	((IS_ENABLED(CONFIG_LOG_BACKEND_FCB_MODE_BLOCK) && !IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) \
		 ? CONFIG_LOG_BACKEND_FCB_OUTPUT_BUFFER_SIZE                                       \
		 : 1)

#define FCB_LOCK() COND_CODE_0(CONFIG_LOG_BACKEND_FCB_BUFFER, (FCB_LOG_LOCK()), ())

#define FCB_UNLOCK() COND_CODE_0(CONFIG_LOG_BACKEND_FCB_BUFFER, (FCB_LOG_UNLOCK()), ())

#define FCB_BUFFER_SIZE                                                                            \
	COND_CODE_0(CONFIG_LOG_BACKEND_FCB_BUFFER, (0), (CONFIG_LOG_BACKEND_FCB_BUFFER_SIZE))

static uint8_t output_buf[MAX_FCB_LOG_ENTRY_SIZE];
static uint8_t msg_buf[MAX_FCB_LOG_ENTRY_SIZE];
static bool panic_mode;

struct fcb_backend_context {
	uint32_t fcb_dropped_count; // Tracks drops explicitly due to lock timeouts
};
static struct fcb_backend_context fcb_ctx;

static uint32_t log_format_current = LOG_OUTPUT_TEXT; // CONFIG_LOG_BACKEND_FCB_OUTPUT_DEFAULT;

const struct log_backend *log_backend_fcb_get(void);

static inline bool is_sync_mode(void)
{
	return IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) || panic_mode;
}

static inline bool is_panic_mode(void)
{
	return panic_mode;
}

static int data_out_overwrite_mode(uint8_t *data, size_t length, void *ctx)
{
	ARG_UNUSED(ctx);

	if (panic_mode && !IS_ENABLED(CONFIG_SENTEK_FCB_LOG_IN_PANIC)) {
		/* fault context: the flash driver may hang here instead of letting the device
		 * reboot */
		return length;
	}
	struct fcb_backend_context *context = &fcb_ctx;
	const struct log_backend *fcb_log_backend = log_backend_fcb_get();

	memset(msg_buf, 0, sizeof(msg_buf));
	memcpy(msg_buf, data, length);
	int clen = ROUND_UP(length, fcb_log_write_align());
	clen = MIN(clen, sizeof(msg_buf));
	if (context->fcb_dropped_count > 0) // first check if still drops
	{
		if (fcb_append_log_entry(msg_buf, clen) < 0) {
			return length; // Return size to clear it from Zephyr's buffer
		}
		// if recovered then ouptut number of dropped messages
		fcb_log_backend->api->dropped(fcb_log_backend, context->fcb_dropped_count);
		context->fcb_dropped_count = 0; // Reset counter
		return length;
	}
	// nothing been dropped prior this message
	if (fcb_append_log_entry(msg_buf, clen) < 0) {
		// log_backend_disable(log_backend_fcb_get());
		context->fcb_dropped_count++;
		return length; // Return size to clear it from Zephyr's buffer
	}
	// Check if we need to print a drop warning.
	if (context->fcb_dropped_count > 0) {
		// This alerts the user over the hardware channel that logs were lost
		// by injecting the standard "--- N messages dropped ---" text
		// log_output_dropped_process( fcb_log_backend->log_output,
		// context->fcb_dropped_count);
		fcb_log_backend->api->dropped(fcb_log_backend, context->fcb_dropped_count);
		context->fcb_dropped_count = 0; // Reset counter
	}

	return length;
}

LOG_OUTPUT_DEFINE(log_output_fcb, data_out_overwrite_mode, output_buf, sizeof(output_buf));

static int log_backend_fcb_cfg(void)
{
	return fcb_init_instance();
}

static void log_backend_fcb_init(struct log_backend const *const backend)
{
	if (CONFIG_FCB) {
		const struct device *flash_dev =
			DEVICE_DT_GET_OR_NULL(DT_CHOSEN(nordic_pm_ext_flash));
		if (flash_dev) {
			if (device_is_ready(flash_dev)) {
				pm_device_action_run(flash_dev, PM_DEVICE_ACTION_RESUME);
			}
		}
		log_backend_fcb_cfg();
	}
}

static void panic(struct log_backend const *const backend)
{
	panic_mode = true;
#if !LOG_MODE_IMMEDIATE
	log_backend_std_panic(&log_output_fcb);
#endif
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_backend_std_dropped(&log_output_fcb,
				cnt); /* can not output drop messages for FCB log backend */
}

static void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	uint32_t flags = LOG_OUTPUT_FLAG_TIMESTAMP | LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP |
			 LOG_OUTPUT_FLAG_LEVEL; // log_backend_std_get_flags();

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_fcb, &msg->log, flags);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}

const struct log_backend_api log_backend_fcb_api = {
	.process = process,
	.panic = panic,
	.init = log_backend_fcb_init,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,
	.format_set = format_set,
};

LOG_BACKEND_DEFINE(log_backend_fcb, log_backend_fcb_api, false);

const struct log_backend *log_backend_fcb_get(void)
{
	return &log_backend_fcb;
}

static int ini_fcb_logs(void)
{
	const struct log_backend *fcb_log_backend = log_backend_fcb_get();
	memset(&fcb_ctx, 0, sizeof(fcb_ctx));
	fcb_log_backend->api->init(fcb_log_backend);
	if (fcb_log_ready()) {
		log_backend_enable(log_backend_fcb_get(), NULL, CONFIG_LOG_MAX_LEVEL);
	}
	return 0;
}

SYS_INIT(ini_fcb_logs, POST_KERNEL, CONFIG_FCB_LOGS_INIT_PRIORITY);