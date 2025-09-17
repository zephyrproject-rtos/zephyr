# Copyright 2024 Université Gustave Eiffel
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_RK3568)
  add_subdirectory(rk3568)
elseif(CONFIG_SOC_RK3588S)
  add_subdirectory(rk3588s)
elseif(CONFIG_SOC_RK3588)
  add_subdirectory(rk3588)
endif()
