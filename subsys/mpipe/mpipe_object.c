/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>

#include <zephyr/kernel.h>

#include <zephyr/mpipe/mpipe_object.h>

int mpipe_object_set_properties(struct mpipe_object *obj, ...)
{
	int ret;
	uint32_t key;
	const void *val;
	va_list args;

	__ASSERT_NO_MSG(obj != NULL);

	if (obj->set_property == NULL) {
		return -ENOTSUP;
	}

	va_start(args, obj);

	while (1) {
		key = va_arg(args, uint32_t);
		val = va_arg(args, const void *);

		if (key == MPIPE_PROP_LIST_END) {
			break;
		}

		ret = obj->set_property(obj, key, val);
		if (ret < 0) {
			va_end(args);
			return ret;
		}
	}

	va_end(args);

	return 0;
}

int mpipe_object_get_properties(struct mpipe_object *obj, ...)
{
	int ret;
	uint32_t key;
	void *val;
	va_list args;

	__ASSERT_NO_MSG(obj != NULL);

	if (obj->get_property == NULL) {
		return -ENOTSUP;
	}

	va_start(args, obj);

	while (1) {
		key = va_arg(args, uint32_t);
		val = va_arg(args, void *);

		if (key == MPIPE_PROP_LIST_END) {
			break;
		}

		ret = obj->get_property(obj, key, val);
		if (ret < 0) {
			va_end(args);
			return ret;
		}
	}

	va_end(args);

	return 0;
}

void mpipe_object_init(struct mpipe_object *obj)
{
	memset(obj, 0, sizeof(struct mpipe_object));
}
