/*
 * Copyright (c) 2015, Intel Corporation
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "qm_pinmux.h"
#include "qm_common.h"

#define MASK_1BIT (0x1)
#define MASK_2BIT (0x3)

static __inline__ uint8_t pin2reg(qm_pin_id_t pin, uint8_t bit_width)
{
	return (pin / (32 / bit_width));
}

static __inline__ uint8_t pin2reg_offs(qm_pin_id_t pin, uint8_t bit_width)
{
	return ((pin % (32 / bit_width)) * bit_width);
}

qm_rc_t qm_pmux_select(qm_pin_id_t pin, qm_pmux_fn_t fn)
{
	QM_CHECK(pin < QM_PIN_ID_NUM, QM_RC_EINVAL);
	QM_CHECK(fn <= QM_PMUX_FN_3, QM_RC_EINVAL);

	uint8_t reg = pin2reg(pin, 2);
	uint8_t offs = pin2reg_offs(pin, 2);

	QM_SCSS_PMUX->pmux_sel[reg] &= ~(MASK_2BIT << offs);
	QM_SCSS_PMUX->pmux_sel[reg] |= (fn << offs);

	return QM_RC_OK;
}

qm_rc_t qm_pmux_set_slew(qm_pin_id_t pin, qm_pmux_slew_t slew)
{
	QM_CHECK(pin < QM_PIN_ID_NUM, QM_RC_EINVAL);
	QM_CHECK(slew < QM_PMUX_SLEW_NUM, QM_RC_EINVAL);

	uint8_t reg = pin2reg(pin, 1);
	uint8_t offs = pin2reg_offs(pin, 1);

	QM_SCSS_PMUX->pmux_slew[reg] &= ~(MASK_1BIT << offs);
	QM_SCSS_PMUX->pmux_slew[reg] |= (slew << offs);

	return QM_RC_OK;
}

qm_rc_t qm_pmux_input_en(qm_pin_id_t pin, bool enable)
{
	QM_CHECK(pin < QM_PIN_ID_NUM, QM_RC_EINVAL);

	uint8_t reg = pin2reg(pin, 1);
	uint8_t offs = pin2reg_offs(pin, 1);

	enable &= MASK_1BIT;

	QM_SCSS_PMUX->pmux_in_en[reg] &= ~(MASK_1BIT << offs);
	QM_SCSS_PMUX->pmux_in_en[reg] |= (enable << offs);

	return QM_RC_OK;
}

qm_rc_t qm_pmux_pullup_en(qm_pin_id_t pin, bool enable)
{
	QM_CHECK(pin < QM_PIN_ID_NUM, QM_RC_EINVAL);

	uint8_t reg = pin2reg(pin, 1);
	uint8_t offs = pin2reg_offs(pin, 1);

	enable &= MASK_1BIT;

	QM_SCSS_PMUX->pmux_pullup[reg] &= ~(MASK_1BIT << offs);
	QM_SCSS_PMUX->pmux_pullup[reg] |= (enable << offs);

	return QM_RC_OK;
}
