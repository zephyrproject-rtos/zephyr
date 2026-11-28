/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#include "libpeer_offerer.h"

#include <errno.h>
#include <string.h>

#include <peer_connection.h>

#include "platform.h"
#include "signaling_client_zephyr.h"

static bool line_has_prefix(const char *line, size_t length, const char *prefix)
{
	size_t prefix_length = strlen(prefix);

	return length >= prefix_length && memcmp(line, prefix, prefix_length) == 0;
}

static bool line_equals(const char *line, size_t length, const char *literal)
{
	return strlen(literal) == length && memcmp(line, literal, length) == 0;
}

static bool line_contains(const char *line, size_t length, const char *literal)
{
	size_t literal_length = strlen(literal);

	if (literal_length > length) {
		return false;
	}
	for (size_t offset = 0U; offset <= length - literal_length; offset++) {
		if (memcmp(line + offset, literal, literal_length) == 0) {
			return true;
		}
	}
	return false;
}

/* m=audio <port> UDP/TLS/RTP/SAVPF 0 */
static bool is_pcmu_media_line(const char *line, size_t length)
{
	static const char prefix[] = "m=audio ";
	static const char suffix[] = " UDP/TLS/RTP/SAVPF 0";
	uint32_t port = 0U;
	size_t port_length = 0U;
	size_t offset = sizeof(prefix) - 1U;

	if (!line_has_prefix(line, length, prefix)) {
		return false;
	}
	while (offset + port_length < length && line[offset + port_length] >= '0' &&
	       line[offset + port_length] <= '9') {
		port = port * 10U + (uint32_t)(line[offset + port_length] - '0');
		port_length++;
	}
	return port_length > 0U && port_length <= 5U && port > 0U && port <= UINT16_MAX &&
	       line_equals(line + offset + port_length, length - offset - port_length, suffix);
}

/* m=video <port> UDP/TLS/RTP/SAVPF 96 */
static bool is_h264_media_line(const char *line, size_t length)
{
	static const char prefix[] = "m=video ";
	static const char suffix[] = " UDP/TLS/RTP/SAVPF 96";
	uint32_t port = 0U;
	size_t port_length = 0U;
	size_t offset = sizeof(prefix) - 1U;

	if (!line_has_prefix(line, length, prefix)) {
		return false;
	}
	while (offset + port_length < length && line[offset + port_length] >= '0' &&
	       line[offset + port_length] <= '9') {
		port = port * 10U + (uint32_t)(line[offset + port_length] - '0');
		port_length++;
	}
	return port_length > 0U && port_length <= 5U && port > 0U && port <= UINT16_MAX &&
	       line_equals(line + offset + port_length, length - offset - port_length, suffix);
}

bool libpeer_sdp_is_pcmu_audio_only(const char *sdp)
{
	size_t media_sections = 0U;
	size_t pcmu_maps = 0U;
	bool in_audio = false;
	bool sendrecv = false;

	if (sdp == NULL || !signaling_sdp_is_complete(sdp, strlen(sdp))) {
		return false;
	}
	for (const char *line = sdp; *line != '\0';) {
		const char *end = strstr(line, "\r\n");
		size_t length = (size_t)(end - line);

		if (line_has_prefix(line, length, "m=")) {
			media_sections++;
			in_audio = media_sections == 1U && is_pcmu_media_line(line, length);
			if (!in_audio) {
				return false;
			}
		} else if (line_equals(line, length, "a=rtpmap:0 PCMU/8000")) {
			if (!in_audio || ++pcmu_maps > 1U) {
				return false;
			}
		} else if (line_has_prefix(line, length, "a=rtpmap:")) {
			return false;
		} else if (line_equals(line, length, "a=sendrecv")) {
			if (!in_audio || sendrecv) {
				return false;
			}
			sendrecv = true;
		} else if (line_equals(line, length, "a=inactive") ||
			   line_equals(line, length, "a=sendonly") ||
			   line_equals(line, length, "a=recvonly")) {
			return false;
		}
		line = end + 2;
	}
	return media_sections == 1U && pcmu_maps == 1U && sendrecv;
}

