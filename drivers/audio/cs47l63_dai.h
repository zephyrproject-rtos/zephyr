/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_dai.h
 * @brief CS47L63 ASP1 serial-port solver and apply path (internal)
 *
 * ASP1 is the port wired to the nRF's I2S. The nRF is always the bit-clock and
 * frame-sync master on this board, so this port is always a slave.
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_CS47L63_DAI_H_
#define ZEPHYR_DRIVERS_AUDIO_CS47L63_DAI_H_

#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief A complete ASP1 configuration.
 *
 * cs47l63_dai_solve() never leaves this half-written: on success every field is
 * ready to write unmodified, on error none of them mean anything.
 */
struct cs47l63_dai_solution {
	/** SAMPLE_RATEn code for the requested frame rate. */
	uint8_t rate_code;
	/** ASP1_FMT code for the requested DAI type. */
	uint8_t fmt;
	/** Slot length in bit-clock cycles, and word length in bits. */
	uint8_t slot_width;
	uint8_t word_len;
	/** ASP1_ENABLES1 value: which receive slots carry audio. */
	uint32_t enables;
};

/**
 * @brief Solve the ASP1 configuration for a class-API DAI request.
 *
 * A request for the codec to drive either clock is refused, not silently
 * corrected: two masters on one bus is a hardware fault, and the nRF is
 * already master.
 *
 * @param dai_type Digital interface type (audio_codec_cfg::dai_type)
 * @param i2s      I2S configuration (audio_codec_cfg::dai_cfg.i2s)
 * @param[out] out Solved configuration
 * @retval 0 on success
 * @retval -EINVAL if @p i2s or @p out is NULL
 * @retval -ENOTSUP if the part cannot express the request - an unsupported DAI
 *         type, a word size it has no encoding for, a frame rate with no
 *         rate-field code, a channel count other than one or two, or a request
 *         for the codec to be bit-clock or frame-sync master
 */
int cs47l63_dai_solve(audio_dai_type_t dai_type, const struct i2s_config *i2s,
		      struct cs47l63_dai_solution *out);

/**
 * @brief Write a solved ASP1 configuration.
 *
 * Also muxes the four GPIO pads that carry ASP1 to their serial-port function;
 * they come out of reset as GPIOs, so a port configured without this is
 * correct in every register and still silent on the wire.
 *
 * The global rate slot and the port's rate selector are written from the one
 * decision the solver made, never computed twice.
 *
 * @param dev Codec device
 * @param sol Solution from @ref cs47l63_dai_solve
 * @return 0 on success, negative errno propagated from the failing bus
 *         transaction
 */
int cs47l63_dai_apply(const struct device *dev, const struct cs47l63_dai_solution *sol);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_CS47L63_DAI_H_ */
