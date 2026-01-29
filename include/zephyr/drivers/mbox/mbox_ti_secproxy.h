/*
 * Copyright (c) 2025, Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the TI Secproxy Mailbox
 *
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_MBOX_TI_SECPROXY_H_
#define INCLUDE_ZEPHYR_DRIVERS_MBOX_TI_SECPROXY_H_

#include <zephyr/drivers/mbox.h>
#include <zephyr/device.h>

/**
 * @brief Send a message over secure proxy then poll for the reply
 *
 * Combine sending and receiving messages over specified channels by polling. This routine disables
 * the interrupt associated with the RX channel during its duration and is only meant to be used
 * during pre kernel stages where kernel services are not available. The RX buffer for the channel
 * should be set up using mbox_register_callback() before calling this API. Since this is a polling
 * API, it also blocks the thread.
 *
 * @param dev: Secure Proxy mailbox device
 * @param rx_channel: Channel ID to receive reply on via polling after sending the message
 * @param tx_channel: Channel ID to send message on
 * @param tx_msg: TX Message struct
 *
 * @retval 0         On success.
 * @retval -EBUSY    If the remote hasn't yet read the last data sent.
 * @retval -EMSGSIZE If the supplied data size is unsupported by the driver.
 * @retval -EIO      If there is an issue with RX thread like being in an error state
 * @retval -ENOBUFS  If the RX buffer is too small for receiving a message
 * @retval -EINVAL   If there was a bad parameter, such as: too-large channel
 *		     descriptor or the device isn't an outbound MBOX channel.
 * @see mbox_register_callback()
 *
 */
int secproxy_mailbox_send_then_poll_rx(const struct device *dev, uint32_t rx_channel,
				       uint32_t tx_channel, const struct mbox_msg *tx_msg);

/**
 * @brief Send a message over secure proxy then poll for the reply, using DT specs
 *
 * @param rx_spec: MBOX DT spec to receive reply on via polling after sending the message
 * @param tx_spec: MBOX DT spec to send message on
 * @param tx_msg: TX Message struct
 *
 * @return See return values for secproxy_mailbox_send_then_poll_rx()
 */
int secproxy_mailbox_send_then_poll_rx_dt(const struct mbox_dt_spec *rx_spec,
					  const struct mbox_dt_spec *tx_spec,
					  const struct mbox_msg *tx_msg)
{
	if (rx_spec->dev != tx_spec->dev) {
		return -EINVAL;
	}

	return secproxy_mailbox_send_then_poll_rx(tx_spec->dev, rx_spec->channel_id,
						  tx_spec->channel_id, tx_msg);
}

#endif /* INCLUDE_ZEPHYR_DRIVERS_MBOX_TI_SECPROXY_H_ */
