/*
 * Copyright 2019-2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "inference_process.hpp"

#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/cortex_m_generic/debug_log_callback.h>
#include <tensorflow/lite/micro/micro_log.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_profiler.h>
#include <tensorflow/lite/schema/schema_generated.h>

#include <cmsis_compiler.h>
#include <inttypes.h>
#include <zephyr/kernel.h>

using namespace std;

namespace
{
bool copyOutput(const TfLiteTensor &src, InferenceProcess::DataPtr &dst)
{
	if (dst.data == nullptr) {
		return false;
	}

	if (src.bytes > dst.size) {
		printf("Tensor size mismatch (bytes): actual=%d, expected%d.\n", src.bytes,
		       dst.size);
		return true;
	}

	copy(src.data.uint8, src.data.uint8 + src.bytes, static_cast<uint8_t *>(dst.data));
	dst.size = src.bytes;

	return false;
}

} /* namespace */

namespace InferenceProcess
{
DataPtr::DataPtr(void *_data, size_t _size) : data(_data), size(_size)
{
}

void DataPtr::invalidate()
{
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
	SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t *>(data), size);
#endif
}

void DataPtr::clean()
{
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
	SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t *>(data), size);
#endif
}

InferenceJob::InferenceJob()
{
}

InferenceJob::InferenceJob(const string &_name, const DataPtr &_networkModel,
			   const vector<DataPtr> &_input, const vector<DataPtr> &_output,
			   const vector<DataPtr> &_expectedOutput)
	: name(_name), networkModel(_networkModel), input(_input), output(_output),
	  expectedOutput(_expectedOutput)
{
}

void InferenceJob::invalidate()
{
	networkModel.invalidate();

	for (auto &it : input) {
		it.invalidate();
	}

	for (auto &it : output) {
		it.invalidate();
	}

	for (auto &it : expectedOutput) {
		it.invalidate();
	}
}

void InferenceJob::clean()
{
	networkModel.clean();

	for (auto &it : input) {
		it.clean();
	}

	for (auto &it : output) {
		it.clean();
	}

	for (auto &it : expectedOutput) {
		it.clean();
	}
}

bool InferenceProcess::runJob(InferenceJob &job)
{
	/* Get model handle and verify that the version is correct */
	const tflite::Model *model = ::tflite::GetModel(job.networkModel.data);
	if (model->version() != TFLITE_SCHEMA_VERSION) {
		printf("Model schema version unsupported: version=%" PRIu32 ", supported=%d.\n",
		       model->version(), TFLITE_SCHEMA_VERSION);
		return true;
	}

	/* Create the TFL micro interpreter */
	tflite::MicroMutableOpResolver <1> resolver;
	resolver.AddEthosU();

	tflite::MicroInterpreter interpreter(model, resolver, tensorArena, tensorArenaSize);

	/* Allocate tensors */
	TfLiteStatus allocate_status = interpreter.AllocateTensors();
	if (allocate_status != kTfLiteOk) {
		printf("Failed to allocate tensors for inference. job=%p\n", &job);
		return true;
	}

	if (job.input.size() != interpreter.inputs_size()) {
		printf("Number of job and network inputs do not match. input=%zu, network=%zu\n",
		       job.input.size(), interpreter.inputs_size());
		return true;
	}

	/* Copy input data */
	for (size_t i = 0; i < interpreter.inputs_size(); ++i) {
		const DataPtr &input = job.input[i];
		const TfLiteTensor *tensor = interpreter.input(i);

		if (input.size != tensor->bytes) {
			printf("Input tensor size mismatch. index=%zu, input=%zu, network=%u\n", i,
			       input.size, tensor->bytes);
			return true;
		}

		copy(static_cast<char *>(input.data), static_cast<char *>(input.data) + input.size,
		     tensor->data.uint8);
	}

	/* Run the inference */
	TfLiteStatus invoke_status = interpreter.Invoke();
	if (invoke_status != kTfLiteOk) {
		printf("Invoke failed for inference. job=%s\n", job.name.c_str());
		return true;
	}

	/* Copy output data */
	if (job.output.size() > 0) {
		if (interpreter.outputs_size() != job.output.size()) {
			printf("Number of job and network outputs do not match. job=%zu, network=%u\n",
			       job.output.size(), interpreter.outputs_size());
			return true;
		}

		for (unsigned i = 0; i < interpreter.outputs_size(); ++i) {
			if (copyOutput(*interpreter.output(i), job.output[i])) {
				return true;
			}
		}
	}

	if (job.expectedOutput.size() > 0) {
		if (job.expectedOutput.size() != interpreter.outputs_size()) {
			printf("Number of job and network expected outputs do not match. job=%zu, network=%zu\n",
			       job.expectedOutput.size(), interpreter.outputs_size());
			return true;
		}

		for (unsigned int i = 0; i < interpreter.outputs_size(); i++) {
			const DataPtr &expected = job.expectedOutput[i];
			const TfLiteTensor *output = interpreter.output(i);

			if (expected.size != output->bytes) {
				printf("Expected output tensor size mismatch. index=%u, expected=%zu, network=%zu\n",
				       i, expected.size, output->bytes);
				return true;
			}

			for (unsigned int j = 0; j < output->bytes; ++j) {
				if (output->data.uint8[j] !=
				    static_cast<uint8_t *>(expected.data)[j]) {
					printf("Expected output tensor data mismatch. index=%u, offset=%u, expected=%02x, network=%02x\n",
					       i, j, static_cast<uint8_t *>(expected.data)[j],
					       output->data.uint8[j]);
					return true;
				}
			}
		}
	}

	return false;
}

} /* namespace InferenceProcess */
