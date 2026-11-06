/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio definitions and utilities header file.
 * @ingroup mpipe_aud_utils
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_H_
#define ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_H_

/**
 * @defgroup mpipe_aud Audio
 * @ingroup mpipe_plugins
 * @brief Elements that capture, process and play back PCM audio.
 *
 * The audio plugin sits on Zephyr's audio APIs - DMIC for capture, I2S and the
 * audio codec API for playback - and describes what they carry as PCM
 * capabilities: a sample rate, a sample width, a channel count and the layout
 * of those channels within a buffer.
 *
 * Audio differs from video in what the buffer count means. A codec starved of
 * data produces an audible artifact rather than a dropped frame, so the number
 * of buffers is settled through the buffer pool query, where a sink can state
 * how much it must have primed before it starts.
 */

/**
 * @defgroup mpipe_aud_utils Utilities
 * @ingroup mpipe_aud
 * @brief Audio definitions and utility helpers.
 * @{
 */

#include <zephyr/audio/audio_caps.h>

#include <zephyr/mpipe/mpipe_structure.h>

/**
 * @name Supported sample rates (Hz)
 * @{
 */
/** @brief 8 kHz sample rate */
#define MPIPE_AUD_SAMPLE_RATE_8000  8000
/** @brief 16 kHz sample rate */
#define MPIPE_AUD_SAMPLE_RATE_16000 16000
/** @brief 32 kHz sample rate */
#define MPIPE_AUD_SAMPLE_RATE_32000 32000
/** @brief 44.1 kHz sample rate */
#define MPIPE_AUD_SAMPLE_RATE_44100 44100
/** @brief 48 kHz sample rate */
#define MPIPE_AUD_SAMPLE_RATE_48000 48000
/** @brief 96 kHz sample rate */
#define MPIPE_AUD_SAMPLE_RATE_96000 96000
/** @} */

/**
 * @name Supported bit widths
 * @{
 */
/** @brief 16 bit width */
#define MPIPE_AUD_BIT_WIDTH_16 16
/** @brief 24 bit width */
#define MPIPE_AUD_BIT_WIDTH_24 24
/** @brief 32 bit width */
#define MPIPE_AUD_BIT_WIDTH_32 32
/** @} */

/**
 * @brief Produce the capability a device advertises at an enumeration index.
 *
 * A device describes its sample rates and bit widths as two masks, and every
 * combination of them is a capability of its own. The enumeration index spans
 * both, so each capability is a plain fixed structure instead of one structure
 * holding two list values. An element whose capability comes from more than one
 * device passes what the devices agree on.
 *
 * @param caps Audio capabilities to enumerate.
 * @param index Zero-based enumeration index.
 * @param filter Capability the result must satisfy, may be NULL.
 * @param[out] out Pointer to storage for the capability at @p index.
 *
 * @retval 0 Success.
 * @retval -EAGAIN The capability at @p index cannot satisfy @p filter.
 * @retval -ENOENT @p index is past the last capability.
 * @retval -EINVAL @p caps is NULL.
 */
int mpipe_aud_enum_caps(const struct audio_caps *caps, uint32_t index,
			const struct mpipe_structure *filter, struct mpipe_structure *out);

/**
 * @brief Read a fixed unsigned field that a capability is required to carry.
 *
 * Audio elements configure their device from several caps fields at once and
 * cannot proceed if one is missing, so this reports the absent field rather
 * than leaving the caller to dereference nothing.
 *
 * @param caps Pointer to the capability to read from.
 * @param field_id Field identifier, see @ref mpipe_caps_field.
 * @param out Pointer to storage for the value, untouched unless 0 is returned.
 *
 * @retval 0 Success.
 * @retval -ENOENT The capability does not carry the field
 * @retval -EINVAL @p out is NULL or the field is not a fixed unsigned value
 */
int mpipe_aud_caps_get_uint(const struct mpipe_structure *caps, uint8_t field_id, uint32_t *out);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_H_ */
