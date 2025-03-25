/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WIFI_CREDENTIALS_H__
#define WIFI_CREDENTIALS_H__

#include <zephyr/types.h>
#include <zephyr/net/wifi.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Library that provides a way to store and load Wi-Fi credentials.
 * @defgroup wifi_credentials Wi-Fi credentials library
 * @ingroup networking
 * @since 4.0
 * @version 0.1.0
 * @{
 */


/* this entry contains a BSSID */
#define WIFI_CREDENTIALS_FLAG_BSSID        BIT(0)
/* this entry is to be preferred over others */
#define WIFI_CREDENTIALS_FLAG_FAVORITE     BIT(1)
/* this entry can use the 2.4 GHz band */
#define WIFI_CREDENTIALS_FLAG_2_4GHz       BIT(2)
/* this entry can use the 5 GHz band */
#define WIFI_CREDENTIALS_FLAG_5GHz         BIT(3)
/* this entry requires management frame protection */
#define WIFI_CREDENTIALS_FLAG_MFP_REQUIRED BIT(4)
/* this entry disables management frame protection */
#define WIFI_CREDENTIALS_FLAG_MFP_DISABLED BIT(5)

/* Maximum length of the password */
#define WIFI_CREDENTIALS_MAX_PASSWORD_LEN                                                          \
	MAX(WIFI_PSK_MAX_LEN, CONFIG_WIFI_CREDENTIALS_SAE_PASSWORD_LENGTH)

/**
 * @brief Wi-Fi credentials entry header
 * @note Every settings entry starts with this header.
 *       Depending on the `type` field, the header can be casted to a larger type.
 *       In addition to SSID (usually a string) and BSSID (a MAC address),
 *       a `flags` field can be used to control some detail settings.
 *
 */
struct wifi_credentials_header {
	/** Wi-Fi security type */
	enum wifi_security_type type;

	/** SSID (Service Set Identifier) */
	char ssid[WIFI_SSID_MAX_LEN];

	/** Length of the SSID */
	size_t ssid_len;

	/** Flags for controlling detail settings */
	uint32_t flags;

	/** Timeout for connecting to the network */
	uint32_t timeout;

	/** BSSID (Basic Service Set Identifier) */
	uint8_t bssid[WIFI_MAC_ADDR_LEN];

	/** Channel on which the network operates */
	uint8_t channel;
};

/**
 * @brief Wi-Fi Personal credentials entry
 * @note Contains only the header and a password.
 *       For PSK security, passwords can be up to `WIFI_PSK_MAX_LEN` bytes long
 *       including NULL termination. For SAE security it can range up to
 *       `CONFIG_WIFI_CREDENTIALS_SAE_PASSWORD_LENGTH`.
 *
 */
struct wifi_credentials_personal {
	/** Header */
	struct wifi_credentials_header header;

	/** Password/PSK */
	char password[WIFI_CREDENTIALS_MAX_PASSWORD_LEN];

	/** Length of the password */
	size_t password_len;
};

/**
 * @brief Wi-Fi Enterprise credentials entry
 * @note This functionality is not yet implemented.
 */
struct wifi_credentials_enterprise {
	/** Header */
	struct wifi_credentials_header header;

	/** Length of the identity */
	size_t identity_len;

	/** Length of the anonymous identity */
	size_t anonymous_identity_len;

	/** Length of the password */
	size_t password_len;

	/** Length of the CA certificate */
	size_t ca_cert_len;

	/** Length of the client certificate */
	size_t client_cert_len;

	/** Length of the private key */
	size_t private_key_len;

	/** Length of the private key password */
	size_t private_key_pw_len;
};

/**
 * @brief Get credentials for given SSID.
 *
 * @param[in] ssid			SSID to look for
 * @param[in] ssid_len			length of SSID
 * @param[out] type			Wi-Fi security type
 * @param[out] bssid_buf		buffer to store BSSID if it was fixed
 * @param[in] bssid_buf_len		length of bssid_buf
 * @param[out] password_buf		buffer to store password
 * @param[in] password_buf_len		length of password_buf
 * @param[out] password_len		length of password
 * @param[out] flags			flags
 * @param[out] channel			channel
 * @param[out] timeout			timeout
 *
 * @return 0		Success.
 * @return -ENOENT	No network with this SSID was found.
 * @return -EINVAL	A required buffer was NULL or invalid SSID length.
 * @return -EPROTO	The network with this SSID is not a personal network.
 */
int wifi_credentials_get_by_ssid_personal(const char *ssid, size_t ssid_len,
					  enum wifi_security_type *type, uint8_t *bssid_buf,
					  size_t bssid_buf_len, char *password_buf,
					  size_t password_buf_len, size_t *password_len,
					  uint32_t *flags, uint8_t *channel, uint32_t *timeout);

/**
 * @brief Set credentials for given SSID.
 *
 * @param[in] ssid		SSID to look for
 * @param[in] ssid_len		length of SSID
 * @param[in] type		Wi-Fi security type
 * @param[in] bssid		BSSID (may be NULL)
 * @param[in] bssid_len		length of BSSID buffer (either 0 or WIFI_MAC_ADDR_LEN)
 * @param[in] password		password
 * @param[in] password_len		length of password
 * @param[in] flags			flags
 * @param[in] channel			Channel
 * @param[in] timeout			Timeout
 *
 * @return 0			Success. Credentials are stored in persistent storage.
 * @return -EINVAL		A required buffer was NULL or security type is not supported.
 * @return -ENOTSUP		Security type is not supported.
 * @return -ENOBUFS		All slots are already taken.
 */
int wifi_credentials_set_personal(const char *ssid, size_t ssid_len, enum wifi_security_type type,
				  const uint8_t *bssid, size_t bssid_len, const char *password,
				  size_t password_len, uint32_t flags, uint8_t channel,
				  uint32_t timeout);

/**
 * @brief Get credentials for given SSID by struct.
 *
 * @param[in] ssid		SSID to look for
 * @param[in] ssid_len		length of SSID
 * @param[out] buf		credentials Pointer to struct where credentials are stored
 *
 * @return 0			Success.
 * @return -ENOENT		No network with this SSID was found.
 * @return -EINVAL		A required buffer was NULL or too small.
 * @return -EPROTO		The network with this SSID is not a personal network.
 */
int wifi_credentials_get_by_ssid_personal_struct(const char *ssid, size_t ssid_len,
						 struct wifi_credentials_personal *buf);

/**
 * @brief Set credentials for given SSID by struct.
 *
 * @param[in] creds		credentials Pointer to struct from which credentials are loaded
 *
 * @return 0			Success.
 * @return -ENOENT		No network with this SSID was found.
 * @return -EINVAL		A required buffer was NULL or incorrect size.
 * @return -ENOBUFS		All slots are already taken.
 */
int wifi_credentials_set_personal_struct(const struct wifi_credentials_personal *creds);

/**
 * @brief Delete credentials for given SSID.
 *
 * @param[in] ssid		SSID to look for
 * @param[in] ssid_len		length of SSID
 *
 * @return			-ENOENT if No network with this SSID was found.
 * @return			0 on success, otherwise a negative error code
 */
int wifi_credentials_delete_by_ssid(const char *ssid, size_t ssid_len);

/**
 * @brief Check if credentials storage is empty.
 *
 * @return			true if credential storage is empty, otherwise false
 */
bool wifi_credentials_is_empty(void);

/**
 * @brief Deletes all stored Wi-Fi credentials.
 *
 * This function deletes all Wi-Fi credentials that have been stored in the system.
 * It is typically used when you want to clear all saved networks.
 *
 * @return			0 on successful, otherwise a negative error code
 */
int wifi_credentials_delete_all(void);

/**
 * @brief Callback type for wifi_credentials_for_each_ssid.
 *
 * @param[in] cb_arg      arguments for the callback function. Appropriate cb_arg is
 *                        transferred by wifi_credentials_for_each_ssid.
 * @param[in] ssid        SSID
 * @param[in] ssid_len    length of SSID
 */
typedef void (*wifi_credentials_ssid_cb)(void *cb_arg, const char *ssid, size_t ssid_len);

/**
 * @brief Call callback for each registered SSID.
 *
 * @param cb		callback
 * @param cb_arg	argument for callback function
 */
void wifi_credentials_for_each_ssid(wifi_credentials_ssid_cb cb, void *cb_arg);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* WIFI_CREDENTIALS_H__ */
