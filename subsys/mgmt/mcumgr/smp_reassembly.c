/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/buf.h>
#include <zephyr/mgmt/mcumgr/smp.h>
#include <mgmt/mgmt.h>
#include <smp/smp.h>
#include "smp_internal.h"

void zephyr_smp_reassembly_init(struct zephyr_smp_transport *zst)
{
	zst->__reassembly.current = NULL;
	zst->__reassembly.expected = 0;
}

int zephyr_smp_reassembly_expected(const struct zephyr_smp_transport *zst)
{
	if (zst->__reassembly.current == NULL) {
		return -EINVAL;
	}

	return zst->__reassembly.expected;
}

int zephyr_smp_reassembly_collect(struct zephyr_smp_transport *zst, const void *buf, uint16_t len)
{
	if (zst->__reassembly.current == NULL) {
		/*
		 * Collecting the first fragment: need to allocate buffer for it and prepare
		 * the reassembly context.
		 */
		if (len >= sizeof(struct mgmt_hdr)) {
			uint16_t expected = sys_be16_to_cpu(((struct mgmt_hdr *)buf)->nh_len);

			/*
			 * The length field in the header does not count the header size,
			 * but the reassembly does so the size needs to be added to the number of
			 * expected bytes.
			 */
			expected += sizeof(struct mgmt_hdr);

			/* Joining net_bufs not supported yet */
			if (len > CONFIG_MCUMGR_BUF_SIZE || expected > CONFIG_MCUMGR_BUF_SIZE) {
				return -ENOSR;
			}

			if (len > expected) {
				return -EOVERFLOW;
			}

			zst->__reassembly.current = mcumgr_buf_alloc();
			if (zst->__reassembly.current != NULL) {
				zst->__reassembly.expected = expected;
			} else {
				return -ENOMEM;
			}
		} else {
			/* Not enough data to even collect header */
			return -ENODATA;
		}
	}

	/* len is expected to be > 0 */
	if (zst->__reassembly.expected >= len) {
		net_buf_add_mem(zst->__reassembly.current, buf, len);
		zst->__reassembly.expected -= len;
	} else {
		/*
		 * A fragment is longer than the expected size and will not fit into the buffer.
		 */
		return -EOVERFLOW;
	}

	return zst->__reassembly.expected;
}

int zephyr_smp_reassembly_complete(struct zephyr_smp_transport *zst, bool force)
{
	if (zst->__reassembly.current == NULL) {
		return -EINVAL;
	}

	if (zst->__reassembly.expected == 0 || force) {
		int expected = zst->__reassembly.expected;

		zephyr_smp_rx_req(zst, zst->__reassembly.current);
		zst->__reassembly.expected = 0;
		zst->__reassembly.current = NULL;
		return expected;
	}
	return -ENODATA;
}

int zephyr_smp_reassembly_drop(struct zephyr_smp_transport *zst)
{
	if (zst->__reassembly.current == NULL) {
		return -EINVAL;
	}

	mcumgr_buf_free(zst->__reassembly.current);
	zst->__reassembly.expected = 0;
	zst->__reassembly.current = NULL;

	return 0;
}

void *zephyr_smp_reassembly_get_ud(const struct zephyr_smp_transport *zst)
{
	if (zst->__reassembly.current != NULL) {
		return net_buf_user_data(zst->__reassembly.current);
	}

	return NULL;
}
