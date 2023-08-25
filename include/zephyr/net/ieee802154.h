/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IEEE 802.15.4 native L2 stack public header
 *
 * @note All references to the standard in this file cite IEEE 802.15.4-2020.
 */

#ifndef ZEPHYR_INCLUDE_NET_IEEE802154_H_
#define ZEPHYR_INCLUDE_NET_IEEE802154_H_

#include <limits.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/crypto/cipher.h>
#include <zephyr/net/ieee802154_radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ieee802154 IEEE 802.15.4 and Thread APIs
 * @since 1.0
 * @version 0.8.0
 * @ingroup connectivity
 *
 * @brief IEEE 802.15.4 native and OpenThread L2, configuration, management and
 * driver APIs
 *
 * @details The IEEE 802.15.4 and Thread subsystems comprise the OpenThread L2
 * subsystem, the native IEEE 802.15.4 L2 subsystem ("Soft" MAC), a mostly
 * vendor and protocol agnostic driver API shared between the OpenThread and
 * native L2 stacks ("Hard" MAC and PHY) as well as several APIs to configure
 * the subsystem (shell, net management, Kconfig, devicetree, etc.).
 *
 * The **OpenThread subsystem API** integrates the external <a
 * href="https://openthread.io">OpenThread</a> stack into Zephyr. It builds upon
 * Zephyr's native IEEE 802.15.4 driver API.
 *
 * The **native IEEE 802.15.4 subsystem APIs** are exposed at different levels
 * and address several audiences:
 *  - shell (end users, application developers):
 *    - a set of IEEE 802.15.4 shell commands (see `shell> ieee802154 help`)
 *  - application API (application developers):
 *    - IPv6, DGRAM and RAW sockets for actual peer-to-peer, multicast and
 *      broadcast data exchange between nodes including connection specific
 *      configuration (sample coming soon, see
 *      https://github.com/linux-wpan/wpan-tools/tree/master/examples for now
 *      which inspired our API and therefore has a similar socket API),
 *    - Kconfig and devicetree configuration options (net config library
 *      extension, subsystem-wide MAC and PHY Kconfig/DT options, driver/vendor
 *      specific Kconfig/DT options, watch out for options prefixed with
 *      IEEE802154/ieee802154),
 *    - Network Management: runtime configuration of the IEEE 802.15.4
 *      protocols stack at the MAC (L2) and PHY (L1) levels
 *      (see @ref ieee802154_mgmt),
 *  - L2 integration (subsystem contributors):
 *    - see @ref ieee802154_l2
 *    - implementation of Zephyr's internal L2-level socket and network context
 *      abstractions (context/socket operations, see @ref net_l2),
 *    - protocol-specific extension to the interface structure (see @ref net_if)
 *    - protocol-specific extensions to the network packet structure
 *      (see @ref net_pkt),
 *
 *  - OpenThread and native IEEE 802.15.4 share a common **driver API** (driver
 *    maintainers/contributors):
 *    - see @ref ieee802154_driver
 *    - a basic, mostly PHY-level driver API to be implemented by all drivers,
 *    - several "hard MAC" (hardware/firmware offloading) extension points for
 *      performance critical or timing sensitive aspects of the protocol
 */

/**
 * @defgroup ieee802154_l2 IEEE 802.15.4 L2
 * @since 1.0
 * @version 0.8.0
 * @ingroup ieee802154
 *
 * @brief IEEE 802.15.4 L2 APIs
 *
 * @details This API provides integration with Zephyr's sockets and network
 * contexts. **Application and driver developers should never interface directly
 * with this API.** It is of interest to subsystem maintainers only.
 *
 * The API implements and extends the following structures:
 *    - implements Zephyr's internal L2-level socket and network context
 *      abstractions (context/socket operations, see @ref net_l2),
 *    - protocol-specific extension to the interface structure (see @ref net_if)
 *    - protocol-specific extensions to the network packet structure
 *      (see @ref net_pkt),
 *
 * @note All section, table and figure references are to the IEEE 802.15.4-2020
 * standard.
 *
 * @{
 */

/**
 * @brief Represents the PHY constant aMaxPhyPacketSize, see section 11.3.
 *
 * @note Currently only 127 byte sized packets are supported although some PHYs
 * (e.g. SUN, MSK, LECIM, ...) support larger packet sizes. Needs to be changed
 * once those PHYs should be fully supported.
 */
#define IEEE802154_MAX_PHY_PACKET_SIZE	127

/**
 * @brief Represents the frame check sequence length, see section 7.2.1.1.
 *
 * @note Currently only a 2 byte FCS is supported although some PHYs (e.g. SUN,
 * TVWS, ...) optionally support a 4 byte FCS. Needs to be changed once those
 * PHYs should be fully supported.
 */
#define IEEE802154_FCS_LENGTH		2

