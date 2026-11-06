/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AIROC_WIFI_CONNECT_SECURITY_H
#define AIROC_WIFI_CONNECT_SECURITY_H

#include <zephyr/net/wifi.h>
#include <whd_types.h>

/**
 * \brief Maps a Zephyr wifi_security_type to the WHD security type
 *        airoc_mgmt_connect() passes to whd_wifi_join(). Pure mapping, no
 *        WHD calls -- kept in its own translation unit so a host-side unit
 *        test can build it alone, without the rest of the driver's
 *        kernel/net_if/SDIO dependencies, same reasoning as
 *        airoc_wifi_security.c's airoc_wifi_security_to_whd().
 *
 *        Distinct from that function: this one covers the full set of
 *        security types airoc_mgmt_connect() (STA join) accepts --
 *        including WEP and the WPA/WPA3-AUTO mixed mode -- not just the
 *        NONE/PSK/SAE subset airoc_mgmt_ap_enable() (raw SoftAP) supports.
 *        Returns WHD_SECURITY_UNKNOWN for anything unrecognized, which the
 *        caller checks for explicitly.
 */
whd_security_t convert_zephyr_security_to_whd(int security);

#endif /* AIROC_WIFI_CONNECT_SECURITY_H */
