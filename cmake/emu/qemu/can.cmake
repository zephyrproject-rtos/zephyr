# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Emulated CAN bus, optionally bridged to a host SocketCAN interface.

if(CONFIG_CAN_KVASER_PCI)
  # Add CAN bus 0 only when QEMU-emulated CAN hardware is actually needed
  list(APPEND QEMU_FLAGS -object can-bus,id=canbus0)

  if(NOT "${CONFIG_CAN_QEMU_IFACE_NAME}" STREQUAL "")
    # Connect CAN bus 0 to host SocketCAN interface
    list(APPEND QEMU_FLAGS
      -object can-host-socketcan,id=canhost0,if=${CONFIG_CAN_QEMU_IFACE_NAME},canbus=canbus0
    )
  endif()

  # Emulate a single-channel Kvaser PCIcan card connected to CAN bus 0
  list(APPEND QEMU_FLAGS -device kvaser_pci,canbus=canbus0)
endif()
