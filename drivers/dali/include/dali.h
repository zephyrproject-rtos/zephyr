#ifndef DRIVERS_DALI_INCLUDE_DALI_H_
#define DRIVERS_DALI_INCLUDE_DALI_H_

#include <zephyr/device.h>
#include <zephyr/sys_clock.h>
#include <errno.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DALI frame and event types
 */
enum dali_event_type {
	DALI_EVENT_NONE,           /**< no event (write/receive) */
	DALI_FRAME_CORRUPT,        /**< corrupt frame (write/receive)*/
	DALI_FRAME_BACKWARD,       /**< backward frame (write/receive) */
	DALI_FRAME_GEAR,           /**< forward 16bit gear frame (write/receive)*/
	DALI_FRAME_GEAR_TWICE,     /**< forward 16bit gear frame, received twice (receive) */
	DALI_FRAME_DEVICE,         /**< forward 24bit device frame (write/receive) */
	DALI_FRAME_DEVICE_TWICE,   /**< forward 24bit device, received twice (receive)*/
	DALI_FRAME_FIRMWARE,       /**< forward 32bit firmware frame (write/receive) */
	DALI_FRAME_FIRMWARE_TWICE, /**< forward 32bit firmware frame (receive) */
	DALI_EVENT_NO_ANSWER,      /**< received no reply (receive) */
	DALI_EVENT_BUS_FAILURE,    /**< detected a bus failure (receive) */
	DALI_EVENT_BUS_IDLE,       /**< detected that bus is idle again after failure (receive) */
};

/**
 * @brief Frame transmission inter-frame priorities.
 * see IEC 62386-101:2022 Table 22 -- Multi-master transmitter settling time values *
 */
enum dali_tx_priority {
	DALI_PRIORITY_BACKWARD_FRAME = 0,
	DALI_PRIORITY_1 = 1,
	DALI_PRIORITY_2 = 2,
	DALI_PRIORITY_3 = 3,
	DALI_PRIORITY_4 = 4,
	DALI_PRIORITY_5 = 5,
};

/** DALI frame */
struct dali_frame {
	uint32_t data;                   /**< LSB aligned payload */
	enum dali_event_type event_type; /**<  event type of frame */
};

/** DALI send frame */
struct dali_tx_frame {
	struct dali_frame frame;        /**< payload frame */
	enum dali_tx_priority priority; /**<  inter frame timing */
	bool is_query; /**<  frame is a query, initiate reception of backward frame */
};

/**
 * @brief DALI driver API
 *
 * This is the mandatory API any driver needs to expose.
 */
struct dali_driver_api {
	int (*recv)(const struct device *dev, struct dali_frame *frame, k_timeout_t timeout);
	int (*send)(const struct device *dev, const struct dali_tx_frame *frame);
	void (*abort)(const struct device *dev);
};

/**
 * @brief Receive a frame or event from DALI bus.
 *
 * All valid frames received on DALI bus are delivered by this function.
 *
 * The caller is responsible to process incoming frames in a timely manner.
 * The queue size is small and if not accessed fast enough, frames are silently
 * dropped.
 * Bus events are also reported via this function as dali_frames.
 * The data on event frames should be ignored.
 *
 * @param[in] dev     DALI device
 * @param[out] frame  received DALI frame, or event
 * @param[in] timeout kernel timeout or one of K_NO_WAIT or K_FOREVER
 *
 * @retval 0          success, report was stored
 * @retval -ENOMSG    returned without waiting or waiting period timed out
 * @retval -EINVAL    invalid input parameters
 */
static inline int dali_receive(const struct device *dev, struct dali_frame *rx_frame,
			       k_timeout_t timeout)
{
	const struct dali_driver_api *api = dev->api;
	return api->recv(dev, rx_frame, timeout);
}

/**
 * @brief Send a frame on DALI bus.
 *
 * This function supports async operation. Any frame is stored into an internal send slot and the
 * dali_send returns immediately. dali_send maintains two send slots. One slot is reserved for
 * backward frames. The other slot is used for all kind of forward frames. In case of a forward
 * frame in its slot that is pending for transmission, it is still possible to provide a backward
 * frame. That backward frame will be transmitted before the pending forward frame whenever
 * possible. There is a strict timing limit from the DALI standard (see IEC 62386-101:2022 8.1.2
 * Table 17) for the timing of backward frames. When these restrictions can not be fulfilled, the
 * backward frame may be dropped and an error code returned.
 *
 * @param[in] dev         DALI device
 * @param[in] frame       DALI frame to transmit
 *
 * @retval 0          success
 * @retval -EINVAL    invalid input parameters
 * @retval -ETIMEDOUT backward frame cannot be send any more within DALI standard
 * @retval -EBUSY     send queue is full. No more memory for more messages, try later
 */
static inline int dali_send(const struct device *dev, const struct dali_tx_frame *tx_frame)
{
	const struct dali_driver_api *api = dev->api;
	return api->send(dev, tx_frame);
}

/**
 * @brief Abort sending forward frames
 *
 * This will abort all pending or ongoing forward frame transmissions. Transmission
 * will be aborted, regardless of bit timings, at the shortest possible time.
 * This can result in corrupt a frame.
 *
 * @param[in] dev         DALI device
 */
static inline void dali_abort(const struct device *dev)
{
	const struct dali_driver_api *api = dev->api;
	api->abort(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_DALI_INCLUDE_DALI_H_ */
