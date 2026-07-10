# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Add a BT serial device when building for bluetooth, unless the
# application explicitly opts out with NO_QEMU_SERIAL_BT_SERVER.

if(CONFIG_BT)
  if(NOT CONFIG_BT_UART)
    set(NO_QEMU_SERIAL_BT_SERVER 1)
  endif()
  if(NOT NO_QEMU_SERIAL_BT_SERVER)
    list(APPEND QEMU_FLAGS -serial unix:/tmp/bt-server-bredr)
  endif()
endif()
