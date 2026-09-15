/*
 * Copyright (c) 2026 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_COAP_CLIENT_INTERNAL_H_
#define ZEPHYR_SUBSYS_NET_LIB_COAP_CLIENT_INTERNAL_H_

#include <errno.h>
#include <string.h>

#include <zephyr/net/coap.h>

/**
 * @brief Verify ETag consistency across the blocks of a block-wise response
 *
 * @rfc{7959,section-2.4} and @rfc{7959,section-2.6} require a client
 * reassembling a representation from a Block2 transfer to compare the ETag
 * option of each block when the server provides one. A change indicates the
 * blocks belong to different representations and reassembly must not
 * continue.
 *
 * On the first block the ETag (or its absence) is recorded. On subsequent
 * blocks it is compared against the recorded value; a change in value or
 * presence is a mismatch. Failing on a presence change is stricter than the
 * RFC's minimum, which only mandates comparing ETags the server provides: a
 * transfer mixing tagged and untagged blocks cannot be verified.
 *
 * @param response Response packet of the current block.
 * @param first_block True when this is the first block of a transfer.
 * @param etag Buffer of COAP_ETAG_MAX_LEN bytes holding the transfer's ETag.
 * @param etag_len Length of the stored ETag, 0 when the first block had none.
 *
 * @retval 0 ETag options are consistent so far.
 * @retval -EBADMSG ETag changed during the transfer.
 * @retval -EINVAL Response options could not be parsed.
 */
static inline int coap_client_check_etag(const struct coap_packet *response, bool first_block,
					 uint8_t *etag, uint8_t *etag_len)
{
	struct coap_option option = {0};
	int count;

	count = coap_find_options(response, COAP_OPTION_ETAG, &option, 1);
	if (count < 0) {
		return count;
	}

	/* RFC 7252: an option length outside the defined range is treated like
	 * an unrecognized option (section 5.4.3, stated for requests, applied
	 * to responses alike), and unrecognized elective options are silently
	 * ignored upon reception (section 5.4.1, not scoped to a direction).
	 */
	if (count > 0 && (option.len == 0U || option.len > COAP_ETAG_MAX_LEN)) {
		count = 0;
	}

	if (first_block) {
		*etag_len = (count > 0) ? option.len : 0U;
		memcpy(etag, option.value, *etag_len);
		return 0;
	}

	if (count == 0) {
		return (*etag_len == 0U) ? 0 : -EBADMSG;
	}

	if (option.len != *etag_len || memcmp(option.value, etag, option.len) != 0) {
		return -EBADMSG;
	}

	return 0;
}

#endif /* ZEPHYR_SUBSYS_NET_LIB_COAP_CLIENT_INTERNAL_H_ */
