/*
 * Copyright (c) 2018, Diego Sueiro
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RDC_IMX_RDC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RDC_IMX_RDC_H_

#define A7_DOMAIN_ID	0
#define A9_DOMAIN_ID	0
#define A53_DOMAIN_ID	0
#define M4_DOMAIN_ID	1
#define M7_DOMAIN_ID	1

#define RDC_DOMAIN_PERM_NONE  (0x0)
#define RDC_DOMAIN_PERM_W     (0x1)
#define RDC_DOMAIN_PERM_R     (0x2)
#define RDC_DOMAIN_PERM_RW    (RDC_DOMAIN_PERM_W|RDC_DOMAIN_PERM_R)

#define RDC_DOMAIN_PERM(domain, perm) (perm << (domain * 2))

#define RDC_DT_VAL(nodelabel) DT_PROP(DT_NODELABEL(nodelabel), rdc)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RDC_IMX_RDC_H_ */
