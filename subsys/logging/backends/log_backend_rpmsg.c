/*
 * Copyright (c) 2025 Lexmark
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <openamp/rpmsg.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util_macro.h>

/**
 * @brief Backend data structure
 * Contains backend-specific runtime data
 */
struct log_backend_rpmsg_data {
	uint32_t log_format_current;
	struct rpmsg_endpoint ept;
};

/**
 * @brief Backend context structure
 * Contains references to output handler and backend data
 */
struct log_backend_rpmsg_ctx {
	const struct log_output *output;
	struct log_backend_rpmsg_data *data;
};

/**
 * @brief Character output function
 * This function is called by the log output subsystem to send data
 *
 * @param data   Pointer to data buffer
 * @param length Number of bytes to output
 * @param ctx    Context pointer (log_backend_rpmsg_ctx)
 * @return       Number of bytes actually output
 */
static int char_out(uint8_t *data, size_t length, void *ctx)
{
	const struct log_backend_rpmsg_ctx *backend_ctx = ctx;
	struct log_backend_rpmsg_data *backend_data = backend_ctx->data;
	int ret = 0;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_RPMSG_BLOCKING_MODE)) {
		ret = rpmsg_send(&backend_data->ept, data, (int)length);
	} else {
		ret = rpmsg_trysend(&backend_data->ept, data, (int)length);
	}

	/**
	 * This function cannot return negative values - see log_output_write()
	 * Return number of bytes sent or 0 if error, beware of potential infinite loop when
	 * returning 0
	 */
	if (ret <= 0) {
#if !defined(CONFIG_LOG_PRINTK)
		printk("RPMSG log backend: message send failed, err %d\n", ret);
#endif
		ret = 0;
	}

	return ret;
}

/**
 * @brief Process log message
 * Main entry point for processing log messages
 *
 * @param backend Pointer to log backend instance
 * @param msg     Log message to process
 */
static void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	const struct log_backend_rpmsg_ctx *ctx = backend->cb->ctx;
	struct log_backend_rpmsg_data *data = ctx->data;
	uint32_t flags = log_backend_std_get_flags();
	log_format_func_t log_output_func;

	/* Get the appropriate formatting function */
	log_output_func = log_format_func_t_get(data->log_format_current);

	/* Format and output the message */
	if (log_output_func != NULL) {
		log_output_func(ctx->output, &msg->log, flags);
	}
}

/**
 * @brief Panic handler
 * Called when system enters panic mode - should flush/output immediately
 *
 * @param backend Pointer to log backend instance
 */
static void panic(const struct log_backend *const backend)
{
	const struct log_backend_rpmsg_ctx *ctx = backend->cb->ctx;

	/* Call standard panic handler */
	log_backend_std_panic(ctx->output);
}

/**
 * @brief Backend initialization
 * Called during log subsystem initialization
 *
 * @param backend Pointer to log backend instance
 */
static void log_backend_rpmsg_init(const struct log_backend *const backend)
{
	struct log_backend_rpmsg_ctx *ctx = backend->cb->ctx;

	/* Set up the output context */
	log_output_ctx_set(ctx->output, (void *)ctx);
}

/**
 * @brief Set log format
 * Called to change the log output format
 *
 * @param backend  Pointer to log backend instance
 * @param log_type New log format type
 * @return         0 on success, negative error code on failure
 */
static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	const struct log_backend_rpmsg_ctx *ctx = backend->cb->ctx;
	struct log_backend_rpmsg_data *data = ctx->data;

	data->log_format_current = log_type;

	return 0;
}

/**
 * @brief Dropped messages handler
 * Called when messages are dropped due to backend being busy
 *
 * @param backend Pointer to log backend instance
 * @param cnt     Number of dropped messages
 */
static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	const struct log_backend_rpmsg_ctx *ctx = backend->cb->ctx;

	/* Use standard dropped message handler */
	log_backend_std_dropped(ctx->output, cnt);
}

/**
 * @brief Log backend API structure
 * Defines the interface functions for this backend
 */
const struct log_backend_api log_backend_rpmsg_api = {
	.process = process,
	.panic = panic,
	.init = log_backend_rpmsg_init,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : &dropped,
	.format_set = format_set,
};

/* Backend data instance */
static struct log_backend_rpmsg_data rpmsg_data = {
	.log_format_current = CONFIG_LOG_BACKEND_RPMSG_OUTPUT_DEFAULT,
};

/* Output buffer for log formatting */
static uint8_t rpmsg_buffer[CONFIG_LOG_BACKEND_RPMSG_BUFFER_SIZE];

/* Log output instance */
LOG_OUTPUT_DEFINE(rpmsg_output, char_out, rpmsg_buffer, sizeof(rpmsg_buffer));

/* Backend context instance */
static const struct log_backend_rpmsg_ctx rpmsg_ctx = {
	.output = &rpmsg_output,
	.data = &rpmsg_data,
};

/* Log backend instance - NOT autostarted */
LOG_BACKEND_DEFINE(log_backend_rpmsg, log_backend_rpmsg_api, false, (void *)&rpmsg_ctx);

static int log_backend_rpmsg_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src,
				void *priv)
{
	struct log_backend const *const backend = &log_backend_rpmsg;

	ARG_UNUSED(ept);
	ARG_UNUSED(data);
	ARG_UNUSED(len);
	ARG_UNUSED(src);
	ARG_UNUSED(priv);

	if (!log_backend_is_active(backend)) {
		log_backend_enable(backend, backend->cb->ctx, CONFIG_LOG_MAX_LEVEL);
	}

	return RPMSG_SUCCESS;
}

int log_backend_rpmsg_init_transport(struct rpmsg_device *rpmsg_dev)
{
	struct log_backend const *const backend = &log_backend_rpmsg;
	int ret;

	ret = rpmsg_create_ept(&rpmsg_data.ept, rpmsg_dev, CONFIG_LOG_BACKEND_RPMSG_SERVICE_NAME,
			       CONFIG_LOG_BACKEND_RPMSG_SRC_ADDR, CONFIG_LOG_BACKEND_RPMSG_DST_ADDR,
			       log_backend_rpmsg_cb, NULL);
	if (ret < 0) {
		return ret;
	}

	if (!log_backend_is_active(backend)) {
		log_backend_init(backend);
	}

	return 0;
}

void log_backend_rpmsg_deinit_transport(void)
{
	struct log_backend const *const backend = &log_backend_rpmsg;

	if (log_backend_is_active(backend)) {
		log_backend_disable(backend);
	}

	rpmsg_destroy_ept(&rpmsg_data.ept);
}
