/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/class/usbh_uac2.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

/* Sine wave generation parameters */
#define SINE_TONE_FREQUENCY_HZ			1000
#define SINE_TONE_AMPLITUDE			16000

/* Host volume level range */
#define HOST_MIN_VOLUME				0
#define HOST_MAX_VOLUME				10

struct sine_wave_ctx {
	uint32_t sample_rate;
	uint8_t channels;
	uint8_t bytes_per_sample;
	double phase;
	double phase_increment;
};

static struct sine_wave_ctx sine_ctx;

/* Device capabilities */
struct device_caps {
	bool has_playback;
	bool has_capture;
};

/* Volume control state */
static uint8_t g_host_cur_volume = 4;

/* Wait for audio device connection */
static void wait_for_audio_connection(const struct device *uac2_dev, struct device_caps *caps)
{
	uint32_t sample_rate;
	uint8_t channels;
	uint8_t bit_resolution;
	int ret_out, ret_in;

	while (true) {
		ret_out = usbh_uac2_get_format(uac2_dev, USBH_PLAYBACK,
					       &sample_rate, &channels, &bit_resolution);
		ret_in = usbh_uac2_get_format(uac2_dev, USBH_CAPTURE,
					      &sample_rate, &channels, &bit_resolution);

		if (ret_out == 0 || ret_in == 0) {
			caps->has_playback = (ret_out == 0);
			caps->has_capture = (ret_in == 0);
			LOG_INF("Audio device connected!");
			return;
		}

		k_sleep(K_MSEC(10));
	}
}

/* Get and display sample rate range */
static void log_sample_rate_range(const struct device *uac2_dev,
				   enum usbh_uac2_stream_dir dir)
{
	struct uac2_ctrl_query query = {
		.id = AUDIO_CID_SAMPLE_RATE,
	};
	const char *indent;
	bool is_discrete;
	uint16_t i;
	int ret;

	ret = usbh_uac2_ctrl_query(uac2_dev, dir, 0, &query);
	if (ret != 0 || query.range.num_subranges == 0) {
		return;
	}

	LOG_INF("  Sample Rate Range (%s):",
		dir == USBH_CAPTURE ? "Capture" : "Playback");

	if (query.range.num_subranges > 1) {
		LOG_INF("    %u supported rate(s):", query.range.num_subranges);
	}

	for (i = 0; i < query.range.num_subranges; i++) {
		indent = (query.range.num_subranges > 1) ? "      " : "    ";
		is_discrete = (query.range.subranges[i].umin32 ==
			       query.range.subranges[i].umax32);

		if (is_discrete) {
			/* Discrete value */
			if (query.range.num_subranges == 1) {
				LOG_INF("%s%u Hz (fixed)", indent,
					query.range.subranges[i].umin32);
			} else {
				LOG_INF("%s[%u] %u Hz", indent, i,
					query.range.subranges[i].umin32);
			}
		} else {
			/* Continuous range */
			if (query.range.num_subranges == 1) {
				LOG_INF("%sMIN: %u Hz", indent,
					query.range.subranges[i].umin32);
				LOG_INF("%sMAX: %u Hz", indent,
					query.range.subranges[i].umax32);
				LOG_INF("%sRES: %u Hz", indent,
					query.range.subranges[i].ustep32);
			} else {
				LOG_INF("%s[%u] %u - %u Hz (step %u)", indent, i,
					query.range.subranges[i].umin32,
					query.range.subranges[i].umax32,
					query.range.subranges[i].ustep32);
			}
		}
	}
}

/* Get and display volume range */
static void log_volume_range(const struct device *uac2_dev,
			      enum usbh_uac2_stream_dir dir)
{
	struct uac2_ctrl_query query = {
		.id = AUDIO_CID_VOLUME,
	};
	const char *indent;
	int16_t min_db;
	int16_t max_db;
	int16_t min_frac;
	int16_t max_frac;
	uint16_t i;
	int ret;

