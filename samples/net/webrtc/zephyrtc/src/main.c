/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#if defined(CONFIG_SOC_MIMXRT1176_CM7)
#include <fsl_soc_src.h>
#endif

#ifdef CONFIG_ARCH_POSIX
#include "posix_board_if.h"
#endif

#include "libpeer_transport_probe.h"
#include "media.h"
#include "media_stats_zephyr.h"
#include "platform.h"

#if defined(CONFIG_ZEPHYRTC_MODE_EXTERNAL)
#include "libpeer_external.h"
#elif defined(CONFIG_ZEPHYRTC_MODE_HTTP_SIGNALING)
#include "signaling_tests.h"
#else
#include "libpeer_loopback.h"
#endif

#define FNV_OFFSET        UINT32_C(2166136261)
#define FNV_PRIME         UINT32_C(16777619)
#define AUDIO_FRAME_BYTES 160U
#define AUDIO_DURATION_NS UINT64_C(20000000)

static uint32_t checksum_frame(uint32_t checksum, const MediaFrame *frame)
{
	for (size_t i = 0; i < frame->size; ++i) {
		checksum ^= frame->data[i];
		checksum *= FNV_PRIME;
	}
	return checksum;
}

static bool frame_is_active(const MediaFrame *frame)
{
	for (size_t i = 0; i < frame->size; ++i) {
		if (frame->data[i] != UINT8_C(0xff)) {
			return true;
		}
	}
	return false;
}

static int application_exit(int result)
{
#ifdef CONFIG_ARCH_POSIX
	posix_exit(result);
#endif
	return result;
}

int main(void)
{
	const MediaConfig config = {
		.test_sources = true,
		.fake_sinks = true,
		.audio_only = true,
	};
	const uint64_t minimum_frames = (uint64_t)CONFIG_ZEPHYRTC_DEMO_RUN_MS / 20U - 2U;
	uint64_t start;
	uint64_t deadline;
	uint64_t frames = 0U;
	uint64_t active_frames = 0U;
	uint64_t last_timestamp = 0U;
	uint32_t checksum = FNV_OFFSET;
	bool have_timestamp = false;
	int result = 0;
	Media *media;
#if defined(CONFIG_SOC_MIMXRT1176_CM7)
	const uint32_t reset_status = SRC_GetResetStatusFlags(SRC);
#endif

	platform_log("info", "native_sim_start", "run_ms=%d", CONFIG_ZEPHYRTC_DEMO_RUN_MS);
#if defined(CONFIG_SOC_MIMXRT1176_CM7)
	platform_log("info", "reset_status", "srsr=%08x", (unsigned int)reset_status);
	SRC_ClearGlobalSystemResetStatus(SRC, reset_status);
	platform_log("info", "reset_status_cleared", "srsr=%08x",
		     (unsigned int)SRC_GetResetStatusFlags(SRC));
#endif
	if (libpeer_transport_probe() != 0) {
		return application_exit(1);
	}
#if defined(CONFIG_ZEPHYRTC_MODE_EXTERNAL)
	if (libpeer_external_run() != 0) {
		return application_exit(1);
	}
	return application_exit(0);
#elif defined(CONFIG_ZEPHYRTC_MODE_HTTP_SIGNALING)
	if (libpeer_signaling_tests_run() != 0) {
		return application_exit(1);
	}
#else
	if (libpeer_loopback_run() != 0) {
		return application_exit(1);
	}
#endif
	media = media_create(&config);
	if (media == NULL || media_start(media) != 0) {
		platform_log("error", "media_start", "failed");
		return application_exit(1);
	}
	start = platform_now_ms();
	deadline = start + CONFIG_ZEPHYRTC_DEMO_RUN_MS;

	while (platform_now_ms() < deadline) {
		MediaFrame frame;

		if (media_poll(media) != 0) {
			platform_log("error", "media_poll", "failed");
			result = 1;
			break;
		}
		if (!media_pull(media, MEDIA_AUDIO, &frame)) {
			platform_sleep_ms(1U);
			continue;
		}
		if (frame.size != AUDIO_FRAME_BYTES || frame.duration_ns != AUDIO_DURATION_NS ||
		    (have_timestamp && frame.timestamp_ns <= last_timestamp)) {
			platform_log("error", "frame_contract",
				     "size=%zu timestamp=%" PRIu64 " duration=%" PRIu64, frame.size,
				     frame.timestamp_ns, frame.duration_ns);
			result = 1;
			media_release(media, MEDIA_AUDIO);
			break;
		}
		frames++;
		if (frame_is_active(&frame)) {
			active_frames++;
		}
		checksum = checksum_frame(checksum, &frame);
		last_timestamp = frame.timestamp_ns;
		have_timestamp = true;
		media_release(media, MEDIA_AUDIO);
	}

	if (frames < minimum_frames || active_frames < 15U ||
	    media_zephyr_generated_audio() != frames || media_zephyr_late_audio() != 0U ||
	    checksum == FNV_OFFSET) {
		result = 1;
	}
	platform_log(result == 0 ? "info" : "error", "native_sim_result",
		     "status=%s frames=%" PRIu64 " active=%" PRIu64 " late=%" PRIu64
		     " checksum=%08" PRIx32,
		     result == 0 ? "pass" : "fail", frames, active_frames,
		     media_zephyr_late_audio(), checksum);
	media_destroy(media);
	return application_exit(result);
}
