/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>

#include <mgmt/mcumgr/transport/smp_internal.h>

#define MCUMGR_TRANSPORT_NETBUF_SIZE CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE

void smp_reassembly_init(struct smp_transport *smpt)
{
	smpt->__reassembly.current = NULL;
	smpt->__reassembly.expected = 0;
}

int smp_reassembly_expected(const struct smp_transport *smpt)
{
	if (smpt->__reassembly.current == NULL) {
		return -EINVAL;
	}

	return smpt->__reassembly.expected;
}

int smp_reassembly_collect(struct smp_transport *smpt, const void *buf, uint16_t len)
{
	if (smpt->__reassembly.current == NULL) {
		/*
		 * Collecting the first fragment: need to allocate buffer for it and prepare
		 * the reassembly context.
		 */
		if (len >= sizeof(struct smp_hdr)) {
			uint16_t expected = sys_be16_to_cpu(((struct smp_hdr *)buf)->nh_len);

			/*
			 * The length field in the header does not count the header size,
			 * but the reassembly does so the size needs to be added to the number of
			 * expected bytes.
			 */
			expected += sizeof(struct smp_hdr);

			/* Joining net_bufs not supported yet */
			if (len > MCUMGR_TRANSPORT_NETBUF_SIZE ||
			    expected > MCUMGR_TRANSPORT_NETBUF_SIZE) {
				return -ENOSR;
			}

			if (len > expected) {
				return -EOVERFLOW;
			}

			smpt->__reassembly.current = smp_packet_alloc();
			if (smpt->__reassembly.current != NULL) {
				smpt->__reassembly.expected = expected;
			} else {
				return -ENOMEM;
			}
		} else {
			/* Not enough data to even collect header */
			return -ENODATA;
		}
	}

	/* len is expected to be > 0 */
	if (smpt->__reassembly.expected >= len) {
		net_buf_add_mem(smpt->__reassembly.current, buf, len);
		smpt->__reassembly.expected -= len;
	} else {
		/*
		 * A fragment is longer than the expected size and will not fit into the buffer.
		 */
		return -EOVERFLOW;
	}

	return smpt->__reassembly.expected;
}

int smp_reassembly_complete(struct smp_transport *smpt, bool force)
{
	if (smpt->__reassembly.current == NULL) {
		return -EINVAL;
	}

	if (smpt->__reassembly.expected == 0 || force) {
		int expected = smpt->__reassembly.expected;

		smp_rx_req(smpt, smpt->__reassembly.current);
		smpt->__reassembly.expected = 0;
		smpt->__reassembly.current = NULL;
		return expected;
	}
	return -ENODATA;
}

int smp_reassembly_drop(struct smp_transport *smpt)
{
	if (smpt->__reassembly.current == NULL) {
		return -EINVAL;
	}

	smp_packet_free(smpt->__reassembly.current);
	smpt->__reassembly.expected = 0;
	smpt->__reassembly.current = NULL;

	return 0;
}

void *smp_reassembly_get_ud(const struct smp_transport *smpt)
{
	if (smpt->__reassembly.current != NULL) {
		return net_buf_user_data(smpt->__reassembly.current);
	}

	return NULL;
}