	ret = usbh_uac2_ctrl_query(uac2_dev, dir, USBH_AUDIO_CHANNEL_MASTER, &query);
	if (ret == -ENOTSUP) {
		LOG_INF("  Volume control not supported (%s)",
			dir == USBH_CAPTURE ? "Capture" : "Playback");
		return;
	}

	if (ret != 0 || query.range.num_subranges == 0) {
		return;
	}

	LOG_INF("  Volume Range (%s):",
		dir == USBH_CAPTURE ? "Capture" : "Playback");

	if (query.range.num_subranges > 1) {
		LOG_INF("    %u subrange(s):", query.range.num_subranges);
	}

	for (i = 0; i < query.range.num_subranges; i++) {
		indent = (query.range.num_subranges > 1) ? "      " : "    ";
		min_db = query.range.subranges[i].min16 / 256;
		max_db = query.range.subranges[i].max16 / 256;
		min_frac = (abs(query.range.subranges[i].min16 % 256) * 100) / 256;
		max_frac = (abs(query.range.subranges[i].max16 % 256) * 100) / 256;

		if (query.range.num_subranges == 1) {
			/* Single range (typical) */
			LOG_INF("%sMIN: %d.%02d dB (0x%04x)", indent,
				min_db, min_frac,
				query.range.subranges[i].min16);
			LOG_INF("%sMAX: %d.%02d dB (0x%04x)", indent,
				max_db, max_frac,
				query.range.subranges[i].max16);
			LOG_INF("%sRES: %d/256 dB (0x%04x)", indent,
				query.range.subranges[i].step16,
				query.range.subranges[i].step16);
		} else {
			/* Multiple ranges (rare) */
			LOG_INF("%s[%u] %d.%02d - %d.%02d dB (step %d/256)", indent, i,
				min_db, min_frac,
				max_db, max_frac,
				query.range.subranges[i].step16);
		}
	}
}

/* Set initial volume */
static int set_initial_volume(const struct device *uac2_dev,
			       enum usbh_uac2_stream_dir dir)
{
	struct uac2_ctrl_query query = {
		.id = AUDIO_CID_VOLUME,
	};
	struct uac2_control ctrl_set;
	struct uac2_control ctrl_get;
	int16_t min_volume, max_volume, volume_step;
	int16_t target_volume;
	int32_t db_int, db_frac;
	int ret;

	ret = usbh_uac2_ctrl_query(uac2_dev, dir, USBH_AUDIO_CHANNEL_MASTER, &query);
	if (ret == -ENOTSUP) {
		LOG_INF("%s volume control not supported, skipping",
			dir == USBH_CAPTURE ? "Capture" : "Playback");
		return 0;
	} else if (ret != 0) {
		LOG_WRN("Failed to get volume range: %d", ret);
		return ret;
	}

	min_volume = query.range.subranges[0].min16;
	max_volume = query.range.subranges[0].max16;
	volume_step = (max_volume - min_volume) / (HOST_MAX_VOLUME - HOST_MIN_VOLUME);
	target_volume = min_volume + (volume_step * g_host_cur_volume);

	db_int = target_volume / 256;
	db_frac = (abs(target_volume % 256) * 100) / 256;

	LOG_INF("Setting %s volume to level %u/%u (%d.%02d dB)",
		dir == USBH_CAPTURE ? "Capture" : "Playback",
		g_host_cur_volume, HOST_MAX_VOLUME,
		db_int, db_frac);

	ctrl_set.id = AUDIO_CID_VOLUME;
	ctrl_set.val16 = target_volume;

	ret = usbh_uac2_ctrl_set(uac2_dev, dir, USBH_AUDIO_CHANNEL_MASTER, &ctrl_set);
	if (ret != 0) {
		LOG_ERR("Failed to set master volume: %d", ret);
		return ret;
	}

