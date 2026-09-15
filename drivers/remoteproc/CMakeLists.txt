# SPDX-License-Identifier: Apache-2.0

if(CONFIG_OPENAMP_RPMSG_VDEV OR CONFIG_OPENAMP_RESOURCE_TABLE)
  zephyr_library()
endif()

if(CONFIG_OPENAMP_RPMSG_VDEV)
  zephyr_library_sources(rpmsg_virtio_device.c)
endif()

if(CONFIG_OPENAMP_RESOURCE_TABLE)
  zephyr_library_sources(resource_table.c)
endif()
