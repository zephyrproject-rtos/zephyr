/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* Software generic segmentation offload: split a block into the packets
 * it stands for, just before they are handed to the L2.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_if, CONFIG_NET_IF_LOG_LEVEL);

#include <errno.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/net_pkt.h>

#include "net_private.h"
#include "ipv4.h"
#include "ipv6.h"
#include "net_stats.h"

/* Read position in the payload of a block. The buffers before buf are still
 * linked to the block, prev is the last of them.
 */
struct gso_src {
	struct net_buf *prev;
	struct net_buf *buf;
	size_t off;
};

static size_t gso_hdr_len(struct net_pkt *pkt)
{
	return net_pkt_ip_hdr_len(pkt) + net_pkt_ip_opts_len(pkt) + NET_UDPH_LEN;
}

uint16_t net_gso_segment_count(struct net_pkt *pkt)
{
	uint16_t gso_size = net_pkt_gso_size(pkt);
	size_t hdr_len;
	size_t len;

	if (gso_size == 0U) {
		return 1U;
	}

	hdr_len = gso_hdr_len(pkt);
	len = net_pkt_get_len(pkt);

	if (len <= hdr_len) {
		return 1U;
	}

	return DIV_ROUND_UP(len - hdr_len, gso_size);
}

bool net_gso_needs_split(struct net_if *iface, struct net_pkt *pkt)
{
	enum net_gso_type type = net_pkt_gso_type(pkt);

	if (type == NET_GSO_NONE) {
		return false;
	}

	if ((net_if_get_gso_types(iface) & BIT(type)) == 0U) {
		return true;
	}

	if (net_pkt_get_len(pkt) > net_if_get_gso_max_size(iface)) {
		return true;
	}

	return net_gso_segment_count(pkt) > net_if_get_gso_max_segs(iface);
}

/* Step over empty buffers and the end of the current one */
static void gso_src_normalize(struct gso_src *src)
{
	while (src->buf != NULL && src->off >= src->buf->len) {
		src->off -= src->buf->len;
		src->prev = src->buf;
		src->buf = src->buf->frags;
	}
}

static void gso_src_init(struct gso_src *src, struct net_pkt *blk, size_t hdr_len)
{
	src->prev = NULL;
	src->buf = blk->buffer;
	src->off = hdr_len;

	gso_src_normalize(src);
}

/* Find the buffer that ends the next len payload bytes, if those bytes start
 * and end on buffer boundaries. Returns NULL otherwise.
 */
static struct net_buf *gso_src_run_end(struct gso_src *src, size_t len)
{
	struct net_buf *last;
	size_t taken = 0U;

	if (src->off != 0U || src->prev == NULL) {
		return NULL;
	}

	for (last = src->buf; last != NULL; last = last->frags) {
		taken += last->len;
		if (taken >= len) {
			break;
		}
	}

	if (last == NULL || taken != len) {
		return NULL;
	}

	return last;
}

/* Unlink the buffers from the current one up to last from the block */
static struct net_buf *gso_src_unlink(struct gso_src *src, struct net_buf *last)
{
	struct net_buf *first = src->buf;

	src->prev->frags = last->frags;
	src->buf = last->frags;
	last->frags = NULL;

	gso_src_normalize(src);

	return first;
}

static int gso_src_copy(struct gso_src *src, struct net_pkt *seg, size_t len)
{
	while (len > 0U) {
		size_t chunk;
		int ret;

		if (src->buf == NULL) {
			return -ENODATA;
		}

		chunk = MIN(src->buf->len - src->off, len);

		ret = net_pkt_write(seg, src->buf->data + src->off, chunk);
		if (ret < 0) {
			return ret;
		}

		src->off += chunk;
		len -= chunk;

		gso_src_normalize(src);
	}

	return 0;
}

static struct net_pkt *gso_alloc_segment(struct net_pkt *blk, size_t hdr_len,
					 k_timeout_t timeout)
{
	struct net_pkt *seg;

	seg = net_pkt_alloc_from_slab(blk->slab, timeout);
	if (seg == NULL) {
		return NULL;
	}

	net_pkt_set_iface(seg, net_pkt_iface(blk));

	/* The context selects the buffer pool, so set it before the buffer
	 * is allocated. The L2 reserves its header room in that buffer.
	 */
	net_pkt_set_context(seg, net_pkt_context(blk));

	if (net_pkt_alloc_buffer(seg, hdr_len, 0, timeout) < 0) {
		net_pkt_unref(seg);
		return NULL;
	}

	net_pkt_copy_attributes(blk, seg);

	return seg;
}

