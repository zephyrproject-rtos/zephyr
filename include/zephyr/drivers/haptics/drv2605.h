/*
 * Copyright (c) 2024 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HAPTICS_DRV2605_H_
#define ZEPHYR_INCLUDE_DRIVERS_HAPTICS_DRV2605_H_

#include <zephyr/drivers/haptics.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRV2605_WAVEFORM_SEQUENCER_MAX 8

enum drv2605_library {
	DRV2605_LIBRARY_EMPTY = 0,
	DRV2605_LIBRARY_TS2200_A,
	DRV2605_LIBRARY_TS2200_B,
	DRV2605_LIBRARY_TS2200_C,
	DRV2605_LIBRARY_TS2200_D,
	DRV2605_LIBRARY_TS2200_E,
	DRV2605_LIBRARY_LRA,
};

enum drv2605_mode {
	DRV2605_MODE_INTERNAL_TRIGGER = 0,
	DRV2605_MODE_EXTERNAL_EDGE_TRIGGER,
	DRV2605_MODE_EXTERNAL_LEVEL_TRIGGER,
	DRV2605_MODE_PWM_ANALOG_INPUT,
	DRV2605_MODE_AUDIO_TO_VIBE,
	DRV2605_MODE_RTP,
	DRV2605_MODE_DIAGNOSTICS,
	DRV2605_MODE_AUTO_CAL,
};

/**
 * @brief DRV2605 haptic driver signal sources
 */
enum drv2605_haptics_source {
	/** The playback source is device ROM */
	DRV2605_HAPTICS_SOURCE_ROM,
	/** The playback source is the RTP buffer */
	DRV2605_HAPTICS_SOURCE_RTP,
	/** The playback source is audio */
	DRV2605_HAPTICS_SOURCE_AUDIO,
	/** The playback source is a PWM signal */
	DRV2605_HAPTICS_SOURCE_PWM,
	/** The playback source is an analog signal */
	DRV2605_HAPTICS_SOURCE_ANALOG,
};

struct drv2605_rom_data {
	enum drv2605_mode trigger;
	enum drv2605_library library;
	uint8_t seq_regs[DRV2605_WAVEFORM_SEQUENCER_MAX];
	uint8_t overdrive_time;
	uint8_t sustain_pos_time;
	uint8_t sustain_neg_time;
	uint8_t brake_time;
};

struct drv2605_rtp_data {
	size_t size;
	uint32_t *rtp_hold_us;
	uint8_t *rtp_input;
};

union drv2605_config_data {
	struct drv2605_rom_data *rom_data;
	struct drv2605_rtp_data *rtp_data;
};

/**
 * @brief Configure the DRV2605 device for a particular signal source
 *
 * @param dev Pointer to the device structure for haptic device instance
 * @param source The type of haptic signal source desired
 * @param config_data Pointer to the configuration data union for the source
 *
 * @retval 0 if successful
 * @retval -ENOTSUP if the signal source is not supported
 * @retval <0 if failed
 */
int drv2605_haptic_config(const struct device *dev, enum drv2605_haptics_source source,
			  const union drv2605_config_data *config_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
