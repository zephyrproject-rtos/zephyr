/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWM2M_RW_OPAQUE_H_
#define LWM2M_RW_OPAQUE_H_

#include "lwm2m_object.h"

extern const struct lwm2m_writer opaque_writer;
extern const struct lwm2m_reader opaque_reader;

int do_read_op_opaque(struct lwm2m_message *msg, int content_format);
int do_write_op_opaque(struct lwm2m_message *msg);

#endif /* LWM2M_RW_OPAQUE_H_ */
