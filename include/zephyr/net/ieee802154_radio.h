/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public IEEE 802.15.4 Radio API
 *
 * All references to the spec refer to IEEE 802.15.4-2020.
 */

#ifndef ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_H_
#define ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_H_

#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_time.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup ieee802154
 * @{
 */

/* See section 6.1: "Some of the timing parameters in definition of the MAC are in units of PHY
 * symbols. For PHYs that have multiple symbol periods, the duration to be used for the MAC
 * parameters is defined in that PHY clause."
 */
#define IEEE802154_PHY_SUN_FSK_863MHZ_915MHZ_SYMBOL_PERIOD_US 20U /* see section 19.1, table 19-1 */
#define IEEE802154_PHY_OQPSK_2450MHZ_SYMBOL_PERIOD_US         16U /* see section 12.3.3 */

/* TODO: Get PHY-specific symbol period from radio API. Requires an attribute getter, see
 *       https://github.com/zephyrproject-rtos/zephyr/issues/50336#issuecomment-1251122582.
 *       For now we assume PHYs that current drivers actually implement.
 */
#define IEEE802154_PHY_SYMBOL_PERIOD_US(is_subg_phy)                                               \
	((is_subg_phy) ? IEEE802154_PHY_SUN_FSK_863MHZ_915MHZ_SYMBOL_PERIOD_US                     \
		       : IEEE802154_PHY_OQPSK_2450MHZ_SYMBOL_PERIOD_US)

/* The inverse of the symbol period as defined in section 6.1. This is not necessarily the true
 * physical symbol period, so take care to use this macro only when either the symbol period used
 * for MAC timing is the same as the physical symbol period or if you actually mean the MAC timing
 * symbol period.
 */
#define IEEE802154_PHY_SYMBOLS_PER_SECOND(symbol_period) (USEC_PER_SEC / symbol_period)

/* in bytes, see section 19.2.4 */
#define IEEE802154_PHY_SUN_FSK_PHR_LEN 2

/* Default PHY PIB attribute aTurnaroundTime, in PHY symbols, see section 11.3, table 11-1. */
#define IEEE802154_PHY_A_TURNAROUND_TIME_DEFAULT 12U

/* PHY PIB attribute aTurnaroundTime for SUN, RS-GFSK, TVWS, and LECIM FSK PHY,
 * in PHY symbols, see section 11.3, table 11-1.
 */
#define IEEE802154_PHY_A_TURNAROUND_TIME_1MS(symbol_period)                                        \
	DIV_ROUND_UP(USEC_PER_MSEC, symbol_period)

/* TODO: Get PHY-specific turnaround time from radio API, see
 *       https://github.com/zephyrproject-rtos/zephyr/issues/50336#issuecomment-1251122582.
 *       For now we assume PHYs that current drivers actually implement.
 */
#define IEEE802154_PHY_A_TURNAROUND_TIME(is_subg_phy)                                              \
	((is_subg_phy) ? IEEE802154_PHY_A_TURNAROUND_TIME_1MS(                                     \
				 IEEE802154_PHY_SYMBOL_PERIOD_US(is_subg_phy))                     \
		       : IEEE802154_PHY_A_TURNAROUND_TIME_DEFAULT)

/* PHY PIB attribute aCcaTime, in PHY symbols, all PHYs except for SUN O-QPSK,
 * see section 11.3, table 11-1.
 */
#define IEEE802154_PHY_A_CCA_TIME 8U

/**
 * @brief IEEE 802.15.4 Channel assignments
 *
 * Channel numbering for 868 MHz, 915 MHz, and 2450 MHz bands (channel page zero).
 *
 * - Channel 0 is for 868.3 MHz.
 * - Channels 1-10 are for 906 to 924 MHz with 2 MHz channel spacing.
 * - Channels 11-26 are for 2405 to 2530 MHz with 5 MHz channel spacing.
 *
 * For more information, please refer to section 10.1.3.
 */
enum ieee802154_channel {
	IEEE802154_SUB_GHZ_CHANNEL_MIN = 0,
	IEEE802154_SUB_GHZ_CHANNEL_MAX = 10,
	IEEE802154_2_4_GHZ_CHANNEL_MIN = 11,
	IEEE802154_2_4_GHZ_CHANNEL_MAX = 26,
};

enum ieee802154_hw_caps {

	/*
	 * PHY capabilities
	 *
	 * The following capabilities describe features of the underlying radio
	 * hardware (PHY/L1).
	 *
	 * Note: A device driver must support the mandatory channel pages,
	 * frequency bands and channels of at least one IEEE 802.15.4 PHY.
	 */

	/**
	 * 2.4Ghz radio supported
	 *
	 * TODO: Replace with channel page attribute.
	 */
	IEEE802154_HW_2_4_GHZ = BIT(0),

	/**
	 * Sub-GHz radio supported
	 *
	 * TODO: Replace with channel page attribute.
	 */
	IEEE802154_HW_SUB_GHZ = BIT(1),

	/** Energy detection (ED) supported (optional) */
	IEEE802154_HW_ENERGY_SCAN = BIT(2),


	/*
	 * MAC offloading capabilities (optional)
	 *
	 * The following MAC/L2 features may optionally be offloaded to
	 * specialized hardware or proprietary driver firmware ("hard MAC").
	 *
	 * L2 implementations will have to provide a "soft MAC" fallback for
	 * these features in case the driver does not support them natively.
	 *
	 * Note: Some of these offloading capabilities may be mandatory in
	 * practice to stay within timing requirements of certain IEEE 802.15.4
	 * protocols, e.g. CPUs may not be fast enough to send ACKs within the
	 * required delays in the 2.4 GHz band without hard MAC support.
	 */

	/** Frame checksum verification supported */
	IEEE802154_HW_FCS = BIT(3),

	/** Filtering of PAN ID, extended and short address supported */
	IEEE802154_HW_FILTER = BIT(4),

	/** Promiscuous mode supported */
	IEEE802154_HW_PROMISC = BIT(5),

	/** CSMA-CA procedure supported on TX */
	IEEE802154_HW_CSMA = BIT(6),

	/** Waits for ACK on TX if AR bit is set in TX pkt */
	IEEE802154_HW_TX_RX_ACK = BIT(7),

	/** Supports retransmission on TX ACK timeout */
	IEEE802154_HW_RETRANSMISSION = BIT(8),

	/** Sends ACK on RX if AR bit is set in RX pkt */
	IEEE802154_HW_RX_TX_ACK = BIT(9),

	/** TX at specified time supported */
	IEEE802154_HW_TXTIME = BIT(10),

	/** TX directly from sleep supported */
	IEEE802154_HW_SLEEP_TO_TX = BIT(11),

	/** Timed RX window scheduling supported */
	IEEE802154_HW_RXTIME = BIT(12),

	/** TX security supported (key management, encryption and authentication) */
	IEEE802154_HW_TX_SEC = BIT(13),

	/* Note: Update also IEEE802154_HW_CAPS_BITS_COMMON_COUNT when changing
	 * the ieee802154_hw_caps type.
	 */
};

/** @brief Number of bits used by ieee802154_hw_caps type. */
#define IEEE802154_HW_CAPS_BITS_COMMON_COUNT (14)

/** @brief This and higher values are specific to the protocol- or driver-specific extensions. */
#define IEEE802154_HW_CAPS_BITS_PRIV_START IEEE802154_HW_CAPS_BITS_COMMON_COUNT

enum ieee802154_filter_type {
	IEEE802154_FILTER_TYPE_IEEE_ADDR,
	IEEE802154_FILTER_TYPE_SHORT_ADDR,
	IEEE802154_FILTER_TYPE_PAN_ID,
	IEEE802154_FILTER_TYPE_SRC_IEEE_ADDR,
	IEEE802154_FILTER_TYPE_SRC_SHORT_ADDR,
};

enum ieee802154_event {
	IEEE802154_EVENT_TX_STARTED, /* Data transmission started */
	IEEE802154_EVENT_RX_FAILED, /* Data reception failed */
	IEEE802154_EVENT_SLEEP, /* Sleep pending */
};

enum ieee802154_rx_fail_reason {
	IEEE802154_RX_FAIL_NOT_RECEIVED,  /* Nothing received */
	IEEE802154_RX_FAIL_INVALID_FCS,   /* Frame had invalid checksum */
	IEEE802154_RX_FAIL_ADDR_FILTERED, /* Address did not match */
	IEEE802154_RX_FAIL_OTHER	  /* General reason */
};

typedef void (*energy_scan_done_cb_t)(const struct device *dev,
				      int16_t max_ed);

typedef void (*ieee802154_event_cb_t)(const struct device *dev,
				      enum ieee802154_event evt,
				      void *event_params);

struct ieee802154_filter {
/** @cond INTERNAL_HIDDEN */
	union {
		uint8_t *ieee_addr; /* in little endian */
		uint16_t short_addr; /* in CPU byte order */
		uint16_t pan_id; /* in CPU byte order */
	};
/** @endcond */
};

struct ieee802154_key {
	uint8_t *key_value;
	uint32_t key_frame_counter;
	bool frame_counter_per_key;
	uint8_t key_id_mode;
	uint8_t key_index;
};

/** IEEE802.15.4 Transmission mode. */
enum ieee802154_tx_mode {
	/** Transmit packet immediately, no CCA. */
	IEEE802154_TX_MODE_DIRECT,

	/** Perform CCA before packet transmission. */
	IEEE802154_TX_MODE_CCA,

	/**
	 * Perform full CSMA/CA procedure before packet transmission.
	 *
	 * @note requires IEEE802154_HW_CSMA capability.
	 */
	IEEE802154_TX_MODE_CSMA_CA,

	/**
	 * Transmit packet in the future, at specified time, no CCA.
	 *
	 * @note requires IEEE802154_HW_TXTIME capability.
	 */
	IEEE802154_TX_MODE_TXTIME,

	/**
	 * Transmit packet in the future, perform CCA before transmission.
	 *
	 * @note requires IEEE802154_HW_TXTIME capability.
	 */
	IEEE802154_TX_MODE_TXTIME_CCA,

	/** Number of modes defined in ieee802154_tx_mode. */
	IEEE802154_TX_MODE_COMMON_COUNT,

	/** This and higher values are specific to the protocol- or driver-specific extensions. */
	IEEE802154_TX_MODE_PRIV_START = IEEE802154_TX_MODE_COMMON_COUNT,
};

/** IEEE802.15.4 Frame Pending Bit table address matching mode. */
enum ieee802154_fpb_mode {
	/** The pending bit shall be set only for addresses found in the list.
	 */
	IEEE802154_FPB_ADDR_MATCH_THREAD,

	/** The pending bit shall be cleared for short addresses found in
	 *  the list.
	 */
	IEEE802154_FPB_ADDR_MATCH_ZIGBEE,
};

/** IEEE802.15.4 driver configuration types. */
enum ieee802154_config_type {
	/** Indicates how radio driver should set Frame Pending bit in ACK
	 *  responses for Data Requests. If enabled, radio driver should
	 *  determine whether to set the bit or not based on the information
	 *  provided with ``IEEE802154_CONFIG_ACK_FPB`` config and FPB address
	 *  matching mode specified. Otherwise, Frame Pending bit should be set
	 *  to ``1`` (see section 6.7.3).
	 *  Requires IEEE802154_HW_TX_RX_ACK capability.
	 */
	IEEE802154_CONFIG_AUTO_ACK_FPB,

	/** Indicates whether to set ACK Frame Pending bit for specific address
	 *  or not. Disabling the Frame Pending bit with no address provided
	 *  (NULL pointer) should disable it for all enabled addresses.
	 *  Requires IEEE802154_HW_TX_RX_ACK capability.
	 */
	IEEE802154_CONFIG_ACK_FPB,

	/** Indicates whether the device is a PAN coordinator. */
	IEEE802154_CONFIG_PAN_COORDINATOR,

	/** Enable/disable promiscuous mode. */
	IEEE802154_CONFIG_PROMISCUOUS,

	/** Specifies new radio event handler. Specifying NULL as a handler
	 *  will disable radio events notification.
	 */
	IEEE802154_CONFIG_EVENT_HANDLER,

	/** Updates MAC keys and key index for radios supporting transmit security. */
	IEEE802154_CONFIG_MAC_KEYS,

	/** Sets the current MAC frame counter value for radios supporting transmit security. */
	IEEE802154_CONFIG_FRAME_COUNTER,

	/** Sets the current MAC frame counter value if the provided value is greater than
	 *  the current one.
	 */
	IEEE802154_CONFIG_FRAME_COUNTER_IF_LARGER,

	/** Configure a radio reception window. This can be used for any
	 *  scheduled reception, e.g.: Zigbee GP device, CSL, TSCH, etc.
	 *
	 *  @note requires IEEE802154_HW_RXTIME capability.
	 */
	IEEE802154_CONFIG_RX_SLOT,

	/** Configure CSL receiver (Endpoint) period
	 *
	 *  In order to configure a CSL receiver the upper layer should combine several
	 *  configuration options in the following way:
	 *    1. Use ``IEEE802154_CONFIG_ENH_ACK_HEADER_IE`` once to inform the radio driver of the
	 *       short and extended addresses of the peer to which it should inject CSL IEs.
	 *    2. Use ``IEEE802154_CONFIG_CSL_RX_TIME`` periodically, before each use of
	 *       ``IEEE802154_CONFIG_CSL_PERIOD`` setting parameters of the nearest CSL RX window,
	 *       and before each use of IEEE_CONFIG_RX_SLOT setting parameters of the following (not
	 *       the nearest one) CSL RX window, to allow the radio driver to calculate the proper
	 *       CSL Phase to the nearest CSL window to inject in the CSL IEs for both transmitted
	 *       data and ACK frames.
	 *    3. Use ``IEEE802154_CONFIG_CSL_PERIOD`` on each value change to update the current CSL
	 *       period value which will be injected in the CSL IEs together with the CSL Phase
	 *       based on ``IEEE802154_CONFIG_CSL_RX_TIME``.
	 *    4. Use ``IEEE802154_CONFIG_RX_SLOT`` periodically to schedule the immediate receive
	 *       window earlier enough before the expected window start time, taking into account
	 *       possible clock drifts and scheduling uncertainties.
	 *
	 *  This diagram shows the usage of the four options over time:
	 *        Start CSL                                  Schedule CSL window
	 *
	 *    ENH_ACK_HEADER_IE                        CSL_RX_TIME (following window)
	 *         |                                        |
	 *         | CSL_RX_TIME (nearest window)           | RX_SLOT (nearest window)
	 *         |    |                                   |   |
	 *         |    | CSL_PERIOD                        |   |
	 *         |    |    |                              |   |
	 *         v    v    v                              v   v
	 *    ----------------------------------------------------------[ CSL window ]-----+
	 *                                            ^                                    |
	 *                                            |                                    |
	 *                                            +--------------------- loop ---------+
	 */
	IEEE802154_CONFIG_CSL_PERIOD,

	/** Configure the next CSL receive window (i.e. "channel sample") center,
	 *  in units of nanoseconds relative to the network subsystem's local clock.
	 */
	IEEE802154_CONFIG_CSL_RX_TIME,

	/** Indicates whether to inject IE into ENH ACK Frame for specific address
	 *  or not. Disabling the ENH ACK with no address provided (NULL pointer)
	 *  should disable it for all enabled addresses.
	 */
	IEEE802154_CONFIG_ENH_ACK_HEADER_IE,

	/** Number of types defined in ieee802154_config_type. */
	IEEE802154_CONFIG_COMMON_COUNT,

	/** This and higher values are specific to the protocol- or driver-specific extensions. */
	IEEE802154_CONFIG_PRIV_START = IEEE802154_CONFIG_COMMON_COUNT,
};

/** IEEE802.15.4 driver configuration data. */
struct ieee802154_config {
	/** Configuration data. */
	union {
		/** ``IEEE802154_CONFIG_AUTO_ACK_FPB`` */
		struct {
			bool enabled;
			enum ieee802154_fpb_mode mode;
		} auto_ack_fpb;

		/** ``IEEE802154_CONFIG_ACK_FPB`` */
		struct {
			uint8_t *addr; /* in little endian for both, short and extended address */
			bool extended;
			bool enabled;
		} ack_fpb;

		/** ``IEEE802154_CONFIG_PAN_COORDINATOR`` */
		bool pan_coordinator;

		/** ``IEEE802154_CONFIG_PROMISCUOUS`` */
		bool promiscuous;

		/** ``IEEE802154_CONFIG_EVENT_HANDLER`` */
		ieee802154_event_cb_t event_handler;

		/** ``IEEE802154_CONFIG_MAC_KEYS``
		 *  Pointer to an array containing a list of keys used
		 *  for MAC encryption. Refer to secKeyIdLookupDescriptor and
		 *  secKeyDescriptor in IEEE 802.15.4
		 *
		 *  key_value field points to a buffer containing the 16 byte
		 *  key. The buffer is copied by the callee.
		 *
		 *  The variable length array is terminated by key_value field
		 *  set to NULL.
		 */
		struct ieee802154_key *mac_keys;

		/** ``IEEE802154_CONFIG_FRAME_COUNTER`` */
		uint32_t frame_counter;

		/** ``IEEE802154_CONFIG_RX_SLOT`` */
		struct {
			/**
			 * Nanosecond resolution timestamp relative to the
			 * network subsystem's local clock defining the start of
			 * the RX window during which the receiver is expected
			 * to be listening (i.e. not including any startup
			 * times).
			 */
			net_time_t start;

			/**
			 * Nanosecond resolution duration of the RX window
			 * relative to the above RX window start time during
			 * which the receiver is expected to be listening (i.e.
			 * not including any shutdown times).
			 */
			net_time_t duration;

			uint8_t channel;
		} rx_slot;

		/**
		 * ``IEEE802154_CONFIG_CSL_PERIOD``
		 *
		 * The CSL period in units of 10 symbol periods,
		 * see section 7.4.2.3.
		 *
		 * in CPU byte order
		 */
		uint32_t csl_period;

		/**
		 * ``IEEE802154_CONFIG_CSL_RX_TIME``
		 *
		 * Nanosecond resolution timestamp relative to the network
		 * subsystem's local clock defining the center of the CSL RX window
		 * at which the receiver is expected to be fully started up
		 * (i.e.  not including any startup times).
		 */
		net_time_t csl_rx_time;

		/** ``IEEE802154_CONFIG_ENH_ACK_HEADER_IE`` */
		struct {
			/**
			 * header IEs to be added to the Enh-Ack frame
			 *
			 * in little endian
			 */
			const uint8_t *data;
			uint16_t data_len;
			/** in CPU byte order */
			uint16_t short_addr;
			/** in big endian */
			const uint8_t *ext_addr;
		} ack_ie;
	};
};

/**
 * @brief IEEE 802.15.4 driver attributes.
 *
 * See @ref ieee802154_attr_value and @ref ieee802154_radio_api for usage
 * details.
 */
enum ieee802154_attr {
	/** Number of attributes defined in ieee802154_attr. */
	IEEE802154_ATTR_COMMON_COUNT,

	/** This and higher values are specific to the protocol- or driver-specific extensions. */
	IEEE802154_ATTR_PRIV_START = IEEE802154_ATTR_COMMON_COUNT,
};

/**
 * @brief IEEE 802.15.4 driver attributes.
 *
 * @details This structure is reserved to scalar and structured attributes that
 * originate in the driver implementation and can neither be implemented as
 * boolean @ref ieee802154_hw_caps nor be derived directly or indirectly by the
 * MAC (L2) layer. In particular this structure MUST NOT be used to return
 * configuration data that originate from L2.
 *
 * @note To keep this union reasonably small, any attribute requiring a large
 * memory area, SHALL be provided pointing to memory allocated from the driver's
 * stack. Clients that need to persist the attribute value SHALL therefore copy
 * such memory before returning control to the driver.
 */
struct ieee802154_attr_value {
	union {
		/* TODO: Please remove when first attribute is added. */
		uint8_t dummy;

		/* TODO: Add driver specific PHY attributes (symbol rate,
		 * aTurnaroundTime, aCcaTime, channels, channel pages, etc.)
		 */
	};
};

/**
 * @brief IEEE 802.15.4 radio interface API.
 *
 */
struct ieee802154_radio_api {
	/**
	 * @brief network interface API
	 *
	 * @note Network devices must extend the network interface API. It is
	 * therefore mandatory to place it at the top of the radio API struct so
	 * that it can be cast to a network interface.
	 */
	struct net_if_api iface_api;

	/**
	 * @brief Get the device driver capabilities.
	 *
	 * @param dev pointer to radio device
	 *
	 * @return Bit field with all supported device driver capabilities.
	 */
	enum ieee802154_hw_caps (*get_capabilities)(const struct device *dev);

	/**
	 * @brief Clear Channel Assessment - Check channel's activity
	 *
	 * @param dev pointer to radio device
	 *
	 * @retval 0 the channel is available
	 * @retval -EBUSY The channel is busy.
	 * @retval -EIO The CCA procedure could not be executed.
	 * @retval -ENOTSUP CCA is not supported by this driver.
	 */
	int (*cca)(const struct device *dev);

	/**
	 * @brief Set current channel
	 *
	 * @param dev pointer to radio device
	 * @param channel the number of the channel to be set in CPU byte order
	 *
	 * @retval 0 channel was successfully set
	 * @retval -EINVAL The given channel is not within the range of valid
	 * channels of the driver's current channel page.
	 * @retval -ENOTSUP The given channel is within the range of valid
	 * channels of the driver's current channel page but unsupported by the
	 * current driver.
	 * @retval -EIO The channel could not be set.
	 */
	int (*set_channel)(const struct device *dev, uint16_t channel);

	/**
	 * @brief Set/Unset PAN ID, extended or short address filters.
	 *
	 * @note requires IEEE802154_HW_FILTER capability.
	 *
	 * @param dev pointer to radio device
	 * @param set true to set the filter, false to remove it
	 * @param type the type of entity to be added/removed from the filter
	 * list (a PAN ID or a source/destination address)
	 * @param filter the entity to be added/removed from the filter list
	 *
	 * @retval 0 The filter was successfully added/removed.
	 * @retval -EINVAL The given filter entity or filter entity type
	 * was not valid.
	 * @retval -ENOTSUP Setting/removing this filter or filter type
	 * is not supported by this driver.
	 * @retval -EIO Error while setting/removing the filter.
	 */
	int (*filter)(const struct device *dev,
		      bool set,
		      enum ieee802154_filter_type type,
		      const struct ieee802154_filter *filter);

	/**
	 * @brief Set TX power level in dbm
	 *
	 * @param dev pointer to radio device
	 * @param dbm TX power in dbm
	 *
	 * @retval 0 The TX power was successfully set.
	 * @retval -EINVAL The given dbm value is invalid or not supported by
	 * the driver.
	 * @retval -EIO The TX power could not be set.
	 */
	int (*set_txpower)(const struct device *dev, int16_t dbm);

	/**
	 * @brief Transmit a packet fragment as a single frame
	 *
	 * @warning The driver must not take ownership of the given network
	 * packet and frame (fragment) buffer. Any data required by the driver
	 * (including the actual frame content) must be read synchronously and
	 * copied internally if transmission is delayed or executed
	 * asynchronously. Both, the packet and the buffer may be re-used or
	 * released immediately after the function returns.
	 *
	 * @note Depending on the level of offloading features supported by the
	 * driver, the frame may not be fully encrypted/authenticated, may not
	 * contain an FCS or may contain incomplete information elements (IEs).
	 * It is the responsibility of L2 implementations to prepare the frame
	 * according to the offloading capabilities announced by the driver and
	 * to decide whether CCA, CSMA/CA or ACK procedures need to be executed
	 * in software ("soft MAC") or will be provided by the driver itself
	 * ("hard MAC").
	 *
	 * @param dev pointer to radio device
	 * @param mode the transmission mode, some of which require specific
	 * offloading capabilities.
	 * @param pkt pointer to the network packet to be transmitted.
	 * @param frag pointer to a network buffer containing a single fragment
	 * with the frame data to be transmitted
	 *
	 * @retval 0 The frame was successfully sent or scheduled. If the driver
	 * supports ACK offloading and the frame requested acknowlegment (AR bit
	 * set), this means that the packet was successfully acknowledged by its
	 * peer.
	 * @retval -ENOTSUP The given TX mode is not supported.
	 * @retval -EIO The frame could not be sent due to some unspecified
	 * error.
	 * @retval -EBUSY The frame could not be sent because the medium was
	 * busy (CSMA/CA or CCA offloading feature only).
	 * @retval -ENOMSG The frame was not confirmed by an ACK packet (TX ACK
	 * offloading feature only).
	 * @retval -ENOBUFS The frame could not be scheduled due to missing
	 * internal buffer resources (timed TX offloading feature only).
	 */
	int (*tx)(const struct device *dev, enum ieee802154_tx_mode mode,
		  struct net_pkt *pkt, struct net_buf *frag);

	/**
	 * @brief Start the device and place it in receive mode.
	 *
	 * @param dev pointer to radio device
	 *
	 * @retval 0 The driver was successfully started.
	 * @retval -EIO The driver could not be started.
	 */
	int (*start)(const struct device *dev);

	/**
	 * @brief Stop the device and switch off the receiver (sleep mode).
	 *
	 * @param dev pointer to radio device
	 *
	 * @retval 0 The driver was successfully stopped.
	 * @retval -EIO The driver could not be stopped.
	 */
	int (*stop)(const struct device *dev);

	/**
	 * @brief Start continuous carrier wave transmission.
	 *
	 * @details To leave this mode, `start()` or `stop()` should be called,
	 * putting the radio driver in receive or sleep mode, respectively.
	 *
	 * @param dev pointer to radio device
	 *
	 * @retval 0 continuous carrier wave transmission started
	 * @retval -EIO not started
	 */
	int (*continuous_carrier)(const struct device *dev);

	/**
	 * @brief Set radio driver configuration.
	 *
	 * @param dev pointer to radio device
	 * @param type the configuration type to be set
	 * @param config the configuration parameters to be set for the given
	 * configuration type
	 *
	 * @retval 0 configuration successful
	 * @retval -ENOTSUP The given configuration type is not supported by
	 * this driver.
	 * @retval -EINVAL The configuration parameters are invalid for the
	 * given configuration type.
	 * @retval -ENOMEM The configuration cannot be saved due to missing
	 * memory resources.
	 * @retval -ENOENT The resource referenced in the configuration
	 * parameters cannot be found in the configuration.
	 */
	int (*configure)(const struct device *dev,
			 enum ieee802154_config_type type,
			 const struct ieee802154_config *config);

	/**
	 * @brief Get the available amount of Sub-GHz channels.
	 *
	 * TODO: Replace with a combination of channel page and channel
	 * attributes.
	 *
	 * @param dev pointer to radio device
	 *
	 * @return number of available channels in the sub-gigahertz band
	 */
	uint16_t (*get_subg_channel_count)(const struct device *dev);

	/**
	 * @brief Run an energy detection scan.
	 *
	 * @note requires IEEE802154_HW_ENERGY_SCAN capability
	 *
	 * @note The radio channel must be set prior to calling this function.
	 *
	 * @param dev pointer to radio device
	 * @param duration duration of energy scan in ms
	 * @param done_cb function called when the energy scan has finished
	 *
	 * @retval 0 the energy detection scan was successfully scheduled
	 *
	 * @retval -EBUSY the energy detection scan could not be scheduled at
	 * this time
	 * @retval -EALREADY a previous energy detection scan has not finished
	 * yet.
	 * @retval -ENOTSUP This driver does not support energy scans.
	 */
	int (*ed_scan)(const struct device *dev,
		       uint16_t duration,
		       energy_scan_done_cb_t done_cb);

	/**
	 * @brief Get the current time in nanoseconds relative to the network
	 * subsystem's local uptime clock as represented by this network
	 * interface.
	 *
	 * See @ref net_time_t for semantic details.
	 *
	 * @note requires IEEE802154_HW_TXTIME and/or IEEE802154_HW_RXTIME
	 * capabilities.
	 *
	 * @param dev pointer to radio device
	 *
	 * @return nanoseconds relative to the network subsystem's local clock
	 */
	net_time_t (*get_time)(const struct device *dev);

	/**
	 * @brief Get the current estimated worst case accuracy (maximum ±
	 * deviation from the nominal frequency) of the network subsystem's
	 * local clock used to calculate tolerances and guard times when
	 * scheduling delayed receive or transmit radio operations.
	 *
	 * The deviation is given in units of PPM (parts per million).
	 *
	 * @note requires IEEE802154_HW_TXTIME and/or IEEE802154_HW_RXTIME
	 * capabilities.
	 *
	 * @note Implementations may estimate this value based on current
	 * operating conditions (e.g. temperature).
	 *
	 * @param dev pointer to radio device
	 *
	 * @return current estimated clock accuracy in PPM
	 */
	uint8_t (*get_sch_acc)(const struct device *dev);

	/**
	 * @brief Get the value of a driver specific attribute.
	 *
	 * @note This function SHALL NOT return any values configurable by the
	 * MAC (L2) layer. It is reserved to non-boolean (i.e. scalar or
	 * structured) attributes that originate from the driver implementation
	 * and cannot be directly or indirectly derived by L2. Boolean
	 * attributes SHALL be implemented as @ref ieee802154_hw_caps.
	 *
	 * @retval 0 The requested attribute is supported by the driver and the
	 * value can be retrieved from the corresponding @ref ieee802154_attr_value
	 * member.
	 *
	 * @retval -ENOENT The driver does not provide the requested attribute.
	 * The value structure has does not been updated with attribute data.
	 */
	int (*attr_get)(const struct device *dev,
			enum ieee802154_attr attr,
			struct ieee802154_attr_value *value);
};

/* Make sure that the network interface API is properly setup inside
 * IEEE 802154 radio API struct (it is the first one).
 */
BUILD_ASSERT(offsetof(struct ieee802154_radio_api, iface_api) == 0);

#define IEEE802154_AR_FLAG_SET (0x20)

/**
 * @brief Check if AR flag is set on the frame inside given net_pkt
 *
 * @param frag A valid pointer on a net_buf structure, must not be NULL,
 *        and its length should be at least 1 byte (ImmAck frames are the
 *        shortest supported frames with 3 bytes excluding FCS).
 *
 * @return true if AR flag is set, false otherwise
 */
static inline bool ieee802154_is_ar_flag_set(struct net_buf *frag)
{
	return (*frag->data & IEEE802154_AR_FLAG_SET);
}

/**
 * @brief Radio driver ACK handling callback into L2 that radio
 *        drivers must call when receiving an ACK package.
 *
 * @details The IEEE 802.15.4 standard prescribes generic procedures for ACK
 *          handling on L2 (MAC) level. L2 stacks therefore have to provides a
 *          fast and re-usable generic implementation of this callback for
 *          radio drivers to call when receiving an ACK packet.
 *
 *          Note: This function is part of Zephyr's 802.15.4 stack L1 -> L2
 *          "inversion-of-control" adaptation API and must be implemented by
 *          all IEEE 802.15.4 L2 stacks.
 *
 * @param iface A valid pointer on a network interface that received the packet
 * @param pkt A valid pointer on a packet to check
 *
 * @return NET_OK if L2 handles the ACK package, NET_CONTINUE or NET_DROP otherwise.
 *
 *         Note: Deviating from other functions in the net stack returning net_verdict,
 *               this function will not unref the package even if it returns NET_OK.
 *
 *         TODO: Fix this deviating behavior.
 */
extern enum net_verdict ieee802154_handle_ack(struct net_if *iface, struct net_pkt *pkt);

/**
 * @brief Radio driver initialization callback into L2 called by radio drivers
 *        to initialize the active L2 stack for a given interface.
 *
 * @details Radio drivers must call this function as part of their own
 *          initialization routine.
 *
 *          Note: This function is part of Zephyr's 802.15.4 stack L1 -> L2
 *          "inversion-of-control" adaptation API and must be implemented by
 *          all IEEE 802.15.4 L2 stacks.
 *
 * @param iface A valid pointer on a network interface
 */
#ifndef CONFIG_IEEE802154_RAW_MODE
extern void ieee802154_init(struct net_if *iface);
#else
#define ieee802154_init(_iface_)
#endif /* CONFIG_IEEE802154_RAW_MODE */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_H_ */
