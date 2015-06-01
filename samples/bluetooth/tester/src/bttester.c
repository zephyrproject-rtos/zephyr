/* bttester.c - Bluetooth Tester */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <toolchain.h>
#include <misc/printk.h>
#include <nanokernel.h>
#include <misc/byteorder.h>
#include <simple/uart.h>

#include "bttester.h"

#define STACKSIZE 2048
static char stack[STACKSIZE];

#define CMD_QUEUED 2
static uint8_t cmd_buf[CMD_QUEUED * IPC_MTU];

static struct nano_fifo cmds_queue;
static struct nano_fifo avail_queue;

static uint8_t register_service(uint8_t *data, uint16_t len)
{
	struct cmd_register_service *cmd = (void *) data;

	switch (cmd->id) {
	case SERVICE_ID_GAP:
		return tester_init_gap();
	default:
		return STATUS_FAILED;
	}
}

static void handle_core(uint8_t opcode, uint8_t *data, uint16_t len)
{
	uint8_t status;

	switch (opcode) {
	case OP_REGISTER_SERVICE:
		status = register_service(data, len);
		break;
	default:
		status = STATUS_FAILED;
		break;
	}

	tester_rsp(SERVICE_ID_CORE, opcode, status);
}

static void cmd_handler(int arg1, int arg2)
{
	while (1) {
		struct ipc_hdr *cmd;
		uint16_t len;

		cmd = nano_fiber_fifo_get_wait(&cmds_queue);

		len = sys_le16_to_cpu(cmd->len);

		switch (cmd->service) {
		case SERVICE_ID_CORE:
			handle_core(cmd->opcode, cmd->data, len);
			break;
		case SERVICE_ID_GAP:
			tester_handle_gap(cmd->opcode, cmd->data, len);
			break;
		default:
			tester_rsp(cmd->service, cmd->opcode, STATUS_FAILED);
			break;
		}

		nano_fiber_fifo_put(&avail_queue, cmd);
	}
}

static uint8_t *recv_cb(uint8_t *buf, size_t *off)
{
	struct ipc_hdr *cmd = (void *) buf;
	uint8_t *new_buf;
	uint16_t len;

	if (*off < sizeof(*cmd)) {
		return buf;
	}

	len = sys_le16_to_cpu(cmd->len);
	if (len > IPC_MTU - sizeof(*cmd)) {
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
		nano_fifo_put(&avail_queue, &cmd_buf[i * IPC_MTU]);
	}

	task_fiber_start(stack, STACKSIZE, cmd_handler, 0, 0, 7, 0);

	uart_simple_register(nano_fifo_get(&avail_queue), IPC_MTU, recv_cb);

	printk("BT tester initialized\n");
}

static void tester_send(uint8_t service, uint8_t opcode, uint8_t *data,
			size_t len)
{
	struct ipc_hdr msg;

	msg.service = service;
	msg.opcode = opcode;
	msg.len = len;

	uart_simple_send((uint8_t *)&msg, sizeof(msg));
	if (data && len) {
		uart_simple_send(data, len);
	}
}

void tester_rsp(uint8_t service, uint8_t opcode, uint8_t status)
{
	struct ipc_status s;

	if (status == STATUS_SUCCESS) {
		tester_send(service, opcode, NULL, 0);
		return;
	}

	s.code = status;
	tester_send(service, OP_STATUS, (uint8_t *) &s, sizeof(s));
}
