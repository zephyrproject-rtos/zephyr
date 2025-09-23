/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

/* Our producer is a timer (interrupt) but could be a thread as well */
struct producer {
	struct mpsc io_q;
};

/* Our producer function, could be done in a thread or interrupt, here
 * we are producing cycle count values from a timer interrupt.
 */
static void producer_periodic(struct k_timer *timer)
{
	struct producer *p = timer->user_data;

	LOG_INF("producer %p trigger", (void *)p);

	struct mpsc_node *n = mpsc_pop(&p->io_q);

	if (n == NULL) {
		LOG_WRN("producer overflowed consumers");
		return;
	}

	struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(n, struct rtio_iodev_sqe, q);

	/* Only accept read/rx requests */
	if (iodev_sqe->sqe.op != RTIO_OP_RX) {
		rtio_iodev_sqe_err(iodev_sqe, -EINVAL);
		return;
	}

	/* Get the rx buffer with a minimum/maximum size pair to fill with the current time */
	uint8_t *buf = NULL;
	uint32_t buf_len = 0;
	int rc = rtio_sqe_rx_buf(iodev_sqe, sizeof(uint64_t), sizeof(uint64_t), &buf, &buf_len);

	LOG_INF("buf %p, len %u", (void *)buf, buf_len);

	if (rc < 0) {
		/* buffer wasn't available or too small */
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	/* We now have a buffer we can produce data into and then signal back to the consumer */
	uint64_t *cycle_count = (uint64_t *)buf;

	/* "Produce" a timestamp */
	*cycle_count = k_cycle_get_64();

	/* Signal read has completed  */
	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

/* Accept incoming commands (e.g. read requests), could come from multiple sources
 * so the only real safe thing to do here is put it into the lock free queue
 */
static void producer_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	struct mpsc *producer_ioq = iodev_sqe->sqe.iodev->data;

	mpsc_push(producer_ioq, &iodev_sqe->q);
}

K_TIMER_DEFINE(producer_tmr, producer_periodic, NULL);
static struct producer producer_data;
const struct rtio_iodev_api producer_api = {.submit = producer_submit};

/* Setup our i/o device, akin to a file handle we can read from */
RTIO_IODEV_DEFINE(producer_iodev, &producer_api, &producer_data);

/* Setup our pair of queues for our consumer, with 1 submission and 1 completion available */
RTIO_DEFINE(rconsumer, 1, 1);

int consumer_loop(struct rtio *consumer, struct rtio_iodev *producer)
{
	/* We can share memory with kernel space without any work, to share
	 * between usermode threads we'd need a k_mem_partition added to
	 * both domains instead
	 */
	uint64_t cycle_count;

	/* Our read submission and completion pair */
	struct rtio_sqe read_sqe;
	struct rtio_cqe read_cqe;
	struct rtio_sqe *read_sqe_handle;

	/* Helper that sets up the submission to be a read request, reading *directly*
	 * into the given buffer pointer without copying
	 */
	rtio_sqe_prep_read(&read_sqe, producer, RTIO_PRIO_NORM, (uint8_t *)&cycle_count,
			   sizeof(cycle_count), NULL);

	/* We can automatically have this read request resubmitted for us */
	read_sqe.flags |= RTIO_SQE_MULTISHOT;

	/* A syscall to copy the control structure (sqe) into kernel mode, and get a handle out
	 * so we can cancel it later if we want
	 */
	rtio_sqe_copy_in_get_handles(consumer, &read_sqe, &read_sqe_handle, 1);

	/* A syscall to submit the queued up requests (there could be many) to all iodevs */
	rtio_submit(consumer, 0);

	/* A consumer loop that waits for read completions in a single syscall
	 *
	 * This never ends but to end the loop we'd cancel the requests to read.
	 *
	 * NOTE: There could be multiple read requests out to multiple producers we could
	 * be waiting on!
	 */
	while (true) {
		/* A syscall to consume a completion, waiting forever for it to arrive */
		rtio_cqe_copy_out(consumer, &read_cqe, 1, K_FOREVER);

		/* The read has been completed, and its safe to read the value until
		 * we attach and submit a request to read into it again
		 */
		LOG_INF("read result %d, cycle count is %llu", read_cqe.result, cycle_count);
	}

	return 0;
}

int main(void)
{
	/* init stuff */
	mpsc_init(&producer_data.io_q);
	producer_tmr.user_data = &producer_data;

	/* Start our producer task (timer interrupt based) */
	k_timer_start(&producer_tmr, K_MSEC(100), K_MSEC(100));

	/* We can enter usermode here with a little work to setup k_objects for the iodev
	 * and struct rtio context
	 * E.g.
	 * k_object_access_grant(&producer, k_current_get());
	 * k_object_access_grant(&consumer, k_current_get());
	 * k_thread_user_mode_enter(consumer_loop, &producer, &consumer, NULL);
	 */

	return consumer_loop(&rconsumer, &producer_iodev);
}
