/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_pad.h>
#include <zephyr/mpipe/mpipe_pipeline.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/base/mpipe_tee.h>

LOG_MODULE_REGISTER(mpipe_tee, CONFIG_MPIPE_LOG_LEVEL);

#define DEFAULT_SRC_PADS_NUM 2

static int mpipe_tee_sink_query_fn(struct mpipe_pad *pad, struct mpipe_dispatch *query)
{
	struct mpipe_tee *tee = (struct mpipe_tee *)pad->object.container;

	switch (query->type) {
	case MPIPE_DISPATCH_CAPS: {
		struct mpipe_structure filter;
		struct mpipe_structure answer;
		bool answered = false;
		int ret;

		filter = *query->caps;

		for (uint8_t i = 0; i < tee->src_pads_num; i++) {
			if (tee->src_pads[i].peer == NULL) {
				continue;
			}

			*query->caps = filter;

			ret = mpipe_pad_query(tee->src_pads[i].peer, query);
			if (ret != 0) {
				return ret;
			}

			if (!answered) {
				answer = *query->caps;
				answered = true;
			} else {
				struct mpipe_structure intersected;

				ret = mpipe_structure_intersect(&answer, query->caps, &intersected);
				if (ret != 0) {
					return ret;
				}

				answer = intersected;
			}
		}

		if (answered) {
			*query->caps = answer;
		}

		return 0;
	}
	case MPIPE_DISPATCH_BUFFER_POOL: {
		struct mpipe_buffer_pool_config merged = {0};

		for (uint8_t i = 0; i < tee->src_pads_num; i++) {
			if (tee->src_pads[i].peer == NULL) {
				continue;
			}

			/* Hand each branch a clean slate, not the previous one's proposal */
			query->pool = NULL;
			query->pool_cfg = (struct mpipe_buffer_pool_config){0};

			int ret = mpipe_pad_query(tee->src_pads[i].peer, query);

			if (ret != 0) {
				return ret;
			}

			/* Get pool configs from pool or standalone config */
			struct mpipe_buffer_pool *pool = query->pool;

			struct mpipe_buffer_pool_config *cfg =
				(pool != NULL) ? &pool->config : &query->pool_cfg;

			/* Combine all downstream branch's pool config proposals */
			if (cfg != NULL) {
				merged.size = MAX(merged.size, cfg->size);
				merged.min_buffers = MAX(merged.min_buffers, cfg->min_buffers);
				int align = sys_lcm(merged.align, cfg->align);

				if (align == 0 && cfg->align != 0) {
					merged.align = cfg->align;
				} else {
					merged.align = align;
				}
			}
		}
		/*
		 * Discard all downstream pool proposals.
		 * Upstream will use its own pool; if a downstream branch cannot
		 * use the buffer, it will need to copy into its own pool.
		 */
		query->pool_cfg = merged;
		/* The merge is the answer: no single branch's pool may travel up */
		query->pool = NULL;

		return 0;
	}
	default:
		return -ENOTSUP;
	}
}

static int mpipe_tee_sink_event_fn(struct mpipe_pad *pad, struct mpipe_dispatch *event)
{
	struct mpipe_tee *tee = (struct mpipe_tee *)pad->object.container;
	int ret = 0;
	int first_err = 0;

	switch (event->type) {
	case MPIPE_DISPATCH_CAPS:
	case MPIPE_DISPATCH_EOS: {
		struct mpipe_structure evt_caps;
		/* An event carrying no capability is informational: only forward it */
		bool is_caps = (event->type == MPIPE_DISPATCH_CAPS && event->caps != NULL);

		if (is_caps) {
			evt_caps = *event->caps;
		}

		for (uint8_t i = 0; i < tee->src_pads_num; i++) {
			if (tee->src_pads[i].peer == NULL) {
				continue;
			}

			if (is_caps) {
				/* Hand each branch the capability that arrived */
				*event->caps = evt_caps;

				ret = mpipe_pad_set_caps(&tee->src_pads[i], &evt_caps);
				if (ret != 0 && first_err == 0) {
					first_err = ret;
				}
			}

			ret = mpipe_pad_send_event(tee->src_pads[i].peer, event);
			if (ret != 0 && first_err == 0) {
				first_err = ret;
			}
		}

		if (is_caps && first_err == 0) {
			first_err = mpipe_pad_set_caps(pad, &evt_caps);
		}

		return first_err;
	}
	default:
		return -ENOTSUP;
	}
}

