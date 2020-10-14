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
 */

#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LoRaWAN class types.
 */
enum lorawan_class {
	LORAWAN_CLASS_A = 0x00,
	LORAWAN_CLASS_B = 0x01,
	LORAWAN_CLASS_C = 0x02,
};

/**
 * @brief LoRaWAN activation types.
 */
enum lorawan_act_type {
	LORAWAN_ACT_OTAA = 0,
	LORAWAN_ACT_ABP,
};

/**
 * @brief LoRaWAN datarate types.
 */
enum lorawan_datarate {
	LORAWAN_DR_0 = 0,
	LORAWAN_DR_1,
	LORAWAN_DR_2,
	LORAWAN_DR_3,
	LORAWAN_DR_4,
	LORAWAN_DR_5,
	LORAWAN_DR_6,
	LORAWAN_DR_7,
	LORAWAN_DR_8,
	LORAWAN_DR_9,
	LORAWAN_DR_10,
	LORAWAN_DR_11,
	LORAWAN_DR_12,
	LORAWAN_DR_13,
	LORAWAN_DR_14,
	LORAWAN_DR_15,
};

/**
 * @brief LoRaWAN message types.
 *
 * Note: The default message type is unconfirmed.
 */
enum lorawan_message_type {
	LORAWAN_MSG_CONFIRMED = BIT(0),
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
	uint8_t *join_eui;
	uint8_t *nwk_key;
	uint8_t *app_key;
};

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

struct lorawan_join_config {
	union {
		struct lorawan_join_otaa otaa;
		struct lorawan_join_abp abp;
	};

	/** Device EUI. Optional if a secure element is present. */
	uint8_t *dev_eui;

	enum lorawan_act_type mode;
};

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
 * @param flags      Flag used to determine the type of message being sent. It
 *                   could be one of the lorawan_message_type. The default
 *                   behaviour is unconfirmed message.
 *
 * @return 0 if successful, negative errno code if failure
 */
int lorawan_send(uint8_t port, uint8_t *data, uint8_t len, uint8_t flags);

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
 * the data rate is treated as a default data rate that wil be used if the
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

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_LORAWAN_LORAWAN_H_ */
