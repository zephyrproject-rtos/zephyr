/*
 * Copyright (c) 2026 Elan Microelectronics Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ELAN_EM32F967_SOC_INFOCTRL_H_
#define ZEPHYR_SOC_ELAN_EM32F967_SOC_INFOCTRL_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

/* Devicetree node label */
#define INFOCTRL_DT_NODE DT_NODELABEL(infoctrl)

/* Register Offsets */
#define MIRC_12M_R_2_OFF 0x1f60
#define MIRC_16M_2_OFF   0x0070
#define MIRC_20M_2_OFF   0x0074
#define MIRC_24M_2_OFF   0x0078
#define MIRC_28M_2_OFF   0x007c
#define MIRC_32M_2_OFF   0x0080

/* Field Masks for MIRC_CTRL */
#define MIRC_TALL_MASK GENMASK(9, 0)   /* [9:0]    MIRC_Tall */
#define MIRC_TV12_MASK GENMASK(12, 10) /* [12:10]  MIRC_TV12 */

#endif /* ZEPHYR_SOC_ELAN_EM32F967_SOC_INFOCTRL_H_ */
