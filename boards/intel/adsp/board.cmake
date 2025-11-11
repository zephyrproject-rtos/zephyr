# Copyright (c) 2022-2025 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_INTEL_ADSP_SIM)
  set(SUPPORTED_EMU_PLATFORMS acesim)
endif()

board_set_rimage_target(${CONFIG_RIMAGE_TARGET})
board_set_flasher_ifnset(intel_adsp)

if(DEFINED ENV{RIMAGE_SIGN_KEY})
  set(RIMAGE_SIGN_KEY $ENV{RIMAGE_SIGN_KEY} CACHE STRING "default key via env")
else()
  set(RIMAGE_SIGN_KEY "otc_private_key_3k.pem" CACHE STRING "default key")
endif()

board_finalize_runner_args(intel_adsp)
