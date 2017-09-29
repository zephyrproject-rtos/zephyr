/*
 * Copyright (c) 2017 Linaro Limited
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

/*
 * Original Authors:
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#ifndef LWM2M_RW_OMA_TLV_H_
#define LWM2M_RW_OMA_TLV_H_

#include <stdint.h>
#include <stddef.h>

#include "lwm2m_object.h"

enum {
	OMA_TLV_TYPE_OBJECT_INSTANCE   = 0,
	OMA_TLV_TYPE_RESOURCE_INSTANCE = 1,
	OMA_TLV_TYPE_MULTI_RESOURCE    = 2,
	OMA_TLV_TYPE_RESOURCE          = 3
};

enum {
	OMA_TLV_LEN_TYPE_NO_LEN    = 0,
	OMA_TLV_LEN_TYPE_8BIT_LEN  = 1,
	OMA_TLV_LEN_TYPE_16BIT_LEN = 2,
	OMA_TLV_LEN_TYPE_24BIT_LEN = 3
};

struct oma_tlv {
	u8_t  type;
	u16_t id; /* can be 8-bit or 16-bit when serialized */
	u32_t length;
	const u8_t *value;
};

extern const struct lwm2m_writer oma_tlv_writer;
extern const struct lwm2m_reader oma_tlv_reader;

int do_write_op_tlv(struct lwm2m_engine_obj *obj,
		    struct lwm2m_engine_context *context);

#endif /* LWM2M_RW_OMA_TLV_H_ */
