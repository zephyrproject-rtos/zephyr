/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#include "libpeer_external.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#if defined(CONFIG_WIFI)
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#endif
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "libpeer_endpoint.h"
#include "libpeer_offerer.h"
#include "media.h"
#include "media_stats_zephyr.h"
#include "platform.h"
#include "signaling_client_zephyr.h"

#define EXTERNAL_CONNECT_TIMEOUT_MS         30000U
#define EXTERNAL_MIN_PACKETS_PER_S          40U
#define EXTERNAL_AUDIO_FRAME_TOLERANCE      10U
#define EXTERNAL_MIN_VIDEO_FRAMES_PER_S     4U
#define EXTERNAL_VIDEO_KEYFRAME_INTERVAL_MS 300000U

static struct libpeer_endpoint endpoint;
static char answer[SIGNALING_ANSWER_BUFFER_BYTES];
K_THREAD_STACK_DEFINE(endpoint_stack, ZEPHYRTC_ENDPOINT_STACK_SIZE);

static void audio_received(const uint8_t *data, size_t size, void *user_data)
{
	Media *media = user_data;
	MediaFrame frame = {
		.data = data,
		.size = size,
	};

	if (media_receive(media, MEDIA_AUDIO, &frame) != 0) {
		platform_log("error", "media_receive", "status=fail size=%zu", size);
	}
}

static void video_received(const uint8_t *data, size_t size, void *user_data)
{
	Media *media = user_data;
	MediaFrame frame = {
		.data = data,
		.size = size,
	};

	int result = media_receive(media, MEDIA_VIDEO, &frame);

	if (result != 0) {
		uint8_t nal_type = size > 4U ? data[4] & 0x1fU : 0U;

		platform_log("error", "video_receive", "status=fail result=%d nal_type=%u size=%zu",
			     result, nal_type, size);
	}
}

static void keyframe_requested(void *user_data)
{
	media_request_keyframe(user_data);
}

#if defined(CONFIG_WIFI)
static int start_wifi(void)
{
	struct net_if *iface = net_if_get_default();
	const size_t ssid_length = strlen(CONFIG_ZEPHYRTC_WIFI_SSID);
	const size_t password_length = strlen(CONFIG_ZEPHYRTC_WIFI_PASSWORD);
	struct wifi_connect_req_params params = {
		.ssid = (const uint8_t *)CONFIG_ZEPHYRTC_WIFI_SSID,
		.ssid_length = ssid_length,
		.psk = (const uint8_t *)CONFIG_ZEPHYRTC_WIFI_PASSWORD,
		.psk_length = password_length,
		.channel = WIFI_CHANNEL_ANY,
		.security =
			password_length == 0U ? WIFI_SECURITY_TYPE_NONE : WIFI_SECURITY_TYPE_PSK,
		.mfp = WIFI_MFP_OPTIONAL,
	};
	int64_t deadline = k_uptime_get() + CONFIG_ZEPHYRTC_NETWORK_TIMEOUT_MS;
	int result;

	if (iface == NULL || ssid_length == 0U || ssid_length > WIFI_SSID_MAX_LEN ||
	    (password_length != 0U && (password_length < 8U || password_length > 64U))) {
		platform_log("error", "wifi_connect", "status=invalid_config");
		return -EINVAL;
	}

	do {
		result = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
		if (result == 0) {
			platform_log("info", "wifi_connect", "status=requested");
			return 0;
		}
		k_sleep(K_MSEC(500));
	} while (k_uptime_get() < deadline);

	platform_log("error", "wifi_connect", "status=request_failed result=%d", result);
	return result;
}
#endif

static int wait_for_network(void)
{
	int64_t deadline = k_uptime_get() + CONFIG_ZEPHYRTC_NETWORK_TIMEOUT_MS;

	while (k_uptime_get() < deadline) {
		struct net_if *iface = net_if_get_default();
		struct net_in_addr *address = NULL;

		if (iface != NULL) {
			address = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED);
		}
		if (address != NULL) {
			struct net_in_addr netmask =
				net_if_ipv4_get_netmask_by_addr(iface, address);
			struct net_in_addr gateway = net_if_ipv4_get_gw(iface);
			char address_text[NET_IPV4_ADDR_LEN];
			char netmask_text[NET_IPV4_ADDR_LEN];
			char gateway_text[NET_IPV4_ADDR_LEN];

			(void)net_addr_ntop(NET_AF_INET, address, address_text,
					    sizeof(address_text));
			(void)net_addr_ntop(NET_AF_INET, &netmask, netmask_text,
					    sizeof(netmask_text));
			(void)net_addr_ntop(NET_AF_INET, &gateway, gateway_text,
					    sizeof(gateway_text));
			platform_log("info", "network_ready",
				     "iface=%d address=%s netmask=%s gateway=%s",
				     net_if_get_by_iface(iface), address_text, netmask_text,
				     gateway_text);
			return 0;
		}
		k_sleep(K_MSEC(100));
	}
	return -ENETDOWN;
}

