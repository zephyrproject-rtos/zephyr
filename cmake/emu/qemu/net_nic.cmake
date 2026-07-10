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
    qemu_append_extra_flags(
      -netdev tap,id=n1,script=no,downscript=no,ifname=${CONFIG_ETH_QEMU_IFACE_NAME}${NET_QEMU_ETH_EXTRA_ARGS}
    )
  elseif(CONFIG_NET_QEMU_USER)
    qemu_append_extra_flags(
      -netdev user,id=n1,${CONFIG_NET_QEMU_USER_EXTRA_ARGS}
    )
  else()
    qemu_append_extra_flags(
      -net none
    )
  endif()
  if(CONFIG_NET_QEMU_ETHERNET OR CONFIG_NET_QEMU_USER)
    qemu_append_extra_flags(
      -device ${CONFIG_ETH_NIC_MODEL},netdev=n1,${CONFIG_NET_QEMU_DEVICE_EXTRA_ARGS}
    )

    # Capture the traffic on the host side of the NIC. QEMU writes the pcap
    # itself, so unlike the PCAP support for the serial transports this needs
    # no FIFOs and no external capture process.
    #
    # NET_QEMU_NETWORKING is a Kconfig choice, so at most one of the ethernet
    # and serial transports is active and PCAP is unambiguous.
    if(PCAP)
      qemu_append_extra_flags(
        -object filter-dump,id=pcap0,netdev=n1,file=${PCAP}
      )
    endif()
  endif()
endif()
