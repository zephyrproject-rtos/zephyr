/*
 * Copyright (c) 2024 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
  * @file
  * @brief Header file providing the API for the DRV2605 haptic driver
  * @ingroup drv2605_interface
  */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HAPTICS_DRV2605_H_
#define ZEPHYR_INCLUDE_DRIVERS_HAPTICS_DRV2605_H_

#include <zephyr/drivers/haptics.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup drv2605_interface DRV2605
 * @ingroup haptics_interface_ext
 * @brief DRV2605 Haptic Driver for ERM and LRA
 * @{
 */

/**
 * @name Helpers
 * @{
 */

/** Maximum number of waveforms that can be stored in the sequencer */
#define DRV2605_WAVEFORM_SEQUENCER_MAX 8

/**
 * @brief Creates a wait/delay value for the waveform sequencer.
 *
 * @details This macro generates a byte value that, when placed in the drv2605_rom_data::seq_regs
 * array, instructs the DRV2605 playback engine to idle for a specified duration.
 *
 * @param ms The desired delay in milliseconds (rounded down to the nearest 10ms). Valid range is
 * 10 to 1270.
 * @return A byte literal representing the wait time for the sequencer.
 */
#define DRV2605_WAVEFORM_SEQUENCER_WAIT_MS(ms) (0x80 | ((ms) / 10))

/** @} */

/**
 * @brief Effect libraries
 *
 * This enumeration defines the different effect libraries that can be used with the DRV2605 when
 * using ROM source. TouchSense 2220 libraries are for open-loop ERM motors,
 * @ref DRV2605_LIBRARY_LRA is to be used for closed-loop LRA motors.
 */
enum drv2605_library {
	DRV2605_LIBRARY_EMPTY = 0,   /**< Empty library */
	DRV2605_LIBRARY_TS2200_A,    /**< TouchSense 2220 A library */
	DRV2605_LIBRARY_TS2200_B,    /**< TouchSense 2220 B library */
	DRV2605_LIBRARY_TS2200_C,    /**< TouchSense 2220 C library */
	DRV2605_LIBRARY_TS2200_D,    /**< TouchSense 2220 D library */
	DRV2605_LIBRARY_TS2200_E,    /**< TouchSense 2220 E library */
	DRV2605_LIBRARY_LRA,         /**< Linear Resonance Actuator (LRA) library */
};

/**
 * @brief Modes of operation
 *
 * @details This enumeration defines the different modes of operation supported by the DRV2605.
 *
 * See Table 5 of the DRV2605 datasheet for more information on the various operation modes.
 */
enum drv2605_mode {
	DRV2605_MODE_INTERNAL_TRIGGER = 0,    /**< Internal trigger mode */
	DRV2605_MODE_EXTERNAL_EDGE_TRIGGER,   /**< External trigger mode (edge) */
	DRV2605_MODE_EXTERNAL_LEVEL_TRIGGER,  /**< External trigger mode (level) */
	DRV2605_MODE_PWM_ANALOG_INPUT,        /**< PWM or Analog input mode */
	DRV2605_MODE_AUDIO_TO_VIBE,           /**< Audio-to-vibe mode */
	DRV2605_MODE_RTP,                     /**< RTP mode */
	DRV2605_MODE_DIAGNOSTICS,             /**< Diagnostics mode */
	DRV2605_MODE_AUTO_CAL,                /**< Auto-calibration mode */
};

/**
 * @brief Types of haptic signal sources.
 *
 * @details This enumeration defines the different types of haptic signal sources supported by the
 * DRV2605.
 */
enum drv2605_haptics_source {
	DRV2605_HAPTICS_SOURCE_ROM,      /**< Playback from the pre-programmed ROM library. */
	DRV2605_HAPTICS_SOURCE_RTP,      /**< Playback from Real-Time Playback (RTP) data stream. */
	DRV2605_HAPTICS_SOURCE_AUDIO,    /**< Playback is generated from an audio signal. */
	DRV2605_HAPTICS_SOURCE_PWM,      /**< Playback is driven by an external PWM signal. */
	DRV2605_HAPTICS_SOURCE_ANALOG,   /**< Playback is driven by an external analog signal. */
};

/**
 * @brief ROM data configuration
 *
 * @details This structure contains configuration data for when the DRV2605 is using the internal
 * ROM as the haptic source (@ref DRV2605_HAPTICS_SOURCE_ROM).
 */
struct drv2605_rom_data {
	/** Mode of operation for triggering the ROM effects. */
	enum drv2605_mode trigger;
	/** Effect library to use for playback. */
	enum drv2605_library library;
	/**
	 * @brief Waveform sequencer contents.
	 *
	 * This array contains the register values for the sequencer registers (0x04 to 0x0B).
	 * Each byte can describe either:
	 * - A waveform identifier (1 to 123) from the selected library to be played.
	 * - A wait time, if the MSB is set. The lower 7 bits are a multiple of 10 ms.
	 * Playback stops at the first zero entry.
	 *
	 * See Table 8 of the DRV2605 datasheet.
	 */
	uint8_t seq_regs[DRV2605_WAVEFORM_SEQUENCER_MAX];
	/**
	 * @brief Overdrive time offset.
	 *
	 * A signed 8-bit value that adds a time offset to the overdrive portion of the waveform.
	 * The offset is `overdrive_time * 5 ms`.
	 *
	 * See Table 10 of the DRV2605 datasheet.
	 */
	uint8_t overdrive_time;
	/**
	 * @brief Sustain positive time offset.
	 *
	 * A signed 8-bit value that adds a time offset to the positive sustain portion of the
	 * waveform. The offset is `sustain_pos_time * 5 ms`.
	 *
	 * See Table 11 of the DRV2605 datasheet.
	 */
	uint8_t sustain_pos_time;
	/**
	 * @brief Sustain negative time offset.
	 *
	 * A signed 8-bit value that adds a time offset to the negative sustain portion of the
	 * waveform. The offset is `sustain_neg_time * 5 ms`.
	 *
	 * See Table 12 of the DRV2605 datasheet.
	 */
	uint8_t sustain_neg_time;
	/**
	 * @brief Brake time offset.
	 *
	 * A signed 8-bit value that adds a time offset to the braking portion of the waveform.
	 * The offset is `brake_time * 5 ms`.
	 *
	 * See Table 13 of the DRV2605 datasheet.
	 */
	uint8_t brake_time;
};

/**
 * @brief Real-Time Playback (RTP) data configuration.
 *
 * @details This structure contains configuration data for when the DRV2605 is in RTP mode
 * (@ref DRV2605_HAPTICS_SOURCE_RTP). It allows for streaming custom haptic waveforms.
 */
struct drv2605_rtp_data {
	/** The number of entries in the @ref rtp_hold_us and @ref rtp_input arrays. */
	size_t size;
	/**
	 * @brief Pointer to an array of hold times.
	 *
	 * Each value specifies the duration in microseconds to hold the corresponding
	 * amplitude from the `rtp_input` array.
	 */
	uint32_t *rtp_hold_us;
	/**
	 * @brief Pointer to an array of RTP amplitude values.
	 *
	 * Each value is an 8-bit amplitude that will be written to the RTP input register (0x02).
	 * These values can be signed or unsigned depending on device configuration.
	 */
	uint8_t *rtp_input;
};

/**
 * @brief Configuration data union.
 *
 * @details This union holds a pointer to the specific configuration data struct required
 * for the selected haptic source. The correct member must be populated before calling
 * drv2605_haptic_config().
 */
union drv2605_config_data {
	/** Pointer to ROM configuration data. Use when source is @ref DRV2605_HAPTICS_SOURCE_ROM */
	struct drv2605_rom_data *rom_data;
	/** Pointer to RTP configuration data. Use when source is @ref DRV2605_HAPTICS_SOURCE_RTP */
	struct drv2605_rtp_data *rtp_data;
};

/**
 * @brief Configure the DRV2605 device for a particular signal source
 *
 * @param dev Pointer to the device structure for haptic device instance
 * @param source The type of haptic signal source desired
 * @param config_data Pointer to the configuration data union for the source
 *
 * @retval 0 success
 * @retval -ENOTSUP signal source not supported
 * @retval -errno another negative error code on failure
 */
int drv2605_haptic_config(const struct device *dev, enum drv2605_haptics_source source,
			  const union drv2605_config_data *config_data);


/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
