#
# Copyright (c) 2018 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

set(WITH_ZEPHYR 1)
set(WITH_ZEPHYR_LIB 1)
set(WITH_DOC OFF CACHE BOOL "" FORCE)
set(WITH_DEFAULT_LOGGER OFF CACHE BOOL "" FORCE)

add_subdirectory(${CONFIG_LIBMETAL_SRC_PATH} libmetal)
