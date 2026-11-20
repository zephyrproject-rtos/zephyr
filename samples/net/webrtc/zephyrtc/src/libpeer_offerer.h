/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#ifndef ZEPHYRTC_ZEPHYR_OFFERER_H_
#define ZEPHYRTC_ZEPHYR_OFFERER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libpeer_endpoint.h"

struct libpeer_offerer_config {
	/* http://IPv4[:port]/path accepted by signaling_exchange(). */
	const char *url;
	/* Total signaling deadline. Zero selects CONFIG_ZEPHYRTC_SIGNALING_TIMEOUT_MS. */
	uint32_t timeout_ms;
	bool (*cancelled)(void *user_data);
	void *user_data;
};

/*
 * Negotiate one PCMU call as the SDP offerer: create the complete offer, POST it, require a
 * complete PCMU-only answer, and apply it. The remote description is not applied to libpeer
 * unless every signaling and profile check passes. On success the endpoint is in the checking
 * state and may be started. @p answer is scratch of at least SIGNALING_ANSWER_BUFFER_BYTES.
 *
 * Returns 0 or a negative errno: -EINVAL for caller or URL errors, -ECONNREFUSED when the peer
 * is unreachable, -ETIMEDOUT, -ECANCELED, -EIO for socket errors, -EBADMSG for an invalid HTTP
 * response or SDP profile, and -EPROTO when libpeer rejects the offer or answer.
 */
int libpeer_offerer_negotiate(struct libpeer_endpoint *endpoint,
			      const struct libpeer_offerer_config *config, char *answer,
			      size_t capacity);

/* True for exactly one audio section offering only payload type 0 (PCMU/8000). */
bool libpeer_sdp_is_pcmu_audio_only(const char *sdp);

/* True for exactly H.264 payload 96 followed by PCMU payload 0, both sendrecv. */
bool libpeer_sdp_is_h264_pcmu(const char *sdp);

#endif /* ZEPHYRTC_ZEPHYR_OFFERER_H_ */
