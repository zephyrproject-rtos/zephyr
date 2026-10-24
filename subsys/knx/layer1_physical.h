/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * KNX Physical Layer — subsystem-internal header.
 * The public L1 interface (enums, primitives, burst TX) is in:
 *   <zephyr/knx/knx_l1.h>
 */
#ifndef ZEPHYR_SUBSYS_KNX_LAYER1_PHYSICAL_H_
#define ZEPHYR_SUBSYS_KNX_LAYER1_PHYSICAL_H_

#include <zephyr/device.h>
#include <zephyr/knx/knx_l1.h>
#include <zephyr/knx/knx_pkt.h>

/*
 * knx_phy_get — the KNX L1 phy device this stack instance talks to.
 *
 * Resolved once at build time from whichever L1 driver is enabled
 * (CONFIG_NCN5130 or CONFIG_KNX_LOOPBACK — mutually exclusive). This is the
 * deliberate single-network "narrow cut" of PLAN.md §10.3: subsys/knx itself
 * is still a singleton (one knx_app_wq, one set of L2 threads), but every
 * call it makes to the L1 driver goes through knx_phy_driver_api via this
 * device handle rather than a link-time-resolved symbol. Generalising
 * subsys/knx to drive N devices (N independent struct knx_network instances)
 * is follow-up work, not required to satisfy "the driver API is the only
 * channel" — see PLAN.md §10.3's open questions.
 */
const struct device *knx_phy_get(void);

/* knx_l1_init — register subsys/knx's upcall handlers with knx_phy_get().
 * Must run before anything can call knx_phy_get()'s device and before the L2
 * RX thread's reset retry loop starts (knx_core.c calls this before
 * knx_l_init()).
 */
void knx_l1_init(void);

#endif /* ZEPHYR_SUBSYS_KNX_LAYER1_PHYSICAL_H_ */