static int gso_build_segment(struct net_pkt *blk, struct net_pkt *seg,
			     struct gso_src *src, size_t hdr_len, size_t seg_len,
			     bool keep_block, k_timeout_t timeout)
{
	struct net_buf *last = NULL;
	int ret;

	if (!keep_block) {
		last = gso_src_run_end(src, seg_len);
	}

	if (last == NULL) {
		/* Allocated before the header is written, so that the
		 * payload follows it.
		 */
		ret = net_pkt_alloc_buffer_raw(seg, seg_len, timeout);
		if (ret < 0) {
			return ret;
		}
	}

	net_pkt_cursor_init(blk);
	net_pkt_cursor_init(seg);

	ret = net_pkt_copy(seg, blk, hdr_len);
	if (ret < 0) {
		return ret;
	}

	if (last != NULL) {
		net_pkt_append_buffer(seg, gso_src_unlink(src, last));
	} else {
		ret = gso_src_copy(src, seg, seg_len);
		if (ret < 0) {
			return ret;
		}
	}

	net_pkt_cursor_init(seg);

	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(seg) == NET_AF_INET) {
		ret = net_ipv4_finalize(seg, NET_IPPROTO_UDP);
	} else if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(seg) == NET_AF_INET6) {
		ret = net_ipv6_finalize(seg, NET_IPPROTO_UDP);
	} else {
		ret = -EAFNOSUPPORT;
	}

	net_pkt_cursor_init(seg);

	return ret;
}

static void gso_update_drop_stats(struct net_pkt *blk, size_t count)
{
	if (!IS_ENABLED(CONFIG_NET_STATISTICS)) {
		return;
	}

	while (count-- > 0U) {
		if (net_pkt_family(blk) == NET_AF_INET6) {
			net_stats_update_ipv6_drop(net_pkt_iface(blk));
		} else {
			net_stats_update_ipv4_drop(net_pkt_iface(blk));
		}
	}
}

int net_gso_segment(struct net_pkt *blk, bool keep_block, k_timeout_t timeout,
		    net_gso_deliver_t deliver, void *user_data)
{
	bool overwrite = net_pkt_is_being_overwritten(blk);
	uint16_t gso_size = net_pkt_gso_size(blk);
	size_t hdr_len = gso_hdr_len(blk);
	size_t len = net_pkt_get_len(blk);
	struct net_pkt_cursor backup;
	struct gso_src src;
	size_t left;
	int ret = 0;

	if (net_pkt_gso_type(blk) != NET_GSO_UDP_L4 || gso_size == 0U || len <= hdr_len) {
		ret = -EINVAL;
		goto out;
	}

	/* The headers are read again for every segment */
	net_pkt_cursor_backup(blk, &backup);
	net_pkt_set_overwrite(blk, true);

	left = len - hdr_len;
	gso_src_init(&src, blk, hdr_len);

	while (left > 0U) {
		size_t seg_len = MIN(left, gso_size);
		struct net_pkt *seg;

		seg = gso_alloc_segment(blk, hdr_len, timeout);
		if (seg == NULL) {
			ret = -ENOMEM;
			break;
		}

		ret = gso_build_segment(blk, seg, &src, hdr_len, seg_len, keep_block, timeout);
		if (ret < 0) {
			net_pkt_unref(seg);
			break;
		}

		left -= seg_len;

		/* The callback owns the segment. A segment that cannot be
		 * sent does not stop the others, as with separate datagrams.
		 */
		deliver(seg, user_data);
	}

	net_pkt_cursor_restore(blk, &backup);
	net_pkt_set_overwrite(blk, overwrite);

	if (ret < 0) {
		NET_DBG("Block %p: %zu bytes not segmented (%d)", blk, left, ret);

		if (!keep_block) {
			gso_update_drop_stats(blk, DIV_ROUND_UP(left, gso_size));
		}
	}

out:
	if (!keep_block) {
		net_pkt_unref(blk);
	}

	return ret;
}
