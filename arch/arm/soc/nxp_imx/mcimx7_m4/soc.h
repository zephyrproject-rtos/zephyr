/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include "rdc.h"
#include "rdc_defs_imx7d.h"
#include "ccm_imx7d.h"
#include "clock_freq.h"
#include "soc_clk_freq.h"

#define RDC_DOMAIN_PERM_NONE  (0x0)
#define RDC_DOMAIN_PERM_W     (0x1)
#define RDC_DOMAIN_PERM_R     (0x2)
#define RDC_DOMAIN_PERM_RW    (RDC_DOMAIN_PERM_W|RDC_DOMAIN_PERM_R)

#define RDC_DOMAIN_PERM(domain, perm) (perm << (domain * 2))

#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _SOC__H_ */
