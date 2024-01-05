/*
 * Copyright 2023 HNU-ESNL
 * Copyright 2023 YNU-SOFTWARE
 * Copyright 2023 PaddlePaddle
 * Copyright 2023 openEuler SIG-Zephyr
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef PADDLE_LITE_API_H_  // NOLINT
#define PADDLE_LITE_API_H_
#include <memory>
#include <string>
#include <vector>
#include "paddle_place.h"  // NOLINT

namespace paddle {
namespace lite_api {

using shape_t = std::vector<int64_t>;
using lod_t = std::vector<std::vector<uint64_t>>;

enum class LiteModelType { kProtobuf = 0, kNaiveBuffer, UNK };
// Methods for allocating L3Cache on Arm platform
enum class L3CacheSetMethod {
  kDeviceL3Cache = 0,  // Use the system L3 Cache size, best performance.
  kDeviceL2Cache = 1,  // Use the system L2 Cache size, trade off performance
                       // with less memory consumption.
  kAbsolute = 2,       // Use the external setting.
  // kAutoGrow = 3,   // Not supported yet, least memory consumption.
};

// return true if current device supports OpenCL model
LITE_API bool IsOpenCLBackendValid();

struct LITE_API Tensor {
  explicit Tensor(void* raw);
  explicit Tensor(const void* raw);

  void Resize(const shape_t& shape);

  /// Readonly data.
  template <typename T>
  const T* data() const;

  template <typename T>
  T* mutable_data(TargetType type = TargetType::kHost) const;

  template <typename T, TargetType type = TargetType::kHost>
  void CopyFromCpu(const T* data);

  template <typename T>
  void CopyToCpu(T* data) const;
  /// Shape of the tensor.
  shape_t shape() const;
  TargetType target() const;
  PrecisionType precision() const;

  // LoD of the tensor
  lod_t lod() const;

  // Set LoD of the tensor
  void SetLoD(const lod_t& lod);

 private:
  void* raw_tensor_;
};

/// The PaddlePredictor defines the basic interfaces for different kinds of
/// predictors.
class LITE_API PaddlePredictor {
 public:
  PaddlePredictor() = default;

  /// Get i-th input.
  virtual std::unique_ptr<Tensor> GetInput(int i) = 0;

  /// Get i-th output.
  virtual std::unique_ptr<const Tensor> GetOutput(int i) const = 0;

  virtual void Run() = 0;
  virtual std::shared_ptr<PaddlePredictor> Clone() = 0;

  virtual std::string GetVersion() const = 0;

  // Get input names
  virtual std::vector<std::string> GetInputNames() = 0;
  // Get output names
  virtual std::vector<std::string> GetOutputNames() = 0;
  // Get output names
  virtual std::vector<std::string> GetParamNames();

  // Get Input by name
  virtual std::unique_ptr<Tensor> GetInputByName(const std::string& name) = 0;

  /// Get a readonly tensor, return null if no one called `name` exists.
  virtual std::unique_ptr<const Tensor> GetTensor(
      const std::string& name) const = 0;
  /// Get a mutable tensor, return null if on one called `name` exists
  /// internal infereces API, not recommanded.
  virtual std::unique_ptr<Tensor> GetMutableTensor(const std::string& name);

  /// Persist the optimized model to disk. This API is only supported by
  /// CxxConfig, and the persisted model can be reused for MobileConfig.
  virtual void SaveOptimizedModel(
      const std::string& model_dir,
      LiteModelType model_type = LiteModelType::kProtobuf,
      bool record_info = false);

  virtual ~PaddlePredictor() = default;

 protected:
  int threads_{1};
  lite_api::PowerMode mode_{lite_api::LITE_POWER_NO_BIND};
};

/// Base class for all the configs.
class LITE_API ConfigBase {
  std::string model_dir_;
  int threads_{1};
  PowerMode mode_{LITE_POWER_NO_BIND};
  // gpu
  bool enable_opencl_tune_{false};
  // to save subgraph model for npu/xpu/...
  std::string subgraph_model_cache_dir_{""};

 public:
  explicit ConfigBase(PowerMode mode = LITE_POWER_NO_BIND, int threads = 1);
  // set Model_dir
  void set_model_dir(const std::string& x) { model_dir_ = x; }
  const std::string& model_dir() const { return model_dir_; }
  // set Power_mode
  void set_power_mode(PowerMode mode);
  PowerMode power_mode() const { return mode_; }
  // set Thread
  void set_threads(int threads);
  int threads() const { return threads_; }
  // set GPU opencl tune
  void set_opencl_tune(bool enable_tune);
  bool opencl_tune() const { return enable_opencl_tune_; }
  // set subgraph_model_dir
  void set_subgraph_model_cache_dir(std::string subgraph_model_cache_dir) {
    subgraph_model_cache_dir_ = subgraph_model_cache_dir;
  }
  const std::string& subgraph_model_cache_dir() const {
    return subgraph_model_cache_dir_;
  }
};

/// CxxConfig is the config for the Full feature predictor.
class LITE_API CxxConfig : public ConfigBase {
  std::vector<Place> valid_places_;
  std::string model_file_;
  std::string param_file_;
  std::vector<std::string> passes_internal_{};
  bool model_from_memory_{false};
#ifdef LITE_WITH_X86
  int x86_math_library_math_threads_ = 1;
#endif
#ifdef LITE_WITH_CUDA
  bool multi_stream_{false};
#endif
#ifdef LITE_WITH_MLU
  lite_api::MLUCoreVersion mlu_core_version_{lite_api::MLUCoreVersion::MLU_270};
  int mlu_core_number_{1};
  DataLayoutType mlu_input_layout_{DATALAYOUT(kNCHW)};
  bool mlu_use_first_conv_{false};
  std::vector<float> mlu_first_conv_mean_;
  std::vector<float> mlu_first_conv_std_;
#endif

 public:
  void set_valid_places(const std::vector<Place>& x) { valid_places_ = x; }
  void set_model_file(const std::string& path) { model_file_ = path; }
  void set_param_file(const std::string& path) { param_file_ = path; }
  void set_model_buffer(const char* model_buffer,
                        size_t model_buffer_size,
                        const char* param_buffer,
                        size_t param_buffer_size) {
    model_file_ = std::string(model_buffer, model_buffer + model_buffer_size);
    param_file_ = std::string(param_buffer, param_buffer + param_buffer_size);
    model_from_memory_ = true;
  }
  // internal inference to choose passes for model optimizing,
  // it's designed for internal developer and not recommanded
  // for comman users.
  void set_passes_internal(
      const std::vector<std::string>& passes_internal = {}) {
    passes_internal_ = passes_internal;
  }
  const std::vector<std::string>& get_passes_internal() const {
    return passes_internal_;
  }
  const std::vector<Place>& valid_places() const { return valid_places_; }
  std::string model_file() const { return model_file_; }
  std::string param_file() const { return param_file_; }
  bool model_from_memory() const { return model_from_memory_; }

#ifdef LITE_WITH_X86
  void set_x86_math_library_num_threads(int threads) {
    x86_math_library_math_threads_ = threads;
  }
  int x86_math_library_num_threads() const {
    return x86_math_library_math_threads_;
  }
#endif
#ifdef LITE_WITH_CUDA
  void set_multi_stream(bool multi_stream) { multi_stream_ = multi_stream; }
  bool multi_stream() const { return multi_stream_; }
#endif

#ifdef LITE_WITH_MLU
  // set MLU core version, which is used when compiling MLU kernels
  void set_mlu_core_version(lite_api::MLUCoreVersion core_version);
  // set MLU core number, which is used when compiling MLU kernels
  void set_mlu_core_number(int core_number);
  // set MLU input layout. User can specify layout of input data to be NHWC,
  // default is NCHW
  void set_mlu_input_layout(DataLayoutType layout);
  // whether use MLU's first conv kernel. First conv is a special kernel
  // provided by MLU, its input is uint8, and also needs two 3-dimentional
  // vectors which save all inputs' mean and std values
  void set_mlu_use_first_conv(bool use_first_conv);
  // set the 3-dimentional mean vector used by MLU's first conv
  void set_mlu_first_conv_mean(const std::vector<float>& mean);
  // set the 3-dimentional std vector used by MLU's first conv
  void set_mlu_first_conv_std(const std::vector<float>& std);

  lite_api::MLUCoreVersion mlu_core_version() const;
  int mlu_core_number() const;
  DataLayoutType mlu_input_layout() const;
  bool mlu_use_first_conv() const;
  const std::vector<float>& mlu_first_conv_mean() const;
  const std::vector<float>& mlu_first_conv_std() const;
#endif

  // XPU only, set the size of the workspace memory from L3 cache for the
  // current thread.
  void set_xpu_workspace_l3_size_per_thread(int l3_size = 0xfffc00);
  // XPU only, specify the target device ID for the current thread.
  // **DEPRECATED**, use xpu_set_device() at the very beginning of each worker
  // thread
  void set_xpu_dev_per_thread(int dev_no = 0);
};

/// MobileConfig is the config for the light weight predictor, it will skip
/// IR optimization or other unnecessary stages.
class LITE_API MobileConfig : public ConfigBase {
  // whether to load data from memory. Model data will be loaded from memory
  // buffer if model_from_memory_ is true.
  bool model_from_memory_{false};

  // model data readed from file or memory buffer in combined format.
  std::string lite_model_file_;

  // NOTE: This is a deprecated variable and will be removed in latter release.
  std::string model_buffer_;
  std::string param_buffer_;

 public:
  // set model data in combined format, `set_model_from_file` refers to loading
  // model from file, set_model_from_buffer refers to loading model from memory
  // buffer
  void set_model_from_file(const std::string& x);
  void set_model_from_buffer(const std::string& x);
  // return model data in lite_model_file_, which is in combined format.
  const std::string& lite_model_file() const { return lite_model_file_; }

  // return model_from_memory_, which indicates whether to load model from
  // memory buffer.
  bool model_from_memory() const { return model_from_memory_; }

  // NOTE: This is a deprecated API and will be removed in latter release.
  void set_model_buffer(const char* model_buffer,
                        size_t model_buffer_size,
                        const char* param_buffer,
                        size_t param_buffer_size);

  // NOTE: This is a deprecated API and will be removed in latter release.
  const std::string& model_buffer() const { return model_buffer_; }

  // NOTE: This is a deprecated API and will be removed in latter release.
  const std::string& param_buffer() const { return param_buffer_; }

  // This is the method for allocating workspace_size according to L3Cache size
  void SetArmL3CacheSize(
      L3CacheSetMethod method = L3CacheSetMethod::kDeviceL3Cache,
      int absolute_val = -1);
};

template <typename ConfigT>
LITE_API std::shared_ptr<PaddlePredictor> CreatePaddlePredictor(const ConfigT&);

}  // namespace lite_api
}  // namespace paddle

#endif  // NOLINT
