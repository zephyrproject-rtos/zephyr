#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_MPIPE_NN)
  zephyr_library_sources_ifdef(CONFIG_MPIPE_NN_CONVERTER mpipe_nn_converter.c)
  zephyr_library_sources_ifdef(CONFIG_MPIPE_NN_RUNNER mpipe_nn_runner.c)
  zephyr_library_sources_ifdef(CONFIG_MPIPE_NN_DECODER mpipe_nn_decoder.c mpipe_nn_results.c)
  zephyr_library_sources_ifdef(CONFIG_MPIPE_NN_BACKEND_FAKE mpipe_nn_backend_fake.c)

  if(CONFIG_MPIPE_NN_BACKEND_TFLM)
    # The TFLM backend is C++; a separate library keeps the C++ compile
    # options away from the C-only "mpipe" library. This block must stay last:
    # zephyr_library_named() retargets subsequent zephyr_library_* calls.
    zephyr_library_named(mpipe_nn_tflm)
    zephyr_library_sources(mpipe_nn_backend_tflm.cpp)
    set(NO_THREADSAFE_STATICS $<TARGET_PROPERTY:compiler-cpp,no_threadsafe_statics>)
    zephyr_library_compile_options($<$<COMPILE_LANGUAGE:CXX>:${NO_THREADSAFE_STATICS}>)
    # The Neutron NPU kernel of hal_nxp, for the custom operator the backend registers
    zephyr_library_link_libraries_ifdef(CONFIG_TFLM_NXP_NEUTRON tflm_nxp_neutron)
  endif()
endif()
