# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Ethernet NIC model and host netdev backend. The serial transports (SLIP, PPP)
# live in net_serial.cmake.
#
# If we are using a suitable ethernet driver inside qemu, then these options
# must be set, otherwise a zephyr instance cannot receive any network packets.
# The Qemu supported ethernet driver should define CONFIG_ETH_NIC_MODEL
# string that tells what nic model Qemu should use.

if(CONFIG_QEMU_TARGET)
  if((CONFIG_NET_QEMU_ETHERNET OR CONFIG_NET_QEMU_USER) AND NOT CONFIG_ETH_NIC_MODEL)
    message(FATAL_ERROR "
      No Qemu ethernet driver configured!
      Enable Qemu supported ethernet driver like e1000 in the device tree."
    )
  elseif(CONFIG_NET_QEMU_ETHERNET)
    if(CONFIG_ETH_QEMU_EXTRA_ARGS)
      set(NET_QEMU_ETH_EXTRA_ARGS ",${CONFIG_ETH_QEMU_EXTRA_ARGS}")
    endif()
    list(APPEND QEMU_EXTRA_FLAGS
      -netdev tap,id=n1,script=no,downscript=no,ifname=${CONFIG_ETH_QEMU_IFACE_NAME}${NET_QEMU_ETH_EXTRA_ARGS}
    )
  elseif(CONFIG_NET_QEMU_USER)
    list(APPEND QEMU_EXTRA_FLAGS
      -netdev user,id=n1,${CONFIG_NET_QEMU_USER_EXTRA_ARGS}
    )
  else()
    list(APPEND QEMU_EXTRA_FLAGS
      -net none
    )
  endif()
  if(CONFIG_NET_QEMU_ETHERNET OR CONFIG_NET_QEMU_USER)
    list(APPEND QEMU_EXTRA_FLAGS
      -device ${CONFIG_ETH_NIC_MODEL},netdev=n1,${CONFIG_NET_QEMU_DEVICE_EXTRA_ARGS}
    )
  endif()
endif()
