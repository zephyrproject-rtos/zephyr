/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_BMC_CONFIG_H_
#define ZEPHYR_INCLUDE_MGMT_BMC_CONFIG_H_

/**
 * @file
 * @brief Runtime configuration of the BMC.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BMC configuration
 * @defgroup bmc_config BMC configuration
 * @ingroup bmc_api
 * @{
 *
 * The BMC keeps its settings under the `bmc/` subtree of the Zephyr settings
 * subsystem, so an application can add product specific keys with its own
 * SETTINGS_STATIC_HANDLER_DEFINE() and pick whichever settings backend suits
 * the board. When CONFIG_BMC_SETTINGS is disabled the values below are still
 * usable, they just fall back to the Kconfig defaults and are not persisted.
 */

/** @brief Maximum length of the BMC hostname, excluding the terminator. */
#define BMC_CONFIG_HOSTNAME_MAX_LEN 20

/** @brief Maximum length of an account name, excluding the terminator. */
#define BMC_CONFIG_USER_MAX_LEN 20

/** @brief Maximum length of the administrator password, excluding the terminator. */
#define BMC_CONFIG_PASSWORD_MAX_LEN 20

/** @brief Maximum length of the NTP server name, excluding the terminator. */
#define BMC_CONFIG_NTP_SERVER_MAX_LEN 40

/**
 * @brief Get the configured BMC hostname.
 *
 * @return The hostname. Never NULL.
 */
const char *bmc_config_hostname(void);

/**
 * @brief Set and persist the BMC hostname.
 *
 * @param hostname New hostname.
 *
 * @return 0 on success, negative errno otherwise.
 */
int bmc_config_hostname_set(const char *hostname);

/**
 * @brief Get the administrator account name.
 *
 * @return The user name of the built-in administrator account.
 */
const char *bmc_config_admin_user(void);

/**
 * @brief Get the administrator password.
 *
 * @return The password. Never NULL.
 */
const char *bmc_config_admin_password(void);

/**
 * @brief Set and persist the administrator password.
 *
 * @param password New password, at most @ref BMC_CONFIG_PASSWORD_MAX_LEN long.
 *
 * @return 0 on success, negative errno otherwise.
 */
int bmc_config_admin_password_set(const char *password);

/**
 * @brief Check whether DHCPv4 is enabled.
 *
 * @return true if DHCPv4 should be used.
 */
bool bmc_config_use_dhcp4(void);

/**
 * @brief Enable or disable DHCPv4 and persist the choice.
 *
 * @param use true to enable DHCPv4.
 *
 * @return 0 on success, negative errno otherwise.
 */
int bmc_config_use_dhcp4_set(bool use);

/**
 * @brief Get the static IPv4 address.
 *
 * @return The address in network byte order, or 0 when unset.
 */
uint32_t bmc_config_static_ip4(void);

/**
 * @brief Set and persist the static IPv4 address.
 *
 * @param str Dotted quad address string, or NULL to clear the address.
 *
 * @return 0 on success, negative errno otherwise.
 */
int bmc_config_static_ip4_set(const char *str);

/**
 * @brief Get the static IPv4 netmask.
 *
 * @return The netmask in network byte order, or 0 when unset.
 */
uint32_t bmc_config_static_ip4_netmask(void);

/**
 * @brief Set and persist the static IPv4 netmask.
 *
 * @param str Dotted quad netmask string, or NULL to clear it.
 *
 * @return 0 on success, negative errno otherwise.
 */
int bmc_config_static_ip4_netmask_set(const char *str);

/**
 * @brief Get the static IPv4 gateway.
 *
 * @return The gateway in network byte order, or 0 when unset.
 */
uint32_t bmc_config_static_ip4_gateway(void);

/**
 * @brief Set and persist the static IPv4 gateway.
 *
 * @param str Dotted quad gateway string, or NULL to clear it.
 *
 * @return 0 on success, negative errno otherwise.
 */
int bmc_config_static_ip4_gateway_set(const char *str);

/**
 * @brief Check whether NTP time synchronisation is enabled.
 *
 * @return true if NTP should be used.
 */
bool bmc_config_use_ntp(void);

/**
 * @brief Enable or disable NTP and persist the choice.
 *
 * @param use true to enable NTP.
 *
 * @return 0 on success, negative errno otherwise.
 */
int bmc_config_use_ntp_set(bool use);

/**
 * @brief Get the configured NTP server.
 *
 * @return The server name. Never NULL.
 */
const char *bmc_config_ntp_server(void);

/**
 * @brief Set and persist the NTP server.
 *
 * @param server New server name.
 *
 * @return 0 on success, negative errno otherwise.
 */
int bmc_config_ntp_server_set(const char *server);

/**
 * @brief Check whether the host is powered on automatically at BMC boot.
 *
 * @return true if the host should be powered on automatically.
 */
bool bmc_config_host_auto_poweron(void);

/**
 * @brief Change and persist the host auto power-on policy.
 *
 * @param on true to power the host on at BMC boot.
 *
 * @return 0 on success, negative errno otherwise.
 */
int bmc_config_host_auto_poweron_set(bool on);

/**
 * @brief Delete the persisted BMC configuration.
 *
 * The in-memory values are left untouched, so callers normally reboot
 * afterwards to pick up the defaults.
 *
 * @return 0 on success, negative errno otherwise.
 */
int bmc_config_clear(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MGMT_BMC_CONFIG_H_ */
