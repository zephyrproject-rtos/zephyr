/*
 * Copyright (c) 2018, Diego Sueiro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IMX_RDC_H
#define __IMX_RDC_H

#define A7_DOMAIN_ID	0
#define A9_DOMAIN_ID	0
#define M4_DOMAIN_ID	1

#define RDC_DOMAIN_PERM_NONE  (0x0)
#define RDC_DOMAIN_PERM_W     (0x1)
#define RDC_DOMAIN_PERM_R     (0x2)
#define RDC_DOMAIN_PERM_RW    (RDC_DOMAIN_PERM_W|RDC_DOMAIN_PERM_R)

#define RDC_DOMAIN_PERM(domain, perm) (perm << (domain * 2))

#endif /* __IMX_RDC_H */
