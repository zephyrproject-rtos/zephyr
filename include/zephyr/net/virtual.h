/** @file
 * @brief Virtual Network Interface
 */

/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_VIRTUAL_H_
#define ZEPHYR_INCLUDE_NET_VIRTUAL_H_

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>

#include <zephyr/sys/util.h>
#include <zephyr/net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Virtual network interface support functions
 * @defgroup virtual Virtual Network Interface Support Functions
 * @since 2.6
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

/** Virtual interface capabilities */
enum virtual_interface_caps {
	/** IPIP tunnel */
	VIRTUAL_INTERFACE_IPIP = BIT(1),

	/** Virtual LAN interface (VLAN) */
	VIRTUAL_INTERFACE_VLAN = BIT(2),

/** @cond INTERNAL_HIDDEN */
	/* Marker for capabilities - must be at the end of the enum.
	 * It is here because the capability list cannot be empty.
	 */
	VIRTUAL_INTERFACE_NUM_CAPS
/** @endcond */
};

/** @cond INTERNAL_HIDDEN */

enum virtual_interface_config_type {
	VIRTUAL_INTERFACE_CONFIG_TYPE_PEER_ADDRESS,
	VIRTUAL_INTERFACE_CONFIG_TYPE_MTU,
	VIRTUAL_INTERFACE_CONFIG_TYPE_LINK_TYPE,
};

struct virtual_interface_link_types {
	int count;
	uint16_t type[COND_CODE_1(CONFIG_NET_CAPTURE_COOKED_MODE,
				  (CONFIG_NET_CAPTURE_COOKED_MODE_MAX_LINK_TYPES),
				  (1))];
};

struct virtual_interface_config {
	sa_family_t family;
	union {
		struct in_addr peer4addr;
		struct in6_addr peer6addr;
		int mtu;
		struct virtual_interface_link_types link_types;
	};
};

#if defined(CONFIG_NET_L2_VIRTUAL)
#define VIRTUAL_MAX_NAME_LEN CONFIG_NET_L2_VIRTUAL_MAX_NAME_LEN
#else
#define VIRTUAL_MAX_NAME_LEN 0
#endif
/** @endcond */

/** Virtual L2 API operations. */
struct virtual_interface_api {
	/**
	 * The net_if_api must be placed in first position in this
	 * struct so that we are compatible with network interface API.
	 */
	struct net_if_api iface_api;

	/** Get the virtual interface capabilities */
	enum virtual_interface_caps (*get_capabilities)(struct net_if *iface);

	/** Start the device */
	int (*start)(const struct device *dev);

	/** Stop the device */
	int (*stop)(const struct device *dev);

	/** Send a network packet */
	int (*send)(struct net_if *iface, struct net_pkt *pkt);

	/**
	 * Receive a network packet.
	 * The callback returns NET_OK if this interface will accept the
	 * packet and pass it upper layers, NET_DROP if the packet is to be
	 * dropped and NET_CONTINUE to pass it to next interface.
	 */
	enum net_verdict (*recv)(struct net_if *iface, struct net_pkt *pkt);

	/** Pass the attachment information to virtual interface */
	int (*attach)(struct net_if *virtual_iface, struct net_if *iface);

	/** Set specific L2 configuration */
	int (*set_config)(struct net_if *iface,
			  enum virtual_interface_config_type type,
			  const struct virtual_interface_config *config);

	/** Get specific L2 configuration */
	int (*get_config)(struct net_if *iface,
			  enum virtual_interface_config_type type,
			  struct virtual_interface_config *config);
};

/* Make sure that the network interface API is properly setup inside
 * Virtual API struct (it is the first one).
 */
BUILD_ASSERT(offsetof(struct virtual_interface_api, iface_api) == 0);

/** Virtual L2 context that is needed to binding to the real network interface
 */
struct virtual_interface_context {
/** @cond INTERNAL_HIDDEN */
	/* Keep track of contexts */
	sys_snode_t node;

