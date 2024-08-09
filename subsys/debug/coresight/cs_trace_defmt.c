/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/debug/coresight/cs_trace_defmt.h>

static cs_trace_defmt_cb callback;
static uint8_t curr_id;

int cs_trace_defmt_init(cs_trace_defmt_cb cb)
{
	callback = cb;
	return 0;
}

int cs_trace_defmt_process(const uint8_t *data, size_t len)
{
	uint8_t buf[15];
	size_t cnt = 0;

	if (len != 16) {
		return -EINVAL;
	}

	uint8_t aux = data[15];
	uint8_t d_id;
	uint8_t cb_id;
	bool do_cb = false;

	for (int i = 0; i < 8; i++) {
		d_id = data[2 * i];
		if (d_id & 0x1) {
			if (cnt != 0) {
				cb_id = curr_id;
				if ((aux >> i) & 0x1) {
					/* next byte belongs to the old stream */
					do_cb = true;
				} else {
					callback(cb_id, buf, cnt);
					cnt = 0;
				}
			}
			curr_id = d_id >> 1;
		} else {
			buf[cnt++] = d_id | ((aux >> i) & 0x1);
		}
		if (i < 7) {
			buf[cnt++] = data[2 * i + 1];
			if (do_cb) {
				do_cb = false;
				callback(cb_id, buf, cnt);
				cnt = 0;
			}
		}
	}

	if (cnt) {
		callback(curr_id, buf, cnt);
	}

	return 0;
}
