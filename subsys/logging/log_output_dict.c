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

	log_output_write(output->func, (uint8_t *)&output_hdr, sizeof(output_hdr),
			 (void *)output->control_block->ctx);

	size_t len;
	uint8_t *data = log_msg_get_package(msg, &len);

	if (len > 0U) {
		log_output_write(output->func, data, len, (void *)output->control_block->ctx);
	}

	data = log_msg_get_data(msg, &len);
	if (len > 0U) {
		log_output_write(output->func, data, len, (void *)output->control_block->ctx);
	}

	log_output_flush(output);
}

void log_dict_output_dropped_process(const struct log_output *output, uint32_t cnt)
{
	struct log_dict_output_dropped_msg_t msg;

	msg.type = MSG_DROPPED_MSG;
	msg.num_dropped_messages = MIN(cnt, 9999);

	log_output_write(output->func, (uint8_t *)&msg, sizeof(msg),
			 (void *)output->control_block->ctx);
}
