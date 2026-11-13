/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Inference backend interface of the neural network plugin.
 * @ingroup mpipe_nn
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_NN_MPIPE_NN_BACKEND_H_
#define ZEPHYR_INCLUDE_MPIPE_NN_MPIPE_NN_BACKEND_H_

/**
 * @addtogroup mpipe_nn
 * @{
 */

#include <stdint.h>

#include <zephyr/mpipe/nn/mpipe_nn.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of dimensions a tensor descriptor carries */
#define MPIPE_NN_TENSOR_MAX_DIMS 4

/**
 * @brief Description of one model input or output tensor
 */
struct mpipe_nn_tensor_desc {
	/** Element data type, an @ref mpipe_nn_tensor_type value */
	uint8_t type;
	/** Number of dimensions */
	uint8_t num_dims;
	/** Dimensions as the model declares them, e.g. NHWC for images */
	uint16_t dims[MPIPE_NN_TENSOR_MAX_DIMS];
	/** Payload size in bytes */
	uint32_t size;
	/** Offset of this tensor in the flat buffer, 4-byte aligned */
	uint32_t offset;
	/** Quantization scale, 0.0f when the tensor is not quantized */
	float scale;
	/** Quantization zero point */
	int32_t zero_point;
};

/**
 * @brief The set of input or output tensors of a loaded model
 *
 * All tensors of one inference travel in a single flat buffer; each
 * descriptor holds its offset. The inference element owns one set per
 * direction and references the output set from the buffer metadata of every
 * buffer it produces, so downstream elements can interpret the payload
 * without the caps system having to describe every tensor.
 */
struct mpipe_nn_tensor_set {
	/** Number of tensors */
	uint8_t count;
	/** Size in bytes of the flat buffer, including alignment padding */
	uint32_t total_size;
	/** Per-tensor descriptions */
	struct mpipe_nn_tensor_desc desc[CONFIG_MPIPE_NN_MAX_IO_TENSORS];
};

struct mpipe_nn_backend;

/**
 * @brief Operations an inference backend implements
 *
 * The interface is plain C so that the elements never depend on the types of
 * any particular runtime. A backend wraps one runtime: the fake backend for
 * tests, TensorFlow Lite Micro for real models, with vendor NPU support as
 * build-time variants of the latter.
 */
struct mpipe_nn_backend_ops {
	/**
	 * @brief Parse the model and prepare it for inference
	 *
	 * @param[in,out] be Backend instance
	 * @param model Model blob
	 * @param arena Working memory for the runtime
	 * @retval 0 Success
	 * @retval -EINVAL Malformed model
	 * @retval -ENOMEM Arena too small; some runtimes also report a missing operator so
	 * @retval -ENOTSUP The model needs an operator the backend does not provide
	 * @retval -EBUSY No backend instance free
	 */
	int (*load)(struct mpipe_nn_backend *be, const struct mpipe_nn_model *model,
		    const struct mpipe_nn_arena *arena);
	/**
	 * @brief Describe the loaded model's input and output tensors
	 *
	 * @param be Backend instance
	 * @param[out] in Input tensor set
	 * @param[out] out Output tensor set, including flat-buffer offsets and
	 *                 quantization parameters
	 * @retval 0 Success
	 * @retval -EINVAL No model loaded
	 * @retval -ENOTSUP A tensor type, rank or count the plugin cannot describe
	 */
	int (*get_io)(struct mpipe_nn_backend *be, struct mpipe_nn_tensor_set *in,
		      struct mpipe_nn_tensor_set *out);
	/**
	 * @brief Run one inference
	 *
	 * Copies the input into the runtime, invokes the model, and copies
	 * every output tensor to its offset in @p out. Blocks the calling
	 * thread for the duration of the inference.
	 *
	 * @param be Backend instance
	 * @param in Input tensor payload
	 * @param in_size Number of input bytes
	 * @param[out] out Flat output buffer
	 * @param out_size Number of output bytes available
	 * @retval 0 Success
	 * @retval -EINVAL No model loaded or input too small
	 * @retval -ENOMEM Output buffer too small
	 * @retval -EIO The runtime failed
	 */
	int (*invoke)(struct mpipe_nn_backend *be, const uint8_t *in, uint32_t in_size,
		      uint8_t *out, uint32_t out_size);
	/**
	 * @brief Release the loaded model
	 *
	 * @param[in,out] be Backend instance
	 * @retval 0 Success
	 */
	int (*unload)(struct mpipe_nn_backend *be);
};

/**
 * @brief An inference backend instance
 */
struct mpipe_nn_backend {
	/** Operations */
	const struct mpipe_nn_backend_ops *ops;
	/** Backend private state */
	void *priv;
};

/**
 * @brief Bind the backend selected at build time
 *
 * Provided by whichever backend implementation is compiled in.
 *
 * @param[in,out] be Backend instance to bind
 * @retval 0 Success
 * @retval -EINVAL Null instance
 */
int mpipe_nn_backend_default_init(struct mpipe_nn_backend *be);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_NN_MPIPE_NN_BACKEND_H_ */
