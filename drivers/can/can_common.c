/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/can.h>
#include <kernel.h>

#define LOG_LEVEL CONFIG_CAN_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(can_driver);

#define WORK_BUF_COUNT_IS_POWER_OF_2 !(CONFIG_CAN_WORKQ_FRAMES_BUF_CNT & \
					(CONFIG_CAN_WORKQ_FRAMES_BUF_CNT - 1))

#define WORK_BUF_MOD_MASK (CONFIG_CAN_WORKQ_FRAMES_BUF_CNT - 1)

#if WORK_BUF_COUNT_IS_POWER_OF_2
#define WORK_BUF_MOD_SIZE(x) ((x) & WORK_BUF_MOD_MASK)
#else
#define WORK_BUF_MOD_SIZE(x) ((x) % CONFIG_CAN_WORKQ_FRAMES_BUF_CNT)
#endif

#define WORK_BUF_FULL 0xFFFF

static void can_msgq_put(struct zcan_frame *frame, void *arg)
{
	struct k_msgq *msgq = (struct k_msgq *)arg;
	int ret;

	__ASSERT_NO_MSG(msgq);

	ret = k_msgq_put(msgq, frame, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Msgq %p overflowed. Frame ID: 0x%x", arg,
			frame->id_type == CAN_STANDARD_IDENTIFIER ?
				frame->std_id : frame->ext_id);
	}
}

int z_impl_can_attach_msgq(struct device *dev, struct k_msgq *msg_q,
			   const struct zcan_filter *filter)
{
	const struct can_driver_api *api = dev->driver_api;

	return api->attach_isr(dev, can_msgq_put, msg_q, filter);
}

static inline void can_work_buffer_init(struct can_frame_buffer *buffer)
{
	buffer->head = 0;
	buffer->tail = 0;
}

static inline int can_work_buffer_put(struct zcan_frame *frame,
				      struct can_frame_buffer *buffer)
{
	u16_t next_head = WORK_BUF_MOD_SIZE(buffer->head + 1);

	if (buffer->head == WORK_BUF_FULL) {
		return -1;
	}

	buffer->buf[buffer->head] = *frame;

	/* Buffer is almost full */
	if (next_head == buffer->tail) {
		buffer->head = WORK_BUF_FULL;
	} else {
		buffer->head = next_head;
	}

	return 0;
}

static inline
struct zcan_frame *can_work_buffer_get_next(struct can_frame_buffer *buffer)
{
	/* Buffer empty */
	if (buffer->head == buffer->tail) {
		return NULL;
	} else {
		return &buffer->buf[buffer->tail];
	}
}

static inline void can_work_buffer_free_next(struct can_frame_buffer *buffer)
{
	u16_t next_tail = WORK_BUF_MOD_SIZE(buffer->tail + 1);

	if (buffer->head == buffer->tail) {
		return;
	}

	if (buffer->head == WORK_BUF_FULL) {
		buffer->head = buffer->tail;
	}

	buffer->tail = next_tail;
}

static void can_work_handler(struct k_work *work)
{
	struct zcan_work *can_work = CONTAINER_OF(work, struct zcan_work,
						  work_item);
	struct zcan_frame *frame;

	while ((frame = can_work_buffer_get_next(&can_work->buf))) {
		can_work->cb(frame, can_work->cb_arg);
		can_work_buffer_free_next(&can_work->buf);
	}
}

static void can_work_isr_put(struct zcan_frame *frame, void *arg)
{
	struct zcan_work *work = (struct zcan_work *)arg;
	int ret;

	ret = can_work_buffer_put(frame, &work->buf);
	if (ret) {
		LOG_ERR("Workq buffer overflow. Msg ID: 0x%x",
			frame->id_type == CAN_STANDARD_IDENTIFIER ?
				frame->std_id : frame->ext_id);
		return;
	}

	k_work_submit_to_queue(work->work_queue, &work->work_item);
}

int can_attach_workq(struct device *dev, struct k_work_q *work_q,
			    struct zcan_work *work,
			    can_rx_callback_t callback, void *callback_arg,
			    const struct zcan_filter *filter)
{
	const struct can_driver_api *api = dev->driver_api;

	k_work_init(&work->work_item, can_work_handler);
	work->work_queue = work_q;
	work->cb = callback;
	work->cb_arg = callback_arg;
	can_work_buffer_init(&work->buf);

	return api->attach_isr(dev, can_work_isr_put, work, filter);
}
