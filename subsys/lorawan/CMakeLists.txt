# SPDX-License-Identifier: Apache-2.0

# lorawan.c depends on the include directories exposed by the loramac-node
# library. Hence, if it exists then the source files are added to that otherwise
# a library with same name is created.
if(TARGET loramac-node)
	set(ZEPHYR_CURRENT_LIBRARY loramac-node)
else()
	zephyr_library_named(loramac-node)
endif()

zephyr_compile_definitions_ifdef(CONFIG_LORAMAC_REGION_AS923 REGION_AS923)
zephyr_compile_definitions_ifdef(CONFIG_LORAMAC_REGION_AU915 REGION_AU915)
zephyr_compile_definitions_ifdef(CONFIG_LORAMAC_REGION_CN470 REGION_CN470)
zephyr_compile_definitions_ifdef(CONFIG_LORAMAC_REGION_CN779 REGION_CN779)
zephyr_compile_definitions_ifdef(CONFIG_LORAMAC_REGION_EU433 REGION_EU433)
zephyr_compile_definitions_ifdef(CONFIG_LORAMAC_REGION_EU868 REGION_EU868)
zephyr_compile_definitions_ifdef(CONFIG_LORAMAC_REGION_KR920 REGION_KR920)
zephyr_compile_definitions_ifdef(CONFIG_LORAMAC_REGION_IN865 REGION_IN865)
zephyr_compile_definitions_ifdef(CONFIG_LORAMAC_REGION_US915 REGION_US915)
zephyr_compile_definitions_ifdef(CONFIG_LORAMAC_REGION_RU864 REGION_RU864)

zephyr_library_sources_ifdef(CONFIG_LORAWAN lorawan.c)
zephyr_library_sources_ifdef(CONFIG_LORAWAN lw_priv.c)
