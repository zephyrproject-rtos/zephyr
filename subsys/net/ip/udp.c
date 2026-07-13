/** @file
 * @brief UDP packet helpers.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_udp, CONFIG_NET_UDP_LOG_LEVEL);

#include <zephyr/net/net_log.h>
#include "net_private.h"
#include "udp_internal.h"
#include "net_stats.h"

#define PKT_WAIT_TIME K_SECONDS(1)

int net_udp_create(struct net_pkt *pkt, uint16_t src_port, uint16_t dst_port)
{
	NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
	struct net_udp_hdr *udp_hdr;

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_access);
	if (!udp_hdr) {
		return -ENOBUFS;
	}

	udp_hdr->src_port = src_port;
	udp_hdr->dst_port = dst_port;
	udp_hdr->len      = 0U;
	udp_hdr->chksum   = 0U;

	return net_pkt_set_data(pkt, &udp_access);
}

int net_udp_finalize(struct net_pkt *pkt, bool force_chksum)
{
	NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
	struct net_udp_hdr *udp_hdr;
	uint16_t length = 0;
	enum net_if_checksum_type type = net_pkt_family(pkt) == NET_AF_INET6 ?
		NET_IF_CHECKSUM_IPV6_UDP : NET_IF_CHECKSUM_IPV4_UDP;

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_access);
	if (!udp_hdr) {
		return -ENOBUFS;
	}

	length = net_pkt_get_len(pkt) - net_pkt_ip_hdr_len(pkt) -
		 net_pkt_ip_opts_len(pkt);

	if (IS_ENABLED(CONFIG_NET_UDP_OPTIONS)) {
		length -= net_pkt_udp_opt_surplus_len(pkt);
	}

	udp_hdr->len = net_htons(length);

	if (net_if_need_calc_tx_checksum(net_pkt_iface(pkt), type) || force_chksum) {
		int ret;
		uint16_t chksum = 0;

		udp_hdr->chksum = 0;
		ret = net_calc_chksum_udp(pkt, &chksum);
		if (ret < 0) {
			return ret;
		}

		udp_hdr->chksum = chksum;
		net_pkt_set_chksum_done(pkt, true);
	}

	return net_pkt_set_data(pkt, &udp_access);
}

struct net_udp_hdr *net_udp_get_hdr(struct net_pkt *pkt,
				    struct net_udp_hdr *hdr)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(udp_access, struct net_udp_hdr);
	struct net_pkt_cursor backup;
	struct net_udp_hdr *udp_hdr;
	bool overwrite;

	udp_access.data = hdr;

	overwrite = net_pkt_is_being_overwritten(pkt);
	net_pkt_set_overwrite(pkt, true);

	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);

	if (net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
			 net_pkt_ip_opts_len(pkt))) {
		udp_hdr = NULL;
		goto out;
	}

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_access);

out:
	net_pkt_cursor_restore(pkt, &backup);
	net_pkt_set_overwrite(pkt, overwrite);

	return udp_hdr;
}

struct net_udp_hdr *net_udp_set_hdr(struct net_pkt *pkt,
				    struct net_udp_hdr *hdr)
{
	NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
	struct net_pkt_cursor backup;
	struct net_udp_hdr *udp_hdr;
	bool overwrite;

	overwrite = net_pkt_is_being_overwritten(pkt);
	net_pkt_set_overwrite(pkt, true);

	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);

	if (net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) +
			 net_pkt_ip_opts_len(pkt))) {
		udp_hdr = NULL;
		goto out;
	}

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_access);
	if (!udp_hdr) {
		goto out;
	}

	memcpy(udp_hdr, hdr, sizeof(struct net_udp_hdr));

	if (net_pkt_set_data(pkt, &udp_access) < 0) {
		udp_hdr = NULL;
		goto out;
	}
out:
	net_pkt_cursor_restore(pkt, &backup);
	net_pkt_set_overwrite(pkt, overwrite);

	return udp_hdr == NULL ? NULL : hdr;
}

int net_udp_register(uint8_t family,
		     const struct net_sockaddr *remote_addr,
		     const struct net_sockaddr *local_addr,
		     uint16_t remote_port,
		     uint16_t local_port,
		     struct net_context *context,
		     net_conn_cb_t cb,
		     void *user_data,
		     struct net_conn_handle **handle)
{
	return net_conn_register(NET_IPPROTO_UDP, NET_SOCK_DGRAM, family, remote_addr,
				 local_addr, remote_port, local_port, context,
				 cb, user_data, handle);
}

int net_udp_unregister(struct net_conn_handle *handle)
{
	return net_conn_unregister(handle);
}

struct net_udp_hdr *net_udp_input(struct net_pkt *pkt,
				  struct net_pkt_data_access *udp_access)
{
	struct net_udp_hdr *udp_hdr;
	uint16_t ip_payload_len;
	uint16_t udp_len;
	int ret;
	uint16_t chksum = 0;
	enum net_if_checksum_type type = net_pkt_family(pkt) == NET_AF_INET6 ?
		NET_IF_CHECKSUM_IPV6_UDP : NET_IF_CHECKSUM_IPV4_UDP;

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, udp_access);
	if (!udp_hdr || net_pkt_set_data(pkt, udp_access)) {
		NET_DBG("DROP: corrupted header");
		goto drop;
	}

	udp_len = net_ntohs(udp_hdr->len);

	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == NET_AF_INET) {
		uint16_t ipv4_total_len = net_ntohs(NET_IPV4_HDR(pkt)->len);
		uint16_t ipv4_hdr_len = net_pkt_ip_hdr_len(pkt);

		if (ipv4_total_len < ipv4_hdr_len) {
			NET_DBG("DROP: Invalid IPv4 total length");
			goto drop;
		}

		ip_payload_len = ipv4_total_len - ipv4_hdr_len;
	} else if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == NET_AF_INET6) {
		uint16_t ipv6_payload_len = net_ntohs(NET_IPV6_HDR(pkt)->len);
		uint16_t ipv6_ext_len = net_pkt_ip_opts_len(pkt);

		if (ipv6_payload_len < ipv6_ext_len) {
			NET_DBG("DROP: Invalid IPv6 payload length");
			goto drop;
		}

		ip_payload_len = ipv6_payload_len - ipv6_ext_len;
	} else {
		NET_DBG("DROP: Unknown protocol family");
		goto drop;
	}

	if (udp_len < NET_UDPH_LEN || udp_len > ip_payload_len) {
		NET_DBG("DROP: Invalid hdr length");
		goto drop;
	}

#if defined(CONFIG_NET_UDP_OPTIONS)
	net_pkt_set_udp_opt_surplus_len(pkt, 0U);

	if (udp_len < ip_payload_len) {
		/* A surplus area carrying UDP options (RFC 9868) is present.
		 * Record its length so it is excluded from the UDP checksum and
		 * from the payload delivered to the application. Do not validate
		 * the options here: per RFC 9868 a malformed surplus area or a
		 * failed OCS MUST NOT cause the datagram to be dropped when the
		 * UDP checksum (which covers only the UDP Length bytes) is valid.
		 * Options are parsed and their OCS validated later; the user data
		 * is delivered regardless.
		 */
		net_pkt_set_udp_opt_surplus_len(pkt, ip_payload_len - udp_len);
	}
