# SPDX-License-Identifier: Apache-2.0

zephyr_library()
if(CONFIG_NET_DSA_LEGACY)
zephyr_library_sources(dsa_legacy.c)
endif()
zephyr_library_sources(dsa.c)
zephyr_library_sources(switch.c)
zephyr_library_sources(port.c)
zephyr_library_sources(slave.c)
zephyr_library_sources(tag.c)
