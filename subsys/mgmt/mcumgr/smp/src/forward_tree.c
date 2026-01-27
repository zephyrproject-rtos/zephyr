/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** SMP Forward Tree - Simple Management Protocol Forward Tree. */

#include <zephyr/sys/byteorder.h>
#include <zephyr/net_buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <assert.h>
#include <string.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#include <mgmt/mcumgr/transport/smp_internal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcumgr_forward, 4);

#define SMP_FORWARD_TREE_PORT_MASK 0x0f
#define SMP_FORWARD_TREE_PORT_BITS 0x04
#define SMP_FORWARD_TREE_MAX_PORTS 0x10

#define DT_SMP_FORWARD_INST		DT_INST(0, zephyr_smpmgr_forward)
#define DT_SMP_UPSTREAM			DT_PHANDLE(DT_SMP_FORWARD_INST, upstream)
#define DT_SMP_UPSTREAM_TRANSPORT 	DT_PHANDLE(DT_SMP_UPSTREAM, transport)

struct smp_forward_tree_transport upstream_transport = {
	.dev = DEVICE_DT_GET(DT_SMP_UPSTREAM_TRANSPORT),
	.type = DT_ENUM_IDX(DT_SMP_UPSTREAM, type),
};

#define INST_DOWN_TRANSPORTS_ENTRY(node_id)					\
{										\
	.dev = DEVICE_DT_GET(DT_PHANDLE(node_id, transport)),			\
	.type = DT_ENUM_IDX(node_id, type),					\
},

#define INST_DOWN_TRANSPORTS(node_id, prop, idx)				\
	INST_DOWN_TRANSPORTS_ENTRY(DT_PHANDLE_BY_IDX(node_id, prop, idx))

struct smp_forward_tree_transport downstream_transport[] = {
	DT_FOREACH_PROP_ELEM(DT_SMP_FORWARD_INST, downstream, INST_DOWN_TRANSPORTS)
};

static int smp_read_hdr(const struct net_buf_simple *nb, struct smp_hdr *dst_hdr)
{
	if (nb->len < sizeof(*dst_hdr)) {
		return MGMT_ERR_EINVAL;
	}

	memcpy(dst_hdr, nb->data, sizeof(*dst_hdr));
	dst_hdr->nh_len = sys_be16_to_cpu(dst_hdr->nh_len);
	dst_hdr->nh_group = sys_be16_to_cpu(dst_hdr->nh_group);

	return 0;
}

static int smp_ft_read_fwd(const struct net_buf_simple *nb,
			   struct smp_forward_tree *dst_fwd)
{
	uint64_t tmp_ft;

	if (nb->len < sizeof(struct smp_forward_tree)) {
		return MGMT_ERR_EINVAL;
	}

	memcpy(&tmp_ft, nb->data + (nb->len - sizeof(uint64_t)), sizeof(uint64_t));
	tmp_ft = sys_be64_to_cpu(tmp_ft);
	memcpy(dst_fwd, &tmp_ft, sizeof(uint64_t));

	return 0;
}

int smp_ft_forward_downstream(struct smp_forward_tree *req_fwd, void *vreq)
{
	uint32_t shift = (req_fwd->hop - 1) * SMP_FORWARD_TREE_PORT_BITS;
	uint8_t port = (req_fwd->port >> shift) & SMP_FORWARD_TREE_PORT_MASK;
	struct smp_transport *smpt = NULL;

	LOG_DBG("port: %d", port);

	if (port >= SMP_FORWARD_TREE_MAX_PORTS
	||  port > ARRAY_SIZE(downstream_transport)) {
		LOG_ERR("Invalid transport index [%d]", port);
		return MGMT_ERR_EINVAL;
	}

	smpt = smp_get_smpt(downstream_transport[port].dev);
	if (smpt == NULL) {
		LOG_ERR("Transport index [%d] not recognized", port);
		for (int i = 0; i < ARRAY_SIZE(downstream_transport); ++i) {
			LOG_DBG("transport[%d]: %s", i, downstream_transport[i].dev->name);
		}

		return MGMT_ERR_EINVAL;
	}

	--req_fwd->hop;

	return smpt->functions.output(smpt->dev, vreq);
}

