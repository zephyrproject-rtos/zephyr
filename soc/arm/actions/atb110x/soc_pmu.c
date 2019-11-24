/*
 * Copyright (c) 2019 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file PMU(vdd/vd12) interface for Actions SoC
 */

#include <kernel.h>
#include "soc_pmu.h"

inline void acts_request_vd12_pd(bool enable)
{
	if (enable)
		sys_write32(sys_read32(VD12_CTL) | (1 << VD12_CTL_VD12PD_EN),
			    VD12_CTL);
	else
		sys_write32(sys_read32(VD12_CTL) & ~(1 << VD12_CTL_VD12PD_EN),
			    VD12_CTL);
}

inline void acts_request_vd12_largebias(bool enable)
{
	if (enable)
		sys_write32(sys_read32(VD12_CTL) |
			    (1 << VD12_CTL_VD12_LGBIAS_EN), VD12_CTL);
	else
		sys_write32(sys_read32(VD12_CTL) &
			    ~(1 << VD12_CTL_VD12_LGBIAS_EN), VD12_CTL);
}

inline void acts_request_vd12(bool enable)
{
	/* VD12 always on */
	ARG_UNUSED(enable);
}

inline void acts_set_vdd(enum vdd_val val)
{
	sys_write32((sys_read32(VDD_CTL) & ~VDD_CTL_VDD_SET_MASK) | val,
		    VDD_CTL);
}
