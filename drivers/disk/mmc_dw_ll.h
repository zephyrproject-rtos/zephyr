/*
 * Copyright (c) 2016-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MMC_DW_LL_H
#define MMC_DW_LL_H

#include "mmc_ll.h"

typedef struct dw_mmc_params {
	uintptr_t	reg_base;
	uintptr_t	desc_base;
	size_t		desc_size;
	int		clk_rate;
	int		bus_width;
	unsigned int	flags;
	enum mmc_device_type	mmc_dev_type;
} dw_mmc_params_t;

struct dw_idmac_desc {
	unsigned int	des0;
	unsigned int	des1;
	unsigned int	des2;
	unsigned int	des3;
};

void dw_mmc_init(dw_mmc_params_t *params, struct mmc_device_info *info);

#endif /* MMC_DW_LL_H */
