/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Neural network inference element.
 * @ingroup mpipe_nn
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_NN_MPIPE_NN_RUNNER_H_
#define ZEPHYR_INCLUDE_MPIPE_NN_MPIPE_NN_RUNNER_H_

/**
 * @addtogroup mpipe_nn
 * @{
 */

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_transform.h>
#include <zephyr/mpipe/nn/mpipe_nn.h>
#include <zephyr/mpipe/nn/mpipe_nn_backend.h>
#include <zephyr/mpipe/nn/mpipe_nn_property.h>

/**
 * @brief Neural network inference element
 *
 * Runs the model set with MPIPE_PROP_NN_MODEL on every input tensor buffer and
 * produces one flat output buffer per inference, with the output tensor set
 * referenced from the buffer metadata. The model and arena are application
 * owned and must be set before leaving READY.
 *
 * The invoke call blocks for the duration of the inference; place a queue
 * element upstream so it runs on its own thread, leaky when the producer
 * must not stall.
 */
struct mpipe_nn_runner {
	/** Base transform element (must be first) */
	struct mpipe_transform transform;
	/** The inference backend */
	struct mpipe_nn_backend backend;
	/** Model blob */
	struct mpipe_nn_model model;
	/** Backend working memory */
	struct mpipe_nn_arena arena;
	/** Input tensors of the loaded model */
	struct mpipe_nn_tensor_set in_set;
	/** Output tensors of the loaded model */
	struct mpipe_nn_tensor_set out_set;
	/** True once the model is loaded and the tensor sets are valid */
	bool loaded;
	/** Internal output buffer pool */
	struct mpipe_buffer_pool out_pool;
};

/**
 * @brief Initialize an inference element
 *
 * @param runner Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_nn_runner_init(struct mpipe_nn_runner *runner, uint8_t id);

/**
 * @brief Unload the model
 *
 * The model stays loaded across replays; call this once the element is done
 * with it, so the backend releases its instance.
 *
 * @param runner Pointer to the element.
 */
void mpipe_nn_runner_unload(struct mpipe_nn_runner *runner);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_NN_MPIPE_NN_RUNNER_H_ */
