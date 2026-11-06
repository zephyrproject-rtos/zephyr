cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(micro_speech_openamp)

# These samples use local static initialization. Since Zephyr doesn't support the
# C++ ABI for thread-safe initialization of local statics and the constructors don't
# appear to require thread safety, we turn it off in the C++ compiler.
set(NO_THREADSAFE_STATICS $<TARGET_PROPERTY:compiler-cpp,no_threadsafe_statics>)
zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:${NO_THREADSAFE_STATICS}>)

# Define common paths
set(OPTIONAL_TFLITE_DIR $ENV{ZEPHYR_BASE}/../optional/modules/lib/tflite-micro/signal)

file(
  GLOB
  app_sources
  src/*
  src/inference/*
  src/transport/*
)

target_include_directories(
  app
  PRIVATE
  src
  ${ZEPHYR_TFLITE_MICRO_MODULE_DIR}/signal
  ${ZEPHYR_TFLITE_MICRO_MODULE_DIR}/signal/micro/kernels/
  ${ZEPHYR_TFLITE_MICRO_MODULE_DIR}/signal/src
  ${LIBMETAL_INCLUDE_DIR}
  ${OPENAMP_INCLUDE_DIR}
  ${PLATFORM_DIR}
)

target_sources(
  app
  PRIVATE
  ${app_sources}

  # Signal processing
  ${OPTIONAL_TFLITE_DIR}/src/fft_auto_scale.cc
  ${OPTIONAL_TFLITE_DIR}/src/max_abs.cc
  ${OPTIONAL_TFLITE_DIR}/src/msb_32.cc
  ${OPTIONAL_TFLITE_DIR}/src/energy.cc
  ${OPTIONAL_TFLITE_DIR}/src/filter_bank.cc
  ${OPTIONAL_TFLITE_DIR}/src/pcan_argc_fixed.cc
  ${OPTIONAL_TFLITE_DIR}/src/filter_bank_log.cc
  ${OPTIONAL_TFLITE_DIR}/src/log.cc
  ${OPTIONAL_TFLITE_DIR}/src/filter_bank_spectral_subtraction.cc

  # Micro kernels
  ${OPTIONAL_TFLITE_DIR}/micro/kernels/energy.cc
  ${OPTIONAL_TFLITE_DIR}/micro/kernels/fft_auto_scale_common.cc
  ${OPTIONAL_TFLITE_DIR}/micro/kernels/filter_bank_square_root_common.cc
  ${OPTIONAL_TFLITE_DIR}/micro/kernels/filter_bank_spectral_subtraction.cc
  ${OPTIONAL_TFLITE_DIR}/micro/kernels/filter_bank.cc
  ${OPTIONAL_TFLITE_DIR}/micro/kernels/pcan.cc
  ${OPTIONAL_TFLITE_DIR}/micro/kernels/filter_bank_log.cc
)

if("${ARCH}" STREQUAL "xtensa")
  target_sources(
  app
  PRIVATE

  # Xtensa kernels
  ${OPTIONAL_TFLITE_DIR}/micro/kernels/xtensa/fft_auto_scale_kernel.cc
  ${OPTIONAL_TFLITE_DIR}/micro/kernels/xtensa/filter_bank_square_root.cc
  ${OPTIONAL_TFLITE_DIR}/micro/kernels/xtensa/xtensa_square_root.S
  )
endif()
