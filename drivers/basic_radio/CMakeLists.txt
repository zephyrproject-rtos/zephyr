# SPDX-License-Identifier: Apache-2.0

# Each vendor backend registers its own zephyr_library() inside its subdirectory.
add_subdirectory_ifdef(CONFIG_BASIC_RADIO_CC23X0 backends/ti_cc23x0)
