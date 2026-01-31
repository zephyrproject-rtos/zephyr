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

	/* RFC 8613 Section 6.1: OSCORE option value format (Figure 5):
	 *   0 1 2 3 4 5 6 7  Bit number
	 *  +-+-+-+-+-+-+-+-+
	 *  |0 0 0|h|k|n|   |  Flag byte
	 *  +-+-+-+-+-+-+-+-+
	 *  |  Partial IV   |  (if n=1, length-prefixed)
	 *  +-+-+-+-+-+-+-+-+
	 *  | kid context   |  (if h=1, length-prefixed)
	 *  +-+-+-+-+-+-+-+-+
	 *  |     kid       |  (if k=1, length-prefixed)
	 *  +-+-+-+-+-+-+-+-+
	 *
	 * Flag byte bits (LSB first):
	 *   - Bit 0: n (Partial IV present)
	 *   - Bit 1: k (kid present)
	 *   - Bit 2: h (kid context present)
	 *   - Bits 3-7: reserved (must be 0)
	 */

	if (oscore_len == 0) {
		/* Empty OSCORE option - no kid present */
		return -ENOENT;
	}

	/* Parse flag byte */
	uint8_t flag_byte = oscore_value[pos++];
	bool n_present = (flag_byte & 0x01) != 0;  /* Partial IV */
	bool k_present = (flag_byte & 0x02) != 0;  /* kid */
	bool h_present = (flag_byte & 0x04) != 0;  /* kid context */

	/* Skip Partial IV if present */
	if (n_present) {
		if (pos >= oscore_len) {
			LOG_ERR("OSCORE option truncated at Partial IV");
			return -EINVAL;
		}
		uint8_t piv_len = oscore_value[pos++];
		if (pos + piv_len > oscore_len) {
			LOG_ERR("OSCORE option Partial IV length invalid");
			return -EINVAL;
		}
		pos += piv_len;
	}

	/* Skip kid context if present */
	if (h_present) {
		if (pos >= oscore_len) {
			LOG_ERR("OSCORE option truncated at kid context");
			return -EINVAL;
		}
		uint8_t kid_ctx_len = oscore_value[pos++];
		if (pos + kid_ctx_len > oscore_len) {
			LOG_ERR("OSCORE option kid context length invalid");
			return -EINVAL;
		}
		pos += kid_ctx_len;
	}

	/* Extract kid if present */
	if (!k_present) {
		return -ENOENT;
	}

	if (pos >= oscore_len) {
		LOG_ERR("OSCORE option truncated at kid");
		return -EINVAL;
	}

	uint8_t kid_field_len = oscore_value[pos++];
	if (pos + kid_field_len > oscore_len) {
		LOG_ERR("OSCORE option kid length invalid");
		return -EINVAL;
	}

	if (kid_field_len > max_kid_len) {
		LOG_ERR("kid too large (%u > %zu)", kid_field_len, max_kid_len);
		return -ENOMEM;
	}

	memcpy(kid, &oscore_value[pos], kid_field_len);
	*kid_len = kid_field_len;

	return 0;
}
