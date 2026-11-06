/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AIROC_WIFI_SECURITY_H
#define AIROC_WIFI_SECURITY_H

#include <errno.h>
#include <zephyr/net/wifi.h>
#include <whd_types.h>

/**
 * \brief Maps a Zephyr wifi_security_type to the WHD security type
 *        airoc_mgmt_ap_enable() passes to whd_wifi_init_ap(). Pure mapping,
 *        no WHD calls -- kept in its own translation unit so a host-side
 *        unit test can build it alone, without the rest of the driver's
 *        kernel/net_if/SDIO dependencies. Returns -ENOTSUP for anything
 *        unrecognized, leaving *out_security unwritten.
 */
int airoc_wifi_security_to_whd(enum wifi_security_type security, whd_security_t *out_security);

#endif /* AIROC_WIFI_SECURITY_H */