int libpeer_external_run(void)
{
	const MediaConfig media_config = {
		.test_sources = !IS_ENABLED(CONFIG_ZEPHYRTC_AUDIO_RT1170),
		.fake_sinks = !IS_ENABLED(CONFIG_ZEPHYRTC_AUDIO_RT1170),
		.audio_only = !IS_ENABLED(CONFIG_ZEPHYRTC_VIDEO_H264),
	};
	const struct libpeer_offerer_config offerer_config = {
		.url = CONFIG_ZEPHYRTC_SIGNALING_URL,
		.timeout_ms = CONFIG_ZEPHYRTC_SIGNALING_TIMEOUT_MS,
	};
	const uint64_t minimum_frames =
		(uint64_t)CONFIG_ZEPHYRTC_EXTERNAL_RUN_MS * EXTERNAL_MIN_PACKETS_PER_S / 1000U;
	const uint64_t minimum_received_frames = minimum_frames - EXTERNAL_AUDIO_FRAME_TOLERANCE;
	const uint64_t minimum_video_frames =
		(uint64_t)CONFIG_ZEPHYRTC_EXTERNAL_RUN_MS * EXTERNAL_MIN_VIDEO_FRAMES_PER_S / 1000U;
	uint64_t submitted = 0U;
	uint64_t submitted_video = 0U;
	uint64_t delivered = 0U;
	uint64_t delivered_video = 0U;
	uint64_t deadline;
	uint64_t next_video_keyframe_request = UINT64_MAX;
	bool stack_initialized = false;
	Media *media = NULL;
	int result = -EIO;

#if defined(CONFIG_WIFI)
	result = start_wifi();
	if (result != 0) {
		goto cleanup;
	}
#endif
	result = wait_for_network();
	if (result != 0) {
		goto cleanup;
	}
	if (libpeer_stack_init() != 0) {
		goto cleanup;
	}
	stack_initialized = true;
	result = libpeer_endpoint_init(&endpoint, "external", 0U, 0U, false,
				       IS_ENABLED(CONFIG_ZEPHYRTC_VIDEO_H264));
	if (result != 0) {
		goto cleanup;
	}
	media = media_create(&media_config);
	if (media == NULL) {
		result = -ENOMEM;
		goto cleanup;
	}
	result = libpeer_endpoint_set_audio_sink(&endpoint, audio_received, media);
	if (result != 0) {
		goto cleanup;
	}
	if (media_enabled(media, MEDIA_VIDEO)) {
		result = libpeer_endpoint_set_video_sink(&endpoint, video_received,
							 keyframe_requested, media);
		if (result != 0) {
			goto cleanup;
		}
	}
	result = libpeer_offerer_negotiate(&endpoint, &offerer_config, answer, sizeof(answer));
	if (result != 0) {
		goto cleanup;
	}
	result = libpeer_endpoint_start(&endpoint, endpoint_stack,
					K_THREAD_STACK_SIZEOF(endpoint_stack));
	if (result != 0) {
		goto cleanup;
	}
	result = libpeer_endpoint_wait_connected(&endpoint, EXTERNAL_CONNECT_TIMEOUT_MS);
	if (result != 0) {
		goto cleanup;
	}

	if (media_start(media) != 0) {
		result = -EIO;
		goto cleanup;
	}
	deadline = platform_now_ms() + CONFIG_ZEPHYRTC_EXTERNAL_RUN_MS;
	if (media_enabled(media, MEDIA_VIDEO)) {
		next_video_keyframe_request =
			platform_now_ms() + EXTERNAL_VIDEO_KEYFRAME_INTERVAL_MS;
	}
	while (platform_now_ms() < deadline) {
		bool submitted_any = false;

		int media_result = media_poll(media);
		bool endpoint_failed = libpeer_endpoint_failed(&endpoint);

		if (media_result != 0 || endpoint_failed) {
			platform_log("error", "external_loop",
				     "status=fail media_result=%d endpoint_failed=%u state=%s",
				     media_result, endpoint_failed,
				     peer_connection_state_to_string(
					     peer_connection_get_state(endpoint.connection)));
			result = -EIO;
			goto cleanup;
		}
		if (media_take_keyframe_request(media)) {
			result = libpeer_endpoint_request_keyframe(&endpoint);
			platform_log(result == 0 ? "info" : "error", "video_keyframe_request",
				     "reason=decoder_failure status=%s result=%d",
				     result == 0 ? "sent" : "fail", result);
			if (result != 0) {
				goto cleanup;
			}
		}
		if (platform_now_ms() >= next_video_keyframe_request) {
			result = libpeer_endpoint_request_keyframe(&endpoint);
			platform_log(result == 0 ? "info" : "error", "video_keyframe_request",
				     "reason=periodic status=%s result=%d",
				     result == 0 ? "sent" : "fail", result);
			if (result != 0) {
				goto cleanup;
			}
			next_video_keyframe_request += EXTERNAL_VIDEO_KEYFRAME_INTERVAL_MS;
		}
		for (int kind = MEDIA_AUDIO; kind <= MEDIA_VIDEO; kind++) {
			MediaFrame frame;

			if (!media_enabled(media, (MediaKind)kind) ||
			    !media_pull(media, (MediaKind)kind, &frame)) {
				continue;
			}
			result = kind == MEDIA_AUDIO
					 ? libpeer_endpoint_send_audio(&endpoint, frame.data,
								       frame.size)
					 : libpeer_endpoint_send_video(&endpoint, frame.data,
								       frame.size);
			media_release(media, (MediaKind)kind);
			if (result != 0) {
				platform_log("error", "external_send", "kind=%s result=%d",
					     kind == MEDIA_AUDIO ? "audio" : "video", result);
				goto cleanup;
			}
			if (kind == MEDIA_AUDIO) {
				submitted++;
			} else {
				submitted_video++;
			}
			submitted_any = true;
		}
		if (!submitted_any) {
			platform_sleep_ms(1U);
		}
	}

	libpeer_endpoint_stop(&endpoint);
	result = media_zephyr_quiesce();
	if (result != 0) {
		goto cleanup;
	}
	delivered = media_decoded(media, MEDIA_AUDIO);
	delivered_video = media_decoded(media, MEDIA_VIDEO);
	result = submitted >= minimum_frames && delivered >= minimum_received_frames &&
				 atomic_get(&endpoint.received_frames) >=
					 (atomic_val_t)minimum_received_frames &&
				 atomic_get(&endpoint.invalid_frames) == 0 &&
				 media_zephyr_late_audio() == 0U &&
				 (!media_enabled(media, MEDIA_VIDEO) ||
				  (submitted_video >= minimum_video_frames &&
				   delivered_video >= minimum_video_frames &&
				   atomic_get(&endpoint.received_video_nalus) > 0))
			 ? 0
			 : -ENODATA;

cleanup:
	libpeer_endpoint_destroy(&endpoint);
	if (media != NULL) {
		int destroy_result = media_destroy(media);

		if (result == 0 && destroy_result != 0) {
			result = destroy_result;
		}
	}
	if (stack_initialized) {
		libpeer_stack_deinit();
	}
	platform_log(result == 0 ? "info" : "error", "external_result",
		     "status=%s result=%d submitted=%llu received=%ld delivered=%llu "
		     "invalid=%ld send_failures=%ld submitted_video=%llu "
		     "received_video_nalus=%ld "
		     "delivered_video=%llu",
		     result == 0 ? "pass" : "fail", result, (unsigned long long)submitted,
		     (long)atomic_get(&endpoint.received_frames), (unsigned long long)delivered,
		     (long)atomic_get(&endpoint.invalid_frames),
		     (long)atomic_get(&endpoint.send_failures), (unsigned long long)submitted_video,
		     (long)atomic_get(&endpoint.received_video_nalus),
		     (unsigned long long)delivered_video);
	return result;
}
