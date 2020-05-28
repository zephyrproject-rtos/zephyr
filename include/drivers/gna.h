/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * Author: Sathish Kuttan <sathish.k.kuttan@intel.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API header file for Intel GNA driver
 */

#ifndef __INCLUDE_GNA__
#define __INCLUDE_GNA__

/**
 * @defgroup gna_interface GNA Interface
 * @ingroup io_interfaces
 * @{
 *
 * This file contains the driver APIs for Intel's
 * Gaussian Mixture Model and Neural Network Accelerator (GNA)
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * GNA driver configuration structure.
 * Currently empty.
 */
struct gna_config {
};

/**
 * GNA Neural Network model header
 * Describes the key parameters of the neural network model
 */
struct gna_model_header {
	uint32_t	labase_offset;
	uint32_t	model_size;
	uint32_t	gna_mode;
	uint32_t	layer_count;
	uint32_t	bytes_per_input;
	uint32_t	bytes_per_output;
	uint32_t	num_input_nodes;
	uint32_t	num_output_nodes;
	uint32_t	input_ptr_offset;
	uint32_t	output_ptr_offset;
	uint32_t	rw_region_size;
	uint32_t	input_scaling_factor;
	uint32_t	output_scaling_factor;
};

/**
 * GNA Neural Network model information to be provided by application
 * during model registration
 */
struct gna_model_info {
	struct gna_model_header *header;
	void *rw_region;
	void *ro_region;
};

/**
 * Request to perform inference on the given neural network model
 */
struct gna_inference_req {
	void *model_handle;
	void *input;
	void *output;
	void *intermediate;
};

/**
 * Statistics of the inference operation returned after completion
 */
struct gna_inference_stats {
	uint32_t total_cycles;
	uint32_t stall_cycles;
	uint32_t cycles_per_sec;
};

/**
 * Result of an inference operation
 */
enum gna_result {
	GNA_RESULT_INFERENCE_COMPLETE,
	GNA_RESULT_SATURATION_OCCURRED,
	GNA_RESULT_OUTPUT_BUFFER_FULL_ERROR,
	GNA_RESULT_PARAM_OUT_OF_RANGE_ERROR,
	GNA_RESULT_GENERIC_ERROR,
};

/**
 * Structure containing a response to the inference request
 */
struct gna_inference_resp {
	enum gna_result result;
	void *output;
	size_t output_len;
	struct gna_inference_stats stats;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * Internal documentation. Skip in public documentation
 */
typedef int (*gna_callback)(struct gna_inference_resp *result);

typedef int (*gna_api_config)(struct device *dev, struct gna_config *cfg);
typedef int (*gna_api_register)(struct device *dev,
		struct gna_model_info *model, void **model_handle);
typedef int (*gna_api_deregister)(struct device *dev, void *model_handle);
typedef int (*gna_api_infer)(struct device *dev, struct gna_inference_req *req,
		gna_callback callback);

struct gna_driver_api {
	gna_api_config		configure;
	gna_api_register	register_model;
	gna_api_deregister	deregister_model;
	gna_api_infer		infer;
};

/**
 * @endcond
 */

/**
 * @brief Configure the GNA device.
 *
 * Configure the GNA device. The GNA device must be configured before
 * registering a model or performing inference
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cfg Device configuration information
 *
 * @retval 0 If the configuration is successful
 * @retval A negative error code in case of a failure.
 */
static inline int gna_configure(struct device *dev, struct gna_config *cfg)
{
	const struct gna_driver_api *api =
		(const struct gna_driver_api *)dev->api;

	return api->configure(dev, cfg);
}

/**
 * @brief Register a neural network model
 *
 * Register a neural network model with the GNA device
 * A model needs to be registered before it can be used to perform inference
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param model Information about the neural network model
 * @param model_handle Handle to the registered model if registration succeeds
 *
 * @retval 0 If registration of the model is successful.
 * @retval A negative error code in case of a failure.
 */
static inline int gna_register_model(struct device *dev,
		struct gna_model_info *model, void **model_handle)
{
	const struct gna_driver_api *api =
		(const struct gna_driver_api *)dev->api;

	return api->register_model(dev, model, model_handle);
}

/**
 * @brief De-register a previously registered neural network model
 *
 * De-register a previously registered neural network model from the GNA device
 * De-registration may be done to free up memory for registering another model
 * Once de-registered, the model can no longer be used to perform inference
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param model Model handle output by gna_register_model API
 *
 * @retval 0 If de-registration of the model is successful.
 * @retval A negative error code in case of a failure.
 */
static inline int gna_deregister_model(struct device *dev, void *model)
{
	const struct gna_driver_api *api =
		(const struct gna_driver_api *)dev->api;

	return api->deregister_model(dev, model);
}

/**
 * @brief Perform inference on a model with input vectors
 *
 * Make an inference request on a previously registered model with an of
 * input data vector
 * A callback is provided for notification of inference completion
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param req Information required to perform inference on a neural network
 * @param callback A callback function to notify inference completion
 *
 * @retval 0 If the request is accepted
 * @retval A negative error code in case of a failure.
 */
static inline int gna_infer(struct device *dev, struct gna_inference_req *req,
	gna_callback callback)
{
	const struct gna_driver_api *api =
		(const struct gna_driver_api *)dev->api;

	return api->infer(dev, req, callback);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __INCLUDE_GNA__ */
