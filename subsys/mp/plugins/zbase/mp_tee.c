/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_dispatch.h>
#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_pad.h>
#include <zephyr/mp/core/mp_pipeline.h>
#include <zephyr/mp/zbase/mp_tee.h>

LOG_MODULE_REGISTER(mp_tee, CONFIG_MP_LOG_LEVEL);

#define DEFAULT_SRCPADS_NUM 2

static int mp_tee_sink_queryfn(struct mp_pad *pad, struct mp_dispatch *query)
{
	struct mp_tee *tee = (struct mp_tee *)pad->object.container;

	switch (query->type) {
	case MP_DISPATCH_CAPS:
		for (uint8_t i = 0; i < tee->srcpads_num; i++) {
			if (tee->srcpads[i].peer == NULL) {
				continue;
			}

			int ret = mp_pad_query(tee->srcpads[i].peer, query);

			if (ret != 0) {
				return ret;
			}
		}

		return 0;
	case MP_DISPATCH_BUFFER_CONFIG: {
		struct mp_buffer_pool_config merged = {0};

		for (uint8_t i = 0; i < tee->srcpads_num; i++) {
			if (tee->srcpads[i].peer == NULL) {
				continue;
			}

			int ret = mp_pad_query(tee->srcpads[i].peer, query);

			if (ret != 0) {
				return ret;
			}

			/* Get pool configs from pool or standalone config */
			struct mp_buffer_pool *pool = mp_dispatch_get_pool(query);
			struct mp_buffer_pool_config *cfg =
				(pool != NULL) ? &pool->config : mp_dispatch_get_pool_config(query);

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
		mp_dispatch_set_pool_config(query, &merged);

		return 0;
	}
	default:
		return -ENOTSUP;
	}
}

static int mp_tee_sink_eventfn(struct mp_pad *pad, struct mp_dispatch *event)
{
	struct mp_tee *tee = (struct mp_tee *)pad->object.container;
	int ret = 0;
	int first_err = 0;

	switch (event->type) {
	case MP_DISPATCH_CAPS:
	case MP_DISPATCH_EOS:
		for (uint8_t i = 0; i < tee->srcpads_num; i++) {
			if (tee->srcpads[i].peer == NULL) {
				continue;
			}

			ret = mp_pad_send_event(tee->srcpads[i].peer, event);
			if (ret != 0 && first_err == 0) {
				first_err = ret;
			}

			if (event->type == MP_DISPATCH_CAPS && ret == 0) {
				struct mp_caps *caps = mp_dispatch_get_caps(event);

				mp_caps_replace(&pad->caps, caps);
				mp_caps_unref(caps);
			}
		}

		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_tee_chainfn(struct mp_pad *pad, struct net_buf *in_buf, struct net_buf **out_buf)
{
	struct mp_tee *tee = (struct mp_tee *)pad->object.container;
	uint8_t i = 0;
	int first_err = 0;
	int ret;

	*out_buf = NULL;

	for (i = 0; i < tee->srcpads_num; i++) {
		ret = mp_pipeline_push_buffer(&tee->srcpads[i], net_buf_ref(in_buf));
		if (ret != 0) {
			LOG_ERR("Tee pushes to srcpad[%u] failed (%d)", i, ret);
			net_buf_unref(in_buf);
			if (first_err == 0) {
				first_err = ret;
			}
		}
	}

	net_buf_unref(in_buf);

	return first_err;
}

static int mp_tee_add_srcpad(struct mp_tee *tee)
{
	if (tee->srcpads_num >= CONFIG_MP_TEE_MAX_SRCPADS_NUM) {
		return -EINVAL;
	}

	mp_pad_init(&tee->srcpads[tee->srcpads_num], tee->srcpads_num, MP_PAD_SRC, MP_PAD_ALWAYS,
		    tee->caps);
	mp_element_add_pad(&tee->element, &tee->srcpads[tee->srcpads_num]);
	tee->srcpads_num++;

	return 0;
}

static int mp_tee_get_property(struct mp_object *obj, uint32_t id, void *val)
{
	struct mp_tee *tee = (struct mp_tee *)obj;

	switch (id) {
	case PROP_TEE_SRCPADS_NUM:
		*(uint8_t *)val = tee->srcpads_num;

		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_tee_set_property(struct mp_object *obj, uint32_t id, const void *val)
{
	struct mp_tee *tee = (struct mp_tee *)obj;

	switch (id) {
	case PROP_TEE_SRCPADS_NUM: {
		uint8_t requested = *(const uint8_t *)val;

		if (!IN_RANGE(requested, DEFAULT_SRCPADS_NUM, CONFIG_MP_TEE_MAX_SRCPADS_NUM)) {
			return -EINVAL;
		}

		while (tee->srcpads_num < requested) {
			mp_tee_add_srcpad(tee);
		}

		return 0;
	}
	default:
		return -ENOTSUP;
	}
}

void mp_tee_init(struct mp_element *self)
{
	struct mp_tee *tee = (struct mp_tee *)self;

	self->object.get_property = mp_tee_get_property;
	self->object.set_property = mp_tee_set_property;

	tee->sinkpad.chainfn = mp_tee_chainfn;
	tee->sinkpad.queryfn = mp_tee_sink_queryfn;
	tee->sinkpad.eventfn = mp_tee_sink_eventfn;
	tee->caps = mp_caps_new_any();

	/* Initialize the sink pad */
	mp_pad_init(&tee->sinkpad, 0, MP_PAD_SINK, MP_PAD_ALWAYS, tee->caps);
	mp_element_add_pad(self, &tee->sinkpad);

	/* Initialize the default source pads */
	tee->srcpads_num = 0;
	while (tee->srcpads_num < DEFAULT_SRCPADS_NUM) {
		mp_tee_add_srcpad(tee);
	}
}
