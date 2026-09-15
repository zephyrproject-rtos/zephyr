/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe_bin.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_object.h>
#include <zephyr/mpipe/mpipe_pad.h>

LOG_MODULE_REGISTER(mpipe_bin, CONFIG_MPIPE_LOG_LEVEL);

/*
 * The framework allocates nothing, so neither may the bus underneath it.
 * subsys/mpipe/Kconfig turns ZBUS_PREFER_DYNAMIC_ALLOCATION off to get there,
 * but that is a default among several and only wins on Kconfig parse order.
 * Say what mpipe needs rather than trusting where its Kconfig is sourced.
 */
#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_DYNAMIC) ||                                       \
	defined(CONFIG_ZBUS_RUNTIME_OBSERVERS_NODE_ALLOC_DYNAMIC)
#error "mpipe needs static zbus allocation: set CONFIG_ZBUS_PREFER_DYNAMIC_ALLOCATION=n"
#endif

#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_STATIC)
BUILD_ASSERT(sizeof(struct mpipe_message) <= CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC_DATA_SIZE,
	     "A zbus message subscriber buffer cannot hold an mpipe_message");
#endif

int mpipe_bin_add(struct mpipe_bin *bin, struct mpipe_element *element, ...)
{
	va_list args;

	__ASSERT_NO_MSG(bin != NULL);

	va_start(args, element);
	while (element != NULL) {
		/*
		 * Check element ID uniqueness in the nearest bin
		 * TODO: Should check in the whole pipeline
		 */
		struct mpipe_object *obj;

		SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
			if (element->object.id == obj->id) {
				va_end(args);
				return -EEXIST;
			}
		}

		if (bin->children_num >= CONFIG_MPIPE_BIN_MAX_CHILDREN) {
			va_end(args);
			return -ENOSPC;
		}

		/* Set the element's parent */
		element->object.container = &bin->element.object;

		/* Add the element to the bin's list of children */
		sys_dlist_append(&bin->children, &element->object.node);
		bin->children_num++;
		element = va_arg(args, struct mpipe_element *);
	}

	va_end(args);

	return 0;
}

static int mpipe_bin_count_linked_pads(struct mpipe_element *element, sys_dlist_t *pad_list)
{
	struct mpipe_object *obj;
	int count = 0;

	SYS_DLIST_FOR_EACH_CONTAINER(pad_list, obj, node) {
		struct mpipe_pad *pad = (struct mpipe_pad *)obj;

		if (pad->peer != NULL) {
			count++;
		}
	}

	return count;
}

static int mpipe_bin_find_element_index(struct mpipe_element *elements[], int num,
					struct mpipe_element *target)
{
	for (int i = 0; i < num; i++) {
		if (elements[i] == target) {
			return i;
		}
	}

	return -1;
}

static void mpipe_bin_decrement_peer_degrees(sys_dlist_t *pad_list,
					     struct mpipe_element *elements[], int degree[],
					     int num_elements)
{
	struct mpipe_object *pad_obj;

	SYS_DLIST_FOR_EACH_CONTAINER(pad_list, pad_obj, node) {
		struct mpipe_pad *pad = (struct mpipe_pad *)pad_obj;

		if (pad->peer == NULL) {
			continue;
		}

		struct mpipe_element *peer_elem =
			(struct mpipe_element *)pad->peer->object.container;
		int idx = mpipe_bin_find_element_index(elements, num_elements, peer_elem);

		if (idx >= 0 && degree[idx] > 0) {
			degree[idx]--;
		}
	}
}

enum mpipe_state_change_return mpipe_bin_change_state_func(struct mpipe_element *self,
							   enum mpipe_state_change transition)
{
	struct mpipe_bin *bin = (struct mpipe_bin *)self;
	struct mpipe_object *obj;
	struct mpipe_element *elements[CONFIG_MPIPE_BIN_MAX_CHILDREN];
	int degree[CONFIG_MPIPE_BIN_MAX_CHILDREN];
	int num_elements = 0;
	int processed = 0;
	bool is_up_transition;

