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

#include <zephyr/mpipe/nn/mpipe_nn_backend.h>
#include <zephyr/mpipe/nn/mpipe_nn_decoder.h>

LOG_MODULE_REGISTER(mpipe_nn_decoder, CONFIG_MPIPE_LOG_LEVEL);

/* Each produced buffer holds exactly one results structure */
NET_BUF_POOL_FIXED_DEFINE(mpipe_nn_decoder_pool, CONFIG_MPIPE_NN_DECODER_POOL_NUM,
			  sizeof(struct mpipe_nn_results), sizeof(struct mpipe_buffer_meta),
			  mpipe_buffer_destroy);

/* Tensor element data types the decoder can interpret, one per enum index */
static const uint8_t nn_decoder_types[] = {
	MPIPE_NN_TENSOR_TYPE_INT8,
	MPIPE_NN_TENSOR_TYPE_UINT8,
	MPIPE_NN_TENSOR_TYPE_FLOAT32,
};

static int nn_decoder_tensor_candidate(uint32_t index, struct mpipe_structure *out)
{
	struct mpipe_value v;
	int ret;

	if (index >= ARRAY_SIZE(nn_decoder_types)) {
		return -ENOENT;
	}

	ret = mpipe_structure_init(out, MPIPE_MEDIA_TENSOR);
	if (ret != 0) {
		return ret;
	}

	v.type = MPIPE_TYPE_UINT;
	v.v_uint = nn_decoder_types[index];
	ret = mpipe_structure_append_value(out, MPIPE_CAPS_TENSOR_TYPE, &v);
	if (ret != 0) {
		mpipe_structure_clear(out);
	}

	return ret;
}

static int nn_decoder_results_candidate(const struct mpipe_nn_decoder *decode,
					struct mpipe_structure *out)
{
	struct mpipe_value v;
	int ret;

	ret = mpipe_structure_init(out, MPIPE_MEDIA_NN_RESULTS);
	if (ret != 0) {
		return ret;
	}

	v.type = MPIPE_TYPE_UINT;
	v.v_uint = decode->decode_type;
	ret = mpipe_structure_append_value(out, MPIPE_CAPS_RESULTS_TYPE, &v);
	if (ret != 0) {
		mpipe_structure_clear(out);
	}

	return ret;
}

static int nn_decoder_enum_caps(struct mpipe_pad *pad, uint32_t index,
				const struct mpipe_structure *filter, struct mpipe_structure *out)
{
	struct mpipe_nn_decoder *decode = (struct mpipe_nn_decoder *)pad->object.container;
	struct mpipe_structure candidate;
	int ret;

	if (pad->direction == MPIPE_PAD_SINK) {
		ret = nn_decoder_tensor_candidate(index, &candidate);
	} else {
		if (index > 0U) {
			return -ENOENT;
		}

		ret = nn_decoder_results_candidate(decode, &candidate);
	}

	if (ret != 0) {
		return ret;
	}

	return mpipe_pad_enum_filter(&candidate, filter, out);
}

static int nn_decoder_transform_caps(struct mpipe_transform *self,
				     enum mpipe_pad_direction direction,
				     const struct mpipe_structure *in, uint32_t index,
				     struct mpipe_structure *out)
{
	struct mpipe_nn_decoder *decode = (struct mpipe_nn_decoder *)self;
	int ret;

	if (in == NULL) {
		return -EINVAL;
	}

	if (direction == MPIPE_PAD_SRC) {
		/* Whatever tensor comes in, one results stream comes out */
		if (index > 0U) {
			return -ENOENT;
		}

		ret = nn_decoder_results_candidate(decode, out);
	} else {
		/* Toward the tensor side, each supported data type is reachable */
		ret = nn_decoder_tensor_candidate(index, out);
	}

	if (ret != 0) {
		return ret;
	}

	/* Decoding changes the payload, not the rate; an input without one is fine */
	if (!mpipe_structure_is_any(in)) {
		ret = mpipe_structure_copy_field(in, out, MPIPE_CAPS_FRAME_INTERVAL);
		if (ret != 0 && ret != -ENOENT) {
			mpipe_structure_clear(out);
			return ret;
		}
	}

	return 0;
}

/* Number of elements of a tensor, from its dimensions */
static uint32_t nn_decoder_num_elements(const struct mpipe_nn_tensor_desc *desc)
{
	uint32_t n = 1;

	for (uint8_t i = 0; i < desc->num_dims; i++) {
		n *= desc->dims[i];
	}

	return n;
}

