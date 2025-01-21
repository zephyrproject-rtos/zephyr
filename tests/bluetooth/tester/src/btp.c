/* bttester.c - Bluetooth Tester */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/toolchain.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/uart_pipe.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp/btp.h"

#define STACKSIZE 2048
static K_THREAD_STACK_DEFINE(stack, STACKSIZE);
static struct k_thread cmd_thread;

#define CMD_QUEUED 2
struct btp_buf {
	intptr_t _reserved;
	uint8_t data[BTP_MTU]; /* includes btp header */
	uint8_t rsp[BTP_DATA_MAX_SIZE];
};

static struct btp_buf cmd_buf[CMD_QUEUED];

static K_FIFO_DEFINE(cmds_queue);
static K_FIFO_DEFINE(avail_queue);

static struct btp_buf *delayed_cmd;

static struct {
	const struct btp_handler *handlers;
	size_t num;
} service_handler[BTP_SERVICE_ID_MAX + 1];

static struct net_buf_simple *rsp_buf = NET_BUF_SIMPLE(BTP_MTU);
static K_MUTEX_DEFINE(rsp_buf_mutex);

static void tester_send_with_index(uint8_t service, uint8_t opcode, uint8_t index,
				   const uint8_t *data, size_t len);
static void tester_rsp_with_index(uint8_t service, uint8_t opcode, uint8_t index,
				  uint8_t status);

void tester_register_command_handlers(uint8_t service,
				      const struct btp_handler *handlers,
				      size_t num)
{
	__ASSERT_NO_MSG(service <= BTP_SERVICE_ID_MAX);
	__ASSERT_NO_MSG(service_handler[service].handlers == NULL);

	service_handler[service].handlers = handlers;
	service_handler[service].num = num;
}

static const struct btp_handler *find_btp_handler(uint8_t service, uint8_t opcode)
{
	if ((service > BTP_SERVICE_ID_MAX) ||
	    (service_handler[service].handlers == NULL)) {
		return NULL;
	}

	for (uint8_t i = 0; i < service_handler[service].num; i++) {
		if (service_handler[service].handlers[i].opcode == opcode) {
			return &service_handler[service].handlers[i];
		}
	}

	return NULL;
}

static void cmd_handler(void *p1, void *p2, void *p3)
{
	while (1) {
		const struct btp_handler *btp;
		struct btp_buf *cmd;
		struct btp_hdr *hdr;
		uint8_t status;
		uint16_t rsp_len = 0;
		uint16_t len;

		cmd = k_fifo_get(&cmds_queue, K_FOREVER);
		hdr = (struct btp_hdr *)cmd->data;

		LOG_DBG("cmd service 0x%02x opcode 0x%02x index 0x%02x",
			hdr->service, hdr->opcode, hdr->index);

		len = sys_le16_to_cpu(hdr->len);

		btp = find_btp_handler(hdr->service, hdr->opcode);
		if (btp) {
			if (btp->index != hdr->index) {
				status = BTP_STATUS_FAILED;
			} else if ((btp->expect_len >= 0) && (btp->expect_len != len)) {
				status = BTP_STATUS_FAILED;
			} else {
				status = btp->func(hdr->data, len,
						   cmd->rsp, &rsp_len);
			}

			__ASSERT_NO_MSG((rsp_len + sizeof(struct btp_hdr)) <= BTP_MTU);
		} else {
			status = BTP_STATUS_UNKNOWN_CMD;
		}
		/* Allow to delay only 1 command. This is for convenience only
		 * of using cmd data without need of copying those in async
		 * functions. Should be not needed eventually.
		 */
		if (status == BTP_STATUS_DELAY_REPLY) {
			__ASSERT_NO_MSG(delayed_cmd == NULL);
			delayed_cmd = cmd;
			continue;
		}

		if ((status == BTP_STATUS_SUCCESS) && rsp_len > 0) {
			tester_send_with_index(hdr->service, hdr->opcode,
					       hdr->index, cmd->rsp, rsp_len);
		} else {
			tester_rsp_with_index(hdr->service, hdr->opcode,
					      hdr->index, status);
		}

		(void)memset(cmd, 0, sizeof(*cmd));
		k_fifo_put(&avail_queue, cmd);
	}
}

static uint8_t *recv_cb(uint8_t *buf, size_t *off)
{
	struct btp_hdr *cmd = (void *) buf;
	struct btp_buf *new_buf;
	uint16_t len;

	if (*off < sizeof(*cmd)) {
		return buf;
	}

	len = sys_le16_to_cpu(cmd->len);
	if (len > BTP_MTU - sizeof(*cmd)) {
		LOG_ERR("BT tester: invalid packet length");
		*off = 0;
		return buf;
	}

	if (*off < sizeof(*cmd) + len) {
		return buf;
	}

	new_buf =  k_fifo_get(&avail_queue, K_NO_WAIT);
	if (!new_buf) {
		LOG_ERR("BT tester: RX overflow");
		*off = 0;
		return buf;
	}

	k_fifo_put(&cmds_queue, CONTAINER_OF(buf, struct btp_buf, data[0]));

	*off = 0;
	return new_buf->data;
}

