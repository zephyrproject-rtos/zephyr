/*
 * Copyright (c) 2017, Intel Corporation
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

/**
 * Calculate the register index for a specific pin.
 *
 * @param[in] pin The pin to be used.
 * @param[in] width The width in bits for each pin in the register.
 *
 * @return The register index of the given pin.
 */
static uint32_t pin_to_register(uint32_t pin, uint32_t width)
{
	return (pin / (32 / width));
}

/**
 * Calculate the offset for a pin within a register.
 *
 * @param[in] pin The pin to be used.
 * @param[in] width The width in bits for each pin in the register.
 *
 * @return The offset for the pin within the register.
 */
static uint32_t pin_to_offset(uint32_t pin, uint32_t width)
{
	return ((pin % (32 / width)) * width);
}

int qm_pmux_select(const qm_pin_id_t pin, const qm_pmux_fn_t fn)
{
	QM_CHECK(pin < QM_PIN_ID_NUM, -EINVAL);
	QM_CHECK(fn <= QM_PMUX_FN_3, -EINVAL);

	uint32_t reg = pin_to_register(pin, 2);
	uint32_t offs = pin_to_offset(pin, 2);

	QM_SCSS_PMUX->pmux_sel[reg] &= ~(MASK_2BIT << offs);
	QM_SCSS_PMUX->pmux_sel[reg] |= (fn << offs);

	return 0;
}

int qm_pmux_set_slew(const qm_pin_id_t pin, const qm_pmux_slew_t slew)
{
	QM_CHECK(pin < QM_PIN_ID_NUM, -EINVAL);
	QM_CHECK(slew < QM_PMUX_SLEW_NUM, -EINVAL);

	uint32_t reg = pin_to_register(pin, 1);
	uint32_t mask = MASK_1BIT << pin_to_offset(pin, 1);

	if (slew == 0) {
		QM_SCSS_PMUX->pmux_slew[reg] &= ~mask;
	} else {
		QM_SCSS_PMUX->pmux_slew[reg] |= mask;
	}
	return 0;
}

int qm_pmux_input_en(const qm_pin_id_t pin, const bool enable)
{
	QM_CHECK(pin < QM_PIN_ID_NUM, -EINVAL);

	uint32_t reg = pin_to_register(pin, 1);
	uint32_t mask = MASK_1BIT << pin_to_offset(pin, 1);

	if (enable == false) {
		QM_SCSS_PMUX->pmux_in_en[reg] &= ~mask;
	} else {
		QM_SCSS_PMUX->pmux_in_en[reg] |= mask;
	}
	return 0;
}

int qm_pmux_pullup_en(const qm_pin_id_t pin, const bool enable)
{
	QM_CHECK(pin < QM_PIN_ID_NUM, -EINVAL);

	uint32_t reg = pin_to_register(pin, 1);
	uint32_t mask = MASK_1BIT << pin_to_offset(pin, 1);

	if (enable == false) {
		QM_SCSS_PMUX->pmux_pullup[reg] &= ~mask;
	} else {
		QM_SCSS_PMUX->pmux_pullup[reg] |= mask;
	}
	return 0;
}
