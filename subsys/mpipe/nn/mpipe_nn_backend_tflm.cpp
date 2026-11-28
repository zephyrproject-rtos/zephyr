/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * TensorFlow Lite Micro backend: the subsystem's only C++ unit, everything
 * crosses the C mpipe_nn_backend vtable
 */

#include <errno.h>
#include <new>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#include <zephyr/mpipe/nn/mpipe_nn_backend.h>

#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/schema/schema_generated.h>
#ifdef CONFIG_TFLM_NXP_NEUTRON
#include <tensorflow/lite/micro/kernels/neutron/neutron.h>
#endif

LOG_MODULE_REGISTER(mpipe_nn_backend_tflm, CONFIG_MPIPE_LOG_LEVEL);

namespace
{

using Resolver = tflite::MicroMutableOpResolver<CONFIG_MPIPE_NN_TFLM_MAX_OPS>;

/* Alignment of each tensor in the flat output buffer */
constexpr uint32_t tensor_align = 4;

/* Instances live in static storage, placement-constructed at load: no heap, no constructors */
struct tflm_instance {
	bool used;
	const tflite::Model *model;
	Resolver *resolver;
	tflite::MicroInterpreter *interpreter;
	alignas(Resolver) unsigned char resolver_storage[sizeof(Resolver)];
	alignas(tflite::MicroInterpreter) unsigned char interpreter_storage[sizeof(
		tflite::MicroInterpreter)];
};

tflm_instance instances[CONFIG_MPIPE_NN_TFLM_MAX_INSTANCES];
struct k_spinlock instances_lock;

/* The operators the resolver knows; a model needing another one fails to load */
bool tflm_register_ops(Resolver *resolver)
{
	bool ok = true;

	ok = ok && (resolver->AddAveragePool2D() == kTfLiteOk);
	ok = ok && (resolver->AddConv2D() == kTfLiteOk);
	ok = ok && (resolver->AddDepthwiseConv2D() == kTfLiteOk);
	ok = ok && (resolver->AddFullyConnected() == kTfLiteOk);
	ok = ok && (resolver->AddMaxPool2D() == kTfLiteOk);
	ok = ok && (resolver->AddReshape() == kTfLiteOk);
	ok = ok && (resolver->AddSoftmax() == kTfLiteOk);
	ok = ok && (resolver->AddAdd() == kTfLiteOk);
	ok = ok && (resolver->AddMul() == kTfLiteOk);
	ok = ok && (resolver->AddMean() == kTfLiteOk);
	ok = ok && (resolver->AddLogistic() == kTfLiteOk);
	ok = ok && (resolver->AddDequantize() == kTfLiteOk);
	ok = ok && (resolver->AddQuantize() == kTfLiteOk);
	ok = ok && (resolver->AddConcatenation() == kTfLiteOk);
	ok = ok && (resolver->AddPad() == kTfLiteOk);
	ok = ok && (resolver->AddSub() == kTfLiteOk);
	ok = ok && (resolver->AddExp() == kTfLiteOk);
	ok = ok && (resolver->AddMinimum() == kTfLiteOk);
	ok = ok && (resolver->AddRelu() == kTfLiteOk);
	ok = ok && (resolver->AddStridedSlice() == kTfLiteOk);
	ok = ok && (resolver->AddSlice() == kTfLiteOk);
	ok = ok && (resolver->AddShape() == kTfLiteOk);
	ok = ok && (resolver->AddPack() == kTfLiteOk);
	/* The fused post-processing of SSD detectors */
	ok = ok && (resolver->AddDetectionPostprocess() == kTfLiteOk);

	return ok;
}

/*
 * The custom operator each supported NPU's compiler emits, one block per
 * vendor. Layers the compiler left untouched, or a whole model it never saw,
 * run on the CPU kernels above without notice.
 */
bool tflm_register_npu_ops(Resolver *resolver)
{
	bool ok = true;

#ifdef CONFIG_ETHOS_U
	ok = ok && (resolver->AddEthosU() == kTfLiteOk);
#endif
#ifdef CONFIG_TFLM_NXP_NEUTRON
	ok = ok && (resolver->AddCustom(tflite::GetString_NEUTRON_GRAPH(),
					tflite::Register_NEUTRON_GRAPH()) == kTfLiteOk);
#endif

	return ok;
}

uint8_t tflm_type(TfLiteType type)
{
	switch (type) {
	case kTfLiteInt8:
		return MPIPE_NN_TENSOR_TYPE_INT8;
	case kTfLiteUInt8:
		return MPIPE_NN_TENSOR_TYPE_UINT8;
	case kTfLiteInt16:
		return MPIPE_NN_TENSOR_TYPE_INT16;
	case kTfLiteFloat32:
		return MPIPE_NN_TENSOR_TYPE_FLOAT32;
	default:
		return MPIPE_NN_TENSOR_TYPE_UNKNOWN;
	}
}

int tflm_describe(const TfLiteTensor *tensor, struct mpipe_nn_tensor_desc *desc, uint32_t *offset)
{
	if (tensor == nullptr) {
		return -EINVAL;
	}

	desc->type = tflm_type(tensor->type);
	if (desc->type == MPIPE_NN_TENSOR_TYPE_UNKNOWN) {
		LOG_ERR("Unsupported tensor element type %d", tensor->type);
		return -ENOTSUP;
	}

	if (tensor->dims->size > MPIPE_NN_TENSOR_MAX_DIMS) {
		LOG_ERR("Tensor rank %d exceeds %d", tensor->dims->size, MPIPE_NN_TENSOR_MAX_DIMS);
		return -ENOTSUP;
	}

	desc->num_dims = (uint8_t)tensor->dims->size;
	for (int i = 0; i < tensor->dims->size; i++) {
		desc->dims[i] = (uint16_t)tensor->dims->data[i];
	}

	desc->size = tensor->bytes;
	desc->offset = ROUND_UP(*offset, tensor_align);
	desc->scale = tensor->params.scale;
	desc->zero_point = tensor->params.zero_point;

	*offset = desc->offset + desc->size;

	return 0;
}

int tflm_load(struct mpipe_nn_backend *be, const struct mpipe_nn_model *model,
	      const struct mpipe_nn_arena *arena)
{
	tflm_instance *inst = nullptr;

	if (model == nullptr || model->data == nullptr || arena == nullptr ||
	    arena->data == nullptr) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&instances_lock);

