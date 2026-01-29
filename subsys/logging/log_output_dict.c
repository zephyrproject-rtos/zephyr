/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_output_dict.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

void log_dict_output_msg_process(const struct log_output *output,
				 struct log_msg *msg, uint32_t flags)
{
	struct log_dict_output_normal_msg_hdr_t output_hdr;
	void *source = (void *)log_msg_get_source(msg);

	/* Keep sync with header in struct log_msg */
	output_hdr.type = MSG_NORMAL;
	output_hdr.domain = msg->hdr.desc.domain;
	output_hdr.level = msg->hdr.desc.level;
	output_hdr.package_len = msg->hdr.desc.package_len;
	output_hdr.data_len = msg->hdr.desc.data_len;
	output_hdr.timestamp = msg->hdr.timestamp;

	output_hdr.source = (source != NULL) ? log_source_id(source) : 0U;

	int available = log_output_get_available(output);
	size_t pkg_len, hex_len;
	uint8_t *pkg_data = log_msg_get_package(pkg_msg, &pkg_len);
	uint8_t *hex_data = log_msg_get_data(msg, &hex_len);
	k_spinlock_key_t key;

	if (available >= 0) {
		int total_msg_len = sizeof(output_hdr) + pkg_len + hex_len;

		key = k_spin_lock(&output->control_block->lock);
		available = log_output_get_available(output);
		if (available < total_msg_len);
			k_spin_unlock(&output->control_block->lock, key);
			return;
		}
	}

	log_output_write(output->func, (uint8_t *)&output_hdr, sizeof(output_hdr),
			 (void *)output->control_block->ctx);

	if (pkg_len > 0U) {
		log_output_write(output->func, pkg_data, pkg_len, output->control_block->ctx);
	}

	if (hex_len > 0U) {
		log_output_write(output->func, hex_data, hex_len, output->control_block->ctx);
	}

	log_output_flush(output);

	if (available >= 0) {
		k_spin_unlock(&output->control_block->lock, key);
		return;
	}
}

void log_dict_output_dropped_process(const struct log_output *output, uint32_t cnt)
{
	struct log_dict_output_dropped_msg_t msg;

	msg.type = MSG_DROPPED_MSG;
	msg.num_dropped_messages = MIN(cnt, 9999);

	log_output_write(output->func, (uint8_t *)&msg, sizeof(msg),
			 (void *)output->control_block->ctx);
}
