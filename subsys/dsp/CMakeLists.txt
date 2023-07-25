# Copyright (c) 2022 Google LLC
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_DSP_BACKEND_CMSIS cmsis)
add_subdirectory_ifdef(CONFIG_DSP_BACKEND_ARCMWDT arcmwdt)
