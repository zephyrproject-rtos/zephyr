# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(basic_radio_common.c)

# Each vendor backend registers its own zephyr_library() inside its subdirectory.
add_subdirectory_ifdef(CONFIG_BASIC_RADIO_CC23X0 backends/ti/cc23x0)
