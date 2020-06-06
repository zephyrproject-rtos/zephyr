/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include "fsl_device_registers.h"

/* helper macro to convert from a DT_INST to HAL clock_ip_name */
#define INST_DT_CLOCK_IP_NAME(n) \
	MAKE_PCC_REGADDR(DT_REG_ADDR(DT_INST_PHANDLE(n, clocks)), \
			DT_INST_CLOCKS_CELL(n, name))

#endif /* _SOC__H_ */
