/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/sys/byteorder.h>

#include "policy_internal.h"

static bool ipv4_options_valid(const uint8_t *options, size_t len)
{
	while (len != 0U) {
		uint8_t kind = options[0];
		size_t size = 1U;

		if (kind == 0U) {
			return true;
		}
		if (kind != 1U) {
			/* A source-routed packet may change its destination after admission. */
			if (kind == 131U || kind == 137U || len < 2U ||
			    options[1] < 2U || options[1] > len) {
				return false;
			}
			size = options[1];
		}
		options += size;
		len -= size;
	}
	return true;
}

bool tailscale_packet_filter_parse(const uint8_t *ip, size_t len,
				  struct tailscale_filter_tuple *tuple)
{
	size_t header_len;
	size_t packet_len;
	uint8_t protocol;

	if (ip == NULL || len == 0U || tuple == NULL) {
		return false;
	}
	switch (ip[0] >> 4U) {
	case 4:
		if (len < 20U || (sys_get_be16(ip + 6U) & ~BIT(14)) != 0U) {
			return false;
		}
		header_len = (ip[0] & 0x0fU) * 4U;
		packet_len = sys_get_be16(ip + 2U);
		if (header_len < 20U || header_len > packet_len || packet_len > len ||
		    !ipv4_options_valid(ip + 20U, header_len - 20U)) {
			return false;
		}
		protocol = ip[9];
		tuple->source = ip + 12U;
		tuple->destination = ip + 16U;
		tuple->family = NET_AF_INET;
		break;
	case 6:
		if (len < 40U) {
			return false;
		}
		header_len = 40U;
		packet_len = header_len + sys_get_be16(ip + 4U);
		if (packet_len > len) {
			return false;
		}
		/* Only direct TCP or UDP follows this header. No extension chain is admitted. */
		protocol = ip[6];
		tuple->source = ip + 8U;
		tuple->destination = ip + 24U;
		tuple->family = NET_AF_INET6;
		break;
	default:
		return false;
	}

	const uint8_t *transport = ip + header_len;
	size_t transport_len = packet_len - header_len;

	switch (protocol) {
	case NET_IPPROTO_TCP:
		if (transport_len < 20U || (transport[12] >> 4U) < 5U ||
		    (transport[12] >> 4U) * 4U > transport_len) {
			return false;
		}
		tuple->tcp_flags = transport[13];
		break;
	case NET_IPPROTO_UDP:
		if (transport_len < 8U || sys_get_be16(transport + 4U) != transport_len) {
			return false;
		}
		tuple->tcp_flags = 0;
		break;
	default:
		return false;
	}
	tuple->protocol = protocol;
	tuple->source_port = sys_get_be16(transport);
	tuple->destination_port = sys_get_be16(transport + 2U);
	return true;
}

static bool address_matches(const struct tailscale_filter_address *range, uint8_t family,
			    const uint8_t *address)
{
	size_t len = family == NET_AF_INET ? 4U : 16U;

	return range->family == NET_AF_UNSPEC ||
	       (range->family == family && memcmp(address, range->first, len) >= 0 &&
		memcmp(address, range->last, len) <= 0);
}

bool tailscale_packet_filter_match(const struct tailscale_packet_filter *filter,
				   const struct tailscale_filter_tuple *tuple)
{
	uint8_t protocol;

	if (filter == NULL || tuple == NULL || tuple->source == NULL ||
	    tuple->destination == NULL ||
	    (tuple->family != NET_AF_INET && tuple->family != NET_AF_INET6) ||
	    (tuple->protocol != NET_IPPROTO_TCP && tuple->protocol != NET_IPPROTO_UDP)) {
		return false;
	}
	if (filter->reserve_ssh && tuple->protocol == NET_IPPROTO_TCP &&
	    tuple->destination_port == 22U) {
		return false;
	}
	protocol = tuple->protocol == NET_IPPROTO_TCP ? TAILSCALE_FILTER_TCP : TAILSCALE_FILTER_UDP;

	for (size_t i = 0; i < filter->count; i++) {
		const struct tailscale_packet_filter_rule *rule = &filter->rules[i];
		bool source = false;

		if ((rule->protocols & protocol) == 0U) {
			continue;
		}
		for (size_t s = 0; s < rule->source_count; s++) {
			if (address_matches(&rule->sources[s], tuple->family, tuple->source)) {
				source = true;
				break;
			}
		}
		if (!source) {
			continue;
		}
		for (size_t d = 0; d < rule->destination_count; d++) {
			const struct tailscale_packet_filter_dst *dst = &rule->destinations[d];

			if (tuple->destination_port >= dst->first_port &&
			    tuple->destination_port <= dst->last_port &&
			    address_matches(&dst->address, tuple->family, tuple->destination)) {
				return true;
			}
		}
	}
	return false;
}

bool tailscale_packet_filter_accept(const struct tailscale_packet_filter *filter, const uint8_t *ip,
				   size_t len)
{
	struct tailscale_filter_tuple tuple;

	return filter != NULL && filter->count != 0U &&
	       tailscale_packet_filter_parse(ip, len, &tuple) &&
	       tailscale_packet_filter_match(filter, &tuple);
}
