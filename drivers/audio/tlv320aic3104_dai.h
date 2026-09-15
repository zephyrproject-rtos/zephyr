/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_DAI_H_
#define ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_DAI_H_

#include <stdint.h>

#include <zephyr/audio/codec.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

	uint8_t r8_asi_ctrl_a;

	uint8_t r9_asi_ctrl_b;
} tlv320aic3104_dai_solution;

int tlv320aic3104_dai_solve(audio_dai_type_t dai_type, uint8_t word_size, uint8_t i2s_options,
			    tlv320aic3104_dai_solution *out);

#ifdef __cplusplus
}
#endif

#endif
