/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_MGMT_BMC_BMC_INTERNAL_H_
#define ZEPHYR_SUBSYS_MGMT_BMC_BMC_INTERNAL_H_

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include <zephyr/sys/util.h>

/*
 * Interfaces shared between the BMC core modules. Anything an out of tree
 * application may need lives in include/zephyr/mgmt/bmc/ instead.
 */

/* net.c */
int bmc_net_set_hostname(const char *hostname);
int bmc_net_apply_static_ip4(void);
int bmc_net_start_dhcp4(void);
int bmc_net_stop_dhcp4(void);

#if defined(CONFIG_NET_DHCPV4)
/* dhcp.c */
int bmc_dhcp4_init(void);
int bmc_dhcp4_start(void);
int bmc_dhcp4_restart(void);
int bmc_dhcp4_stop(void);
#else
static inline int bmc_dhcp4_init(void)
{
	return 0;
}

static inline int bmc_dhcp4_start(void)
{
	return -ENOTSUP;
}

static inline int bmc_dhcp4_restart(void)
{
	return 0;
}

static inline int bmc_dhcp4_stop(void)
{
	return 0;
}
#endif /* CONFIG_NET_DHCPV4 */

/* auth.c */
void bmc_auth_warn_default_password(void);

/* config.c */
int bmc_config_load(void);

/* Console event bits owned by the BMC core transports, see bmc/console.h. */
#define BMC_CONSOLE_EVENT_TCP_CLIENT BIT(1)
#define BMC_CONSOLE_EVENT_WS_CLIENT  BIT(2)

/* http.c */
int bmc_http_start(void);

#if defined(CONFIG_BMC_NTP)
/* ntp.c */
int bmc_ntp_start(void);
int bmc_ntp_stop(void);
bool bmc_ntp_is_synced(void);
#else
static inline int bmc_ntp_start(void)
{
	return -ENOTSUP;
}

static inline int bmc_ntp_stop(void)
{
	return -ENOTSUP;
}

static inline bool bmc_ntp_is_synced(void)
{
	return false;
}
#endif /* CONFIG_BMC_NTP */

#if defined(CONFIG_BMC_RTC)
/* rtc.c */
int bmc_rtc_set_from_clock(void);
int bmc_time_set_from_iso_str(const char *str);
#else
static inline int bmc_rtc_set_from_clock(void)
{
	return -ENOTSUP;
}

static inline int bmc_time_set_from_iso_str(const char *str)
{
	ARG_UNUSED(str);
	return -ENOTSUP;
}
#endif /* CONFIG_BMC_RTC */

#endif /* ZEPHYR_SUBSYS_MGMT_BMC_BMC_INTERNAL_H_ */
