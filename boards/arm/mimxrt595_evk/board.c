/*
 * Copyright 2022,  NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include "fsl_power.h"

static int mimxrt595_evk_init(const struct device *dev)
{
	/* Set the correct voltage range according to the board. */
	power_pad_vrange_t vrange = {
		.Vdde0Range = kPadVol_171_198,
		.Vdde1Range = kPadVol_171_198,
		.Vdde2Range = kPadVol_171_198,
		.Vdde3Range = kPadVol_300_360,
		.Vdde4Range = kPadVol_171_198
	};

	POWER_SetPadVolRange(&vrange);

	return 0;
}

SYS_INIT(mimxrt595_evk_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
