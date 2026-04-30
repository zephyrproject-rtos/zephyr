# Copyright (c) 2026 Sylvain Mosnier
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MECH)
  zephyr_library()
  zephyr_library_sources(mech.c)
  zephyr_library_sources_ifdef(CONFIG_MECH_INERTIAL      inertial.c)
  zephyr_library_sources_ifdef(CONFIG_MECH_TRANSMISSION  transmission.c)
endif()
