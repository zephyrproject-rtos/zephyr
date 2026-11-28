/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>

#include <zephyr/mpipe/nn/mpipe_nn_runner.h>

LOG_MODULE_REGISTER(mpipe_nn_runner, CONFIG_MPIPE_LOG_LEVEL);

/* Output pool: one flat buffer per inference, holding every output tensor */
NET_BUF_POOL_FIXED_DEFINE(mpipe_nn_runner_pool, CONFIG_MPIPE_NN_RUNNER_POOL_NUM,
			  CONFIG_MPIPE_NN_RUNNER_MAX_OUT_SIZE, sizeof(struct mpipe_buffer_meta),
			  mpipe_buffer_destroy);

/*
 * Load the model and read its tensor sets, once. Called on READY to PAUSED,
 * which the bin walks sink to source, so before the source negotiates
 */
static int nn_runner_ensure_loaded(struct mpipe_nn_runner *runner)
{
	int ret;

	if (runner->loaded) {
		return 0;
	}

	if (runner->backend.ops == NULL) {
		ret = mpipe_nn_backend_default_init(&runner->backend);
		if (ret != 0) {
			LOG_ERR("No inference backend available (%d)", ret);
			return ret;
		}
	}

	if (runner->model.data == NULL || runner->arena.data == NULL) {
		LOG_ERR("Model or arena not set; set MPIPE_PROP_NN_MODEL and MPIPE_PROP_NN_ARENA");
		return -ENODEV;
	}

	ret = runner->backend.ops->load(&runner->backend, &runner->model, &runner->arena);
	if (ret != 0) {
		LOG_ERR("Failed to load model (%d)", ret);
		return ret;
	}

	ret = runner->backend.ops->get_io(&runner->backend, &runner->in_set, &runner->out_set);
	if (ret != 0) {
		LOG_ERR("Failed to describe model I/O (%d)", ret);
		(void)runner->backend.ops->unload(&runner->backend);
		return ret;
	}

	if (runner->in_set.count != 1U) {
		LOG_ERR("Only single-input models are supported (%u inputs)", runner->in_set.count);
		(void)runner->backend.ops->unload(&runner->backend);
		return -ENOTSUP;
	}

	if (runner->out_set.total_size > CONFIG_MPIPE_NN_RUNNER_MAX_OUT_SIZE) {
		LOG_ERR("Model output %u exceeds CONFIG_MPIPE_NN_RUNNER_MAX_OUT_SIZE=%u",
			runner->out_set.total_size, CONFIG_MPIPE_NN_RUNNER_MAX_OUT_SIZE);
		(void)runner->backend.ops->unload(&runner->backend);
		return -ENOMEM;
	}

	runner->loaded = true;

	return 0;
}

/* Build the capability of the model's single input tensor */
static int nn_runner_input_candidate(struct mpipe_nn_runner *runner, struct mpipe_structure *out)
{
	const struct mpipe_nn_tensor_desc *in = &runner->in_set.desc[0];
	struct mpipe_value v;
	int ret;

	ret = mpipe_structure_init(out, MPIPE_MEDIA_TENSOR);
	if (ret != 0) {
		return ret;
	}

	v.type = MPIPE_TYPE_UINT;
	v.v_uint = in->type;
	ret = mpipe_structure_append_value(out, MPIPE_CAPS_TENSOR_TYPE, &v);
	if (ret != 0) {
		goto err;
	}

	/* An image-shaped input: NHWC with N == 1 */
	if (in->num_dims == 4U) {
		v.type = MPIPE_TYPE_UINT;
		v.v_uint = in->dims[3];
		ret = mpipe_structure_append_value(out, MPIPE_CAPS_NUM_CHANNELS, &v);
		if (ret != 0) {
			goto err;
		}

		v.type = MPIPE_TYPE_UINT;
		v.v_uint = in->dims[2];
		ret = mpipe_structure_append_value(out, MPIPE_CAPS_IMAGE_WIDTH, &v);
		if (ret != 0) {
			goto err;
		}

		v.type = MPIPE_TYPE_UINT;
		v.v_uint = in->dims[1];
		ret = mpipe_structure_append_value(out, MPIPE_CAPS_IMAGE_HEIGHT, &v);
		if (ret != 0) {
			goto err;
		}
	}

	return 0;
err:
	mpipe_structure_clear(out);

	return ret;
}

static int nn_runner_enum_caps(struct mpipe_pad *pad, uint32_t index,
			       const struct mpipe_structure *filter, struct mpipe_structure *out)
{
	struct mpipe_nn_runner *runner = (struct mpipe_nn_runner *)pad->object.container;
	struct mpipe_structure candidate;
	int ret;

	/* A fixed model has exactly one input capability */
	if (index > 0U) {
		return -ENOENT;
	}

	ret = nn_runner_ensure_loaded(runner);
	if (ret != 0) {
		return ret;
	}

	ret = nn_runner_input_candidate(runner, &candidate);
	if (ret != 0) {
		return ret;
	}

	return mpipe_pad_enum_filter(&candidate, filter, out);
}

