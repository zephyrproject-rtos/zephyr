# SPDX-License-Identifier: Apache-2.0

zephyr_library()
if(CONFIG_NET_DSA_DEPRECATED)
zephyr_library_sources(dsa.c)
endif()
