/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private API for USB device controller (UDC) drivers
 */

#ifndef ZEPHYR_INCLUDE_UDC_COMMON_H
#define ZEPHYR_INCLUDE_UDC_COMMON_H

#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/byteorder.h>

#define CTRL_PIPE_STAGE_SETUP		0
#define CTRL_PIPE_STAGE_DATA_OUT	1
#define CTRL_PIPE_STAGE_DATA_IN		2
#define CTRL_PIPE_STAGE_NO_DATA		3
#define CTRL_PIPE_STAGE_STATUS_OUT	4
#define CTRL_PIPE_STAGE_STATUS_IN	5
#define CTRL_PIPE_STAGE_ERROR		6

/**
 * @brief Get driver's private data
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return pointer to driver's private data
 */
static inline void *udc_get_private(const struct device *dev)
{
	struct udc_data *data = dev->data;

	return data->priv;
}

/**
 * @brief Helper function to set suspended status
 *
 * This function can be used by the driver to set suspended status
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] values True to set suspended status
 */
void udc_set_suspended(const struct device *dev, const bool value);

/**
 * @brief Get pointer to endpoint configuration structure.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Endpoint address
 *
 * @return pointer to endpoint configuration or NULL on error.
 */
struct udc_ep_config *udc_get_ep_cfg(const struct device *dev,
				     const uint8_t ep);

/**
 * @brief Checks if the endpoint is busy
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Endpoint address
 *
 * @return true if endpoint is busy
 */
bool udc_ep_is_busy(const struct device *dev, const uint8_t ep);

/**
 * @brief Helper function to set endpoint busy state
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Endpoint address
 * @param[in] busy   Busy state
 */
void udc_ep_set_busy(const struct device *dev, const uint8_t ep,
		     const bool busy);

/**
 * @brief Get UDC request from endpoint FIFO.
 *
 * This function removes request from endpoint FIFO.
 * Use it when transfer is finished and request should
 * be passed to the higher level.
 *
 * @param[in] dev     Pointer to device struct of the driver instance
 * @param[in] ep      Endpoint address
 *
 * @return pointer to UDC request or NULL on error.
 */
struct net_buf *udc_buf_get(const struct device *dev,
			    const uint8_t ep);

/**
 * @brief Get all UDC request from endpoint FIFO.
 *
 * Get all UDC request from endpoint FIFO as single-linked list.
 * This function removes all request from endpoint FIFO and
 * is typically used to dequeue endpoint FIFO.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Endpoint address
 *
 * @return pointer to UDC request or NULL on error.
 */
struct net_buf *udc_buf_get_all(const struct device *dev,
				const uint8_t ep);

/**
 * @brief Peek request at the head of endpoint FIFO.
 *
 * Return request from the head of endpoint FIFO without removing.
 * Use it when request buffer is required for a transfer.
 *
 * @param[in] dev     Pointer to device struct of the driver instance
 * @param[in] ep      Endpoint address
 *
 * @return pointer to request or NULL on error.
 */
struct net_buf *udc_buf_peek(const struct device *dev,
			     const uint8_t ep);

/**
 * @brief Put request at the tail of endpoint FIFO.
 *
 * @param[in] ep_cfg Pointer to endpoint configuration
 * @param[in] buf    Pointer to UDC request buffer
 *
 * @return pointer to request or NULL on error.
 */
void udc_buf_put(struct udc_ep_config *const ep_cfg,
		 struct net_buf *const buf);
/**
 * @brief Helper function to send UDC event to a higher level.
 *
 * The callback would typically sends UDC even to a message queue (k_msgq).
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] type   Event type
 * @param[in] status Event status
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EPERM controller is not initialized
 */
int udc_submit_event(const struct device *dev,
		     const enum udc_event_type type,
		     const int status);

/**
 * @brief Helper function to send UDC endpoint event to a higher level.
 *
 * Type of this event is hardcoded to UDC_EVT_EP_REQUEST.
 * The callback would typically sends UDC even to a message queue (k_msgq).
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] buf    Pointer to UDC request buffer
 * @param[in] err    Request result
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EPERM controller is not initialized
 */
int udc_submit_ep_event(const struct device *dev,
			struct net_buf *const buf,
			const int err);
/**
 * @brief Helper function to enable endpoint.
 *
 * This function can be used by the driver to enable control IN/OUT endpoint.
 *
 * @param[in] dev        Pointer to device struct of the driver instance
 * @param[in] ep         Endpoint address (same as bEndpointAddress)
 * @param[in] attributes Endpoint attributes (same as bmAttributes)
 * @param[in] mps        Maximum packet size (same as wMaxPacketSize)
 * @param[in] interval   Polling interval (same as bInterval)
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -ENODEV endpoint is not assigned or no configuration found
 * @retval -EALREADY endpoint is already enabled
 */
int udc_ep_enable_internal(const struct device *dev,
			   const uint8_t ep,
			   const uint8_t attributes,
			   const uint16_t mps,
			   const uint8_t interval);

