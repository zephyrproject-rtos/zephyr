/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/logging/log.h>

#include "mp_bus.h"
#include "mp_element.h"
#include "mp_pad.h"
#include "mp_pipeline.h"
#include "mp_src.h"

LOG_MODULE_REGISTER(mp_pipeline, CONFIG_LIBMP_LOG_LEVEL);

static int mp_pipeline_set_property(struct mp_object *obj, uint32_t id, const void *val)
{
	return 0;
}

static int mp_pipeline_get_property(struct mp_object *obj, uint32_t id, void *val)
{
	return 0;
}

static void mp_pipeline_thread(void *p1, void *p2, void *p3)
{
	struct mp_bin *bin = p1;
	struct mp_pipeline *pipeline = p1;
	struct mp_object *obj;
	struct mp_element *element;
	struct mp_src *src = NULL;
	struct mp_buffer *buffer = NULL;
	struct mp_buffer *out_buf = NULL;
	struct mp_message *eos_message = NULL;
	struct mp_element *cur_elem;
	struct mp_pad *cur_srcpad;
	struct mp_pad *next_sinkpad;
	sys_dnode_t *srcpad_node;
	uint32_t count = 0;

	/* Currently, only pipelines without branches and push mode are supported */

	/* Find the 1st source element */
	SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
		element = MP_ELEMENT(obj);
		if (sys_dlist_is_empty(&element->sinkpads)) {
			src = MP_SRC(element);
			break;
		}
	}

	if (src == NULL) {
		LOG_ERR("No source element found in the pipeline");
		return;
	}

	pipeline->task.running = true;

	/* Main loop - in push mode, driven by source producing buffers */
	while (pipeline->task.running) {
		/* Get buffer from source element's pool */
		if (!src->pool->acquire_buffer(src->pool, &buffer)) {
			LOG_ERR("Failed to acquire buffer from source");
			break;
		}

		/* Process buffer through the pipeline by calling each element's chainfn */
		cur_elem = MP_ELEMENT(src);
		while (cur_elem != NULL && buffer != NULL) {
			/* Only elements having a single srcpad is supported for now */

			/* Get the 1st srcpad of the current element */
			srcpad_node = sys_dlist_peek_head(&cur_elem->srcpads);
			if (srcpad_node == NULL) {
				/* This is the sink element (no srcpad), we're done */
				break;
			}
			cur_srcpad = CONTAINER_OF(srcpad_node, struct mp_pad, object.node);

			/* Check if there's a peer (next element) */
			if (cur_srcpad->peer == NULL) {
				LOG_ERR("srcpad has no peer");
				break;
			}

			/* Call the next element's chainfn to process the buffer */
			next_sinkpad = cur_srcpad->peer;
			if (next_sinkpad->chainfn != NULL) {
				out_buf = NULL;

				if (!next_sinkpad->chainfn(next_sinkpad, buffer, &out_buf)) {
					LOG_ERR("chainfn failed for element %d",
						MP_OBJECT(next_sinkpad->object.container)->id);
					/* Clean up buffer on error */
					if (buffer != NULL) {
						mp_buffer_unref(buffer);
					}
					buffer = NULL;
					break;
				}

				if (out_buf == NULL) {
					break;
				}

				/* Use output buffer as input for next element */
				buffer = out_buf;
			}

			/* Move to next element */
			cur_elem = MP_ELEMENT(next_sinkpad->object.container);
		}

		/* Stop the pipeline when reaching the src's num_buffers */
		if (src->num_buffers != 0 && ++count == src->num_buffers) {
			/* Post EOS message to the pipeline's bus */
			pipeline->task.running = false;
			eos_message = mp_message_new(MP_MESSAGE_EOS, MP_OBJECT(src), NULL);
			mp_bus_post(&bin->bus, eos_message);
		}
	}
}

static enum mp_state_change_return mp_pipeline_change_state(struct mp_element *element,
							    enum mp_state_change transition)
{
	struct mp_pipeline *pipeline = MP_PIPELINE(element);
	enum mp_state_change_return ret = mp_bin_change_state_func(element, transition);

	if (ret != MP_STATE_CHANGE_SUCCESS) {
		return ret;
	}

	LOG_DBG("Pipeline id %u has successfully changed state to %u", MP_OBJECT(element)->id,
		MP_STATE_TRANSITION_NEXT(transition));

	/* Start the pipeline processing thread */
	if (transition == MP_STATE_CHANGE_PAUSED_TO_PLAYING) {
		mp_task_create(&pipeline->task, mp_pipeline_thread, element, NULL, NULL,
			       CONFIG_MP_THREAD_DEFAULT_PRIORITY);
	}

	/* Paused the pipeline processing thread */
	if (transition == MP_STATE_CHANGE_PLAYING_TO_PAUSED) {
		pipeline->task.running = false;
		/* TODO: Wait for thread to finish */
	}

	return ret;
}

void mp_pipeline_init(struct mp_element *self)
{
	/* Init base class */
	mp_bin_init(self);

	self->object.set_property = mp_pipeline_set_property;
	self->object.get_property = mp_pipeline_get_property;
	self->change_state = mp_pipeline_change_state;
}
