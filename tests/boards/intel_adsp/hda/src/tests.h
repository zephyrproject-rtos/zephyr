/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_TESTS_INTEL_ADSP_TESTS_H
#define ZEPHYR_TESTS_INTEL_ADSP_TESTS_H

#include <zephyr/sys_clock.h>
#include <cavs_ipc.h>
#include <cavstool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <ztest.h>

void test_hda_host_in_smoke(void);
void test_hda_host_out_smoke(void);
void test_hda_host_in_dma(void);


/* Turn this define on to see register dumps after each step */
#define INTEL_ADSP_HDA_DBG 0

#define CONCAT3(x, y, z) x ## y ## z

#define STREAM_SET_BASE(stream_set) CONCAT3(HDA_, stream_set, _BASE)
#define STREAM_SET_NAME(stream_set) STRINGIFY(stream_set)

#if INTEL_ADSP_HDA_DBG
#define hda_dump_regs(stream_set, stream_id, ...) \
	printk(__VA_ARGS__); printk(": ");        \
	intel_adsp_hda_dbg(STREAM_SET_NAME(stream_set), STREAM_SET_BASE(stream_set), stream_id)
#else
#define hda_dump_regs(stream_set, stream_id, msg, ...) do {} while (0)
#endif

static inline void hda_ipc_msg(const struct device *dev, uint32_t data,
			       uint32_t ext, k_timeout_t timeout)
{
	zassert_true(cavs_ipc_send_message_sync(dev, data, ext, timeout),
		"Unexpected ipc send message failure, try increasing IPC_TIMEOUT");
}

#endif /* ZEPHYR_TESTS_INTEL_ADSP_TESTS_H */
