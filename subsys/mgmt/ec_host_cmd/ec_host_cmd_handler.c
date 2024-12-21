/*
 * Copyright (c) 2020 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/ec_host_cmd/ec_host_cmd.h>
#include <zephyr/mgmt/ec_host_cmd/backend.h>
#include <zephyr/sys/iterable_sections.h>
#include <stdio.h>
#include <string.h>

LOG_MODULE_REGISTER(host_cmd_handler, CONFIG_EC_HC_LOG_LEVEL);

#ifdef CONFIG_EC_HOST_CMD_INITIALIZE_AT_BOOT
#define EC_HOST_CMD_CHOSEN_BACKEND_LIST                                                            \
	zephyr_host_cmd_espi_backend, zephyr_host_cmd_shi_backend, zephyr_host_cmd_uart_backend,   \
		zephyr_host_cmd_spi_backend

#define EC_HOST_CMD_ADD_CHOSEN(chosen) COND_CODE_1(DT_NODE_EXISTS(DT_CHOSEN(chosen)), (1), (0))

#define NUMBER_OF_CHOSEN_BACKENDS                                                                  \
	FOR_EACH(EC_HOST_CMD_ADD_CHOSEN, (+), EC_HOST_CMD_CHOSEN_BACKEND_LIST)                     \
	+0

BUILD_ASSERT(NUMBER_OF_CHOSEN_BACKENDS < 2, "Number of chosen backends > 1");
#endif

#define RX_HEADER_SIZE (sizeof(struct ec_host_cmd_request_header))
#define TX_HEADER_SIZE (sizeof(struct ec_host_cmd_response_header))

#ifdef CONFIG_EC_HOST_CMD_NOCACHE_BUFFERS
#define BUFFERS_CACHE_ATTR __nocache
#else
#define BUFFERS_CACHE_ATTR
#endif

COND_CODE_1(CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_DEF,
	    (static uint8_t hc_rx_buffer[CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_SIZE] __aligned(4)
	    BUFFERS_CACHE_ATTR;), ())
COND_CODE_1(CONFIG_EC_HOST_CMD_HANDLER_TX_BUFFER_DEF,
	    (static uint8_t hc_tx_buffer[CONFIG_EC_HOST_CMD_HANDLER_TX_BUFFER_SIZE] __aligned(4)
	    BUFFERS_CACHE_ATTR;), ())

#ifdef CONFIG_EC_HOST_CMD_DEDICATED_THREAD
static K_KERNEL_STACK_DEFINE(hc_stack, CONFIG_EC_HOST_CMD_HANDLER_STACK_SIZE);
#endif /* CONFIG_EC_HOST_CMD_DEDICATED_THREAD */

static struct ec_host_cmd ec_host_cmd = {
	.rx_ctx = {
			.buf = COND_CODE_1(CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_DEF, (hc_rx_buffer),
					   (NULL)),
			.len_max = COND_CODE_1(CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_DEF,
					       (CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_SIZE), (0)),
		},
	.tx = {
			.buf = COND_CODE_1(CONFIG_EC_HOST_CMD_HANDLER_TX_BUFFER_DEF, (hc_tx_buffer),
					   (NULL)),
			.len_max = COND_CODE_1(CONFIG_EC_HOST_CMD_HANDLER_TX_BUFFER_DEF,
					       (CONFIG_EC_HOST_CMD_HANDLER_TX_BUFFER_SIZE), (0)),
		},
};

#ifdef CONFIG_EC_HOST_CMD_IN_PROGRESS_STATUS
/* Indicates that a command has sent EC_HOST_CMD_IN_PROGRESS but hasn't sent a final status */
static bool cmd_in_progress;

/* The final result of the last command that has sent EC_HOST_CMD_IN_PROGRESS */
static enum ec_host_cmd_status saved_status = EC_HOST_CMD_UNAVAILABLE;
static struct k_work work_in_progress;
ec_host_cmd_in_progress_cb_t cb_in_progress;
static void *user_data_in_progress;
#endif /* CONFIG_EC_HOST_CMD_IN_PROGRESS_STATUS */