/**
 * @brief Helper function to disable endpoint.
 *
 * This function can be used by the driver to disable control IN/OUT endpoint.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Endpoint address
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -ENODEV endpoint is not assigned or no configuration found
 * @retval -EALREADY endpoint is already enabled
 */
int udc_ep_disable_internal(const struct device *dev,
			    const uint8_t ep);

/**
 * @brief Helper function to register endpoint configuration.
 *
 * This function initializes endpoint FIFO and
 * appends endpoint configuration to drivers endpoint list.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] cfg    Pointer to endpoint configuration structure
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EACCES controller is initialized or enabled
 */
int udc_register_ep(const struct device *dev,
		    struct udc_ep_config *const cfg);

/**
 * @brief Set setup flag in requests metadata.
 *
 * A control transfer can be either setup or data OUT,
 * use this function to mark request as setup packet.
 *
 * @param[in] buf    Pointer to UDC request buffer
 */
void udc_ep_buf_set_setup(struct net_buf *const buf);

/**
 * @brief Checks whether the driver must finish transfer with a ZLP
 *
 * @param[in] buf    Pointer to UDC request buffer
 *
 * @return true if ZLP is requested
 */
bool udc_ep_buf_has_zlp(const struct net_buf *const buf);

/**
 * @brief Clear ZLP flag
 *
 * @param[in] buf    Pointer to UDC request buffer
 */
void udc_ep_buf_clear_zlp(const struct net_buf *const buf);

/**
 * @brief Locking function for the drivers.
 *
 * @param[in] dev     Pointer to device struct of the driver instance
 * @param[in] timeout Timeout
 *
 * @return values provided by k_mutex_lock()
 */
static inline int udc_lock_internal(const struct device *dev,
				    k_timeout_t timeout)
{
	struct udc_data *data = dev->data;

	return k_mutex_lock(&data->mutex, timeout);
}

/**
 * @brief Unlocking function for the drivers.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return values provided by k_mutex_lock()
 */
static inline int udc_unlock_internal(const struct device *dev)
{
	struct udc_data *data = dev->data;

	return k_mutex_unlock(&data->mutex);
}

/**
 * @brief Allocate UDC control transfer buffer
 *
 * Allocate a new buffer from common control transfer buffer pool.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] ep     Endpoint address
 * @param[in] size   Size of the request buffer
 *
 * @return pointer to allocated request or NULL on error.
 */
struct net_buf *udc_ctrl_alloc(const struct device *dev,
			       const uint8_t ep,
			       const size_t size);

static inline uint16_t udc_data_stage_length(const struct net_buf *const buf)
{
	struct usb_setup_packet *setup = (void *)buf->data;

	return sys_le16_to_cpu(setup->wLength);
}

/**
 * @brief Checks whether the current control transfer stage is Data Stage OUT
 *
 * @param[in] dev   Pointer to device struct of the driver instance
 *
 * @return true if stage is Data Stage OUT
 */
bool udc_ctrl_stage_is_data_out(const struct device *dev);

/**
 * @brief Checks whether the current control transfer stage is Data Stage IN
 *
 * @param[in] dev   Pointer to device struct of the driver instance
 *
 * @return true if stage is Data Stage IN
 */
bool udc_ctrl_stage_is_data_in(const struct device *dev);

/**
 * @brief Checks whether the current control transfer stage is Status IN
 *
 * @param[in] dev   Pointer to device struct of the driver instance
 *
 * @return true if stage is Data Stage IN
 */
bool udc_ctrl_stage_is_status_in(const struct device *dev);

/**
 * @brief Checks whether the current control transfer stage is Status OUT
 *
 * @param[in] dev   Pointer to device struct of the driver instance
 *
 * @return true if stage is Data Stage IN
 */
bool udc_ctrl_stage_is_status_out(const struct device *dev);

/**
 * @brief Checks whether the current control transfer stage is Status no-data
 *
 * @param[in] dev   Pointer to device struct of the driver instance
 *
 * @return true if stage is Status no-data
 */
bool udc_ctrl_stage_is_no_data(const struct device *dev);

/**
 * @brief Submit Control Write (s-out-status) transfer
 *
 * Allocate buffer for data stage IN,
 * submit both setup and data buffer to upper layer.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] dout   Pointer to UDC buffer containing data transaction
 *
 * @return 0 on success, all other values should be treated as error.
 */
int udc_ctrl_submit_s_out_status(const struct device *dev,
				 struct net_buf *const dout);

/**
 * @brief Prepare control data IN stage
 *
 * Allocate buffer for data stage IN,
 * submit both setup and data buffer to upper layer.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return 0 on success, all other values should be treated as error.
 */
int udc_ctrl_submit_s_in_status(const struct device *dev);

/**
 * @brief Prepare control (no-data) status stage
 *
 * Allocate buffer for status stage IN,
 * submit both setup and status buffer to upper layer.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return 0 on success, all other values should be treated as error.
 */
int udc_ctrl_submit_s_status(const struct device *dev);

