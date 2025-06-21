/*
 * Copyright (c) 2025 EXACT Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_PUBLISHER_H_
#define ZEPHYR_INCLUDE_ZBUS_PUBLISHER_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

void zbus_publisher_work_handler(struct k_work *work);
extern struct k_work_q *const zbus_workq;

/** @endcond */

struct zbus_publisher {
	struct k_work_delayable work;
	volatile k_timeout_t period;
	void *data;
	struct zbus_channel *chan;
};

/**
 * @brief Zbus publisher definition.
 *
 * @param      _name    The publisher's name.
 * @param      _p_chan  The channel the publisher publishes to.
 * @param      _p_msg   A pointer to the publisher's message.
 */
#define ZBUS_CHANNEL_PUBLISHER_DEFINE(_name, _p_chan, _p_msg)                                      \
	static struct zbus_publisher _name = {                                                     \
		.work = Z_WORK_DELAYABLE_INITIALIZER(zbus_publisher_work_handler),                 \
		.data = (void *)_p_msg,                                                            \
		.chan = (struct zbus_channel *)_p_chan,                                            \
		.period = K_NO_WAIT,                                                               \
	}

/**
 * @brief      Schedule a publisher to publish its message after a delay.
 *
 * @param      publisher  The publisher
 * @param[in]  delay      The delay
 *
 * @warning Care should be taken when using publisher's on channels with subscribers. If multiple
 * messages are delivered on the same tick one will be missed unless the subscriber thread has a
 * higher priority.
 *
 * @return as with k_work_schedule_for_queue().
 */
static inline int zbus_chan_publisher_schedule(struct zbus_publisher *publisher, k_timeout_t delay)
{
	return k_work_schedule_for_queue(zbus_workq, &publisher->work, delay);
}

/**
 * @brief      Reschedule a publisher to publish its message after a delay.
 *
 * @param      publisher  The publisher
 * @param[in]  delay      The delay
 *
 * @warning Care should be taken when using publisher's on channels with subscribers. If multiple
 * messages are delivered on the same tick one will be missed unless the subscriber thread has a
 * higher priority.
 *
 * @return as with k_work_schedule_for_queue().
 */
static inline int zbus_chan_publisher_reschedule(struct zbus_publisher *publisher,
						 k_timeout_t delay)
{
	return k_work_reschedule_for_queue(zbus_workq, &publisher->work, delay);
}

/**
 * @brief      Cancel a publisher's queued message.
 *
 * @param      publisher  The publisher
 *
 * @return as with k_work_cancel_delayable().
 */
static inline int zbus_chan_publisher_cancel(struct zbus_publisher *publisher)
{
	return k_work_cancel_delayable(&publisher->work);
}

/**
 * @brief      Synchronously cancel a publisher's queued message.
 *
 * @param      publisher  The publisher
 *
 * @return as with k_work_cancel_delayable_sync().
 */
static inline int zbus_chan_publisher_cancel_sync(struct zbus_publisher *publisher)
{
	_ZBUS_ASSERT(!k_is_in_isr(), "cannot cancel work synchronously from ISR");
	_ZBUS_ASSERT(k_current_get() != k_work_queue_thread_get(zbus_workq),
		     "cannot cancel publisher synchronously from work queue running the publisher");

	struct k_work_sync sync;

	return k_work_cancel_delayable_sync(&publisher->work, &sync);
}

/**
 * @brief      Reschedules a publisher and configures it to reschedule itself at the provided period
 * provided.
 *
 * @param      publisher  The publisher
 * @param[in]  delay      The delay
 * @param[in]  period     The period
 *
 * @return as with k_work_reschedule_for_queue().
 */
static inline int zbus_chan_publisher_start(struct zbus_publisher *publisher, k_timeout_t delay,
					    k_timeout_t period)
{
	publisher->period = period;
	return zbus_chan_publisher_reschedule(publisher, delay);
}

/**
 * @brief      Cancel a publisher's queued message and disables it from rescheduling itself.
 *
 * @param      publisher  The publisher
 *
 * @return as with k_work_cancel_delayable().
 */
static inline int zbus_chan_publisher_stop(struct zbus_publisher *publisher)
{
	publisher->period = K_NO_WAIT;
	return zbus_chan_publisher_cancel(publisher);
}

/**
 * @brief      Synchronously cancel a publisher's queued message and disables it from rescheduling
 * itself.
 *
 * @param      publisher  The publisher
 *
 * @return as with k_work_cancel_delayable_sync().
 */
static inline int zbus_chan_publisher_stop_sync(struct zbus_publisher *publisher)
{
	publisher->period = K_NO_WAIT;
	return zbus_chan_publisher_cancel_sync(publisher);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_PUBLISHER_H_ */
