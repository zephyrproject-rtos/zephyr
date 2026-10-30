# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

set_linker_property(NO_CREATE PROPERTY c_library    "-lc")
set_linker_property(NO_CREATE PROPERTY rt_library   "-lgcc")
set_linker_property(NO_CREATE PROPERTY c++_library  "-lstdc++")
set_linker_property(NO_CREATE PROPERTY hal_library  "-lhal")

if(CONFIG_XTENSA_LIBC)
  set_linker_property(NO_CREATE PROPERTY m_library "-lm")
  set_linker_property(APPEND PROPERTY link_order_library "m")
endif()

set_linker_property(APPEND PROPERTY link_order_library "c;rt")

if(CONFIG_SIMULATOR_XTENSA)
  set_linker_property(NO_CREATE PROPERTY sim_library "-lsim")
  set_linker_property(APPEND PROPERTY link_order_library "sim")
endif()

set_linker_property(APPEND PROPERTY link_order_library "hal")
