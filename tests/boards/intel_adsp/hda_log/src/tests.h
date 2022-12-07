/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_TESTS_INTEL_ADSP_TESTS_H
#define ZEPHYR_TESTS_INTEL_ADSP_TESTS_H

#include <intel_adsp_ipc.h>
#include <cavstool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>

static inline void hda_ipc_msg(const struct device *dev, uint32_t data,
			       uint32_t ext, k_timeout_t timeout)
{
	zassert_true(intel_adsp_ipc_send_message_sync(dev, data, ext, timeout),
		"Unexpected ipc send message failure, try increasing IPC_TIMEOUT");
}

#endif /* ZEPHYR_TESTS_INTEL_ADSP_TESTS_H */
