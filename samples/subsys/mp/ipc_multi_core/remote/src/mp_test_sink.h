/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MP_TEST_SINK_H_
#define MP_TEST_SINK_H_

#include <zephyr/mp/core/mp_sink.h>

struct mp_test_sink {
	struct mp_sink base;
	int frame_count;
};

void mp_test_sink_init(struct mp_element *self);

#endif /* MP_TEST_SINK_H_ */