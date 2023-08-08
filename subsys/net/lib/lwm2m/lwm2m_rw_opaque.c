/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Copyright (c) 2015, Yanzi Networks AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LOG_MODULE_NAME net_lwm2m_opaque
#define LOG_LEVEL	CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lwm2m_object.h"
#include "lwm2m_rw_opaque.h"
#include "lwm2m_engine.h"
#include "lwm2m_util.h"

static int get_opaque(struct lwm2m_input_context *in, uint8_t *value,
		      size_t buflen, struct lwm2m_opaque_context *opaque,
		      bool *last_block)
{
	uint16_t in_len;

	if (opaque->remaining == 0) {
		coap_packet_get_payload(in->in_cpkt, &in_len);

		if (in_len == 0) {
			return -ENODATA;
		}

		if (in->block_ctx != NULL) {
			uint32_t block_num =
				in->block_ctx->ctx.current /
				coap_block_size_to_bytes(
					in->block_ctx->ctx.block_size);

			if (block_num == 0) {
				opaque->len = in->block_ctx->ctx.total_size;
			}

			if (opaque->len == 0) {
				/* No size1 option provided, use current
				 * payload size. This will reset on next packet
				 * received.
				 */
				opaque->remaining = in_len;
			} else {
				opaque->remaining = opaque->len;
			}

		} else {
			opaque->len = in_len;
			opaque->remaining = in_len;
		}
	}

	return lwm2m_engine_get_opaque_more(in, value, buflen,
					    opaque, last_block);
}

static int put_opaque(struct lwm2m_output_context *out,
			  struct lwm2m_obj_path *path, char *buf,
			  size_t buflen)
{
	int ret;

	ret = buf_append(CPKT_BUF_WRITE(out->out_cpkt), buf, buflen);
	if (ret < 0) {
		return ret;
	}

	return buflen;
}


const struct lwm2m_writer opaque_writer = {
	.put_opaque = put_opaque,
};

const struct lwm2m_reader opaque_reader = {
	.get_opaque = get_opaque,
};

int do_read_op_opaque(struct lwm2m_message *msg, int content_format)
{
	/* Opaque can only return single resource (instance) */
	if (msg->path.level < LWM2M_PATH_LEVEL_RESOURCE) {
		return -EPERM;
	} else if (msg->path.level > LWM2M_PATH_LEVEL_RESOURCE) {
		if (!IS_ENABLED(CONFIG_LWM2M_VERSION_1_1)) {
			return -ENOENT;
		} else if (msg->path.level > LWM2M_PATH_LEVEL_RESOURCE_INST) {
			return -ENOENT;
		}
	}

	return lwm2m_perform_read_op(msg, content_format);
}

int do_write_op_opaque(struct lwm2m_message *msg)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_engine_obj_field *obj_field;
	struct lwm2m_engine_res *res = NULL;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	int ret;
	uint8_t created = 0U;

	ret = lwm2m_get_or_create_engine_obj(msg, &obj_inst, &created);
	if (ret < 0) {
		return ret;
	}

	ret = lwm2m_engine_validate_write_access(msg, obj_inst, &obj_field);
	if (ret < 0) {
		return ret;
	}

	ret = lwm2m_engine_get_create_res_inst(&msg->path, &res, &res_inst);
	if (ret < 0) {
		return -ENOENT;
	}

	if (msg->path.level < LWM2M_PATH_LEVEL_RESOURCE) {
		msg->path.level = LWM2M_PATH_LEVEL_RESOURCE;
	}

	return lwm2m_write_handler(obj_inst, res, res_inst, obj_field, msg);
}
