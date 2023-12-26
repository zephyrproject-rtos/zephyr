# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED BOARD_REVISION)
  return()
elseif(${BOARD_REVISION} STREQUAL "usb")
  return()
else()
  message(FATAL_ERROR "Board revision `${BOARD_REVISION}` for board \
          `${BOARD}` not found. Please specify a valid board revision.")
endif()
