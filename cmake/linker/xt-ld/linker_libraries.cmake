# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

set_linker_property(NO_CREATE PROPERTY c_library    "-lc")
set_linker_property(NO_CREATE PROPERTY rt_library   "-lgcc")
set_linker_property(NO_CREATE PROPERTY c++_library  "-lstdc++")
set_linker_property(NO_CREATE PROPERTY hal_library  "-lhal")
set_linker_property(PROPERTY link_order_library "c;rt;hal")