/* Dequantized value of element @p i of a tensor */
static float nn_decoder_value_at(const struct mpipe_nn_tensor_desc *desc, const uint8_t *data,
				 uint32_t i)
{
	float q;

	switch (desc->type) {
	case MPIPE_NN_TENSOR_TYPE_INT8:
		q = (float)((const int8_t *)data)[i];
		break;
	case MPIPE_NN_TENSOR_TYPE_UINT8:
		q = (float)data[i];
		break;
	case MPIPE_NN_TENSOR_TYPE_FLOAT32:
		return ((const float *)(const void *)data)[i];
	default:
		return 0.0f;
	}

	if (desc->scale != 0.0f) {
		return desc->scale * (q - (float)desc->zero_point);
	}

	return q;
}

/*
 * Classification: dequantize the scores of output tensor 0 and keep the best
 * ones, sorted by descending score.
 */
static void nn_decoder_classification(const struct mpipe_nn_tensor_set *set, const uint8_t *data,
				      struct mpipe_nn_results *res)
{
	const struct mpipe_nn_tensor_desc *desc = &set->desc[0];
	const uint8_t *scores = &data[desc->offset];
	uint32_t num = nn_decoder_num_elements(desc);

	res->type = MPIPE_NN_RESULTS_CLASSIFICATION;
	res->count = 0;

	for (uint32_t i = 0; i < num; i++) {
		float score = nn_decoder_value_at(desc, scores, i);
		uint8_t pos = res->count;

		/* Insertion into the fixed-size ranking */
		while (pos > 0U && res->cls[pos - 1U].score < score) {
			if (pos < CONFIG_MPIPE_NN_RESULTS_MAX) {
				res->cls[pos] = res->cls[pos - 1U];
			}
			pos--;
		}

		if (pos < CONFIG_MPIPE_NN_RESULTS_MAX) {
			res->cls[pos].class_id = (uint16_t)i;
			res->cls[pos].score = score;

			if (res->count < CONFIG_MPIPE_NN_RESULTS_MAX) {
				res->count++;
			}
		}
	}
}

/* Intersection over union of two normalized boxes */
static float nn_decoder_iou(const struct mpipe_nn_box *a, const struct mpipe_nn_box *b)
{
	float iw = MIN(a->xmax, b->xmax) - MAX(a->xmin, b->xmin);
	float ih = MIN(a->ymax, b->ymax) - MAX(a->ymin, b->ymin);
	float inter;
	float uni;

	if (iw <= 0.0f || ih <= 0.0f) {
		return 0.0f;
	}

	inter = iw * ih;
	uni = (a->xmax - a->xmin) * (a->ymax - a->ymin) +
	      (b->xmax - b->xmin) * (b->ymax - b->ymin) - inter;

	return (uni > 0.0f) ? inter / uni : 0.0f;
}

/* Scored box list layout (see mpipe_nn_decoder.h): rank, then suppress overlaps */
static void nn_decoder_box_list(struct mpipe_nn_decoder *decode,
				const struct mpipe_nn_tensor_desc *desc, const uint8_t *data,
				struct mpipe_nn_results *res)
{
	const uint8_t *rows = &data[desc->offset];
	uint32_t cols;
	uint32_t num_rows;
	uint32_t first_class;
	uint8_t num_cand = 0;

	if (desc->num_dims < 2U || desc->dims[desc->num_dims - 1U] < 5U) {
		LOG_ERR("Unexpected box list layout");
		return;
	}

	cols = desc->dims[desc->num_dims - 1U];
	num_rows = nn_decoder_num_elements(desc) / cols;
	first_class = (cols > 5U) ? 1U : 0U;

	for (uint32_t r = 0; r < num_rows; r++) {
		struct mpipe_nn_box box = {.score = -1.0f};
		uint8_t pos = num_cand;

		for (uint32_t c = first_class; c < cols - 4U; c++) {
			float score = nn_decoder_value_at(desc, rows, r * cols + c);

			if (score > box.score) {
				box.score = score;
				box.class_id = (uint16_t)c;
			}
		}

		if (box.score < decode->threshold) {
			continue;
		}

		box.xmin = nn_decoder_value_at(desc, rows, r * cols + cols - 4U);
		box.ymin = nn_decoder_value_at(desc, rows, r * cols + cols - 3U);
		box.xmax = nn_decoder_value_at(desc, rows, r * cols + cols - 2U);
		box.ymax = nn_decoder_value_at(desc, rows, r * cols + cols - 1U);

		/* Insertion into the fixed-size ranking */
		while (pos > 0U && decode->cand[pos - 1U].score < box.score) {
			if (pos < CONFIG_MPIPE_NN_DECODER_CANDIDATES_MAX) {
				decode->cand[pos] = decode->cand[pos - 1U];
			}
			pos--;
		}

		if (pos < CONFIG_MPIPE_NN_DECODER_CANDIDATES_MAX) {
			decode->cand[pos] = box;

			if (num_cand < CONFIG_MPIPE_NN_DECODER_CANDIDATES_MAX) {
				num_cand++;
			}
		}
	}

