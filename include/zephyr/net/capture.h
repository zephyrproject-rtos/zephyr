/** @file
 * @brief Network packet capture definitions
 *
 * Definitions for capturing network packets.
 */

/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_CAPTURE_H_
#define ZEPHYR_INCLUDE_NET_CAPTURE_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network packet capture support functions
 * @defgroup net_capture Network packet capture
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */

struct net_if;
struct net_pkt;
struct device;

struct net_capture_interface_api {
	/** Cleanup the setup. This will also disable capturing. After this
	 * call, the setup function can be called again.
	 */
	int (*cleanup)(const struct device *dev);

	/** Enable / start capturing data */
	int (*enable)(const struct device *dev, struct net_if *iface);

	/** Disable / stop capturing data */
	int (*disable)(const struct device *dev);

	/** Is capturing enabled (returns true) or disabled (returns false).
	 */
	bool (*is_enabled)(const struct device *dev);

	/** Send captured data */
	int (*send)(const struct device *dev, struct net_if *iface, struct net_pkt *pkt);
};

/** @endcond */

/**
 * @brief Setup network packet capturing support.
 *
 * @param remote_addr The value tells the tunnel remote/outer endpoint
 *        IP address. The IP address can be either IPv4 or IPv6 address.
 *        This address is used to select the network interface where the tunnel
 *        is created.
 * @param my_local_addr The local/inner IP address of the tunnel. Can contain
 *        also port number which is used as UDP source port.
 * @param peer_addr The peer/inner IP address of the tunnel. Can contain
 *        also port number which is used as UDP destination port.
 * @param dev Network capture device. This is returned to the caller.
 *
 * @return 0 if ok, <0 if network packet capture setup failed
 */
int net_capture_setup(const char *remote_addr, const char *my_local_addr, const char *peer_addr,
		      const struct device **dev);

/**
 * @brief Cleanup network packet capturing support.
 *
 * @details This should be called after the capturing is done and resources
 *          can be released.
 *
 * @param dev Network capture device. User must allocate using the
 *            net_capture_setup() function.
 *
 * @return 0 if ok, <0 if network packet capture cleanup failed
 */
static inline int net_capture_cleanup(const struct device *dev)
{
#if defined(CONFIG_NET_CAPTURE)
	const struct net_capture_interface_api *api =
		(const struct net_capture_interface_api *)dev->api;

	return api->cleanup(dev);
#else
	ARG_UNUSED(dev);

	return -ENOTSUP;
#endif
}

/**
 * @brief Enable network packet capturing support.
 *
 * @details This creates tunnel network interface where all the
 * captured packets are pushed. The captured network packets are
 * placed in UDP packets that are sent to tunnel peer.
 *
 * @param dev Network capture device
 * @param iface Network interface we are starting to capture packets.
 *
 * @return 0 if ok, <0 if network packet capture enable failed
 */
static inline int net_capture_enable(const struct device *dev, struct net_if *iface)
{
#if defined(CONFIG_NET_CAPTURE)
	const struct net_capture_interface_api *api =
		(const struct net_capture_interface_api *)dev->api;

	return api->enable(dev, iface);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return -ENOTSUP;
#endif
}

/**
 * @brief Is network packet capture enabled or disabled.
 *
 * @param dev Network capture device
 *
 * @return True if enabled, False if network capture is disabled.
 */
static inline bool net_capture_is_enabled(const struct device *dev)
{
#if defined(CONFIG_NET_CAPTURE)
	const struct net_capture_interface_api *api =
		(const struct net_capture_interface_api *)dev->api;

	return api->is_enabled(dev);
#else
	ARG_UNUSED(dev);

	return false;
#endif
}

/**
 * @brief Disable network packet capturing support.
 *
 * @param dev Network capture device
 *
 * @return 0 if ok, <0 if network packet capture disable failed
 */
static inline int net_capture_disable(const struct device *dev)
{
#if defined(CONFIG_NET_CAPTURE)
	const struct net_capture_interface_api *api =
		(const struct net_capture_interface_api *)dev->api;

	return api->disable(dev);
#else
	ARG_UNUSED(dev);

	return -ENOTSUP;
#endif
}

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Send captured packet.
 *
 * @param dev Network capture device
 * @param iface Network interface the packet is being sent
 * @param pkt The network packet that is sent
 *
 * @return 0 if ok, <0 if network packet capture send failed
 */
static inline int net_capture_send(const struct device *dev, struct net_if *iface,
				   struct net_pkt *pkt)
{
#if defined(CONFIG_NET_CAPTURE)
	const struct net_capture_interface_api *api =
		(const struct net_capture_interface_api *)dev->api;

	return api->send(dev, iface, pkt);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);
	ARG_UNUSED(pkt);

	return -ENOTSUP;
#endif
}

/**
 * @brief Check if the network packet needs to be captured or not.
 *        This is called for every network packet being sent.
 *
 * @param iface Network interface the packet is being sent
 * @param pkt The network packet that is sent
 */
#if defined(CONFIG_NET_CAPTURE)
void net_capture_pkt(struct net_if *iface, struct net_pkt *pkt);
#else
static inline void net_capture_pkt(struct net_if *iface, struct net_pkt *pkt)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(pkt);
}
#endif

struct net_capture_info {
	const struct device *capture_dev;
	struct net_if *capture_iface;
	struct net_if *tunnel_iface;
	struct sockaddr *peer;
	struct sockaddr *local;
	bool is_enabled;
};

/**
 * @typedef net_capture_cb_t
 * @brief Callback used while iterating over capture devices
 *
 * @param info Information about capture device
 * @param user_data A valid pointer to user data or NULL
 */
typedef void (*net_capture_cb_t)(struct net_capture_info *info, void *user_data);

/**
 * @brief Go through all the capture devices in order to get
 *        information about them. This is mainly useful in
 *        net-shell to print data about currently active
 *        captures.
 *
 * @param cb Callback to call for each capture device
 * @param user_data User supplied data
 */
#if defined(CONFIG_NET_CAPTURE)
void net_capture_foreach(net_capture_cb_t cb, void *user_data);
#else
static inline void net_capture_foreach(net_capture_cb_t cb, void *user_data)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);
}
#endif

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_CAPTURE_H_ */
