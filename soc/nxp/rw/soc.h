/*
 * Copyright 2022-2023 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#ifndef _ASMLANGUAGE
#include <zephyr/sys/util.h>
#include <fsl_common.h>

/* Add include for DTS generated information */
#include <zephyr/devicetree.h>

#endif /* !_ASMLANGUAGE */


#ifdef CONFIG_MEMC
int flexspi_clock_set_freq(uint32_t clock_name, uint32_t rate);
#endif


#endif /* _SOC__H_ */
