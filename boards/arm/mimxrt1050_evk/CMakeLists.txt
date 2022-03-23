#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if (CONFIG_DISPLAY)
message(WARNING "
CONFIG_DISPLAY: Running this firmware on a board without a display may damage the board
")
endif()