	for (size_t i = 0; i < ARRAY_SIZE(instances); i++) {
		if (!instances[i].used) {
			inst = &instances[i];
			inst->used = true;
			break;
		}
	}

	k_spin_unlock(&instances_lock, key);

	if (inst == nullptr) {
		LOG_ERR("No free TFLM instance; raise CONFIG_MPIPE_NN_TFLM_MAX_INSTANCES");
		return -EBUSY;
	}

	inst->model = tflite::GetModel(model->data);
	if (inst->model->version() != TFLITE_SCHEMA_VERSION) {
		LOG_ERR("Model schema %u, runtime supports %d", (uint32_t)inst->model->version(),
			TFLITE_SCHEMA_VERSION);
		inst->used = false;
		return -EINVAL;
	}

	inst->resolver = new (inst->resolver_storage) Resolver();
	if (!tflm_register_ops(inst->resolver) || !tflm_register_npu_ops(inst->resolver)) {
		LOG_ERR("Operator registration overflowed CONFIG_MPIPE_NN_TFLM_MAX_OPS=%d",
			CONFIG_MPIPE_NN_TFLM_MAX_OPS);
		inst->resolver->~Resolver();
		inst->used = false;
		return -E2BIG;
	}

	inst->interpreter = new (inst->interpreter_storage)
		tflite::MicroInterpreter(inst->model, *inst->resolver, arena->data, arena->size);

