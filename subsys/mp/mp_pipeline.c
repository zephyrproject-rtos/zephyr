/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/mp/mp_bus.h>
#include <zephyr/mp/mp_dispatch.h>
#include <zephyr/mp/mp_element.h>
#include <zephyr/mp/mp_message.h>
#include <zephyr/mp/mp_pad.h>
#include <zephyr/mp/mp_pipeline.h>
#include <zephyr/mp/mp_src.h>

LOG_MODULE_REGISTER(mp_pipeline, CONFIG_MP_LOG_LEVEL);

static uint32_t mp_pipeline_count_sinks(struct mp_bin *bin)
{
	struct mp_object *obj;
	struct mp_element *element;
	uint32_t count = 0;

	SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
		element = (struct mp_element *)obj;
		if (sys_dlist_is_empty(&element->srcpads)) {
			count++;
		}
	}

	return count;
}

static void mp_pipeline_set_flushing(struct mp_bin *bin, bool flush)
{
	struct mp_object *obj;
	struct mp_element *element;
	struct mp_object *pad_obj;

	SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
		element = (struct mp_element *)obj;

		SYS_DLIST_FOR_EACH_CONTAINER(&element->sinkpads, pad_obj, node) {
			atomic_set(&((struct mp_pad *)pad_obj)->flushing, flush ? 1 : 0);
		}

		SYS_DLIST_FOR_EACH_CONTAINER(&element->srcpads, pad_obj, node) {
			atomic_set(&((struct mp_pad *)pad_obj)->flushing, flush ? 1 : 0);
		}
	}
}

/*
 * Bus sync handler that aggregates EOS messages from all sinks.
 *
 * A pipeline with N sinks receives N EOS messages (one per sink). The
 * application must only be notified once every sink has finished, otherwise it
 * could tear down the pipeline while another branch is still processing. This
 * handler drops the first N-1 EOS messages and passes only the last one to the
 * application. Non-EOS messages are always passed through unchanged.
 *
 * It runs in the posting (pipeline/queue) thread, so it should stays short and
 * must never triggers a state change.
 */
static enum mp_bus_sync_reply mp_pipeline_eos_handler(struct mp_bus *bus,
						      struct mp_message *message, void *user_data)
{
	struct mp_pipeline *pipeline = user_data;
	uint32_t seen;

	ARG_UNUSED(bus);

	/* Only EOS is aggregated; everything else reaches the application. */
	if (message->type != MP_MESSAGE_EOS) {
		return MP_BUS_PASS;
	}

	/* atomic_inc() returns the value prior to the increment */
	seen = (uint32_t)atomic_inc(&pipeline->eos_count) + 1;

	/* Pass through once all sinks have signaled EOS (or if none are tracked) */
	if (pipeline->num_sinks == 0 || seen >= pipeline->num_sinks) {
		return MP_BUS_PASS;
	}

	/* Not all sinks are done yet: swallow this intermediate EOS */
	return MP_BUS_DROP;
}

int mp_pipeline_push_buffer(struct mp_pad *srcpad, struct net_buf *buffer)
{
	struct mp_pad *cur_srcpad = srcpad;
	struct mp_pad *next_sinkpad;
	struct mp_element *next_elem;
	struct net_buf *out_buf;
	sys_dnode_t *srcpad_node;
	int ret;

	if (cur_srcpad == NULL || buffer == NULL) {
		return -EINVAL;
	}

	while (cur_srcpad != NULL && buffer != NULL) {
		next_sinkpad = cur_srcpad->peer;

		if (next_sinkpad == NULL) {
			LOG_ERR("srcpad has no peer");
			net_buf_unref(buffer);
			return -ENOTCONN;
		}

		/*
		 * Flushing: drop the buffer rather than pushing it into an element
		 * whose caps have been reset or whose buffer pool has been freed.
		 */
		if (atomic_get(&next_sinkpad->flushing)) {
			net_buf_unref(buffer);
			return 0;
		}

		if (next_sinkpad->chainfn != NULL) {
			out_buf = NULL;

			ret = next_sinkpad->chainfn(next_sinkpad, buffer, &out_buf);
			if (ret != 0) {
				LOG_ERR("chainfn failed for element %u (%d)",
					next_sinkpad->object.container->id, ret);
				return ret;
			}

			if (out_buf == NULL) {
				/* Buffer consumed (by a sink or a queue element), exit the loop */
				break;
			}

			buffer = out_buf;
		}

		/* Move to the next element's first srcpad */
		next_elem = (struct mp_element *)next_sinkpad->object.container;
		srcpad_node = sys_dlist_peek_head(&next_elem->srcpads);
		if (srcpad_node == NULL) {
			/* Sink element reached - done */
			break;
		}
		cur_srcpad = CONTAINER_OF(srcpad_node, struct mp_pad, object.node);
	}

	return 0;
}

