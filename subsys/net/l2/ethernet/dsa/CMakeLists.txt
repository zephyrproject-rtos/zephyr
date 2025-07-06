# SPDX-License-Identifier: Apache-2.0

zephyr_library()
if(CONFIG_NET_DSA_DEPRECATED)
zephyr_library_sources(dsa.c)
else()
zephyr_library_sources(dsa_core.c)
zephyr_library_sources(dsa_port.c)
zephyr_library_sources(dsa_user.c)
zephyr_library_sources(dsa_tag.c)
endif()
