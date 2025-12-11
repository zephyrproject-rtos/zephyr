# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# Linker flags for fixed linking with standard libraries, such as the C and runtime libraries.
# It is the responsibility of the linker infrastructure to use those properties to specify the
# correct placement of those libraries for correct link order.
# For example, GCC usually has the order: -lc -lgcc
# It is also possible to define extra libraries of the form `<name>_library`, and then include
# Fixed library search path can be defined in the `lib_include_dir` property if needed.
# <name> in the link_order_property.
# Usage example:
# set_linker_property(PROPERTY lib_include_dir "-L/path/to/libs")
# set_linker_property(PROPERTY c_library  "-lc")
# set_linker_property(PROPERTY rt_library "-lgcc")
# set_linker_property(PROPERTY link_order_library "c;rt")
