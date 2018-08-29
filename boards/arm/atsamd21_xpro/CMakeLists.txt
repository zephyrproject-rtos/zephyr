# Copyright (c) 2018 Bryan O'Donoghue
# SPDX-License-Identifier: Apache-2.0
if(CONFIG_PINMUX_SAM0)
  target_sources(board PRIVATE ${CMAKE_CURRENT_LIST_DIR}/pinmux.c)
endif()
