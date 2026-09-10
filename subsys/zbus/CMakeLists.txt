# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(zbus.c)

if(CONFIG_ZBUS_RUNTIME_OBSERVERS)
  zephyr_library_sources(zbus_runtime_observers.c)
endif()

add_subdirectory_ifdef(CONFIG_ZBUS_PROXY_AGENT proxy_agent)

zephyr_library_sources(zbus_iterable_sections.c)
