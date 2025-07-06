# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

zephyr_get(SB_SNIPPET)
if(NOT SB_SNIPPET AND SNIPPET)
  set_ifndef(SB_SNIPPET ${SNIPPET})
endif()
set(SNIPPET_NAMESPACE SB_SNIPPET)
set(SNIPPET_PYTHON_EXTRA_ARGS --sysbuild)
set(SNIPPET_APP_DIR ${APP_DIR})
include(${ZEPHYR_BASE}/cmake/modules/snippets.cmake)

set(SNIPPET_NAMESPACE)
set(SNIPPET_PYTHON_EXTRA_ARGS)
set(SNIPPET_APP_DIR)