#ifdef CONFIG_EC_HOST_CMD_LOG_SUPPRESSED
static uint16_t suppressed_cmds[CONFIG_EC_HOST_CMD_LOG_SUPPRESSED_NUMBER];
static uint16_t suppressed_cmds_count[CONFIG_EC_HOST_CMD_LOG_SUPPRESSED_NUMBER];
static int64_t suppressed_cmds_deadline = CONFIG_EC_HOST_CMD_LOG_SUPPRESSED_INTERVAL_SECS * 1000U;
static size_t suppressed_cmds_number;
#endif /* CONFIG_EC_HOST_CMD_LOG_SUPPRESSED */

static uint8_t cal_checksum(const uint8_t *const buffer, const uint16_t size)
{
	uint8_t checksum = 0;

	for (size_t i = 0; i < size; ++i) {
		checksum += buffer[i];
	}
	return (uint8_t)(-checksum);
}

#ifdef CONFIG_EC_HOST_CMD_IN_PROGRESS_STATUS
bool ec_host_cmd_send_in_progress_ended(void)
{
	return !cmd_in_progress;
}

enum ec_host_cmd_status ec_host_cmd_send_in_progress_status(void)
{
	enum ec_host_cmd_status ret = saved_status;

	saved_status = EC_HOST_CMD_UNAVAILABLE;

	return ret;
}

enum ec_host_cmd_status ec_host_cmd_send_in_progress_continue(ec_host_cmd_in_progress_cb_t cb,
							      void *user_data)
{
	if (cmd_in_progress) {
		return EC_HOST_CMD_BUSY;
	}

	cmd_in_progress = true;
	cb_in_progress = cb;
	user_data_in_progress = user_data;
	saved_status = EC_HOST_CMD_UNAVAILABLE;
	LOG_INF("HC pending");
	k_work_submit(&work_in_progress);

	return EC_HOST_CMD_SUCCESS;
}

static void handler_in_progress(struct k_work *work)
{
	if (cb_in_progress != NULL) {
		saved_status = cb_in_progress(user_data_in_progress);
		LOG_INF("HC pending done, result=%d", saved_status);
	} else {
		saved_status = EC_HOST_CMD_UNAVAILABLE;
		LOG_ERR("HC incorrect IN_PROGRESS callback");
	}
	cb_in_progress = NULL;
	cmd_in_progress = false;
}
#endif /* CONFIG_EC_HOST_CMD_IN_PROGRESS_STATUS */

#ifdef CONFIG_EC_HOST_CMD_LOG_SUPPRESSED
int ec_host_cmd_add_suppressed(uint16_t cmd_id)
{
	if (suppressed_cmds_number >= CONFIG_EC_HOST_CMD_LOG_SUPPRESSED_NUMBER) {
		return -EIO;
	}

	suppressed_cmds[suppressed_cmds_number] = cmd_id;
	++suppressed_cmds_number;

	return 0;
}

static bool ec_host_cmd_is_suppressed(uint16_t cmd_id)
{
	int i;

	for (i = 0; i < suppressed_cmds_number; i++) {
		if (suppressed_cmds[i] == cmd_id) {
			suppressed_cmds_count[i]++;

			return true;
		}
	}

	return false;
}

void ec_host_cmd_dump_suppressed(void)
{
	int i;
	int64_t uptime = k_uptime_get();

	LOG_PRINTK("[%llds HC Suppressed:", uptime / 1000U);
	for (i = 0; i < suppressed_cmds_number; i++) {
		LOG_PRINTK(" 0x%x=%d", suppressed_cmds[i], suppressed_cmds_count[i]);
		suppressed_cmds_count[i] = 0;
	}
	LOG_PRINTK("]\n");

	/* Reset the timer */
	suppressed_cmds_deadline = uptime + CONFIG_EC_HOST_CMD_LOG_SUPPRESSED_INTERVAL_SECS * 1000U;
}

static void ec_host_cmd_check_suppressed(void)
{
	if (k_uptime_get() >= suppressed_cmds_deadline) {
		ec_host_cmd_dump_suppressed();
	}
}
#endif /* CONFIG_EC_HOST_CMD_LOG_SUPPRESSED */

