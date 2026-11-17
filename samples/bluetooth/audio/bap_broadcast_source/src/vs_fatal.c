/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/fatal_types.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include "vs_fatal.h"

static const char *fatal_reason_str(uint32_t reason)
{
	switch (reason) {
	case K_ERR_CPU_EXCEPTION:
		return "CPU exception";
	case K_ERR_SPURIOUS_IRQ:
		return "spurious interrupt";
	case K_ERR_STACK_CHK_FAIL:
		return "stack overflow";
	case K_ERR_KERNEL_OOPS:
		return "kernel oops";
	case K_ERR_KERNEL_PANIC:
		return "kernel panic";
	default:
		return "unknown";
	}
}

static bool stack_frame_decode(struct net_buf_simple *buf)
{
	const struct bt_hci_vs_fatal_error_cpu_data_cortex_m *cpu;
	const struct bt_hci_vs_fatal_error_stack_frame *sf;
	uint32_t reason;

	if (buf->len != (sizeof(*sf) + sizeof(*cpu))) {
		return false;
	}

	sf = net_buf_simple_pull_mem(buf, sizeof(*sf));
	if (sf->cpu_type != BT_HCI_EVT_VS_ERROR_CPU_TYPE_CORTEX_M) {
		return false;
	}

	cpu = net_buf_simple_pull_mem(buf, sizeof(*cpu));
	reason = sys_le32_to_cpu(sf->reason);

	printk("Controller fatal error: reason %u (%s), Cortex-M stack frame:\n", reason,
	       fatal_reason_str(reason));
	printk("  r0/a1 0x%08x  r1/a2 0x%08x  r2/a3 0x%08x  r3/a4 0x%08x\n",
	       sys_le32_to_cpu(cpu->a1), sys_le32_to_cpu(cpu->a2), sys_le32_to_cpu(cpu->a3),
	       sys_le32_to_cpu(cpu->a4));
	printk("  r12/ip 0x%08x  r14/lr 0x%08x\n", sys_le32_to_cpu(cpu->ip),
	       sys_le32_to_cpu(cpu->lr));
	printk("  r15/pc 0x%08x (faulting instruction)  xpsr 0x%08x\n", sys_le32_to_cpu(cpu->pc),
	       sys_le32_to_cpu(cpu->xpsr));

	return true;
}

/* File name, NUL terminated, followed by the little-endian 32-bit line number */
static bool file_line_decode(struct net_buf_simple *buf, const char *kind, bool has_pc)
{
	const char *file;
	uint64_t pc = 0U;
	uint32_t line;
	size_t len;

	if (has_pc) {
		if (buf->len < sizeof(uint64_t)) {
			return false;
		}

		pc = net_buf_simple_pull_le64(buf);
	}

	if (buf->len < (sizeof(uint32_t) + 1U)) {
		return false;
	}

	len = buf->len - sizeof(uint32_t);
	file = (const char *)buf->data;
	if ((file[len - 1U] != '\0') || (strlen(file) != (len - 1U))) {
		return false;
	}

	(void)net_buf_simple_pull_mem(buf, len);
	line = net_buf_simple_pull_le32(buf);

	if (has_pc) {
		printk("Controller %s: %s:%u, pc 0x%08x%08x\n", kind, file, line,
		       (uint32_t)(pc >> 32), (uint32_t)pc);
	} else {
		printk("Controller %s: %s:%u\n", kind, file, line);
	}

	return true;
}

bool vs_fatal_vnd_evt(struct net_buf_simple *buf)
{
	uint8_t type;
	bool handled;

	if (buf->len < 1U) {
		return false;
	}

	type = net_buf_simple_pull_u8(buf);
	switch (type) {
	case BT_HCI_EVT_VS_ERROR_DATA_TYPE_STACK_FRAME:
		handled = stack_frame_decode(buf);
		break;

	case BT_HCI_EVT_VS_ERROR_DATA_TYPE_CTRL_ASSERT:
		handled = file_line_decode(buf, "assert", false);
		break;

	case BT_HCI_EVT_VS_ERROR_DATA_TYPE_TRACE:
		/* Shares its code with the Trace Info subevent, decoded only when well formed */
		handled = file_line_decode(buf, "fatal error trace", true);
		break;

	default:
		handled = false;
		break;
	}

	if (handled) {
		/* The Controller has halted, output the decoded fatal error synchronously */
		LOG_PANIC();
	}

	return handled;
}