/**
 * @brief IEEE 802.15.4 "hardware" MTU (not to be confused with L3/IP MTU), i.e.
 * the actual payload available to the next higher layer.
 *
 * @details This is equivalent to the IEEE 802.15.4 MAC frame length minus
 * checksum bytes which is again equivalent to the PHY payload aka PSDU length
 * minus checksum bytes. This definition exists for compatibility with the same
 * concept in Linux and Zephyr's L3. It is not a concept from the IEEE 802.15.4
 * standard.
 *
 * @note Currently only the original frame size from the 2006 standard version
 * and earlier is supported. The 2015+ standard introduced PHYs with larger PHY
 * payload. These are not (yet) supported in Zephyr.
 */
#define IEEE802154_MTU			(IEEE802154_MAX_PHY_PACKET_SIZE - IEEE802154_FCS_LENGTH)

/* TODO: Support flexible MTU and FCS lengths for IEEE 802.15.4-2015ff */

/** IEEE 802.15.4 short address length. */
#define IEEE802154_SHORT_ADDR_LENGTH	2

/** IEEE 802.15.4 extended address length. */
#define IEEE802154_EXT_ADDR_LENGTH	8

/** IEEE 802.15.4 maximum address length. */
#define IEEE802154_MAX_ADDR_LENGTH	IEEE802154_EXT_ADDR_LENGTH

/**
 * A special channel value that symbolizes "all" channels or "any" channel -
 * depending on context.
 */
#define IEEE802154_NO_CHANNEL		USHRT_MAX

/**
 * Represents the IEEE 802.15.4 broadcast short address, see sections 6.1 and
 * 8.4.3, table 8-94, macShortAddress.
 */
#define IEEE802154_BROADCAST_ADDRESS	     0xffff

/**
 * Represents a special IEEE 802.15.4 short address that indicates that a device
 * has been associated with a coordinator but did not receive a short address,
 * see sections 6.4.1 and 8.4.3, table 8-94, macShortAddress.
 */
#define IEEE802154_NO_SHORT_ADDRESS_ASSIGNED 0xfffe

/** Represents the IEEE 802.15.4 broadcast PAN ID, see section 6.1. */
#define IEEE802154_BROADCAST_PAN_ID 0xffff

/**
 * Represents a special value of the macShortAddress MAC PIB attribute, while the
 * device is not associated, see section 8.4.3, table 8-94.
 */
#define IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED IEEE802154_BROADCAST_ADDRESS

/**
 * Represents a special value of the macPanId MAC PIB attribute, while the
 * device is not associated, see section 8.4.3, table 8-94.
 */
#define IEEE802154_PAN_ID_NOT_ASSOCIATED	IEEE802154_BROADCAST_PAN_ID

/** Interface-level security attributes, see section 9.5. */
struct ieee802154_security_ctx {
	/**
	 * Interface-level outgoing frame counter, section 9.5, table 9-8,
	 * secFrameCounter.
	 *
	 * Only used when the driver does not implement key-specific frame
	 * counters.
	 */
	uint32_t frame_counter;

	/** @cond INTERNAL_HIDDEN */
	struct cipher_ctx enc;
	struct cipher_ctx dec;
	/** INTERNAL_HIDDEN @endcond */

	/**
	 * @brief Interface-level frame encryption security key material
	 *
	 * @details Currently native L2 only supports a single secKeySource, see
	 * section 9.5, table 9-9, in combination with secKeyMode zero (implicit
	 * key mode), see section 9.4.2.3, table 9-7.
	 *
	 * @warning This is no longer in accordance with the 2015+ versions of
	 * the standard and needs to be extended in the future for full security
	 * procedure compliance.
	 */
	uint8_t key[16];

	/** Length in bytes of the interface-level security key material. */
	uint8_t key_len;

	/**
	 * @brief Frame security level, possible values are defined in section
	 * 9.4.2.2, table 9-6.
	 *
	 * @warning Currently native L2 allows to configure one common security
	 * level for all frame types, commands and information elements. This is
	 * no longer in accordance with the 2015+ versions of the standard and
	 * needs to be extended in the future for full security procedure
	 * compliance.
	 */
	uint8_t level	: 3;

	/**
	 * @brief Frame security key mode
	 *
	 * @details Currently only implicit key mode is partially supported, see
	 * section 9.4.2.3, table 9-7, secKeyMode.
	 *
	 * @warning This is no longer in accordance with the 2015+ versions of
	 * the standard and needs to be extended in the future for full security
	 * procedure compliance.
	 */
	uint8_t key_mode	: 2;

	/** @cond INTERNAL_HIDDEN */
	uint8_t _unused	: 3;
	/** INTERNAL_HIDDEN @endcond */
};

