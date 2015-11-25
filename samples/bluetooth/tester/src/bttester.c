/* bttester.c - Bluetooth Tester */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <toolchain.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <console/uart_pipe.h>

#include "bttester.h"

#define STACKSIZE 2048
static char stack[STACKSIZE];

#define CMD_QUEUED 2
static uint8_t cmd_buf[CMD_QUEUED * BTP_MTU];

static struct nano_fifo cmds_queue;
static struct nano_fifo avail_queue;

static void supported_commands(uint8_t *data, uint16_t len)
{
	uint8_t buf[1];
	struct core_read_supported_commands_rp *rp = (void *) buf;

	buf[0] = 1 << CORE_READ_SUPPORTED_COMMANDS;
	buf[0] |= 1 << CORE_READ_SUPPORTED_SERVICES;
	buf[0] |= 1 << CORE_REGISTER_SERVICE;

	tester_rsp_full(BTP_SERVICE_ID_CORE, CORE_READ_SUPPORTED_COMMANDS,
			BTP_INDEX_NONE, (uint8_t *) rp, sizeof(buf));
}

static void supported_services(uint8_t *data, uint16_t len)
{
	uint8_t buf[1];
	struct core_read_supported_services_rp *rp = (void *) buf;

	buf[0] = 1 << BTP_SERVICE_ID_CORE;
	buf[0] |= 1 << BTP_SERVICE_ID_GAP;
	buf[0] |= 1 << BTP_SERVICE_ID_GATT;

	tester_rsp_full(BTP_SERVICE_ID_CORE, CORE_READ_SUPPORTED_SERVICES,
			BTP_INDEX_NONE, (uint8_t *) rp, sizeof(buf));
}

static void register_service(uint8_t *data, uint16_t len)
{
	struct core_register_service_cmd *cmd = (void *) data;
	uint8_t status;

	switch (cmd->id) {
	case BTP_SERVICE_ID_GAP:
		status = tester_init_gap();
		break;
	case BTP_SERVICE_ID_GATT:
		status = tester_init_gatt();
		break;
	default:
		status = BTP_STATUS_FAILED;
		break;
	}

	tester_rsp(BTP_SERVICE_ID_CORE, CORE_REGISTER_SERVICE, BTP_INDEX_NONE,
		   status);
}

static void handle_core(uint8_t opcode, uint8_t index, uint8_t *data,
			uint16_t len)
{
	if (index != BTP_INDEX_NONE) {
		tester_rsp(BTP_SERVICE_ID_CORE, opcode, index, BTP_STATUS_FAILED);
		return;
	}

	switch (opcode) {
	case CORE_READ_SUPPORTED_COMMANDS:
		supported_commands(data, len);
		return;
	case CORE_READ_SUPPORTED_SERVICES:
		supported_services(data, len);
		return;
	case CORE_REGISTER_SERVICE:
		register_service(data, len);
		return;
	default:
		tester_rsp(BTP_SERVICE_ID_CORE, opcode, BTP_INDEX_NONE,
			   BTP_STATUS_UNKNOWN_CMD);
		return;
	}
}

static void cmd_handler(int arg1, int arg2)
{
	while (1) {
		struct btp_hdr *cmd;
		uint16_t len;

		cmd = nano_fiber_fifo_get_wait(&cmds_queue);

		len = sys_le16_to_cpu(cmd->len);

		/* TODO
		 * verify if service is registered before calling handler
		 */

		switch (cmd->service) {
		case BTP_SERVICE_ID_CORE:
			handle_core(cmd->opcode, cmd->index, cmd->data, len);
			break;
		case BTP_SERVICE_ID_GAP:
			tester_handle_gap(cmd->opcode, cmd->index, cmd->data,
					  len);
			break;
		case BTP_SERVICE_ID_GATT:
			tester_handle_gatt(cmd->opcode, cmd->index, cmd->data,
					    len);
			break;
		default:
			tester_rsp(cmd->service, cmd->opcode, cmd->index,
				   BTP_STATUS_FAILED);
			break;
		}

		nano_fiber_fifo_put(&avail_queue, cmd);
	}
}

static uint8_t *recv_cb(uint8_t *buf, size_t *off)
{
	struct btp_hdr *cmd = (void *) buf;
	uint8_t *new_buf;
	uint16_t len;

	if (*off < sizeof(*cmd)) {
		return buf;
	}

	len = sys_le16_to_cpu(cmd->len);
	if (len > BTP_MTU - sizeof(*cmd)) {
		printk("BT tester: invalid packet length\n");
		*off = 0;
		return buf;
	}

	if (*off < sizeof(*cmd) + len) {
		return buf;
	}

	new_buf =  nano_fifo_get(&avail_queue);
	if (!new_buf) {
		printk("BT tester: RX overflow\n");
		*off = 0;
		return buf;
	}

	nano_fifo_put(&cmds_queue, buf);

	*off = 0;
	return new_buf;
}

void tester_init(void)
{
	int i;

	nano_fifo_init(&cmds_queue);
	nano_fifo_init(&avail_queue);

	for (i = 0; i < CMD_QUEUED; i++) {
		nano_fifo_put(&avail_queue, &cmd_buf[i * BTP_MTU]);
	}

	task_fiber_start(stack, STACKSIZE, cmd_handler, 0, 0, 7, 0);

	uart_pipe_register(nano_fifo_get(&avail_queue), BTP_MTU, recv_cb);

	printk("BT tester initialized\n");
}

static void tester_send(uint8_t service, uint8_t opcode, uint8_t index,
			uint8_t *data, size_t len)
{
	struct btp_hdr msg;

	msg.service = service;
	msg.opcode = opcode;
	msg.index = index;
	msg.len = len;

	uart_pipe_send((uint8_t *)&msg, sizeof(msg));
	if (data && len) {
		uart_pipe_send(data, len);
	}
}

void tester_rsp(uint8_t service, uint8_t opcode, uint8_t index, uint8_t status)
{
	struct btp_status s;

	if (status == BTP_STATUS_SUCCESS) {
		tester_send(service, opcode, index, NULL, 0);
		return;
	}

	s.code = status;
	tester_send(service, BTP_STATUS, index, (uint8_t *) &s, sizeof(s));
}

void tester_rsp_full(uint8_t service, uint8_t opcode, uint8_t index,
		     uint8_t *data, size_t len)
{
	tester_send(service, opcode, index, data, len);
}
