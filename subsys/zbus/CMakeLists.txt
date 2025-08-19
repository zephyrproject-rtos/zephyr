# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(zbus.c)

if(CONFIG_ZBUS_RUNTIME_OBSERVERS)
  zephyr_library_sources(zbus_runtime_observers.c)
endif()

if(CONFIG_ZBUS_PROXY_AGENT)
  add_subdirectory(proxy_agent)
endif()

zephyr_library_sources(zbus_iterable_sections.c)
