/*
 * Copyright (c) 2020 Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_LORAWAN_LORAWAN_H_
#define ZEPHYR_INCLUDE_LORAWAN_LORAWAN_H_

/**
 * @file
 * @brief Public LoRaWAN APIs
 * @defgroup lorawan_api LoRaWAN APIs
 * @since 2.5
 * @version 0.1.0
 * @ingroup connectivity
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LoRaWAN class types.
 */
enum lorawan_class {
	LORAWAN_CLASS_A = 0x00, /**< Class A device */
	LORAWAN_CLASS_B = 0x01, /**< Class B device */
	LORAWAN_CLASS_C = 0x02, /**< Class C device */
};

/**
 * @brief LoRaWAN activation types.
 */
enum lorawan_act_type {
	LORAWAN_ACT_OTAA = 0, /**< Over-the-Air Activation (OTAA) */
	LORAWAN_ACT_ABP,      /**< Activation by Personalization (ABP) */
};

/**
 * @brief LoRaWAN datarate types.
 */
enum lorawan_datarate {
	LORAWAN_DR_0 = 0, /**< DR0 data rate */
	LORAWAN_DR_1,     /**< DR1 data rate */
	LORAWAN_DR_2,     /**< DR2 data rate */
	LORAWAN_DR_3,     /**< DR3 data rate */
	LORAWAN_DR_4,     /**< DR4 data rate */
	LORAWAN_DR_5,     /**< DR5 data rate */
	LORAWAN_DR_6,     /**< DR6 data rate */
	LORAWAN_DR_7,     /**< DR7 data rate */
	LORAWAN_DR_8,     /**< DR8 data rate */
	LORAWAN_DR_9,     /**< DR9 data rate */
	LORAWAN_DR_10,    /**< DR10 data rate */
	LORAWAN_DR_11,    /**< DR11 data rate */
	LORAWAN_DR_12,    /**< DR12 data rate */
	LORAWAN_DR_13,    /**< DR13 data rate */
	LORAWAN_DR_14,    /**< DR14 data rate */
	LORAWAN_DR_15,    /**< DR15 data rate */
};

/**
 * @brief LoRaWAN region types.
 */
enum lorawan_region {
	LORAWAN_REGION_AS923, /**< Asia 923 MHz frequency band */
	LORAWAN_REGION_AU915, /**< Australia 915 MHz frequency band */
	LORAWAN_REGION_CN470, /**< China 470 MHz frequency band */
	LORAWAN_REGION_CN779, /**< China 779 MHz frequency band */
	LORAWAN_REGION_EU433, /**< Europe 433 MHz frequency band */
	LORAWAN_REGION_EU868, /**< Europe 868 MHz frequency band */
	LORAWAN_REGION_KR920, /**< South Korea 920 MHz frequency band */
	LORAWAN_REGION_IN865, /**< India 865 MHz frequency band */
	LORAWAN_REGION_US915, /**< United States 915 MHz frequency band */
	LORAWAN_REGION_RU864, /**< Russia 864 MHz frequency band */
};

/**
 * @brief LoRaWAN message types.
 */
enum lorawan_message_type {
	LORAWAN_MSG_UNCONFIRMED = 0,  /**< Unconfirmed message */
	LORAWAN_MSG_CONFIRMED,        /**< Confirmed message */
};

/**
 * @brief LoRaWAN join parameters for over-the-Air activation (OTAA)
 *
 * Note that all of the fields use LoRaWAN 1.1 terminology.
 *
 * All parameters are optional if a secure element is present in which
 * case the values stored in the secure element will be used instead.
 */
struct lorawan_join_otaa {
	/** Join EUI */
	uint8_t *join_eui;
	/** Network Key */
	uint8_t *nwk_key;
	/** Application Key */
	uint8_t *app_key;
	/**
	 * Device Nonce
	 *
	 * Starting with LoRaWAN 1.0.4 the DevNonce must be monotonically
	 * increasing for each OTAA join with the same EUI. The DevNonce
	 * should be stored in non-volatile memory by the application.
	 */
	uint16_t dev_nonce;
};

/**
 * @brief LoRaWAN join parameters for activation by personalization (ABP)
 */
