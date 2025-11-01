/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "mp_bin.h"
#include "mp_pad.h"

LOG_MODULE_REGISTER(mp_bin, CONFIG_LIBMP_LOG_LEVEL);

bool mp_bin_add(MpBin *bin, MpElement *element, ...)
{
	va_list args;

	va_start(args, element);
	MpBus *bus = mp_element_get_bus(&bin->element);

	while (element) {
		/* Check element name uniqueness in the bin */
		MpObject *obj;

		SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
			if (strcmp(element->object.name, obj->name) == 0) {
				return false;
			}
		}

		/* Set the element's parent */
		element->object.container = MP_OBJECT(bin);

		/* Add the element to the bin's list of children */
		sys_dlist_append(&bin->children, &element->object.node);
		bin->children_num++;
		element->bus = bus;

		element = va_arg(args, MpElement *);
	}

	va_end(args);

	return true;
}

MpStateChangeReturn mp_bin_change_state_func(MpElement *self, MpStateChange transition)
{
	MpObject *obj;
	MpElement *element = NULL;
	MpStateChangeReturn ret = MP_STATE_CHANGE_FAILURE;
	MpBin *bin = MP_BIN(self);
	MpState next = (MpState)MP_STATE_TRANSITION_NEXT(transition);

	/* TODO: Activate bin's own src pads */

	/*
	 * TODO: Implement topology ordering: https://en.wikipedia.org/wiki/Topological_sorting
	 * For simplicity, support only simple pipelines without branched for now.
	 */

	/* Find the 1st sink element */
	SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
		element = MP_ELEMENT_CAST(obj);
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
		sys_dnode_t *first_sinkpad_node = sys_dlist_peek_head(&element->sinkpads);

		if (first_sinkpad_node == NULL) {
			LOG_DBG("We reached the source\n");
			break;
		}

		MpPad *first_sinkpad = MP_PAD(CONTAINER_OF(first_sinkpad_node, MpObject, node));

		/* Get next element */
		element = MP_ELEMENT_CAST(first_sinkpad->peer->object.container);
	}

	LOG_DBG("State changed to %d", next);

	return MP_STATE_CHANGE_SUCCESS;
}

void mp_bin_init(MpElement *self)
{
	MpBin *bin = MP_BIN(self);

	self->change_state = mp_bin_change_state_func;

	sys_dlist_init(&bin->children);
}
