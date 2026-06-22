# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# Per default armclang (Arm Compiler 6) doesn't need explicit C library linking
# so we only need to set link order linking in case a custom C library is linked
# in, such as picolibc.
set_property(TARGET linker APPEND PROPERTY link_order_library "c;rt")
