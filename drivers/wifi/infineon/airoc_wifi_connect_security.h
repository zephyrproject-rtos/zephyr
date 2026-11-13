/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AIROC_WIFI_CONNECT_SECURITY_H
#define AIROC_WIFI_CONNECT_SECURITY_H

#include <stdbool.h>
#include <zephyr/net/wifi.h>
#include <whd_types.h>

/**
 * \brief Maps a Zephyr wifi_security_type to its WHD security type. Pure
 *        mapping, no WHD calls, so a host-side unit test can build this
 *        translation unit alone. Used by both airoc_mgmt_connect() (STA
 *        join, every case) and airoc_mgmt_ap_enable() (raw SoftAP, limited
 *        to what airoc_wifi_security_supported_for_ap() allows). Returns
 *        WHD_SECURITY_UNKNOWN for anything unrecognized.
 */
whd_security_t airoc_convert_zephyr_security_to_whd(int security);

/**
 * \brief True if a WHD security type is one airoc_mgmt_ap_enable() can pass
 *        to whd_wifi_init_ap(). WHD has no such grouping; this is the raw
 *        SoftAP path's own allow-list.
 */
bool airoc_wifi_security_supported_for_ap(whd_security_t security);

#endif /* AIROC_WIFI_CONNECT_SECURITY_H */
