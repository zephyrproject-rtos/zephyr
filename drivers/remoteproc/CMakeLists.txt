# SPDX-License-Identifier: Apache-2.0

if(CONFIG_REMOTEPROC)
  zephyr_library()
  zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
  zephyr_library_sources(remoteproc.c)
  zephyr_library_sources(rpmsg_virtio_device.c)
endif()
