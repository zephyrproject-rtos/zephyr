/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/logging/log.h>

#include "sample_driver.h"
#include "app_shared.h"
#include "app_a.h"
#include "app_syscall.h"

LOG_MODULE_REGISTER(app_a);

#define MAX_MSGS	8

/* Resource pool for allocations made by the kernel on behalf of system
 * calls. Needed for k_queue_alloc_append()
 */
K_HEAP_DEFINE(app_a_resource_pool, 256 * 5 + 128);

/* Define app_a_partition, where all globals for this app will be routed.
 * The partition starting address and size are populated by build system
 * and linker magic.
 */
K_APPMEM_PARTITION_DEFINE(app_a_partition);

/* Memory domain for application A, set up and installed in app_a_entry() */
static struct k_mem_domain app_a_domain;

/* Message queue for IPC between the driver callback and the monitor thread.
 *
 * This message queue is being statically initialized, no need to call
 * k_msgq_init() on it.
 */
K_MSGQ_DEFINE(mqueue, SAMPLE_DRIVER_MSG_SIZE, MAX_MSGS, 4);

/* Processing thread. This takes data that has been processed by application
 * B and writes it to the sample_driver, completing the control loop
 */
struct k_thread writeback_thread;
K_THREAD_STACK_DEFINE(writeback_stack, 2048);

/* Global data used by application A. By tagging with APP_A_BSS or APP_A_DATA,
 * we ensure all this gets linked into the continuous region denoted by
 * app_a_partition.
 */
APP_A_BSS const struct device *sample_device;
APP_A_BSS unsigned int pending_count;

/* ISR-level callback function. Runs in supervisor mode. Does what's needed
 * to get the data into this application's accessible memory and have the
 * worker thread running in user mode do the rest.
 */
void sample_callback(const struct device *dev, void *context, void *data)
{
	int ret;

	ARG_UNUSED(context);

	LOG_DBG("sample callback with %p", data);

	/* All the callback does is place the data payload into the
	 * message queue. This will wake up the monitor thread for further
	 * processing.
	 *
	 * We use a message queue because it will perform a data copy for us
	 * when buffering this data.
	 */
	ret = k_msgq_put(&mqueue, data, K_NO_WAIT);
	if (ret) {
		LOG_ERR("k_msgq_put failed with %d", ret);
	}
}

static void monitor_entry(void *p1, void *p2, void *p3)
{
	int ret;
	void *payload;
	unsigned int monitor_count = 0;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Monitor thread, running in user mode. Responsible for pulling
	 * data out of the message queue for further writeback.
	 */
	LOG_DBG("monitor thread entered");

	ret = sample_driver_state_set(sample_device, true);
	if (ret != 0) {
		LOG_ERR("couldn't start driver interrupts");
		k_oops();
	}

	while (monitor_count < NUM_LOOPS) {
		payload = sys_heap_alloc(&shared_pool,
					 SAMPLE_DRIVER_MSG_SIZE);
		if (payload == NULL) {
			LOG_ERR("couldn't alloc memory from shared pool");
			k_oops();
			continue;
		}

		/* Sleep waiting for some data to appear in the queue,
		 * and then copy it into the payload buffer.
		 */
		LOG_DBG("monitor thread waiting for data...");
		ret = k_msgq_get(&mqueue, payload, K_FOREVER);
		if (ret != 0) {
			LOG_ERR("k_msgq_get() failed with %d", ret);
			k_oops();
		}


		LOG_INF("monitor thread got data payload #%u", monitor_count);
		LOG_DBG("pending payloads: %u", pending_count);

		/* Put the payload in the queue for data to process by
		 * app B. This does not copy the data. Because we are using
		 * k_queue from user mode, we need to use the
		 * k_queue_alloc_append() variant, which needs to allocate
		 * some memory on the kernel side from our thread
		 * resource pool.
		 */
		pending_count++;
		k_queue_alloc_append(&shared_queue_incoming, payload);
		monitor_count++;
	}

	/* Tell the driver to stop delivering interrupts, we're closing up
	 * shop
	 */
	ret = sample_driver_state_set(sample_device, false);
	if (ret != 0) {
		LOG_ERR("couldn't disable driver");
		k_oops();
	}
	LOG_DBG("monitor thread exiting");
}

