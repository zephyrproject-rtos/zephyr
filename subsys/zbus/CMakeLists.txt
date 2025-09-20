# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(zbus.c)

if(CONFIG_ZBUS_RUNTIME_OBSERVERS)
    zephyr_library_sources(zbus_runtime_observers.c)
endif()

if(CONFIG_ZBUS_MULTIDOMAIN)
    zephyr_library_sources(multidomain/zbus_multidomain.c)
endif()

if(CONFIG_ZBUS_MULTIDOMAIN_UART)
    zephyr_library_sources(multidomain/zbus_multidomain_uart.c)
endif()

if(CONFIG_ZBUS_MULTIDOMAIN_IPC)
    zephyr_library_sources(multidomain/zbus_multidomain_ipc.c)
endif()

zephyr_library_sources(zbus_iterable_sections.c)