static void mp_pipeline_thread_func(void *p1, void *p2, void *p3)
{
	struct mp_bin *bin = p1;
	struct mp_pipeline *pipeline = p1;
	struct mp_object *obj;
	struct mp_element *element;
	struct mp_src *src = NULL;
	struct net_buf *buffer = NULL;
	struct mp_dispatch eos_event;
	uint32_t count = 0;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Find the 1st source element */
	SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
		element = (struct mp_element *)obj;
		if (sys_dlist_is_empty(&element->sinkpads)) {
			src = (struct mp_src *)element;
			break;
		}
	}

	if (src == NULL || src->pool == NULL || src->pool->acquire_buffer == NULL) {
		return;
	}

	while (mp_thread_wait(&pipeline->thread) == 0) {
		int acq_ret = 0;
		bool reached_limit = (src->num_buffers != 0 && count == src->num_buffers);

		if (!reached_limit) {
			acq_ret = src->pool->acquire_buffer(src->pool, &buffer);
		}

		/*
		 * EOS: playback sources reach the end of the file or
		 * live sources reache the limit number of buffers (-ENODATA)
		 */
		bool is_eos = reached_limit || acq_ret == -ENODATA;

		if (reached_limit || acq_ret != 0) {
			if (is_eos) {
				mp_dispatch_eos_init(&eos_event);
				if (mp_pad_send_event(src->srcpad.peer, &eos_event) != 0) {
					LOG_ERR("Failed to send EOS event downstream");
				}
			} else if (acq_ret != -EPIPE) {
				/*
				 * Not an EOS neither a forced-stop flush: a real error.
				 * TODO: Ideally, post an ERROR message to the bus here.
				 */
				LOG_ERR("Source failed to acquire a buffer (%d)", acq_ret);
			}
			count = 0;
			/* Self-pause: block in wait() until next resume */
			mp_thread_pause(&pipeline->thread);
			continue;
		}
		count++;
		if (mp_pipeline_push_buffer(&src->srcpad, buffer) != 0) {
			LOG_ERR("Failed to push buffer downstream");
		}
	}

	LOG_DBG("Pipeline thread exiting");
}

static enum mp_state_change_return mp_pipeline_change_state(struct mp_element *element,
							    enum mp_state_change transition)
{
	struct mp_pipeline *pipeline = (struct mp_pipeline *)element;
	enum mp_state_change_return ret;

	/*
	 * DOWN: Pipeline thread should be handled before children state change, i.e. source needs
	 * to stop producing buffers first.
	 */
	switch (transition) {
	case MP_STATE_CHANGE_PLAYING_TO_PAUSED:
		/*
		 * Only pause the source thread here (no join). Buffers already
		 * queued downstream are preserved so a subsequent resume to
		 * PLAYING continues without data loss. The source is guaranteed
		 * to be paused before this returns, so it stops producing.
		 *
		 * Do not set the flushing flag here: this is a pause, not a teardown, so in-flight
		 * and queued buffers must be kept intact for a subsequent resume.
		 */
		mp_thread_pause(&pipeline->thread);
		break;
	case MP_STATE_CHANGE_PAUSED_TO_READY:
		/*
		 * Teardown: raise the per-pad flushing gate BEFORE the children dismantle their
		 * caps and buffer pools. The source thread was paused on PLAYING -> PAUSED but may
		 * still be parked mid-chain holding a buffer. Once that buffer resumes (e.g.
		 * threads woke up to extit), the flushing gate in mp_pipeline_push_buffer() drops
		 * it instead of pushing it through an element whose caps have been reset or whose
		 * pool has been freed.
		 */
		mp_pipeline_set_flushing(&pipeline->bin, true);
		break;
	default:
		break;
	}

	/* Children state change: UP = sink-to-source, DOWN = source-to-sink */
	ret = mp_bin_change_state_func(element, transition);

	if (ret != MP_STATE_CHANGE_SUCCESS) {
		return ret;
	}

	/*
	 * DOWN (PAUSED -> READY): join the pipeline thread AFTER the children have
	 * transitioned. The source is already paused (from PLAYING -> PAUSED), so it
	 * is not producing new buffers. Draining the children first (each queue drains
	 * and unrefs its buffers on PAUSED -> READY) frees msgq slots and releases the
	 * pipeline thread if it was blocked in a full queue's k_msgq_put(K_FOREVER).
	 * Only then can the join complete, avoiding a teardown deadlock.
	 */
	if (transition == MP_STATE_CHANGE_PAUSED_TO_READY) {
		mp_thread_join(&pipeline->thread, K_FOREVER);
		/* Stop aggregating EOS and reset for a clean re-run */
		mp_bus_set_sync_handler(&pipeline->bin.bus, NULL, NULL);
		atomic_set(&pipeline->eos_count, 0);
	}

	/*
	 * UP: Pipeline thread should be handled after children state change, i.e., children
	 * need to be prepared before receiving buffers from source
	 */
	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		/* Clear the flushing gate so buffers can flow again */
		mp_pipeline_set_flushing(&pipeline->bin, false);

		/* Create the thread but do not start it (K_FOREVER) */
		if (mp_thread_create(&pipeline->thread, mp_pipeline_thread_func, element, NULL,
				     NULL, CONFIG_MP_THREAD_DEFAULT_PRIORITY, K_FOREVER) == NULL) {
			LOG_ERR("Failed to create a new pipeline thread");
			return MP_STATE_CHANGE_FAILURE;
		}

		/*
		 * Arm EOS aggregation now that the topology is fully built and before the source
		 * thread is resumed (i.e. before any EOS can be produced).
		 */
		pipeline->num_sinks = mp_pipeline_count_sinks(&pipeline->bin);
		atomic_set(&pipeline->eos_count, 0);
		mp_bus_set_sync_handler(&pipeline->bin.bus, mp_pipeline_eos_handler, pipeline);
		break;

	case MP_STATE_CHANGE_PAUSED_TO_PLAYING:
		mp_thread_resume(&pipeline->thread);
		break;
	default:
		break;
	}

	LOG_DBG("Pipeline id %u has changed state to %u", element->object.id,
		MP_STATE_TRANSITION_NEXT(transition));

	return MP_STATE_CHANGE_SUCCESS;
}

void mp_pipeline_init(struct mp_element *self)
{
	/* Initialize base class */
	mp_bin_init(self);
	self->change_state = mp_pipeline_change_state;
}