#if defined(CONFIG_UART_PIPE)
/* Uart Pipe */
static void uart_init(uint8_t *data)
{
	uart_pipe_register(data, BTP_MTU, recv_cb);
}

static void uart_send(const uint8_t *data, size_t len)
{
	uart_pipe_send(data, len);
}
#else /* !CONFIG_UART_PIPE */
static uint8_t *recv_buf;
static size_t recv_off;
static const struct device *const dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static void timer_expiry_cb(struct k_timer *timer)
{
	uint8_t c;

	while (uart_poll_in(dev, &c) == 0) {
		recv_buf[recv_off++] = c;
		recv_buf = recv_cb(recv_buf, &recv_off);
	}
}

K_TIMER_DEFINE(timer, timer_expiry_cb, NULL);

/* Uart Poll */
static void uart_init(uint8_t *data)
{
	__ASSERT_NO_MSG(device_is_ready(dev));

	recv_buf = data;

	k_timer_start(&timer, K_MSEC(10), K_MSEC(10));
}

static void uart_send(const uint8_t *data, size_t len)
{
	int i;

	for (i = 0; i < len; i++) {
		uart_poll_out(dev, data[i]);
	}
}
#endif /* CONFIG_UART_PIPE */

void tester_init(void)
{
	int i;
	struct btp_buf *buf;

	LOG_DBG("Initializing tester");

	for (i = 0; i < CMD_QUEUED; i++) {
		k_fifo_put(&avail_queue, &cmd_buf[i]);
	}

	k_thread_create(&cmd_thread, stack, STACKSIZE, cmd_handler,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	buf = k_fifo_get(&avail_queue, K_NO_WAIT);

	uart_init(buf->data);

	/* core service is always available */
	tester_init_core();

	tester_send_with_index(BTP_SERVICE_ID_CORE, BTP_CORE_EV_IUT_READY,
			      BTP_INDEX_NONE, NULL, 0);
}

int tester_rsp_buffer_lock(void)
{
	if (k_mutex_lock(&rsp_buf_mutex, Z_FOREVER) != 0) {
		LOG_ERR("Cannot lock rsp_ring_buf");

		return -EACCES;
	}

	return 0;
}

void tester_rsp_buffer_unlock(void)
{
	k_mutex_unlock(&rsp_buf_mutex);
}

void tester_rsp_buffer_free(void)
{
	net_buf_simple_init(rsp_buf, 0);
}

void tester_rsp_buffer_allocate(size_t len, uint8_t **data)
{
	tester_rsp_buffer_free();

	*data = net_buf_simple_add(rsp_buf, len);
}

static void tester_send_with_index(uint8_t service, uint8_t opcode, uint8_t index,
				   const uint8_t *data, size_t len)
{
	struct btp_hdr msg;

	msg.service = service;
	msg.opcode = opcode;
	msg.index = index;
	msg.len = sys_cpu_to_le16(len);

	uart_send((uint8_t *)&msg, sizeof(msg));
	if (data && len) {
		uart_send(data, len);
	}
}

static void tester_rsp_with_index(uint8_t service, uint8_t opcode, uint8_t index,
				  uint8_t status)
{
	struct btp_status s;

	LOG_DBG("service 0x%02x opcode 0x%02x index 0x%02x status 0x%02x", service, opcode, index,
		status);

	if (status == BTP_STATUS_SUCCESS) {
		tester_send_with_index(service, opcode, index, NULL, 0);
		return;
	}

	s.code = status;
	tester_send_with_index(service, BTP_STATUS, index, (uint8_t *) &s, sizeof(s));
}

void tester_event(uint8_t service, uint8_t opcode, const void *data, size_t len)
{
	__ASSERT_NO_MSG(opcode >= 0x80);

	LOG_DBG("service 0x%02x opcode 0x%02x", service, opcode);

	tester_send_with_index(service, opcode, BTP_INDEX, data, len);
}

void tester_rsp_full(uint8_t service, uint8_t opcode, const void *rsp, size_t len)
{
	struct btp_buf *cmd;

	__ASSERT_NO_MSG(opcode < 0x80);
	__ASSERT_NO_MSG(delayed_cmd != NULL);

	LOG_DBG("service 0x%02x opcode 0x%02x", service, opcode);

	tester_send_with_index(service, opcode, BTP_INDEX, rsp, len);

	cmd = delayed_cmd;
	delayed_cmd = NULL;

	(void)memset(cmd, 0, sizeof(*cmd));
	k_fifo_put(&avail_queue, cmd);
}

void tester_rsp(uint8_t service, uint8_t opcode, uint8_t status)
{
	struct btp_buf *cmd;

	__ASSERT_NO_MSG(opcode < 0x80);
	__ASSERT_NO_MSG(delayed_cmd != NULL);

	LOG_DBG("service 0x%02x opcode 0x%02x status 0x%02x", service, opcode, status);

	tester_rsp_with_index(service, opcode, BTP_INDEX, status);

	cmd = delayed_cmd;
	delayed_cmd = NULL;

	(void)memset(cmd, 0, sizeof(*cmd));
	k_fifo_put(&avail_queue, cmd);
}
