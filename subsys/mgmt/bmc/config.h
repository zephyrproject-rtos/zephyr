/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __CONFIG_H__
#define __CONFIG_H__

int config_init(void);
int config_clear(void);
const char *config_bmc_hostname(void);
int config_bmc_hostname_set(const char *hostname);
int config_bmc_password_set(const char *password);
bool config_bmc_use_ntp(void);
int config_bmc_use_ntp_set(bool use);
const char *config_bmc_ntp_server(void);
int config_bmc_ntp_server_set(const char *server);
uint32_t config_bmc_default_ip4(void);
int config_bmc_default_ip4_set(const char *str);
uint32_t config_bmc_default_ip4_nm(void);
int config_bmc_default_ip4_nm_set(const char *str);
uint32_t config_bmc_default_ip4_gw(void);
int config_bmc_default_ip4_gw_set(const char *str);
bool config_bmc_use_dhcp4(void);
int config_bmc_use_dhcp4_set(bool use);
bool config_host_auto_poweron(void);
int config_host_auto_poweron_set(bool on);
const char *config_bmc_admin_password(void);
bool config_bmc_https_psk(const char **psk, int *psk_len);
#endif