static void send_status_response(const struct ec_host_cmd_backend *backend,
				 struct ec_host_cmd_tx_buf *tx,
				 const enum ec_host_cmd_status status)
{
	struct ec_host_cmd_response_header *const tx_header = (void *)tx->buf;

	tx_header->prtcl_ver = 3;
	tx_header->result = status;
	tx_header->data_len = 0;
	tx_header->reserved = 0;
	tx_header->checksum = 0;
	tx_header->checksum = cal_checksum((uint8_t *)tx_header, TX_HEADER_SIZE);

	tx->len = TX_HEADER_SIZE;

	backend->api->send(backend);
}

static enum ec_host_cmd_status verify_rx(struct ec_host_cmd_rx_ctx *rx)
{
	/* rx buf and len now have valid incoming data */
	if (rx->len < RX_HEADER_SIZE) {
		return EC_HOST_CMD_REQUEST_TRUNCATED;
	}

	const struct ec_host_cmd_request_header *rx_header =
		(struct ec_host_cmd_request_header *)rx->buf;

	/* Only support version 3 */
	if (rx_header->prtcl_ver != 3) {
		return EC_HOST_CMD_INVALID_HEADER;
	}

	const uint16_t rx_valid_data_size = rx_header->data_len + RX_HEADER_SIZE;
	/*
	 * Ensure we received at least as much data as is expected.
	 * It is okay to receive more since some hardware interfaces
	 * add on extra padding bytes at the end.
	 */
	if (rx->len < rx_valid_data_size) {
		return EC_HOST_CMD_REQUEST_TRUNCATED;
	}

	/* Validate checksum */
	if (cal_checksum((uint8_t *)rx_header, rx_valid_data_size) != 0) {
		return EC_HOST_CMD_INVALID_CHECKSUM;
	}

	return EC_HOST_CMD_SUCCESS;
}

static enum ec_host_cmd_status validate_handler(const struct ec_host_cmd_handler *handler,
						const struct ec_host_cmd_handler_args *args)
{
	if (handler->min_rqt_size > args->input_buf_size) {
		return EC_HOST_CMD_REQUEST_TRUNCATED;
	}

	if (handler->min_rsp_size > args->output_buf_max) {
		return EC_HOST_CMD_INVALID_RESPONSE;
	}

	if (args->version >= NUM_BITS(handler->version_mask) ||
	    !(handler->version_mask & BIT(args->version))) {
		return EC_HOST_CMD_INVALID_VERSION;
	}

	return EC_HOST_CMD_SUCCESS;
}

static enum ec_host_cmd_status prepare_response(struct ec_host_cmd_tx_buf *tx, uint16_t len)
{
	struct ec_host_cmd_response_header *const tx_header = (void *)tx->buf;

	tx_header->prtcl_ver = 3;
	tx_header->result = EC_HOST_CMD_SUCCESS;
	tx_header->data_len = len;
	tx_header->reserved = 0;

	const uint16_t tx_valid_data_size = tx_header->data_len + TX_HEADER_SIZE;

	if (tx_valid_data_size > tx->len_max) {
		return EC_HOST_CMD_INVALID_RESPONSE;
	}

	/* Calculate checksum */
	tx_header->checksum = 0;
	tx_header->checksum = cal_checksum(tx->buf, tx_valid_data_size);

	tx->len = tx_valid_data_size;

	return EC_HOST_CMD_SUCCESS;
}

void ec_host_cmd_set_user_cb(ec_host_cmd_user_cb_t cb, void *user_data)
{
	struct ec_host_cmd *hc = &ec_host_cmd;

	hc->user_cb = cb;
	hc->user_data = user_data;
}

int ec_host_cmd_send_response(enum ec_host_cmd_status status,
			      const struct ec_host_cmd_handler_args *args)
{
	struct ec_host_cmd *hc = &ec_host_cmd;
	struct ec_host_cmd_tx_buf *tx = &hc->tx;

	if (hc->state != EC_HOST_CMD_STATE_PROCESSING) {
		LOG_ERR("Unexpected state while sending");
		return -ENOTSUP;
	}
	hc->state = EC_HOST_CMD_STATE_SENDING;

	if (status != EC_HOST_CMD_SUCCESS) {
		const struct ec_host_cmd_request_header *const rx_header =
			(const struct ec_host_cmd_request_header *const)hc->rx_ctx.buf;

		LOG_INF("HC 0x%04x err %d", rx_header->cmd_id, status);
		send_status_response(hc->backend, tx, status);
		return status;
	}

#ifdef CONFIG_EC_HOST_CMD_LOG_DBG_BUFFERS
	if (args->output_buf_size) {
		LOG_HEXDUMP_DBG(args->output_buf, args->output_buf_size, "HC resp:");
	}
#endif

	status = prepare_response(tx, args->output_buf_size);
	if (status != EC_HOST_CMD_SUCCESS) {
		send_status_response(hc->backend, tx, status);
		return status;
	}

	return hc->backend->api->send(hc->backend);
}

