/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/bluetooth/buf.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_ctlr_hci_vs_err);

#define STR_NULL_TERMINATOR 0x00

/* A memory pool for vendor specific events for fatal error reporting purposes. */
NET_BUF_POOL_FIXED_DEFINE(vs_err_tx_pool, 1, BT_BUF_EVT_RX_SIZE, 0, NULL);

static struct net_buf *vs_err_evt_create(uint8_t subevt, uint8_t len)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&vs_err_tx_pool, K_FOREVER);
	if (buf) {
		struct bt_hci_evt_le_meta_event *me;
		struct bt_hci_evt_hdr *hdr;

		net_buf_add_u8(buf, BT_HCI_H4_EVT);

		hdr = net_buf_add(buf, sizeof(*hdr));
		hdr->evt = BT_HCI_EVT_VENDOR;
		hdr->len = len + sizeof(*me);

		me = net_buf_add(buf, sizeof(*me));
		me->subevent = subevt;
	}

	return buf;
}

/* The alias for convenience of Controller HCI implementation. Controller is built for
 * a particular architecture hence the alias will allow to avoid conditional compilation.
 * Host may be not aware of hardware architecture the Controller is working on, hence
 * all CPU data types for supported architectures should be available during build, hence
 * the alias is defined here.
 */
#if defined(CONFIG_CPU_CORTEX_M)
static void vs_err_fatal_cpu_data_fill(struct bt_hci_vs_fatal_error_cpu_data_cortex_m *cpu_data,
				       const struct arch_esf *esf)
{
	cpu_data->a1 = sys_cpu_to_le32(esf->basic.a1);
	cpu_data->a2 = sys_cpu_to_le32(esf->basic.a2);
	cpu_data->a3 = sys_cpu_to_le32(esf->basic.a3);
	cpu_data->a4 = sys_cpu_to_le32(esf->basic.a4);
	cpu_data->ip = sys_cpu_to_le32(esf->basic.ip);
	cpu_data->lr = sys_cpu_to_le32(esf->basic.lr);
	cpu_data->pc = sys_cpu_to_le32(esf->basic.pc);
	cpu_data->xpsr = sys_cpu_to_le32(esf->basic.xpsr);
}

struct net_buf *hci_vs_err_stack_frame(unsigned int reason, const struct arch_esf *esf)
{
	/* Prepare vendor specific HCI Fatal Error event */
	struct bt_hci_vs_fatal_error_stack_frame *sf;
	struct bt_hci_vs_fatal_error_cpu_data_cortex_m *cpu_data;
	struct net_buf *buf;

	buf = vs_err_evt_create(BT_HCI_EVT_VS_ERROR_DATA_TYPE_STACK_FRAME,
				sizeof(*sf) + sizeof(*cpu_data));
	if (buf != NULL) {
		sf = net_buf_add(buf, (sizeof(*sf) + sizeof(*cpu_data)));
		sf->reason = sys_cpu_to_le32(reason);
		sf->cpu_type = BT_HCI_EVT_VS_ERROR_CPU_TYPE_CORTEX_M;

		vs_err_fatal_cpu_data_fill((void *)sf->cpu_data, esf);
	} else {
		LOG_WRN("Can't create HCI Fatal Error event");
	}

	return buf;
}

#else /* !CONFIG_CPU_CORTEX_M */
struct net_buf *hci_vs_err_stack_frame(unsigned int reason, const struct arch_esf *esf)
{
	return NULL;
}
#endif /* !CONFIG_CPU_CORTEX_M */

static struct net_buf *hci_vs_err_trace_create(uint8_t data_type,
					       const char *file_path,
					       uint32_t line, uint64_t pc)
{
	uint32_t file_name_len = 0U, pos = 0U;
	struct net_buf *buf = NULL;

	if (file_path) {
		/* Extract file name from a path */
		while (file_path[file_name_len] != '\0') {
			if (file_path[file_name_len] == '/') {
				pos = file_name_len + 1;
			}
			file_name_len++;
		}
		file_path += pos;
		file_name_len -= pos;

		/* If file name was found in file_path, in other words: file_path is not empty
		 * string and is not `foo/bar/`.
		 */
		if (file_name_len) {
			/* Total data length: len = file name strlen + \0 + sizeof(line number)
			 * Maximum length of an HCI event data is BT_BUF_EVT_RX_SIZE. If total data
			 * length exceeds this maximum, truncate file name.
			 */
			uint32_t data_len = 1 + sizeof(line);

			/* If a buffer is created for a TRACE data, include sizeof(pc) in total
			 * length.
			 */
			if (data_type == BT_HCI_EVT_VS_ERROR_DATA_TYPE_TRACE) {
				data_len += sizeof(pc);
			}

			if (data_len + file_name_len > BT_BUF_EVT_RX_SIZE) {
				uint32_t overflow_len =
					file_name_len + data_len - BT_BUF_EVT_RX_SIZE;

				/* Truncate the file name length by number of overflow bytes */
				file_name_len -= overflow_len;
			}

			/* Get total event data length including file name length */
			data_len += file_name_len;

			/* Prepare vendor specific HCI Fatal Error event */
			buf = vs_err_evt_create(data_type, data_len);
			if (buf != NULL) {
				if (data_type == BT_HCI_EVT_VS_ERROR_DATA_TYPE_TRACE) {
					net_buf_add_le64(buf, pc);
				}
				net_buf_add_mem(buf, file_path, file_name_len);
				net_buf_add_u8(buf, STR_NULL_TERMINATOR);
				net_buf_add_le32(buf, line);
			} else {
				LOG_ERR("Can't create HCI Fatal Error event");
			}
		}
	}

	return buf;
}

struct net_buf *hci_vs_err_trace(const char *file, uint32_t line, uint64_t pc)
{
	return hci_vs_err_trace_create(BT_HCI_EVT_VS_ERROR_DATA_TYPE_TRACE, file, line, pc);
}

struct net_buf *hci_vs_err_assert(const char *file, uint32_t line)
{
	/* ASSERT data does not contain PC counter, because of that zero constant is used */
	return hci_vs_err_trace_create(BT_HCI_EVT_VS_ERROR_DATA_TYPE_CTRL_ASSERT, file, line, 0U);
}