#endif /* CONFIG_NET_UDP_OPTIONS */

	if (IS_ENABLED(CONFIG_NET_UDP_CHECKSUM) &&
	    (net_if_need_calc_rx_checksum(net_pkt_iface(pkt), type) ||
	     net_pkt_is_ip_reassembled(pkt))) {
		if (!udp_hdr->chksum) {
			if (IS_ENABLED(CONFIG_NET_UDP_MISSING_CHECKSUM) &&
			    net_pkt_family(pkt) == NET_AF_INET) {
				goto out;
			}

			goto drop;
		}

		ret = net_calc_verify_chksum_udp(pkt, &chksum);
		if (ret < 0 || chksum != 0U) {
			NET_DBG("DROP: checksum mismatch");
			goto drop;
		}
	}
out:
	return udp_hdr;
drop:
	net_stats_update_udp_chkerr(net_pkt_iface(pkt));
	return NULL;
}

#if defined(CONFIG_NET_UDP_OPTIONS)
static uint16_t udp_opt_fold_checksum(uint32_t sum)
{
	while (sum >> 16) {
		sum = (sum & 0xffffU) + (sum >> 16);
	}

	return (uint16_t)sum;
}

static int udp_opt_get_header_info(struct net_pkt *pkt, uint16_t *udp_len,
				   uint16_t *udp_chksum)
{
	struct net_udp_hdr udp_hdr;
	struct net_udp_hdr *hdr;

	if (pkt == NULL || udp_len == NULL || udp_chksum == NULL) {
		return -EINVAL;
	}

	hdr = net_udp_get_hdr(pkt, &udp_hdr);
	if (hdr == NULL) {
		return -EINVAL;
	}

	*udp_len = net_ntohs(hdr->len);
	*udp_chksum = hdr->chksum;

	return 0;
}

