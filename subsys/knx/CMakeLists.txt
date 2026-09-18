# Copyright (c) 2026 Fabien Proriol
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories_ifdef(CONFIG_KNX_STACK .)

zephyr_sources_ifdef(CONFIG_KNX_STACK
  knx_core.c
  layer1_physical.c
  layer2_data_link.c
  layer3_network.c
  layer4_transport.c
  layer7_application.c
  object_device.c
  object_address_table.c
  object_association_table.c
  object_application_program.c
  object_interface_program.c
  object_group_table.c
  object_interface.c
  object_memory.c
  )

zephyr_sources_ifdef(CONFIG_KNX_DPT dpt.c)
zephyr_sources_ifdef(CONFIG_KNX_DPT knx_group_object.c)
