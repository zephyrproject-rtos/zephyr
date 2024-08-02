/** @file
 * @brief Hostname configuration definitions
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_HOSTNAME_H_
#define ZEPHYR_INCLUDE_NET_HOSTNAME_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network hostname configuration library
 * @defgroup net_hostname Network Hostname Library
 * @since 1.10
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

#if defined(CONFIG_NET_HOSTNAME_MAX_LEN)
#define NET_HOSTNAME_MAX_LEN                                                                       \
	MAX(CONFIG_NET_HOSTNAME_MAX_LEN,                                                           \
	    (sizeof(CONFIG_NET_HOSTNAME) - 1 +                                                     \
	     (IS_ENABLED(CONFIG_NET_HOSTNAME_UNIQUE) ? sizeof("0011223344556677") - 1 : 0)))
#else
/** Maximum hostname length */
#define NET_HOSTNAME_MAX_LEN                                                                       \
	(sizeof(CONFIG_NET_HOSTNAME) - 1 +                                                         \
	 (IS_ENABLED(CONFIG_NET_HOSTNAME_UNIQUE) ? sizeof("0011223344556677") - 1 : 0))
#endif

/** @cond INTERNAL_HIDDEN */

#if defined(CONFIG_NET_HOSTNAME_ENABLE)
#define NET_HOSTNAME_SIZE NET_HOSTNAME_MAX_LEN + 1
#else
#define NET_HOSTNAME_SIZE 1
#endif

/** @endcond */

/**
 * @brief Get the device hostname
 *
 * @details Return pointer to device hostname.
 *
 * @return Pointer to hostname or NULL if not set.
 */
#if defined(CONFIG_NET_HOSTNAME_ENABLE)
const char *net_hostname_get(void);
#else
static inline const char *net_hostname_get(void)
{
	return "zephyr";
}
#endif /* CONFIG_NET_HOSTNAME_ENABLE */

/**
 * @brief Set the device hostname
 *
 * @param host new hostname as char array.
 * @param len Length of the hostname array.
 *
 * @return 0 if ok, <0 on error
 */
#if defined(CONFIG_NET_HOSTNAME_DYNAMIC)
int net_hostname_set(char *host, size_t len);
#else
static inline int net_hostname_set(char *host, size_t len)
{
	return -ENOTSUP;
}
#endif

/**
 * @brief Initialize and set the device hostname.
 *
 */
#if defined(CONFIG_NET_HOSTNAME_ENABLE)
void net_hostname_init(void);
#else
static inline void net_hostname_init(void)
{
}
#endif /* CONFIG_NET_HOSTNAME_ENABLE */

/**
 * @brief Set the device hostname postfix
 *
 * @details Convert the hostname postfix to hexadecimal value and set the
 * device hostname with the converted value. This is only used if
 * CONFIG_NET_HOSTNAME_UNIQUE is set.
 *
 * @param hostname_postfix Usually link address. The function will convert this
 * to a hexadecimal string.
 * @param postfix_len Length of the hostname_postfix array.
 *
 * @return 0 if ok, <0 if error
 */
#if defined(CONFIG_NET_HOSTNAME_UNIQUE)
int net_hostname_set_postfix(const uint8_t *hostname_postfix,
			      int postfix_len);
#else
static inline int net_hostname_set_postfix(const uint8_t *hostname_postfix,
					   int postfix_len)
{
	ARG_UNUSED(hostname_postfix);
	ARG_UNUSED(postfix_len);
	return -EMSGSIZE;
}
#endif /* CONFIG_NET_HOSTNAME_UNIQUE */

/**
 * @brief Set the postfix string for the network hostname.
 *
 * @details Set the hostname postfix string for the network hostname as is, without any conversion.
 * This is only used if CONFIG_NET_HOSTNAME_UNIQUE is set. The function checks if the combined
 * length of the default hostname (defined by CONFIG_NET_HOSTNAME) and the postfix does not exceed
 * NET_HOSTNAME_MAX_LEN. If the postfix is too long, the function returns an
 * error.
 *
 * @param hostname_postfix Pointer to the postfix string to be appended to the network hostname.
 * @param postfix_len Length of the hostname_postfix array.
 *
 * @return 0 if ok, <0 if error
 */
#if defined(CONFIG_NET_HOSTNAME_UNIQUE)
int net_hostname_set_postfix_str(const uint8_t *hostname_postfix,
			     int postfix_len);
#else
static inline int net_hostname_set_postfix_str(const uint8_t *hostname_postfix,
					   int postfix_len)
{
	ARG_UNUSED(hostname_postfix);
	ARG_UNUSED(postfix_len);
	return -EMSGSIZE;
}
#endif /* CONFIG_NET_HOSTNAME_UNIQUE */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_HOSTNAME_H_ */