static int udp_opt_calc_ocs(struct net_pkt *pkt,
			    uint16_t surplus_offset,
			    uint16_t surplus_len,
			    uint16_t *calc_ocs,
			    uint16_t *wire_ocs,
			    bool *pad_ok)
{
	uint32_t sum = 0U;
	size_t abs_val = 0U;
	size_t idx = 0U;
	size_t end = (size_t)surplus_offset + surplus_len;
	uint8_t hi = 0U;
	uint8_t wire_hi = 0U;
	uint8_t wire_lo = 0U;
	bool have_hi = false;
	uint8_t align_pad;

	if (pkt == NULL || calc_ocs == NULL || surplus_len == 0U) {
		return -EINVAL;
	}

	align_pad = surplus_offset & 0x1U;
	if (surplus_len < (uint16_t)(align_pad + sizeof(uint16_t))) {
		return -EINVAL;
	}

	if (end > net_pkt_get_len(pkt)) {
		return -EINVAL;
	}

	if (pad_ok != NULL) {
		*pad_ok = true;
	}

	NET_PKT_FRAG_FOR_EACH(pkt, frag) {
		for (size_t i = 0U; i < frag->len; i++) {
			size_t rel;
			uint8_t byte;

			if (abs_val < surplus_offset) {
				abs_val++;
				continue;
			}

			if (abs_val >= end) {
				break;
			}

			byte = frag->data[i];
			rel = idx++;

			if (rel < align_pad && byte != 0U && pad_ok != NULL) {
				*pad_ok = false;
			}

			if (rel == align_pad) {
				wire_hi = byte;
			} else if (rel == (size_t)align_pad + 1U) {
				wire_lo = byte;
			}

			if (rel == align_pad || rel == (size_t)align_pad + 1U) {
				byte = 0U;
			}

			if (!have_hi) {
				hi = byte;
				have_hi = true;
			} else {
				sum += ((uint16_t)hi << 8) | byte;
				sum = (sum & 0xffffU) + (sum >> 16);
				have_hi = false;
			}

			abs_val++;
		}

		if (abs_val >= end) {
			break;
		}
	}

	if (idx != surplus_len) {
		return -EINVAL;
	}

	if (have_hi) {
		sum += ((uint16_t)hi << 8);
		sum = (sum & 0xffffU) + (sum >> 16);
	}

	sum += surplus_len;
	sum = udp_opt_fold_checksum(sum);

	*calc_ocs = (uint16_t)(~sum);

	if (wire_ocs != NULL) {
		*wire_ocs = ((uint16_t)wire_hi << 8) | wire_lo;
	}

	return 0;
}

static int udp_opt_parse_tlvs(struct net_pkt *pkt, uint16_t offset,
			      uint16_t length,
			      struct net_udp_opt_info *info)
{
	struct net_pkt_cursor backup;
	uint16_t remaining = length;
	int ret = 0;

