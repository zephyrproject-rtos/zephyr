/*
 * Copyright (c) 2023 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Modem SMS for SMS common structure.
 *
 * Modem SMS handling for modem driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_SMS_H_
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_SMS_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#define SMS_PHONE_MAX_LEN 16
#define SMS_TIME_MAX_LEN  26

/*
 * Structure for an outgoing SMS message
 *   Note: All fields in sms_out and sms_in are NULL terminated
 */
struct sms_out {
	/* Target phone number */
	char phone[SMS_PHONE_MAX_LEN];

	/* Character array containing the message to be sent, NULL terminated */
	char msg[CONFIG_MODEM_SMS_OUT_MSG_MAX_LEN + 1];
};

/*
 * Structure for an incoming SMS message
 *   Note: All fields in sms_out and sms_in are NULL terminated
 */
struct sms_in {

	/* Message orignator number */
	char phone[SMS_PHONE_MAX_LEN];

	/* SCTS Timestamp from the message */
	char time[SMS_TIME_MAX_LEN];

	/* Character array containing the message received, NULL terminated */
	char msg[CONFIG_MODEM_SMS_IN_MSG_MAX_LEN + 1];

	/* Concatenated SMS reference number */
	uint8_t csms_ref;

	/* Concatednamed SMS index number */
	uint8_t csms_idx;

	/* SMS Receive Timeout */
	k_timeout_t timeout;
};

/* Modem context SMS send function */

/**
 * @brief Modem context SMS send function
 *
 * @param sms Pointer to an sms_out struct containing the message to be sent
 * @return 0 on success, negative error value on failure
 */
typedef int (*send_sms_func_t)(const struct sms_out *sms);

/**
 * @brief Modem context SMS receive function
 *
 * @param sms Pointer to an sms_in struct to which the message shall be written
 * @return 0 on success, negative error value on failure
 */
typedef int (*recv_sms_func_t)(struct sms_in *sms);

/**
 * @brief Modem context SMS receive callback enable function
 *
 * @param enable Enablement status: true to enable, false to disable
 * @return 0 on success, negative error value on failure
 */
typedef int (*recv_sms_cb_en_func_t)(bool enable);

/**
 * Keeping this for compatibility, but this could likely be removed
 */
enum io_ctl {
	SMS_SEND,
	SMS_RECV,
};

/*
 * SMS Receive callback structure.
 */
struct sms_recv_cb {
	/**
	 * @brief A new SMS message has been received.
	 *
	 * This callback notifies the application of a new message.
	 *
	 *
	 * @param dev Device pointer to the modem which received the message
	 * @param sms Received SMS Message
	 * @param csms_ref CSMS Reference number (if available, value is set to -1 if not)
	 * @param csms_idx CSMS Index number (if available, value is set to 1 if not)
	 * @param csms_tot CSMS Total segment count (if available, value is set to 1 if not)
	 *
	 */
	void (*recv)(const struct device *dev, struct sms_in *sms, int csms_ref, int csms_idx,
		     int csms_tot);

	/** Internally used field for list handling */
	sys_snode_t node;
};

/**
 * @brief Send SMS from a modem
 *
 * @param dev Device pointer for the modem from which the message shall be read
 * @param sms Pointer to an sms_out struct containing the message to be sent
 * @return 0 on success, negative error value on failure
 */
int sms_msg_send(const struct device *dev, const struct sms_out *sms);

/**
 * @brief Read SMS message from a modem
 *
 * @param dev Device pointer for the modem from which the message shall be read
 * @param sms Pointer to an sms_in struct to which the message shall be written
 * @return 0 on success, negative error value on failure
 */
int sms_msg_recv(const struct device *dev, struct sms_in *sms);

/**
 * @brief Enable SMS receive callbacks on a specified modem
 *
 * @param dev Device pointer for the modem on which to enable callbacks
 * @param enable Enablement status: true to enable, false to disable
 * @return 0 on success, negative error value on failure
 */
int sms_recv_cb_en(const struct device *dev, bool enable);

/**
 * @brief Register SMS receive callbacks.
 *
 * @param cb Callback struct.
 *
 * @return Zero on success or negative error code otherwise
 */
int sms_recv_cb_register(struct sms_recv_cb *cb);

/**
 * @brief Unregister SMS receive callbacks.
 *
 * @param cb Callback struct.
 *
 * @return Zero on success or negative error code otherwise
 */
int sms_recv_cb_unregister(struct sms_recv_cb *cb);

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_SMS_H_ */