bool libpeer_sdp_is_h264_pcmu(const char *sdp)
{
	size_t media_sections = 0U;
	size_t h264_maps = 0U;
	size_t pcmu_maps = 0U;
	size_t h264_fmtp = 0U;
	bool video_sendrecv = false;
	bool audio_sendrecv = false;
	int section = 0;

	if (sdp == NULL || !signaling_sdp_is_complete(sdp, strlen(sdp))) {
		return false;
	}
	for (const char *line = sdp; *line != '\0';) {
		const char *end = strstr(line, "\r\n");
		size_t length = (size_t)(end - line);

		if (line_has_prefix(line, length, "m=")) {
			media_sections++;
			if (media_sections == 1U && is_h264_media_line(line, length)) {
				section = 1;
			} else if (media_sections == 2U && is_pcmu_media_line(line, length)) {
				section = 2;
			} else {
				return false;
			}
		} else if (line_equals(line, length, "a=rtpmap:96 H264/90000")) {
			if (section != 1 || ++h264_maps > 1U) {
				return false;
			}
		} else if (line_equals(line, length, "a=rtpmap:0 PCMU/8000")) {
			if (section != 2 || ++pcmu_maps > 1U) {
				return false;
			}
		} else if (line_has_prefix(line, length, "a=rtpmap:")) {
			return false;
		} else if (line_has_prefix(line, length, "a=fmtp:")) {
			if (section != 1 || !line_has_prefix(line, length, "a=fmtp:96 ") ||
			    !line_contains(line, length, "packetization-mode=1") ||
			    (!line_contains(line, length, "profile-level-id=42c0") &&
			     !line_contains(line, length, "profile-level-id=42e0")) ||
			    ++h264_fmtp > 1U) {
				return false;
			}
		} else if (line_equals(line, length, "a=sendrecv")) {
			if (section == 1 && !video_sendrecv) {
				video_sendrecv = true;
			} else if (section == 2 && !audio_sendrecv) {
				audio_sendrecv = true;
			} else {
				return false;
			}
		} else if (line_equals(line, length, "a=inactive") ||
			   line_equals(line, length, "a=sendonly") ||
			   line_equals(line, length, "a=recvonly")) {
			return false;
		}
		line = end + 2;
	}
	return media_sections == 2U && h264_maps == 1U && h264_fmtp == 1U && pcmu_maps == 1U &&
	       video_sendrecv && audio_sendrecv;
}

static int map_signaling_result(enum signaling_result result)
{
	switch (result) {
	case SIGNALING_OK:
		return 0;
	case SIGNALING_BAD_ARGUMENT:
	case SIGNALING_BAD_URL:
	case SIGNALING_BAD_OFFER:
		return -EINVAL;
	case SIGNALING_UNREACHABLE:
		return -ECONNREFUSED;
	case SIGNALING_TIMEOUT:
		return -ETIMEDOUT;
	case SIGNALING_CANCELLED:
		return -ECANCELED;
	case SIGNALING_IO_ERROR:
		return -EIO;
	default:
		return -EBADMSG;
	}
}

int libpeer_offerer_negotiate(struct libpeer_endpoint *endpoint,
			      const struct libpeer_offerer_config *config, char *answer,
			      size_t capacity)
{
	struct signaling_request request;
	enum signaling_result signaling;
	const char *offer;
	size_t answer_length;
	int result;

	if (endpoint == NULL || endpoint->connection == NULL || config == NULL ||
	    config->url == NULL || answer == NULL || capacity < SIGNALING_ANSWER_BUFFER_BYTES) {
		return -EINVAL;
	}
	offer = peer_connection_create_offer(endpoint->connection);
	if (offer == NULL || libpeer_endpoint_failed(endpoint)) {
		platform_log("error", "offerer", "step=create_offer status=fail");
		return -EPROTO;
	}

	request.url = config->url;
	request.offer = offer;
	request.timeout_ms = config->timeout_ms;
	request.cancelled = config->cancelled;
	request.user_data = config->user_data;
	signaling = signaling_exchange(&request, answer, capacity, &answer_length);
	result = map_signaling_result(signaling);
	if (result != 0) {
		platform_log("error", "offerer", "step=http_exchange status=%s",
			     signaling_result_str(signaling));
		return result;
	}
	platform_log("info", "offerer",
		     "step=http_answer status=ok offer_bytes=%zu answer_bytes=%zu", strlen(offer),
		     answer_length);

	if (endpoint->video_enabled ? !libpeer_sdp_is_h264_pcmu(answer)
				    : !libpeer_sdp_is_pcmu_audio_only(answer)) {
		platform_log("error", "offerer", "step=answer_profile status=reject");
		return -EBADMSG;
	}
	peer_connection_set_remote_description(endpoint->connection, answer, SDP_TYPE_ANSWER);
	if (libpeer_endpoint_failed(endpoint) ||
	    peer_connection_get_state(endpoint->connection) != PEER_CONNECTION_CHECKING) {
		platform_log("error", "offerer", "step=apply_answer status=fail");
		return -EPROTO;
	}
	platform_log("info", "offerer", "step=apply_answer status=ok");
	return 0;
}