	if (length == 0U) {
		return 0;
	}

	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);

	ret = net_pkt_skip(pkt, offset);
	if (ret < 0) {
		ret = -EINVAL;
		goto out;
	}

	while (remaining > 0U) {
		uint8_t kind;
		uint8_t opt_len;
		uint16_t opt_payload;
		uint16_t payload_len;

		ret = net_pkt_read_u8(pkt, &kind);
		if (ret < 0) {
			ret = -EINVAL;
			goto out;
		}

		remaining--;

		if (kind == NET_UDP_OPT_KIND_EOL) {
			/* Remaining bytes are expected to be explicit zero padding. */
			while (remaining > 0U) {
				uint8_t pad;

				ret = net_pkt_read_u8(pkt, &pad);
				if (ret < 0 || pad != 0U) {
					ret = -EINVAL;
					goto out;
				}

				remaining--;
			}

			break;
		}

		if (kind == NET_UDP_OPT_KIND_NOP) {
			continue;
		}

		/* RFC 9868 Section 7: an unsupported option in the UNSAFE range
		 * (kinds 192-255) signals that the datagram's user data was
		 * transformed in a way this receiver does not understand. All
		 * option processing MUST be terminated and every option in the
		 * datagram discarded. No UNSAFE option is currently supported.
		 */
		if (kind >= NET_UDP_OPT_KIND_UCMP) {
			ret = -EINVAL;
			goto out;
		}

		if (remaining < 1U) {
			ret = -EINVAL;
			goto out;
		}

		ret = net_pkt_read_u8(pkt, &opt_len);
		if (ret < 0) {
			ret = -EINVAL;
			goto out;
		}

		remaining--;

		if (opt_len == 255U) {
			/* RFC 9868 Section 5: options longer than 254 bytes use
			 * the extended format (Kind, 0xff, 2-byte Extended
			 * Length). The Extended Length counts the whole option,
			 * including the Kind, 0xff and Extended Length bytes, so
			 * the minimum valid value is 4.
			 */
			uint16_t ext_len;

			if (remaining < sizeof(uint16_t)) {
				ret = -EINVAL;
				goto out;
			}

			ret = net_pkt_read_be16(pkt, &ext_len);
			if (ret < 0) {
				ret = -EINVAL;
				goto out;
			}

			remaining -= sizeof(uint16_t);

			if (ext_len < 4U) {
				ret = -EINVAL;
				goto out;
			}

			opt_payload = ext_len - 4U;
		} else {
			if (opt_len < 2U) {
				ret = -EINVAL;
				goto out;
			}

			opt_payload = (uint16_t)opt_len - 2U;
		}

		payload_len = opt_payload;
		if (payload_len > remaining) {
			ret = -EINVAL;
			goto out;
		}

		/* RFC 9868 Section 7: a known option whose length does not match
		 * the value defined for its kind is an error and MUST result in
		 * all options being discarded. Unknown options in the SAFE range
		 * are ignored (skipped) via the default case.
		 */
		switch (kind) {
		case NET_UDP_OPT_KIND_APC:
			if (payload_len != sizeof(uint32_t)) {
				ret = -EINVAL;
				goto out;
			}

			/* The APC CRC32c is carried to the application as-is; the
			 * options layer does not verify it (RFC 9868 leaves the
			 * payload checksum to the endpoints/application).
			 */
			ret = net_pkt_read_be32(pkt, &info->apc_crc);
			if (ret < 0) {
				ret = -EINVAL;
				goto out;
			}

			info->present |= NET_UDP_OPT_F_APC;
			payload_len = 0U;
			break;

		case NET_UDP_OPT_KIND_MDS:
			if (payload_len != sizeof(uint16_t)) {
				ret = -EINVAL;
				goto out;
			}

			ret = net_pkt_read_be16(pkt, &info->mds);
			if (ret < 0) {
				ret = -EINVAL;
				goto out;
			}

			info->present |= NET_UDP_OPT_F_MDS;
			payload_len = 0U;
			break;

		case NET_UDP_OPT_KIND_MRDS:
			if (payload_len != 3U) {
				ret = -EINVAL;
				goto out;
			}

			ret = net_pkt_read_be16(pkt, &info->mrds.size);
			if (ret < 0) {
				ret = -EINVAL;
				goto out;
			}

			ret = net_pkt_read_u8(pkt, &info->mrds.segs);
			if (ret < 0) {
				ret = -EINVAL;
				goto out;
			}

			info->present |= NET_UDP_OPT_F_MRDS;
			payload_len = 0U;
			break;

		case NET_UDP_OPT_KIND_REQ:
			if (payload_len != sizeof(uint32_t)) {
				ret = -EINVAL;
				goto out;
			}

			ret = net_pkt_read_be32(pkt, &info->req_token);
			if (ret < 0) {
				ret = -EINVAL;
				goto out;
			}

			info->present |= NET_UDP_OPT_F_REQ;
			payload_len = 0U;
			break;

		case NET_UDP_OPT_KIND_RES:
			if (payload_len != sizeof(uint32_t)) {
				ret = -EINVAL;
				goto out;
			}

			ret = net_pkt_read_be32(pkt, &info->res_token);
			if (ret < 0) {
				ret = -EINVAL;
				goto out;
			}

			info->present |= NET_UDP_OPT_F_RES;
			payload_len = 0U;
			break;

		case NET_UDP_OPT_KIND_TIME:
			if (payload_len != (sizeof(uint32_t) * 2U)) {
				ret = -EINVAL;
				goto out;
			}

			ret = net_pkt_read_be32(pkt, &info->time.tsval);
			if (ret < 0) {
				ret = -EINVAL;
				goto out;
			}

			ret = net_pkt_read_be32(pkt, &info->time.tsecr);
			if (ret < 0) {
				ret = -EINVAL;
				goto out;
			}

			info->present |= NET_UDP_OPT_F_TIME;
			payload_len = 0U;
			break;

		default:
			break;
		}

		if (payload_len > 0U) {
			ret = net_pkt_skip(pkt, payload_len);
			if (ret < 0) {
				ret = -EINVAL;
				goto out;
			}
		}

		remaining -= opt_payload;
	}

