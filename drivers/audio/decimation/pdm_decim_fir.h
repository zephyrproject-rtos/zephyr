/*
 * Copyright (c) 2018, Intel Corporation
 * All rights reserved.
 *
 * Author:	Seppo Ingalsuo <seppo.ingalsuo@linux.intel.com>
 *		Sathish Kuttan <sathish.k.kuttan@intel.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __PDM_DECIM_FIR__H__
#define __PDM_DECIM_FIR__H__

#define DMIC_FIR_LIST_LENGTH 8

/* Format for generated coefficients tables */

struct pdm_decim {
	int decim_factor;
	int length;
	int shift;
	int relative_passband;
	int relative_stopband;
	int passband_ripple;
	int stopband_ripple;
	const s32_t *coef;
};

struct pdm_decim **pdm_decim_get_fir_list(void);

#endif /* __PDM_DECIM_FIR__H__ */
