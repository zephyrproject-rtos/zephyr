/*
 * Copyright (c) 2026 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap, CONFIG_COAP_LOG_LEVEL);

#include <zephyr/net/coap.h>
#include "coap_oscore_option.h"

#include <errno.h>
#include <string.h>

int coap_oscore_option_extract_kid(const struct coap_packet *cpkt,
				   uint8_t *kid, size_t *kid_len)
{
	struct coap_option option;
	int ret;
	const uint8_t *oscore_value;
	size_t oscore_len;
	size_t pos = 0;
	size_t max_kid_len = *kid_len;

	if (cpkt == NULL || kid == NULL || kid_len == NULL) {
		return -EINVAL;
	}

	/* Find OSCORE option */
	ret = coap_find_options(cpkt, COAP_OPTION_OSCORE, &option, 1);
	if (ret <= 0) {
		return -ENOENT;
	}

	oscore_value = option.value;
	oscore_len = option.len;

	/* RFC 8613 Section 6.1: OSCORE option value format (Figure 10):
	 *   0 1 2 3 4 5 6 7 <------------- n bytes -------------->
	 *  +-+-+-+-+-+-+-+-+--------------------------------------
	 *  |0 0 0|h|k|  n  |       Partial IV (if any) ...
	 *  +-+-+-+-+-+-+-+-+--------------------------------------
	 *
	 *   <- 1 byte -> <----- s bytes ------>
	 *  +------------+----------------------+------------------+
	 *  | s (if any) | kid context (if any) | kid (if any) ... |
	 *  +------------+----------------------+------------------+
	 *
	 * Flag byte bits (LSB first):
	 *   - Bits 0-2: n (Partial IV length, 0-5 valid, 6-7 reserved)
	 *   - Bit 3: k (kid present flag)
	 *   - Bit 4: h (kid context present flag)
	 *   - Bits 5-7: reserved (must be 0)
	 */

	if (oscore_len == 0) {
		/* Empty OSCORE option - no kid present */
		return -ENOENT;
	}

	/* Parse flag byte */
	uint8_t flags = oscore_value[pos++];

	/* RFC 8613 Section 2: If flags are all zero, option value must be empty */
	if (flags == 0x00) {
		LOG_ERR("OSCORE option with flags=0x00 must be empty (RFC 8613 Section 2)");
		return -EINVAL;
	}
	uint8_t n = flags & 0x07;        /* Partial IV length (bits 0-2) */
	bool k = (flags & 0x08) != 0;    /* kid flag (bit 3) */
	bool h = (flags & 0x10) != 0;    /* kid context flag (bit 4) */
	uint8_t reserved = flags & 0xE0; /* Reserved bits (bits 5-7) */

	/* RFC 8613 §6.1: Reserved bits must be zero */
	if (reserved != 0) {
		LOG_ERR("OSCORE option has reserved bits set (0x%02x)", flags);
		return -EINVAL;
	}

	/* RFC 8613 §6.1: n=6 and n=7 are reserved */
	if (n == 6 || n == 7) {
		LOG_ERR("OSCORE option has reserved Partial IV length (%u)", n);
		return -EINVAL;
	}

	/* Check we have enough bytes for Partial IV */
	if (pos + n > oscore_len) {
		LOG_ERR("OSCORE option too short for Partial IV");
		return -EINVAL;
	}

	/* Skip n bytes of Partial IV */
	pos += n;

	/* Skip kid context if present */
	if (h) {
		if (pos >= oscore_len) {
			LOG_ERR("OSCORE option truncated at kid context length");
			return -EINVAL;
		}
		uint8_t s = oscore_value[pos++];
		if (pos + s > oscore_len) {
			LOG_ERR("OSCORE option kid context length invalid");
			return -EINVAL;
		}
		pos += s;
	}

	/* Extract kid if present */
	if (!k) {
		return -ENOENT;
	}

	/* RFC 8613 §6.1: kid is the remaining bytes (not length-prefixed) */
	size_t kid_len_on_wire = oscore_len - pos;

	if (kid_len_on_wire > max_kid_len) {
		LOG_ERR("kid too large (%zu > %zu)", kid_len_on_wire, max_kid_len);
		return -ENOMEM;
	}

	memcpy(kid, &oscore_value[pos], kid_len_on_wire);
	*kid_len = kid_len_on_wire;

	return 0;
}
