#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_basic.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_data.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_multicontext.c)

target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/test_multicontext_handler.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_oom.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_subs.c)
