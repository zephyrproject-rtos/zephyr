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

MpElementFactory *mp_element_factory_find(const char *name)
{
	STRUCT_SECTION_FOREACH(_MpElementFactory, ef) {
		if (strcmp(ef->name, name) == 0) {
			return ef;
		}
	}

	return NULL;
}

MpElement *mp_element_factory_create(const char *fname, const char *ename)
{
	MpElementFactory *ef = mp_element_factory_find(fname);

	if (ef) {
		MpElement *element = (MpElement *)k_calloc(1, ef->size);

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
