/*
 * Copyright (c) 2016, Intel Corporation
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

#include "vreg.h"

typedef enum { AON_VR = 0, PLAT3P3_VR, PLAT1P8_VR, HOST_VR, VREG_NUM } vreg_t;

QM_RW uint32_t *vreg[VREG_NUM] = {
    &QM_SCSS_PMU->aon_vr, &QM_SCSS_PMU->plat3p3_vr, &QM_SCSS_PMU->plat1p8_vr,
    &QM_SCSS_PMU->host_vr};

static int vreg_set_mode(const vreg_t id, const vreg_mode_t mode)
{
	QM_CHECK(mode < VREG_MODE_NUM, -EINVAL);
	uint32_t vr;

	vr = *vreg[id];

	switch (mode) {
	case VREG_MODE_SWITCHING:
		vr |= QM_SCSS_VR_EN;
		vr &= ~QM_SCSS_VR_VREG_SEL;
		break;
	case VREG_MODE_LINEAR:
		vr |= QM_SCSS_VR_EN;
		vr |= QM_SCSS_VR_VREG_SEL;
		break;
	case VREG_MODE_SHUTDOWN:
		vr &= ~QM_SCSS_VR_EN;
		break;
	default:
		break;
	}

	*vreg[id] = vr;

	while ((mode == VREG_MODE_SWITCHING) &&
	       (*vreg[id] & QM_SCSS_VR_ROK) == 0) {
	}

	return 0;
}

int vreg_aon_set_mode(const vreg_mode_t mode)
{
	QM_CHECK(mode < VREG_MODE_NUM, -EINVAL);
	QM_CHECK(mode != VREG_MODE_SWITCHING, -EINVAL);

	return vreg_set_mode(AON_VR, mode);
}

int vreg_plat3p3_set_mode(const vreg_mode_t mode)
{
	QM_CHECK(mode < VREG_MODE_NUM, -EINVAL);
	return vreg_set_mode(PLAT3P3_VR, mode);
}

int vreg_plat1p8_set_mode(const vreg_mode_t mode)
{
	QM_CHECK(mode < VREG_MODE_NUM, -EINVAL);
	return vreg_set_mode(PLAT1P8_VR, mode);
}

int vreg_host_set_mode(const vreg_mode_t mode)
{
	QM_CHECK(mode < VREG_MODE_NUM, -EINVAL);
	return vreg_set_mode(HOST_VR, mode);
}
