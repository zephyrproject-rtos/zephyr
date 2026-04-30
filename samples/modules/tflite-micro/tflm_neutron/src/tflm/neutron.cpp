/* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "NeutronDriver.h"
#include "flatbuffers/flexbuffers.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/kernels/kernel_util.h"
#include "tensorflow/lite/micro/micro_context.h"
#include "tensorflow/lite/micro/micro_log.h"

#if defined(__ICCARM__)
extern "C" void* memalign(size_t alignment, size_t size) {
  return aligned_alloc(alignment, size);
}
#elif defined(__ARMCC_VERSION)
extern "C" void* memalign(size_t alignment, size_t size) {
  void* ptr = NULL;
  posix_memalign(&ptr, alignment, size);
  return ptr;
}
#endif

namespace tflite {
namespace {

typedef struct {
  NeutronModelConfig model_config;
  NeutronDataConfig data_config;
  NeutronModelHandle model_handle;
  int inputs_index;
  int outputs_index;
} NeutronTfLiteConfig;

#ifdef EXTERNAL_MEM
#ifndef SCRATCH_WEIGHTS_SRAM_SIZE
#define SCRATCH_WEIGHTS_SRAM_SIZE 1200 * 1024
#endif

static uint8_t scratchWeightsBuffer[SCRATCH_WEIGHTS_SRAM_SIZE]
    __attribute__((aligned(16))) __attribute__((section("NonCacheable")));
static NeutronConfig* n_config = nullptr;

void copy(void* dst, void* src, uint32_t size, uint32_t channel) {
  if (((uintptr_t)dst + size - (uintptr_t)scratchWeightsBuffer) >
      SCRATCH_WEIGHTS_SRAM_SIZE) {
    MicroPrintf(
        "scratchWeightsBuffer Overflow, Requested: %x "
        "SCRATCH_WEIGHTS_SRAM_SIZE: %x\r\n",
        ((uintptr_t)dst + size - (uintptr_t)scratchWeightsBuffer),
        SCRATCH_WEIGHTS_SRAM_SIZE);
    while (1);
    return;
  }
  memcpy(dst, src, size);
}
void wait(uint32_t channel) {}
#endif

static int driver_ref_count = 0;

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);

  if (driver_ref_count++ == 0) {
    NeutronError error = ENONE;
    error = neutronInit();
    if (error != ENONE) {
      MicroPrintf("Internal Neutron NPU driver error %x in init!", error);
    }
  }

  return context->AllocatePersistentBuffer(context,
                                           sizeof(NeutronTfLiteConfig));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(context != nullptr);
  TFLITE_DCHECK(node->user_data != nullptr);
  TF_LITE_ENSURE(context, node->inputs->size > 2);
  TF_LITE_ENSURE(context, node->outputs->size > 0);

  TfLiteStatus status = kTfLiteOk;
  NeutronTfLiteConfig* neutron =
      static_cast<NeutronTfLiteConfig*>(node->user_data);
  MicroContext* micro_context = GetMicroContext(context);

  TF_LITE_ENSURE_STATUS(context->RequestScratchBufferInArena(
      context, node->inputs->size * sizeof(void*), &neutron->inputs_index));
  TF_LITE_ENSURE_STATUS(context->RequestScratchBufferInArena(
      context, node->outputs->size * sizeof(void*), &neutron->outputs_index));

  TfLiteTensor* microcode =
      micro_context->AllocateTempInputTensor(node, node->inputs->size - 3);
  TfLiteTensor* weights =
      micro_context->AllocateTempInputTensor(node, node->inputs->size - 2);
  TfLiteTensor* kernels =
      micro_context->AllocateTempInputTensor(node, node->inputs->size - 1);

  NeutronError error = ENONE;
  neutron->model_config = {.microcode = microcode->data.data,
                           .weights = weights->data.data,
                           .kernels = kernels->data.data};

  NeutronModelHandle hdl_mem = static_cast<NeutronModelHandle>(
      context->AllocatePersistentBuffer(context, neutronGetModelContextSize()));
  if (hdl_mem == nullptr) {
    return kTfLiteError;
  }
  neutron->model_handle = hdl_mem;
  error = neutronModelPrepare(&neutron->model_config, &neutron->model_handle);

  if (error != ENONE) {
    if ((GET_ERROR_COMPONENT(error) == ERROR_COMPONENT_DRIVER) &&
        (GET_ERROR_CATEGORY(error) == ERROR_CATEGORY_DRIVER_UCODE)) {
      MicroPrintf(
          "Incompatible Neutron NPU microcode and driver versions! Please, "
          "convert the model with Neutron converter tool intended for this SDK "
          "release.");
    } else {
      MicroPrintf("Internal Neutron NPU driver error %x in model prepare!",
                  error);
    }
    status = kTfLiteError;
  }

  micro_context->DeallocateTempTfLiteTensor(microcode);
  micro_context->DeallocateTempTfLiteTensor(weights);
  micro_context->DeallocateTempTfLiteTensor(kernels);

#ifdef EXTERNAL_MEM
  n_config = static_cast<NeutronConfig*>(
      context->AllocatePersistentBuffer(context, sizeof(NeutronConfig)));
  n_config->copy = copy;
  n_config->wait = wait;
  error = neutronSetConfig(n_config);
  if (error != ENONE) {
    MicroPrintf("Set Neutron NPU config error %x in model prepare!", error);
    return kTfLiteError;
  }
#endif

  return status;
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(context != nullptr);
  TFLITE_DCHECK(node->user_data != nullptr);

  NeutronTfLiteConfig* neutron =
      static_cast<NeutronTfLiteConfig*>(node->user_data);

  neutron->data_config.inputs = reinterpret_cast<const void**>(
      context->GetScratchBuffer(context, neutron->inputs_index));
  neutron->data_config.outputs = reinterpret_cast<void**>(
      context->GetScratchBuffer(context, neutron->outputs_index));

  for (int i = 0; i < node->inputs->size - 3; i++) {
    TfLiteEvalTensor* input =
        context->GetEvalTensor(context, node->inputs->data[i]);
    neutron->data_config.inputs[i] = input->data.data;
  }

  for (int i = 0; i < node->outputs->size - 1; i++) {
    TfLiteEvalTensor* output =
        context->GetEvalTensor(context, node->outputs->data[i]);
    neutron->data_config.outputs[i] = output->data.data;
  }

  if (node->outputs->size > 0) {
    TfLiteEvalTensor* scratch = context->GetEvalTensor(
        context, node->outputs->data[node->outputs->size - 3]);
    neutron->data_config.outputs[node->outputs->size - 1] = scratch->data.data;
    neutron->data_config.scratch = scratch->data.data;
  }

#ifdef EXTERNAL_MEM
  neutron->data_config.scratchWeights = (void*)scratchWeightsBuffer;
#endif

#ifdef NEUTRON_PROFILE
  uint8_t profile = 0;
  static NeutronTraceConfig trace_config = {
      .traceConfig = profile, .traceBuffer = nullptr, .traceBufferSize = 0};
  neutronSetTrace(neutron->model_handle, &trace_config);
#endif

  NeutronError error = ENONE;
  error = neutronRunBlocking(neutron->model_handle, &neutron->data_config);
  if (error != ENONE) {
    MicroPrintf("Internal Neutron NPU driver error %x in model run!", error);
    return kTfLiteError;
  }

  return kTfLiteOk;
}

void Free(TfLiteContext* context, void* buffer) {
  TFLITE_DCHECK(context != nullptr);
  TFLITE_DCHECK(buffer != nullptr);

  NeutronTfLiteConfig* neutron = static_cast<NeutronTfLiteConfig*>(buffer);

  NeutronError error = ENONE;
  error = neutronModelUnprepare(neutron->model_handle);
  if (error != ENONE) {
    MicroPrintf("Internal Neutron NPU driver error %x in model unprepare!",
                error);
  }

  if (--driver_ref_count == 0) {
    error = neutronDeinit();
    if (error != ENONE) {
      MicroPrintf("Internal Neutron NPU driver error %x in deinit!", error);
    }
  }
}

}  // namespace

TFLMRegistration* Register_NEUTRON_GRAPH() {
  static TFLMRegistration r =
      tflite::micro::RegisterOp(Init, Prepare, Eval, Free, nullptr);
  return &r;
}

const char* GetString_NEUTRON_GRAPH() { return "NeutronGraph"; }

}  // namespace tflite