	ctrl_get.id = AUDIO_CID_VOLUME;
	ret = usbh_uac2_ctrl_get(uac2_dev, dir, USBH_AUDIO_CHANNEL_MASTER, &ctrl_get);
	if (ret == 0) {
		db_int = ctrl_get.val16 / 256;
		db_frac = (abs(ctrl_get.val16 % 256) * 100) / 256;

		LOG_INF("Current %s volume: %d.%02d dB",
			dir == USBH_CAPTURE ? "Capture" : "Playback",
			db_int, db_frac);
	}

	return 0;
}

/* Unmute audio */
static int unmute_audio(const struct device *uac2_dev,
			enum usbh_uac2_stream_dir dir)
{
	struct uac2_control ctrl = {
		.id = AUDIO_CID_MUTE,
		.uval8 = 0,
	};
	int ret;

	ret = usbh_uac2_ctrl_set(uac2_dev, dir, USBH_AUDIO_CHANNEL_MASTER, &ctrl);
	if (ret == -ENOTSUP) {
		LOG_INF("%s mute control not supported, skipping",
			dir == USBH_CAPTURE ? "Capture" : "Playback");
		return 0;
	} else if (ret == 0) {
		LOG_INF("%s audio unmuted",
			dir == USBH_CAPTURE ? "Capture" : "Playback");
	}

	return ret;
}

/* Print audio device information */
static void log_audio_info(const struct device *uac2_dev, const struct device_caps *caps)
{
	uint32_t sample_rate;
	uint8_t channels;
	uint8_t bit_resolution;
	int ret;

	LOG_INF("=== Audio Device Information ===");

	LOG_INF("Device Capabilities:");
	if (caps->has_capture && caps->has_playback) {
		LOG_INF("  Type: Full-Duplex (Capture + Playback)");
	} else if (caps->has_capture) {
		LOG_INF("  Type: Capture Only (Microphone/Line-In)");
	} else if (caps->has_playback) {
		LOG_INF("  Type: Playback Only (Headphones/Speakers)");
	}

	if (caps->has_capture) {
		ret = usbh_uac2_get_format(uac2_dev, USBH_CAPTURE,
					   &sample_rate, &channels, &bit_resolution);
		if (ret == 0) {
			LOG_INF("Capture Format:");
			LOG_INF("  Sample Rate: %u Hz", sample_rate);
			LOG_INF("  Channels: %u", channels);
			LOG_INF("  Bit Resolution: %u bits", bit_resolution);

			log_sample_rate_range(uac2_dev, USBH_CAPTURE);
			log_volume_range(uac2_dev, USBH_CAPTURE);
		}
	}

	if (caps->has_playback) {
		ret = usbh_uac2_get_format(uac2_dev, USBH_PLAYBACK,
					   &sample_rate, &channels, &bit_resolution);
		if (ret == 0) {
			LOG_INF("Playback Format:");
			LOG_INF("  Sample Rate: %u Hz", sample_rate);
			LOG_INF("  Channels: %u", channels);
			LOG_INF("  Bit Resolution: %u bits", bit_resolution);

			log_sample_rate_range(uac2_dev, USBH_PLAYBACK);
			log_volume_range(uac2_dev, USBH_PLAYBACK);
		}
	}
}

/* Initialize sine wave generator */
static void init_sine_wave(struct sine_wave_ctx *ctx,
			   uint32_t sample_rate,
			   uint8_t channels,
			   uint8_t bit_resolution)
{
	ctx->sample_rate = sample_rate;
	ctx->channels = channels;
	ctx->bytes_per_sample = (bit_resolution + 7) / 8;
	ctx->phase = 0.0;
	ctx->phase_increment = 2.0 * 3.14159265358979323846 * SINE_TONE_FREQUENCY_HZ / sample_rate;
}

