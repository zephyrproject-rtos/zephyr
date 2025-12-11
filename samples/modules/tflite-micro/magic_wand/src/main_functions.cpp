/*
 * Copyright 2019 The TensorFlow Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "main_functions.hpp"

#include "accelerometer_handler.hpp"
#include "constants.hpp"
#include "gesture_predictor.hpp"
#include "magic_wand_model_data.hpp"
#include "output_handler.hpp"
#include <tensorflow/lite/micro/micro_log.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/schema/schema_generated.h>

/* Globals, used for compatibility with Arduino-style sketches. */
namespace {
	const tflite::Model *model = nullptr;
	tflite::MicroInterpreter *interpreter = nullptr;
	TfLiteTensor *model_input = nullptr;
	int input_length;

	/* Create an area of memory to use for input, output, and intermediate arrays.
	* The size of this will depend on the model you're using, and may need to be
	* determined by experimentation.
	*/
	constexpr int kTensorArenaSize = 60 * 1024;
	uint8_t tensor_arena[kTensorArenaSize];
} /* namespace */

/* The name of this function is important for Arduino compatibility. */
void setup(void)
{
	/* Map the model into a usable data structure. This doesn't involve any
	 * copying or parsing, it's a very lightweight operation.
	 */
	model = tflite::GetModel(g_magic_wand_model_data);
	if (model->version() != TFLITE_SCHEMA_VERSION) {
		MicroPrintf("Model provided is schema version %d not equal "
				    "to supported version %d.",
				    model->version(), TFLITE_SCHEMA_VERSION);
		return;
	}

	/* Pull in only the operation implementations we need.
	 * This relies on a complete list of all the ops needed by this graph.
	 * An easier approach is to just use the AllOpsResolver, but this will
	 * incur some penalty in code space for op implementations that are not
	 * needed by this graph.
	 */
	static tflite::MicroMutableOpResolver < 5 > micro_op_resolver; /* NOLINT */
	micro_op_resolver.AddConv2D();
	micro_op_resolver.AddDepthwiseConv2D();
	micro_op_resolver.AddFullyConnected();
	micro_op_resolver.AddMaxPool2D();
	micro_op_resolver.AddSoftmax();

	/* Build an interpreter to run the model with. */
	static tflite::MicroInterpreter static_interpreter(
		model, micro_op_resolver, tensor_arena, kTensorArenaSize);
	interpreter = &static_interpreter;

	/* Allocate memory from the tensor_arena for the model's tensors. */
	interpreter->AllocateTensors();

	/* Obtain pointer to the model's input tensor. */
	model_input = interpreter->input(0);
	if ((model_input->dims->size != 4) || (model_input->dims->data[0] != 1) ||
	    (model_input->dims->data[1] != 128) ||
	    (model_input->dims->data[2] != kChannelNumber) ||
	    (model_input->type != kTfLiteFloat32)) {
		MicroPrintf("Bad input tensor parameters in model");
		return;
	}

	input_length = model_input->bytes / sizeof(float);

	TfLiteStatus setup_status = SetupAccelerometer();
	if (setup_status != kTfLiteOk) {
		MicroPrintf("Set up failed\n");
	}
}

void loop(void)
{
	/* Attempt to read new data from the accelerometer. */
	bool got_data =
		ReadAccelerometer(model_input->data.f, input_length);

	/* If there was no new data, wait until next time. */
	if (!got_data) {
		return;
	}

	/* Run inference, and report any error */
	TfLiteStatus invoke_status = interpreter->Invoke();
	if (invoke_status != kTfLiteOk) {
		MicroPrintf("Invoke failed on index: %d\n", begin_index);
		return;
	}
	/* Analyze the results to obtain a prediction */
	int gesture_index = PredictGesture(interpreter->output(0)->data.f);

	/* Produce an output */
	HandleOutput(gesture_index);
}
