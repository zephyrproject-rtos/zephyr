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
endif()
