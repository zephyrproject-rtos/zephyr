/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio I2S codec sink element header file.
 */

#ifndef __MP_ZAUD_I2S_CODEC_SINK_H__
#define __MP_ZAUD_I2S_CODEC_SINK_H__

#include <zephyr/device.h>

#include <src/core/mp_sink.h>

/** @brief Cast object pointer to MpZaudI2sCodecSink pointer */
#define MP_ZAUD_I2S_CODEC_SINK(self) ((MpZaudI2sCodecSink *)self)

/**
 * @struct MpZaudI2sCodecSink
 * @brief Audio I2S codec sink element structure.
 *
 * This structure represents an audio sink element that outputs audio data
 * through an I2S interface to an external codec device. It manages the
 * I2S communication and codec control.
 */
typedef struct {
	/** Base sink element structure */
	MpSink sink;
	/** I2S device instance for audio data transmission */
	const struct device *i2s_dev;
	/** Codec device instance for configuration */
	const struct device *codec_dev;
	/** Memory slab for audio buffer allocation */
	struct k_mem_slab *mem_slab;
	/** Number of buffers written at the beginning of the stream */
	uint8_t count;
	/** Flag indicating if the sink has been started */
	bool started;
} MpZaudI2sCodecSink;

/**
 * @brief Initialize an audio I2S codec sink element.
 *
 * This function initializes the I2S codec sink element with default
 * values and sets up the function pointers.
 *
 * The function expects the following device tree nodes:
 * - i2s_codec_tx alias for I2S transmission device
 * - audio_codec node label for codec configuration device
 *
 * @param self Pointer to the MpElement structure to be initialized as an
 *             I2S codec sink element.
 *
 * @note This function will log errors and return early if required devices
 *       are not ready or not found in device tree.
 */
void mp_zaud_i2s_codec_sink_init(MpElement *self);

#endif /* __MP_ZAUD_I2S_CODEC_SINK_H__ */
