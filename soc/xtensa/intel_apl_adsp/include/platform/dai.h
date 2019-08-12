/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2018 Intel Corporation. All rights reserved.
 *
 * Author: Marcin Maka <marcin.maka@linux.intel.com>
 */

#ifndef __PLATFORM_DAI_H__
#define __PLATFORM_DAI_H__

/* APOLLOLAKE */

/* SSP */

/*
 * Number of base and extended SSP ports must be defined separately
 * since some HW registers are in two groups, one for base and one
 * for extended.
 */

/** \brief Number of 'base' SSP ports available */
#define DAI_NUM_SSP_BASE	4

/** \brief Number of 'extended' SSP ports available */
#define DAI_NUM_SSP_EXT		2

/* HD/A */

/** \brief Number of HD/A Link Outputs */
#define DAI_NUM_HDA_OUT		6

/** \brief Number of HD/A Link Inputs */
#define DAI_NUM_HDA_IN		7

int dai_init(void);

#endif
