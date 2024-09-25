# Copyright (c) 2023 Meta Platforms
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CMSIS_NN)

  set(CMSIS_NN_DIR ${ZEPHYR_CURRENT_MODULE_DIR})
  set(cmsis_glue_path ${ZEPHYR_CMSIS_MODULE_DIR})

  zephyr_library()

  zephyr_library_compile_options(-Ofast)

  zephyr_include_directories(${CMSIS_NN_DIR}/Include)

  zephyr_library_include_directories(${cmsis_glue_path}/CMSIS/Core/Include)

  if(CONFIG_CMSIS_NN_ACTIVATION)
    file(GLOB SRC "${CMSIS_NN_DIR}/Source/ActivationFunctions/*_s8*.c")
    file(GLOB SRC_S16 "${CMSIS_NN_DIR}/Source/ActivationFunctions/*_s16*.c")
    zephyr_library_sources(${SRC} ${SRC_S16}
      ${CMSIS_NN_DIR}/Source/ActivationFunctions/arm_relu_q7.c
      ${CMSIS_NN_DIR}/Source/ActivationFunctions/arm_relu_q15.c)
  endif()

  if(CONFIG_CMSIS_NN_BASICMATH)
    file(GLOB SRC "${CMSIS_NN_DIR}/Source/BasicMathFunctions/*_*.c")
    zephyr_library_sources(${SRC})
  endif()

  if(CONFIG_CMSIS_NN_CONCATENATION)
    file(GLOB SRC "${CMSIS_NN_DIR}/Source/ConcatenationFunctions/*_*.c")
    zephyr_library_sources(${SRC})
  endif()

  if(CONFIG_CMSIS_NN_CONVOLUTION)
    file(GLOB SRC_S4 "${CMSIS_NN_DIR}/Source/ConvolutionFunctions/*_s4*.c")
    file(GLOB SRC_S8 "${CMSIS_NN_DIR}/Source/ConvolutionFunctions/*_s8*.c")
    file(GLOB SRC_S16 "${CMSIS_NN_DIR}/Source/ConvolutionFunctions/*_s16*.c")
    zephyr_library_sources(${SRC_S4} ${SRC_S8} ${SRC_S16})
  endif()

  if(CONFIG_CMSIS_NN_FULLYCONNECTED)
    file(GLOB SRC_S4 "${CMSIS_NN_DIR}/Source/FullyConnectedFunctions/*_s4.c")
    file(GLOB SRC_S8 "${CMSIS_NN_DIR}/Source/FullyConnectedFunctions/*_s8.c")
    file(GLOB SRC_S16 "${CMSIS_NN_DIR}/Source/FullyConnectedFunctions/*_s16*.c")
    zephyr_library_sources(${SRC_S4} ${SRC_S8} ${SRC_S16})
  endif()

  if(CONFIG_CMSIS_NN_NNSUPPORT)
    file(GLOB SRC_S4 "${CMSIS_NN_DIR}/Source/NNSupportFunctions/*_s4*.c")
    file(GLOB SRC_S8 "${CMSIS_NN_DIR}/Source/NNSupportFunctions/*_s8*.c")
    file(GLOB SRC_S16 "${CMSIS_NN_DIR}/Source/NNSupportFunctions/*_s16*.c")
    zephyr_library_sources(${SRC_S4} ${SRC_S8} ${SRC_S16}
      ${CMSIS_NN_DIR}/Source/NNSupportFunctions/arm_nntables.c
      ${CMSIS_NN_DIR}/Source/NNSupportFunctions/arm_q7_to_q15_with_offset.c
      ${CMSIS_NN_DIR}/Source/NNSupportFunctions/arm_s8_to_s16_unordered_with_offset.c)
  endif()

  if(CONFIG_CMSIS_NN_POOLING)
    file(GLOB SRC "${CMSIS_NN_DIR}/Source/PoolingFunctions/*_s8.c")
    file(GLOB SRC_S16 "${CMSIS_NN_DIR}/Source/PoolingFunctions/*_s16.c")
    zephyr_library_sources(${SRC} ${SRC_S16})
  endif()

  if(CONFIG_CMSIS_NN_RESHAPE)
    file(GLOB SRC "${CMSIS_NN_DIR}/Source/ReshapeFunctions/*_*.c")
    zephyr_library_sources(${SRC})
  endif()

  if(CONFIG_CMSIS_NN_SOFTMAX)
    file(GLOB SRC "${CMSIS_NN_DIR}/Source/SoftmaxFunctions/*_s8.c")
    zephyr_library_sources(${SRC}
      ${CMSIS_NN_DIR}/Source/SoftmaxFunctions/arm_softmax_s8_s16.c
      ${CMSIS_NN_DIR}/Source/SoftmaxFunctions/arm_softmax_s16.c
      ${CMSIS_NN_DIR}/Source/SoftmaxFunctions/arm_nn_softmax_common_s8.c)
  endif()

  if(CONFIG_CMSIS_NN_SVD)
    file(GLOB SRC "${CMSIS_NN_DIR}/Source/SVDFunctions/*_s8.c")
    zephyr_library_sources(${SRC})
  endif()

  if(CONFIG_CMSIS_NN_LSTM)
    file(GLOB SRC "${CMSIS_NN_DIR}/Source/LSTMFunctions/*_s16.c")
    zephyr_library_sources(${SRC})
  endif()

endif()
