/*
 * Copyright (c) 2022-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SENSING_SENSOR_H_
#define ZEPHYR_INCLUDE_SENSING_SENSOR_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sensing/sensing.h>

struct sensing_connection {
	const struct sensing_sensor_info *info;
	const struct sensing_callback_list *cb_list;
	void *userdata;
	enum sensing_sensor_mode mode;
	q31_t attributes[SENSOR_ATTR_COMMON_COUNT];
	uint32_t attribute_mask;
} __packed __aligned(4);

#define SENSING_CONNECTION_DT_DEFINE(node_id, target_node_id, type, _cb_list)                      \
	SENSING_DMEM STRUCT_SECTION_ITERABLE(sensing_connection, node_id##_sensing_connection) = { \
		.info = SENSING_SENSOR_INFO_GET(target_node_id, type),                             \
		.cb_list = (_cb_list),                                                             \
	}

STRUCT_SECTION_START_EXTERN(sensing_connection);
STRUCT_SECTION_END_EXTERN(sensing_connection);

#define SENSING_SENSOR_INFO_DT_NAME(node_id, type)                                                 \
	_CONCAT(_CONCAT(__sensing_sensor_info_, DEVICE_DT_NAME_GET(node_id)), type)

#define SENSING_SENSOR_INFO_INST_DEFINE_NAMED(node_id, name, prop, idx, _iodev)                    \
	IF_ENABLED(CONFIG_SENSING_SHELL, (static char node_id##_##idx##_name_buffer[5];))          \
	const STRUCT_SECTION_ITERABLE(sensing_sensor_info, name) = {                               \
		.info = &SENSOR_INFO_DT_NAME(DT_PHANDLE(node_id, dev)),                            \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.type = DT_PROP_BY_IDX(node_id, prop, idx),                                        \
		.iodev = &(_iodev),                                                                \
		IF_ENABLED(CONFIG_SENSING_SHELL,                                                   \
			   (.shell_name = node_id##_##idx##_name_buffer, ))};

#define SENSING_SENSOR_INFO_INST_DEFINE(node_id, prop, idx, _iodev)                                \
	SENSING_SENSOR_INFO_INST_DEFINE_NAMED(                                                     \
		node_id, SENSING_SENSOR_INFO_DT_NAME(node_id, DT_PROP_BY_IDX(node_id, prop, idx)), \
		prop, idx, _iodev)

#define SENSING_SENSOR_INFO_DT_DEFINE(node_id)                                                     \
	SENSOR_DT_READ_IODEV(node_id##_read_iodev, node_id, SENSOR_CHAN_ALL);                      \
	DT_FOREACH_PROP_ELEM_VARGS(node_id, sensor_types, SENSING_SENSOR_INFO_INST_DEFINE,         \
				   node_id##_read_iodev)

#define SENSING_SENSOR_DT_DEFINE(node_id, init_fn, pm_device, data_ptr, cfg_ptr, level, prio,      \
				 api_ptr, ...)                                                     \
	SENSING_SENSOR_INFO_DT_DEFINE(node_id);                                                    \
	DEVICE_DT_DEFINE(node_id, init_fn, pm_device, data_ptr, cfg_ptr, level, prio, api_ptr,     \
			 __VA_ARGS__)

#define SENSING_SENSOR_DT_INST_DEFINE(inst, ...)                                                   \
	SENSING_SENSOR_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

#define SENSING_SENSOR_INFO_GET(node_id, type) &SENSING_SENSOR_INFO_DT_NAME(node_id, type)

STRUCT_SECTION_START_EXTERN(sensing_sensor_info);
STRUCT_SECTION_END_EXTERN(sensing_sensor_info);

#if defined(CONFIG_HAS_DTS) || defined(__DOXYGEN__)
#define Z_MAYBE_SENSING_SENSOR_INFO_DECLARE_INTERNAL_DEFINE(node_id, prop, idx)                    \
	extern const struct sensing_sensor_info SENSING_SENSOR_INFO_DT_NAME(                       \
		node_id, DT_PROP_BY_IDX(node_id, prop, idx));

#define Z_MAYBE_SENSING_SENSOR_INFO_DECLARE_INTERNAL(node_id)                                      \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, sensor_types),                                       \
		    (DT_FOREACH_PROP_ELEM(node_id, sensor_types,                                   \
					  Z_MAYBE_SENSING_SENSOR_INFO_DECLARE_INTERNAL_DEFINE)),   \
		    ())

DT_FOREACH_STATUS_OKAY_NODE(Z_MAYBE_SENSING_SENSOR_INFO_DECLARE_INTERNAL);
#endif /* CONFIG_HAS_DTS || __DOXYGEN__ */

#ifdef __cplusplus
}
#endif

#endif /*ZEPHYR_INCLUDE_SENSING_SENSOR_H_*/
