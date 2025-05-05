/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for network link address
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_LINKADDR_H_
#define ZEPHYR_INCLUDE_NET_NET_LINKADDR_H_

#include <zephyr/types.h>
#include <stdbool.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network link address library
 * @defgroup net_linkaddr Network Link Address Library
 * @since 1.0
 * @version 1.0.0
 * @ingroup networking
 * @{
 */

/** Maximum length of the link address */
#if defined(CONFIG_NET_L2_PHY_IEEE802154) || defined(CONFIG_NET_L2_PPP)
#define NET_LINK_ADDR_MAX_LENGTH 8
#else
#define NET_LINK_ADDR_MAX_LENGTH 6
#endif

/**
 * Type of the link address. This indicates the network technology that this
 * address is used in. Note that in order to save space we store the value
 * into a uint8_t variable, so please do not introduce any values > 255 in
 * this enum.
 */
enum net_link_type {
	/** Unknown link address type. */
	NET_LINK_UNKNOWN = 0,
	/** IEEE 802.15.4 link address. */
	NET_LINK_IEEE802154,
	/** Bluetooth IPSP link address. */
	NET_LINK_BLUETOOTH,
	/** Ethernet link address. */
	NET_LINK_ETHERNET,
	/** Dummy link address. Used in testing apps and loopback support. */
	NET_LINK_DUMMY,
	/** CANBUS link address. */
	NET_LINK_CANBUS_RAW,
} __packed;

/**
 *  @brief Hardware link address structure
 *
 *  Used to hold the link address information. This variant is needed
 *  when we have to store the link layer address.
 *
 *  Note that you cannot cast this to net_linkaddr as uint8_t * is
 *  handled differently than uint8_t addr[] and the fields are purposely
 *  in different order.
 */
struct net_linkaddr {
	/** What kind of address is this for */
	uint8_t type;

	/** The real length of the ll address. */
	uint8_t len;

	/** The array of bytes representing the address */
	uint8_t addr[NET_LINK_ADDR_MAX_LENGTH]; /* in big endian */
};

/**
 * @brief Compare two link layer addresses.
 *
 * @param lladdr1 Pointer to a link layer address
 * @param lladdr2 Pointer to a link layer address
 *
 * @return True if the addresses are the same, false otherwise.
 */
static inline bool net_linkaddr_cmp(struct net_linkaddr *lladdr1,
				    struct net_linkaddr *lladdr2)
{
	if (!lladdr1 || !lladdr2) {
		return false;
	}

	if (lladdr1->len != lladdr2->len) {
		return false;
	}

	return !memcmp(lladdr1->addr, lladdr2->addr, lladdr1->len);
}

/**
 *
 * @brief Set the member data of a link layer address storage structure.
 *
 * @param lladdr The link address storage structure to change.
 * @param new_addr Array of bytes containing the link address.
 * @param new_len Length of the link address array.
 * This value should always be <= NET_LINK_ADDR_MAX_LENGTH.
 * @return 0 if ok, <0 if error
 */
static inline int net_linkaddr_set(struct net_linkaddr *lladdr,
				   const uint8_t *new_addr,
				   uint8_t new_len)
{
	if (lladdr == NULL || new_addr == NULL) {
		return -EINVAL;
	}

	if (new_len > NET_LINK_ADDR_MAX_LENGTH) {
		return -EMSGSIZE;
	}

	lladdr->len = new_len;
	memcpy(lladdr->addr, new_addr, new_len);

	return 0;
}

/**
 * @brief Copy link address from one variable to another.
 *
 * @param dst The link address structure destination.
 * @param src The link address structure to source.
 * @return 0 if ok, <0 if error
 */
static inline int net_linkaddr_copy(struct net_linkaddr *dst,
				    const struct net_linkaddr *src)
{
	if (dst == NULL || src == NULL) {
		return -EINVAL;
	}

	if (src->len > NET_LINK_ADDR_MAX_LENGTH) {
		return -EMSGSIZE;
	}

	dst->type = src->type;
	dst->len = src->len;
	memcpy(dst->addr, src->addr, src->len);

	return 0;
}

/**
 * @brief Create a link address structure.
 *
 * @param lladdr The link address structure to change.
 * @param addr Array of bytes containing the link address. If set to NULL,
 * the address will be cleared.
 * @param len Length of the link address array.
 * @param type Type of the link address.
 * @return 0 if ok, <0 if error
 */
static inline int net_linkaddr_create(struct net_linkaddr *lladdr,
				      const uint8_t *addr, uint8_t len,
				      enum net_link_type type)
{
	if (lladdr == NULL) {
		return -EINVAL;
	}

	if (len > NET_LINK_ADDR_MAX_LENGTH) {
		return -EMSGSIZE;
	}

	if (addr == NULL) {
		memset(lladdr->addr, 0, NET_LINK_ADDR_MAX_LENGTH);
	} else {
		memcpy(lladdr->addr, addr, len);
	}

	lladdr->type = type;
	lladdr->len = len;

	return 0;
}

/**
 * @brief Clear link address.
 *
 * @param lladdr The link address structure.
 * @return 0 if ok, <0 if error
 */
static inline int net_linkaddr_clear(struct net_linkaddr *lladdr)
{
	return net_linkaddr_create(lladdr, NULL, 0, NET_LINK_UNKNOWN);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_NET_LINKADDR_H_ */
