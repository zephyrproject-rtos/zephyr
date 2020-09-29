# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PINMUX)
zephyr_library()
zephyr_library_sources(pinmux.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
endif()

if (CONFIG_BUILD_WITH_TFM)
	set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
		#Execute post build script postbuild.sh
		COMMAND ${CMAKE_BINARY_DIR}/tfm/install/postbuild.sh
      )
endif()
