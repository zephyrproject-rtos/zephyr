/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * Private header shared by Renesas RA WUC and power management
 * implementation (wuc_renesas_ra.c, power.c, poweroff.c).
 * Not part of the public driver API.
 */

#ifndef ZEPHYR_DRIVERS_WUC_WUC_RENESAS_RA_PRIV_H_
#define ZEPHYR_DRIVERS_WUC_WUC_RENESAS_RA_PRIV_H_

#include <zephyr/spinlock.h>
#include <r_lpm.h>

struct device;

struct ra_wuc_data {
	struct k_spinlock lock;

	lpm_deep_standby_cancel_source_bits_t cancel_source;
	lpm_deep_standby_cancel_edge_bits_t   cancel_edge;

	uint64_t triggered_snapshot;
};

lpm_deep_standby_cancel_source_bits_t ra_wuc_get_cancel_source(const struct device *dev);
lpm_deep_standby_cancel_edge_bits_t ra_wuc_get_cancel_edge(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_WUC_WUC_RENESAS_RA_PRIV_H_ */