	for (uint8_t i = 0; i < num_cand && res->count < CONFIG_MPIPE_NN_RESULTS_MAX; i++) {
		bool keep = true;

		for (uint8_t j = 0; j < res->count; j++) {
			if (nn_decoder_iou(&decode->cand[i], &res->boxes[j]) > decode->nms_iou) {
				keep = false;
				break;
			}
		}

		if (keep) {
			res->boxes[res->count] = decode->cand[i];
			res->count++;
		}
	}
}

/*
 * Detection: a single output tensor is a scored box list, four are the
 * TFLite_Detection_PostProcess layout (see mpipe_nn_decoder.h), checked
 * against the descriptors because output ordering varies between models
 */
static void nn_decoder_detection(struct mpipe_nn_decoder *decode,
				 const struct mpipe_nn_tensor_set *set, const uint8_t *data,
				 struct mpipe_nn_results *res)
{
	const struct mpipe_nn_tensor_desc *boxes;
	const struct mpipe_nn_tensor_desc *classes;
	const struct mpipe_nn_tensor_desc *scores;
	const struct mpipe_nn_tensor_desc *count;
	uint32_t n;

	res->type = MPIPE_NN_RESULTS_DETECTION;
	res->count = 0;

	if (set->count == 1U) {
		nn_decoder_box_list(decode, &set->desc[0], data, res);
		return;
	}

	if (set->count < 4U) {
		LOG_ERR("Detection needs 4 output tensors, model has %u", set->count);
		return;
	}

	boxes = &set->desc[0];
	classes = &set->desc[1];
	scores = &set->desc[2];
	count = &set->desc[3];

	if (boxes->num_dims < 2U || boxes->dims[boxes->num_dims - 1U] != 4U ||
	    nn_decoder_num_elements(count) != 1U) {
		LOG_ERR("Unexpected detection output layout");
		return;
	}

	n = (uint32_t)nn_decoder_value_at(count, &data[count->offset], 0);
	n = MIN(n, nn_decoder_num_elements(scores));
	n = MIN(n, nn_decoder_num_elements(classes));
	n = MIN(n, nn_decoder_num_elements(boxes) / 4U);

	for (uint32_t i = 0; i < n; i++) {
		float score = nn_decoder_value_at(scores, &data[scores->offset], i);
		struct mpipe_nn_box *box;

		if (score < decode->threshold || res->count >= CONFIG_MPIPE_NN_RESULTS_MAX) {
			continue;
		}

		box = &res->boxes[res->count];
		box->ymin = nn_decoder_value_at(boxes, &data[boxes->offset], 4U * i);
		box->xmin = nn_decoder_value_at(boxes, &data[boxes->offset], 4U * i + 1U);
		box->ymax = nn_decoder_value_at(boxes, &data[boxes->offset], 4U * i + 2U);
		box->xmax = nn_decoder_value_at(boxes, &data[boxes->offset], 4U * i + 3U);
		box->score = score;
		box->class_id = (uint16_t)nn_decoder_value_at(classes, &data[classes->offset], i);
		res->count++;
	}
}

static int nn_decoder_process_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
				 struct net_buf **out_buf)
{
	struct mpipe_transform *transform = (struct mpipe_transform *)pad->object.container;
	struct mpipe_nn_decoder *decode = (struct mpipe_nn_decoder *)transform;
	struct mpipe_buffer_pool *out_pool = transform->out_pool;
	const struct mpipe_nn_tensor_set *set = mpipe_buffer_get_meta(in_buf)->priv;
	struct mpipe_nn_results *res;
	struct net_buf *out = NULL;
	struct mpipe_buffer_meta *meta;

	*out_buf = NULL;

	if (set == NULL || set->count == 0U) {
		LOG_ERR("Buffer carries no tensor set description");
		goto err;
	}

	if (mpipe_buffer_get_meta(in_buf)->bytes_used < set->total_size) {
		LOG_ERR("Buffer smaller than its tensor set: %u < %u",
			mpipe_buffer_get_meta(in_buf)->bytes_used, set->total_size);
		goto err;
	}

	if (out_pool->acquire_buffer(out_pool, &out) != 0 || out == NULL) {
		LOG_DBG("No free results buffer, inference dropped");
		net_buf_unref(in_buf);
		return 0;
	}

