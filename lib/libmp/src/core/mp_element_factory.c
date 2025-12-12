/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>

#include <zephyr/kernel.h>

#include "mp_element.h"
#include "mp_element_factory.h"

static struct mp_element_factory *mp_element_factory_find(enum mp_element_factory_id eid)
{
	STRUCT_SECTION_FOREACH(mp_element_factory, ef) {
		if (ef->id == eid) {
			return ef;
		}
	}

	return NULL;
}

struct mp_element *mp_element_factory_create(enum mp_element_factory_id eid, uint8_t id)
{
	struct mp_element_factory *ef = mp_element_factory_find(eid);

	if (ef) {
		struct mp_element *element = (struct mp_element *)k_calloc(1, ef->size);

		element->factory = ef;
		element->object.id = id;

		/* Init base element */
		mp_element_init(element);

		/* Init custom element */
		ef->init(element);

		return element;
	}

	return NULL;
}
