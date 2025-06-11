# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

set_linker_property(NO_CREATE PROPERTY c_library    "-lc")
set_linker_property(NO_CREATE PROPERTY rt_library   "-lgcc")
set_linker_property(NO_CREATE PROPERTY c++_library  "-lstdc++")
set_linker_property(NO_CREATE PROPERTY hal_library  "-lhal")
if(CONFIG_XTENSA_LIBC)
  set_linker_property(NO_CREATE PROPERTY m_library   "-lm")
  set_linker_property(NO_CREATE PROPERTY sim_library "-lsim")
  set_linker_property(PROPERTY link_order_library "m;c;rt;sim;hal")
else()
  set_linker_property(PROPERTY link_order_library "c;rt;hal")
endif()