	res = (struct mpipe_nn_results *)out->data;
	memset(res, 0, sizeof(*res));
	decode->seq++;
	res->seq = decode->seq;
	res->pts = mpipe_buffer_get_meta(in_buf)->pts;

	switch (decode->decode_type) {
	case MPIPE_NN_RESULTS_CLASSIFICATION:
		nn_decoder_classification(set, in_buf->data, res);
		break;
	case MPIPE_NN_RESULTS_DETECTION:
		nn_decoder_detection(decode, set, in_buf->data, res);
		break;
	default:
		LOG_ERR("Unsupported decode type %u", decode->decode_type);
		net_buf_unref(out);
		goto err;
	}

	out->len = sizeof(*res);

	meta = mpipe_buffer_get_meta(out);
	meta->bytes_used = sizeof(*res);
	meta->pts = res->pts;

	net_buf_unref(in_buf);
	*out_buf = out;

	return 0;
err:
	net_buf_unref(in_buf);

	return -EIO;
}

static int nn_decoder_decide_buffer_pool(struct mpipe_transform *self, struct mpipe_dispatch *query)
{
	struct mpipe_nn_decoder *decode = (struct mpipe_nn_decoder *)self;

	ARG_UNUSED(query);

	/* Results buffers are fixed-size; the internal pool always fits */
	self->out_pool = &decode->out_pool;

	return 0;
}

static int nn_decoder_change_state(struct mpipe_element *element,
				   enum mpipe_state_change transition)
{
	struct mpipe_nn_decoder *decode = (struct mpipe_nn_decoder *)element;

	/* A replay is a new stream: its results are numbered from one again */
	if (transition == MPIPE_STATE_CHANGE_READY_TO_PAUSED) {
		decode->seq = 0;
	}

	return mpipe_transform_change_state(element, transition);
}

static int nn_decoder_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe_nn_decoder *decode = (struct mpipe_nn_decoder *)obj;

	if (val == NULL) {
		return -EINVAL;
	}

	switch (key) {
	case MPIPE_PROP_NN_DECODER_TYPE:
		decode->decode_type = *(const uint8_t *)val;
		return 0;
	case MPIPE_PROP_NN_DECODER_THRESHOLD:
		decode->threshold = *(const float *)val;
		return 0;
	case MPIPE_PROP_NN_DECODER_NMS_IOU:
		decode->nms_iou = *(const float *)val;
		return 0;
	default:
		LOG_ERR("Property %u is unknown", key);
		return -ENOTSUP;
	}
}

static int nn_decoder_out_pool_acquire(struct mpipe_buffer_pool *pool, struct net_buf **buf)
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

static int nn_decoder_out_pool_release(struct mpipe_buffer_pool *pool, struct net_buf *buf)
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

int mpipe_nn_decoder_init(struct mpipe_nn_decoder *decode, uint8_t id)
{
	__ASSERT_NO_MSG(decode != NULL);

	struct mpipe_element *self = &decode->transform.element;
	struct mpipe_transform *transform = &decode->transform;
	int ret = mpipe_transform_init(transform, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "nn_decoder");

	self->object.set_property = nn_decoder_set_property;
	self->change_state = nn_decoder_change_state;

	transform->sink_pad.enum_caps_fn = nn_decoder_enum_caps;
	transform->src_pad.enum_caps_fn = nn_decoder_enum_caps;

	decode->decode_type = MPIPE_NN_RESULTS_CLASSIFICATION;
	decode->threshold = 0.5f;
	decode->nms_iou = 0.5f;
	decode->seq = 0;

	/* The net_buf pool is static: no start hook, one results structure per buffer */
	const struct mpipe_buffer_pool_config pool_req = {
		.size = sizeof(struct mpipe_nn_results),
		.align = 1,
		.min_buffers = CONFIG_MPIPE_NN_DECODER_POOL_NUM,
		.max_buffers = CONFIG_MPIPE_NN_DECODER_POOL_NUM,
	};

	mpipe_buffer_pool_init(&decode->out_pool);
	decode->out_pool.nb_pool = &mpipe_nn_decoder_pool;
	(void)mpipe_buffer_pool_set_req_config(&decode->out_pool, &pool_req);
	decode->out_pool.acquire_buffer = nn_decoder_out_pool_acquire;
	decode->out_pool.release_buffer = nn_decoder_out_pool_release;

	transform->mode = MPIPE_TRANSFORM_MODE_NORMAL;
	transform->out_pool = &decode->out_pool;
	transform->transform_caps = nn_decoder_transform_caps;
	transform->propose_buffer_pool = NULL;
	transform->decide_buffer_pool = nn_decoder_decide_buffer_pool;
	transform->sink_pad.process_fn = nn_decoder_process_fn;

	return 0;
}