enum ieee802154_device_role {
	IEEE802154_DEVICE_ROLE_ENDDEVICE,
	IEEE802154_DEVICE_ROLE_COORDINATOR,
	IEEE802154_DEVICE_ROLE_PAN_COORDINATOR,
};

/** IEEE 802.15.4 L2 context. */
struct ieee802154_context {
	/**
	 * @brief PAN ID
	 *
	 * @details The identifier of the PAN on which the device is operating.
	 * If this value is 0xffff, the device is not associated. See section
	 * 8.4.3.1, table 8-94, macPanId.
	 *
	 * in CPU byte order
	 */
	uint16_t pan_id;

	/**
	 * @brief Channel Number
	 *
	 * @details The RF channel to use for all transmissions and receptions,
	 * see section 11.3, table 11-2, phyCurrentChannel. The allowable range
	 * of values is PHY dependent as defined in section 10.1.3.
	 *
	 * in CPU byte order
	 */
	uint16_t channel;

	/**
	 * @brief Short Address (in CPU byte order)
	 *
	 * @details Range:
	 *  * 0x0000–0xfffd: associated, short address was assigned
	 *  * 0xfffe: associated but no short address assigned
	 *  * 0xffff: not associated (default),
	 *
	 * See section 6.4.1, table 6-4 (Usage of the shart address) and
	 * section 8.4.3.1, table 8-94, macShortAddress.
	 */
	uint16_t short_addr;

	/**
	 * @brief Extended Address (in little endian)
	 *
	 * @details The extended address is device specific, usually permanently
	 * stored on the device and immutable.
	 *
	 * See section 8.4.3.1, table 8-94, macExtendedAddress.
	 */
	uint8_t ext_addr[IEEE802154_MAX_ADDR_LENGTH];

	/** Link layer address (in big endian) */
	struct net_linkaddr_storage linkaddr;

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	/** Security context */
	struct ieee802154_security_ctx sec_ctx;
#endif

#ifdef CONFIG_NET_L2_IEEE802154_MGMT
	/** Pointer to scanning parameters and results, guarded by scan_ctx_lock */
	struct ieee802154_req_params *scan_ctx;

	/**
	 * Used to maintain integrity of data for all fields in this struct
	 * unless otherwise documented on field level.
	 */
	struct k_sem scan_ctx_lock;

	/**
	 * @brief Coordinator extended address
	 *
	 * @details see section 8.4.3.1, table 8-94, macCoordExtendedAddress,
	 * the address of the coordinator through which the device is
	 * associated.
	 *
	 * A value of zero indicates that a coordinator extended address is
	 * unknown (default).
	 *
	 * in little endian
	 */
	uint8_t coord_ext_addr[IEEE802154_MAX_ADDR_LENGTH];

	/**
	 * @brief Coordinator short address
	 *
	 * @details see section 8.4.3.1, table 8-94, macCoordShortAddress, the
	 * short address assigned to the coordinator through which the device is
	 * associated.
	 *
	 * A value of 0xfffe indicates that the coordinator is only using its
	 * extended address. A value of 0xffff indicates that this value is
	 * unknown.
	 *
	 * in CPU byte order
	 */
	uint16_t coord_short_addr;
#endif

	/** Transmission power in dBm. */
	int16_t tx_power;

	/** L2 flags */
	enum net_l2_flags flags;

	/**
	 * @brief Data sequence number
	 *
	 * @details The sequence number added to the transmitted Data frame or
	 * MAC command, see section 8.4.3.1, table 8-94, macDsn.
	 */
	uint8_t sequence;

	/**
	 * @brief Device Role
	 *
	 * @details See section 6.1: A device may be operating as end device
	 * (0), coordinator (1), or PAN coordinator (2). If no device role is
	 * explicitly configured then the device will be treated as an end
	 * device.
	 *
	 * A value of 3 is undefined.
	 *
	 * Can be read/set via @ref ieee802154_device_role.
	 */
	uint8_t device_role : 2;

	/** @cond INTERNAL_HIDDEN */
	uint8_t _unused : 5;
	/** INTERNAL_HIDDEN @endcond */

	/**
	 * ACK requested flag, guarded by ack_lock
	 */
	uint8_t ack_requested: 1;

	/** ACK expected sequence number, guarded by ack_lock */
	uint8_t ack_seq;

	/** ACK lock, guards ack_* fields */
	struct k_sem ack_lock;

	/**
	 * @brief Context lock
	 *
	 * @details This lock guards all mutable context attributes unless
	 * otherwise mentioned on attribute level.
	 */
	struct k_sem ctx_lock;
};

/** @cond INTERNAL_HIDDEN */

/* L2 context type to be used with NET_L2_GET_CTX_TYPE */
#define IEEE802154_L2_CTX_TYPE	struct ieee802154_context

/** INTERNAL_HIDDEN @endcond */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_IEEE802154_H_ */
