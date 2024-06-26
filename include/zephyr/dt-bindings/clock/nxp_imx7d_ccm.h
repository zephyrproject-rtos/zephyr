/*
 * Copyright (c) 2024, MAKEEN Energy A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IMX7D_CCM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IMX7D_CCM_H_

/* Pre and Post dividers for the root CCM */
#define CCM_DT_ROOT_PRE(nodelabel) DT_PROP(DT_NODELABEL(nodelabel), ccm_pre)
#define CCM_DT_ROOT_POST(nodelabel) DT_PROP(DT_NODELABEL(nodelabel), ccm_post)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IMX7D_CCM_H_ */
