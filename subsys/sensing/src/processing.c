//
// Created by peress on 05/07/23.
//

#include <zephyr/kernel.h>
#include <zephyr/sensing/sensor.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>

#include "sensing/internal/sensing.h"

LOG_MODULE_REGISTER(sensing_processing, CONFIG_SENSING_LOG_LEVEL);

static void process_info_node(struct sensing_sensor_info *info, const void *data)
{
	LOG_DBG("Broadcasting data for [%d], data=%p",
		(int)(info - STRUCT_SECTION_START(sensing_sensor_info)), data);
	sys_mutex_lock(__sensing_connection_pool.lock, K_FOREVER);
	for (int i = 0; i < __CONNECTION_POOL_COUNT; ++i) {
		struct sensing_connection *connection =
			&STRUCT_SECTION_START(sensing_connection)[i];
		if (!__sensing_is_connected(info, connection)) {
			continue;
		}
		sensing_data_event_t cb = connection->cb_list->on_data_event;

		if (cb == NULL) {
			continue;
		}
		cb(connection, data, connection->userdata);
	}
	sys_mutex_unlock(__sensing_connection_pool.lock);
}

static void processing_task(void *a, void *b, void *c)
{
	uint8_t *data = NULL;
	uint32_t data_len = 0;
	int rc;
	int get_data_rc;

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	if (IS_ENABLED(CONFIG_USERSPACE) && !k_is_user_context()) {
		rtio_access_grant(&sensing_rtio_ctx, k_current_get());
		k_thread_user_mode_enter(processing_task, a, b, c);
	}

	while (true) {
		struct rtio_cqe cqe;

		rc = rtio_cqe_copy_out(&sensing_rtio_ctx, &cqe, 1, K_FOREVER);

		if (rc < 1) {
			continue;
		}

		/* Cache the data from the CQE */
		rc = cqe.result;

		/* Get the associated data */
		get_data_rc =
			rtio_cqe_get_mempool_buffer(&sensing_rtio_ctx, &cqe, &data, &data_len);
		if (get_data_rc != 0 || data_len == 0) {
			continue;
		}

		if ((uintptr_t)cqe.userdata >=
			    (uintptr_t)STRUCT_SECTION_START(sensing_sensor_info) &&
		    (uintptr_t)cqe.userdata < (uintptr_t)STRUCT_SECTION_END(sensing_sensor_info)) {
			// We got back a result for an info node
			process_info_node(cqe.userdata, data);
		}

		rtio_release_buffer(&sensing_rtio_ctx, data, data_len);
	}
}

K_THREAD_DEFINE(sensing_processor, CONFIG_SENSING_PROCESSING_THREAD_STACK_SIZE, processing_task,
		NULL, NULL, NULL, CONFIG_SENSING_PROCESSING_THREAD_PRIORITY, 0, 0);
