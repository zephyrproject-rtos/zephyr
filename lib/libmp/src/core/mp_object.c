/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>

#include <zephyr/kernel.h>

#include "mp_object.h"
#include "mp_property.h"

int mp_object_set_properties(MpObject *obj, ...)
{
	int ret;
	uint32_t key;
	const void *val;
	va_list args;

	__ASSERT_NO_MSG(obj != NULL);

	va_start(args, obj);

	while (true) {
		key = va_arg(args, uint32_t);
		val = va_arg(args, const void *);

		if (key == PROP_LIST_END) {
			break;
		}

		ret = obj->set_property(obj, key, val);
		if (ret < 0) {
			return ret;
		}
	}

	va_end(args);

	return 0;
}

int mp_object_get_properties(MpObject *obj, ...)
{
	int ret;
	uint32_t key;
	void *val;
	va_list args;

	__ASSERT_NO_MSG(obj != NULL);

	va_start(args, obj);

	while (true) {
		key = va_arg(args, uint32_t);
		val = va_arg(args, void *);

		if (key == PROP_LIST_END) {
			break;
		}

		ret = obj->get_property(obj, key, val);
		if (ret < 0) {
			return ret;
		}
	}

	va_end(args);

	return 0;
}

MpObject *mp_object_ref(MpObject *obj)
{
	if (obj == NULL) {
		return NULL;
	}

	atomic_inc(&obj->ref);

	return obj;
}

void mp_object_unref(MpObject *obj)
{
	if (obj != NULL) {
		__ASSERT_NO_MSG(atomic_get(&obj->ref) > 0);
		if (atomic_dec(&obj->ref) == 1) {
			obj->release(obj);
		}
	}
}

void mp_object_replace(MpObject **ptr, MpObject *new_obj)
{
	MpObject *old_ref;

	__ASSERT_NO_MSG(ptr != NULL);
	old_ref = *ptr;
	*ptr = mp_object_ref(new_obj);
	mp_object_unref(old_ref);
}
