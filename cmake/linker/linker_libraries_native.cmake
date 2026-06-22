# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# When doing native builds, then we default to host libraries.
# No reason for loading linker libraries properties in this case, however we do
# define link order because that allows the build system to hook in alternative
# C library implementations, such as minimal libc or picolibc.

# Empty on purpose as we default to host libraries selected by the linker.
set_linker_property(PROPERTY c_library "")
set_linker_property(PROPERTY rt_library "")
set_linker_property(PROPERTY c++_library "")

# Although library properties are empty per default, then we still define link
# order as this allows to update libraries in use elsewhere.
if(CONFIG_CPP)
  set_property(TARGET linker PROPERTY link_order_library "c++")
endif()

set_property(TARGET linker APPEND PROPERTY link_order_library "c;rt")
