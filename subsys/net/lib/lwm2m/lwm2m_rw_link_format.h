/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWM2M_RW_LINK_FORMAT_H_
#define LWM2M_RW_LINK_FORMAT_H_

#include "lwm2m_object.h"

enum link_format_mode {
	LINK_FORMAT_MODE_DISCOVERY,
	LINK_FORMAT_MODE_BOOTSTRAP_DISCOVERY,
	LINK_FORMAT_MODE_REGISTER,
};

struct link_format_out_formatter_data {
	uint8_t request_level;
	uint8_t mode;
	bool is_first : 1;
};

extern const struct lwm2m_writer link_format_writer;

int do_discover_op_link_format(struct lwm2m_message *msg, bool is_bootstrap);
int do_register_op_link_format(struct lwm2m_message *msg);

#endif /* LWM2M_RW_LINK_FORMAT_H_ */
