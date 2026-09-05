/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tlv320aic3104_dai.h"
#include "tlv320aic3104_regs.h"

#include <errno.h>

#include <zephyr/drivers/i2s.h>

int tlv320aic3104_dai_solve(audio_dai_type_t dai_type, uint8_t word_size, uint8_t i2s_options,
			    tlv320aic3104_dai_solution *out)
{
	uint8_t mode;
	uint8_t wordlen;
	uint8_t r8;

	if (out == NULL) {
		return -EINVAL;
	}

	switch (dai_type) {
	case AUDIO_DAI_TYPE_I2S:
		mode = ASI_CTRL_B_MODE_I2S;
		break;
	case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
		mode = ASI_CTRL_B_MODE_LEFT_JUSTIFIED;
		break;
	case AUDIO_DAI_TYPE_RIGHT_JUSTIFIED:
		mode = ASI_CTRL_B_MODE_RIGHT_JUSTIFIED;
		break;
	case AUDIO_DAI_TYPE_PCM:
		mode = ASI_CTRL_B_MODE_DSP;
		break;
	case AUDIO_DAI_TYPE_PCMA:
	case AUDIO_DAI_TYPE_PCMB:
	case AUDIO_DAI_TYPE_INVALID:
	default:
		return -ENOTSUP;
	}

	switch (word_size) {
	case 16:
		wordlen = ASI_CTRL_B_WORDLEN_16;
		break;
	case 20:
		wordlen = ASI_CTRL_B_WORDLEN_20;
		break;
	case 24:
		wordlen = ASI_CTRL_B_WORDLEN_24;
		break;
	case 32:
		wordlen = ASI_CTRL_B_WORDLEN_32;
		break;
	default:
		return -ENOTSUP;
	}

	r8 = ASI_CTRL_A_HIZ_DOUT;
	if ((i2s_options & I2S_OPT_BIT_CLK_TARGET) != 0) {
		r8 |= ASI_CTRL_A_BCLK_MASTER;
	}
	if ((i2s_options & I2S_OPT_FRAME_CLK_TARGET) != 0) {
		r8 |= ASI_CTRL_A_WCLK_MASTER;
	}

	out->r8_asi_ctrl_a = r8;
	out->r9_asi_ctrl_b = (uint8_t)(mode | wordlen);
	return 0;
}
