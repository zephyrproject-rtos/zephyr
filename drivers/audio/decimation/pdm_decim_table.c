/*
 * Copyright (c) 2018, Intel Corporation
 * All rights reserved.
 *
 * Author:	Seppo Ingalsuo <seppo.ingalsuo@linux.intel.com>
 *		Sathish Kuttan <sathish.k.kuttan@intel.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* PDM decimation FIR filters */

#include <zephyr/zephyr.h>

#include "pdm_decim_fir.h"

extern struct pdm_decim pdm_decim_int32_02_4375_5100_010_095;
extern struct pdm_decim pdm_decim_int32_02_4288_5100_010_095;
extern struct pdm_decim pdm_decim_int32_03_4375_5100_010_095;
extern struct pdm_decim pdm_decim_int32_03_3850_5100_010_095;
extern struct pdm_decim pdm_decim_int32_04_4375_5100_010_095;
extern struct pdm_decim pdm_decim_int32_05_4331_5100_010_095;
extern struct pdm_decim pdm_decim_int32_06_4156_5100_010_095;
extern struct pdm_decim pdm_decim_int32_08_4156_5380_010_090;

/* Note: Higher spec filter must be before lower spec filter
 * if there are multiple filters for a decimation factor. The naming
 * scheme of coefficients set is:
 * <type>_<decim factor>_<rel passband>_<rel stopband>_<ripple>_<attenuation>
 */
static struct pdm_decim *fir_list[DMIC_FIR_LIST_LENGTH] = {
	&pdm_decim_int32_02_4375_5100_010_095,
	&pdm_decim_int32_02_4288_5100_010_095,
	&pdm_decim_int32_03_4375_5100_010_095,
	&pdm_decim_int32_03_3850_5100_010_095,
	&pdm_decim_int32_04_4375_5100_010_095,
	&pdm_decim_int32_05_4331_5100_010_095,
	&pdm_decim_int32_06_4156_5100_010_095,
	&pdm_decim_int32_08_4156_5380_010_090,
};

struct pdm_decim **pdm_decim_get_fir_list(void)
{
	return fir_list;
}