void ec_host_cmd_rx_notify(void)
{
	struct ec_host_cmd *hc = &ec_host_cmd;
	struct ec_host_cmd_rx_ctx *rx = &hc->rx_ctx;

	hc->rx_status = verify_rx(rx);

	if (!hc->rx_status && hc->user_cb) {
		hc->user_cb(rx, hc->user_data);
	}

	k_sem_give(&hc->rx_ready);
}

static void ec_host_cmd_log_request(const uint8_t *rx_buf)
{
	static uint16_t prev_cmd;
	const struct ec_host_cmd_request_header *const rx_header =
		(const struct ec_host_cmd_request_header *const)rx_buf;

#ifdef CONFIG_EC_HOST_CMD_LOG_SUPPRESSED
	if (ec_host_cmd_is_suppressed(rx_header->cmd_id)) {
		ec_host_cmd_check_suppressed();

		return;
	}
#endif /* CONFIG_EC_HOST_CMD_LOG_SUPPRESSED */

	if (IS_ENABLED(CONFIG_EC_HOST_CMD_LOG_DBG_BUFFERS)) {
		if (rx_header->data_len) {
			const uint8_t *rx_data = rx_buf + RX_HEADER_SIZE;
			static const char dbg_fmt[] = "HC 0x%04x.%d:";
			/* Use sizeof because "%04x" needs 4 bytes for command id, and
			 * %d needs 2 bytes for version, so no additional buffer is required.
			 */
			char dbg_raw[sizeof(dbg_fmt)];

			snprintf(dbg_raw, sizeof(dbg_raw), dbg_fmt, rx_header->cmd_id,
				 rx_header->cmd_ver);
			LOG_HEXDUMP_DBG(rx_data, rx_header->data_len, dbg_raw);

			return;
		}
	}

	/* In normal output mode, skip printing repeats of the same command
	 * that occur in rapid succession - such as flash commands during
	 * software sync.
	 */
	if (rx_header->cmd_id != prev_cmd) {
		prev_cmd = rx_header->cmd_id;
		LOG_INF("HC 0x%04x", rx_header->cmd_id);
	} else {
		LOG_DBG("HC 0x%04x", rx_header->cmd_id);
	}
}

FUNC_NORETURN static void ec_host_cmd_thread(void *hc_handle, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	enum ec_host_cmd_status status;
	struct ec_host_cmd *hc = (struct ec_host_cmd *)hc_handle;
	struct ec_host_cmd_rx_ctx *rx = &hc->rx_ctx;
	struct ec_host_cmd_tx_buf *tx = &hc->tx;
	const struct ec_host_cmd_handler *found_handler;
	const struct ec_host_cmd_request_header *const rx_header = (void *)rx->buf;
	/* The pointer to rx buffer is constant during communication */
	struct ec_host_cmd_handler_args args = {
		.output_buf = (uint8_t *)tx->buf + TX_HEADER_SIZE,
		.input_buf = rx->buf + RX_HEADER_SIZE,
		.reserved = NULL,
	};

	__ASSERT(hc->state != EC_HOST_CMD_STATE_DISABLED, "HC backend not initialized");

	while (1) {
		hc->state = EC_HOST_CMD_STATE_RECEIVING;
		/* Wait until RX messages is received on host interface */
		k_sem_take(&hc->rx_ready, K_FOREVER);
		hc->state = EC_HOST_CMD_STATE_PROCESSING;

		ec_host_cmd_log_request(rx->buf);

		/* Check status of the rx data, that has been verified in
		 * ec_host_cmd_send_received.
		 */
		if (hc->rx_status != EC_HOST_CMD_SUCCESS) {
			ec_host_cmd_send_response(hc->rx_status, &args);
			continue;
		}

		found_handler = NULL;
		STRUCT_SECTION_FOREACH(ec_host_cmd_handler, handler) {
			if (handler->id == rx_header->cmd_id) {
				found_handler = handler;
				break;
			}
		}

		/* No handler in this image for requested command */
		if (found_handler == NULL) {
			ec_host_cmd_send_response(EC_HOST_CMD_INVALID_COMMAND, &args);
			continue;
		}

		args.command = rx_header->cmd_id;
		args.version = rx_header->cmd_ver;
		args.input_buf_size = rx_header->data_len;
		args.output_buf_max = tx->len_max - TX_HEADER_SIZE,
		args.output_buf_size = 0;

		status = validate_handler(found_handler, &args);
		if (status != EC_HOST_CMD_SUCCESS) {
			ec_host_cmd_send_response(status, &args);
			continue;
		}

		/*
		 * Pre-emptively clear the entire response buffer so we do not
		 * have any left over contents from previous host commands.
		 */
		memset(args.output_buf, 0, args.output_buf_max);

		status = found_handler->handler(&args);

		ec_host_cmd_send_response(status, &args);
	}
}