/* Generate sine wave samples */
static uint16_t generate_sine_wave(uint8_t *buffer, uint16_t max_len, void *user_data)
{
	struct sine_wave_ctx *ctx = (struct sine_wave_ctx *)user_data;
	uint16_t bytes_per_frame = ctx->channels * ctx->bytes_per_sample;
	uint16_t num_frames = max_len / bytes_per_frame;
	uint16_t actual_len = num_frames * bytes_per_frame;
	int16_t *samples_16 = (int16_t *)buffer;
	uint16_t sample_idx = 0;
	double sine_value;
	int16_t sample;

	if (ctx->bytes_per_sample != 2) {
		memset(buffer, 0, actual_len);
		return actual_len;
	}

	for (uint16_t frame = 0; frame < num_frames; frame++) {
		sine_value = sin(ctx->phase);
		sample = (int16_t)(sine_value * SINE_TONE_AMPLITUDE);

		for (uint8_t ch = 0; ch < ctx->channels; ch++) {
			samples_16[sample_idx++] = sample;
		}

		ctx->phase += ctx->phase_increment;
		if (ctx->phase >= 2.0 * 3.14159265358979323846) {
			ctx->phase -= 2.0 * 3.14159265358979323846;
		}
	}

	return actual_len;
}

static uint16_t audio_in_callback(uint8_t *data, uint16_t len, void *user_data)
{
	static uint32_t packet_count;

	ARG_UNUSED(user_data);

	packet_count++;

	if (packet_count % 100 == 0) {
		LOG_DBG("Received audio packet #%u, %u bytes", packet_count, len);
	}

	return 0;
}

static int test_sample_rates(const struct device *uac2_dev,
			     const struct device_caps *caps)
{
	const uint32_t test_rates[] = {8000, 16000, 44100, 48000, 96000};
	struct uac2_ctrl_query query = {
		.id = AUDIO_CID_SAMPLE_RATE,
	};
	bool supported;
	uint16_t i, j;
	int ret;

	LOG_INF("=== Testing Sample Rate Support ===");

	/* Test Playback */
	if (caps->has_playback) {
		ret = usbh_uac2_ctrl_query(uac2_dev, USBH_PLAYBACK, 0, &query);
		if (ret != 0 || query.range.num_subranges == 0) {
			goto test_capture;
		}

		LOG_INF("Playback Sample Rates:");

		for (i = 0; i < ARRAY_SIZE(test_rates); i++) {
			supported = false;

			for (j = 0; j < query.range.num_subranges; j++) {
				/* Check discrete value */
				if (query.range.subranges[j].umin32 ==
				    query.range.subranges[j].umax32) {
					if (test_rates[i] == query.range.subranges[j].umin32) {
						supported = true;
						break;
					}
					continue;
				}

				/* Check continuous range */
				if (test_rates[i] >= query.range.subranges[j].umin32 &&
				    test_rates[i] <= query.range.subranges[j].umax32) {
					supported = true;
					break;
				}
			}

			LOG_INF("  %u Hz: %s", test_rates[i],
				supported ? "Supported" : "Not supported");
		}
	}

test_capture:
	/* Test Capture */
	if (caps->has_capture) {
		ret = usbh_uac2_ctrl_query(uac2_dev, USBH_CAPTURE, 0, &query);
		if (ret != 0 || query.range.num_subranges == 0) {
			return 0;
		}

		LOG_INF("Capture Sample Rates:");

		for (i = 0; i < ARRAY_SIZE(test_rates); i++) {
			supported = false;

			for (j = 0; j < query.range.num_subranges; j++) {
				/* Check discrete value */
				if (query.range.subranges[j].umin32 ==
				    query.range.subranges[j].umax32) {
					if (test_rates[i] == query.range.subranges[j].umin32) {
						supported = true;
						break;
					}
					continue;
				}

				/* Check continuous range */
				if (test_rates[i] >= query.range.subranges[j].umin32 &&
				    test_rates[i] <= query.range.subranges[j].umax32) {
					supported = true;
					break;
				}
			}

			LOG_INF("  %u Hz: %s", test_rates[i],
				supported ? "Supported" : "Not supported");
		}
	}

	return 0;
}