	/* My virtual network interface */
	struct net_if *virtual_iface;
/** @endcond */

	/**
	 * Other network interface this virtual network interface is
	 * attached to. These values can be chained so virtual network
	 * interfaces can run on top of other virtual interfaces.
	 */
	struct net_if *iface;

	/**
	 * This tells what L2 features does virtual support.
	 */
	enum net_l2_flags virtual_l2_flags;

	/** Is this context already initialized */
	bool is_init;

	/** Link address for this network interface */
	struct net_linkaddr_storage lladdr;

	/** User friendly name of this L2 layer. */
	char name[VIRTUAL_MAX_NAME_LEN];
};

/**
 * @brief Attach virtual network interface to the given network interface.
 *
 * @param virtual_iface Virtual network interface.
 * @param iface Network interface we are attached to. This can be NULL,
 * if we want to detach.
 *
 * @return 0 if ok, <0 if attaching failed
 */
int net_virtual_interface_attach(struct net_if *virtual_iface,
				  struct net_if *iface);

/**
 * @brief Return network interface related to this virtual network interface.
 * The returned network interface is below this virtual network interface.
 *
 * @param iface Virtual network interface.
 *
 * @return Network interface related to this virtual interface or
 *         NULL if no such interface exists.
 */
struct net_if *net_virtual_get_iface(struct net_if *iface);

/**
 * @brief Return the name of the virtual network interface L2.
 *
 * @param iface Virtual network interface.
 * @param buf Buffer to store the name
 * @param len Max buffer length
 *
 * @return Name of the virtual network interface.
 */
char *net_virtual_get_name(struct net_if *iface, char *buf, size_t len);

/**
 * @brief Set the name of the virtual network interface L2.
 *
 * @param iface Virtual network interface.
 * @param name Name of the virtual L2 layer.
 */
void net_virtual_set_name(struct net_if *iface, const char *name);

/**
 * @brief Set the L2 flags of the virtual network interface.
 *
 * @param iface Virtual network interface.
 * @param flags L2 flags to set.
 *
 * @return Previous flags that were set.
 */
enum net_l2_flags net_virtual_set_flags(struct net_if *iface,
					enum net_l2_flags flags);

/**
 * @brief Feed the IP pkt to stack if tunneling is enabled.
 *
 * @param input_iface Network interface receiving the pkt.
 * @param remote_addr IP address of the sender.
 * @param pkt Network packet.
 *
 * @return Verdict what to do with the packet.
 */
enum net_verdict net_virtual_input(struct net_if *input_iface,
				   struct net_addr *remote_addr,
				   struct net_pkt *pkt);

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Initialize the network interface so that a virtual
 *        interface can be attached to it.
 *
 * @param iface Network interface
 */
#if defined(CONFIG_NET_L2_VIRTUAL)
void net_virtual_init(struct net_if *iface);
#else
static inline void net_virtual_init(struct net_if *iface)
{
	ARG_UNUSED(iface);
}
#endif

/**
 * @brief Update the carrier state of the virtual network interface.
 *        This is called if the underlying interface is going down.
 *
 * @param iface Network interface
 */
#if defined(CONFIG_NET_L2_VIRTUAL)
void net_virtual_disable(struct net_if *iface);
#else
static inline void net_virtual_disable(struct net_if *iface)
{
	ARG_UNUSED(iface);
}
#endif

/**
 * @brief Update the carrier state of the virtual network interface.
 *        This is called if the underlying interface is going up.
 *
 * @param iface Network interface
 */
#if defined(CONFIG_NET_L2_VIRTUAL)
void net_virtual_enable(struct net_if *iface);
#else
static inline void net_virtual_enable(struct net_if *iface)
{
	ARG_UNUSED(iface);
}
#endif

#define VIRTUAL_L2_CTX_TYPE	struct virtual_interface_context

/**
 * @brief Return virtual device hardware capability information.
 *
 * @param iface Network interface
 *
 * @return Hardware capabilities
 */