out:
	net_pkt_cursor_restore(pkt, &backup);

	return ret;
}

int net_udp_opt_parse(struct net_pkt *pkt, struct net_udp_opt_info *info)
{
	uint16_t surplus_len;
	uint16_t udp_len;
	uint16_t udp_chksum;
	uint16_t calc_ocs;
	uint16_t wire_ocs;
	uint16_t surplus_offset;
	uint16_t opt_offset;
	uint16_t opt_len;
	bool pad_ok = true;
	int ret;

	if (pkt == NULL || info == NULL) {
		return -EINVAL;
	}

	(void)memset(info, 0, sizeof(*info));

	surplus_len = net_pkt_udp_opt_surplus_len(pkt);
	if (surplus_len == 0U) {
		return -ENOENT;
	}

	ret = udp_opt_get_header_info(pkt, &udp_len, &udp_chksum);
	if (ret < 0) {
		return ret;
	}

	surplus_offset = net_pkt_ip_hdr_len(pkt) + net_pkt_ip_opts_len(pkt) + udp_len;

	ret = udp_opt_calc_ocs(pkt, surplus_offset, surplus_len,
	&calc_ocs, &wire_ocs, &pad_ok);
	if (ret < 0) {
		return ret;
	}

	info->present |= NET_UDP_OPT_F_OCS;

	if (!pad_ok) {
		info->ocs_valid = false;
		return -EINVAL;
	}

	if (udp_chksum != 0U && wire_ocs == 0U) {
		/* RFC 9868: when the UDP checksum is non-zero the OCS MUST be
		 * non-zero. A zero OCS here is invalid; ignore the options.
		 */
		info->ocs_valid = false;
		return -EINVAL;
	}

	if (wire_ocs == 0U && udp_chksum == 0U) {
		/* RFC 9868: with a zero UDP checksum the OCS may be unused
		 * (zero). The options are still parsed and processed.
		 */
		info->ocs_valid = true;
	} else {
		info->ocs_valid = (wire_ocs == calc_ocs);
		if (!info->ocs_valid) {
			return -EINVAL;
		}
	}

	opt_offset = surplus_offset + (surplus_offset & 0x1U) + sizeof(uint16_t);
	opt_len = surplus_len - (uint16_t)((surplus_offset & 0x1U) + sizeof(uint16_t));

	ret = udp_opt_parse_tlvs(pkt, opt_offset, opt_len, info);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int udp_opt_write_tlv_u16(struct net_pkt *pkt, uint8_t kind, uint16_t value)
{
	int ret;

	ret = net_pkt_write_u8(pkt, kind);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_u8(pkt, 4U);
	if (ret < 0) {
		return ret;
	}

	return net_pkt_write_be16(pkt, value);
}

static int udp_opt_write_tlv_u32(struct net_pkt *pkt, uint8_t kind, uint32_t value)
{
	int ret;

	ret = net_pkt_write_u8(pkt, kind);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_u8(pkt, 6U);
	if (ret < 0) {
		return ret;
	}

	return net_pkt_write_be32(pkt, value);
}

static int udp_opt_write_tlv_time(struct net_pkt *pkt, uint32_t tsval, uint32_t tsecr)
{
	int ret;

	ret = net_pkt_write_u8(pkt, NET_UDP_OPT_KIND_TIME);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_u8(pkt, 10U);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_be32(pkt, tsval);
	if (ret < 0) {
		return ret;
	}

	return net_pkt_write_be32(pkt, tsecr);
}

static int udp_opt_write_tlv_mrds(struct net_pkt *pkt, uint16_t size, uint8_t segs)
{
	int ret;

	ret = net_pkt_write_u8(pkt, NET_UDP_OPT_KIND_MRDS);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_u8(pkt, 5U);
	if (ret < 0) {
		return ret;
	}

	ret = net_pkt_write_be16(pkt, size);
	if (ret < 0) {
		return ret;
	}

	return net_pkt_write_u8(pkt, segs);
}

static uint16_t udp_opt_tx_tlv_len(uint32_t enabled, uint16_t mds, uint16_t mrds_size)
{
	uint16_t len = 0U;

	if ((enabled & NET_UDP_OPT_F_APC) != 0U) {
		len += 6U;
	}

	if ((enabled & NET_UDP_OPT_F_MDS) != 0U && mds != 0U) {
		len += 4U;
	}

	if ((enabled & NET_UDP_OPT_F_MRDS) != 0U && mrds_size != 0U) {
		len += 5U;
	}

	if ((enabled & NET_UDP_OPT_F_REQ) != 0U) {
		len += 6U;
	}

	if ((enabled & NET_UDP_OPT_F_RES) != 0U) {
		len += 6U;
	}

	if ((enabled & NET_UDP_OPT_F_TIME) != 0U) {
		len += 10U;
	}

	return len;
}

uint16_t net_udp_opt_surplus_len_with_opts(struct net_context *ctx, uint16_t data_len,
					   const struct net_udp_opt_info *opts)
{
	const uint32_t tlv_mask = NET_UDP_OPT_F_APC | NET_UDP_OPT_F_MDS |
				  NET_UDP_OPT_F_MRDS | NET_UDP_OPT_F_REQ |
				  NET_UDP_OPT_F_RES | NET_UDP_OPT_F_TIME;
	uint16_t tlv_len;
	uint32_t enabled;
	uint16_t mds;
	uint16_t mrds_size;

	if (ctx == NULL) {
		return 0U;
	}

	enabled = ctx->options.udp_opt.enabled;
	mds = ctx->options.udp_opt.mds;
	mrds_size = ctx->options.udp_opt.mrds.size;

	if (opts != NULL) {
		enabled |= opts->present & tlv_mask;

		if ((opts->present & NET_UDP_OPT_F_MDS) != 0U && opts->mds != 0U) {
			mds = opts->mds;
		}

		if ((opts->present & NET_UDP_OPT_F_MRDS) != 0U && opts->mrds.size != 0U) {
			mrds_size = opts->mrds.size;
		}
	}

	if ((enabled & tlv_mask) != 0U) {
		enabled |= NET_UDP_OPT_F_OCS;
	}

	if ((enabled & NET_UDP_OPT_F_OCS) == 0U) {
		return 0U;
	}

	tlv_len = udp_opt_tx_tlv_len(enabled, mds, mrds_size);

	/* Reserve two bytes for OCS and one alignment pad for odd data lengths. */
	return ((data_len & 0x1U) != 0U ? 3U : 2U) + tlv_len;
}

uint16_t net_udp_opt_surplus_len(struct net_context *ctx, uint16_t data_len)
{
	return net_udp_opt_surplus_len_with_opts(ctx, data_len, NULL);
}

int net_udp_opt_append(struct net_pkt *pkt, struct net_context *ctx,
		       const struct net_udp_opt_info *opts)
{
	const uint32_t tlv_mask = NET_UDP_OPT_F_APC | NET_UDP_OPT_F_MDS |
				  NET_UDP_OPT_F_MRDS | NET_UDP_OPT_F_REQ |
				  NET_UDP_OPT_F_RES | NET_UDP_OPT_F_TIME;
	uint32_t enabled;
	uint16_t tlv_len;
	uint16_t surplus_offset;
	uint16_t surplus_len;
	uint16_t mds;
	uint16_t mrds_size;
	uint32_t apc_crc = 0U;
	uint32_t req_token = 0U;
	uint32_t res_token = 0U;
	uint32_t time_tsval = 0U;
	uint32_t time_tsecr = 0U;
	uint8_t align_pad;
	uint8_t mrds_segs;
	int ret;

	if (pkt == NULL || ctx == NULL) {
		return -EINVAL;
	}

	enabled = ctx->options.udp_opt.enabled;
	mds = ctx->options.udp_opt.mds;
	mrds_size = ctx->options.udp_opt.mrds.size;
	mrds_segs = ctx->options.udp_opt.mrds.segs;

	if (opts != NULL) {
		enabled |= opts->present & tlv_mask;

		if ((opts->present & NET_UDP_OPT_F_APC) != 0U) {
			apc_crc = opts->apc_crc;
		}

		if ((opts->present & NET_UDP_OPT_F_REQ) != 0U) {
			req_token = opts->req_token;
		}

		if ((opts->present & NET_UDP_OPT_F_RES) != 0U) {
			res_token = opts->res_token;
		}

		if ((opts->present & NET_UDP_OPT_F_TIME) != 0U) {
			time_tsval = opts->time.tsval;
			time_tsecr = opts->time.tsecr;
		}

		if ((opts->present & NET_UDP_OPT_F_MDS) != 0U && opts->mds != 0U) {
			mds = opts->mds;
		}

		if ((opts->present & NET_UDP_OPT_F_MRDS) != 0U && opts->mrds.size != 0U) {
			mrds_size = opts->mrds.size;
			mrds_segs = opts->mrds.segs;
		}
	}

	if ((enabled & tlv_mask) != 0U) {
		enabled |= NET_UDP_OPT_F_OCS;
	}

	if ((enabled & NET_UDP_OPT_F_OCS) == 0U) {
		net_pkt_set_udp_opt_surplus_len(pkt, 0U);
		return 0;
	}

	tlv_len = udp_opt_tx_tlv_len(enabled, mds, mrds_size);
	surplus_offset = net_pkt_get_len(pkt);
	align_pad = surplus_offset & 0x1U;
	surplus_len = (uint16_t)align_pad + sizeof(uint16_t) + tlv_len;

	if (align_pad != 0U) {
		ret = net_pkt_write_u8(pkt, 0U);
		if (ret < 0) {
			return ret;
		}
	}

	ret = net_pkt_write_be16(pkt, 0U);
	if (ret < 0) {
		return ret;
	}

	if ((enabled & NET_UDP_OPT_F_APC) != 0U) {
		ret = udp_opt_write_tlv_u32(pkt, NET_UDP_OPT_KIND_APC, apc_crc);
		if (ret < 0) {
			return ret;
		}
	}

	if ((enabled & NET_UDP_OPT_F_MDS) != 0U && mds != 0U) {
		ret = udp_opt_write_tlv_u16(pkt, NET_UDP_OPT_KIND_MDS, mds);
		if (ret < 0) {
			return ret;
		}
	}

	if ((enabled & NET_UDP_OPT_F_MRDS) != 0U && mrds_size != 0U) {
		ret = udp_opt_write_tlv_mrds(pkt, mrds_size, mrds_segs);
		if (ret < 0) {
			return ret;
		}
	}

	if ((enabled & NET_UDP_OPT_F_REQ) != 0U) {
		ret = udp_opt_write_tlv_u32(pkt, NET_UDP_OPT_KIND_REQ, req_token);
		if (ret < 0) {
			return ret;
		}
	}

	if ((enabled & NET_UDP_OPT_F_RES) != 0U) {
		ret = udp_opt_write_tlv_u32(pkt, NET_UDP_OPT_KIND_RES, res_token);
		if (ret < 0) {
			return ret;
		}
	}

	if ((enabled & NET_UDP_OPT_F_TIME) != 0U) {
		ret = udp_opt_write_tlv_time(pkt, time_tsval, time_tsecr);
		if (ret < 0) {
			return ret;
		}
	}

	net_pkt_set_udp_opt_surplus_len(pkt, surplus_len);

	return 0;
}

int net_udp_opt_finalize_ocs(struct net_pkt *pkt,
			     uint16_t surplus_offset,
			     uint16_t surplus_len)
{
	struct net_pkt_cursor backup;
	bool overwrite;
	uint16_t udp_len;
	uint16_t udp_chksum;
	uint16_t ocs;
	uint16_t ocs_offset;
	uint8_t align_pad;
	int ret;

	if (pkt == NULL || surplus_len == 0U) {
		return -EINVAL;
	}

	ret = udp_opt_get_header_info(pkt, &udp_len, &udp_chksum);
	if (ret < 0) {
		return ret;
	}

	if ((size_t)surplus_offset + surplus_len > net_pkt_get_len(pkt)) {
		return -EINVAL;
	}

	ret = udp_opt_calc_ocs(pkt, surplus_offset, surplus_len, &ocs, NULL, NULL);
	if (ret < 0) {
		return ret;
	}

	if (udp_chksum != 0U && ocs == 0U) {
		ocs = 0xffffU;
	}

	align_pad = surplus_offset & 0x1U;
	ocs_offset = surplus_offset + align_pad;

	overwrite = net_pkt_is_being_overwritten(pkt);
	net_pkt_set_overwrite(pkt, true);

	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);

	ret = net_pkt_skip(pkt, ocs_offset);
	if (ret == 0) {
		ret = net_pkt_write_be16(pkt, ocs);
	}

	net_pkt_cursor_restore(pkt, &backup);
	net_pkt_set_overwrite(pkt, overwrite);

	return ret;
}
#else /* CONFIG_NET_UDP_OPTIONS */
int net_udp_opt_parse(struct net_pkt *pkt, struct net_udp_opt_info *info)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(info);

	return -ENOTSUP;
}

uint16_t net_udp_opt_surplus_len(struct net_context *ctx, uint16_t data_len)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(data_len);

	return 0U;
}

int net_udp_opt_append(struct net_pkt *pkt, struct net_context *ctx,
		       const struct net_udp_opt_info *opts)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(ctx);
	ARG_UNUSED(opts);

	return -ENOTSUP;
}

int net_udp_opt_finalize_ocs(struct net_pkt *pkt,
			     uint16_t surplus_offset,
			     uint16_t surplus_len)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(surplus_offset);
	ARG_UNUSED(surplus_len);

	return -ENOTSUP;
}
#endif /* CONFIG_NET_UDP_OPTIONS */