/**
 * Intercept all SMP requests in an incoming packet. Each intercepted requests
 * is evaluated sequentially looking in the header to detect the forward tree
 * bit. When the Forward Tree bit is set the data length contains 8 additional
 * bytes at end of the package. The Forward Tree evaluate the protocol counter
 * to detected if the value is 0. When the value is zero the package is send to
 * the mgmt/smp.c to process the content locally (final destination). If the
 * counter is greater the zero the content is forwarded to the correspondent
 * port number. If the port does not exists the package is dropped and an error
 * is returned.
 *
 * If a request elicits an error response, processing of the packet is aborted.
 * This function consumes the supplied request buffer regardless of the outcome.
 *
 * The function will return MGMT_ERR_EOK (0) when given an empty input stream,
 * and will also release the buffer from the stream; it does not return
 * MTMT_ERR_ECORRUPT, or any other MGMT error, because there was no error while
 * processing of the input stream, it is callers fault that an empty stream has
 * been passed to the function.
 *
 * @param streamer	The streamer to use for reading, writing, and transmitting.
 * @param req		A buffer containing the request packet.
 *
 * @return 0 on success or when input stream is empty;
 *         MGMT_ERR_ECORRUPT if buffer starts with non SMP data header or there
 *         is not enough bytes to process header, or other MGMT_ERR_[...] code on
 *         failure.
 */
int smp_ft_process_request_packet(struct smp_streamer *streamer, void *vreq)
{
	struct smp_hdr req_hdr = { 0 };
	struct net_buf_simple clone = { 0 };
	struct smp_forward_tree req_fwd = { 0 };
	struct net_buf *req = vreq;
	struct smp_transport *smpt;
	int rc = 0;

	LOG_DBG("incomming forward request...");

	/*
	 * This clone will copy the size and max length. The pointers will
	 * reference the real data. This means that any change in the data in
	 * the cloned data will change the real data.
	 */
	net_buf_simple_clone(&req->b, &clone);

	do {
		/* Read the management header. */
		rc = smp_read_hdr(&clone, &req_hdr);
		if (rc != 0) {
			rc = MGMT_ERR_ECORRUPT;
			LOG_ERR("Corrupted 1");
			break;
		}

		LOG_DBG("Group ID: %04x", req_hdr.nh_group);
		LOG_DBG("Seq Num:  %02x", req_hdr.nh_seq);
		LOG_DBG("CMD ID:   %02x", req_hdr.nh_id);
		LOG_DBG("OP:       %02x", req_hdr.nh_op);
		LOG_DBG("Flags:    %02x", req_hdr.nh_flags);
		LOG_DBG("Len:      %04x", req_hdr.nh_len);

		/* Does buffer contain only one message and it is complete? */
		net_buf_simple_pull(&clone, sizeof(struct smp_hdr));

		if (clone.len != req_hdr.nh_len) {
			rc = MGMT_ERR_ECORRUPT;
			LOG_ERR("Corrupted 2");
			break;
		}

		if (req_hdr.nh_flags & SMP_HDR_FLAG_FORWARD_TREE) {
			LOG_DBG("Processing Forward Tree Protocol");
			if (smp_ft_read_fwd(&clone, &req_fwd)) {
				rc = MGMT_ERR_ECORRUPT;
				LOG_ERR("Corrupted 3");
				break;
			}

			LOG_DBG("hops: %02x", req_fwd.hop);
			for (int i = req_fwd.hop - 1; i >= 0; --i) {
				uint8_t port = ((req_fwd.port >> (i * SMP_FORWARD_TREE_PORT_BITS))
						& SMP_FORWARD_TREE_PORT_MASK);
				LOG_DBG("fwd[%02d]: %02x", i + 1, port);
			}

			if (req_fwd.hop > 0) {
				LOG_ERR("forward downstream");
				rc = smp_ft_forward_downstream(&req_fwd, vreq);
				break;
			}

			// Drop forward tree content from payload
			net_buf_simple_remove_mem(&clone, sizeof(struct smp_forward_tree));

			// Adjust Header
			req_hdr.nh_flags &= ~SMP_HDR_FLAG_FORWARD_TREE;
			req_hdr.nh_len -= sizeof(struct smp_forward_tree);

			// Replace Header
			net_buf_simple_push_mem(&clone, &req_hdr, sizeof(struct smp_hdr));
		}

		if (streamer->smpt->dev == upstream_transport.dev) {
			LOG_DBG("local port: %s", streamer->smpt->dev->name);
			rc = smp_process_request_packet(streamer, vreq);
		} else {
			smpt = smp_get_smpt(upstream_transport.dev);
			if (smpt == NULL) {
				rc = MGMT_ERR_ECORRUPT;
				LOG_ERR("Corrupted 4");
				break;
			}

			LOG_DBG("forward upstream: %s", smpt->dev->name);
			rc = smpt->functions.output(smpt->dev, vreq);
		}
	} while (0);

	LOG_DBG("finish forward request...");

	smp_free_buf(req, streamer->smpt);

	return rc;
}