static int nn_runner_transform_caps(struct mpipe_transform *self,
				    enum mpipe_pad_direction direction,
				    const struct mpipe_structure *in, uint32_t index,
				    struct mpipe_structure *out)
{
	struct mpipe_nn_runner *runner = (struct mpipe_nn_runner *)self;
	struct mpipe_value v;
	int ret;

	if (in == NULL) {
		return -EINVAL;
	}

	/* The model fixes both sides: one transformation, whatever comes in */
	if (index > 0U) {
		return -ENOENT;
	}

	ret = nn_runner_ensure_loaded(runner);
	if (ret != 0) {
		return ret;
	}

	if (direction == MPIPE_PAD_SRC) {
		ret = mpipe_structure_init(out, MPIPE_MEDIA_TENSOR);
		if (ret != 0) {
			return ret;
		}

		v.type = MPIPE_TYPE_UINT;
		v.v_uint = runner->out_set.desc[0].type;
		ret = mpipe_structure_append_value(out, MPIPE_CAPS_TENSOR_TYPE, &v);
		if (ret != 0) {
			mpipe_structure_clear(out);
			return ret;
		}
	} else {
		ret = nn_runner_input_candidate(runner, out);
		if (ret != 0) {
			return ret;
		}
	}

	/* Inference changes the tensors, not the rate; an input without one is fine */
	if (!mpipe_structure_is_any(in)) {
		ret = mpipe_structure_copy_field(in, out, MPIPE_CAPS_FRAME_INTERVAL);
		if (ret != 0 && ret != -ENOENT) {
			mpipe_structure_clear(out);
			return ret;
		}
	}

	return 0;
}

static int nn_runner_set_caps(struct mpipe_transform *transform, enum mpipe_pad_direction direction,
			      const struct mpipe_structure *caps)
{
	struct mpipe_nn_runner *runner = (struct mpipe_nn_runner *)transform;
	int ret;

	if (caps == NULL) {
		return -EINVAL;
	}

	ret = mpipe_transform_set_caps(transform, direction, caps);
	if (ret < 0) {
		return ret;
	}

	if (direction == MPIPE_PAD_SRC && runner->loaded) {
		runner->out_pool.config.size = runner->out_set.total_size;
	}

	return 0;
}

static int nn_runner_decide_buffer_pool(struct mpipe_transform *self, struct mpipe_dispatch *query)
{
	struct mpipe_nn_runner *runner = (struct mpipe_nn_runner *)self;

	ARG_UNUSED(query);

	/*
	 * Always the internal pool, ignoring downstream proposals: every
	 * produced buffer carries a pointer to this element's output tensor
	 * set in its metadata, a contract only buffers of this pool honor.
	 */
	self->out_pool = &runner->out_pool;

	return 0;
}

static int nn_runner_process_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
				struct net_buf **out_buf)
{
	struct mpipe_transform *transform = (struct mpipe_transform *)pad->object.container;
	struct mpipe_nn_runner *runner = (struct mpipe_nn_runner *)transform;
	struct mpipe_buffer_pool *out_pool = transform->out_pool;
	struct net_buf *out = NULL;
	struct mpipe_buffer_meta *meta;
	int ret;

	*out_buf = NULL;

	if (!runner->loaded) {
		LOG_ERR("No model loaded");
		net_buf_unref(in_buf);
		return -EINVAL;
	}

	if (mpipe_buffer_get_meta(in_buf)->bytes_used < runner->in_set.desc[0].size) {
		LOG_ERR("Input tensor too small: %u < %u",
			mpipe_buffer_get_meta(in_buf)->bytes_used, runner->in_set.desc[0].size);
		goto err;
	}

	if (out_pool->acquire_buffer(out_pool, &out) != 0 || out == NULL) {
		LOG_DBG("No free output buffer, inference dropped");
		net_buf_unref(in_buf);
		return 0;
	}

	ret = runner->backend.ops->invoke(&runner->backend, in_buf->data,
					  runner->in_set.desc[0].size, out->data,
					  runner->out_set.total_size);
	if (ret != 0) {
		LOG_ERR("Inference failed (%d)", ret);
		net_buf_unref(out);
		goto err;
	}

	meta = mpipe_buffer_get_meta(out);
	meta->bytes_used = runner->out_set.total_size;
	meta->pts = mpipe_buffer_get_meta(in_buf)->pts;
	meta->priv = &runner->out_set;
	out->len = runner->out_set.total_size;

	net_buf_unref(in_buf);
	*out_buf = out;

	return 0;
err:
	net_buf_unref(in_buf);