struct lorawan_join_abp {
	/** Device address on the network */
	uint32_t dev_addr;
	/** Application session key */
	uint8_t *app_skey;
	/** Network session key */
	uint8_t *nwk_skey;
	/** Application EUI */
	uint8_t *app_eui;
};

/**
 * @brief LoRaWAN join parameters
 */
struct lorawan_join_config {
	/** Join parameters */
	union {
		struct lorawan_join_otaa otaa; /**< OTAA join parameters */
		struct lorawan_join_abp abp;   /**< ABP join parameters */
	};

	/** Device EUI. Optional if a secure element is present. */
	uint8_t *dev_eui;

	/** Activation mode */
	enum lorawan_act_type mode;
};

/** Flag to indicate receiving on any port */
#define LW_RECV_PORT_ANY UINT16_MAX

/**
 * @brief LoRaWAN downlink callback parameters
 */
struct lorawan_downlink_cb {
	/**
	 * @brief Port to handle messages for.
	 *
	 * - Port 0: TX packet acknowledgements
	 * - Ports 1-255: Standard downlink port
	 * - LW_RECV_PORT_ANY: All downlinks
	 */
	uint16_t port;
	/**
	 * @brief Callback function to run on downlink data
	 *
	 * @note Callbacks are run on the system workqueue,
	 *       and should therefore be as short as possible.
	 *
	 * @param port Port message was sent on
	 * @param data_pending Network server has more downlink packets pending
	 * @param rssi Received signal strength in dBm
	 * @param snr Signal to Noise ratio in dBm
	 * @param len Length of data received, will be 0 for ACKs
	 * @param data Data received, will be NULL for ACKs
	 */
	void (*cb)(uint8_t port, bool data_pending,
		   int16_t rssi, int8_t snr,
		   uint8_t len, const uint8_t *data);
	/** Node for callback list */
	sys_snode_t node;
};

/**
 * @brief Defines the battery level callback handler function signature.
 *
 * @retval 0      if the node is connected to an external power source
 * @retval 1..254 battery level, where 1 is the minimum and 254 is the maximum value
 * @retval 255    if the node was not able to measure the battery level
 */
typedef uint8_t (*lorawan_battery_level_cb_t)(void);

/**
 * @brief Defines the datarate changed callback handler function signature.
 *
 * @param dr Updated datarate.
 */
typedef void (*lorawan_dr_changed_cb_t)(enum lorawan_datarate dr);

/**
 * @brief Register a battery level callback function.
 *
 * Provide the LoRaWAN stack with a function to be called whenever a battery
 * level needs to be read.
 *
 * Should no callback be provided the lorawan backend will report 255.
 *
 * @param cb Pointer to the battery level function
 */
void lorawan_register_battery_level_callback(lorawan_battery_level_cb_t cb);

/**
 * @brief Register a callback to be run on downlink packets
 *
 * @param cb Pointer to structure containing callback parameters
 */
void lorawan_register_downlink_callback(struct lorawan_downlink_cb *cb);

/**
 * @brief Register a callback to be called when the datarate changes
 *
 * The callback is called once upon successfully joining a network and again
 * each time the datarate changes due to ADR.
 *
 * @param cb Pointer to datarate update callback
 */
void lorawan_register_dr_changed_callback(lorawan_dr_changed_cb_t cb);

/**
 * @brief Join the LoRaWAN network
 *
 * Join the LoRaWAN network using OTAA or AWB.
 *
 * @param config Configuration to be used
 *
 * @return 0 if successful, negative errno code if failure
 */
int lorawan_join(const struct lorawan_join_config *config);

/**
 * @brief Start the LoRaWAN stack
 *
 * This function need to be called before joining the network.
 *
 * @return 0 if successful, negative errno code if failure
 */
int lorawan_start(void);

/**
 * @brief Send data to the LoRaWAN network
 *
 * Send data to the connected LoRaWAN network.
 *
 * @param port       Port to be used for sending data. Must be set if the
 *                   payload is not empty.
 * @param data       Data buffer to be sent
 * @param len        Length of the buffer to be sent. Maximum length of this
 *                   buffer is 255 bytes but the actual payload size varies with
 *                   region and datarate.
 * @param type       Specifies if the message shall be confirmed or unconfirmed.
 *                   Must be one of @ref lorawan_message_type.
 *
 * @return 0 if successful, negative errno code if failure
 */
