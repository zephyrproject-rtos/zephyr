/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <string.h>

#include <zephyr/kernel.h>

#include "mp_element.h"
#include "mp_element_factory.h"

struct mp_element_factory *mp_element_factory_find(const char *name)
{
	STRUCT_SECTION_FOREACH(mp_element_factory, ef) {
		if (strcmp(ef->name, name) == 0) {
			return ef;
		}
	}

	return NULL;
}

struct mp_element *mp_element_factory_create(const char *fname, const char *ename)
{
	struct mp_element_factory *ef = mp_element_factory_find(fname);

	if (ef) {
		struct mp_element *element = (struct mp_element *)k_calloc(1, ef->size);

		element->factory = ef;
		element->object.name = ename;

		/* Init base element */
		mp_element_init(element);

		/* Init custom element */
		ef->init(element);

		return element;
	}

	return NULL;
}
