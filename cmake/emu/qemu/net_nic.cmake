# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Ethernet NIC model and host netdev backend. The serial transports (SLIP, PPP)
# live in net_serial.cmake.
#
# If we are using a suitable ethernet driver inside qemu, then these options
# must be set, otherwise a zephyr instance cannot receive any network packets.
# The Qemu supported ethernet driver should define CONFIG_ETH_NIC_MODEL
# string that tells what nic model Qemu should use.

if((CONFIG_NET_QEMU_ETHERNET OR CONFIG_NET_QEMU_USER) AND NOT CONFIG_ETH_NIC_MODEL)
  message(FATAL_ERROR "
    No Qemu ethernet driver configured!
    Enable Qemu supported ethernet driver like e1000 in the device tree."
  )
elseif(CONFIG_NET_QEMU_ETHERNET)
  if(CONFIG_ETH_QEMU_EXTRA_ARGS)
    set(NET_QEMU_ETH_EXTRA_ARGS ",${CONFIG_ETH_QEMU_EXTRA_ARGS}")
  endif()
  if(CMAKE_HOST_APPLE)
    # macOS has no tuntap device, so QEMU connects through the vmnet
    # framework in host mode. vmnet creates the host side interface itself,
    # so there is no interface name to pass. A network identifier disables
    # vmnet's DHCP server together with its RFC 1918 address range, so the
    # host interface starts without an address and any subnet can be
    # assigned to it, as with the TAP interface on Linux.
    #
    # Each build directory gets its own identifier, generated once and kept
    # in the cache, so instances of different builds are on separate
    # networks. Pass the same NET_QEMU_VMNET_NET_UUID to several builds to
    # put them on one network.
    if(NOT DEFINED NET_QEMU_VMNET_NET_UUID)
      string(RANDOM LENGTH 32 seed)
      # A name-based UUID (RFC 4122 version 5) of the random seed, so the
      # result is well-formed and vmnet accepts it.
      string(UUID uuid
        NAMESPACE 6ba7b810-9dad-11d1-80b4-00c04fd430c8 NAME "${seed}" TYPE SHA1
      )
      set(NET_QEMU_VMNET_NET_UUID "${uuid}" CACHE STRING
        "vmnet network identifier of the QEMU ethernet backend"
      )
    endif()
    set(NET_QEMU_NETDEV
      vmnet-host,id=n1,net-uuid=${NET_QEMU_VMNET_NET_UUID}${NET_QEMU_ETH_EXTRA_ARGS}
    )
  else()
    set(NET_QEMU_NETDEV
      tap,id=n1,script=no,downscript=no,ifname=${CONFIG_ETH_QEMU_IFACE_NAME}${NET_QEMU_ETH_EXTRA_ARGS}
    )
  endif()
elseif(CONFIG_NET_QEMU_USER)
  if(CONFIG_NET_QEMU_USER_EXTRA_ARGS)
    set(NET_QEMU_USER_EXTRA_ARGS ",${CONFIG_NET_QEMU_USER_EXTRA_ARGS}")
  endif()
  set(NET_QEMU_NETDEV user,id=n1${NET_QEMU_USER_EXTRA_ARGS})
else()
  qemu_append_extra_flags(
    -net none
  )
endif()
if(CONFIG_NET_QEMU_ETHERNET OR CONFIG_NET_QEMU_USER)
  if(CONFIG_NET_QEMU_DEVICE_EXTRA_ARGS)
    set(NET_QEMU_DEVICE_EXTRA_ARGS ",${CONFIG_NET_QEMU_DEVICE_EXTRA_ARGS}")
  endif()
  if(CONFIG_ETH_NIC_MODEL_ONBOARD)
    # A NIC built into the emulated machine cannot be created with -device,
    # so configure it together with its backend using -nic instead.
    qemu_append_extra_flags(
      -nic ${NET_QEMU_NETDEV},model=${CONFIG_ETH_NIC_MODEL}${NET_QEMU_DEVICE_EXTRA_ARGS}
    )
  else()
    qemu_append_extra_flags(
      -netdev ${NET_QEMU_NETDEV}
      -device ${CONFIG_ETH_NIC_MODEL},netdev=n1${NET_QEMU_DEVICE_EXTRA_ARGS}
    )
  endif()

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