static inline enum virtual_interface_caps
net_virtual_get_iface_capabilities(struct net_if *iface)
{
	const struct virtual_interface_api *virt =
		(struct virtual_interface_api *)net_if_get_device(iface)->api;

	if (!virt->get_capabilities) {
		return (enum virtual_interface_caps)0;
	}

	return virt->get_capabilities(iface);
}

#define Z_NET_VIRTUAL_INTERFACE_INIT(node_id, dev_id, name, init_fn,	\
				     pm, data, config, prio, api, mtu)	\
	Z_NET_DEVICE_INIT(node_id, dev_id, name, init_fn, pm, data,	\
			  config, prio, api, VIRTUAL_L2,		\
			  NET_L2_GET_CTX_TYPE(VIRTUAL_L2), mtu)

#define Z_NET_VIRTUAL_INTERFACE_INIT_INSTANCE(node_id, dev_id, name,	\
					      inst, init_fn, pm, data,	\
					      config, prio, api, mtu)	\
	Z_NET_DEVICE_INIT_INSTANCE(node_id, dev_id, name, inst,		\
				   init_fn, pm, data,			\
				   config, prio, api, VIRTUAL_L2,	\
				   NET_L2_GET_CTX_TYPE(VIRTUAL_L2), mtu)
/** @endcond */

/**
 * @brief Create a virtual network interface. Binding to another interface
 *        is done at runtime by calling net_virtual_interface_attach().
 *        The attaching is done automatically when setting up tunneling
 *        when peer IP address is set in IP tunneling driver.
 *
 * @param dev_id Network device id.
 * @param name The name this instance of the driver exposes to
 * the system.
 * @param init_fn Address to the init function of the driver.
 * @param pm Reference to struct pm_device associated with the device.
 * (optional).
 * @param data Pointer to the device's private data.
 * @param config The address to the structure containing the
 * configuration information for this instance of the driver.
 * @param prio The initialization level at which configuration occurs.
 * @param api Provides an initial pointer to the API function struct
 * used by the driver. Can be NULL.
 * @param mtu Maximum transfer unit in bytes for this network interface.
 * This is the default value and its value can be tweaked at runtime.
 */
#define NET_VIRTUAL_INTERFACE_INIT(dev_id, name, init_fn, pm, data,	\
				   config, prio, api, mtu)		\
	Z_NET_VIRTUAL_INTERFACE_INIT(DT_INVALID_NODE, dev_id, name,	\
				     init_fn, pm, data, config, prio,	\
				     api, mtu)

/**
 * @brief Create a virtual network interface. Binding to another interface
 *        is done at runtime by calling net_virtual_interface_attach().
 *        The attaching is done automatically when setting up tunneling
 *        when peer IP address is set in IP tunneling driver.
 *
 * @param dev_id Network device id.
 * @param name The name this instance of the driver exposes to
 * the system.
 * @param inst instance number
 * @param init_fn Address to the init function of the driver.
 * @param pm Reference to struct pm_device associated with the device.
 * (optional).
 * @param data Pointer to the device's private data.
 * @param config The address to the structure containing the
 * configuration information for this instance of the driver.
 * @param prio The initialization level at which configuration occurs.
 * @param api Provides an initial pointer to the API function struct
 * used by the driver. Can be NULL.
 * @param mtu Maximum transfer unit in bytes for this network interface.
 * This is the default value and its value can be tweaked at runtime.
 */
#define NET_VIRTUAL_INTERFACE_INIT_INSTANCE(dev_id, name, inst,		 \
					    init_fn, pm, data,		 \
					    config, prio, api, mtu)	 \
	Z_NET_VIRTUAL_INTERFACE_INIT_INSTANCE(DT_INVALID_NODE, dev_id,	 \
					      name, inst,		 \
					      init_fn, pm, data, config, \
					      prio, api, mtu)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_VIRTUAL_H_ */
