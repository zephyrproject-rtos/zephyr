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

#include "rar.h"

#if (HAS_RAR)
int rar_set_mode(const rar_state_t mode)
{
	QM_CHECK(mode <= RAR_RETENTION, -EINVAL);
	volatile uint32_t i = 32;
	volatile uint32_t reg;

	switch (mode) {
	case RAR_RETENTION:
		QM_SCSS_PMU->aon_vr |=
		    (QM_AON_VR_PASS_CODE | QM_AON_VR_ROK_BUF_VREG_MASK);
		QM_SCSS_PMU->aon_vr |=
		    (QM_AON_VR_PASS_CODE | QM_AON_VR_VREG_SEL);
		break;

	case RAR_NORMAL:
		reg = QM_SCSS_PMU->aon_vr & ~QM_AON_VR_VREG_SEL;
		QM_SCSS_PMU->aon_vr = QM_AON_VR_PASS_CODE | reg;
		/* Wait for >= 2usec, at most 64 clock cycles. */
		while (i--) {
			__asm__ __volatile__("nop");
		}
		reg = QM_SCSS_PMU->aon_vr & ~QM_AON_VR_ROK_BUF_VREG_MASK;
		QM_SCSS_PMU->aon_vr = QM_AON_VR_PASS_CODE | reg;
		break;
	}
	return 0;
}
#endif