/**
 * @brief Submit status transaction
 *
 * Submit both status transaction to upper layer.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] dout   Pointer to UDC buffer containing data transaction
 *
 * @return 0 on success, all other values should be treated as error.
 */
int udc_ctrl_submit_status(const struct device *dev,
			   struct net_buf *const buf);

/**
 * @brief Update internal control stage status based on the net_buf metadata
 *
 * Use it in the driver to update the stage, typically there are
 * three places where this function should be called:
 * - when a setup packet is received
 * - when a data stage is completed (all data stage transactions)
 * - when a status stage transaction is finished
 *
 * The functions of type udc_ctrl_stage_is_*() can be called before or
 * after this function, depending on the desired action.
 * To keep protocol processing running the following should be taken
 * into account:
 *
 * - Upper layer may not allocate buffers but remove or release buffers
 *   from the chain that are no longer needed. Only control IN transfers may
 *   be enqueued by the upper layer.
 *
 * - For "Control Write" (s-out-status), the driver should allocate the buffer,
 *   insert it as a fragment to setup buffer and perform the Data Stage
 *   transaction. Allocate and insert a fragment for the status (IN) stage to
 *   setup buffer, and then pass setup packet with the chain of s-out-status to
 *   upper layer. Upper layer should either halt control endpoint or
 *   enqueue status buffer for status stage. There should be second
 *   notification to upper layer when the status transaction is finished.
 *
 *   ->driver_foo_setup_rcvd(dev)
 *     ->udc_ctrl_update_stage(dev, buf)
 *     ->udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, wLength)
 *     ->driver_foo_xfer_start(dev, USB_CONTROL_EP_OUT)
 *
 *   ->driver_foo_dout_rcvd(dev)
 *     -...
 *     ->driver_foo_feed_next_dout(dev, ....)
 *     -...
 *     ->udc_ctrl_update_stage(dev, dout_buf)
 *     -...
 *     ->udc_ctrl_submit_s_out_status(dev, dout_buf);
 *
 *   ->driver_foo_din_rcvd(dev)
 *     -...
 *     ->udc_ctrl_submit_status(dev, status_buf);
 *     -...
 *     ->udc_ctrl_update_stage(dev, status_buf)
 *
 * - For "Control Read" (s-in-status), depending on the controller,
 *   the driver should reserve the buffers for subsequent status stage and
 *   setup packet and prepare everything. The driver should allocate the buffer
 *   for IN transaction insert it as a fragment to setup buffer, and pass
 *   the chain of s-in to upper layer. Upper layer should either halt control
 *   endpoint or enqueue (in) buffer. There should be second
 *   notification to upper layer when the status transaction is finished.
 *
 *   ->driver_foo_setup_rcvd(dev)
 *     ->udc_ctrl_update_stage(dev, buf)
 *     ->driver_foo_feed_next_dout(dev, ....)
 *     -...
 *     ->udc_ctrl_submit_s_in_status(dev);
 *
 *   ->driver_foo_din_rcvd(dev)
 *     -...
 *     ->udc_ctrl_update_stage(dev, dout_buf)
 *     -...
 *
 *   ->driver_foo_dout_rcvd(dev)
 *     -...
 *     ->udc_ctrl_submit_status(dev, status_buf);
 *     -...
 *     ->udc_ctrl_update_stage(dev, dout_buf)
 *
 * - For "No-data Control" (s-status), the driver should allocate the buffer
 *   for the status (IN) stage, insert it as a fragment to setup buffer,
 *   and then pass setup packet with the chain of s-status to
 *   upper layer. Upper layer should either halt control endpoint or
 *   enqueue status buffer for status stage. There should be second
 *   notification to upper layer when the status transaction is finished.
 *
 *   ->driver_foo_setup_rcvd(dev)
 *     ->udc_ctrl_update_stage(dev, buf)
 *     ->driver_foo_feed_next_dout(dev, ....)
 *     -...
 *     ->udc_ctrl_submit_s_status(dev);
 *
 *   ->driver_foo_din_rcvd(dev)
 *     -...
 *     ->udc_ctrl_submit_status(dev, status_buf);
 *     -...
 *     ->udc_ctrl_update_stage(dev, status_buf)
 *
 * Please refer to Chapter 8.5.3 Control Transfers USB 2.0 spec.
 *
 * @param[in] dev   Pointer to device struct of the driver instance
 * @param[in] buf   Buffer containing setup packet
 *
 * @return 0 on success, all other values should be treated as error.
 */
void udc_ctrl_update_stage(const struct device *dev,
			   struct net_buf *const buf);

#if defined(CONFIG_UDC_WORKQUEUE)
extern struct k_work_q udc_work_q;

static inline struct k_work_q *udc_get_work_q(void)
{
	return &udc_work_q;
}
#else
static inline struct k_work_q *udc_get_work_q(void)
{
	return &k_sys_work_q;
}
#endif

#endif /* ZEPHYR_INCLUDE_UDC_COMMON_H */
