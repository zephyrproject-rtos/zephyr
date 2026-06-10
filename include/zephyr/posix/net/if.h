/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Network interface names and indices.
 * @ingroup posix
 *
 * Provides functions for mapping between network interface names and
 * their numeric indices.
 *
 * @posix_header{net_if.h}
 */

#ifndef ZEPHYR_INCLUDE_POSIX_NET_IF_H_
#define ZEPHYR_INCLUDE_POSIX_NET_IF_H_

#ifdef CONFIG_NET_INTERFACE_NAME_LEN
/**
 * @brief Maximum length of a network interface name including the NUL terminator.
 */
#define IF_NAMESIZE CONFIG_NET_INTERFACE_NAME_LEN
#else
/**
 * @brief Maximum length of a network interface name including the NUL terminator.
 */
#define IF_NAMESIZE 1
#endif

#if !defined(IFNAMSIZ)
/**
 * @brief Legacy alias for @c IF_NAMESIZE.
 */
#define IFNAMSIZ IF_NAMESIZE
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network interface name-to-index mapping.
 */
struct if_nameindex {
	/**
	 * @brief Numeric index of the interface.
	 */
	unsigned int if_index;
	/**
	 * @brief Null-terminated name of the interface.
	 */
	char *if_name;
};

/**
 * @brief Map a network interface index to its corresponding name.
 *
 * @param ifindex Numeric interface index.
 * @param ifname  Output buffer of at least @c IF_NAMESIZE bytes.
 *
 * @return @p ifname on success, or NULL with errno set on failure.
 *
 * @posix_func{if_indextoname}
 */
char *if_indextoname(unsigned int ifindex, char *ifname);

/**
 * @brief Free memory allocated by if_nameindex.
 *
 * @param ptr List to free (must have been returned by if_nameindex()).
 *
 * @posix_func{if_freenameindex}
 */
void if_freenameindex(struct if_nameindex *ptr);

/**
 * @brief Return all network interface names and indexes.
 *
 * The returned array is terminated by an entry with @c if_index == 0 and
 * @c if_name == NULL. Free with if_freenameindex().
 *
 * @return Allocated array on success, or NULL with errno set on failure.
 *
 * @posix_func{if_nameindex}
 */
struct if_nameindex *if_nameindex(void);

/**
 * @brief Map a network interface name to its corresponding index.
 *
 * @param ifname Interface name string.
 *
 * @return Interface index on success, or 0 with errno set on failure.
 *
 * @posix_func{if_nametoindex}
 */
unsigned int if_nametoindex(const char *ifname);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_NET_IF_H_ */