int lorawan_send(uint8_t port, uint8_t *data, uint8_t len, enum lorawan_message_type type);

/**
 * @brief Set the current device class
 *
 * Change the current device class. This function may be called before
 * or after a network connection has been established.
 *
 * @param dev_class New device class
 *
 * @return 0 if successful, negative errno code if failure
 */
int lorawan_set_class(enum lorawan_class dev_class);

/**
 * @brief Set the number of tries used for transmissions
 *
 * @param tries Number of tries to be used
 *
 * @return 0 if successful, negative errno code if failure
 */
int lorawan_set_conf_msg_tries(uint8_t tries);

/**
 * @brief Enable Adaptive Data Rate (ADR)
 *
 * Control whether adaptive data rate (ADR) is enabled. When ADR is enabled,
 * the data rate is treated as a default data rate that will be used if the
 * ADR algorithm has not established a data rate. ADR should normally only
 * be enabled for devices with stable RF conditions (i.e., devices in a mostly
 * static location).
 *
 * @param enable Enable or Disable adaptive data rate.
 */
void lorawan_enable_adr(bool enable);

/**
 * @brief Set the default data rate
 *
 * Change the default data rate.
 *
 * @param dr Data rate used for transmissions
 *
 * @return 0 if successful, negative errno code if failure
 */
int lorawan_set_datarate(enum lorawan_datarate dr);

/**
 * @brief Get the minimum possible datarate
 *
 * The minimum possible datarate may change in response to a TxParamSetupReq
 * command from the network server.
 *
 * @return Minimum possible data rate
 */
enum lorawan_datarate lorawan_get_min_datarate(void);

/**
 * @brief Get the current payload sizes
 *
 * Query the current payload sizes. The maximum payload size varies with
 * datarate, while the current payload size can be less due to MAC layer
 * commands which are inserted into uplink packets.
 *
 * @param max_next_payload_size Maximum payload size for the next transmission
 * @param max_payload_size Maximum payload size for this datarate
 */
void lorawan_get_payload_sizes(uint8_t *max_next_payload_size,
			       uint8_t *max_payload_size);

/**
 * @brief Set the region and frequency to be used
 *
 * Control the LoRa region and frequency settings. This should be called before
 * @a lorawan_start(). If you only have support for a single region selected via
 * Kconfig, this function does not need to be called at all.
 *
 * @param region The region to be selected
 * @return 0 if successful, negative errno otherwise
 */
int lorawan_set_region(enum lorawan_region region);

#ifdef CONFIG_LORAWAN_APP_CLOCK_SYNC

/**
 * @brief Run Application Layer Clock Synchronization service
 *
 * This service sends out its current time in a regular interval (configurable
 * via Kconfig) and receives a correction offset from the application server if
 * the clock deviation is considered too large.
 *
 * Clock synchronization is required for firmware upgrades over multicast
 * sessions, but can also be used independent of a FUOTA process.
 *
 * @return 0 if successful, negative errno otherwise.
 */
int lorawan_clock_sync_run(void);

/**
 * @brief Retrieve the current synchronized time
 *
 * This function uses the GPS epoch format, as used in all LoRaWAN services.
 *
 * The GPS epoch started on 1980-01-06T00:00:00Z, but has since diverged
 * from UTC, as it does not consider corrections like leap seconds.
 *
 * @param gps_time Synchronized time in GPS epoch format truncated to 32-bit.
 *
 * @return 0 if successful, -EAGAIN if the clock is not yet synchronized.
 */
int lorawan_clock_sync_get(uint32_t *gps_time);

#endif /* CONFIG_LORAWAN_APP_CLOCK_SYNC */

#ifdef CONFIG_LORAWAN_FRAG_TRANSPORT

/**
 * @brief Run Fragmented Data Block Transport service
 *
 * This service receives fragmented data (usually firmware images) and
 * stores them in the image-1 flash partition.
 *
 * After all fragments have been received, the provided callback is invoked.
 *
 * @param transport_finished_cb Callback for notification of finished data transfer.
 *
 * @return 0 if successful, negative errno otherwise.
 */
int lorawan_frag_transport_run(void (*transport_finished_cb)(void));

#endif /* CONFIG_LORAWAN_FRAG_TRANSPORT */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_LORAWAN_LORAWAN_H_ */
