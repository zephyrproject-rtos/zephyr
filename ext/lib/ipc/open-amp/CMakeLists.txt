#
# Copyright (c) 2018 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

set(WITH_ZEPHYR 1)
set(WITH_ZEPHYR_LIB 1)
set(WITH_LIBMETAL_FIND OFF CACHE BOOL "" FORCE)
set(LIBMETAL_INCLUDE_DIR ${ZEPHYR_BINARY_DIR}/ext/hal/libmetal/lib/include)
set(LIBMETAL_LIB ${ZEPHYR_BINARY_DIR}/ext/hal/libmetal/lib)

add_subdirectory(${CONFIG_OPENAMP_SRC_PATH} open-amp)