	return -EIO;
}

static int nn_runner_change_state(struct mpipe_element *element, enum mpipe_state_change transition)
{
	struct mpipe_nn_runner *runner = (struct mpipe_nn_runner *)element;
	int ret;

	switch (transition) {
	case MPIPE_STATE_CHANGE_READY_TO_PAUSED:
		ret = nn_runner_ensure_loaded(runner);
		if (ret != 0) {
			return ret;
		}
		break;
	default:
		/* The model stays loaded across replay; only caps reset */
		break;
	}

	return mpipe_transform_change_state(element, transition);
}

static int nn_runner_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe_nn_runner *runner = (struct mpipe_nn_runner *)obj;

	if (val == NULL) {
		return -EINVAL;
	}

	switch (key) {
	case MPIPE_PROP_NN_MODEL:
		runner->model = *(const struct mpipe_nn_model *)val;
		break;
	case MPIPE_PROP_NN_ARENA:
		runner->arena = *(const struct mpipe_nn_arena *)val;
		break;
	default:
		return -ENOTSUP;
	}

	/* A new model or arena invalidates whatever was loaded */
	mpipe_nn_runner_unload(runner);

	return 0;
}

static int nn_runner_out_pool_acquire(struct mpipe_buffer_pool *pool, struct net_buf **buf)
{
	struct net_buf *out;
	struct mpipe_buffer_meta *m;

	__ASSERT_NO_MSG(pool != NULL);
	__ASSERT_NO_MSG(buf != NULL);

	out = net_buf_alloc_len(pool->nb_pool, pool->config.size, K_NO_WAIT);
	if (out == NULL) {
		return -ENOBUFS;
	}

	m = mpipe_buffer_get_meta(out);
	m->pool = pool;
	m->bytes_used = 0;
	m->pts = 0;
	m->driver_buf = NULL;
	m->priv = NULL;
	out->len = 0;

	*buf = out;

	return 0;
}

static int nn_runner_out_pool_release(struct mpipe_buffer_pool *pool, struct net_buf *buf)
{
	ARG_UNUSED(pool);

	__ASSERT_NO_MSG(buf != NULL);

	struct mpipe_buffer_meta *m = mpipe_buffer_get_meta(buf);

	m->bytes_used = 0;
	m->pts = 0;
	m->driver_buf = NULL;
	m->priv = NULL;
	buf->len = 0;

	return 0;
}

void mpipe_nn_runner_unload(struct mpipe_nn_runner *runner)
{
	__ASSERT_NO_MSG(runner != NULL);

	if (runner->loaded) {
		(void)runner->backend.ops->unload(&runner->backend);
		runner->loaded = false;
	}
}

int mpipe_nn_runner_init(struct mpipe_nn_runner *runner, uint8_t id)
{
	__ASSERT_NO_MSG(runner != NULL);

	struct mpipe_element *self = &runner->transform.element;
	struct mpipe_transform *transform = &runner->transform;
	int ret = mpipe_transform_init(transform, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "nn_runner");

	self->object.set_property = nn_runner_set_property;
	self->change_state = nn_runner_change_state;

	/* The src pad keeps the default: the output tensor type comes from transform_caps */
	transform->sink_pad.enum_caps_fn = nn_runner_enum_caps;

	runner->backend.ops = NULL;
	runner->backend.priv = NULL;
	runner->model.data = NULL;
	runner->model.size = 0;
	runner->arena.data = NULL;
	runner->arena.size = 0;
	runner->loaded = false;
	memset(&runner->in_set, 0, sizeof(runner->in_set));
	memset(&runner->out_set, 0, sizeof(runner->out_set));

	/* The net_buf pool is static: no start hook, the size follows the model */
	const struct mpipe_buffer_pool_config pool_req = {
		.size = 0,
		.align = 1,
		.min_buffers = CONFIG_MPIPE_NN_RUNNER_POOL_NUM,
		.max_buffers = CONFIG_MPIPE_NN_RUNNER_POOL_NUM,
	};

	mpipe_buffer_pool_init(&runner->out_pool);
	runner->out_pool.nb_pool = &mpipe_nn_runner_pool;
	(void)mpipe_buffer_pool_set_req_config(&runner->out_pool, &pool_req);
	runner->out_pool.acquire_buffer = nn_runner_out_pool_acquire;
	runner->out_pool.release_buffer = nn_runner_out_pool_release;

	transform->mode = MPIPE_TRANSFORM_MODE_NORMAL;
	transform->out_pool = &runner->out_pool;
	transform->set_caps = nn_runner_set_caps;
	transform->transform_caps = nn_runner_transform_caps;
	transform->propose_buffer_pool = NULL;
	transform->decide_buffer_pool = nn_runner_decide_buffer_pool;
	transform->sink_pad.process_fn = nn_runner_process_fn;

	return 0;
}
