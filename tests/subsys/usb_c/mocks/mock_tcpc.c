/*
 * Copyright 2023 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mock_tcpc

#include <zephyr/logging/log.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <stddef.h>

#include "mock_tcpc.h"
#include "mock_tester.h"

/**
 * @brief The message header consists of 2-bytes
 */
#define MSG_HEADER_SIZE 2

/**
 * @brief Get the state of the CC1 and CC2 lines
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int mt_get_cc(const struct device *dev, enum tc_cc_voltage_state *cc1,
		     enum tc_cc_voltage_state *cc2)
{
	*cc1 = tester_get_cc1();
	*cc2 = tester_get_cc2();

	return 0;
}

/**
 * @brief Set the CC pull resistor
 *
 * @retval 0 on success
 */
static int mt_set_cc(const struct device *dev, enum tc_cc_pull pull)
{
	tester_set_uut_cc(pull);

	return 0;
}

/**
 * @brief Set the CC polrity
 *
 * @return 0 on success
 */
static int mt_set_polarity(const struct device *dev, enum tc_cc_polarity polarity)
{
	tester_set_uut_polarity(polarity);

	return 0;
}

/**
 * @brief Enable or Disable Power Delivery
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int mt_set_rx_enable(const struct device *dev, bool enable)
{
	struct mock_tcpc_data *data = dev->data;

	data->rx_enable = enable;

	return 0;
}

/**
 * @brief Set the Power and Data role used when sending goodCRC messages
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int mt_set_roles(const struct device *dev, enum tc_power_role power_role,
			enum tc_data_role data_role)
{
	struct mock_tcpc_data *data = dev->data;

	data->msg_header.pr = power_role;
	data->msg_header.dr = data_role;

	return 0;
}

/**
 * @brief Transmit a power delivery message from DUT to Tester
 *
 * @retval 0 on success
 * @retval -EFAULT on failure
 */
static int mt_transmit_data(const struct device *dev, struct pd_msg *msg)
{
	struct mock_tcpc_data *data = dev->data;

	/* Simulate message transmit by copying it to the TCPC's tx_msg buffer */
	memcpy(&data->tx_msg, msg, sizeof(struct pd_msg));
	/* Simulate reception of GoodCRC from Tester by calling the TCPC's alert handler */
	data->alert_handler(dev, data->alert_data, TCPC_ALERT_TRANSMIT_MSG_SUCCESS);
	/* TCPC has a pending message to transmit */
	data->pending_tx_msg = true;

	return 0;
}

/**
 *
 * @brief Tests if a RX message is pending
 *
 * @retval true if a RX message is pending, else false
 */
static bool mt_is_rx_pending_msg(const struct device *dev, enum pd_packet_type *type)
{
	struct mock_tcpc_data *data = dev->data;

	/* Check if TCPC has a pending RX message */
	if (data->pending_rx_msg == false) {
		return false;
	}

	/* Return type of pending message if pointer isn't NULL */
	if (type != NULL) {
		*type = data->rx_msg.type;
	}

	return true;
}

/**
 * @brief Retrieves the Power Delivery message from the TCPC
 *	  The UUT calls this function in the Protocol Layer
 *	  to receive the message from the TCPC.
 *
 * @retval number of bytes received
 * @retval -EIO on no message to retrieve
 * @retval -EFAULT on buf being NULL
 */
static int mt_receive_data(const struct device *dev, struct pd_msg *msg)
{
	struct mock_tcpc_data *data = dev->data;

	if (msg == NULL) {
		return -EFAULT;
	}

	/* Make sure we have a message to retrieve */
	if (mt_is_rx_pending_msg(dev, NULL) == false) {
		return -EIO;
	}

	/* Clear pending RX msg */
	data->pending_rx_msg = false;

	/* Handle hard reset message */
	if (data->rx_msg.type == PD_PACKET_TX_HARD_RESET) {
		data->alert_handler(dev, data->alert_data, TCPC_ALERT_HARD_RESET_RECEIVED);
		return -EIO;
	}

	/* return the message to the caller */
	memcpy(msg, &data->rx_msg, sizeof(struct pd_msg));

	/* return the length of the message plush header size */
	return msg->len + MSG_HEADER_SIZE;
}

/**
 * @brief Sets the alert function that's called when an interrupt is triggered
 *        due to a TCPC alert
 *
 * @retval 0 on success
 * @retval -EINVAL on failure
 */
static int mt_set_alert_handler_cb(const struct device *dev, tcpc_alert_handler_cb_t handler,
				   void *alert_data)
{
	struct mock_tcpc_data *data = dev->data;

	data->alert_handler = handler;
	data->alert_data = alert_data;

	return 0;
}

/**
 * @brief Initializes the MOCK TCPC
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int mt_init(const struct device *dev)
{
	struct mock_tcpc_data *data = dev->data;

	/* Give tester access to the mock tcpc device */
	tester_set_tcpc_device(data);

	/* Clear msg pending flags */
	data->pending_rx_msg = false;
	data->pending_tx_msg = false;

	return 0;
}

static const struct tcpc_driver_api driver_api = {
	.init = mt_init,
	.set_alert_handler_cb = mt_set_alert_handler_cb,
	.get_cc = mt_get_cc,
	.set_rx_enable = mt_set_rx_enable,
	.is_rx_pending_msg = mt_is_rx_pending_msg,
	.receive_data = mt_receive_data,
	.transmit_data = mt_transmit_data,
	.select_rp_value = NULL,
	.get_rp_value = NULL,
	.set_cc = mt_set_cc,
	.set_roles = mt_set_roles,
	.set_vconn_cb = NULL,
	.set_vconn = NULL,
	.set_cc_polarity = mt_set_polarity,
	.dump_std_reg = NULL,
	.set_bist_test_mode = NULL,
	.sop_prime_enable = NULL,
};

#define MOCK_TCPC_DRIVER_INIT(inst)                                                                \
	static struct mock_tcpc_data drv_data_##inst;                                              \
	static const struct mock_tcpc_config drv_config_##inst;                                    \
	DEVICE_DT_INST_DEFINE(inst, &mt_init, NULL, &drv_data_##inst, &drv_config_##inst,          \
			      POST_KERNEL, CONFIG_USBC_INIT_PRIORITY, &driver_api);

DT_INST_FOREACH_STATUS_OKAY(MOCK_TCPC_DRIVER_INIT)
