# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PM)
  zephyr_library_sources(policy_events.c policy_latency.c policy_state_lock.c)

  if(CONFIG_PM_POLICY_DEVICE_CONSTRAINTS)
    zephyr_library_sources(policy_device_lock.c)
  endif()

  if(CONFIG_PM_POLICY_DEFAULT)
    zephyr_library_sources(policy_default.c)
  endif()
endif()
