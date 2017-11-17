/** @file
 * @brief Hostname configuration definitions
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_HOSTNAME_H
#define __NET_HOSTNAME_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network hostname configuration library
 * @defgroup net_hostname Network Hostname Library
 * @ingroup networking
 * @{
 */

#if defined(CONFIG_NET_HOSTNAME_ENABLE)
/**
 * @brief Get the device hostname
 *
 * @details Return pointer to device hostname.
 *
 * @return Pointer to hostname or NULL if not set.
 */
const char *net_hostname_get(void);

/**
 * @brief Initialize and set the device hostname.
 *
 */
void net_hostname_init(void);

#else
static inline const char *net_hostname_get(void)
{
	return NULL;
}

static inline void net_hostname_init(void)
{
}
#endif /* CONFIG_NET_HOSTNAME_ENABLE */

#if defined(CONFIG_NET_HOSTNAME_UNIQUE)
/**
 * @brief Set the device hostname postfix
 *
 * @details Set the device hostname to some value. This is only used if
 * CONFIG_NET_HOSTNAME_UNIQUE is set.
 *
 * @param hostname_postfix Usually link address. The function will convert this
 * to a string.
 * @param postfix_len Length of the hostname_postfix array.
 *
 * @return 0 if ok, <0 if error
 */
int net_hostname_set_postfix(const u8_t *hostname_postfix,
			      int postfix_len);
#else
static inline int net_hostname_set_postfix(const u8_t *hostname_postfix,
					   int postfix_len)
{
	return -EMSGSIZE;
}
#endif /* CONFIG_NET_HOSTNAME_UNIQUE */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __NET_HOSTNAME_H */
