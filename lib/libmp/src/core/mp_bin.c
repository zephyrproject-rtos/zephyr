/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "mp_bin.h"
#include "mp_pad.h"

LOG_MODULE_REGISTER(mp_bin, CONFIG_LIBMP_LOG_LEVEL);

bool mp_bin_add(struct mp_bin *bin, struct mp_element *element, ...)
{
	va_list args;

	va_start(args, element);
	while (element) {
		/*
		 * Check element ID uniqueness in the nearest bin
		 * TODO: Should check in the whole pipeline
		 */
		struct mp_object *obj;

		SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
			if (MP_OBJECT(element)->id == obj->id) {
				return false;
			}
		}

		/* Set the element's parent */
		MP_OBJECT(element)->container = MP_OBJECT(bin);

		/* Add the element to the bin's list of children */
		sys_dlist_append(&bin->children, &element->object.node);
		bin->children_num++;
		element = va_arg(args, struct mp_element *);
	}

	va_end(args);

	return true;
}

enum mp_state_change_return mp_bin_change_state_func(struct mp_element *self,
						     enum mp_state_change transition)
{
	struct mp_object *obj;
	struct mp_element *element = NULL;
	enum mp_state_change_return ret = MP_STATE_CHANGE_FAILURE;
	struct mp_bin *bin = MP_BIN(self);
	enum mp_state next = MP_STATE_TRANSITION_NEXT(transition);
	struct mp_pad *first_sinkpad;
	sys_dnode_t *first_sinkpad_node;

	/* TODO: Activate bin's own src pads */

	/*
	 * TODO: Implement topology ordering: https://en.wikipedia.org/wiki/Topological_sorting
	 * For simplicity, support only simple pipelines without branched for now.
	 */

	/* Find the 1st sink element */
	SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
		element = MP_ELEMENT(obj);
		if (sys_dlist_is_empty(&element->srcpads)) {
			break;
		}
	}

	/* Change state of each element in the pipeline */
	while (element != NULL) {
		ret = element->change_state(element, transition);
		if (ret != MP_STATE_CHANGE_SUCCESS) {
			return ret;
		}

		/* Get the 1st sinkpad of the element */
		first_sinkpad_node = sys_dlist_peek_head(&element->sinkpads);
		if (first_sinkpad_node == NULL) {
			LOG_DBG("We reached the source\n");
			break;
		}

		first_sinkpad = CONTAINER_OF(first_sinkpad_node, struct mp_pad, object.node);
		/* Get next element */
		element = MP_ELEMENT(first_sinkpad->peer->object.container);
	}

	LOG_DBG("State changed to %d", next);

	return MP_STATE_CHANGE_SUCCESS;
}

void mp_bin_init(struct mp_element *self)
{
	struct mp_bin *bin = MP_BIN(self);

	self->change_state = mp_bin_change_state_func;

	sys_dlist_init(&bin->children);

	mp_bus_init(&bin->bus);
}