	if (inst->interpreter->AllocateTensors() != kTfLiteOk) {
		LOG_ERR("AllocateTensors failed: arena too small or missing operator");
		inst->interpreter->~MicroInterpreter();
		inst->resolver->~Resolver();
		inst->used = false;
		return -ENOMEM;
	}

	LOG_INF("Arena: %u of %u bytes used", (uint32_t)inst->interpreter->arena_used_bytes(),
		arena->size);

	be->priv = inst;

	return 0;
}

int tflm_get_io(struct mpipe_nn_backend *be, struct mpipe_nn_tensor_set *in,
		struct mpipe_nn_tensor_set *out)
{
	tflm_instance *inst = static_cast<tflm_instance *>(be->priv);
	uint32_t offset;
	int ret;

	if (inst == nullptr) {
		return -EINVAL;
	}

	if (inst->interpreter->inputs_size() > CONFIG_MPIPE_NN_MAX_IO_TENSORS ||
	    inst->interpreter->outputs_size() > CONFIG_MPIPE_NN_MAX_IO_TENSORS) {
		LOG_ERR("Model I/O exceeds CONFIG_MPIPE_NN_MAX_IO_TENSORS=%d",
			CONFIG_MPIPE_NN_MAX_IO_TENSORS);
		return -ENOTSUP;
	}

	memset(in, 0, sizeof(*in));
	in->count = (uint8_t)inst->interpreter->inputs_size();
	offset = 0;
	for (uint8_t i = 0; i < in->count; i++) {
		ret = tflm_describe(inst->interpreter->input(i), &in->desc[i], &offset);
		if (ret != 0) {
			return ret;
		}
	}
	in->total_size = offset;

	memset(out, 0, sizeof(*out));
	out->count = (uint8_t)inst->interpreter->outputs_size();
	offset = 0;
	for (uint8_t i = 0; i < out->count; i++) {
		ret = tflm_describe(inst->interpreter->output(i), &out->desc[i], &offset);
		if (ret != 0) {
			return ret;
		}
	}
	out->total_size = offset;

	return 0;
}

int tflm_invoke(struct mpipe_nn_backend *be, const uint8_t *in, uint32_t in_size, uint8_t *out,
		uint32_t out_size)
{
	tflm_instance *inst = static_cast<tflm_instance *>(be->priv);
	TfLiteTensor *input;

	if (inst == nullptr || in == nullptr || out == nullptr) {
		return -EINVAL;
	}

	input = inst->interpreter->input(0);
	if (input == nullptr || in_size < input->bytes) {
		return -EINVAL;
	}

	/* The interpreter owns the tensor memory, so input and output cross by copy */
	memcpy(input->data.data, in, input->bytes);

	if (inst->interpreter->Invoke() != kTfLiteOk) {
		LOG_ERR("Invoke failed");
		return -EIO;
	}

	uint32_t offset = 0;

	for (size_t i = 0; i < inst->interpreter->outputs_size(); i++) {
		const TfLiteTensor *output = inst->interpreter->output(i);

		offset = ROUND_UP(offset, tensor_align);
		if (output == nullptr || offset + output->bytes > out_size) {
			return -ENOMEM;
		}

		memcpy(&out[offset], output->data.data, output->bytes);
		offset += output->bytes;
	}

	return 0;
}

int tflm_unload(struct mpipe_nn_backend *be)
{
	tflm_instance *inst = static_cast<tflm_instance *>(be->priv);

	if (inst != nullptr) {
		inst->interpreter->~MicroInterpreter();
		inst->resolver->~Resolver();
		inst->used = false;
		be->priv = nullptr;
	}

	return 0;
}

const struct mpipe_nn_backend_ops tflm_ops = {
	.load = tflm_load,
	.get_io = tflm_get_io,
	.invoke = tflm_invoke,
	.unload = tflm_unload,
};

} /* namespace */

int mpipe_nn_backend_default_init(struct mpipe_nn_backend *be)
{
	if (be == nullptr) {
		return -EINVAL;
	}

	be->ops = &tflm_ops;
	be->priv = nullptr;

	return 0;
}