static void writeback_entry(void *p1, void *p2, void *p3)
{
	void *data;
	unsigned int writeback_count = 0;
	int ret;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_DBG("writeback thread entered");

	while (writeback_count < NUM_LOOPS) {
		/* Grab a data payload processed by Application B,
		 * send it to the driver, and free the buffer.
		 */
		data = k_queue_get(&shared_queue_outgoing, K_FOREVER);
		if (data == NULL) {
			LOG_ERR("no data?");
			k_oops();
		}

		LOG_INF("writing processed data back to the sample device");
		sample_driver_write(sample_device, data);
		sys_heap_free(&shared_pool, data);
		pending_count--;
		writeback_count++;
	}

	/* Fairly meaningless example to show an application-defined system
	 * call being defined and used.
	 */
	ret = magic_syscall(&writeback_count);
	if (ret != 0) {
		LOG_ERR("no more magic!");
		k_oops();
	}

	LOG_DBG("writeback thread exiting");
	LOG_INF("SUCCESS");
}

/* Supervisor mode setup function for application A */
void app_a_entry(void *p1, void *p2, void *p3)
{
	int ret;
	struct k_mem_partition *parts[] = {
#if Z_LIBC_PARTITION_EXISTS
		&z_libc_partition,
#endif
		&app_a_partition, &shared_partition
	};

	sample_device = device_get_binding(SAMPLE_DRIVER_NAME_0);
	if (sample_device == NULL) {
		LOG_ERR("bad sample device");
		k_oops();
	}

	/* Initialize a memory domain with the specified partitions
	 * and add ourself to this domain. We need access to our own
	 * partition, the shared partition, and any common libc partition
	 * if it exists.
	 */
	ret = k_mem_domain_init(&app_a_domain, ARRAY_SIZE(parts), parts);
	__ASSERT(ret == 0, "k_mem_domain_init failed %d", ret);
	ARG_UNUSED(ret);

	k_mem_domain_add_thread(&app_a_domain, k_current_get());

	/* Assign a resource pool to serve for kernel-side allocations on
	 * behalf of application A. Needed for k_queue_alloc_append().
	 */
	k_thread_heap_assign(k_current_get(), &app_a_resource_pool);

	/* Set the callback function for the sample driver. This has to be
	 * done from supervisor mode, as this code will run in supervisor
	 * mode in IRQ context.
	 */
	sample_driver_set_callback(sample_device, sample_callback, NULL);

	/* Set up the writeback thread, which takes processed data from
	 * application B and sends it to the sample device.
	 *
	 * This child thread automatically inherits the memory domain of
	 * this thread that created it; it will be a member of app_a_domain.
	 *
	 * Initialize this thread with K_FOREVER timeout so we can
	 * modify its permissions and then start it.
	 */
	k_thread_create(&writeback_thread, writeback_stack,
			K_THREAD_STACK_SIZEOF(writeback_stack),
			writeback_entry, NULL, NULL, NULL,
			-1, K_USER, K_FOREVER);
	k_thread_access_grant(&writeback_thread, &shared_queue_outgoing,
			      sample_device);
	k_thread_start(&writeback_thread);

	/* We are about to drop to user mode and become the monitor thread.
	 * Grant ourselves access to the kernel objects we need for
	 * the monitor thread to function.
	 *
	 * Monitor thread needs access to the message queue shared with the
	 * ISR, and the queue to send data to the processing thread in
	 * App B.
	 */
	k_thread_access_grant(k_current_get(), &mqueue, sample_device,
			      &shared_queue_incoming);

	/* We now do a one-way transition to user mode, and will end up
	 * in monitor_thread(). We could create another thread which just
	 * starts in user mode, but this lets us re-use the current one.
	 */
	k_thread_user_mode_enter(monitor_entry, NULL, NULL, NULL);
}
