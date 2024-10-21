/*
 * Copyright (c) 2024 Junho Lee <junho@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/pcie/controller.h>

#include <zephyr/posix/posix_types.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/posix/time.h>

#include "eth.h"
#include "eth_tsn_nic_priv.h"

int tsn_fill_metadata(const struct device *dev, uint64_t now, struct tx_buffer *buf)
{
	buf->metadata.fail_policy = TSN_FAIL_POLICY_DROP;

	buf->metadata.from.tick = 0;
	buf->metadata.from.priority = 0;
	buf->metadata.to.tick = (1 << 29) - 1;
	buf->metadata.to.priority = 0;
	buf->metadata.delay_from.tick = 0;
	buf->metadata.delay_from.priority = 0;
	buf->metadata.delay_to.tick = (1 << 29) - 1;
	buf->metadata.delay_to.priority = 0;

	buf->metadata.timestamp_id = TSN_TIMESTAMP_ID_NONE;

	return 0;
}
