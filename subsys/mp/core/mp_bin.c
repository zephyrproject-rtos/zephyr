/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zephyr/mp/core/mp_bin.h>
#include <zephyr/mp/core/mp_pad.h>

LOG_MODULE_REGISTER(mp_bin, CONFIG_MP_LOG_LEVEL);

int mp_bin_add(struct mp_bin *bin, struct mp_element *element, ...)
{
	va_list args;

	if (bin == NULL) {
		return -EINVAL;
	}

	va_start(args, element);
	while (element != NULL) {
		/*
		 * Check element ID uniqueness in the nearest bin
		 * TODO: Should check in the whole pipeline
		 */
		struct mp_object *obj;

		SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
			if (element->object.id == obj->id) {
				va_end(args);
				return -EEXIST;
			}
		}

		if (bin->children_num >= CONFIG_MP_BIN_MAX_CHILDREN) {
			va_end(args);
			return -ENOSPC;
		}

		/* Set the element's parent */
		element->object.container = &bin->element.object;

		/* Add the element to the bin's list of children */
		sys_dlist_append(&bin->children, &element->object.node);
		bin->children_num++;
		element = va_arg(args, struct mp_element *);
	}

	va_end(args);

	return 0;
}

static int mp_bin_count_linked_pads(struct mp_element *element, sys_dlist_t *pad_list)
{
	struct mp_object *obj;
	int count = 0;

	SYS_DLIST_FOR_EACH_CONTAINER(pad_list, obj, node) {
		struct mp_pad *pad = (struct mp_pad *)obj;

		if (pad->peer != NULL) {
			count++;
		}
	}

	return count;
}

static int mp_bin_find_element_index(struct mp_element *elements[], int num,
				     struct mp_element *target)
{
	for (int i = 0; i < num; i++) {
		if (elements[i] == target) {
			return i;
		}
	}

	return -1;
}

static void mp_bin_decrement_peer_degrees(sys_dlist_t *pad_list, struct mp_element *elements[],
					  int degree[], int num_elements)
{
	struct mp_object *pad_obj;

	SYS_DLIST_FOR_EACH_CONTAINER(pad_list, pad_obj, node) {
		struct mp_pad *pad = (struct mp_pad *)pad_obj;

		if (pad->peer == NULL) {
			continue;
		}

		struct mp_element *peer_elem = (struct mp_element *)pad->peer->object.container;
		int idx = mp_bin_find_element_index(elements, num_elements, peer_elem);

		if (idx >= 0 && degree[idx] > 0) {
			degree[idx]--;
		}
	}
}

enum mp_state_change_return mp_bin_change_state_func(struct mp_element *self,
						     enum mp_state_change transition)
{
	struct mp_bin *bin = (struct mp_bin *)self;
	struct mp_object *obj;
	struct mp_element *elements[CONFIG_MP_BIN_MAX_CHILDREN];
	int degree[CONFIG_MP_BIN_MAX_CHILDREN];
	int num_elements = 0;
	int processed = 0;
	bool is_up_transition;

	/*
	 * Topological sort using BFS.
	 *
	 * For UP/DOWN transitions:
	 *   - Sinks/Sources first (elements with no srcpad/sinkpad links have degree 0)
	 *   - degree = number of linked srcpads/sinkpads
	 *   - After processing element E, for each sinkpad/srcpad of E, find the
	 *     peer srcpad/sinkpad's container element and decrement its degree.
	 */

	is_up_transition = (transition == MP_STATE_CHANGE_READY_TO_PAUSED ||
			    transition == MP_STATE_CHANGE_PAUSED_TO_PLAYING);

	/* Build elements array and compute initial degrees */
	SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
		if (num_elements >= CONFIG_MP_BIN_MAX_CHILDREN) {
			LOG_ERR("Too many elements in bin (max %d)", CONFIG_MP_BIN_MAX_CHILDREN);
			return MP_STATE_CHANGE_FAILURE;
		}

		struct mp_element *elem = (struct mp_element *)obj;

		elements[num_elements] = elem;

		if (is_up_transition) {
			/* UP: degree = number of linked srcpads */
			degree[num_elements] = mp_bin_count_linked_pads(elem, &elem->srcpads);
		} else {
			/* DOWN: degree = number of linked sinkpads */
			degree[num_elements] = mp_bin_count_linked_pads(elem, &elem->sinkpads);
		}

		num_elements++;
	}

	/* Process elements in topological order */
	while (processed < num_elements) {
		bool found = false;

		for (int i = 0; i < num_elements; i++) {
			if (degree[i] != 0) {
				continue;
			}

			/* Mark as processed by setting degree to -1 */
			degree[i] = -1;
			found = true;
			processed++;

			/* Change state of this element */
			enum mp_state_change_return ret;

			ret = elements[i]->change_state(elements[i], transition);
			if (ret != MP_STATE_CHANGE_SUCCESS) {
				return ret;
			}

			/*
			 * Decrement degree of peer elements.
			 *
			 * For UP/DOWN transitions:
			 *   - Iterate sinkpads/srcpads of this element to find the peer
			 *     sinkpad/srcpad'scontainer element and decrement its degree.
			 */
			sys_dlist_t *pad_list =
				is_up_transition ? &elements[i]->sinkpads : &elements[i]->srcpads;

			mp_bin_decrement_peer_degrees(pad_list, elements, degree, num_elements);
		}

		if (!found) {
			LOG_ERR("Cycle detected in pipeline topology or unlinked element");
			return MP_STATE_CHANGE_FAILURE;
		}
	}

	return MP_STATE_CHANGE_SUCCESS;
}

void mp_bin_init(struct mp_element *self)
{
	struct mp_bin *bin = (struct mp_bin *)self;

	self->change_state = mp_bin_change_state_func;

	sys_dlist_init(&bin->children);

	mp_bus_init(&bin->bus);
}