static int configure_audio_format(const struct device *uac2_dev,
				   const struct device_caps *caps)
{
	struct uac2_control ctrl = {
		.id = AUDIO_CID_SAMPLE_RATE,
	};
	int ret;

	LOG_INF("=== Configuring Audio Format ===");

	if (caps->has_playback) {
		ctrl.uval32 = 48000;
		ret = usbh_uac2_ctrl_set(uac2_dev, USBH_PLAYBACK, 0, &ctrl);
		if (ret != 0) {
			LOG_ERR("Failed to set playback sample rate to 48000 Hz: %d", ret);
			return ret;
		}

		ret = usbh_uac2_ctrl_get(uac2_dev, USBH_PLAYBACK, 0, &ctrl);
		if (ret == 0) {
			LOG_INF("  Playback sample rate: %u Hz", ctrl.uval32);
		}

		set_initial_volume(uac2_dev, USBH_PLAYBACK);
		unmute_audio(uac2_dev, USBH_PLAYBACK);
	}

	if (caps->has_capture) {
		ctrl.uval32 = 48000;
		ret = usbh_uac2_ctrl_set(uac2_dev, USBH_CAPTURE, 0, &ctrl);
		if (ret == 0) {
			ret = usbh_uac2_ctrl_get(uac2_dev, USBH_CAPTURE, 0, &ctrl);
			if (ret == 0) {
				LOG_INF("  Capture sample rate: %u Hz", ctrl.uval32);
			}

			set_initial_volume(uac2_dev, USBH_CAPTURE);
			unmute_audio(uac2_dev, USBH_CAPTURE);
		}
	}

	return 0;
}

static int start_audio_playback(const struct device *uac2_dev,
				const struct device_caps *caps)
{
	uint32_t sample_rate;
	uint8_t channels;
	uint8_t bit_resolution;
	int ret;

	if (!caps->has_playback) {
		LOG_INF("Device does not support playback");
		return -ENOTSUP;
	}

	LOG_INF("=== Starting Audio Playback ===");

	ret = usbh_uac2_get_format(uac2_dev, USBH_PLAYBACK,
				   &sample_rate, &channels, &bit_resolution);
	if (ret != 0) {
		LOG_ERR("Failed to get audio format: %d", ret);
		return ret;
	}

	init_sine_wave(&sine_ctx, sample_rate, channels, bit_resolution);

	ret = usbh_uac2_start_stream_out(uac2_dev, generate_sine_wave, &sine_ctx);
	if (ret != 0) {
		LOG_ERR("Failed to start audio OUT stream: %d", ret);
		return ret;
	}

	LOG_INF("Playback started - playing %d Hz sine wave continuously", SINE_TONE_FREQUENCY_HZ);
	return 0;
}

static int stop_audio_playback(const struct device *uac2_dev,
			       const struct device_caps *caps)
{
	int ret;

	if (!caps->has_playback) {
		return 0;
	}

	LOG_INF("Stopping playback...");

	ret = usbh_uac2_stop_stream_out(uac2_dev);
	if (ret != 0 && ret != -ENODEV) {
		LOG_ERR("Failed to stop audio OUT stream: %d", ret);
		return ret;
	}

	LOG_INF("Playback stopped");
	return 0;
}

static int start_audio_capture(const struct device *uac2_dev,
			       const struct device_caps *caps)
{
	int ret;

	if (!caps->has_capture) {
		return 0;
	}

	LOG_INF("=== Starting Audio Capture ===");

	ret = usbh_uac2_start_stream_in(uac2_dev, audio_in_callback, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to start audio IN stream: %d", ret);
		return ret;
	}

	LOG_INF("Capture started - recording audio data");
	return 0;
}