	/*
	 * Topological sort using BFS.
	 *
	 * For UP/DOWN transitions:
	 *   - Sinks/Sources first (elements with no src_pad/sink_pad links have degree 0)
	 *   - degree = number of linked src_pads/sink_pads
	 *   - After processing element E, for each sink_pad/src_pad of E, find the
	 *     peer src_pad/sink_pad's container element and decrement its degree.
	 */

	is_up_transition = (transition == MPIPE_STATE_CHANGE_READY_TO_PAUSED ||
			    transition == MPIPE_STATE_CHANGE_PAUSED_TO_PLAYING);

	/* Build elements array and compute initial degrees */
	SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
		if (num_elements >= CONFIG_MPIPE_BIN_MAX_CHILDREN) {
			LOG_ERR("Too many elements in bin (max %d)", CONFIG_MPIPE_BIN_MAX_CHILDREN);
			return MPIPE_STATE_CHANGE_FAILURE;
		}

		struct mpipe_element *elem = (struct mpipe_element *)obj;

		elements[num_elements] = elem;

		if (is_up_transition) {
			/* UP: degree = number of linked src_pads */
			degree[num_elements] = mpipe_bin_count_linked_pads(elem, &elem->src_pads);
		} else {
			/* DOWN: degree = number of linked sink_pads */
			degree[num_elements] = mpipe_bin_count_linked_pads(elem, &elem->sink_pads);
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
			enum mpipe_state_change_return ret;

			ret = elements[i]->change_state(elements[i], transition);
			if (ret != MPIPE_STATE_CHANGE_SUCCESS) {
				return ret;
			}

			elements[i]->current_state = MPIPE_STATE_TRANSITION_NEXT(transition);

			/*
			 * Decrement degree of peer elements.
			 *
			 * For UP/DOWN transitions:
			 *   - Iterate sink_pads/src_pads of this element to find the peer
			 *     sink_pad/src_pad'scontainer element and decrement its degree.
			 */
			sys_dlist_t *pad_list =
				is_up_transition ? &elements[i]->sink_pads : &elements[i]->src_pads;

			mpipe_bin_decrement_peer_degrees(pad_list, elements, degree, num_elements);
		}

		if (!found) {
			LOG_ERR("Cycle detected in pipeline topology or unlinked element");
			return MPIPE_STATE_CHANGE_FAILURE;
		}
	}

	return MPIPE_STATE_CHANGE_SUCCESS;
}

/*
 * Bring up the bin's bus channel, with no validator: a bin has no opinion on
 * the messages passing through it. An element wrapping the bin installs one
 * with mpipe_bin_set_bus_validator().
 *
 * chan_msg is the channel's backing message buffer. It is required for every
 * bus channel and observer type: each publish copies the message into it, and
 * channel init rejects a NULL buffer.
 *
 * The channel is deliberately left out of zbus_runtime_channel_register().
 * That registry only feeds zbus_iterate_over_channels(), which mpipe never
 * calls; publishing and attaching observers work off the channel itself.
 */
static void mpipe_bin_init_bus(struct mpipe_bin *bin)
{
	zbus_runtime_channel_init(&bin->bus, &bin->chan_data, NULL, ZBUS_CHAN_ID_INVALID, NULL,
				  &bin->chan_msg, sizeof(bin->chan_msg), bin);
}

int mpipe_bin_init(struct mpipe_bin *bin, uint8_t id)
{
	__ASSERT_NO_MSG(bin != NULL);

	struct mpipe_element *self = &bin->element;
	int ret = mpipe_element_init(self, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "bin");

	self->change_state = mpipe_bin_change_state_func;
	self->object.flags |= MPIPE_OBJECT_FLAG_BIN;

	sys_dlist_init(&bin->children);
	mpipe_bin_init_bus(bin);

	return 0;
}

int mpipe_bin_set_bus_validator(struct mpipe_bin *bin, zbus_validator bus_validator,
				void *user_data)
{
	struct zbus_channel *chan;
	int ret;

	__ASSERT_NO_MSG(bin != NULL);

	chan = &bin->bus.channel;

	/* zbus_chan_claim() is the public form of taking the channel's lock */
	ret = zbus_chan_claim(chan, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	chan->validator = bus_validator;
	chan->user_data = user_data;

	return zbus_chan_finish(chan);
}
