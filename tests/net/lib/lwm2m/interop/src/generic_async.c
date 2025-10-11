/*
 * Copyright (c) 2025 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME app_generic_async
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <lwm2m_resource_ids.h>
#include <lwm2m_engine.h>
#include <zephyr/net/lwm2m.h>

#if defined(CONFIG_LWM2M_ASYNC_RESPONSES)

/* Dummy generic sensor instance demonstrating asynchronous operations. */

static struct k_work_delayable delayed_exec_work;
static struct k_work_delayable delayed_read_work;
static struct k_work_delayable delayed_write_work;

static double sensor_value;
static int64_t value_timestamp;

static void update_sensor_value(void)
{
	sensor_value += 1.0;
	value_timestamp = k_uptime_get();

	lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_GENERIC_SENSOR_ID, 0,
				 SENSOR_VALUE_RID), sensor_value);
}

/* Asynchronous read example */

static struct k_fifo pending_reads;

static void *value_read_cb(uint16_t obj_inst_id,
			   uint16_t res_id,
			   uint16_t res_inst_id,
			   size_t *data_len)
{
	static void *value_ctx;

	ARG_UNUSED(obj_inst_id);
	ARG_UNUSED(res_id);
	ARG_UNUSED(res_inst_id);
	ARG_UNUSED(data_len);

	/* Assume data is valid for 100 miliseconds second after update */
	if (value_timestamp != 0 && k_uptime_get() < value_timestamp + 100) {
		goto got_data;
	}

	/* Simulate delayed sensor read. */
	k_work_schedule(&delayed_read_work, K_MSEC(500));

	value_ctx = lwm2m_response_postpone(lwm2m_rd_client_ctx());
	if (value_ctx == NULL) {
		LOG_INF("Engine rejected postponed response request, block.");
		goto wait_data;
	}

	if (k_fifo_alloc_put(&pending_reads, value_ctx) < 0) {
		(void)lwm2m_response_resume(lwm2m_rd_client_ctx(), value_ctx, -ENOMEM);
	}

	return NULL;

wait_data:
	lwm2m_acknowledge(lwm2m_rd_client_ctx());
	k_sleep(K_MSEC(500));
	update_sensor_value();

got_data:
	*data_len = sizeof(sensor_value);

	return &sensor_value;
}

static void delayed_read_cb(struct k_work *work)
{
	void *value_ctx;
	int ret;

	ARG_UNUSED(work);

	update_sensor_value();

	/* Resume pending reads. */
	while ((value_ctx = k_fifo_get(&pending_reads, K_NO_WAIT)) != NULL) {
		ret = lwm2m_response_resume(lwm2m_rd_client_ctx(), value_ctx, 0);
		if (ret < 0) {
			LOG_INF("Resuming delayed read failed %d", ret);
		}
	}
}

/* Asynchronous write example */

static void *app_type_ctx;

int app_type_post_write_cb(uint16_t obj_inst_id, uint16_t res_id,
			   uint16_t res_inst_id, uint8_t *data,
			   uint16_t data_len, bool last_block,
			   size_t total_size, size_t offset)
{
	ARG_UNUSED(obj_inst_id);
	ARG_UNUSED(res_id);
	ARG_UNUSED(res_inst_id);
	ARG_UNUSED(data);
	ARG_UNUSED(data_len);
	ARG_UNUSED(last_block);
	ARG_UNUSED(total_size);
	ARG_UNUSED(offset);

	/* In case another write is already pending, return an error. */
	if (app_type_ctx != NULL) {
		return -ECANCELED;
	}

	app_type_ctx = lwm2m_response_postpone(lwm2m_rd_client_ctx());
	if (app_type_ctx != NULL) {
		/* Simulate delayed write processing. */
		k_work_schedule(&delayed_write_work, K_MSEC(500));
	} else {
		LOG_INF("Engine rejected postponed response request, block.");

		/* Simulate delayed write processing. */
		lwm2m_acknowledge(lwm2m_rd_client_ctx());
		k_sleep(K_MSEC(500));
	}

	return 0;
}

static void delayed_write_cb(struct k_work *work)
{
	int ret;

	ARG_UNUSED(work);

	if (app_type_ctx == NULL) {
		return;
	}

	ret = lwm2m_response_resume(lwm2m_rd_client_ctx(), app_type_ctx, 0);
	if (ret < 0) {
		LOG_INF("Resuming delayed write failed %d", ret);
	}

	app_type_ctx = NULL;
}

/* Asynchronous exec example */

static void *min_max_reset_ctx;

static void min_max_reset(void)
{
	lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_GENERIC_SENSOR_ID, 0,
				 MIN_MEASURED_VALUE_RID), sensor_value);
	lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_GENERIC_SENSOR_ID, 0,
				 MAX_MEASURED_VALUE_RID), sensor_value);
}

static int min_max_reset_exec_cb(uint16_t obj_inst_id, uint8_t *args,
				 uint16_t args_len)
{
	ARG_UNUSED(obj_inst_id);
	ARG_UNUSED(args);
	ARG_UNUSED(args_len);

	/* In case another exec is already pending, return an error. */
	if (min_max_reset_ctx != NULL) {
		return -ECANCELED;
	}

	min_max_reset_ctx = lwm2m_response_postpone(lwm2m_rd_client_ctx());
	if (min_max_reset_ctx != NULL) {
		/* Simulate delayed exec processing. */
		k_work_schedule(&delayed_exec_work, K_MSEC(500));
	} else {
		LOG_INF("Engine rejected postponed response request, block.");

		/* Simulate delayed exec processing. */
		lwm2m_acknowledge(lwm2m_rd_client_ctx());
		k_sleep(K_MSEC(500));
		min_max_reset();
	}

	return 0;
}

static void delayed_exec_cb(struct k_work *work)
{
	int ret;

	ARG_UNUSED(work);

	if (min_max_reset_ctx == NULL) {
		return;
	}

	min_max_reset();

	ret = lwm2m_response_resume(lwm2m_rd_client_ctx(), min_max_reset_ctx, 0);
	if (ret < 0) {
		LOG_INF("Resuming delayed exec failed %d", ret);
	}

	min_max_reset_ctx = NULL;
}

static int create_generic_async(uint16_t obj_inst_id)
{
	sensor_value = 0.0;

	lwm2m_register_read_callback(&LWM2M_OBJ(IPSO_OBJECT_GENERIC_SENSOR_ID,
						obj_inst_id, SENSOR_VALUE_RID),
				     value_read_cb);
	lwm2m_register_post_write_callback(&LWM2M_OBJ(IPSO_OBJECT_GENERIC_SENSOR_ID,
						      obj_inst_id, APPLICATION_TYPE_RID),
					   app_type_post_write_cb);
	lwm2m_register_exec_callback(&LWM2M_OBJ(IPSO_OBJECT_GENERIC_SENSOR_ID, obj_inst_id,
						RESET_MIN_MAX_MEASURED_VALUES_RID),
				     min_max_reset_exec_cb);

	return 0;
}

static int init_generic_async_object(void)
{
	lwm2m_register_create_callback(IPSO_OBJECT_GENERIC_SENSOR_ID, create_generic_async);

	k_work_init_delayable(&delayed_exec_work, delayed_exec_cb);
	k_work_init_delayable(&delayed_read_work, delayed_read_cb);
	k_work_init_delayable(&delayed_write_work, delayed_write_cb);
	k_fifo_init(&pending_reads);

	return 0;
}

LWM2M_APP_INIT(init_generic_async_object);

#endif /* defined(CONFIG_LWM2M_ASYNC_RESPONSES) */