static int stop_audio_capture(const struct device *uac2_dev,
			      const struct device_caps *caps)
{
	int ret;

	if (!caps->has_capture) {
		return 0;
	}

	LOG_DBG("Stopping capture...");

	ret = usbh_uac2_stop_stream_in(uac2_dev);
	if (ret != 0 && ret != -ENODEV) {
		LOG_ERR("Failed to stop audio IN stream: %d", ret);
		return ret;
	}

	LOG_DBG("Capture stopped");
	return 0;
}

/* Monitor playback and check for device disconnection */
static int monitor_connection(const struct device *uac2_dev,
			      const struct device_caps *caps)
{
	uint32_t elapsed_seconds = 0;
	uint32_t sample_rate;
	uint8_t channels;
	uint8_t bit_resolution;
	int ret;

	while (true) {
		k_sleep(K_SECONDS(1));
		elapsed_seconds++;

		if (caps->has_playback) {
			ret = usbh_uac2_get_format(uac2_dev, USBH_PLAYBACK,
						   &sample_rate, &channels, &bit_resolution);
		} else if (caps->has_capture) {
			ret = usbh_uac2_get_format(uac2_dev, USBH_CAPTURE,
						   &sample_rate, &channels, &bit_resolution);
		} else {
			ret = -ENODEV;
		}

		if (ret == -ENODEV) {
			return ret;
		} else if (ret != 0) {
			stop_audio_playback(uac2_dev, caps);
			stop_audio_capture(uac2_dev, caps);
			LOG_ERR("Error checking device status: %d", ret);
			return ret;
		}

		if (elapsed_seconds % 10 == 0) {
			if (caps->has_playback) {
				LOG_INF("Playback: %u seconds elapsed",
					elapsed_seconds);
			}
			if (caps->has_capture) {
				LOG_INF("Capture: %u seconds elapsed",
					elapsed_seconds);
			}
		}
	}

	return 0;
}

int main(void)
{
	const struct device *uac2_dev = device_get_binding("usbh_uac2_0");
	struct device_caps caps;
	int err;

	LOG_INF("===========================================");
	LOG_INF("USB Audio Class 2.0 Host Sample Application");
	LOG_INF("===========================================");

	if (uac2_dev == NULL) {
		LOG_ERR("USB host audio device not found");
		return -ENODEV;
	}

	if (!device_is_ready(uac2_dev)) {
		LOG_ERR("USB host audio device is not ready");
		return -ENODEV;
	}

	err = usbh_init(&uhs_ctx);
	if (err) {
		LOG_ERR("Failed to initialize USB host support: %d", err);
		return err;
	}

	err = usbh_enable(&uhs_ctx);
	if (err) {
		LOG_ERR("Failed to enable USB host support: %d", err);
		return err;
	}

	err = uhc_sof_enable(uhs_ctx.dev);
	if (err) {
		LOG_ERR("Failed to start SoF");
		return err;
	}

	LOG_INF("USB host audio device is ready");

	while (true) {
		LOG_INF("Waiting for USB audio device to connect...");
		LOG_INF("Please connect a USB Audio Class 2.0 device");
		wait_for_audio_connection(uac2_dev, &caps);

		log_audio_info(uac2_dev, &caps);
		test_sample_rates(uac2_dev, &caps);

		err = configure_audio_format(uac2_dev, &caps);
		if (err != 0) {
			LOG_ERR("Failed to configure audio format");
			k_sleep(K_MSEC(50));
			continue;
		}

		err = start_audio_playback(uac2_dev, &caps);
		if (err != 0) {
			LOG_ERR("Failed to start audio playback");
			k_sleep(K_MSEC(50));
			continue;
		}

		err = start_audio_capture(uac2_dev, &caps);
		if (err != 0) {
			LOG_ERR("Failed to start audio recording");
			k_sleep(K_MSEC(50));
			continue;
		}

		monitor_connection(uac2_dev, &caps);

		k_sleep(K_MSEC(200));
	}

	return 0;
}