static int mpipe_tee_chain_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
			      struct net_buf **out_buf)
{
	struct mpipe_tee *tee = (struct mpipe_tee *)pad->object.container;
	uint8_t i = 0;
	int first_err = 0;
	int ret;

	*out_buf = NULL;

	for (i = 0; i < tee->src_pads_num; i++) {
		if (tee->src_pads[i].peer == NULL) {
			continue;
		}

		/* The push consumes the branch's reference, success or failure */
		ret = mpipe_push_buffer(&tee->src_pads[i], net_buf_ref(in_buf));
		if (ret != 0) {
			LOG_ERR("Tee pushes to src_pad[%u] failed (%d)", i, ret);
			if (first_err == 0) {
				first_err = ret;
			}
		}
	}

	net_buf_unref(in_buf);

	return first_err;
}

static enum mpipe_state_change_return mpipe_tee_change_state(struct mpipe_element *self,
							     enum mpipe_state_change transition)
{
	switch (transition) {
	case MPIPE_STATE_CHANGE_PAUSED_TO_READY:
		mpipe_element_reset_pad_caps(self);
		break;
	default:
		break;
	}

	return MPIPE_STATE_CHANGE_SUCCESS;
}

static int mpipe_tee_add_src_pad(struct mpipe_tee *tee)
{
	if (tee->src_pads_num >= CONFIG_MPIPE_BASE_TEE_MAX_SRC_PADS_NUM) {
		return -EINVAL;
	}

	mpipe_pad_init(&tee->src_pads[tee->src_pads_num], tee->src_pads_num, MPIPE_PAD_SRC,
		       MPIPE_PAD_ALWAYS);
	mpipe_element_add_pad(&tee->element, &tee->src_pads[tee->src_pads_num]);
	tee->src_pads_num++;

	return 0;
}

static int mpipe_tee_get_property(struct mpipe_object *obj, uint32_t id, void *val)
{
	struct mpipe_tee *tee = (struct mpipe_tee *)obj;

	switch (id) {
	case MPIPE_PROP_BASE_TEE_SRC_PADS_NUM:
		*(uint8_t *)val = tee->src_pads_num;

		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mpipe_tee_set_property(struct mpipe_object *obj, uint32_t id, const void *val)
{
	struct mpipe_tee *tee = (struct mpipe_tee *)obj;

	switch (id) {
	case MPIPE_PROP_BASE_TEE_SRC_PADS_NUM: {
		uint8_t requested = *(const uint8_t *)val;

		if (!IN_RANGE(requested, DEFAULT_SRC_PADS_NUM,
			      CONFIG_MPIPE_BASE_TEE_MAX_SRC_PADS_NUM)) {
			return -EINVAL;
		}

		while (tee->src_pads_num < requested) {
			mpipe_tee_add_src_pad(tee);
		}

		return 0;
	}
	default:
		return -ENOTSUP;
	}
}

int mpipe_tee_init(struct mpipe_tee *tee, uint8_t id)
{
	__ASSERT_NO_MSG(tee != NULL);

	struct mpipe_element *self = &tee->element;
	int ret = mpipe_element_init(self, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "tee");

	self->object.get_property = mpipe_tee_get_property;
	self->object.set_property = mpipe_tee_set_property;
	self->change_state = mpipe_tee_change_state;

	tee->sink_pad.chain_fn = mpipe_tee_chain_fn;
	tee->sink_pad.query_fn = mpipe_tee_sink_query_fn;
	tee->sink_pad.event_fn = mpipe_tee_sink_event_fn;

	/* Initialize the sink pad */
	mpipe_pad_init(&tee->sink_pad, 0, MPIPE_PAD_SINK, MPIPE_PAD_ALWAYS);
	mpipe_element_add_pad(self, &tee->sink_pad);

	/* Initialize the default source pads */
	tee->src_pads_num = 0;
	while (tee->src_pads_num < DEFAULT_SRC_PADS_NUM) {
		mpipe_tee_add_src_pad(tee);
	}

	return 0;
}
