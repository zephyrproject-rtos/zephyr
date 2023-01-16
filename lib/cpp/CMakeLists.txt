# SPDX-License-Identifier: Apache-2.0

add_subdirectory(abi)

add_subdirectory_ifdef(CONFIG_MINIMAL_LIBCPP minimal)