#ifndef CONFIG_EC_HOST_CMD_DEDICATED_THREAD
FUNC_NORETURN void ec_host_cmd_task(void)
{
	ec_host_cmd_thread(&ec_host_cmd, NULL, NULL);
}
#endif

int ec_host_cmd_init(struct ec_host_cmd_backend *backend)
{
	struct ec_host_cmd *hc = &ec_host_cmd;
	int ret;
	uint8_t *handler_tx_buf, *handler_rx_buf;
	uint8_t *handler_tx_buf_end, *handler_rx_buf_end;
	uint8_t *backend_tx_buf, *backend_rx_buf;

	hc->backend = backend;

	/* Allow writing to rx buff at startup */
	k_sem_init(&hc->rx_ready, 0, 1);

#ifdef CONFIG_EC_HOST_CMD_IN_PROGRESS_STATUS
	k_work_init(&work_in_progress, handler_in_progress);
#endif /* CONFIG_EC_HOST_CMD_IN_PROGRESS_STATUS */

	handler_tx_buf = hc->tx.buf;
	handler_rx_buf = hc->rx_ctx.buf;
	handler_tx_buf_end = handler_tx_buf + CONFIG_EC_HOST_CMD_HANDLER_TX_BUFFER_SIZE;
	handler_rx_buf_end = handler_rx_buf + CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_SIZE;

	ret = backend->api->init(backend, &hc->rx_ctx, &hc->tx);

	backend_tx_buf = hc->tx.buf;
	backend_rx_buf = hc->rx_ctx.buf;

	if (ret != 0) {
		return ret;
	}

	if (!backend_tx_buf || !backend_rx_buf) {
		LOG_ERR("No buffer for Host Command communication");
		return -EIO;
	}

	hc->state = EC_HOST_CMD_STATE_RECEIVING;

	/* Check if a backend uses provided buffers. The buffer pointers can be shifted within the
	 * buffer to make space for preamble. Make sure the rx/tx pointers are within the provided
	 * buffers ranges.
	 */
	if ((handler_tx_buf &&
	     !((handler_tx_buf <= backend_tx_buf) && (handler_tx_buf_end > backend_tx_buf))) ||
	    (handler_rx_buf &&
	     !((handler_rx_buf <= backend_rx_buf) && (handler_rx_buf_end > backend_rx_buf)))) {
		LOG_WRN("Host Command handler provided unused buffer");
	}

#ifdef CONFIG_EC_HOST_CMD_DEDICATED_THREAD
	k_thread_create(&hc->thread, hc_stack, CONFIG_EC_HOST_CMD_HANDLER_STACK_SIZE,
			ec_host_cmd_thread, (void *)hc, NULL, NULL, CONFIG_EC_HOST_CMD_HANDLER_PRIO,
			0, K_NO_WAIT);
	k_thread_name_set(&hc->thread, "ec_host_cmd");
#endif /* CONFIG_EC_HOST_CMD_DEDICATED_THREAD */

	return 0;
}

const struct ec_host_cmd *ec_host_cmd_get_hc(void)
{
	return &ec_host_cmd;
}
