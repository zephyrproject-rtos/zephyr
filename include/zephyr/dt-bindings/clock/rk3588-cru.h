/*
 * Copyright (c) 2026 KylinSoft Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * RK3588 CRU clock identifiers used by the GMAC controllers.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RK3588_CRU_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RK3588_CRU_H_

#define MCLK_GMAC0_OUT     248
#define CLK_GMAC0_PTP_REF  308
#define CLK_GMAC1_PTP_REF  309
#define CLK_GMAC_125M      310
#define CLK_GMAC_50M       311
#define PCLK_GMAC0         344
#define PCLK_GMAC1         345
#define ACLK_GMAC0         349
#define ACLK_GMAC1         350

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RK3588_CRU_H_ */
